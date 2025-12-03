#ifndef _AUDIO_STEREO_MTAPS_ECHO_H__
#define _AUDIO_STEREO_MTAPS_ECHO_H__

#include "Effect_MtapsEcho_api.h"

struct stereo_mtaps_echo_update_parm { /*与_mTapsEcho_Mparm_context_结构关联*/
    int maxpredelay_ms;
    int maxdelay_ms;
    int delay;                //0到100
    int feedback;             //0到100
    int wetgain;               //0到200
    int drygain;               //0到100
    int lowpass_freq;               //10到20000
    int highpass_freq;              //10到20000
    int commonbuf;                // used only when stereo
    int repeatTime;
    muEchoMparm_chanel_context echo_subparm[2];
    int reserve;
    /*固件算法选配 */
    u16 type;   // 0:iir or 1:fir
    u16 wet_bit_wide;//高级配置选配干声24bit湿声只有16bit
};

struct stereo_mtaps_echo_tool_set {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct stereo_mtaps_echo_update_parm parm;
};

struct audio_stereo_mtaps_echo_parm {
    mTapsEcho_Mparm_context parm;
    u16 type;
    u16 wet_bit_wide;
    u32 sample_rate;
    u8 ch_num;
    u8 bypass;
};

struct audio_stereo_mtaps_echo {
    void *workbuf;
    EFFECT_StereoMEcho_FUNC_API *ops;           //函数指针
    struct audio_stereo_mtaps_echo_parm parm;
    u8 status;
    u8 update;
};

/*
 * 打开模块
 */
struct audio_stereo_mtaps_echo *audio_stereo_mtaps_echo_open(struct audio_stereo_mtaps_echo_parm *parm);
/*
 * 处理
 */
int audio_stereo_mtaps_echo_run(struct audio_stereo_mtaps_echo *hdl, short *in, short *out, int len);
/*
 * 关闭模块
 */
void audio_stereo_mtaps_echo_close(struct audio_stereo_mtaps_echo *hdl);
/*
 *  参数更新
 */
void audio_stereo_mtaps_echo_update_parm(struct audio_stereo_mtaps_echo *hdl, struct stereo_mtaps_echo_tool_set *parm);

#endif
