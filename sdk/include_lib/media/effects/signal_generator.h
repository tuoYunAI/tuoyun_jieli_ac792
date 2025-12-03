#ifndef _SIGNALE_GENERATOR_H_
#define _SIGNALE_GENERATOR_H_

#include "media/audio_base.h"
#include "effects/effects_adj.h"

#define SLIENCE 0
#define SINE_WAVE 1
#define WHITE_NOISE 2

struct signal_generator_parm {
    int signal_type;//slicence 、sine wave、white noise
    u32 center_frequency;  //sine wave时有效
    float gain;            //sine wave、white noise 时有效
};

struct signal_generator_tool_set {
    int is_bypass;
    int process_type; //0:replace 1:mix
    struct signal_generator_parm parm;
};

struct signal_generator {
    struct signal_generator_tool_set cfg;
    float gain;            //sine wave、white noise 时有效
    u32 sample_rate;
    void *sine;
    int sine_len;
    float idx;
    u32 next;
    u32 seed;
    u8 ch_num;
    u8 bit_width;
};

#define COMFORT_NOISE_SEED 457
#ifndef DATA16
#define DATA16		32767
#endif
#ifndef DATA32
#define DATA32	    8388607
#endif

extern float eq_db2mag(float x);

extern void signal_generator_run(struct signal_generator *hdl, s16 *data, u32 data_len);

extern void signal_generator_uninit(struct signal_generator *hdl);

#endif
