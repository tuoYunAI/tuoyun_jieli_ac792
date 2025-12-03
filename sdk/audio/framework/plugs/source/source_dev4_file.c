#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".source_dev4_file.data.bss")
#pragma data_seg(".source_dev4_file.data")
#pragma const_seg(".source_dev4_file.text.const")
#pragma code_seg(".source_dev4_file.text")
#endif
#include "classic/hci_lmp.h"
#include "source_node.h"
#include "jlstream.h"
#include "media/audio_base.h"
#include "app_config.h"

/*
   若源节点为中断节点，则需打开SOURCE_DEV4_IRQ_ENABLE
   并且在中断中调用source_dev4_packet_rx_notify, 用于中断收到数据时，主动唤醒数据流
*/
#define SOURCE_DEV4_IRQ_ENABLE		0		//中断节点使能

#if TCFG_SOURCE_DEV4_NODE_ENABLE

struct source_dev4_file_hdl {
    u8 start;
    void *file;
    struct stream_node *node;
    const struct stream_file_ops *file_ops;
};

//解码读文件缓存buf大小,如果遇到卡速较慢,播放高码率文件有卡顿情况，可增加此缓存大小
static const int config_file_buf_size = 4 * 1024;

static enum stream_node_state source_dev4_get_frame(void *_hdl, struct stream_frame **_frame)
{
    int ret = 0;
    struct source_dev4_file_hdl *hdl = (struct source_dev4_file_hdl *)_hdl;
    struct stream_frame *frame;

    if (!hdl->file) {
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    frame = jlstream_get_frame(hdl->node->oport, config_file_buf_size);
    frame->len = ret = hdl->file_ops->read(hdl->file, frame->data, config_file_buf_size);

    if (ret <= 0) {
        jlstream_free_frame(frame);
        frame = NULL;
    }

    *_frame = frame;

    return NODE_STA_RUN;
}

//自定义源节点挂起
static void source_dev4_suspend(struct source_dev4_file_hdl *hdl)
{
    /*
       do something
    	1、启动丢数-避免suspend时 自定义源节点数据溢出
    	2、挂起时，源节点需要的其他流程
    */
}

//自定义源节点 初始化
static void source_dev4_open(struct source_dev4_file_hdl *hdl)
{
    /*
       do something
    	1、关闭丢数-用于suspend流程保护
    	2、(中断节点需要)注册自定义源节点收包回调 source_dev4_packet_rx_notify
    	3、自定义源节点启动流程
    */
}

//自定义源节点 停止
static void source_dev4_close(struct source_dev4_file_hdl *hdl)
{
    //do something
}

static void source_dev4_packet_rx_notify(void *_hdl)
{
    struct source_dev4_file_hdl *hdl = (struct source_dev4_file_hdl *)_hdl;
    //启动状态才触发
    if (hdl->start) {
        jlstream_wakeup_thread(NULL, hdl->node, NULL);
    }
}


static void *source_dev4_init(void *priv, struct stream_node *node)
{
    struct source_dev4_file_hdl *hdl = zalloc(sizeof(*hdl));
    printf("source_dev4_init");
    hdl->node = node;
#if SOURCE_DEV4_IRQ_ENABLE
    node->type |= NODE_TYPE_IRQ;
#endif/*SOURCE_DEV4_IRQ_ENABLE*/
    return hdl;
}

//获取当前节点数据参数
static int source_dev4_ioc_get_fmt(struct source_dev4_file_hdl *hdl, struct stream_fmt *fmt)
{
    int ret = 0;
    if (hdl->file_ops->get_fmt) {
        ret = hdl->file_ops->get_fmt(hdl->file, fmt);
    } else {
        printf("source_dev4 get fmt err\n");
        ret = -EINVAL;
    }
    return ret;
}

static void source_dev4_ioc_suspend(struct source_dev4_file_hdl *hdl)
{
    printf("source_dev4_ioc_suspend");
    hdl->start = 0;
    source_dev4_suspend(hdl);
}

static void source_dev4_ioc_start(struct source_dev4_file_hdl *hdl)
{
    printf("source_dev4_ioc_start");
    source_dev4_open(hdl);
    stream_node_ioctl(hdl->node, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, 0);

    hdl->start = 1;
}

static void source_dev4_ioc_stop(struct source_dev4_file_hdl *hdl)
{
    printf("source_dev4_ioc_stop");
    hdl->start = 0;
    source_dev4_close(hdl);
}

static void source_dev4_ioc_set_file(struct source_dev4_file_hdl *hdl, struct stream_file_info *info)
{
    hdl->file = info->file;
    hdl->file_ops = info->ops;
}

static int source_dev4_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct source_dev4_file_hdl *hdl = (struct source_dev4_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_GET_FMT:
        ret = source_dev4_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SUSPEND:
        source_dev4_ioc_suspend(hdl);
        break;
    case NODE_IOC_START:
        source_dev4_ioc_start(hdl);
        break;
    case NODE_IOC_STOP:
        source_dev4_ioc_stop(hdl);
        break;
    case NODE_IOC_SET_FILE:
        source_dev4_ioc_set_file(hdl, (struct stream_file_info *)arg);
        break;
    }

    return ret;
}

static void source_dev4_release(void *_hdl)
{
    struct source_dev4_file_hdl *hdl = (struct source_dev4_file_hdl *)_hdl;
    printf("source_dev4_release");

    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(source_dev4_file_plug) = {
    .uuid       = NODE_UUID_SOURCE_DEV4,
    .init       = source_dev4_init,
    .get_frame  = source_dev4_get_frame,
    .ioctl      = source_dev4_ioctl,
    .release    = source_dev4_release,
};

#endif

