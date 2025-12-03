#ifndef _DISTORTIONEXP_H_
#define _DISTORTIONEXP_H_

#include "AudioEffect_DataType.h"

enum {
    DISTORT_K0 = 0,
    DISTORT_K0_ENHANCE
};

typedef  struct  _DISTRIONEXP_PARM_ {
    int wetgain;               //-300-300
    int drygain;               //0-100
    int distortBlend;           // 0 到100
    int distortDrive;           // 0dB  -  100dB
    int reserved0;
    int reserved1;
    af_DataType dataTypeobj;
} DISTRIONEXP_PARM;

typedef struct _DISTORTION_FUNC_API_ {
    u32(*need_buf)(DISTRIONEXP_PARM *vc_parm);
    int (*open)(void *ptr, u32 sr, u32 nch, DISTRIONEXP_PARM *vc_parm);        //中途改变参数，可以调init
    void (*run)(void *ptr, short *indata, short *outdata, int PointsPerChannel);    //len是 每声道多少点
    void (*init)(void *ptr, DISTRIONEXP_PARM *vc_parm);        //中途改变参数，可以调init
} DISTORTION_FUNC_API;


extern DISTORTION_FUNC_API *get_distortEXP_ops();

#endif

