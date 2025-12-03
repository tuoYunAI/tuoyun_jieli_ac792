/**********************************************************************************
* File:             dma2d_adapter.c
*
* Author:
* Created:
* Description:      dma2d插件实现
*                   作为imc后处理插件使用,进行图像合成
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

#define LOG_TAG_CONST       DMA2D_ADAPTER
#define LOG_TAG             "[DMA2D_ADAPTER]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"

#define DMA2D_MAX_CHANNEL  2

static int *g_dma2d_used[DMA2D_MAX_CHANNEL];

struct dma2d_filter_handle {
    u8 dma2d_task_busy; //don't touch

    u8 channel;
    u8 ref;
    u8 state;
    u32  pid;
    u8  task_kill;

    u8 source_channel;
    int input_width;
    int input_height;
    u8  input_format;

    int output_width;
    int output_height;
    u8  output_format;
    u16 line_cnt;

    OS_SEM task_sem;
    buffer_meta_t *buffer_meta;

    u8 *dma2d_out_buf;

    void *sticker;
};


#ifdef USE_LVGL_V8_UI_DEMO
#include "lvgl_v8/src/draw/lv_img_buf.h"
#include "lvgl_v8/src/extra/jl_extra/libs/rle/lv_rle.h"
static int __get_sticker(const char *src, lv_img_dsc_t *bin_dsc)
{
    FILE *fp = NULL;
    struct vfs_attr file_attr = {0};
    u32 file_addr = 0;
    if (src[0] == 'm') {
        fp = fopen(src, "r");
        if (fp == NULL) {
            printf("%s not exist\n", src);
            return -1;
        }
        fget_attrs(fp, &file_attr);
        fclose(fp);
        file_addr = file_attr.sclust;

        memcpy(bin_dsc, file_addr + 20, sizeof(lv_img_dsc_t));
        bin_dsc->data = (file_addr + (bin_dsc->data));
    }

    return 0;
}

extern uint16_t lv_rle_get_stride(rle_info_t *rle_info, uint8_t *adr_mode);
static int dma2d_filter_image_combine(u16 x, u16 y, u8 *sticker_data, u16 sticker_w, u16 sticker_h,
                                      u8 *bg_data, u16 bg_w, u16 bg_h)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};
    rle_info_t *rle_info = (rle_info_t *)sticker_data;
    log_info("sticker: (%d,%d) %d x %d", x, y, sticker_w, sticker_h);
    log_info("bg:%d x %d", bg_w, bg_h);
    log_info("rle_info:");
    log_info("width:%d", rle_info->width);
    log_info("height:%d", rle_info->height);
    log_info("format:%d", rle_info->format);
    log_info("lut_format:%d", rle_info->lut_format);
    log_info("rle_len:%d", rle_info->len);
    if (x + sticker_w > bg_w) {
        printf("\n%s line:%d combine sticker over width!!!\n", __func__, __LINE__);
    }
    if (y + sticker_h > bg_h) {
        printf("\n%s line:%d combine sticker over height!!!\n", __func__, __LINE__);
    }
    fg_layer_param.data = (uint8_t *)sticker_data + rle_info->addr;
    fg_layer_param.rle_en = 1;
    fg_layer_param.clut = sticker_data + rle_info->lut_addr;
    fg_layer_param.clut_size = rle_info->lut_len;
    fg_layer_param.clut_format = rle_info->lut_format;
    fg_layer_param.image_size = rle_info->len;
    fg_layer_param.stride = lv_rle_get_stride(rle_info, NULL);
    fg_layer_param.format = rle_info->format;

    bg_layer_param.x = x;
    bg_layer_param.y = y;
    bg_layer_param.data = bg_data;
    bg_layer_param.stride = bg_w * 2;
    bg_layer_param.format = JLDMA2D_FORMAT_YUV422_BT601;
    bg_layer_param.rbs = 1;

    out_layer_param.data = bg_data + (y * bg_w + x) * 2;
    out_layer_param.w = (sticker_w < bg_w) ? sticker_w : bg_w;
    out_layer_param.h = (sticker_h < bg_h) ? sticker_h : bg_h;
    out_layer_param.stride = bg_w * 2;
    out_layer_param.format = JLDMA2D_FORMAT_YUV422_BT601;
    out_layer_param.rbs = 1;
    out_layer_param.dither_en = 1;

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_run();
    dma2d_wait_done();

    return 0;
}
#endif


static int dma2d_fliter_buffer_dump(u8 *buffer, int buffer_size)
{
    //dump out
    FILE *f = fopen(CONFIG_ROOT_PATH"dma2d_out.dat", "w+");
    if (f) {
        printf("dump dma2d fliter buffer out\n");
        fwrite(buffer, buffer_size, 1, f);
        fclose(f);
    }
    return 0;
}

static void dma2d_filter_task(void *arg)
{
    pipe_plugin_t *plugin = (pipe_plugin_t *)arg;
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;

    buffer_t *buffer_in = port_get_input_endpoint(plugin->port, 0)->data_buffer;
    buffer_t *buffer_out = port_get_output_endpoint(plugin->port, 0)->data_buffer;
    buffer_meta_t *buffer_meta_in = NULL;
    buffer_meta_t *buffer_meta_out = NULL;

    if (buffer_check_vaild(buffer_in) == false && buffer_check_vaild(buffer_out) == false) {
        ASSERT(0, "dma2d_filter buffer in and out memory isn't ok. Please malloc firstly.\r\n");
        return ;
    }
    struct image_sticker *sticker = NULL;

    while (1) {

        int err = os_sem_pend(&hdl->task_sem, 0);

        if (hdl->task_kill) {
            log_debug("dma2d_filter task exit.\r\n");
            break;
        }
        //dma2d处理
        u8 *image_data = buffer_get_memory_address(buffer_in);
        sticker = hdl->sticker;
        log_info("dma2d filter data: %s  %x\n", (char *)sticker->addr, (unsigned int)image_data);

#ifdef USE_LVGL_V8_UI_DEMO
        lv_img_dsc_t bin_dsc = {0};
        __get_sticker(sticker->addr, &bin_dsc);
        dma2d_filter_image_combine(sticker->x, sticker->y, bin_dsc.data, bin_dsc.header.w, bin_dsc.header.h,
                                   image_data, hdl->output_width, hdl->output_height);
        /* dma2d_fliter_buffer_dump(image_data,hdl->output_width*hdl->output_height * 2); */
#else
        log_error("dma2d fliter combine should open lvgl macro\n");
#endif
        if (buffer_out->type == FAST_BUFFER || buffer_out->type == GENERAL_BUFFER) {
            int out_height = hdl->line_cnt ? hdl->line_cnt : hdl->output_height;
            message_request(plugin->port, REP_COMPOSITE_BUF_NOTIFY | MESSAGE_NEXT, (void *)image_data);
        }
        hdl->dma2d_task_busy = 0;

    }

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

static int dma2d_filter_start(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
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

    sprintf(task_name, "dma2d_filter%d_task", hdl->channel);
    thread_fork(task_name, 20, 2048, 0, &hdl->pid, dma2d_filter_task, plugin);

    hdl->ref++;
    hdl->state = PLUGIN_RUNNING;

    return 0;
}



static int dma2d_filter_prepare(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
    ASSERT(hdl);

    if (hdl->state != PLUGIN_INITED) {
        log_debug("%s prepare skipped. cur state: %d\r\n", plugin->name, hdl->state);
        return 0;
    }

    os_sem_create(&hdl->task_sem, 0);

    hdl->state = PLUGIN_READY;

    return 0;
}


static int dma2d_filter_connect(pipe_plugin_t *prev_plugin, pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(prev_plugin);
    ASSERT(plugin);

    if (port_check_connected(prev_plugin, plugin)) {
        port_source_channel_inc(prev_plugin, plugin, source_channel);
        return 0;
    }

    log_info("plugin link: %s -> %s", prev_plugin->name, plugin->name);

    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
    struct buffer_api api = {0};

    hdl->source_channel = source_channel;
    buffer_info_t info = {0};
    info.source_channel = hdl->source_channel;
    buffer_t *buffer = buffer_init(plugin->name, &info);

    pipe_endpoint_t *out_endpoint = port_add_output_endpoint(prev_plugin->port, plugin->name, source_channel);
    pipe_endpoint_t *in_endpoint = port_add_input_endpoint(plugin->port, prev_plugin->name, source_channel);

    out_endpoint->data_buffer = buffer;

    in_endpoint->data_buffer = buffer;

    //set source info
    pipe_common_t fmt_info = {0};
    fmt_info.channel = source_channel;
    message_request(plugin->port, GET_DATA_INFO | MESSAGE_PREV, &fmt_info);
    hdl->input_width = fmt_info.width;
    hdl->input_height = fmt_info.height;
    /* hdl->input_format = fmt_info.format; */
    hdl->output_width = fmt_info.width;
    hdl->output_height = fmt_info.height;
    log_info("prepare input: %dx%d %d\n", hdl->input_width, hdl->input_height, hdl->input_format);

    info.buffer_type = GENERAL_BUFFER;
    info.memory_type = DDR_MEMORY;
    int block_size = hdl->input_width * hdl->input_height * ((hdl->input_format == FORMAT_YUV420P) ? (3.0 / 2) : 2);
    info.buffer_size = block_size;//(block_size + sizeof(buffer_meta_t) + LBUF_RESERVE_LEN);
    int ret = buffer_alloc_memory(buffer, &info);
    if (ret == 0) {
        log_debug("dma2d buffer %x len :%d ", buffer->mem.mem_addr, info.buffer_size);
    }


    return 0;
}



static int dma2d_filter_init(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    int channel = check_channel_legal(plugin, DMA2D_MAX_CHANNEL);
    struct dma2d_filter_handle *hdl = NULL;
    if (!g_dma2d_used[channel]) {
        hdl = (struct dma2d_filter_handle *)zalloc(sizeof(struct dma2d_filter_handle));
    } else {
        hdl = g_dma2d_used[channel];
    }
    ASSERT(hdl);

    if (hdl->state != PLUGIN_UNINIT) {
        log_debug("%s init skipped. cur state: %d\r\n", plugin->name, hdl->state);
        ASSERT(plugin->private_data, "cur plugin used by other pipeline.");
        return 0;
    }

    hdl->channel = channel;

    log_info("dma2d_filter_init");

    hdl->state = PLUGIN_INITED;
    plugin->private_data = hdl;

    g_dma2d_used[hdl->channel] = hdl;

    return 0;
}

static int dma2d_filter_stop(pipe_plugin_t *plugin)
{
    ASSERT(plugin);
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
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
    os_sem_post(&hdl->task_sem);
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

static int dma2d_filter_reset(pipe_plugin_t *plugin, int source_channel)
{
    ASSERT(plugin);
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
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

    g_dma2d_used[hdl->channel] = NULL;
    plugin->private_data = NULL;

    os_sem_del(&hdl->task_sem, OS_DEL_ALWAYS);

    free(hdl);

    return 0;
}


static int dma2d_filter_get_parameter(pipe_plugin_t *plugin, int cmd, void *arg)
{
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
    if (!hdl) {
        return -1;
    }

    switch (cmd) {
    default:
        break;
    }
    return 0;
}

static int dma2d_filter_set_parameter(pipe_plugin_t *plugin, int cmd, void *arg)
{
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
    if (!hdl) {
        return -1;
    }

    switch (cmd) {
    case PIPELINE_SET_BUFFER_LINE:
        /* hdl->line_cnt = *(int *)arg; */
        break;
    case PIPELINE_SET_FORMAT:
        struct video_format *f = (struct video_format *)arg;
        hdl->sticker = f->sticker;
        hdl->input_format = FORMAT_YUV422_YUYV;
        hdl->output_format = FORMAT_YUV422_YUYV;
        break;
    default:
        break;
    }
    return 0;
}

static int dma2d_filter_message_callback(pipe_plugin_t *plugin, int cmd, void *arg)
{
    struct dma2d_filter_handle *hdl = (struct dma2d_filter_handle *)plugin->private_data;
    int ret = PIPE_MESSAGE_OK;
    int type = cmd & ~MESSAGE_ALL;
    u8 prev = (cmd & MESSAGE_PREV) ? 1 : 0;

    switch (type) {
    case GET_DATA_INFO:
        pipe_common_t *info = (pipe_common_t *)arg;
        if (info->channel == hdl->source_channel) {
            if (prev) {
                info->format = hdl->output_format;
                info->width = hdl->output_width;
                info->height = hdl->output_height;
            } else {
                info->format = hdl->input_format;
                info->width = hdl->input_width;
                info->height = hdl->input_height;
            }
            return PIPE_MESSAGE_OK;
        }
        ret = PIPE_MESSAGE_CONTINUE;

    case IMC_FRAME_DONE:
        os_sem_set(&hdl->task_sem, 0);
        os_sem_post(&hdl->task_sem);
        break;
    default:
        ret = message_request(plugin->port, cmd, arg);
        break;
    }

    return ret;

}


REGISTER_PLUGIN(dma2d0) = {
    .name           = "dma2d0",
    .type           = PLUGIN_FILTER,
    .init           = dma2d_filter_init,
    .connect        = dma2d_filter_connect,
    .prepare        = dma2d_filter_prepare,
    .start          = dma2d_filter_start,
    .stop           = dma2d_filter_stop,
    .reset          = dma2d_filter_reset,
    .get_parameter  = NULL,
    .set_parameter  = dma2d_filter_set_parameter,
    .msg_cb         = dma2d_filter_message_callback,
};
REGISTER_PLUGIN(dma2d1) = {
    .name           = "dma2d1",
    .type           = PLUGIN_FILTER,
    .init           = dma2d_filter_init,
    .connect        = dma2d_filter_connect,
    .prepare        = dma2d_filter_prepare,
    .start          = dma2d_filter_start,
    .stop           = dma2d_filter_stop,
    .reset          = dma2d_filter_reset,
    .get_parameter  = NULL,
    .set_parameter  = dma2d_filter_set_parameter,
    .msg_cb         = dma2d_filter_message_callback,
};

