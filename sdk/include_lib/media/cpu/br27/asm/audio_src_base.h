/*****************************************************************
>file name : audio_src_base.h
>create time : Wed 02 Mar 2022 11:12:07 AM CST
*****************************************************************/
#ifndef _AUDIO_SRC_BASE_H_
#define _AUDIO_SRC_BASE_H_
#include "asm/audio_src.h"
#include "generic/circular_buf.h"

#define AUDIO_SRC_HW_VERSION_2      0x0
#define AUDIO_SRC_HW_VERSION_3      0x1



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

u8 audio_src_base_get_hw_core_id(void *resample);

int audio_src_base_filter_frames(void *resample);

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

/*
struct audio_src_buffer {
    void *buf;
    int len;
};
*/

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

struct audio_src_driver {
    void *(*open)(struct audio_src_base_params *params);
    int (*ioctl)(void *private_data, u32 cmd, void *arg);
    int (*write)(void *private_data, void *data, int len);
    void (*close)(void *private_data);
};

#define AUDIO_SRC_IOCTL_SET_OUTPUT          _IOW('S', 0, sizeof(struct audio_src_output))
#define AUDIO_SRC_IOCTL_SET_CHANNEL         _IOW('S', 1, sizeof(u8))
#define AUDIO_SRC_IOCTL_SET_INPUT_BUF       _IOW('S', 2, sizeof(struct audio_src_buffer))
#define AUDIO_SRC_IOCTL_SET_OUTPUT_BUF      _IOW('S', 3, sizeof(struct audio_src_buffer))
#define AUDIO_SRC_IOCTL_SET_SAMPLE_RATE     _IOW('S', 4, sizeof(struct audio_src_sample_rate))
#define AUDIO_SRC_IOCTL_SET_IRQ_CALLBACK    _IOW('S', 5, sizeof(struct audio_src_irq_callback))
#define AUDIO_SRC_IOCTL_SET_SILENCE         _IOW('S', 6, sizeof(struct audio_src_silence_output))
#define AUDIO_SRC_IOCTL_RUN_ONCE_SCALE      _IOW('S', 7, sizeof(struct audio_src_once_scale))
#define AUDIO_SRC_IOCTL_PUSH_DATA_OUTPUT    _IOW('S', 8, sizeof(int))
#define AUDIO_SRC_IOCTL_STOP                _IOW('S', 9, sizeof(int))
#define AUDIO_SRC_IOCTL_GET_FLOAT_POSITION  _IOR('S', 10, sizeof(float))
#define AUDIO_SRC_IOCTL_GET_INPUT_FRAMES    _IOR('S', 11, sizeof(int))
#define AUDIO_SRC_IOCTL_GET_OUTPUT_FRAMES   _IOR('S', 12, sizeof(u32))
#define AUDIO_SRC_IOCTL_GET_BUFFERED_FRAMES _IOR('S', 13, sizeof(int))
#define AUDIO_SRC_IOCTL_GET_HW_CORE_ID      _IOR('S', 14, sizeof(int))
#define AUDIO_SRC_IOCTL_FRAME_RESAMPLE      _IOW('S', 15, sizeof(int))
#define AUDIO_SRC_IOCTL_GET_FILTER_FRAMES   _IOW('S', 16, sizeof(int))
#define AUDIO_SRC_IOCTL_GET_PHASE           _IOR('S', 17, sizeof(int))

#define RESAMPLE_STATE_IDLE                 0
#define RESAMPLE_STATE_PEND_IRQ             1
#define RESAMPLE_STATE_IRQ_RISE             2

#define NORMAL_TYPE_ENABLE          BIT(1)
#define LOCK_TYPE_ENABLE            BIT(2)
#define ALL_TYPE_ENABLE             0xff

#define INSIDE_BUFFER_MODE              0
#define EXTERN_BUFFER_MODE              1

#define INPUT_ORIGNAL_SOUND             0
#define INPUT_FADEOUT_TO_SILENCE        1
#define INPUT_SILENCE_SOUND             2
#define INPUT_FADEIN_FROM_SILENCE       3

#define DIGITAL_OdB_VOLUME              16384

#endif

