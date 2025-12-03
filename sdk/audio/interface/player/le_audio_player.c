#include "le_audio_player.h"
#include "classic/tws_api.h"
#include "media/audio_base.h"
#include "media/bt_audio_timestamp.h"
#include "scene_switch.h"
#include "volume_node.h"
#include "system/spinlock.h"
#include "system/timer.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif
#include "app_config.h"


#if TCFG_LE_AUDIO_STREAM_ENABLE

#define LOG_TAG     		"[LEAUDIO_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"

struct le_audio_player {
    struct jlstream *stream;
    void *le_audio;
    u16 timer;
    u8 player_id;
#if TCFG_KBOX_1T3_MODE_EN
    u8 *stream_addr;
    char name[16];
    u8 le_audio_num;
    u8 inused;
    s16 dvol;
    u8 dvol_index;
    u8 save_vol_cnt;
    u16 save_vol_timer;
#endif
};

static u8 g_player_id = 0;

static struct le_audio_player *g_le_audio_player = NULL;

#if TCFG_KBOX_1T3_MODE_EN

static struct le_audio_player g_le_audio_player_file[2] = {0};

static DEFINE_SPINLOCK(lock);

static const u8 vol_table[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

u8 le_audio_player_handle_is_have(u8 *addr)
{
    for (int i = 0; i < 2; i++) {
        if (g_le_audio_player_file[i].inused) {
            if (g_le_audio_player_file[i].stream_addr == addr) {
                return 1;
            }
        }
    }
    return 0;
}

void *le_audio_player_other_handle_is_busy(u8 *addr)
{
    for (int i = 0; i < 2; i++) {
        if (g_le_audio_player_file[i].inused) {
            if (g_le_audio_player_file[i].stream_addr ==  addr) {
                return &g_le_audio_player_file[i];
            }
        }
    }
    return NULL;
}

static void *le_audio_player_handle_open(u8 *addr)
{
    spin_lock(&lock);
    for (int i = 0; i < 2; i++) {
        if (g_le_audio_player_file[i].inused == 0) {
            g_le_audio_player_file[i].inused = 1;
            g_le_audio_player_file[i].le_audio_num = i;
            g_le_audio_player_file[i].stream_addr = addr;
            g_le_audio_player_file[i].player_id = g_player_id;
            spin_unlock(&lock);
            return &g_le_audio_player_file[i];
        }
    }
    spin_unlock(&lock);
    return NULL;
}

static void le_audio_player_handle_close(u8 *addr)
{
    for (int i = 0; i < 2; i++) {
        if (g_le_audio_player_file[i].inused) {
            if (g_le_audio_player_file[i].stream_addr == addr) {
                g_le_audio_player_file[i].le_audio_num = 0;
                g_le_audio_player_file[i].stream_addr = 0;
                g_le_audio_player_file[i].player_id = 0;
                g_le_audio_player_file[i].inused = 0;
            }
        }
    }
}

static void *get_le_audio_player_handle(u8 *addr)
{
    spin_lock(&lock);
    for (int i = 0; i < 2; i++) {
        if (g_le_audio_player_file[i].inused) {
            if (g_le_audio_player_file[i].stream_addr ==  addr) {
                spin_unlock(&lock);
                return &g_le_audio_player_file[i];
            }
        }
    }
    spin_unlock(&lock);
    return NULL;
}

void *get_cur_le_audio_player_handle(void)
{
    for (int i = 0; i < 2; i++) {
        if (g_le_audio_player_file[i].inused) {
            return &g_le_audio_player_file[i];
        }
    }
    return NULL;
}

static int le_audio_player_create(u8 *conn)
{
    int uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mic_effect");
    struct le_audio_player *player = get_le_audio_player_handle(conn);
    if (player) {
        if (player->stream) {
            log_error("le audio stream is exist");
            return -EEXIST;
        }
    } else {
        player = le_audio_player_handle_open(conn);
        if (!player) {
            return -ENOMEM;
        }
    }

    if (player->le_audio_num) {
        log_info("====== LE_Audio_Sink1 ======");
        strcpy(player->name, "LE_Audio_Sink1");
    } else {
        log_info("====== LE_Audio_Sink0 ======");
        strcpy(player->name, "LE_Audio_Sink0");
    }

    player->le_audio = conn;
    player->stream = jlstream_pipeline_parse_by_node_name(uuid, player->name);

    if (!player->stream) {
        log_error("create le audio  stream faild");
        return -EFAULT;
    }

    return 0;
}

static void le_audio_volume_save_do(void *priv)
{
    u8 le_audio_num = (u8)priv;

    if (++g_le_audio_player_file[le_audio_num].save_vol_cnt >= 5) {
        sys_timer_del(g_le_audio_player_file[le_audio_num].save_vol_timer);
        g_le_audio_player_file[le_audio_num].save_vol_timer = 0;
        g_le_audio_player_file[le_audio_num].save_vol_cnt = 0;
        log_info("save le audio vol:%d", g_le_audio_player_file[le_audio_num].dvol);
        syscfg_write(CFG_WIRELESS_MIC0_VOLUME + le_audio_num, &g_le_audio_player_file[le_audio_num].dvol, 2);
    }
}

static void le_audio_volume_change(u32 le_audio_num)
{
    g_le_audio_player_file[le_audio_num].save_vol_cnt = 0;

    if (g_le_audio_player_file[le_audio_num].save_vol_timer == 0) {
        g_le_audio_player_file[le_audio_num].save_vol_timer = sys_timer_add((void *)le_audio_num, le_audio_volume_save_do, 1000);//中断里不能操作vm 关中断不能操作vm
    }
}

int le_audio_set_dvol(u8 le_audio_num, u8 vol)
{
    const char *vol_name = le_audio_num ? "Vol_WMic1" : "Vol_WMic0";

    struct volume_cfg cfg = {0};
    cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
    cfg.cur_vol = vol;

    if (g_le_audio_player_file[le_audio_num].inused) {
        jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, vol_name, (void *)&cfg, sizeof(struct volume_cfg));

        log_info("le audo dvol name: %s, le audo dvol:%d", vol_name, vol);

        if (vol != g_le_audio_player_file[le_audio_num].dvol) {
            g_le_audio_player_file[le_audio_num].dvol = vol;
            le_audio_volume_change(le_audio_num);
        }
    }

    return 0;
}

s16 le_audio_get_dvol(u8 le_audio_num)
{
    if (g_le_audio_player_file[le_audio_num].inused) {
        return g_le_audio_player_file[le_audio_num].dvol;
    }
    return 0;
}

void le_audio_dvol_up(u8 le_audio_num)
{
    if (g_le_audio_player_file[le_audio_num].inused) {
        if (g_le_audio_player_file[le_audio_num].dvol_index < sizeof(vol_table) / sizeof(vol_table[0]) - 1) {
            g_le_audio_player_file[le_audio_num].dvol_index++;
            le_audio_set_dvol(le_audio_num, vol_table[g_le_audio_player_file[le_audio_num].dvol_index]);
        } else {
            log_info("le audio volum is max");
        }
    }
}

void le_audio_dvol_down(u8 le_audio_num)
{
    if (g_le_audio_player_file[le_audio_num].inused) {
        if (g_le_audio_player_file[le_audio_num].dvol_index) {
            g_le_audio_player_file[le_audio_num].dvol_index--;
            le_audio_set_dvol(le_audio_num, vol_table[g_le_audio_player_file[le_audio_num].dvol_index]);
        } else {
            log_info("le audio volum is min");
        }
    }
}
#endif

static void le_audio_player_callback(void *private_data, int event)
{
    struct le_audio_player *player = (struct le_audio_player *)private_data;

    if (!player) {
        return;
    }

    u8 find = 0;
#if TCFG_KBOX_1T3_MODE_EN
    for (u8 i = 0; i < (sizeof(g_le_audio_player_file) / sizeof(struct le_audio_player) - 1); i++) {
        if (player->player_id == g_le_audio_player_file[i].player_id) {
            find = 1;
            break;
        }
    }
#else
    find = (player->player_id == g_le_audio_player->player_id);
#endif

    if (!find) {
        return;
    }

    log_info("le audio player callback : %d", event);

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
#if TCFG_KBOX_1T3_MODE_EN
        int ret = syscfg_read(CFG_WIRELESS_MIC0_VOLUME + player->le_audio_num, &player->dvol, 2);
        if (ret < 0) {
            player->dvol = 100;
        }
        player->dvol_index = player->dvol / 10;
        if (player->dvol_index >= sizeof(vol_table) / sizeof(vol_table[0])) {
            player->dvol_index = sizeof(vol_table) / sizeof(vol_table[0]) - 1;
        }
        le_audio_set_dvol(player->le_audio_num, player->dvol);

        log_info("le_audio_player_callback, le_audio_num:%d, dvol:%d", player->le_audio_num, player->dvol);
#endif
        break;
    case STREAM_EVENT_PREEMPTED:


        break;
    }
}

int le_audio_player_open(u8 *conn, struct le_audio_stream_params *lea_param)
{
    int err;

    g_player_id ++;

#if TCFG_KBOX_1T3_MODE_EN
    err = le_audio_player_create(conn);
    if (err) {
        return err;
    }

    struct le_audio_player *player = get_le_audio_player_handle(conn);
    if (!player) {
        return -1;
    }

    jlstream_set_scene(player->stream, STREAM_SCENE_WIRELESS_MIC);
    if (lea_param) {
        int frame_point = lea_param->fmt.frame_dms * lea_param->fmt.sample_rate / 1000 / 10;
        jlstream_node_ioctl(player->stream, NODE_UUID_MIXER, NODE_IOC_SET_PRIV_FMT, (int)frame_point);
    }
#else
    int uuid;

    struct le_audio_player *player = g_le_audio_player;
    if (player) {
        return 0;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    if (lea_param && lea_param->service_type == LEA_SERVICE_CALL) {
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"le_audio_call");
    } else {
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"le_audio");
    }

    player->le_audio = conn;

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_LE_AUDIO_SINK);
    if (!player->stream) {
        log_error("create le_audio stream faild");
        free(player);
        return -1;
    }

    if (lea_param) {
        if (lea_param->service_type == LEA_SERVICE_CALL) {
            log_info("LEA Service Type:Call");
            jlstream_set_scene(player->stream, STREAM_SCENE_LEA_CALL);
        } else {
            log_info("LEA Service Type:Media");
            jlstream_set_scene(player->stream, STREAM_SCENE_LE_AUDIO);
        }
    } else {
        jlstream_set_scene(player->stream, STREAM_SCENE_LE_AUDIO);
    }
#endif

    player->player_id = g_player_id;

    jlstream_set_callback(player->stream, player, le_audio_player_callback);

    int channel = le_audio_stream_get_dec_ch_mode(player->le_audio); //这里设置解码输出的声道类型
    jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, channel);

    err = jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                              NODE_IOC_SET_BTADDR, (int)player->le_audio);
    if (err == 0) {
#if TCFG_KBOX_1T3_MODE_EN
        if (player->le_audio_num) {
            jlstream_add_thread(player->stream, "wl_mic_effect2");
            //jlstream_add_thread(player->stream, "wl_mic_effect4");
        } else {
            jlstream_add_thread(player->stream, "wl_mic_effect1");
            //jlstream_add_thread(player->stream, "wl_mic_effect3");
        }
#endif
        err = jlstream_start(player->stream);
        if (err) {
            return err;
        }
    } else {
#if !TCFG_KBOX_1T3_MODE_EN
        jlstream_release(player->stream);
        free(player);
#endif
        return err;
    }

    log_info("le audio player open success : 0x%x, 0x%x, 0x%x", (u32)player, (u32)player->le_audio, (u32)player->stream);

    g_le_audio_player = player;

    return 0;
}

void le_audio_player_close(u8 *conn)
{
#if TCFG_KBOX_1T3_MODE_EN
    struct le_audio_player *player = get_le_audio_player_handle(conn);
    if (!player) {
        return;
    }
    /* if (memcmp(player->stream_addr, conn, 6)) { */
    /*     return; */
    /* } */ //不用指针来判断，因为释放后去申请可能会申请到同一个指针，采用自增的id来判断是否为同一条

#else
    struct le_audio_player *player = g_le_audio_player;

    if (!player) {
        return;
    }
#endif

    u8 find = 0;
#if TCFG_KBOX_1T3_MODE_EN
    for (u8 i = 0; i < (sizeof(g_le_audio_player_file) / sizeof(struct le_audio_player) - 1); i++) {
        if (player->player_id == g_le_audio_player_file[i].player_id) {
            find = 1;
            break;
        }
    }
#else
    find = (player->player_id == g_le_audio_player->player_id);
#endif

    if (!find) {
        return;
    }

    if (player->le_audio != conn) {
        return;
    }

    log_info("le audio player close : 0x%x, 0x%x, 0x%x", (u32)player, (u32)player->le_audio, (u32)player->stream);

    if (player->stream) {
        jlstream_stop(player->stream, 20);
        jlstream_release(player->stream);
    }

#if TCFG_KBOX_1T3_MODE_EN
    le_audio_player_handle_close(conn);
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"mic_effect");
#else
    free(player);
    g_le_audio_player = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"le_audio");
#endif
}

bool le_audio_player_is_playing(void)
{
    if (g_le_audio_player) {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief 获取leaudio播放器的情景
 *
 * @return STREAM_SCENE_LE_AUDIO:leaudio 播歌; STREAM_SCENE_LEA_CALL:leaudio 通话;
 * 				STREAM_SCENE_NONE: leaudio player没有开启
 */
enum stream_scene le_audio_player_get_stream_scene(void)
{
    if (g_le_audio_player && g_le_audio_player->stream) {
        return g_le_audio_player->stream->scene;
    }
    return STREAM_SCENE_NONE;
}

#endif /*LE_AUDIO_STREAM_ENABLE*/

