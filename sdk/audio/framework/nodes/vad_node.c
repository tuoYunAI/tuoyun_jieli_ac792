#include "vad_node.h"
#include "jlstream.h"
#include "generic/circular_buf.h"
#include "effects/effects_adj.h"

#define LOG_TAG_CONST AUDIO_NODE
#define LOG_TAG     "[VAD-NODE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"

typedef enum {
    VAD_WAKEUP,
    VAD_SPEAKING,
    VAD_PLAYING,
} vad_status_enum_t;

struct vad_node_hdl {
    u8 start            : 1;
    u8 speak_stop       : 1;
    u8 vad_status_bef   : 2;
    u8 vad_status       : 2;
    u8 aligned          : 2; //仅对齐
    u32 sample_rate;
    u32 input_buf_len;
    int pid;
    u8 *input_buf;
    void *vad;
    cbuffer_t cbuf;
    vad_node_priv_t priv;
    OS_SEM vad_sem;
};

extern int vad_main(void *vad, char *data, int length);
extern void *vad_init(int sample_rate, int voice_valid_time, int speaking_silence_valid_time);
extern void vad_free(void *vad);;
extern void vad_reset(void *vad);

static void vad_node_task(void *_hdl)
{
    char vad_data[640];
    u16 rlen;
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)_hdl;
    u16 data_len = hdl->sample_rate == 8000 ? 320 : 640;

    while (1) {
        do {
            rlen = cbuf_read(&hdl->cbuf, vad_data, data_len);
            if (rlen == 0) {
                os_sem_pend(&hdl->vad_sem, 0); /* 等待有数据再读，释放CPU资源 */
                if (!hdl->start) {
                    return;        /* 任务要被删除 */
                }
            }
        } while (rlen == 0);

        hdl->vad_status_bef = hdl->vad_status;
        hdl->vad_status = vad_main(hdl->vad, vad_data, data_len);

        if ((hdl->vad_status_bef == VAD_SPEAKING) && (hdl->vad_status == VAD_PLAYING)) {
            hdl->vad_status = VAD_WAKEUP;
            hdl->speak_stop = 1;
            vad_reset(hdl->vad);
            if (hdl->priv.vad_callback) {
                hdl->priv.vad_callback(VAD_EVENT_SPEAK_STOP);
            }
        } else if ((hdl->vad_status_bef == VAD_WAKEUP) && (hdl->vad_status == VAD_SPEAKING)) {
            hdl->speak_stop = 0;
            if (hdl->priv.vad_callback) {
                hdl->priv.vad_callback(VAD_EVENT_SPEAK_START);
            }
        }
        if (hdl->priv.auto_refresh_disable && hdl->speak_stop) {
            hdl->start = 0;
            return;
        }
    }
}

static void vad_node_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)iport->node->private_data;
    struct stream_node *node = hdl_node(hdl);
    struct stream_frame *frame;

    frame = jlstream_pull_frame(iport, note);
    if (!frame) {
        return;
    }
    if (hdl->start) {
        if (hdl->input_buf_len < (frame->len + 640)) {
            if (hdl->input_buf) {
                free(hdl->input_buf);
            }
            hdl->input_buf_len = 2 * 1024;
            if (hdl->input_buf_len < frame->len + 640) {
                hdl->input_buf_len = frame->len + 640;
            }
            hdl->input_buf = (u8 *)zalloc(hdl->input_buf_len);
            if (!hdl->input_buf) {
                log_error("vad input_buf malloc fail!");
                return;
            }
            cbuf_init(&hdl->cbuf, hdl->input_buf, hdl->input_buf_len);
        }

        if (cbuf_is_write_able(&hdl->cbuf, frame->len)) {
            cbuf_write(&hdl->cbuf, frame->data, frame->len);
        }
        if (cbuf_get_data_size(&hdl->cbuf) >= (hdl->sample_rate == 8000 ? 320 : 640)) {
            os_sem_set(&hdl->vad_sem, 0);
            os_sem_post(&hdl->vad_sem);
        }
    }
    jlstream_push_frame(node->oport, frame);
}

static int vad_node_bind(struct stream_node *node, u16 uuid)
{
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)node->private_data;
    os_sem_create(&hdl->vad_sem, 0);
    return 0;
}

static int vad_node_start(struct vad_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    ASSERT(fmt->sample_rate == 8000 || fmt->sample_rate == 16000, "vad-sample_rate is only support 8kHZ or 16kHZ");

    hdl->sample_rate = fmt->sample_rate;
    hdl->vad_status = VAD_WAKEUP;
    hdl->vad = vad_init(hdl->sample_rate, hdl->priv.start_threshold, hdl->priv.stop_threshold);
    if (!hdl->vad) {
        log_error("vad open fail!");
        return 1;
    }

    log_info("VAD parm:sr = %d, start_threshold = %d, stop_threshold = %d", hdl->sample_rate, hdl->priv.start_threshold, hdl->priv.stop_threshold);

    hdl->start = 1;
    return thread_fork_multiple("audio_vad", 11, 768, 0, &hdl->pid, vad_node_task, hdl);
}

static void vad_ioc_open_input_port(struct stream_iport *iport)
{
    iport->handle_frame = vad_node_handle_frame;
}

static int vad_node_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        vad_ioc_open_input_port(iport);
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_START:
        if (hdl->start) {
            return 0;
        }
        ret = vad_node_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start == 0) {
            return 0;
        }
        hdl->start = 0;
        os_sem_set(&hdl->vad_sem, 0);
        os_sem_post(&hdl->vad_sem);
        thread_kill(&hdl->pid, KILL_WAIT);
        log_info("vad node quit");
        break;
    case NODE_IOC_SET_PRIV_FMT:
        memcpy(&hdl->priv, (vad_node_priv_t *)arg, sizeof(vad_node_priv_t));
        break;
    }

    return ret;
}

static void vad_node_release(struct stream_node *node)
{
    struct vad_node_hdl *hdl = (struct vad_node_hdl *)node->private_data;
    if (hdl) {
        os_sem_del(&hdl->vad_sem, OS_DEL_ALWAYS);
        if (hdl->vad) {
            vad_free(hdl->vad);
            hdl->vad = NULL;
        }
        if (hdl->input_buf) {
            free(hdl->input_buf);
        }
    }
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(vad_node_adapter) = {
    .name       = "vad",
    .uuid       = NODE_UUID_VAD,
    .bind       = vad_node_bind,
    .ioctl      = vad_node_ioctl,
    .release    = vad_node_release,
    .hdl_size   = sizeof(struct vad_node_hdl),
};

