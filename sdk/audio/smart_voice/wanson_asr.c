#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".wasnon_asr.data.bss")
#pragma data_seg(".wanson_asr.data")
#pragma const_seg(".wasnon_asr.text.const")
#pragma code_seg(".wanson_asr.text")
#endif
/*****************************************************************
>file name : wanson_asr.c
>create time : Thu 23 Dec 2021 10:00:58 AM CST
>wanson语音识别算法平台接入
*****************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "wanson_asr.h"
#include "smart_voice.h"
#include "voice_mic_data.h"

#define ASR_FRAME_SAMPLES   480 /*语音识别帧长（采样点）*/

#if ((defined TCFG_AUDIO_ASR_DEVELOP) && (TCFG_AUDIO_ASR_DEVELOP == ASR_CFG_WANSON))
struct wanson_platform_asr_context {
    void *mic;
    u8 core;
    u8 data_enable;
    Fst fst_1;
    Fst fst_2;
};

extern const int config_wanson_asr_enable;

static struct wanson_platform_asr_context *__this = NULL;

#define WANSON_AEC_DATA_TO_SD 0

static void Wanson_ASR_Reset(unsigned char domain)
{
    __this->fst_1.states = fst01_states;
    __this->fst_1.num_states = fst01_num_states;
    __this->fst_1.finals = fst01_finals;
    __this->fst_1.num_finals = fst01_num_finals;
    __this->fst_1.words = fst01_words;

    __this->fst_2.states = fst02_states;
    __this->fst_2.num_states = fst02_num_states;
    __this->fst_2.finals = fst02_finals;
    __this->fst_2.num_finals = fst02_num_finals;
    __this->fst_2.words = fst02_words;

    if (domain == 0) {
        Wanson_ASR_Set_Fst(&__this->fst_1);
    } else if (domain == 1) {
        Wanson_ASR_Set_Fst(&__this->fst_2);
    }
}


static int wanson_algorithm_init(void)
{
    if (Wanson_ASR_Init() < 0) {
        return 0;
    }

    Wanson_ASR_Reset(0);
    return 1;
}

/*
 * 算法引擎打开函数
 */
static int wanson_asr_core_open(int sample_rate)
{
    return wanson_algorithm_init();
}

/*
 * 算法引擎关闭函数
 */
static void wanson_asr_core_close(u8 core)
{
    Wanson_ASR_Release();
    return;
}

/*
 * 算法引擎数据处理
 */
static int wanson_asr_core_data_handler(u8 core, void *data, int len)
{
    float score = 0;
    const char *text = NULL;

    if (1 == Wanson_ASR_Recog(data, ASR_FRAME_SAMPLES, &text, &score)) {
#if WANSON_AEC_DATA_TO_SD
        aec_data_to_sd_close();
#endif
        printf("{ ============wanson_wakeup_word: %s}\n", text);
    }

    return 0;
}

/*
*********************************************************************
*       wanson asr data handler
* Description: 用户算法平台平台语音识别数据处理
* Arguments  : asr - 语音识别数据管理结构
* Return	 : 0 - 处理成功， 非0 - 处理失败.
* Note(s)    : 该函数通过读取mic数据送入算法引擎完成语音帧.
*********************************************************************
*/
static int wanson_asr_data_handler(struct wanson_platform_asr_context *asr)
{
    if (!asr->mic) {
        return -EINVAL;
    }

    s16 data[ASR_FRAME_SAMPLES];

    int len = voice_mic_data_read(asr->mic, data, sizeof(data));
    if (len < sizeof(data)) {
        return -EINVAL;
    }

    if (asr->core) {
        wanson_asr_core_data_handler(asr->core, data, sizeof(data));
    }

    return 0;
}

/*
*********************************************************************
*       wanson_asr core handler
* Description: 用户自定义语音识别处理
* Arguments  : priv - 语音识别私有数据
*              taskq_type - TASK消息类型
*              msg  - 消息存储指针(对应自身模块post的消息)
* Return	 : None.
* Note(s)    : 音频平台资源控制以及ASR主要识别算法引擎.
*********************************************************************
*/
int wanson_asr_core_handler(void *priv, int taskq_type, int *msg)
{
    struct wanson_platform_asr_context *asr = (struct wanson_platform_asr_context *)priv;
    int err = ASR_CORE_STANDBY;

    if (taskq_type != OS_TASKQ) {
        return err;
    }

    switch (msg[0]) {
    case SMART_VOICE_MSG_MIC_OPEN: /*语音识别打开 - MIC打开，算法引擎打开*/
#if WANSON_AEC_DATA_TO_SD
        aec_data_to_sd_open();
#endif
        /* msg[1] - MIC数据源，msg[2] - 音频采样率，msg[3] - mic的数据总缓冲长度*/
        asr->mic = voice_mic_data_open(msg[1], msg[2], msg[3]);
        asr->core = wanson_asr_core_open(msg[2]);
        asr->data_enable = 1;
        err = ASR_CORE_WAKEUP;
#if (TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC)
        audio_smart_voice_aec_open();
#endif
        break;
    case SMART_VOICE_MSG_SWITCH_SOURCE:
        /*这里进行MIC的数据源切换 （主系统MIC或VAD MIC）*/

        break;
    case SMART_VOICE_MSG_MIC_CLOSE: /*语音识别关闭 - MIC关闭，算法引擎关闭*/
        /* msg[2] - 信号量*/
#if (TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC)
        audio_smart_voice_aec_close();
#endif
        voice_mic_data_close(asr->mic);
        asr->mic = NULL;
        wanson_asr_core_close(asr->core);
        asr->core = 0;
        asr->data_enable = 0;
        os_sem_post((OS_SEM *)msg[1]);
        break;
    case SMART_VOICE_MSG_DMA: /*MIC通路数据DMA消息*/
        err = ASR_CORE_WAKEUP;
        break;
    default:
        break;
    }

    if (asr->data_enable) {
        err = wanson_asr_data_handler(asr);
        err = err ? ASR_CORE_STANDBY : ASR_CORE_WAKEUP;
    }
    return err;
}

int wanson_platform_asr_open(void)
{
    if (!config_wanson_asr_enable) {
        return 0;
    }

    struct wanson_platform_asr_context *asr = (struct wanson_platform_asr_context *)zalloc(sizeof(struct wanson_platform_asr_context));

    if (!asr) {
        return -ENOMEM;
    }

    int err = smart_voice_core_create(asr);
    if (err != OS_NO_ERR) {
        goto __err;
    }

    smart_voice_core_post_msg(4, SMART_VOICE_MSG_MIC_OPEN, VOICE_MCU_MIC, 16000, 4096);

    __this = asr;
    return 0;
__err:
    if (asr) {
        free(asr);
    }
    return err;
}

void wasnson_platform_asr_close(void)
{
    if (!config_wanson_asr_enable) {
        return;
    }

    if (__this) {
        OS_SEM *sem = (OS_SEM *)malloc(sizeof(OS_SEM));
        os_sem_create(sem, 0);
        smart_voice_core_post_msg(2, SMART_VOICE_MSG_MIC_CLOSE, (int)sem);
        os_sem_pend(sem, 0);
        os_sem_del(sem, OS_DEL_ALWAYS);
        free(sem);
        smart_voice_core_free();
        free(__this);
        __this = NULL;
    }
}

void wanson_asr_hdl_free()
{
    if (__this) {
        free(__this);
        __this = NULL;
    }
}

#endif
