#ifndef _RING_PLAYER_H_
#define _RING_PLAYER_H_

int play_ring_file(const char *file_name);

int play_ring_file_alone(const char *file_name);

int play_ring_file_with_callback(const char *file_name, void *priv, tone_player_cb_t callback);

int play_ring_file_alone_with_callback(const char *file_name, void *priv, tone_player_cb_t callback);

bool ring_player_runing(void);

void ring_player_stop(void);

#endif

