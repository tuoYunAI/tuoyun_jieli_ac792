
#include "scan_list.h"

#define LOG_TAG             "[WIFI]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
//#define LOG_DEBUG_ENABLE
//#define LOG_DUMP_ENABLE
//#define LOG_CLI_ENABLE
#include "system/debug.h"




typedef struct _scan_list_control {
    OS_SEM scan_sem;
    OS_MUTEX scan_mutex;
    u8 sem_flag;
} scan_list_control, *pscan_list_control;

static scan_list_control scan_list = {0};
static pscan_list_control pscan_list = &scan_list;

static int scan_list_sem_pend(int timeout)
{
    int err = -1;
    if (os_sem_valid(&pscan_list->scan_sem)) {
        pscan_list->sem_flag = 1;
        err = os_sem_pend(&pscan_list->scan_sem, timeout);
        if (err) {
            printf("scan_list_sem_pend timeout\n");
            pscan_list->sem_flag = 0;
        }
    }

    return err;
}

void scan_list_sem_post(void)
{
    if (os_sem_valid(&pscan_list->scan_sem) && pscan_list->sem_flag) {
        os_sem_post(&pscan_list->scan_sem);
        pscan_list->sem_flag = 0;
    }
}

static void scan_list_sem_del(void)
{
    if (os_sem_valid(&pscan_list->scan_sem)) {
        os_sem_del(&pscan_list->scan_sem, OS_DEL_ALWAYS);
    }
}

static void scan_list_mutex_pend(void)
{
    if (os_mutex_valid(&pscan_list->scan_mutex)) {
        os_mutex_pend(&pscan_list->scan_mutex, 0);
    }
}

static void scan_list_mutex_post(void)
{
    if (os_mutex_valid(&pscan_list->scan_mutex)) {
        os_mutex_post(&pscan_list->scan_mutex);
    }
}

static void scan_list_mutex_del(void)
{
    if (os_mutex_valid(&pscan_list->scan_mutex)) {
        os_mutex_del(&pscan_list->scan_mutex, OS_DEL_ALWAYS);
    }
}

struct wifi_scan_ssid_info *wifi_scan_list_get(u32 *total_scan_num)
{
    u32 sta_ssid_num;
    struct wifi_scan_ssid_info *sta_ssid_info;

    if (!os_sem_valid(&pscan_list->scan_sem)) {
        os_sem_create(&pscan_list->scan_sem, 0);
    }

    if (!os_mutex_valid(&pscan_list->scan_mutex)) {
        os_mutex_create(&pscan_list->scan_mutex);
    }

    scan_list_mutex_pend();

    wifi_scan_req();

    //等待WIFI_EVENT_STA_SCAN_COMPLETED事件
    if (scan_list_sem_pend(500)) { //单位10ms
        printf("get_scan_list timeout");
        pscan_list->sem_flag = 0;
        scan_list_mutex_post();
        /* scan_list_sem_del(); */
        return NULL;
    }

    sta_ssid_num = 0;
    sta_ssid_info = wifi_get_scan_result(&sta_ssid_num);
    if (total_scan_num) {
        *total_scan_num = sta_ssid_num;
    }

    wifi_clear_scan_result();
    scan_list_mutex_post();
    /* scan_list_sem_del(); */
    return sta_ssid_info;
}

void wifi_scan_list_release(struct wifi_scan_ssid_info *list)
{
    if (list) {
        free(list);
    }
}

//example
void wifi_get_list_example(void)
{
    u32 scan_num = 0;
    struct wifi_scan_ssid_info *list = wifi_scan_list_get(&scan_num);
    if (!list) {
        printf("wifi_scan_list_get fail\n");
        return;
    }

    printf("sta_ssid_num: %d\n", scan_num);
    for (int i = 0; i < scan_num; i++) {
        log_info("wifi_sta_scan_channel_test ssid = [%s],rssi = %d,snr = %d, auth_mode = %s",
                 list[i].ssid, list[i].rssi, list[i].snr, get_wifi_auth_mode(list[i].auth_mode));
    }
    wifi_scan_list_release(list);
}

