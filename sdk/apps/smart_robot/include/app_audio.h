
#ifndef __AUDIO__
#define __AUDIO__

#include "os/os_api.h"
#include "app_protocol.h"

typedef struct{
    int event;
    void *arg;
}audio_event_t, *audio_event_prt;

/**
 * 对话开始
 * downlink_media_param是个临时变量, 需要自行保存参数
 */
int dialog_audio_init(media_parameter_ptr downlink_media_param);

int dialog_audio_close(void);


int dialog_proc_speak_status(device_working_status_t  status);

int dialog_audio_dec_frame_write(audio_stream_packet_ptr packet);

void tuoyun_asr_recorder_open();
int tuoyun_asr_recorder_close();

#endif