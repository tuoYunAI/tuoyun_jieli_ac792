#ifndef _AUDIO_PINGPONG_PCM_DELAY_API_H_
#define _AUDIO_PINGPONG_PCM_DELAY_API_H_

#include "effects/Effect_pingpong_api.h"

struct pingpong_pcm_delay_update_parm {
    int maxdelay_ms;       // 0到300
    int justmodDelay;        // 0或者1
    int pitchVb;             //0到100
    int feedback;             //0到100
    int lowCut;               //10到20000
    int highCut;             //10到20000
    int wetgain;             //0到100
    int drygain;             //0到100
    int reserved;
};

typedef  struct _pingpong_pcm_delay_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct pingpong_pcm_delay_update_parm parm;
} pingpong_pcm_delay_param_tool_set;

struct pingpong_pcm_delay_open_parm {
    pingpong_parm_context param;
    u32 sample_rate;
    u8 ch_num;
    u8 bypass;
};

typedef struct _pingpong_pcm_delay_hdl {
    EFFECT_PINGPONG_FUNC_API *ops;
    void *workbuf;
    struct pingpong_pcm_delay_open_parm parm;
    int cur_max_delay;
    u8 update;
    u8 status;
} pingpong_pcm_delay_hdl;

/*
 * 打开
 */
pingpong_pcm_delay_hdl *audio_pingpong_pcm_delay_open(struct pingpong_pcm_delay_open_parm *param);
/*
 *  关闭
 */
void audio_pingpong_pcm_delay_close(pingpong_pcm_delay_hdl *hdl);
/*
 *  参数更新
 */
void audio_pingpong_pcm_delay_update_parm(pingpong_pcm_delay_hdl *hdl, struct pingpong_pcm_delay_update_parm *parm);
/*
 *  数据处理
 */
int audio_pingpong_pcm_delay_run(pingpong_pcm_delay_hdl *hdl, s16 *indata, s16 *outdata, int len);
/*
 *  暂停处理
 */
void audio_pingpong_pcm_delay_bypass(pingpong_pcm_delay_hdl *hdl, u8 bypass);
#endif

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

