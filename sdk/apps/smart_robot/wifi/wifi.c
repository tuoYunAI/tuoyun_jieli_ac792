#include "app_config.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "dhcp_srv/dhcp_srv.h"
#include "event/net_event.h"
#include "net/assign_macaddr.h"
#include "syscfg_id.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "app_wifi.h"
#include "app_event.h"

#if TCFG_WIFI_ENABLE


#define LOG_TAG             "[WIFI-MAIN]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"



#define WIFI_SSID_LEN        32
#define WIFI_PWD_LEN         63
typedef struct  {
    enum WIFI_MODE mode;
    int update;   // 1:有更新, 0:无更新
    char ssid[WIFI_SSID_LEN+1];
    char pwd[WIFI_PWD_LEN+1];
} wifi_config_info_t, *wifi_config_info_ptr;

static u16 g_wifi_connection_timer = 0;
static u8 g_is_in_config_network_state = 0; //是否处于配置网络状态

static u8 g_mac_addr[6] = {0};
wifi_config_info_t g_wifi_info = {
    .mode = STA_MODE,
    .update = 0,
    .ssid = "",
    .pwd = "",
};

#define FORCE_DEFAULT_MODE 0 //配置wifi_on之后的模式,0为使用最后记忆的模式, 1为强制默认模式, 3-200为STA连接超时时间多少秒,如果超时都连接不上就连接最后记忆的或者最优网络

#define CONNECT_BEST_SSID  0 //配置如果啟動WIFI后在STA模式下, 是否挑选连接记忆过的信号最优WIFI


#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
static char mac_addr_succ_flag;
#endif

#ifdef CONFIG_STATIC_IPADDR_ENABLE
static u8 use_static_ipaddr_flag;
#endif


#ifdef CONFIG_STATIC_IPADDR_ENABLE
struct sta_ip_info {
    u8 ssid[33];
    u32 ip;
    u32 gw;
    u32 netmask;
    u32 dns;
    u8 gw_mac[6];
    u8 local_mac[6];
    u8 chanel;
};

static void wifi_set_sta_ip_info(void)
{
    struct sta_ip_info  sta_ip_info;
    syscfg_read(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info));

    struct lan_setting lan_setting_info = {

        .WIRELESS_IP_ADDR0  = (u8)(sta_ip_info.ip >> 0),
        .WIRELESS_IP_ADDR1  = (u8)(sta_ip_info.ip >> 8),
        .WIRELESS_IP_ADDR2  = (u8)(sta_ip_info.ip >> 16),
        .WIRELESS_IP_ADDR3  = (u8)(sta_ip_info.ip >> 24),

        .WIRELESS_NETMASK0  = (u8)(sta_ip_info.netmask >> 0),
        .WIRELESS_NETMASK1  = (u8)(sta_ip_info.netmask >> 8),
        .WIRELESS_NETMASK2  = (u8)(sta_ip_info.netmask >> 16),
        .WIRELESS_NETMASK3  = (u8)(sta_ip_info.netmask >> 24),

        .WIRELESS_GATEWAY0   = (u8)(sta_ip_info.gw >> 0),
        .WIRELESS_GATEWAY1   = (u8)(sta_ip_info.gw >> 8),
        .WIRELESS_GATEWAY2   = (u8)(sta_ip_info.gw >> 16),
        .WIRELESS_GATEWAY3   = (u8)(sta_ip_info.gw >> 24),
    };

    net_set_lan_info(&lan_setting_info);
}

static int compare_dhcp_ipaddr(void)
{
    use_static_ipaddr_flag = 0;

    u8 local_mac[6];
    u8 gw_mac[6];
    struct sta_ip_info  sta_ip_info;
    struct netif_info netif_info;
    int ret = syscfg_read(VM_STA_IPADDR_INDEX, (char *)&sta_ip_info, sizeof(struct sta_ip_info));

    if (ret < 0) {
        log_info("compare_dhcp_ipaddr NO VM_STA_IPADDR_INDEX");
        return -1;
    }

    lwip_get_netif_info(WIFI_NETIF, &netif_info);

    struct wifi_mode_info info;
    info.mode = STA_MODE;
    wifi_get_mode_cur_info(&info);

    wifi_get_bssid(gw_mac);
    wifi_get_mac(local_mac);

    if (!strcmp(info.ssid, (const char *)sta_ip_info.ssid)
        && !memcmp(local_mac, sta_ip_info.local_mac, 6)
        && !memcmp(gw_mac, sta_ip_info.gw_mac, 6)
        /*&& sta_ip_info.gw==sta_ip_info.dns//如果路由器没接网线/没联网,每次连接都去重新获取DHCP*/
       ) {
        use_static_ipaddr_flag = 1;
        log_info("compare_dhcp_ipaddr Match");
        return 0;
    }

    log_info("compare_dhcp_ipaddr not Match!!! [%s][%s],[0x%x,0x%x][0x%x,0x%x],[0x%x]", info.ssid, sta_ip_info.ssid, local_mac[0], local_mac[5], sta_ip_info.local_mac[0], sta_ip_info.local_mac[5], sta_ip_info.dns);

    return -1;
}

static void store_dhcp_ipaddr(void)
{
    struct sta_ip_info  sta_ip_info = {0};
    u8 sta_channel;
    u8 local_mac[6];
    u8 gw_mac[6];

    if (use_static_ipaddr_flag) { //记忆IP匹配成功,不需要重新保存
        return;
    }

    struct netif_info netif_info;
    lwip_get_netif_info(WIFI_NETIF, &netif_info);

    struct wifi_mode_info info;
    info.mode = STA_MODE;
    wifi_get_mode_cur_info(&info);

    sta_channel = wifi_get_channel();
    wifi_get_mac(local_mac);
    wifi_get_bssid(gw_mac);

    strcpy((char *)sta_ip_info.ssid, info.ssid);
    memcpy(sta_ip_info.gw_mac, gw_mac, 6);
    memcpy(sta_ip_info.local_mac, local_mac, 6);
    sta_ip_info.ip =  netif_info.ip;
    sta_ip_info.netmask =  netif_info.netmask;
    sta_ip_info.gw =  netif_info.gw;
    sta_ip_info.chanel = sta_channel;
    sta_ip_info.dns = *(u32 *)dns_getserver(0);

    syscfg_write(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info));

    log_info("store_dhcp_ipaddr");
}

void dns_set_server(u32 *dnsserver)
{
    struct sta_ip_info  sta_ip_info;
    if (syscfg_read(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info)) < 0) {
        *dnsserver = 0;
    } else {
        *dnsserver = sta_ip_info.dns;
    }
}

#endif

u32 get_wifi_ip(){

    struct netif_info netif = {0};
    
    lwip_get_netif_info(WIFI_NETIF, &netif);
    return netif.ip;
}

static int get_stored_sta_list(struct wifi_stored_sta_info wifi_stored_sta_info[]){
    int ssid_stored_cnt;

    ssid_stored_cnt = wifi_get_stored_sta_info(wifi_stored_sta_info);
    log_info("Stored SSID count=%d\r\n", ssid_stored_cnt);

    for (int i = 0; i < ssid_stored_cnt; i++) {
        log_info("wifi_get_stored_sta_info[%d]= %s \r\n", i, wifi_stored_sta_info[i].ssid);
    }

    return ssid_stored_cnt;
}


static void wifi_sta_save_current_ssid(void)
{
    struct wifi_mode_info info;
    info.mode = STA_MODE;
    wifi_get_mode_cur_info(&info);
    if (!info.ssid || !info.pwd) {
        return;
    }
    if (strncmp(info.ssid, g_wifi_info.ssid, WIFI_SSID_LEN) != 0 || strncmp(info.pwd, g_wifi_info.pwd, WIFI_PWD_LEN) != 0) {
        return;
    }
    if (!g_wifi_info.update) {
        return;
    }

    g_wifi_info.update = 0;
    wifi_set_sta_connect_best_ssid(1); //自动连接保存过的最佳WIFI
    wifi_store_mode_info(STA_MODE, info.ssid, info.pwd);
}

void wifi_return_sta_mode(void)
{
    if (!wifi_is_on()) {
        return;
    }
    struct wifi_mode_info info;
    info.mode = STA_MODE;
    int ret = wifi_get_mode_stored_info(&info);
    if (ret) {//如果没保存过SSID
        info.ssid = g_wifi_info.ssid;
        info.pwd = g_wifi_info.pwd;
    }
    wifi_clear_scan_result(); //根据扫描结果连接信号最优ssid之前先清除之前结果,防止之前最优信号的ssid已下线
    wifi_set_sta_connect_best_ssid(1); //自动连接保存过的最佳WIFI
    wifi_enter_sta_mode(info.ssid, info.pwd);
}

void wifi_sta_connect(char *ssid, char *pwd)
{
    struct wifi_mode_info info = {0};
    wifi_get_mode_cur_info(&info);
    log_info("wifi_get_mode_cur_info, mode: %d ssid %s pwd %s\r\n", info.mode, info.ssid, info.pwd);
    if (info.ssid != NULL && info.pwd != NULL && info.mode == STA_MODE
         && !strcmp(info.ssid, ssid) && !strcmp(info.pwd, pwd)) {
        log_info("wifi_sta_connect: ssid %s already connect", ssid);
        return;
    }
    
    wifi_set_sta_connect_best_ssid(0);
    wifi_enter_sta_mode(ssid, pwd);
}

void wifi_sta_connected(void)
{
    if (g_wifi_connection_timer) {
        sys_timeout_del(g_wifi_connection_timer);
        g_wifi_connection_timer = 0;
    }

    wifi_sta_save_current_ssid();    
    if (g_is_in_config_network_state == 0) {
        struct net_event net = {0};
        net.arg = "net";
        net.event = NET_EVENT_CONNECTED;
        net_event_notify(NET_EVENT_FROM_USER, &net);
    }else{
        g_is_in_config_network_state = 0;
        struct app_event event = {
            .event = APP_EVENT_WIFI_CFG_FINISH,
            .arg = NULL
        };
        app_event_notify(APP_EVENT_FROM_USER, &event);
    }
    
}

static int wifi_event_callback(void *network_ctx, enum WIFI_EVENT event)
{
    struct net_event net = {0};
    net.arg = "net";
    int ret = 0;

    switch (event) {
    case WIFI_EVENT_MODULE_INIT:
        
        wifi_set_sta_connect_timeout(30);   //配置STA模式连接超时后事件回调通知时间

        break;

    case WIFI_EVENT_MODULE_START:
        log_info("network_user_callback->WIFI_EVENT_MODULE_START");

        struct wifi_stored_sta_info wifi_stored_sta_info[32];
        int cnt = get_stored_sta_list(wifi_stored_sta_info);

        /*
        strcpy((char *)wifi_stored_sta_info[0].ssid, "0812");
        strcpy((char *)wifi_stored_sta_info[0].pwd, "SSPY0812");
        cnt = 1;
        */
        if (cnt > 0) {
            strncpy((char *)g_wifi_info.ssid, (const char *)wifi_stored_sta_info[0].ssid, sizeof(g_wifi_info.ssid) - 1);
            strncpy((char *)g_wifi_info.pwd, (const char *)wifi_stored_sta_info[0].pwd, sizeof(g_wifi_info.pwd) - 1);
        }else{
            net.event = NET_CONNECT_TIMEOUT_NOT_FOUND_SSID;
            net_event_notify(NET_EVENT_FROM_USER, &net);
        } 

        struct wifi_store_info wifi_default_mode_parm;
        memset(&wifi_default_mode_parm, 0, sizeof(struct wifi_store_info));
        wifi_default_mode_parm.mode = STA_MODE;
        strncpy((char *)wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE],
                (const char *)g_wifi_info.pwd,
                sizeof(wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE]) - 1);

        wifi_default_mode_parm.connect_best_network = CONNECT_BEST_SSID;

        wifi_set_default_mode(&wifi_default_mode_parm, FORCE_DEFAULT_MODE, wifi_default_mode_parm.mode == STA_MODE); //配置STA模式情况下,把默认配置SSID也存储起来,以后即使保存过其他SSID,也不会覆盖丢失,使用连接最优信号SSID策略的情况下可以匹配连接
        

        
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
        break;
    case WIFI_EVENT_MODULE_STOP:
        log_info("network_user_callback->WIFI_EVENT_MODULE_STOP");
        break;

    case WIFI_EVENT_AP_START:
        log_info("network_user_callback->WIFI_EVENT_AP_START,CH=%d", wifi_get_channel());
        //wifi_rxfilter_cfg(7);    //过滤广播+多播+not_my_bssid
        break;
    case WIFI_EVENT_AP_STOP:
        log_info("network_user_callback->WIFI_EVENT_AP_STOP");
        break;

    case WIFI_EVENT_STA_START:
        log_info("network_user_callback->WIFI_EVENT_STA_START");
        break;
    case WIFI_EVENT_MODULE_START_ERR:
        log_info("network_user_callback->WIFI_EVENT_MODULE_START_ERR");
        break;
    case WIFI_EVENT_STA_STOP:
        log_info("network_user_callback->WIFI_EVENT_STA_STOP");
        break;
    case WIFI_EVENT_STA_DISCONNECT:
        log_info("network_user_callback->WIFI_STA_DISCONNECT");

        /*wifi_rxfilter_cfg(0);*/

#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
        if (!mac_addr_succ_flag) {
            break;
        }
#endif

        net.event = NET_EVENT_DISCONNECTED;
        net_event_notify(NET_EVENT_FROM_USER, &net);

#ifndef WIFI_MODE_CYCLE_TEST
        //if (!request_connect_flag) { //如果是应用程序主动请求连接导致断线就不需要发送重连事件, 否则像信号不好导致断线的原因就发送重连事件
            // net.event = NET_EVENT_DISCONNECTED_AND_REQ_CONNECT;
            // net_event_notify(NET_EVENT_FROM_USER, &net);
        //}
#endif
        break;

    case WIFI_EVENT_STA_SCANNED_SSID:
        log_info("network_user_callback->WIFI_EVENT_STA_SCANNED_SSID");
        break;
    case WIFI_EVENT_STA_SCAN_COMPLETED:
        log_info("network_user_callback->WIFI_STA_SCAN_COMPLETED");
        struct wifi_scan_ssid_info *sta_ssid_info;
        u32 sta_ssid_num = 0;
        sta_ssid_info = wifi_get_scan_result(&sta_ssid_num);
        log_info("scan done, num=%d\r\n", sta_ssid_num);
        /*
        for (u32 i = 0; i < sta_ssid_num; i++) {
            printf("2:wifi_get_scan_result[%d]= %s, ch=%d, rssi=%d\r\n", i, sta_ssid_info[i].ssid, sta_ssid_info[i].channel_number, sta_ssid_info[i].rssi_db);
        }*/
        free(sta_ssid_info);
#ifdef CONFIG_AIRKISS_NET_CFG
        extern void airkiss_ssid_check(void);
        airkiss_ssid_check();
#endif
        void scan_list_sem_post(void);
        scan_list_sem_post();
        break;
    case WIFI_EVENT_STA_CONNECT_SUCC:
        log_info("network_user_callback->WIFI_STA_CONNECT_SUCC,CH=%d", wifi_get_channel());
        /*wifi_rxfilter_cfg(3);    //过滤not_my_bssid,如果需要使用扫描空中SSID就不要过滤*/
#ifdef CONFIG_STATIC_IPADDR_ENABLE
        if (0 == compare_dhcp_ipaddr()) {
            wifi_set_sta_ip_info();
            ret = 1;
        }
#endif

#if 0
        u8 txop_limit, cwmin, cwmax, aifsn;
        wifi_edca_parm_get(0, &txop_limit, &cwmin, &cwmax, &aifsn);
        printf("txop_limit: %d, cwmin: %d, cwmax: %d, aifsn: %d\n", txop_limit, cwmin, cwmax, aifsn);
        wifi_edca_parm_set(0, 30, 4, 10, 2);
#endif
        break;

    case WIFI_EVENT_MP_TEST_START:
        log_info("network_user_callback->WIFI_EVENT_MP_TEST_START");
        break;
    case WIFI_EVENT_MP_TEST_STOP:
        log_info("network_user_callback->WIFI_EVENT_MP_TEST_STOP");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        log_info("network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID");
        net.event = NET_CONNECT_TIMEOUT_NOT_FOUND_SSID;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_STA_CONNECT_ASSOCIAT_FAIL:
        log_info("network_user_callback->WIFI_STA_CONNECT_ASSOCIAT_FAIL");
        net.event = NET_CONNECT_ASSOCIAT_FAIL;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT:
        log_info("network_user_callback->WIFI_STA_CONNECT_ASSOCIAT_TIMEOUT");
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        log_info("network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC");

        //有些使用了加密的路由器刚获取IP地址后前几个包都会没响应，怀疑路由器没转发成功
        void connect_broadcast(void);
        connect_broadcast();
        wifi_sta_connected(); 
#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
        if (!is_server_assign_macaddr_ok()) { //如果使用服务器分配MAC地址的情况,需要确认MAC地址有效才发送连接成功事件到APP层,否则先访问服务器分配mac地址
            server_assign_macaddr(wifi_return_sta_mode);
            break;
        }
        mac_addr_succ_flag = 1;
#endif
#ifdef CONFIG_STATIC_IPADDR_ENABLE
        store_dhcp_ipaddr();
#endif       
        break;
    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT:
        log_info("etwork_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_TIMEOUT");
        break;

    case WIFI_EVENT_SMP_CFG_START:
        log_info("network_user_callback->WIFI_EVENT_SMP_CFG_START");
        break;
    case WIFI_EVENT_SMP_CFG_STOP:
        log_info("network_user_callback->WIFI_EVENT_SMP_CFG_STOP");
        break;
    case WIFI_EVENT_SMP_CFG_TIMEOUT:
        log_info("network_user_callback->WIFI_EVENT_SMP_CFG_TIMEOUT");
        net.event = NET_EVENT_SMP_CFG_TIMEOUT;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;
    case WIFI_EVENT_SMP_CFG_COMPLETED:
        log_info("network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED");
        net.event = NET_SMP_CFG_COMPLETED;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_AP_ON_ASSOC:
        struct eth_addr *hwaddr = (struct eth_addr *)network_ctx;
        log_info("WIFI_EVENT_AP_ON_ASSOC hwaddr = %02x:%02x:%02x:%02x:%02x:%02x",
                 hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
        break;
    case WIFI_EVENT_AP_ON_DISCONNECTED:
        struct ip4_addr ipaddr;
        hwaddr = (struct eth_addr *)network_ctx;
        dhcps_get_ipaddr(hwaddr->addr, &ipaddr);
        log_info("WIFI_EVENT_AP_ON_DISCONNECTED hwaddr = %02x:%02x:%02x:%02x:%02x:%02x, ipaddr = [%d.%d.%d.%d]",
                 hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5],
                 ip4_addr1(&ipaddr), ip4_addr2(&ipaddr), ip4_addr3(&ipaddr), ip4_addr4(&ipaddr));
        break;
    case WIFI_EVENT_STA_IP_GOT_IPV6_SUCC:
        log_info("network_user_callback->WIFI_EVENT_STA_IP_GOT_IPV6_SUCC");
        break;

    case WIFI_EVENT_P2P_START:
        log_info("network_user_callback->WIFI_EVENT_P2P_START");
        break;
    case WIFI_EVENT_P2P_STOP:
        log_info("network_user_callback->WIFI_EVENT_P2P_STOP");
        break;
    case WIFI_EVENT_P2P_GC_CONNECTED:
        log_info("network_user_callback->WIFI_EVENT_P2P_GC_CONNECTED");
        break;
    case WIFI_EVENT_P2P_GC_DISCONNECTED:
        log_info("network_user_callback->WIFI_EVENT_P2P_GC_DISCONNECTED");
        //wifi_enter_p2p_mode(P2P_GC_MODE, WIFI_P2P_DEVICE_NAME);
        break;
    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC:
        log_info("network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC");
        break;
    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT:
        log_info("network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT");
        break;
    case WIFI_EVENT_P2P_GO_STA_CONNECTED:
        log_info("network_user_callback->WIFI_EVENT_P2P_GO_STA_CONNECTED");
        break;
    case WIFI_EVENT_P2P_GO_STA_DISCONNECTED:
        log_info("network_user_callback->WIFI_EVENT_P2P_GO_STA_DISCONNECTED");
        //wifi_enter_p2p_mode(P2P_GO_MODE, WIFI_P2P_DEVICE_NAME);
        break;
    default:
        break;
    }

    return ret;
}

static void wifi_rx_cb(void *rxwi, struct ieee80211_frame *wh, void *data, u32 len, void *priv)
{
    char *str_frm_type;

    switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) {
    case IEEE80211_FC0_TYPE_MGT:
        switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) {
        case IEEE80211_FC_STYPE_ASSOC_REQ:
            str_frm_type = "association req";
            break;
        case IEEE80211_FC_STYPE_ASSOC_RESP:
            str_frm_type = "association resp";
            break;
        case IEEE80211_FC_STYPE_REASSOC_REQ:
            str_frm_type = "reassociation req";
            break;
        case IEEE80211_FC_STYPE_REASSOC_RESP:
            str_frm_type = "reassociation resp";
            break;
        case IEEE80211_FC_STYPE_PROBE_REQ:
            str_frm_type = "probe req";
            break;
        case IEEE80211_FC_STYPE_PROBE_RESP:
            str_frm_type = "probe resp";
            break;
        case IEEE80211_FC_STYPE_BEACON:
            str_frm_type = "beacon";
            break;
        case IEEE80211_FC_STYPE_ATIM:
            str_frm_type = "atim";
            break;
        case IEEE80211_FC_STYPE_DISASSOC:
            str_frm_type = "disassociation";
            break;
        case IEEE80211_FC_STYPE_AUTH:
            str_frm_type = "authentication";
            break;
        case IEEE80211_FC_STYPE_DEAUTH:
            str_frm_type = "deauthentication";
            break;
        case IEEE80211_FC_STYPE_ACTION:
            str_frm_type = "action";
            break;
        default:
            str_frm_type = "unknown mgmt";
            break;
        }
        break;
    case IEEE80211_FC0_TYPE_CTL:
        str_frm_type = "control";
        break;
    case IEEE80211_FC0_TYPE_DATA:
        str_frm_type = "data";
        break;
    default:
        str_frm_type = "unknown";
        break;
    }

    log_info("wifi recv:%s", str_frm_type);
}



void enter_wifi_config_mode(void)
{
    if (!wifi_is_on()) {
        log_info("WIFI is OFF, can not enter config mode");
        return;
    }
    g_is_in_config_network_state = 1;
}

int init_network_connection_timeout(void)
{
    log_info("init_network_connection_timeout\n");
    enter_wifi_config_mode();
    struct net_event net = {0};
    net.arg = "net";
    net.event = NET_CONNECT_TIMEOUT_NOT_FOUND_SSID;
    net_event_notify(NET_EVENT_FROM_USER, &net);
    return 0;
}

int start_wifi_network(void)
{
    wifi_set_store_ssid_cnt(NETWORK_SSID_INFO_CNT);
    wifi_set_event_callback(wifi_event_callback);

    wifi_on();
    wifi_set_long_retry(4);
    wifi_set_short_retry(7);

    wifi_get_mac(g_mac_addr);
    log_info("WIFI MAC: %02x:%02x:%02x:%02x:%02x:%02x", g_mac_addr[0], g_mac_addr[1], g_mac_addr[2], g_mac_addr[3], g_mac_addr[4], g_mac_addr[5]);

    //wifi_on之后即可初始化服务器类型的网络应用程序
#ifdef CONFIG_IPERF_ENABLE
    extern void iperf_test(void);
    iperf_test();
#endif


    g_wifi_connection_timer = sys_timeout_add_to_task("sys_timer", NULL, init_network_connection_timeout, 60000); // 60秒后如果还连不上就进入配置网络状态
    
    log_info("start_wifi_network, ssid=%s, pwd=%s", g_wifi_info.ssid, g_wifi_info.pwd);
    wifi_sta_connect(g_wifi_info.ssid, g_wifi_info.pwd);
    log_info("start_wifi_network: done");

    //wifi raw 测试
    //wifi_raw_test();
    //while (1) {
    //    os_time_dly(50 * 100);
    //}
  
    return 0;
}



void wifi_status(void)
{
    
    if (wifi_is_on()) {
        /* stats_display(); //LWIP stats */
        log_info("WIFI U= %d KB/s, D= %d KB/s", wifi_get_upload_rate() / 1024, wifi_get_download_rate() / 1024);

        log_info("Router_RSSI=%d,Quality=%d", wifi_get_rssi(), wifi_get_cqi()); //侦测路由器端信号质量
/*
        int ret = wifi_scan_req();
        if (ret == 0) {
            printf("WIFI SCANing");
        } else if (ret == -1) {
            printf("WIFI SCAN Busy");
        } */
    }
/*
    struct app_event event = {
        .event = APP_EVENT_DATA,
        .arg = "wifi"
    };
    app_event_notify(APP_EVENT_FROM_USER, &event);*/
}



int get_wifi_mac(char* buf){

    return wifi_get_mac(buf);

}

int start_scan_wifi(void){
    wifi_clear_scan_result(); 
    return wifi_scan_req();
}


struct wifi_scan_ssid_info *get_wifi_scan_result(u32 *ssid_cnt){
    return wifi_get_scan_result(ssid_cnt);
}

int config_wifi_param(char *ssid, char *pwd){
    if (strlen(ssid) > WIFI_SSID_LEN || strlen(pwd) > WIFI_PWD_LEN) {
        return -1;
    }
    if (strcmp(ssid, g_wifi_info.ssid) == 0 && strcmp(pwd, g_wifi_info.pwd) == 0) {
        return 0;
    }

    strncpy(g_wifi_info.ssid, ssid, WIFI_SSID_LEN);
    g_wifi_info.ssid[WIFI_SSID_LEN] = 0;
    strncpy(g_wifi_info.pwd, pwd, WIFI_PWD_LEN);
    g_wifi_info.pwd[WIFI_PWD_LEN] = 0;
    g_wifi_info.update = 1;
    return 0;
}

int comfirm_wifi_param(void){
    if (g_wifi_info.update == 0) {
        enum wifi_sta_connect_state status = wifi_get_sta_connect_state();
        if(status == WIFI_STA_NETWORK_STACK_DHCP_SUCC || status == WIFI_STA_CONNECT_SUCC) {
            log_info("comfirm_wifi_param: already connecting or connected");
            return 0;
        }
    }

    log_info("comfirm_wifi_param: ssid=%s, pwd=%s", g_wifi_info.ssid, g_wifi_info.pwd);
    wifi_set_sta_connect_best_ssid(0);
    wifi_enter_sta_mode(g_wifi_info.ssid, g_wifi_info.pwd);
    return 0;
}

int check_wifi_connected(void){
    if (wifi_is_on()) {
        enum wifi_sta_connect_state status = wifi_get_sta_connect_state();
        if (status == WIFI_STA_CONNECTING || status == WIFI_STA_CONNECT_SUCC || status == WIFI_STA_DISCONNECT) {      
            return -1;
        }
        
        int ret = 0;
        if (status == WIFI_STA_NETWORK_STACK_DHCP_SUCC || status == WIFI_STA_IP_GOT_IPV6_SUCC) {  
            struct wifi_mode_info info;
            info.mode = STA_MODE;
            wifi_get_mode_cur_info(&info);
            if (strncmp(info.ssid, g_wifi_info.ssid, WIFI_SSID_LEN) == 0) {
                return 0;
            }    
            log_info("connected to other ssid=%s", info.ssid);
            return 2;
        }else{
            return status + 10;
        }
        
    }
    return 1;
}

u8 check_if_in_config_network_state(void){
    log_info("check_if_in_config_network_state=%d", g_is_in_config_network_state);
    return g_is_in_config_network_state;
}

#endif
