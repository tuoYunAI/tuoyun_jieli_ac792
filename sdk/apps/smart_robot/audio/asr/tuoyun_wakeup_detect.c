#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tuoyun_asr_recorder.data.bss")
#pragma data_seg(".tuoyun_asr_recorder.data")
#pragma const_seg(".tuoyun_asr_recorder.text.const")
#pragma code_seg(".tuoyun_asr_recorder.text")
#endif

#include "app_config.h"
#include "system/includes.h"
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

#if WAKEUP_ALGORITHM
typedef struct LDEEW_ENGINE LDEEW_ENGINE_S;

char *LDEEW_Get_DeviceName(void);
char *LDEEW_version(void);
int LDEEW_memSize(void);
LDEEW_ENGINE_S *LDEEW_RUN(void *pvMemBase, int len, void *pvWkpEnv, void *pvWkpHandler, void *pvAsrHandler);
int LDEEW_feed(LDEEW_ENGINE_S *pstLfespdEng, char *pcData, int iLen);

LDEEW_ENGINE_S *pstLfespdEng = NULL;

#endif


typedef struct tuoyun_asr_recorder {
    struct jlstream *stream;
    int inited;
    int started;
}tuoyun_asr_recorder_t, *tuoyun_asr_recorder_ptr;



static u8* m_buf;
static cbuffer_t cbuf;
static buf_len = 1024*20;

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
    if (g_tuoyun_asr_recorder->started == 0) {
        os_mutex_post(&mutex);
        return 0;
    }


    u32 wlen = cbuf_write(&cbuf, data, len);
    if (wlen == 0) {
        cbuf_clear(&cbuf);
        return 0;
    }

    os_mutex_post(&mutex);
    return wlen;
}

static const struct stream_file_ops asr_tx_ops = {
    .write = recorder_data_output,
};

static int vad_callback(enum vad_event event)
{
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
        cbuf_clear(&cbuf);
        g_tuoyun_asr_recorder->started = 1;
        break;
    case VAD_EVENT_SPEAK_STOP:
        log_info("@@@@@@@@@VAD_EVENT_SPEAK_STOP");
        int blen = cbuf_get_data_size(&cbuf);
        log_info("asr data len: %d", blen);
        g_tuoyun_asr_recorder->started = 0;
        char* buf = malloc(blen);
        if (!buf){
            log_error("failed to alloc mem");
            break;
        }
        int read_len = cbuf_read(&cbuf, buf, blen);
        if (read_len > 0) {
            #if WAKEUP_ALGORITHM
            LDEEW_feed(pstLfespdEng, (char *)buf, read_len);
            #endif
        }
        
        break;
    }
    os_mutex_post(&mutex);
    return 0;
}

void tuoyun_asr_recorder_open()
{
    int err;
    struct stream_fmt fmt = {0};
    struct encoder_fmt enc_fmt = {0};
    vad_node_priv_t vad = {0};

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

    cbuf_clear(&cbuf);
    g_tuoyun_asr_recorder->inited = 1;
    os_mutex_post(&mutex);

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


#if WAKEUP_ALGORITHM
#if AISP_DUAL_MIC_ALGORITHM
static int aec_handler(void *pvUsrData, s8 *pcData, s32 iLen, s8 iStatus)
#else
//typedef int (*LDEEW_handler_t)(void *user_data, int status, char *json, int bytes);
static int aec_handler(void *pcData, int iStatus, char *json, int iLen)
#endif
{
    u32 wlen;


    return 0;
}

static int wtk_handler(void *pvUsrData, int iIdx, char *pcJson)
{
    
    json_object *obj = NULL;
    const char *keyword = NULL;

    obj = json_tokener_parse(pcJson);
    if (obj) {
        keyword = json_get_string(obj, "wakeupWord");
        if (keyword) {
            log_info("@@@@@@@@@@@@@@@@@@@+++++++++={ wakeup_word: %s}\n", keyword);
            if (!strcmp(keyword, "zan ting bo fang")) {
                
            } else if (!strcmp(keyword, "bo fang yin yue")) {
                
            } else if (!strcmp(keyword, "shang yi shou")) {
                
            } else if (!strcmp(keyword, "xia yi shou")) {
                
            } else if (!strcmp(keyword, "da sheng yi dian")) {
                
            } else if (!strcmp(keyword, "xiao sheng yi dian")) {
                
            } else {
                
            }

        }
        json_object_put(obj);
    }

    return 0;
}



typedef struct dui_auth_info {
    char *productKey;
    char *productId;
    char *ProductSecret;
} dui_auth_info_t;

static void aisp_init()
{
    u32 memPoolLen = LDEEW_memSize();  //获取算法需要的heap大小
    u32 mic_len, linein_len;

    log_info("aisp_main run 0x%x,%s\r\n", (u32)LDEEW_version(), LDEEW_version());
    log_info("memPoolLen is:%d\n", memPoolLen);

    dui_auth_info_t auth_info;
    /* 此处信息请根据dui信息修改 https://www.duiopen.com/ */
    auth_info.productKey = "58f1aeeb54fadab27a6ce70fd222ec46";
    auth_info.productId = "279594353";
    auth_info.ProductSecret = "89249b0fb48d7c12454a079fc97aee72";
    extern int app_dui_auth_second(dui_auth_info_t *auth_info, u8 mark);
    if (0 != app_dui_auth_second(&auth_info, 0)) {
        log_info("dui auth fail, please contact aispeech!!!");
        return;
    }
    log_info("app_dui_auth_second ok");

    void *pcMemPool = calloc(1, memPoolLen);  //申请内存
    if (NULL == pcMemPool) {
        return;
    }

    /* start engine and pass auth cfg*/
    pstLfespdEng = LDEEW_RUN(pcMemPool, memPoolLen, "words=xiao ai tong xue,da sheng yi dian,xiao sheng yi dian,zan ting bo fang,xia yi shou;thresh=0.60,0.32,0.32,0.33,0.33;", wtk_handler, aec_handler); //唤醒词：小爱同学
    /* pstLfespdEng = LDEEW_RUN(pcMemPool, memPoolLen, "words=ni hao xiao le,da sheng yi dian,xiao sheng yi dian,zan ting bo fang,xia yi shou;thresh=0.60,0.32,0.32,0.33,0.33;", wtk_handler, aec_handler); //唤醒词：你好小乐 */
    if (NULL == pstLfespdEng || pstLfespdEng == (void *)0xffffffff) {
        log_error("LDEEW_Start auth fail \n");
        free(pcMemPool);
        return;
    }
    log_info("LDEEW_Start auth OK \n");


    free(pcMemPool);
}

#endif


static void asr_init(void *priv)
{
    m_buf = malloc(buf_len);
    if (m_buf == NULL) {
        log_info("malloc fail\n");
        return;
    }

    cbuf_init(&cbuf, m_buf, buf_len);

    if (os_mutex_create(&mutex) != OS_NO_ERR) {
        log_error("%s os_mutex_create buf_mutex fail\n", __FILE__);
        return;
    }

#if WAKEUP_ALGORITHM
    aisp_init();
#endif    
}

late_initcall(asr_init);