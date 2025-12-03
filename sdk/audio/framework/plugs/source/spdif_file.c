#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spdif_file.data.bss")
#pragma data_seg(".spdif_file.data")
#pragma const_seg(".spdif_file.text.const")
#pragma code_seg(".spdif_file.text")
#endif
#include "source_node.h"
#include "media/audio_splicing.h"
#include "audio_config.h"
#include "jlstream.h"
#include "spdif_file.h"
#include "app_config.h"
#include "effects/effects_adj.h"
#include "media/audio_general.h"


#if TCFG_SPDIF_ENABLE
#include "audio_dai/audio_spdif_slave.h"

#define US_PER_SECOND   (1000000L)
#define MS_PER_SECOND   (1000L)
static const u8 spdif_analog_port[] = {
    IO_PORTA_06,
    IO_PORTA_08,
    IO_PORTC_00,
    IO_PORTC_02,
};

/* SPDIF淡入 */
#define SPDIF_FADE_IN_EN    1		//spdif 淡入使能
/* #define SPDIF_FADE_IN_STEP2  0.01f	//24bit spdif淡入的步进 */
#define SPDIF_FADE_IN_STEP  0.01f	//spdif淡入的步进

/* SPDIF检错机制采样率的容错宽度 */
#define SPDIF_SR_ERROR_TOLERANT_WIDTH  100

/* SPDIF 一开始丢帧的数量 */
#define SPDIF_DROP_FRAME_CNT		400

/* SPDIF ONLINE 检测使能 */
#define SPDIF_ONLINE_DET_EN			1
#define SPDIF_ONLINE_DET_TIME_MS	500

/*
 * 针对采样率的特殊情况处理：
 * 1.当输入为16k时，spdif_get_sr 值一成不变，而 spdif_slave_get_sr 标准还是16k左右
 * 2.当输入为12k时，spdif_slave_get_sr 在12k左右
 * 3.当输入为11025时，spdif_slave_get_sr 在11025左右
 * 4.当输入为8000时，spdif_slave_get_sr 在8k左右
 */
enum spdif_special_case {
    SPDIF_SPECIAL_NONE = 0,
    SPDIF_SPECIAL_16000,
    SPDIF_SPECIAL_12000,
    SPDIF_SPECIAL_11025,
    SPDIF_SPECIAL_8000,
};

struct spdif_file_hdl {
    SPDIF_SLAVE_PARM spdif_slave_parm;
    void *source_node;
    struct stream_node *node;
    enum audio_spdif_state_enum state;
    u32 sample_rate_set;	//记录下最终设置的采样率
    u32 coding_type;
    u32 channel_mode;
    u32 spdif_set_data_dma_len;	//记录下spdif每次设置的data_dma_len，用于决定是否需要重启
    u32 sw_real_time_sr;		//记录下软件实时的采样率大小
    u32 last_irq_timestamp;
#if SPDIF_FADE_IN_EN
    float fade_in_coefficient;	//淡入系数
    u8 new_frame_can_fade_in;
#endif
#if SPDIF_ONLINE_DET_EN
    u32 irq_running_cnt;		//该值用来判断当前spdif是否有起中断，当该值不为0时，说明spdif起中断往后面推数
    u16 online_det_timer_id;	//检测spdif是否在线的定时器id
#endif
    u16 drop_frame_cnt;		//驱动去掉了帧错误重启操作，需要先丢掉几帧
    u16 hdmi_wait_cec_drop_packet_cnt;	//hdmi 等待cec通信成功的丢包数量
    u8 format_correct_cnt;
    u8 input_channel_num;
    u8 start;
    u8 have_data;	//该变量用来表示是否有接收到数据
    u8 special_idx;			//该值非0 表示采样率发生了特殊情况
    u8 need_restart_flag;	//需要重启数据流的标志，该值为0表示无特殊情况发生
    u8 need_fade_in_flag;	//需要进行淡入的标志
    u8 online_flag;			//标记spdif是否在线, 1表示在线
    u8 is_hdmi_source;		//当前源是否是HDMI源的标记
    u8 hdmi_stop_push_data;	//决定spdif HDMI是否可以往后面推数, 改变量由app层进行控制
    u8 Qval;//数据的饱和位宽
};
struct spdif_file_hdl *spdif_file_t = NULL;

static DEFINE_SPINLOCK(spdif_lock);

struct spdif_file_cfg spdif_cfg;	//spdif的配置参数信息

/* 记录 spdif 数据流格式信息 */
struct spdif_format_t {
    u8 init;
    u8 get_fmt_complete;	//只有该值为1才能打开spdif player
    u8 is_pcm;
    u32 coding_type;
    u32 channel_mode;
    u32 sample_rate;
    u32 last_sample_rate;	// 用来决定是否是采样率发生改变，决定是否需要置上重启的标志
    u32 spdif_data_dma_len;
    u32 spdif_inf_dma_len;
};
struct spdif_format_t spdif_format = {
    .spdif_data_dma_len = SPDIF_DATA_DMA_LEN,
    .spdif_inf_dma_len = SPDIF_INF_DMA_LEN,
};

/* 保存spdif IO配置信息 */
struct spdif_port_config_t {
    u8 port;
    u8 port_mode;
    u8 init;
};
struct spdif_port_config_t spdif_port_config = {0};


struct spdif_file_cfg *audio_spdif_file_get_cfg(void)
{
    return &spdif_cfg;
}

int get_hdmi_cec_io(void)
{
    return uuid2gpio(spdif_cfg.cec_io_port);
}
u8 get_spdif_analog_port_index(u8 port)
{
    for (int i = 0; i < sizeof(spdif_analog_port); i++) {
        if (spdif_analog_port[i] == port) {
            printf("analog port index[%d]", i);
            return i;
        }
    }
    return 0xff;
}
void audio_spdif_file_init(void)
{
    /*
     *解析配置文件内效果配置
     * */
    int rlen = syscfg_read(CFG_SPDIF_CFG_ID, &spdif_cfg, sizeof(struct spdif_file_cfg) - 8);
    if (rlen) {
        printf("hdmi port%x;dport[%x] \n", uuid2gpio(spdif_cfg.hdmi_port[0]), uuid2gpio(spdif_cfg.hdmi_port[1]));
        printf("cec io [%x]", uuid2gpio(spdif_cfg.cec_io_port));
        printf("iis scl porta [%x]", uuid2gpio(spdif_cfg.iic_scl_port));
        printf("iis sda porta [%x]", uuid2gpio(spdif_cfg.iic_sda_port));

        printf("iic num %x;hdmi det mode[%x] \n", spdif_cfg.iic_num, spdif_cfg.hdmi_det_mode);
        printf("hdmi det port [%x]", uuid2gpio(spdif_cfg.hdmi_det_port));

        rlen = syscfg_read(CFG_OPT_CFG_ID, &spdif_cfg.opt_port[0], 4);
        if (rlen) {
            printf("opt port%x;dport[%x] \n", uuid2gpio(spdif_cfg.opt_port[0]), uuid2gpio(spdif_cfg.opt_port[1]));
        } else {
            printf("read opt cfg err \n");
        }
        rlen = syscfg_read(CFG_COAL_CFG_ID, &spdif_cfg.coal_port[0], 4);
        if (rlen) {
            printf("caol port%x;dport[%x] \n", uuid2gpio(spdif_cfg.coal_port[0]), uuid2gpio(spdif_cfg.coal_port[1]));
        } else {
            printf("read coal cfg err \n");
        }
        //设置默认输入源
        if ((uuid2gpio(spdif_cfg.hdmi_port[0]) != 0xff)
            || (uuid2gpio(spdif_cfg.hdmi_port[1]) != 0xff)) {
            if (uuid2gpio(spdif_cfg.hdmi_port[0]) != 0xff) {
                spdif_port_config.port = uuid2gpio(spdif_cfg.hdmi_port[0]);
                spdif_port_config.port_mode = get_spdif_analog_port_index(spdif_port_config.port);
                if (spdif_port_config.port_mode != 0xff) {
                    spdif_port_config.port_mode += SPDIF_IN_ANALOG_PORT_A;
                } else {
                    spdif_port_config.port_mode = SPDIF_IN_DIGITAL_PORT_A;
                }
            } else {
                spdif_port_config.port = uuid2gpio(spdif_cfg.hdmi_port[1]);
                spdif_port_config.port_mode = SPDIF_IN_DIGITAL_PORT_A;
            }
            spdif_port_config.init = 1;
        } else {
            if ((uuid2gpio(spdif_cfg.opt_port[0]) != 0xff)
                || (uuid2gpio(spdif_cfg.opt_port[1]) != 0xff)) {
                if (uuid2gpio(spdif_cfg.opt_port[0]) != 0xff) {
                    spdif_port_config.port = uuid2gpio(spdif_cfg.opt_port[0]);
                    spdif_port_config.port_mode = get_spdif_analog_port_index(spdif_port_config.port);
                    if (spdif_port_config.port_mode != 0xff) {
                        spdif_port_config.port_mode += SPDIF_IN_ANALOG_PORT_A;
                    } else {
                        spdif_port_config.port_mode = SPDIF_IN_DIGITAL_PORT_A;
                    }
                } else {
                    spdif_port_config.port = uuid2gpio(spdif_cfg.opt_port[1]);
                    spdif_port_config.port_mode = SPDIF_IN_DIGITAL_PORT_A;
                }
                spdif_port_config.init = 1;

            } else {
                if (uuid2gpio(spdif_cfg.coal_port[0]) != 0xff) {
                    spdif_port_config.port = uuid2gpio(spdif_cfg.coal_port[0]);
                    spdif_port_config.port_mode = get_spdif_analog_port_index(spdif_port_config.port);
                    if (spdif_port_config.port_mode != 0xff) {
                        spdif_port_config.port_mode += SPDIF_IN_ANALOG_PORT_A;
                    } else {
                        spdif_port_config.port_mode = SPDIF_IN_DIGITAL_PORT_A;
                    }
                    spdif_port_config.init = 1;
                } else if (uuid2gpio(spdif_cfg.coal_port[1]) != 0xff) {
                    spdif_port_config.port = uuid2gpio(spdif_cfg.opt_port[1]);
                    spdif_port_config.port_mode = SPDIF_IN_DIGITAL_PORT_A;
                    spdif_port_config.init = 1;
                }
            }
        }
    } else {
    }
}

/*
 * 获取当前输入源是否是HDMI
 * 返回值：1：当前输入源是HDMI, 0：当前输入源不是HDMI, -1：获取失败
 */
int spdif_is_hdmi_source(void)
{
    struct spdif_file_hdl *hdl = spdif_file_t;
    if (hdl) {
        return hdl->is_hdmi_source;
    }
    return -1;
}

/*
 * 如果当前为HDMI数据源时，需等CEC通信后数据稳定了，由外部控制是否往后面推数
 */
void spdif_hdmi_set_push_data_en(u8 en)
{
    struct spdif_file_hdl *hdl = spdif_file_t;
    if (hdl && hdl->is_hdmi_source == 1) {
        hdl->hdmi_stop_push_data = !en;
    }
}


/*
 * 获取 spdif 输入源 IO
 * 返回 0xff 表示获取失败
 */
u8 get_spdif_source_io(void)
{
    u8 source_port = 0xff;
    struct spdif_file_hdl *hdl = spdif_file_t;
    if (hdl)	{
        source_port = hdl->spdif_slave_parm.ch_cfg[0].data_io;
    }
    return source_port;
}

/* 通过inf信息获取采样率信息
 * 返回值：采样率
 * note:
 */
static int spdif_get_sr(void)
{
    u32 inf = JL_SPDIF->SS_CSB0;
    u32 tmp = ((inf & 0xF000000) >> 24) ^ 0xF;
    switch (tmp) {
    case 1:
        return 192000;
    case 2:
        return 12000;
    case 3:
        return 176400;
    case 5:
        return 96000;
    case 6:
        return 8000;
    case 7:
        return 88200;
    case 8:
        return 16000;
    case 9:
        return 24000;
    case 10:
        return 11025;
    case 11:
        return 22050;
    case 12:
        return 32000;
    case 13:
        return 48000;
    case 15:
        return 44100;
    default:
        printf("[%s], can't identify spdif sampal rate, return default sr:48000!\n", __func__);
        break;
    }
    return 48000;
}

/* 获取 spdif 的声道数, 值为0表示默认值双声道 */
static int spdif_get_voice_chhannel_num(void)
{
    int inf = JL_SPDIF->SS_CSB0;
    int tmp = (inf & 0xF00000) >> 19;
    switch (tmp) {
    case 0:
        return AUDIO_CH_LR;
    case 1:
        return AUDIO_CH_L;
    case 2:
        return AUDIO_CH_R;
    default:
        printf("[%s], can't identify spdif channel num, return default channel num:AUDIO_CH_LR!\n", __func__);
        break;
    }
    return AUDIO_CH_LR;
}


/* 该回调可以用来获取帧信息
 * 每一次帧对应192 个 byte， VUCP|VUCP 分为左右声道组成一个byte，一共有192个byte
 * 每次取其中的一位，读取出来每一帧就有对应 192 个位, 192/8 = 24 byte
 * note : bit_C 值需要和寄存器组 JL_SPDIF->SS_CSBx 对应
 */
static void spdif_inf_isr_cb(void *buf, u32 len)
{
#if 0
    /* printf("inf len = %d\n", len);	//192 */
    u8 *u8_buf = (u8 *)buf;
    u32 bit_C[6] = {0};	//192个bit，24个byte, 6个字
    u32 bit_U[6] = {0};	//192个bit，24个byte, 6个字
    /* put_buf(u8_buf, 192); */
    for (int i = 0; i < 192; i++) {
        bit_C[i / 32] |= (((u8_buf[i] & 0x04) >> 2) << (i % 32));	//channel bit
        /* printf("%d", ((u8_buf[i] & 0x04) >> 2) ); */
        bit_U[i / 32] |= (((u8_buf[i] & 0x02) >> 1) << (i % 32));	//user bit
    }
    for (int i = 0; i < 6; i++) {
        printf("bit_C[%d] = 0x%X\n", i, bit_C[i]);
    }
    for (int i = 0; i < 6; i++) {
        printf("bit_U[%d] = 0x%X\n", i, bit_U[i]);
    }
    y_printf("JL_SPDIF->SS_CSB0 = 0x%X\n", JL_SPDIF->SS_CSB0);
    y_printf("JL_SPDIF->SS_CSB1 = 0x%X\n", JL_SPDIF->SS_CSB1);
    y_printf("JL_SPDIF->SS_CSB2 = 0x%X\n", JL_SPDIF->SS_CSB2);
    y_printf("JL_SPDIF->SS_CSB3 = 0x%X\n", JL_SPDIF->SS_CSB3);
    y_printf("JL_SPDIF->SS_CSB4 = 0x%X\n", JL_SPDIF->SS_CSB4);
    y_printf("JL_SPDIF->SS_CSB5 = 0x%X\n", JL_SPDIF->SS_CSB5);
#endif
}

/*
 * @description: 获取spdif设置的 data_dma_len 和 inf_dma_len，由spdif采样率决定
 *               当大于96k时，data_dma_len 值为 SPDIF_DATA_DMA_LEN * 3, inf_dma_len值为SPDIF_INF_DMA_LEN * 3;
 *               否则为 SPDIF_DATA_DMA_LEN 和 SPDIF_INF_DMA_LEN;
 * @note: 通过获取data_dma_len 的值可以知道中断点数的大小，为 spdif_data_dma_len / 2
 */
void spdif_get_dma_len(u32 *spdif_data_dma_len, u32 *spdif_inf_dma_len)
{
    *spdif_data_dma_len = spdif_format.spdif_data_dma_len;
    *spdif_inf_dma_len = spdif_format.spdif_inf_dma_len;
}

/* spdif 数据输入格式检测, 返回非0值代表格式错误 */
static int spdif_format_monitor()
{
    int ret = 0;
    if (spdif_format.get_fmt_complete == 1) {
        // 已经获取过了spdif数据格式
        if (spdif_slave_get_data_format() == SPDIF_S_DAT_PCM) {
            if (spdif_format.coding_type != AUDIO_CODING_PCM) {
                spdif_format.get_fmt_complete = 0;
                spdif_format.init = 0;
                ret = -1;
            }
        } else {
            if (spdif_format.coding_type != AUDIO_CODING_DTS) {
                spdif_format.get_fmt_complete = 0;
                spdif_format.init = 0;
                ret = -2;
            }
        }

        // 无特殊情况继续检测采样率
        if (spdif_file_t->special_idx == SPDIF_SPECIAL_NONE) {
            if (spdif_format.sample_rate != spdif_get_sr()) {
                spdif_format.get_fmt_complete = 0;
                spdif_format.init = 0;
                ret = -3;
            }
        }
    }
    if (ret != 0) {
        /* spdif_file_t->need_restart_flag = 1;	//重启标志置一 */
    }
    return ret;
}


/*
 * 针对采样率的特殊情况处理：
 * 1.当输入为16k时，spdif_get_sr 值一成不变，而 spdif_slave_get_sr 标准还是16k左右
 * 2.当输入为12k时，spdif_slave_get_sr 在12k左右
 * 3.当输入为11025时，spdif_slave_get_sr 在11025左右
 * 4.当输入为8000时，spdif_slave_get_sr 在8k左右
 * return : 该函数返回特殊情况下标, 表示发生特殊情况
 * note : 发生特殊情况下无视矫错机制, 当特殊情况发生时，spdif_get_sr 值都在441k以下
 */
static u8 spdif_sr_special_deal(int sr, int slave_sr)
{
    /* static u8 special_flag = 0; */
    static int last_slave_sr = 0;
    static u8 special_cnt = 0;
    //特殊情况下标。当特殊情况和当前特殊情况不相等时，说明是第一次，需要进行处理.
    // special_idx 为 0 表示没有发生特殊情况
    static u8 special_idx = 0;

    if (sr >= 44100) {
        special_cnt = 0;
        special_idx = SPDIF_SPECIAL_NONE;
        goto __exit;
    }

    //稳定16k采样率，且波动不大
    /* printf(">> %d\n", slave_sr); */
    if ((__builtin_abs(16000 - slave_sr) < SPDIF_SR_ERROR_TOLERANT_WIDTH) && (__builtin_abs(last_slave_sr - slave_sr) < 20)) {
        /* 特殊情况 1 */
        special_cnt++;
        if (special_cnt >= 5) {
            special_cnt = 5;
            // 认为发生了特殊情况1
            spdif_format.sample_rate = 16000;
            if (special_idx != SPDIF_SPECIAL_16000) {
                // 第一次发生了特殊情况，需要处理
                special_idx = SPDIF_SPECIAL_16000;
                spdif_format.init = 1;
                spdif_format.get_fmt_complete = 1;
                spdif_file_t->need_restart_flag = 1;
                goto __exit;
            }
        }
    } else if ((__builtin_abs(12000 - slave_sr) < SPDIF_SR_ERROR_TOLERANT_WIDTH) && (__builtin_abs(last_slave_sr - slave_sr) < 20)) {
        /* 特殊情况 2 */
        special_cnt++;
        if (special_cnt >= 5) {
            special_cnt = 5;
            // 认为发生了特殊情况1
            spdif_format.sample_rate = 12000;
            if (special_idx != SPDIF_SPECIAL_12000) {
                // 第一次发生了特殊情况，需要处理
                special_idx = SPDIF_SPECIAL_12000;
                spdif_format.init = 1;
                spdif_format.get_fmt_complete = 1;
                spdif_file_t->need_restart_flag = 1;
                goto __exit;
            }
        }
    } else if ((__builtin_abs(11025 - slave_sr) < SPDIF_SR_ERROR_TOLERANT_WIDTH) && (__builtin_abs(last_slave_sr - slave_sr) < 20)) {
        /* 特殊情况 3 */
        special_cnt++;
        if (special_cnt >= 5) {
            special_cnt = 5;
            // 认为发生了特殊情况1
            spdif_format.sample_rate = 11025;
            if (special_idx != SPDIF_SPECIAL_11025) {
                // 第一次发生了特殊情况，需要处理
                special_idx = SPDIF_SPECIAL_11025;
                spdif_format.init = 1;
                spdif_format.get_fmt_complete = 1;
                spdif_file_t->need_restart_flag = 1;
                goto __exit;
            }
        }
    } else if ((__builtin_abs(8000 - slave_sr) < SPDIF_SR_ERROR_TOLERANT_WIDTH) && (__builtin_abs(last_slave_sr - slave_sr) < 20)) {
        /* 特殊情况 4 */
        special_cnt++;
        if (special_cnt >= 5) {
            special_cnt = 5;
            // 认为发生了特殊情况1
            spdif_format.sample_rate = 8000;
            if (special_idx != SPDIF_SPECIAL_8000) {
                // 第一次发生了特殊情况，需要处理
                special_idx = SPDIF_SPECIAL_8000;
                spdif_format.init = 1;
                spdif_format.get_fmt_complete = 1;
                spdif_file_t->need_restart_flag = 1;
                goto __exit;
            }
        }
    } else {
        // 无特殊情况
        special_cnt--;
        if (special_cnt <= 1) {
            special_cnt = 1;
            if (special_idx != SPDIF_SPECIAL_NONE) {
                //上一次是特殊情况，这次退出了特殊情况
                special_idx = SPDIF_SPECIAL_NONE;
                spdif_format.init = 0;
                spdif_format.get_fmt_complete = 0;
                spdif_file_t->need_restart_flag = 1;
                goto __exit;
            }
        }
    }

__exit:
    last_slave_sr = slave_sr;
    return special_idx;
}


/*
 * 计算spdif 采样率的函数
 * 思路：中断点数 / 中断间隔时间 = 采样率
 */
static u32 spdif_get_sr_by_software(u32 irq_timestamp, u32 last_irq_timestamp)
{
    u32 diff_time = 0;
    if (irq_timestamp >= last_irq_timestamp) {
        diff_time = irq_timestamp - last_irq_timestamp;
    } else {
        diff_time = 0xffffffff - last_irq_timestamp + irq_timestamp + 1;
    }
    u32 sr = 0;
    u32 spdif_data_dma_len = 0;
    u32 spdif_inf_dma_len = 0;
    spdif_get_dma_len(&spdif_data_dma_len, &spdif_inf_dma_len);
    if (diff_time) {
        sr = (spdif_data_dma_len / 2 / 4) * ((float)US_PER_SECOND / (float)diff_time);
    }
    /* y_printf("---> diff_time:%d, last_t:%d, irq_t:%d\n", diff_time, last_irq_timestamp, irq_timestamp); */
    return sr;
}

/*
 * @description: spdif 数据中断回调函数
 * @return：无
 * @note: spdif 数据获取的都是双声道数据，len = SPDIF_DATA_DMA_LEN/2, 除2是因为乒乓buf, buf 地址是一个乒乓Buf dma地址
 */
static void spdif_data_isr_cb(void *buf, u32 len)
{
    struct spdif_file_hdl *hdl = spdif_file_t;
    struct stream_frame *frame = NULL;

    s16 *s16_buf = (s16 *)buf;
    s32 *s32_buf = (s32 *)buf;

    if (!hdl) {
        return;
    }
    struct stream_node  *source_node = hdl->source_node;
    u32 spdifrx_isr_timestamp = audio_jiffies_usec();

#if SPDIF_ONLINE_DET_EN
    // irq_running_cnt 作为中断标志
    hdl->irq_running_cnt++;
#endif

    /* 软件方法计算采样率 */
    u32 irq_timestamp = audio_jiffies_usec();
    if (hdl->last_irq_timestamp) {
        hdl->sw_real_time_sr = spdif_get_sr_by_software(irq_timestamp, hdl->last_irq_timestamp);
    }
    hdl->last_irq_timestamp = irq_timestamp;

    /* y_printf(">> [msg sr]:%d, [hw sr]:%d, [sw sr]:%d, [hdl(fmt) sr]:%d\n", spdif_get_sr(), spdif_slave_get_sr(), hdl->sw_real_time_sr, hdl->sample_rate_set); */

    /* 驱动去掉了帧错误重启的操作，这里需要过滤掉由于信号干扰产生的帧错误 */
    if (hdl->sw_real_time_sr < 1000) {
        hdl->need_fade_in_flag = 1;	//做淡入
        return;
    }

    /* 一开始丢掉几帧，防止一开始几帧的数据不稳定带来的错误 */
    if (hdl->drop_frame_cnt) {
        hdl->drop_frame_cnt--;
        return;
    }


    // 滤掉超过192k采样率的,  spdif_slave_get_sr 的值会到216k左右
    if (spdif_slave_get_sr() > 200000 && spdif_get_sr() == 192000) {
        return;
    }


    // 先获取已知的格式
    if (spdif_format.init == 0 || spdif_format.get_fmt_complete == 0) {
        if (spdif_slave_get_data_format() == SPDIF_S_DAT_PCM) {
            spdif_format.is_pcm = 1;
            spdif_format.coding_type = AUDIO_CODING_PCM;
        } else {
            spdif_format.coding_type = AUDIO_CODING_DTS;
            spdif_format.is_pcm = 0;
        }
        spdif_format.sample_rate = spdif_get_sr();
        hdl->input_channel_num = spdif_get_voice_chhannel_num();

        /* printf("[%s] format.sr:%d, slave_get_sr:%d\n",__func__, spdif_format.sample_rate, spdif_slave_get_sr()); */
        //矫错机制, 放宽矫错机制，LSB时钟会实时变化导致硬件算出的采样率有可能是不准的
        if (__builtin_abs((int)(spdif_format.sample_rate - spdif_slave_get_sr())) <= SPDIF_SR_ERROR_TOLERANT_WIDTH) {
            if (++hdl->format_correct_cnt == 5) {
                //认为spdif格式获取稳定
                hdl->format_correct_cnt = 0;
                if (spdif_format.sample_rate > 48000 && spdif_format.sample_rate <= 96000) {
                    // 这里 88.2k、96k的采样率中断点数也加大，是为了解决该采样率下叠加提示音导致内存溢出的现象
                    spdif_format.spdif_data_dma_len = SPDIF_DATA_DMA_LEN * 3;
                    spdif_format.spdif_inf_dma_len = SPDIF_INF_DMA_LEN * 3;
                } else if (spdif_format.sample_rate > 96000) {
                    // 当采样率为192000或者176400时，播放过程叠加提示音可能会导致内存溢出，因此增加中断点数
                    spdif_format.spdif_data_dma_len = SPDIF_DATA_DMA_LEN * 4;
                    spdif_format.spdif_inf_dma_len = SPDIF_INF_DMA_LEN * 4;
                } else {
                    spdif_format.spdif_data_dma_len = SPDIF_DATA_DMA_LEN;
                    spdif_format.spdif_inf_dma_len = SPDIF_INF_DMA_LEN;
                }
                if (spdif_format.spdif_data_dma_len != hdl->spdif_set_data_dma_len) {
                    hdl->need_restart_flag = 1;
                }
                if (hdl->have_data == 1 && spdif_format.last_sample_rate != spdif_format.sample_rate) {
                    // 说明在打开整条数据流的前提下，中间采样率发生了改变
                    spdif_format.last_sample_rate = spdif_format.sample_rate;
                    hdl->need_restart_flag = 1;
                }
                spdif_format.init = 1;
                spdif_format.get_fmt_complete = 1;
            }
        } else {
            hdl->format_correct_cnt = 0;
        }
    }

    /* 特殊情况处理 */
    hdl->special_idx = spdif_sr_special_deal(spdif_get_sr(), (int)spdif_slave_get_sr());

    if (spdif_format.get_fmt_complete == 0) {
        return;
    }

    //hdl->have_data 置1标示接收到数据, 此时打开数据流
#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)
    if (hdl->have_data == 0 && spdif_format.get_fmt_complete == 1 && get_broadcast_role() != BROADCAST_ROLE_RECEIVER) {
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if (hdl->have_data == 0 && spdif_format.get_fmt_complete == 1 && get_auracast_role() != APP_AURACAST_AS_SINK) {
#else
    if (hdl->have_data == 0 && spdif_format.get_fmt_complete == 1) {
#endif
        hdl->have_data = 1;	//该值为1说明打开了数据流
        // 有数据进来，打开数据流, 解码开始通过 spdif_fread 拿数
        spdif_open_player_by_taskq();
        return;
    }

    // spdif 格式检查
    if (spdif_format_monitor() != 0) {
        /* putchar('E'); */
        return;
    }


    if (hdl->need_restart_flag == 1) {
        hdl->need_restart_flag = 0;
        spdif_restart_by_taskq();	//重启
        return;
    }

    //需要等整条数据流都打开, 否则会出现异常
    if (hdl->start != 1) {
        return;
    }

    /* 防止出现有输入96k、88.2k的采样率被识别为48k 的现象 */
    if (hdl->special_idx == SPDIF_SPECIAL_NONE) {
        if (__builtin_abs((int)(hdl->sample_rate_set - spdif_slave_get_sr())) > SPDIF_SR_ERROR_TOLERANT_WIDTH) {
            if (++hdl->format_correct_cnt == 5) {
                hdl->format_correct_cnt = 0;
                hdl->need_restart_flag = 1;
                spdif_format.init = 0;
                spdif_format.get_fmt_complete = 0;
                return;
            }
        } else {
            hdl->format_correct_cnt = 0;
        }
    }

    if (hdl->is_hdmi_source) {
        //HDMI数据源需要等待数据稳定(CEC通信成功)再往后面推数
        if (hdl->hdmi_stop_push_data) {
            if (++hdl->hdmi_wait_cec_drop_packet_cnt >= 100) {
                hdl->hdmi_wait_cec_drop_packet_cnt = 0;
                hdl->hdmi_stop_push_data = 0;
            }
            return;
        }
    }

    if (spdif_slave_get_data_format() == SPDIF_S_DAT_PCM) {	//PCM
        //淡入处理
#if SPDIF_FADE_IN_EN
        hdl->new_frame_can_fade_in = 1;
        float step = SPDIF_FADE_IN_STEP;

        if (hdl->need_fade_in_flag) {
            if (hdl->spdif_slave_parm.data_mode == SPDIF_S_DAT_16BIT) {
                for (int i = 0; i < len / 2; i++) {
                    s16 bufi = s16_buf[i];
                    static s16 last_bufi = 0;
                    //存在0值点, 在0值点附近做淡出
                    if (bufi >= 0 && last_bufi <= 0 && hdl->new_frame_can_fade_in == 1) {
                        hdl->fade_in_coefficient += step;
                        hdl->new_frame_can_fade_in = 0;
                    }
                    last_bufi = bufi;

                    s16_buf[i] = (s16)((float)s16_buf[i] * hdl->fade_in_coefficient);
                }
            } else {
                for (int i = 0; i < len / 4; i++) {
                    s32_buf[i] = (s32)((float)s32_buf[i] * hdl->fade_in_coefficient);
                }
            }

            //此帧没有存在0值点，则直接做步进
            if (hdl->new_frame_can_fade_in == 1) {
                hdl->fade_in_coefficient += step;
                hdl->new_frame_can_fade_in = 0;
            }

            if (hdl->fade_in_coefficient >= 1.0f) {
                // 淡入完成
                g_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Spidf Fade In finsh!\n")
                hdl->need_fade_in_flag = 0;
                hdl->fade_in_coefficient = 0.0f;
            }
        }
#endif

    } else {	//非PCM
        return;
    }

    frame = source_plug_get_output_frame(source_node, len);
    if (!frame) {
        return;
    }
    if (hdl->spdif_slave_parm.data_mode == SPDIF_S_DAT_16BIT) {
        frame->len    = len;
        frame->flags  = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp = spdifrx_isr_timestamp;
        memcpy(frame->data, buf, len);
        source_plug_put_output_frame(source_node, frame);
    } else if (hdl->spdif_slave_parm.data_mode == SPDIF_S_DAT_24BIT) {
        frame->len    = len;
        frame->flags  = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp = spdifrx_isr_timestamp;
        memcpy(frame->data, buf, len);
        source_plug_put_output_frame(source_node, frame);
    }
}

#if SPDIF_ONLINE_DET_EN
/*
 * @description: 该定时器回调用于检测spdif是否在线
 * @note: 为了解决spdif拔掉数据线不起中断时，来提示音死机的现象
 *        当检测到不起中断时，会先关闭数据流，再打开spdif，等到有spdif数据来时，才会在中断里打开spdif的数据流
          这样，当spdif掉线时，再来提示音，此时spdif 数据流处于关闭的状态，提示音通路走的是 TONE_NORMAL 通路，当spdif在线时，提示音走的是 MEDIA 的通路
 */
static void spdif_online_det_timer(void *priv)
{
    struct spdif_file_hdl *hdl = spdif_file_t;

#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)
    //广播下不做掉线检查
    if (get_broadcast_role()) {
        if (hdl->irq_running_cnt) {
            // spdif 在线
            hdl->irq_running_cnt = 0;
            hdl->online_flag = 1;
        } else {
            hdl->online_flag = 0;
        }
        return;
    }
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    //广播下不做掉线检查
    if (get_auracast_role()) {
        if (hdl->irq_running_cnt) {
            // spdif 在线
            hdl->irq_running_cnt = 0;
            hdl->online_flag = 1;
        } else {
            hdl->online_flag = 0;
        }
        return;
    }
#endif

    if (hdl) {
        if (hdl->irq_running_cnt) {
            // spdif 在线
            hdl->irq_running_cnt = 0;
            hdl->online_flag = 1;
        } else {
            // spdif 离线
            if (hdl->start == 1 && hdl->online_flag == 1) {
                //start == 1 说明数据流已经打开
                if (hdl->is_hdmi_source == 0) {
                    //同轴或光纤的处理
                    r_printf("\n>>>>>>>>>>>> SPDIF LOST CONNECTION !! <<<<<<<<<<<<<\n");
                    hdl->online_flag = 0;
                    spdif_restart_by_taskq();	//重启: close_player + spdif_start
                } else {
                    //HDMI 的处理
                    if (hdmi_is_online()) {
                        //这种情况是HDMI插入但电视给错误信号或者电视不给信号的情况
                        //这种暂时不做处理
                        putchar('N');
                    } else {
                        // 拔掉HDMI的情况
                        r_printf("\n>>>>>>>>>>>> HDMI LOST CONNECTION !! <<<<<<<<<<<<<\n");
                        hdl->online_flag = 0;
                        spdif_restart_by_taskq();	//重启: close_player + spdif_start
                    }
                }
            }
        }
    }
}
#endif


/*
 * @description: 打开检测spdif是否在线的定时器
 * @note: 为了解决spdif拔掉数据线不起中断时，来提示音死机的现象
 *        当检测到不起中断时，会先关闭数据流，再打开spdif(也就是重启)，等到有spdif数据来时，才会在中断里打开spdif的数据流
 */
static void spdif_open_online_det_timer(void)
{
#if SPDIF_ONLINE_DET_EN
    if (spdif_file_t &&spdif_file_t->online_det_timer_id == 0) {
        // 最小检测检测时间 = 中断点数/采样率 * 2, 单位ms，*10是为了防止误测, 排除可屏蔽中断产生的干扰
        /* u32 spdif_online_det_time_ms = (spdif_format.spdif_data_dma_len / 2 / 2 / 2) / (spdif_format.sample_rate / MS_PER_SECOND) * 2 * 10; */
        u32 spdif_online_det_time_ms = SPDIF_ONLINE_DET_TIME_MS;
        spdif_file_t->online_det_timer_id = usr_timer_add(NULL, spdif_online_det_timer, spdif_online_det_time_ms, 0);
        g_printf(">>>>> %s, %d, det ms : %d, open spdif online detect timer success!\n", __func__, __LINE__, spdif_online_det_time_ms)
    } else {
        r_printf(">>>>> %s, %d, open spdif online detect timer failed!\n", __func__, __LINE__);
    }
#else
    r_printf(">>>>> %s, %d, open spdif online detect timer failed!\n", __func__, __LINE__);
#endif
}


/*
 * @description: 删除检测spdif是否在线的定时器
 */
static void spdif_close_online_det_timer(void)
{
#if SPDIF_ONLINE_DET_EN
    if (spdif_file_t &&spdif_file_t->online_det_timer_id) {
        usr_timer_del(spdif_file_t->online_det_timer_id);
        spdif_file_t->online_det_timer_id = 0;
    }
#endif
}


/*
 * @description: 关闭spdif，停止spdif 数据中断数据输入
 * @return：返回0表示成功
 * @note:
 */
int spdif_stop(void)
{
    struct spdif_file_hdl *hdl = spdif_file_t;
    printf(" --- spdif_stop ---\n");
    if (!hdl) {
        return -1;
    }

    if (hdl->state != AUDIO_SPDIF_STATE_START) {
        r_printf(">> %s, %d, spdif state is not AUDIO_SPDIF_STATE_START!\n", __func__, __LINE__);
        return 0;
    }
    int ret = spdif_slave_channel_close(&hdl->spdif_slave_parm.ch_cfg[0]);
    if (ret == 0) {
        ret = spdif_slave_stop(&hdl->spdif_slave_parm);
        if (ret == 0) {
            hdl->state = AUDIO_SPDIF_STATE_STOP;
            return 0;
        }
    }
    return -1;
}

/*
 * @description: 初始化spdif 驱动
 * @return：返回0表示成功
 * @note: 一些驱动相关的配置可以在这个函数里进行配置
 */
static int spdif_driver_init(void)
{
    struct spdif_file_hdl *hdl = spdif_file_t;
    int ret = -1;
    u32 spdif_data_dma_len = 0;
    u32 spdif_inf_dma_len = 0;
    if (hdl) {
        printf("\n=========  spdif_open  =========\n");
        hdl->spdif_slave_parm.rx_clk = SPDIF_RX_CLK;
        struct audio_general_params *params = audio_general_get_param();
        hdl->spdif_slave_parm.data_mode = params->media_bit_width ?
                                          SPDIF_S_DAT_24BIT : SPDIF_S_DAT_16BIT;
        if (spdif_port_config.init == 1) {
            hdl->spdif_slave_parm.ch_cfg[0].port_sel = spdif_port_config.port_mode;
            hdl->spdif_slave_parm.ch_cfg[0].data_io = spdif_port_config.port;
        } else {
            // IO 默认配置
            hdl->spdif_slave_parm.ch_cfg[0].port_sel = SPDIF_IN_DIGITAL_PORT_A;
            hdl->spdif_slave_parm.ch_cfg[0].data_io = IO_PORTA_09;
        }

        if (hdl->spdif_slave_parm.ch_cfg[0].data_io == uuid2gpio(spdif_cfg.hdmi_port[0]) ||
            hdl->spdif_slave_parm.ch_cfg[0].data_io == uuid2gpio(spdif_cfg.hdmi_port[1])) {
            //HDMI 源
            hdl->is_hdmi_source = 1;
        } else {
            hdl->is_hdmi_source = 0;
        }
        hdl->spdif_slave_parm.ch_cfg[0].et_den = 1;	//拔出检测
        hdl->spdif_slave_parm.ch_cfg[0].is_den = 1;	//插入检测
        spdif_get_dma_len(&spdif_data_dma_len, &spdif_inf_dma_len);
        hdl->spdif_set_data_dma_len = spdif_data_dma_len;
        y_printf(">>>>>>>>>>>>>>>> data_dma_len:%d, inf_dma_len:%d\n", spdif_data_dma_len, spdif_inf_dma_len);
        if (hdl->spdif_slave_parm.data_mode == SPDIF_S_DAT_16BIT) {
            hdl->spdif_slave_parm.data_dma_len = spdif_data_dma_len;
            hdl->spdif_slave_parm.inf_dma_len  = spdif_inf_dma_len;
        } else {	//hdl->spdif_slave_parm.data_mode == SPDIF_S_DAT_24BIT
            hdl->spdif_slave_parm.data_dma_len = spdif_data_dma_len * 2;
            hdl->spdif_slave_parm.inf_dma_len  = spdif_inf_dma_len;
        }
        hdl->spdif_slave_parm.data_isr_cb  = spdif_data_isr_cb;
        hdl->spdif_slave_parm.inf_isr_cb   = spdif_inf_isr_cb;
        /* hdl->spdif_slave_parm.inf_isr_cb   = NULL; */
        hdl->state = AUDIO_SPDIF_STATE_CLOSE;
#if SPDIF_FADE_IN_EN
        hdl->fade_in_coefficient = 0.0f;
#endif
        hdl->last_irq_timestamp = 0;
        hdl->format_correct_cnt = 0;
        hdl->hdmi_wait_cec_drop_packet_cnt = 0;

        spdif_slave_init(&(hdl->spdif_slave_parm));
        spdif_slave_channel_init(&(hdl->spdif_slave_parm), 0);

        hdl->state = AUDIO_SPDIF_STATE_INIT;
        printf(">>>>> [%s], spdif open succ\n", __func__);
        ret = 0;
    }
    return ret;
}

/*
 * @description: 打开spdif，此时spdif 数据中断开始有数据输入
 * @return：返回0表示成功
 * @note:
 */
int spdif_start(void)
{
    struct spdif_file_hdl *hdl = spdif_file_t;
    if (!hdl) {
        return -1;
    }
    printf("\n=========  spdif_start  =========\n");
    if (hdl->state != AUDIO_SPDIF_STATE_INIT) {		// state = 2
        spdif_driver_init();
    }
    int ret = 0;
    ret = spdif_slave_channel_open(&hdl->spdif_slave_parm.ch_cfg[0]);
    if (ret == 0) {
        ret = spdif_slave_start(&hdl->spdif_slave_parm);
        if (ret == 1) {
            hdl->state = AUDIO_SPDIF_STATE_START;
            // spdif 192k采样率输入，同步节点硬件SRC, 时钟需卡的较高
            // 设置中断丢帧数量
            hdl->drop_frame_cnt = SPDIF_DROP_FRAME_CNT;
            printf(">>>>>>>>>> spdif start succ\n");
            return 0;
        }
    }
    return -1;
}



/* spdif 获取格式 */
static int spdif_ioc_get_fmt(struct spdif_file_hdl *hdl, struct stream_fmt *fmt)
{
    printf("==========  spdif_ioc_get_fmt  ==========\n");

    if (spdif_format.init == 0) {
        printf("-----> default fmt\n");
        goto __default;
    } else {
        r_printf("-----> inited fmt\n");
        if (spdif_format.is_pcm == 1) {
            fmt->coding_type = AUDIO_CODING_PCM;
        } else {
            fmt->coding_type = AUDIO_CODING_DTS;
        }
        hdl->channel_mode = AUDIO_CH_LR;
        fmt->channel_mode = hdl->channel_mode;
        fmt->sample_rate = spdif_format.sample_rate;
        hdl->coding_type = fmt->coding_type;
        hdl->sample_rate_set = fmt->sample_rate;
        if (hdl->spdif_slave_parm.data_mode == SPDIF_S_DAT_24BIT) {
            fmt->bit_wide = DATA_BIT_WIDE_24BIT;
        }
        return 0;
    }


__default:
    /* fmt->coding_type = AUDIO_CODING_DTS;	 */
    fmt->coding_type = AUDIO_CODING_PCM;	//默认PCM
    fmt->channel_mode   = AUDIO_CH_LR;
    fmt->sample_rate    = 48000;
    hdl->channel_mode = fmt->channel_mode;
    hdl->coding_type = fmt->coding_type;
    hdl->sample_rate_set = fmt->sample_rate;
    return 0;
}

static int spdif_ioc_set_fmt(struct spdif_file_hdl *hdl, struct stream_fmt *fmt)
{
    printf("==========  spdif_ioc_set_fmt  ==========\n");
    hdl->sample_rate_set = fmt->sample_rate;
    hdl->channel_mode = fmt->channel_mode;
    return 0;
}

static int spdif_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct spdif_file_hdl *hdl = (struct spdif_file_hdl *)_hdl;
    switch (cmd) {
    case NODE_IOC_GET_FMT:
        ret = spdif_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = spdif_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        break;
    case NODE_IOC_START:
        if (hdl->start == 0) {
            hdl->need_fade_in_flag = 1;
            //打开检测spdif在线的定时器, 此时是player刚打开的状态
            spdif_open_online_det_timer();
            hdl->start = 1;
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
            ret = spdif_stop();
        }
        break;
    }
    return ret;
}



/*
 * @description: 初始化spdif
 * @return：返回 struct spdif_file_hdl 结构体类型指针
 * @note: 无
 */
void *spdif_init(void)
{
    if (spdif_file_t) {
        return spdif_file_t;
    }

    printf(" --- spdif_init ---\n");

    struct spdif_file_hdl *hdl = zalloc(sizeof(*hdl));
    if (hdl) {
        spdif_file_t = hdl;
        /* 初始化 spdif 驱动配置 */
        spdif_driver_init();
    }
    return spdif_file_t;
}


/*
 * @description: 释放掉spdif 申请的内存
 * @param: 参数传入 spdif_file_hdl 结构体指针类型的参数
 * @return：无
 * @note: 无
 */
void spdif_release(void *_hdl)
{
    struct spdif_file_hdl *hdl = (struct spdif_file_hdl *)_hdl;

    y_printf(" --- spdif_release ---\n");

    spin_lock(&spdif_lock);
    if (!hdl) {
        hdl = spdif_file_t;
        if (!hdl) {
            spin_unlock(&spdif_lock);
            return;
        }
    }
    // 需要先调用spdif_stop 函数
    if (hdl->state != AUDIO_SPDIF_STATE_STOP) {
        printf("Need call spdif_stop() function before spdif_release() function!\n");
        spin_unlock(&spdif_lock);
        return;
    }

    // 关闭spdif检测是否在线的定时器
    spdif_close_online_det_timer();
    spdif_slave_uninit(&hdl->spdif_slave_parm);
    hdl->state = AUDIO_SPDIF_STATE_CLOSE;
    free(hdl);
    hdl = NULL;
    spdif_file_t = NULL;	//全局变量

    spin_unlock(&spdif_lock);
    printf(">>[%s] : spdif release succ\n", __func__);
}


/*
 * @description: spdif file 初始化
 * @return: spdif_file_hdl 结构体类型的指针
 * @note: 无
 */
static void *spdif_file_init(void *source_node, struct stream_node *node)
{
    struct spdif_file_hdl *hdl = NULL;
    if (spdif_file_t != NULL) {
        hdl = spdif_file_t;
    } else {
        hdl = zalloc(sizeof(*hdl));
    }

    printf("--- spdif_file_init ---\n");

    if (hdl) {
        hdl->source_node = source_node;
        hdl->node = node;
        node->type |= NODE_TYPE_IRQ;

        if (hdl->state == AUDIO_SPDIF_STATE_INIT) {
            printf(">>> %s, %d, spdif state is AUDIO_SPDIF_STATE_INIT\n", __func__, __LINE__);
            spdif_start();
        } else if (hdl->state == AUDIO_SPDIF_STATE_CLOSE) {
            printf(">>> %s, %d, spdif state is AUDIO_SPDIF_STATE_CLOSE\n", __func__, __LINE__);
            spdif_init();
            spdif_start();
        }
    }
    return hdl;
}

/* 提供给外部用于设置默认启动的spdif io 的接口,spdif_init前设置
 * port:      比如 IO_PORTA_06
 * port_mode: 比如 SPDIF_IN_ANALOG_PORT_A 或者 SPDIF_IN_DIGITAL_PORT_A(详见 SPDIF_SLAVE_CH 枚举)
 */
void set_spdif_default_io(u8 port, u8 port_mode)
{
    spdif_port_config.init = 1;
    spdif_port_config.port = port;
    spdif_port_config.port_mode = port_mode;

}

/* 提供给外部用于切换 spdif io 的接口
 * port:      比如 IO_PORTA_06
 * port_mode: 比如 SPDIF_IN_ANALOG_PORT_A 或者 SPDIF_IN_DIGITAL_PORT_A(详见 SPDIF_SLAVE_CH 枚举)
 */
void spdif_io_port_switch(u8 port, u8 port_mode)
{
    if (port_mode > SPDIF_IN_ANALOG_PORT_D) {
        printf("%s, %d, param err!\n", __func__, __LINE__);
        return;
    }
    spdif_port_config.init = 1;
    spdif_port_config.port = port;
    spdif_port_config.port_mode = port_mode;

    struct spdif_file_hdl *hdl = spdif_file_t;
    if (hdl) {
        spdif_restart_by_taskq();	//重启: close_player + spdif_start
        spdif_switch_source_unmute();
        g_printf("%s, spdif io switch, port io : [%d]!!\n", __func__, spdif_port_config.port);
    }
}

/*HDMI-OPT-COAL 0-2*/
void spdif_set_port_by_index(u8 index)
{
    u8 port_sel;
    u8 next_port = 0xff;
    u8 next_port_mode = 0xff;
    if (spdif_port_config.init) {
        port_sel = spdif_port_config.port;
        switch (index) {
        case 0:
            next_port = uuid2gpio(spdif_cfg.hdmi_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.hdmi_port[1]);
            if (next_port != 0xff) {
                break;
            }
            break;
        case 1:
            next_port = uuid2gpio(spdif_cfg.opt_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.opt_port[1]);
            if (next_port != 0xff) {
                break;
            }
            break;
        case 2:
            next_port = uuid2gpio(spdif_cfg.coal_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.coal_port[1]);
            if (next_port != 0xff) {
                break;
            }
            break;
        }
        if (next_port != 0xff && next_port != port_sel) {
            next_port_mode = get_spdif_analog_port_index(next_port);
            if (next_port_mode != 0xff) {
                spdif_io_port_switch(next_port, next_port_mode + SPDIF_IN_ANALOG_PORT_A);
            } else {
                spdif_io_port_switch(next_port, SPDIF_IN_DIGITAL_PORT_A);
            }
            log_i("switch port [%x] \n", next_port);
        }
    }
}

u8 spdif_get_cur_port_index(void)
{
    u8 port_sel;
    u8 ret = 0XFF;
    if (spdif_port_config.init) {
        port_sel = spdif_port_config.port;
        if (port_sel == uuid2gpio(spdif_cfg.hdmi_port[0])
            ||  port_sel == uuid2gpio(spdif_cfg.hdmi_port[1])) {
            ret = 0;
        }
        if (port_sel == uuid2gpio(spdif_cfg.opt_port[0])
            ||  port_sel == uuid2gpio(spdif_cfg.opt_port[1])) {
            ret = 1;
        }
        if (port_sel == uuid2gpio(spdif_cfg.coal_port[0])
            ||  port_sel == uuid2gpio(spdif_cfg.coal_port[1])) {
            ret = 2;
        }
    }
    return ret;
}


/*循环切换配置HDMI-OPT-COAL*/
void spdif_io_loop_switch(void)
{
    u8 port_sel, i;
    u8 next_port = 0xff;
    u8 next_port_mode = 0xff;
    if (spdif_port_config.init) {
        port_sel = spdif_port_config.port;
        if (port_sel == uuid2gpio(spdif_cfg.hdmi_port[0])
            ||  port_sel == uuid2gpio(spdif_cfg.hdmi_port[1])) {
            i = 0;
        }
        if (port_sel == uuid2gpio(spdif_cfg.opt_port[0])
            ||  port_sel == uuid2gpio(spdif_cfg.opt_port[1])) {
            i = 1;
        }
        if (port_sel == uuid2gpio(spdif_cfg.coal_port[0])
            ||  port_sel == uuid2gpio(spdif_cfg.coal_port[1])) {
            i = 2;
        }
        switch (i) {
        case 2:
            next_port = uuid2gpio(spdif_cfg.hdmi_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.hdmi_port[1]);
            if (next_port != 0xff) {
                break;
            }
        case 0:
            next_port = uuid2gpio(spdif_cfg.opt_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.opt_port[1]);
            if (next_port != 0xff) {
                break;
            }
        case 1:
            next_port = uuid2gpio(spdif_cfg.coal_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.coal_port[1]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.hdmi_port[0]);
            if (next_port != 0xff) {
                break;
            }
            next_port = uuid2gpio(spdif_cfg.hdmi_port[1]);
            if (next_port != 0xff) {
                break;
            }
        }
        if (next_port != 0xff && next_port != port_sel) {
            next_port_mode = get_spdif_analog_port_index(next_port);
            if (next_port_mode != 0xff) {
                spdif_io_port_switch(next_port, next_port_mode + SPDIF_IN_ANALOG_PORT_A);
            } else {
                spdif_io_port_switch(next_port, SPDIF_IN_DIGITAL_PORT_A);
            }
            log_i("switch next port [%x] \n", next_port);
        }

    }
}

REGISTER_SOURCE_NODE_PLUG(spdif_file_plug) = {
    .uuid       = NODE_UUID_SPDIF,
    .init       = spdif_file_init,
    .ioctl      = spdif_ioctl,
    .release    = spdif_release,
};

#endif





