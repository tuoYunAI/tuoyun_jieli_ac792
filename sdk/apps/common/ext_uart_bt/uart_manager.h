#ifndef _UART_MANAGER_COMMON_H_
#define _UART_MANAGER_COMMON_H_

#include "serial_packager.h"

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - 主蓝牙控制
 *************************************************************************************************/
typedef enum {
    PROTOCOL_TAG_SET_BT_ON_OFF           = 0x01,     /**< 开关蓝牙 */
    PROTOCOL_TAG_GET_BT_NAME             = 0x02,     /**< 获取蓝牙名 */
    PROTOCOL_TAG_SET_BT_NAME             = 0x03,     /**< 设置蓝牙名 */
    PROTOCOL_TAG_GET_BT_MAC              = 0x04,     /**< 获取蓝牙MAC地址 */
    PROTOCOL_TAG_SET_BT_MAC              = 0x05,     /**< 设置蓝牙MAC */
    PROTOCOL_TAG_SEND_BT_STATUS          = 0x06,     /**< 发送蓝牙状态信息 */
    PROTOCOL_TAG_START_CONNEC            = 0x07,     /**< 开始蓝牙连接 */
    PROTOCOL_TAG_BT_MUSIC_PP             = 0x08,     /**< 控制音乐播放/暂停 */
    PROTOCOL_TAG_BT_MUSIC_PREV           = 0x09,     /**< 控制上一首歌曲 */
    PROTOCOL_TAG_BT_MUSIC_NEXT           = 0x0A,     /**< 控制下一首歌曲 */
    PROTOCOL_TAG_BT_MUSIC_STATUS_CHANGE  = 0x0B,     /**< 通知音乐状态变化 */
    PROTOCOL_TAG_BT_MUSIC_VOL_UP         = 0x0C,     /**< 增加音乐音量 */
    PROTOCOL_TAG_BT_MUSIC_VOL_DOWN       = 0x0D,     /**< 减少音乐音量 */
    PROTOCOL_TAG_BT_MUSIC_VOL_GET        = 0x0E,     /**< 获取当前音量 */
    PROTOCOL_TAG_BT_MUSIC_VOL_CHANGE     = 0x0F,     /**< 通知音量变化 */
    PROTOCOL_TAG_BT_REQ_PAIR             = 0x10,     /**< 请求配对 */
    PROTOCOL_TAG_BT_PAIR_RSP             = 0x11,     /**< 配对响应 */
    PROTOCOL_TAG_BT_GET_REMOTE_NAME      = 0x12,     /**< 获取远程名称 */
} InstrProtocolTag_e;

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - HFP通话控制
 *************************************************************************************************/
typedef enum {
    /* CMD */
    PROTOCOL_TAG_HFP_CALL_STATUS_CHANGE   = 0x01,   /**< 通话状态变化 */
    PROTOCOL_TAG_HFP_CALL_CUR_NUMBER      = 0x02,   /**< 当前通话号码 */
    PROTOCOL_TAG_HFP_CALL_CUR_NAME        = 0x03,   /**< 当前通话名称 */
    PROTOCOL_TAG_HFP_CALL_CTRL            = 0x04,   /**< 通话控制 */
    PROTOCOL_TAG_HFP_CALL_NUMBER          = 0x05,   /**< 通话号码 */
    PROTOCOL_TAG_HFP_CALL_LAST_NUMBER     = 0x06,   /**< 回拨上一个电话 */
    PROTOCOL_TAG_HFP_SECOND_CALL_STATUS   = 0x07,   /**< 第二个手机通话状态变化 */
    PROTOCOL_TAG_HFP_SECOND_CALL_CTRL     = 0x08,   /**< 第二个手机通话控制 */
} InstrProtocolTag_hfpcall;

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - PBAP电话簿访问
 *************************************************************************************************/
typedef enum {
    /* CMD */
    PROTOCOL_TAG_BT_PBAP_LIST_READ         = 0x01,  /**< 读取电话簿列表 */
    PROTOCOL_TAG_BT_PBAP_LIST_READ_RESP    = 0x02,  /**< 电话簿列表读取响应 */
    PROTOCOL_TAG_BT_PBAP_LIST_READ_END     = 0x03,  /**< 电话簿列表读取结束 */
} InstrProtocolTag_btpbap;

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - 蓝牙音乐信息
 *************************************************************************************************/
typedef enum {
    /* CMD */
    PROTOCOL_TAG_RCV_BT_MUSIC_NAME       = 0x01,     /**< 接收蓝牙音乐歌名 */
    PROTOCOL_TAG_RCV_BT_MUSIC_ARTIST     = 0x02,     /**< 接收蓝牙音乐艺术家 */
    PROTOCOL_TAG_RCV_BT_MUSIC_LYRIC      = 0x03,     /**< 接收蓝牙音乐歌词 */
    PROTOCOL_TAG_RCV_BT_MUSIC_ALBUM      = 0x04,     /**< 接收蓝牙音乐专辑 */
    PROTOCOL_TAG_RCV_BT_MUSIC_CUR_TIME   = 0x05,     /**< 接收蓝牙音乐当前播放时间 */
    PROTOCOL_TAG_RCV_BT_MUSIC_TOTLE_TIME = 0x06,     /**< 接收蓝牙音乐总时间 */
} InstrProtocolTag_btmusicinfo;

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - 蓝牙专辑传输
 *************************************************************************************************/
typedef enum {
    /* CMD */
    PROTOCOL_TAG_BT_ALBUM_START          = 0x01,     /**< 蓝牙专辑传输开始 */
    PROTOCOL_TAG_BT_ALBUM_TRANSFER_DATA  = 0x02,     /**< 蓝牙专辑数据传输 */
    PROTOCOL_TAG_BT_ALBUM_TRANSFER_DATA_END = 0x03,  /**< 蓝牙专辑数据传输结束 */
    PROTOCOL_TAG_BT_ALBUM_END            = 0x04,     /**< 蓝牙专辑传输结束 */
    PROTOCOL_TAG_BT_ALBUM_TERMINATE      = 0x05,     /**< 蓝牙专辑传输终止 */
} InstrProtocolTag_btalbun;

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - MCU控制
 *************************************************************************************************/
typedef enum {
    /* CMD */
    PROTOCOL_TAG_MCU_SHUTOFF               = 0x01,     /**< 关机 */
    PROTOCOL_TAG_MCU_POWERON_STATUS        = 0x02,     /**< 开机状态 */
    PROTOCOL_TAG_MCU_RF_POINT_CTRL         = 0x03,     /**< RF频点控制 */
} InstrProtocolTag_mcu_ctrl;

/*************************************************************************************************/
/**
 * @brief   指令协议标签枚举 - OTA升级控制
 *************************************************************************************************/
typedef enum {
    /* CMD */
    PROTOCOL_TAG_OTA_START               = 0x01,     /**< OTA升级开始 */
    PROTOCOL_TAG_OTA_DATA_TRANSFER       = 0x02,     /**< OTA数据传输 */
    PROTOCOL_TAG_OTA_DATA_TRANSFER_END   = 0x03,     /**< OTA数据传输结束 */
    PROTOCOL_TAG_OTA_END                 = 0x04,     /**< OTA升级结束 */
    PROTOCOL_TAG_OTA_TERMINATE           = 0x05,     /**< OTA升级暂停 */
    PROTOCOL_TAG_OTA_DATA_CAN_SEND       = 0x06,     /**< OTA数据可发送 */
    PROTOCOL_TAG_OTA_GET_VERSION         = 0xFF,     /**< 获取OTA版本号 */
} InstrProtocolTag_ota;

/*************************************************************************************************/
/**
 * @brief   操作码枚举
 *************************************************************************************************/
typedef enum {
    OP_CODE_DAT_CMD_CONCTRL              = 0x01,     /**< 数据命令包传输控制 */
    OP_CODE_BT_CONCTRL                   = 0x02,     /**< 经典蓝牙控制 */
    OP_CODE_BLE_CONCTRL                  = 0x03,     /**< 低功耗蓝牙控制 */
    OP_CODE_HFP_CALL_CONCTRL             = 0x04,     /**< 蓝牙通话控制 */
    OP_CODE_BT_PABP                      = 0x05,     /**< 电话簿访问协议控制 */
    OP_CODE_BT_MUSIC_INFO                = 0x06,     /**< 蓝牙音乐信息控制 */
    OP_CODE_BT_MUSIC_ALBUM               = 0x07,     /**< 蓝牙音乐专辑控制 */
    OP_CODE_BT_MCU_CONTROL               = 0xEF,     /**< MCU控制 */
    OP_CODE_OTA_CONCTRL                  = 0xF0,     /**< OTA升级控制 */
} op_code_t;

/*************************************************************************************************/
/**
 * @brief   蓝牙状态枚举
 *************************************************************************************************/
typedef enum {
    BT_STATE_SET_ON            = 0x01,     /**< 打开蓝牙 */
    BT_STATE_SET_OFF           = 0x02,     /**< 关闭蓝牙 */
} bt_status_t;

/*************************************************************************************************/
/**
 * @brief   自定义UART API接口结构体
 *************************************************************************************************/
typedef struct {
    void (*custom_uart_dev_init)(void);     /**< 串口初始化函数指针 */
    void (*custom_uart_send_cmd_handle)(u8 cmd, u8 sn, bt_cmd_type_t rsp_flag, u8 *data, u32 len);  /**< 发送命令处理函数指针 */
    void (*custom_uart_response_cmd_handle)(u8 cmd, u8 sn, bt_cmd_type_t rsp_flag, uint8_t cmd_stats, u8 *data, u32 len);  /**< 响应命令处理函数指针 */
    void (*custom_uart_send_data_handle)(u8 cmd, uint8_t sn, bt_data_type_t rsp_flag, uint8_t respon_code, uint8_t *buffer, uint32_t buffer_len);  /**< 发送数据处理函数指针 */
    void (*custom_uart_response_data_handle)(u8 cmd, uint8_t sn, bt_data_type_t rsp_flag, uint8_t cmd_stats, uint8_t rsp_opcode, uint8_t *buffer, uint32_t buffer_len);  /**< 响应数据处理函数指针 */
} custom_uart_api_t;

/* 外部变量声明 */
extern custom_uart_api_t custom_uart_api;

/* 宏定义 */
#define INTSR_TASK_UART_API (&custom_uart_api)  /**< UART API接口指针宏 */

#endif /* _UART_MANAGER_COMMON_H_ */

