#ifndef _EFFECT_PINGPONG_API_H_
#define _EFFECT_PINGPONG_API_H_

#include "AudioEffect_DataType.h"

typedef struct _pingpong_parm_context_ {
    int maxdelay_ms;       // 0到300
    int justmodDelay;        // 0或者1
    int pitchVb;             //0到100
    int feedback;             //0到100
    int lowCut;               //10到20000
    int highCut;             //10到20000
    int wetgain;             //0到100
    int drygain;             //0到100
    int reserved;
    af_DataType dataTypeobj;
} pingpong_parm_context;

typedef struct __EFFECT_PINGPONG_FUNC_API_ {
    unsigned int (*need_buf)(int sr, int nch, pingpong_parm_context *parm);
    unsigned int (*open)(unsigned int *ptr, int sr, int nch, pingpong_parm_context *parm);
    unsigned int (*init)(unsigned int *ptr, pingpong_parm_context *parm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);             // len是多少对点
} EFFECT_PINGPONG_FUNC_API;

extern EFFECT_PINGPONG_FUNC_API *get_pingpong_func_api();

#endif
