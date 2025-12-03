#ifndef _BREAKPOINT_H_
#define _BREAKPOINT_H_

#include "media/audio_decoder.h"
#include "music/music_player.h"

breakpoint_t *breakpoint_handle_creat(void);
void breakpoint_handle_destroy(breakpoint_t **bp);
bool breakpoint_vm_read(breakpoint_t *bp, const char *logo);
void breakpoint_vm_write(breakpoint_t *bp, const char *logo);

#endif//__BREAKPOINT_H__

