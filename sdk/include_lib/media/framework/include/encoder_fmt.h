#ifndef _ENCODER_FMT_H_
#define _ENCODER_FMT_H_

#include "media/audio_base.h"

enum change_file_step {
    SEAMLESS_OPEN_FILE,
    SEAMLESS_CHANGE_FILE,
};

struct encoder_fmt {
    u8 quality;
    u8 complexity;
    u8 sw_hw_option;
    u8 ch_num;
    u8 bit_width;
    u8 format;
    u16 frame_dms;
    u32 bit_rate;
    u32 sample_rate;
};

/*
 * 无缝录音配置, 支持配置录制多长时间(秒)后切换文件
 * advance_time: 提前多少秒调用change_file(priv, SEAMLESS_OPEN_FILE), 用于提前创建新文件
 * time: 单个文件录音时长(秒)
 */
struct seamless_recording {
    u8 advance_time;
    u16 time;
    void *priv;
    /*
     * 此回调函数在音频流任务中调用,不能执行耗时长的操作,否则可能导致音频播放卡顿
     */
    int (*change_file)(void *priv, enum change_file_step step);
};

#endif

