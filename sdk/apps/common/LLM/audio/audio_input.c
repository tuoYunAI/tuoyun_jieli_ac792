#include "app_config.h"
#include "audio_input.h"
#include "system/includes.h"
#include "device/device.h"
#include "event/device_event.h"
#include "usb/host/usb_host.h"
#include "usb/host/audio.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "jlstream.h"

#define MSG_START_NET_AUDIO_PLAY   1
#define MSG_START_LOCAL_AUDIO_PLAY   2
#define MSG_STOP_NET_AUDIO_PLAY   3
#define MSG_STOP_LOCAL_AUDIO_PLAY   4
#define MSG_START_AUDIO_RECORDER   5
#define MSG_STOP_AUDIO_RECORDER   6
#define MSG_START_AUDIO_TEST   7
#define MSG_STOP_AUDIO_TEST  8
#define MSG_AUDIO_RECORDER_FIRST_PLAY_NEXT_TEST   9
#define MSG_STOP_AUDIO_RECORDER_AND_START_PLAY_TEST   10
#define MSG_AUDIO_RECORDER_FIRST_PLAY_NEXT_STOP_PLAY_TEST   11


#define AUDIO_PLAY_VOICE_VOLUME   100
#define AUDIO_RECORD_VOICE_VOLUME 100

#ifdef CONFIG_VOLC_RTC_ENABLE
#define AUDIO_RECORD_VOICE_UPLORD_LEN (320) //RTC固定音频时长 10ms
#else
#define AUDIO_RECORD_VOICE_UPLORD_LEN (3200)
#endif

#define _AUDIO_TASK_NAME    "audio_task"
#define _AUDIO_WAIT_TIMEOUT    500
#define _AUDIO_UAC_TRY_TIMES    10

#define MSG_AUDIO_RECORDER_FIRST_PLAY_NEXT_TEST_STOP   0
#define MSG_AUDIO_RECORDER_FIRST_PLAY_NEXT_TEST_START   1
#define MSG_AUDIO_RECORDER_FIRST_PLAY_NEXT_TEST_RECORDER_START   2
#define MSG_AUDIO_RECORDER_FIRST_PLAY_NEXT_TEST_PLAY_START   3

#ifdef CONFIG_AEC_ENC_ENABLE
/*aec module enable bit define*/
#define AEC_EN              BIT(0)
#define NLP_EN              BIT(1)
#define ANS_EN              BIT(2)

#define AEC_MODE_ADVANCE	(AEC_EN | NLP_EN | ANS_EN)
#define AEC_MODE_REDUCE		(NLP_EN | ANS_EN)
#define AEC_MODE_SIMPLEX	(ANS_EN)
#define AEC_MODE_KWS	(AEC_EN | ANS_EN)
#endif

extern void *video_mic_recorder_open(u16 sample_rate, u8 code_type, void *priv, void (*cb)(void *, u8 *, u32));
extern int video_mic_recorder_close(void *priv);

extern void *vir_source_player_open(void *file, struct stream_file_ops *ops);
extern int vir_source_player_pp(void *player_);
extern int vir_source_player_resume(void *player_);
extern int vir_source_player_pause(void *player_);
extern int vir_source_player_start(void *player_);
extern void vir_source_player_close(void *player_);

typedef struct {
    CHAR_T *play_src;
    int   play_src_len;
    INT_T   play_len;
} _AUDIO_LOCAL_MSG;

typedef unsigned short WORD;

typedef struct {
    cbuffer_t pcm_cbuff_w;
    cbuffer_t pcm_cbuff_r;
    struct server *enc_server;
    struct server *dec_server;
    UCHAR_T   status;
    BOOL_T is_audio_play_open;
    BOOL_T is_audio_record_open;
} _AUDIO_HANDLE;

typedef struct {
    OS_SEM    r_sem;
    OS_QUEUE msg_que;
    BOOL_T pcm_wait_sem;
    void *task_handle;
} _AUDIO_CTRL;

typedef struct {
    unsigned int              cmd;
    UCHAR_T                  *data;
    unsigned int              data_len;
} _AUDIO_CTRL_MSG;

static _AUDIO_CTRL g_audio_ctrl = {0};
static _AUDIO_HANDLE g_audio_hdl = {0};
static _AUDIO_LOCAL_MSG  g_local_file = {0};
static unsigned int SAMPLE_RATE = 8000;//48000;//8000;
static unsigned int CHANNEL     = 1;
static unsigned int BIT_DEP     = 16;
static char audio_data[AUDIO_RECORD_VOICE_UPLORD_LEN] = {0};
static unsigned int voice_buf_size = AUDIO_RECORD_VOICE_UPLORD_LEN;
static int audio_power_off = 0;
static int audio_uac_retry_times = 0;
static unsigned int audio_test_recorder_first_play_next = 0;
static unsigned int audio_test_recorder_first_play_times = 0;


/***************************************audio*********************************************/

static int _send_audio_msg(IN CONST unsigned int msgid, IN CONST VOID *data, IN CONST unsigned int len)
{
    int op_ret = 0;
    int msg_num = 0;
    if (!&g_audio_ctrl.msg_que) {
        return -1;
    }

    _AUDIO_CTRL_MSG *msg_data;
    msg_data = malloc(sizeof(_AUDIO_CTRL_MSG) + 1);
    if (!msg_data) {
        return -1;
    }
    memset(msg_data, 0, sizeof(_AUDIO_CTRL_MSG) + 1);
    msg_data->cmd = msgid;

    if (data && len) {
        msg_data->data = malloc(len + 1);
        if (!msg_data->data) {
            if (msg_data) {
                free(msg_data);
            }
            return -1;
        }
        memset(msg_data->data, 0, len + 1);
        memcpy(msg_data->data, data, len);
        msg_data->data_len = len;
    } else {
        msg_data->data = NULL;
    }

    op_ret = os_q_post(&g_audio_ctrl.msg_que, msg_data);
    if (0 != op_ret) {
        if (msg_data->data) {
            free(msg_data->data);
        }
        if (msg_data) {
            free(msg_data);
        }
        return op_ret;
    }

    return 0;
}

int _device_get_voice_data(VOID *data, unsigned int max_len)
{
    cbuffer_t *cbuf = (cbuffer_t *)&g_audio_hdl.pcm_cbuff_w;
    unsigned int rlen = 0;
    static flag = 1;

#ifdef CONFIG_VOLC_RTC_ENABLE
    mdelay(20);
#endif
    rlen = cbuf_get_data_size(cbuf);
    if (rlen == 0) {
        /* audio_debug("device_get_voice_data no data"); */
        return 0;
    } else {
        /* audio_debug("device_get_voice_data %d",rlen); */
    }

    if (voice_buf_size <= cbuf_get_data_size(cbuf)) {
        rlen = rlen > max_len ? max_len : rlen;
        cbuf_read(cbuf, data, rlen);
        return rlen;
    }
    return 0;
}


int _device_write_voice_data(VOID *data, unsigned int len)
{
    cbuffer_t *cbuf = (cbuffer_t *)&g_audio_hdl.pcm_cbuff_r;
    int  i = 0;

    unsigned int write_len = cbuf_write(cbuf, data, len);
    /* printf("%s, len=%d wite_len=%d", __FUNCTION__, len, write_len); */
    if (0 == write_len) {
        //上层buf写不进去时清空一下，避免出现声音滞后的情况
        cbuf_clear(cbuf);
        /* audio_debug("cbuf_write full"); */
    } else if (write_len != len) {
        unsigned int rlen = cbuf_get_data_size(cbuf);
        /* audio_debug("wite_len %d len %d rlen %d", write_len, len, rlen); */
    } else {
        /* audio_debug("write_len %d len %d", write_len, len); */
    }

    os_sem_post(&g_audio_ctrl.r_sem);
    //此回调返回0录音就会自动停止
    return len;
}


static int virtual_dev_read(void *file, u8 *buf, int len)
{
    cbuffer_t *cbuf = NULL;
    cbuf = (cbuffer_t *)&g_audio_hdl.pcm_cbuff_r;
    int tmp_len;
    int rlen = len;
    unsigned int cbuf_len = 0;
    u8 *p = (u8 *)buf;
    do {
        cbuf_len = cbuf_get_data_size(cbuf);
        tmp_len = cbuf_len > rlen ? rlen : cbuf_len;
        cbuf_read(cbuf, p, tmp_len);
        rlen -= tmp_len;
        p += tmp_len;
        cbuf_len = cbuf_get_data_size(cbuf);
        if (cbuf_len == 0) {
            /* g_audio_ctrl.pcm_wait_sem = 1; */
            /* os_sem_pend(&g_audio_ctrl.r_sem, _AUDIO_WAIT_TIMEOUT); */
            os_sem_pend(&g_audio_ctrl.r_sem, 0);
            /* g_audio_ctrl.pcm_wait_sem = 0; */
        }
    } while (rlen > 0);

    return len - rlen;
}

static int virtual_dev_get_fmt(void *file, struct stream_fmt *fmt)
{
    /* if (!file) { */
    /* return -1; */
    /* } */

    fmt->sample_rate = SAMPLE_RATE;
    fmt->coding_type = AUDIO_CODING_PCM;
    fmt->channel_mode = AUDIO_CH_L;
    /* fmt->bit_wide = 16; */
    fmt->frame_dms = 4;

    return 0;
}

static const struct stream_file_ops virtual_dev_ops = {
    .read       = virtual_dev_read,
    .get_fmt    = virtual_dev_get_fmt,
};

//编码器输出PCM数据
static void recorder_vfs_fwrite(void *file, u8 *data, u32 len)
{
    /* static int cnt; */
    int ret;
    /* cbuffer_t *cbuf = (cbuffer_t *)file; */
    cbuffer_t *cbuf = (cbuffer_t *)&g_audio_hdl.pcm_cbuff_w;

    if (0 == cbuf_write(cbuf, data, len)) {
        // 上层buf写不进去时清空一下，避免出现声音滞后的情况
        cbuf_clear(cbuf);
    } else {
        /* audio_debug("cbuf write %d data", len); */
    }
    /* return len; */
}

int recoder_state = 0;


//*****************************收音和播放************************
VOID _device_net_audio(BOOL_T flag)
{
    audio_debug("ty_device_net_audio %d", flag);
    if (flag) {
        //音频播放功放
        _send_audio_msg(MSG_START_NET_AUDIO_PLAY, 0, NULL);
        _send_audio_msg(MSG_START_AUDIO_RECORDER, 0, NULL);
    } else {
        _send_audio_msg(MSG_STOP_NET_AUDIO_PLAY, 0, NULL);
        _send_audio_msg(MSG_STOP_AUDIO_RECORDER, 0, NULL);
    }

}

VOID _device_net_audio_play(BOOL_T flag)
{
    audio_debug("_device_net_audio_play %d", flag);
    if (flag) {
        //音频播放功放
        _send_audio_msg(MSG_START_NET_AUDIO_PLAY, 0, NULL);
    } else {
        _send_audio_msg(MSG_STOP_NET_AUDIO_PLAY, 0, NULL);
    }

}

VOID _device_net_audio_recorder(BOOL_T flag)
{
    audio_debug("_device_net_audio_recorder %d", flag);
    if (flag) {
        //音频播放功放
        _send_audio_msg(MSG_START_AUDIO_RECORDER, 0, NULL);
    } else {
        _send_audio_msg(MSG_STOP_AUDIO_RECORDER, 0, NULL);
    }

}
//***************************************************************
BOOL_T is_audio_play_open(VOID)
{
    return g_audio_hdl.is_audio_play_open;
}

static VOID __audio_task(void *pArg)
{
    int op_ret = 0;
    _AUDIO_CTRL_MSG *msg_data;
    int err;
    int msg[16] = {0,};
    while (1) {
        //阻塞等待消息
        op_ret = os_q_pend(&g_audio_ctrl.msg_que, 0, msg);
        if (op_ret != 0) {
            if (op_ret != -1) {
                audio_debug("tal_queue_fetch op_ret:%d", op_ret);
            }
            continue;
        }
        msg_data = (_AUDIO_CTRL_MSG *)msg[0];
        switch (msg_data->cmd) {
        case MSG_START_NET_AUDIO_PLAY: {
            cbuf_clear(&g_audio_hdl.pcm_cbuff_r);
            if (g_audio_hdl.is_audio_play_open) {
                vir_source_player_close(g_audio_hdl.dec_server);
                g_audio_hdl.dec_server = NULL;
            }
            if (!g_audio_hdl.dec_server) {
                g_audio_hdl.dec_server = vir_source_player_open(&g_audio_hdl.pcm_cbuff_r,  &virtual_dev_ops);
                if (!g_audio_hdl.dec_server) {
                    printf("dev flow open err");
                } else  {
                    audio_debug("dec server_open succ!");
                    vir_source_player_start(g_audio_hdl.dec_server);
                }
            }
            g_audio_hdl.is_audio_play_open = TRUE;
        }
        break;

        case MSG_STOP_NET_AUDIO_PLAY: {
            cbuf_clear(&g_audio_hdl.pcm_cbuff_r);
            g_audio_hdl.is_audio_play_open = FALSE;
            vir_source_player_close(g_audio_hdl.dec_server);
            g_audio_hdl.dec_server = NULL;
        }
        break;

        case MSG_START_AUDIO_RECORDER: {
            cbuf_clear(&g_audio_hdl.pcm_cbuff_w);
            if (g_audio_hdl.is_audio_record_open) {
                err = video_mic_recorder_close(g_audio_hdl.enc_server);
                g_audio_hdl.enc_server = NULL;
            }
            if (!g_audio_hdl.enc_server) {
                g_audio_hdl.enc_server = video_mic_recorder_open(SAMPLE_RATE, 0, NULL, recorder_vfs_fwrite);
            }
            g_audio_hdl.is_audio_record_open = true;
        }
        break;

        case MSG_STOP_AUDIO_RECORDER: {
            cbuf_clear(&g_audio_hdl.pcm_cbuff_w);
            g_audio_hdl.is_audio_record_open = FALSE;
            video_mic_recorder_close(g_audio_hdl.enc_server);
            g_audio_hdl.enc_server = NULL;
        }
        break;

        }
        if (msg_data) {
            if (msg_data->data) {
                free(msg_data->data);
            }
            free(msg_data);
            msg_data = NULL;
        }
    }
}

int audio_cfg_init(_AUDIO_PARAM *audio_param)
{
    audio_debug("into audio cfg init");
    int op_ret = -1;
    if (audio_param->sample_rate > 8000) {
        voice_buf_size = AUDIO_RECORD_VOICE_UPLORD_LEN;
    }
    audio_debug("audio param:%d %d %d %d", audio_param->sample_rate, audio_param->bit_dept, audio_param->channel_num, audio_param->audio_power_off);

    SAMPLE_RATE = audio_param->sample_rate;
    CHANNEL = audio_param->channel_num;
    BIT_DEP = audio_param->bit_dept;
    audio_power_off = audio_param->audio_power_off;
    return 0;
}

static int _audio_soft_init(VOID)
{
    int op_ret;
    u8 *pcm_buff_w = NULL;
    u8 *pcm_buff_r = NULL;
    audio_debug("into volc audio soft init");
    pcm_buff_w = malloc(SAMPLE_RATE * CHANNEL * 1);
    cbuf_init(&g_audio_hdl.pcm_cbuff_w, pcm_buff_w, SAMPLE_RATE * CHANNEL * 1);
    pcm_buff_r = malloc(SAMPLE_RATE * CHANNEL * 1);
    cbuf_init(&g_audio_hdl.pcm_cbuff_r, pcm_buff_r, SAMPLE_RATE * CHANNEL * 1);

    op_ret = os_sem_create(&g_audio_ctrl.r_sem, 0);
    if (op_ret != 0) {
        audio_debug("_hal_semaphore_create_init create semphore err:%d", op_ret);
        return op_ret;
    }

    g_audio_hdl.enc_server = video_mic_recorder_open(SAMPLE_RATE, 0, NULL, recorder_vfs_fwrite);
    if (g_audio_hdl.enc_server) {
        audio_debug("enc server_open succ!");
        g_audio_hdl.is_audio_record_open = true;
        goto play_;
    } else {
    }

play_:
    g_audio_hdl.dec_server = vir_source_player_open(&g_audio_hdl.pcm_cbuff_r,  &virtual_dev_ops);
    if (!g_audio_hdl.dec_server) {
        printf("dev flow open err");
    } else  {
        audio_debug("dec server_open succ!");
        /* mdelay(1*1000); */
        vir_source_player_start(g_audio_hdl.dec_server);
        printf("\n -[function] %s -[line] %d\n", __FUNCTION__, __LINE__);
        goto task_;
    }

task_:

    QS queue_size = (sizeof(_AUDIO_CTRL_MSG *) * 20 + sizeof(WORD) - 1) / sizeof(WORD);
    op_ret = os_q_create(&g_audio_ctrl.msg_que, queue_size);

    printf("msg que:%x, r_sem:%x", g_audio_ctrl.msg_que, g_audio_ctrl.r_sem);

    thread_fork(_AUDIO_TASK_NAME, 5, 512, 512, g_audio_ctrl.task_handle, __audio_task, NULL);

    audio_debug("_audio_task create");
    /* vir_source_player_start(g_audio_hdl.dec_server); */
    return op_ret;
}

VOID _audio_init(_AUDIO_PARAM *audio_param)
{
    audio_debug("into volc audio init!");
    static char init = 0;

    audio_cfg_init(audio_param);
    if (!init) {
        _audio_soft_init();
    }

    init = 1;
}

void audio_stream_init(int sample_rate, int bit_dept, int channel_num)
{
    _AUDIO_PARAM audio_param;
    audio_param.audio_power_off = false;
    audio_param.bit_dept = bit_dept;
    audio_param.channel_num = channel_num;
    audio_param.sample_rate = sample_rate;
    _audio_init(&audio_param);
}

void start_audio_stream(void)
{
    _device_net_audio(true);
}

void stop_audio_stream(void)
{
    _device_net_audio(false);
}

int get_recoder_state()
{
    return recoder_state;
}

