#ifndef _HOWLING_PITCHSHIFER_API_H_
#define _HOWLING_pitchshifer_api_h_

#include "AudioEffect_DataType.h"

/*#define  EFFECT_OLD_RECORD          0x01
#define  EFFECT_MOYIN               0x0*/
//#define  EFFECT_ROBORT_FLAG          0X04

enum {
    EFFECT_HOWLING_PS        = 0x01,              //1.5《=》12 ms
    EFFECT_HOWLING_HE       = 0x02,
    EFFECT_HOWLING_FS       = 0x04,
};

enum {
    PLATFORM_BR30 = 0,
    PLATFORM_BR36 = 1
};

typedef struct HOWLING_PITCHSHIFT_PARM_ {
    s16 ps_parm;
    s16 fe_parm;
    u32 effect_v;
    af_DataType dataTypeobj;
} HOWLING_PITCHSHIFT_PARM;

typedef struct _HOWLING_PITCHSHIFT_FUNC_API_ {
    u32(*need_buf)(int flag);
    int (*open)(void *ptr, u32 sr, HOWLING_PITCHSHIFT_PARM *pitchshift_obj);       //中途改变参数，可以调init
    void (*run)(void *ptr, short *indata, short *outdata, int len);   //len是多少个点
} HOWLING_PITCHSHIFT_FUNC_API;

extern HOWLING_PITCHSHIFT_FUNC_API *get_howling_ps_func_api();
extern HOWLING_PITCHSHIFT_FUNC_API *get_howling_ps_func_api_mask();

#endif

