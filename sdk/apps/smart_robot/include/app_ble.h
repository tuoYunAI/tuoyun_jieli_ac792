
#ifndef __BLE_PROV_H__
#define __BLE_PROV_H__
#include "event/bt_event.h"


/**
 * @brief  Initialize BLE provisioning module
 *         After calling this function, the Bluetooth device can be 
 *         scanned by the mobile APP and provisioning operations can be performed.
 * @return void
 * @note 
 */
void bt_ble_module_init(void);

int bt_connction_status_event_handler(struct bt_event *bt);

#endif