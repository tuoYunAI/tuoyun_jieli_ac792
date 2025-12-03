#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb_host_mic_node.data.bss")
#pragma data_seg(".usb_host_mic_node.data")
#pragma const_seg(".usb_host_mic_node.text.const")
#pragma code_seg(".usb_host_mic_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "sync/audio_syncts.h"
#include "generic/circular_buf.h"
#include "audio_splicing.h"
#include "audio_config_def.h"
#include "media/audio_general.h"
#include "effects/convert_data.h"
#include "media/audio_dev_sync.h"
#include "media/audio_cfifo.h"
#include "reference_time.h"
#include "app_config.h"
#include "asm/usb.h"


#if (TCFG_HOST_AUDIO_ENABLE)

#define USB_HOST_MIC_LOG_ENABLE          1//0
#if USB_HOST_MIC_LOG_ENABLE
#define LOG_TAG     "[USB_HOST_MIC]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_DEBUG_ENABLE
#include "system/debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#define log_warn(fmt, ...)
#endif

#define UAC_OUT_BUF_MAX_MS   TCFG_AUDIO_DAC_BUFFER_TIME_MS

//硬件通道的状态
#define USB_HOST_MIC_STATE_INIT        0
#define USB_HOST_MIC_STATE_OPEN        1
#define USB_HOST_MIC_STATE_START       2
#define USB_HOST_MIC_STATE_PREPARE     3
#define USB_HOST_MIC_STATE_STOP        4
#define USB_HOST_MIC_STATE_UNINIT

#define AUDIO_CFIFO_IDLE      0
#define AUDIO_CFIFO_INITED    1
#define AUDIO_CFIFO_START     2
#define AUDIO_CFIFO_CLOSE     3

struct audio_usb_host_mic_channel_attr {
    u16 delay_time;            /*usb_host_mic通道延时*/
    u16 protect_time;          /*usb_host_mic延时保护时间*/
    u8 write_mode;             /*usb_host_mic写入模式*/
    u8 dev_properties;         /*关联蓝牙同步的主设备使能，只能有一个节点是主设备*/
} __attribute__((packed));

struct audio_usb_host_mic_channel {
    struct audio_usb_host_mic_channel_attr attr;   /*usb_host_mic通道属性*/
    struct audio_cfifo_channel fifo;        /*usb_host_mic cfifo通道管理*/
    struct usb_host_mic_node_hdl *node_hdl;
    u8 state;                              /*usb_host_mic状态*/
};

struct usb_host_mic_fmt_t {
    u8 usb_id;
    u8 channel;
    u8 bit;
    u32 sample_rate;
};

struct usb_host_mic_hdl {
    struct list_head sync_list;
    struct list_head dev_sync_list;
    struct audio_cfifo cfifo;   /*DAC cfifo结构管理*/
    struct usb_host_mic_fmt_t fmt;
    spinlock_t lock;
    u8 state;                   /*硬件通道的状态*/
    u8 fifo_state;
    u8 iport_bit_width;         //保存输入节点的位宽
    u16 frame_len;
    u8 *out_cbuf;               //uac缓存cbuf
    u8 *frame_buf;              //pull frame后需转换成uac格式的一帧buf
    u32 fade_value;
};

static struct usb_host_mic_hdl usb_host_mic[USB_MAX_HW_NUM];

struct usb_host_mic_node_hdl {
    char name[16];
    struct audio_usb_host_mic_channel usb_host_mic_ch;
    void *dev_sync;
    struct usb_host_mic_hdl *usb_host_mic;
    struct usb_host_mic_hdl *mic;
    struct list_head entry;
    struct audio_usb_host_mic_channel_attr attr;     /*usb_host_mic通道属性*/
    enum stream_scene scene;
    u8 iport_bit_width;     //保存输入节点的位宽
    u8 syncts_enabled;
    u8 start;
    u8 force_write_slience_data_en;
    u8 force_write_slience_data;
    u8 reference_network;
};

struct audio_usb_host_mic_sync_node {
    u8 need_triggered;
    u8 triggered;
    u8 network;
    u32 timestamp;
    void *hdl;
    struct list_head entry;
    void *ch;
};

extern int iis_adapter_link_to_syncts_check(void *syncts);
extern int dac_adapter_link_to_syncts_check(void *syncts);
static int usb_host_mic_tx_handler(void *priv, void *buf, int len);
static int audio_usb_host_mic_channel_write(struct audio_usb_host_mic_channel *ch, void *buf, int len);

static void audio_usb_host_mic_fade_out(struct usb_host_mic_hdl *hdl, void *data, u16 len, u8 ch_num)
{
    if (hdl->iport_bit_width) {
        if (config_media_24bit_enable) {
            hdl->fade_value = jlstream_fade_out_32bit(hdl->fade_value, FADE_GAIN_MAX / (len / 4 / ch_num), (int *)data, len, ch_num);
        }
    } else {
        hdl->fade_value = jlstream_fade_out(hdl->fade_value, FADE_GAIN_MAX / (len / 2 / ch_num), (short *)data, len, ch_num);
    }
}

static void audio_usb_host_mic_fade_in(struct usb_host_mic_hdl *hdl, void *data, u16 len, u8 ch_num)
{
    if (hdl->fade_value < FADE_GAIN_MAX) {
        if (hdl->iport_bit_width) {
            if (config_media_24bit_enable) {
                hdl->fade_value = jlstream_fade_in_32bit(hdl->fade_value, FADE_GAIN_MAX / (len / 4 / ch_num), (s32 *)data, len, ch_num);
            }
        } else {
            hdl->fade_value = jlstream_fade_in(hdl->fade_value, FADE_GAIN_MAX / (len / 2 / ch_num), (s16 *)data, len, ch_num);
        }
    }
}

static int audio_usb_host_mic_data_len(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch)
{
    if (!hdl) {
        return 0;
    }

    int buffered_frames = 0;

    spin_lock(&hdl->lock);

    if (ch->state != AUDIO_CFIFO_START) {
        goto ret_value;
    }

    buffered_frames = audio_cfifo_channel_unread_samples(&ch->fifo);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }

ret_value:
    spin_unlock(&hdl->lock);

    return buffered_frames;
}

static int dev_sync_output_handler(void *priv, void *data, int len)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)priv;
    if (!hdl) {
        return 0;
    }

    return audio_usb_host_mic_channel_write((void *)&hdl->usb_host_mic_ch, data, len);
}

static void dev_sync_start(struct stream_iport *iport)
{
    if (config_dev_sync_enable) {
        struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;
        if (hdl && hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            if (!hdl->dev_sync) {
                struct dev_sync_params params = {0};
                params.in_sample_rate = hdl->usb_host_mic->fmt.sample_rate;
                params.out_sample_rate = hdl->usb_host_mic->fmt.sample_rate;
                params.channel = hdl->usb_host_mic->fmt.channel;
                params.bit_width = iport->prev->fmt.bit_wide;
                params.priv = hdl;
                params.handle = dev_sync_output_handler;
                hdl->dev_sync = dev_sync_open(&params);
                spin_lock(&hdl->usb_host_mic->lock);
                list_add(&hdl->entry, &hdl->usb_host_mic->dev_sync_list);
                spin_unlock(&hdl->usb_host_mic->lock);
            }
        }
    }
}

static int audio_usb_host_mic_add_syncts_with_timestamp(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch, void *syncts, u32 timestamp)
{
    if (!hdl) {
        return 0;
    }

    if (iis_adapter_link_to_syncts_check(syncts)) {
        log_debug("syncts has beed link to iis");
        return 0;
    }
#if TCFG_DAC_NODE_ENABLE
    if (dac_adapter_link_to_syncts_check(syncts)) {
        log_debug("syncts has beed link to dac");
        return 0;
    }
#endif

    struct audio_usb_host_mic_sync_node *node;

    spin_lock(&hdl->lock);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->hdl == (u32)syncts) {
            spin_unlock(&hdl->lock);
            return 0;
        }
    }
    node = (struct audio_usb_host_mic_sync_node *)zalloc(sizeof(struct audio_usb_host_mic_sync_node));
    if (!node) {
        spin_unlock(&hdl->lock);
        return 0;
    }
    node->hdl = syncts;
    node->ch = ch;
    node->timestamp = timestamp;
    node->network = sound_pcm_get_syncts_network(syncts);
    list_add(&node->entry, &hdl->sync_list);
    spin_unlock(&hdl->lock);

    log_debug("add usb_host_mic syncts %x", (u32)syncts);

    return 1;
}

static void audio_pcmmic_remove_syncts_handle(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch, void *syncts)
{
    if (!hdl) {
        return;
    }

    struct audio_usb_host_mic_sync_node *node;

    spin_lock(&hdl->lock);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->ch != (u32)ch)  {
            continue;
        }
        if (node->hdl == syncts) {
            goto __remove_node;
        }
    }
    spin_unlock(&hdl->lock);

    return;

__remove_node:
    list_del(&node->entry);
    spin_unlock(&hdl->lock);
    free(node);

    log_debug("del usb_host_mic syncts %x", (u32)syncts);
}

static int audio_usb_host_mic_syncts_handler(struct stream_iport *iport, int arg)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;
    struct audio_syncts_ioc_params *params = (struct audio_syncts_ioc_params *)arg;

    if (!params) {
        return 0;
    }

    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        if (hdl->attr.dev_properties != SYNC_TO_MASTER_DEV) {
            audio_usb_host_mic_add_syncts_with_timestamp(hdl->usb_host_mic, &hdl->usb_host_mic_ch, (void *)params->data[0], params->data[1]);
        }
        if (hdl->reference_network == 0xff) {
            hdl->reference_network = params->data[2];
        }
        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        audio_pcmmic_remove_syncts_handle(hdl->usb_host_mic, &hdl->usb_host_mic_ch, (void *)params->data[0]);
        break;
    }

    return 0;
}

static void audio_usb_host_mic_syncts_trigger_with_timestamp(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch, u32 timestamp)
{
    if (!hdl) {
        return;
    }

    spin_lock(&hdl->lock);

    struct audio_usb_host_mic_sync_node *node;

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->need_triggered || ((u32)node->ch != (u32)ch)) {
            continue;
        }
        node->need_triggered = 1;
    }

    spin_unlock(&hdl->lock);
}

static void audio_usb_host_mic_force_use_syncts_frames(struct usb_host_mic_hdl *hdl, int frames, u32 timestamp)
{
    if (!hdl) {
        return;
    }

    struct audio_usb_host_mic_sync_node *node;

    spin_lock(&hdl->lock);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->triggered) {
            if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
                u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
                if (!timestamp_enable) {
                    continue;
                }
            }
            int time_diff = node->timestamp - timestamp;
            if (time_diff > 0) {
                continue;
            }
            if (node->hdl) {
                sound_pcm_update_frame_num(node->hdl, frames);
            }
        }
    }
    spin_unlock(&hdl->lock);
}

static int audio_usb_host_mic_channel_fifo_write(struct audio_usb_host_mic_channel *ch, void *data, int len, u8 is_fixed_data)
{
    int w_len;

    if (len == 0) {
        return 0;
    }

    w_len = audio_cfifo_channel_write(&ch->fifo, data, len, is_fixed_data);

    return w_len;
}

static int audio_usb_host_mic_channel_write(struct audio_usb_host_mic_channel *ch, void *buf, int len)
{
    return audio_usb_host_mic_channel_fifo_write(ch, buf, len, 0);
}

static int audio_usb_host_mic_fill_slience_frames(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch, int frames)
{
    if (!hdl) {
        return 0;
    }

    u32 point_offset = hdl->iport_bit_width ? 2 : 1;

    int wlen = audio_usb_host_mic_channel_fifo_write(ch, (void *)0, frames * hdl->fmt.channel << point_offset, 1);

    return (wlen >> point_offset) / hdl->fmt.channel;
}

__attribute__((always_inline))
static int audio_usb_host_mic_update_syncts_frames(struct usb_host_mic_hdl *hdl, int frames)
{
    struct audio_usb_host_mic_sync_node *node;

    u32 point_offset = hdl->iport_bit_width ? 2 : 1;

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->triggered) {
            continue;
        }
        if (!node->need_triggered) {
            continue;
        }

        if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
            u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
            if (!timestamp_enable) {
                continue;
            }
        }
        sound_pcm_syncts_latch_trigger(node->hdl);
        node->triggered = 1;
    }

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->triggered) {
            continue;
        }
        /*
         * 这里需要将其他通道延时数据大于音频同步计数通道的延迟数据溢出值通知到音频同步
         * 锁存模块，否则会出现硬件锁存不同通道数据的溢出误差
         */
        int diff_samples = audio_cfifo_channel_unread_diff_samples(&((struct audio_usb_host_mic_channel *)node->ch)->fifo);
        sound_pcm_update_overflow_frames(node->hdl, diff_samples);

        sound_pcm_update_frame_num(node->hdl, frames);
        if (node->network == AUDIO_NETWORK_LOCAL && (((struct audio_usb_host_mic_channel *)node->ch)->state == AUDIO_CFIFO_START)) {
            if (audio_syncts_latch_enable(node->hdl)) {
                u32 time = audio_jiffies_usec();
                sound_pcm_update_frame_num_and_time(node->hdl, 0, time, 0);
            }
        }
    }

    return 0;
}

static void audio_usb_host_mic_channel_read(struct usb_host_mic_hdl *hdl, s16 *addr, int len, u8 ch_num)
{
    u32 point_offset = hdl->iport_bit_width ? 2 : 1;

    if (hdl->fifo_state == AUDIO_CFIFO_INITED) {
        u32 rlen = audio_cfifo_read_data(&hdl->cfifo, addr, len);
        if (rlen) {
            /* putchar('R'); */
            audio_usb_host_mic_update_syncts_frames(hdl, (rlen >> point_offset) / ch_num);
            if (rlen < len) {
                audio_usb_host_mic_fade_out(hdl, addr, rlen, ch_num);
                memset((void *)((u32)addr + rlen), 0x0, len - rlen);
            } else {
                audio_usb_host_mic_fade_in(hdl, addr, len, ch_num);
            }
        } else {
            putchar('M');
            audio_usb_host_mic_fade_out(hdl, addr, len, ch_num);
            /* puts("usb_host_mic_empty\n"); */
        }
    }
}

static bool audio_usb_host_mic_is_working(struct usb_host_mic_hdl *hdl)
{
    if (!hdl) {
        return FALSE;
    }
    if ((hdl->state != USB_HOST_MIC_STATE_OPEN) && (hdl->state != USB_HOST_MIC_STATE_START)) {
        return FALSE;
    }

    return TRUE;
}

static int audio_usb_host_mic_new_channel(struct usb_host_mic_node_hdl *hdl, struct audio_usb_host_mic_channel *ch)
{
    if (!hdl || !ch) {
        return -EINVAL;
    }

    memset(ch, 0x0, sizeof(struct audio_usb_host_mic_channel));
    ch->node_hdl = hdl;
    ch->state = AUDIO_CFIFO_IDLE;

    return 0;
}

static void audio_usb_host_mic_channel_set_attr(struct audio_usb_host_mic_channel *ch, void *attr)
{
    if (ch) {
        if (!attr) {
            return;
        }
        memcpy(&ch->attr, attr, sizeof(struct audio_usb_host_mic_channel_attr));
    }
}

static void audio_usb_host_mic_channel_start(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch)
{
    if (!hdl) {
        return;
    }

    spin_lock(&hdl->lock);

    if (!audio_usb_host_mic_is_working(hdl)) {
        hdl->fade_value = 0;
        hdl = hdl;
        hdl->state = USB_HOST_MIC_STATE_OPEN;
        hdl->fifo_state = AUDIO_CFIFO_IDLE;
    }

    if (hdl->fifo_state != AUDIO_CFIFO_INITED) {
        u32 point_size = (!hdl->iport_bit_width) ? 2 : 4;
        u32 usb_host_mic_buf_size = UAC_OUT_BUF_MAX_MS * ((AUDIO_DAC_MAX_SAMPLE_RATE + 999) / 1000) * hdl->fmt.channel * point_size; //20ms延时的buf
        if (!hdl->out_cbuf) {
            hdl->out_cbuf = malloc(usb_host_mic_buf_size);
        }

        hdl->cfifo.bit_wide = hdl->iport_bit_width;
        audio_cfifo_init(&hdl->cfifo, hdl->out_cbuf, usb_host_mic_buf_size, hdl->fmt.sample_rate, hdl->fmt.channel);
        hdl->fifo_state = AUDIO_CFIFO_INITED;
    }
    if (hdl->state != USB_HOST_MIC_STATE_START) {
        audio_cfifo_channel_add(&hdl->cfifo, &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
        ch->state = AUDIO_CFIFO_START;
        hdl->state = USB_HOST_MIC_STATE_START;
    } else {
        if (ch->state != AUDIO_CFIFO_START) {
            audio_cfifo_channel_add(&hdl->cfifo, &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
            ch->state = AUDIO_CFIFO_START;
        }
    }
    spin_unlock(&hdl->lock);
}

static int audio_usb_host_mic_channel_close(struct usb_host_mic_hdl *hdl, struct audio_usb_host_mic_channel *ch)
{
    if (!hdl) {
        return 0;
    }

    spin_lock(&hdl->lock);

    if (ch->state != AUDIO_CFIFO_IDLE && ch->state != AUDIO_CFIFO_CLOSE) {
        if (ch->state == AUDIO_CFIFO_START) {
            audio_cfifo_channel_del(&ch->fifo);
            ch->state = AUDIO_CFIFO_CLOSE;
        }
    }
    if (hdl->fifo_state != AUDIO_CFIFO_IDLE) {
        if (audio_cfifo_channel_num(&hdl->cfifo) == 0) {
            hdl->state = USB_HOST_MIC_STATE_STOP;
            hdl->fifo_state = AUDIO_CFIFO_CLOSE;
            set_usb_host_mic_recorder_tx_handler(hdl->fmt.usb_id, NULL, NULL);
            spin_unlock(&hdl->lock);
            if (hdl->out_cbuf) {
                free(hdl->out_cbuf);
                hdl->out_cbuf = NULL;
            }
            if (hdl->frame_buf) {
                free(hdl->frame_buf);
                hdl->frame_buf = NULL;
            }
            hdl->fade_value = 0;
            return 0;
        }
    }

    spin_unlock(&hdl->lock);

    return 0;
}

static int usb_host_mic_adpater_detect_timestamp(struct usb_host_mic_node_hdl *hdl, struct stream_frame *frame)
{
    int diff = 0;
    u32 current_time = 0;

    if (frame->flags & FRAME_FLAG_SYS_TIMESTAMP_ENABLE) {
        // sys timestamp
        return 0;
    }

    if (!(frame->flags & FRAME_FLAG_TIMESTAMP_ENABLE) || hdl->force_write_slience_data_en) {
        if (!hdl->force_write_slience_data) { //无播放同步时，强制填一段静音包
            hdl->force_write_slience_data = 1;
            int slience_time_us = (hdl->attr.protect_time ? hdl->attr.protect_time : 8) * 1000;
            int slience_frames = (u64)slience_time_us * hdl->usb_host_mic->fmt.sample_rate / 1000000;
            audio_usb_host_mic_fill_slience_frames(hdl->usb_host_mic, &hdl->usb_host_mic_ch, slience_frames);
        }
        hdl->syncts_enabled = 1;
        return 0;
    }

    if (hdl->syncts_enabled) {
        audio_usb_host_mic_syncts_trigger_with_timestamp(hdl->usb_host_mic, &hdl->usb_host_mic_ch, frame->timestamp);
        return 0;
    }

    if (hdl->reference_network == AUDIO_NETWORK_LOCAL) {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_usb_host_mic_data_len(hdl->usb_host_mic, &hdl->usb_host_mic_ch), hdl->usb_host_mic->fmt.sample_rate);
    } else {
        u32 reference_time = 0;
        u8 net_addr[6];
        u8 network = audio_reference_clock_network(net_addr);
        if (network != ((u8) - 1)) {
            reference_time = audio_reference_clock_time();
            if (reference_time == (u32) - 1) {
                goto syncts_start;
            }
            if (network == 2) {
                current_time = reference_time * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_usb_host_mic_data_len(hdl->usb_host_mic, &hdl->usb_host_mic_ch), hdl->usb_host_mic->fmt.sample_rate);
            } else {
                current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_usb_host_mic_data_len(hdl->usb_host_mic, &hdl->usb_host_mic_ch), hdl->usb_host_mic->fmt.sample_rate);
            }
        }
    }

    diff = frame->timestamp - current_time;
    diff /= TIMESTAMP_US_DENOMINATOR;
    if (diff < 0) {
        if (__builtin_abs(diff) <= 1000) {
            goto syncts_start;
        }
        log_debug("ts %u cur_t %u, diff_us %d", frame->timestamp, current_time, diff);
        log_debug("dump frame, %s network %d", hdl->name, hdl->reference_network);
        return 2;
    }

    int slience_frames = (u64)diff * hdl->usb_host_mic->fmt.sample_rate / 1000000;
    int filled_frames = audio_usb_host_mic_fill_slience_frames(hdl->usb_host_mic, &hdl->usb_host_mic_ch, slience_frames);
    if (filled_frames < slience_frames) {
        return 1;
    }

syncts_start:
    log_debug("%s, ts %u cur_t %u, diff_us %d", hdl->name, frame->timestamp, current_time, diff);
    hdl->syncts_enabled = 1;
    audio_usb_host_mic_syncts_trigger_with_timestamp(hdl->usb_host_mic, &hdl->usb_host_mic_ch, frame->timestamp);

    return 0;
}

static void pcm_data_convert_to_uac_data(struct usb_host_mic_hdl *hdl, void *in, void *out, int pcm_frames)
{
    if (hdl->fmt.bit == 16) {
        if (hdl->iport_bit_width) {
            audio_convert_data_32bit_to_16bit_round((s32 *)in, (s16 *)out, pcm_frames);
        }
    } else if (hdl->fmt.bit == 24) {
        if (hdl->iport_bit_width) {
            audio_convert_data_4byte24bit_to_3byte24bit((s32 *)in, (s32 *)out, pcm_frames);
        } else {
            audio_convert_data_16bit_to_3byte24bit((s16 *)in, (s32 *)out, pcm_frames);
        }
    }
}

static int usb_host_mic_tx_handler(void *priv, void *buf, int len)
{
    struct usb_host_mic_hdl *hdl = (struct usb_host_mic_hdl *)priv;
    if (!hdl) {
        return 0;
    }
    if (!len) {
        return 0;
    }
    int rlen = len;

    spin_lock(&hdl->lock);

    if (hdl->fmt.bit == 16 && !hdl->iport_bit_width) {//16->16
        audio_usb_host_mic_channel_read(hdl, (s16 *)buf, rlen, hdl->fmt.channel);
        if (config_dev_sync_enable) {
            struct usb_host_mic_node_hdl *node_hdl;
            list_for_each_entry(node_hdl, &hdl->dev_sync_list, entry) {
                if (node_hdl->dev_sync) {
                    dev_sync_calculate_output_samplerate(node_hdl->dev_sync, len, 0);
                }
            }
        }
    } else {
        u32 point_offset = hdl->iport_bit_width ? 2 : 1;
        int pcm_frames;
        if (hdl->fmt.bit == 16) {
            pcm_frames = (rlen / 2) / hdl->fmt.channel;
        } else {
            pcm_frames = (rlen * 8 / 24) / hdl->fmt.channel;
        }
        rlen = (pcm_frames << point_offset) * hdl->fmt.channel;
        if (rlen > hdl->frame_len) {
            if (hdl->frame_buf) {
                free(hdl->frame_buf);
                hdl->frame_buf = NULL;
            }
        }
        if (!hdl->frame_buf) {
            hdl->frame_buf = zalloc(rlen);
            hdl->frame_len = rlen;
        }

        audio_usb_host_mic_channel_read(hdl, (s16 *)hdl->frame_buf, rlen, hdl->fmt.channel);
        pcm_data_convert_to_uac_data(hdl, hdl->frame_buf, buf, pcm_frames * hdl->fmt.channel);

        if (config_dev_sync_enable) {
            struct usb_host_mic_node_hdl *node_hdl;
            list_for_each_entry(node_hdl, &hdl->dev_sync_list, entry) {
                if (node_hdl->dev_sync) {
                    dev_sync_calculate_output_samplerate(node_hdl->dev_sync, rlen, 0);
                }
            }
        }
    }
    spin_unlock(&hdl->lock);

    return len;
}

static int usb_host_mic_channel_latch_time(struct usb_host_mic_node_hdl *hdl, u32 *latch_time, u32(*get_time)(u32 reference_network), u32 reference_network)
{
    spin_lock(&hdl->usb_host_mic->lock);
    *latch_time = get_time(reference_network);
    spin_unlock(&hdl->usb_host_mic->lock);
    int buffered_frames = audio_usb_host_mic_data_len(hdl->usb_host_mic, &hdl->usb_host_mic_ch);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }
    return buffered_frames;
}

static void usb_host_mic_synchronize_with_main_device(struct stream_iport *iport, struct stream_frame *frame)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;

    if (hdl->dev_sync) {
        struct sync_rate_params params = {0};
        params.d_sample_rate = frame->d_sample_rate;
        params.buffered_frames = usb_host_mic_channel_latch_time(hdl, &params.current_time, dev_sync_latch_time, hdl->reference_network);
        params.timestamp = frame->timestamp;
        params.name = hdl->name;
        dev_sync_update_rate(hdl->dev_sync, &params);
    }
}

// 数据流节点回调，做数据缓存
__NODE_CACHE_CODE(usb_host_mic)
static void usb_host_mic_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;
    struct stream_frame *frame;
    int wlen = 0;

    if (hdl->start == 0) {
        return;
    }

    audio_usb_host_mic_channel_start(hdl->usb_host_mic, &hdl->usb_host_mic_ch);

    dev_sync_start(iport);

    while (1) {
        frame = jlstream_pull_frame(iport, NULL);
        if (!frame) {
            break;
        }

        if (config_dev_sync_enable) {
            if (frame->offset == 0) {
                usb_host_mic_synchronize_with_main_device(iport, frame);
            }
        }

        int err = usb_host_mic_adpater_detect_timestamp(hdl, frame);
        if (err == 1) {
            jlstream_return_frame(iport, frame);
            break;
        }

        if (err == 2) { //需要直接丢弃改帧数据
            u32 point_offset = hdl->iport_bit_width ? 2 : 1;
            audio_usb_host_mic_force_use_syncts_frames(hdl->usb_host_mic, (frame->len >> point_offset) / hdl->usb_host_mic->fmt.channel, frame->timestamp);
            jlstream_free_frame(frame);
            continue;
        }
        s16 *data = (s16 *)(frame->data + frame->offset);
        u32 remain = frame->len - frame->offset;

        if (hdl->dev_sync) {
            if (config_dev_sync_enable) {
                wlen = dev_sync_write(hdl->dev_sync, data, remain);
            }
        } else {
            wlen = audio_usb_host_mic_channel_write(&hdl->usb_host_mic_ch, data, remain);
        }
        frame->offset += wlen;
        if (wlen < remain) {
            note->state |= NODE_STA_OUTPUT_BLOCKED;
            jlstream_return_frame(iport, frame);
            break;
        }

        jlstream_free_frame(frame);
    }
}

static void usb_host_mic_ioc_start(struct usb_host_mic_node_hdl *hdl)
{
    hdl->iport_bit_width = hdl_node(hdl)->iport->prev->fmt.bit_wide;

    audio_usb_host_mic_new_channel(hdl, (void *)&hdl->usb_host_mic_ch);
    audio_usb_host_mic_channel_set_attr(&hdl->usb_host_mic_ch, &hdl->attr);

    spin_lock(&hdl->usb_host_mic->lock);
    hdl->start = 1;
    hdl->usb_host_mic->iport_bit_width = audio_general_out_dev_bit_width();
    spin_unlock(&hdl->usb_host_mic->lock);

    log_debug("%s, usb_host_mic_hdl.iport_bit_width %d", hdl->name, hdl->usb_host_mic->iport_bit_width);
}

static void usb_host_mic_ioc_stop(struct usb_host_mic_node_hdl *hdl)
{
    hdl->start = 0;
    hdl->syncts_enabled = 0;
    hdl->force_write_slience_data = 0;

    audio_usb_host_mic_channel_close(hdl->usb_host_mic,  &hdl->usb_host_mic_ch);

    if (config_dev_sync_enable) {
        if (hdl->dev_sync) {
            spin_lock(&hdl->usb_host_mic->lock);
            list_del(&hdl->entry);
            spin_unlock(&hdl->usb_host_mic->lock);
            dev_sync_close(hdl->dev_sync);
            hdl->dev_sync = NULL;
        }
    }

    log_debug("%s ioc stop", hdl->name);
}

static void usb_host_mic_adapter_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = usb_host_mic_handle_frame;
}

static int usb_host_mic_ioc_negotiate(struct stream_iport *iport)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    int ret = NEGO_STA_ACCPTED;
    u8 channel_mode;

    if (audio_general_out_dev_bit_width()) {
        if (in_fmt->bit_wide != DATA_BIT_WIDE_24BIT) {
            in_fmt->bit_wide = DATA_BIT_WIDE_24BIT;
            ret = NEGO_STA_CONTINUE;
        }
    } else {
        if (in_fmt->bit_wide != DATA_BIT_WIDE_16BIT) {
            in_fmt->bit_wide = DATA_BIT_WIDE_16BIT;
            ret = NEGO_STA_CONTINUE;
        }
    }

    if (in_fmt->sample_rate != hdl->usb_host_mic->fmt.sample_rate) {
        in_fmt->sample_rate = hdl->usb_host_mic->fmt.sample_rate;
        ret = NEGO_STA_CONTINUE;
    }

    if (hdl->usb_host_mic->fmt.channel == 2) {
        channel_mode = AUDIO_CH_LR;
    } else {
        channel_mode = AUDIO_CH_DIFF;
    }

    if (in_fmt->channel_mode != channel_mode) {
        if (channel_mode == AUDIO_CH_DIFF) {
            if (in_fmt->channel_mode != AUDIO_CH_L &&
                in_fmt->channel_mode != AUDIO_CH_R &&
                in_fmt->channel_mode != AUDIO_CH_MIX) {
                in_fmt->channel_mode = AUDIO_CH_MIX;
                ret = NEGO_STA_CONTINUE;
            }
        } else {
            in_fmt->channel_mode = channel_mode;
            ret = NEGO_STA_CONTINUE;
        }
    }

    log_debug("## Func: %s, negotiate_state:%d, output_rate:%d, channel_mode:%d, sr:%d, bit_width %d", __func__, ret, in_fmt->sample_rate, channel_mode, in_fmt->sample_rate, in_fmt->bit_wide);

    return ret;
}

static int audio_usb_host_mic_buffer_delay_time(struct stream_iport *iport)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;
    if (!hdl) {
        return 0;
    }

    u32 points_per_ch = audio_usb_host_mic_data_len(hdl->usb_host_mic, &hdl->usb_host_mic_ch);
    int rate = hdl->usb_host_mic->fmt.sample_rate;
    ASSERT(rate != 0);

    return (points_per_ch * 10000) / rate;//10000:表示ms*10
}

static int usb_host_mic_ioc_set_fmt(struct usb_host_mic_node_hdl *hdl, struct stream_fmt *fmt)
{
#if USB_MAX_HW_NUM > 1
    hdl->usb_host_mic = fmt->chconfig_id ? &usb_host_mic[1] : &usb_host_mic[0];
#else
    hdl->usb_host_mic = &usb_host_mic[0];
#endif
    hdl->usb_host_mic->fmt.channel = fmt->channel_mode == AUDIO_CH_LR ? 2 : 1;
    hdl->usb_host_mic->fmt.bit = fmt->bit_wide == DATA_BIT_WIDE_24BIT ? 24 : 16;
    hdl->usb_host_mic->fmt.sample_rate = fmt->sample_rate;
    hdl->usb_host_mic->fmt.usb_id = fmt->chconfig_id;

    set_usb_host_mic_recorder_tx_handler(hdl->usb_host_mic->fmt.usb_id, (void *)hdl->usb_host_mic, usb_host_mic_tx_handler);

    return 0;
}

static int usb_host_mic_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        usb_host_mic_adapter_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= usb_host_mic_ioc_negotiate(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_SET_FMT:
        usb_host_mic_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_GET_DELAY:
        return audio_usb_host_mic_buffer_delay_time(iport);
    case NODE_IOC_START:
        usb_host_mic_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        usb_host_mic_ioc_stop(hdl);
        break;
    case NODE_IOC_SYNCTS:
        audio_usb_host_mic_syncts_handler(iport, arg);
        break;
    case NODE_IOC_SET_PARAM:
        hdl->reference_network = arg;
        break;
    case NODE_IOC_SET_PRIV_FMT: //手动控制是否预填静音包
        hdl->force_write_slience_data_en = arg;
        break;
    default:
        break;
    }

    return ret;
}

static int usb_host_mic_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct usb_host_mic_node_hdl *hdl = (struct usb_host_mic_node_hdl *)node->private_data;

    int ret = jlstream_read_node_data_new(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (void *)&hdl->attr, hdl->name);
    if (!ret) {
        hdl->attr.delay_time = 50;
        hdl->attr.protect_time = 8;
        hdl->attr.write_mode = WRITE_MODE_BLOCK;
        log_warn("usb_host_mic node param read err, set default");
    }
    hdl->reference_network = 0xff;

    return 0;
}

static void usb_host_mic_adapter_release(struct stream_node *node)
{

}

REGISTER_STREAM_NODE_ADAPTER(usb_host_mic_node_adapter) = {
    .name       = "usb_host_mic",
    .uuid       = NODE_UUID_USB_HOST_MIC,
    .bind       = usb_host_mic_adapter_bind,
    .ioctl      = usb_host_mic_adapter_ioctl,
    .release    = usb_host_mic_adapter_release,
    .hdl_size   = sizeof(struct usb_host_mic_node_hdl),
};

static int usb_host_mic_handle_init(void)
{
    INIT_LIST_HEAD(&usb_host_mic[0].sync_list);
    INIT_LIST_HEAD(&usb_host_mic[0].dev_sync_list);
    spin_lock_init(&usb_host_mic[0].lock);

#if USB_MAX_HW_NUM > 1
    INIT_LIST_HEAD(&usb_host_mic[1].sync_list);
    INIT_LIST_HEAD(&usb_host_mic[1].dev_sync_list);
    spin_lock_init(&usb_host_mic[1].lock);
#endif

    return 0;
}
early_initcall(usb_host_mic_handle_init);

#endif
