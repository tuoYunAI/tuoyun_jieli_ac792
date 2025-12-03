#ifndef __TUOYUN_VOICE_RECORDER_H__
#define __TUOYUN_VOICE_RECORDER_H__

#include "generic/typedef.h"
#include "jlstream.h"

typedef struct tuoyun_voice_param {
    u8 ai_type;
    u8 quality;
    u8 complexity;
    u8 format_mode;
    u32 frame_ms;
    int sample_rate;
    void *priv;
    int code_type;
    int (*output)(void *priv, void *data, u32 len);
    void (*event_callback)(void *priv, int event, int value);
}tuoyun_voice_param_t, *tuoyun_voice_param_ptr;


void tuoyun_voice_recorder_open(struct tuoyun_voice_param *param);
int tuoyun_voice_recorder_close();

#endif // __TUOYUN_VOICE_RECORDER_H__