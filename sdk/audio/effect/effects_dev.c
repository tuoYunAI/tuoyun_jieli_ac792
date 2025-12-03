#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "jlstream.h"
#include "effects/effects_adj.h"
#include "effects_dev.h"

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[EFFECTS_ADJ]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"


#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".effect_dev.data.bss")
#pragma data_seg(".effect_dev.data")
#pragma const_seg(".effect_dev.text.const")
#pragma code_seg(".effect_dev.text")
#endif

#if (TCFG_EFFECT_DEV0_NODE_ENABLE || TCFG_EFFECT_DEV1_NODE_ENABLE || TCFG_EFFECT_DEV2_NODE_ENABLE || TCFG_EFFECT_DEV2_NODE_ENABLE || TCFG_EFFECT_DEV3_NODE_ENABLE || TCFG_EFFECT_DEV4_NODE_ENABLE)

void effect_dev_init(struct packet_ctrl *hdl, u32 process_points_per_ch)
{
    if (process_points_per_ch) {
        hdl->frame_len = process_points_per_ch * hdl->in_ch_num * (2 << hdl->bit_width);
        hdl->remain_buf = zalloc(hdl->frame_len);
    }
}

void effect_dev_process(struct packet_ctrl *hdl, struct stream_iport *iport,  struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_frame *out_frame;
    struct stream_node *node = iport->node;

    if (hdl->remain_buf) {
        frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!frame) {
            return;
        }

        /*算法出来一帧的数据长度，byte*/
        int total_len = hdl->remain_len + frame->len;  //记录目前还有多少数据
        u8 pack_frame_num = total_len / hdl->frame_len;//每次数据需要跑多少帧
        u16 pack_frame_len = pack_frame_num * hdl->frame_len;       //记录本次需要跑多少数据
        u16 out_frame_len = hdl->out_ch_num * pack_frame_len / hdl->in_ch_num;

        if (pack_frame_num) {
            if (!out_frame) {
                out_frame = jlstream_get_frame(node->oport, out_frame_len);
                if (!out_frame) {
                    return;
                }
            }
            int out_offset = 0;
            if (hdl->remain_len) {
                int need_size = hdl->frame_len - hdl->remain_len;
                /*拷贝上一次剩余的数据*/
                memcpy((u8 *)hdl->remain_buf + hdl->remain_len, frame->data, need_size);
                if (hdl->out_ch_num == hdl->in_ch_num) {
                    memcpy(out_frame->data, hdl->remain_buf, hdl->frame_len);
                }
                hdl->effect_run(hdl->node_hdl, (s16 *)hdl->remain_buf, (s16 *)out_frame->data,
                                hdl->frame_len);
                hdl->remain_len = 0;
                frame->offset += need_size;
                out_offset = hdl->frame_len * hdl->out_ch_num / hdl->in_ch_num;
                pack_frame_num--;
            }
            while (pack_frame_num--) {
                if (hdl->out_ch_num == hdl->in_ch_num) {
                    memcpy(out_frame->data + out_offset, frame->data + frame->offset,
                           hdl->frame_len);
                }
                hdl->effect_run(hdl->node_hdl, (s16 *)(frame->data + frame->offset),
                                (s16 *)(out_frame->data + out_offset), hdl->frame_len);
                frame->offset += hdl->frame_len;
                out_offset += hdl->frame_len * hdl->out_ch_num  / hdl->in_ch_num;
            }
            if (frame->offset < frame->len) {
                memcpy(hdl->remain_buf, frame->data + frame->offset, frame->len - frame->offset);
                hdl->remain_len = frame->len - frame->offset;
            }
            out_frame->len = out_frame_len;
            jlstream_push_frame(node->oport, out_frame);	//将数据推到oport
            jlstream_free_frame(frame);	//释放iport资源
        } else {
            /*当前数据不够跑一帧算法时*/
            memcpy((void *)((int)hdl->remain_buf + hdl->remain_len), frame->data, frame->len);
            hdl->remain_len += frame->len;
            jlstream_free_frame(frame);	//释放iport资源
        }
    } else {
        frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!frame) {
            return;
        }

        if (hdl->out_ch_num != hdl->in_ch_num) {
            int out_len = hdl->out_ch_num * frame->len / hdl->in_ch_num;
            out_frame = jlstream_get_frame(node->oport, out_len);
            if (!out_frame) {
                return;
            }
            hdl->effect_run(hdl->node_hdl, (s16 *)frame->data, (s16 *)out_frame->data, frame->len);
            out_frame->len = out_len;
            jlstream_push_frame(node->oport, out_frame);	//将数据推到oport
            jlstream_free_frame(frame);	//释放iport资源
        } else {
            hdl->effect_run(hdl->node_hdl, (s16 *)frame->data, (s16 *)frame->data, frame->len);
            jlstream_push_frame(node->oport, frame);	//将数据推到oport
        }
    }
}

void effect_dev_close(struct packet_ctrl *hdl)
{
    hdl->remain_len = 0;
    if (hdl->remain_buf) {
        free(hdl->remain_buf);
        hdl->remain_buf = NULL;
    }
    if (hdl->pack_frame) {
        free(hdl->pack_frame);
        hdl->pack_frame = NULL;
    }
    if (hdl->out_frame) {
        jlstream_free_frame(hdl->out_frame);
        hdl->out_frame = NULL;
    }
}

#endif
