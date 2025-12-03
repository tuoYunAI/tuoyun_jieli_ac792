#ifndef _AUDIO_VOCAL_TRACK_API_H_
#define _AUDIO_VOCAL_TRACK_API_H_

#include "generic/typedef.h"
#include "generic/list.h"
#include "media/includes.h"

#define FL_FR (BIT(0)|BIT(1))
#define RL_RR (BIT(2)|BIT(3))


struct audio_vocal_track;

struct audio_vocal_track_handler {
    int (*vocal_track_output)(struct audio_vocal_track *, s16 *, u16);
};

struct audio_vocal_track {
    struct list_head head;		// 链表头
    s16 *output;				// 输出buf
    u16 points;			        // 输出buf总点数
    u16 remain_points;	        // 输出剩余点数
    u32 process_len;	        // 输出了多少
    u8  channel_num;	        // 声道数
    u8  sample_sync;            //记录同步模块句柄
    volatile u8 active;	        // 活动标记。1-正在运行
    u8 bit_wide;
    u32 sample_rate;	        // 当前采样率
    const struct audio_vocal_track_handler *vocal_track_handler;
    void *vocal_track_synthesis_hdl;//记录vocal_track_node
};

struct audio_vocal_track_ch {
    u8 start;		        // 启动标记。1-已经启动。输出时标记一次
    u8 open;		        // 打开标记。1-已经打开。输出标记为1，reset标记为0
    u8 pause;		        // 暂停标记。1-暂停
    u8 wait_resume;
    u8  start_offset;
    u16 offset;			        // 当前通道在输出buf中的偏移位置
    u32 synthesis_tar;          //输入目标声道 FL:bit0, FR:bit1, RL:bit2, RR:bit3

    struct list_head list_entry;
    struct audio_vocal_track *vocal_track;
};

/*----------------------------------------------------------------------------*/
/**@brief   打开声道合并
   @param   *vocal_track: 句柄
   @param   len:声道合并buf长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_track_open(struct audio_vocal_track *vocal_track, u32 len);

/*----------------------------------------------------------------------------*/
/**@brief   关闭声道合并
   @param   *vocal_track: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_track_close(struct audio_vocal_track *vocal_track);

/*----------------------------------------------------------------------------*/
/**@brief   声道合并的通道数
   @param   *vocal_track: 句柄
   @param   channel_num:输出通道数，支持（四声道）
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_track_set_channel_num(struct audio_vocal_track *vocal_track, u8 channel_num);

/*
 *设置声道组合器的输出buf
 * */
void audio_vocal_track_set_output_buf(struct audio_vocal_track *vocal_track, s16 *buf, u16 len);

/*----------------------------------------------------------------------------*/
/**@brief   声道合并数据流激活
   @param   *priv: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_track_stream_resume(void *priv);

/*----------------------------------------------------------------------------*/
/**@brief   声道合并输入源通道打开
   @param    *ch: 源通道句柄
   @param    *vocal_track:声道合并句柄
   @param    synthesis_tar:输出的目标声道  FL_FR、RL_RR
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_track_synthesis_open(struct audio_vocal_track_ch *ch, struct audio_vocal_track *vocal_track, u32 synthesis_tar);

/*----------------------------------------------------------------------------*/
/**@brief    声道合并输入源通道关闭
   @param    *ch:声道输入源句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_track_synthesis_close(struct audio_vocal_track_ch *ch);

/*----------------------------------------------------------------------------*/
/**@brief    设置声道合并输出回调接口
   @param    *vocal_track: 句柄, *handler:输出回调
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_track_set_output_handler(struct audio_vocal_track *vocal_track, const struct audio_vocal_track_handler *handler);

/*----------------------------------------------------------------------------*/
/**@brief    声道合并处理
   @param    *ch:声道输入源句柄, data:输入数据，len:输入数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_track_synthesis_write(struct audio_vocal_track_ch *ch, s16 *data, u32 len);

void audio_vocal_track_set_bit_wide(struct audio_vocal_track *vocal_track, u8 bit_wide);

#endif

