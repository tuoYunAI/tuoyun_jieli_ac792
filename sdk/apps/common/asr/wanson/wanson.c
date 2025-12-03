//做应用请参考以下两个分组的指令
//分组0 主要用于在播放的情况下做识别，有较高的识别率
//Wanson_ASR_Reset(0);
// 小艾小艾
// 打开助手
// 接听电话
// 挂断电话

//分组1 主要用于唤醒词识别后，音乐播放停止后用来做识别
//Wanson_ASR_Reset(1);
// 小艾小艾
// 打开助手
// 接听电话
// 挂断电话
// 打开灯光
// 关闭灯光
// 调暗一点
// 调暗灯光
// 调亮一点
// 调亮灯光
// 最大亮度
// 最小亮度
// 中等亮度
// 七彩模式
// 七彩变色
// 停止播放
// 停止变色
// 红色灯光
// 橙色灯光
// 黄色灯光
// 绿色灯光
// 青色灯光
// 蓝色灯光
// 紫色灯光
// 白色灯光
// 粉色灯光
// 音乐模式
// 播放音乐
// 继续播放
// 暂停播放
// 停止播放
// 上一首
// 下一首
// 音量大一点
// 音量小一点
// 调大音量
// 调小音量
// 最大音量
// 最小音量
// 中等音量

#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "fn_asr.h"
#include "event.h"
#include "app_config.h"
#include "fst_01.h"
#include "fst_02.h"

#if  (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == WANSON_ALGORITHM)

#ifdef CONFIG_AEC_ENC_ENABLE

/*aec module enable bit define*/
#define AEC_EN              BIT(0)
#define NLP_EN              BIT(1)
#define ANS_EN              BIT(2)

#define AEC_MODE_ADVANCE	(AEC_EN | NLP_EN | ANS_EN)
#define AEC_MODE_REDUCE		(NLP_EN | ANS_EN)
#define AEC_MODE_SIMPLEX	(ANS_EN)
#define AEC_MODE_KWS	(AEC_EN | ANS_EN)

#endif

#define AEC_DATA_TO_SD 0 //将唤醒前的mic/dac/aec数据写卡进行查看,3channel(mic,dac,aec)

#define ONCE_SR_POINTS	160
#define WANSON_BUF_SIZE	(12 * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#define ASR_FEAME		(3 * ONCE_SR_POINTS)

static struct {
    int pid;
    u32 sample_rate;
    int volatile run_flag;
    OS_SEM sem;
    s16 mic_buf[WANSON_BUF_SIZE];
    void *mic_enc;
    cbuffer_t mic_cbuf;
    Fst fst_1;
    Fst fst_2;
} wanson_server;

#define __this (&wanson_server)

static void Wanson_ASR_Reset(unsigned char domain)
{
    __this->fst_1.states = fst01_states;
    __this->fst_1.num_states = fst01_num_states;
    __this->fst_1.finals = fst01_finals;
    __this->fst_1.num_finals = fst01_num_finals;
    __this->fst_1.words = fst01_words;

    __this->fst_2.states = fst02_states;
    __this->fst_2.num_states = fst02_num_states;
    __this->fst_2.finals = fst02_finals;
    __this->fst_2.num_finals = fst02_num_finals;
    __this->fst_2.words = fst02_words;

    if (domain == 0) {
        Wanson_ASR_Set_Fst(&__this->fst_1);
    } else if (domain == 1) {
        Wanson_ASR_Set_Fst(&__this->fst_2);
    }
}

static int wanson_algorithm_init()
{
    if (Wanson_ASR_Init() < 0) {
        return -1;
    }

    Wanson_ASR_Reset(0);
    return 0;
}

static int wanson_vfs_fwrite(void *file, void *data, u32 len)
{
    cbuffer_t *cbuf = (cbuffer_t *)file;

    u32 wlen = cbuf_write(cbuf, data, len);
    if (wlen != len) {
        cbuf_clear(&__this->mic_cbuf);
        puts("wanson busy!\n");
    }
    if (file == (void *)&__this->mic_cbuf) {
        os_sem_set(&__this->sem, 0);
        os_sem_post(&__this->sem);
    }

    return len;
}

static int wanson_vfs_fclose(void *file)
{
    return 0;
}

static const struct audio_vfs_ops wanson_vfs_ops = {
    .fwrite = wanson_vfs_fwrite,
    .fclose = wanson_vfs_fclose,
};

void aisp_resume(void)
{
    union audio_req req = { 0 };

    Wanson_ASR_Reset(0);

    __this->run_flag = 1;
    os_sem_set(&__this->sem, 0);
    os_sem_post(&__this->sem);

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL);
    req.enc.frame_size = ONCE_SR_POINTS * 2 * req.enc.channel;
    req.enc.volume = CONFIG_AISP_MIC_ADC_GAIN;
    req.enc.sample_rate = __this->sample_rate;
    req.enc.format = "pcm";
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    req.enc.sample_source = "plnk0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    req.enc.sample_source = "plnk1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    req.enc.sample_source = "iis0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    req.enc.sample_source = "iis1";
#else
    req.enc.sample_source = "mic";
#endif
    req.enc.vfs_ops = &wanson_vfs_ops;
    req.enc.output_buf_len = req.enc.frame_size * 3;
    req.enc.file = (FILE *)&__this->mic_cbuf;

#ifdef CONFIG_AEC_ENC_ENABLE
    struct aec_s_attr aec_param = {0};
    aec_param.EnableBit = AEC_MODE_ADVANCE;
    req.enc.aec_attr = &aec_param;
    req.enc.aec_enable = 1;

    extern void get_cfg_file_aec_config(struct aec_s_attr * aec_param);
    get_cfg_file_aec_config(&aec_param);

    if (aec_param.EnableBit == 0) {
        req.enc.aec_enable = 0;
        req.enc.aec_attr = NULL;
    }
    if (aec_param.EnableBit != AEC_MODE_ADVANCE) {
        aec_param.output_way = 0;
    }

#if defined TCFG_AUDIO_ADC_ALL_CHANNEL_OPEN && defined CONFIG_AISP_LINEIN_ADC_CHANNEL && defined CONFIG_AEC_HW_REC_LINEIN_ENABLE
    if (req.enc.aec_enable) {
        aec_param.output_way = 1;	 //1:使用硬件回采 0:使用软件回采
        req.enc.rec_linein_channel_bit_map = BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL); //设置linein的回采通道
        req.enc.channel_bit_map |= BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);	 //配置回采硬件通道
        if (CONFIG_AISP_LINEIN_ADC_CHANNEL < CONFIG_PHONE_CALL_ADC_CHANNEL) {
            req.enc.ch_data_exchange = 1;	 //如果回采通道使用的硬件channel比MIC通道使用的硬件channel靠前的话处理数据时需要交换一下顺序
        }
    }
#endif

    if (req.enc.sample_rate == 16000) {
        aec_param.wideband = 1;
#ifdef CONFIG_AEC_USE_PLAY_MUSIC_ENABLE
        aec_param.hw_delay_offset = 640;
#else
        aec_param.hw_delay_offset = 30;
#endif
    } else {
        aec_param.wideband = 0;
        aec_param.hw_delay_offset = 75;
    }

#ifdef CONFIG_AEC_USE_PLAY_MUSIC_ENABLE
    if (aec_param.output_way == 0) {
        aec_param.dac_ref_sr = 48000; //aec软件回采dac参考采样率
    }
#endif

#endif

    server_request(__this->mic_enc, AUDIO_REQ_ENC, &req);

#if defined CONFIG_AEC_ENC_ENABLE && !defined CONFIG_FPGA_ENABLE
    if (aec_param.output_way) {
#ifdef TCFG_AUDIO_ADC_ALL_CHANNEL_OPEN
        extern void adc_multiplex_set_gain(const char *source, u8 channel_bit_map, u8 gain);
        adc_multiplex_set_gain("linein", BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL), CONFIG_AISP_LINEIN_ADC_GAIN * 2);
#endif
    }
#endif
}

/* unsigned char net_key_test_flag = 0; */
static void wanson_task(void *priv)
{
    short buf[ONCE_SR_POINTS];
    short kws_buf[ASR_FEAME];
    u32 mic_len, linein_len;

    const char *text = NULL;
    float score = 0;

    if (wanson_algorithm_init()) {
        return;
    }
    printf("wanson asr start OK \n");

#if AEC_DATA_TO_SD
    aec_data_to_sd_open();
#endif

    aisp_resume();

    static unsigned char tttt_flag = 0;
    while (1) {
        if (!__this->run_flag) {
            os_sem_pend(&__this->sem, 0);
            continue;
        }
        mic_len = cbuf_read(&__this->mic_cbuf, kws_buf, sizeof(kws_buf));
        if (!mic_len) {
            os_sem_pend(&__this->sem, 0);
            continue;
        }

        if (1 == Wanson_ASR_Recog(kws_buf, ASR_FEAME, &text, &score)) {
            printf("{ ==================wakeup_word: %s}\n", text);
#if AEC_DATA_TO_SD
            aec_data_to_sd_close();
#endif
        }
    }

    Wanson_ASR_Release();

__exit:
    __this->run_flag = 0;
}

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        break;
    default:
        break;
    }
}

void *get_asr_read_input_cb(void)
{
    if (!__this->run_flag) {
        return NULL;
    }

    return NULL;
}

int aisp_open(u16 sample_rate)
{
    union audio_req req = { 0 };

    __this->mic_enc = server_open("audio_server", "enc");
    server_register_event_handler(__this->mic_enc, NULL, enc_server_event_handler);
    cbuf_init(&__this->mic_cbuf, __this->mic_buf, sizeof(__this->mic_buf));
    os_sem_create(&__this->sem, 0);
    __this->sample_rate = sample_rate;

    return thread_fork("wanson", 3, 3 * 1024, 0, &__this->pid, wanson_task, __this);
}

void aisp_suspend(void)
{
    union audio_req req = { 0 };

    __this->run_flag = 0;

    req.enc.cmd = AUDIO_ENC_STOP;
    server_request(__this->mic_enc, AUDIO_REQ_ENC, &req);
    cbuf_clear(&__this->mic_cbuf);
}

#endif

