
#ifndef __WIFI_H__
#define __WIFI_H__

#if TCFG_WIFI_ENABLE
#include "generic/typedef.h"
#include "wifi/wifi_def.h"


int start_wifi_network(void);

/**
 * @brief 打印wifi状态信息
 *
 * @param 
 */
void wifi_status(void);

/**
 * @brief wifi_get_mac，获取WIFI MAC地址
 *
 * @param mac 指向存储MAC地址的缓存数组，数组大小为6
 */
int get_wifi_mac(char* buf);


/**
 * @brief start_scan_wifi STA模式或者AP模式下启动一次扫描空中SSID
 * @note STA模式下调用返回-1代表WIFI OFF/正在扫描/连接中,无需启动扫描, 可以等待几秒或者扫描完成事件到来获取结果
 */
int start_scan_wifi(void);

/**
 * @brief get_wifi_scan_result,获取扫描结果
 */
struct wifi_scan_ssid_info *get_wifi_scan_result(u32 *ssid_cnt);

int config_wifi_param(char *ssid, char *pwd);

int comfirm_wifi_param(void);

int check_wifi_connected(void);

void enter_config_network_state(void);

u8 check_if_in_config_network_state(void);

u32 get_wifi_ip();

char* get_device_unique_code();

#endif

#endif