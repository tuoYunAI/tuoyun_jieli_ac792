#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_mic_recorder.data.bss")
#pragma data_seg(".pc_mic_recorder.data")
#pragma const_seg(".pc_mic_recorder.text.const")
#pragma code_seg(".pc_mic_recorder.text")
#endif
#include "pc_mic_recorder.h"
#include "audio_config_def.h"
#include "volume_node.h"
#include "audio_cvp.h"
#include "pc_spk_player.h"
#include "app_config.h"


#if TCFG_USB_SLAVE_AUDIO_ENABLE && TCFG_USB_SLAVE_AUDIO_MIC_ENABLE

#define LOG_TAG             "[PC_MIC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

#include "uac_stream.h"

static struct list_head head = LIST_HEAD_INIT(head);

static u8 pcm_mic_recorder_check = 0;

void pcm_mic_recorder_dump(struct pc_mic_recorder *recorder, int force_dump)
{
    if (recorder && recorder->stream) {
        jlstream_node_ioctl(recorder->stream, NODE_UUID_SOURCE, NODE_IOC_FORCE_DUMP_PACKET, force_dump);
    }
}

static void pc_mic_recorder_callback(void *private_data, int event)
{
    struct pc_mic_recorder *recorder = (struct pc_mic_recorder *)private_data;
    struct pc_mic_recorder *p, *n;
    u16 volume;
    u8 find = 0;

    switch (event) {
    case STREAM_EVENT_START:
        list_for_each_entry_safe(p, n, &head, entry) {
            if (p == recorder) {
                find = 1;
                break;
            }
        }
        if (!find) {
            break;
        }
        uac_mic_stream_get_volume(recorder->usb_id, &volume);
        struct volume_cfg cfg = {0};
        cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
        cfg.cur_vol = volume;
        int err = jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, "Vol_PcMic", (void *)&cfg, sizeof(struct volume_cfg));
        log_info("pc mic vol: %d, ret: %d", volume, err);
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && TCFG_IIS_NODE_ENABLE
#if TCFG_USB_SLAVE_AUDIO_ENABLE && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        /*打开pc mic，没有开skp，忽略外部参考数据*/
        if (!pc_spk_player_runing() || pc_spk_player_mute_status()) {
            log_info("CVP_OUTWAY_REF_IGNORE");
            audio_cvp_ioctl(CVP_OUTWAY_REF_IGNORE, 1, NULL);
        }
#endif
#endif
        break;
    }
}

struct pc_mic_recorder *pc_mic_recorder_open(struct stream_fmt *fmt)
{
    u16 source_uuid = NODE_UUID_ADC;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"pc_mic");
    if (uuid == 0) {
        return NULL;
    }

    struct pc_mic_recorder *recorder = zalloc(sizeof(*recorder));
    if (!recorder) {
        return NULL;
    }

    recorder->stream = jlstream_pipeline_parse_by_node_name(uuid, "USB_ADC");

    if (!recorder->stream) {
        goto __exit0;
    }

    jlstream_node_ioctl(recorder->stream, NODE_UUID_PC_MIC, NODE_IOC_SET_FMT, (int)fmt);

    //设置ADC的中断点数
    int err = jlstream_node_ioctl(recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS_MUSIC_MODE);
    if (err) {
        goto __exit1;
    }

    u16 node_uuid = get_cvp_node_uuid();
    //根据回音消除的类型，将配置传递到对应的节点
    if (node_uuid) {
#if !(TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && TCFG_IIS_NODE_ENABLE)
        int audio_setup_dac_get_sample_rate(void);
        u32 ref_sr = audio_setup_dac_get_sample_rate();
        jlstream_node_ioctl(recorder->stream, node_uuid, NODE_IOC_SET_FMT, (int)ref_sr);
#endif
        err = jlstream_node_ioctl(recorder->stream, node_uuid, NODE_IOC_SET_PRIV_FMT, source_uuid);
        if (err && (err != -ENOENT)) {	//兼容没有cvp节点的情况
            goto __exit1;
        }
    }

    jlstream_set_callback(recorder->stream, recorder, pc_mic_recorder_callback);
    jlstream_set_scene(recorder->stream, STREAM_SCENE_PC_MIC);

    list_add_tail(&recorder->entry, &head);

    jlstream_add_thread(recorder->stream, NULL);

    err = jlstream_start(recorder->stream);
    if (err) {
        goto __exit1;
    }

    pcm_mic_recorder_dump(recorder, pcm_mic_recorder_check);

    return recorder;

__exit1:
    jlstream_release(recorder->stream);
__exit0:
    free(recorder);

    return NULL;
}

void pc_mic_recorder_close(struct pc_mic_recorder *recorder)
{
    if (!recorder) {
        return;
    }

    list_del(&recorder->entry);

    if (recorder->stream) {
        jlstream_stop(recorder->stream, 0);
        jlstream_release(recorder->stream);
    }

    free(recorder);

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECORDER, (int)"pc_mic");
}

bool pc_mic_recorder_runing(void)
{
    return uac_get_mic_stream_status(0)
#if USB_MAX_HW_NUM > 1
           || uac_get_mic_stream_status(1)
#endif
           ;
}

#endif

