#include "multi_protocol_main.h"
#include "btstack/avctp_user.h"
#include "app_config.h"
#if TCFG_USER_TWS_ENABLE
#include "app_msg.h"
#include "classic/tws_api.h"
#include "event/bt_event.h"
#include "bt_tws.h"
#endif

#if THIRD_PARTY_PROTOCOLS_SEL || (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN | LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN))

#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)
#include "gfps_platform_api.h"
#endif

#if (BT_AI_SEL_PROTOCOL & REALME_EN)
#include "realme_platform_api.h"
#endif

#define LOG_TAG     		"[MULT_PROTOCOL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"


void multi_protocol_state_update_callback(void *_hdl, uint8_t state, uint8_t *packet, uint16_t size)
{
    log_info("state callback 0x%x, 0x%x, 0x%x, %d", (u32)_hdl, state, (u32)packet, size);

    switch (state) {
    case APP_BLE_CONNECTION_COMPLETE:
    case APP_BLE_DISCONNECTION_COMPLETE:
    case APP_SPP_CONNECTION_COMPLETE:
    case APP_SPP_DISCONNECTION_COMPLETE:
#if TCFG_USER_TWS_ENABLE
        multi_protocol_tws_sync_send();
#if (BT_AI_SEL_PROTOCOL & REALME_EN)
        realme_tws_sync_state_send();
#endif
#endif
        break;
    };
}

#if TCFG_USER_TWS_ENABLE

void multi_protocol_bt_tws_poweroff_handler(void)
{
    log_info("tws poweroff : %d %d", get_bt_tws_connect_status(), tws_api_get_role());

    if (!get_bt_tws_connect_status() || !(tws_api_get_role() == TWS_ROLE_MASTER)) {
        return;
    }

    multi_protocol_tws_sync_send();

#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)
    gfps_sync_info_to_new_master();
#endif

#if (BT_AI_SEL_PROTOCOL & REALME_EN)
    realme_tws_sync_state_send();
#endif
}

static int multi_protocol_tws_status_event_handler(void *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 phone_link_connection = evt->args[1];
    u8 reason = evt->args[2];

    log_info("tws status callback 0x%x", evt->event);

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
#if TCFG_USER_TWS_ENABLE
        multi_protocol_tws_sync_send();
#endif
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        break;
    case TWS_EVENT_ROLE_SWITCH:
        break;
    }
    return 0;
}

REGISTER_APP_EVENT_HANDLER(multi_protocol_tws_status_event) = {
    .event      = SYS_BT_EVENT,
    .from       = BT_EVENT_FROM_TWS,
    .handler    = multi_protocol_tws_status_event_handler,
};

#endif

#if 0

static int multi_protocol_bt_status_event_handler(struct bt_event *bt)
{
    return 0;
}

static int bt_app_status_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
    multi_protocol_bt_status_event_handler(bt);
    return 0;
}
APP_MSG_HANDLER(multi_protocol_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = bt_app_status_event_handler,
};


static int multi_protocol_bt_hci_event_handler(struct bt_event *bt)
{
    log_info("multi protocol bt hci event handler\n");
    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");

        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        break;
    default:
        break;

    }
    return 0;
}

int multi_protocol_hci_event_handler(int *event)
{
    multi_protocol_bt_hci_event_handler((struct bt_event *)event);
    return 0;
}
APP_MSG_HANDLER(multi_protocol_hci_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = multi_protocol_hci_event_handler,
};

#endif

#endif

