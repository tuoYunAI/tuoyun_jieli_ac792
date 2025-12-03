#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_spk_file.data.bss")
#pragma data_seg(".pc_spk_file.data")
#pragma const_seg(".pc_spk_file.text.const")
#pragma code_seg(".pc_spk_file.text")
#endif
#include "source_node.h"
#include "media/audio_splicing.h"
#include "cvp/cvp_common.h"
#include "audio_config.h"
#include "jlstream.h"
#include "effects/effects_adj.h"
#include "generic/circular_buf.h"
#include "system/timer.h"
#include "event/device_event.h"
#include "app_config.h"

#if TCFG_USB_SLAVE_AUDIO_ENABLE && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#endif

#include "uac_stream.h"

//足够缓存20ms的数据
#define PC_SPK_CACHE_BUF_LEN   ((SPK_AUDIO_RATE/1000) * 4 * 20)

/* PC SPK 在线检测 */
#define PC_SPK_ONLINE_DET_EN   1
#define PC_SPK_ONLINE_DET_TIME 10		//50->20

#define SPK_PUSH_FRAME_NUM        5 //SPK一次push的帧数，单位：uac rx中断间隔
#define SPK_REPAIR_PACKET_ENABLE  0// pc_spk丢包修复使能,数据流内需在播放同步前接入plc node
#define SPK_LOST_DATA_AUTO_CLOSE  1


struct pc_spk_fmt_t {
    u8 channel;
    u8 bit;
    u8 id;
    u32 sample_rate;
};

struct pc_spk_file_hdl {
    void *source_node;
    struct stream_node *node;
    cbuffer_t spk_cache_cbuffer;
    int sr;
    u32 timestamp;
    u32 irq_cnt;    //进中断++，用来给定时器判断是否中断没有起
    u8 *cache_buf;
    u16 prev_len;
    u16 det_timer_id;
    u8 start;
    u8 data_run;
    u8 det_mute;
    u8 repair_flag;
    struct pc_spk_fmt_t fmt;
};

#define SPK_PUSH_FRAME_NUM 5 //SPK一次push的帧数，单位：uac rx中断间隔

static u8 mute_status;

bool pc_spk_player_mute_status(void)
{
    return mute_status ? TRUE : FALSE;
}

static void pc_spk_data_isr_cb(void *priv, void *buf, int len)
{
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)priv;
    struct stream_frame *frame;

#if SPK_LOST_DATA_AUTO_CLOSE
    if (mute_status) {
        if (mute_status != 0xff) {
            struct device_event event = {0};
            event.event = USB_AUDIO_PLAY_OPEN;
            event.arg = (void *)(int)mute_status - 1;
            device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
            mute_status = 0xff;
        }
        return;
    }
#endif

    if (!hdl) {
        return;
    }

    if (!hdl->start) {
        return;
    }

#if SPK_REPAIR_PACKET_ENABLE
    if (!len) {
        hdl->repair_flag = 1;
        len = hdl->prev_len;
    } else {
        hdl->prev_len = len;
    }
#else
    if (!len) { //增加0长帧的过滤，避免引起后续节点的写异常
        return;
    }
#endif

    struct stream_node  *source_node = hdl->source_node;

    if (!hdl->cache_buf) {
        int cache_buf_len = len * SPK_PUSH_FRAME_NUM * 4; //4块输出buf
        //申请cbuffer
        hdl->cache_buf = zalloc(cache_buf_len);
        if (hdl->cache_buf) {
            cbuf_init(&hdl->spk_cache_cbuffer, hdl->cache_buf, cache_buf_len);
        }
    }

    if (cbuf_get_data_len(&hdl->spk_cache_cbuffer) == 0) {
        hdl->timestamp = audio_jiffies_usec();
    }

    hdl->irq_cnt++;

    //1ms 起一次中断，一次长度192, 中断太快,需缓存
    int wlen = cbuf_write(&hdl->spk_cache_cbuffer, buf, len);
    if (wlen != len) {
        /*putchar('W');*/
    }

    u32 cache_len = cbuf_get_data_len(&hdl->spk_cache_cbuffer);
    if (cache_len >= len * SPK_PUSH_FRAME_NUM) {
        frame = source_plug_get_output_frame(source_node, cache_len);
        if (!frame) {
            return;
        }
        frame->len    = cache_len;
#if 1
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = hdl->timestamp * TIMESTAMP_US_DENOMINATOR;
#else
        frame->flags  = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp = hdl->timestamp;
#endif

        cbuf_read(&hdl->spk_cache_cbuffer, frame->data, frame->len);

#if SPK_REPAIR_PACKET_ENABLE
        if (hdl->repair_flag) { //补包标志
            hdl->repair_flag = 0;
            frame->flags |= FRAME_FLAG_FILL_PACKET;
            memset(frame->data, frame->len, 0x0);
        }
#endif
        source_plug_put_output_frame(source_node, frame);
        hdl->data_run = 1;
    }
}

/* 定时器检测 pcspk 在线 */
static void pcspk_det_timer_cb(void *priv)
{
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)priv;

    if (hdl && hdl->start) {
        if (hdl->irq_cnt) {
            hdl->irq_cnt = 0;
            if (hdl->det_mute) {
#if SPK_LOST_DATA_AUTO_CLOSE
                return;
#endif
                hdl->det_mute = 0;
                mute_status = FALSE;
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && TCFG_IIS_NODE_ENABLE
                //先开pc mic，后开spk，需要取消忽略外部数据，重启aec
                if (audio_aec_status()) {
                    audio_aec_reboot(0);
                    audio_cvp_ref_data_align_reset();
                }
#endif
            }
        } else {
            if (hdl->data_run && !hdl->det_mute) {
                hdl->det_mute = 1;
                mute_status = BIT(hdl->fmt.id);
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && TCFG_IIS_NODE_ENABLE
                if (audio_aec_status()) {
                    //忽略参考数据
                    audio_cvp_ioctl(CVP_OUTWAY_REF_IGNORE, 1, NULL);
                    audio_cvp_ref_data_align_reset();
                }
#endif
                //已经往后面推数据突然中断没有起的情况
                printf(">>>>>>> PCSPK LOST CONNECT <<<<<<<");
                //user_apm_mute(1);
                hdl->data_run = 0;
#if !SPK_LOST_DATA_AUTO_CLOSE
                return;
#endif
#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)
                if (get_broadcast_role() == 1) {
                    //广播（发送端）
                    /* printf("pc spk lost audio stream, broadcast audio need suspend!"); */
                    /* pc_mode_broadcast_deal_by_taskq(BROADCAST_MUSIC_STOP); */
                } else
#endif
                {
                    struct device_event event = {0};
                    event.event = USB_AUDIO_PLAY_CLOSE;
                    event.arg = (void *)(int)hdl->fmt.id;
                    device_event_notify(DEVICE_EVENT_FROM_UAC, &event);
                }
            }
        }
    }
}

static void *pc_spk_file_init(void *source_node, struct stream_node *node)
{
    struct pc_spk_file_hdl *hdl = zalloc(sizeof(*hdl));
    if (!hdl) {
        return NULL;
    }

    node->type |= NODE_TYPE_IRQ;
    hdl->source_node = source_node;
    hdl->node = node;

    return hdl;
}

static int pc_spk_ioc_get_fmt(struct pc_spk_file_hdl *hdl, struct stream_fmt *fmt)
{
    fmt->coding_type = AUDIO_CODING_PCM;
    if (hdl->fmt.channel == 2) {
        fmt->channel_mode = AUDIO_CH_LR;
    } else {
        fmt->channel_mode = AUDIO_CH_L;
    }

    fmt->sample_rate = hdl->fmt.sample_rate;
    fmt->bit_wide = (hdl->fmt.bit == 24) ? DATA_BIT_WIDE_24BIT : DATA_BIT_WIDE_16BIT;

    return 0;
}

static int pc_spk_ioc_get_fmt_ex(struct pc_spk_file_hdl *hdl, struct stream_fmt_ex *fmt)
{
    fmt->pcm_24bit_type = (hdl->fmt.bit == 24) ? PCM_24BIT_DATA_3BYTE : PCM_24BIT_DATA_4BYTE;
    return 1;
}

static int pc_spk_ioc_set_fmt(struct pc_spk_file_hdl *hdl, struct stream_fmt *fmt)
{
    hdl->fmt.channel = fmt->channel_mode == AUDIO_CH_LR ? 2 : 1;
    hdl->fmt.bit = fmt->bit_wide == DATA_BIT_WIDE_24BIT ? 24 : 16;
    hdl->fmt.sample_rate = fmt->sample_rate;
    hdl->fmt.id = fmt->chconfig_id;

    set_uac_speaker_rx_handler(fmt->chconfig_id, hdl, pc_spk_data_isr_cb);

    return 0;
}

//打开pcspk 在线检测定时器
static void pcspk_open_det_timer(struct pc_spk_file_hdl *hdl)
{
    if (!hdl->det_timer_id) {
        hdl->det_timer_id = usr_timer_add(hdl, pcspk_det_timer_cb, PC_SPK_ONLINE_DET_TIME, 0);
    }
}

//关闭 pcspk 在线检测定时器
static void pcspk_close_det_timer(struct pc_spk_file_hdl *hdl)
{
    if (hdl->det_timer_id) {
        usr_timer_del(hdl->det_timer_id);
        hdl->det_timer_id = 0;
    }
}

static int pc_spk_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_GET_FMT:
        pc_spk_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_GET_FMT_EX:
        ret = pc_spk_ioc_get_fmt_ex(hdl, (struct stream_fmt_ex *)arg);
        break;
    case NODE_IOC_SET_FMT:
        pc_spk_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        break;
    case NODE_IOC_GET_PARAM:
        *(u32 *)arg = (u32)hdl;
        break;
    case NODE_IOC_START:
        if (hdl->start == 0) {
#if PC_SPK_ONLINE_DET_EN
            pcspk_open_det_timer(hdl);
#endif
            hdl->data_run = 0;
            hdl->det_mute = 0;
            hdl->start = 1;
            mute_status = 0;
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
        }
        /* mute_status = 0; */
        break;
    }

    return ret;
}

static void pc_spk_release(void *_hdl)
{
    struct pc_spk_file_hdl *hdl = (struct pc_spk_file_hdl *)_hdl;

    if (!hdl) {
        return;
    }

#if PC_SPK_ONLINE_DET_EN
    pcspk_close_det_timer(hdl);
#endif
#if SPK_LOST_DATA_AUTO_CLOSE
    if (!hdl->det_mute)
#endif
        set_uac_speaker_rx_handler(hdl->fmt.id, NULL, NULL);
    free(hdl->cache_buf);
    free(hdl);
}

REGISTER_SOURCE_NODE_PLUG(pc_spk_file_plug) = {
    .uuid       = NODE_UUID_PC_SPK,
    .init       = pc_spk_file_init,
    .ioctl      = pc_spk_ioctl,
    .release    = pc_spk_release,
};

#endif

