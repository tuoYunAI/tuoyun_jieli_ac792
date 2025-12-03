#include "jlstream.h"
#include "classic/tws_api.h"
#include "media/audio_base.h"
#include "media/bt_audio_timestamp.h"
#include "scene_switch.h"
#include "volume_node.h"
#include "local_tws_player.h"
#include "tws/jl_tws.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif
#include "app_config.h"


#if TCFG_LOCAL_TWS_ENABLE

#define LOG_TAG     		"[LOCAL-TWS-PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"

struct local_tws_player {
    struct jlstream *stream;
};

static void local_tws_player_callback(void *private_data, int event)
{
    struct local_tws_player *player = (struct local_tws_player *)private_data;

    log_info("local tws player callback : %d", event);

    switch (event) {
    case STREAM_EVENT_START:
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
        musci_vocal_remover_update_parm();
#endif
#if AUDIO_VBASS_LINK_VOLUME
        vbass_link_volume();
#endif
#if AUDIO_EQ_LINK_VOLUME
        eq_link_volume();
#endif
        break;
    }
}

void *local_tws_player_open(struct local_tws_player_param *param)
{
    int uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"local_tws");
    if (!uuid) {
        return NULL;
    }

    struct local_tws_player *player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_LOCAL_TWS_SINK);
    if (!player->stream) {
        log_error("create local_tws stream faild");
        free(player);
        return NULL;
    }
    jlstream_set_scene(player->stream, STREAM_SCENE_LOCAL_TWS);

    jlstream_set_callback(player->stream, player, local_tws_player_callback);

    int channel = AUDIO_CH_LR; //这里设置解码输出的声道类型

    if (param) {
#if 0
        printf("tws_ch:%d", param->tws_channel);
        printf("channel_mode:%d", param->channel_mode);
        printf("sr:%d", param->sample_rate);
        printf("coding_type:%d", param->coding_type);
        printf("durations:%d", param->durations);
        printf("bit_rate:%d", param->bit_rate);
#endif
        jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                            NODE_IOC_SET_BTADDR, (int)param->tws_channel);
        jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                            NODE_IOC_SET_FMT, (int)param);

        channel = param->channel_mode == JL_TWS_CH_MONO ? AUDIO_CH_MIX : AUDIO_CH_LR;
    }

    jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, channel);

    int err = jlstream_start(player->stream);
    if (err) {
        jlstream_release(player->stream);
        free(player);
        return NULL;
    }

    log_info("local tws player open success : 0x%x, 0x%x", (u32)player, (u32)player->stream);

    return player;
}

void local_tws_player_close(void *priv)
{
    struct local_tws_player *player = (struct local_tws_player *)priv;

    if (!player) {
        return;
    }

    log_info("local tws player close :  0x%x, 0x%x", (u32)player, (u32)player->stream);

    if (player->stream) {
        jlstream_stop(player->stream, 0);
        jlstream_release(player->stream);
    }
    free(player);
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"local_tws");
}

#endif
