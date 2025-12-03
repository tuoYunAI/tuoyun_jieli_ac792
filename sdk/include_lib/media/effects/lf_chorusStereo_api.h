#ifndef _LF_CHORUSSTEREO_API_H_
#define _LF_CHORUSSTEREO_API_H_

#include "AudioEffect_DataType.h"

typedef struct _chorusStereo_parm_ {
    unsigned int nch;
    unsigned int samplerate;
    int feedback;                //-100 to 100
    float mod_rate;              //0-10hz
    float delay;                 //1-50ms
    int mod_depth;               //1-100
    int wetgain;                 //-300-300
    int drygain;                 //0-100
    int LRDiff;                  //0-100
    int reserved0;
    int reserved1;
    af_DataType dataTypeobj;
} chorusStereo_parm;

typedef struct _CHORUS_STEREO_FUNC_API_ {
    u32(*need_buf)(void *vc_parm);
    u32(*tmp_buf_size)(void *vc_parm);
    int (*init)(void *ptr, void *vc_parm, void *tmpbuf);        //中途改变参数，可以调init
    int (*set_tmpbuf)(void *ptr, void *tmpbuf);        //中途改变参数，可以调init
    int (*run)(void *ptr, void *indata, void *outdata, int PointsPerChannel);    //len是 每声道多少点
    int (*update)(void *ptr, void *vc_parm);        //中途改变参数，可以调init
} CHORUS_STEREO_FUNC_API;

#ifdef __cplusplus
extern "C"
{
extern CHORUS_STEREO_FUNC_API *get_chorus_stereo_ops();
}
#else
extern CHORUS_STEREO_FUNC_API *get_chorus_stereo_ops();
#endif

#endif
