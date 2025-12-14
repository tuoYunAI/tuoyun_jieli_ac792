#ifndef _VIRTUAL_PLAYER_H_
#define _VIRTUAL_PLAYER_H_


typedef int (*player_event_callback_fn)( enum stream_event);

void tuoyun_audio_player_start(player_event_callback_fn callback);

void tuoyun_audio_player_clear(void);

int tuoyun_audio_write_data(void *buf, int len);

/* 获取音乐播放器状态  */
int tuoyun_player_get_status();

void tuoyun_audio_player_set_volume(s16 volume);

int tuoyun_audio_player_get_volume();
#endif

