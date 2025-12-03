#include "qrcode/qrcode.h"
#include "json_c/json_tokener.h"
#include "mbedtls/base64.h"
#include "get_yuv_data.h"
#include "app_config.h"

#ifdef CONFIG_QR_CODE_NET_CFG

#define LOG_TAG             "[QR_CODE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


static void *qr_decoder = NULL;
static u8 qr_run_flag;

#ifdef CONFIG_TURING_SDK_ENABLE
static char turing_openid[12];

const char *get_qrcode_result_openid(void)
{
    return turing_openid[0] ? turing_openid : NULL;
}
#endif

extern int qr_code_net_set_ssid_pwd(const char *ssid, const char *pwd);

static int qrcode_other_type_find(const char *str, const char **ssid, const char **pwd)//配网二维码其他类型查找
{
    if (!str || !ssid || !pwd) {
        return 0;
    }

    const char *p, *e, *s;
    char *fs = NULL, *fp = NULL;
    const char *keyword[] = {
        //WIFI   ssid  pwd   end;
        "WIFI:", "S:", "P:", ";",//huawei,xiaomi: WIFI:T:WPA;S:GJ1;P:8888888899;;
        NULL, NULL, NULL, NULL,
    };
    *pwd = *ssid = NULL;
    for (int i = 0; keyword[i] != NULL; i += 4) {
        p = strstr(str, keyword[i]);
        if (p) {
            p = strstr(p + strlen(keyword[i]), keyword[i + 3]);//查找WIFI的结束符
            if (p) {
                for (int j = 1; j <= 2; j++) {//j=1,ssid; j=2,pwd
                    s = strstr(p, keyword[i + j]);//查找ssid/pwd起始符
                    if (s) {
                        s += strlen(keyword[i + j]);
                        e = strstr(s, keyword[i + 3]);//查找ssid/pwd结束符
                        if (e && (e - s) > 0) {
                            if (j == 1) {
                                fs = (char *)&s[e - s];
                                *ssid = s;
                            } else {
                                fp = (char *)&s[e - s];
                                *pwd = s;
                            }
                        }
                    }

                }
            }
        }
        if (fs) {
            *fs = 0;
            if (fp) {
                *fp = 0;
            }
            return 1;
        }
    }

    return 0;
}

static void qrcode_process(u8 *inputFrame, u32 size, int width, int height)
{
    int ret;
    int enc_type = 0;
    char *buf = NULL;
    int buf_size = 0;
    int md_detected = 0; //是否检测到运动物体
    json_object *new_obj = NULL;
    json_object *key = NULL;

    if (qr_decoder && qr_run_flag)  {
        qrcode_detectAndDecode(qr_decoder, inputFrame, &md_detected);

        ret = qrcode_get_result(qr_decoder, &buf, &buf_size, &enc_type);

        if (buf_size > 0 && ret == 0) {
            buf[buf_size] = 0;
#ifdef CONFIG_TURING_SDK_ENABLE
            unsigned long olen = 0;
            ret = mbedtls_base64_decode((u8 *)buf, strlen(buf), &olen, (u8 *)buf, strlen(buf));
            if (ret) {
                return;
            }
            buf[olen] = 0;
#endif
            log_info("qr code decode: %s", buf);

            const char *str_ssid = NULL, *str_pwd = NULL;
            new_obj = json_tokener_parse(buf);
            if (!new_obj) {
                goto qr_other;
            }
            str_ssid = json_object_get_string(json_object_object_get(new_obj, "ssid"));
            if (!str_ssid) {
                goto qr_other;
            }
            log_info("qr code net config ssid : %s", str_ssid);
#ifdef CONFIG_TURING_SDK_ENABLE
            const char *openid = json_object_get_string(json_object_object_get(new_obj, "TuringBindID"));
            if (openid) {
                snprintf(turing_openid, sizeof(turing_openid), "%s", openid);
            }

            str_pwd = json_object_get_string(json_object_object_get(new_obj, "pwd"));
#else
            str_pwd = json_object_get_string(json_object_object_get(new_obj, "pass"));
#endif

qr_other:
            if (!str_ssid) {
                if (!qrcode_other_type_find(buf, &str_ssid, &str_pwd)) {
                    goto __exit;
                }
            }
            if (str_pwd) {
                log_info("qr code net config pwd : %s", str_pwd);
            } else {
                str_pwd = "";
            }

            qr_code_net_set_ssid_pwd(str_ssid, str_pwd);
            qr_run_flag = 0;
        }
    }

__exit:
    if (new_obj) {
        json_object_put(new_obj);
    }
}

void qr_code_net_cfg_init(void)
{
#ifdef CONFIG_TURING_SDK_ENABLE
    memset(turing_openid, 0, sizeof(turing_openid));
#endif
    //不镜像
    qr_decoder = qrcode_init(TCFG_YUV_DATA_OUT_WIDTH, TCFG_YUV_DATA_OUT_HEIGHT, TCFG_YUV_DATA_OUT_WIDTH, QRCODE_MODE_NORMAL/* QRCODE_MODE_FAST*/, 60, 1, 0);
    //垂直镜像
    //qr_decoder = qrcode_init(YUV_DATA_WIDTH, YUV_DATA_HEIGHT, YUV_DATA_WIDTH, QRCODE_MODE_NORMAL/* QRCODE_MODE_FAST*/, 60, 2, 1);
    //水平镜像
    //qr_decoder = qrcode_init(YUV_DATA_WIDTH, YUV_DATA_HEIGHT, YUV_DATA_WIDTH, QRCODE_MODE_NORMAL/* QRCODE_MODE_FAST*/, 60, 2, 2);
    if (!qr_decoder) {
        log_error("qrdecode init err");
        return;
    }

    qr_run_flag = 1;

    get_yuv_init(qrcode_process);
}

void qr_code_net_cfg_uninit(void)
{
    qr_run_flag = 0;

    get_yuv_uninit();

    if (qr_decoder) {
        qrcode_deinit(qr_decoder);
        qr_decoder = NULL;
    }
}

#endif
