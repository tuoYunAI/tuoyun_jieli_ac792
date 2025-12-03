#ifndef _ESCO_RECODER_H_
#define _ESCO_RECODER_H_

#include "generic/typedef.h"

#define COMMON_SCO   	  0    //普通SCO
#define JL_DOGLE_ACL  	  1    //dongle   ACL链路


int esco_recoder_open(u8 link_type, void *bt_addr);

void esco_recoder_close(void);

int esco_recoder_switch(u8 en);

int esco_recoder_reset(void);

int audio_sidetone_open(void);

int audio_sidetone_close(void);

int get_audio_sidetone_state(void);


#endif
