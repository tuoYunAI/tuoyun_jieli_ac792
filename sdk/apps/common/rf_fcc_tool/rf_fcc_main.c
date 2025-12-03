#include "rf_fcc_main.h"

#if TCFG_RF_FCC_TEST_ENABLE || TCFG_RF_PRODUCT_TEST_ENABLE

#include "device/gpio.h"
#include "device/uart.h"
#include "btcontroller_modules.h"
#include "utils/syscfg/syscfg_id.h"
#include "asm/clock.h"
#include "asm/crc16.h"
#include "fs/fs.h"
#include "json_c/json_tokener.h"
#ifdef CONFIG_NET_ENABLE
#include "lwip.h"
#include "wifi/wifi_connect.h"
#endif

#ifdef CONFIG_BT_ENABLE
#include "bluetooth.h"
#endif


#if 1
#define     log_info(x, ...)     printf("[RF_FCC_TEST][INFO] " x " ", ## __VA_ARGS__)
#define     log_err(x, ...)      printf("[RF_FCC_TEST][ERR] " x " ", ## __VA_ARGS__)
#else
#define     log_info(...)
#define     log_err(...)
#endif

#define WIFI_SIGN_INDEX	      (90)
#define MAX_DGAIN   		  (128)
#define MIN_DGAIN   		  (0)
#define MAX_XOSC    		  (15)
#define MIN_XOSC    		  (0)
#define TXINFO_SIZE			  (4)
#define USB_DIR_OUT	          (0)
#define USB_DIR_IN	          (0x80)
#define WIFI_TXWI_HEAD_SIZE   (20)
#define WIFI_80211_FILL_SIZE  (27) //保留一些字节不填充, 为了避开duration段和上层发送1个字节也支持
#define MAC_SYS_CTRL          (0x1004)		/*MAC_CSR1 */
#define TX_INR_IN_ADJ   	  (1000)
#define TX_LEN_IN_ADJ   	  (1000)

#define LOOP_TEST_TRIM 0//循环测试校准时打开，重置读取txt文件配置进行循环测试

struct fcc_info {
    char res[8];
    u16 crc;
};


struct rate_info {
    const char string[6];
    u8 phy;
    u8 mcs;
};


struct ctl_params {
    int usb_id;
    int tx_pid;
    u32 rand_num;
    u32 total_rx_cnt;
    void *uart_hdl;
    struct FCC_TX tx_params;
    struct FCC_RX rx_params;
    struct list_head head;
    OS_SEM conn_sem;
    OS_SEM cdc_sem;
    OS_MUTEX list_mutex;
    struct WIFI_SIGN_INFO g_sign_info;
    struct rate_info tx_rate_tab[20];
    u8 online_flag;
    u8 tx_stop_flag;
    char sign_ssid[33], sign_pwd[65];
    u8 wifi_send_pkg[1564] __attribute__((aligned(4)));
    u8 uart_circlebuf[1024] __attribute__((aligned(4)));
    char fcc_str[3][8];
    s8 ana_pw;
    u8 test_mode;
    u8 signalling_mode;
} *__THIS;

static u8 g_cur_mode;
static struct fcc_mode g_mode_info = {0};

static const u8 FCC_RSP_ARRAY[7]       = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x00};
static const u8 FCC_RSP_FAIL_ARRAY[7]  = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x01};
static const u8 FCC_READY_RSP_ARRAY[7] = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x02};
static u8 FCC_HEART_RSP_ARRAY[10] = {0x04, 0x0E, 0x04, 0x01, 0xA1, 0xA3, 0x00, 0x00, 0x00, 0x00};
static const u8 FCC_WIFI_ARRAY[10]      = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01};
static const u8 FCC_BT_ARRAY[10]        = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x02};
static const u8 FCC_RESET_ARRAY[10]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x03};
static const u8 FCC_RES_SUCC_ARRAY[10]  = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x04};
static const u8 FCC_RES_FAIL_ARRAY[10]  = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05};
static const u8 FCC_BT_DUT_ARRAY[10]    = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
static const u8 FCC_BT_BQB_ARRAY[10]    = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07};
static const u8 FCC_HEART_ARRAY[10]     = {0x01, 0xA1, 0xA3, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00};
static const u8 FCC_READY_ARRAY[10]     = {0x01, 0xA1, 0xA3, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01};

static const u8 FCC_EXIT_ARRAY[10]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08};

static const u8 FCC_SET_BT_WIFI_MAC_ARRAY[10]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x09};
static const u8 FCC_GET_BT_WIFI_MAC_ARRAY[10]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0A};
static u8 FCC_GET_BT_WIFI_MAC_RSP_ARRAY[13] = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const u8 FCC_SET_BLE_MAC_ARRAY[10]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0B};
static const u8 FCC_GET_BLE_MAC_ARRAY[10]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0C};
static u8 FCC_GET_BLE_MAC_RSP_ARRAY[13] = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

extern short CHL_BUSY_CONFIG;
int wifi_get_mac(u8 *mac);
u32 sdio_mac_rreg(u32 reg);
void wf_tx_sine_test(void);
void wf_read_xosc(u8 *xosc);
void wf_write_xosc(u8 *xosc);
void wifi_get_mcs_dgain(u8 *mcs_dgain);
void wl_anl_actl_init(char init_sel);
static void rf_fcc_save_change_mode(void);
void sdio_mac_wreg(u32 wr_addr, u32 dat);
void set_pa_config_data(u8 user_data[7]);
void wifi_set_channel(unsigned char set_freq);
u8 *fcc_common_data_deal(u8 packet_type, u8 *data, u32 len, u32 *rsp_len, u8 *reset);
void mp_test_pa_mcs_dgain_set(char mode, char mcs, unsigned char gain);
static void read_res(void);
static int udp_server_init(int port);
void SignalingModeSet(unsigned char enable);
void wf_phy_rst(void);
void wf_phy_tx_always_test(void);
static void report_data(u8 opcode, void *data, u32 len);
u8 set_search_bd_name(char *name);
void bt_set_local_name(const char *name);
void wf_tx_sine_test_reset(unsigned char reset);
void bredr_set_dut_enble(u8 en, u8 phone);
u8 rf_fcc_adj_res_read(char *str, void *data);
u8 rf_fcc_adj_res_write(char *str, void *data);

static u8 array_cmp(void *a_array, void *b_array, u32 size)
{
    int i;
    u8 *a = (u8 *)a_array;
    u8 *b = (u8 *)b_array;

    for (i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            break;
        }
    }
    return (i < size ? 1 : 0);
}


__attribute__((weak)) const char *rf_fcc_get_uart(void)
{
    return "uart1";
}


static u8 comm_dev_init(void)
{
#ifdef FCC_USB_COMM
    os_sem_create(&__THIS->cdc_sem, 0);
#else
    __THIS->uart_hdl = dev_open(rf_fcc_get_uart(), 0);
    if (!__THIS->uart_hdl) {
        return -1;
    }

    dev_ioctl(__THIS->uart_hdl, IOCTL_UART_SET_CIRCULAR_BUFF_ADDR, (u32)__THIS->uart_circlebuf);
    dev_ioctl(__THIS->uart_hdl, IOCTL_UART_SET_CIRCULAR_BUFF_LENTH, sizeof(__THIS->uart_circlebuf));
    dev_ioctl(__THIS->uart_hdl, IOCTL_UART_SET_RECV_BLOCK, 1);
    dev_ioctl(__THIS->uart_hdl, IOCTL_UART_SET_RECV_TIMEOUT, 1000);
    dev_ioctl(__THIS->uart_hdl, IOCTL_UART_START, 0);
#endif
    return 0;
}


static int comm_dev_tx_data(u8 *buf, u32 len)
{
#ifdef FCC_USB_COMM
    return cdc_write_data(__THIS->usb_id, buf, len);
#else
    return dev_write(__THIS->uart_hdl, buf, len);
#endif
}


static int comm_dev_rx_data(u8 *buf, u32 len)
{
#ifdef FCC_USB_COMM
    os_sem_pend(&__THIS->cdc_sem, 0);
    return cdc_read_data(__THIS->usb_id, buf, len);
#else
    return dev_read(__THIS->uart_hdl, buf, len);
#endif
}


#ifdef FCC_USB_COMM
#include "event.h"
#include "device_event.h"
static int product_device_event_handler(struct sys_event *e)
{
    struct device_event *event = (struct device_event *)e->payload;
    const char *usb_msg = (const char *)event->value;

    if (e->from == DEVICE_EVENT_FROM_OTG) {
        if (usb_msg[0] == 's') {
            if (event->event == DEVICE_EVENT_IN) {
                printf("usb %c online", usb_msg[2]);
                __THIS->online_flag = 1;
                __THIS->usb_id = usb_msg[2] - '0';
            } else if (event->event == DEVICE_EVENT_OUT) {
                printf("usb %c offline", usb_msg[2]);
                __THIS->online_flag = 0;
            }
        }
    }
    return 0;
}

SYS_EVENT_STATIC_HANDLER_REGISTER(product_device_event, 0) = {
    .event_type     = SYS_DEVICE_EVENT,
    .prob_handler   = product_device_event_handler,
    .post_handler   = NULL,
};
#endif


u8 fcc_comm_cdc_post()
{
#ifdef FCC_USB_COMM
    os_sem_post(&__THIS->cdc_sem);
    return 1;
#else
    return 0;
#endif
}


void fcc_comm_dev_tx_data(int packet_type, const u8 *packet, int size)
{
    if (!config_rf_test_enable) {
        return;
    }

#ifdef FCC_USB_COMM
    u8 data[64] = {0};
    data[0] = packet_type;
    memcpy(&data[1], packet, size);
    cdc_write_data(__THIS->usb_id, data, size + sizeof(u8));
#else
    dev_write(__THIS->uart_hdl, &packet_type, sizeof(u8));
    dev_write(__THIS->uart_hdl, packet, size);
#endif
}


static void comm_dev_recv_task(void *priv)
{
    struct fcc_data *fcc = NULL;
    struct host_data *host = NULL;
    int len, move_len, crc_check_len;
    u32 rsp_len, offset, remain_size;
    u8 *buf, *rsp = NULL, reset = 0, data[256];

#ifdef FCC_USB_COMM
    while (!__THIS->online_flag) {
        os_time_dly(10);
    }
#endif

#ifdef CONFIG_BT_ENABLE
    while (g_cur_mode != FCC_WIFI_MODE) {
        memset(data, 0, sizeof(data));
        len = comm_dev_rx_data(data, sizeof(data));
        if (len <= 0) {
            os_time_dly(5);
            continue;
        }

        switch (data[0]) {
        case HCI_COMMAND_DATA_PACKET:
            rsp  = fcc_common_data_deal(data[0], &data[1], len - 1, &rsp_len, &reset);
            if (rsp || reset || rsp_len) {
                if (rsp) {
                    comm_dev_tx_data(rsp, rsp_len);
                }

                if (reset) {
                    cpu_reset();
                }
                break;
            }

            if (!g_cur_mode) {
                log_err("no in fcc mode");
                break;
            }

            void fcc_bt_packet_handler(int packet_type, const u8 * packet, int size);
            fcc_bt_packet_handler(data[0], &data[1], len - 1);
            break;

        case HCI_ACL_DATA_PACKET:
            break;

        case HCI_ISO_DATA_PACKET:
            break;

#ifdef CONFIG_FCC_BT_ADJ_ENABLE
        case 0xFE:
            u16 *opcode = &data[1];
            switch (*opcode) {
            case 0x01:
                u8 mode = data[4];
                u8 *name = &data[5];
                log_info("op = %x, m = %d, n = %s", *opcode, mode, name);
                g_mode_info.mode = FCC_BT_ADJ_MODE;
                g_mode_info.params[0] = mode;
                strcpy(&g_mode_info.params[1], name);
                rf_fcc_save_change_mode();
                log_info("COMMON_BT OPCODE : change to FCC_BT_ADJ_MODE %s", mode ? "MASTER" : "SALVE");
                cpu_reset();
                break;

            default:
                break;
            }
            break;
#endif
        default:
            break;
        }
    }
#endif

#ifdef CONFIG_NET_ENABLE
    for (;;) {
        move_len = 0;
        memset(data, 0, sizeof(data));
        len = comm_dev_rx_data(data, sizeof(data));
        if (len <= 0) {
            os_time_dly(5);
            continue;
        }

        /* put_buf(data, len); */
        rsp  = fcc_common_data_deal(data[0], &data[1], len - 1, &rsp_len, &reset);
        if (rsp) {
            comm_dev_tx_data(rsp, rsp_len);
        }

        if (reset) {
            cpu_reset();
        }

        if (!g_cur_mode) {
            log_err("no in fcc mode");
            continue;
        }

        while (move_len < len) {
            buf = (u8 *)data + move_len;
            if (buf[0] == 'J' && buf[1] == 'L') {
                host = zalloc(sizeof(struct host_data));
                if (!host) {
                    goto _exit_deal_;
                }

                host->self = host;
                host->flag = 0;
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

                os_mutex_pend(&__THIS->list_mutex, 0);
                list_add_tail(&host->entry, &__THIS->head);
                os_mutex_post(&__THIS->list_mutex);
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
#endif
}


#ifdef CONFIG_NET_ENABLE
static void wifi_tx_data(u8 *pkg, int len, u32 rate, u8 bw, u8 short_gi)// 最大包1513
{
    u16 *PktLen = &__THIS->wifi_send_pkg[0];
    u16 *MPDUtotalByteCount = &__THIS->wifi_send_pkg[10];
    *PktLen = WIFI_80211_FILL_SIZE + len + WIFI_TXWI_HEAD_SIZE + 4 - 8;
    *MPDUtotalByteCount = WIFI_80211_FILL_SIZE + len;

    if (pkg) {
        put_buf(pkg, len);
        memcpy(&__THIS->wifi_send_pkg[WIFI_TXWI_HEAD_SIZE + WIFI_80211_FILL_SIZE], pkg, len);
    } else {
        memset(&__THIS->wifi_send_pkg[WIFI_TXWI_HEAD_SIZE + WIFI_80211_FILL_SIZE], 0xaa, len);
    }

    struct urb urb;
    urb.pipe = USB_DIR_OUT;
    urb.complete = NULL;
    urb.transfer_buffer = __THIS->wifi_send_pkg;
    urb.transfer_buffer_length = len + WIFI_TXWI_HEAD_SIZE + WIFI_80211_FILL_SIZE + 4;

    PTXWI_STRUC pTxWI = TXINFO_SIZE + (u8 *)urb.transfer_buffer;
    pTxWI->PHYMODE    = __THIS->tx_rate_tab[rate].phy;
    pTxWI->MCS        = __THIS->tx_rate_tab[rate].mcs;
    pTxWI->BW         = bw;
    pTxWI->ShortGI    = !short_gi;
    usb_submit_urb(&urb, 0);
}


static void wifi_rx_frame_cb(void *rxwi, void *header, void *data, u32 len, void *reserve)
{
    /* PRXWI_STRUC pRxWI = (PRXWI_STRUC *)rxwi; */
    u8 *src_mac = (u8 *)data + 34;
    /* u8 *src_mac = (u8 *)data + 28; */

    /* putchar('A'); */
    /* if (pRxWI->BW == __THIS->rx_params.bandwidth && \ */
    /*     pRxWI->ShortGI == __THIS->rx_params.short_gi) { */
    /* putchar('B'); */
    /* put_buf((u8 *)data, len); */
    if (__THIS->rx_params.filter_enable) {
        /* log_info("recv_mac:%x:%x:%x:%x:%x:%x\n", src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]); */
        if (!array_cmp(src_mac, __THIS->rx_params.filter_mac, sizeof(__THIS->rx_params.filter_mac))) {
            /* putchar('C'); */
            __THIS->total_rx_cnt++;
        }
    } else {
        __THIS->total_rx_cnt++;
    }
    /* } */
}


static int wifi_event_callback(void *network_ctx, enum WIFI_EVENT event)
{
    u8 mac[6];
    static u8 init = 0;
    struct wifi_store_info wifi_default_mode_parm = {0};

    log_info("%s, event = %d\n", __FUNCTION__, event);

    switch (event) {
    case WIFI_EVENT_MODULE_INIT:

        int init_net_device_mac_addr(char *macaddr, char ap_mode);
        init_net_device_mac_addr(mac, 0);
        if (!strcmp(__THIS->g_sign_info.str, WIFI_SIGN_STR)) {
            wifi_default_mode_parm.mode = STA_MODE;
            strcpy(__THIS->sign_ssid, __THIS->g_sign_info.data.ssid);
            strcpy(__THIS->sign_pwd, __THIS->g_sign_info.data.pwd);
        } else {
            wifi_default_mode_parm.mode = init ? STA_MODE : MP_TEST_MODE;
            init = 1;
        }

        if (wifi_default_mode_parm.mode <= AP_MODE) {
            strncpy((char *)wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE], __THIS->sign_ssid, sizeof(wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE]) - 1);
            strncpy((char *)wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE], __THIS->sign_pwd, sizeof(wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE]) - 1);
        }
        wifi_set_default_mode(&wifi_default_mode_parm, 1, wifi_default_mode_parm.mode == STA_MODE);
        break;

    case WIFI_EVENT_MODULE_START:
#if 0

        u32  tx_rate_control_tab = // 不需要哪个速率就删除掉,可以动态设定
            0
            | BIT(0) //0:CCK 1M
            | BIT(1) //1:CCK 2M
            | BIT(2) //2:CCK 5.5M
            | BIT(3) //3:OFDM 6M
            | BIT(4) //4:MCS0/7.2M
            | BIT(5) //5:OFDM 9M
            | BIT(6) //6:CCK 11M
            | BIT(7) //7:OFDM 12M
            | BIT(8) //8:MCS1/14.4M
            | BIT(9) //9:OFDM 18M
            | BIT(10) //10:MCS2/21.7M
            | BIT(11) //11:OFDM 24M
            | BIT(12) //12:MCS3/28.9M
            | BIT(13) //13:OFDM 36M
            | BIT(14) //14:MCS4/43.3M
            | BIT(15) //15:OFDM 48M
            | BIT(16) //16:OFDM 54M
            | BIT(17) //17:MCS5/57.8M
            | BIT(18) //18:MCS6/65.0M
            | BIT(19) //19:MCS7/72.2M
            ;
        wifi_set_tx_rate_control_tab(tx_rate_control_tab);
#endif
        break;

    case WIFI_EVENT_STA_START:
        if (!strcmp(__THIS->g_sign_info.str, WIFI_SIGN_STR)) {
            SignalingModeSet(1);
            wf_write_xosc(__THIS->g_sign_info.data.xosc);
            set_pa_config_data(__THIS->g_sign_info.data.pa);
            wl_anl_actl_init(0);
            for (u8 i = 0; i < sizeof(__THIS->g_sign_info.data.gain); i++) {
                mp_test_pa_mcs_dgain_set(__THIS->tx_rate_tab[i].phy, __THIS->tx_rate_tab[i].mcs, __THIS->g_sign_info.data.gain[i]);
            }
            udp_server_init(30136);
        }
#ifdef CONFIG_IPERF_ENABLE
        extern void iperf_test(void);
        iperf_test();
#endif
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        os_sem_post(&__THIS->conn_sem);
        break;

    case WIFI_EVENT_STA_STOP:
        void wifi_adaptivity_exit(void);
        wifi_adaptivity_exit();
        break;

    default:
        break;
    }

    return 0;
}


static void tx_test_task(void *priv)
{
    u32 cnt = 0;

    for (;;) {
        wifi_tx_data((u8 *)priv, __THIS->tx_params.packet_len, __THIS->tx_params.rate, __THIS->tx_params.bandwidth, __THIS->tx_params.short_gi);
        /* putchar('T'); */
        if (__THIS->tx_params.send_interval) {
            delay_us(__THIS->tx_params.send_interval);
        }

        if (__THIS->tx_params.npackets) {
            cnt++;
            if (cnt >= __THIS->tx_params.npackets) {
                __THIS->tx_pid = 0;
                return;
            }
        }

        if (__THIS->tx_stop_flag) {
            __THIS->tx_pid = 0;
            return;
        }
    }
}


static void start_tx_test_data(void *priv)
{
    wifi_set_long_retry(0);
    wifi_set_short_retry(0);
    __THIS->tx_stop_flag = 0;
    thread_fork("tx_test_task", 10, 256, 0, &__THIS->tx_pid, tx_test_task, priv);
}


static void stop_tx_test_data(void)
{
    if (__THIS->tx_pid) {
        __THIS->tx_stop_flag = 1;
        while (__THIS->tx_pid) {
            os_time_dly(1);
        }
    }

    /* if (__THIS->tx_pid) { */
    /*     thread_kill(&__THIS->tx_pid, KILL_FORCE); */
    /*     __THIS->tx_pid = 0; */
    /* } */
}


static void report_data(u8 opcode, void *data, u32 len)
{
    u16 crc;
    u32 offset;
    u8 buf[64] = {0};
    struct fcc_data *fcc;

    /* log_info("opcode = %d, len = %d\n", opcode, len); */
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
    comm_dev_tx_data(buf, offset);
    /* put_buf(buf, offset); */
}
#endif


#ifdef CONFIG_FCC_BT_ADJ_ENABLE
void fcc_bt_adj_report(u8 *name, u8 flag)
{
    u8 rsp[38] = {0xFE, 0x0E, 3, 0, 0x01, 0};
    rsp[5] = flag;
    if (!flag) {
        strcpy(&rsp[6], name);
        rsp[2] = 3 + strlen(name);
    }
    comm_dev_tx_data(rsp, strlen(name) + 6);
}
#endif


#ifdef CMD_DEBUG
static void fcc_data_show(u8 opcode, void *priv)
{
    u8 *mac, mode, *pa;
    struct FCC_TX *tx;
    struct FCC_RX *rx;
    struct FREQ_ADJ *freq_adj;
    struct PWR_ADJ *pwr_adj;
    struct WIFI_SIGN *sign;
    struct FCC_RX_STAT *rxs;

    switch (opcode) {
    case OP_FCC_INQ_RAND_NUM:
        /* log_info("OP_FCC_INQ_RAND_NUM:\n"); */
        putchar('H');
        break;

    case OP_FCC_START_TX:
        tx = (struct FCC_TX *)priv;
        log_info("\tOP_FCC_START_TX:\n");
        log_info("\tpa            = %d%d%d%d%d%d%d\n", tx->pa[0], tx->pa[1], \
                 tx->pa[2], tx->pa[3], tx->pa[4], tx->pa[5], tx->pa[6]);
        log_info("\txosc          = %d, %d\n", tx->xosc[0], tx->xosc[1]);
        log_info("\tchannel       = %d\n", tx->channel);
        log_info("\tbandwidth     = %d\n", tx->bandwidth);
        log_info("\tshort_gi      = %d\n", tx->short_gi);
        log_info("\tantenna_x     = %d\n", tx->antenna_x);
        log_info("\tpathx_txpower = %d\n", tx->pathx_txpower);
        log_info("\trate          = %s\n", __THIS->tx_rate_tab[tx->rate].string);
        log_info("\tnpackets      = %d\n", tx->npackets);
        log_info("\tpacket_len    = %d\n", tx->packet_len);
        log_info("\tsend_interval = %d\n", tx->send_interval);
        log_info("\tcw_flag       = %d\n", tx->cw_flag);
        break;

    case OP_FCC_STOP_TX:
        log_info("\tOP_FCC_STOP_TX:\n");
        break;

    case OP_FCC_START_RX:
        rx = (struct FCC_RX *)priv;
        log_info("OP_FCC_START_RX:\n");
        log_info("\tpa            = %d%d%d%d%d%d%d\n", rx->pa[0], rx->pa[1], \
                 rx->pa[2], rx->pa[3], rx->pa[4], rx->pa[5], rx->pa[6]);
        log_info("\txosc          = %d, %d\n", rx->xosc[0], rx->xosc[1]);
        log_info("\tchannel       = %d\n", rx->channel);
        log_info("\tbandwidth     = %d\n", rx->bandwidth);
        log_info("\tshort_gi      = %d\n", rx->short_gi);
        log_info("\tantenna_x     = %d\n", rx->antenna_x);
        log_info("\tfilter_enable = %d\n", rx->filter_enable);
        log_info("\tfilter_mac    = %x:%x:%x:%x:%x:%x\n", rx->filter_mac[0], rx->filter_mac[1], \
                 rx->filter_mac[2], rx->filter_mac[3], rx->filter_mac[4], rx->filter_mac[5]);
        break;

    case OP_FCC_STOP_RX:
        log_info("OP_FCC_STOP_RX\n");
        break;

    case OP_FCC_SET_MAC:
        log_info("OP_FCC_SET_MAC:\n");
        mac = (u8 *)priv;
        log_info("MAC = %X:%X:%X:%X:%X:%X\n", mac[0], mac[1],
                 mac[2], mac[3], mac[4], mac[5]);
        break;

    case OP_FCC_INQ_MAC:
        log_info("OP_FCC_INQ_MAC:\n");
        break;

    case OP_STA_FREQ_ADJ:
        log_info("OP_STA_FREQ_ADJ:\n");
        freq_adj = (struct FREQ_ADJ *)priv;
        log_info("\tchannel       = %d\n", freq_adj->channel);
        log_info("\trate          = %d\n", freq_adj->rate);
        log_info("\tpathx_txpower = %d\n", freq_adj->pathx_txpower);
        log_info("\tthr_max       = %.2f\n", freq_adj->thr_max);
        log_info("\tthr_min       = %.2f\n", freq_adj->thr_min);
        log_info("\tmax_cnt       = %d\n", freq_adj->max_cnt);
        log_info("\tstep          = %d\n", freq_adj->step);
        break;

    case OP_IN_FREQ_ADJ:
        log_info("OP_IN_FREQ_ADJ:\n");
        log_info("\tcur_freq = %.2f\n", *((float *)priv));
        break;

    case OP_STA_PWR_ADJ:
        log_info("OP_STA_PWR_ADJ:\n");
        pwr_adj = (struct PWR_ADJ *)priv;
        log_info("\tchannel = %d\n", pwr_adj->channel);
        log_info("\trate    = %d\n", pwr_adj->rate);
        log_info("\tthr_max = %.2f\n", pwr_adj->thr_max);
        log_info("\tthr_min = %.2f\n", pwr_adj->thr_min);
        log_info("\tmax_cnt = %d\n", pwr_adj->max_cnt);
        log_info("\tstep    = %d\n", pwr_adj->step);
        break;

    case OP_IN_PWR_ADJ:
        log_info("OP_IN_PWR_ADJ:\n");
        log_info("\tcur_pwr  = %.2f\n", *((float *)priv));
        log_info("\tcur_evm  = %.2f\n", *((float *)priv + 1));
        log_info("\tcur_mask = %.2f\n", *((float *)priv + 2));
        break;

    case OP_FCC_INQ_DEF_DATA:
        log_info("OP_FCC_INQ_DEF_DATA:\n");
        break;


    case OP_FCC_SET_PA:
        log_info("OP_FCC_SET_PA:\n");
        pa = (u8 *)priv;
        log_info("\tpa = %d%d%d%d%d%d%d\n", pa[0], pa[1], pa[2], pa[3], pa[4], pa[5], pa[6]);
        break;

    case OP_FCC_SET_CH:
        log_info("OP_FCC_SET_CH:\n");
        log_info("\tCH = %d\n", *(u8 *)priv);
        break;

    case OP_FCC_SET_TX_RATE:
        log_info("OP_FCC_SET_TX_RATE:\n");
        log_info("\trate = %s\n", __THIS->tx_rate_tab[*(u8 *)priv].string);
        break;

    case OP_FCC_SET_TX_GAIN:
        log_info("OP_FCC_SET_TX_GAIN:\n");
        log_info("\tgain = %d, rate = %s\n", *(u8 *)priv, __THIS->tx_rate_tab[__THIS->tx_params.rate].string);
        break;

    case OP_FCC_ENTER_WIFI_SIGN:
        log_info("OP_FCC_ENTER_WIFI_SIGN:\n");
        sign = (struct WIFI_SIGN *)priv;
        log_info("\tssid = %s\n", sign->ssid);
        log_info("\tpwd  = %s\n", sign->pwd);
        log_info("\txosc = %d, %d\n", sign->xosc[0], sign->xosc[1]);
        log_info("\tpa   = %d%d%d%d%d%d%d\n", sign->pa[0], sign->pa[1], \
                 sign->pa[2], sign->pa[3], sign->pa[4], sign->pa[5], sign->pa[6]);
        log_info("\tB_GAIN = %d, %d, %d, %d\n", sign->gain[0], sign->gain[1], sign->gain[2], sign->gain[3]);
        log_info("\tG_GAIN = %d, %d, %d, %d, %d, %d, %d, %d\n", sign->gain[4], sign->gain[5], sign->gain[6], sign->gain[7], \
                 sign->gain[8], sign->gain[9], sign->gain[10], sign->gain[11]);
        log_info("\tN_GAIN = %d, %d, %d, %d, %d, %d, %d, %d\n", sign->gain[12], sign->gain[13], sign->gain[14], sign->gain[15], \
                 sign->gain[16], sign->gain[17], sign->gain[18], sign->gain[19]);
        break;

    case OP_FCC_START_RX_STAT:
        rxs = (struct FCC_RX_STAT *)priv;
        log_info("OP_FCC_START_RX_STAT:\n");
        log_info("\tpa            = %d%d%d%d%d%d%d\n", rxs->pa[0], rxs->pa[1], \
                 rxs->pa[2], rxs->pa[3], rxs->pa[4], rxs->pa[5], rxs->pa[6]);
        log_info("\txosc          = %d, %d\n", rxs->xosc[0], rxs->xosc[1]);
        log_info("\tchannel       = %d\n", rxs->channel);
        log_info("\tbandwidth     = %d\n", rxs->bandwidth);
        log_info("\tshort_gi      = %d\n", rxs->short_gi);
        log_info("\tantenna_x     = %d\n", rxs->antenna_x);
        log_info("\tstat_time     = %d\n", rxs->stat_time);
        log_info("\tfilter_enable = %d\n", rxs->filter_enable);
        log_info("\tfilter_mac    = %x:%x:%x:%x:%x:%x\n", rxs->filter_mac[0], rxs->filter_mac[1], \
                 rxs->filter_mac[2], rxs->filter_mac[3], rxs->filter_mac[4], rxs->filter_mac[5]);
        break;

    case OP_FCC_STOP_RX_STAT:
        log_info("OP_FCC_STOP_RX_STAT\n");
        break;

    case OP_FCC_INQ_RX_STAT:
        log_info("OP_FCC_INQ_RX_STAT\n");
        break;

    case OP_FCC_WIFI_CONN:
        log_info("OP_FCC_WIFI_CONN:\n");
        log_info("\tssid = %s\n", ((struct WIFI_CONN_INFO *)priv)->ssid);
        log_info("\tpwd  = %s\n", ((struct WIFI_CONN_INFO *)priv)->pwd);

        void wifi_adaptivity_start(void);
        wifi_adaptivity_start();

        break;

    default:
        break;
    }
}
#endif


#ifdef CONFIG_NET_ENABLE
static void cal_xosc(float cur_freq, u8 *cur_xosc, u8 step)
{
    for (int i = 0; i < step; i++) {
        if (cur_freq > 0) {
            if (cur_xosc[0] > cur_xosc[1]) {
                cur_xosc[1]++;
                cur_xosc[1] = (cur_xosc[1] > MAX_XOSC) ? MAX_XOSC : cur_xosc[1];
            } else {
                cur_xosc[0]++;
                cur_xosc[0] = (cur_xosc[0] > MAX_XOSC) ? MAX_XOSC : cur_xosc[0];
            }
        } else {
            if (cur_xosc[0] > cur_xosc[1]) {
                cur_xosc[0]--;
                cur_xosc[0] = (cur_xosc[0] < MIN_XOSC) ? MIN_XOSC : cur_xosc[0];
            } else {
                cur_xosc[1]--;
                cur_xosc[1] = (cur_xosc[1] < MIN_XOSC) ? MIN_XOSC : cur_xosc[1];
            }
        }
    }
}


void cal_gain(u8 rate, u8 gain, u8 *gain_array)
{
    s8 offset;
    u8 i, range_l, range_r;

    if (!strcmp(__THIS->tx_rate_tab[rate].string, "11M")) {
        range_l = 0;
        range_r = 3;
    } else if (!strcmp(__THIS->tx_rate_tab[rate].string, "54M")) {
        range_l = 4;
        range_r = 11;
    } else if (!strcmp(__THIS->tx_rate_tab[rate].string, "HTMCS7")) {
        range_l = 12;
        range_r = 19;
    } else {
        gain_array[rate] = gain;
        return;
    }

    offset = gain - gain_array[rate];
    for (i = range_l; i <= range_r; i++) {
        gain_array[i] += offset;
    }
}
#endif


#ifdef CONFIG_BT_ENABLE
u8 fcc_freq_adj_read(s16 *data)
{
    if (rf_fcc_adj_res_read("offset", data)) {
        printf("\n ---  [RF_FCC_ADJ]fre_offset %d ---- \n", *data);
        return 1;
    } else {
        printf("\n ---  [RF_FCC_ADJ]fre_offset NULL ---- \n");
    }
    return 0;
}
#endif

#define FCC_MAC_SIZE (6)

u8 fcc_rf_mac_wr(u8 *type, u8 *mac, u8 is_write)
{
    u8 exist[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8 rscorr, idx, *str[] = {"EDR_WIFI", "BLE"}, temp[FCC_MAC_SIZE];

    for (idx = 0; idx < ARRAY_SIZE(str); idx++) {
        if (!strcmp(type, str[idx])) {
            break;
        }
    }

    switch (idx) {
    case 0:
    case 2:
        if (is_write) {
            if (syscfg_read(CFG_BT_MAC_ADDR, temp, FCC_MAC_SIZE) == FCC_MAC_SIZE) {
                if (!memcmp(temp, mac, FCC_MAC_SIZE) && memcmp(exist, mac, FCC_MAC_SIZE)) {
                    rscorr = 0;
                    break;
                }
            }
            rscorr = syscfg_write(CFG_BT_MAC_ADDR, mac, FCC_MAC_SIZE);
        } else {
            rscorr = syscfg_read(CFG_BT_MAC_ADDR, mac, FCC_MAC_SIZE);
        }
        break;

    case 1:
        if (is_write) {
            if (syscfg_read(CFG_BLE_MAC_ADDR, temp, FCC_MAC_SIZE) == FCC_MAC_SIZE) {
                if (!memcmp(temp, mac, FCC_MAC_SIZE) && memcmp(exist, mac, FCC_MAC_SIZE)) {
                    rscorr = 0;
                    break;
                }
            }
            rscorr = syscfg_write(CFG_BLE_MAC_ADDR, mac, FCC_MAC_SIZE);
        } else {
            rscorr = syscfg_read(CFG_BLE_MAC_ADDR, mac, FCC_MAC_SIZE);
        }
        break;

    default:
        rscorr = 0;
        break;
    }

    return rscorr;
}

u8 *fcc_common_data_deal(u8 packet_type, u8 *data, u32 len, u32 *rsp_len, u8 *reset)
{
    if (!config_rf_test_enable) {
        return NULL;
    }

    u8 mac[FCC_MAC_SIZE];
    s32 ret;
    u8 buf[16] = {0};
    static u8 ready = 0;

    if (len < sizeof(FCC_BT_ARRAY) - 1) {
        *reset = 0;
        *rsp_len = 0;
        return NULL;
    }

    buf[0] = packet_type;
    memcpy(&buf[1], data, sizeof(buf) - 1);

    if (!array_cmp(buf, FCC_WIFI_ARRAY, sizeof(FCC_WIFI_ARRAY))) {
        log_info("COMMON OPCODE : change to FCC_WIFI_MODE");
#ifdef CONFIG_NET_ENABLE
        g_mode_info.mode = FCC_WIFI_MODE;
        rf_fcc_save_change_mode();
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
#else
        log_err("no support WIFI Moudle\n");
        *reset = 0;
        return NULL;
#endif
    } else if (!array_cmp(buf, FCC_BT_ARRAY, sizeof(FCC_BT_ARRAY))) {
        log_info("COMMON OPCODE : change to FCC_BT_MODE");
#ifdef CONFIG_BT_ENABLE
        g_mode_info.mode = FCC_BT_MODE;
        rf_fcc_save_change_mode();
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
#else
        log_err("no support BT Moudle\n");
        *reset = 0;
        return NULL;
#endif
    } else if (!array_cmp(buf, FCC_BT_DUT_ARRAY, sizeof(FCC_BT_DUT_ARRAY))) {
        log_info("COMMON OPCODE : change to FCC_BT_DUT_MODE");
#ifdef CONFIG_BT_ENABLE
        g_mode_info.mode = FCC_BT_DUT_MODE;
        rf_fcc_save_change_mode();
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
#else
        log_err("no support BT DUT Moudle\n");
        *reset = 0;
        return NULL;
#endif
    } else if (!array_cmp(buf, FCC_BT_BQB_ARRAY, sizeof(FCC_BT_BQB_ARRAY))) {
        log_info("COMMON OPCODE : change to FCC_BT_BQB_MODE");
#ifdef CONFIG_BT_ENABLE
        g_mode_info.mode = FCC_BT_BQB_MODE;
        rf_fcc_save_change_mode();
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
#else
        log_err("no support BT BQB Moudle\n");
        *reset = 0;
        return NULL;
#endif
    } else if (!array_cmp(buf, FCC_RESET_ARRAY, sizeof(FCC_RESET_ARRAY))) {
        log_info("COMMON OPCODE : RESET");
        g_mode_info.mode = g_cur_mode;
        rf_fcc_save_change_mode();
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_EXIT_ARRAY, sizeof(FCC_EXIT_ARRAY))) {
        log_info("COMMON OPCODE : EXIT FCC");
        ///TODO
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_HEART_ARRAY, sizeof(FCC_HEART_ARRAY))) {
        /* log_info("COMMON OPCODE : HEART"); */
        putchar('H');
        /* log_info("__THIS->rand_num = %d\n", __THIS->rand_num); */
        memcpy(&FCC_HEART_RSP_ARRAY[6], &__THIS->rand_num, sizeof(__THIS->rand_num));
        *reset = 0;
        *rsp_len = sizeof(FCC_HEART_RSP_ARRAY);
        return FCC_HEART_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_RES_SUCC_ARRAY, sizeof(FCC_RES_SUCC_ARRAY))) {
        log_info("COMMON OPCODE : RES_SUCC");
        read_res();
        fcc_res_handler(true);
#if LOOP_TEST_TRIM
        void clear_efuse(void);
        clear_efuse();
#endif
        struct fcc_info info;
        strcpy(info.res, __THIS->fcc_str[0]);
        info.crc = CRC16(info.res, sizeof(info.res));
        ret = syscfg_write(WIFI_BT_FCC_RES_FLAG, &info, sizeof(struct fcc_info));
        *reset = 0;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        ready = 1;
        return ((ret == sizeof(struct fcc_info)) ? FCC_RSP_ARRAY : FCC_RSP_FAIL_ARRAY);
    } else if (!array_cmp(buf, FCC_RES_FAIL_ARRAY, sizeof(FCC_RES_FAIL_ARRAY))) {
        log_info("COMMON OPCODE : RES_FAIL");
        read_res();
        fcc_res_handler(false);
#if LOOP_TEST_TRIM
        void clear_efuse(void);
        clear_efuse();
#endif
        struct fcc_info info;
        strcpy(info.res, __THIS->fcc_str[1]);
        info.crc = CRC16(info.res, sizeof(info.res));
        ret = syscfg_write(WIFI_BT_FCC_RES_FLAG, &info, sizeof(struct fcc_info));
        *reset = 0;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        ready = 1;
        return ((ret == sizeof(struct fcc_info)) ? FCC_RSP_ARRAY : FCC_RSP_FAIL_ARRAY);
    } else if (!array_cmp(buf, FCC_READY_ARRAY, sizeof(FCC_READY_ARRAY))) {
        log_info("COMMON OPCODE : READY");
        *reset = 0;
        *rsp_len = sizeof(FCC_READY_RSP_ARRAY);
        return (ready ? NULL : FCC_READY_RSP_ARRAY);
    } else if (!array_cmp(buf, FCC_GET_BT_WIFI_MAC_ARRAY, sizeof(FCC_GET_BT_WIFI_MAC_ARRAY))) {
        log_info("COMMON OPCODE : GET_BT_WIFI_MAC");
        fcc_rf_mac_wr("EDR_WIFI", mac, 0);
        *rsp_len = sizeof(FCC_GET_BT_WIFI_MAC_RSP_ARRAY);
        memcpy(FCC_GET_BT_WIFI_MAC_RSP_ARRAY + 7, mac, 6);
        return FCC_GET_BT_WIFI_MAC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_SET_BT_WIFI_MAC_ARRAY, sizeof(FCC_SET_BT_WIFI_MAC_ARRAY))) {
        log_info("COMMON OPCODE : SET_BT_WIFI_MAC");
        memcpy(mac, buf + 10, 6);
        fcc_rf_mac_wr("EDR_WIFI", mac, 1);
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_GET_BLE_MAC_ARRAY, sizeof(FCC_GET_BLE_MAC_ARRAY))) {
        log_info("COMMON OPCODE : GET_BLE_MAC");
        fcc_rf_mac_wr("BLE", mac, 0);
        *rsp_len = sizeof(FCC_GET_BLE_MAC_RSP_ARRAY);
        memcpy(FCC_GET_BLE_MAC_RSP_ARRAY + 7, mac, 6);
        return FCC_GET_BLE_MAC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_SET_BLE_MAC_ARRAY, sizeof(FCC_SET_BLE_MAC_ARRAY))) {
        log_info("COMMON OPCODE : SET_BLE_MAC");
        memcpy(mac, buf + 10, 6);
        fcc_rf_mac_wr("BLE", mac, 1);
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;

    } else {
        *reset = 0;
        *rsp_len = 0;
        return NULL;
    }
    return NULL;
}


#ifdef CONFIG_NET_ENABLE
static void fcc_data_deal_task(void *priv)
{
    s32 ret;
    u8 *xosc;
    u8 mac[6], gain_array[20];
    u8 save_flag, status, cur_dgain, step;
    float cur_freq, cur_pwr;
    static u8 adj_cnt, max_cnt;
    static float thr_max, thr_min;
    u32 offset, val, last_cnt, res;
    static u8 cur_xosc[2];
    struct wifi_mode_info cur_info = {0};
    struct FCC_HIS his = {0};
    struct host_data *host, *n;
    struct fcc_data *fcc;
    struct FCC_TX *tx;
    struct FCC_RX *rx;
    struct FCC_RX_STAT *rxs;
    struct FREQ_ADJ *freq_adj;
    struct PWR_ADJ *pwr_adj;
    struct WIFI_SIGN *sign;
    u8 none_pa[7] = {0};
    static u8 last_pa[7] = {0}, last_ch = 0;
    u8 resp_data[5] = {0};
    struct netif_info netif_info;
    static u8 lcw = FALSE;

    for (;;) {
        if (wifi_is_on()) {
            log_info("wifi_is_on");
            break;
        }
        os_time_dly(10);
    }

    for (;;) {
        os_mutex_pend(&__THIS->list_mutex, 0);
        list_for_each_entry_safe(host, n, &__THIS->head, entry) {
            fcc = &host->fcc_data;
#ifdef CMD_DEBUG
            fcc_data_show(fcc->opcode, fcc->params);
#endif

            if (!__THIS->signalling_mode && fcc->opcode == OP_FCC_ENTER_WIFI_SIGN) {
                goto __get;
            }


            wifi_get_mode_cur_info(&cur_info);
            if (fcc->opcode > OP_IN_WIFI && fcc->opcode < OP_OUT_WIFI && cur_info.mode != MP_TEST_MODE) {
                wifi_enter_mp_test_mode();
                os_time_dly(2);
            }

#if 0 //for cmd need reset
            if (fcc->opcode == OP_FCC_ENTER_TEST) {
                save_flag = (*((u8 *)fcc->params) == FCC_BT_MODE) ? 1 : 0;
                his.mode = FCC_BT_MODE;
            } else {
                save_flag = 0;
            }

            if (save_flag) {
                offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);
                memcpy((u8 *)&his.data, (u8 *)fcc, offset);
                memcpy((u8 *)&his.data + offset, fcc->params, fcc->params_len);
                offset += fcc->params_len;
                memcpy((u8 *)&his.data + offset, &fcc->crc, sizeof(fcc->crc));
                put_buf(&his, sizeof(struct FCC_HIS));

                syscfg_write(CFG_USER_DEFINE_TEST, (u8 *)&his, sizeof(struct FCC_HIS));
                cpu_reset();
            }
#endif

            memset(&__THIS->g_sign_info, 0, sizeof(struct WIFI_SIGN_INFO));
            syscfg_write(WIFI_SIGN_INDEX, &__THIS->g_sign_info, sizeof(struct WIFI_SIGN_INFO));
            if (__THIS->signalling_mode && fcc->opcode != OP_FCC_ENTER_WIFI_SIGN) {
                cpu_reset();
            }

__get:
            if (lcw && (fcc->opcode != OP_FCC_START_TX)) {
                lcw = FALSE;
                wf_tx_sine_test_reset(1);
                wf_phy_rst();
            }

            switch (fcc->opcode) {
            case OP_FCC_INQ_RAND_NUM:
                /* log_info("__THIS->rand_num = %d\n", __THIS->rand_num); */
                report_data(OP_FCC_RPT_RAND_NUM, &__THIS->rand_num, sizeof(__THIS->rand_num));
                break;

            case OP_FCC_START_TX:

                stop_tx_test_data();
                tx = (struct FCC_TX *)fcc->params;
                xosc = ((u8 *)fcc->params) + sizeof(tx->pa);

                if (!__THIS->tx_params.send_interval) {
                    wf_phy_rst();
                }
                wf_write_xosc(xosc);

                //认证模式下使用默认PA
                if (array_cmp(none_pa, tx->pa, sizeof(none_pa))) {
                    set_pa_config_data(tx->pa);
                    wl_anl_actl_init(0);
                }

                wifi_set_channel(tx->channel);
                mp_test_pa_mcs_dgain_set(__THIS->tx_rate_tab[tx->rate].phy, __THIS->tx_rate_tab[tx->rate].mcs, tx->pathx_txpower);

                if (tx->cw_flag) {
#if 0   //TODO 测试复现复位phy层会出现波形不正常
                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val = val | 0x00000002;
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val = val & ~(0x00000002);
                    sdio_mac_wreg(MAC_SYS_CTRL, val);
#endif

                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val &= ~(1 << 3);
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val = val | 0x00000010;
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    if (!lcw) {
                        wf_tx_sine_test_reset(0);
                        lcw = TRUE;
                    }
                    wf_tx_sine_test();
                } else {
                    if (lcw) {
                        lcw = FALSE;
                        wf_tx_sine_test_reset(1);
                        wf_phy_rst();
                    }
                }

                memcpy(&__THIS->tx_params, tx, sizeof(struct FCC_TX));

                if (!(((struct FCC_TX *)tx)->send_interval)) {
                    start_tx_test_data(NULL);
                    os_time_dly(10);
                    stop_tx_test_data();
                    wf_phy_tx_always_test();
                } else {
                    start_tx_test_data(NULL);
                }
                status = ST_SUCC;

                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_STOP_TX:
                if (!__THIS->tx_params.send_interval) {
                    wf_phy_rst();
                }
                stop_tx_test_data();
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_START_RX:
                stop_tx_test_data();
                rx = (struct FCC_RX *)fcc->params;
                xosc = ((u8 *)fcc->params) + sizeof(tx->pa);

                wf_write_xosc(xosc);
                wifi_set_channel(rx->channel);
                wifi_set_frame_cb(wifi_rx_frame_cb, NULL);

                memcpy(&__THIS->rx_params, rx, sizeof(struct FCC_RX));
                __THIS->total_rx_cnt = 0;
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_STOP_RX:
                wifi_set_frame_cb(NULL, NULL);
                report_data(OP_FCC_RPT_RX_RES, &__THIS->total_rx_cnt, sizeof(__THIS->total_rx_cnt));
                log_info("__THIS->total_rx_cnt = %d\n", __THIS->total_rx_cnt);
                __THIS->total_rx_cnt = 0;
                break;

            case OP_FCC_SET_MAC:
                wifi_set_mac(fcc->params);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_INQ_MAC:
                memset(mac, 0, sizeof(mac));
                wifi_get_mac(mac);
                log_info("MAC= %X:%X:%X:%X:%X:%X\n", mac[0], mac[1], \
                         mac[2], mac[3], mac[4], mac[5]);
                report_data(OP_FCC_RPT_MAC, mac, sizeof(mac));
                break;

            case OP_STA_FREQ_ADJ:
                freq_adj = (struct FREQ_ADJ *)fcc->params;

                wf_read_xosc(cur_xosc);
                wifi_set_channel(freq_adj->channel);

                adj_cnt = 0;
                step    = freq_adj->step;
                max_cnt = freq_adj->max_cnt;
                thr_max = freq_adj->thr_max;
                thr_min = freq_adj->thr_min;

                stop_tx_test_data();
                memset(&__THIS->tx_params, 0, sizeof(struct FCC_TX));
                __THIS->tx_params.pathx_txpower = freq_adj->pathx_txpower;
                __THIS->tx_params.rate          = freq_adj->rate;
                __THIS->tx_params.packet_len    = TX_LEN_IN_ADJ;
                __THIS->tx_params.send_interval = TX_INR_IN_ADJ;
                log_info("START_FREQ_ADJ: cur_xosc = %d, %d\n", cur_xosc[0], cur_xosc[1]);
                start_tx_test_data(NULL);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_IN_FREQ_ADJ:
                cur_freq = *((float *)fcc->params);
                adj_cnt++;
                if (adj_cnt > max_cnt || (cur_freq >= thr_min && cur_freq <= thr_max)) {
                    stop_tx_test_data();
                    log_info("STOP_FREQ_ADJ: %d, %d, adj_cnt = %d / %d\n", cur_xosc[0], cur_xosc[1], adj_cnt, max_cnt);
                    status = (cur_freq >= thr_min && cur_freq <= thr_max) ?  ST_SUCC : ST_FAIL;
                    if (status == ST_SUCC) {
                        ret = syscfg_write(VM_XOSC_INDEX, cur_xosc, sizeof(cur_xosc));
                        status = (ret == sizeof(cur_xosc)) ? ST_SUCC : ST_FAIL;
                        log_info("FREQ_ADJ_SAVE: ret = %d, status = %d\n", ret, status);
                    }
                    report_data(OP_FIN_FREQ_ADJ, &status, sizeof(status));
                } else {
                    cal_xosc(cur_freq, cur_xosc, step);
                    wf_write_xosc(cur_xosc);
                    log_info("IN_FREQ_ADJ: %d, %d, adj_cnt = %d / %d\n", cur_xosc[0], cur_xosc[1], adj_cnt, max_cnt);

                    status = ST_SUCC;
                    report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                }
                break;

            case OP_STA_PWR_ADJ:
                pwr_adj = (struct PWR_ADJ *)fcc->params;

                stop_tx_test_data();
                wifi_set_channel(pwr_adj->channel);
                wifi_get_mcs_dgain(gain_array);
                cur_dgain = gain_array[pwr_adj->rate];
                mp_test_pa_mcs_dgain_set(__THIS->tx_rate_tab[pwr_adj->rate].phy, __THIS->tx_rate_tab[pwr_adj->rate].mcs, cur_dgain);

                adj_cnt = 0;
                step    = pwr_adj->step;
                max_cnt = pwr_adj->max_cnt;
                thr_max = pwr_adj->thr_max;
                thr_min = pwr_adj->thr_min;

                memset(&__THIS->tx_params, 0, sizeof(struct FCC_TX));
                __THIS->tx_params.pathx_txpower = cur_dgain;
                __THIS->tx_params.rate = pwr_adj->rate;
                __THIS->tx_params.packet_len    = TX_LEN_IN_ADJ;
                __THIS->tx_params.send_interval = TX_INR_IN_ADJ;
                log_info("START_PWR_ADJ: rate = %s, cur_dgain = %d\n", __THIS->tx_rate_tab[__THIS->tx_params.rate].string, cur_dgain);
                start_tx_test_data(NULL);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_IN_PWR_ADJ:
                cur_pwr = *((float *)fcc->params);
                adj_cnt++;
                if (adj_cnt > max_cnt || (cur_pwr >= thr_min && cur_pwr <= thr_max)) {
                    stop_tx_test_data();
                    log_info("STOP_PWR_ADJ: rate = %s, dgain = %d, adj_cnt = %d / %d\n", __THIS->tx_rate_tab[__THIS->tx_params.rate].string, cur_dgain, adj_cnt, max_cnt);
                    status = (cur_pwr >= thr_min && cur_pwr <= thr_max) ?  ST_SUCC : ST_FAIL;
                    if (status == ST_SUCC) {
                        gain_array[__THIS->tx_params.rate] = cur_dgain;
                        /* cal_gain(__THIS->tx_params.rate, cur_dgain, gain_array); */
                        ret = syscfg_write(VM_WIFI_PA_MCS_DGAIN, gain_array, sizeof(gain_array));
                        status = (ret == sizeof(gain_array)) ? ST_SUCC : ST_FAIL;
                        log_info("PWR_ADJ_SAVE: ret = %d, status = %d\n", ret, status);
                    }
                    report_data(OP_FIN_PWR_ADJ, &status, sizeof(status));
                } else {
                    if (cur_pwr < thr_min) {
                        cur_dgain = (cur_dgain + step) > MAX_DGAIN ? MAX_DGAIN : (cur_dgain + step);
                    } else if (cur_pwr > thr_max) {
                        cur_dgain = (cur_dgain - step) < MIN_DGAIN ? MIN_DGAIN : (cur_dgain - step);
                    }

                    log_info("IN_PWR_ADJ: rate = %s, dgain = %d, adj_cnt = %d / %d\n", __THIS->tx_rate_tab[__THIS->tx_params.rate].string, cur_dgain, adj_cnt, max_cnt);

                    stop_tx_test_data();
                    mp_test_pa_mcs_dgain_set(__THIS->tx_rate_tab[__THIS->tx_params.rate].phy, __THIS->tx_rate_tab[__THIS->tx_params.rate].mcs, cur_dgain);
                    start_tx_test_data(NULL);
                    status = ST_SUCC;
                    report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                }
                break;

            case OP_FCC_INQ_DEF_DATA:
                struct WIFI_DEF_DATA def_data = {0};
                syscfg_read(VM_WIFI_PA_DATA, (u8 *)def_data.pa, sizeof(def_data.pa));
                syscfg_read(VM_XOSC_INDEX, (u8 *)def_data.xosc, sizeof(def_data.xosc));
                syscfg_read(VM_WIFI_PA_MCS_DGAIN, (u8 *)def_data.mcs_dgain, sizeof(def_data.mcs_dgain));
                report_data(OP_FCC_RPT_DEF_DATA, &def_data, sizeof(struct WIFI_DEF_DATA));
                break;

            case OP_FCC_SET_PA:
                if (array_cmp(last_pa, fcc->params, sizeof(last_pa))) {
                    set_pa_config_data(fcc->params);
                    wl_anl_actl_init(0);
                    memcpy(last_pa, fcc->params, sizeof(last_pa));
                } else {
                    log_info("the same PA\n");
                }
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_SET_CH:
                if (last_ch != *(fcc->params)) {
                    last_ch = *(fcc->params);
                    wifi_set_channel(*(fcc->params));
                } else {
                    log_info("the same CH\n");
                }
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_SET_TX_RATE:
                if (__THIS->tx_pid) {
                    stop_tx_test_data();
                    __THIS->tx_params.rate = *(fcc->params);
                    start_tx_test_data(NULL);
                    status = ST_SUCC;
                } else {
                    status = ST_FAIL;
                }
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_SET_TX_GAIN:
                if (__THIS->tx_pid) {
                    mp_test_pa_mcs_dgain_set(__THIS->tx_rate_tab[__THIS->tx_params.rate].phy, __THIS->tx_rate_tab[__THIS->tx_params.rate].mcs, *(fcc->params));
                    status = ST_SUCC;
                } else {
                    status = ST_FAIL;
                }
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_ENTER_WIFI_SIGN:
                sign = (struct WIFI_SIGN *)fcc->params;
                strcpy(__THIS->g_sign_info.str, WIFI_SIGN_STR);
                memcpy(&__THIS->g_sign_info.data, sign, sizeof(struct WIFI_SIGN));
                syscfg_write(WIFI_SIGN_INDEX, &__THIS->g_sign_info, sizeof(struct WIFI_SIGN_INFO));

                if (!__THIS->signalling_mode) {
                    status = ST_SUCC;
                    report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                    cpu_reset();
                }

                strcpy(__THIS->sign_ssid, sign->ssid);
                strcpy(__THIS->sign_pwd, sign->pwd);
                stop_tx_test_data();
                wifi_set_frame_cb(NULL, NULL);
                /* extern u8 ntp_get_time_init; */
                /* ntp_get_time_init = 0; */
                wifi_off();
                wifi_on();

                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));

#ifdef CONFIG_IPERF_ENABLE
                extern void iperf_test(void);
                iperf_test();
#endif
                break;

            case OP_FCC_START_RX_STAT:
                stop_tx_test_data();
                rxs = (struct FCC_RX_STAT *)fcc->params;
                xosc = ((u8 *)fcc->params) + sizeof(tx->pa);

                wf_write_xosc(xosc);
                wifi_set_channel(rxs->channel);
                wifi_set_frame_cb(wifi_rx_frame_cb, NULL);

                memcpy(&__THIS->rx_params, rxs, sizeof(struct FCC_RX));
                last_cnt = 0;
                __THIS->total_rx_cnt = 0;
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_STOP_RX_STAT:
                wifi_set_frame_cb(NULL, NULL);
                report_data(OP_FCC_RPT_RX_RES, &__THIS->total_rx_cnt, sizeof(__THIS->total_rx_cnt));
                log_info("__THIS->total_rx_cnt = %d\n", __THIS->total_rx_cnt);
                __THIS->total_rx_cnt = 0;
                break;

            case OP_FCC_INQ_RX_STAT:
                res = __THIS->total_rx_cnt - last_cnt;
                log_info("RX_STAT: res = %d, now = %d, last = %d\n", res, __THIS->total_rx_cnt, last_cnt);
                last_cnt = __THIS->total_rx_cnt;
                report_data(OP_FCC_RPT_RX_STAT, &res, sizeof(res));
                break;

            case OP_FCC_WIFI_CONN:
#if 0
                stop_tx_test_data();
                wifi_set_frame_cb(NULL, NULL);

                /* extern u8 ntp_get_time_init; */
                /* ntp_get_time_init = 0; */
                wifi_set_connect_sta_block(TRUE);
                wifi_set_sta_connect_timeout(30);
                resp_data[0] = wifi_enter_sta_mode(((struct WIFI_CONN_INFO *)fcc->params)->ssid, ((struct WIFI_CONN_INFO *)fcc->params)->pwd) ? ST_FAIL : ST_SUCC;

                if (resp_data[0] == ST_SUCC) {
                    void lwip_get_netif_info(u8_t lwip_netif, struct netif_info * info);
                    lwip_get_netif_info(WIFI_NETIF, &netif_info);
                    memcpy(&resp_data[1], &netif_info.ip, sizeof(u32));
                }
                report_data(OP_FCC_RPT_ST, &resp_data, sizeof(resp_data));
#else
                sign = (struct WIFI_SIGN *)fcc->params;
                strcpy(__THIS->g_sign_info.str, WIFI_CONN_STR);
                memcpy(&__THIS->g_sign_info.data, sign, sizeof(struct WIFI_SIGN));

                strcpy(__THIS->sign_ssid, sign->ssid);
                strcpy(__THIS->sign_pwd, sign->pwd);
                stop_tx_test_data();
                wifi_set_frame_cb(NULL, NULL);
                /* extern u8 ntp_get_time_init; */
                /* ntp_get_time_init = 0; */
                wifi_off();
                wifi_on();

                resp_data[0] = os_sem_pend(&__THIS->conn_sem, 30 * 100) ? ST_FAIL : ST_SUCC;
                if (resp_data[0] == ST_SUCC) {
                    void lwip_get_netif_info(u8_t lwip_netif, struct netif_info * info);
                    lwip_get_netif_info(WIFI_NETIF, &netif_info);
                    memcpy(&resp_data[1], &netif_info.ip, sizeof(u32));
                }
                report_data(OP_FCC_RPT_ST, &resp_data, sizeof(resp_data));
#endif

#ifdef CONFIG_IPERF_ENABLE
                extern void iperf_test(void);
                iperf_test();
#endif
                break;

            default:
                break;
            }
            list_del(&host->entry);
            free(fcc->params);
            free(host->self);
            break;
        }
        os_mutex_post(&__THIS->list_mutex);
        os_time_dly(1);
    }
}
#endif

u8 get_fcc_info(void)
{
    u8 ret = FCC_NULL;
    struct fcc_info info;

    if (syscfg_read(WIFI_BT_FCC_RES_FLAG, &info, sizeof(struct fcc_info)) == sizeof(struct fcc_info)) {
        if (info.crc == CRC16(info.res, sizeof(info.res))) {
            if (!strcmp(__THIS->fcc_str[0], info.res)) {
                ret = FCC_SUCC;
            } else if (!strcmp(__THIS->fcc_str[1], info.res)) {
                ret = FCC_FAIL;
            }
        }
    }

    log_info("==================FCC adjust %s==================\n", __THIS->fcc_str[ret]);
    return ret;
}


#if LOOP_TEST_TRIM
void clear_efuse(void)
{
    log_info("clear efuse");

    s16 bt_freq;
    FILE *fd = NULL;
    char *buf;
    u8 edr_power, ble_power;
    json_object *new_obj = NULL;
    u8 xosc[2];

    if (!(fd = fopen(SDFILE_RES_ROOT_PATH"cfg/FCC_PROFILE.txt", "r"))) {
        log_info("clear efuse fopen fail, use defaul value to test");
        //没有txt文件情况进行下列默认值进行重置进行循环测试
#ifdef CONFIG_NET_ENABLE
        u8 xosc[2];
        xosc[0] = wifi_calibration_param.xosc_l;
        xosc[1] = wifi_calibration_param.xosc_r;
        syscfg_write(VM_XOSC_INDEX, xosc, 2);
        wf_write_xosc(xosc);
        syscfg_write(VM_WIFI_PA_MCS_DGAIN, (void *)wifi_calibration_param.mcs_dgain, sizeof(wifi_calibration_param.mcs_dgain));
#endif

#ifdef CONFIG_BT_ENABLE
        s16 adj_freq;
        u8 ble_power, bt_power;
        ret = syscfg_read(CFG_BT_RF_POWER_ID, &bt_power, 1);
        if (ret < 0) {
            log_error("clear efuse read edr rf power err");
            bt_power = 10;
        }

        ret = syscfg_read(CFG_BLE_RF_POWER_ID, &ble_power, 1);
        if (ret < 0) {
            log_error("clear efuse read ble rf power err");
            ble_power = 10;
        }

        ret = syscfg_read(CFG_BT_FRE_OFFSET, &adj_freq, 1);
        if (ret < 0) {
            log_error("clear efuse read bt freq offset err");
            adj_freq = -30;
        }

        rf_fcc_adj_res_write("ble", &ble_power);
        rf_fcc_adj_res_write("edr", &bt_power);
        rf_fcc_adj_res_write("offset", &adj_freq);
#endif
        goto _cefuse_exit;
    }

    if (!(buf = zalloc(1024))) {
        log_info("clear efuse buf alloc fail");
        goto _cefuse_exit;
    }

    if (fread(buf, 1024, 1, fd) <= 0) {
        log_info("clear efuse fread fail");
        goto _cefuse_exit;
    }

    printf(" clear efuse read buf = %s\n", buf);
    if (!(new_obj = json_tokener_parse((const char *)buf))) {
        log_info("clear efuse json parse fail");
        goto _cefuse_exit;
    }

#ifdef CONFIG_BT_ENABLE
    if (json_object_object_get(new_obj, "BT_FREQ")) {
        bt_freq = json_object_get_int(json_object_object_get(new_obj, "BT_FREQ"));
        rf_fcc_adj_res_write("offset", &bt_freq);
    }

    if (json_object_object_get(new_obj, "EDR_POWER")) {
        edr_power = json_object_get_int(json_object_object_get(new_obj, "EDR_POWER"));
        rf_fcc_adj_res_write("edr", &edr_power);
        log_info("clear efuse set rf edr_power: %d", edr_power);
    }

    if (json_object_object_get(new_obj, "BLE_POWER")) {
        ble_power = json_object_get_int(json_object_object_get(new_obj, "BLE_POWER"));
        rf_fcc_adj_res_write("ble", &ble_power);
        log_info("clear efuse set rf ble_power: %d", ble_power);
    }

    if (json_object_object_get(new_obj, "EDR_POWER") && json_object_object_get(new_obj, "EDR_POWER")) {
        extern void bt_max_pwr_set(u8 pwr, u8 pg_pwr, u8 iq_pwr, u8 ble_pwr);
        bt_max_pwr_set(edr_power, 6, 6, ble_power);
    }
#endif

#ifdef CONFIG_NET_ENABLE
    if (json_object_object_get(new_obj, "XOSC")) {
        if (ARRAY_SIZE(xosc) == json_object_array_length(json_object_object_get(new_obj, "XOSC"))) {
            for (u8 i = 0; i < json_object_array_length(json_object_object_get(new_obj, "XOSC")); i++) {
                xosc[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(new_obj, "XOSC"), i));
            }
            syscfg_write(VM_XOSC_INDEX, xosc, 2);
            wf_write_xosc(xosc);
            log_info("clear efuse set osc: %d %d", xosc[0], xosc[1]);
        }
    }

    u8 save_gain[20];
    if (json_object_object_get(new_obj, "DGAIN")) {
        if (ARRAY_SIZE(save_gain) == json_object_array_length(json_object_object_get(new_obj, "DGAIN"))) {
            for (u8 i = 0; i < json_object_array_length(json_object_object_get(new_obj, "DGAIN")); i++) {
                save_gain[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(new_obj, "DGAIN"), i));
                log_info("clear efuse set dgain: %d", save_gain[i]);
            }
            syscfg_write(VM_WIFI_PA_MCS_DGAIN, save_gain, sizeof(save_gain));
        }
    }
#endif

_cefuse_exit:
    if (new_obj) {
        json_object_put(new_obj);
    }

    if (buf) {
        free(buf);
    }

    if (fd) {
        fclose(fd);
    }
}
#endif


#ifdef CMD_DEBUG
static void create_cmd_data(u8 opcode, void *data, u32 len)
{
    u16 crc;
    u32 offset;
    u8 buf[160] = {0};
    struct fcc_data *fcc;

    /* log_info("opcode = %d, len = %d\n", opcode, len); */
    ((struct fcc_data *)buf)->mark[0] = 'J';
    ((struct fcc_data *)buf)->mark[1] = 'L';
    ((struct fcc_data *)buf)->opcode = opcode;
    ((struct fcc_data *)buf)->params_len = len;

    offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);
    if (data) {
        memcpy(buf + offset, (u8 *)data, len);
    }

    offset += len;
    crc = CRC16(buf, offset);
    memcpy(buf + offset, &crc, sizeof(fcc->crc));

    offset += sizeof(fcc->crc);
    put_buf(buf, offset);
}


static void create_fcc_cmd_demo(void)
{
    log_info("================OP_FCC_WIFI_CONN================\n");
    struct WIFI_CONN_INFO info = {0};
    strcpy((char *)info.ssid, "GJ1");
    strcpy((char *)info.pwd, "8888888899");
    create_cmd_data(OP_FCC_WIFI_CONN, &info, sizeof(struct WIFI_CONN_INFO));

    log_info("================OP_FCC_ENTER_WIFI_SIGN================\n");
    struct WIFI_SIGN sign = {0};
    strcpy(sign.ssid, "GJ");
    strcpy(sign.pwd, "8888888899");
    memset(sign.xosc, 0x0b, sizeof(sign.xosc));
    memset(sign.pa, 0x07, sizeof(sign.pa));
    memset(sign.gain, 32, sizeof(sign.gain));
    create_cmd_data(OP_FCC_ENTER_WIFI_SIGN, &sign, sizeof(struct WIFI_SIGN));

    log_info("====================OP_FCC_STOP_TX====================\n");
    create_cmd_data(OP_FCC_STOP_TX, NULL, 0);

    log_info("====================OP_FCC_STOP_RX====================\n");
    create_cmd_data(OP_FCC_STOP_RX, NULL, 0);

    log_info("==================OP_FCC_INQ_RAND_NUM=================\n");
    create_cmd_data(OP_FCC_INQ_RAND_NUM, NULL, 0);

    log_info("====================OP_FCC_SET_PA====================\n");
    u8 pa_set[7] = {7, 7, 7, 7, 7, 7, 7};
    create_cmd_data(OP_FCC_SET_PA, pa_set, sizeof(pa_set));

    log_info("====================OP_FCC_SET_CH====================\n");
    u8 ch_set = 7;
    create_cmd_data(OP_FCC_SET_CH, &ch_set, sizeof(ch_set));

    log_info("====================OP_FCC_SET_TX_RATE====================\n");
    u8 rate_set = 1;
    create_cmd_data(OP_FCC_SET_TX_RATE, &rate_set, sizeof(rate_set));

    log_info("====================OP_FCC_SET_TX_GAIN====================\n");
    u8 gain_set = 72;
    create_cmd_data(OP_FCC_SET_TX_GAIN, &gain_set, sizeof(gain_set));

    log_info("====================OP_FCC_SET_MAC====================\n");
    u8 mac[6];
    memset(mac, 0xaa, 6);
    create_cmd_data(OP_FCC_SET_MAC, mac, 6);

    log_info("====================OP_FCC_INQ_MAC====================\n");
    create_cmd_data(OP_FCC_INQ_MAC, NULL, 0);

    log_info("=================OP_FCC_INQ_DEF_DATA==================\n");
    create_cmd_data(OP_FCC_INQ_DEF_DATA, NULL, 0);

    log_info("====================TX_TEST, XOSC=====================\n");
    struct FCC_TX data = {0};

    u8 pa[7] = {1, 7, 4, 7, 9, 1, 7};
    u8 xosc[2] = {11, 10};

    memcpy(data.pa, pa, 7);
    data.channel       = 7;
    data.bandwidth     = 0;
    data.short_gi      = 0;
    data.antenna_x     = 0;
    data.pathx_txpower = 32;
    data.rate          = 3;
    data.npackets      = 0;
    data.packet_len    = 1000;
    data.send_interval = 1000;
    data.cw_flag       = 0;
    for (int i = 0; i < 16; i++) {
        xosc[0] = i;
        xosc[1] = i;
        memcpy(data.xosc, xosc, 2);
        /* log_info("xosc = %d, %d\n", xosc[0], xosc[1]); */
        /* create_cmd_data(OP_FCC_START_TX, &data, sizeof(struct FCC_TX)); */
    }

    log_info("====================TX_TEST, DGAIN=====================\n");
    xosc[0] = 11;
    xosc[1] = 10;
    memcpy(data.pa, pa, 7);
    memcpy(data.xosc, xosc, 2);
    data.channel       = 7;
    data.bandwidth     = 0;
    data.short_gi      = 0;
    data.antenna_x     = 0;
    data.pathx_txpower = 32;
    data.rate          = 3;
    data.npackets      = 0;
    data.packet_len    = 1000;
    data.send_interval = 1000;
    data.cw_flag       = 0;
    for (int i = 0; i < 5; i++) {
        data.pathx_txpower = 32 * i;
        log_info("dgain = %d\n", data.pathx_txpower);
        create_cmd_data(OP_FCC_START_TX, &data, sizeof(struct FCC_TX));
    }


    log_info("========================RX_TEST========================\n");
    struct FCC_RX rx = {0};

    xosc[0] = 11;
    xosc[1] = 10;
    memcpy(rx.pa, pa, 7);
    memcpy(rx.xosc, xosc, 2);
    rx.channel       = 7;
    rx.bandwidth     = 0;
    rx.short_gi      = 0;
    rx.antenna_x     = 0;
    rx.filter_enable = 0;
    create_cmd_data(OP_FCC_START_RX, &rx, sizeof(struct FCC_RX));

    log_info("========================RX_STAT_TEST========================\n");
    struct FCC_RX_STAT rxs = {0};

    xosc[0] = 11;
    xosc[1] = 10;
    memcpy(rxs.pa, pa, 7);
    memcpy(rxs.xosc, xosc, 2);
    rxs.channel       = 7;
    rxs.bandwidth     = 0;
    rxs.short_gi      = 0;
    rxs.antenna_x     = 0;
    rxs.stat_time     = 1;
    rxs.filter_enable = 0;
    create_cmd_data(OP_FCC_START_RX_STAT, &rxs, sizeof(struct FCC_RX_STAT));

    log_info("====================OP_FCC_STOP_RX_STAT====================\n");
    create_cmd_data(OP_FCC_STOP_RX_STAT, NULL, 0);

    log_info("====================OP_FCC_INQ_RX_STAT====================\n");
    create_cmd_data(OP_FCC_INQ_RX_STAT, NULL, 0);

    log_info("====================freq_adj=====================\n");
    float cur_freq;
    struct FREQ_ADJ adj = {0};

    adj.channel   = 7;
    adj.rate      = 3;
    adj.pathx_txpower = 32;
    adj.thr_max   = 5.5;
    adj.thr_min   = -5.5;
    adj.max_cnt   = 3;
    adj.step      = 1;
    create_cmd_data(OP_STA_FREQ_ADJ, &adj, sizeof(struct FREQ_ADJ));

    adj.step      = 2;
    create_cmd_data(OP_STA_FREQ_ADJ, &adj, sizeof(struct FREQ_ADJ));

    cur_freq  = 30.02;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    cur_freq  = 20.03;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    cur_freq  = 10.05;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    cur_freq  = 4.5;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    log_info("=====================pwr_adj=====================\n");
    s8 cur_pwr;
    struct PWR_ADJ pwr_adj = {0};
    float res_buf[3];

    pwr_adj.channel   = 7;
    pwr_adj.rate      = 3;
    pwr_adj.thr_max   = 15.15;
    pwr_adj.thr_min   = 10.10;
    pwr_adj.max_cnt   = 3;
    pwr_adj.step      = 16;
    create_cmd_data(OP_STA_PWR_ADJ, &pwr_adj, sizeof(struct PWR_ADJ));

    res_buf[1] = 66.66;
    res_buf[2] = 77.77;

    res_buf[0] = 2.2;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));

    res_buf[0] = 5.5;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));

    res_buf[0] = 6.6;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));

    res_buf[0] = 12.12;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));
}
#endif


u8 get_rf_fcc_mode(void)
{
    return g_cur_mode;
}


#if (CONFIG_RF_FCC_TRIGGER_MODE == IO_TRIGGER_MODE)
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
#endif


#if (CONFIG_RF_FCC_TRIGGER_MODE == GPCNT_TRIGGER_MODE)
static u8 fcc_enter_gpcnt_trigger(void)
{
    u8 cnt = 0;
    u32 gpcnt = 1;
    void *gpcnt_hd = NULL;

    gpcnt_hd = dev_open("gpcnt", NULL);
    if (!gpcnt_hd) {
        log_info("gpcnt open err\n");
        return 0;
    }

    for (cnt = 0; cnt < CONFIG_RF_FCC_GPCNT_TRIGGER_CNT; cnt++) {
        dev_ioctl(gpcnt_hd, IOCTL_GET_GPCNT, (u32)&gpcnt);
        if (gpcnt < CONFIG_RF_FCC_GPCNT_TRIGGER_FREQ_L || \
            gpcnt > CONFIG_RF_FCC_GPCNT_TRIGGER_FREQ_H) {
            break;
        }
        os_time_dly(1);
    }

    dev_close(gpcnt_hd);
    return (cnt >= CONFIG_RF_FCC_GPCNT_TRIGGER_CNT) ? 1 : 0;
}
#endif


#if (CONFIG_RF_FCC_TRIGGER_MODE == USER_DEF_MODE)
__attribute__((weak))u8 fcc_enter_user_def(void)
{
    return 0;
}
#endif


static void rf_fcc_save_change_mode(void)
{
    u8 check_len;

    strcpy(g_mode_info.str, "change");
    check_len = sizeof(struct fcc_mode) - sizeof(&g_mode_info.crc);
    g_mode_info.crc  = CRC16(&g_mode_info, check_len);
    syscfg_write(WIFI_BT_FCC_CHANGE, &g_mode_info, sizeof(struct fcc_mode));
}


static u8 rf_fcc_get_change_mode(void)
{
    u8 check_len;
    struct fcc_mode info = {0};

    if (syscfg_read(WIFI_BT_FCC_CHANGE, &g_mode_info, sizeof(struct fcc_mode)) == sizeof(struct fcc_mode)) {
        check_len = sizeof(struct fcc_mode) - sizeof(&g_mode_info.crc);
        if (CRC16(&g_mode_info, check_len) == g_mode_info.crc) {
            g_cur_mode = g_mode_info.mode;
            log_info("%s, g_cur_mode = %d\n", __FUNCTION__, g_cur_mode);
#ifndef RF_FCC_MODE_KEEP
            syscfg_write(WIFI_BT_FCC_CHANGE, &info, sizeof(struct fcc_mode));
#endif
            return g_cur_mode;
        }
    }
    return 0;
}


#ifdef CONFIG_FCC_BT_ADJ_ENABLE
struct fcc_mode *rf_fcc_bt_adj_info(void)
{
    return &g_mode_info;
}
#endif


static int get_gpio(const char *p)
{
    u32 port;
    if (p[0] == 'P') {
        if (p[1] >= 'A' && p[1] <= 'E') {
            port = p[1] - 'A';
            port *= IO_GROUP_NUM;
        } else if (p[1] == 'F') {
            /* return IO_PORTF_00; */
        } else {
            return -EINVAL;
        }
        int mask = (p[2] - '0') * 10 + (p[3] - '0');
        if (mask < 16) {
            port += mask;
            return port;
        }
    }
    return -EINVAL;
}


static void fcc_cfg_parse(void)
{
    s16 bt_freq;
    u8 edr_power, ble_power;
    FILE *fd = NULL;
    char *buf;
    const char *str;
    json_object *new_obj = NULL;
    int gpio;

    if (!(buf = zalloc(1024))) {
        goto _parse_exit_;
    }

    if (!(fd = fopen(SDFILE_RES_ROOT_PATH"cfg/FCC_PROFILE.txt", "r"))) {
        goto _parse_exit_;
    }

    if (fread(buf, 1024, 1, fd) <= 0) {
        goto _parse_exit_;
    }

    printf("buf = %s\n", buf);
    if (!(new_obj = json_tokener_parse((const char *)buf))) {
        goto _parse_exit_;
    }

#ifdef CONFIG_DEBUG_ENABLE
    if (json_object_object_get(new_obj, "DEBUG_PORT")) {
        str = json_object_get_string(json_object_object_get(new_obj, "DEBUG_PORT"));
        if (str) {
            gpio = get_gpio(str);
            if (gpio >= 0) {
                gpio_enable_function(gpio, GPIO_FUNC_UART0_TX, 1);
            }
        }
    }
#endif

#ifdef CONFIG_BT_ENABLE
    if (!rf_fcc_adj_res_read("offset", &bt_freq) && json_object_object_get(new_obj, "BT_FREQ")) {
        bt_freq = json_object_get_int(json_object_object_get(new_obj, "BT_FREQ"));
        rf_fcc_adj_res_write("offset", &bt_freq);
    }

    if (!rf_fcc_adj_res_read("edr", &edr_power) && json_object_object_get(new_obj, "EDR_POWER")) {
        edr_power = json_object_get_int(json_object_object_get(new_obj, "EDR_POWER"));
        rf_fcc_adj_res_write("edr", &edr_power);
        if (!rf_fcc_adj_res_read("ble", &ble_power) && json_object_object_get(new_obj, "BLE_POWER")) {
            ble_power = json_object_get_int(json_object_object_get(new_obj, "BLE_POWER"));
            rf_fcc_adj_res_write("ble", &ble_power);
            extern void bt_max_pwr_set(u8 pwr, u8 pg_pwr, u8 iq_pwr, u8 ble_pwr);
            bt_max_pwr_set(edr_power, 6, 6, ble_power);
            log_info("set rf edr_power: %d, ble_power: %d", edr_power, ble_power);
        }
    }
#endif

    if (json_object_object_get(new_obj, "ANALOG")) {
        __THIS->ana_pw = json_object_get_int(json_object_object_get(new_obj, "ANALOG"));
        log_info("set analog power: %d", __THIS->ana_pw);
    }

    u8 save_gain[20];
    if (syscfg_read(VM_WIFI_PA_MCS_DGAIN, save_gain, sizeof(save_gain)) != sizeof(save_gain) && json_object_object_get(new_obj, "DGAIN")) {
        if (ARRAY_SIZE(save_gain) == json_object_array_length(json_object_object_get(new_obj, "DGAIN"))) {
            for (u8 i = 0; i < json_object_array_length(json_object_object_get(new_obj, "DGAIN")); i++) {
                save_gain[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(new_obj, "DGAIN"), i));
                log_info("set dgain: %d", save_gain[i]);
            }
            syscfg_write(VM_WIFI_PA_MCS_DGAIN, save_gain, sizeof(save_gain));
        }
    }

    u8 xosc[2], pa[7];
    if (syscfg_read(VM_XOSC_INDEX, xosc, sizeof(xosc)) != sizeof(xosc) && json_object_object_get(new_obj, "XOSC")) {
        if (ARRAY_SIZE(xosc) == json_object_array_length(json_object_object_get(new_obj, "XOSC"))) {
            for (u8 i = 0; i < json_object_array_length(json_object_object_get(new_obj, "XOSC")); i++) {
                xosc[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(new_obj, "XOSC"), i));
            }
            syscfg_write(VM_XOSC_INDEX, xosc, 2);
            wf_write_xosc(xosc);
            log_info("set osc: %d %d", xosc[0], xosc[1]);
        }
    }

    if (syscfg_read(VM_WIFI_PA_DATA, pa, sizeof(pa)) != sizeof(pa) && json_object_object_get(new_obj, "PA")) {
        if (ARRAY_SIZE(pa) == json_object_array_length(json_object_object_get(new_obj, "PA"))) {
            for (u8 i = 0; i < ARRAY_SIZE(pa); i++) {
                pa[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(new_obj, "PA"), i));
            }
            syscfg_write(VM_WIFI_PA_DATA, pa, sizeof(pa));
            set_pa_config_data(pa);
            log_info("set pa: %d %d %d %d %d %d %d", pa[0], pa[1], pa[2], pa[3], pa[4], pa[5], pa[6]);
        }
    }

    if (json_object_object_get(new_obj, "PAEN")) {
        str = json_object_get_string(json_object_object_get(new_obj, "PAEN"));
        if (str) {
            gpio = get_gpio(str);
            if (gpio >= 0) {
                gpio_set_mode(gpio, GPIO_OUTPUT_LOW);
                gpio_och_sel_output_signal(gpio, OUTPUT_CH_SIGNAL_WL_AMPE);
            }
        }

        extern char WIFI_PA_ENABLE;
        WIFI_PA_ENABLE = 1;
    }

    if (json_object_object_get(new_obj, "LANEN")) {
        str = json_object_get_string(json_object_object_get(new_obj, "LANEN"));
        if (str) {
            gpio = get_gpio(str);
            if (gpio >= 0) {
                gpio_set_mode(gpio, GPIO_OUTPUT_LOW);
                gpio_och_sel_output_signal(gpio, OUTPUT_CH_SIGNAL_WL_LNAE);
            }
        }
    }

_parse_exit_:
    if (new_obj) {
        json_object_put(new_obj);
    }

    if (buf) {
        free(buf);
    }

    if (fd) {
        fclose(fd);
    }
}

u8 rf_fcc_test_init(void)
{
    u8 mode;
    u8 mac[6] = {0};
    struct FCC_HIS his = {0};
    struct host_data *host;
    struct fcc_data *fcc;

    if (!config_rf_test_enable) {
        return 0;
    }

    u8 fcc_str[3][8] = {
        {"SUCC"},
        {"FAIL"},
        {"NULL"},
    };

    struct rate_info tx_rate_tab[] = {
        {"1M",		0,		0},
        {"2M",		0,		1},
        {"5.5M",	0,		2},
        {"11M",		0,		3},

        {"6M",		1,		0},
        {"9M",		1,		1},
        {"12M",		1,		2},
        {"18M",		1,		3},
        {"24M",		1,		4},
        {"36M",		1,		5},
        {"48M",		1,		6},
        {"54M",		1,		7},

        {"HTMCS0",	2,		0},
        {"HTMCS1",	2,		1},
        {"HTMCS2",	2,		2},
        {"HTMCS3",	2,		3},
        {"HTMCS4",	2,		4},
        {"HTMCS5",	2,		5},
        {"HTMCS6",	2,		6},
        {"HTMCS7",	2,		7},
    };

#if 1
    u8 wifi_head[] = {
        0xc6, 0x00, 0x00, 0x04, 0xB0, 0x00, 0x04, 0x80, 0x35, 0x01, 0xB6, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*保留一些字节*/
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
#else
    u8 wifi_head[] = {
        0xc6, 0x00, 0x00, 0x04, 0xB0, 0x00, 0x04, 0x80,
        0x35, 0x01, 0xB6, 0x40, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x88, 0x88,
        /*dst*/     0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
        /*src*/     0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
        /*BSSID*/   0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
        /*Seq,Frag num*/                    0x88, 0x88,
    };
#endif

    if (!(mode = rf_fcc_get_change_mode())) {
#if (CONFIG_RF_FCC_TRIGGER_MODE == IO_TRIGGER_MODE)
        if (!fcc_enter_io_trigger()) {
            return 0;
        }
#elif (CONFIG_RF_FCC_TRIGGER_MODE == GPCNT_TRIGGER_MODE)
        if (!fcc_enter_gpcnt_trigger()) {
            return 0;
        }
#elif (CONFIG_RF_FCC_TRIGGER_MODE == USER_DEF_MODE)
        if (!fcc_enter_user_def()) {
            return 0;
        }
#elif (CONFIG_RF_FCC_TRIGGER_MODE == AUTO_STARTUP_MODE)
#else
        return 0;
#endif
    }

    __THIS = (struct ctl_params *)zalloc(sizeof(struct ctl_params));
    ASSERT(__THIS);
    memcpy(__THIS->wifi_send_pkg, wifi_head, sizeof(wifi_head));
    memcpy(__THIS->fcc_str, fcc_str, sizeof(fcc_str));
    memcpy(__THIS->tx_rate_tab, tx_rate_tab, sizeof(tx_rate_tab));

    __THIS->ana_pw = -1;
    fcc_cfg_parse();

#ifdef CONFIG_NET_ENABLE
    if (syscfg_read(WIFI_SIGN_INDEX, &__THIS->g_sign_info, sizeof(struct WIFI_SIGN_INFO)) == sizeof(struct WIFI_SIGN_INFO)) {
        if (!strcmp(__THIS->g_sign_info.str, WIFI_SIGN_STR) && (!mode || mode == FCC_WIFI_MODE)) {
            log_info("%s, %s\n", __FUNCTION__, __THIS->g_sign_info.str);
            mode = FCC_WIFI_MODE;
            g_cur_mode = FCC_WIFI_MODE;
            __THIS->signalling_mode = 1;
#ifdef CMD_DEBUG
            fcc_data_show(OP_FCC_ENTER_WIFI_SIGN, &__THIS->g_sign_info.data);
#endif
            SignalingModeSet(1);
        }
    }
#endif

    get_fcc_info();
    /* create_fcc_cmd_demo(); */
    __THIS->rand_num = random32(0) & 2147483647;
    log_info("__THIS->rand_num = %u\n", __THIS->rand_num);

    INIT_LIST_HEAD(&__THIS->head);

#if 0 //for cmd need reset
    int read_ret = syscfg_read(CFG_USER_DEFINE_TEST, (u8 *)&his, sizeof(struct FCC_HIS));
    if (read_ret == sizeof(struct FCC_HIS)) {
        put_buf(&his, sizeof(struct FCC_HIS));
        if (his.mode == FCC_WIFI_MODE) {
            host = zalloc(sizeof(struct host_data));
            host->flag = 1;
            host->self = host;
            fcc = &host->fcc_data;

            memcpy((u8 *) & (fcc->mark),       his.data,  sizeof(fcc->mark));
            memcpy((u8 *) & (fcc->opcode),     his.data + sizeof(fcc->mark), sizeof(fcc->opcode));
            memcpy((u8 *) & (fcc->params_len), his.data + sizeof(fcc->mark) + sizeof(fcc->opcode), sizeof(fcc->params_len));
            memcpy((u8 *) & (fcc->crc),        his.data + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len) + fcc->params_len, sizeof(fcc->crc));

            fcc->params = (u8 *)zalloc(fcc->params_len);
            memcpy(fcc->params, his.data + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len), fcc->params_len);

            list_add_tail(&host->entry, &__THIS->head);
            g_cur_mode = FCC_WIFI_MODE;
        } else if (his.mode == FCC_BT_MODE) {
            g_cur_mode = FCC_BT_MODE;
        }

        memset(&his, 0xaa, sizeof(struct FCC_HIS));
        int ret = syscfg_write(CFG_USER_DEFINE_TEST, &his, sizeof(struct FCC_HIS));
        log_info("syscfg_write, ret = %d\n", ret);
        if (g_cur_mode == FCC_BT_MODE) {
            return 0;
        }
    }
#endif

#ifdef FCC_USB_COMM
    os_sem_create(&__THIS->cdc_sem, 0);
#endif

    comm_dev_init();
    thread_fork("comm_dev_recv_task", 28, 256, 0, NULL, comm_dev_recv_task, NULL);

    if (mode == FCC_BT_MODE) {
#ifdef CONFIG_BT_ENABLE
#ifdef CONFIG_NET_ENABLE
        wifi_set_mac(mac);
#endif
        config_btctler_mode = BT_FCC;
        __THIS->test_mode = BT_FCC;
        config_btctler_hci_standard = 1;
        fcc_bt_ble_module_init();
        return FCC_BT_MODE;
#else
        return 0;
#endif
    } else if (mode == FCC_BT_DUT_MODE) {
#ifdef CONFIG_BT_ENABLE
#ifdef CONFIG_NET_ENABLE
        wifi_set_mac(mac);
#endif
        config_btctler_mode = BT_NORMAL;
        __THIS->test_mode = 3;
        config_btctler_hci_standard = 0;
        fcc_bt_ble_module_init();
        bredr_set_dut_enble(1, 1);
        return FCC_BT_MODE;
#else
        return 0;
#endif
    } else if (mode == FCC_BT_BQB_MODE) {
#ifdef CONFIG_BT_ENABLE
#ifdef CONFIG_NET_ENABLE
        wifi_set_mac(mac);
#endif
        config_btctler_mode = BT_BQB;
        __THIS->test_mode = BT_BQB;
        config_btctler_hci_standard = 1;
        fcc_bt_ble_module_init();
        return FCC_BT_MODE;
#else
        return 0;
#endif
    } else if (mode == FCC_BT_ADJ_MODE) {
#if (defined CONFIG_BT_ENABLE) && (defined CONFIG_FCC_BT_ADJ_ENABLE)
#ifdef CONFIG_NET_ENABLE
        wifi_set_mac(mac);
#endif
        extern int CONFIG_DISTURB_SCAN_ENABLE;
        CONFIG_DISTURB_SCAN_ENABLE = 1;
        config_btctler_mode = BT_NORMAL;
        __THIS->test_mode = 0;
        config_btctler_hci_standard = 1;
        if (g_mode_info.params[0]) {
            /* set_search_bd_name(&g_mode_info.params[1]); */
            bt_set_local_name("JL-AC79XX-EDR_TSET");
        } else {
            bt_set_local_name(&g_mode_info.params[1]);
        }
        fcc_bt_ble_module_init();
        return FCC_BT_ADJ_MODE;
#else
        return 0;
#endif
    } else if (mode == FCC_WIFI_MODE) {
#ifdef CONFIG_NET_ENABLE
        if (!__THIS->signalling_mode) {
            __THIS->test_mode = 1;
        } else {
            __THIS->test_mode = 0xff;
            clk_set("hsb", 96000000);
            clk_set("sys", 192000000);
        }
        //wifi_set_sta_connect_timeout(10000);
        wifi_set_event_callback(wifi_event_callback);
        /* CHL_BUSY_CONFIG = (0xe & 0x0f); //0xe | 0xc ;open close */
        wifi_on();

        __THIS->tx_params.send_interval = 10;

        os_sem_create(&__THIS->conn_sem, 0);
        os_mutex_create(&__THIS->list_mutex);
        thread_fork("fcc_data_deal_task", 26, 1024, 0, NULL, fcc_data_deal_task, NULL);

        if (__THIS->ana_pw > 0) {
            extern void wifi_set_pwr(unsigned char pwr_sel);
            wifi_set_pwr(__THIS->ana_pw);
        }

        return FCC_WIFI_MODE;
#else
        return 0;
#endif
    }

#if ((CONFIG_RF_FCC_TRIGGER_MODE == IO_TRIGGER_MODE) || \
     (CONFIG_RF_FCC_TRIGGER_MODE == GPCNT_TRIGGER_MODE) || \
     (CONFIG_RF_FCC_TRIGGER_MODE == USER_DEF_MODE) || \
     (CONFIG_RF_FCC_TRIGGER_MODE == AUTO_STARTUP_MODE))
    return FCC_WAIT_MODE;
#endif
    return 0;
}


#ifdef CONFIG_NET_ENABLE
void wifi_tx_data_test(u8 channel, u8 power, u8 rate, u8 *packet, u32 packet_len, u32 npackets, u32 tx_interval)
{
    if (packet_len > 1513 || packet_len < 1) {
        log_err("%s, params err, packet_len = %d\n", __FUNCTION__, packet_len);
    }

    stop_tx_test_data();

    memset(&__THIS->tx_params, 0, sizeof(struct FCC_TX));
    __THIS->tx_params.rate          = rate;
    __THIS->tx_params.packet_len    = packet_len;
    __THIS->tx_params.npackets      = npackets;
    __THIS->tx_params.send_interval = tx_interval;

    wifi_set_channel(channel);
    mp_test_pa_mcs_dgain_set(__THIS->tx_rate_tab[rate].phy, __THIS->tx_rate_tab[rate].mcs, power);
    start_tx_test_data(packet);
}
#endif


static void read_res(void)
{

#ifdef CONFIG_BT_ENABLE
    u8 bt_power = 0, ble_power = 0;
    rf_fcc_adj_res_read("edr", &bt_power);
    rf_fcc_adj_res_read("ble", &ble_power);
#endif

#ifdef CONFIG_NET_ENABLE
    struct wifi_calibration_param cal = {0};
    syscfg_read(VM_XOSC_INDEX, &cal.xosc_l, 2);
    syscfg_read(VM_WIFI_PA_MCS_DGAIN, &cal.mcs_dgain, sizeof(cal.mcs_dgain));
    syscfg_read(VM_WIFI_PA_DATA, &cal.pa_trim_data, sizeof(cal.pa_trim_data));
#endif
    log_info("=========================CAL_RES=========================\n");
#ifdef CONFIG_BT_ENABLE
    log_info("bt_power = %d, ble_power = %d\n", bt_power, ble_power);
#endif

#ifdef CONFIG_NET_ENABLE
    log_info("xosc_l = %d, xosc_r = %d\n", cal.xosc_l, cal.xosc_r);
    log_info("PA : \n");
    put_buf(&cal.pa_trim_data, sizeof(cal.pa_trim_data));
    log_info("MCS_GAIN : \n");
    put_buf(&cal.mcs_dgain, sizeof(cal.mcs_dgain));
    log_info("=========================CAL_RES=========================\n");
#endif
}


#ifdef CONFIG_NET_ENABLE
#include "sock_api/sock_api.h"

static void udp_recv_handler(void *socket_fd)
{
    static u32 sock_rcv_cnt;
    struct sockaddr_in remote_addr;
    socklen_t len = sizeof(remote_addr);
    int recv_len;
    u8 recv_buf[4];
    u32 start_time = 0;
    u32 debug_flag = 0;

    for (;;) {
        recv_len = sock_recvfrom(socket_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&remote_addr, &len);

        if (recv_len > 0) {

            if (start_time == 0) {
                start_time = timer_get_ms();
            }

            ++sock_rcv_cnt;
        }

        if (timer_get_ms() - start_time >= 30 * 1000) {
            if (debug_flag == 0) {
                debug_flag = 1;
                printf("\r\n\r\n STA MODE UDP RECV PKG CNT = %d\r\n\r\n", sock_rcv_cnt);
            }
        }
    }
}


static int udp_server_init(int port)
{
    static void *socket_fd;
    struct sockaddr_in local_addr = {0};

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port);

    socket_fd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (socket_fd == NULL) {
        printf("%s build socket fail\n",  __FILE__);
        return -1;
    }

    if (sock_bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr))) {
        printf("%s sock_bind fail\n", __FILE__);
        return -1;
    }

    sock_set_recv_timeout(socket_fd, 3000);

    //创建线程，用于接收tcp_client的数据
    if (thread_fork("udp_recv_handler", 25, 512, 0, NULL, udp_recv_handler, socket_fd) != OS_NO_ERR) {
        printf("%s thread fork fail\n", __FILE__);
        return -1;
    }

    return 0;
}
#endif


__attribute__((weak))void fcc_res_handler(u8 res)
{
    log_info("%s, res = %d\n", __func__, res);
    if (res == true) {
        gpio_direction_output(CONFIG_RF_FCC_TRIGGER_IO_PORT, 1);
    } else {
        gpio_direction_output(CONFIG_RF_FCC_TRIGGER_IO_PORT, 0);
    }
}


#ifdef CONFIG_NET_ENABLE
u8 fcc_wifi_sign_dgain_set(u8 phy, u8 mcs)
{
    if (!config_rf_test_enable) {
        return 0;
    }

    u8 idx;
    if (!strcmp(__THIS->g_sign_info.str, WIFI_SIGN_STR)) {
        for (idx = 0; idx < ARRAY_SIZE(__THIS->tx_rate_tab); idx++) {
            if (__THIS->tx_rate_tab[idx].phy == phy && __THIS->tx_rate_tab[idx].mcs == mcs) {
                break;
            }
        }

        if (idx >= ARRAY_SIZE(__THIS->tx_rate_tab)) {
            return FALSE;
        }

        printf("%s, dgain = %d\n", __THIS->tx_rate_tab[idx].string, __THIS->g_sign_info.data.gain[idx]);
        mp_test_pa_mcs_dgain_set(phy, mcs, __THIS->g_sign_info.data.gain[idx]);
        return TRUE;
    }
    return FALSE;
}
#endif


sec(.volatile_ram_code)
u8 is_fcc_auth(void)
{
#if TCFG_RF_PRODUCT_TEST_ENABLE
    return 0;
#endif
    if (!__THIS) {
        return 0;
    }
    return __THIS->test_mode;
}

u8 wifi_trim_is_always_enable(void)
{
    return config_rf_test_enable;
}

#endif

