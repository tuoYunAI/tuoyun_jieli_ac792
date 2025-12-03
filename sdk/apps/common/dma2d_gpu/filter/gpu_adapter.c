/**********************************************************************************
* File:             gpu_adapter.c
*
* Author:
* Created:          10/12/24
* Description:      gpu插件实现
*                   作为imc后处理插件使用,主要用于图像垂直镜像显示
********************************************************************************/

#include "app_config.h"
#include "video/video.h"
#include "video/videobuf.h"
#include "pipeline_plugin.h"
#include "gpu_adapter.h"
#include "pipeline_core.h"
#include "pipeline_hash.h"
#include "message_event.h"
#include "spinlock.h"
#include "asm/gpu/jlvg_hw.h"
#include "asm/dma2d_driver.h"
#include "fs/fs.h"

#define LOG_TAG_CONST       GPU_ADAPTER
#define LOG_TAG             "[GPU_ADAPTER]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"

#define GPU_MAX_CHANNEL  4
//1:用cpu软件处理图像垂直镜像,用到了硬件dma copy,耗费一行buffer的内存
//0:用gpu硬件处理图像垂直镜像,耗费一帧buffer的内存,速度比软件快30%
#define SOFT_VER_MIRROR_EN 1

static int *g_gpu_used[GPU_MAX_CHANNEL];

struct gpu_handle {
    u8 gpu_task_busy; //don't touch

    u8 channel;
    u8 ref;
    u8 state;
    /* spinlock_t lock; */
    u32  pid;
    u8  task_kill;
    u8 mirror;
    u8 enc_rotate;

    int input_width;
    int input_height;
    u8  input_format;

    int output_width;
    int output_height;
    u8  output_format;

    OS_SEM task_sem;
    buffer_meta_t *buffer_meta;

    u8 *tmp_line;
    u8 *gpu_out_buf;
};


static void __sw_vertical_mirror(struct gpu_handle *hdl, uint8_t *src, int src_w, int src_h, u8 format)
{
    uint16_t *color_p = (uint16_t *)src;

#if 0
    uint16_t tmp;
    DcuInvalidRegion((void *)src, src_w * src_h * 2);
    //垂直镜像
    for (int h = 0; h < src_h / 2; h++) {
        uint16_t *row_start = color_p + (src_w * h);
        uint16_t *row_end = color_p + (src_w * (src_h - 1 - h));
        // 交换整行
        for (int w = 0; w < src_w; w++) {
            tmp = row_start[w];
            row_start[w] = row_end[w];
            row_end[w] = tmp;
        }
    }
    DcuFlushRegion((void *)src, src_w * src_h * 2);
#endif

    //垂直镜像
    int line_bytes = src_w * 2;
    uint16_t *row_start = color_p;
    uint16_t *row_end = color_p + (src_w * (src_h - 1));
    while (row_start < row_end) {
        // 交换整行
        dma_copy_sync(NO_CACHE_ADDR(hdl->tmp_line), NO_CACHE_ADDR(row_start), line_bytes);
        dma_copy_sync(NO_CACHE_ADDR(row_start), NO_CACHE_ADDR(row_end), line_bytes);
        dma_copy_sync(NO_CACHE_ADDR(row_end), NO_CACHE_ADDR(hdl->tmp_line), line_bytes);
        row_start += src_w;
        row_end -= src_w;
    }

}

static void gpu_task(void *arg)
{
    pipe_plugin_t *plugin = (pipe_plugin_t *)arg;
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;

    buffer_t *buffer_in = port_get_input_endpoint(plugin->port, 0)->data_buffer;
    buffer_t *buffer_out = port_get_output_endpoint(plugin->port, 0)->data_buffer;
    buffer_meta_t *buffer_meta_in = NULL;
    buffer_meta_t *buffer_meta_out = NULL;

    if (buffer_check_vaild(buffer_in) == false && buffer_check_vaild(buffer_out) == false) {
        ASSERT(0, "gpu buffer in and out memory isn't ok. Please malloc firstly.\r\n");
        return ;
    }


    u32 fcnt = 0;
    while (1) {

        if (hdl->task_kill) {
            log_debug("gpu task exit.\r\n");
            break;
        }

        buffer_meta_in = buffer_in->read_data(buffer_in, 0);
        if (!buffer_meta_in) {
            log_warn("gpu buffer in memory read_data null!\n");
            continue;
        }

        //GPU处理
        u8 *input_data = buffer_meta_in->ext_data;
        /* u32 _j = get_system_us(); */
        /* u32 end; */
        if (hdl->mirror) {
#if SOFT_VER_MIRROR_EN
            hdl->gpu_task_busy = 0x55; //set busy

            //cpu软件垂直镜像处理
            __sw_vertical_mirror(hdl, input_data, hdl->input_width, hdl->input_height, hdl->input_format);


#else
            //gpu硬件垂直镜像处理
            int fb_frame_buf_mirror(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int dst_width, int dst_height, int mirror, int src_format, int dst_format);
            fb_frame_buf_mirror(input_data, hdl->gpu_out_buf, hdl->input_width, hdl->input_height, hdl->output_width, hdl->output_height, 2, hdl->input_format, hdl->input_format);
            hdl->buffer_meta->ext_data = hdl->gpu_out_buf;
#endif
            /* end = get_system_us() - _j; */
            /* printf("mirror use:%dus\n", end); */
        }
        if (hdl->enc_rotate) {
            //putchar("G");
#if TCFG_LCD_ENABLE
            int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int src_stride,
                                    int dst_width, int dst_height, int dst_stride, int degree, int xoffset, int yoffset,
                                    int in_format, int out_format, uint8_t mirror);
            fb_frame_buf_rotate(input_data, hdl->gpu_out_buf, hdl->input_width, hdl->input_height, 0, hdl->output_width, hdl->output_height, 0,
                                hdl->enc_rotate, 0, 0, hdl->input_format, hdl->input_format, 0);

#endif
            hdl->buffer_meta->ext_data = hdl->gpu_out_buf;

        }

        buffer_out->update_data(buffer_out, hdl->buffer_meta);
        hdl->gpu_task_busy = 0;

    }

}

static buffer_meta_t *gpu_get_write_able_addr(buffer_t *buffer, size_t size)
{
    int type = buffer->type;
    pipe_plugin_t *plugin = buffer->private_data;

    if (type == EXTERN_BUFFER) {
        buffer_t *buffer_out = port_get_output_endpoint(plugin->port, 0)->data_buffer;
        buffer_meta_t *buffer_meta = buffer_out->get_write_addr(buffer_out, size);

        return buffer_meta;
    } else {
        log_error("%s Not implemented", __FUNCTION__);
        return NULL;
    }
    return NULL;
}

static int gpu_update_data(buffer_t *buffer, buffer_meta_t *buffer_meta)
{
    int type = buffer->type;
    pipe_plugin_t *plugin = buffer->private_data;
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;

    if (type == EXTERN_BUFFER) {
        hdl->buffer_meta = buffer_meta;
        message_request(plugin->port, DISP_FB_PUT_BUF | MESSAGE_NEXT, buffer_meta->ext_data);
        os_sem_set(&hdl->task_sem, 0);
        os_sem_post(&hdl->task_sem);
        return 0;
    } else {
        log_error("%s Not implemented", __FUNCTION__);
        return -1;
    }
    return -1;
}

static int gpu_free_data_size(buffer_t *buffer)
{
    int type = buffer->type;
    if (type == EXTERN_BUFFER) {
    } else {
        log_error("%s Not implemented", __FUNCTION__);
        return -1;
    }
    return -1;
}

static buffer_meta_t *gpu_get_read_data(buffer_t *buffer, int no_wait)
{
    int type = buffer->type;
    pipe_plugin_t *plugin = buffer->private_data;
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;

    if (type == EXTERN_BUFFER) {
        int err = os_sem_pend(&hdl->task_sem, 50);
        if (err != OS_ERR_NONE) {
            return NULL;
        } else {
            return hdl->buffer_meta;
        }
    } else {
        log_error("%s Not implemented", __FUNCTION__);
        return NULL;
    }
    return NULL;
}

static int gpu_free_read_data(buffer_t *buffer, buffer_meta_t *buffer_meta)
{
    int type = buffer->type;
    pipe_plugin_t *plugin = buffer->private_data;

    if (type == EXTERN_BUFFER) {
        buffer_t *buffer_out = port_get_output_endpoint(plugin->port, 0)->data_buffer;
        buffer_out->free_read_data(buffer_out, buffer_meta);
    } else {
        log_error("%s Not implemented", __FUNCTION__);
        return -1;
    }
    return -1;
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

static int gpu_start(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    ASSERT(hdl);
    char task_name[20];

    if (hdl->state == PLUGIN_RUNNING) {
        hdl->ref++;
        log_debug("%s start. ref: %d\r\n", plugin->name, hdl->ref);
        return 0;
    } else if (hdl->state != PLUGIN_READY && hdl->state != PLUGIN_PAUSED) {
        log_debug("%s start skipped. cur state: %d\r\n", plugin->name, hdl->state);
        return 0;
    }

    sprintf(task_name, "gpu%d_task", hdl->channel);
    thread_fork(task_name, 20, 2048, 0, &hdl->pid, gpu_task, plugin);
#if SOFT_VER_MIRROR_EN
    if (hdl->tmp_line == NULL) {
        hdl->tmp_line = malloc(hdl->input_width * 1 * fb_get_format_bpp(hdl->input_format) / 8);
    }
#else
    if (hdl->gpu_out_buf == NULL) {
        hdl->gpu_out_buf = malloc(hdl->output_width * hdl->output_height * fb_get_format_bpp(hdl->output_format) / 8);
    }
#endif
    if (hdl->enc_rotate) {
        if (hdl->gpu_out_buf == NULL) {
            hdl->gpu_out_buf = malloc(hdl->output_width * hdl->output_height * fb_get_format_bpp(hdl->output_format) / 8);
        }
    }

    hdl->ref++;
    hdl->state = PLUGIN_RUNNING;

    return 0;
}



static int gpu_prepare(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    ASSERT(hdl);
    pipe_msg_group_t msg_group = {0};
    pipe_common_t *common = NULL, *n = NULL;

    if (hdl->state != PLUGIN_INITED) {
        log_debug("%s prepare skipped. cur state: %d\r\n", plugin->name, hdl->state);
        return 0;
    }

    os_sem_create(&hdl->task_sem, 0);

    //get prev input info
    INIT_LIST_HEAD(&msg_group.list);
    message_request(plugin->port, GET_DATA_INFO | MESSAGE_PREV, &msg_group);
    list_for_each_entry_safe(common, n, &msg_group.list, entry) {
        hdl->input_width = common->width;
        hdl->input_height = common->height;
        hdl->input_format = common->format;
        free(common);
    }
    log_info("prepare input: %dx%d %d\n", hdl->input_width, hdl->input_height, hdl->input_format);

    //get next output info
    INIT_LIST_HEAD(&msg_group.list);
    message_request(plugin->port, GET_DATA_INFO | MESSAGE_NEXT, &msg_group);
    list_for_each_entry_safe(common, n, &msg_group.list, entry) {
        hdl->output_width = common->width;
        hdl->output_height = common->height;

        //todo 需要转换下
        hdl->output_format = common->format;
        free(common);
    }
    log_info("prepare output: %dx%d %d\n", hdl->output_width, hdl->output_height, hdl->output_format);

    hdl->state = PLUGIN_READY;

    return 0;
}


static int gpu_connect(pipe_plugin_t *prev_plugin, pipe_plugin_t *plugin, int source_channel)
{
#if 0
    ASSERT(prev_plugin);
    ASSERT(plugin);

    if (port_check_connected(prev_plugin, plugin)) {
        port_source_channel_inc(prev_plugin, plugin, source_channel);
        return 0;
    }

    u32 buffer_len;

    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    struct buffer_api api = {0};

    api.private_data    = plugin;
    api.get_write_addr  = gpu_get_write_able_addr;
    api.update_data     = gpu_update_data;
    api.get_free_size   = gpu_free_data_size;
    api.read_data       = gpu_get_read_data;
    api.free_read_data  = gpu_free_read_data;

    buffer_t *buffer = buffer_init(plugin->name, EXTERN_BUFFER, &api);

    port_add_output_endpoint(prev_plugin->port, plugin->name, source_channel);
    port_add_input_endpoint(plugin->port, prev_plugin->name, source_channel);

    for (int i = 0 ; i < port_output_endpoint_num(prev_plugin->port); i++) {
        // log_info("out direction %s -> %s", prev_plugin->name, port_get_output_endpoint_name(prev_plugin->port, i));
        pipe_endpoint_t *out_endpoint = port_get_output_endpoint(prev_plugin->port, i);
        out_endpoint->data_buffer = buffer;
    }

    for (int i = 0 ; i < port_input_endpoint_num(plugin->port); i++) {
        // log_info("in direction %s <- %s", plugin->name, port_get_input_endpoint_name(plugin->port, i));
        pipe_endpoint_t *in_endpoint = port_get_input_endpoint(plugin->port, i);
        in_endpoint->data_buffer = buffer;
    }
#else
    ASSERT(0);
#endif

    return 0;
}



static int gpu_init(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    int channel = check_channel_legal(plugin, GPU_MAX_CHANNEL);
    struct gpu_handle *hdl = NULL;
    if (!g_gpu_used[channel]) {
        hdl = (struct gpu_handle *)zalloc(sizeof(struct gpu_handle));
    } else {
        hdl = g_gpu_used[channel];
    }
    ASSERT(hdl);

    if (hdl->state != PLUGIN_UNINIT) {
        log_debug("%s init skipped. cur state: %d\r\n", plugin->name, hdl->state);
        ASSERT(plugin->private_data, "cur plugin used by other pipeline.");
        return 0;
    }

    /* spin_lock_init(&hdl->lock); */

    hdl->channel = channel;

    log_info("gpu_init");

    hdl->state = PLUGIN_INITED;
    plugin->private_data = hdl;

    g_gpu_used[hdl->channel] = hdl;

    return 0;
}

static int gpu_stop(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->state != PLUGIN_RUNNING) {
        log_debug("%s isn't ready for stop. state: %d\r\n", plugin->name, hdl->state);
        return 0;
    }

    ASSERT(hdl->ref);
    hdl->ref--;
    if (hdl->ref > 0) {
        return 0;
    }

    hdl->task_kill = true;
    thread_kill(&hdl->pid, KILL_WAIT);
    hdl->task_kill = false;

    for (int i = 0 ; i < port_input_endpoint_num(plugin->port); i++) {
        pipe_endpoint_t *in_endpoint = port_get_input_endpoint(plugin->port, i);
        if (in_endpoint) {
            buffer_early_uninit(in_endpoint->data_buffer);
        }

    }

    hdl->state = PLUGIN_PAUSED;

    return 0;
}

static int gpu_reset(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->ref > 0) {
        return 0;
    }

    hdl->state = PLUGIN_UNINIT;

    for (int i = 0 ; i < port_input_endpoint_num(plugin->port); i++) {
        pipe_endpoint_t *in_endpoint = port_get_input_endpoint(plugin->port, i);
        if (in_endpoint) {
            buffer_uninit(in_endpoint->data_buffer, source_channel);
        }

    }

    g_gpu_used[hdl->channel] = NULL;
    plugin->private_data = NULL;

#if SOFT_VER_MIRROR_EN
    if (hdl->tmp_line) {
        free(hdl->tmp_line);
        hdl->tmp_line = NULL;
    }
#else
    if (hdl->gpu_out_buf) {
        free(hdl->gpu_out_buf);
        hdl->gpu_out_buf = NULL;
    }
#endif

    os_sem_del(&hdl->task_sem, OS_DEL_ALWAYS);

    free(hdl);

    return 0;
}


static int gpu_get_parameter(pipe_plugin_t *plugin, int cmd, void *arg)
{
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    if (!hdl) {
        return -1;
    }

    switch (cmd) {
    default:
        break;
    }
    return 0;
}

static int gpu_set_parameter(pipe_plugin_t *plugin, int cmd, void *arg)
{
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    if (!hdl) {
        return -1;
    }

    switch (cmd) {
    case PIPELINE_SET_FORMAT:
        struct video_format *f = (struct video_format *)arg;
        /* hdl->mirror = f->win.mirror; */
        hdl->enc_rotate = f->enc_rotate;
        break;
    default:
        break;
    }
    return 0;
}

static int gpu_message_callback(pipe_plugin_t *plugin, int cmd, void *arg)
{
    struct gpu_handle *hdl = (struct gpu_handle *)plugin->private_data;
    int ret = PIPE_MESSAGE_OK;
    int type = cmd & ~MESSAGE_ALL;

    switch (type) {
    default:
        ret = message_request(plugin->port, cmd, arg);
        break;
    }

    return ret;

}


REGISTER_PLUGIN(gpu0) = {
    .name           = "gpu0",
    .type           = PLUGIN_FILTER,
    .init           = gpu_init,
    .connect        = gpu_connect,
    .prepare        = gpu_prepare,
    .start          = gpu_start,
    .stop           = gpu_stop,
    .reset          = gpu_reset,
    .get_parameter  = gpu_get_parameter,
    .set_parameter  = gpu_set_parameter,
    .msg_cb         = gpu_message_callback,
};
REGISTER_PLUGIN(gpu1) = {
    .name           = "gpu1",
    .type           = PLUGIN_FILTER,
    .init           = gpu_init,
    .connect        = gpu_connect,
    .prepare        = gpu_prepare,
    .start          = gpu_start,
    .stop           = gpu_stop,
    .reset          = gpu_reset,
    .get_parameter  = gpu_get_parameter,
    .set_parameter  = gpu_set_parameter,
    .msg_cb         = gpu_message_callback,
};
REGISTER_PLUGIN(gpu2) = {
    .name           = "gpu2",
    .type           = PLUGIN_FILTER,
    .init           = gpu_init,
    .connect        = gpu_connect,
    .prepare        = gpu_prepare,
    .start          = gpu_start,
    .stop           = gpu_stop,
    .reset          = gpu_reset,
    .get_parameter  = gpu_get_parameter,
    .set_parameter  = gpu_set_parameter,
    .msg_cb         = gpu_message_callback,
};
REGISTER_PLUGIN(gpu3) = {
    .name           = "gpu3",
    .type           = PLUGIN_FILTER,
    .init           = gpu_init,
    .connect        = gpu_connect,
    .prepare        = gpu_prepare,
    .start          = gpu_start,
    .stop           = gpu_stop,
    .reset          = gpu_reset,
    .get_parameter  = gpu_get_parameter,
    .set_parameter  = gpu_set_parameter,
    .msg_cb         = gpu_message_callback,
};

