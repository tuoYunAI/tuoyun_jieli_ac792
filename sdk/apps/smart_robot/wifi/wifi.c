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

typedef struct {
    char ssid[WIFI_SSID_LEN + 1];
    char pwd[WIFI_PWD_LEN + 1];
} wifi_credential_t, *wifi_credential_ptr;

typedef struct  {
    int inited;                     //是否初始化
    int total_cnt;                  //备选的SSID总数
    int current_idx;                //当前连接的SSID索引
    int is_in_config_network_state; //是否处于配置网络状态
    int updated;                    // 1:有更新, 0:无更新
    int interruption_cnt;           //连接中断计数
    enum WIFI_MODE mode;
    u8 mac_addr[6];
    wifi_credential_t ssid_pwd[NETWORK_SSID_INFO_CNT];
} wifi_config_info_t, *wifi_config_info_ptr;

static u16 g_wifi_connection_timer = 0;

wifi_config_info_t g_wifi_info = {
    .inited = 0,
    .total_cnt = 0,
    .current_idx = 0,
    .is_in_config_network_state = 0,
    .updated = 0,
    .interruption_cnt = 0,
    .mode = STA_MODE,
    .mac_addr = {0}
};

#define FORCE_DEFAULT_MODE 0 //配置wifi_on之后的模式,0为使用最后记忆的模式, 1为强制默认模式, 3-200为STA连接超时时间多少秒,如果超时都连接不上就连接最后记忆的或者最优网络

#define CONNECT_BEST_SSID  0 //配置如果啟動WIFI后在STA模式下, 是否挑选连接记忆过的信号最优WIFI


#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
static char mac_addr_succ_flag;
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
        log_info("@@@@@@@@wifi_sta_save_current_ssid: ssid or pwd is NULL\r\n");
        return;
    }
    wifi_credential_ptr credential = &g_wifi_info.ssid_pwd[g_wifi_info.current_idx];
    if (strncmp(info.ssid, credential->ssid, WIFI_SSID_LEN) != 0 || strncmp(info.pwd, credential->pwd, WIFI_PWD_LEN) != 0) {
        log_info("@@@@@@@@wifi_sta_save_current_ssid: ssid or pwd changed\r\n");
        return;
    }

    if (!g_wifi_info.updated) {
        log_info("@@@@@@@@wifi_sta_save_current_ssid: no update\r\n");
        return;
    }
    wifi_del_stored_sta_info(info.ssid);
    log_info("@@@@@@@wifi_sta_save_current_ssid: ssid %s pwd %s\r\n", info.ssid, info.pwd);
    
    wifi_store_mode_info(STA_MODE, info.ssid, info.pwd);
    g_wifi_info.updated = 0;
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
    if (g_wifi_info.is_in_config_network_state == 0) {
        struct net_event net = {0};
        net.arg = g_wifi_info.interruption_cnt;
        net.event = NET_EVENT_CONNECTED;
        net_event_notify(NET_EVENT_FROM_USER, &net);
    }else{
        g_wifi_info.is_in_config_network_state = 0;
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

        struct wifi_store_info wifi_default_mode_parm = {
            .mode = STA_MODE,
            .connect_best_network = CONNECT_BEST_SSID            
        };
        wifi_set_default_mode(&wifi_default_mode_parm, FORCE_DEFAULT_MODE, wifi_default_mode_parm.mode == STA_MODE); //配置STA模式情况下,把默认配置SSID也存储起来,以后即使保存过其他SSID,也不会覆盖丢失,使用连接最优信号SSID策略的情况下可以匹配连接
        break;

    case WIFI_EVENT_MODULE_START:
        log_info("network_user_callback->WIFI_EVENT_MODULE_START");
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
        g_wifi_info.interruption_cnt++;
        app_event_t ev = {
            .event = APP_EVENT_WIFI_DISCONNECTED,
            .arg = NULL
        };
        app_event_notify(APP_EVENT_FROM_USER, &ev);
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
        break;
    case WIFI_EVENT_SMP_CFG_COMPLETED:
        log_info("network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED");
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



static void connect_to_wifi(int index){
    if (index < 0 || index >= g_wifi_info.total_cnt) {
        log_info("connect_to_wifi: index %d out of range", index);
        return;
    }
    wifi_credential_ptr credential = &g_wifi_info.ssid_pwd[index];
    log_info("connect_to_wifi: ssid=%s, pwd=%s", credential->ssid, credential->pwd);

    app_event_t event = {
        .event = APP_EVENT_CONNECTING_TO_WIFI,
        .arg = credential->ssid
    };
    app_event_notify(APP_EVENT_FROM_USER, &event);
    
    wifi_set_sta_connect_best_ssid(0);
    wifi_enter_sta_mode(credential->ssid, credential->pwd);
    
}



int init_network_connection_timeout(void)
{
    if (g_wifi_info.current_idx <= 0){
        log_info("init_network_connection_timeout\n");
        struct net_event net = {0};
        net.arg = "net";
        net.event = NET_CONNECT_TIMEOUT_NOT_FOUND_SSID;
        net_event_notify(NET_EVENT_FROM_USER, &net);

        sys_timer_del(g_wifi_connection_timer);
        g_wifi_connection_timer = 0;    
        return 0;
    }

    g_wifi_info.current_idx--;
    connect_to_wifi(g_wifi_info.current_idx);    
}


int start_wifi_network(void)
{
    if (!g_wifi_info.inited) {
        wifi_set_store_ssid_cnt(NETWORK_SSID_INFO_CNT);
        wifi_set_event_callback(wifi_event_callback);
        log_info("start_wifi_network: wifi on");
        wifi_on();
        wifi_set_long_retry(4);
        wifi_set_short_retry(7);

        wifi_get_mac(g_wifi_info.mac_addr);

        struct wifi_stored_sta_info wifi_stored_sta_info[NETWORK_SSID_INFO_CNT+3];
        int cnt = get_stored_sta_list(wifi_stored_sta_info);

        if (cnt > 0) {
            g_wifi_info.total_cnt = cnt > NETWORK_SSID_INFO_CNT ? NETWORK_SSID_INFO_CNT : cnt;
            for (int i = 0; i < g_wifi_info.total_cnt; i++) {
                strncpy(g_wifi_info.ssid_pwd[i].ssid, (const char *)wifi_stored_sta_info[i].ssid, WIFI_SSID_LEN);
                strncpy(g_wifi_info.ssid_pwd[i].pwd, (const char *)wifi_stored_sta_info[i].pwd, WIFI_PWD_LEN);
            }
        }
        g_wifi_info.inited = 1;

        u16 id = sys_timer_add_to_task("sys_timer", NULL, wifi_status, 20 * 1000);
    }
    
    if (g_wifi_info.total_cnt <= 0) {

        struct net_event net = {0};
        net.arg = "net";
        net.event = NET_CONNECT_TIMEOUT_NOT_FOUND_SSID;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        return 0;
    }
    
    log_info("start_connecting_procedure\n");
    if (g_wifi_connection_timer) {
        sys_timeout_del(g_wifi_connection_timer);
        g_wifi_connection_timer = 0;    
    }
    g_wifi_info.current_idx = g_wifi_info.total_cnt - 1;
    connect_to_wifi(g_wifi_info.current_idx);
    g_wifi_connection_timer = sys_timer_add_to_task("sys_timer", NULL, init_network_connection_timeout, 20000); 

    return 0;
}



void wifi_status(void)
{
    if (wifi_is_on()) {
        log_info("WIFI U= %d KB/s, D= %d KB/s", wifi_get_upload_rate() / 1024, wifi_get_download_rate() / 1024);
        log_info("Router_RSSI=%d,Quality=%d", wifi_get_rssi(), wifi_get_cqi()); //侦测路由器端信号质量
    }
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
        log_info("config_wifi_param: ssid or pwd length error");
        return -1;
    }
    int found = 0;
    for (int i = 0; i < g_wifi_info.total_cnt; i++) {
        if (strncmp(ssid, g_wifi_info.ssid_pwd[i].ssid, WIFI_SSID_LEN) == 0) {
            found = 1;
            g_wifi_info.current_idx = i;
            break;
        }
    }

    if (found && strncmp(pwd, g_wifi_info.ssid_pwd[g_wifi_info.current_idx].pwd, WIFI_PWD_LEN) == 0) {
        log_info("config_wifi_param: ssid and pwd no change");
        return 0;
    }

    if (!found) {
        if (g_wifi_info.total_cnt < NETWORK_SSID_INFO_CNT) {
            g_wifi_info.current_idx = g_wifi_info.total_cnt;
            g_wifi_info.total_cnt++;
        }else{
            g_wifi_info.current_idx = 0;  
        }
    }    

    wifi_credential_ptr credential = &g_wifi_info.ssid_pwd[g_wifi_info.current_idx];
    strncpy(credential->ssid, ssid, WIFI_SSID_LEN);
    credential->ssid[WIFI_SSID_LEN] = 0;
    strncpy(credential->pwd, pwd, WIFI_PWD_LEN);
    credential->pwd[WIFI_PWD_LEN] = 0;
    g_wifi_info.updated = 1;
    log_info("config_wifi_param: ssid %s pwd %s, save to index: %d", credential->ssid, credential->pwd, g_wifi_info.current_idx);
    return 0;
}

int comfirm_wifi_param(void){
    if (g_wifi_info.updated == 0) {
        enum wifi_sta_connect_state status = wifi_get_sta_connect_state();
        if(status == WIFI_STA_NETWORK_STACK_DHCP_SUCC || status == WIFI_STA_CONNECT_SUCC) {
            log_info("comfirm_wifi_param: already connecting or connected");
            return 0;
        }
    }
    wifi_credential_ptr credential = &g_wifi_info.ssid_pwd[g_wifi_info.current_idx];
    log_info("comfirm_wifi_param: ssid=%s, pwd=%s", credential->ssid, credential->pwd);
    wifi_set_sta_connect_best_ssid(0);
    wifi_enter_sta_mode(credential->ssid, credential->pwd);
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
            if (strncmp(info.ssid, g_wifi_info.ssid_pwd[g_wifi_info.current_idx].ssid, WIFI_SSID_LEN) == 0) {
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

void enter_config_network_state(void){
    g_wifi_info.is_in_config_network_state = 1;
}

u8 check_if_in_config_network_state(void){
    log_info("check_if_in_config_network_state=%d", g_wifi_info.is_in_config_network_state);
    return g_wifi_info.is_in_config_network_state;
}

#endif
