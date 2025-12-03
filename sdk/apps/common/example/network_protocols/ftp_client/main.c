#include "system/includes.h"
#include "os/os_api.h"
#include <stdlib.h>
#include "app_config.h"
#include "wifi/wifi_connect.h"
#include "ftpclient/ftpclient.h"

static char t_buf[4 * 1024];
static char s_buf[16 * 1024];
static int file_len = sizeof(s_buf);
static int send_len = 0;

int ftp_get_hdl(void *ftp_ctx, char *buf, unsigned int bufsize, void *priv)
{
    put_buf(buf, 32);
    return bufsize;
}

int ftp_put_hdl(void *ftp_ctx, char *buf, unsigned int bufsize, void *priv)
{
    int len;
    len = file_len > bufsize ? bufsize : file_len;
    memcpy(buf, s_buf + send_len, len);
    send_len += len;
    file_len -= len;

    //len < 0 ,为错误; len = 0, 为传输结束，因此当文件传输完成后需要return 0;
    return len;
}

void ftp_client_demo(void)
{
    ftp_ctx ctx;
    ctx.hostname = "192.168.1.101";
    ctx.username = "username";
    ctx.passwd = "passwd";
#if 0
    ctx.method.write.buf = t_buf;
    ctx.method.write.buf_size = sizeof(t_buf);
    ctx.method.write.filename = "filename";
    ctx.method.write.ftp_write = ftp_get_hdl;
    ftp_get(&ctx);
#else
    memset(s_buf, 0xcc, sizeof(s_buf));
    ctx.method.read.buf = t_buf;
    ctx.method.read.buf_size = sizeof(t_buf);
    ctx.method.read.filename = "filename";
    ctx.method.read.ftp_read = ftp_put_hdl;
    ftp_put(&ctx);
#endif
}

static void ftp_client_task(void *priv)
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
        os_time_dly(200);
    }
    ftp_client_demo();
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("ftp_client_task", 10, 2 * 1024, 0, NULL, ftp_client_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
late_initcall(c_main);

