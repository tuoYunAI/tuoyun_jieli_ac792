/**
 * @file serial_packager.c
 * @brief 串口数据包封装器
 * @author
 * @version 1.0
 * @date
 */

#include "app_config.h"
#include "serial_packager.h"
#include "uart_manager.h"

#define LOG_TAG         "[SERIAL_PACKAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"

#if TCFG_INSTR_DEV_UART_ENABLE

static cmd_send_header_t cmd_packet = {0};        /**< 命令发送包头结构体 */
static cmd_response_header_t cmd_rsp_packet = {0}; /**< 命令回复包头结构体 */

/**
 * @brief CRC计算函数
 *
 * @param buf 数据缓冲区指针
 * @param len 数据长度
 * @param init CRC初始值
 * @return u16 计算得到的CRC值
 *
 * @details 使用CRC16-XMODEM算法计算CRC
 */
u16 calculate_crc(u8 *buf, u32 len, u16 init)
{
    // log_debug("%s weak func", __func__);
    //CRC16-XMODEM
    u16 poly = 0x1021;
    u16 crc = init;
    while (len--) {
        u16 data = *buf++;
        crc ^= data << 8; //逆序
        for (u8 i = 0; i < 8; ++i) {
            crc = crc & 0x8000 ? (crc << 1) ^ poly : crc << 1; //逆序
        }
    }
    return crc;
}

/**
 * @brief 发送命令包数据
 *
 * @param packet 命令包结构体指针
 *
 * @details 将命令包结构体转换为字节数组并通过串口发送
 */
static void serial_send_cmd(cmd_send_header_t *packet)
{
    if (packet == NULL) {
        log_info("cmd pack err!!!\n");
        return;
    }

    uint8_t fixed_len = offsetof(cmd_send_header_t, data);
    // printf("offsetof(cmd_send_header_t, data) : %d packet->len :%d\n", offsetof(cmd_send_header_t, data), packet->len);
    // 计算总数据长度：固定包头 + 数据长度
    uint16_t total_len = fixed_len + packet->len;
    uint8_t *send_buffer = (uint8_t *)malloc(total_len);
    // printf("total_len : %d\n", total_len);

    if (send_buffer == NULL) {
        log_info("send buf malloc err!!!\n");
        return; // 内存分配失败
    }

    uint16_t offset = 0;

    // 填充包头固定部分
    send_buffer[offset++] = packet->header[0]; // 0xFE
    send_buffer[offset++] = packet->header[1]; // 0x55
    send_buffer[offset++] = packet->header[2]; // 0xAA
    send_buffer[offset++] = packet->type;      // 命令类型

    // SN码
    send_buffer[offset++] = packet->sn;

    // CRC位置先填充0，后面计算后再更新
    send_buffer[offset++] = 0x00; // CRC高字节（暂存）
    send_buffer[offset++] = 0x00; // CRC低字节（暂存）

    int crc_data_pos = offset;
    send_buffer[offset++] = packet->code; // 命令码

    // 数据长度（小端序）
    send_buffer[offset++] = packet->len & 0xFF;        // 长度低字节
    send_buffer[offset++] = (packet->len >> 8) & 0xFF; // 长度高字节

    // 填充数据载荷
    if (packet->data != NULL && packet->len > 0) {
        memcpy(&send_buffer[offset], packet->data, packet->len);
        offset += packet->len;
    }

    //printf("offset : %d\n", offset);
    // 计算CRC范围（从OpCode字段开始到数据结束）
    uint16_t crc_data_length = 1 + 2 + packet->len;
    uint16_t crc = calculate_crc(&send_buffer[crc_data_pos], crc_data_length, 0);
    //printf("\ncrc : 0x%x\n", crc);

    // 更新CRC到缓冲区
    send_buffer[5] = crc & 0xFF;        // CRC低字节
    send_buffer[6] = (crc >> 8) & 0xFF; // CRC高字节

    log_debug("%s send buf len:%d", __func__, total_len);
    put_buf(send_buffer, total_len);

    // 发送数据
    uart_dev_write(send_buffer, total_len);

    // 释放缓冲区
    free(send_buffer);
}

/**
 * @brief 发送命令回复包数据
 *
 * @param packet 命令回复包结构体指针
 *
 * @details 将命令回复包结构体转换为字节数组并通过串口发送
 */
static void serial_rsp_send_cmd(cmd_response_header_t *packet)
{
    if (packet == NULL) {
        log_info("cmd rsp pack err!!!\n");
        return;
    }

    uint8_t fixed_len = offsetof(cmd_response_header_t, data);
    // 计算总数据长度：固定包头 + 数据长度
    uint16_t total_len = fixed_len + (packet->len);
    uint8_t *send_buffer = (uint8_t *)malloc(total_len);
    //printf("offsetof(cmd_response_header_t, data) : %d packet->len :%d\n", offsetof(cmd_response_header_t, data), packet->len);

    if (send_buffer == NULL) {
        log_info("send buf malloc err!!!\n");
        return; // 内存分配失败
    }

    uint16_t offset = 0;

    // 填充包头固定部分
    send_buffer[offset++] = packet->header[0]; // 0xFE
    send_buffer[offset++] = packet->header[1]; // 0x55
    send_buffer[offset++] = packet->header[2]; // 0xAA
    send_buffer[offset++] = packet->type;      // 命令类型

    // SN码
    send_buffer[offset++] = packet->sn;

    // CRC位置先填充0，后面计算后再更新
    send_buffer[offset++] = 0x00; // CRC高字节（暂存）
    send_buffer[offset++] = 0x00; // CRC低字节（暂存）

    int crc_data_pos = offset;
    send_buffer[offset++] = packet->code; // 命令码

    // 数据长度（小端序）
    send_buffer[offset++] = (packet->len + 1) & 0xFF;        // 长度低字节(包含cmd_status位)
    send_buffer[offset++] = ((packet->len + 1) >> 8) & 0xFF; // 长度高字节(包含cmd_status位)
    send_buffer[offset++] = packet->cmd_status;        // 状态
    // 填充数据载荷
    if (packet->data != NULL && packet->len > 0) {
        memcpy(&send_buffer[offset], packet->data, packet->len);
        offset += packet->len;
    }

    printf("offset : %d\n", offset);
    // 计算CRC范围（从OpCode字段开始到数据结束）
    uint16_t crc_data_length = 1 + 2 + 1 + packet->len;
    // put_buf(&send_buffer[crc_data_pos], crc_data_length);
    uint16_t crc = calculate_crc(&send_buffer[crc_data_pos], crc_data_length, 0);
    printf("\ncrc : 0x%x\n", crc);

    // 更新CRC到缓冲区
    send_buffer[5] = crc & 0xFF;        // CRC低字节
    send_buffer[6] = (crc >> 8) & 0xFF; // CRC高字节

    printf("%s send buf len:%d\n", __func__, total_len);
    put_buf(send_buffer, total_len);
    // 发送数据
    uart_dev_write(send_buffer, total_len);

    // 释放缓冲区
    free(send_buffer);
}

/**
 * @brief 构造命令请求包
 *
 * @param sn 序列号
 * @param rsp_flag 响应标志
 * @param op_code 操作码
 * @param buffer 数据缓冲区
 * @param buffer_len 数据长度
 * @return size_t 返回0（函数实际无返回值）
 */
size_t construct_serial_cmd_request(uint8_t sn, bt_cmd_type_t rsp_flag, uint8_t op_code, uint8_t *buffer, uint32_t buffer_len)
{
    // 填充包头
    cmd_packet.header[0] = SERIAL_PROTOCOL_HEADER_0;
    cmd_packet.header[1] = SERIAL_PROTOCOL_HEADER_1;
    cmd_packet.header[2] = SERIAL_PROTOCOL_HEADER_2;
    cmd_packet.type = rsp_flag;
    cmd_packet.sn = sn;

    cmd_packet.code = op_code;
    cmd_packet.len = buffer_len;
    cmd_packet.data = buffer;

    // 复制到输出缓冲区
    serial_send_cmd(&cmd_packet);

    return 0;
}

/**
 * @brief 构造命令回复请求包
 *
 * @param sn 序列号
 * @param rsp_flag 响应标志
 * @param op_code 操作码
 * @param cmd_stats 命令状态
 * @param buffer 数据缓冲区
 * @param buffer_len 数据长度
 * @return size_t 返回0（函数实际无返回值）
 */
size_t construct_serial_rsp_cmd_request(uint8_t sn, bt_cmd_type_t rsp_flag, uint8_t op_code, uint8_t cmd_stats, uint8_t *buffer, uint32_t buffer_len)
{
    // 填充包头
    cmd_rsp_packet.header[0] = SERIAL_PROTOCOL_HEADER_0;
    cmd_rsp_packet.header[1] = SERIAL_PROTOCOL_HEADER_1;
    cmd_rsp_packet.header[2] = SERIAL_PROTOCOL_HEADER_2;
    cmd_rsp_packet.type = rsp_flag;
    cmd_rsp_packet.sn = sn;

    cmd_rsp_packet.code = op_code;
    cmd_rsp_packet.len = buffer_len;
    cmd_rsp_packet.cmd_status = cmd_stats;
    cmd_rsp_packet.data = buffer;

    // 复制到输出缓冲区
    serial_rsp_send_cmd(&cmd_rsp_packet);

    return 0;
}

/**
 * @brief 发送命令包函数
 *
 * @param op_cmd 操作命令码
 * @param sn 序列号
 * @param rsp_flag 响应标志
 * @param data 数据指针
 * @param len 数据长度
 * @return int 返回值（当前实现中总是返回0）
 */
int instr_send_cmd(u8 op_cmd, u8 sn, bt_cmd_type_t rsp_flag, u8 *data, u32 len)
{
    printf("%s op_cmd:%d\n", __func__, op_cmd);
    int ret = 0;

    switch (op_cmd) {
    case OP_CODE_BT_CONCTRL:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_BT_CONCTRL, data, len);
        break;
    case OP_CODE_HFP_CALL_CONCTRL:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_HFP_CALL_CONCTRL, data, len);
        break;
    case OP_CODE_BT_PABP:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_BT_PABP, data, len);
        break;
    case OP_CODE_BT_MUSIC_INFO:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_BT_MUSIC_INFO, data, len);
        break;
    case OP_CODE_BT_MUSIC_ALBUM:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_BT_MUSIC_ALBUM, data, len);
        break;
    case OP_CODE_BT_MCU_CONTROL:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_BT_MCU_CONTROL, data, len);
        break;
    case OP_CODE_OTA_CONCTRL:
        ret = construct_serial_cmd_request(sn, rsp_flag, OP_CODE_OTA_CONCTRL, data, len);
        break;
    default:
        log_error("unkown opcode!!!\n");
        break;
    }

    return ret;
}

/**
 * @brief 发送命令回复包函数
 *
 * @param op_cmd 操作命令码
 * @param sn 序列号
 * @param rsp_flag 响应标志
 * @param cmd_stats 命令状态
 * @param data 数据指针
 * @param len 数据长度
 * @return int 返回值（当前实现中总是返回0）
 */
int instr_rsp_send_cmd(u8 op_cmd, u8 sn, bt_cmd_type_t rsp_flag, uint8_t cmd_stats, u8 *data, u32 len)
{
    printf("%s op_cmd:%d\n", __func__, op_cmd);
    int ret = 0;

    switch (op_cmd) {
    case OP_CODE_BT_CONCTRL:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_BT_CONCTRL, cmd_stats, data, len);
        break;
    case OP_CODE_HFP_CALL_CONCTRL:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_HFP_CALL_CONCTRL, cmd_stats, data, len);
        break;
    case OP_CODE_BT_PABP:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_BT_PABP, cmd_stats, data, len);
        break;
    case OP_CODE_BT_MUSIC_INFO:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_BT_MUSIC_INFO, cmd_stats, data, len);
        break;
    case OP_CODE_BT_MUSIC_ALBUM:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_BT_MUSIC_ALBUM, cmd_stats, data, len);
        break;
    case OP_CODE_BT_MCU_CONTROL:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_BT_MCU_CONTROL, cmd_stats, data, len);
        break;
    case OP_CODE_OTA_CONCTRL:
        ret = construct_serial_rsp_cmd_request(sn, rsp_flag, OP_CODE_OTA_CONCTRL, cmd_stats, data, len);
        break;
    default:
        break;
    }

    return ret;
}
#endif

