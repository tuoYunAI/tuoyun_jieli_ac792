#ifndef _SOURCE_NODE_MGR_H_
#define _SOURCE_NODE_MGR_H_

#include "jlstream.h"
#include "media/audio_base.h"

enum {
    SRC_NODE_ERR_FILE_END = 1,
};

struct source_node_plug {
    u16 uuid;
    u16 frame_size;
    void *(*init)(void *, struct stream_node *node);
    int (*read)(void *, u8 *buf, int len);
    int (*seek)(void *, u32 pos);
    enum stream_node_state(*get_frame)(void *, struct stream_frame **);
    int (*ioctl)(void *, int cmd, int arg);
    void (*release)(void *);
};

#define REGISTER_SOURCE_NODE_PLUG(plug) \
    const struct source_node_plug plug SEC_USED(.source_node_plug)

struct stream_frame *source_plug_get_output_frame(void *, int len);

void source_plug_put_output_frame(void *, struct stream_frame *);

void source_plug_free_frame(void *, struct stream_frame *);

u16 get_source_node_plug_uuid(void *source_node);

struct stream_frame *source_plug_get_output_frame_by_id(void *_hdl, int id, int len);

void source_plug_put_output_frame_by_id(void *_hdl, int id, struct stream_frame *frame);

void source_node_push_data(struct stream_snode *snode, struct stream_note *note);

int pcm_stream_data_write(u8 *data, u32 data_len); //pcm流写入数据接口，返回非0为成功写入数据长度

#endif
