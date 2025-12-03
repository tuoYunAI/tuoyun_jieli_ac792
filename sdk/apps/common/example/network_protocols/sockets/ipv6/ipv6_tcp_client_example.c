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
#define SERVER_ADDR "fe80::ce37:36c1:39e0:a514%27"
#define BUF_SIZE 512
#define LOG_TAG "[IPv6 TCP Client]"
static const char send_data[] = "IPv6 TCP Client Test!";


void ipv6_tcp_client(void)
{
    char *recv_buf = NULL;
    void *sockfd;
    unsigned int bytes_receive;
    struct sockaddr_in6 server_addr;

    /*申请接收buffer内存 */
    recv_buf = malloc(BUF_SIZE);
    if (NULL == recv_buf) {
        printf(LOG_TAG"malloc err\n");
        return;
    }

    /*创建socket*/
    if ((sockfd = sock_reg(PF_INET6, SOCK_STREAM, 0, NULL, NULL)) == NULL) {
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

    if (0 != sock_connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in6))) {
        printf(LOG_TAG"sock_connect err\n");
        free(recv_buf);
        return;
    }

    while (1) {
        bytes_receive = sock_recv(sockfd, recv_buf, BUF_SIZE, 0);

        if (bytes_receive <= 0) {
            sock_unreg(sockfd);
            free(recv_buf);
            break;
        }

        recv_buf[bytes_receive] = '\0';

        if (strcmp(recv_buf, "q") == 0 || strcmp(recv_buf, "Q") == 0) {
            sock_unreg(sockfd);
            free(recv_buf);
            break;
        } else {
            /*打印接收到的数据*/
            printf(LOG_TAG"data received:%s\n", recv_buf);
        }

        /*发送指定数据到客户端*/
        sock_send(sockfd, send_data, strlen(send_data), 0);
    }
}

static void ipv6_tcp_client_start(void *priv)
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

    ipv6_tcp_client();
}

void ipv6_tcp_client_main(void *priv)
{
    if (thread_fork("ipv6_tcp_client_start", 10, 512, 0, NULL, ipv6_tcp_client_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(ipv6_tcp_client_main);
#endif

