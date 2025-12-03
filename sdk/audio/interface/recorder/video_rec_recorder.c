#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".video_rec_recorder.data.bss")
#pragma data_seg(".video_rec_recorder.data")
#pragma const_seg(".video_rec_recorder.text.const")
#pragma code_seg(".video_rec_recorder.text")
#endif
#include "jlstream.h"
#include "encoder_fmt.h"
#include "vad_node.h"


struct video_rec_recorder {
    struct jlstream *stream;

    void *priv;
    void (* cb)(void *, u8 *, u32);
};

static void video_rec_recorder_callback(void *private_data, int event)
{
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

static int recorder_data_output(void *priv, u8 *buf, int len)
{
    struct video_rec_recorder *recorder = (struct video_rec_recorder *)priv;


    if (recorder->cb) {
        recorder->cb(recorder->priv, buf, len);
    }

    return len;
}

static const struct stream_file_ops video_rec_tx_ops = {
    .write = recorder_data_output,
};

void *video_mic_recorder_open(u16 sample_rate, u8 code_type, void *priv, void (*cb)(void *, u8 *, u32))
{
    int err;
    struct stream_fmt fmt = {0};
    struct encoder_fmt enc_fmt = {0};

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"vir_voice");
    if (uuid == 0) {
        return NULL;
    }
    struct video_rec_recorder *recorder = zalloc(sizeof(*recorder));
    if (!recorder) {
        return NULL;
    }

    recorder->cb   = cb;
    recorder->priv = priv;


    recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!recorder->stream) {
        goto __exit0;
    }

    enc_fmt.quality = 0;
    enc_fmt.sample_rate = sample_rate;
    enc_fmt.frame_dms = 1024;
    fmt.sample_rate = sample_rate;
    if (!code_type) {
        fmt.coding_type = AUDIO_CODING_PCM;
    } else {
        fmt.coding_type = code_type;
    }


    err = jlstream_node_ioctl(recorder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_FMT, (int)(&enc_fmt));
    if (err) {
        goto __exit1;
    }

    err = jlstream_node_ioctl(recorder->stream, NODE_UUID_VIR_DATA_TX, NODE_IOC_SET_FMT, (int)(&fmt));
    if (err) {
        goto __exit1;
    }

    //设置ADC的中断点数
    err = jlstream_node_ioctl(recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 320);
    if (err) {
        goto __exit1;
    }

    struct stream_file_info info = {
        .file = (void *)recorder,
        .ops = &video_rec_tx_ops
    };
    err = stream_node_ioctl((struct stream_node *)recorder->stream->snode, NODE_UUID_VIR_DATA_TX, NODE_IOC_SET_FILE, (int)&info);
    if (err) {
        goto __exit1;
    }

    jlstream_set_callback(recorder->stream, recorder->stream, video_rec_recorder_callback);
    jlstream_set_scene(recorder->stream, STREAM_SCENE_VIR_DATA_TX);

    err = jlstream_start(recorder->stream);
    if (err) {
        goto __exit1;
    }
    return recorder;

__exit1:
    jlstream_release(recorder->stream);
__exit0:
    free(recorder);
    return NULL;

}

int video_mic_recorder_close(void *priv)
{
    struct video_rec_recorder *recorder = (struct video_rec_recorder *)priv;

    if (!recorder) {
        return 0;
    }

    jlstream_stop(recorder->stream, 0);
    jlstream_release(recorder->stream);

    free(recorder);

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECORDER, (int)"video_rec");

    return 0;
}

