#include "system/includes.h"
#include "app_config.h"
#include "uart_manager.h"

#if TCFG_INSTR_DEV_UART_ENABLE

#define LOG_TAG         "[UART_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABL
#define LOG_ERROR_ENABLE
#include "system/debug.h"

/**
 * @description: uart 发送cmd接口
 * @param {u8} cmd：命令号
 * @param {bt_cmd_type_t} rsp_flag：响应类型
 * @param {u8 *} data：有效数据
 * @param {u32} len : 有效数据长度
 * @return {void}
 */
static void uart_send_cmd_deal(u8 cmd, u8 sn, bt_cmd_type_t rsp_flag, u8 *data, u32 len)
{
    printf("uart_send_cmd_deal---cmd:%d\n", cmd);

    if (!data) {
        log_error("data is null\n");
        return;
    }

    instr_send_cmd(cmd, sn, rsp_flag, data, len);
}

/**
 * @description: uart 响应cmd接口
 * @param {u8} cmd：命令号
 * @param {bt_cmd_type_t} rsp_flag：响应类型
 * @param {u8 } cmd_stats：回复命令状态
 * @param {u8 *} data：有效数据
 * @param {u32} len : 有效数据长度
 * @return {void}
 */
static void uart_response_cmd_deal(u8 cmd, u8 sn, bt_cmd_type_t rsp_flag, uint8_t cmd_stats, u8 *data, u32 len)
{
    printf("uart_response_cmd_deal");

    if (!data) {
        log_error("data is null\n");
        return;
    }

    instr_rsp_send_cmd(cmd, sn, rsp_flag, cmd_stats, data, len);
}

/**
 * @description: uart 发送data cmd接口
 * @param {u8} cmd：命令号
 * @param {u8} sn：sn码
 * @param {bt_cmd_type_t} rsp_flag：响应类型
 * @param {u8} respon_code：响应的命令
*  @param {u8} data：有效数据
 * @param {u32} len : 有效数据长度
 * @return {void}
 */
static void uart_send_data_deal(u8 cmd, uint8_t sn, bt_data_type_t rsp_flag, uint8_t respon_code, uint8_t *buffer, uint32_t buffer_len)
{
    printf("uart_send_data_deal\n");
}

/**
 * @description: 响应data包接口
 * @param {void}
 * @return {void}
 */
static void uart_response_data_deal(u8 cmd, uint8_t sn, bt_data_type_t rsp_flag, uint8_t cmd_stats, uint8_t rsp_opcode, uint8_t *buffer, uint32_t buffer_len)
{
    printf("uart_response_data_deal\n");
}

/**
 * @description: 串口初始化接口
 * @param {void}
 * @return {void}
 */
static void uart_dev_init(void)
{
    printf("uart dev init\n");
    uart_driver_init();
}

custom_uart_api_t custom_uart_api = {
    .custom_uart_dev_init               = uart_dev_init,            //串口初始化
    .custom_uart_send_cmd_handle        = uart_send_cmd_deal,       //发送命令包处理
    .custom_uart_response_cmd_handle    = uart_response_cmd_deal,   //回复从机命令处理
    .custom_uart_send_data_handle       = uart_send_data_deal,      //发送数据包处理（未启用）
    .custom_uart_response_data_handle   = uart_response_data_deal,  //回复从机数据包处理（未启用）
};

static int instr_uart_test(void)
{
    log_info("instr_uart_test\n");

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_dev_init();
    }

    return 0;
}
late_initcall(instr_uart_test);
#endif

