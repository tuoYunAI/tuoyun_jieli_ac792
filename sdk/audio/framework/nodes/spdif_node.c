#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spdif_node.data.bss")
#pragma data_seg(".spdif_node.data")
#pragma const_seg(".spdif_node.text.const")
#pragma code_seg(".spdif_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "sync/audio_syncts.h"
#include "circular_buf.h"
#include "audio_splicing.h"
#include "app_config.h"
#include "gpio.h"
#include "audio_cvp.h"
#include "media/audio_general.h"
#include "reference_time.h"
#include "audio_config_def.h"
#include "effects/effects_adj.h"
#include "media/audio_dev_sync.h"

#if TCFG_SPDIF_MASTER_NODE_ENABLE

#include "audio_spdif_master.h"     ///to compile

#define SPDIF_DEBUG 0

struct spdif_node_hdl {
    char name[16];
    u8 spdif_start; //节点start 标记
    u8 spdif_init;
    u8 syncts_enabled;
    u8 nch;
    u8 force_write_slience_data_en;
    u8 force_write_slience_data;
    u32 sample_rate;
    struct stream_frame *frame;
    enum stream_scene scene;
    struct audio_spdif_channel spdif_ch;
    struct audio_spdif_channel_attr attr;
    void *dev_sync;
    u8 timestamp_ok;
    u8 reference_network;
    u8 time_out_ts_not_align;//超时的时间戳是否需要丢弃
};

extern const int config_media_tws_en;
extern struct audio_spdif_hdl spdif_hdl;

static u8 scene_cfg_index_get(u8 scene)
{
    //按可视化工具中的配置项顺序排列
    switch (scene) {
    case STREAM_SCENE_A2DP:
        return 1;
    case STREAM_SCENE_LINEIN:
        return 2;
    case STREAM_SCENE_MUSIC:
        return 3;
    case STREAM_SCENE_FM:
        return 4;
    case STREAM_SCENE_SPDIF:
        return 5;
    default:
        return 0;
    }
}

/* 获取可视化工具配置的参数 */
static void spdif_node_param_init(struct spdif_node_hdl *hdl,
                                  struct stream_node *node)
{
    /*
     *获取配置文件内的参数,及名字
     * */
    u8 cfg_index = scene_cfg_index_get(hdl->scene);
    int ret = jlstream_read_node_data_by_cfg_index(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, cfg_index, (void *)&hdl->attr, hdl->name);
    if (!ret) {
        ret = jlstream_read_node_data_by_cfg_index(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, 0, (void *)&hdl->attr, hdl->name);
    }

    //参数配置
    if (!ret) {
        //获取不到配置参数
        hdl->attr.delay_time = 50;
        hdl->attr.protect_time = 8;
        hdl->attr.write_mode = WRITE_MODE_BLOCK;
    } else {
        printf("\n delay time %d pro time %d \n", hdl->attr.delay_time, hdl->attr.protect_time);
    }

}

static int dev_sync_output_handler(void *_hdl, void *data, int len)
{
    struct spdif_node_hdl *hdl = _hdl;
#if SPDIF_DEBUG
    u32 buffered_frames = (JL_SPDIF->SM_ABUFLEN - JL_SPDIF->SM_ASHN - 1) >> hdl_node(hdl)->iport->prev->fmt.bit_wide;
    if (buffered_frames < 40) {
        printf("spdif will enpty\n");

        extern int mast_iis_frames();
        mast_iis_frames();

    }
#endif
    int wlen = audio_spdif_channel_write((void *)&hdl->spdif_ch, &spdif_hdl, data, len);
    if (config_dev_sync_enable) {
        dev_sync_calculate_output_samplerate(hdl->dev_sync, wlen, audio_spdif_data_len(&hdl->spdif_ch));
    }
    return wlen;
}

static void dev_sync_start(struct stream_iport *iport)
{
    if (config_dev_sync_enable) {
        struct spdif_node_hdl *hdl = (struct spdif_node_hdl *)iport->node->private_data;
        if (hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            struct dev_sync_params params = {0};
            params.in_sample_rate = hdl->sample_rate;
            params.out_sample_rate = hdl->sample_rate;
            params.channel = hdl->nch;
            params.bit_width = iport->prev->fmt.bit_wide;
            params.priv = hdl;
            params.handle = dev_sync_output_handler;
            if (!hdl->dev_sync) {
                hdl->dev_sync = dev_sync_open(&params);
            }
        }
    }
}

static int audio_spdif_channel_latch_time(struct audio_spdif_channel *ch, u32 *latch_time, u32(*get_time)(u32 reference_network), u32 reference_network)
{
    int buffered_frames = 0;
    struct audio_spdif_hdl *hdl = (struct audio_spdif_hdl *)ch->hd_spdif;
    if (!hdl) {
        return 0;
    }
    os_mutex_pend(&hdl->mutex, 0);
    spin_lock(&hdl->lock);
    *latch_time = get_time(reference_network);
    // 2024.06.20--to compile
    /* buffered_frames = ((JL_SPDIF->SM_ABUFLEN - JL_SPDIF->SM_ASHN - 1) >> ch->fifo.bit_wide) - audio_cfifo_channel_unread_diff_samples(&ch->fifo); */
    /* if (buffered_frames < 0) { */
    /* buffered_frames = 0; */
    /* } */
    spin_unlock(&hdl->lock);
    os_mutex_post(&hdl->mutex);
    return buffered_frames;
}


static void spdif_synchronize_with_main_device(struct stream_iport *iport, struct stream_frame *frame)
{
    struct spdif_node_hdl *hdl = (struct spdif_node_hdl *)iport->node->private_data;
    if ((hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) && hdl->timestamp_ok) {
        struct sync_rate_params params = {0};
        params.d_sample_rate = frame->d_sample_rate;
        params.buffered_frames = audio_spdif_channel_latch_time(&hdl->spdif_ch, &params.current_time, dev_sync_latch_time, hdl->reference_network);
        params.timestamp = frame->timestamp;
        params.name = hdl->name;
        dev_sync_update_rate(hdl->dev_sync, &params);
    }
}

static int spdif_adpater_detect_timestamp(struct spdif_node_hdl *hdl, struct stream_frame *frame)
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
            int slience_frames = (u64)slience_time_us * hdl->sample_rate / 1000000;
            audio_spdif_fill_slience_frames(&hdl->spdif_ch, slience_frames);
        }
        return 0;
    }

    if (hdl->syncts_enabled) {
        audio_spdif_syncts_trigger_with_timestamp(&hdl->spdif_ch, frame->timestamp);
        return 0;
    }

    /*printf("----[%d %d %d]----\n", bt_audio_reference_clock_time(0xff), audio_spdif_data_len(&hdl->spdif_ch), hdl->sample_rate);*/
    u32 local_network = 0;
    u32 reference_time = 0;
    u8 net_addr[6];
    u8 network = audio_reference_clock_network(net_addr);
    if (network != ((u8) - 1)) {
        reference_time = audio_reference_clock_time();
        if (reference_time == (u32) - 1) {
            goto syncts_start;
        }
        /* current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_dac_buffered_frames(&dac_hdl), hdl->sample_rate); */
        if (network == 2) {
            current_time = reference_time * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_spdif_data_len(&hdl->spdif_ch), hdl->sample_rate);
        } else {
            current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_spdif_data_len(&hdl->spdif_ch), hdl->sample_rate);
        }
#if SPDIF_DEBUG
        putchar('#');
#endif
    } else {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_spdif_data_len(&hdl->spdif_ch), hdl->sample_rate);
        local_network = 1;
    }
    if (hdl->reference_network == AUDIO_NETWORK_LOCAL) {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_spdif_data_len(&hdl->spdif_ch), hdl->sample_rate);
        local_network = 1;
    }

    diff = frame->timestamp - current_time;
    diff /= TIMESTAMP_US_DENOMINATOR;
    if (diff < 0) {
        if (__builtin_abs(diff) <= 1000) {
            goto syncts_start;
        }

        if (hdl->time_out_ts_not_align) { //不做对齐
            if (local_network && !config_media_tws_en) {//本地网络时钟，不需要时间戳对齐
                hdl->syncts_enabled = 1;
                audio_spdif_syncts_trigger_with_timestamp(&hdl->spdif_ch, frame->timestamp);
#if SPDIF_DEBUG
                putchar('c');
#endif
                return 0;
            }
        }
#if SPDIF_DEBUG
        putchar('S');
        printf("[%s, %u %u %d]\n", hdl->name, frame->timestamp, current_time, diff);
#endif
        return 2;
    }

    if (diff > 1000000) {
        printf("spdif node timestamp error : %u, %u, %u, diff : %d\n", frame->timestamp, current_time, reference_time, diff);
    }
    int slience_frames = (u64)diff * hdl->sample_rate / 1000000;

    int filled_frames = audio_spdif_fill_slience_frames(&hdl->spdif_ch, slience_frames);
#if SPDIF_DEBUG
    putchar('K');
    printf("spdif slience_frames %d\n", filled_frames);
#endif
    if (filled_frames < slience_frames) {
        return 1;
    }

syncts_start:

    /*printf("< %s, %u %u %d>-\n", hdl->name, frame->timestamp, current_time, diff);*/
    hdl->syncts_enabled = 1;
    audio_spdif_syncts_trigger_with_timestamp(&hdl->spdif_ch, frame->timestamp);

    return 0;
}


__NODE_CACHE_CODE(spdif)
static void spdif_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct spdif_node_hdl *hdl = (struct spdif_node_hdl *)iport->node->private_data;

    struct stream_frame *frame;
    struct stream_node *node = iport->node;

    audio_spdif_channel_start((void *)&hdl->spdif_ch);

    dev_sync_start(iport);

    u16 point_offset = 1;
    if (iport->prev->fmt.bit_wide) {
        point_offset = 2;
    }

    while (1) {
        frame = jlstream_pull_frame(iport, NULL);
        if (!frame) {
            break;;
        }
        if (config_dev_sync_enable) {
            if (frame->offset == 0) {
                spdif_synchronize_with_main_device(iport, frame);
            }
        }


        int err = spdif_adpater_detect_timestamp(hdl, frame);
        if (err == 1) { //需要继续填充数据至时间戳
            hdl->timestamp_ok = 0;
            jlstream_return_frame(iport, frame);
            break;
        }

        if (err == 2) { //需要直接丢弃改帧数据
            audio_spdif_force_use_syncts_frames(&hdl->spdif_ch, (frame->len >> point_offset) / hdl->nch, frame->timestamp);
            jlstream_free_frame(frame);
            hdl->timestamp_ok = 0;
            continue;
        }
        hdl->timestamp_ok = 1;

        u32 remain = frame->len - frame->offset;
        s16 *data = (s16 *)(frame->data + frame->offset);
        int wlen = 0;
        if (hdl->dev_sync) {
            if (config_dev_sync_enable) {
                wlen = dev_sync_write(hdl->dev_sync, data, remain);
            }
        } else {
            wlen = audio_spdif_channel_write((void *)&hdl->spdif_ch, &spdif_hdl, data, remain);
        }

        frame->offset += wlen;

        if (wlen < remain) {
            // 输出有剩余
            note->state |= NODE_STA_OUTPUT_BLOCKED;
            jlstream_return_frame(iport, frame);
            break;
        }
        jlstream_free_frame(frame);
    }
}

// flag   0:time  1:points
static int spdif_ioc_get_delay(struct spdif_node_hdl *hdl, struct audio_spdif_channel *ch)
{
    int len = audio_spdif_data_len(ch);
    if (len == 0) {
        return 0;
    }
    int rate = audio_spdif_get_sample_rate(&spdif_hdl);
    ASSERT(rate != 0);

    return (len * 10000) / rate;
}

static int spdif_ioc_negotiate(struct stream_iport *iport)
{
    int ret = NEGO_STA_ACCPTED;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct spdif_node_hdl *hdl = (struct spdif_node_hdl *)iport->node->private_data;

    struct audio_general_params *params = audio_general_get_param();
    u8 bit_width = audio_general_out_dev_bit_width();
    if (bit_width) {
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

    if (in_fmt->channel_mode != AUDIO_CH_LR) {
        in_fmt->channel_mode = AUDIO_CH_LR;
        ret = NEGO_STA_CONTINUE;
    }

    u32 sample_rate = 0;
    if (hdl->spdif_start || audio_spdif_is_working(&spdif_hdl)) {
        sample_rate = audio_spdif_get_sample_rate(&spdif_hdl);
        if (in_fmt->sample_rate != sample_rate) {
            if (in_fmt->sample_rate > sample_rate && sample_rate < 44100) {
                if (params->sample_rate == 0 && !(hdl->scene & STREAM_SCENE_TONE)) {
                    return NEGO_STA_SUSPEND;
                }
            }
            in_fmt->sample_rate = sample_rate;
            ret = NEGO_STA_CONTINUE;
        }
    } else {
        if (hdl->scene != STREAM_SCENE_ESCO) {
            if (hdl->sample_rate) {
                sample_rate = hdl->sample_rate;
            } else {
                sample_rate = params->sample_rate;
            }
        } else {
            sample_rate = in_fmt->sample_rate;
        }
        if (in_fmt->sample_rate != sample_rate) {
            in_fmt->sample_rate = sample_rate;
            ret = NEGO_STA_CONTINUE;
        }
    }

    hdl->sample_rate = in_fmt->sample_rate;
    hdl->nch = AUDIO_CH_NUM(in_fmt->channel_mode);

    // 保证上一个节点输入的是PCM格式，IIS输出数据格式目前只能是PCM
    if (in_fmt->coding_type != AUDIO_CODING_PCM) {
        /* r_printf("%s, %d, spdif output nonsupport coding_type except PCM", __func__, __LINE__); */
        /*         in_fmt->coding_type = AUDIO_CODING_PCM; */
        /* ret = NEGO_STA_CONTINUE; */

    }

    return ret;
}



static void spdif_adapter_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = spdif_handle_frame;
}

static void spdif_ioc_start(struct spdif_node_hdl *hdl)
{
    if (hdl->spdif_start == 0) {
        hdl->spdif_start = 1;

        if (!audio_spdif_is_working(&spdif_hdl)) {
            audio_spdif_set_sample_rate(&spdif_hdl, hdl->sample_rate);
            spdif_master_init(&spdif_hdl);
        }

        spdif_node_param_init(hdl, hdl_node(hdl));
        audio_spdif_new_channel(&spdif_hdl, &hdl->spdif_ch);
        audio_spdif_channel_set_attr(&hdl->spdif_ch, &hdl->attr);
    }
}

static void spdif_ioc_stop(struct spdif_node_hdl *hdl)
{
    if (config_dev_sync_enable) {
        if (hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            if (hdl->dev_sync) {
                dev_sync_close(hdl->dev_sync);
                hdl->dev_sync = NULL;
            }
        }
    }

    if (audio_spdif_is_working(&spdif_hdl)) {
        puts("audio_spdif_channel_close \n");
        audio_spdif_channel_close(&hdl->spdif_ch);
    }
    if (hdl->spdif_start) {
        hdl->spdif_start = 0;
        // 停止 spdif out
        /* spdif_master_stop(); */
#if (TCFG_CVP_DEVELOP_ENABLE) == 0
#if (!TCFG_AUDIO_DUAL_MIC_ENABLE)
        if (hdl->scene == STREAM_SCENE_ESCO) {
            audio_cvp_ref_src_close();
        }
#endif
#endif
    }

    /* hdl->timestamp_pass = 0; */
}
static int spdif_adapter_syncts_ioctl(struct spdif_node_hdl *hdl, struct audio_syncts_ioc_params *params)
{
    if (!params) {
        return 0;
    }
    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        printf("=====spdif node name: %s\n", hdl->name);
        audio_spdif_add_syncts_with_timestamp(&hdl->spdif_ch, (void *)params->data[0], params->data[1]);

        if (hdl->reference_network == 0xff) {
            hdl->reference_network = params->data[2];
        }

        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        audio_spdif_remove_syncts_handle(&hdl->spdif_ch, (void *)params->data[0]);
        break;
    }
    return 0;
}

static int spdif_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct spdif_node_hdl *hdl = (struct spdif_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        spdif_adapter_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= spdif_ioc_negotiate(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_GET_DELAY:
        int self_delay = spdif_ioc_get_delay(hdl, &hdl->spdif_ch);
        if (hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            return self_delay - 50;//多输出从设备查询到延时减5ms，减小流程延迟导致依据的主设备延迟偏差
        }
        return self_delay;
    case NODE_IOC_START:
        spdif_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
        hdl->syncts_enabled = 0;
        hdl->force_write_slience_data = 0;
        if (hdl->spdif_start == 0) {
            hdl->sample_rate = 0;
        }
        break;
    case NODE_IOC_STOP:
        spdif_ioc_stop(hdl);
        break;
    case NODE_IOC_SYNCTS:
        spdif_adapter_syncts_ioctl(hdl, (struct audio_syncts_ioc_params *)arg);
        break;
    case NODE_IOC_GET_ODEV_CACHE:
        return audio_spdif_data_len(&hdl->spdif_ch);
    case NODE_IOC_SET_PARAM:
        hdl->reference_network = arg;
        break;
    case NODE_IOC_SET_TS_PARM:
        hdl->time_out_ts_not_align = arg;
        break;
    case NODE_IOC_SET_PRIV_FMT: //手动控制是否预填静音包
        hdl->force_write_slience_data_en = arg;
        break;
    default:
        break;
    }

    return ret;
}


static int spdif_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct spdif_node_hdl *hdl = (struct spdif_node_hdl *)node->private_data;

    hdl->reference_network = 0xff;
    return 0;
}


static void spdif_adapter_release(struct stream_node *node)
{
}

REGISTER_STREAM_NODE_ADAPTER(spdif_node_adapter) = {
    .name       = "spdif_master",
    .uuid       = NODE_UUID_SPDIF_MASTER,
    .bind       = spdif_adapter_bind,
    .ioctl      = spdif_adapter_ioctl,
    .release    = spdif_adapter_release,
    .hdl_size   = sizeof(struct spdif_node_hdl),
};

#endif

