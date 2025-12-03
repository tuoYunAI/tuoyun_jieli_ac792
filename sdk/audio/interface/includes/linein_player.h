#ifndef LINEIN_PLAYER_H
#define LINEIN_PLAYER_H

#include "effect/effects_default_param.h"
#include "jlstream.h"

struct linein_player {
    struct jlstream *stream;
    s8 linein_pitch_mode;
};

struct linein_player *linein_player_open(void);

void linein_player_close(struct linein_player *);

int linein_file_pitch_up(struct linein_player *);

int linein_file_pitch_down(struct linein_player *);

int linein_file_set_pitch(struct linein_player *, pitch_level_t pitch_mode);

void linein_file_pitch_mode_init(struct linein_player *, pitch_level_t pitch_mode);

#endif

