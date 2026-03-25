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
static OS_MUTEX mutex;
static int m_asr_recording = 0;
extern void aisp_resume(void);


static int recorder_data_output(void *priv, u8 *data, int len)
{
    if(m_asr_recording == 0){
        return len;
    }
    if (!data || len == 0) {
        return 0;
    }

    extern int aisp_vfs_fwrite(void *data, u32 len);
    int wlen = aisp_vfs_fwrite(data, len);
    return wlen;
}


static int vad_callback(enum vad_event event)
{
#if 0     //必须一直输入pcm数据来维持kws工作

#endif    
    return 0;
}

void tuoyun_asr_recorder_open()
{
#if LOCAL_AUDIO_LOOP_TEST
    return;
#endif    
    aisp_resume();
    os_mutex_pend(&mutex, 0);
    m_asr_recording = 1;
    os_mutex_post(&mutex);
    log_info("tuoyun asr recorder init success");   
    return;
}

int tuoyun_asr_recorder_close()
{
#if LOCAL_AUDIO_LOOP_TEST
    return 0;
#endif    
    os_mutex_pend(&mutex, 0);
    m_asr_recording = 0;
    os_mutex_post(&mutex);
    aisp_suspend();
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
    if (!m_asr_recording) {
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
    void *video_mic_recorder_open(u16 sample_rate, u8 code_type, void *priv, void (*cb)(void *, u8 *, u32));
    video_mic_recorder_open(16000, 0, NULL, recorder_data_output);
}


late_initcall(asr_init);

