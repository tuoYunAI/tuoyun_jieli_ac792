#ifndef _MULTI_BOX_ADV_H_
#define _MULTI_BOX_ADV_H_

//主机是否只扫描广播包不做连接设备处理
#define MULTI_BOX_ADV_FILTER_ENABLE
#define MULTI_BOX_ADV_PREFIX	"ADV_"

#define MULTI_BOX_ROLE_UNKNOWN  0
#define MULTI_BOX_ROLE_MASTER   1
#define MULTI_BOX_ROLE_SLAVE    2

static const u8 service_32bit_uuid[4] = {0x7A, 0x39, 0xB2, 0x6E};
static const u8 company_id[2] = {0xD6, 0x05};


void multi_box_adv_all_init(void);
void multi_box_adv_all_exit(void);
int multi_box_adv_adv_enable(u8 enable);
void set_multi_box_adv_data(u8 adv_data);
void set_multi_box_adv_volume_data(u8 volume);
void multi_box_bis_change_volume_notify(void);
void multi_box_bis_role_change(u8 role);
void set_multi_box_adv_interval_min(u16 adv_interval);
u8 get_multi_box_adv_data(void);
void multi_box_adv_data_handle(u8 *mac_addr, u8 adv_data);
void multi_box_scan_all_init(void);
void multi_box_scan_all_exit(void);
void set_multi_box_scan_param(u8 scan_window, u8 scan_interval);
u16 get_multi_box_number(void);
void reset_multi_box_number(void);
u8 get_multi_box_role(void);
bool check_multi_box_bis_is_exist(void);
void set_multi_box_role(u8 role);
void clear_multi_box_mac_list(void);

#endif

