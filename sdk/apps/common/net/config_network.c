#include "wifi/wifi_connect.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "os/os_api.h"
#include "system/init.h"
#include "lwip.h"
#include "device/device.h"
#include "system/app_core.h"
#include "server/server_core.h"
#include "system/timer.h"
#include "asm/debug.h"
#include "asm/power_interface.h"
#include "app_config.h"
#include "http/http_cli.h"
#include "system/timer.h"
#include "json_c/json_tokener.h"
#include "voiceprint_cfg.h"
#include "server/net_server.h"
#include <time.h>
#include "event/net_event.h"

#ifdef CONFIG_NET_ENABLE

#define LOG_TAG             "[NET_CFG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


#if defined THIRD_PARTY_PROTOCOLS_SEL && \
        ((THIRD_PARTY_PROTOCOLS_SEL & MIJIA_EN) || (THIRD_PARTY_PROTOCOLS_SEL & NET_CFG_EN) || (THIRD_PARTY_PROTOCOLS_SEL & DUEROS_EN))
#define TCFG_BLE_NET_CFG_EN	1

extern void le_net_cfg_all_init(void);
extern void le_net_cfg_all_exit(void);
extern void le_net_cfg_dueros_all_init(void);
extern void le_net_cfg_dueros_all_exit(void);
#endif

static u8 config_network_flag;
static u8 ble_config_complete_flag;

#ifdef CONFIG_AIRKISS_NET_CFG
static struct airkiss_result {
    struct smp_cfg_result result;
    char scan_ssid_found;
} airkiss_result;
#endif

#ifdef CONFIG_WSC_NET_CFG
static struct wsc_result {
    char cfg_ssid[33];
    char ap_pincode[9];
    char ssid[33];
    char pwd[65];
} wsc_result;
#endif

#if TCFG_USER_BLE_ENABLE && TCFG_BLE_NET_CFG_EN
static struct bt_net_result {
    char ssid[33];
    char pwd[65];
} bt_net_result;
#endif

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
static struct voiceprint_result {
    char ssid[33];
    char pwd[65];
    char rand_str[8];
} voiceprint_result;
#endif

#ifdef CONFIG_QR_CODE_NET_CFG
static struct qr_code_net_result {
    char ssid[33];
    char pwd[65];
} qr_code_net_result;

void qr_code_net_cfg_init(void);
void qr_code_net_cfg_uninit(void);
#endif

extern void wifi_sta_connect(char *ssid, char *pwd, char save);
extern int url_decode(const char *input, const u32 inlen, char *output, const u32 outbuf_len);

#ifdef CONFIG_WSC_NET_CFG
void config_wsc_ssid_and_pincode(const char *ssid, const char *pincode)
{
    if (ssid && strlen(ssid) < sizeof(wsc_result.cfg_ssid)) {
        strcpy(wsc_result.cfg_ssid, ssid);
    }
    if (pincode && strlen(pincode) < sizeof(wsc_result.ap_pincode)) {
        strcpy(wsc_result.ap_pincode, pincode);
    }
}
#endif

u8 is_in_config_network_state(void)
{
    return config_network_flag;
}

void config_network_start(void)
{
    config_network_flag = 1;

#ifdef CONFIG_AIRKISS_NET_CFG
    memset(&airkiss_result, 0, sizeof(airkiss_result));
    wifi_set_smp_cfg_timeout(100);
    if (!wifi_is_on()) {
        wifi_on();
    }
    wifi_enter_smp_cfg_mode();
#else
#ifdef CONFIG_WSC_NET_CFG
    memset(&wsc_result.ssid, 0, sizeof(wsc_result.ssid));
    memset(&wsc_result.pwd, 0, sizeof(wsc_result.pwd));
    if (!wifi_is_on()) {
        wifi_on();
    }
    wifi_sta_connect(wsc_result.cfg_ssid, "", 0);
    wifi_wsc_start(wsc_result.cfg_ssid, wsc_result.ap_pincode, wsc_result.ap_pincode[0] ? STA_REGISTRAR_PIN_MODE : STA_ENROLLEE_PIN_MODE);
#else
#ifdef CONFIG_DUER_SDK_ENABLE
    if (!wifi_is_on()) {
        wifi_on();
    }
#endif
#endif
#endif

#ifdef CONFIG_QR_CODE_NET_CFG
    // 为了节省内存,关闭WIFI
    /* void wifi_off(void); */
    /* wifi_off(); */
    memset(&qr_code_net_result, 0, sizeof(qr_code_net_result));
    qr_code_net_cfg_init();
#endif

#if TCFG_USER_BLE_ENABLE && TCFG_BLE_NET_CFG_EN
    memset(&bt_net_result, 0, sizeof(bt_net_result));
    ble_config_complete_flag = 0;
#if defined THIRD_PARTY_PROTOCOLS_SEL && (THIRD_PARTY_PROTOCOLS_SEL & NET_CFG_EN)
    le_net_cfg_all_init();
#endif
#if defined THIRD_PARTY_PROTOCOLS_SEL && (THIRD_PARTY_PROTOCOLS_SEL & DUEROS_EN)
    le_net_cfg_dueros_all_init();
#endif
#endif

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
    memset(&voiceprint_result, 0, sizeof(voiceprint_result));
    voiceprint_cfg_start();
#endif
}

void config_network_stop(void)
{
    config_network_flag = 0;

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
    voiceprint_cfg_stop();
#endif
#ifdef CONFIG_QR_CODE_NET_CFG
    qr_code_net_cfg_uninit();
    // 为了节省内存,重新打开WIFI
    /* void wifi_on(void); */
    /* wifi_on(); */
#endif
#if TCFG_USER_BLE_ENABLE && TCFG_BLE_NET_CFG_EN && !TCFG_POWER_ON_ENABLE_BLE
    if (!ble_config_complete_flag) {
#if defined THIRD_PARTY_PROTOCOLS_SEL && (THIRD_PARTY_PROTOCOLS_SEL & NET_CFG_EN)
        le_net_cfg_all_exit();
#endif
#if defined THIRD_PARTY_PROTOCOLS_SEL && (THIRD_PARTY_PROTOCOLS_SEL & DUEROS_EN)
        le_net_cfg_dueros_all_exit();
#endif
    }
#endif
}

#if TCFG_USER_BLE_ENABLE && TCFG_BLE_NET_CFG_EN


#if TCFG_BT_NET_CFG_TENCENT_EN
//腾讯连连
void tencent_net_config(const char *recv_buf, int recv_len, u8 mode)
{
    struct net_event net;
    switch (mode) {
    case 1:
        strncpy(bt_net_result.ssid, recv_buf, recv_len);
        log_info("bt net config ssid : %s", bt_net_result.ssid);
        break;
    case 2:
        strncpy(bt_net_result.pwd, recv_buf, recv_len);
        log_info("bt net config pwd : %s", bt_net_result.pwd);
        break;
    case 3:
        net.arg = "net";
        net.event = NET_EVENT_SMP_CFG_FINISH;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        ble_config_complete_flag = 1;
        break;
    default:
        break;
    }
}
#endif

u8 bt_net_config_get_complete_flag(void)
{
    return ble_config_complete_flag;
}

int bt_net_config_info_set(const char *ssid, const char *pwd)
{
    if (ssid) {
        strcpy(bt_net_result.ssid, ssid);
    }
    if (pwd) {
        strcpy(bt_net_result.pwd, pwd);
    }

    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

#if defined THIRD_PARTY_PROTOCOLS_SEL && (THIRD_PARTY_PROTOCOLS_SEL & DUEROS_EN)
    ble_config_complete_flag = 1;
#endif

    return 0;
}

int bt_net_config_set_ssid_pwd(const char *data)
{
    json_object *json = json_tokener_parse(data);
    const char *str_ssid, *str_pwd;

    if (json) {
#if TCFG_BT_NET_CFG_DUI_EN
        str_ssid = json_object_get_string(json_object_object_get(json, "SSID"));
#else
        str_ssid = json_object_get_string(json_object_object_get(json, "ssid"));
#endif

        if (str_ssid) {
#if TCFG_BT_NET_CFG_TURING_EN
            url_decode(str_ssid, strlen(str_ssid), bt_net_result.ssid, sizeof(bt_net_result.ssid));
#else
            strcpy(bt_net_result.ssid, str_ssid);
#endif
            log_info("bt net config ssid : %s", str_ssid);
        } else {
            json_object_put(json);
            return -1;
        }

#if TCFG_BT_NET_CFG_DUI_EN
        str_pwd = json_object_get_string(json_object_object_get(json, "PSK"));
#else
        str_pwd = json_object_get_string(json_object_object_get(json, "pass"));
#endif

        if (str_pwd) {
#if TCFG_BT_NET_CFG_TURING_EN
            url_decode(str_pwd, strlen(str_pwd), bt_net_result.pwd, sizeof(bt_net_result.pwd));
#else
            strcpy(bt_net_result.pwd, str_pwd);
#endif
            log_info("bt net config pwd : %s", str_pwd);
        } else {
            memset(bt_net_result.pwd, 0, sizeof(bt_net_result.pwd));
        }

        json_object_put(json);
        struct net_event net;
        net.arg = "net";
        net.event = NET_EVENT_SMP_CFG_FINISH;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        return 0;
    }

    return -1;
}
#endif

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
int voiceprint_cfg_net_set_ssid_pwd(const char *ssid, const char *pwd, const char *rand_str)
{
    strcpy(voiceprint_result.ssid, ssid);
    strcpy(voiceprint_result.pwd, pwd);
    strcpy(voiceprint_result.rand_str, rand_str);

    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

    return 0;
}
#endif

#ifdef CONFIG_QR_CODE_NET_CFG
int qr_code_net_set_ssid_pwd(const char *ssid, const char *pwd)
{
    strcpy(qr_code_net_result.ssid, ssid);
    strcpy(qr_code_net_result.pwd, pwd);

    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

    return 0;
}
#endif

#ifdef CONFIG_WSC_NET_CFG
int wsc_net_set_ssid_pwd(const char *ssid, const char *pwd, int ssid_len, int pwd_len)
{
    memcpy(wsc_result.ssid, ssid, ssid_len);
    memcpy(wsc_result.pwd, pwd, pwd_len);

    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

    return 0;
}
#endif

void config_network_connect(void)
{
#if TCFG_USER_BLE_ENABLE && TCFG_BLE_NET_CFG_EN

    if (bt_net_result.ssid[0]) {
        wifi_sta_connect(bt_net_result.ssid, bt_net_result.pwd, 1);
        return;
    }

#endif
#ifdef CONFIG_QR_CODE_NET_CFG

    if (qr_code_net_result.ssid[0]) {
        wifi_sta_connect(qr_code_net_result.ssid, qr_code_net_result.pwd, 1);
        return;
    }

#endif
#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE

    if (voiceprint_result.ssid[0]) {
        wifi_sta_connect(voiceprint_result.ssid, voiceprint_result.pwd, 1);
        return;
    }

#endif
#ifdef CONFIG_AIRKISS_NET_CFG

    if (airkiss_result.result.ssid[0]) {
        wifi_sta_connect(airkiss_result.result.ssid, airkiss_result.result.passphrase, 1);
        return;
    }

#endif
#ifdef CONFIG_WSC_NET_CFG

    if (wsc_result.ssid[0]) {
        wifi_sta_connect(wsc_result.ssid, wsc_result.pwd, 1);
        return;
    }

#endif
}

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
const char *get_voiceprint_result_random(void)
{
    return voiceprint_result.rand_str[0] ? voiceprint_result.rand_str : NULL;
}

static void voiceprint_broadcast(void)
{
    if (voiceprint_result.rand_str[0]) {

#define SEND_VOICE_PRINT_MSG		"https://robot.jieliapp.com/wx/wechat/network/config/voice/success?wifirand=%s"

        int ret = 0;
        char url[128];

        http_body_obj http_body_buf;
        httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));

        if (!ctx) {
            return;
        }

        sprintf(url, SEND_VOICE_PRINT_MSG, voiceprint_result.rand_str);

        memset(&http_body_buf, 0x0, sizeof(http_body_obj));

        http_body_buf.recv_len = 0;
        http_body_buf.buf_len = 4 * 1024;
        http_body_buf.buf_count = 1;
        http_body_buf.p = (char *) malloc(http_body_buf.buf_len * sizeof(char));

        if (!http_body_buf.p) {
            free(ctx);
            return;
        }

        ctx->url = url;
        ctx->priv = &http_body_buf;
        ctx->connection = "close";

        ret = httpcli_get(ctx);

        if (ret != HERROR_OK) {
            log_error("voiceprint_broadcast fail!");
        }

        if (http_body_buf.p) {
            free(http_body_buf.p);
        }

        free(ctx);

        voiceprint_result.rand_str[0] = 0;
    }
}
#endif

#ifdef CONFIG_AIRKISS_NET_CFG

void wifi_smp_set_ssid_pwd(void)
{
    struct smp_cfg_result smp_cfg;

    wifi_get_cfg_net_result(&smp_cfg);

    if (smp_cfg.type == AIRKISS_SMP_CFG) {
        log_info("AIRKISS INFO, SSID = %s, PWD = %s, ssid_crc = 0x%x, ran_val = 0x%x", smp_cfg.ssid, smp_cfg.passphrase, smp_cfg.ssid_crc, smp_cfg.random_val);
        airkiss_result.result.type = AIRKISS_SMP_CFG ;
        airkiss_result.result.ssid_crc = smp_cfg.ssid_crc;
        airkiss_result.result.random_val = smp_cfg.random_val;
        strcpy(airkiss_result.result.ssid, smp_cfg.ssid);
        strcpy(airkiss_result.result.passphrase, smp_cfg.passphrase);

        if (airkiss_result.result.ssid_crc == wifi_airkiss_calcrc_bytes((u8 *)airkiss_result.result.ssid, strlen(airkiss_result.result.ssid))) {
            airkiss_result.scan_ssid_found = 1;

            struct net_event net;
            net.arg = "net";
            net.event = NET_EVENT_SMP_CFG_FINISH;
            net_event_notify(NET_EVENT_FROM_USER, &net);
        } else {
            wifi_scan_req();
        }

        return;
    }

    strcpy(airkiss_result.result.ssid, smp_cfg.ssid);
    strcpy(airkiss_result.result.passphrase, smp_cfg.passphrase);
    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);
}

void airkiss_ssid_check(void)
{
    u32 i;

    if (airkiss_result.result.type != AIRKISS_SMP_CFG ||  airkiss_result.scan_ssid_found) {
        return;
    }
    struct wifi_scan_ssid_info *sta_ssid_info;
    u32 sta_ssid_num = 0;
    sta_ssid_info = wifi_get_scan_result(&sta_ssid_num);
    wifi_clear_scan_result();

    for (i = 0; i < sta_ssid_num; i++) {
        if (!strncmp(airkiss_result.result.ssid, sta_ssid_info[i].ssid, strlen(airkiss_result.result.ssid))) {
            /* CHECK_AIRKISS_SSID_CRC: */
            if (airkiss_result.result.ssid_crc == wifi_airkiss_calcrc_bytes((u8 *)sta_ssid_info[i].ssid, strlen(sta_ssid_info[i].ssid))) {
                log_info("find airkiss ssid = [%s]", sta_ssid_info[i].ssid);
                strcpy(airkiss_result.result.ssid, sta_ssid_info[i].ssid);
                airkiss_result.scan_ssid_found = 1;

                struct net_event net;
                net.arg = "net";
                net.event = NET_EVENT_SMP_CFG_FINISH;
                net_event_notify(NET_EVENT_FROM_USER, &net);

                free(sta_ssid_info);
                return;
            }
        } else {
            /*goto CHECK_AIRKISS_SSID_CRC;*/
        }
    }

    free(sta_ssid_info);

    log_info("cannot found airkiss ssid[%s] !!!", airkiss_result.result.ssid);
}

static void airkiss_broadcast(void)
{
    int i, ret;
    int onOff = 1;
    int sock;
    struct sockaddr_in dest_addr;

    if (airkiss_result.result.type != AIRKISS_SMP_CFG) {
        return;
    }

    log_info("airkiss_broadcast random_val");

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == -1) {
        log_error("%s %d->Error in socket()", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = 0;
    ret = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (ret == -1) {
        log_error("%s %d->Error in bind()", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                     (char *)&onOff, sizeof(onOff));

    if (ret == -1) {
        log_error("%s %d->Error in setsockopt() SO_BROADCAST", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr.s_addr);
    dest_addr.sin_port = htons(10000);

    for (i = 0; i < 8; i++) {
        ret = sendto(sock, (unsigned char *)&airkiss_result.result.random_val, 1, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

        if (ret == -1) {
            log_error("%s %d->Error in sendto", __FUNCTION__, __LINE__);
        }

        msleep(20);
    }

    memset(&airkiss_result, 0, sizeof(airkiss_result));

EXIT:

    if (sock != -1) {
        closesocket(sock);
    }
}
#endif

void connect_broadcast(void)
{
    int i, ret;
    int onOff = 1;
    int sock;
    struct sockaddr_in dest_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        goto EXIT;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = 0;
    ret = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret == -1) {
        goto EXIT;
    }

    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                     (char *)&onOff, sizeof(onOff));
    if (ret == -1) {
        goto EXIT;
    }

    inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr.s_addr);
    dest_addr.sin_port = htons(10001);

    for (i = 0; i < 10; i++) {
        ret = sendto(sock, "CONNECT OK", 10, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
        if (ret == -1) {
            goto EXIT;
        }
        msleep(10);
    }

EXIT:
    if (sock != -1) {
        closesocket(sock);
    }
}

void config_network_broadcast(void)
{
#ifdef CONFIG_AIRKISS_NET_CFG
    airkiss_broadcast();
#endif
#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
    /* voiceprint_broadcast(); */
#endif
    //有些使用了加密的路由器刚获取IP地址后前几个包都会没响应，怀疑路由器没转发成功
    connect_broadcast();
}

#endif
