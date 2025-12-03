#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vir_data_rx_node.data.bss")
#pragma data_seg(".vir_data_rx_node.data")
#pragma const_seg(".vir_data_rx_node.text.const")
#pragma code_seg(".vir_data_rx_node.text")
#endif
#include "source_node.h"
/* #include "decoder_node.h" */

#define LOG_TAG     		"[VIR_DATA_RX_PLUG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"

struct vir_data_rx_hdl {
    void *file;
    struct stream_node *node;
    const struct stream_file_ops *file_ops;
};

static int vir_data_rx_fseek(void *_hdl, u32 fpos)
{
    struct vir_data_rx_hdl *hdl = (struct vir_data_rx_hdl *)_hdl;

    if (!hdl->file) {
        return -1;
    }

    return hdl->file_ops->seek(hdl->file, fpos, SEEK_SET);
}

static int vir_data_rx_fread(void *_hdl, u8 *data, int size)
{
    struct vir_data_rx_hdl *hdl = (struct vir_data_rx_hdl *)_hdl;

    if (!hdl->file) {
        return 0;
    }

    return hdl->file_ops->read(hdl->file, data, size);
}

static void *vir_data_rx_init(void *priv, struct stream_node *node)
{
    struct vir_data_rx_hdl *hdl = zalloc(sizeof(*hdl));
    hdl->node = node;
    return hdl;
}

static void vir_data_rx_ioc_set_file(struct vir_data_rx_hdl *hdl, struct stream_file_info *info)
{
    hdl->file = info->file;
    hdl->file_ops = info->ops;
}

static int vir_data_rx_ioc_file_start(struct vir_data_rx_hdl *hdl)
{
    int err = 0;
    if (jlstream_is_contains_node_from(hdl->node, NODE_UUID_PLAY_SYNC) ||
        jlstream_is_contains_node_from(hdl->node, NODE_UUID_CAPTURE_SYNC)) {
        err = stream_node_ioctl(hdl->node, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, 0);
        if (err) {
            log_error("Music file set timestamp error");
        }
        /* log_debug("Music file set timestamp : %d", err); */
    }

    return err;
}

static int vir_data_rx_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct vir_data_rx_hdl *hdl = (struct vir_data_rx_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_SET_FILE:
        vir_data_rx_ioc_set_file(hdl, (struct stream_file_info *)arg);
        break;
    case NODE_IOC_GET_FMT:
        if (hdl->file_ops->get_fmt) {
            ret = hdl->file_ops->get_fmt(hdl->file, (struct stream_fmt *)arg);
        } else {
            ret = -EINVAL;
        }
        break;
    case NODE_IOC_START:
        vir_data_rx_ioc_file_start(hdl);
        break;
    case NODE_IOC_STOP:
        break;
    }

    return ret;
}

static void vir_data_rx_release(void *_hdl)
{
    struct vir_data_rx_hdl *hdl = (struct vir_data_rx_hdl *)_hdl;

    if (hdl->file) {
        hdl->file_ops->close(hdl->file);
    }
    free(hdl);
}

REGISTER_SOURCE_NODE_PLUG(vir_data_rx_plug) = {
    .uuid       = NODE_UUID_VIR_DATA_RX,
    .init       = vir_data_rx_init,
    .read       = vir_data_rx_fread,
    .seek       = vir_data_rx_fseek,
    .ioctl      = vir_data_rx_ioctl,
    .release    = vir_data_rx_release,
};

