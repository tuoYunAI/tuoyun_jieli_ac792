#ifndef _VIR_SOURCE_PLAYER_H_
#define _VIR_SOURCE_PLAYER_H_

#include "generic/typedef.h"
#include "jlstream.h"

struct vir_source_player {
    struct list_head entry;
    struct jlstream *stream;
    s8 music_speed_mode; //播放倍速
    u8 playing;
};

void *vir_source_player_open(void *file, struct stream_file_ops *ops);
int vir_source_player_pp(void *player_);
int vir_source_player_resume(void *player_);
int vir_source_player_pause(void *player_);
int vir_source_player_start(void *player_);
void vir_source_player_close(void *player_);

#endif //__VIR_SOURCE_PLAYER_H

