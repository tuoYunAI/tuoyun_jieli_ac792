#ifndef _REVERB_LITE_API_H_
#define _REVERB_LITE_API_H_

#include "audio_reverb_common.h"

typedef struct _Plate_reverb_vch_parm_ {
    int bufcoef;
    int buf_tradeoff;
    int vnchout;
} Plate_reverb_vch_parm;

typedef struct __PLATE_REVERB_VCH_FUNC_API_ {
    unsigned int (*need_buf)(Plate_reverb_parm *preverb_parm, Plate_reverb_vch_parm *vch_obj);
    int (*open)(unsigned int *ptr, Plate_reverb_parm *preverb_parm, EF_REVERB0_FIX_PARM *echo_fix_parm, Plate_reverb_vch_parm *vch_obj);
    int (*init)(unsigned int *ptr, Plate_reverb_parm *preverb_parm);
    int (*run)(unsigned int *ptr, short *inbuf, short *outdata, int len);
} PLATE_REVERB0_VCH_FUNC_API;

extern PLATE_REVERB0_VCH_FUNC_API *get_plate_reverb_lite_vch_func_api();

#endif
