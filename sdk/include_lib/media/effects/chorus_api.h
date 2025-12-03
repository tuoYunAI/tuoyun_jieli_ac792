#ifndef _CHOURS_API_H_
#define _CHOURS_API_H_

#include "AudioEffect_DataType.h"

typedef  struct  _CHORUS_PARM_ {
    int wetgain;               //0-100
    int drygain;
    int mod_depth;
    int mod_rate;
    int feedback;
    int delay_length;
    af_DataType dataTypeobj;
} CHORUS_PARM;

typedef struct _CHORUS_FUNC_API_ {
    u32(*need_buf)(CHORUS_PARM *vc_parm);
    int (*open)(void *ptr, u32 sr, u32 nch, CHORUS_PARM *vc_parm);       //中途改变参数，可以调init
    void (*run)(void *ptr, short *indata, short *outdata, int PointsPerChannel);    //len是 每声道多少点
    void (*init)(void *ptr, CHORUS_PARM *vc_parm);        //中途改变参数，可以调init
} CHORUS_FUNC_API;


extern CHORUS_FUNC_API *get_chorus_ops();

#endif

