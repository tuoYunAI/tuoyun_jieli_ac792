#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vir_data_tx_node.data.bss")
#pragma data_seg(".vir_data_tx_node.data")
#pragma const_seg(".vir_data_tx_node.text.const")
#pragma code_seg(".vir_data_tx_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "app_config.h"


#define LOG_TAG_CONST AUDIO_NODE
#define LOG_TAG             "[VIR_DATA_TX_NODE]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

struct vir_data_tx_hdl {
    u8 start;
    enum stream_node_state state;
    void *file;
    int pid;
    struct stream_frame *frame;
    const struct stream_file_ops *fops;
    struct stream_fmt fmt;
    OS_SEM sem;
};

static void vir_data_tx_task(void *_hdl)
{
    struct vir_data_tx_hdl *hdl = (struct vir_data_tx_hdl *)_hdl;
    struct stream_frame *frame = hdl->frame;

    while (1) {
        if (jlstream_get_iport_frame_num(hdl_node(hdl)->iport) == 0) {
            os_sem_pend(&hdl->sem, 0);
            if (!hdl->start) {
                return;
            }
        }
        while (1) {
            if (!frame) {
                frame = jlstream_pull_frame(hdl_node(hdl)->iport, NULL);
                if (!frame) {
                    break;
                }
                hdl->fops->write(hdl->file, frame->data, frame->len);
                jlstream_free_frame(frame);
                frame = NULL;
                continue;
            }
        }
    }
}

static int vir_data_tx_get_delay(struct vir_data_tx_hdl *hdl, struct stream_note *note)
{
    if (note->stream->scene == STREAM_SCENE_VIR_DATA_TX) {
        if (!list_empty(&hdl_node(hdl)->iport->frames)) {
            return 20;
        }
        return 0;
    }
    return -1;
}


static void vir_data_tx_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct vir_data_tx_hdl *hdl = (struct vir_data_tx_hdl *)iport->node->private_data;

    hdl->state = note->state;
    os_sem_set(&hdl->sem, 0);
    os_sem_post(&hdl->sem);
}

static int vir_data_tx_bind(struct stream_node *node, u16 uuid)
{
    struct vir_data_tx_hdl *hdl = (struct vir_data_tx_hdl *)node->private_data;
    os_sem_create(&hdl->sem, 0);
    return 0;
}

static void vir_data_tx_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = vir_data_tx_handle_frame;
}

static int vir_data_tx_ioc_fmt_nego(struct vir_data_tx_hdl *hdl, struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;

    if (!hdl->fmt.coding_type) {
        return 0;
    }

    in_fmt->coding_type = hdl->fmt.coding_type;
    in_fmt->sample_rate = hdl->fmt.sample_rate;
    in_fmt->channel_mode = AUDIO_CH_MIX;

    return NEGO_STA_ACCPTED;
}

static void vir_data_tx_start(struct vir_data_tx_hdl *hdl)
{
    thread_fork_multiple("vir_data_tx", 25, 512, 0, &hdl->pid, vir_data_tx_task, hdl);
}

static int vir_data_tx_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct vir_data_tx_hdl *hdl = (struct vir_data_tx_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        vir_data_tx_open_iport(iport);
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_FILE:
        hdl->fops = ((struct stream_file_info *)arg)->ops;
        hdl->file = ((struct stream_file_info *)arg)->file;
        break;
    case NODE_IOC_SET_FMT:
        struct stream_fmt *fmt = (struct stream_fmt *)arg;
        hdl->fmt.coding_type = fmt->coding_type;
        hdl->fmt.sample_rate = fmt->sample_rate;
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= vir_data_tx_ioc_fmt_nego(hdl, iport);
        break;
    case NODE_IOC_START:
        if (hdl->start) {
            break;
        }
        hdl->start = 1;
        vir_data_tx_start(hdl);
        break;
    case NODE_IOC_GET_DELAY:
        return vir_data_tx_get_delay(hdl, (struct stream_note *)arg);
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
            os_sem_set(&hdl->sem, 0);
            os_sem_post(&hdl->sem);
            log_info("vir_data_tx node quit");
            thread_kill(&hdl->pid, KILL_WAIT);
        }
        break;
    }

    return 0;
}

static void vir_data_tx_release(struct stream_node *node)
{
    struct vir_data_tx_hdl *hdl = (struct vir_data_tx_hdl *)node->private_data;

    if (hdl) {
        os_sem_del(&hdl->sem, OS_DEL_ALWAYS);
    }
}

#if TCFG_VIRTUAL_DATA_TX_NODE_ENABLE
REGISTER_STREAM_NODE_ADAPTER(vir_data_tx_adapter) = {
    .name       = "vir_data_tx",
    .uuid       = NODE_UUID_VIR_DATA_TX,
    .bind       = vir_data_tx_bind,
    .ioctl      = vir_data_tx_ioctl,
    .release    = vir_data_tx_release,
    .hdl_size   = sizeof(struct vir_data_tx_hdl),
};
#endif

#if TCFG_ACOUSTIC_COMMUNICATION_NODE_ENABLE
REGISTER_STREAM_NODE_ADAPTER(acoustic_com_adapter) = {
    .name       = "acoustic_com",
    .uuid       = NODE_UUID_ACOUSTIC_COM,
    .bind       = vir_data_tx_bind,
    .ioctl      = vir_data_tx_ioctl,
    .release    = vir_data_tx_release,
    .hdl_size   = sizeof(struct vir_data_tx_hdl),
};
#endif

#if TCFG_AI_TX_NODE_ENABLE
REGISTER_STREAM_NODE_ADAPTER(ai_tx_adapter) = {
    .name       = "ai_tx",
    .uuid       = NODE_UUID_AI_TX,
    .bind       = vir_data_tx_bind,
    .ioctl      = vir_data_tx_ioctl,
    .release    = vir_data_tx_release,
    .hdl_size   = sizeof(struct vir_data_tx_hdl),
};
#endif
