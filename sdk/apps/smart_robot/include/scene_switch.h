#ifndef _SCENE_SWITCH_H_
#define _SCENE_SWITCH_H_

#include "generic/typedef.h"

/* 音乐模式：获取场景序号 */
u8 get_current_scene(void);

/* 音乐模式：设置默认场景序号 */
void set_default_scene(u8 index);

/* 音乐模式：获取EQ配置序号 */
u8 get_music_eq_preset_index(void);

void set_music_eq_preset_index(u8 index);

/* 音乐模式：根据参数组序号进行场景切换 */
void effect_scene_set(u8 scene);

/* 音乐模式：根据参数组个数顺序切换场景 */
void effect_scene_switch(void);

/* mic混响：获取场景序号 */
u8 get_mic_current_scene(void);

/* mic混响：根据参数组序号进行场景切换 */
void mic_effect_scene_set(u8 scene);

/* mic混响：根据参数组个数顺序切换场景 */
void mic_effect_scene_switch(void);

void music_vocal_remover_switch(void);

void musci_vocal_remover_update_parm(void);

u8 get_music_vocal_remover_status(void);

#endif

