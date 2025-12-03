#ifndef  _EFFECT_MtapsECHO_API_H_
#define  _EFFECT_MtapsECHO_API_H_

#include "AudioEffect_DataType.h"

#define  MEcho_MAXTAPS    3

typedef struct  delay_gain {
    int delay;
    int gain;
} delay_gain_context;

typedef struct _muEchoMparm_chanel_context_ {
    struct  delay_gain gain_par[MEcho_MAXTAPS];
    int predelay;
} muEchoMparm_chanel_context;

typedef struct _mTapsEcho_Mparm_context_ {
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
    af_DataType dataTypeobj;
} mTapsEcho_Mparm_context;

typedef struct __EFFECT_StereoMEcho_FUNC_API_ {
    unsigned int (*need_buf)(int sr, int nch, mTapsEcho_Mparm_context *parm);
    unsigned int (*open)(unsigned int *ptr, int sr, int nch, mTapsEcho_Mparm_context *parm);
    unsigned int (*init)(unsigned int *ptr, mTapsEcho_Mparm_context *parm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);             // len是多少对点
} EFFECT_StereoMEcho_FUNC_API;


extern EFFECT_StereoMEcho_FUNC_API *get_StereoMEcho_func_api();
extern EFFECT_StereoMEcho_FUNC_API *get_StereoMEcho_fir_func_api();
extern EFFECT_StereoMEcho_FUNC_API *get_StereoMEcho24_16_func_api();
extern EFFECT_StereoMEcho_FUNC_API *get_StereoMEcho24_16_fir_func_api();

#endif
