#ifndef _AUDIO_CHORUS_ADVANCE_API_H_
#define _AUDIO_CHORUS_ADVANCE_API_H_

#include "effects/lf_chorusStereo_api.h"

struct chorus_advance_udpate {
    int feedback;             //0-100
    float mod_rate;
    float delay;
    int mod_depth;               // 0-100
    int LRDiff;           //0-360
    int wetgain;
    int drygain;
    int reserved0;
    int reserved1;
};

struct chorus_advance_param_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct chorus_advance_udpate parm;
};

#endif

