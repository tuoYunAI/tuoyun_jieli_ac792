#include "sock_api/sock_api.h"
#include "os/os_api.h"
#include "app_config.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#if LWIP_IPV6

#define SERVER_PORT 32769
#define BUF_SIZE 512
#define SERVER_ADDR "fe80::ce37:36c1:39e0:a514%27"
#define LOG_TAG "[IPv6 UDP Server]"
static const char send_data[] = "IPv6 UDP Client Test!";

void ipv6_udp_client(void)
{
    void *sockfd = NULL;
    struct sockaddr_in6 server_addr;
    struct sockaddr_in6 client_addr;
    char *recv_buf = NULL;
    unsigned int bytes_receive;
    unsigned int addr_len = sizeof(struct sockaddr);

    /*申请接收buffer内存 */
    recv_buf = malloc(BUF_SIZE);
    if (NULL == recv_buf) {
        printf(LOG_TAG"malloc err\n");
        return;
    }

    /*创建socket*/
    if ((sockfd = sock_reg(AF_INET6, SOCK_DGRAM, 0, NULL, NULL)) == NULL) {
        printf(LOG_TAG"malloc err\n");
        free(recv_buf);
        return;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in6));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET6, SERVER_ADDR, &server_addr.sin6_addr.s6_addr) != 1) {
        printf(LOG_TAG"inet_pton err\n");
        free(recv_buf);
        return;
    }

    if (sock_sendto(sockfd, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0) {
        printf(LOG_TAG"sock_sendto err\n");
        free(recv_buf);
        return;
    }

    bytes_receive = sock_recvfrom(sockfd, recv_buf, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
    if (bytes_receive <= 0) {
        sock_unreg(sockfd);
        free(recv_buf);
        return;
    }

    recv_buf[bytes_receive] = '\0';
    printf(LOG_TAG"from(IP:%s, PORT:%d\n)", inet6_ntoa(client_addr.sin6_addr), ntohs(client_addr.sin6_port));

    /*打印接收到的数据*/
    printf(LOG_TAG"data received:%s\n", recv_buf);

    sock_unreg(sockfd);
    free(recv_buf);
}

static void ipv6_udp_client_start(void *priv)
{
    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(100);
    }

    ipv6_udp_client();
}

void ipv6_udp_client_main(void *priv)
{
    if (thread_fork("ipv6_udp_client_start", 10, 512, 0, NULL, ipv6_udp_client_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(ipv6_udp_client_main);

#endif

