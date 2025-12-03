/*************************************************************************************************/
/*!
*  \file      le_audio_user.h
*
*  \brief
*
*  Copyright (c) 2011-2024 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _LE_AUDIO_USER_H_
#define _LE_AUDIO_USER_H_
#include "typedef.h"

struct le_audio_format {
    //TODO
    u8  service_type;
    u8  channel;
    u16 frame_duration;
    u32 bit_rate;
    u32 coding_type;
    u32 sample_rate;
    u32 sdu_interval;
    u32 iso_interval_us;
    u32 flush_timeout;
};

struct le_audio_connection {
    u8 group;
    u8 channel;
    u16 handle;
    struct le_audio_connection *next;
    //TODO
};

struct le_audio_tx_ops {
    const char *name;

    void *(*open)(u8 group, u8 handle);

    int (*send)(void *conn, u8 index, void *data, int len, u32 timestamp);

    int (*get_format)(void *conn, struct le_audio_format *fmt);

    void (*set_tick_callback)(void *conn, void *priv, void (*callback)(void *, u32, int));

    void (*close)(void *);
};

#define REGISTER_LE_AUDIO_TX_OPS(name) \
    const struct le_audio_tx_ops name sec(.le_audio_tx_ops)


const struct le_audio_tx_ops *le_audio_get_tx_ops(const char *name);

#endif
