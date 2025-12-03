#ifndef _AUDIO_THREE_D_API_H_
#define _AUDIO_THREE_D_API_H_

#include "effects/threeD_api.h"

typedef struct _ThreeDUpdateParam {//与threeD_parm_context关联
    int drygain;      // 0 to 100
    int wetgain;      // -100 to 200
    int wet_highpass_freq;
    int wet_lowpass_freq;
    int reserved;
} ThreeDUpdateParam;

typedef struct _threed_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    ThreeDUpdateParam parm;
} three_d_param_tool_set;

struct three_d_param {
    threeD_parm_context param;
    u32 samplerate;
    u32 ch_num;
    u8 bypass;
};

//立体声增强
struct audio_three_d {
    void *workbuf;           //three_d 运行句柄及buf
    ThreeD_FUNC_API *ops;
    struct three_d_param parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
};

int audio_three_d_run(struct audio_three_d *hdl, void *datain, void *dataout, u32 len);
struct audio_three_d *audio_three_d_open(struct three_d_param *parm);
int audio_three_d_close(struct audio_three_d *hdl);
int audio_three_d_update_parm(struct audio_three_d *hdl, ThreeDUpdateParam *parm);
void audio_three_d_bypass(struct audio_three_d *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

