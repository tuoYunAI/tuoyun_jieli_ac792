#ifndef _ESCO_PLAYER_H_
#define _ESCO_PLAYER_H_

int esco_player_open(u8 *bt_addr);

void esco_player_close(void);

bool esco_player_runing(void);

bool esco_player_get_btaddr(u8 *btaddr);

bool esco_player_is_playing(u8 *btaddr);

int esco_player_start(u8 *bt_addr);

int esco_player_suspend(u8 *bt_addr);


#endif
