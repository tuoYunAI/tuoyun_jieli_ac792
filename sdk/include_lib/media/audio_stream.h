#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include "generic/includes.h"
#include "media/audio_base.h"


// 数据流IOCTRL命令
#define AUDIO_STREAM_IOCTRL_CMD_CHECK_ACTIVE		(1) // 检查数据流是否活动


struct audio_stream_entry;
struct audio_frame_copy;


/*
 * 数据流中的传输内容
 */
struct audio_data_frame {
    u8 channel;				// 通道数
    u16 stop : 1;			// 数据流停止标志
    u16 data_sync : 1;		// 数据流同步标志
    u16 no_subsequent : 1;	// 1-不再执行后续的数据流；0-正常执行
    u32 sample_rate;		// 采样率
    u16 offset;             // 数据偏移
    u16 data_len;			// 数据长度
    s16 *data;				// 数据地址
};

/*
 * 数据流
 * 多个数据流节点组成一个数据流
 */
struct audio_stream {
    struct audio_stream_entry *entry;	// 数据流节点
    void *priv;							// resume私有参数
    void (*resume)(void *priv);			// 激活回调接口
};

/*
 * 数据流群组
 * 用于串联多个数据流，在调用resume/clear等操作时依次处理各个数据流
 * 如数据流stream0最后一个节点是entry0_n，数据流1最后一个节点是entry1_n
 * 把这两个数据流加入到群组test_group中
 * audio_stream_group_add_entry(&test_group, &entry0_n);
 * audio_stream_group_add_entry(&test_group, &entry1_n);
 * 另一个数据流streamX的第一个节点是entryX_0，把群组关联到streamX数据流
 * entryX_0.group = &test_group;
 * 那么调用streamX中的resume就会依次调用stream0和stream1的resume了
 */
struct audio_stream_group {
    struct audio_stream_entry *entry;	// 数据流节点
};

/*
 * 数据流节点
 * 数据流节点依次串联，数据依次往后传递
 */
struct audio_stream_entry {
    u8  pass_by;    // 1-同步处理（即往后传的buf就是上层传入的buf）；
    // 0-异步处理（数据存到其他buf再往后传）
    u8  remain;		// 1-上次数据没输出完。0-上次数据输出完
    u16 offset;		// 同步处理时的数据偏移
    struct audio_stream *stream;		// 所属的数据流
    struct audio_stream_entry *input;	// 上一个节点
    struct audio_stream_entry *output;	// 下一个节点
    struct audio_stream_entry *sibling;	// 数据流群组中的数据流节点链表
    struct audio_stream_group *group;	// 数据流群组节点
    struct audio_frame_copy *frame_copy;	// 数据分支节点
    int (*prob_handler)(struct audio_stream_entry *,  struct audio_data_frame *in);	// 预处理
    int (*data_handler)(struct audio_stream_entry *,  struct audio_data_frame *in,
                        struct audio_data_frame *out);					// 数据处理
    void (*data_process_len)(struct audio_stream_entry *,  int len);	// 后级返回使用的数据长度
    void (*data_clear)(struct audio_stream_entry *);					// 清除节点数据
    int (*ioctrl)(struct audio_stream_entry *, int cmd, int *param);	// 节点IOCTRL
};

/*
 * 数据流分支
 * 支持一传多（分支）功能，内部自动生成struct audio_frame_copy来处理多个分支
 * 有多少个分支就会申请多少个空间，数据分别拷贝到对应分支空间然后往后传递
 */
struct audio_frame_copy {
    struct list_head head;				// 链表。用于连接各个分支
    struct audio_data_frame frame;		// 保存上层的传输内容
    struct audio_stream_entry entry;	// 连接上层的节点
};

#endif

