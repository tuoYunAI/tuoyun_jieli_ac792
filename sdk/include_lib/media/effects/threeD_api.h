#ifndef _STEREO_WINDEN_ADV_API_H_
#define _STEREO_WINDEN_ADV_API_H_

#include "AudioEffect_DataType.h"

typedef struct _threeD_parm_context_ {
    int drygain;      // 0 to 100
    int wetgain;      // -100 to 200
    int wet_highpass_freq;
    int wet_lowpass_freq;
    int reserved;
    af_DataType dataTypeobj;
} threeD_parm_context;

typedef struct __ThreeD_FUNC_API_ {
    unsigned int (*need_buf)(threeD_parm_context *parm);
    unsigned int (*open)(unsigned int *ptr, int sr, int nch, threeD_parm_context *parm);
    unsigned int (*init)(unsigned int *ptr, threeD_parm_context *parm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);            // len是多少对点
} ThreeD_FUNC_API;


extern ThreeD_FUNC_API *get_threeD_func_api();

#endif
