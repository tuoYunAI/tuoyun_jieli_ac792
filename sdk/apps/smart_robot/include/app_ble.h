
#ifndef __BLE_PROV_H__
#define __BLE_PROV_H__
#include "event/bt_event.h"

void bt_ble_module_init(void);

int bt_connction_status_event_handler(struct bt_event *bt);

#endif