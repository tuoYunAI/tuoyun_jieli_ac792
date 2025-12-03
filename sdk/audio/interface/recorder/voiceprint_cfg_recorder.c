#include "app_config.h"
#include "os/os_api.h"
#include "jlstream.h"
#include "media/includes.h"

#if (defined CONFIG_ACOUSTIC_COMMUNICATION_ENABLE) && (defined CONFIG_MEDIA_ENABLE)

#include "voiceprint_cfg.h"
#include "MultSine_Control.h"

#define LOG_TAG             "[NET_CFG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


typedef struct __us_ctl {
    s16  us_buf[2][512];
    u8   us_bufnum;
    u8   csb_key[2];
    u8   csb_cnt;
} _us_ctl;

typedef struct __us_wifi {
    u8 ssid_pwd[128];
    u8 ssid_pwd_cnt;
    u8 head_is_recv;
    u8 lens;
    u8 j;
    u8 l;
    u32 time_out_cnt;
} _us_wifi;

enum {
    US_COME_CMD,
    US_CLOSE_CMD,
    US_INIT_CMD,
    US_CALC_CMD,
    US_WIFI_SUCC,
    US_BYTE_OT,
};


#define US_STA_NAME_LEN    33
#define US_STA_PWD_LEN     65
#define US_RANDOM_LEN   	8
#define TotalFreq          17

struct vp_cfg {
    struct jlstream *stream;
    _us_ctl us_ctl;
    _us_wifi us_wifi;
    u8 us_ssid_pwd_is_recv;
    char us_ssid[US_STA_NAME_LEN];
    char us_pwd[US_STA_PWD_LEN];
    u8 us_random[US_RANDOM_LEN];
    int pre_code;
    MultSine_Control *us_con;
    int pid;
};
static struct vp_cfg *vp_cfg;

#define VOICE_PRINT_TASK_NAME "voiceprint"

static void media_rec_stop(void);
static void media_rec_start(void);

static int const coeff[TotalFreq] = {
    //48000
    615432,
    818268,
    1019133,
    1217542,
    1413018,
    1605091,
    1793296,
    1977181,
    2156303,
    2330230,
    2498544,
    2660838,
    2816722,
    2965821,
    3107774,
    3242241,
    3368897
};

static void us_code_post(int code)
{
    int  i;
    int real_code;

    for (i = 0; i < TotalFreq; i++) {
        if (code & (1 << i)) {
            code = i;
        }
    }

    vp_cfg->us_wifi.time_out_cnt = 0;

    if (code != vp_cfg->pre_code) {
        if (code == 0x10) {
            real_code = vp_cfg->pre_code;
        } else {
            real_code = code;
        }

        vp_cfg->pre_code = code;

        if (vp_cfg->us_ctl.csb_cnt == 0) {
            vp_cfg->us_ctl.csb_key[0] = real_code;
            vp_cfg->us_ctl.csb_cnt = 1;
            /*os_taskq_post(VOICE_PRINT_TASK_NAME, 1, US_COME_CMD);*/
        } else {
            vp_cfg->us_ctl.csb_key[1] = real_code ;
            vp_cfg->us_ctl.csb_cnt = 0;
            os_taskq_post(VOICE_PRINT_TASK_NAME, 1, US_COME_CMD);
        }
    }
}

static void us_close(void)
{
    log_info("us_close");

    media_rec_stop();
}

static void us_init(void)
{
    log_info("us init");

    MultSine_Control_init(vp_cfg->us_con, TotalFreq, 256, 1, 2000, us_code_post, coeff, 1);

    media_rec_start();
}

static u8 get_ssid_pwd_deal(u8 *buf, u8 len)
{
    u8 i, fen = 0, dian = 0, rand = 0;
    for (i = 0; i < len; i++) {
        if (buf[i] == 0x02) {
            fen = i;
        } else if (buf[i] == 0x03) {
            dian = i;
        } else if (buf[i] == 0x04) {
            rand = i;
        }
    }

    if (fen == 0 || dian == 0 || rand == 0 || fen >= dian || dian >= rand) {
        return -1;
    }

    memset(vp_cfg->us_ssid, 0, US_STA_NAME_LEN);
    memset(vp_cfg->us_pwd, 0, US_STA_PWD_LEN);
    memset(vp_cfg->us_random, 0, US_RANDOM_LEN);
    memcpy(vp_cfg->us_ssid, buf, fen);
    memcpy(vp_cfg->us_pwd, buf + fen + 1, dian - fen - 1);
    memcpy(vp_cfg->us_random, buf + dian + 1, rand - dian - 1);
    vp_cfg->us_ssid_pwd_is_recv = 1;

    log_info("us_ssid:%s", vp_cfg->us_ssid);
    log_info("us_pwd :%s", vp_cfg->us_pwd);
    log_info("us_rand :%s", vp_cfg->us_random);

    os_taskq_post(VOICE_PRINT_TASK_NAME, 1, US_WIFI_SUCC);

    return 0;
}

static void us_wifi_byte_deal(u8 a, u8 b)
{
    u8 i_cnt;
    u8 val = a << 4 | b;

    log_info("us recv: 0x%x, %c", val, val);

    vp_cfg->us_wifi.ssid_pwd[vp_cfg->us_wifi.ssid_pwd_cnt] = val;
    if (vp_cfg->us_wifi.ssid_pwd_cnt == 0) {
        vp_cfg->us_wifi.j = val;
        if (vp_cfg->us_wifi.j != 'J') {
            goto _us_wifi_reset;
        }

        log_info("us recv J");
    }

    if (vp_cfg->us_wifi.ssid_pwd_cnt == 1) {
        vp_cfg->us_wifi.l = val;
        if (vp_cfg->us_wifi.l != 'L') {
            goto _us_wifi_reset;
        }

        log_info("us recv L");
    }

    if (vp_cfg->us_wifi.ssid_pwd_cnt == 2) {
        vp_cfg->us_wifi.lens = val;
        if (vp_cfg->us_wifi.lens >= 128) {
            goto _us_wifi_reset;
        }
        /*printf(" us recv len = %d\n", vp_cfg->us_wifi.lens);*/
    }

    vp_cfg->us_wifi.ssid_pwd_cnt++;
    if (vp_cfg->us_wifi.ssid_pwd_cnt >= 128) {
        goto _us_wifi_reset;
    }

    if (vp_cfg->us_wifi.ssid_pwd_cnt == vp_cfg->us_wifi.lens) {
        //收完一整个包,校验解释数据
        val = 0;
        for (i_cnt = 0; i_cnt < vp_cfg->us_wifi.lens - 1; i_cnt++) {
            val += vp_cfg->us_wifi.ssid_pwd[i_cnt];
        }
        /*printf("us val = 0x%x,recv = 0x%x\n", val, vp_cfg->us_wifi.ssid_pwd[vp_cfg->us_wifi.lens - 1]);*/
        if (val != vp_cfg->us_wifi.ssid_pwd[vp_cfg->us_wifi.lens - 1]) {
            goto _us_wifi_reset;
        }

        val = get_ssid_pwd_deal(vp_cfg->us_wifi.ssid_pwd + 3, vp_cfg->us_wifi.lens - 4); //去掉头部和校验
        if (val) {
            goto _us_wifi_reset;
        }
    }

    return;

_us_wifi_reset:
    log_info("us wifi reset...");

    memset(&vp_cfg->us_wifi, 0, sizeof(_us_wifi));
    vp_cfg->us_ctl.csb_cnt = 0;
    vp_cfg->pre_code = 0x10;

    return;
}

#if 0
static void ustimeout_fun(void *parm)
{
    //2ms
    parm = parm;

    //多久未收到一个字节超时
    vp_cfg->us_wifi.time_out_cnt++;//0.8s
    if (vp_cfg->us_wifi.time_out_cnt >= 400) {
        printf("us ot\n");
        vp_cfg->us_wifi.time_out_cnt = 0;
        os_taskq_post(VOICE_PRINT_TASK_NAME, 1, US_BYTE_OT);
    }

}
#endif

static void us_task(void *arg)
{
    u8 temp_us_bufnum;
    int res;
    int msg[32];

    vp_cfg->pre_code = 0x10;
    us_init();

    while (1) {
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        switch (res) {
        case OS_TASKQ:
            switch (msg[0]) {
            case Q_USER:
                switch (msg[1]) {
                case US_CALC_CMD:
                    temp_us_bufnum = vp_cfg->us_ctl.us_bufnum ^ 1;
                    if (vp_cfg->us_ssid_pwd_is_recv == 0) {
#if 0
                        static FILE *file_test;
                        if (file_test == NULL) {
                            file_test = fopen("mnt/spiflash/res/ms.raw", "r");
                        }
                        res = fread(file_test, &vp_cfg->us_ctl.us_buf[temp_us_bufnum], 512);
                        if (res != 512) {
                            fclose(file_test);
                            file_test = NULL;
                        }
#endif
                        MultSine_Control_run(vp_cfg->us_con, (short *)&vp_cfg->us_ctl.us_buf[temp_us_bufnum], 256);
                    }
                    break;

                case US_WIFI_SUCC:
                    log_info("US_WIFI_SUCC");
                    extern int voiceprint_cfg_net_set_ssid_pwd(const char *ssid, const char *pwd, const char *rand_str);
                    voiceprint_cfg_net_set_ssid_pwd(vp_cfg->us_ssid, vp_cfg->us_pwd, (const char *)vp_cfg->us_random);
                case US_CLOSE_CMD:
                    log_info("US_CLOSE_CMD");
                    us_close();
                    return;
                case US_BYTE_OT:
                    memset(&vp_cfg->us_wifi, 0, sizeof(_us_wifi));
                    vp_cfg->us_ctl.csb_cnt = 0;
                    vp_cfg->pre_code = 0x10;
                    MultSine_Control_init(vp_cfg->us_con, TotalFreq, 256, 1, 2000, us_code_post, coeff, 1);
                    break;
                case US_COME_CMD:
                    //us_printf("US_COME_CMD:%d   %d\n",vp_cfg->us_ctl.csb_key[0],vp_cfg->us_ctl.csb_key[1]);
                    us_wifi_byte_deal(vp_cfg->us_ctl.csb_key[0], vp_cfg->us_ctl.csb_key[1]);
                    break;
                default :
                    break;
                }
                break;
            default:
                break;
            }
            break;
        }
    }
}

static int media_fwrite(void *p, u8 *buf, int len)
{
    if (vp_cfg->us_ssid_pwd_is_recv == 0) {
        s16 *buf1 = (s16 *)buf;

        memcpy(vp_cfg->us_ctl.us_buf[vp_cfg->us_ctl.us_bufnum], buf1, len);

        vp_cfg->us_ctl.us_bufnum ^= 1;

        os_taskq_post(VOICE_PRINT_TASK_NAME, 1, US_CALC_CMD);
    }

    return len;
}

static const struct stream_file_ops acoustic_com_ops = {
    .write = media_fwrite,
};

static void media_rec_stop(void)
{
    if (!vp_cfg->stream) {
        return;
    }
    jlstream_stop(vp_cfg->stream, 50);
    jlstream_release(vp_cfg->stream);
    vp_cfg->stream = NULL;
}

static void media_rec_start(void)
{
    int uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"acoustic_communicat");
    if (uuid == 0) {
        return;
    }

    struct jlstream *stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!stream) {
        log_error("jlstream_pipeliine_parse fail!");
        return;
    }

    int err = jlstream_node_ioctl(stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256);
    if (err) {
        goto __exit;
    }
    jlstream_set_scene(stream, STREAM_SCENE_VIR_DATA_TX);

    struct stream_file_info info = {
        .ops = &acoustic_com_ops
    };
    err = stream_node_ioctl((struct stream_node *)stream->snode, NODE_UUID_ACOUSTIC_COM, NODE_IOC_SET_FILE, (int)&info);
    if (err) {
        goto __exit;
    }

    err = jlstream_start(stream);
    if (err) {
        goto __exit;
    }

    vp_cfg->stream = stream;

    return;

__exit:
    jlstream_release(stream);
}

int voiceprint_cfg_start(void)
{
    if (vp_cfg) {
        return 0;
    }

    vp_cfg = zalloc(sizeof(struct vp_cfg) + MultSine_Control_QuaryBufSize(17, 256));
    if (!vp_cfg) {
        return -1;
    }
    vp_cfg->us_con = (MultSine_Control *)(((char *)vp_cfg) + sizeof(struct vp_cfg));

    return thread_fork(VOICE_PRINT_TASK_NAME, 11, 2048, 32, &vp_cfg->pid, us_task, NULL);
}

void voiceprint_cfg_stop(void)
{
    if (vp_cfg == NULL) {
        return;
    }

    int retry = 0;
    int ret;
    vp_cfg->us_ssid_pwd_is_recv = 1;

    do {
        ret = os_taskq_post(VOICE_PRINT_TASK_NAME, 1, US_CLOSE_CMD);
        if (ret != OS_NO_ERR) {
            msleep(50);
            retry++;
        } else {
            break;
        }
    } while (retry < 5);

    if (ret != OS_NO_ERR) {
        log_error("voiceprint_cfg_stop fail");
    }

    thread_kill(&vp_cfg->pid, KILL_WAIT);
    free(vp_cfg);
    vp_cfg = NULL;
}

#endif
