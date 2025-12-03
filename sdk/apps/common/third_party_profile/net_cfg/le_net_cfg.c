// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************
#include "event/net_event.h"
#include "syscfg/syscfg_id.h"
#include "asm/crc16.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "third_party/common/ble_user.h"
#include "multi_protocol_main.h"
#include "le_net_cfg.h"
#include "le_common.h"
#include "app_config.h"


#if (THIRD_PARTY_PROTOCOLS_SEL & NET_CFG_EN)

#define LOG_TAG     		"[LE_NET_CFG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


static volatile hci_con_handle_t con_handle;

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt;
static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
};

#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))
#define ADV_INTERVAL_MIN          (160)

static void *le_net_cfg_ble_hdl;
static char gap_device_name[BT_NAME_LEN_MAX] = "JL-NET-CFG(BLE)";
static u8 complete;
static u8 ble_work_state;

static int app_send_user_data(u16 handle, const u8 *data, u16 len, u8 handle_type);
int le_net_cfg_adv_enable(u8 enable);
u8 is_in_config_network_state(void);


//配网成回复命令
static const u8 rsp_cmd0[] = {
    0x4A, 0x4C, 0x00, 0x0e, 0x10, 0x02,
    '{', '"', 's', 't', 'a', 't', 'u', 's', '"', ':', '0', '}',
    0x7C, 0xC3, 0xFF,
};

//正在配网回复命令
static const u8 rsp_cmd1[] = {
    0x4A, 0x4C, 0x00, 0x0e, 0x10, 0x02,
    '{', '"', 's', 't', 'a', 't', 'u', 's', '"', ':', '1', '}',
    0x4F, 0xF2, 0xFF,
};

//配网失败回复命令
static const u8 rsp_cmd2[] = {
    0x4A, 0x4C, 0x00, 0x0e, 0x10, 0x02,
    '{', '"', 's', 't', 'a', 't', 'u', 's', '"', ':', '2', '}',
    0x1A, 0xA1, 0xFF,
};

//未找到ssid回复命令
static const u8 rsp_cmd3[] = {
    0x4A, 0x4C, 0x00, 0x0e, 0x10, 0x02,
    '{', '"', 's', 't', 'a', 't', 'u', 's', '"', ':', '3', '}',
    0x29, 0x90, 0xFF,
};

//密码错误回复命令
static const u8 rsp_cmd4[] = {
    0x4A, 0x4C, 0x00, 0x0e, 0x10, 0x02,
    '{', '"', 's', 't', 'a', 't', 'u', 's', '"', ':', '4', '}',
    0xB0, 0x07, 0xFF,
};

//------------------------------------------------------
static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:-%d-%d-%d-%d", param->interval_min, param->interval_max, param->latency, param->timeout);

    if (con_handle) {
        ble_op_conn_param_request(con_handle, param);
    }
}

static void check_connetion_updata_deal(void)
{
    if (connection_update_enable && connection_update_cnt < CONN_PARAM_TABLE_CNT) {
        send_request_connect_parameter(connection_update_cnt);
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int conn_interval, conn_latency, conn_timeout;

    /* con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet); */
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d", conn_interval);
    log_info("conn_latency = %d", conn_latency);
    log_info("conn_timeout = %d", conn_timeout);
}

static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x", ble_work_state, state);
        ble_work_state = state;
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static void le_net_cfg_cbk_sm_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u", tmp32);
            break;
        }
        break;
    }
}

static void can_send_now_wakeup(void)
{
    /* putchar('E'); */
}

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */
//sm_conn init

/* LISTING_START(packetHandler): Packet Handler */
static void le_net_cfg_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    u8 status;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                status = hci_subevent_le_enhanced_connection_complete_get_status(packet);
                if (status) {
                    log_error("LE_SLAVE CONNECTION FAIL!!! %0x", status);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE: %0x", con_handle);
                log_info("conn_interval = %d", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                set_ble_work_state(BLE_ST_CONNECT);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: 0x%0x", con_handle);
                connection_update_complete_success(packet + 8);
                set_ble_work_state(BLE_ST_CONNECT);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE");
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: 0x%0x", packet[5]);
            con_handle = 0;
            set_ble_work_state(BLE_ST_DISCONN);
#if !TCFG_POWER_ON_ENABLE_BLE
            if (is_in_config_network_state()) {
#else
            {
#endif
                le_net_cfg_adv_enable(1);
            }
            connection_update_cnt = 0;
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("update_rsp: 0x%02x", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;
        }
        break;
    }
}

//sm_conn init-
/* LISTING_END */

/*
 * @section ATT Read
 *
 * @text The ATT Server handles all reads to constant data. For dynamic data like the custom characteristic, the registered
 * att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
 * with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
 * See Listing attRead.
 */

/* LISTING_START(attRead): ATT Read */

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t le_net_cfg_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x, buffer= 0x%08x", handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = strlen(gap_device_name);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("read gap_name: %s", gap_device_name);
        }
        break;

    case ATT_CHARACTERISTIC_ae82_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = multi_att_get_ccc_config(connection_handle, handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        break;
    }

    log_info("att_value_len = %d", att_value_len);

    return att_value_len;
}

static u16 user_buf_offset, user_data_size;
static u8 user_buf[128];

static void check_net_info_if_recv_complete(void)
{
    if (user_buf[user_buf_offset - 1] == 0xFF && user_buf_offset == user_data_size + 7) {
        user_buf[sizeof(user_buf) - 1] = 0;
        extern int bt_net_config_set_ssid_pwd(const char *data);
        u16 crc16 = user_buf[user_buf_offset - 2] + (user_buf[user_buf_offset - 3] << 8);
        log_info_hexdump(&user_buf[2], user_data_size + 2);

        if (crc16 == CRC16(&user_buf[2], user_data_size + 2)) {
            //去掉2byte crc 1byte 结束符
            user_buf[user_buf_offset - 3] = 0;
            if (0 == bt_net_config_set_ssid_pwd((const char *)&user_buf[6])) {
                app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, rsp_cmd1, sizeof(rsp_cmd1), ATT_OP_AUTO_READ_CCC);
                complete = 1;
                user_buf_offset = 0;
                user_data_size = 0;
                memset(user_buf, 0, sizeof(user_buf));
            }
        } else {
            app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, rsp_cmd2, sizeof(rsp_cmd2), ATT_OP_AUTO_READ_CCC);
            user_buf_offset = 0;
            user_data_size = 0;
            memset(user_buf, 0, sizeof(user_buf));
        }
    }
}

/* LISTING_END */
/*
 * @section ATT Write
 *
 * @text The only valid ATT write in this example is to the Client Characteristic Configuration, which configures notification
 * and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent. See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
static int le_net_cfg_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    u16 handle = att_handle;

    log_info("write_callback, handle= 0x%04x, size = %d", handle, buffer_size);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_ae82_01_CLIENT_CONFIGURATION_HANDLE:
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        check_connetion_updata_deal();
        log_info("write ccc: 0x%04x, 0x%02x", handle, buffer[0]);
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        user_buf_offset = 0;
        user_data_size = 0;
        memset(user_buf, 0, sizeof(user_buf));
        complete = 0;
        break;

    case ATT_CHARACTERISTIC_ae81_01_VALUE_HANDLE:
        log_info("-ae81_rx(%d):", buffer_size);
        log_info_hexdump(buffer, buffer_size);

        if (!user_buf_offset) {
            if (buffer[0] == 'J' && buffer[1] == 'L' && buffer[4] == 0x10 && buffer[5] == 0x01) {
                user_data_size = (buffer[2] << 8) + buffer[3];
                user_buf_offset += buffer_size;
                if (buffer_size < sizeof(user_buf) && user_data_size < 32 + 64 + 24) {
                    memcpy(user_buf, buffer, buffer_size);
                    check_net_info_if_recv_complete();
                    break;
                }
            }
        } else {
            if (user_buf_offset + buffer_size < sizeof(user_buf)) {
                memcpy(user_buf + user_buf_offset, buffer, buffer_size);
                user_buf_offset += buffer_size;
                if (user_buf_offset < user_data_size + 7) {
                    break;
                }
                check_net_info_if_recv_complete();
                break;
            }
        }

        log_error("error : bt net config fail !!!");
        user_buf_offset = 0;
        user_data_size = 0;
        memset(user_buf, 0, sizeof(user_buf));
        app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, rsp_cmd2, sizeof(rsp_cmd2), ATT_OP_AUTO_READ_CCC);
        break;

    default:
        break;
    }

    return 0;
}

static int app_send_user_data(u16 handle, const u8 *data, u16 len, u8 handle_type)
{
    if (!con_handle || !le_net_cfg_ble_hdl) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!multi_att_get_ccc_config(con_handle, handle + 1)) {
        log_info("fail,no write ccc!!!, 0x%04x", handle + 1);
        return APP_BLE_NO_WRITE_CCC;
    }

    int ret = app_ble_att_send_data(le_net_cfg_ble_hdl, handle, (u8 *)data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_error("app_send_fail:%d !!!!!!", ret);
    }

    return ret;
}

static int make_set_adv_data(u8 *buf)
{
    u8 offset = 0;

    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x18, 1); */
    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x1A, 1); */
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xAF00, 2);

    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("adv_data overflow!!!!!!");
        return 0;
    }

    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);

    return offset;
}

static int make_set_rsp_data(u8 *buf)
{
    u8 offset = 0;

    u8 name_len = strlen(gap_device_name);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (u8 *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("rsp_data overflow!!!!!!");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);

    return offset;
}

//广播参数设置
static void advertisements_setup_init(void)
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    uint8_t advData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t rspData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t len = 0;

    len = make_set_adv_data(advData);
    if (len) {
        app_ble_adv_data_set(le_net_cfg_ble_hdl, advData, len);
    }
    len = make_set_rsp_data(rspData);
    if (len) {
        app_ble_rsp_data_set(le_net_cfg_ble_hdl, rspData, len);
    }

    app_ble_set_adv_param(le_net_cfg_ble_hdl, ADV_INTERVAL_MIN, adv_type, adv_channel);
}

static int set_adv_enable(u8 en)
{
    ble_state_e next_state, cur_state;

    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state = get_ble_work_state();

    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break;
    default:
        log_error("cur_state is exception");
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }

    log_info("adv_en:%d", en);

    set_ble_work_state(next_state);

    if (en) {
        advertisements_setup_init();
    }

    app_ble_adv_enable(le_net_cfg_ble_hdl, en);

    return APP_BLE_NO_ERROR;
}

int le_net_cfg_adv_enable(u8 enable)
{
    if (!le_net_cfg_ble_hdl) {
        return -1;
    }

    if (enable == app_ble_adv_state_get(le_net_cfg_ble_hdl)) {
        return 0;
    }

    complete = 0;

    return set_adv_enable(enable);
}

void le_net_cfg_all_init(void)
{
    u8 ble_mac_addr[6];

    log_info("le_net_cfg_all_init");

    if (le_net_cfg_ble_hdl == NULL) {
        con_handle = 0;
        set_ble_work_state(BLE_ST_INIT_OK);

        le_net_cfg_ble_hdl = app_ble_hdl_alloc();
        if (le_net_cfg_ble_hdl == NULL) {
            log_error("le_net_cfg_ble_hdl alloc err !");
            return;
        }
        bt_make_ble_address(ble_mac_addr, (u8 *)bt_get_mac_addr());
        app_ble_set_mac_addr(le_net_cfg_ble_hdl, (void *)ble_mac_addr);
        app_ble_profile_set(le_net_cfg_ble_hdl, le_net_cfg_profile_data);
        app_ble_att_read_callback_register(le_net_cfg_ble_hdl, le_net_cfg_att_read_callback);
        app_ble_att_write_callback_register(le_net_cfg_ble_hdl, le_net_cfg_att_write_callback);
        app_ble_att_server_packet_handler_register(le_net_cfg_ble_hdl, le_net_cfg_cbk_packet_handler);
        app_ble_hci_event_callback_register(le_net_cfg_ble_hdl, le_net_cfg_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(le_net_cfg_ble_hdl, le_net_cfg_cbk_packet_handler);
        app_ble_sm_event_callback_register(le_net_cfg_ble_hdl, le_net_cfg_cbk_sm_packet_handler);
        le_net_cfg_adv_enable(1);
    }
}

void le_net_cfg_all_exit(void)
{
    log_info("le_net_cfg_all_exit");

    if (le_net_cfg_ble_hdl == NULL) {
        return;
    }

    // BLE exit
    if (app_ble_get_hdl_con_handle(le_net_cfg_ble_hdl)) {
        if (complete) {
            return;
        }
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            set_ble_work_state(BLE_ST_SEND_DISCONN);
        }
        app_ble_disconnect(le_net_cfg_ble_hdl);
    }
    le_net_cfg_adv_enable(0);
    app_ble_hdl_free(le_net_cfg_ble_hdl);
    le_net_cfg_ble_hdl = NULL;
}

void le_net_cfg_modify_ble_name(const char *name)
{
    if (strlen(name) <= BT_NAME_LEN_MAX) {
        memset(gap_device_name, 0x0, BT_NAME_LEN_MAX);
        memcpy(gap_device_name, name, strlen(name));
        log_info("modify_ble_name: %s", gap_device_name);
    }
}

void ble_cfg_net_result_notify(int event)
{
    if (!complete) {
        return;
    }

    if (event == NET_CONNECT_TIMEOUT_NOT_FOUND_SSID) {
        app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, rsp_cmd3, sizeof(rsp_cmd3), ATT_OP_AUTO_READ_CCC);
    } else if (event == NET_CONNECT_ASSOCIAT_FAIL) {
        app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, rsp_cmd4, sizeof(rsp_cmd4), ATT_OP_AUTO_READ_CCC);
    } else if (event == NET_EVENT_CONNECTED) {
        app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, rsp_cmd0, sizeof(rsp_cmd0), ATT_OP_AUTO_READ_CCC);
        complete = 0;
    }
}

#endif

