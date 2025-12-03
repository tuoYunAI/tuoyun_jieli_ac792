#include "rf_fcc_main.h"

#ifdef TCFG_EXT_RF_FCC_TEST_ENABLE

#if 1
#define     log_info(x, ...)     printf("[RF_FCC_TEST][INFO] " x " ", ## __VA_ARGS__)
#define     log_err(x, ...)      printf("[RF_FCC_TEST][ERR] " x " ", ## __VA_ARGS__)
#else
#define     log_info(...)
#define     log_err(...)
#endif

static struct list_head head;

static const char *antenna_x_tab[] = {
    "a",
    "b",
    "ab",
};

static const char *tx_rate_tab[] = {
    "1M",
    "2M",
    "5.5M",
    "6M",
    "9M",
    "11M",
    "12M",
    "18M",
    "24M",
    "36M",
    "48M",
    "54M",
    "HTMCS0",
    "HTMCS1",
    "HTMCS2",
    "HTMCS3",
    "HTMCS4",
    "HTMCS5",
    "HTMCS6",
    "HTMCS7",
};


__attribute__((weak)) const char *fcc_get_uart(void)
{
    return "uart1";
}


__attribute__((weak))void fcc_res_handler(u8 res)
{
    log_info("%s, res = %d\n", __func__, res);
}


static u8 fcc_enter_io_trigger(void)
{
    u8 cnt = 0;

    gpio_set_pull_up(CONFIG_RF_FCC_TRIGGER_IO_PORT, !CONFIG_RF_FCC_TRIGGER_IO_STATE);
    gpio_set_pull_down(CONFIG_RF_FCC_TRIGGER_IO_PORT, CONFIG_RF_FCC_TRIGGER_IO_STATE);
    gpio_direction_input(CONFIG_RF_FCC_TRIGGER_IO_PORT);

    for (cnt = 0; cnt < CONFIG_RF_FCC_TRIGGER_IO_CNT; cnt++) {
        if (gpio_read(CONFIG_RF_FCC_TRIGGER_IO_PORT) != CONFIG_RF_FCC_TRIGGER_IO_STATE) {
            break;
        }
        os_time_dly(1);
    }
    return (cnt >= CONFIG_RF_FCC_TRIGGER_IO_CNT) ? 1 : 0;
}


__attribute__((weak))u8 fcc_enter_user_def(void)
{
    return 0;
}


static u8 fcc_enter_check(void)
{
#if (CONFIG_RF_FCC_TRIGGER_MODE == IO_TRIGGER_MODE)
    return fcc_enter_io_trigger();
#elif (CONFIG_RF_FCC_TRIGGER_MODE == USER_DEF_MODE)
    return fcc_enter_user_def();
#elif (CONFIG_RF_FCC_TRIGGER_MODE == AUTO_STARTUP_MODE)
    return 1;
#else
    return 0;
#endif
}


static void opcode_demo_create(u8 opcode, void *data, u32 len)
{
    u16 crc;
    u32 offset;
    u8 buf[MAX_DATA_SIZE] = {0};
    struct fcc_data *fcc;

    //log_info("opcode = %d, len = %d\n", opcode, len);
    ((struct fcc_data *)buf)->mark[0] = 'J';
    ((struct fcc_data *)buf)->mark[1] = 'L';
    ((struct fcc_data *)buf)->opcode = opcode;
    ((struct fcc_data *)buf)->params_len = len;

    offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);

    if (data && len) {
        memcpy(buf + offset, (u8 *)data, len);
    }

    offset += len;
    crc = CRC16(buf, offset);
    memcpy(buf + offset, &crc, sizeof(fcc->crc));
    offset += sizeof(fcc->crc);

    put_buf(buf, offset);
}


static void opcode_demo_show(void)
{
    u8 params[32], params_len;

    log_info(">>>>>>>>>查询小机是否在线>>>>>>>>>\n");
    opcode_demo_create(OP_INQURY_READY_ST, NULL, 0);
    log_info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

    params[0] = 1;
    params_len = 1;
    log_info(">>>>>>>>>>>写入测试结果>>>>>>>>>>>\n");
    opcode_demo_create(OP_WRITE_TEST_RES, params, params_len);
    log_info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

    log_info(">>>>>>>>>>>停止当前测试>>>>>>>>>>>\n");
    opcode_demo_create(OP_EXIT_CUR_TEST, NULL, 0);
    log_info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


static void opcode_data_show(struct fcc_data *fcc)
{
    switch (fcc->opcode) {
    case OP_CONTINUOUS_PACKET_TX:
        log_info("opcode = FCC_CPTX\n");
        log_info("\tmp_channel    = %d\n", ((FCC_CPTX *)fcc->params)->mp_channel);
        log_info("\tbandwidth     = %d\n", ((FCC_CPTX *)fcc->params)->bandwidth);
        log_info("\tshort_gi      = %d\n", ((FCC_CPTX *)fcc->params)->short_gi);
        log_info("\tantenna_x     = %d\n", ((FCC_CPTX *)fcc->params)->antenna_x);
        log_info("\tpathx_txpower = %d\n", ((FCC_CPTX *)fcc->params)->pathx_txpower);
        log_info("\tmp_rate       = %d\n", ((FCC_CPTX *)fcc->params)->mp_rate);
        log_info("\ttx_durtime    = %d\n", ((FCC_CPTX *)fcc->params)->tx_durtime);
        break;

    case OP_COUNT_PACKET_TX:
        log_info("opcode = FCC_CPTX_COUNT\n");
        log_info("\tmp_channel    = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->mp_channel);
        log_info("\tbandwidth     = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->bandwidth);
        log_info("\tshort_gi      = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->short_gi);
        log_info("\tantenna_x     = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->antenna_x);
        log_info("\tpathx_txpower = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->pathx_txpower);
        log_info("\tmp_rate       = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->mp_rate);
        log_info("\tnpackets      = %d\n", ((FCC_CPTX_COUNT *)fcc->params)->npackets);
        break;

    case OP_CARRIER_TX:
        log_info("opcode = FCC_CS\n");
        log_info("\tmp_channel    = %d\n", ((FCC_CS *)fcc->params)->mp_channel);
        log_info("\tbandwidth     = %d\n", ((FCC_CS *)fcc->params)->bandwidth);
        log_info("\tshort_gi      = %d\n", ((FCC_CS *)fcc->params)->short_gi);
        log_info("\tantenna_x     = %d\n", ((FCC_CS *)fcc->params)->antenna_x);
        log_info("\tpathx_txpower = %d\n", ((FCC_CS *)fcc->params)->pathx_txpower);
        log_info("\tmp_rate       = %d\n", ((FCC_CS *)fcc->params)->mp_rate);
        log_info("\ttx_durtime    = %d\n", ((FCC_CS *)fcc->params)->tx_durtime);
        break;

    case OP_CONTINUOUS_TX:
        log_info("opcode = FCC_CTX\n");
        log_info("\tmp_channel    = %d\n", ((FCC_CTX *)fcc->params)->mp_channel);
        log_info("\tbandwidth     = %d\n", ((FCC_CTX *)fcc->params)->bandwidth);
        log_info("\tshort_gi      = %d\n", ((FCC_CTX *)fcc->params)->short_gi);
        log_info("\tantenna_x     = %d\n", ((FCC_CTX *)fcc->params)->antenna_x);
        log_info("\tpathx_txpower = %d\n", ((FCC_CTX *)fcc->params)->pathx_txpower);
        log_info("\tmp_rate       = %d\n", ((FCC_CTX *)fcc->params)->mp_rate);
        log_info("\ttx_durtime    = %d\n", ((FCC_CTX *)fcc->params)->tx_durtime);
        break;

    case OP_GET_MAC:
        break;

    case OP_RX_TEST:
        log_info("opcode = FCC_RX\n");
        log_info("\tmp_channel    = %d\n", ((FCC_RX *)fcc->params)->mp_channel);
        log_info("\tbandwidth     = %d\n", ((FCC_RX *)fcc->params)->bandwidth);
        log_info("\tshort_gi      = %d\n", ((FCC_RX *)fcc->params)->short_gi);
        log_info("\tantenna_x     = %d\n", ((FCC_RX *)fcc->params)->antenna_x);
        log_info("\trx_durtime    = %d\n", ((FCC_RX *)fcc->params)->rx_durtime);
        break;

    case OP_SINGLE_TONE_TX:
        log_info("opcode = FCC_STT\n");
        log_info("\tmp_channel    = %d\n", ((FCC_STT *)fcc->params)->mp_channel);
        log_info("\tbandwidth     = %d\n", ((FCC_STT *)fcc->params)->bandwidth);
        log_info("\tshort_gi      = %d\n", ((FCC_STT *)fcc->params)->short_gi);
        log_info("\tantenna_x     = %d\n", ((FCC_STT *)fcc->params)->antenna_x);
        log_info("\tpathx_txpower = %d\n", ((FCC_STT *)fcc->params)->pathx_txpower);
        log_info("\tmp_rate       = %d\n", ((FCC_STT *)fcc->params)->mp_rate);
        log_info("\ttx_durtime    = %d\n", ((FCC_STT *)fcc->params)->tx_durtime);
        break;

    case OP_INQURY_READY_ST:
        log_info("opcode = OP_INQURY_READY_ST\n");
        break;

    case OP_WRITE_TEST_RES:
        log_info("opcode = OP_WRITE_TEST_RES\n");
        log_info("\ttest_res = %d\n", *((u8 *)fcc->params));
        break;

    case OP_EXIT_CUR_TEST:
        log_info("opcode = OP_EXIT_CUR_TEST\n");
        break;

    default:
        break;
    }
}


static void opcode_respond(u8 opcode, void *data, u32 len)
{
    u16 crc;
    u32 offset;
    u8 buf[MAX_DATA_SIZE] = {0};
    struct fcc_data *fcc;

    //log_info("opcode = %d, len = %d\n", opcode, len);
    ((struct fcc_data *)buf)->mark[0] = 'J';
    ((struct fcc_data *)buf)->mark[1] = 'L';
    ((struct fcc_data *)buf)->opcode = opcode;
    ((struct fcc_data *)buf)->params_len = len;

    offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);
    memcpy(buf + offset, (u8 *)data, len);

    offset += len;
    crc = CRC16(buf, offset);
    memcpy(buf + offset, &crc, sizeof(fcc->crc));

    offset += sizeof(fcc->crc);
    fcc_comm_ops()->write(buf, offset);

    log_info("resp_data -->>");
    put_buf(buf, offset);
}


static void opcode_recv_task(void *priv)
{
    u8 *buf, uart_data[MAX_DATA_SIZE];
    int len, move_len, crc_check_len;
    struct fcc_data *fcc = NULL;
    struct host_data *host = NULL;


    for (;;) {
        move_len = 0;
        memset(uart_data, 0, sizeof(uart_data));
        len = fcc_comm_ops()->read(uart_data, sizeof(uart_data));
        if (len < 0) {
            continue;
        }

        log_info("recv_data -->>");
        put_buf(uart_data, len);

        while (move_len < len) {
            buf = (u8 *)uart_data + move_len;
            if (buf[0] == 'J' && buf[1] == 'L') {
                host = zalloc(sizeof(struct host_data));
                if (!host) {
                    goto _exit_deal_;
                }

                host->self = host;
                fcc = &host->fcc_data;

                memcpy((u8 *) & (fcc->mark),       buf,  sizeof(fcc->mark));
                memcpy((u8 *) & (fcc->opcode),     buf + sizeof(fcc->mark), sizeof(fcc->opcode));
                memcpy((u8 *) & (fcc->params_len), buf + sizeof(fcc->mark) + sizeof(fcc->opcode), sizeof(fcc->params_len));
                memcpy((u8 *) & (fcc->crc),        buf + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len) + fcc->params_len, sizeof(fcc->crc));

                if (fcc->params_len) {
                    fcc->params = (u8 *)zalloc(fcc->params_len);
                    if (!fcc->params) {
                        goto _exit_deal_;
                    }
                    memcpy(fcc->params, buf + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len), fcc->params_len);
                }


                crc_check_len = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len) + fcc->params_len;
                if (fcc->crc != CRC16(buf, crc_check_len)) {
                    log_err("%s, crc16 error\n", __FUNCTION__);
                    goto _exit_deal_;
                }

                if (fcc->opcode < OP_START || fcc->opcode > OP_END) {
                    log_err("%s, opcode no support\n", __FUNCTION__);
                    goto _exit_deal_;
                }

                wifi_fcc_moudle_init();
                wifi_set_mp_test_status(1);

                list_add_tail(&host->entry, &head);
                host = NULL;
                fcc = NULL;
                move_len += crc_check_len + sizeof(fcc->crc);
            } else {
                break;
            }
        }
_exit_deal_:
        if (host) {
            free(host);
            host = NULL;
        }
        if (fcc) {
            free(fcc);
            fcc = NULL;
        }
    }
}


static void opcode_deal_task(void *priv)
{
    struct fcc_data *fcc;
    struct host_data *host;
    u8 status, rsp_len, rsp_params[MAX_PARAMS_SIZE];

    for (;;) {
        list_for_each_entry(host, &head, entry) {
            fcc = &host->fcc_data;
            opcode_data_show(fcc);

            wifi_set_mp_test_status(0);

            status = 0;
            rsp_params[0] = fcc->opcode;
            rsp_params[1] = status;
            rsp_len = 2;
            opcode_respond(OP_RESPOND, rsp_params, rsp_len);

            switch (fcc->opcode) {
            case OP_CONTINUOUS_PACKET_TX:
                WiFi_Continuous_Packet_Tx_testing(((FCC_CPTX *)fcc->params)->mp_channel, \
                                                  ((FCC_CPTX *)fcc->params)->bandwidth,  \
                                                  ((FCC_CPTX *)fcc->params)->short_gi,   \
                                                  (unsigned char *)antenna_x_tab[((FCC_CPTX *)fcc->params)->antenna_x], \
                                                  ((FCC_CPTX *)fcc->params)->pathx_txpower, \
                                                  (const char *)tx_rate_tab[((FCC_CPTX *)fcc->params)->mp_rate], \
                                                  ((FCC_CPTX *)fcc->params)->tx_durtime);
                break;

            case OP_COUNT_PACKET_TX:
                WiFi_Count_Packet_Tx_testing(((FCC_CPTX_COUNT *)fcc->params)->mp_channel, \
                                             ((FCC_CPTX_COUNT *)fcc->params)->bandwidth,  \
                                             ((FCC_CPTX_COUNT *)fcc->params)->short_gi,   \
                                             (unsigned char *)antenna_x_tab[((FCC_CPTX_COUNT *)fcc->params)->antenna_x], \
                                             ((FCC_CPTX_COUNT *)fcc->params)->pathx_txpower, \
                                             (const char *)tx_rate_tab[((FCC_CPTX_COUNT *)fcc->params)->mp_rate], \
                                             ((FCC_CPTX_COUNT *)fcc->params)->npackets);

                break;

            case OP_CARRIER_TX:
                WiFi_Carrier_suppression_testing(((FCC_CS *)fcc->params)->mp_channel, \
                                                 ((FCC_CS *)fcc->params)->bandwidth,  \
                                                 ((FCC_CS *)fcc->params)->short_gi,   \
                                                 (unsigned char *)antenna_x_tab[((FCC_CS *)fcc->params)->antenna_x], \
                                                 ((FCC_CS *)fcc->params)->pathx_txpower, \
                                                 (const char *)tx_rate_tab[((FCC_CS *)fcc->params)->mp_rate], \
                                                 ((FCC_CS *)fcc->params)->tx_durtime);
                break;

            case OP_CONTINUOUS_TX:
                WiFi_Continuous_Tx_testing(((FCC_CTX *)fcc->params)->mp_channel, \
                                           ((FCC_CTX *)fcc->params)->bandwidth,      \
                                           ((FCC_CTX *)fcc->params)->short_gi,       \
                                           (unsigned char *)antenna_x_tab[((FCC_CTX *)fcc->params)->antenna_x], \
                                           ((FCC_CTX *)fcc->params)->pathx_txpower,  \
                                           (const char *)tx_rate_tab[((FCC_CTX *)fcc->params)->mp_rate], \
                                           ((FCC_CTX *)fcc->params)->tx_durtime);
                break;

            case OP_GET_MAC:
                break;

            case OP_RX_TEST:
                WiFi_Air_Rx_testing(((FCC_RX *)fcc->params)->mp_channel, \
                                    ((FCC_RX *)fcc->params)->bandwidth,  \
                                    ((FCC_RX *)fcc->params)->short_gi,   \
                                    (unsigned char *)antenna_x_tab[((FCC_RX *)fcc->params)->antenna_x], \
                                    ((FCC_RX *)fcc->params)->rx_durtime);
                break;

            case OP_SINGLE_TONE_TX:
                WiFi_Single_Tone_Tx_testing(((FCC_STT *)fcc->params)->mp_channel, \
                                            ((FCC_STT *)fcc->params)->bandwidth,  \
                                            ((FCC_STT *)fcc->params)->short_gi,   \
                                            (unsigned char *)antenna_x_tab[((FCC_STT *)fcc->params)->antenna_x], \
                                            ((FCC_STT *)fcc->params)->pathx_txpower, \
                                            (const char *)tx_rate_tab[((FCC_STT *)fcc->params)->mp_rate], \
                                            ((FCC_STT *)fcc->params)->tx_durtime);
                break;

            case OP_INQURY_READY_ST:
                break;

            case OP_WRITE_TEST_RES:
                fcc_res_handler(*((u8 *)fcc->params));
                break;

            case OP_EXIT_CUR_TEST:
                wifi_set_mp_test_status(1);
                break;

            default:
                break;
            }

            list_del(&host->entry);
            free(fcc->params);
            free(host->self);
            break;
        }
        os_time_dly(1);
    }
}


void rf_fcc_tool_init(void)
{
    if (!fcc_enter_check()) {
        return;
    }

    log_info("\n\n================ENTER FCC TEST MODE================\n\n");
    opcode_demo_show();

    INIT_LIST_HEAD(&head);
    fcc_comm_ops()->init();

    thread_fork("opcode_recv_task", 28, 2048, 0, NULL, opcode_recv_task, NULL);
    thread_fork("opcode_deal_task", 26, 1024, 0, NULL, opcode_deal_task, NULL);

    while (1) {
        os_time_dly(1);
    }
}


#endif


