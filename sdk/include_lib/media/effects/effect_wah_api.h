#ifndef _EFFECT_WAH_API_H_
#define _EFFECT_WAH_API_H_

#include "AudioEffect_DataType.h"

enum {
    AWAH_TYPE0 = 0,
    AWAH_TYPE1
};

typedef struct _wah_parm_context_ {
    int wahType;         //从下拉框选，AWAH_TYPE0,AWAH_TYPE1
    int moderate;         //0到15
    int freq_min;        //10到20000
    int freq_max;         //10到20000
    int wet;             //0到200
    int dry;             //0到100
    int reserved;        //保留位
    af_DataType dataTypeobj;
} wah_parm_context;

typedef struct __EFFECT_WAH_FUNC_API_ {
    unsigned int (*need_buf)(wah_parm_context *parm);
    unsigned int (*open)(unsigned int *ptr, int sr, int nch, wah_parm_context *parm);
    unsigned int (*init)(unsigned int *ptr, wah_parm_context *parm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);            // len是多少对点
} EFFECT_WAH_FUNC_API;


extern EFFECT_WAH_FUNC_API *get_autowah_func_api();

#endif
