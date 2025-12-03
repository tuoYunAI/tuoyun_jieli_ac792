#ifndef __USB_HOST_MIC_RECORDER__
#define __USB_HOST_MIC_RECORDER__

#include "jlstream.h"
#include "asm/usb.h"

struct usb_host_mic_recorder {
    struct list_head entry;
    struct jlstream *stream;
    u8 usb_id;
};

struct usb_host_mic_recorder *usb_host_mic_recorder_open(struct stream_fmt *fmt);

void usb_host_mic_recorder_close(struct usb_host_mic_recorder *recorder);

int usb_host_mic_set_volume_by_taskq(u32 mic_vol);

bool usb_host_mic_recorder_runing(void);

void usb_host_pcm_mic_recorder_dump(struct usb_host_mic_recorder *recorder, int force_dump);


void set_usb_host_mic_recorder_tx_handler(const usb_dev usb_id, void *priv, void (*tx_handler)(void *, void *, int));
void usb_host_mic_recorder_get_buf(const usb_dev usb_id, void *buf, u32 len);

#endif


