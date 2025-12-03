#ifndef _A2DP_PLAYER_H_
#define _A2DP_PLAYER_H_

#include "effect/effects_default_param.h"

int a2dp_player_open(u8 *btaddr);

void a2dp_player_close(u8 *btaddr);

bool a2dp_player_get_btaddr(u8 *btaddr);

int a2dp_player_runing(void);

bool a2dp_player_is_playing(u8 *bt_addr);

void a2dp_player_tws_event_handler(int *msg);

void a2dp_play_close(u8 *bt_addr);

void a2dp_player_low_latency_enable(u8 enable);

void a2dp_file_low_latency_enable(u8 enable);

int a2dp_file_pitch_up(void);

int a2dp_file_pitch_down(void);

int a2dp_file_set_pitch(pitch_level_t pitch_mode);

void a2dp_file_pitch_mode_init(pitch_level_t pitch_mode);

void a2dp_player_reset(void);

void a2dp_player_breaker_mode(u8 mode,
                              u16 uuid_a, const char *name_a,
                              u16 uuid_b, const char *name_b);

u8 a2dp_file_get_low_latency_status(void);

#endif
