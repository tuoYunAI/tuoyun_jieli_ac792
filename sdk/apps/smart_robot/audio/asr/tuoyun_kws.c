#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "event.h"
#include "app_config.h"
#include "jlsp_far_keyword.h"
#include "event/key_event.h"
#include "app_audio.h"
#include "app_event.h"
/* #include "jlsp_kws_aec.h" */

#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == JLKWS_ALGORITHM)

#define LOG_TAG             "[TYJLKWS]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

enum {
    WAKE_UP_WORD_XIAOJIE = 2,
    PLAY_MUSIC_EVENT = 4,
    STOP_MUSIC_EVENT,
    VOLUME_INC_EVENT = 7,
    VOLUME_DEC_EVENT,
    SONG_PREVIOUS_EVENT,
    SONG_NEXT_EVENT,
};


void aisp_resume(void);

#define ONCE_SR_POINTS	160//256

#define AISP_BUF_SIZE	(ONCE_SR_POINTS * 2 * 5)	//跑不过来时适当加大倍数
#define MIC_SR_LEN		(ONCE_SR_POINTS * 2)

#ifdef CONFIG_AEC_ENC_ENABLE

/*aec module enable bit define*/
#define AEC_EN              BIT(0)
#define NLP_EN              BIT(1)
#define ANS_EN              BIT(2)

#define AEC_MODE_ADVANCE	(AEC_EN | NLP_EN | ANS_EN)
#define AEC_MODE_REDUCE		(NLP_EN | ANS_EN)
#define AEC_MODE_SIMPLEX	(ANS_EN)

#endif

#define AEC_DATA_TO_SD 0 //将唤醒前的mic/dac/aec数据写卡进行查看,3channel(mic,dac,aec)

static struct {
    int pid;
    u8 volatile exit_flag;
    int volatile run_flag;
    OS_SEM sem;
    s16 mic_buf[AISP_BUF_SIZE * 2];
    cbuffer_t mic_cbuf;
} aisp_server;

#define __this (&aisp_server)

static const float confidence[8] = {
    0.5, 0.5, 0.5, 0.5, //小杰小杰，小杰同学，播放音乐，暂停播放
    0.5, 0.5, 0.5, 0.5, //增大音量，减小音量，上一首, 下一首
};

static char *STR_WAKE_UP_WORD_XIAOJIE = "小杰";
static void aisp_task(void *priv)
{
    u32 mic_len, linein_len;
    u32 time = 0, time_cnt = 0, cnt = 0;
    int ret;
    int model = 0;
    int model_size, private_heap_size, share_heap_size;
    void *kws = NULL;
    u8 *private_heap = NULL, *share_heap = NULL;
    int online = 0;

    jl_far_kws_model_get_heap_size(model, &model_size, &private_heap_size, &share_heap_size);

    private_heap = zalloc(private_heap_size);
    if (!private_heap) {
        log_error("malloc private heap fail");
        goto __exit;
    }

    share_heap	 = zalloc(share_heap_size);
    if (!share_heap) {
        log_error("malloc share heap fail");
        goto __exit;
    }

    kws = jl_far_kws_model_init(model, private_heap, private_heap_size, share_heap, share_heap_size, model_size, confidence, online);
    if (!kws) {
        log_error("jlkws init fail");
        goto __exit;
    }

    log_info("jlkws init success");
    aisp_resume();

    while (1) {
        if (__this->exit_flag) {
            log_info("jlkws task exit");
            break;
        }

        if (!__this->run_flag) {
            log_info("jlkws task pause");
            os_sem_pend(&__this->sem, 0);
            continue;
        }
        
        u32 len = cbuf_get_data_size(&__this->mic_cbuf);
        if (len < ONCE_SR_POINTS * 2) {
            os_time_dly(8);
            continue;
        }

        short near_data_buf[ONCE_SR_POINTS];
        mic_len = cbuf_read(&__this->mic_cbuf, near_data_buf, ONCE_SR_POINTS * 2);
        if (!mic_len) {
            continue;
        }

        time = timer_get_ms();
        ret = jl_far_kws_model_process(kws, model, (u8 *)near_data_buf, sizeof(near_data_buf));
        if (ret > 1) {
            log_info("jlkws wakeup event : %d", ret);

            //add your button event according to ret
            
            if (ret == WAKE_UP_WORD_XIAOJIE) {
                log_info("jlkws wakeup word detected");
                struct app_event event = {
                    .event = APP_EVENT_WAKEUP_WORD_DETECTED,
                    .arg = (void *)STR_WAKE_UP_WORD_XIAOJIE,
                };
                app_event_notify(APP_EVENT_FROM_USER, &event);
            } else{
                struct key_event key = {0};
                if (ret == PLAY_MUSIC_EVENT) {
                    key.action = KEY_EVENT_UP;
                    key.value = KEY_LOCAL;
                } else if (ret == STOP_MUSIC_EVENT) {
                    key.action = KEY_EVENT_HOLD;
                    key.value = KEY_LOCAL;
                } else if (ret == VOLUME_INC_EVENT) {
                    key.action = KEY_EVENT_CLICK;
                    key.value = KEY_VOLUME_INC;
                } else if (ret == VOLUME_DEC_EVENT) {
                    key.action = KEY_EVENT_CLICK;
                    key.value = KEY_VOLUME_DEC;
                } else if (ret == SONG_PREVIOUS_EVENT) {
                    key.action = KEY_EVENT_LONG;
                    key.value = KEY_UP;
                } else if (ret == SONG_NEXT_EVENT) {
                    key.action = KEY_EVENT_LONG;
                    key.value = KEY_DOWN;
                } else {
                    key.action = KEY_EVENT_CLICK;
                    key.value = KEY_ENC;
                }

                key.type = KEY_EVENT_USER;
                key_event_notify(KEY_EVENT_FROM_USER, &key);
            }

            jl_far_kws_model_reset(kws);
        }

        time_cnt += timer_get_ms() - time;
        if (++cnt == 100) {
            /* printf("aec time :%d \n", time_cnt); */
            time_cnt = cnt = 0;
        }
    }

__exit:

    if (kws) {
        jl_far_kws_model_free(kws);
    }
    if (private_heap) {
        free(private_heap);
    }
    if (share_heap) {
        free(share_heap);
    }

    __this->run_flag = 0;

}

int aisp_vfs_fwrite(void *data, u32 len)
{
    cbuffer_t *cbuf = &__this->mic_cbuf;

    u32 wlen = cbuf_write(cbuf, data, len);
    if (wlen != len) {
        cbuf_clear(cbuf);
        log_warn("jlkws busy!");
    }
    return len;
}

static int aisp_vfs_fclose(void *file)
{
    return 0;
}


int aisp_open()
{
    __this->exit_flag = 0;
    cbuf_init(&__this->mic_cbuf, __this->mic_buf, sizeof(__this->mic_buf));
    os_sem_create(&__this->sem, 0);

    return thread_fork("aisp", 3, 1024, 0, &__this->pid, aisp_task, __this);
}

void aisp_suspend(void)
{
    log_info("++++++aisp_suspend");
    if (!__this->run_flag) {
        return;
    }

    __this->run_flag = 0;
    cbuf_clear(&__this->mic_cbuf);
}

void aisp_resume(void)
{   

    log_info("++++++aisp_resume");
    if (__this->run_flag) {
        return;
    }
    __this->run_flag = 1;
    os_sem_set(&__this->sem, 0);
    os_sem_post(&__this->sem);
}

void aisp_close(void)
{
    if (__this->exit_flag) {
        return;
    }

    aisp_suspend();

    __this->exit_flag = 1;

    os_sem_post(&__this->sem);

    thread_kill(&__this->pid, KILL_WAIT);
}

#endif

