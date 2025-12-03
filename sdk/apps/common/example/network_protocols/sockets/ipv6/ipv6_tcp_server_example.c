
#include "sock_api/sock_api.h"
#include "os/os_api.h"
#include "app_config.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#if LWIP_IPV6

#define SERVER_PORT 32768
#define  BACK_LOG 3
#define BUF_SIZE 512
static const char send_data[] = "IPv6 TCP Server Test!";
#define LOG_TAG "[IPv6 TCP Server]"

void ipv6_tcp_server(void)
{
    char *recv_buf = NULL;
    void *sockfd, *clientfd = NULL;
    bool stop = false;
    unsigned int bytes_receive;
    unsigned int opt = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 server_addr, client_addr;

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

    /*初始化服务端地址*/
    server_addr.sin6_family = AF_INET6;
    memcpy(server_addr.sin6_addr.s6_addr, IP6_ADDR_ANY, 16);
    server_addr.sin6_port = htons(SERVER_PORT);

    /*绑定socket到服务端地址*/
    if (sock_bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        printf(LOG_TAG"malloc err\n");
        free(recv_buf);
        return;
    }

    /*监听socket, 设置最大连接数量BACK_LOG*/
    if (sock_listen(sockfd, BACK_LOG) != 0) {
        printf(LOG_TAG"malloc err\n");
        free(recv_buf);
        return;
    }

    while (stop != true) {
        /* 等待客户端连接*/
        clientfd = sock_accept(sockfd, (struct sockaddr *)&client_addr, &opt, NULL, NULL);
        printf(LOG_TAG"from(IP:%s, PORT:%d)\n", inet6_ntoa(client_addr.sin6_addr), ntohs(client_addr.sin6_port));

        while (1) {
            /*发送指定数据到客户端*/
            sock_send(clientfd, send_data, strlen(send_data), 0);

            bytes_receive = sock_recv(clientfd, recv_buf, BUF_SIZE, 0);
            if (bytes_receive <= 0) {
                /*关闭客户端socket*/
                sock_unreg(clientfd);
                break;
            }

            recv_buf[bytes_receive] = '\0';

            if (strcmp(recv_buf, "q") == 0 || strcmp(recv_buf, "Q") == 0) {
                /*关闭客户端socket*/
                sock_unreg(clientfd);
                break;
            } else if (strcmp(recv_buf, "exit") == 0) {
                /*关闭客户端socket*/
                sock_unreg(clientfd);

                /*关闭tcp 服务端*/
                stop = true;
                break;
            } else {
                /*打印接收到的数据*/
                printf(LOG_TAG"data received:%s\n", recv_buf);
            }
        }
    }


    /*关闭tcp 服务端socket*/
    sock_unreg(sockfd);
    free(recv_buf);
}

static void ipv6_tcp_server_start(void *priv)
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

    ipv6_tcp_server();
}

void ipv6_tcp_server_main(void *priv)
{
    if (thread_fork("ipv6_tcp_server_start", 10, 512, 0, NULL, ipv6_tcp_server_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(ipv6_tcp_server_main);

#endif


