#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".linein_player.data.bss")
#pragma data_seg(".linein_player.data")
#pragma const_seg(".linein_player.text.const")
#pragma code_seg(".linein_player.text")
#endif
#include "linein_player.h"
#include "app_config.h"
#include "effects/audio_pitchspeed.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#include "scene_switch.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif

#define LOG_TAG             "[LINEIN_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


static void linein_player_callback(void *private_data, int event)
{
    struct jlstream *stream = (struct jlstream *)private_data;

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

struct linein_player *linein_player_open(void)
{
    int err;
    struct linein_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"linein");
    if (uuid == 0) {
        return NULL;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return NULL;
    }
    player->linein_pitch_mode = PITCH_0;

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_LINEIN);
    if (!player->stream) {
        player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS0_RX);
    }

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

#if TCFG_USER_EMITTER_ENABLE
    extern u8 *get_cur_connect_emitter_mac_addr(void);
    u8 *bt_addr = get_cur_connect_emitter_mac_addr();
    if (bt_addr) {
        jlstream_node_ioctl(player->stream, NODE_UUID_A2DP_TX, NODE_IOC_SET_BTADDR, (int)bt_addr);
    }
#endif

    //设置中断点数
#ifdef AUDIO_LINEIN_IRQ_POINTS
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_LINEIN_IRQ_POINTS);
#else
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);
#endif
    jlstream_set_callback(player->stream, player->stream, linein_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_LINEIN);

#if TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE
    //解码帧长短得情况下，使用三线程推数
    jlstream_add_thread(player->stream, "media0");
    jlstream_add_thread(player->stream, "media1");
#if defined(CONFIG_CPU_BR28)
    jlstream_add_thread(player->stream, "media2");
#endif
#endif

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    return player;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return NULL;
}

void linein_player_close(struct linein_player *player)
{
    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"linein");
}

//变调接口
int linein_file_pitch_up(struct linein_player *player)
{
    if (!player) {
        return -1;
    }
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (++player->linein_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        player->linein_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    log_info("play pitch up+++ %d", player->linein_pitch_mode);
    int ret = linein_file_set_pitch(player, player->linein_pitch_mode);
    return ret ? -1 : player->linein_pitch_mode;
}

int linein_file_pitch_down(struct linein_player *player)
{
    if (!player) {
        return -1;
    }

    if (--player->linein_pitch_mode < 0) {
        player->linein_pitch_mode = 0;
    }
    log_info("play pitch down--- %d", player->linein_pitch_mode);
    int ret = linein_file_set_pitch(player, player->linein_pitch_mode);
    return ret ? -1 : player->linein_pitch_mode;
}

int linein_file_set_pitch(struct linein_player *player, pitch_level_t pitch_mode)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (player) {
        player->linein_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

void linein_file_pitch_mode_init(struct linein_player *player, pitch_level_t pitch_mode)
{
    if (player) {
        player->linein_pitch_mode = pitch_mode;
    }
}
