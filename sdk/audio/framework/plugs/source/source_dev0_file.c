#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".source_dev0_file.data.bss")
#pragma data_seg(".source_dev0_file.data")
#pragma const_seg(".source_dev0_file.text.const")
#pragma code_seg(".source_dev0_file.text")
#endif
#include "classic/hci_lmp.h"
#include "source_node.h"
#include "jlstream.h"
#include "media/audio_base.h"
#include "app_config.h"
#include "generic/circular_buf.h"

/*
   若源节点为中断节点，则需打开SOURCE_DEV0_IRQ_ENABLE
   并且在中断中调用source_dev0_packet_rx_notify, 用于中断收到数据时，主动唤醒数据流
*/
#define SOURCE_DEV0_IRQ_ENABLE		0		//中断节点使能

#if TCFG_SOURCE_DEV0_NODE_ENABLE

#define PCM_CBUF_LEN (200*1024)  //pcm_stream_data_write外部流数据写入的cbuf长度，不够时适当加大

struct source_dev0_file_hdl {
    u8 start;					//启动标志
    struct stream_node *node;	//节点句柄
    u8 bt_addr[6];				//蓝牙MAC地址
    u8 channel_mode;
    u32 sample_rate;
    u32 coding_type;
    void *cache_buf;
    cbuffer_t save_cbuf;
};

struct source_dev0_file_hdl *g_dev_flow_hdl = NULL;

//return wlen返回成功写入缓存的数据长度,返回0未成功写入数据
int pcm_stream_data_write(u8 *data, u32 data_len)
{
    int wlen = 0;
    if (g_dev_flow_hdl) {
        if (g_dev_flow_hdl->start) {
            wlen = cbuf_write(&g_dev_flow_hdl->save_cbuf, data, data_len);
            if (wlen == 0) {
                cbuf_clear(&g_dev_flow_hdl->save_cbuf);
            }
        }
    }

    return wlen;
}

/*
   输入源节点数据
   	hdl 节点私有句柄
   	*len 源节点数据长度
   	return 源节点数据指针,没有数据时候返回NULL
*/
static u8 *source_dev0_get_packet(struct source_dev0_file_hdl *hdl, u32 *len)
{
    u8 *packet = NULL;
    u32 packet_len = 0;

    int rlen;
    int data_len = cbuf_get_data_size(&g_dev_flow_hdl->save_cbuf);
    if (data_len > 0) {
        u8 *pcm_buf = malloc(data_len);
        if (!pcm_buf) {
            return NULL;
        }
        rlen = cbuf_read(&g_dev_flow_hdl->save_cbuf, pcm_buf, data_len);
        packet_len = rlen;
        packet = pcm_buf;
    }

    if (data_len == 0) {
        return NULL;
    }

    *len = packet_len;
    return packet;
}

//释放源节点数据
static int source_dev0_free_packet(struct source_dev0_file_hdl *hdl, u8 *packet)
{
    free(packet);
    return 0;
}

//自定义源节点挂起
static void source_dev0_suspend(struct source_dev0_file_hdl *hdl)
{
    /*
       do something
    	1、启动丢数-避免suspend时 自定义源节点数据溢出
    	2、挂起时，源节点需要的其他流程
    */
}

//自定义源节点 初始化
static void source_dev0_open(struct source_dev0_file_hdl *hdl)
{
    /*
       do something
    	1、关闭丢数-用于suspend流程保护
    	2、(中断节点需要)注册自定义源节点收包回调 source_dev0_packet_rx_notify
    	3、自定义源节点启动流程
    */
}

//自定义源节点 停止
static void source_dev0_close(struct source_dev0_file_hdl *hdl)
{
    if (hdl->cache_buf) {
        free(hdl->cache_buf);
        hdl->cache_buf = NULL;
    }
}

/*
   自定义源节点 参数设置
   *fmt 目标参数,如采样率，数据类型，通道模式
*/
static void source_dev0_get_fmt(struct source_dev0_file_hdl *hdl, struct stream_fmt *fmt)
{
    //set data format
    fmt->sample_rate = hdl->sample_rate;
    fmt->coding_type = hdl->coding_type;
    fmt->channel_mode = hdl->channel_mode;
}

/*
	(当前节点为中断节点使用)
   自定义源节点收包回调
   - 用于自定义源节点收到数时, 唤醒数据流
*/
static void source_dev0_packet_rx_notify(void *_hdl)
{
    struct source_dev0_file_hdl *hdl = (struct source_dev0_file_hdl *)_hdl;
    //启动状态才触发
    if (hdl->start) {
        jlstream_wakeup_thread(NULL, hdl->node, NULL);
    }
}


static enum stream_node_state source_dev0_get_frame(void *_hdl, struct stream_frame **_frame)
{
    u32 len = 0;
    struct source_dev0_file_hdl *hdl = (struct source_dev0_file_hdl *)_hdl;
    struct stream_frame *frame;

    //1、获取当前节点数据
    u8 *packet = source_dev0_get_packet(hdl, &len);
    if (!packet) {
        *_frame = NULL;
        //表示源节点获取不到数据
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    //2、申请数据流frame空间
    frame = jlstream_get_frame(hdl->node->oport, len);
    frame->len = len;

    //3、将当前节点数据拷贝到frame
    memcpy(frame->data, packet, len);

    //4、释放当前节点数据
    source_dev0_free_packet(hdl, packet);

    *_frame = frame;

    return NODE_STA_RUN;
}

static void *source_dev0_init(void *priv, struct stream_node *node)
{
    struct source_dev0_file_hdl *hdl = zalloc(sizeof(*hdl));
    if (!hdl) {
        return NULL;
    }

    printf("source_dev0_init");
    hdl->node = node;
#if SOURCE_DEV0_IRQ_ENABLE
    node->type |= NODE_TYPE_IRQ;
#endif/*SOURCE_DEV0_IRQ_ENABLE*/

    hdl->cache_buf = malloc(PCM_CBUF_LEN);
    if (!hdl->cache_buf) {
        free(hdl);
        return NULL;
    }
    cbuf_init(&hdl->save_cbuf, hdl->cache_buf, PCM_CBUF_LEN);

    g_dev_flow_hdl = hdl;

    return hdl;
}

//获取当前蓝牙地址
static int source_dev0_ioc_set_bt_addr(struct source_dev0_file_hdl *hdl, u8 *bt_addr)
{
    memcpy(hdl->bt_addr, bt_addr, 6);
    return 0;
}

//获取当前节点数据参数
static void source_dev0_ioc_get_fmt(struct source_dev0_file_hdl *hdl, struct stream_fmt *fmt)
{
    source_dev0_get_fmt(hdl, fmt);
}

static void source_dev0_ioc_suspend(struct source_dev0_file_hdl *hdl)
{
    printf("source_dev0_ioc_suspend");
    hdl->start = 0;
    source_dev0_suspend(hdl);
}

static void source_dev0_ioc_start(struct source_dev0_file_hdl *hdl)
{
    printf("source_dev0_ioc_start");
    source_dev0_open(hdl);
    stream_node_ioctl(hdl->node, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, 0);
    hdl->start = 1;
}

static void source_dev0_ioc_stop(struct source_dev0_file_hdl *hdl)
{
    printf("source_dev0_ioc_stop");
    hdl->start = 0;
    source_dev0_close(hdl);
}

static void source_dev0_ioc_set_prev_fmt(struct source_dev0_file_hdl *hdl, struct stream_fmt *fmt)
{
    hdl->channel_mode = fmt->channel_mode;
    hdl->sample_rate =  fmt->sample_rate;
    hdl->coding_type = fmt->coding_type;
}

static int source_dev0_ioctl(void *_hdl, int cmd, int arg)
{
    struct source_dev0_file_hdl *hdl = (struct source_dev0_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_SET_PRIV_FMT:
        source_dev0_ioc_set_prev_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_BTADDR:
        source_dev0_ioc_set_bt_addr(hdl, (u8 *)arg);
        break;
    case NODE_IOC_GET_FMT:
        source_dev0_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        break;
    case NODE_IOC_SUSPEND:
        source_dev0_ioc_suspend(hdl);
        break;
    case NODE_IOC_START:
        source_dev0_ioc_start(hdl);
        break;
    case NODE_IOC_STOP:
        source_dev0_ioc_stop(hdl);
        break;
    }

    return 0;
}

static void source_dev0_release(void *_hdl)
{
    struct source_dev0_file_hdl *hdl = (struct source_dev0_file_hdl *)_hdl;
    printf("source_dev0_release");

    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(source_dev0_file_plug) = {
    .uuid       = NODE_UUID_SOURCE_DEV0,
    .init       = source_dev0_init,
    .get_frame  = source_dev0_get_frame,
    .ioctl      = source_dev0_ioctl,
    .release    = source_dev0_release,
};

#endif








