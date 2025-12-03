#ifndef _AUDIO_DISTORTION_API_H_
#define _AUDIO_DISTORTION_API_H_

#include "effects/distortionExp_api.h"

typedef struct _DistortionUpdateParam {//与DISTRIONEXP_PARM关联
    int wetgain;               //-300-300
    int drygain;               //0-100
    int distortBlend;           // 0 到100
    int distortDrive;           // 0dB  -  100dB
    int reserved0;
    int reserved1;
} DistortionUpdateParam;

typedef struct _distortion_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    DistortionUpdateParam parm;
} distortion_param_tool_set;

struct distortion_param {
    DISTRIONEXP_PARM param;
    u32 samplerate;
    u32 ch_num;
    u8 bypass;
};

//立体声增强
struct audio_distortion {
    void *workbuf;           //distortion 运行句柄及buf
    DISTORTION_FUNC_API *ops;
    struct distortion_param parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
};

int audio_distortion_run(struct audio_distortion *hdl, void *datain, void *dataout, u32 len);
struct audio_distortion *audio_distortion_open(struct distortion_param *parm);
int audio_distortion_close(struct audio_distortion *hdl);
int audio_distortion_update_parm(struct audio_distortion *hdl, DistortionUpdateParam *parm);
void audio_distortion_bypass(struct audio_distortion *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

