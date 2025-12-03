// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btcontroller_modules.h"
#include "bt_common.h"

#include "third_party/common/ble_user.h"
#include "le_net_cfg_dueros.h"
#include "le_common.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_NET_CFG_DUEROS)

#define LOG_TAG             "[BLE_DUEROS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

//------
#define ATT_LOCAL_PAYLOAD_SIZE    (64)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512 * 4)              //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 *att_ram_buffer;
//---------------

//---------------
#define ADV_INTERVAL_MIN          (160)

static volatile hci_con_handle_t con_handle;

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
};
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))

static uint8_t ble_wifi_cfg_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00
};

// 广播包内容
/* static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31 */
// scan_rsp 内容
/* static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31 */

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

#define adv_data       &att_ram_buffer[0]
#define scan_rsp_data  &att_ram_buffer[32]

static char gap_device_name[BT_NAME_LEN_MAX];
static u8 gap_device_name_len;
static u8 ble_work_state;
static u8 adv_ctrl_en;
static OS_MUTEX mutex;

//------------------------------------------------------

int duer_wifi_cfg_init(const char *client_id, const char *device_id, const char *pub_key);
int duer_wifi_cfg_deinit(void);
int duer_dipb_data_handler_recv_data(const uint8_t *data, uint32_t data_len, uint16_t handle);
const char *get_dueros_client_id(void);
const char *get_dueros_device_id(void);
const char *get_dueros_pub_key(void);
const u8 *bt_get_mac_addr(void);
void bt_ble_exit(void);
u8 bt_net_config_get_complete_flag(void);


static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:%d-%d-%d-%d", param->interval_min, param->interval_max, param->latency, param->timeout);

    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, con_handle, param);
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

static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
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
            log_info("Passkey display: %06u.", tmp32);
            break;
        }
        break;
    }
}

static void can_send_now_wakeup(void)
{

}

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void server_profile_start(u16 handle)
{
    ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
    set_ble_work_state(BLE_ST_CONNECT);
    /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
}

/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
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
                    log_info("LE_SLAVE CONNECTION FAIL!!! :%0x", status);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE: %0x", con_handle);
                log_info("conn_interval = %d", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                server_profile_start(con_handle);
                break;
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x", con_handle);
                connection_update_complete_success(packet + 8);
                server_profile_start(con_handle);
                break;
            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;
            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE");
                /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
                break;
            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x", packet[5]);
            con_handle = 0;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);
            if (bt_net_config_get_complete_flag()) {
                bt_ble_exit();
            } else {
                os_mutex_pend(&mutex, 0);
                bt_ble_adv_enable(1);
                os_mutex_post(&mutex);
            }
            connection_update_cnt = 0;
            break;
        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %d", mtu);
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, mtu);
            break;
        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("update_rsp: %02x", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;
        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE = %d", packet[2]);
            break;
        }
        break;
    }
}

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
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x, buffer_size= %08x", handle, buffer_size);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = gap_device_name_len;
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("read gap_name: %s", gap_device_name);
        }
        break;
    case ATT_CHARACTERISTIC_2a01_01_VALUE_HANDLE:
        att_value_len = 2;
        const u8 Appearance[2] = {0x14, 0x04};
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Appearance[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2222_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_3333_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
    default:
        break;
    }

    log_info("att_value_len= %d", att_value_len);

    return att_value_len;
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
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    u16 handle = att_handle;

    log_info("write_callback, handle= 0x%04x, size = %d", handle, buffer_size);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        break;
    case ATT_CHARACTERISTIC_2222_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_3333_01_CLIENT_CONFIGURATION_HANDLE:
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        check_connetion_updata_deal();
        log_info("write ccc: %04x, %02x", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        break;
    case ATT_CHARACTERISTIC_2222_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_3333_01_VALUE_HANDLE:
        log_info_hexdump(buffer, buffer_size);
        //解决非对齐访问
        if (buffer && buffer_size > 0) {
            void *data = malloc(buffer_size);
            if (data) {
                memcpy(data, buffer, buffer_size);
                duer_dipb_data_handler_recv_data(data, buffer_size, handle);
                free(data);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

static int app_send_user_data(u16 handle, const uint8_t *data, uint32_t len, u8 handle_type)
{
    int ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!att_get_ccc_config(handle + 1)) {
        log_info("handle %04x send fail,no write ccc!!!", handle + 1);
        return APP_BLE_NO_WRITE_CCC;
    }

    ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_info("app_send_fail: %d", ret);
    }

    return ret;
}

static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;
    u8 manufacturer_data[4] = {0x1C, 0x01, 0x01, 0x11};

    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x18, 1); */
    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x1A, 1); */
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS, ble_wifi_cfg_service_uuid128, sizeof(ble_wifi_cfg_service_uuid128));
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, manufacturer_data, 4);

    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("adv_data overflow");
        return -1;
    }

    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);

    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, offset, buf);

    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("rsp_data overflow");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);

    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, offset, buf);

    return 0;
}

//广播参数设置
static void advertisements_setup_init(void)
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int ret = 0;

    if (!att_ram_buffer) {
        att_ram_buffer = malloc(ATT_RAM_BUFSIZE);
        ASSERT(att_ram_buffer != NULL);
    }

    ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, ADV_INTERVAL_MIN, adv_type, adv_channel);

    ret |= make_set_adv_data();
    ret |= make_set_rsp_data();

    if (ret) {
        log_error("advertisements_setup_init fail");
    }
}

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void reset_passkey_cb(u32 *key)
{
#if 1
    u32 newkey = rand32();//获取随机数

    newkey &= 0xfffff;
    if (newkey > 999999) {
        newkey = newkey - 999999; //不能大于999999
    }
    *key = newkey; //小于或等于六位数
    printf("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}

extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
extern void le_device_db_init(void);

static void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_request_security(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}

void ble_profile_init(void)
{
    log_info("ble profile init");

    le_device_db_init();

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#endif

    /* setup ATT server */
    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(cbk_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);
}

static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en && en) {
        return APP_BLE_OPERATION_ERROR;
    }

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
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }

    log_info("adv_en: %d", en);

    set_ble_work_state(next_state);
    if (en) {
        advertisements_setup_init();
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en);

    return APP_BLE_NO_ERROR;
}

static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info("ble send disconnect");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, con_handle);
        } else {
            log_info("ble wait disconnect...");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

static void ble_module_enable(u8 en)
{
    os_mutex_pend(&mutex, 0);
    log_info("mode_en:%d", en);
    if (en) {
        extern u8 get_ble_gatt_role(void);
        if (1 == get_ble_gatt_role()) {
            ble_stack_gatt_role(0);
            // register for HCI events
            hci_event_callback_set(&cbk_packet_handler);
            le_l2cap_register_packet_handler(&cbk_packet_handler);
        }
        adv_ctrl_en = 1;
        bt_ble_adv_enable(1);
    } else {
        if (con_handle) {
            adv_ctrl_en = 0;
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
    }
    os_mutex_post(&mutex);
}

void bt_ble_init(void)
{
    const u8 *mac_addr = bt_get_mac_addr();

    log_info("***** ble_init******");

    if (!os_mutex_valid(&mutex)) {
        os_mutex_create(&mutex);
    }

    //增加后缀，区分名字
    gap_device_name_len = snprintf(gap_device_name, sizeof(gap_device_name), "xdtp-D2-%02X%02X", mac_addr[4], mac_addr[5]);

    log_info("ble name(%d): %s", gap_device_name_len, gap_device_name);

    duer_wifi_cfg_init(get_dueros_client_id(), get_dueros_device_id(), get_dueros_pub_key());

    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);
}

static void free_att_ram_buffer(void *p)
{
    free(p);
}

static void stack_exit(void)
{
    ble_module_enable(0);

    if (!att_ram_buffer) {
        sys_timeout_add_to_task("sys_timer", att_ram_buffer, free_att_ram_buffer, 3000);
        att_ram_buffer = NULL;
    }
    /* set_ble_work_state(BLE_ST_SEND_STACK_EXIT); */
    /* ble_user_cmd_prepare(BLE_CMD_STACK_EXIT, 1, 0); */
    /* set_ble_work_state(BLE_ST_STACK_EXIT_COMPLETE); */
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******");

    stack_exit();

    duer_wifi_cfg_deinit();
}

int duer_ble_send_data(uint8_t *data, uint32_t data_len, uint16_t attr_id)
{
    return app_send_user_data(attr_id, data, data_len, ATT_OP_AUTO_READ_CCC);
}

#endif

