#ifndef _PC_MIC_RECORDER_
#define _PC_MIC_RECORDER_

#include "jlstream.h"

struct pc_mic_recorder {
    struct list_head entry;
    struct jlstream *stream;
    u8 usb_id;
};

struct pc_mic_recorder *pc_mic_recorder_open(struct stream_fmt *fmt);

void pc_mic_recorder_close(struct pc_mic_recorder *recorder);

int pc_mic_set_volume_by_taskq(u32 mic_vol);

bool pc_mic_recorder_runing(void);

void pcm_mic_recorder_dump(struct pc_mic_recorder *recorder, int force_dump);

#endif


