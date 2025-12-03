#include "rf_fcc_main.h"

static u8 mp_test_status;
static void *wifi_dev = NULL;

extern int set_mp_channel(int channel);//Set channel to 1 . 2, 3, 4~13 etc.
extern void msleep(unsigned int ms);
extern unsigned int time(unsigned int *timer);


static int network_user_callback(void *network_ctx, enum WIFI_EVENT state, void *priv)
{
    struct cfg_info info;

    switch (state) {

    case WIFI_EVENT_MODULE_INIT:
        puts("|network_user_callback->WIFI_EVENT_MODULE_INIT\n");
//wifi module port seting
        info.port_status = 0;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);
        msleep(10);
        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);

        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_CS, (u32)&info);

        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_WKUP, (u32)&info);

        msleep(100);
        break;

    case WIFI_EVENT_MODULE_START:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START\n");
        info.ssid = "mmmmmmmm";
        info.pwd = "11111111111";
        info.mode = AP_MODE;
        info.force_default_mode = 1;
        dev_ioctl(wifi_dev, DEV_SAVE_DEFAULT_MODE, (u32)&info);
        break;

    case WIFI_EVENT_MODULE_STOP:
        puts("|network_user_callback->WIFI_EVENT_MODULE_STOP\n");
        break;

    case WIFI_EVENT_AP_START:
        puts("|network_user_callback->WIFI_EVENT_AP_START\n");
        break;

    case WIFI_EVENT_AP_STOP:
        puts("|network_user_callback->WIFI_EVENT_AP_STOP\n");
        break;

    case WIFI_EVENT_STA_START:
        puts("|network_user_callback->WIFI_EVENT_STA_START\n");
        break;

    case WIFI_EVENT_MODULE_START_ERR:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START_ERR\n");
        break;

    case WIFI_EVENT_STA_STOP:
        puts("|network_user_callback->WIFI_EVENT_STA_STOP\n");
        break;

    case WIFI_EVENT_STA_DISCONNECT:
        puts("|network_user_callback->WIFI_STA_DISCONNECT\n");
        break;

    case WIFI_EVENT_STA_SCAN_COMPLETED:
        puts("|network_user_callback->WIFI_STA_SCAN_COMPLETED\n");
        break;

    case WIFI_EVENT_STA_CONNECT_SUCC:
        puts("|network_user_callback->WIFI_EVENT_STA_CONNECT_SUCC\n");
        break;

    case WIFI_EVENT_MP_TEST_START:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_START\n");
        break;

    case WIFI_EVENT_MP_TEST_STOP:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_STOP\n");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID\n");
        break;

    case WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL .....\n");
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC\n");
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_TIMEOUT\n");
        break;

    case WIFI_EVENT_P2P_START:
        puts("|network_user_callback->WIFI_EVENT_P2P_START\n");
        break;

    case WIFI_EVENT_P2P_STOP:
        puts("|network_user_callback->WIFI_EVENT_P2P_STOP\n");
        break;

    case WIFI_EVENT_P2P_GC_DISCONNECTED:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_DISCONNECTED\n");
        break;

    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC\n");
        break;

    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT\n");
        break;

    case WIFI_EVENT_SMP_CFG_START:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_START\n");
        break;

    case WIFI_EVENT_SMP_CFG_STOP:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_STOP\n");
        break;

    case WIFI_EVENT_SMP_CFG_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_TIMEOUT\n");
        break;

    case WIFI_EVENT_SMP_CFG_COMPLETED:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED\n");
        break;

    case WIFI_EVENT_PM_SUSPEND:
        puts("|network_user_callback->WIFI_EVENT_PM_SUSPEND\n");
        break;

    case WIFI_EVENT_PM_RESUME:
        puts("|network_user_callback->WIFI_EVENT_PM_RESUME\n");
        break;

    case WIFI_EVENT_AP_ON_ASSOC:
        puts("|network_user_callback->WIFI_EVENT_AP_ON_ASSOC\n");
        break;

    case WIFI_EVENT_AP_ON_DISCONNECTED:
        puts("|network_user_callback->WIFI_EVENT_AP_ON_DISCONNECTED\n");
        break;

    default:
        break;
    }
    return 0;
}
void wifi_fcc_moudle_init(void)
{
    static u8 init = 0;
    struct cfg_info info = {0};

    if (init) {
        return;
    }

    init = 1;
    wifi_dev = dev_open("wifi", NULL);
    info.cb = network_user_callback;
    info.net_priv = NULL;
    dev_ioctl(wifi_dev, DEV_SET_CB_FUNC, (u32)&info);

    info.tx_pwr_lmt_enable = 0;//  解除WIFI发送功率限制
    dev_ioctl(wifi_dev, DEV_SET_WIFI_TX_PWR_LMT_ENABLE, (u32)&info);
    info.tx_pwr_by_rate = 1;// 设置WIFI根据不同datarate打不同power
    dev_ioctl(wifi_dev, DEV_SET_WIFI_TX_PWR_BY_RATE, (u32)&info);
    dev_ioctl(wifi_dev, DEV_NETWORK_START, 0);
}


void wifi_set_mp_test_status(u8 status)
{
    mp_test_status = status;
}

u8 wifi_get_mp_test_status(void)
{
    return mp_test_status;
}


static void time_delay_ticks(u32 ticks)
{
    for (u32 i = 0; i < ticks; i++) {
        if (wifi_get_mp_test_status()) {
            break;
        } else {
            os_time_dly(1);
        }
    }
}

/*************************************************************************************************************************************/

/**
@brief: 无间断调制讯号发送测试
    @param: mp_channel:1 , 2, 3, 4~13 etc.
            bandwidth: 0(20M)/1(40M)/2(80M)
            short_gi:0(long_GI),1(short_GI)
            antenna_x: "a" or "b" or "ab"
            patha_txpower:0~63
            pathb_txpower:0~63
            mp_rate: set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108;N Rate: MCS0 = 128,MCS1 = 129,MCS 2=130....MCS15 = 143 etc.
            tx_durtime: how mang seconds duration

    @note:  default example: Continuous_Tx_testing(10, 0, 0, "a", 44,108,10);

ifconfig wlan0 up							# Enable Device for MP operation
iwpriv wlan0 mp_start							# enter MP mode
iwpriv wlan0 mp_channel 1						# set channel to 1 . 2, 3, 4~13 etc.
iwpriv wlan0 mp_bandwidth 40M=0,shortGI=0				# set 20M mode and long GI,set 40M is 40M=1.
iwpriv wlan0 mp_ant_tx a						# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
iwpriv wlan0 mp_txpower patha=44,pathb=44				# set path A and path B Tx power level,the Range is 0~63.
iwpriv wlan0 mp_rate 108						# set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108;N Rate: MCS0 = 128,MCS1 = 129,MCS 2=130....MCS15 = 143 etc.
iwpriv wlan0 mp_ctx background						# start continuous Tx
iwpriv wlan0 mp_ctx stop						# stop continuous Tx
iwpriv wlan0 mp_stop							# exit MP mode
ifconfig wlan0 down							# close WLAN interface
*/
int WiFi_Continuous_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime)
{
    int ret;

    ret = mp_start(1);
    if (ret) {
        return ret;
    }

    ret = set_mp_channel(mp_channel);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_bandwidth(bandwidth, short_gi);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_ant_tx(antenna_x);
    if (ret) {
        goto ERR_EXIT;
    }

    if (pathx_txpower != 0) {
        ret = mp_set_tx_power(pathx_txpower, pathx_txpower, pathx_txpower, pathx_txpower);
    }
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_rate(mp_rate);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_reset_stats();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_con_tx();
    if (ret) {
        goto ERR_EXIT;
    }

    while (tx_durtime--) {
        if (wifi_get_mp_test_status() == 1) {
            break;
        }

        time_delay_ticks(100);

//        ret = mp_query(NULL, NULL);
//        if(ret)
//            goto ERR_EXIT;
    }

    ret = mp_ctx_stop();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    mp_start(0);

    return -1;
}


/**
@brief: 不限封包数量调制讯号发送测试
    @param: mp_channel:1 , 2, 3, 4~13 etc.
            bandwidth: 0(20M)/1(40M)/2(80M)
            short_gi:0(long_GI),1(short_GI)
            antenna_x: "a" or "b" or "ab"
            patha_txpower:0~63
            pathb_txpower:0~63
            mp_rate: set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108;N Rate: MCS0 = 128,MCS1 = 129,MCS 2=130....MCS15 = 143 etc.
            tx_durtime: how mang seconds duration

    @note:  default example: Continuous_Packet_Tx_testing(10, 0, 0, "a", 44,108,10);

ifconfig wlan0 up							# Enable Device for MP operation
iwpriv wlan0 mp_start							# enter MP mode
iwpriv wlan0 mp_channel 1						# set channel to 1 . 2, 3, 4~13 etc.
iwpriv wlan0 mp_bandwidth 40M=0,shortGI=0				# set 20M mode and long GI,set 40M is 40M=1.
iwpriv wlan0 mp_ant_tx a						# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
iwpriv wlan0 mp_txpower patha=44,pathb=44				# set path A and path B Tx power level , the Range is 0~63.
iwpriv wlan0 mp_rate 108						# set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108 ; N Rate: MCS0 = 128, MCS1 = 129 MCS 2=130....MCS15 = 143 etc.
iwpriv wlan0 mp_ctx background,pkt					# start continuous Packet Tx
iwpriv wlan0 mp_ctx stop						# stop continuous Tx
iwpriv wlan0 mp_stop							# exit MP mode
ifconfig wlan0 down							# close WLAN interface
*/
int WiFi_Continuous_Packet_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime)
{
    int ret;

    ret = mp_start(1);
    if (ret) {
        return ret;
    }

    ret = set_mp_channel(mp_channel);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_bandwidth(bandwidth, short_gi);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_ant_tx(antenna_x);
    if (ret) {
        goto ERR_EXIT;
    }

    if (pathx_txpower != 0) {
        ret = mp_set_tx_power(pathx_txpower, pathx_txpower, pathx_txpower, pathx_txpower);
    }
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_rate(mp_rate);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_reset_stats();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_con_pkt_tx();
    if (ret) {
        goto ERR_EXIT;
    }

    while (tx_durtime--) {
        if (wifi_get_mp_test_status() == 1) {
            break;
        }

        time_delay_ticks(100);

//        ret = mp_query(NULL, NULL);
//        if(ret)
//            goto ERR_EXIT;
    }

    ret = mp_ctx_stop();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    mp_start(0);

    return -1;
}



/**
@brief: 有限封包数量调制讯号发送测试
    @param: mp_channel:1 , 2, 3, 4~13 etc.
            bandwidth: 0(20M)/1(40M)/2(80M)
            short_gi:0(long_GI),1(short_GI)
            antenna_x: "a" or "b" or "ab"
            patha_txpower:0~63
            pathb_txpower:0~63
            mp_rate: set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108;N Rate: MCS0 = 128,MCS1 = 129,MCS 2=130....MCS15 = 143 etc.
            npackets: Number of packets start packet Tx

    @note:  default example: Count_Packet_Tx_testing(10, 0, 0, "a", 44,108,10);

ifconfig wlan0 up							# Enable Device for MP operation
iwpriv wlan0 mp_start							# Enter MP mode
iwpriv wlan0 mp_channel 1						# Set channel to 1 . 2, 3, 4~13 etc.
iwpriv wlan0 mp_bandwidth 40M=0,shortGI=0				# Set 20M mode and long GI,set 40M is 40M=1.
iwpriv wlan0 mp_ant_tx a						# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
iwpriv wlan0 mp_txpower patha=44,pathb=44				# Set path A and path B Tx power level , the Range is 0~63.
iwpriv wlan0 mp_rate 108						# Set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108 ; N Rate: MCS0 = 128, MCS1 = 129 MCS 2=130....MCS15 = 143 etc.
iwpriv wlan0 mp_ctx count=%d,pkt					# "%d" Number of packets start packet Tx
iwpriv wlan0 mp_ctx stop						# stop continuous Tx
iwpriv wlan0 mp_stop							# exit MP mode
ifconfig wlan0 down							# close WLAN interface
*/
int WiFi_Count_Packet_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower,  const char *mp_rate, u32 npackets)
{
    int ret, tx_pkt, time_lapse;

    ret = mp_start(1);
    if (ret) {
        return ret;
    }

    ret = set_mp_channel(mp_channel);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_bandwidth(bandwidth, short_gi);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_ant_tx(antenna_x);
    if (ret) {
        goto ERR_EXIT;
    }

    if (pathx_txpower != 0) {
        ret = mp_set_tx_power(pathx_txpower, pathx_txpower, pathx_txpower, pathx_txpower);
    }
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_rate(mp_rate);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_reset_stats();
    if (ret) {
        goto ERR_EXIT;
    }


    ret = mp_count_pkt_tx(npackets);
    if (ret) {
        goto ERR_EXIT;
    }

    time_lapse = time(NULL);
    while (1) {
        if (wifi_get_mp_test_status() == 1) {
            break;
        }

        time_delay_ticks(100);

        ret = mp_query((unsigned int *)&tx_pkt, NULL);
        if (ret) {
            goto ERR_EXIT;
        }

        if (tx_pkt == npackets) {
            printf("TX[%d] pkts Time-consuming = %d Sec \n\n", npackets, time(NULL) - time_lapse);
            break;
        }
    }

    ret = mp_ctx_stop();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    mp_start(0);

    return -1;
}

/**
@brief: 载波抑制发送测试
    @param: mp_channel:1 , 2, 3, 4~13 etc.
            bandwidth: 0(20M)/1(40M)/2(80M)
            short_gi:0(long_GI),1(short_GI)
            antenna_x: "a" or "b" or "ab"
            patha_txpower:0~63
            pathb_txpower:0~63
            mp_rate: set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108;N Rate: MCS0 = 128,MCS1 = 129,MCS 2=130....MCS15 = 143 etc.
            tx_durtime: how mang seconds duration

    @note:  default example: Carrier_suppression_testing(10, 0, 0, "a", 44,44,108,10);

ifconfig wlan0 up						# Enable Device for MP operation
iwpriv wlan0 mp_start						# enter MP mode
iwpriv wlan0 mp_channel 1					# set channel to 1 . 2, 3, 4~13 etc.
iwpriv wlan0 mp_bandwidth 40M=0,shortGI=0			# set 20M mode and long GI,set 40M is 40M=1.
iwpriv wlan0 mp_ant_tx a					# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
iwpriv wlan0 mp_txpower patha=44,pathb=44			# set path A and path B Tx power level , the Range is 0~63.
iwpriv wlan0 mp_rate 108					# set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108 ; N Rate: MCS0 = 128, MCS1 = 129 MCS 2=130....MCS15 = 143 etc.
iwpriv wlan0 mp_ctx background,cs				# start sending carrier suppression signal
iwpriv wlan0 mp_ctx stop					# stop continuous Tx
iwpriv wlan0 mp_stop						# exit MP mode
ifconfig wlan0 down						# close WLAN interface
*/

int WiFi_Carrier_suppression_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime)
{
    int ret;

    ret = mp_start(1);
    if (ret) {
        return ret;
    }

    //delay_ms(1000);

    ret = set_mp_channel(mp_channel);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_bandwidth(bandwidth, short_gi);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_ant_tx(antenna_x);
    if (ret) {
        goto ERR_EXIT;
    }

    if (pathx_txpower != 0) {
        ret = mp_set_tx_power(pathx_txpower, pathx_txpower, pathx_txpower, pathx_txpower);
    }
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_rate(mp_rate);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_reset_stats();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_carrier_suppression_tx();
    if (ret) {
        goto ERR_EXIT;
    }

    while (tx_durtime--) {
        if (wifi_get_mp_test_status() == 1) {
            break;
        }

        time_delay_ticks(100);

//        ret = mp_query(NULL, NULL);
//        if(ret)
//            goto ERR_EXIT;
    }

    ret = mp_ctx_stop();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    mp_start(0);

    return -1;
}


/**
@brief: 无调制讯号单频载波发送测试
    @param: mp_channel:1 , 2, 3, 4~13 etc.
            bandwidth: 0(20M)/1(40M)/2(80M)
            short_gi:0(long_GI),1(short_GI)
            antenna_x: "a" or "b" or "ab"
            patha_txpower:0~63
            pathb_txpower:0~63
            mp_rate: set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108;N Rate: MCS0 = 128,MCS1 = 129,MCS 2=130....MCS15 = 143 etc.
            tx_durtime: how mang seconds duration

    @note:  default example: Single_Tone_Tx_testing(10, 0, 0, "a", 44,44,108,10);

ifconfig wlan0 up						# Enable Device for MP operation
iwpriv wlan0 mp_start						# enter MP mode
iwpriv wlan0 mp_channel 1					# set channel to 1 . 2, 3, 4~13 etc.
iwpriv wlan0 mp_bandwidth 40M=0,shortGI=0			# set 20M mode and long GI,set 40M is 40M=1.
iwpriv wlan0 mp_ant_tx a					# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
iwpriv wlan0 mp_txpower patha=44,pathb=44			# set path A and path B Tx power level , the Range is 0~63.
iwpriv wlan0 mp_rate 108					# set OFDM data rate to 54Mbps,ex: CCK 1M = 2, CCK 5.5M = 11 ;OFDM 6M=12、54M = 108 ; N Rate: MCS0 = 128, MCS1 = 129 MCS 2=130....MCS15 = 143 etc.
iwpriv wlan0 mp_ctx background,stone				# start sending single tone signal
iwpriv wlan0 mp_ctx stop					# stop continuous Tx
iwpriv wlan0 mp_stop						# exit MP mode
ifconfig wlan0 down						# close WLAN interface
*/

int WiFi_Single_Tone_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime)
{
    int ret;

    ret = mp_start(1);
    if (ret) {
        return ret;
    }

    ret = set_mp_channel(mp_channel);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_bandwidth(bandwidth, short_gi);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_ant_tx(antenna_x);
    if (ret) {
        goto ERR_EXIT;
    }

    if (pathx_txpower != 0) {
        ret = mp_set_tx_power(pathx_txpower, pathx_txpower, pathx_txpower, pathx_txpower);
    }
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_rate(mp_rate);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_reset_stats();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_single_tone_tx();
    if (ret) {
        goto ERR_EXIT;
    }

    while (tx_durtime--) {
        if (wifi_get_mp_test_status() == 1) {
            break;
        }

        time_delay_ticks(100);

//        ret = mp_query(NULL, NULL);
//        if(ret)
//            goto ERR_EXIT;
    }

    ret = mp_ctx_stop();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    mp_start(0);

    return -1;
}

/**
@brief: 接收封包测试,测试接收数据量与错误率
    @param: mp_channel:1 , 2, 3, 4~13 etc.
            bandwidth: 0(20M)/1(40M)/2(80M)
            short_gi:0(long_GI),1(short_GI)
            antenna_x: "a" or "b" or "ab"
            rx_durtime: how mang seconds duration

    @note:  default example: Air_Rx_testing(1, 0, 0, "a", 10);

ifconfig wlan0 up					# Enable Device for MP operation
iwpriv wlan0 mp_start					# Enter MP mode
iwpriv wlan0 mp_channel 1				# Set channel to 1 . 2, 3, 4~13 etc.
iwpriv wlan0 mp_bandwidth 40M=0,shortGI=0               # Set 20M mode and long GI or set to 40M is 40M=1.
iwpriv wlan0 mp_ant_rx a				# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.
iwpriv wlan0 mp_arx start				# start air Rx teseting.
iwpriv wlan0 mp_query					# get the statistics.
iwpriv wlan0 mp_arx stop or iwpriv wlan0 mp_reset_stats	# Stop air Rx test and show the Statistics / Reset Counter.
iwpriv wlan0 mp_stop					# exit MP mode
ifconfig wlan0 down					# close WLAN interface
*/
int WiFi_Air_Rx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u32 rx_durtime)
{
    int ret;

    ret = mp_start(1);
    if (ret) {
        return ret;
    }

    ret = set_mp_channel(mp_channel);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_bandwidth(bandwidth, short_gi);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_set_ant_rx(antenna_x);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_reset_stats();
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_arx_start(1);
    if (ret) {
        goto ERR_EXIT;
    }

    while (rx_durtime--) {
        if (wifi_get_mp_test_status() == 1) {
            break;
        }

        time_delay_ticks(100);

        ret = mp_query(NULL, NULL);
        if (ret) {
            goto ERR_EXIT;
        }
    }

    ret = mp_arx_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    ret = mp_start(0);
    if (ret) {
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    mp_start(0);

    return -1;
}

