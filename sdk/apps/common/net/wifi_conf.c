#include <string.h>
#include "generic/typedef.h"
#include "app_config.h"
#include "asm/crc16.h"
#include "asm/gpio.h"
#include "syscfg/syscfg_id.h"
#include "asm/sfc_norflash_api.h"
#ifdef CONFIG_NET_ENABLE
#include "wifi/wifi_connect.h"
#include "lwip.h"
#endif

/*
// 设置WIFI DEBUG 信息输出等级
Debug information verbosity: lower values indicate higher urgency
0:RT_DEBUG_OFF
1:RT_DEBUG_ERROR
2:RT_DEBUG_WARN
3:RT_DEBUG_TRACE
4:RT_DEBUG_INFO
5:RT_DEBUG_LOUD
*/
const u8 RTDebugLevel = 2;

#if TCFG_RF_FCC_TEST_ENABLE
const u8 config_rf_test_enable = 1;
#else
#if TCFG_RF_PRODUCT_TEST_ENABLE
u8 config_rf_test_enable = 0;
#else
const u8 config_rf_test_enable = 0;
#endif
#endif

const char WL_TX_DEBUG = 0; //WIFI底层发送数据FIFO繁忙打印
const char WL_RX_DEBUG = 0; //WIFI底层接收FIFO塞满导致丢包打印

const char WL_TX_ERR_RATIO_DEBUG_SEC = 0; //统计每秒TX数据包成功/失败/重发和错误率的情况,配置每隔多少秒打印一次

const char WL_RX_ERR_RATIO_DEBUG_SEC = 0; //统计每秒RX数据包成功/失败/和错误率的情况,配置每隔多少秒打印一次

const u16 WL_RX_BACK_GROUND_ERR_CNT_PER_SECOND = 10; //硬件少了寄存器统计接收正确包,所以统计正确包和误报率不准,人工设定一个当前环境干扰每秒钟的本底错误包作为弥补,一开始需要先观察打印确认

const u8 WL_TX_PEND_DEBUG_SEC = 2; //WIFI底层FIFO塞满导致连续多少秒都发送不出数据时打印, 一般认为是干扰严重/wifi板TX性能差/CPU被挡等因素导致

const char WL_RX_PEND_DEBUG_SEC = 2; //统计WIFI底层连续多少秒都接收不到空中包时打印,一般认为是进了屏蔽房/加了MAC层过滤/板子硬件性能太差/CPU太繁忙来接收线程来不及取数因素导致

const char WL_RX_OVERFLOW_DEBUG = 0; //统计WIFI底层接收FIFO塞满导致丢包打印,一般认为对端发送太猛/空中干扰太强/CPU太繁忙来接收线程来不及取数因素导致, 使能后如果出现丢包打印每秒丢多少个数据包

#if TCFG_RF_FCC_TEST_ENABLE
char WIFI_PA_ENABLE = 0; //wifi开启外挂硬件PA功率放大，需要根据实际原理图配置需要映射的IO
#else
const char WIFI_PA_ENABLE = 0; //wifi开启外挂硬件PA功率放大，需要根据实际原理图配置需要映射的IO
#endif

#if defined CONFIG_NO_SDRAM_ENABLE
const u8 RxReorderEnable = 0; //底层包乱序整理，0为关闭(关闭时UDP重发包也会上传到上层, 但关闭可以减少内存消耗)，1为开启
const u32 MAX_REORDERING_MPDU_NUM = 1; //当RxReorderEnable开启时需要内存较大，该值也需要根据实际设置大一点(128)
#else
const u8 RxReorderEnable = 0; //底层包乱序整理，0为关闭(关闭时UDP重发包也会上传到上层, 但关闭可以减少内存消耗)，1为开启
const u32 MAX_REORDERING_MPDU_NUM = 128; //当RxReorderEnable开启时需要内存较大，该值也需要根据实际设置大一点(128)
#endif

const u16 MAX_CHANNEL_TIME_BSS_INFRA = 120;//扫描每个信道停留时间,单位ms,最小20ms, 200-400ms最佳

const char WIFI_CHANNEL_QUALITY_INDICATION_BAD = 5; //STA模式下的信道通信质量差阈值,一旦低于这个值就断线重连,如果配置为-1则信号质量再差也不通知断线,但是太久不重连会被路由器认为死亡踢掉的风险

#if TCFG_RF_FCC_TEST_ENABLE
const char wifi_temperature_drift_trim_on = 0; //WiFi温度漂移校准开关,0为关闭，1为打开
#else
const char wifi_temperature_drift_trim_on = 1; //WiFi温度漂移校准开关,0为关闭，1为打开
#endif
const char wifi_temperature_drift_trim_degree = 22; //触发trim动作的温差值

const char wifi_ap_scan_support = 0; //ap扫描开关，0为关闭，1为开启

const char wifi_ap_miss_owndevice_channel = 0; //自己设备信道错开功能，0为关闭，1为开启

const char wifi_rate_adapt_strategy = 0; //ap或sta模式下的wifi速率控制策略，0为使用默认策略，1为用于图传的策略，2为用于iperf的策略

const char wifi_rts_auto_trigger_enable = 1;

const u16 MAX_PACKETS_IN_QUEUE = 64; //配置WiFi驱动最大发送数据包队列
const u16 MAX_PACKETS_IN_MCAST_PS_QUEUE = 16;  //配置WiFi驱动最大发送MCAST-power-save包队列 //modify by lyx 32
const u16 MAX_PACKETS_IN_PS_QUEUE	= 128; //配置WiFi驱动最大发送power-save队列	//128	/*16 */

#if TCFG_RF_FCC_TEST_ENABLE
const u8 RFIinitUseTrimValue = 0;//记忆wifi rf 初始化使用vm记忆的trim的值,可大大降低wifi初始化时间
#else
const u8 RFIinitUseTrimValue = 1;//记忆wifi rf 初始化使用vm记忆的trim的值,可大大降低wifi初始化时间
#endif

#ifdef CONFIG_STATIC_IPADDR_ENABLE
const u8 IPV4_ADDR_CONFLICT_DETECT = 1;  //开启静态ip冲突检测
#else
const u8 IPV4_ADDR_CONFLICT_DETECT = 0;
#endif

const u8 ntp_get_time_init = 1;	//连上网后调用ntp向ntp_host列表所有服务器获取时间, 0为关闭, 1为开启
const u32 rtc_update_interval = 0; //每经过设置的时间间隔后会通过ntp对rtc进行校准, 单位毫秒

const u8 CONFIG_SDIO_SLAVE_MODE = 0; //wifi模块作为sdio从机

#ifdef CONFIG_UCOS_ENABLE
const u8 WIFI_TX_FULL_WAIT_MODE = 0; // 可以设置为0,1,2, 数值越大一定程度上会提高wifi的吞吐率，但同时会加大wifi线程占据cpu的比重
#else
const u8 WIFI_TX_FULL_WAIT_MODE = 1; // 可以设置为0,1,2, 数值越大一定程度上会提高wifi的吞吐率，但同时会加大wifi线程占据cpu的比重
#endif

const u8 WIFI_COLD_START_FAST_CONNECTION = 1; //启用WIFI冷启动快连, 0为关闭，1为开启

#if defined CONFIG_NO_SDRAM_ENABLE
const u8 MAX_LEN_OF_BSS_TABLE = 2; //BSS table 个数设置用于存放扫描结果，最少为1个，不能为0，个数越多占用内存越大
#else
const u8 MAX_LEN_OF_BSS_TABLE = 6; //BSS table 个数设置用于存放扫描结果，最少为1个，不能为0，个数越多占用内存越大
#endif

const u8 wifi_recv_poll = 0; //wifi接收数据方式，0为中断方式接收，1为轮询方式接收

#if defined CONFIG_WIFI_IDLE_RESUME_BASEBAND_ENABLE
u8 wifi_lowpower_mode = 1;
#else
u8 wifi_lowpower_mode = 0;
#endif

const u8 CONFIG_WIFI_USE_TLSF_MEM = 0; //配置wifi使用独立的内存管理，与系统内存管理分割开
const unsigned int CONFIG_WIFI_MAX_MEM_LIMIT = 200 * 1024; //允许wifi使用的内存大小
const unsigned char CONFIG_AP_TXQ_PRI = 0;  //AP模式下tx和rx队列分配,tx占最大比重，用于改善发送为主的性能


#if TCFG_RF_FCC_TEST_ENABLE
//WIFI Adaptivity
/*n/8 dBm 干扰功率阈值, 设置值和真实值有-20dBm的差值, 即默认值为(-80*8)时，干扰功率为-60dBm时进行规避, 最低配置值为(-127*8)*/
short CHL_PWR_THR = (-70 * 8); //75
short CHL_BUSY_CONFIG = (0xe & 0x0f); //0xe
#else
//WIFI Adaptivity
/*n/8 dBm 干扰功率阈值, 设置值和真实值有-20dBm的差值, 即默认值为(-80*8)时，干扰功率为-60dBm时进行规避, 最低配置值为(-127*8)*/
short CHL_PWR_THR = (-80 * 8);
short CHL_BUSY_CONFIG = (0xc & 0x0f); //0xe
#endif

const u8 CONFIG_WPA3_SUPPORT = 1;  //1：使能wpa3支持，0：关闭wpa3支持
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#ifdef CONFIG_NET_ENABLE
#if 0
static void print_debug_ipv4(u32 daddr, u32 saddr)
{
    put_buf(&daddr, 4);
    put_buf(&saddr, 4);
    printf("daddr : %s\n", inet_ntoa(daddr));
    printf("saddr : %s\n", inet_ntoa(saddr));
}
#endif

void wifi_password_wrong_notify(const u8 *ssid, const u8 *passphrase)
{
#if 0
    if (ssid) {
        printf("wrong password info, ssid[%s]\n", ssid);

        u8 wifi_ssid_cnt = wifi_get_store_ssid_cnt();
        if (wifi_ssid_cnt > 1) {
            struct wifi_stored_sta_info sta_info_read[wifi_ssid_cnt];
            memset(&sta_info_read, 0, sizeof(sta_info_read));

            for (int k = 0; k < wifi_ssid_cnt; k++) {
                if (syscfg_read(WIFI_STA_INFO_IDX_START + k, (char *)&sta_info_read[k], sizeof(struct wifi_stored_sta_info)) < 0) {
                    break;
                }

                if (!strcmp((const char *)sta_info_read[k].ssid, ssid)) {
                    printf("Incorrect password, [%d]%s delete!!", k, sta_info_read[k].ssid);
                    sta_info_read[k].ssid[0] = 0; //密码错误就清除保存的SSID
                    syscfg_write(WIFI_STA_INFO_IDX_START + k, (char *)&sta_info_read[k], sizeof(struct wifi_stored_sta_info));
                }
            }
        }
    }

    //设置 best ssid flag, 让驱动连接保存的best ssid
    wifi_set_sta_connect_best_ssid(1);
#endif
}

//用于根据LWIP接收队列溢出情况下快速丢包减轻CPU负担,预留空间接收重要数据包
int lwip_low_level_inputput_filter(u8 *pkg, u32 len)
{
    if (config_rf_test_enable) {
        return 0;
    }

    static const u8 bc_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define ipv4addr_ismulticast(addr) ((addr & PP_HTONL(0xf0000000UL)) == PP_HTONL(0xe0000000UL))

    //根据LWIP接收缓存情况快速丢包减轻CPU负担,并且防止丢失重要重要数据包
    struct iphdr_e *iph = (struct iphdr_e *)(pkg + 10);
    u16 protoType = ntohs(iph->h_proto);

    /*------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define ICMP_ER   0    /* echo reply */
#define ICMP_ECHO 8    /* echo */
    if ((protoType != 0x0806) && iph->iphd.protocol == 1 && *((u8 *)iph + sizeof(struct iphdr_e)) != ICMP_ER && *((u8 *)iph + sizeof(struct iphdr_e)) != ICMP_ECHO) { //如果是ICMP并且不是echo和echo reply就丢弃, 因为是一些Time_Ex等无用包
        putchar('C');
        return -1;
    }
    /*------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define PBUF_RESERVED_FOR_ARP 1 //最起码预留一个 PBUF_POOL给ARP请求,否则不回复路由器导致断流
#define PBUF_RESERVED_FOR_TCP (PBUF_RESERVED_FOR_ARP+1)
    u32 remain_pbuf_pool = memp_get_pbuf_pool_free_cnt();
    if (remain_pbuf_pool <= PBUF_RESERVED_FOR_TCP) {
        if (remain_pbuf_pool == 0) { //PBUF_POOL 一个也没了,直接丢弃
            putchar('X');
            return -1;
        } else if (remain_pbuf_pool <= PBUF_RESERVED_FOR_ARP && protoType != 0x0806) {//PBUF_POOL 在剩余小于 PBUF_RESERVED_FOR_ARP 的情况下, 丢弃非ARP包
            putchar('Y');
            return -1;
        } else if (!(protoType == 0x0800 && iph->iphd.protocol == 6)) { //PBUF_POOL 在剩余小于 PBUF_RESERVED_FOR_TCP 的情况下, 丢弃非TCP包
            putchar('D');
            return -1;
        }
    }
    return 0;
}

//在WIFI底层发送队列不足的情况, 预留空间给重要数据包发送
int lwip_low_level_output_filter(u8 *pkg, u32 len)
{
    if (config_rf_test_enable) {
        return 0;
    }

    struct iphdr_e *iph = (struct iphdr_e *)(pkg + 10);
    u16 protoType = ntohs(iph->h_proto);

    u32 remain_wifi_txq = wifi_get_remain_tx_queue(0);

#define WIFI_TXQ_RESERVED_FOR_ARP 1 //最起码预留一个WIFI_TXQ给ARP请求,否则不回复路由器导致断流
#define WIFI_TXQ_RESERVED_FOR_TCP (WIFI_TXQ_RESERVED_FOR_ARP+1)

    if (remain_wifi_txq <= WIFI_TXQ_RESERVED_FOR_TCP) {
        if (remain_wifi_txq == 0) { //WIFI_TXQ 一个也没了,直接丢弃
            putchar('G');
            return -1;
        } else if (remain_wifi_txq <= WIFI_TXQ_RESERVED_FOR_ARP && protoType != 0x0806) {//WIFI_TXQ 在剩余小于 WIFI_TXQ_RESERVED_FOR_ARP 的情况下, 丢弃非ARP包
            putchar('O');
            return -1;
        } else if (!(protoType == 0x0800 && iph->iphd.protocol == 6)) { //WIFI_TXQ 在剩余小于 WIFI_TXQ_RESERVED_FOR_TCP 的情况下, 丢弃非TCP包
            putchar('H');
            return -1;
        }
    }
    return 0;
}

//空中干扰/网络拥塞严重的情况,WIFI底层发送较慢,MAX_PACKETS_IN_QUEUE 队列满导致丢弃上层数据包
int socket_send_but_netif_busy_hook(int s, char type_udp)
{
    if (type_udp) {
        putchar('@');
        /* os_time_dly(30); //根据实际应用发送情况调节, 针对UDP多释放一下CPU, 一方面有利于系统其他线程顺畅运行, 另一方面防止猛发送导致网络拥塞加剧 */
    } else {
        putchar('|');
        /* os_time_dly(2); */
    }

    return 0;
}

int wifi_recv_pkg_and_soft_filter(u8 *pkg, u32 len)  //通过软件过滤无用数据帧减轻cpu压力,pkg[20]就是对应抓包工具第一个字节的802.11 MAC Header 字段
{
#if 0
    static u32 thdll, count;
    int ret;
    ret = time_lapse(&thdll, 1000);
    if (ret) {
        printf("sdio_recv_cnt = %d,  %d \r\n", ret, count);
        count = 0;
    }
    ++count;
#endif

#if 0 //调试用
    static const u8 bc_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define ipv4addr_ismulticast(addr) ((addr & PP_HTONL(0xf0000000UL)) == PP_HTONL(0xe0000000UL))

    struct ieee80211_frame *wh = (struct ieee80211_frame *)&pkg[20];
    if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_DATA) {// 只丢弃数据帧
        if (pkg[54] == 0X88 && pkg[55] == 0x8E) {} else //如果是EAPOL不要丢弃
            if (len >= (52 + sizeof(struct iphdr_e))) {
                u16 protoType;
                struct iphdr_e *iph;
                if (0 == memcmp(bc_mac, wh->i_addr1, 6)) {
                    //广播
                    iph = (struct iphdr_e *)((u8 *)pkg + 48);
                } else {
                    iph = (struct iphdr_e *)((u8 *)pkg + 52);
                }
                protoType = ntohs(iph->h_proto);

                if (protoType == 0x0800) { //ipv4
                    if (iph->iphd.protocol == 17) { //udp
                        if (ipv4addr_ismulticast(iph->iphd.daddr)) { //组播
                            /*putchar('M');*/
                        } else {
                            /*putchar('U');*/
                        }

                        /* print_debug_ipv4(iph->iphd.daddr, iph->iphd.saddr); */

                    } else if (iph->iphd.protocol == 6) { //tcp
                        /*putchar('T');*/
                        /* print_debug_ipv4(iph->iphd.daddr, iph->iphd.saddr); */
                    }
                } else if (protoType == 0x0806) { //arp
                    /*putchar('R');*/
                }
            }
    }
#endif

    return 0;
}

#endif
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#if defined CONFIG_RF_TRIM_CODE_MOVABLE || defined CONFIG_RF_TRIM_CODE_AT_RAM

extern u32 __rf_trim_movable_slot_start[];
extern u32 __rf_trim_movable_slot_end[];
extern u8 __rf_trim_movable_region_start[];
extern u8 __rf_trim_movable_region_end[];

extern u32 __rf_trim_code_start, __rf_trim_code_end;

AT(.volatile_ram_code)
u8 *enter_wifi_rf_trim_region(u32 **start_of_region)
{
    u8 *rf_trim_code_run_addr = NULL;
#if defined CONFIG_RF_TRIM_CODE_MOVABLE
    u32 code_size = __rf_trim_movable_region_end - __rf_trim_movable_region_start;
    printf("rf_trim_movable code _start:0x%x, code_size:%d \r\n", (u32)__rf_trim_movable_region_start, code_size);
    rf_trim_code_run_addr = malloc(code_size);
    if (rf_trim_code_run_addr == NULL) {
        return NULL;
    }
    code_movable_load(__rf_trim_movable_region_start, code_size, rf_trim_code_run_addr, __rf_trim_movable_slot_start, __rf_trim_movable_slot_end, start_of_region);
#endif

    norflash_enter_spi_code(0);

#if defined CONFIG_RF_TRIM_CODE_MOVABLE && !defined CONFIG_NO_SDRAM_ENABLE
    icache_flush(rf_trim_code_run_addr, code_size);
#endif

    return rf_trim_code_run_addr;
}

AT(.volatile_ram_code)
void exit_wifi_rf_trim_region(u32 **start_of_region, u8 *rf_trim_code_run_addr)
{
#if defined CONFIG_RF_TRIM_CODE_MOVABLE
    if (rf_trim_code_run_addr == NULL) {
        return;
    }
#endif

    norflash_exit_spi_code(0);

#if defined CONFIG_RF_TRIM_CODE_MOVABLE
    code_movable_unload(__rf_trim_movable_region_start, __rf_trim_movable_slot_start, __rf_trim_movable_slot_end, start_of_region);
    free(rf_trim_code_run_addr);
#endif
}

#endif
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#if 0
WirelessMode:
PHY_11BG_MIXED = 0,
PHY_11B = 1,
PHY_11A = 2,,
PHY_11ABG_MIXED = 3,,
PHY_11G = 4,
PHY_11ABGN_MIXED,	/* both band   5 */
PHY_11N_2_4G,		/* 11n-only with 2.4G band      6 */
PHY_11GN_MIXED,		/* 2.4G band      7 */
PHY_11AN_MIXED,		/* 5G  band       8 */
PHY_11BGN_MIXED,	/* if check 802.11b.      9 */
PHY_11AGN_MIXED,	/* if check 802.11b.      10 */
PHY_11N_5G,		/* 11n-only with 5G band                11 */
#endif

#ifdef CONFIG_NET_ENABLE

/*
 MaxStaNum  最大连接数不能超过 MAX_LEN_OF_MAC_TABLE(5)
 */
static char WL_AP_DAT[] = {
    "\
Default\n\
MacAddress=00:00:00:00:00:00\n\
CountryRegion=1\n\
CountryRegionABand=0\n\
CountryCode=CN\n\
BssidNum=1\n\
MaxStaNum=2\n\
IdleTimeout=300\n\
SSID=####SSID_LENTH_MUST_LESS_THAN_32\n\
WirelessMode=9\n\
TxRate=0\n\
Channel=12#\n\
BasicRate=15\n\
BeaconPeriod=100\n\
DtimPeriod=1\n\
TxPower=100\n\
DisableOLBC=0\n\
BGProtection=0\n\
TxAntenna=\n\
RxAntenna=\n\
TxPreamble=1\n\
RTSThreshold=2347\n\
FragThreshold=2346\n\
TxBurst=0\n\
PktAggregate=0\n\
TurboRate=0\n\
WmmCapable=0\n\
APSDCapable=0\n\
DLSCapable=0\n\
APAifsn=3;7;1;1\n\
APCwmin=4;4;3;2\n\
APCwmax=6;10;4;3\n\
APTxop=0;0;94;47\n\
APACM=0;0;0;0\n\
BSSAifsn=3;7;2;2\n\
BSSCwmin=4;4;3;2\n\
BSSCwmax=10;10;4;3\n\
BSSTxop=0;0;94;47\n\
BSSACM=0;0;0;0\n\
AckPolicy=0;0;0;0\n\
NoForwarding=0\n\
NoForwardingBTNBSSID=0\n\
HideSSID=0\n\
StationKeepAlive=0\n\
ShortSlot=1\n\
AutoChannelSelect=0\n\
IEEE8021X=0\n\
IEEE80211H=0\n\
CSPeriod=10\n\
WirelessEvent=0\n\
IdsEnable=0\n\
AuthFloodThreshold=32\n\
AssocReqFloodThreshold=32\n\
ReassocReqFloodThreshold=32\n\
ProbeReqFloodThreshold=32\n\
DisassocFloodThreshold=32\n\
DeauthFloodThreshold=32\n\
EapReqFooldThreshold=32\n\
PreAuth=0\n\
AuthMode=################\n\
EncrypType=################\n\
RekeyInterval=0\n\
RekeyMethod=DISABLE\n\
PMKCachePeriod=10\n\
WPAPSK=#########wpa_passphrase_lenth_must_more_than_7_and_less_than_63\n\
DefaultKeyID=1\n\
Key1Type=0\n\
Key1Str=\n\
Key2Type=0\n\
Key2Str=\n\
Key3Type=0\n\
Key3Str=\n\
Key4Type=0\n\
Key4Str=\n\
HSCounter=0\n\
AccessPolicy0=0\n\
AccessControlList0=\n\
AccessPolicy1=0\n\
AccessControlList1=\n\
AccessPolicy2=0\n\
AccessControlList2=\n\
AccessPolicy3=0\n\
AccessControlList3=\n\
WdsEnable=0\n\
WdsEncrypType=NONE\n\
WdsList=\n\
WdsKey=\n\
RADIUS_Server=192.168.2.3\n\
RADIUS_Port=1812\n\
RADIUS_Key=ralink\n\
own_ip_addr=192.168.5.234\n\
EAPifname=br0\n\
PreAuthifname=br0\n\
HT_HTC=0\n\
HT_RDG=1\n\
HT_EXTCHA=0\n\
HT_LinkAdapt=0\n\
HT_OpMode=0\n\
HT_MpduDensity=4\n\
HT_BW=0\n\
HT_BADecline=0\n\
HT_AutoBA=1\n\
HT_AMSDU=0\n\
HT_BAWinSize=64\n\
HT_GI=0\n\
HT_MCS=33\n\
MeshId=MESH\n\
MeshAutoLink=1\n\
MeshAuthMode=OPEN\n\
MeshEncrypType=NONE\n\
MeshWPAKEY=\n\
MeshDefaultkey=1\n\
MeshWEPKEY=\n\
WscManufacturer=\n\
WscModelName=\n\
WscDeviceName=\n\
WscModelNumber=\n\
WscSerialNumber=\n\
RadioOn=1\n\
PMFMFPC=0\n\
PMFMFPR=0\n\
PMFSHA256=0"
};

#if defined CONFIG_LOW_POWER_ENABLE
#define WL_STA_SLEEP	1
#elif defined CONFIG_WIFI_IDLE_RESUME_BASEBAND_ENABLE
#define WL_STA_SLEEP	1//2
#else
#define WL_STA_SLEEP	0
#endif

static const char WL_STA_DAT[] = {
    "\
Default\n\
MacAddress=00:00:00:00:00:00\n\
CountryRegion=1\n\
CountryRegionABand=0\n\
CountryCode=CN\n\
ChannelGeography=1\n\
SSID=DEFAULT_CONNECT_SSID\n\
NetworkType=Infra\n\
WirelessMode=9\n\
Channel=1\n\
BeaconPeriod=100\n\
TxPower=100\n\
BGProtection=0\n\
TxPreamble=1\n\
RTSThreshold=2347\n\
FragThreshold=2346\n\
TxBurst=0\n\
PktAggregate=0\n\
WmmCapable=1\n\
AckPolicy=0;0;0;0\n\
AuthMode=OPEN\n\
EncrypType=NONE\n\
WPAPSK=\n\
DefaultKeyID=1\n\
Key1Type=0\n\
Key1Str=\n\
Key2Type=0\n\
Key2Str=\n\
Key3Type=0\n\
Key3Str=\n\
Key4Type=0\n\
Key4Str=\n"
#if WL_STA_SLEEP == 1
    "PSMode=Legacy_PSP\n"
#elif WL_STA_SLEEP == 2
    "PSMode=Fast_PSP\n"
#else
    "PSMode=CAM\n"
#endif
    "AutoRoaming=0\n\
RoamThreshold=70\n\
APSDCapable=1\n\
APSDAC=0;0;0;0\n\
HT_RDG=1\n\
HT_EXTCHA=0\n\
HT_OpMode=0\n\
HT_MpduDensity=4\n\
HT_BW=0\n\
HT_BADecline=0\n\
HT_AutoBA=1\n\
HT_AMSDU=0\n\
HT_BAWinSize=64\n\
HT_GI=0\n\
HT_MCS=33\n\
HT_MIMOPSMode=3\n\
HT_DisallowTKIP=1\n\
HT_STBC=0\n\
EthConvertMode=\n\
EthCloneMac=\n\
IEEE80211H=0\n\
TGnWifiTest=0\n\
WirelessEvent=0\n\
MeshId=MESH\n\
MeshAutoLink=1\n\
MeshAuthMode=OPEN\n\
MeshEncrypType=NONE\n\
MeshWPAKEY=\n\
MeshDefaultkey=1\n\
MeshWEPKEY=\n\
CarrierDetect=0\n\
AntDiversity=0\n\
BeaconLostTime=16\n\
FtSupport=0\n\
Wapiifname=ra0\n\
WapiPsk=\n\
WapiPskType=\n\
WapiUserCertPath=\n\
WapiAsCertPath=\n\
PSP_XLINK_MODE=0\n\
WscManufacturer=\n\
WscModelName=\n\
WscDeviceName=\n\
WscModelNumber=\n\
WscSerialNumber=\n\
RadioOn=1\n\
WIDIEnable=0\n\
P2P_L2SD_SCAN_TOGGLE=8\n\
Wsc4digitPinCode=0\n\
P2P_WIDIEnable=0\n\
PMFMFPC=0\n\
PMFMFPR=0\n\
PMFSHA256=0"
};

void wsc_sta_config_set_ssid_pwd(const char *ssid, const char *pwd, int ssid_len, int pwd_len)
{
#ifdef CONFIG_WSC_NET_CFG
    extern int wsc_net_set_ssid_pwd(const char *ssid, const char *pwd, int ssid_len, int pwd_len);
    wsc_net_set_ssid_pwd(ssid, pwd, ssid_len, pwd_len);
#endif
}

const char *GET_WL_STA_DAT(void)
{
    return WL_STA_DAT;
}

int GET_WL_STA_DAT_LEN(void)
{
    return strlen(WL_STA_DAT);
}

const char *GET_WL_AP_DAT(void)
{
    return WL_AP_DAT;
}

int GET_WL_AP_DAT_LEN(void)
{
    return strlen(WL_AP_DAT);
}

#if 0
const char *get_wl_cfg_data(int is_sta)
{
    const char *data = NULL;

    if (is_sta) {
        data = malloc(strlen(WL_STA_DAT) + 1);
    } else {
        data = malloc(strlen(WL_AP_DAT) + 1);
    }

    if (!data) {
        return NULL;
    }

    if (is_sta) {
        strcpy(data, WL_STA_DAT);
    } else {
        strcpy(data, WL_AP_DAT);
    }

    return data;
}

static u32 write_offset = 0;

void *RtmpOSFileOpen(const char *fileName, int mode, int opt)
{
    write_offset = 0;
    return zalloc(4096);
}

void RtmpOSFileClose(void *ptr)
{
    printf("wifi cfg file : %s\n", (const char *)ptr);
    free(ptr);
}

int RtmpOSFileWrite(void *ptr, const char *str, int len)
{
    memcpy(ptr + write_offset, str, len);
    write_offset += len;
    return len;
}
#endif

int wl_set_wifi_channel(int channel)
{
    char channel_str[3] = {0};

    if (channel < 1 || channel > 14) {
        return -1;
    }

    sprintf(channel_str, "%d", channel);

    const char *find_channel = "Channel=";
    char *channel_position = strstr(GET_WL_AP_DAT(), find_channel) + strlen(find_channel);

    strcpy(channel_position, channel_str);
    channel_position[strlen(channel_str)] = '\n';
    memset(channel_position + strlen(channel_str) + 1, '#', 3 - strlen(channel_str) - 1);

    return 0;
}

static int wl_set_ssid(const char *ssid)
{
    if (ssid == 0) {
        return -1;
    }

    const char *find_ssid = "SSID=";
    char *ssid_position = strstr(GET_WL_AP_DAT(), find_ssid) + strlen(find_ssid);

    if (strlen(ssid) > 32) {
        printf("set_ssid_passphrase fail,ssid len (0x%lx) longer than 32!\n", strlen(ssid));
        return -1;
    }

    strcpy(ssid_position, ssid);
    ssid_position[strlen(ssid)] = '\n';
    memset(ssid_position + strlen(ssid) + 1, '#', 32 - strlen(ssid) - 1);

    return 0;
}

static int wl_set_passphrase(const char *passphrase)
{
    if (passphrase == 0) {
        return -1;
    }

    const char *AUTH_MODE, *ENCRYP_TYPE;
    const char *find_wpa_passphrase = "WPAPSK=";
    const char *find_AuthMode = "AuthMode=";
    const char *find_EncrypType = "EncrypType=";

    char *wpa_passphrase_position = strstr(GET_WL_AP_DAT(), find_wpa_passphrase) + strlen(find_wpa_passphrase);
    char *AuthMode_position = strstr(GET_WL_AP_DAT(), find_AuthMode) + strlen(find_AuthMode);
    char *EncrypType_position = strstr(GET_WL_AP_DAT(), find_EncrypType) + strlen(find_EncrypType);

    if (strcmp(passphrase, "")) {
        if ((strlen(passphrase) < 8) || (strlen(passphrase) > 63)) {
            printf("set_ssid_passphrase fail,passphrase len (0x%lx) must more than 7 and less than 63!\n", strlen(passphrase));
            return -1;
        }

        strcpy(wpa_passphrase_position, passphrase);
        wpa_passphrase_position[strlen(passphrase)] = '\n';
        memset(wpa_passphrase_position + strlen(passphrase) + 1, '#', 63 - strlen(passphrase) - 1);

        AUTH_MODE = "WPA2PSK";
        strcpy(AuthMode_position, AUTH_MODE);
        AuthMode_position[strlen(AUTH_MODE)] = '\n';
        memset(AuthMode_position + strlen(AUTH_MODE) + 1, '#', 16 - strlen(AUTH_MODE) - 1);

        ENCRYP_TYPE = "AES";
        strcpy(EncrypType_position, ENCRYP_TYPE);
        EncrypType_position[strlen(ENCRYP_TYPE)] = '\n';
        memset(EncrypType_position + strlen(ENCRYP_TYPE) + 1, '#', 16 - strlen(ENCRYP_TYPE) - 1);
    } else {
        AUTH_MODE = "OPEN";
        strcpy(AuthMode_position, AUTH_MODE);
        AuthMode_position[strlen(AUTH_MODE)] = '\n';
        memset(AuthMode_position + strlen(AUTH_MODE) + 1, '#', 16 - strlen(AUTH_MODE) - 1);

        ENCRYP_TYPE = "NONE";
        strcpy(EncrypType_position, ENCRYP_TYPE);
        EncrypType_position[strlen(ENCRYP_TYPE)] = '\n';
        memset(EncrypType_position + strlen(ENCRYP_TYPE) + 1, '#', 16 - strlen(ENCRYP_TYPE) - 1);
    }

    return 0;
}

int wl_ap_init(const char *ssid, const char *passphrase)
{
    int ret;
    wl_set_ssid(ssid);
    wl_set_passphrase(passphrase);

//    printf("WLAP_DAT = \r\n %s \r\n",GET_WL_AP_DAT());

    return 0;
}

void set_bss_table_record(char *data, u32 len)
{
    struct wifi_mode_info info = {0};
    info.mode = NONE_MODE;
    wifi_get_mode_cur_info(&info);
    if (info.mode == SMP_CFG_MODE) {
        return;
    }
    syscfg_write(WIFI_BSS_TABLE, data, len);
}
int get_bss_table_record(char *data, u32 len)
{
    int ret;
    ret = syscfg_read(WIFI_BSS_TABLE, data, len);
    if (ret < 0) {
        return 0;
    }
    return 1;
}

void set_scan_result_record(char *data, u32 len)
{
    syscfg_write(WIFI_SCAN_INFO, data, len);
}
int get_stored_scan_record(char *data, u32 len)
{
    int ret;
    ret = syscfg_read(WIFI_SCAN_INFO, data, len);
    if (ret < 0) {
        return 0;
    }
    return 1;
}

#if 1
struct pmk_info {
    u32 crc;
    char pmk[32];
};
int get_stored_pmk(char *password, char *ssid, int ssidlength, char *output)
{
    int ret;
    u32 crc;
    struct pmk_info info;

    /* ret = db_select_buffer(WIFI_PMK_INFO, (char *)&info, sizeof(struct pmk_info)); */
    ret = syscfg_read(WIFI_PMK_INFO, (char *)&info, sizeof(struct pmk_info));
    if (ret < 0) {
        return 0;
    }

    crc = CRC16(password, strlen(password));
    crc += CRC16(ssid, ssidlength);

    if (crc != info.crc) {
        return 0;
    }

    memcpy(output, info.pmk, sizeof(info.pmk));
    printf("stored pmk_info match[%s] [%s],,,\n", ssid, password);
    return 1;
}
void set_stored_pmk(char *password, char *ssid, int ssidlength, char *output)
{
    printf("pmk_info not match, store... [%s] [%s],,,\n", ssid, password);

    struct pmk_info info;
    info.crc = CRC16(password, strlen(password));
    info.crc += CRC16(ssid, ssidlength);
    memcpy(info.pmk, output, sizeof(info.pmk));
    /* db_update_buffer(WIFI_PMK_INFO, (char *)&info, sizeof(struct pmk_info)); */
    syscfg_write(WIFI_PMK_INFO, (char *)&info, sizeof(struct pmk_info));
}
#endif

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int wifi_get_rf_trim_data(void *info, int size)
{
    if (syscfg_read(VM_WIFI_RF_INIT_INFO, info, size) != size) {
        return -1;
    }
    return 0;
}

int wifi_set_rf_trim_data(void *info, int size)
{
    if (syscfg_write(VM_WIFI_RF_INIT_INFO, info, size) != size) {
        return -1;
    }
    return 0;
}

//用来解决编译链接问题
__attribute__((weak)) int netdev_get_mac_addr(char *mac_addr)
{
    return 0;
}

__attribute__((weak)) u8 *lte_module_get_mac_addr(void)
{
    return NULL;
}

#define TX_ERR_ADJUST_THRESHOLD 45
#define TX_ERR_ADJUST_COUNTERS 4
static int tx_adjust_counters = 0;

void wifi_edca_parm_get(u8 ac_type, u8 *txop_limit, u8 *cwmin, u8 *cwmax, u8 *aifsn);
void wifi_edca_parm_set(u8 ac_type, u8 txop_limit, u8 cwmin, u8 cwmax, u8 aifsn);

void wifi_edca_adjust(u8 ac_type, u8 txop_limit, u8 cwmin, u8 cwmax, u8 aifsn)
{
    u8 txop_limit_t, cwmin_t, cwmax_t, aifsn_t;
    wifi_edca_parm_get(ac_type, &txop_limit_t, &cwmin_t, &cwmax_t, &aifsn_t);
    //printf("txop_limit: %d, cwmin: %d, cwmax: %d, aifsn: %d\n", txop_limit, cwmin, cwmax, aifsn);
    if (txop_limit_t != txop_limit || cwmin_t != cwmin || cwmax_t != cwmax || aifsn_t != aifsn) {
        wifi_edca_parm_set(ac_type, txop_limit, cwmin, cwmax, aifsn);
    }
}

void wifi_tx_states_count_callback(unsigned long total_count, unsigned long retransmit_count,
                                   unsigned long fail_count, unsigned long err_ratio)
{
    return;
#if 1 //根据错包率调整edca参数
    if (err_ratio <= TX_ERR_ADJUST_THRESHOLD) {
        if (tx_adjust_counters) {
            tx_adjust_counters--;
        }
    } else {
        tx_adjust_counters++;
    }
    //printf("err_ratio: %d, total_count: %d, tx_adjust_counters: %d\n", err_ratio,  total_count, tx_adjust_counters);

    if (tx_adjust_counters < TX_ERR_ADJUST_COUNTERS) {
        wifi_edca_adjust(0, 255, 1, 1, 1);
    } else {
        wifi_edca_adjust(0, 0, 4, 10, 3);
    }

    if (tx_adjust_counters > TX_ERR_ADJUST_COUNTERS + 3) {
        tx_adjust_counters = TX_ERR_ADJUST_COUNTERS + 3;
    }
#endif
}


/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
const char log_tag_const_v_WEBSOCKET AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_WEBSOCKET AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_WEBSOCKET AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_WEBSOCKET AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_WEBSOCKET AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_TMALLGENIE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_TMALLGENIE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_TMALLGENIE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_TMALLGENIE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_TMALLGENIE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_DUI AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_DUI AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_DUI AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_DUI AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_DUI AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_TURING AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_TURING AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_TURING AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_TURING AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_TURING AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_WT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_WT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_WT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_WT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_WT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
