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

//#define  SENDING_END_MODE
#define  RECEIVING_END_MODE

#define MAC_ADDR_LEN 6
#define HEAD_802_11_OFFSET 20

typedef struct _head_802_11 {
    unsigned short  fc;
    unsigned short  duration;
    unsigned char   addr1[MAC_ADDR_LEN];
    unsigned char   addr2[MAC_ADDR_LEN];
    unsigned char	addr3[MAC_ADDR_LEN];
    unsigned short	frag: 4;
    unsigned short	sequence: 12;
    unsigned char 	data[0];
} head_802_11, *phead_802_11;

#ifdef SENDING_END_MODE
#define UDP_TEST_PORT 32768
#define DEST_IP_ADDR "192.168.1.2"
static u8 send_buf[1500];

struct lan_setting lan_setting_info = {
    .WIRELESS_IP_ADDR0  = 192,
    .WIRELESS_IP_ADDR1  = 168,
    .WIRELESS_IP_ADDR2  = 1,
    .WIRELESS_IP_ADDR3  = 3,

    .WIRELESS_NETMASK0  = 255,
    .WIRELESS_NETMASK1  = 255,
    .WIRELESS_NETMASK2  = 255,
    .WIRELESS_NETMASK3  = 0,

    .WIRELESS_GATEWAY0  = 192,
    .WIRELESS_GATEWAY1  = 168,
    .WIRELESS_GATEWAY2  = 1,
    .WIRELESS_GATEWAY3  = 1,

    .SERVER_IPADDR1  = 192,
    .SERVER_IPADDR2  = 168,
    .SERVER_IPADDR3  = 1,
    .SERVER_IPADDR4  = 1,

    .CLIENT_IPADDR1  = 192,
    .CLIENT_IPADDR2  = 168,
    .CLIENT_IPADDR3  = 1,
    .CLIENT_IPADDR4  = 2,

    .SUB_NET_MASK1   = 255,
    .SUB_NET_MASK2   = 255,
    .SUB_NET_MASK3   = 255,
    .SUB_NET_MASK4   = 0,
};
#endif

#ifdef RECEIVING_END_MODE
#define UDP_TEST_PORT 32768
static u8 recv_buf[1500] = {0};

struct lan_setting lan_setting_info = {
    .WIRELESS_IP_ADDR0  = 192,
    .WIRELESS_IP_ADDR1  = 168,
    .WIRELESS_IP_ADDR2  = 1,
    .WIRELESS_IP_ADDR3  = 2,

    .WIRELESS_NETMASK0  = 255,
    .WIRELESS_NETMASK1  = 255,
    .WIRELESS_NETMASK2  = 255,
    .WIRELESS_NETMASK3  = 0,

    .WIRELESS_GATEWAY0  = 192,
    .WIRELESS_GATEWAY1  = 168,
    .WIRELESS_GATEWAY2  = 1,
    .WIRELESS_GATEWAY3  = 1,

    .SERVER_IPADDR1  = 192,
    .SERVER_IPADDR2  = 168,
    .SERVER_IPADDR3  = 1,
    .SERVER_IPADDR4  = 1,

    .CLIENT_IPADDR1  = 192,
    .CLIENT_IPADDR2  = 168,
    .CLIENT_IPADDR3  = 1,
    .CLIENT_IPADDR4  = 2,

    .SUB_NET_MASK1   = 255,
    .SUB_NET_MASK2   = 255,
    .SUB_NET_MASK3   = 255,
    .SUB_NET_MASK4   = 0,
};
#endif

static int lwip_set_lan_info(struct lan_setting *__lan_setting_info)
{
    struct lan_setting *lan_setting_info = net_get_lan_info(WIFI_RAW_NETIF);
    if (!lan_setting_info) {
        return -1;
    }

    lan_setting_info->WIRELESS_IP_ADDR0  = __lan_setting_info->WIRELESS_IP_ADDR0;
    lan_setting_info->WIRELESS_IP_ADDR1  = __lan_setting_info->WIRELESS_IP_ADDR1;
    lan_setting_info->WIRELESS_IP_ADDR2  = __lan_setting_info->WIRELESS_IP_ADDR2;
    lan_setting_info->WIRELESS_IP_ADDR3  = __lan_setting_info->WIRELESS_IP_ADDR3;

    lan_setting_info->WIRELESS_NETMASK0  = __lan_setting_info->WIRELESS_NETMASK0;
    lan_setting_info->WIRELESS_NETMASK1  = __lan_setting_info->WIRELESS_NETMASK1;
    lan_setting_info->WIRELESS_NETMASK2  = __lan_setting_info->WIRELESS_NETMASK2;
    lan_setting_info->WIRELESS_NETMASK3  = __lan_setting_info->WIRELESS_NETMASK3;

    lan_setting_info->WIRELESS_GATEWAY0  = __lan_setting_info->WIRELESS_GATEWAY0;
    lan_setting_info->WIRELESS_GATEWAY1  = __lan_setting_info->WIRELESS_GATEWAY1;
    lan_setting_info->WIRELESS_GATEWAY2  = __lan_setting_info->WIRELESS_GATEWAY2;
    lan_setting_info->WIRELESS_GATEWAY3  = __lan_setting_info->WIRELESS_GATEWAY3;

    lan_setting_info->SERVER_IPADDR1     = __lan_setting_info->SERVER_IPADDR1;
    lan_setting_info->SERVER_IPADDR2     = __lan_setting_info->SERVER_IPADDR2;
    lan_setting_info->SERVER_IPADDR3     = __lan_setting_info->SERVER_IPADDR3;
    lan_setting_info->SERVER_IPADDR4     = __lan_setting_info->SERVER_IPADDR4;

    lan_setting_info->CLIENT_IPADDR1     = __lan_setting_info->CLIENT_IPADDR1;
    lan_setting_info->CLIENT_IPADDR2     = __lan_setting_info->CLIENT_IPADDR2;
    lan_setting_info->CLIENT_IPADDR3     = __lan_setting_info->CLIENT_IPADDR3;
    lan_setting_info->CLIENT_IPADDR4     = __lan_setting_info->CLIENT_IPADDR4;

    lan_setting_info->SUB_NET_MASK1      = __lan_setting_info->SUB_NET_MASK1;
    lan_setting_info->SUB_NET_MASK2      = __lan_setting_info->SUB_NET_MASK2;
    lan_setting_info->SUB_NET_MASK3      = __lan_setting_info->SUB_NET_MASK3;
    lan_setting_info->SUB_NET_MASK4      = __lan_setting_info->SUB_NET_MASK4;

    return 0;
}

void wifi_raw_test(void)
{
    //设置静态ip地址
    lwip_set_lan_info(&lan_setting_info);

    //1: 使能socket方式； 0：禁用socket方式
    wifi_raw_on(1);

    char dest_mac[6] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
    char src_mac[6] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88};

    //设置发送包的802.11头部信息, 设置源mac， 目标mac， seq号等信息
    phead_802_11 pHdr = wifi_get_wifi_send_pkg_ptr() + HEAD_802_11_OFFSET;
    memcpy(pHdr->addr1, dest_mac, 6);
    memcpy(pHdr->addr2, src_mac, 6);
    memcpy(pHdr->addr3, dest_mac, 6);
    pHdr->frag = 8;

#ifdef SENDING_END_MODE

    //lwipopt.h 中需要使能 ETHARP_SUPPORT_STATIC_ENTRIES
    //设置静态arp映射 : dest ip and dest mac
    //需要建立arp映射才能正常通信
    lwip_etharp_add_static_entry(DEST_IP_ADDR, dest_mac);

    //创建socket通信
    void *socket_fd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (socket_fd == NULL) {
        printf("%s sock_reg fail\n",  __FILE__);
        return;
    }

    memset(send_buf, 0xAA, sizeof(send_buf));

    struct sockaddr_in remote_addr = {0};
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(DEST_IP_ADDR);
    remote_addr.sin_port = htons(UDP_TEST_PORT);


    for (;;) {
        //发送数据
        sock_sendto(socket_fd, send_buf, sizeof(send_buf), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));
    }
#endif

#ifdef RECEIVING_END_MODE
    //创建socket通信
    void *socket_fd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (socket_fd == NULL) {
        printf("%s sock_reg fail\n",  __FILE__);
        return;
    }

    struct sockaddr_in local_addr = {0};

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(UDP_TEST_PORT);

    if (sock_bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr))) {
        printf("%s sock_bind fail\n", __FILE__);
        return;
    }

    struct sockaddr_in remote_addr = {0};
    socklen_t len = sizeof(remote_addr);
    int recv_len = 0;

    for (;;) {
        //接收udp_client的数据，并获取udp_client的地址信息
        recv_len = sock_recvfrom(socket_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&remote_addr, &len);

        printf("\n recv_len is %d  \n", recv_len);
        //put_buf(recv_buf, recv_len);
        memset(recv_buf, 0, sizeof(recv_buf));
    }

#endif

}



