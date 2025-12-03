#ifndef _AVI_AUDIO_PLAYER_H_
#define _AVI_AUDIO_PLAYER_H_

int avi_audio_player_open(u32 code_type, u8 ch_mode, u32 sr, u8 * (*get_data_cb)(u32 *));

void avi_audio_player_close(void);

bool avi_audio_player_runing(void);

#endif
