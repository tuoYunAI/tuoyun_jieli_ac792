#include "pipeline_core.h"
#include "message_event.h"
#include "sock_api/sock_api.h"
#include "app_config.h"
#include "server/rt_stream_pkg.h"
#include "screen_mirror_api.h"

#define LOG_TAG_CONST       SCREEN_MIRROR_ADAPTER
#define LOG_TAG             "[SCREEN_MIRROR_ADAPTER]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"

#ifdef CONFIG_NET_SCR

struct scr_handle {
    u8 channel;
    u8 ref;
    u8 state;
    spinlock_t lock;

    int socket_type;
    void (*ack_cb)(int);
    void *sock_hdl;
    void *cli_sock_hdl;

    u8 sock_fps;
    u8 disp_fps;
    u32 seq;

    struct sockaddr_in cli_addr;
    int src_width;
    int src_height;
    u8 fps;
    u32 old_frame_seq;
    u32 jpg_size;

    int pid;	// 收包线程
};

struct scr_state {
    u8 fps_min;
    u8 fps_max;
    u32 fps_sum;

    float jpg_sz_min;  //KB
    float jpg_sz_max;
    u32 jpg_sz_sum;

    u32 cnt;
};

#define SCR_THREAD_TASK_NAME    "scr_recv"
#define JPG_BUF_MAX_SIZE  250 * 1024
#define CHECK_CODE 0x88
#define CHECK_CODE_NUM 32
#define UDP_MAX_RECV  1 * 1472
static u8 recv_buf[UDP_MAX_RECV];


#define SCR_CHANNEL_MAX     (1)
static struct scr_handle *g_scr_used[SCR_CHANNEL_MAX];
static struct scr_state g_scr_state = {
    .fps_min = 0xFF,
    .fps_max = 0,
    .fps_sum = 0,
    .jpg_sz_min = 1e6f,
    .jpg_sz_max = 0,
    .jpg_sz_sum = 0,
    .cnt = 0,
};

static int bytecmp(unsigned char *p, unsigned char ch, unsigned int num)
{
    while (num-- > 0) {
        if (*p++ != ch) {
            return -1;
        }
    }
    return 0;
}

static int check_channel_legal(pipe_plugin_t *plugin, u8 channel_max)
{
    int channel = 0;
    u8 plugin_name_len = strlen(plugin->ops->name) - 1;
    char plugin_channel = plugin->name[plugin_name_len];

    if (plugin_channel != '\0' &&
        plugin_channel >= '0' &&
        plugin_channel <= '9') {
        channel = plugin_channel - '0';
    } else {
        ASSERT(0, "plugin name is illegal : %s, please check!!", plugin->name);
    }

    if (channel > channel_max) {
        ASSERT(0, "channel is illegal : %s, please check!!", plugin->name);
    }

    return channel;
}

static int scr_init(pipe_plugin_t *plugin)
{
    struct scr_handle *hdl = NULL;

    int channel = check_channel_legal(plugin, SCR_CHANNEL_MAX);
    if (channel < 0) {
        log_error("get channel ,plugin_name err:%s", plugin->ops->name);
        return -1;
    }

    if (!g_scr_used[channel]) {
        hdl = (struct scr_handle *)zalloc(sizeof(struct scr_handle));
    } else {
        hdl = g_scr_used[channel];
    }

    if (!hdl) {
        log_error("scr init hdl:%x", hdl);
        return -1;
    }

    if (hdl->state != PLUGIN_UNINIT) {
        log_debug("%s %s skipped. cur state: %s\r\n", plugin->name, __func__, plugin_state_str(hdl->state));
        if (!plugin->private_data) {
            plugin->private_data = hdl;
        }
        return 0;
    }

    spin_lock_init(&hdl->lock);

    hdl->channel = channel;
    hdl->old_frame_seq = 0;
    plugin->private_data = hdl;
    g_scr_used[channel] = hdl;
    hdl->state = PLUGIN_INITED;

    return 0;
}

static int scr_connect(pipe_plugin_t *prev_plugin, pipe_plugin_t *plugin, int source_channel)
{
    return 0;
}

static int net_scr_sock_init(struct scr_handle *hdl)
{
    int ret;

    if (hdl->socket_type == 0) {
        log_error("socket type is NULL.");
        return -1;
    }

    hdl->sock_hdl = sock_reg(AF_INET, hdl->socket_type, 0, NULL, NULL);
    if (hdl->sock_hdl == NULL) {
        log_error("socket reg failed.");
        return -1;
    }

    if (sock_set_reuseaddr(hdl->sock_hdl)) {
        log_error("socket reuseaddr failed.");
        sock_unreg(hdl->sock_hdl);
        hdl->sock_hdl = NULL;
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port  = htons(NET_SCR_PORT);

    ret = sock_bind(hdl->sock_hdl, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    if (ret) {
        log_error("socket bind failed.");
        sock_unreg(hdl->sock_hdl);
        hdl->sock_hdl = NULL;
        return -1;
    }

    if (hdl->socket_type == SOCK_STREAM) {
        ret = sock_listen(hdl->sock_hdl, 0xFF);
        if (ret) {
            log_error("socket listen failed.");
            sock_unreg(hdl->sock_hdl);
            hdl->sock_hdl = NULL;
            return -1;
        }
    }

    return 0;
}

static int scr_prepare(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->state != PLUGIN_INITED) {
        log_debug("%s %s skipped. cur state: %s\r\n", plugin->name, __func__, plugin_state_str(hdl->state));
        return 0;
    }

    if (net_scr_sock_init(hdl)) {
        log_error("socket init err\n");
        free(hdl);
        return -1;
    }

    hdl->state = PLUGIN_READY;

    return 0;
}

static int get_jpg_packet(struct scr_handle *hdl, char *buf, int len, struct __JPG_INFO *jpg)
{
    static u32 total_payload_len = 0;
    static u8 finish = 0;
    u32 position = 0;
    u8 frame_type;
    u32 cur_frame_seq, frame_offset, slice_data_len, frame_size;
    u32 frame_head_size = sizeof(struct frm_head);

    if (len < frame_head_size) {
        log_error("recv len err\n");
        return -1;
    }

    do {
        struct frm_head  *head_info = (struct frm_head *)(buf + position);
        frame_type = head_info->type & 0x7F;
        cur_frame_seq = head_info->seq;
        frame_offset = head_info->offset;
        slice_data_len = head_info->payload_size;
        frame_size = head_info->frm_sz;
        //图片大小大于缓冲buf，不处理(可以根据对应芯片型号增大缓存)
        if (frame_size > JPG_BUF_MAX_SIZE) {
            log_error("frame size too large.\n");
            return -1;
        }
        len -= (frame_head_size + slice_data_len);
        if (len < 0) {
            log_error("remain len err.\n");
            return -1;
        }

        switch (frame_type) {
        case JPEG_TYPE_VIDEO:
            //如果当前seq大于旧的seq,认为是新的数据包,组包重新初始
            do {
                if (cur_frame_seq < hdl->old_frame_seq) {
                    //如果当前的seq小于旧的seq,说明是旧的数据包,跳过不处理
                    break;
                } else if (cur_frame_seq > hdl->old_frame_seq) {
                    if (total_payload_len && (finish == 0)) {
                    }
                    hdl->old_frame_seq = cur_frame_seq;
                    total_payload_len = 0;
                    finish = 0;
                    memset(jpg->buf, CHECK_CODE, JPG_BUF_MAX_SIZE);
                } else {
                    //同一帧数据但是分成很多包发送,不处理
                }

                //校验是不是接收到重复的包
                if (bytecmp((u8 *)jpg->buf + frame_offset, CHECK_CODE, CHECK_CODE_NUM) != 0) {
                    // log_debug("repeat the packet.\n");
                    break;
                }

                memcpy(jpg->buf + frame_offset, (buf + position) + frame_head_size, slice_data_len);
                total_payload_len += slice_data_len;
                if (total_payload_len == frame_size) { //如果数据量相等,说明帧数据接收完成
                    jpg->buf_len = total_payload_len;
                    hdl->seq = cur_frame_seq;
                    finish = 1;
                    //log_debug("jpg recv finish, jpg_size: %d", frame_size);
                    return 0; //返回0表示完成
                }
            } while (0);

            //position是剩余数据的偏移量
            position += (frame_head_size + slice_data_len);
            break;

        default:
            log_warn("default, %s - %d\n", __FUNCTION__, __LINE__);
            break;
        }
    } while (len > 0);

    return -1;
}

#if 0
static int save_jpg_data(struct __JPG_INFO *image)
{
    void *fd = NULL;
    fd = fopen(CONFIG_ROOT_PATH"image******.jpg", "w+");
    if (fd) {
        fwrite(image->buf, image->buf_len, 1, fd);
        fclose(fd);
        return 0;
    }
    return -1;
}
#endif

static void reset_scr_state(void)
{
    g_scr_state.fps_min = 0xFF;
    g_scr_state.fps_max = 0;
    g_scr_state.fps_sum = 0;

    g_scr_state.jpg_sz_min = 1e6f;
    g_scr_state.jpg_sz_max = 0.0f;
    g_scr_state.jpg_sz_sum = 0;

    g_scr_state.cnt = 0;
}

int get_in_ui_navi_flag();

static void statistic_socket_fps(void *priv)
{
    static char fps_buf[32], jpg_sz_buf[32];
    static u8 timer_cnt = 0;
    struct scr_handle *hdl = (struct scr_handle *)priv;
    ASSERT(hdl);

    u8 cur_fps = hdl->sock_fps;
    float cur_jpg_sz = (hdl->jpg_size + 1023) / 1024.0f;
    log_info("sock fps: %d, disp fps: %d\n", hdl->sock_fps, hdl->disp_fps);
    if (get_in_ui_navi_flag()) { 	//只有在导航界面才能更新导航UI数据
        if (cur_fps < g_scr_state.fps_min) {
            g_scr_state.fps_min = cur_fps;
        }
        if (cur_fps > g_scr_state.fps_max) {
            g_scr_state.fps_max = cur_fps;
        }

        if (cur_jpg_sz < g_scr_state.jpg_sz_min) {
            g_scr_state.jpg_sz_min = (float)cur_jpg_sz;
        }
        if (cur_jpg_sz > g_scr_state.jpg_sz_max) {
            g_scr_state.jpg_sz_max = (float)cur_jpg_sz;
        }

        g_scr_state.fps_sum = g_scr_state.fps_sum + cur_fps;
        g_scr_state.jpg_sz_sum = g_scr_state.jpg_sz_sum + cur_jpg_sz;
        g_scr_state.cnt = g_scr_state.cnt + 1;

        float fps_avg = (g_scr_state.cnt > 0) ? ((float)g_scr_state.fps_sum / g_scr_state.cnt) : 0.0f;
        float jpg_sz_avg = (g_scr_state.cnt > 0) ? ((float)g_scr_state.jpg_sz_sum / g_scr_state.cnt) : 0.0f;

        log_info("max fps: %d, min fps: %d, avg fps: %.2f\n", g_scr_state.fps_max, g_scr_state.fps_min, fps_avg);
        log_info("max jpg_size: %.2f, min jpg_size: %.2f, avg jpg_size: %.2f\n", g_scr_state.jpg_sz_max, g_scr_state.jpg_sz_min, jpg_sz_avg);

        snprintf(fps_buf, sizeof(fps_buf), "%d, %d, %.1f", g_scr_state.fps_max, g_scr_state.fps_min, fps_avg);
        snprintf(jpg_sz_buf, sizeof(jpg_sz_buf), "%.1f, %.1f, %.1f", g_scr_state.jpg_sz_max, g_scr_state.jpg_sz_min, jpg_sz_avg);

        update_text_lbl_7(fps_buf);
        update_text_lbl_9(jpg_sz_buf);
        if (timer_cnt >= 100) {
            log_info("the average fps over 100 samples is: %.2f", fps_avg);
            reset_scr_state();
            timer_cnt = 0;
        }
    }
    hdl->sock_fps = 0;
    hdl->disp_fps = 0;
    timer_cnt++;
}

static void scr_recv_task(void *priv)
{
    buffer_meta_t *buffer_meta = NULL;

    int finish_flag = -1;
    int recv_len = 0;

    pipe_plugin_t *plugin = (pipe_plugin_t *)priv;
    //目前只考虑单端点的情况
    buffer_t *buffer = port_get_output_endpoint(plugin->port, 0)->data_buffer;

    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;

    struct sockaddr_in cli_addr = {0};
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (hdl->socket_type == SOCK_STREAM) {
        struct frm_head head_info;
        u32 frm_sz;

        hdl->cli_sock_hdl = sock_accept(hdl->sock_hdl, &cli_addr, &addrlen, NULL, NULL);
        if (!hdl->cli_sock_hdl) {
            ASSERT(0);
        }
        if (cli_addr.sin_addr.s_addr != hdl->cli_addr.sin_addr.s_addr) {
            log_error("socket accept failed.");
            return ;
        }
    }

    //外部传进来的
    struct __JPG_INFO *jpg = zalloc(sizeof(struct __JPG_INFO) + JPG_BUF_MAX_SIZE);
    ASSERT(jpg);

    jpg->src_w = hdl->src_width;
    jpg->src_h = hdl->src_height;
    int timer_id = 0;
    timer_id = sys_timer_add_to_task("app_core", hdl, statistic_socket_fps, 1000);
    while (hdl->state == PLUGIN_RUNNING) {
        //tcp
        if (hdl->socket_type == SOCK_STREAM) {
            //接收头部信息
            struct frm_head head_info = {0};
            recv_len = sock_recv(hdl->cli_sock_hdl, &head_info, sizeof(struct frm_head), MSG_WAITALL);
            if (recv_len <= 0) {
                log_warn("recv len = %d", recv_len);
                break;
            }

            if (recv_len != sizeof(struct frm_head)) {
                log_error("recv len = %d, need head len = %d", recv_len, sizeof(struct frm_head));
                continue;
            }
            hdl->seq = head_info.seq;
            if (hdl->ack_cb) {  //net_scr_ack_func
                hdl->ack_cb(hdl->seq);
            }

            recv_len = sock_recv(hdl->cli_sock_hdl, jpg->buf, head_info.frm_sz, MSG_WAITALL);
            if (recv_len <= 0 || recv_len != head_info.frm_sz) {
                log_error("recv len = %d, need buf len = %d", recv_len, head_info.frm_sz);
                break;
            }
            jpg->buf_len = recv_len;
            finish_flag = 0;
        } else {
            recv_len = sock_recvfrom(hdl->sock_hdl, recv_buf, sizeof(recv_buf), 0, &cli_addr, &addrlen);
            if (recv_len < 0) {
                log_warn("recv len = %d", recv_len);
                break;
            }

            if (cli_addr.sin_addr.s_addr != hdl->cli_addr.sin_addr.s_addr) {
                log_error("recv from data source err, cli addr: %x %x.", cli_addr.sin_addr.s_addr, hdl->cli_addr.sin_addr.s_addr);
                continue;
            }
            finish_flag = get_jpg_packet(hdl, recv_buf, recv_len, jpg);
        }

        if (!finish_flag) {
            hdl->jpg_size = jpg->buf_len;

            buffer_meta = buffer->get_write_addr(buffer, jpg->buf_len);
            if (!buffer_meta) {
                log_warn("%s: next moudle buffer no enough. free_len: %d, need_len: %d", __func__, buffer->get_free_size(buffer), jpg->buf_len);
                do {
                    buffer_meta = buffer->read_data(buffer, BUFFER_NO_WAIT);
                    buffer->free_read_data(buffer, buffer_meta);
                } while (buffer->get_free_size(buffer) < jpg->buf_len);

                buffer_meta = buffer->get_write_addr(buffer, jpg->buf_len);
                ASSERT(buffer_meta);
            }
            if (buffer_meta) {
                //根据规则发送回包
                if (hdl->ack_cb && hdl->socket_type != SOCK_STREAM) {  //net_scr_ack_func
                    hdl->ack_cb(hdl->seq);
                }
                if (buffer->type == EXTERN_BUFFER) {
                    memcpy(buffer_meta->ext_data, jpg->buf, jpg->buf_len);
                } else {
                    memcpy(buffer_meta->data, jpg->buf, jpg->buf_len);
                }
                buffer_meta->data_len = jpg->buf_len;
                buffer->update_data(buffer, buffer_meta);
                hdl->disp_fps++;
            }
            buffer_meta = NULL;
            finish_flag = -1;
            hdl->sock_fps++;
        }
    }

    sys_timer_del(timer_id);
    free(jpg);
}

static int scr_start(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->state == PLUGIN_UNINIT || hdl->state == PLUGIN_INITED) {
        log_debug("%s %s skipped. cur state: %s\r\n", plugin->name, __func__, plugin_state_str(hdl->state));
        return 0;
    }

    hdl->ref++;

    if (hdl->state == PLUGIN_RUNNING) {
        log_debug("%s start. ref: %d\r\n", plugin->name, hdl->ref);
    } else {
        hdl->state = PLUGIN_RUNNING;
        char task_name[32] = {0};
        sprintf(task_name, SCR_THREAD_TASK_NAME"%d", hdl->channel);
        if (!hdl->pid) {
            thread_fork(task_name, 22, 1024, 32, &hdl->pid, scr_recv_task, plugin);
        }
    }

    return 0;
}

static int scr_stop(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->state !=  PLUGIN_RUNNING) {
        log_debug("%s %s skipped. cur state: %s\r\n", plugin->name, __func__, plugin_state_str(hdl->state));
        return 0;
    }

    ASSERT(hdl->ref);
    hdl->ref--;
    if (hdl->ref > 0) {
        return 0;
    }
    reset_scr_state();
    hdl->state = PLUGIN_PAUSED;

    return 0;
}


static int net_scr_sock_uninit(struct scr_handle *hdl)
{
    if (hdl->socket_type == SOCK_STREAM) {
        sock_unreg(hdl->cli_sock_hdl);
        hdl->cli_sock_hdl = NULL;
    }
    sock_unreg(hdl->sock_hdl);
    hdl->sock_hdl = NULL;

    return 0;
}

static int scr_reset(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->ref > 0) {
        return 0;
    }

    net_scr_sock_uninit(hdl);
    reset_scr_state();
    hdl->state = PLUGIN_UNINIT;

    return 0;
}

static int scr_get_parameter(pipe_plugin_t *plugin, int cmd, void *arg, int source_channel)
{
    ASSERT(plugin);
    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;
    if (!hdl) {
        return -1;
    }
    int ret = 0;

    switch (cmd) {
    default:
        break;

    }
    return ret;
}

static int scr_set_parameter(pipe_plugin_t *plugin, int cmd, void *arg, int source_channel)
{
    int ret = 0;
    ASSERT(plugin);
    struct scr_handle *hdl = (struct scr_handle *)plugin->private_data;
    if (!hdl) {
        return -1;
    }

    switch (cmd) {
    case PIPELINE_SET_FORMAT:
        struct video_format *f = (struct video_format *)arg;
        hdl->src_width = f->src_width;
        hdl->src_height = f->src_height;
        hdl->fps     = f->fps;
        break;
    case PIPELINE_SCR_CLI_ADR:
        memcpy(&hdl->cli_addr, (struct sockaddr_in *)arg, sizeof(struct sockaddr_in));
        break;
    case PIPELINE_SCR_SOCK_TYPE:
        hdl->socket_type = *(int *)arg;
        break;
    case PIPELINE_SCR_ACK_CALLBACK:
        hdl->ack_cb = *(int *)arg;
        break;
    default:
        break;

    }

    return ret;

}

static int scr_message_callback(pipe_plugin_t *plugin, int cmd, void *arg)
{
    int ret = PIPE_MESSAGE_OK;
    struct scr_handle *hdl = plugin->private_data;
    int type = cmd & ~MESSAGE_ALL;

    switch (type) {
    case GET_DATA_INFO:
        pipe_common_t *info = (pipe_common_t *)arg;
        if (info->channel == (hdl->channel + PLUGIN_SOURCE_SCR_BASE)) {
            info->width = hdl->src_width;
            info->height = hdl->src_height;
            info->fps = hdl->fps;
            return PIPE_MESSAGE_OK;
        }
        ret = PIPE_MESSAGE_CONTINUE;
        break;
    default:
        ret = PIPE_MESSAGE_NONE;
        break;
    }

    return ret;
}

#define SERVER_TCP_PORT 8888
static int tcp_client_init(const char *server_ip, const int server_port)
{
    void *sock = NULL;
    struct sockaddr_in dest;

    //创建socket
    sock = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (sock == NULL) {
        log_error("sock_reg fail.\n");
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(server_ip);
    dest.sin_port = htons(server_port);
    if (0 != sock_connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in))) {
        log_error("sock_connect fail.\n");
        sock_unreg(sock);
        return -1;
    }

    log_info("tcp_client_connect_task succ!");
    if (sock) {
        sock_unreg(sock);
        sock = NULL;
    }

    return 0;
}

static void tcp_client_connect_task(void *priv)
{
    log_info("---------------------------------------------->");
    char gateway[16] = {0};
    get_gateway(1, gateway);

    log_info("tcp connect : server ip[%s], port[%d]\n", gateway, SERVER_TCP_PORT);

    tcp_client_init(gateway, SERVER_TCP_PORT);
    log_info("<----------------------------------------------");
}

void connect_to_server_port_8888(void)
{
    if (thread_fork("tcp_client_connect_task", 10, 512, 0, NULL, tcp_client_connect_task, NULL) != OS_NO_ERR) {
        log_error("thread fork fail\n");
    }
}


REGISTER_PLUGIN(scr0) = {
    .name           = "scr0",
    .type           = PLUGIN_SOURCE,
    .init           = scr_init,
    .connect        = scr_connect,
    .prepare        = scr_prepare,
    .start          = scr_start,
    .stop           = scr_stop,
    .reset          = scr_reset,
    .get_parameter  = scr_get_parameter,
    .set_parameter  = scr_set_parameter,
    .msg_cb         = scr_message_callback,
};


#endif
