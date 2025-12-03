#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".esco_player.data.bss")
#pragma data_seg(".esco_player.data")
#pragma const_seg(".esco_player.text.const")
#pragma code_seg(".esco_player.text")
#endif
#include "jlstream.h"
#include "esco_player.h"
#include "app_config.h"

#if TCFG_USER_BT_CLASSIC_ENABLE && TCFG_BT_SUPPORT_PROFILE_HFP

struct esco_player {
    u8 bt_addr[6];
    struct jlstream *stream;
};

static struct esco_player *g_esco_player;
extern const int config_audio_dac_mix_enable;

static void esco_player_callback(void *private_data, int event)
{
    struct esco_player *player = g_esco_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    default:
        break;
    }
}

int esco_player_open(u8 *bt_addr)
{
    int err;
    struct esco_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"esco");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ESCO_RX);
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

#if TCFG_AUDIO_SIDETONE_ENABLE
    if (config_audio_dac_mix_enable) {
        set_aec_ref_dac_ch_name(SIDETONE_ESCO_DAC_NAME);
        aec_ref_dac_ch_data_read_init();
    }
#endif

#if TCFG_ESCO_DL_CVSD_SR_USE_16K
    jlstream_node_ioctl(player->stream, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SET_PRIV_FMT, TCFG_ESCO_DL_CVSD_SR_USE_16K);
#endif /*TCFG_ESCO_DL_CVSD_SR_USE_16K*/

    jlstream_set_callback(player->stream, player->stream, esco_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_ESCO);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_BTADDR, (int)bt_addr);

#if TCFG_NS_NODE_ENABLE
    jlstream_node_ioctl(player->stream, NODE_UUID_NOISE_SUPPRESSOR, NODE_IOC_SET_BTADDR, (int)bt_addr);
#endif

#if TCFG_DNS_NODE_ENABLE
    jlstream_node_ioctl(player->stream, NODE_UUID_DNS_NOISE_SUPPRESSOR, NODE_IOC_SET_BTADDR, (int)bt_addr);
#endif

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    memcpy(player->bt_addr, bt_addr, 6);
    g_esco_player = player;


    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool esco_player_runing(void)
{
    return g_esco_player != NULL;
}

bool esco_player_get_btaddr(u8 *btaddr)
{
    if (g_esco_player) {
        memcpy(btaddr, g_esco_player->bt_addr, 6);
        return true;
    }
    return false;
}

int esco_player_suspend(u8 *bt_addr)
{
    if (g_esco_player && (memcmp(g_esco_player->bt_addr, bt_addr, 6) == 0)) {
        jlstream_ioctl(g_esco_player->stream, NODE_IOC_SUSPEND, 0);
    }
    return 0;
}

int esco_player_start(u8 *bt_addr)
{
    if (g_esco_player && (memcmp(g_esco_player->bt_addr, bt_addr, 6) == 0)) {
        jlstream_ioctl(g_esco_player->stream, NODE_IOC_START, 0);
    }
    return 0;
}

/*
	ESCO状态检测，
	param:btaddr 目标蓝牙地址, 传NULL则只检查耳机状态
 */
bool esco_player_is_playing(u8 *btaddr)
{
    int cur_addr = 1;
    if (btaddr && g_esco_player) {	//当前蓝牙地址检测
        cur_addr = (!memcmp(btaddr, g_esco_player->bt_addr, 6));
    }
    if (g_esco_player && cur_addr) {
        return true;
    }
    return false;
}

void esco_player_close(void)
{
    struct esco_player *player = g_esco_player;

    if (!player) {
        return;
    }

    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);

    free(player);
    g_esco_player = NULL;

#if TCFG_AUDIO_SIDETONE_ENABLE
    if (config_audio_dac_mix_enable) {
        aec_ref_dac_ch_data_read_exit();
    }
#endif

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"esco");
}

#else

bool esco_player_is_playing(u8 *btaddr)
{
    return false;
}

bool esco_player_runing(void)
{
    return false;
}

#endif
