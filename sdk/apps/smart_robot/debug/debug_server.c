#include "typedef.h"
#include "app_config.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/includes.h"

#include "sock_api/sock_api.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "debug_server.h"
#include "app_tone.h"


#define MAX_RECV_BUF_SIZE   50 //单次能接收数据的最大字节数(Bytes)

static u8* buf;
static cbuffer_t cbuf;
static buf_len = 1024*10;
static int port = 6000;


static int accept_pid = -1;    //tcp_sock_accpet线程的句柄
static int recv_pid = -1;      //tcp_recv_accpet线程的句柄

//tcp_client数据结构
struct tcp_client_info {
    struct list_head entry;         //链表节点
    struct sockaddr_in remote_addr; //tcp_client地址信息
    void *fd;                       //建立连接后的套接字
};

//tcp_server数据结构
struct tcp_server_info {
    int inited;
    struct list_head client_head;  //链表
    struct sockaddr_in local_addr; //tcp_server地址信息
    void *fd;                      //tcp_server套接字
    OS_MUTEX tcp_mutex;            //互斥锁
    OS_MUTEX buf_mutex;
};

static struct tcp_server_info server_info = {.inited = 0};

static void tcp_sock_accpet(void);
static void tcp_recv_handler(void);
static struct tcp_client_info *get_tcp_client_info(void);
static void tcp_client_quit(struct tcp_client_info *priv);


//功  能：向tcp_client发送消息
//参  数：void *sock_hdl ：socket句柄
//        const void *buf：需要发送的数据BUF
//        u32 len        : BUF大小
//返回值：返回发送成功的数据的字节数；返回-1，发送失败
static int tcp_send_data(void *sock_hdl, const void *buf, u32 len)
{
    return sock_send(sock_hdl, buf, len, 0);
}


//功  能：接收tcp_client的数据
//参  数：void *sock_hdl ：socket句柄
//        void *buf      ：需要接收数据BUF
//        u32 len        : BUF大小
//返回值：返回接收成功的数据的字节数；返回-1，发送失败
static int tcp_recv_data(void *sock_hdl, void *buf, u32 len)
{
    return sock_recv(sock_hdl, buf, len, 0);
}


//功  能：tcp_server接收线程，用于接收tcp_client的数据
//参  数: 无
//返回值：无
static void tcp_recv_handler(void)
{
    struct tcp_client_info *client_info = NULL;
    char recv_buf[MAX_RECV_BUF_SIZE] = {0};

_reconnect_:

    do {
        client_info = get_tcp_client_info();
        os_time_dly(5);
    } while (client_info == NULL);

    for (;;) {
        if (tcp_recv_data(client_info->fd, recv_buf, sizeof(recv_buf)) > 0) {
            printf("Received data from (ip : %s, port : %d)\r\n", inet_ntoa(client_info->remote_addr.sin_addr), client_info->remote_addr.sin_port);
            printf("recv_buf = %s.\n", recv_buf);
            memset(recv_buf, 0, sizeof(recv_buf));
            //此处可添加数据处理函数
            tcp_send_data(client_info->fd, "Data received successfully!", strlen("Data received successfully!"));
        } else {
            tcp_client_quit(client_info);
            goto _reconnect_;
        }
    }
}

//功  能：tcp_sock_accpet线程，用于接收tcp_client的连接请求
//参  数: void *priv：NULL
//返回值：无
static void tcp_sock_accpet(void)
{
    socklen_t len = sizeof(server_info.local_addr);

    for (;;) {
        struct tcp_client_info *client_info = calloc(1, sizeof(struct tcp_client_info));
        if (client_info == NULL) {
            printf(" %s calloc fail\n", __FILE__);
            return;
        }

        client_info->fd  = sock_accept(server_info.fd, (struct sockaddr *)&client_info->remote_addr, &len, NULL, NULL);
        if (client_info->fd == NULL) {
            printf("%s socket_accept fail\n",  __FILE__);
            return;
        }

        os_mutex_pend(&server_info.tcp_mutex, 0);
        list_add_tail(&client_info->entry, &server_info.client_head);
        os_mutex_post(&server_info.tcp_mutex);

        printf("%s, build connnect success.\n", inet_ntoa(client_info->remote_addr.sin_addr));
    }
}




//功  能：搭建tcp_server
//参  数: int port：端口号
//返回值：返回0，成功；返回-1，失败
static int tcp_server_init(int port)
{
    u32 opt = 1;

    memset(&server_info, 0, sizeof(server_info));

    server_info.local_addr.sin_family = AF_INET;
    server_info.local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_info.local_addr.sin_port = htons(port);

    server_info.fd = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (server_info.fd == NULL) {
        printf("%s build socket fail\n",  __FILE__);
        return -1;
    }

    if (sock_setsockopt(server_info.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("%s sock_setsockopt fail\n", __FILE__);
        return -1;
    }

    if (sock_bind(server_info.fd, (struct sockaddr *)&server_info.local_addr, sizeof(struct sockaddr))) {
        printf("%s sock_bind fail\n", __FILE__);
        return -1;
    }

    if (sock_listen(server_info.fd, 0x2) != 0) {
        printf("%s sock_listen fail\n", __FILE__);
        return -1;
    }

    if (os_mutex_create(&server_info.tcp_mutex) != OS_NO_ERR) {
        printf("%s os_mutex_create tcp_mutex fail\n", __FILE__);
        return -1;
    }
    if (os_mutex_create(&server_info.buf_mutex) != OS_NO_ERR) {
        printf("%s os_mutex_create buf_mutex fail\n", __FILE__);
        return -1;
    }

    INIT_LIST_HEAD(&server_info.client_head);

    //创建线程，用于接收tcp_client的连接请求
    if (thread_fork("tcp_sock_accpet", 26, 256, 0, &accept_pid, tcp_sock_accpet, NULL) != OS_NO_ERR) {
        printf("%s thread fork fail\n", __FILE__);
        return -1;
    }

    //创建线程，用于接收tcp_client的数据
    if (thread_fork("tcp_recv_handler", 25, 512, 0, &recv_pid, tcp_recv_handler, NULL) != OS_NO_ERR) {
        printf("%s thread fork fail\n", __FILE__);
        return -1;
    }

    server_info.inited = 1;
    return 0;
}



//功  能：注销tcp_client
//参  数: struct tcp_client_info *priv：需要注销的tcp_client
//返回值：无
static void tcp_client_quit(struct tcp_client_info *priv)
{
    list_del(&priv->entry);
    sock_set_quit(priv->fd);
    sock_unreg(priv->fd);
    priv->fd = NULL;
    free(priv);
}

//功  能：获取当前最新的tcp_client
//参  数: 无
//返回值：返回获取的tcp_client
static struct tcp_client_info *get_tcp_client_info(void)
{
    struct list_head *pos = NULL;
    struct tcp_client_info *client_info = NULL;
    struct tcp_client_info *old_client_info = NULL;


    os_mutex_pend(&server_info.tcp_mutex, 0);

    list_for_each(pos, &server_info.client_head) {
        client_info = list_entry(pos, struct tcp_client_info, entry);
    }

    os_mutex_post(&server_info.tcp_mutex);

    return client_info;
}



//功  能：注销tcp_server
//参  数: 无
//返回值：无
static void tcp_server_exit(void)
{
    struct list_head *pos = NULL;
    struct tcp_client_info *client_info = NULL;

    thread_kill(&accept_pid, KILL_WAIT);
    thread_kill(&recv_pid, KILL_WAIT);

    os_mutex_pend(&server_info.tcp_mutex, 0);

    list_for_each(pos, &server_info.client_head) {
        client_info = list_entry(pos, struct tcp_client_info, entry);
        if (client_info) {
            list_del(&client_info->entry);
            sock_unreg(client_info->fd);
            client_info->fd = NULL;
            free(client_info);
        }
    }

    os_mutex_post(&server_info.tcp_mutex);

    os_mutex_del(&server_info.tcp_mutex, OS_DEL_NO_PEND);
}




static void debug_server_transmiting(void *priv)
{
    int read_len = 0;
    u8 temp_buf[100];

    while (1) {
        if (list_empty(&server_info.client_head)) {
            os_time_dly(5);
            continue;
        }

        int blen = cbuf_get_data_size(&cbuf);
        if(blen == 0){
            continue;;
        }   
        read_len = cbuf_read(&cbuf, temp_buf, blen > sizeof(temp_buf)?sizeof(temp_buf):blen);
        if (read_len > 0) {

            struct list_head *pos = NULL;
            struct tcp_client_info *client_info = NULL;
            struct tcp_client_info *old_client_info = NULL;


            os_mutex_pend(&server_info.tcp_mutex, 0);

            list_for_each(pos, &server_info.client_head) {
                client_info = list_entry(pos, struct tcp_client_info, entry);
                    if (client_info && client_info->fd) {
                    tcp_send_data(client_info->fd, temp_buf, read_len);
                }
            }

            os_mutex_post(&server_info.tcp_mutex);
            
            
        } else {
            os_time_dly(5);
        }
    }
}

static void tcp_server_start(void *priv)
{
    int err;
    enum wifi_sta_connect_state state;

    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }

        os_time_dly(1000);
    }

    err = tcp_server_init(port);
    if (err == -1) {
        printf("tcp_server_init faile\n");
        tcp_server_exit();
        return;
    }

    struct netif_info netif_info;
    lwip_get_netif_info(WIFI_NETIF, &netif_info);
    int idx = 3;
    int gap_ms = 50;
    while(idx >= 0){
        u8 last = (ntohl(netif_info.ip) >> (8*idx)) & 0xFF;
        int cnt  = 0;
        if (last >= 100){
            int num = last / 100;
            play_tone_file(get_tone_files()->num[num]);
            os_time_dly(gap_ms);
            tone_player_stop();
        }
        
        if (last >= 10) {
            int num = (last % 100) / 10;
            play_tone_file(get_tone_files()->num[num]);
            os_time_dly(gap_ms);
            tone_player_stop();
        }
        
        int num = last % 10;
        play_tone_file(get_tone_files()->num[num]);
        os_time_dly(gap_ms);
        tone_player_stop();
        os_time_dly(60);
        idx--;
    }
    
    char* ip = inet_ntoa(netif_info.ip);
    printf("Debug TCP Server started! Listening on %s:%d\n", ip, port);

    if (thread_fork("debug_tcp_server_tranmiting", 16, 1024, 0, NULL, debug_server_transmiting, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

void write_debug_data(u8* data, u32 len)
{

    u32 wlen = cbuf_write(&cbuf, data, len);
    if (wlen == 0) {
        cbuf_clear(&cbuf);
        return;
    }
    return;
}   

/* 格式化并写入日志内容 */
static u8 msg_buf[1024] = {0};
void dbg_print(const char *format, ...)
{
    if (!server_info.inited || !format){
        return;
    }

    os_mutex_pend(&server_info.buf_mutex, 0);
    
    /* 写入日志前缀 */
    u8 pre[20] = {0};
    snprintf(pre, sizeof(pre), "[%08d] ", os_time_get());
    write_debug_data(pre, strnlen(pre, sizeof(pre)));

    
    va_list args;
    va_start(args, format);
    int msg_len = vsnprintf(msg_buf, sizeof(msg_buf), format, args);
    va_end(args);
    
    if (msg_len > 0) {
        write_debug_data(msg_buf, msg_len < sizeof(msg_buf) ? msg_len : sizeof(msg_buf) - 1);
    }
    
    
    os_mutex_post(&server_info.buf_mutex);

    return;
}


void dbg_printf_buf(u8 *buf, u32 len){
    if (!server_info.inited || !buf || len == 0){
        return;
    }

    os_mutex_pend(&server_info.buf_mutex, 0);
    
    /* 以十六进制格式打印缓冲区内容 */
    u8 hex_buf[80];  /* 每行最多16字节,格式为 "XX XX XX ... " */
    u32 i = 0;
    
    while (i < len) {
        int line_len = 0;
        int bytes_in_line = (len - i) > 16 ? 16 : (len - i);
        
        /* 格式化一行的十六进制数据 */
        for (int j = 0; j < bytes_in_line; j++) {
            line_len += snprintf(hex_buf + line_len, sizeof(hex_buf) - line_len, 
                                "%02X ", buf[i + j]);
        }
        
        /* 添加换行符 */
        if (line_len < sizeof(hex_buf) - 1) {
            hex_buf[line_len++] = '\n';
        }
        
        write_debug_data(hex_buf, line_len);
        i += bytes_in_line;
    }
    
    os_mutex_post(&server_info.buf_mutex);

    return;
}



#ifdef TELNET_LOG_OUTPUT

void c_main(void *priv)
{
    buf = malloc(buf_len);
    if (buf == NULL) {
        printf("malloc fail\n");
        return;
    }

    cbuf_init(&cbuf, buf, buf_len);

    if (thread_fork("debug_tcp_server_start", 10, 512, 0, NULL, tcp_server_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

early_initcall(c_main);

#endif