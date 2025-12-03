#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb_host_spk_player.data.bss")
#pragma data_seg(".usb_host_spk_player.data")
#pragma const_seg(".usb_host_spk_player.text.const")
#pragma code_seg(".usb_host_spk_player.text")
#endif
#include "system/timer.h"
#include "system/includes.h"
#include "usb_host_spk_player.h"
#include "effects/audio_pitchspeed.h"
#include "app_core.h"
#include "audio_config.h"
#include "scene_switch.h"
#include "app_config.h"
#include "audio_cvp.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif
#include "host/audio.h"

#if (TCFG_HOST_AUDIO_ENABLE)

#define LOG_TAG             "[USB_HOST_SPK_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

static struct list_head head = LIST_HEAD_INIT(head);

extern void dac_try_power_on_task_delete();

static void usb_host_spk_player_callback(void *private_data, int event)
{
    struct usb_host_spk_player *player = (struct usb_host_spk_player *)private_data;
    struct usb_host_spk_player *p, *n;

    switch (event) {
    case STREAM_EVENT_START:
        list_for_each_entry_safe(p, n, &head, entry) {
            if (p == player) {
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && TCFG_IIS_NODE_ENABLE
                //先开pc mic，后开spk，需要取消忽略外部数据，重启aec
                if (audio_aec_status()) {
                    audio_aec_reboot(0);
                    audio_cvp_ref_data_align_reset();
                }
#endif
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
        break;
    }
}

struct usb_host_spk_player *usb_host_spk_player_open(struct stream_fmt *fmt)
{
    struct usb_host_spk_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"usb_host_spk");
    if (uuid == 0) {
        return NULL;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    player->usb_host_spk_pitch_mode = PITCH_0;
    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_USB_HOST_SPK);

    if (!player->stream) {
        goto __exit0;
    }

    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 192);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_FMT, (int)fmt);
    jlstream_set_callback(player->stream, player, usb_host_spk_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_PC_SPK);

    list_add_tail(&player->entry, &head);

    int err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    } else {
        dac_try_power_on_task_delete();
    }

    player->usb_id = fmt->chconfig_id;
    player->open_flag = 1;

    return player;

__exit1:
    list_del(&player->entry);
    jlstream_release(player->stream);
__exit0:
    free(player);

    return NULL;
}

static void *usb_host_rx_priv[USB_MAX_HW_NUM];
static void (*usb_host_rx_handler[USB_MAX_HW_NUM])(void *, void *, int);

void set_usb_host_speaker_rx_handler(const usb_dev usb_id, void *priv, void (*rx_handler)(void *, void *, int))
{
    usb_host_rx_priv[usb_id] = priv;
    usb_host_rx_handler[usb_id] = rx_handler;
}

void usb_host_spk_player_put_buf(const usb_dev usb_id, void *buf, u32 len)
{
    if (usb_host_rx_handler[usb_id]) {
        usb_host_rx_handler[usb_id](usb_host_rx_priv[usb_id], buf, len);
    }
}

// 返回1说明player 在运行
bool usb_host_spk_player_runing(void)
{
    if (list_empty(&head)) {
        return FALSE;
    }

    return 1;
}

void usb_host_spk_player_close(struct usb_host_spk_player *player)
{
    if (!player) {
        return;
    }

    player->open_flag = 0;

#if 0//pc + bt 通过mixer叠加的环境， 因usbrx已经停止，无法驱动数据流。需手动提前将当前的mixer ch关闭
    struct mixer_ch_pause pause = {0};
    pause.ch_idx = 7;
    jlstream_set_node_param(NODE_UUID_MIXER, "MIXER27", &pause, sizeof(pause));
#endif
    list_del(&player->entry);

    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);
    free(player);

#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && TCFG_IIS_NODE_ENABLE
    if (audio_aec_status()) {
        //忽略参考数据
        audio_cvp_ioctl(CVP_OUTWAY_REF_IGNORE, 1, NULL);
        audio_cvp_ref_data_align_reset();
    }
#endif
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"pc_spk");
}

//变调接口
int usb_host_spk_player_pitch_up(struct usb_host_spk_player *player)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (!player) {
        return -1;
    }

    if (++player->usb_host_spk_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        player->usb_host_spk_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }

    log_info("play pitch up+++%d", player->usb_host_spk_pitch_mode);

    int err = usb_host_spk_player_set_pitch(player, player->usb_host_spk_pitch_mode);
    return !err ? player->usb_host_spk_pitch_mode : -1;
}

int usb_host_spk_player_pitch_down(struct usb_host_spk_player *player)
{
    if (!player) {
        return -1;
    }

    if (--player->usb_host_spk_pitch_mode < 0) {
        player->usb_host_spk_pitch_mode = 0;
    }

    log_info("play pitch down---%d", player->usb_host_spk_pitch_mode);

    int err = usb_host_spk_player_set_pitch(player, player->usb_host_spk_pitch_mode);
    return !err ? player->usb_host_spk_pitch_mode : -1;
}

int usb_host_spk_player_set_pitch(struct usb_host_spk_player *player, pitch_level_t pitch_mode)
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
        player->usb_host_spk_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

#endif
