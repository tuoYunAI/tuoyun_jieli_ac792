#ifndef _AUDIO_REVERB_LITE_H_
#define _AUDIO_REVERB_LITE_H_

#include "reverb_lite_api.h"

struct reverb_lite_update_parm {
    int wet;                      //0-300%
    int dry;                      //0-200%
    int pre_delay;                 //0-40ms
    int highcutoff;                //0-20k 高频截止
    int diffusion;                  //0-100%
    int decayfactor;                //0-100%
    int highfrequencydamping;       //0-100%
    int modulate;                  // 0或1
    int roomsize;                   //20%-100%
    int bufcoef;
    int buf_tradeoff;
};

struct reverb_lite_param_tool_set {
    int is_bypass;
    struct reverb_lite_update_parm parm;
};

struct audio_reverb_lite_parm {
    Plate_reverb_parm plate_parm;
    Plate_reverb_vch_parm vch_parm;
    EF_REVERB0_FIX_PARM fixparm;
    u8 bypass;
};

struct audio_reverb_lite {
    void *workbuf;
    PLATE_REVERB0_VCH_FUNC_API *ops;
    struct audio_reverb_lite_parm parm;
    u8 status;
    u8 update;
};

struct audio_reverb_lite *audio_reverb_lite_open(struct audio_reverb_lite_parm *parm);

int audio_reverb_lite_run(struct audio_reverb_lite *hdl, void *in, void *out, int len);

void audio_reverb_lite_close(struct audio_reverb_lite *hdl);

void audio_reverb_lite_update_parm(struct audio_reverb_lite *hdl, struct reverb_lite_update_parm *parm);

void audio_reverb_lite_bypass(struct audio_reverb_lite *hdl, u8 bypass);

#endif
