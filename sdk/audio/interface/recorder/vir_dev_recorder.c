#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vir_dev_recorder.data.bss")
#pragma data_seg(".vir_dev_recorder.data")
#pragma const_seg(".vir_dev_recorder.text.const")
#pragma code_seg(".vir_dev_recorder.text")
#endif
#include "jlstream.h"
#include "encoder_fmt.h"
#include "audio_config_def.h"

#define LOG_TAG             "[VIRTUAL_RECORDER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

struct vir_voice_param {
    u8 quality;
    u8 complexity;
    u8 format_mode;
    u32 frame_ms;
    int sample_rate;
    void *priv;
    int code_type;
    int (*output)(void *priv, void *data, u32 len);
};

struct vir_dev_recorder {
    struct jlstream *stream;
    struct vir_voice_param param;
};

static void vir_dev_recorder_callback(void *private_data, int event)
{
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

static int vir_recorder_data_output(void *priv, u8 *buf, int len)
{
    struct vir_dev_recorder *recorder = (struct vir_dev_recorder *)priv;

    if (recorder->param.output) {
        recorder->param.output(recorder->param.priv, buf, len);
    }

    return len;
}

static const struct stream_file_ops vir_tx_ops = {
    .write = vir_recorder_data_output,
};

void *vir_dev_recorder_open(struct vir_voice_param *param)
{
    int err;
    struct stream_fmt fmt = {0};
    struct encoder_fmt enc_fmt = {0};

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"vir_voice");
    if (uuid == 0) {
        return NULL;
    }

    struct vir_dev_recorder *recorder = zalloc(sizeof(*recorder));
    if (!recorder) {
        return NULL;
    }

    memcpy(&recorder->param, param, sizeof(*param));

    recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!recorder->stream) {
        goto __exit0;
    }

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
        enc_fmt.quality = param->quality; //建议配置成4-5,使用5
        enc_fmt.complexity = param->complexity; //建议值0-2,2音质最好
        fmt.coding_type = AUDIO_CODING_SPEEX;
        break;
    }

    if (param->code_type == AUDIO_CODING_OPUS || param->code_type == AUDIO_CODING_SPEEX) {
        err = jlstream_node_ioctl(recorder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_PRIV_FMT, (int)(&enc_fmt));
        if (err) {
            goto __exit1;
        }
    }

    //设置ADC的中断点数
    err = jlstream_node_ioctl(recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS_MUSIC_MODE);
    if (err) {
        goto __exit1;
    }

    struct stream_file_info info = {
        .file = (void *)recorder,
        .ops = &vir_tx_ops
    };
    err = stream_node_ioctl((struct stream_node *)recorder->stream->snode, NODE_UUID_VIR_DATA_TX, NODE_IOC_SET_FILE, (int)&info);
    if (err) {
        goto __exit1;
    }

    jlstream_set_callback(recorder->stream, recorder->stream, vir_dev_recorder_callback);
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

int vir_dev_recorder_close(void *priv)
{
    struct vir_dev_recorder *recorder = (struct vir_dev_recorder *)priv;

    if (!recorder) {
        return 0;
    }

    jlstream_stop(recorder->stream, 50);
    jlstream_release(recorder->stream);

    free(recorder);

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECORDER, (int)"vir_voice");

    return 0;
}






#define VIRTUAL_AUDIO_TEST 0

#if VIRTUAL_AUDIO_TEST

#include "vir_dev_player.h"
#include "network_download/net_audio_buf.h"

typedef struct {
    struct vir_player *player;
    struct vir_dev_recorder *recorder;
    void *net_buf;
    OS_SEM r_sem;
    OS_SEM start_dec_sem;
    u8 odata_flag;
    u32 enc_len;
    u8 stop_flag;
} vir_music_hdl;

static vir_music_hdl vir_hdl;

#define __this (&vir_hdl)

static int virtual_audio_player_decode_event_callback(void *priv, int parm, enum stream_event event)
{
    switch (event) {
    case STREAM_EVENT_START:
        log_info("STREAM_EVENT_START");
        break;
    case STREAM_EVENT_STOP:
        log_info("STREAM_EVENT_STOP");
        virtual_dev_player_stop(__this->player);
        break;
    case STREAM_EVENT_END:
        log_info("STREAM_EVENT_END");
        break;
    default:
        break;
    }

    return 0;
}

static const struct stream_file_ops virtual_test_ops;

static int vir_voice_vfs_fwrite(void *priv, void *data, u32 len)
{
    int wlen = net_buf_write(data, len, __this->net_buf);
    if (wlen == 0) {
        log_error("vir_voice_vfs_fwrite full");
    }

    os_sem_set(&__this->r_sem, 0);
    os_sem_post(&__this->r_sem);

    if (!__this->odata_flag) {
        __this->odata_flag = 1;
        __this->enc_len = wlen;
        os_sem_set(&__this->start_dec_sem, 0);
        os_sem_post(&__this->start_dec_sem);
    }

    return wlen;
}

static int virtual_dev_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    int rlen, dlen;
    struct vir_player *player = (struct vir_player *)file;

    while (1) {
        if (!player->file) {
            break;
        }

        dlen = net_buf_get_data_size(player->file);
        if (dlen == 0) {
            if (__this->stop_flag) {
                return 0;
            }
            os_sem_pend(&__this->r_sem, 200);
            continue;
        }

        if (__this->stop_flag) {
            if (dlen < len) {
                rlen = net_buf_read(buf, dlen, player->file);
            } else {
                rlen = net_buf_read(buf, len, player->file);
            }

            return rlen;
        } else {
            if (dlen < __this->enc_len) {
                rlen = net_buf_read(buf + offset, dlen, player->file);
            } else {
                rlen = net_buf_read(buf + offset, __this->enc_len, player->file);
            }
        }

        if (rlen > 0) {
            break;
        }
    }

    player->read_err = 0;
    offset += rlen;

    return offset;
}


static int virtual_dev_seek(void *file, int offset, int fromwhere)
{
    struct vir_player *player = (struct vir_player *)file;
    return net_buf_seek(offset, fromwhere, player->file);
}

static int virtual_dev_close(void *file)
{
    return 0;
}

static int virtual_dev_get_fmt(void *file, struct stream_fmt *fmt)
{
    struct vir_player *player = (struct vir_player *)file;
    u8 buf[80];

    fmt->coding_type = player->coding_type;

    if (fmt->coding_type == AUDIO_CODING_SPEEX) {
        virtual_test_ops.seek(player, 0, SEEK_SET);
        virtual_test_ops.read(player, buf, 4);

        if ((buf[1] == 0x00) && (buf[3] == 0x00) && (buf[2] == 0x54 || buf[2] == 0x53)) {
            fmt->with_head_data = 1;
        }
        if (fmt->with_head_data) {
            fmt->sample_rate = buf[2] == 0x54 ? 16000 : 8000;
        }
        fmt->channel_mode = AUDIO_CH_MIX;
        fmt->quality = CONFIG_SPEEX_DEC_FILE_QUALITY;
        if (!fmt->with_head_data) {
            fmt->sample_rate = CONFIG_SPEEX_DEC_FILE_SAMPLERATE;
        }
        virtual_test_ops.seek(player, 0, SEEK_SET);
        return 0;
    }

    if (fmt->coding_type == AUDIO_CODING_OPUS || fmt->coding_type == AUDIO_CODING_STENC_OPUS) {
        fmt->quality = AUDIO_ATTR_OPUS_CBR_PKTLEN_TYPE;//CONFIG_OPUS_DEC_FILE_TYPE;
        if (fmt->quality == AUDIO_ATTR_OPUS_CBR_PKTLEN_TYPE) {
            fmt->opus_pkt_len = 40;//CONFIG_OPUS_DEC_PACKET_LEN;
        }
        return -EINVAL;
    }

    if (fmt->coding_type == AUDIO_CODING_PCM) {
        fmt->sample_rate   = 16000;
        fmt->channel_mode  = AUDIO_CH_L;
        fmt->pcm_file_mode = 1;
        return 0;
    }

    return -EINVAL;
}

static const struct stream_file_ops virtual_test_ops = {
    .read       = virtual_dev_read,
    .seek       = virtual_dev_seek,
    .close      = virtual_dev_close,
    .get_fmt    = virtual_dev_get_fmt,
};

static void virtual_dec_thread()
{
    //等编码fwrite写数到net_buf再打开解码
    while (!__this->odata_flag) {
        os_sem_pend(&__this->start_dec_sem, 0);
        /* os_time_dly(5); */
        __this->player = virtual_dev_play_callback((FILE *)__this->net_buf, &virtual_test_ops, NULL, virtual_audio_player_decode_event_callback, NULL, AUDIO_CODING_PCM);
    }
}

static void vir_audio_close()
{
    __this->stop_flag = 1;
    vir_dev_recorder_close(__this->recorder);
    os_sem_set(&__this->r_sem, 0);
    os_sem_post(&__this->r_sem);
    virtual_dev_player_stop(__this->player);
}

//通过 编码 - net_buf - 解码 实现编码虚拟源输出和解码虚拟源输入测试
void virtual_audio_test()
{
    //初始化net buf
    u32 bufsize = 64 * 1024;
    __this->net_buf = net_buf_init(&bufsize, NULL);

    if (!__this->net_buf) {
        log_error("virtual_test net_buf_init fail");
    }
    net_buf_active(__this->net_buf);
    net_buf_set_time_out(0, __this->net_buf);

    os_sem_create(&__this->r_sem, 0);
    os_sem_create(&__this->start_dec_sem, 0);

    //编码参数和回调数据设置
    struct vir_voice_param param = {0};
    param.output = vir_voice_vfs_fwrite;
    param.code_type = AUDIO_CODING_PCM;
    param.complexity = 0;
    param.format_mode = 0;
    param.frame_ms = 0;
    param.quality = 0;

    //编码虚拟源输出
    __this->recorder = vir_dev_recorder_open(&param);

    //解码虚拟源输入
    thread_fork("virtual_dec_thread", 20, 2 * 1024, 0, 0, virtual_dec_thread, NULL);

    /* sys_timeout_add(NULL, vir_audio_close, 5*1000); */
}

#endif //VIRTUAL_AUDIO_TEST
