#include "app_config.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "btstack/btstack_task.h"
#include "btstack/le/att.h"
#include "btstack/le/le_user.h"
#include "btstack/avctp_user.h"
#include "bt_common.h"
#include "le_common.h"
#include "le_net_central.h"
#include "event/bt_event.h"
#include "syscfg/syscfg_id.h"
#include "../multi_demo/le_multi_common.h"
#include "app_ble.h"

#define LOG_TAG             "[BLE]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
// #define LOG_DEBUG_ENABLE
// #define LOG_DUMP_ENABLE
// #define LOG_CHAR_ENABLE
#include "system/debug.h"


void bt_ble_module_init(void)
{
#if TCFG_USER_BLE_ENABLE
    u8 tmp_ble_addr[6];
    extern const u8 *bt_get_mac_addr(void);
    bt_make_ble_address(tmp_ble_addr, (u8 *)bt_get_mac_addr());
    le_controller_set_mac((void *)tmp_ble_addr);
    log_debug("-----edr + ble 's address-----");
    put_buf((void *)bt_get_mac_addr(), 6);
    put_buf((void *)tmp_ble_addr, 6);
#if TCFG_BLE_MASTER_CENTRAL_EN || TCFG_TRANS_MULTI_BLE_EN
    extern void ble_client_config_init(void);
    ble_client_config_init();
#endif
#endif

    btstack_init();
}

int bt_connction_status_event_handler(struct bt_event *bt)
{

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        log_info("BT_STATUS_INIT_OK\n");
        if (BT_MODE_IS(BT_BQB)) {
            ble_bqb_test_thread_init();
            break;
        }

#if TCFG_TRANS_DATA_EN
        bt_ble_init();
#endif
        break;
    }

    return 0;
}



