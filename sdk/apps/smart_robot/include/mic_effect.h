#ifndef _AUDIO_MIC_EFFECT_H_
#define _AUDIO_MIC_EFFECT_H_

bool mic_effect_player_runing(void);
int mic_effect_player_open(void);
void mic_effect_player_close(void);
void mic_effect_player_pause(u8 mark);
void mic_effect_set_dvol(u8 vol);
void mic_effect_dvol_up(void);
void mic_effect_dvol_down(void);
void mic_effect_set_irq_point_unit(u16 point_unit);

#endif
