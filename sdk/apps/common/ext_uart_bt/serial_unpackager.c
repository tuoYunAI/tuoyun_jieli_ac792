#include "app_config.h"
#include "serial_packager.h"
#include "uart_manager.h"

#define LOG_TAG         "[SERIAL_PACKAGER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "system/debug.h"

#if TCFG_INSTR_DEV_UART_ENABLE

/**
 * @brief 验证协议头是否正确
 *
 * @param head 协议头数据指针
 * @return int 1-正确，0-错误
 */
int verify_protocol_head(const uint8_t *head)
{
    return (head[0] == SERIAL_PROTOCOL_HEADER_0 &&
            head[1] == SERIAL_PROTOCOL_HEADER_1 &&
            head[2] == SERIAL_PROTOCOL_HEADER_2);
}

/**
 * @brief 验证从机发送的命令包CRC是否正常
 *
 * @param raw_data 原始数据
 * @param total_length 数据总长度
 * @return int 1-校验成功，0-校验失败
 */
int cmd_verify_crc(u8 *raw_data, size_t total_length)
{
    printf("cmd_verify_crc...\n");
    // put_buf(raw_data, total_length);

    int code_offset = offsetof(cmd_send_header_t, code);
    //printf("code_offset : %d\n", code_offset);
    int crc_offset = offsetof(cmd_send_header_t, crc);
    // printf("crc_offset : %d\n", crc_offset);

    // 从原始数据中提取接收到的CRC
    uint16_t received_crc = 0;
    memcpy(&received_crc, &raw_data[crc_offset], sizeof(uint16_t));

    // 计算CRC值
    uint16_t calculated_crc = calculate_crc(raw_data + code_offset, total_length - code_offset, 0);

    // 比较CRC值
    if (calculated_crc == received_crc) {
        printf("CRC校验成功: 计算值=0x%04X, 接收值=0x%04X\n", calculated_crc, received_crc);
        return 1; // 成功
    } else {
        printf("CRC校验失败: 计算值=0x%04X, 接收值=0x%04X\n", calculated_crc, received_crc);
        return 0; // 失败
    }
}

/**
 * @brief 验证命令回复包CRC是否正常
 *
 * @param raw_data 原始数据
 * @param total_length 数据总长度
 * @return int 1-校验成功，0-校验失败
 */
int cmd_rsp_verify_crc(u8 *raw_data, size_t total_length)
{
    printf("cmd_rsp_verify_crc...\n");
    // put_buf(raw_data, total_length);

    int code_offset = offsetof(cmd_response_header_t, code);
    // printf("code_offset : %d\n", code_offset);
    int crc_offset = offsetof(cmd_response_header_t, crc);
    // printf("crc_offset : %d\n", crc_offset);

    // 从原始数据中提取接收到的CRC
    uint16_t received_crc = 0;
    memcpy(&received_crc, &raw_data[crc_offset], sizeof(uint16_t));

    // printf("raw_data + code_offset %d\n", *(raw_data + code_offset));
    // printf("total_length - code_offset %d\n", total_length - code_offset);
    // put_buf(raw_data + code_offset, total_length - code_offset);

    // 计算CRC值
    uint16_t calculated_crc = calculate_crc(raw_data + code_offset, total_length - code_offset, 0);

    // 比较CRC值
    if (calculated_crc == received_crc) {
        printf("CRC校验成功: 计算值=0x%04X, 接收值=0x%04X\n", calculated_crc, received_crc);
        return 1; // 成功
    } else {
        printf("CRC校验失败: 计算值=0x%04X, 接收值=0x%04X\n", calculated_crc, received_crc);
        return 0; // 失败
    }
}

/**
 * @brief 将接收到的数据转换为cmd_send_header_t结构体形式
 *
 * @param packet 输出的结构体指针
 * @param raw_data 原始数据指针
 */
void constru_cmd_send_packet_from_raw(cmd_send_header_t *packet, const uint8_t *raw_data)
{
    int offset = 0;

    packet->header[0] = raw_data[offset++];
    packet->header[1] = raw_data[offset++];
    packet->header[2] = raw_data[offset++];

    packet->type = raw_data[offset++];
    packet->sn = raw_data[offset++];
    memcpy(&packet->crc, &raw_data[offset], sizeof(packet->crc));
    offset += 2;
    packet->code = raw_data[offset++];
    memcpy(&packet->len, &raw_data[offset], sizeof(packet->len));
    offset += 2;

    // parsed_packet.len 是 status + payload 的长度
    // 我们已经解析了 cmd_status（1字节），所以数据载荷长度是 len - 1
    uint16_t payload_length = packet->len;
    if (payload_length > 0) {
        packet->data = &raw_data[offset]; // 数据紧跟在固定头之后
    } else {
        printf("cmd avlid data is null\n");
    }
}

/**
 * @brief 将接收到的数据转换为cmd_response_header_t结构体形式
 *
 * @param packet 输出的结构体指针
 * @param raw_data 原始数据指针
 */
void constru_cmd_response_packet_from_raw(cmd_response_header_t *packet, const uint8_t *raw_data)
{
    printf("%s %d\n", __func__, __LINE__);

    int offset = 0;

    packet->header[0] = raw_data[offset++];
    packet->header[1] = raw_data[offset++];
    packet->header[2] = raw_data[offset++];

    packet->type = raw_data[offset++];
    packet->sn = raw_data[offset++];
    memcpy(&packet->crc, &raw_data[offset], sizeof(packet->crc));
    offset += 2;
    packet->code = raw_data[offset++];
    // printf("sizeof(packet->len) : %d\n", sizeof(packet->len));
    memcpy(&packet->len, &raw_data[offset], sizeof(packet->len));
    offset += 2;
    packet->cmd_status = raw_data[offset++];

    // parsed_packet.len 是 status + payload 的长度
    // 我们已经解析了 cmd_status（1字节），所以数据载荷长度是 len - 1
    uint16_t payload_length = packet->len - 1;
    // printf("payload_length : %d\n", payload_length);
    if (payload_length > 0) {
        packet->data = &raw_data[offset]; // 数据紧跟在固定头之后
    } else {
        r_printf("rsp avlid data is null\n");
    }
}

/**
 * @brief 解析从机发送的命令包（需要回复）
 *
 * @param raw_data 原始数据
 * @param length 数据长度
 */
void parse_slave_command_with_rsp(const uint8_t *raw_data, int length)
{
    printf("%s %d length : %d\n", __func__, __LINE__, length);
    // put_buf(raw_data, length);

    u8 flag = false;

    // 1. 首先，检查最基本的数据长度
    if (length < offsetof(cmd_send_header_t, data)) {
        // 数据长度不足以包含包头，错误处理
        printf("slave cmd data lenght not enough!!!\n");
        return;
    }

    int crc_ok = cmd_verify_crc(raw_data, length);
    if (crc_ok == 0) {
        printf("verify crc fail!!!\n");
        return;
    }

    cmd_send_header_t *packet = (cmd_send_header_t *)malloc(sizeof(cmd_send_header_t));
    if (!packet) {
        log_error("malloc cmm send header fail!!!\n");
        return;
    }

    constru_cmd_send_packet_from_raw(packet, raw_data); //封装成cmd_send_header_t结构
    printf("%s %d packet->code : %d\n", __func__, __LINE__, packet->code);

    switch (packet->code) {
    case OP_CODE_BT_CONCTRL:    //经典蓝牙控制回复
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        if (packet->data) {
            switch (packet->data[0]) {
            case PROTOCOL_TAG_SEND_BT_STATUS:    //从机发送蓝牙状态过来，主机需要回复命令
                printf("recv bt status success.\n");
                put_buf(&packet->data[1], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_STATUS_CHANGE:    //从机通知音乐状态变化
                printf("recv music status success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_VOL_CHANGE:    //蓝牙音乐通知音量变化
                printf("bt music vol change success!!!\n");
                put_buf(&packet->data[0], packet->len);
                break;
            case PROTOCOL_TAG_BT_GET_REMOTE_NAME:    //远程获取蓝牙名
                printf("bt get remote name success!!!\n");
                break;
            default:
                flag = true;
                break;
            }
            //回复从机
            if (flag == false) {
                bt_device_bt_rsp_control(packet->sn, UART_ERR_SUCCESS, &packet->data[0], packet->len);
            }
        }
        break;
    case OP_CODE_BT_MUSIC_ALBUM:    //专辑处理回复
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_BT_ALBUM_START:    //开始专辑传输
            log_debug("bt music alum transport start:\n");
            //TO DO 需要在这里回复从机
            bt_music_album_rsp_control(packet->sn, UART_ERR_SUCCESS, &packet->data[0], packet->len);
            break;
        case PROTOCOL_TAG_BT_ALBUM_TRANSFER_DATA_END:    //专辑数据发送完成
            log_debug("bt music alum transport end:\n");
            //TO DO 需要在这里回复专辑确认结束命令
            bt_music_album_rsp_control(packet->sn, UART_ERR_SUCCESS, &packet->data[0], packet->len);
            break;
        default:
            break;
        }
        break;
    case OP_CODE_HFP_CALL_CONCTRL:    //通话状态，需要回复手机
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_HFP_CALL_STATUS_CHANGE:   //通话状态改变
            printf("HFP_CALL_STATUS_CHANGE...\n");
            //to do（回复一下从机命令）
            break;
        case PROTOCOL_TAG_HFP_CALL_CUR_NUMBER:  //来电号码
            printf("HFP_CALL_STATUS_CHANGE...\n");
            //to do（回复一下从机命令）
            break;
        case PROTOCOL_TAG_HFP_CALL_CUR_NAME:  //来电姓名
            printf("HFP_CALL_STATUS_CHANGE...\n");
            //to do（回复一下从机命令）
            break;
        case PROTOCOL_TAG_HFP_SECOND_CALL_STATUS:  //第二个手机通话状态变化
            printf("PROTOCOL_TAG_HFP_SECOND_CALL_STATUS...\n");
            //to do（回复一下从机命令）
            break;

        default:
            flag = true;
            break;
        }
        //需要回复从机
        if (flag == false) {
            bt_phone_hfp_rsp_slave(UART_ERR_SUCCESS, packet->data, 1);
        }
        break;
    case OP_CODE_BT_PABP:    //获取通话记录
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_BT_PBAP_LIST_READ_END:    //获取电话本结束
            printf("PROTOCOL_TAG_BT_PBAP_LIST_READ_END...\n");
            break;
        default:
            flag = true;
            break;
        }
        bt_phone_hfp_rsp_slave(UART_ERR_SUCCESS, packet->data, packet->len);
        break;
    case OP_CODE_BT_MUSIC_INFO:    //歌曲信息处理
        break;
    case OP_CODE_OTA_CONCTRL:    //蓝牙OTA相关从机回复
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_OTA_DATA_CAN_SEND:    //(从机收到4K数据)，校验通过后可继续发送升级文件
            printf("PROTOCOL_TAG_OTA_DATA_CAN_SEND...\n");
            bt_ota_sem_post();
            break;
        default:
            break;
        }
        break;
    case OP_CODE_BT_MCU_CONTROL:    //MCU控制回复
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_MCU_POWERON_STATUS:    //从机发送开机状态回复
            printf("PROTOCOL_TAG_MCU_POWERON_STATUS...\n");
            bt_device_mcu_rsp_control(packet->sn, UART_ERR_SUCCESS, &packet->data[0], packet->len);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    free(packet);   //释放申请的内存
    printf("free packet. \n\n");
}

/**
 * @brief 解析从机发送的命令包（无需回复）
 *
 * @param raw_data 原始数据
 * @param length 数据长度
 */
void parse_slave_command_without_rsp(const uint8_t *raw_data, int length)
{
    printf("%s %d length : %d\n", __func__, __LINE__, length);
    // put_buf(raw_data, length);

    // 检查最基本的数据长度,固定头部长度,data字节之前的长度为固定长度
    if (length < offsetof(cmd_send_header_t, data)) {
        // 数据长度不足以包含包头，错误处理
        log_error("raw data lenght not enough!!!\n");
        return;
    }

    int crc_ok = cmd_verify_crc(raw_data, length);
    if (crc_ok == 0) {
        log_error("verify crc fail!!!\n");
        return;
    }

    cmd_send_header_t *packet = (cmd_send_header_t *)malloc(sizeof(cmd_send_header_t));
    if (!packet) {
        log_error("malloc cmm send header fail!!!\n");
        return;
    }

    constru_cmd_send_packet_from_raw(packet, raw_data); //封装成cmd_send_header_t结构

    y_printf("packet->code = 0x%x", packet->code);
    switch (packet->code) {
    case OP_CODE_BT_CONCTRL:    //经典蓝牙控制
        break;
    case OP_CODE_BT_MUSIC_ALBUM:    //接收专辑数据传输
        switch (packet->data[0]) {
        case PROTOCOL_TAG_BT_ALBUM_TRANSFER_DATA:    //接收到专辑数据
            log_debug("bt music album data:\n");
            put_buf(&packet->data[1], packet->len - 1);
            break;
        default:
            break;
        }
        break;
    case OP_CODE_HFP_CALL_CONCTRL:    //通话状态，需要回复手机
        if (packet->len < 0) {
            log_debug("bt phone call fail!!!\n");
            break;
        }
        log_debug("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        default:
            break;
        }
        break;
    case OP_CODE_BT_PABP:    //获取通话记录
        log_debug("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_BT_PBAP_LIST_READ_RESP:   //返回联系人信息
            log_debug("PROTOCOL_TAG_BT_PBAP_LIST_READ_RESP...\n");
            break;
        default:
            break;
        }
        break;
    case OP_CODE_BT_MUSIC_INFO:    //歌曲信息处理
        log_debug("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_RCV_BT_MUSIC_NAME:   //歌曲名字处理
            log_debug("PROTOCOL_TAG_RCV_BT_MUSIC_NAME...\n");
            put_buf(&packet->data[0], packet->len);
            break;
        case PROTOCOL_TAG_RCV_BT_MUSIC_ARTIST:   //歌曲名字处理
            log_debug("PROTOCOL_TAG_RCV_BT_MUSIC_ARTIST...\n");
            put_buf(&packet->data[0], packet->len);
            break;
        case PROTOCOL_TAG_RCV_BT_MUSIC_LYRIC:   //歌词
            log_debug("PROTOCOL_TAG_RCV_BT_MUSIC_LYRIC...\n");
            put_buf(&packet->data[0], packet->len);
            break;
        case PROTOCOL_TAG_RCV_BT_MUSIC_ALBUM:   //音乐专辑
            log_debug("PROTOCOL_TAG_RCV_BT_MUSIC_ALBUM...\n");
            put_buf(&packet->data[1], packet->len - 1);
            break;
        case PROTOCOL_TAG_RCV_BT_MUSIC_CUR_TIME:   //当前音乐播放时间
            log_debug("PROTOCOL_TAG_RCV_BT_MUSIC_CUR_TIME...\n");
            put_buf(&packet->data[0], packet->len);
            break;
        case PROTOCOL_TAG_RCV_BT_MUSIC_TOTLE_TIME:   //总的播放时间
            log_debug("PROTOCOL_TAG_RCV_BT_MUSIC_TOTLE_TIME...\n");
            put_buf(&packet->data[0], packet->len);
            break;
        default:
            break;
        }
        break;
    case OP_CODE_OTA_CONCTRL:    //蓝牙OTA相关从机回复
        log_debug("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_OTA_END:
            log_debug("收到从机的升级结束命令，结束升级\n");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    free(packet);   //释放申请的内存
    log_debug("free packet. \n\n");
}

/**
 * @brief 解析从机的回复命令（无需回复）
 *
 * @param raw_data 原始数据
 * @param length 数据长度
 */
void parse_slave_rsp_command_without_rsp(const uint8_t *raw_data, int length)
{
    printf("%s %d length : %d\n", __func__, __LINE__, length);
    // put_buf(raw_data, length);

    //检查最基本的数据长度
    if (length < offsetof(cmd_response_header_t, data)) {
        // 数据长度不足以包含包头，错误处理
        printf("slave rsp lenght not enough!!!\n");
        return;
    }

    int crc_ok = cmd_rsp_verify_crc(raw_data, length);
    if (crc_ok == 0) {
        printf("verify crc fail!!!\n");
        return;
    }
    cmd_response_header_t *packet = (cmd_response_header_t *)malloc(sizeof(cmd_response_header_t));
    if (packet == NULL) {
        log_error("cmd_response_header_t malloc error\n");
        return;
    }

    constru_cmd_response_packet_from_raw(packet, raw_data);

    printf("%s %d packet->code : %d\n", __func__, __LINE__, packet->code);
    switch (packet->code) {
    case OP_CODE_BT_CONCTRL:    //经典蓝牙控制回复
        printf("packet->cmd_status:%d [line:%d]\n", packet->cmd_status, __LINE__);
        if (packet->cmd_status != UART_ERR_SUCCESS || packet->len == 0) {
            log_error("bt conctrl cmd status or len fail!!!\n");
            break;
        }
        if (packet->data) {
            switch (packet->data[0]) {
            case PROTOCOL_TAG_SET_BT_ON_OFF:    //蓝牙开关回复
                log_debug("bt contrl set on of success.\n");
                break;
            case PROTOCOL_TAG_SET_BT_NAME:    //设置蓝牙名
                log_debug("bt name set name success\n");
                put_buf(&packet->data[1], packet->len - 1);
                break;
            case PROTOCOL_TAG_GET_BT_NAME:    //获取蓝牙名
                log_debug("bt get name success.\n");
                put_buf(&packet->data[1], packet->len - 1);
                break;
            case PROTOCOL_TAG_SET_BT_MAC:    //设置蓝牙MAC地址
                log_debug("bt set mac success.\n");
                put_buf(&packet->data[1], packet->len - 1);
                break;
            case PROTOCOL_TAG_GET_BT_MAC:    //获取蓝牙MAC地址
                log_debug("bt get mac success.\n");
                put_buf(&packet->data[1], packet->len - 1);
                break;
            case PROTOCOL_TAG_START_CONNEC:    //蓝牙回连成功
                log_debug("bt start re connec success.\n");
                put_buf(&packet->data[1], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_PP:    //蓝牙音乐暂停启动
                log_debug("bt music pp success.\n");
                put_buf(&packet->data[0], packet->len);
                break;
            case PROTOCOL_TAG_BT_MUSIC_PREV:    //蓝牙音乐上一首回复
                log_debug("bt music prev success.\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_NEXT:    //蓝牙音乐下一首回复
                log_debug("bt music next success.\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_STATUS_CHANGE:    //从机通知音乐状态变化
                log_debug("recv music status success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_VOL_UP:    //蓝牙音乐音量增加
                log_debug("bt music vol up success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_VOL_DOWN:    //蓝牙音乐音量减小
                log_debug("bt music vol down success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_MUSIC_VOL_GET:    //蓝牙音乐音量获取
                log_debug("bt music vol get success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_REQ_PAIR:    //请求配对回复
                log_debug("bt req pair success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            case PROTOCOL_TAG_BT_PAIR_RSP:    //配对响应
                log_debug("bt req pair rsp success!!!\n");
                put_buf(&packet->data[0], packet->len - 1);
                break;
            default:
                break;
            }

            put_buf(&packet->data[0], packet->len - 1);
        }
        break;
    case OP_CODE_BT_MUSIC_INFO:    //蓝牙音乐相关
        printf("packet->cmd_status:%d\n", packet->cmd_status);
        if (packet->cmd_status != UART_ERR_SUCCESS || packet->len == 0) {
            log_debug("bt music cmd_status fail!!!\n");
            break;
        }
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        default:
            break;
        }
        break;
    case OP_CODE_OTA_CONCTRL:    //蓝牙OTA相关从机回复
        if (packet->cmd_status != UART_ERR_SUCCESS || packet->len == 0) {
            log_debug("bt ota contrl fail!!!\n");
            break;
        }
        log_debug("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_OTA_START:   //开始升级OTA
            log_debug("RECV PROTOCOL_TAG_OTA_START...\n");
            bt_ota_sem_post();
            break;
        case PROTOCOL_TAG_OTA_DATA_TRANSFER_END:   //传输结束，从机回复
            log_debug("PROTOCOL_TAG_OTA_BLOCK_CRC...\n");
            break;
        case PROTOCOL_TAG_OTA_END:   //CRC校验结果
            log_debug("PROTOCOL_TAG_OTA_END...\n");
            //TO DO（CRC校验通过，从机升级完成，发送结束升级命令给主机，主机回复一下）
            bt_ota_send_end_conctrl();
            break;
        case PROTOCOL_TAG_OTA_GET_VERSION:   //获取从机的OTA版本
            log_debug("PROTOCOL_TAG_OTA_GET_VERSION...\n");
            put_buf(&packet->data[1], packet->len - 1);
            break;
        default:
            break;
        }
        break;
    case OP_CODE_HFP_CALL_CONCTRL:    //通话状态，需要回复手机
        printf("packet->cmd_status:%d line[%d]\n", packet->cmd_status, __LINE__);
        if (packet->len == 0) {
            log_debug("bt phone call fail!!!\n");
            break;
        }
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_HFP_CALL_CTRL:  //通话控制
            log_debug("PROTOCOL_TAG_HFP_CALL_CTRL...\n");
            break;
        case PROTOCOL_TAG_HFP_SECOND_CALL_CTRL:  //三方通话控制通话控制
            log_debug("PROTOCOL_TAG_HFP_SECOND_CALL_CTRL...\n");
            break;
        default:
            break;
        }
        break;
    case OP_CODE_BT_PABP:    //获取通话记录
        printf("packet->cmd_status:%d line : %d\n", packet->cmd_status, __LINE__);
        if (packet->cmd_status != UART_ERR_SUCCESS || packet->len == 0) {
            log_debug("bt pabp cmd_status fail!!!\n");
            break;
        }
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_BT_PBAP_LIST_READ:   //返回联系人信息
            log_debug("PROTOCOL_TAG_BT_PBAP_LIST_READ...\n");
            break;
        }
        break;
    case OP_CODE_BT_MUSIC_ALBUM:    //专辑
        printf("packet->cmd_status:%d line : %d\n", packet->cmd_status, __LINE__);
        if (packet->cmd_status != UART_ERR_SUCCESS || packet->len == 0) {
            log_debug("bt pabp cmd_status fail!!!\n");
            break;
        }
        printf("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_BT_ALBUM_END:    //专辑数据发送完成
            log_debug("PROTOCOL_TAG_BT_ALBUM_END:\n");
            break;
        default:
            break;
        }
        break;
    case OP_CODE_BT_MCU_CONTROL:    //MCU控制
        log_debug("packet->cmd_status:%d line : %d\n", packet->cmd_status, __LINE__);
        if (packet->cmd_status != UART_ERR_SUCCESS || packet->len == 0) {
            log_debug("bt mcu ctrl cmd_status fail!!!\n");
            break;
        }
        log_debug("packet->data[0]:%d line:%d\n", packet->data[0], __LINE__);
        switch (packet->data[0]) {
        case PROTOCOL_TAG_MCU_SHUTOFF:    //关机
            log_debug("PROTOCOL_TAG_MCU_SHUTOFF:\n");
            break;
        case PROTOCOL_TAG_MCU_POWERON_STATUS:    //开机状态
            log_debug("PROTOCOL_TAG_MCU_POWERON_STATUS:\n");
            break;
        case PROTOCOL_TAG_MCU_RF_POINT_CTRL:    //蓝牙频点设置
            log_debug("PROTOCOL_TAG_MCU_RF_POINT_CTRL:\n");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    free(packet);   //释放申请的内存
    printf("free packet. \n\n");
}

/**
 * @brief 读取数据并进行协议解析
 *
 * @param data 数据指针
 * @param length 数据长度
 * @return int 0-成功，负数-错误码
 */
int pack_parserread(u8 *data, u32 length)
{
    g_printf("pack_parserread, length: %d", length);

    if (!data || length == 0) {
        log_error("Invalid input parameters");
        return -UART_ERR_UNDEFINED;
    }

    put_buf(data, length);

    // 验证协议头
    if (!verify_protocol_head(data)) {
        printf("无效的协议头: 0x%02X%02X%02X (期望: 0xFE55AA)\n",
               data[0], data[1], data[2]);
        return -UART_ERR_FAILED;
    }

    // 解析第一个字节的各个字段,是命令包回复还是数据包回复
    uint8_t type = data[3];

    log_debug("type:0x%x", type);

    switch (type) {
    case NO_RSP_CMD_TYPE_REQUEST:   //收到从机的命令包，无需回复,无cmd_status位
        y_printf("recv cmd type@ 1\n");
        parse_slave_command_without_rsp(data, length);
        break;
    case RSP_CMD_TYPE_REQUEST:  //收到从机的命令包，需要回复,无cmd_status位
        y_printf("recv cmd type@ 2\n");
        parse_slave_command_with_rsp(data, length);
        break;
    case RSP_CMD_TYPE_RSP:      //收到从机的命令回复包，无需回复,带status位
        y_printf("recv cmd type@ 3 type : %d\n", type);
        parse_slave_rsp_command_without_rsp(data, length);
        break;
    default:
        log_error("unkown type,please check!\n");
        break;
    }

    return 0;
}
#endif

