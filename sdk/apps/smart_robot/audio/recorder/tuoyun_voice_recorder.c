#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tuoyun_voice_recorder.data.bss")
#pragma data_seg(".tuoyun_voice_recorder.data")
#pragma const_seg(".tuoyun_voice_recorder.text.const")
#pragma code_seg(".tuoyun_voice_recorder.text")
#endif
#include "jlstream.h"
#include "encoder_fmt.h"
#include "vad_node.h"
#include "app_event.h"
#include "tuoyun_voice_recorder.h"


#define LOG_TAG             "[RECORDER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


typedef struct tuoyun_voice_recorder {
    struct jlstream *stream;
    tuoyun_voice_param_t param;
}tuoyun_voice_recorder_t, *tuoyun_voice_recorder_ptr;


static tuoyun_voice_recorder_ptr g_tuoyun_voice_recorder = NULL;



static void tuoyun_voice_recorder_callback(void *private_data, int event)
{
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

static int tuoyun_recorder_data_output(void *priv, u8 *buf, int len)
{
    tuoyun_voice_recorder_ptr recorder = (tuoyun_voice_recorder_ptr)priv;

    if (recorder->param.output) {
        recorder->param.output(recorder->param.priv, buf, len);
    }

    return len;
}

static const struct stream_file_ops ai_tx_ops = {
    .write = tuoyun_recorder_data_output,
};

static int vad_callback(enum vad_event event)
{
    switch (event) {
    case VAD_EVENT_SPEAK_START:
        log_info("@TIMING@ 0 VAD_EVENT_SPEAK_START");
        break;
    case VAD_EVENT_SPEAK_STOP:
        log_info("@TIMING@ 1 VAD_EVENT_SPEAK_STOP");
        break;
    }

    return 0;
}

void tuoyun_voice_recorder_open(struct tuoyun_voice_param *param)
{
    int err;
    struct stream_fmt fmt = {0};
    struct encoder_fmt enc_fmt = {0};
    vad_node_priv_t vad = {0};

    app_event_t ev = {
        .event = APP_EVENT_AUDIO_MIC_BEFORE_OPEN,
        .arg = NULL
    };
    app_event_notify(APP_EVENT_FROM_AUDIO, &ev);
    extern void tuoyun_asr_recorder_close();
    tuoyun_asr_recorder_close();
    
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"ai_voice");
    if (uuid == 0) {
        return;
    }

    g_tuoyun_voice_recorder = zalloc(sizeof(*g_tuoyun_voice_recorder));
    if (!g_tuoyun_voice_recorder) {
        return;
    }

    memcpy(&g_tuoyun_voice_recorder->param, param, sizeof(*param));

    g_tuoyun_voice_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!g_tuoyun_voice_recorder->stream) {
        g_tuoyun_voice_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_PDM_MIC);
    }
    if (!g_tuoyun_voice_recorder->stream) {
        g_tuoyun_voice_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS0_RX);
    }
    if (!g_tuoyun_voice_recorder->stream) {
        goto __exit0;
    }

    fmt.sample_rate = param->sample_rate;

    switch (param->code_type) {
    case AUDIO_CODING_OPUS:
        //  bitrate
        //     16000,32000,64000 这三个码率分别对应非ogg解码库
        //     的 OPUS_SRINDEX 值为0,1,2
        //  format
        //     0:百度_无头.
        //     1:酷狗_eng+range.
        //     2:ogg封装,pc软件可播放.
        //     3:size+rangeFinal. 源码可兼容版本.
        //  complexity
        //     0|1|2|3     3质量最好.速度要求最高.
        //  frame_ms (由frame_dms / 10得出)
        //     20|40|60|80|100 ms.
        //  sample_rate
        //     sample_rate=16k         ignore
        //
        //   注意
        //   1. struct encoder_fmt是配置编码器私有参数
        //   有效的参数：
        //   complexity, format, frame_dms
        //   不起效的参数：
        //   bit_rate, sample_rate, ch_num, bit_width
        enc_fmt.complexity = param->complexity;
        enc_fmt.format = param->format_mode;
        enc_fmt.frame_dms = param->frame_ms * 10;    //与工具保持一致，要乘以10,表示20ms
        fmt.coding_type = AUDIO_CODING_OPUS;
        break;
    case AUDIO_CODING_SPEEX:
        enc_fmt.quality = param->quality;
        enc_fmt.complexity = param->complexity; //0~9
        fmt.coding_type = AUDIO_CODING_SPEEX;
        break;
    case AUDIO_CODING_AAC:
        enc_fmt.format = param->format_mode; //标准头部
        break;
    default:
        goto __exit1;
    }

    err = jlstream_node_ioctl(g_tuoyun_voice_recorder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_PRIV_FMT, (int)(&enc_fmt));
    if (err) {
        goto __exit1;
    }

    err = jlstream_node_ioctl(g_tuoyun_voice_recorder->stream, NODE_UUID_AI_TX, NODE_IOC_SET_FMT, (int)(&fmt));
    if (err) {
        goto __exit1;
    }

    //设置ADC的中断点数
    err = jlstream_node_ioctl(g_tuoyun_voice_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 320);
    if (err) {
        goto __exit1;
    }

    vad.vad_callback = vad_callback;
    jlstream_node_ioctl(g_tuoyun_voice_recorder->stream, NODE_UUID_VAD, NODE_IOC_SET_PRIV_FMT, (int)&vad);

    struct stream_file_info info = {
        .file = (void *)g_tuoyun_voice_recorder,
        .ops = &ai_tx_ops
    };
    err = stream_node_ioctl((struct stream_node *)g_tuoyun_voice_recorder->stream->snode, NODE_UUID_AI_TX, NODE_IOC_SET_FILE, (int)&info);
    if (err) {
        goto __exit1;
    }

    jlstream_set_callback(g_tuoyun_voice_recorder->stream, g_tuoyun_voice_recorder->stream, tuoyun_voice_recorder_callback);
    jlstream_set_scene(g_tuoyun_voice_recorder->stream, STREAM_SCENE_AI_VOICE);

    err = jlstream_start(g_tuoyun_voice_recorder->stream);
    if (err) {
        goto __exit1;
    }
    log_info("tuoyun voice recorder init success");


    ev.event = APP_EVENT_AUDIO_MIC_AFTER_OPEN;
    app_event_notify(APP_EVENT_FROM_AUDIO, &ev);
    return;

__exit1:
    jlstream_release(g_tuoyun_voice_recorder->stream);
__exit0:
    free(g_tuoyun_voice_recorder);
    g_tuoyun_voice_recorder = NULL;
    return;
}

int tuoyun_voice_recorder_close()
{

    if (!g_tuoyun_voice_recorder) {
        return 0;
    }

    app_event_t ev = {
        .event = APP_EVENT_AUDIO_MIC_BEFORE_CLOSE,
        .arg = NULL
    };
    app_event_notify(APP_EVENT_FROM_AUDIO, &ev);

    jlstream_stop(g_tuoyun_voice_recorder->stream, 0);
    jlstream_release(g_tuoyun_voice_recorder->stream);

    free(g_tuoyun_voice_recorder);
    g_tuoyun_voice_recorder = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECORDER, (int)"ai_voice");

    os_time_dly(10); //确保资源释放完毕
    ev.event = APP_EVENT_AUDIO_MIC_AFTER_CLOSE;
    app_event_notify(APP_EVENT_FROM_AUDIO, &ev);
    return 0;
}

