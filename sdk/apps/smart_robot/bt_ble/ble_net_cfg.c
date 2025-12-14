/*********************************************************************************************
    *   Filename        : le_server_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

#include "system/app_core.h"
#include "system/includes.h"
#include "event/net_event.h"
#include "app_config.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "system/wait.h"
#include "third_party/common/ble_user.h"
#include "le_common.h"
#include "app_ble.h"

#ifdef TCFG_DEF_BLE_NET_CFG

#include "ble_net_cfg.h"
#include "app_wifi.h"


#define LOG_TAG             "[BLE_NC]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
//#define LOG_DEBUG_ENABLE
//#define LOG_DUMP_ENABLE
//#define LOG_CHAR_ENABLE
#include "system/debug.h"


#define ACTION_CMD_GET_UNIQUE_CODE   0x00
#define ACTION_CMD_GET_VENDOR_UID    0x01
#define ACTION_CMD_GET_PRODUCT_TYPE  0x02
#define ACTION_CMD_START_SCAN_WIFI   0x10
#define ACTION_CMD_GET_WIFI_COUNT    0x11
#define ACTION_CMD_CONFIRM_WIFI      0x20
#define ACTION_CMD_WIFI_STATUS       0x21

typedef struct {

    u8 get_product_type_result;
    u8 get_unique_code_result;
    u8 unique_code[33];
    u8 get_vendor_uid_result;
    u8 vendor_uid[32];
    u32 wifi_list_cnt;
    u8 wifi_list[10][36]; //最多缓存10个记录, length + rssi + auth mode + ssid, 第1个字节保存总长度, 第2个字节保存rssi, 第3个字节保存auth mode
    u8 ssid[33];
    u8 pwd[64];
    u8 last_action_result; //上次操作结果, 1-失败, 0-成功
    u8 last_cmd; //上次操作的命令
} net_cfg_status_t;


static net_cfg_status_t g_net_cfg_status = {0};

//------
#define ATT_LOCAL_PAYLOAD_SIZE    (128)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------

//---------------
#define ADV_INTERVAL_MIN          (160)

static volatile hci_con_handle_t con_handle;
static void ble_module_enable(u8 en);

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




#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

#define adv_data       &att_ram_buffer[0]
#define scan_rsp_data  &att_ram_buffer[32]

static char gap_device_name[BT_NAME_LEN_MAX] = "TY-AC79NN"; //默认的蓝牙名字
static u8 complete = 0;
static u8 gap_device_name_len = 0;
static u8 ble_work_state = 0;
static u8 adv_ctrl_en;
static OS_MUTEX mutex;
static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, const u8 *data, u16 len, u8 handle_type);

// Complete Local Name  默认的蓝牙名字
extern const char *bt_get_local_name();

/********************************************/
#define WIFI_CONN_CNT   100   //延迟 ( x )*(20/100) s  等待wifi连接
#define DEFAULT_SEND_MAX_LEN	18		//数据包默认最大包长度 >= 6

#define DEFAULT_SSID_MAX_LEN		32		//默认ssid最大长度
#define DEFAULT_PWD_MAX_LEN			64		//默认pwd最大长度
#define DEFAULT_TOKEN_LEN			32		//默认token长度


//1字节\0
static u8 save_token_info[DEFAULT_TOKEN_LEN + 1] = {0};	//存放token信息

static int send_max_length;	//一次发送最长的长度
static int BTCombo_flag = 0;		//配网步骤标志位

enum {
    BTCOMBO_RECEIVE = 1,
    SSID_RECEIVE,
    PWD_RECEIVE,
    CONNECT_REQUEST,
    TOKEN_RECEIVE,
    ERROR_OCCURRED,
};
/*******************************************/
//------------------------------------------------------


#if (defined CONFIG_QR_CODE_NET_CFG) || (defined CONFIG_ACOUSTIC_COMMUNICATION_ENABLE)

static u16 user_buf_offset = 0, user_data_size = 0;
static u8 user_buf[128];

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



static void check_net_info_if_recv_complete(void)
{
    if (user_buf[user_buf_offset - 1] == 0xFF && user_buf_offset == user_data_size + 7) {
        user_buf[sizeof(user_buf) - 1] = 0;
        extern int bt_net_config_set_ssid_pwd(const char *data);
        extern u16 CRC16(const void *ptr, u32 len);
        u16 crc16 = user_buf[user_buf_offset - 2] + (user_buf[user_buf_offset - 3] << 8);
        put_buf(&user_buf[2], user_data_size + 2);
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


#endif





static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
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

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}

static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
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
            log_info("Just Works Confirmed.\n");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;
        }
        break;
    }
}

int net_config_done_reset(void){
    log_info("reset system after net config success to shutdown the bluetooth etc.\n");
    system_soft_reset();
    return 0;
}

static void can_send_now_wakeup(void)
{
    putchar('E');
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }
}

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void server_profile_start(u16 con_handle)
{
    ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
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
    u8 sub_type = hci_event_packet_get_type(packet);
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (sub_type) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                status = hci_subevent_le_enhanced_connection_complete_get_status(packet);
                if (status) {
                    log_info("LE_SLAVE CONNECTION FAIL!!! %0x\n", status);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : %0x\n", con_handle);
                log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                server_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                connection_update_complete_success(packet + 8);
                server_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
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
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            con_handle = 0;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);
            if (check_if_in_config_network_state()) 
            {
                log_info("re-start adv for config network\n");
                os_mutex_pend(&mutex, 0);
                bt_ble_adv_enable(1);
                complete = 0;
                os_mutex_post(&mutex);
            }
            connection_update_cnt = 0;
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %d\n", mtu);
            send_max_length = mtu;	//yii:如果手机端有做mtu交换，则更新下最大包长度
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, mtu);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
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
int a = 30;
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);
    switch (handle) {
    case ATT_CHARACTERISTIC_GET_DEVICE_INFO_HANDLE:
        if (g_net_cfg_status.last_cmd == ACTION_CMD_GET_UNIQUE_CODE && g_net_cfg_status.get_unique_code_result == 0) {
            int msg_len = sizeof(g_net_cfg_status.unique_code);
            if (buffer == NULL) {
                return 33;
            }
            if (offset >= msg_len) {
                break;
            }

            memcpy(buffer, g_net_cfg_status.unique_code, strnlen(g_net_cfg_status.unique_code, sizeof(g_net_cfg_status.unique_code) - 1));
            g_net_cfg_status.get_unique_code_result = 1; //只读一次
            return buffer_size;
        }else if (g_net_cfg_status.last_cmd == ACTION_CMD_GET_VENDOR_UID && g_net_cfg_status.get_vendor_uid_result == 0) {
            int msg_len = strlen(g_net_cfg_status.vendor_uid);
            if (buffer == NULL) {
                return msg_len;
            }
            if (offset >= msg_len) {
                break;
            }
            memcpy(buffer, &g_net_cfg_status.vendor_uid[offset], strlen(g_net_cfg_status.vendor_uid) - offset);
            g_net_cfg_status.get_vendor_uid_result = 1; //只读一次
            return buffer_size;
        }else if (g_net_cfg_status.last_cmd == ACTION_CMD_GET_PRODUCT_TYPE && g_net_cfg_status.get_product_type_result == 0) {
            char* type = PRODUCT_TYPE;
            int msg_len = strlen(type);
            if (buffer == NULL) {
                return msg_len;
            }
            if (offset >= msg_len) {
                break;
            }
            
            memcpy(buffer, &type[offset], strlen(type) - offset);
            g_net_cfg_status.get_product_type_result = 1; //只读一次
            return buffer_size;
        }else{
            log_info("err, last cmd: %d, get_unique_code_result: %d, get_vendor_uid_result: %d\n", 
                g_net_cfg_status.last_cmd, g_net_cfg_status.get_unique_code_result, g_net_cfg_status.get_vendor_uid_result );
        }
        break;      
    case ATT_CHARACTERISTIC_ACTION_CMD_HANDLE:
        if (buffer == NULL) {
            return 1;
        }
        if (ACTION_CMD_CONFIRM_WIFI == g_net_cfg_status.last_cmd || ACTION_CMD_START_SCAN_WIFI == g_net_cfg_status.last_cmd) {   
            *buffer = g_net_cfg_status.last_action_result;
            g_net_cfg_status.last_action_result = 1; //只读一次
        }
        else if ( ACTION_CMD_WIFI_STATUS == g_net_cfg_status.last_cmd ){
            u8 ret = (u8)check_wifi_connected();
            log_info("check_wifi_connected ret= %d\n", ret);
            
            *buffer = ret;
            if(ret == 0){
                sys_timeout_add_to_task("sys_timer", NULL, net_config_done_reset, 3000);
            }
        }
        return 1;
        break;
    
    case ATT_CHARACTERISTIC_WIFI_SSID_HANDLE:
    case ATT_CHARACTERISTIC_WIFI_PWD_HANDLE:
        if (buffer == NULL) {
            return 1;
        }
        *buffer = g_net_cfg_status.last_action_result; 
        g_net_cfg_status.last_action_result = 1; //只读一次
        return 1;
        break;
    case ATT_CHARACTERISTIC_WIFI_CNT_HANDLE:
        if (buffer == NULL) {
            return 1;
        }
        *buffer = (u8)g_net_cfg_status.wifi_list_cnt;
        return 1;
        break;
    case ATT_CHARACTERISTIC_FETCH_WIFI_LIST_HANDLE:
        if (g_net_cfg_status.wifi_list_cnt > 0) {
            int msg_len = 0;
            for(int i = 0; i < g_net_cfg_status.wifi_list_cnt; i++) {
                msg_len += g_net_cfg_status.wifi_list[i][0];
            }
            if (buffer == NULL) {
                log_info("wifi list total len: %d", msg_len);
                return msg_len;
            }
            
            if (offset >= msg_len) {
                break;
            }
            
            // 找到起始位置对应的记录
            int current_pos = 0; 
            int entry_index = 0;
            while (entry_index < g_net_cfg_status.wifi_list_cnt) {
                int entry_len = g_net_cfg_status.wifi_list[entry_index][0];
                if (current_pos + entry_len > offset) {
                    break;  // 找到起始记录
                }
                current_pos += entry_len;
                entry_index++;
            }

            // 开始复制数据
            int bytes_to_copy = 0;
            int buffer_offset = 0;
            while (entry_index < g_net_cfg_status.wifi_list_cnt && buffer_offset < buffer_size) {
                int entry_len = g_net_cfg_status.wifi_list[entry_index][0];
                int entry_offset = (entry_index == 0) ? (offset - current_pos) : 0;
                
                bytes_to_copy = entry_len - entry_offset;
                if (bytes_to_copy > (buffer_size - buffer_offset)) {
                    
                    if (entry_index == 0) {
                        bytes_to_copy = buffer_size - buffer_offset;
                        memcpy(buffer + buffer_offset, 
                            &g_net_cfg_status.wifi_list[entry_index][entry_offset], 
                            bytes_to_copy);
                        buffer_offset += bytes_to_copy;
                        break; // 起始记录可以部分复制，复制后停止
                    }else{
                        break; // 剩余空间不足以容纳整个记录，停止复制
                    }                    
                }

                if (bytes_to_copy > 0) {
                    
                    memcpy(buffer + buffer_offset, 
                        &g_net_cfg_status.wifi_list[entry_index][entry_offset], 
                        bytes_to_copy);
                    buffer_offset += bytes_to_copy;
                }

                entry_index++;
            }

            return buffer_offset;  // 返回实际复制的字节数
        }
        break;    
    default:
        break;
    }

    return att_value_len;
}

void set_default_length()
{
    send_max_length = DEFAULT_SEND_MAX_LEN;
}


void send_reply_token(char *para_format, int len)
{
    u8 send_buf[64] = {0};
    int seq = 0;
    int offset = 0;
    char ready_send[128] = {0};
    strncpy(ready_send, para_format, len);
    int remain_len = len;
    int send_len = send_max_length - 6;

    /* printf("buf = %s  ---%lu", ready_send, strlen(ready_send)); */
    /* put_buf((const unsigned char *)ready_send, strlen(ready_send)); */

    while (remain_len > send_len) {
        memset(send_buf, 0, sizeof(send_buf));
        send_buf[0] = 0x15;                 //0x15 用户发送token 或者接收 token绑定状态信息
        send_buf[1] = 0x10 | 0x04;          //0x14  0x10分片 0x04 设备发数
        send_buf[2] = seq++;                //序列号,每次加1
        send_buf[3] = send_len + 2;         //包含剩余数据长度在内2byte的数据长度
        send_buf[4] = remain_len % 0xff;    //剩余数据长度低位
        send_buf[5] = remain_len / 0xff;    //剩余数据长度高位

        memcpy(&send_buf[6], ready_send + offset, send_len);
        //-- app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, send_buf, send_len + 6, ATT_OP_AUTO_READ_CCC);

        remain_len  -= send_len;
        offset      += send_len;
        printf("-------remain len  %d     offset = %d", remain_len, offset);
        put_buf(send_buf, send_len + 6);
    }
    memset(send_buf, 0, sizeof(send_buf));
    send_buf[0] = 0x15;
    send_buf[1] = 0x00 | 0x04;              //0x04
    send_buf[2] = seq++;
    send_buf[3] = remain_len;

    memcpy(&send_buf[4], ready_send + offset, remain_len); //15 04 01 0E 41 00
    //app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, send_buf, remain_len + 4, ATT_OP_AUTO_READ_CCC);
    put_buf(send_buf, remain_len + 4);

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
    int result = 0;
    u16 handle = att_handle;
    log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);
    if (buffer && buffer_size) {
        log_info_hexdump(buffer, buffer_size);
    }
    u8 action_cmd = 0;
    switch (handle) {

    case ATT_CHARACTERISTIC_GET_DEVICE_INFO_HANDLE:
        if (buffer_size != 1) {
            log_info("err: action cmd len err: %d\n", buffer_size);
            break;
        }
        action_cmd = buffer[0];
        g_net_cfg_status.last_cmd = action_cmd;
        if (action_cmd == ACTION_CMD_GET_UNIQUE_CODE) {
            strncpy((char *)g_net_cfg_status.unique_code, get_device_unique_code(), sizeof(g_net_cfg_status.unique_code)-1);
            g_net_cfg_status.get_unique_code_result= 0;
            break;
        }
        else if (action_cmd == ACTION_CMD_GET_VENDOR_UID)
        {
            strcpy((char *)g_net_cfg_status.vendor_uid, PRODUCT_VENDOR_UID);
            g_net_cfg_status.get_vendor_uid_result = 0;
            log_info("get vendor uid: %s\n", g_net_cfg_status.vendor_uid);
            break;
        }
        else if (action_cmd == ACTION_CMD_GET_PRODUCT_TYPE)
        {
            g_net_cfg_status.get_product_type_result= 0;
            log_info("get product type: %s\n", PRODUCT_TYPE);
            break;
        }
        
        break;

    case ATT_CHARACTERISTIC_ACTION_CMD_HANDLE:
        if (buffer_size != 1) {
            log_info("err: action cmd len err: %d\n", buffer_size);
            break;
        }
        action_cmd = buffer[0];
        g_net_cfg_status.last_cmd = action_cmd;
        switch (action_cmd) {
        case ACTION_CMD_START_SCAN_WIFI:
            g_net_cfg_status.wifi_list_cnt = 0;
            int ret = start_scan_wifi();
            if (ret == -1 || ret ==0) {
                g_net_cfg_status.last_action_result = 0;
                
            }else{
                g_net_cfg_status.last_action_result = ret;
            }
            break;

        case ACTION_CMD_CONFIRM_WIFI:
            if (g_net_cfg_status.ssid[0] == 0 || g_net_cfg_status.ssid[0] == 0) {
                log_info("err: ssid or password is empty\n");
                g_net_cfg_status.last_action_result = 1;
                break;
            }
            config_wifi_param(g_net_cfg_status.ssid, g_net_cfg_status.pwd);
            comfirm_wifi_param();
            g_net_cfg_status.last_action_result = 0;
            break;
        default:
            break;
        }
        break;

    case ATT_CHARACTERISTIC_WIFI_CNT_HANDLE:
        if (buffer_size != 1) {
            log_info("err: action cmd len err: %d\n", buffer_size);
            break;
        }
        u8 action_cmd = buffer[0];
        if (action_cmd != ACTION_CMD_GET_WIFI_COUNT) 
        {
            log_info("err: action cmd err: %d\n", action_cmd);
            break;
        }
        struct wifi_scan_ssid_info * wifi_list = get_wifi_scan_result(&g_net_cfg_status.wifi_list_cnt);
        
        if (wifi_list && g_net_cfg_status.wifi_list_cnt > 0) {
            int i;
            for (i = 0; i < g_net_cfg_status.wifi_list_cnt && i < 10; i++) {
                g_net_cfg_status.wifi_list[i][0] = 3 + strnlen((const char *)wifi_list[i].ssid, 32); //length
                g_net_cfg_status.wifi_list[i][1] = wifi_list[i].rssi;
                g_net_cfg_status.wifi_list[i][2] = wifi_list[i].auth_mode;
                strncpy((char *)&g_net_cfg_status.wifi_list[i][3], wifi_list[i].ssid, 32);
            }
        }   
        break;
    case ATT_CHARACTERISTIC_FETCH_WIFI_LIST_HANDLE:
        if (buffer_size < 2) {
            log_info("err: fetch wifi list len err: %d\n", buffer_size);
            break;
        }
        break;
    case ATT_CHARACTERISTIC_WIFI_SSID_HANDLE:
        if (buffer_size < 1 || buffer_size > DEFAULT_SSID_MAX_LEN + 1) {
            log_info("err: ssid len err: %d\n", buffer_size);
            g_net_cfg_status.last_action_result = 1;
            break;
        }
        int ssid_size = sizeof(g_net_cfg_status.ssid) - 1;
        memset(g_net_cfg_status.ssid, 0, sizeof(g_net_cfg_status.ssid));
        memcpy(g_net_cfg_status.ssid, buffer, buffer_size > ssid_size ? ssid_size : buffer_size);
        g_net_cfg_status.last_action_result = 0;
        log_info("set ssid: %s\n", &g_net_cfg_status.ssid);
        
        break;
    
    case ATT_CHARACTERISTIC_WIFI_PWD_HANDLE:
        if (buffer_size < 1 || buffer_size > DEFAULT_PWD_MAX_LEN + 1) {
            g_net_cfg_status.last_action_result = 1;
            break;
        }
        int pwd_size = sizeof(g_net_cfg_status.pwd) - 1;
        memset(g_net_cfg_status.pwd, 0, sizeof(g_net_cfg_status.pwd));
        memcpy(g_net_cfg_status.pwd, buffer, buffer_size > pwd_size ? pwd_size : buffer_size);
        g_net_cfg_status.last_action_result = 0;
        
        break;    
    default:
        break;
    }

    return 0;
}

static int app_send_user_data(u16 handle, const u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    os_mutex_pend(&mutex, 0);

    if (!att_get_ccc_config(handle + 1)) {
        log_info("fail,no write ccc,%04x\n", handle + 1);
        os_mutex_post(&mutex);
        return APP_BLE_NO_WRITE_CCC;
    }

    ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    os_mutex_post(&mutex);

    if (ret) {
        log_info("app_send_fail:%d \n", ret);
    }
    return ret;
}

//------------------------------------------------------
static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);

    /**
     * 杰理的芯片的蓝牙advertisServiceUUID统一为 3F6D7A84-2C4A-4C5E-9B2A-7D38B2A0F7C1, APP端根据这个UUID识别杰理的设备
     */
     u8 uuid_128[16] = {0xC1, 0xF7, 0xA0, 0xB2, 0x38, 0x7D, 0x2A, 0x9B,
                        0x5E, 0x4C, 0x4A, 0x2C, 0x84, 0x7A, 0x6D, 0x3F
     };//128bits 倒序排列
     offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS, uuid_128, sizeof(uuid_128));

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
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
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, offset, buf);
    return 0;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, ADV_INTERVAL_MIN, adv_type, adv_channel);

    ret |= make_set_adv_data();
    ret |= make_set_rsp_data();

    if (ret) {
        log_info("advertisements_setup_init fail");
        return;
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
    log_info("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}

extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
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

extern void le_device_db_init(void);
void ble_profile_init(void)
{
    log_info("ble profile init\n");
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
    log_info("adv_en:%d\n", en);
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
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_user_cmd_prepare(BLE_CMD_ATT_VAILD_LEN, 1, &vaild_len);
    return vaild_len;
}

static int app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return 0; 
}

static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}

static int regiest_wakeup_send(void *priv, void *cbk)
{
    ble_resume_send_wakeup = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

static void ble_module_enable(u8 en)
{
    os_mutex_pend(&mutex, 0);
    log_info("mode_en:%d\n", en);
    if (en) {
        extern u8 get_ble_gatt_role(void);
        if (1 == get_ble_gatt_role()) {
            ble_stack_gatt_role(0);
            // register for HCI events
            hci_event_callback_set(&cbk_packet_handler);
            le_l2cap_register_packet_handler(&cbk_packet_handler);
        }
        complete = 0;
        adv_ctrl_en = 1;
        bt_ble_adv_enable(1);
    } else {
        if (con_handle) {
            if (complete) {
                os_mutex_post(&mutex);
                return;
            }
            adv_ctrl_en = 0;
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
        complete = 0;
    }
    os_mutex_post(&mutex);
}

//static const char ble_ext_name[] = "(BLE)";

void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    //const char *name_p = bt_get_local_name();
    //u8 ext_name_len = sizeof(ble_ext_name) - 1;

    if (!os_mutex_valid(&mutex)) {
        os_mutex_create(&mutex);
    }

    gap_device_name_len = strlen(gap_device_name);
    //if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) {
    //    gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    //}

    //增加后缀，区分名字
    //memcpy(gap_device_name, name_p, gap_device_name_len);
    //memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);
    //gap_device_name_len += ext_name_len;

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);
    BTCombo_flag = 0;	//初始化清空标志位
}

static void stack_exit(void)
{
    ble_module_enable(0);
    /* set_ble_work_state(BLE_ST_SEND_STACK_EXIT); */
    /* ble_user_cmd_prepare(BLE_CMD_STACK_EXIT, 1, 0); */
    /* set_ble_work_state(BLE_ST_STACK_EXIT_COMPLETE); */
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");

    stack_exit();
}

void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)app_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};

void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}

#endif


