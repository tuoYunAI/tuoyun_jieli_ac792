#include "btstack/avctp_user.h"
#include "multi_protocol_main.h"
#include "le_alipay.h"
#include "ble_user.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & ALIPAY_EN)

#include "alipay.h"

#define LOG_TAG     		"[LE_ALIPAY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"


static void (*upay_recv_callback)(const uint8_t *data, u16 len);
static void *le_alipay_hdl = NULL;

static const uint8_t le_alipay_profile_data[] = {
    //////////////////////////////////////////////////////
    //
    // 0x0001 PRIMARY_SERVICE    1800
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,

    /* CHARACTERISTIC,    2a00, READ | WRITE | DYNAMIC, */
    // 0x0002 CHARACTERISTIC 2a00 READ | WRITE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x0a, 0x03, 0x00, 0x00, 0x2a,
    // 0x0003 VALUE 2a00 READ | WRITE | DYNAMIC
    0x08, 0x00, 0x0a, 0x01, 0x03, 0x00, 0x00, 0x2a,

    //////////////////////////////////////////////////////
    //
    // 0x0004 PRIMARY_SERVICE    1801
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x01, 0x18,

    /* CHARACTERISTIC,    2a05, INDICATE, */
    // 0x0005 CHARACTERISTIC 2a05 INDICATE
    0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x20, 0x06, 0x00, 0x05, 0x2a,
    // 0x0006 VALUE 2a05 INDICATE
    0x08, 0x00, 0x20, 0x00, 0x06, 0x00, 0x05, 0x2a,
    // 0x0007 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x07, 0x00, 0x02, 0x29, 0x00, 0x00,


    //////////////////////////////////////////////////////
    //
    // 0x0030 PRIMARY_SERVICE    3802
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x30, 0x00, 0x00, 0x28, 0x02, 0x38,

    /* CHARACTERISTIC,    4a02, READ | WRITE | NOTIFY | DYNAMIC, */
    // 0x0031 CHARACTERISTIC 4a02 READ | WRITE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x31, 0x00, 0x03, 0x28, 0x1a, 0x32, 0x00, 0x02, 0x4a,
    // 0x0032 VALUE 4a02 READ | WRITE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x1a, 0x01, 0x32, 0x00, 0x02, 0x4a,
    // 0x0033 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x33, 0x00, 0x02, 0x29, 0x00, 0x00,

    // END
    0x00, 0x00,
};


//
// characteristics <--> handles
//
#define ATT_CHARACTERISTIC_4a02_01_VALUE_HANDLE 0x0032
#define ATT_CHARACTERISTIC_4a02_01_CLIENT_CONFIGURATION_HANDLE 0x0033

static u16 alipay_adv_interval_min = 150;
u8 alipay_adv_en;/*upay绑定模式*/

static int ble_disconnect(void *priv)
{
    if (app_ble_get_hdl_con_handle(le_alipay_hdl)) {
        /* if (BLE_ST_SEND_DISCONN != rcsp_get_ble_work_state()) { */
        log_info(">>>ble send disconnect\n");
        /* set_ble_work_state(BLE_ST_SEND_DISCONN); */
        app_ble_disconnect(le_alipay_hdl);
        /* app_ble_disconnect(rcsp_server_ble_hdl1); */
        /* } else { */
        /* log_info(">>>ble wait disconnect...\n"); */
        /* } */
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

static void ble_app_disconnect(void)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    ble_disconnect(NULL);
}

static int alipay_adv_enable(u8 enable);

void set_address_for_adv_handle(u8 adv_handle, u8 *addr);

/*upay recieve data regies*/
void upay_ble_regiest_recv_handle(void (*handle)(const uint8_t *data, u16 len))
{
    upay_recv_callback = handle;
}

/*upay send data api*/
int upay_ble_send_data(u8 *data, u16 len)
{
    log_info("upay_ble_send(%d):", len);
    log_info_hexdump(data, len);
    int ret = app_ble_att_send_data(le_alipay_hdl, ATT_CHARACTERISTIC_4a02_01_VALUE_HANDLE, data, len, ATT_OP_NOTIFY);
    if (ret) {
        log_error("send fail");
    }
    return ret;
}

static void alipay_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 con_handle;

    log_info("cbk packet_type:0x%x, packet[0]:0x%x, packet[2]:0x%x", packet_type, packet[0], packet[2]);

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_CAN_SEND_NOW:
            log_info("ATT_EVENT_CAN_SEND_NOW");
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = little_endian_read_16(packet, 4);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x", con_handle);
                // reverse_bd_addr(&packet[8], addr);
                put_buf(&packet[8], 6);
                break;
            default:
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x", packet[5]);
            /* alipay_adv_enable(1); */
            alipay_adv_enable(alipay_adv_en);
            /* ble_module_enable(!alipay_adv_en); */
            break;
        default:
            break;
        }
        break;
    }
    return;
}

static uint16_t alipay_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("<-------------read_callback, handle= 0x%04x,buffer= %08x", handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_4a02_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = multi_att_get_ccc_config(connection_handle, handle);
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

static int alipay_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    u16 handle = att_handle;

    log_info("<-------------write_callback, handle= 0x%04x,size = %d", handle, buffer_size);

    switch (handle) {
    case ATT_CHARACTERISTIC_4a02_01_CLIENT_CONFIGURATION_HANDLE:
        log_info("------write ccc 4a20:%04x, %02x", att_handle, buffer[0]);
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        break;
    case ATT_CHARACTERISTIC_4a02_01_VALUE_HANDLE:
        log_info("upay_ble_recv(%d):", buffer_size);
        put_buf(buffer, buffer_size);
        if (upay_recv_callback) {
            upay_recv_callback(buffer, buffer_size);
        }
        break;
    default:
        break;
    }
    return 0;
}

static u8 alipay_fill_adv_data(u8 *adv_data)
{
    u8 offset = 0;
    u8 service_data[8];
    const uint8_t *edr_addr = bt_get_mac_addr();
    u8 alipay_addr[6];
    memcpy(alipay_addr, edr_addr, 6);
    alipay_addr[5] = alipay_addr[5] + 1;
    offset += make_eir_packet_val(&adv_data[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    service_data[0] = 0x02;
    service_data[1] = 0x38;
    /* le_controller_get_mac(tmp_data); */
    swapX(alipay_addr, &service_data[2], 6);
    offset += make_eir_packet_data(&adv_data[offset], offset, HCI_EIR_DATATYPE_SERVICE_DATA, service_data, 8);
    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("***rsp_data overflow!!!!!!");
        return 0;
    }
    return offset;
}

static u8 alipay_fill_rsp_data(u8 *rsp_data)
{
    u8 offset = 0;
    const char *edr_name = bt_get_local_name();
    offset += make_eir_packet_data(&rsp_data[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, strlen(edr_name));
    return 0;
}

static int alipay_adv_enable(u8 enable)
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    uint8_t advData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t rspData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t len = 0;

    if (enable == app_ble_adv_state_get(le_alipay_hdl)) {
        return 0;
    }
    if (enable) {
        app_ble_set_adv_param(le_alipay_hdl, alipay_adv_interval_min, adv_type, adv_channel);
        len = alipay_fill_adv_data(advData);
        if (len) {
            app_ble_adv_data_set(le_alipay_hdl, advData, len);
        }
        len = alipay_fill_rsp_data(rspData);
        if (len) {
            app_ble_rsp_data_set(le_alipay_hdl, rspData, len);
        }
        u8 *addr = app_ble_adv_addr_get(le_alipay_hdl);
        u8 adv_handle = app_ble_adv_handle_get(le_alipay_hdl);
        set_address_for_adv_handle(adv_handle, addr);
    }
    app_ble_adv_enable(le_alipay_hdl, enable);
    return 0;
}

u8 *get_alipay_adv_addr(void)
{
    return app_ble_adv_addr_get(le_alipay_hdl);
}

/*open or close upay*/
void upay_ble_mode_enable(u8 enable)
{
    log_info("upay_mode_enable= %d", enable);
    if (enable) {
        ble_app_disconnect();
        /* ble_module_enable(!enable); */
    } else {
        app_ble_disconnect(le_alipay_hdl);
        if (!app_ble_get_hdl_con_handle(le_alipay_hdl)) {
            /* ble_module_enable(!enable); */
        }
    }
    alipay_adv_enable(enable);
    alipay_adv_en = enable;
}

void alipay_init(void)
{
    log_info("alipay_init");

    const uint8_t *edr_addr = bt_get_mac_addr();
    log_info("edr addr:");
    put_buf((uint8_t *)edr_addr, 6);

    u8 alipay_addr[6];
    memcpy(alipay_addr, edr_addr, 6);
    alipay_addr[5] = alipay_addr[5] + 1;

    // BLE init
    if (le_alipay_hdl == NULL) {
        le_alipay_hdl = app_ble_hdl_alloc();
        if (le_alipay_hdl == NULL) {
            log_error("le_alipay_hdl alloc err !");
            return;
        }
        app_ble_set_mac_addr(le_alipay_hdl, (void *)alipay_addr);
        app_ble_profile_set(le_alipay_hdl, le_alipay_profile_data);
        app_ble_att_read_callback_register(le_alipay_hdl, alipay_att_read_callback);
        app_ble_att_write_callback_register(le_alipay_hdl, alipay_att_write_callback);
        app_ble_att_server_packet_handler_register(le_alipay_hdl, alipay_cbk_packet_handler);
        app_ble_hci_event_callback_register(le_alipay_hdl, alipay_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(le_alipay_hdl, alipay_cbk_packet_handler);
        /* alipay_adv_enable(1); */
    }
}

void alipay_exit(void)
{
    log_info("alipay_exit");

    if (!le_alipay_hdl) {
        return;
    }

    if (app_ble_get_hdl_con_handle(le_alipay_hdl)) {
        app_ble_disconnect(le_alipay_hdl);
    }
    alipay_adv_enable(0);
    app_ble_hdl_free(le_alipay_hdl);
    le_alipay_hdl = NULL;
}

#endif

