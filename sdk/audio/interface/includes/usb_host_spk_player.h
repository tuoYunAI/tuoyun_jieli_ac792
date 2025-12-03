#ifndef __USB_HOST_SPK_PLAYER_H__
#define __USB_HOST_SPK_PLAYER_H__

#include "jlstream.h"
#include "effect/effects_default_param.h"
#include "asm/usb.h"

struct usb_host_spk_player {
    struct list_head entry;
    struct jlstream *stream;
    u8 open_flag;
    u8 usb_id;
    s8 usb_host_spk_pitch_mode;
};


struct usb_host_spk_player *pc_spk_player_open(struct stream_fmt *fmt);

void usb_host_spk_player_close(struct usb_host_spk_player *player);

bool usb_host_spk_player_runing(void);

bool usb_host_spk_player_mute_status(void);

int usb_host_spk_player_pitch_up(struct usb_host_spk_player *player);

int usb_host_spk_player_pitch_down(struct usb_host_spk_player *player);

int usb_host_spk_player_set_pitch(struct usb_host_spk_player *player, pitch_level_t pitch_mode);


void usb_host_spk_player_put_buf(const usb_dev usb_id, void *buf, u32 len);
void set_usb_host_speaker_rx_handler(const usb_dev usb_id, void *priv, void (*rx_handler)(void *, void *, int));

#endif


