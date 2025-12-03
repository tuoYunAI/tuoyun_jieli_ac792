/**
 * @file bt_ota_manager.c
 * @brief 蓝牙OTA管理模块
 * @author
 * @version 1.0
 * @date
 */

#include "system/includes.h"
#include "app_config.h"
#include "uart_manager.h"
#include "fs/fs.h"

#define LOG_TAG         "[BT_OTA_MANAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"

#if TCFG_INSTR_DEV_UART_ENABLE

/* 全局变量定义 */
int ota_pid = 0;                /**< OTA任务ID */
int exit_flag = 0;              /**< 退出标志 */
static OS_SEM psem;             /**< 信号量 */

#define OTA_PACKET_SIZE 512     /**< OTA数据包大小 */
#define ALIGNMENT_4K 4096       /**< 4K对齐大小 */

#define OTA_FILE_PATH CONFIG_ROOT_PATH"db_update_data_v1.2.bin"
/**
 * @brief 判断u32值是否是4096的整数倍（除法方法）
 * @param value 要判断的u32值
 * @return 1-是4096倍数，0-不是4096倍数
 */
int is_4k_multiple_div(uint32_t value)
{
    return (value / ALIGNMENT_4K) * ALIGNMENT_4K == value;
}

static int bt_ota_task_uninit(void);

/**
 * @brief 请求从机OTA版本信息
 * @param None
 * @return None
 */
void bt_ota_request_version_info(void)
{
    log_debug("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_OTA_GET_VERSION;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_OTA_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, &data, 1);
    }
}

/**
 * @brief 蓝牙OTA开始升级命令
 * @param ota_crc OTA文件CRC校验值
 * @param ota_len OTA文件长度
 * @return None
 */
void bt_ota_start_control(u16 ota_crc, u32 ota_len)
{
    printf("%s %d\n", __func__, __LINE__);

    u32 len = sizeof(ota_crc) + sizeof(ota_len) + 1;

    u8 *data = (u8 *)malloc(len);
    if (!data) {
        log_error("bt ota malloc error\n");
        return;
    }

    data[0] = PROTOCOL_TAG_OTA_START;   // 设置协议标签
    memcpy(data + 1, &ota_len, sizeof(ota_len));
    memcpy(data + 1 + sizeof(ota_len), &ota_crc, sizeof(ota_crc));

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_OTA_CONCTRL, 1, RSP_CMD_TYPE_REQUEST, data, len);
    }

    free(data);
}

/**
 * @brief OTA数据分包传输
 * @param sn 序列号
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 0-成功，-1-失败
 */
int bt_ota_send_packet(u8 sn, u8 *buffer, u32 len)
{
    log_debug("%s %d\n", __func__, __LINE__);

    u8 *data = (u8 *)malloc(len + 1);
    if (!data) {
        printf("ota malloc fail!!!\n");
        return -1;
    }

    data[0] = PROTOCOL_TAG_OTA_DATA_TRANSFER;
    memcpy(&data[1], buffer, len);

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_OTA_CONCTRL, sn, BT_CMD_TYPE_REQUEST_WIHOUT_RSP, data, len + 1);
    }

    free(data);

    return 0;
}

/**
 * @brief 开始OTA升级
 * @param ota_path OTA文件路径
 * @return 0-成功，-1-失败
 */
int bt_ota_start(const u8 *ota_path)
{
    printf("%s %d\n", __func__, __LINE__);

    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(1);
    }

    // 读取固件文件
    FILE *file = fopen(ota_path, "r");
    printf("ota_path : %s\n", ota_path);

    if (!file) {
        printf("无法打开固件文件\n");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    u32 file_size = flen(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *firmware_data = malloc(file_size);
    if (!firmware_data) {
        log_error("申请file_size错误!!!\n");
        return -1;
    }
    fread(firmware_data, 1, file_size, file);
    fclose(file);

    u16 file_crc = calculate_crc(firmware_data, file_size, 0);
    g_printf("file_crc : 0x%x file_size : 0x%x\n", file_crc, file_size);

    bt_ota_start_control(file_crc, file_size);

    free(firmware_data);

    return 0;
}

/**
 * @brief OTA发送升级文件完成，等待从机回复CRC校验结果
 * @param None
 * @return 0-成功
 */
int bt_ota_send_finish_conctrl()
{
    log_debug("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_OTA_DATA_TRANSFER_END;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_OTA_CONCTRL, 1, BT_CMD_TYPE_REQUEST_WITH_RSP, &data, 1);
    }

    return 0;
}

/**
 * @brief OTA发送升级结束命令
 * @param None
 * @return 0-成功
 */
int bt_ota_send_end_conctrl()
{
    log_debug("%s %d\n", __func__, __LINE__);

    u8 data = PROTOCOL_TAG_OTA_END;

    if (INTSR_TASK_UART_API) {
        INTSR_TASK_UART_API->custom_uart_send_cmd_handle(OP_CODE_OTA_CONCTRL, 1, BT_CMD_TYPE_REQUEST_WIHOUT_RSP, &data, 1);
    }

    return 0;
}

/**
 * @brief 创建OTA信号量
 * @param None
 * @return None
 */
void bt_ota_sem_create(void)
{
    printf("%s %d\n", __func__, __LINE__);
    os_sem_create(&psem, 0);
}

/**
 * @brief 等待OTA信号量
 * @param None
 * @return None
 */
void bt_ota_sem_pending(void)
{
    r_printf("%s %d\n", __func__, __LINE__);
    os_sem_pend(&psem, 0);
}

/**
 * @brief 释放OTA信号量
 * @param None
 * @return None
 */
void bt_ota_sem_post(void)
{
    printf("%s %d\n", __func__, __LINE__);
    os_sem_post(&psem);
}

/**
 * @brief 删除OTA信号量
 * @param None
 * @return None
 */
void bt_ota_sem_del(void)
{
    printf("%s %d\n", __func__, __LINE__);
    if (os_sem_valid(&psem)) {
        os_sem_del(&psem, 0);
    }
}

/**
 * @brief OTA数据发送任务
 * @param priv 任务私有参数
 * @return None
 */
static void ota_send_task(void *priv)
{
    log_debug("%s %d\n", __func__, __LINE__);

    uint32_t offset = 0, total_size;
    uint16_t seq_num = 0;

    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(1);
    }

    // 读取固件文件
    FILE *file = fopen(OTA_FILE_PATH, "r");
    if (!file) {
        printf("无法打开固件文件\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    u32 file_size = flen(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *firmware_data = malloc(file_size + 2); // 预留2字节给校验和
    fread(firmware_data, 1, file_size, file);
    fclose(file);

    total_size = file_size;

    printf("pengding wiat start ota rsp...\n");
    bt_ota_sem_pending();

    while (1) {

        if (exit_flag) {
            break;
        }
        uint16_t chunk_size = (total_size - offset) > OTA_PACKET_SIZE ?
                              OTA_PACKET_SIZE : (total_size - offset);


        bt_ota_send_packet(seq_num, firmware_data + offset, chunk_size);

        g_printf("chunk_size : %d seq_num : %d\n", chunk_size, seq_num);
        offset += chunk_size;
        seq_num++;

        if (is_4k_multiple_div(offset)) {   //发完4K等一下从机写到flash后回复
            bt_ota_sem_pending();
        }

        if (chunk_size < OTA_PACKET_SIZE) {
            log_debug("send complete...\n");
            y_printf("已发送的总数据长度：%d\n", offset);
            break;
        }
    }

    bt_ota_task_uninit();
    free(firmware_data);
}

/**
 * @brief 初始化OTA任务
 * @param None
 * @return 错误码
 */
int bt_ota_task_init(void)
{
    printf("%s %d\n", __func__, __LINE__);

    if (ota_pid) {
        printf("ota task is init.\n");
        return -1;
    }
    exit_flag = 0;
    bt_ota_sem_create();
    int err = thread_fork("ota_send_task", 20, 2048, 256, &ota_pid, ota_send_task, NULL);
    if (err != OS_ERR_NONE) {
        r_printf("creat uart_recevice_task fail %x\n", err);
    }

    return err;
}

/**
 * @brief 注销初始化OTA任务
 * @param None
 * @return 错误码
 */
static int bt_ota_task_uninit(void)
{
    printf("%s %d\n", __func__, __LINE__);

    if (ota_pid) {
        thread_kill(&ota_pid, KILL_WAIT);
        ota_pid = 0;
        exit_flag = 0;
        bt_ota_sem_del();

        //发送升级数据结束
        bt_ota_send_finish_conctrl();
    }

    return 0;
}

/**
 * @brief 通知OTA发送停止
 * @param None
 * @return None
 */
void notify_ota_send_stop()
{
    bt_ota_sem_post();
    exit_flag = 1;
}

/**
 * @brief 开始蓝牙OTA更新
 * @param None
 * @return None
 */
void bt_ota_start_update(void)
{
    printf("Starting Bluetooth OTA update (function: %s, line: %d)\n", __func__, __LINE__);

    // 初始化OTA任务，等待从机响应
    bt_ota_task_init();
    // 开始OTA更新过程，指定固件文件路径
    bt_ota_start(OTA_FILE_PATH);
}

#endif

