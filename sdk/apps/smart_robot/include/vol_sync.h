#ifndef _VOL_SYNC_H_
#define _VOL_SYNC_H_

#include "generic/typedef.h"

void vol_sys_tab_init(void);

void bt_set_music_device_volume(int volume);

int bt_get_phone_device_vol(void);

void opid_play_vol_sync_fun(s16 *vol, u8 mode);

#endif
