#ifndef _AUDIO_SRC_BASE_H_
#define _AUDIO_SRC_BASE_H_

#include "asm/audio_src.h"
#include "generic/circular_buf.h"

struct audio_src_base_params {
    u8 version;
    u8 channel;
    u8 bit_wide;
    u8 type;
    int in_sample_rate;
    int out_sample_rate;
};

struct resample_frame {
    u8 nch;
    int offset;
    int len;
    int size;
    void *data;
};

void *audio_src_base_open(u8 channel, int in_sample_rate, int out_sample_rate, u8 type);

void *audio_src_base_version_open(struct audio_src_base_params *params);

int audio_src_base_set_output_handler(void *resample,
                                      void *priv,
                                      int (*handler)(void *priv, void *data, int len));

int audio_src_base_set_channel(void *resample, u8 channel);

int audio_src_base_set_in_buffer(void *resample, void *buf, int len);

int audio_src_base_set_input_buff(void *resample, void *buf, int len);

int audio_src_base_resample_config(void *resample, int in_rate, int out_rate);

int audio_src_base_write(void *resample, void *data, int len);

int audio_src_base_stop(void *resample);

int audio_src_base_run_scale(void *resample);

int audio_src_base_input_frames(void *resample);

u32 audio_src_base_out_frames(void *resample);

float audio_src_base_position(void *resample);

int audio_src_base_scale_output(void *resample, int in_sample_rate, int out_sample_rate, int frames);

int audio_src_base_bufferd_frames(void *resample);

int audio_src_base_set_silence(void *resample, u8 silence, int fade_time);

int audio_src_base_wait_irq_callback(void *resample, void *priv, void (*callback)(void *));

void audio_src_base_close(void *resample);

int audio_src_base_push_data_out(void *resample);

int audio_src_base_frame_resample(void *resample, struct resample_frame *in_frame, struct resample_frame *out_frame);

int audio_src_base_get_phase(void *resample);

int audio_src_base_filter_frames(void *resample);

u8 audio_src_base_get_hw_core_id(void *resample);

struct audio_src_hw_mapping {
    u8 used_num;
    u8 id;
};

struct audio_src_queue {
    cbuffer_t cbuf;
    u8 buffer[0];
};

struct audio_src_output {
    void *priv;
    int (*handler)(void *priv, void *data, int len);
};

struct audio_src_sample_rate {
    int in_sample_rate;
    int out_sample_rate;
};

struct audio_src_once_scale {
    int in_sample_rate;
    int out_sample_rate;
    int frames;
};

struct audio_src_silence_output {
    u8 enable;
    int fade_time;
};

struct audio_src_irq_callback {
    void *priv;
    void (*callback)(void *);
};

#define RESAMPLE_STATE_IDLE                 0
#define RESAMPLE_STATE_PEND_IRQ             1
#define RESAMPLE_STATE_IRQ_RISE             2

#define INPUT_ORIGNAL_SOUND             0
#define INPUT_FADEOUT_TO_SILENCE        1
#define INPUT_SILENCE_SOUND             2
#define INPUT_FADEIN_FROM_SILENCE       3

#define DIGITAL_OdB_VOLUME              16384

#endif

