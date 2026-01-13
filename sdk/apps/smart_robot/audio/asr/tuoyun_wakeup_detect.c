#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tuoyun_asr_recorder.data.bss")
#pragma data_seg(".tuoyun_asr_recorder.data")
#pragma const_seg(".tuoyun_asr_recorder.text.const")
#pragma code_seg(".tuoyun_asr_recorder.text")
#endif


#include "app_config.h"
#include "system/includes.h"
#include "os/os_cfg.h"
#include "jlstream.h"
#include "app_audio.h"
#include "encoder_fmt.h"
#include "vad_node.h"
#include "generic/circular_buf.h"
#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "tuoyun_wakeup_detect.h"

#define LOG_TAG             "[ASR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"



#define WAKEUP_ALGORITHM   0

#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == AISP_ALGORITHM)

#if defined CONFIG_VIDEO_ENABLE || defined CONFIG_NO_SDRAM_ENABLE
#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic/1选择双mic算法
#else
#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic/1选择双mic算法
#endif

#endif


extern void aisp_suspend(void);

typedef struct tuoyun_asr_recorder {
    struct jlstream *stream;
    int inited;
}tuoyun_asr_recorder_t, *tuoyun_asr_recorder_ptr;

static tuoyun_asr_recorder_ptr g_tuoyun_asr_recorder = NULL;
static OS_MUTEX mutex;




static void asr_recorder_callback(void *private_data, int event)
{
    struct jlstream *stream = (struct jlstream *)private_data;
    
    switch (event) {
    case STREAM_EVENT_START:
        log_info("tuoyun asr recorder start");
        break;
        case STREAM_EVENT_STOP:
        log_info("tuoyun asr recorder stop");
        break;
    case STREAM_EVENT_END:
        log_info("tuoyun asr recorder end");
        break;
    default:
        log_info("***tuoyun asr recorder event: %d", event);
        break;    

    }
}

static int recorder_data_output(void *priv, u8 *data, int len)
{
    if (!data || len == 0) {
        return 0;
    }

    os_mutex_pend(&mutex, 0);
    if (!g_tuoyun_asr_recorder || !g_tuoyun_asr_recorder->inited) {
        os_mutex_post(&mutex);
        return 0;
    }

    extern int aisp_vfs_fwrite(void *data, u32 len);
    int wlen = aisp_vfs_fwrite(data, len);

    os_mutex_post(&mutex);
    return wlen;
}

static const struct stream_file_ops asr_tx_ops = {
    .write = recorder_data_output,
};

static int vad_callback(enum vad_event event)
{
#if 0     //必须一直输入pcm数据来维持kws工作
    os_mutex_pend(&mutex, 0);
    if (!g_tuoyun_asr_recorder || !g_tuoyun_asr_recorder->inited) {
        os_mutex_post(&mutex);
        log_info("vad_callback recorder not inited");
        return 0;
    }
    log_info("vad_callback event: %d", event);

    switch (event) {
    case VAD_EVENT_SPEAK_START:
        log_info("@@@@@@@@@VAD_EVENT_SPEAK_START");
        break;
    case VAD_EVENT_SPEAK_STOP:
        log_info("@@@@@@@@@VAD_EVENT_SPEAK_STOP");   
        break;
    }
    os_mutex_post(&mutex);
#endif    
    return 0;
}

void tuoyun_asr_recorder_open()
{
    int err;
    struct stream_fmt fmt = {0};
    struct encoder_fmt enc_fmt = {0};
    vad_node_priv_t vad = {0};
    log_info("tuoyun_asr_recorder_open ---- open");

    os_mutex_pend(&mutex, 0);
    if (g_tuoyun_asr_recorder) {
        if (g_tuoyun_asr_recorder->inited){
            os_mutex_post(&mutex);
            log_info("tuoyun asr recorder already inited");
            return;
        }
        if (g_tuoyun_asr_recorder->stream) {
            jlstream_stop(g_tuoyun_asr_recorder->stream, 0);
            jlstream_release(g_tuoyun_asr_recorder->stream);
            g_tuoyun_asr_recorder->stream = NULL;
        }
        free(g_tuoyun_asr_recorder);
        g_tuoyun_asr_recorder = NULL;
    }

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"ai_voice");
    if (uuid == 0) {
        os_mutex_post(&mutex);
        return;
    }
    
    g_tuoyun_asr_recorder = zalloc(sizeof(*g_tuoyun_asr_recorder));
    if (!g_tuoyun_asr_recorder) {
        os_mutex_post(&mutex);
        return;
    }
    
    g_tuoyun_asr_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!g_tuoyun_asr_recorder->stream) {
        g_tuoyun_asr_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_PDM_MIC);
    }
    if (!g_tuoyun_asr_recorder->stream) {
        g_tuoyun_asr_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS0_RX);
    }
    if (!g_tuoyun_asr_recorder->stream) {
        goto __exit0;
    }

    fmt.sample_rate = 16000;
    fmt.coding_type = AUDIO_CODING_PCM;
    enc_fmt.frame_dms = 20 * 10;    //与工具保持一致，要乘以10,表示20ms

    err = jlstream_node_ioctl(g_tuoyun_asr_recorder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_PRIV_FMT, (int)(&enc_fmt));
    if (err) {
        goto __exit1;
    }

    err = jlstream_node_ioctl(g_tuoyun_asr_recorder->stream, NODE_UUID_AI_TX, NODE_IOC_SET_FMT, (int)(&fmt));
    if (err) {
        goto __exit1;
    }

    //设置ADC的中断点数
    err = jlstream_node_ioctl(g_tuoyun_asr_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 320);
    if (err) {
        goto __exit1;
    }

    vad.vad_callback = vad_callback;
    jlstream_node_ioctl(g_tuoyun_asr_recorder->stream, NODE_UUID_VAD, NODE_IOC_SET_PRIV_FMT, (int)&vad);

    struct stream_file_info info = {
        .file = (void *)g_tuoyun_asr_recorder,
        .ops = &asr_tx_ops
    };
    err = stream_node_ioctl((struct stream_node *)g_tuoyun_asr_recorder->stream->snode, NODE_UUID_AI_TX, NODE_IOC_SET_FILE, (int)&info);
    if (err) {
        goto __exit1;
    }

    jlstream_set_callback(g_tuoyun_asr_recorder->stream, g_tuoyun_asr_recorder->stream, asr_recorder_callback);
    jlstream_set_scene(g_tuoyun_asr_recorder->stream, STREAM_SCENE_AI_VOICE);

    err = jlstream_start(g_tuoyun_asr_recorder->stream);
    if (err) {
        goto __exit1;
    }

    g_tuoyun_asr_recorder->inited = 1;
    os_mutex_post(&mutex);
    aisp_resume();

    log_info("tuoyun asr recorder init success");
    return;

__exit1:
    jlstream_release(g_tuoyun_asr_recorder->stream);
__exit0:
    free(g_tuoyun_asr_recorder);
    g_tuoyun_asr_recorder = NULL;
    os_mutex_post(&mutex);    
    return;
}

int tuoyun_asr_recorder_close()
{
    log_info("tuoyun_asr_recorder_close ---- close");
    aisp_suspend();
    os_mutex_pend(&mutex, 0);
    if (!g_tuoyun_asr_recorder) {
        os_mutex_post(&mutex);    
        return 0;
    }

    jlstream_stop(g_tuoyun_asr_recorder->stream, 0);
    jlstream_release(g_tuoyun_asr_recorder->stream);
    jlstream_event_notify(STREAM_EVENT_CLOSE_RECORDER, (int)"ai_voice");

    free(g_tuoyun_asr_recorder);
    g_tuoyun_asr_recorder = NULL;

    os_mutex_post(&mutex);    
    log_info("tuoyun asr recorder close");
    return 0;
}

static void aisp_keep_alive_worker(void *priv)
{
    extern void aisp_close();
    aisp_close();
    log_info("@@++@@ close aisp closed");
    os_time_dly(10);
    extern int aisp_open();
    log_info("@@++@@ close aisp started");
    aisp_open();

    os_mutex_pend(&mutex, 0);
    if (!g_tuoyun_asr_recorder || !g_tuoyun_asr_recorder->inited) {
        aisp_suspend();
    }
    os_mutex_post(&mutex);
}

static void aisp_keeper(void *priv)
{
    thread_fork("aisp_keep_alive_worker", 25, 8000, 0, 0, aisp_keep_alive_worker, NULL);
    return;
}

static void asr_init(void *priv)
{
    if (os_mutex_create(&mutex) != OS_NO_ERR) {
        log_error("%s os_mutex_create buf_mutex fail\n", __FILE__);
        return;
    }

    extern int aisp_open();
    aisp_open();

    /**
     * 每10分钟重启一次aisp
     */
    sys_timer_add_to_task("sys_timer", NULL, aisp_keeper, 10 * 60 * 1000);

}


late_initcall(asr_init);

