// *****************************************************************************
/* EXAMPLE_START
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************
#include "event/bt_event.h"
#include "syscfg/syscfg_id.h"
#include "btstack/btstack_task.h"
#include "btstack/le/ble_api.h"
#include "btstack/bluetooth.h"
#include "btstack/le/le_user.h"
#include "btcontroller_modules.h"
#include "multi_protocol_main.h"
#include "bt_common.h"
#include "ble_user.h"
#include "le_common.h"
#include "le_hogp.h"
#include "app_config.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & LE_HOGP_EN)

#include "standard_hid.h"

#define LOG_TAG     		"[LE_HOGP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};
//用户可配对的，这是样机跟客户开发的app配对的秘钥
//const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b};

//---------------
//---------------
// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN            (160 * 1)

static void *le_hogp_ble_hdl;
static volatile hci_con_handle_t con_handle;
static u16 cur_conn_latency;

//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t connection_update_enable = 1;
static uint8_t connection_update_cnt = 0;     //当前请求的参数表index
static uint8_t connection_update_waiting = 0; //请求已被接收，等待主机更新参数

//参数表
static const struct conn_update_param_t Peripheral_Preferred_Connection_Parameters[] = {
    {6, 9,  100, 600}, //android
    {12, 12, 30, 400}, //ios
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(Peripheral_Preferred_Connection_Parameters)/sizeof(struct conn_update_param_t))


static char gap_device_name[BT_NAME_LEN_MAX] = "JL-HID-TEST(BLE)";
static u8 gap_device_name_len;      //名字长度，不包含结束符
static u8 ble_work_state;           //ble 状态变化
static u8 first_pair_flag;          //第一次配对标记
static u16 adv_interval_val;        //广播周期 (unit:0.625ms)

//--------------------------------------------
//配置有配对绑定时,先定向广播快连,再普通广播;否则只作普通广播
#define PAIR_DIREDT_ADV_EN          1

#define BLE_VM_HEAD_TAG             (0xB35C)
#define BLE_VM_TAIL_TAG             (0x5CB3)

struct pair_info_t {
    u16 head_tag;             //头标识
    u8  pair_flag: 2;         //配对信息是否有效
    u8  direct_adv_flag: 1;   //是否需要定向广播
    u8  direct_adv_cnt: 5;    //定向广播次数
    u8  peer_address_info[7]; //绑定的对方地址信息
    u16 tail_tag;//尾标识
};

//配对信息表
static struct pair_info_t conn_pair_info;
static u8 cur_peer_addr_info[7];//当前连接对方地址信息

#define REPEAT_DIRECT_ADV_COUNT  (2)// *1.28s
#define REPEAT_DIRECT_ADV_TIMER  (100)//

//为了及时响应手机数据包，某些流程会忽略进入latency的控制
//如下是定义忽略进入的interval个数
#define LATENCY_SKIP_INTERVAL_MIN     (3)
#define LATENCY_SKIP_INTERVAL_KEEP    (15)
#define LATENCY_SKIP_INTERVAL_LONG    (0xffff)

//hid device infomation
static u8 Appearance[] = {BLE_APPEARANCE_GENERIC_HID & 0xff, BLE_APPEARANCE_GENERIC_HID >> 8}; //
static const char Manufacturer_Name_String[] = "zhuhai_jieli";
static const char Model_Number_String[] = "hid_mouse";
static const char Serial_Number_String[] = "000000";
static const char Hardware_Revision_String[] = "0.0.1";
static const char Firmware_Revision_String[] = "0.0.1";
static const char Software_Revision_String[] = "0.0.1";
static const u8 System_ID[] = {0, 0, 0, 0, 0, 0, 0, 0};

//定义的产品信息,for test
#define  PNP_VID_SOURCE   0x02
#define  PNP_VID          0x05ac //0x05d6
#define  PNP_PID          0x022C //
#define  PNP_PID_VERSION  0x011b //1.1.11

static const u8 PnP_ID[] = {PNP_VID_SOURCE, PNP_VID & 0xFF, PNP_VID >> 8, PNP_PID & 0xFF, PNP_PID >> 8, PNP_PID_VERSION & 0xFF, PNP_PID_VERSION >> 8};
/* static const u8 PnP_ID[] = {0x02, 0x17, 0x27, 0x40, 0x00, 0x23, 0x00}; */
/* static const u8 PnP_ID[] = {0x02, 0xac, 0x05, 0x2c, 0x02, 0x1b, 0x01}; */

/* static const u8 hid_information[] = {0x11, 0x01, 0x00, 0x01}; */
static const u8 hid_information[] = {0x01, 0x01, 0x00, 0x03};

static const u8 *report_map; //描述符
static u16 report_map_size;//描述符大小
#define HID_REPORT_MAP_DATA    report_map
#define HID_REPORT_MAP_SIZE    report_map_size

//report 发生变化，通过service change 通知主机重新获取
static u8 hid_report_change = 0;
static u8 hid_battery_level = 88;
//------------------------------------------------------
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static void (*le_hogp_output_callback)(u8 *buffer, u16 size) = NULL;

static void resume_all_ccc_enable(u8 update_request);
static void ble_hid_set_config(void);
void ble_hid_transfer_channel_recieve(u8 *packet, u16 size);
void app_ble_att_set_ccc_config(void *_hdl, u16 att_handle, uint16_t cfg);
static int set_adv_enable(u32 en);


static void ble_bt_evnet_post(u8 arg_type, u8 priv_event, u8 *args, u32 value)
{
    struct bt_event e = {0};
    if (args) {
        memcpy(e.args, args, 7);
    }
    e.event = priv_event;
    e.value = value;
    bt_event_notify(arg_type, &e);
}

static void ble_state_to_user(u8 state, u8 reason)
{
    ble_bt_evnet_post(BT_EVENT_FROM_BLE, state, NULL, reason);
}

//@function 检测连接参数是否需要更新
static void check_connetion_updata_deal(void)
{
    static struct conn_update_param_t param;
    if (connection_update_enable) {
        if (connection_update_cnt < CONN_PARAM_TABLE_CNT) {
            memcpy(&param, &Peripheral_Preferred_Connection_Parameters[connection_update_cnt], sizeof(struct conn_update_param_t));
            log_info("update_request:-%d-%d-%d-%d", param.interval_min, param.interval_max, param.latency, param.timeout);
            if (con_handle) {
                ble_op_conn_param_request(con_handle, (u32)&param);
            }
        }
    }
}

//@function 接参数更新信息
static void connection_update_complete_success(u8 *packet, u8 connected_init)
{
    int conn_interval, conn_latency, conn_timeout;

    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d", conn_interval);
    log_info("conn_latency = %d", conn_latency);
    log_info("conn_timeout = %d", conn_timeout);

    if (connected_init) {
        if (conn_pair_info.pair_flag
            && (conn_latency && (0 == memcmp(conn_pair_info.peer_address_info, packet - 1, 7)))) { //
            //有配对信息，带latency连接，快速enable ccc 发数
            log_info("reconnect ok");
            resume_all_ccc_enable(0);
        }
    } else {
        //not creat connection,judge
        if ((conn_interval == 12) && (conn_latency == 4)) {
            log_info("do update_again");
            connection_update_cnt = 1;
            check_connetion_updata_deal();
        } else if ((conn_latency == 0)
                   && (connection_update_cnt == CONN_PARAM_TABLE_CNT)
                   && (Peripheral_Preferred_Connection_Parameters[0].latency != 0)) {
            log_info("latency is 0,update_again");
            connection_update_cnt = 0;
            check_connetion_updata_deal();
        }
    }

    cur_conn_latency = conn_latency;
}

static void set_ble_work_state(ble_state_e state, u8 reason)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback(NULL, state);
        }
        ble_state_to_user(state, reason);
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static void conn_pair_vm_do(struct pair_info_t *info, u8 rw_flag)
{
#if PAIR_DIREDT_ADV_EN
    int ret;
    int vm_len = sizeof(struct pair_info_t);

    log_info("conn_pair_info vm_do:%d", rw_flag);

    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_MODE_INFO, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--");
        }

        if ((BLE_VM_HEAD_TAG == info->head_tag) && (BLE_VM_TAIL_TAG == info->tail_tag)) {
            log_info("-exist--");
            log_info_hexdump((u8 *)info, vm_len);
            info->direct_adv_flag = 1;
        } else {
            memset(info, 0, vm_len);
            info->head_tag = BLE_VM_HEAD_TAG;
            info->tail_tag = BLE_VM_TAIL_TAG;
        }
    } else {
        info->direct_adv_flag = 1;
        syscfg_write(CFG_BLE_MODE_INFO, (u8 *)info, vm_len);
    }
#endif
}

//检测是否需要定向广播
static void ble_check_need_pair_adv(void)
{
#if PAIR_DIREDT_ADV_EN
    log_info("%s", __FUNCTION__);
    if (conn_pair_info.pair_flag && conn_pair_info.direct_adv_flag) {
        conn_pair_info.direct_adv_cnt = REPEAT_DIRECT_ADV_COUNT;
    }
#endif
}

//协议栈sm 消息回调处理
static void le_hogp_cbk_sm_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            log_info("Just Works Confirmed.");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            first_pair_flag = 1;
            memcpy(conn_pair_info.peer_address_info, &packet[4], 7);
            conn_pair_info.pair_flag = 0;
            ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_KEEP);
            ble_state_to_user(BLE_PRIV_MSG_PAIR_CONFIRM, 0);
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            u32 tmp32;
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u", tmp32);
            ble_state_to_user(BLE_PRIV_MSG_PAIR_CONFIRM, 1);
            break;
        }
        break;
    }
}

static void check_report_map_change(hci_con_handle_t connection_handle)
{
#if 0 //部分手机不支持
    static const u16 change_handle_table[2] = {ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE, ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE};
    if (hid_report_change && first_pair_flag && multi_att_get_ccc_config(connection_handle, ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE)) {
        log_info("send services changed");
        app_ble_att_send_data(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a05_01_VALUE_HANDLE, change_handle_table, 4, ATT_OP_INDICATE);
        hid_report_change = 0;
    }
#endif
}

//回连状态，主动使能通知
static void resume_all_ccc_enable(u8 update_request)
{
    log_info("resume_all_ccc_enable");

    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    app_ble_att_set_ccc_config(le_hogp_ble_hdl, ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

    set_ble_work_state(BLE_ST_NOTIFY_IDICATE, 0);

    if (update_request) {
        check_connetion_updata_deal();
        ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_MIN);
    }
}

//协议栈回调唤醒数据发送
static void can_send_now_wakeup(void)
{
    /* putchar('E'); */
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }
}

//参考识别手机系统
static void att_check_remote_result(u16 con_handle, remote_type_e remote_type)
{
    log_info("le_hogp %02x, remote_type= %02x", con_handle, remote_type);
    /* ble_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_BLE_REMOTE_TYPE, NULL, remote_type); */
}

/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
//协议栈事件消息处理
static void le_hogp_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;

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
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                switch (packet[3]) {
                case BT_OP_SUCCESS:
                    //init
                    first_pair_flag = 0;
                    connection_update_cnt = 0;
                    connection_update_waiting = 0;
                    cur_conn_latency = 0;
                    connection_update_enable = 1;

                    con_handle = little_endian_read_16(packet, 4);
                    log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x", con_handle);

                    log_info_hexdump(packet + 7, 7);
                    memcpy(cur_peer_addr_info, packet + 7, 7);
                    ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_KEEP);
                    set_ble_work_state(BLE_ST_CONNECT, 0);
                    connection_update_complete_success(packet + 8, 1);

                    //清除PC端历史键值
                    /* extern void clear_mouse_packet_data(void); */
                    /* sys_timeout_add(NULL, clear_mouse_packet_data, 50); */
                    /* att_server_set_exchange_mtu(con_handle); //主动请求交换MTU */
                    /* ble_vendor_interval_event_enable(con_handle, 1); //enable interval事件-->HCI_SUBEVENT_LE_VENDOR_INTERVAL_COMPLETE */
                    break;

                case BT_ERR_ADVERTISING_TIMEOUT:
                    //定向广播超时结束
                    log_info("BT_ERR_ADVERTISING_TIMEOUT");
                    set_adv_enable(0);
                    set_adv_enable(1);
                    break;

                default:
                    break;
                }
            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_waiting = 0;
                connection_update_complete_success(packet, 0);
                break;

            case HCI_SUBEVENT_LE_VENDOR_INTERVAL_COMPLETE:
                log_info("INTERVAL_EVENT:%04x, %04x", little_endian_read_16(packet, 4), little_endian_read_16(packet, 6));
                /* put_buf(packet, size + 1); */
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x", packet[5]);
            con_handle = 0;
            set_ble_work_state(BLE_ST_DISCONN, packet[5]);
#if 0
            if (UPDATE_MODULE_IS_SUPPORT(UPDATE_BLE_TEST_EN)) {
                if (!ble_update_get_ready_jump_flag()) {
                    if (packet[5] == 0x08) {
                        //超时断开,检测是否配对广播
                        ble_check_need_pair_adv();
                    }
                    set_adv_enable(1);
                } else {
                    log_info("no open adv\n");
                }
            }
#else
            if (packet[5] == 0x08) {
                //超时断开,检测是否配对广播
                ble_check_need_pair_adv();
            }
            set_adv_enable(1);
#endif
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("HCI_EVENT_VENDOR_REMOTE_TEST");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("update_rsp: %02x", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!");
                check_connetion_updata_deal();
            } else {
                connection_update_waiting = 1;
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d", packet[2]);
            if (!packet[2]) {
                if (first_pair_flag) {
                    //只在配对时启动检查
                    att_server_set_check_remote(con_handle, att_check_remote_result);
#if PAIR_DIREDT_ADV_EN
                    conn_pair_info.pair_flag = 1;
                    conn_pair_vm_do(&conn_pair_info, 1);
#endif
                } else {
                    if ((!conn_pair_info.pair_flag) || memcmp(cur_peer_addr_info, conn_pair_info.peer_address_info, 7)) {
                        log_info("record reconnect peer");
                        put_buf(cur_peer_addr_info, 7);
                        put_buf(conn_pair_info.peer_address_info, 7);
                        memcpy(conn_pair_info.peer_address_info, cur_peer_addr_info, 7);
                        conn_pair_info.pair_flag = 1;
                        conn_pair_vm_do(&conn_pair_info, 1);
                    }
                    resume_all_ccc_enable(1);

                    //回连时,从配对表中获取
                    u8 tmp_buf[6];
                    u8 remote_type = 0;
                    swapX(&cur_peer_addr_info[1], tmp_buf, 6);
                    ble_list_get_remote_type(tmp_buf, cur_peer_addr_info[0], &remote_type);
                    log_info("list's remote_type:%d", remote_type);
                    att_check_remote_result(con_handle, remote_type);
                }
                check_report_map_change(con_handle);
                ble_state_to_user(BLE_PRIV_PAIR_ENCRYPTION_CHANGE, first_pair_flag);
            }
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

//主机操作ATT read,协议栈回调处理
static uint16_t le_hogp_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x", handle, (u32)buffer);

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
        att_value_len = sizeof(Appearance);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Appearance[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a04_01_VALUE_HANDLE:
        att_value_len = 8;//fixed
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            log_info("get Peripheral_Preferred_Connection_Parameters");
            memcpy(buffer, &Peripheral_Preferred_Connection_Parameters[0], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a29_01_VALUE_HANDLE:
        att_value_len = strlen(Manufacturer_Name_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Manufacturer_Name_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a24_01_VALUE_HANDLE:
        att_value_len = strlen(Model_Number_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Model_Number_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a25_01_VALUE_HANDLE:
        att_value_len = strlen(Serial_Number_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Serial_Number_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a27_01_VALUE_HANDLE:
        att_value_len = strlen(Hardware_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Hardware_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a26_01_VALUE_HANDLE:
        att_value_len = strlen(Firmware_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Firmware_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a28_01_VALUE_HANDLE:
        att_value_len = strlen(Software_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Software_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a23_01_VALUE_HANDLE:
        att_value_len = sizeof(System_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &System_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a50_01_VALUE_HANDLE:
        log_info("read PnP_ID");
        att_value_len = sizeof(PnP_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &PnP_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a19_01_VALUE_HANDLE:
        att_value_len = 1;
        if (buffer) {
            buffer[0] = hid_battery_level;
        }
        break;

    case ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE:
        att_value_len = HID_REPORT_MAP_SIZE;
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &HID_REPORT_MAP_DATA[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a4a_01_VALUE_HANDLE:
        att_value_len = sizeof(hid_information);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &hid_information[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
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

/* LISTING_END */
/*
 * @section ATT Write
 *
 * @text The only valid ATT write in this example is to the Client Characteristic Configuration, which configures notification
 * and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent. See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
//主机操作ATT write,协议栈回调处理
static int le_hogp_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 handle = att_handle;

    log_info("write_callback, handle= 0x%04x,size = %d", handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_2a4d_03_VALUE_HANDLE:
        put_buf(buffer, buffer_size);           //键盘led灯状态
        if (le_hogp_output_callback) {
            le_hogp_output_callback(buffer, buffer_size);
        }
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        if (buffer[0]) {
            check_report_map_change(connection_handle);
        }
        break;

    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE:
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE, 0);
        ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_MIN); //
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
        if ((cur_conn_latency == 0)
            && (connection_update_waiting == 0)
            && (connection_update_cnt == CONN_PARAM_TABLE_CNT)
            && (Peripheral_Preferred_Connection_Parameters[0].latency != 0)) {
            connection_update_cnt = 0;//update again
        }
        check_connetion_updata_deal();
        log_info("write ccc:%04x, %02x", handle, buffer[0]);
        multi_att_set_ccc_config(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_ae41_01_VALUE_HANDLE:
        ble_hid_transfer_channel_recieve(buffer, buffer_size);
        break;

    default:
        break;
    }
    return 0;
}

//att handle操作，notify or indicate
static int app_send_user_data(u16 handle, u8 *data, u16 len, att_op_type_e handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle || !le_hogp_ble_hdl) {
        return APP_BLE_OPERATION_ERROR;
    }

    putchar('@');
    /* put_buf(data, len); */

    ret = app_ble_att_send_data(le_hogp_ble_hdl, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_error("app_send_fail:%d !!!", ret);
    }

    return ret;
}

//------------------------------------------------------
static u8 adv_name_ok = 0; //名字优先放入adv包

static int make_set_adv_data(u8 *buf)
{
    u8 offset = 0;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, HID_UUID_16, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_APPEARANCE_DATA, Appearance, 2);

    //check set name is ok?
    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);

    if (name_len <= vaild_len) {
        adv_name_ok = 1;
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);
        log_info("name in adv_data");
    } else {
        adv_name_ok = 0;
    }

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

    if (!adv_name_ok) {
        u8 name_len = gap_device_name_len;
        u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
        if (name_len > vaild_len) {
            name_len = vaild_len;
        }
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);
        log_info("name in rsp_data");
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("***rsp_data overflow!!!!!!");
        return 0;
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
    uint16_t adv_interval_val = ADV_INTERVAL_MIN;

    len = make_set_adv_data(advData);
    if (len) {
        app_ble_adv_data_set(le_hogp_ble_hdl, advData, len);
    }
    len = make_set_rsp_data(rspData);
    if (len) {
        app_ble_rsp_data_set(le_hogp_ble_hdl, rspData, len);
    }

    if (conn_pair_info.pair_flag && conn_pair_info.direct_adv_flag && conn_pair_info.direct_adv_cnt) {
        log_info(">>>direct_adv......");
        adv_type = ADV_DIRECT_IND;
        conn_pair_info.direct_adv_cnt--;
        app_ble_set_adv_param(le_hogp_ble_hdl, adv_interval_val, adv_type, adv_channel);
        app_ble_set_adv_peer_param(le_hogp_ble_hdl, conn_pair_info.peer_address_info[0], &conn_pair_info.peer_address_info[1]);
    } else {
        app_ble_set_adv_param(le_hogp_ble_hdl, adv_interval_val, adv_type, adv_channel);
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
    log_info("set new_key= %06u", *key);
#else
    *key = 123456; //for debug
#endif
}

static int set_adv_enable(u32 en)
{
    ble_state_e next_state, cur_state;

    if (!le_hogp_ble_hdl) {
        return -1;
    }

    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
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
    set_ble_work_state(next_state, 0);
    if (en) {
        advertisements_setup_init();
    }
    app_ble_adv_enable(le_hogp_ble_hdl, en);

    return APP_BLE_NO_ERROR;
}

int le_hogp_adv_enable(u8 enable)
{
    if (!le_hogp_ble_hdl) {
        return -1;
    }

    if (enable == app_ble_adv_state_get(le_hogp_ble_hdl)) {
        return 0;
    }

    if (enable) {
        ble_check_need_pair_adv();
    }

    return set_adv_enable(enable);
}

void le_hogp_all_init(void)
{
    u8 ble_mac_addr[6];

    log_info("le_hogp_all_init");

    if (le_hogp_ble_hdl == NULL) {
        ble_hid_set_config();
        con_handle = 0;
        gap_device_name_len = strlen(gap_device_name);
        memset(&conn_pair_info, 0, sizeof(struct pair_info_t));
        conn_pair_vm_do(&conn_pair_info, 0);
        set_ble_work_state(BLE_ST_INIT_OK, 0);

        le_hogp_ble_hdl = app_ble_hdl_alloc();
        if (le_hogp_ble_hdl == NULL) {
            log_error("le_hogp_ble_hdl alloc err !");
            return;
        }
        bt_make_ble_address(ble_mac_addr, (u8 *)bt_get_mac_addr());
        app_ble_set_mac_addr(le_hogp_ble_hdl, (void *)ble_mac_addr);
        app_ble_profile_set(le_hogp_ble_hdl, le_hogp_profile_data);
        app_ble_att_read_callback_register(le_hogp_ble_hdl, le_hogp_att_read_callback);
        app_ble_att_write_callback_register(le_hogp_ble_hdl, le_hogp_att_write_callback);
        app_ble_att_server_packet_handler_register(le_hogp_ble_hdl, le_hogp_cbk_packet_handler);
        app_ble_hci_event_callback_register(le_hogp_ble_hdl, le_hogp_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(le_hogp_ble_hdl, le_hogp_cbk_packet_handler);
        app_ble_sm_event_callback_register(le_hogp_ble_hdl, le_hogp_cbk_sm_packet_handler);
#if PASSKEY_ENTER_ENABLE
        reset_PK_cb_register(reset_passkey_cb);
#endif
        le_hogp_adv_enable(1);
    }
}

void le_hogp_all_exit(void)
{
    log_info("le_hogp_all_exit");

    // BLE exit
    if (app_ble_get_hdl_con_handle(le_hogp_ble_hdl)) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            set_ble_work_state(BLE_ST_SEND_DISCONN, 0);
        }
        ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_KEEP);
        app_ble_disconnect(le_hogp_ble_hdl);
    }
    le_hogp_adv_enable(0);
    app_ble_hdl_free(le_hogp_ble_hdl);
    le_hogp_ble_hdl = NULL;
}

void le_hogp_modify_ble_name(const char *name)
{
    if (strlen(name) <= BT_NAME_LEN_MAX) {
        memset(gap_device_name, 0x0, BT_NAME_LEN_MAX);
        memcpy(gap_device_name, name, strlen(name));
        gap_device_name_len = strlen(gap_device_name);
        log_info("modify_ble_name: %s", gap_device_name);
    }
}

void le_hogp_set_icon(u16 class_type)
{
    memcpy(Appearance, &class_type, 2);
}

void le_hogp_set_ReportMap(const u8 *map, u16 size)
{
    report_map = map;
    report_map_size = size;
    hid_report_change = 1;
}

void le_hogp_set_output_callback(void *cb)
{
    le_hogp_output_callback = cb;
}

//配对绑定配置
void le_hogp_set_pair_config(u8 pair_max, u8 is_allow_cover)
{
    ble_list_config_reset(pair_max, is_allow_cover); //开1对1配对绑定
}

//开可配对绑定允许
void le_hogp_set_pair_allow(void)
{
    conn_pair_info.pair_flag = 0;
    conn_pair_vm_do(&conn_pair_info, 1);
    ble_list_clear_all();
}

u8 *le_hogp_cur_connect_addrinfo(void)
{
    return cur_peer_addr_info;
}

bool le_hogp_is_connect(void)
{
    return (ble_work_state == BLE_ST_NOTIFY_IDICATE ? 1 : 0);
}

static const u16 report_id_handle_table[] = {
    0,
    HID_REPORT_ID_01_SEND_HANDLE,
    HID_REPORT_ID_02_SEND_HANDLE,
    HID_REPORT_ID_03_SEND_HANDLE,
    HID_REPORT_ID_04_SEND_HANDLE,
    HID_REPORT_ID_05_SEND_HANDLE,
    HID_REPORT_ID_06_SEND_HANDLE,
};

int ble_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    if (report_id == 0 || report_id > 6) {
        log_info("report_id %d,err!!!", report_id);
        return -1;
    }
    return app_send_user_data(report_id_handle_table[report_id], data, len, ATT_OP_AUTO_READ_CCC);
}

void ble_hid_key_deal_test(u16 key_msg)
{
    if (key_msg) {
        ble_hid_data_send(1, (u8 *)&key_msg, 2);
        key_msg = 0;//key release
        ble_hid_data_send(1, (u8 *)&key_msg, 2);
    }
}

int ble_hid_transfer_channel_send(u8 *packet, u16 size)
{
    /* log_info("transfer_tx(%d):", size); */
    /* log_info_hexdump(packet, size); */
    return app_send_user_data(ATT_CHARACTERISTIC_ae42_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
}

void __attribute__((weak)) ble_hid_transfer_channel_recieve(u8 *packet, u16 size)
{
    log_info("transfer_rx(%d):", size);
    log_info_hexdump(packet, size);
    //ble_hid_transfer_channel_send(packet,size);//for test
}

//控制配对模式
void ble_set_pair_list_control(u8 mode)
{
    bool ret = 0;
    u8 connect_address[6];

    switch (mode) {
    case 0:
        //close pair,关配对
        ret = ble_list_pair_accept(0);
        break;
    case 1:
        //open pair,开配对
        ret = ble_list_pair_accept(1);
        break;
    case 2:
        //绑定已配对设备，不再接受新的配对
        swapX(&cur_peer_addr_info[1], connect_address, 6);
        //bond current's device
        ret = ble_list_bonding_remote(connect_address, cur_peer_addr_info[0]);
        //close pair
        ble_list_pair_accept(0);
        break;
    default:
        break;
    }

    log_info("%s: %02x,ret=%02x", __FUNCTION__, mode, ret);
}


static const u8 hid_descriptor_keyboard_boot_mode[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

//----------------------------------
#define KEYBOARD_REPORT_MAP \
	USAGE_PAGE(CONSUMER_PAGE),              \
	USAGE(CONSUMER_CONTROL),                \
	COLLECTION(APPLICATION),                \
	REPORT_ID(1),      \
	USAGE(VOLUME_INC),                  \
	USAGE(VOLUME_DEC),                  \
	USAGE(PLAY_PAUSE),                  \
	USAGE(MUTE),                        \
	USAGE(SCAN_PREV_TRACK),             \
	USAGE(SCAN_NEXT_TRACK),             \
	USAGE(FAST_FORWARD),                \
	USAGE(REWIND),                      \
	LOGICAL_MIN(0),                     \
	LOGICAL_MAX(1),                     \
	REPORT_SIZE(1),                     \
	REPORT_COUNT(16),                   \
	INPUT(0x02),                        \
	END_COLLECTION,                         \

static const u8 hid_report_map[] = {KEYBOARD_REPORT_MAP};

static void ble_hid_set_config(void)
{
    le_hogp_set_icon(0x03c1);//ble keyboard demo
    le_hogp_set_ReportMap(hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode));
}

#endif

