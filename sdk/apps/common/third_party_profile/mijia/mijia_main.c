#include "app_config.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & MIJIA_EN)

#include "custom_mi_config.h"
#include "mible_log.h"
#include "app_ble_spp_api.h"
#include "avctp_user.h"
#include "mijia_profiles/stdio_service_server.h"
#include "miio_user_api.h"

void *mijia_ble_hdl = NULL;
static u8 mijia_ble_addr[6];
static OS_SEM mijia_sem;

const u8 *bt_get_mac_addr(void);

const u8 *get_mijia_ble_addr(void)
{
    return mijia_ble_addr;
}

void mi_schd_event_handler(schd_evt_t *p_event)
{
    MI_LOG_INFO("USER CUSTOM CALLBACK RECV EVT ID 0x%x\n", p_event->id);

    switch (p_event->id) {
    case SCHD_EVT_OOB_REQUEST:
        MI_LOG_INFO("App selected IO cap is 0x%04X\n", p_event->data.IO_capability);
        switch (p_event->data.IO_capability) {
        case 0x0080:
            //mi_schd_oob_rsp(qr_code, 16);
            MI_LOG_INFO(MI_LOG_COLOR_GREEN "Please scan QR code label.\n");
            break;

        default:
            MI_LOG_ERROR("Selected IO cap is not supported.\n");
        }
        break;

    case SCHD_EVT_KEY_DEL_FAIL:
        MI_LOG_ERROR("SCHD_EVT_KEY_DEL_FAIL\n");
    case SCHD_EVT_KEY_DEL_SUCC:
        MI_LOG_ERROR("SCHD_EVT_KEY_DEL_SUCC\n");
        miio_ble_user_adv_stop();
        miio_ble_user_adv_init(1);
        miio_ble_user_adv_start(100);
        break;

    case SCHD_EVT_REG_SUCCESS:
        break;

    case SCHD_EVT_KEY_FOUND:
        break;

    case SCHD_EVT_KEY_NOT_FOUND:
        break;

    default:
        break;
    }
}

static void stdio_rx_handler(uint8_t *p, uint8_t l)
{
    int errno;
    /* RX plain text (It has been decrypted) */
    MI_LOG_INFO("RX raw data\n");
    MI_LOG_HEXDUMP(p, l);

    /* TX plain text (It will be encrypted before send out.) */
    errno = stdio_tx(p, l);
    MI_ERR_CHECK(errno);
}

/////////////////////////////////////


static const uint8_t mijia_profile_data[] = {
    //////////////////////////////////////////////////////
    //
    // 0x0001 PRIMARY_SERVICE  fe95
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x95, 0xfe,

    /* CHARACTERISTIC,  0004, READ | DYNAMIC, */
    // 0x0002 CHARACTERISTIC 0004 READ | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x02, 0x03, 0x00, 0x04, 0x00,
    // 0x0003 VALUE 0004 READ | DYNAMIC
    0x08, 0x00, 0x02, 0x01, 0x03, 0x00, 0x04, 0x00,

    /* CHARACTERISTIC,  0005, READ | NOTIFY | DYNAMIC, */
    // 0x0004 CHARACTERISTIC 0005 READ | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x04, 0x00, 0x03, 0x28, 0x12, 0x05, 0x00, 0x05, 0x00,
    // 0x0005 VALUE 0005 READ | NOTIFY | DYNAMIC
    0x08, 0x00, 0x12, 0x01, 0x05, 0x00, 0x05, 0x00,
    // 0x0006 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x06, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  0010, WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC, */
    // 0x0007 CHARACTERISTIC 0010 WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x07, 0x00, 0x03, 0x28, 0x14, 0x08, 0x00, 0x10, 0x00,
    // 0x0008 VALUE 0010 WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x14, 0x01, 0x08, 0x00, 0x10, 0x00,
    // 0x0009 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x09, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  0019, WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC, */
    // 0x000a CHARACTERISTIC 0019 WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x03, 0x28, 0x14, 0x0b, 0x00, 0x19, 0x00,
    // 0x000b VALUE 0019 WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x14, 0x01, 0x0b, 0x00, 0x19, 0x00,
    // 0x000c CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x0c, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  0017, WRITE | NOTIFY | DYNAMIC, */
    // 0x000d CHARACTERISTIC 0017 WRITE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x0d, 0x00, 0x03, 0x28, 0x18, 0x0e, 0x00, 0x17, 0x00,
    // 0x000e VALUE 0017 WRITE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x18, 0x01, 0x0e, 0x00, 0x17, 0x00,
    // 0x000f CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x0f, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  0018, WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC, */
    // 0x0010 CHARACTERISTIC 0018 WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x10, 0x00, 0x03, 0x28, 0x14, 0x11, 0x00, 0x18, 0x00,
    // 0x0011 VALUE 0018 WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x14, 0x01, 0x11, 0x00, 0x18, 0x00,
    // 0x0012 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x12, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  001a, WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC, */
    // 0x0013 CHARACTERISTIC 001a WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x13, 0x00, 0x03, 0x28, 0x14, 0x14, 0x00, 0x1a, 0x00,
    // 0x0014 VALUE 001a WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x14, 0x01, 0x14, 0x00, 0x1a, 0x00,
    // 0x0015 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x15, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  001b, WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC, */
    // 0x0016 CHARACTERISTIC 001b WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x16, 0x00, 0x03, 0x28, 0x14, 0x17, 0x00, 0x1b, 0x00,
    // 0x0017 VALUE 001b WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x14, 0x01, 0x17, 0x00, 0x1b, 0x00,
    // 0x0018 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x18, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  001c, WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC, */
    // 0x0019 CHARACTERISTIC 001c WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x19, 0x00, 0x03, 0x28, 0x14, 0x1a, 0x00, 0x1c, 0x00,
    // 0x001a VALUE 001c WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x14, 0x01, 0x1a, 0x00, 0x1c, 0x00,
    // 0x001b CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x1b, 0x00, 0x02, 0x29, 0x00, 0x00,

    //////////////////////////////////////////////////////
    //
    // 0x001c PRIMARY_SERVICE  00000100-0065-6C62-2E74-6F696D2E696D
    //
    //////////////////////////////////////////////////////
    0x18, 0x00, 0x02, 0x00, 0x1c, 0x00, 0x00, 0x28, 0x6d, 0x69, 0x2e, 0x6d, 0x69, 0x6f, 0x74, 0x2e, 0x62, 0x6c, 0x65, 0x00, 0x00, 0x01, 0x00, 0x00,

    /* CHARACTERISTIC,  00000101-0065-6C62-2E74-6F696D2E696D, WRITE_WITHOUT_RESPONSE | DYNAMIC */
    // 0x001d CHARACTERISTIC 00000101-0065-6C62-2E74-6F696D2E696D WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x1b, 0x00, 0x02, 0x00, 0x1d, 0x00, 0x03, 0x28, 0x04, 0x1e, 0x00, 0x6d, 0x69, 0x2e, 0x6d, 0x69, 0x6f, 0x74, 0x2e, 0x62, 0x6c, 0x65, 0x00, 0x01, 0x01, 0x00, 0x00,
    // 0x001e VALUE 00000101-0065-6C62-2E74-6F696D2E696D WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x16, 0x00, 0x04, 0x03, 0x1e, 0x00, 0x6d, 0x69, 0x2e, 0x6d, 0x69, 0x6f, 0x74, 0x2e, 0x62, 0x6c, 0x65, 0x00, 0x01, 0x01, 0x00, 0x00,

    /* CHARACTERISTIC,  00000102-0065-6C62-2E74-6F696D2E696D, NOTIFY | DYNAMIC */
    // 0x001f CHARACTERISTIC 00000102-0065-6C62-2E74-6F696D2E696D NOTIFY | DYNAMIC
    0x1b, 0x00, 0x02, 0x00, 0x1f, 0x00, 0x03, 0x28, 0x10, 0x20, 0x00, 0x6d, 0x69, 0x2e, 0x6d, 0x69, 0x6f, 0x74, 0x2e, 0x62, 0x6c, 0x65, 0x00, 0x02, 0x01, 0x00, 0x00,
    // 0x0020 VALUE 00000102-0065-6C62-2E74-6F696D2E696D NOTIFY | DYNAMIC
    0x16, 0x00, 0x10, 0x03, 0x20, 0x00, 0x6d, 0x69, 0x2e, 0x6d, 0x69, 0x6f, 0x74, 0x2e, 0x62, 0x6c, 0x65, 0x00, 0x02, 0x01, 0x00, 0x00,
    // 0x0021 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x21, 0x00, 0x02, 0x29, 0x00, 0x00,

    // END
    0x00, 0x00,
};

//
// characteristics <--> handles
//
#define ATT_CHARACTERISTIC_0004_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_0005_01_VALUE_HANDLE 0x0005
#define ATT_CHARACTERISTIC_0005_01_CLIENT_CONFIGURATION_HANDLE 0x0006
#define ATT_CHARACTERISTIC_0010_01_VALUE_HANDLE 0x0008
#define ATT_CHARACTERISTIC_0010_01_CLIENT_CONFIGURATION_HANDLE 0x0009
#define ATT_CHARACTERISTIC_0019_01_VALUE_HANDLE 0x000b
#define ATT_CHARACTERISTIC_0019_01_CLIENT_CONFIGURATION_HANDLE 0x000c
#define ATT_CHARACTERISTIC_0017_01_VALUE_HANDLE 0x000e
#define ATT_CHARACTERISTIC_0017_01_CLIENT_CONFIGURATION_HANDLE 0x000f
#define ATT_CHARACTERISTIC_0018_01_VALUE_HANDLE 0x0011
#define ATT_CHARACTERISTIC_0018_01_CLIENT_CONFIGURATION_HANDLE 0x0012
#define ATT_CHARACTERISTIC_001a_01_VALUE_HANDLE 0x0014
#define ATT_CHARACTERISTIC_001a_01_CLIENT_CONFIGURATION_HANDLE 0x0015
#define ATT_CHARACTERISTIC_001b_01_VALUE_HANDLE 0x0017
#define ATT_CHARACTERISTIC_001b_01_CLIENT_CONFIGURATION_HANDLE 0x0018
#define ATT_CHARACTERISTIC_001c_01_VALUE_HANDLE 0x001a
#define ATT_CHARACTERISTIC_001c_01_CLIENT_CONFIGURATION_HANDLE 0x001b
#define ATT_CHARACTERISTIC_00000101_0065_6C62_2E74_6F696D2E696D_01_VALUE_HANDLE 0x001e
#define ATT_CHARACTERISTIC_00000102_0065_6C62_2E74_6F696D2E696D_01_VALUE_HANDLE 0x0020
#define ATT_CHARACTERISTIC_00000102_0065_6C62_2E74_6F696D2E696D_01_CLIENT_CONFIGURATION_HANDLE 0x0021


static uint16_t custom_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;
    mible_gatts_evt_t evt;
    mible_gatts_evt_param_t param;
    printf("<-------------read_callback, handle= 0x%04x,buffer= %08x", handle, (u32)buffer);
    switch (handle) {
    /* case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE: */
    /* const char *gap_name = bt_get_local_name(); */
    /* att_value_len = strlen(gap_name); */
    /* if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) { */
    /* break; */
    /* } */
    /* if (buffer) { */
    /* memcpy(buffer, &gap_name[offset], buffer_size); */
    /* att_value_len = buffer_size; */
    /* printf("\n------read gap_name: %s", gap_name); */
    /* } */
    /* break; */
    case ATT_CHARACTERISTIC_0004_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0005_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0010_01_VALUE_HANDLE:
        mible_gatts_value_get(0x01, handle, buffer, (uint8_t *)&att_value_len);
        if (buffer) {
            put_buf(buffer, att_value_len);
        }
        break;
    case ATT_CHARACTERISTIC_0019_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0017_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0018_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_001a_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_001b_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_001c_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_00000101_0065_6C62_2E74_6F696D2E696D_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_00000102_0065_6C62_2E74_6F696D2E696D_01_VALUE_HANDLE:
        evt = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
        param.conn_handle = connection_handle;
        param.read.value_handle = handle;
        mible_gatts_event_callback(evt, &param);
        break;
    case ATT_CHARACTERISTIC_0005_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0010_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0019_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0017_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0018_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_001a_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_001b_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_001c_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_00000102_0065_6C62_2E74_6F696D2E696D_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = multi_att_get_ccc_config(connection_handle, handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
    default:
        break;
    }
    printf("att_value_len= %d", att_value_len);
    return att_value_len;
    return 0;
}


int mijia_ble_send(u8 *data, u32 len)
{
    int ret = 0;
    /* int i; */
    /* printf("mijia_ble_send len = %d", len); */
    /* put_buf(data, len); */
    /* ret = app_ble_att_send_data(mijia_ble_hdl, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC); */
    /* if (ret) { */
    /* printf("send fail\n"); */
    /* } */
    return ret;
}

static int custom_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;
    u16 handle = att_handle;
    mible_gatts_evt_t evt;
    mible_gatts_evt_param_t param;
    printf("<-------------write_callback, handle= 0x%04x,size = %d", handle, buffer_size);
    //put_buf(buffer, buffer_size);
    switch (handle) {
    /* case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE: */
    /* break; */
    /* case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE: */
    /* printf("rx(%d):\n", buffer_size); */
    /* put_buf(buffer, buffer_size); */
    /* // test */
    /* mijia_ble_send(buffer, buffer_size); */
    /* break; */
    case ATT_CHARACTERISTIC_0017_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0018_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_00000101_0065_6C62_2E74_6F696D2E696D_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_00000102_0065_6C62_2E74_6F696D2E696D_01_VALUE_HANDLE:
        evt = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
        param.conn_handle = connection_handle;
        param.write.value_handle = handle;
        param.write.offset = offset;
        param.write.data = buffer;
        param.write.len = buffer_size;
        mible_gatts_event_callback(evt, &param);
        os_sem_post(&mijia_sem);
        break;
    case ATT_CHARACTERISTIC_0004_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0005_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_0010_01_VALUE_HANDLE:
        mible_gatts_value_set(0x01, handle, offset, buffer, buffer_size);
    case ATT_CHARACTERISTIC_0019_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_001a_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_001b_01_VALUE_HANDLE:
    case ATT_CHARACTERISTIC_001c_01_VALUE_HANDLE:
        evt = MIBLE_GATTS_EVT_WRITE;
        param.conn_handle = connection_handle;
        param.write.value_handle = handle;
        param.write.offset = offset;
        param.write.data = buffer;
        param.write.len = buffer_size;
        mible_gatts_event_callback(evt, &param);
        os_sem_post(&mijia_sem);
        break;
    case ATT_CHARACTERISTIC_0005_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0010_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0019_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0017_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_0018_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_001a_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_001b_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_001c_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_00000102_0065_6C62_2E74_6F696D2E696D_01_CLIENT_CONFIGURATION_HANDLE:
        printf("\nwrite ccc:%04x, %02x\n", handle, buffer[0]);
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        break;
    default:
        break;
    }
    return 0;
}

static void custom_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 con_handle;
    mible_gap_evt_param_t evt_param;
    // printf("cbk packet_type:0x%x, packet[0]:0x%x, packet[2]:0x%x", packet_type, packet[0], packet[2]);
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_CAN_SEND_NOW:
            printf("ATT_EVENT_CAN_SEND_NOW");
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = little_endian_read_16(packet, 4);
                printf("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: 0x%x", con_handle);
                evt_param.conn_handle = con_handle;
                //reverse_bd_addr(&packet[8], evt_param.connect.peer_addr);
                memcpy(evt_param.connect.peer_addr, &packet[8], 6);
                evt_param.connect.type = MIBLE_ADDRESS_TYPE_RANDOM;
                evt_param.connect.role = MIBLE_GAP_PERIPHERAL;
                evt_param.connect.conn_param.min_conn_interval = little_endian_read_16(packet, 14);
                evt_param.connect.conn_param.max_conn_interval = little_endian_read_16(packet, 14);
                evt_param.connect.conn_param.slave_latency = little_endian_read_16(packet, 16);
                evt_param.connect.conn_param.conn_sup_timeout = little_endian_read_16(packet, 18);
                printf("conn_interval = %d\n", evt_param.connect.conn_param.max_conn_interval);
                printf("conn_latency  = %d\n",  evt_param.connect.conn_param.slave_latency);
                printf("conn_timeout  = %d\n",  evt_param.connect.conn_param.conn_sup_timeout);
                mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &evt_param);
                put_buf(&packet[8], 6);
                att_server_set_exchange_mtu(con_handle); //主动请求交换MTU
                break;
            default:
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("HCI_EVENT_DISCONNECTION_COMPLETE: %0x", packet[5]);
            //CONNECTION_TIMEOUT;
            //REMOTE_USER_TERMINATED;
            //LOCAL_HOST_TERMINATED;
            evt_param.disconnect.reason = REMOTE_USER_TERMINATED;
            mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNECT, &evt_param);
            break;
        default:
            break;
        }
        break;
    }
    return;
}


///////////////////////////////////////

static void mijia_ble_init(void)
{
    if (mijia_ble_hdl == NULL) {
        mijia_ble_hdl = app_ble_hdl_alloc();
        if (mijia_ble_hdl == NULL) {
            printf("mijia_ble_hdl alloc err !\n");
            return;
        }
        app_ble_set_mac_addr(mijia_ble_hdl, (void *)mijia_ble_addr);
        app_ble_profile_set(mijia_ble_hdl, mijia_profile_data);
        app_ble_att_read_callback_register(mijia_ble_hdl, custom_att_read_callback);
        app_ble_att_write_callback_register(mijia_ble_hdl, custom_att_write_callback);
        app_ble_att_server_packet_handler_register(mijia_ble_hdl, custom_cbk_packet_handler);
        app_ble_hci_event_callback_register(mijia_ble_hdl, custom_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(mijia_ble_hdl, custom_cbk_packet_handler);
    }
}

static void gatt_test_notify(void)
{
    miio_gatt_property_changed(1, 2, property_value_new_uchar(80));
    miio_gatt_property_changed(2, 4, property_value_new_float(-12.3f));
    miio_gatt_property_changed(3, 6, property_value_new_float(45.6f));
}

static void on_property_set(property_operation_t *o)
{
    printf("mijia [on_property_set] siid %d, piid %d\n", o->siid, o->piid);

    if (o->value == NULL) {
        MI_LOG_ERROR("value is NULL\n");
        return;
    }
    o->code = 0;
}

static void on_property_get(property_operation_t *o)
{
    printf("mijia [on_property_get] siid %d, piid %d\n", o->siid, o->piid);
    o->value = property_value_new_float(888.88);
}

static void on_action_invoke(action_operation_t *o)
{
    printf("mijia [on_action_invoke] siid %d, aiid %d\n", o->siid, o->aiid);
    o->code = 0;
}


static const uint8_t support_devinfo[] = {
    DEV_INFO_NEW_SN,
    DEV_INFO_HARDWARE_VERSION
};

static void user_devinfo_callback(dev_info_type_t type, dev_info_t *buf)
{
    printf("user_devinfo_callback 0x%x\n", type);
    switch (type) {
    case DEV_INFO_SUPPORT:
        buf->len = sizeof(support_devinfo);
        memcpy(buf->buff, support_devinfo, buf->len);
        break;
    case DEV_INFO_NEW_SN:
        buf->len = strlen("123456789/123456789/123456789/123456789/123456789/123456789/12");
        memcpy(buf->buff, "123456789/123456789/123456789/123456789/123456789/123456789/12", buf->len);
        break;
    case DEV_INFO_HARDWARE_VERSION:
        buf->len = strlen("WL83");
        memcpy(buf->buff, "WL83", buf->len);
        break;
    default:
        buf->code = MI_ERR_NOT_FOUND;
        return;
    }
    buf->code = MI_SUCCESS;
    return;
}

static const uint8_t support_devinfo_1[] = {
    DEV_INFO_DEVICE_SN,
    DEV_INFO_HARDWARE_VERSION,
    DEV_INFO_LATITUDE,
    DEV_INFO_LONGITUDE,
    DEV_INFO_VENDOR1,
    DEV_INFO_VENDOR2,
};

static void user_devinfo_callback1(dev_info_type_t type, dev_info_t *buf)
{
    printf("user_devinfo_callback 0x%x\n", type);
    switch (type) {
    case DEV_INFO_SUPPORT:
        buf->len = sizeof(support_devinfo);
        memcpy(buf->buff, support_devinfo, buf->len);
        break;
    case DEV_INFO_DEVICE_SN:
        buf->len = strlen("12345/A8Q600002");
        memcpy(buf->buff, "12345/A8Q600002", buf->len);
        break;
    case DEV_INFO_HARDWARE_VERSION:
        buf->len = strlen("nRF52832.DK.1");
        memcpy(buf->buff, "nRF52832.DK.1", buf->len);
        break;
    case DEV_INFO_VENDOR1:
        buf->len = strlen("VendorData.1");
        memcpy(buf->buff, "VendorData.1", buf->len);
        break;
    case DEV_INFO_VENDOR2:
        buf->len = strlen("VendorData.2");
        memcpy(buf->buff, "VendorData.2", buf->len);
        break;
    case DEV_INFO_LATITUDE:
        buf->len = strlen("116.397128");
        memcpy(buf->buff, "116.397128", buf->len);
        break;
    case DEV_INFO_LONGITUDE:
        buf->len = strlen("39.916527");
        memcpy(buf->buff, "39.916527", buf->len);
        break;
    default:
        buf->code = MI_ERR_NOT_FOUND;
        return;
    }
    buf->code = MI_SUCCESS;
    return;
}


static void mijia_loop_process(void *parm)
{
    /* printf("mijia_loop_process\n"); */
    while (1) {
        os_sem_pend(&mijia_sem, 0);
        mi_schd_process();
        mi_schd_process();
        mi_schd_process();
        mible_tasks_exec();
    }
}

/* u8 user_data[] = "JL"; */

static void mi_process(void *priv)
{
    os_sem_post(&mijia_sem);
}

static void *gatewaytest_timer;

static void gatewaytest_handler(void *p_context)
{
    static float temp = 0.0f;
    static float hum = 0.0f;
    static int8_t  battery = 0;
    printf("test_handler:%d\n", miio_ble_get_registered_state());
    if (miio_ble_get_registered_state()) {
        temp = temp < 100.0f ? temp + 1.0f : -30.0f;
        hum = hum < 100.0f ? hum + 1.0f : 0.0f;
        battery = battery < 100 ? battery + 1 : 0;
        miio_ble_property_changed(3, 1001, property_value_new_float(temp), 0);
        miio_ble_property_changed(3, 1008, property_value_new_float(hum), 1);
        miio_ble_property_changed(2, 1003, property_value_new_uchar(battery), 0);
        miio_ble_event_occurred(2, 1001, NULL, 0);
    }
}

static void user_app_init(void)
{
    uint32_t delay_ms = 6000;
    miio_timer_create(&gatewaytest_timer, gatewaytest_handler, MIBLE_TIMER_REPEATED);
    miio_timer_start(gatewaytest_timer, delay_ms, NULL);
    //sys_timer_add(NULL, gatewaytest_handler, 1000);
}

int mijia_all_init(void)
{
    bt_make_ble_address(mijia_ble_addr, (u8 *)bt_get_mac_addr());
    mijia_ble_addr[5] |= 0xC0;

    os_sem_create(&mijia_sem, 0);
    //step1: init ble stack
    //ble_stack_init();
    //gap_init();
    //gatt_init();
    //step2: init ble service
    //services_init();
    mi_service_init();
    miio_system_info_callback_register(user_devinfo_callback);
    miio_gatt_spec_init(on_property_set, on_property_get, on_action_invoke, 64, 8);
    stdio_service_init(stdio_rx_handler);
    //step3: init & start mi schd
    mi_scheduler_init(10, mi_schd_event_handler, NULL);
    mi_scheduler_start(SYS_KEY_RESTORE);
    //mi_scheduler_start(SYS_KEY_DELETE);
    //step4: start adv
    mijia_ble_init();
    miio_ble_user_adv_init(1);
    miio_ble_user_adv_start(100);   // pairing
    //miio_ble_user_adv_start(500);       // paired
    /* mibeacon_adv_data_set(1, 0, user_data, 2); */
    /* mibeacon_adv_start(300); */

    //step5: init user function
    //user_app_init();
    //step6: wait event to exec functions in main loop
    /* while(1){ */
    /* os_time_dly(50); */
    /* mi_schd_process(); */
    /* mible_tasks_exec(); */
    /* os_time_dly(50); */
    /* user_app_main_thread(); */
    /* } */

    task_create(mijia_loop_process, NULL, "app_proto");
    sys_timer_add(NULL, mi_process, 500);
    return 0;
}

int mijia_all_exit(void)
{
    return 0;
}

void mijia_key_delete(void)
{
    mi_scheduler_start(SYS_KEY_DELETE);
}

void test_mijia_key_do_in_task(int key)
{
    printf("test_mijia_key_do_in_task %d\n", key);
    switch (key) {
    case 0:
        mijia_key_delete();
        break;
    case 1:
        gatt_test_notify();
        break;
    }
}

void test_mijia_key_do(u8 key)
{
    int msg[3];
    msg[0] = (int)test_mijia_key_do_in_task;
    msg[1] = 1;
    msg[2] = (int)key;
    int err = os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("%s post fail\n", __func__);
    }
}

#endif
