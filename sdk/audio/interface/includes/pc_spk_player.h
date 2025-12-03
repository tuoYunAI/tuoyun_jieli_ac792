#ifndef _PC_SPK_PLAYER_H_
#define _PC_SPK_PLAYER_H_

#include "jlstream.h"
#include "effect/effects_default_param.h"

struct pc_spk_player {
    struct list_head entry;
    struct jlstream *stream;
    u8 open_flag;
    u8 usb_id;
    s8 pc_spk_pitch_mode;
};


struct pc_spk_player *pc_spk_player_open(struct stream_fmt *fmt);

void pc_spk_player_close(struct pc_spk_player *player);

bool pc_spk_player_runing(void);

bool pc_spk_player_mute_status(void);

int pc_spk_player_pitch_up(struct pc_spk_player *player);

int pc_spk_player_pitch_down(struct pc_spk_player *player);

int pc_spk_player_set_pitch(struct pc_spk_player *player, pitch_level_t pitch_mode);

#endif


