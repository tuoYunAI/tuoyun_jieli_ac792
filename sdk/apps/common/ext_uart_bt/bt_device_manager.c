#include "system/includes.h"
#include "app_config.h"
#include "uart_manager.h"

#define LOG_TAG         "[BT_DEVICE_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "system/debug.h"

#if TCFG_INSTR_DEV_UART_ENABLE

// 信道范围：2402~2480 MHz，共79个信道
#define CHANNEL_COUNT 79
#define BITARRAY_SIZE ((CHANNEL_COUNT + 7) / 8)  // 计算需要的字节数
// 位数组类型定义
typedef uint8_t ChannelBitArray[BITARRAY_SIZE];
ChannelBitArray channels;

/**
 * @brief 蓝牙开关控制
 *
 * @param on 开关状态：0-关闭，非0-开启
 */
void bt_on_off_control(u8 on)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data[2] = {0};

    data[0] = PROTOCOL_TAG_SET_BT_ON_OFF;

    if (!on) {
        data[1] = BT_STATE_SET_OFF;
    } else {
        data[1] = BT_STATE_SET_ON;
    }

    if (INTSR_TASK_UART_API) {
        //经典蓝牙控制类下的蓝牙开关命令
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, sizeof(data));
    }
}

/**
 * @brief 蓝牙名称设置控制
 *
 * @param name 要设置的名称字符串
 * @param len 名称长度
 */
void bt_name_set_control(u8 *name, u32 len)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 *data = (u8 *)malloc(len + 2);
    if (data == NULL) {
        log_debug("malloc data fail!!!\n");
        return;
    }

    data[0] = PROTOCOL_TAG_SET_BT_NAME;
    memcpy(&data[1], name, len);
    data[len + 1] = '\0';

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, data, len + 2);
    }

    free(data);
}

/**
 * @brief 获取蓝牙名称控制
 */
void bt_name_get_control()
{
    printf("%s %d\n", __func__, __LINE__);
    u8 data = PROTOCOL_TAG_GET_BT_NAME;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }

}

/**
 * @brief 蓝牙MAC地址设置控制
 *
 * @param mac 要设置的MAC地址数据
 * @param len MAC地址长度
 */
void bt_mac_set_control(u8 *mac, u32 len)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 *data = (u8 *)malloc(len + 1);
    if (data == NULL) {
        log_debug("malloc data fail!!!\n");
        return;
    }

    data[0] = PROTOCOL_TAG_SET_BT_MAC;
    memcpy(&data[1], mac, len);

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, data, len + 1);
    }

    free(data);
}

/**
* @brief 蓝牙MAC地址获取控制
*/
void bt_mac_get_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_GET_BT_MAC;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 远程获取手机名称控制
 */
void bt_get_remote_name_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_GET_REMOTE_NAME;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 控制MCU(JL710)关机
 */
void bt_shutdown_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_MCU_SHUTOFF;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_MCU_CONTROL, 1, RSP_CMD_TYPE_REQUEST, &data, sizeof(data));
    }
}

/**
 * @brief 获取MCU(JL710)的开机状态
 */
void bt_get_poweron_status_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_MCU_POWERON_STATUS;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_MCU_CONTROL, 1, RSP_CMD_TYPE_REQUEST, &data, sizeof(data));
    }
}

/**
 * @brief 开始回连蓝牙
 */
void bt_start_connec_control(void)
{
    printf("%s %d\n", __func__, __LINE__);
    u8 data = PROTOCOL_TAG_START_CONNEC;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 回复从机蓝牙状态收到
 *
 * @param rsp_data 响应数据
 * @param len 响应数据长度
 */
void bt_device_bt_rsp_control(u8 sn, UART_ErrorCode cmd_status, u8 *rsp_data, u32 len)
{
    printf("%s %d\n", __func__, __LINE__);

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_response_cmd_handle(OP_CODE_BT_CONCTRL, sn, RSP_CMD_TYPE_RSP, cmd_status, rsp_data, len);
    }
}

/**
 * @brief 回复MCU控制蓝牙状态收到
 *
 * @param rsp_data 响应数据
 * @param len 响应数据长度
 */
void bt_device_mcu_rsp_control(u8 sn, UART_ErrorCode cmd_status, u8 *rsp_data, u32 len)
{
    printf("%s %d\n", __func__, __LINE__);

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_response_cmd_handle(OP_CODE_BT_MCU_CONTROL, sn, RSP_CMD_TYPE_RSP, cmd_status, rsp_data, len);
    }
}

/**
 * @brief 初始化所有信道为不可用
 *
 * @param bitarray 信道位数组
 */
void init_channels(ChannelBitArray bitarray)
{
    memset(bitarray, 0, BITARRAY_SIZE);
}

/**
 * @brief 设置单个信道状态
 *
 * @param bitarray 信道位数组
 * @param channel_index 信道索引(0-78)
 * @param available 可用状态：0-不可用，非0-可用
 */
void set_channel(ChannelBitArray bitarray, int channel_index, int available)
{
    if (channel_index < 0 || channel_index >= CHANNEL_COUNT) {
        return;  // 信道索引超出范围
    }

    int byte_index = channel_index / 8;
    int bit_index = channel_index % 8;

    if (available) {
        bitarray[byte_index] |= (1 << bit_index);    // 设置位为1
    } else {
        bitarray[byte_index] &= ~(1 << bit_index);   // 设置位为0
    }
}

/**
 * @brief 获取单个信道状态
 *
 * @param bitarray 信道位数组
 * @param channel_index 信道索引(0-78)
 * @return int 信道状态：1-可用，0-不可用
 */
int get_channel(const ChannelBitArray bitarray, int channel_index)
{
    if (channel_index < 0 || channel_index >= CHANNEL_COUNT) {
        return 0;  // 信道索引超出范围，返回不可用
    }

    int byte_index = channel_index / 8;
    int bit_index = channel_index % 8;

    return (bitarray[byte_index] >> bit_index) & 1;
}

/**
 * @brief 批量设置信道状态（使用数组）
 *
 * @param bitarray 信道位数组
 * @param channels 信道索引数组
 * @param count 信道数量
 * @param available 可用状态：0-不可用，非0-可用
 */
void set_channels_range(ChannelBitArray bitarray, const int *channels, int count, int available)
{
    for (int i = 0; i < count; i++) {
        set_channel(bitarray, channels[i], available);
    }
}

/**
 * @brief 批量设置连续信道
 *
 * @param bitarray 信道位数组
 * @param start 起始信道索引
 * @param end 结束信道索引
 * @param available 可用状态：0-不可用，非0-可用
 */
void set_channels_continuous(ChannelBitArray bitarray, int start, int end, int available)
{
    for (int i = start; i <= end; i++) {
        set_channel(bitarray, i, available);
    }
}

/**
 * @brief 打印所有信道状态
 *
 * @param bitarray 信道位数组
 */
void print_channels(const ChannelBitArray bitarray)
{
    printf("信道状态 (2402~2480MHz):\n");
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        printf("信道%d(%dMHz): %s\n",
               i, 2402 + i * 2,
               get_channel(bitarray, i) ? "可用" : "不可用");
    }
}

/**
 * @brief 蓝牙RF信道初始化
 */
void bt_rf_channel_init()
{
    printf("%s %d\n", __func__, __LINE__);

    // 初始化所有信道为不可用
    init_channels(channels);

    // 批量设置多个信道
    int available_channels[] = {10, 11, 12, 20, 21, 22, 30, 35, 37};
    printf("available_channels) : %d\n", sizeof(available_channels));
    set_channels_range(channels, available_channels, sizeof(available_channels) / sizeof(available_channels[0]), 1);

    // 设置连续的信道范围
    set_channels_continuous(channels, 60, 70, 1);  // 2522~2542MHz 可用

    // 打印信道状态
    print_channels(channels);

    // 检查特定信道状态
    printf("\n信道37状态: %s\n", get_channel(channels, 37) ? "可用" : "不可用");
}

/**
 * @brief 设置JL710芯片的工作频点
 * @note 需要在初始化或连接前调用
 */
void bt_set_rf_frequency_control()
{
    printf("%s %d\n", __func__, __LINE__);

    bt_rf_channel_init();

#define CHANNEL_DATA_SIZE   (BITARRAY_SIZE + 1)

    u8 *channel_info = (u8 *)malloc(CHANNEL_DATA_SIZE);
    if (!channel_info) {
        log_error("malloc channel_info err!\n");
        return;
    }

    channel_info[0] = PROTOCOL_TAG_MCU_RF_POINT_CTRL;
    memcpy(&channel_info[1], channels, BITARRAY_SIZE);

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_MCU_CONTROL, 1, RSP_CMD_TYPE_REQUEST, channel_info, CHANNEL_DATA_SIZE);
    }

    free(channel_info);
}
#endif

