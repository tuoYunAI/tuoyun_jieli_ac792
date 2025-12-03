#include "system/includes.h"
#include "app_config.h"
#include "uart_manager.h"

#define LOG_TAG         "[BT_DEVICE_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "system/debug.h"

#if TCFG_INSTR_DEV_UART_ENABLE

/**
 * @brief 蓝牙音乐音量增加控制
 */
void bt_music_vol_up_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_MUSIC_VOL_UP;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 蓝牙音乐音量减少控制
 */
void bt_music_vol_down_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_MUSIC_VOL_DOWN;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 蓝牙音乐获取当前音量控制
 */
void bt_music_vol_get_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_MUSIC_VOL_GET;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 蓝牙音乐暂停/播放控制
 * @note 切换播放和暂停状态
 */
void bt_music_pp_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_MUSIC_PP;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 蓝牙音乐上一首控制
 */
void bt_music_prev_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_MUSIC_PREV;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 蓝牙音乐下一首控制
 */
void bt_music_next_control(void)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_BT_MUSIC_NEXT;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 发送蓝牙配对响应控制
 *
 * @param confirm 确认标志：0-取消配对，非0-确认配对
 */
void bt_pair_rsp_control(u8 confirm)
{
    printf("%s %d\n", __func__, __LINE__);

    u8 data[2] = {0};

    data[0] = PROTOCOL_TAG_BT_PAIR_RSP;
    data[1] = confirm;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_BT_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, data, sizeof(data));
    }
}

/**
 * @brief 回复从机专辑相关命令
 *
 * @param cmd_status 命令状态码
 * @param data 响应数据
 * @param len 响应数据长度
 */
void bt_music_album_rsp_control(u8 sn, UART_ErrorCode cmd_status, u8 *data, u32 len)
{
    printf("%s %d\n", __func__, __LINE__);

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_response_cmd_handle(OP_CODE_BT_MUSIC_ALBUM, sn, RSP_CMD_TYPE_RSP, cmd_status, data, len);
    }
}

#endif

