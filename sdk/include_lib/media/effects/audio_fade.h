#ifndef _AUDIO_FADE_H_
#define _AUDIO_FADE_H_

struct fade_update_parm {
    int type;
    int dump_time;
    int fade_time;
};

typedef struct _FadeParam_TOOL_SET {
    int is_bypass;
    struct fade_update_parm parm;
} fade_param_tool_set;

struct fade_open_parm {
    struct fade_update_parm parm;
    u32 samplerate;
    u32 channel_num;
    u8 bit_width;
};

typedef struct _fade_hdl {
    struct fade_open_parm fade_parm;
    u8 update;
    u8 status;
    u8 fade_en; //使能淡入标志位
    u32 dump_len; //开头丢弃的数据长度（byte），对应dump_time
    u32 fade_len; //需要淡入的数据长度（byte），对应fade_time
    u32 fade_points; //记录淡入总点数，该值不会做递减操作
    u32 fade_gain;
    u8 bit_width;
} fade_hdl;

//打开
fade_hdl *audio_fade_open(struct fade_open_parm *param);

//关闭
void audio_fade_close(fade_hdl *hdl);

//参数更新
void audio_fade_update_parm(fade_hdl *hdl, struct fade_update_parm *parm);

//数据处理
int audio_fade_run(fade_hdl *hdl, s16 *indata, s16 *outdata, int len);

//暂停处理
void audio_fade_bypass(fade_hdl *hdl, u8 bypass);

#endif
