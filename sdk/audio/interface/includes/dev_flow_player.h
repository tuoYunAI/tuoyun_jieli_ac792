#ifndef DEV_FLOW_PLAYER_H
#define DEV_FLOW_PLAYER_H



int dev_flow_player_open(u32 code_type, u8 ch_mode, u32 sr);

void dev_flow_player_close();

bool dev_flow_player_runing();



#endif
