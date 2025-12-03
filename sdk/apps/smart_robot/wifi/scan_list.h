
#ifndef __SCAN_LIST_H__
#define __SCAN_LIST_H__
#include "wifi/wifi_connect.h"
#include "os/os_api.h"

struct wifi_scan_ssid_info *wifi_scan_list_get(u32 *total_scan_num);
void wifi_scan_list_release(struct wifi_scan_ssid_info *list);

//需要在WIFI_EVENT_STA_SCAN_COMPLETED事件时调用
void scan_list_sem_post(void);

#endif
