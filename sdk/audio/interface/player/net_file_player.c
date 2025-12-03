#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".net_player.data.bss")
#pragma data_seg(".net_player.data")
#pragma const_seg(".net_player.text.const")
#pragma code_seg(".net_player.text")
#endif
#include "net_file_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/wait.h"
#include "effects/audio_pitchspeed.h"
#include "jldemuxer.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#include "network_download/net_download.h"
#include "le_audio_player.h"
#include "scene_switch.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif

#if TCFG_LRC_LYRICS_ENABLE
#include "lyrics/lyrics_api.h"
#endif

#if TCFG_APP_NET_MUSIC_EN

#define LOG_TAG             "[NET_MUSIC_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


struct net_file_player_hdl {
    u32 player_id;
    OS_MUTEX mutex;
    struct list_head head;
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
    struct net_file_player *cur_player; //当前的音乐播放器句柄
#endif
};

static struct net_file_player_hdl g_file_player;

static const struct stream_file_ops net_file_ops;

static void net_player_free(struct net_file_player *player)
{
    if (--player->ref == 0) {
        if (player->wait) {
            wait_completion_del(player->wait);
            player->wait = 0;
        }
        if (player->net_file) {
            net_download_close(player->net_file);
            player->net_file = NULL;
        }
        if (player->net_lrc_file) {
            net_download_close(player->net_lrc_file);
            player->net_lrc_file = NULL;
        }
        if (player->break_point_flag == 1) {
            free(player->break_point);
            player->break_point = NULL;
            player->break_point_flag = 0;
        }
        free(player);
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        if (g_file_player.cur_player == player) {
            g_file_player.cur_player = NULL;
        }
#endif
    }
}

static void net_player_callback(void *priv, int event)
{
    struct net_file_player *player = (struct net_file_player *)priv;

    log_info("net_callback: 0x%x, 0x%x", event, (u32)player);

    switch (event) {
    case STREAM_EVENT_START:
        //先判断是否为空防止触发异常
        os_mutex_pend(&g_file_player.mutex, 0);
        if (list_empty(&(g_file_player.head))) {
            os_mutex_post(&g_file_player.mutex);
            break;
        }
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
        musci_vocal_remover_update_parm();
#endif
#if AUDIO_VBASS_LINK_VOLUME
        vbass_link_volume();
#endif
#if AUDIO_EQ_LINK_VOLUME
        eq_link_volume();
#endif
        if (player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_START);
        }
        os_mutex_post(&g_file_player.mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        //先判断是否为空防止触发异常
        os_mutex_pend(&g_file_player.mutex, 0);
        if (list_empty(&(g_file_player.head))) {
            os_mutex_post(&g_file_player.mutex);
            break;
        }

        player->wait = 0;

        if (event == STREAM_EVENT_STOP && player->callback) {
            player->callback(player->priv, player->read_err, STREAM_EVENT_END);
        }

        if (player->callback) {
            player->callback(player->priv, player->read_err, STREAM_EVENT_STOP);
        }

        os_mutex_post(&g_file_player.mutex);
        break;
    }
}

static int net_file_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    int rlen;
    struct net_file_player *player = (struct net_file_player *)file;

    while (len) {
        if (!player->net_file) {
            break;
        }

        rlen = net_download_read(player->net_file, buf + offset, len);

        if (rlen <= 0) {
            if (rlen == -2) {
                player->read_err = 1; //数据读取超时
            } else {
                if (rlen != 0) {
                    player->read_err = 2;
                    return -1;
                }
            }
            break;
        }
        player->read_err = 0;
        offset += rlen;
        if ((len -= rlen) == 0) {
            break;
        }
    }

    return offset;
}

static int net_file_seek(void *file, int offset, int fromwhere)
{
    struct net_file_player *player = (struct net_file_player *)file;
    return net_download_seek(player->net_file, offset, fromwhere);
}

static int net_file_flen(void *file)
{
    struct net_file_player *player = (struct net_file_player *)file;
    u32 len = 0;
    if (player->net_file) {
        len = net_download_get_file_len(player->net_file);
    }
    return len;
}

static int net_file_close(void *file)
{
    /* struct net_file_player *player = (struct net_file_player *)file; */

    return 0;
}

static int net_file_get_fmt(void *file, struct stream_fmt *fmt)
{
    struct net_file_player *player = (struct net_file_player *)file;
    if (!player) {
        return -1;
    }

    const char *music_type = net_download_get_media_type(player->net_file);
    char name[16] = {"netmusic."};

    if (music_type && music_type[0] != '\0') {
        strcpy(name + strlen(name), music_type);
    } else {
        strcpy(name + strlen(name), "mp3");
    }

#if 0
    struct stream_file_info info = {
        .file = player,
        .fname = name,
        .ops  = &net_file_ops,
        .scene = player->scene,
    };

    return jldemuxer_get_tone_file_fmt(&info, fmt);
#else
    return -EINVAL;
#endif
}

static const struct stream_file_ops net_file_ops = {
    .read       = net_file_read,
    .seek       = net_file_seek,
    .close      = net_file_close,
    .get_fmt    = net_file_get_fmt,
};

int net_file_player_pp(struct net_file_player *net_player)
{
    if (net_player && net_player->stream) {
        if (net_player->status == FILE_PLAYER_PAUSE) {
#if 0
            //时间戳使能，多设备播放才需要配置，需接入播放同步节点
            u32 timestamp = audio_jiffies_usec() + 30 * 1000;
            jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif
        }
        if (net_player->net_file && net_player->status != FILE_PLAYER_STOP) {
            net_download_set_pp(net_player->net_file, net_player->status == FILE_PLAYER_START);
        }
        jlstream_pp_toggle(net_player->stream, 50);
        if (net_player->status != FILE_PLAYER_STOP) {
            net_player->status = ((net_player->status == FILE_PLAYER_START) ? FILE_PLAYER_PAUSE : FILE_PLAYER_START);
        }
        return 0;
    } else {
        return -1;
    }
}

int net_file_player_ff(u16 step_s, struct net_file_player *net_player)
{
    if (net_player && net_player->stream) {
        return jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FF, step_s);
    } else {
        return -1;
    }
}

int net_file_player_fr(u16 step_s, struct net_file_player *net_player)
{
    if (net_player && net_player->stream) {
        return jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FR, step_s);
    } else {
        return -1;
    }
}

int net_file_get_cur_time(struct net_file_player *net_player)
{
    os_mutex_pend(&g_file_player.mutex, 0);

    if (list_empty(&(g_file_player.head))) {          //先判断是否为空
        os_mutex_post(&g_file_player.mutex);
        return -1;
    }

    if (!net_player) {
        net_player = list_first_entry(&(g_file_player.head), struct net_file_player, entry);
    }
    if (net_player && net_player->stream) {
        int ret = jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_CUR_TIME, 0);
        os_mutex_post(&g_file_player.mutex);
        return ret;
    }

    os_mutex_post(&g_file_player.mutex);

    return -1;
}

int net_file_get_total_time(struct net_file_player *net_player)
{
    os_mutex_pend(&g_file_player.mutex, 0);

    if (list_empty(&(g_file_player.head))) {          //先判断是否为空
        os_mutex_post(&g_file_player.mutex);
        return -1;
    }

    if (!net_player) {
        net_player = list_first_entry(&(g_file_player.head), struct net_file_player, entry);
    }
    if (net_player && net_player->stream) {
        int ret = jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_TOTAL_TIME, 0);
        os_mutex_post(&g_file_player.mutex);
        return ret;
    }

    os_mutex_post(&g_file_player.mutex);

    return -1;
}

int net_file_get_player_status(struct net_file_player *net_player)
{
    enum play_status status = FILE_PLAYER_STOP;  //播放结束

    os_mutex_pend(&g_file_player.mutex, 0);

    if (!(list_empty(&(g_file_player.head))) && net_player && net_player->stream) {
        status = net_player->status;
        os_mutex_post(&g_file_player.mutex);
        return status;
    }

    os_mutex_post(&g_file_player.mutex);

    return status;
}

//变调接口
int net_file_pitch_up(struct net_file_player *net_player)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (!net_player) {
        return -1;
    }

    if (++net_player->music_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        net_player->music_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }

    log_info("play pitch up+++%d", net_player->music_pitch_mode);

    int err = net_file_set_pitch(net_player, net_player->music_pitch_mode);
    return !err ? net_player->music_pitch_mode : -1;
}

int net_file_pitch_down(struct net_file_player *net_player)
{
    if (!net_player) {
        return -1;
    }

    if (--net_player->music_pitch_mode < 0) {
        net_player->music_pitch_mode = 0;
    }

    log_info("play pitch down---%d", net_player->music_pitch_mode);

    int err = net_file_set_pitch(net_player, net_player->music_pitch_mode);
    return !err ? net_player->music_pitch_mode : -1;
}

int net_file_set_pitch(struct net_file_player *net_player, pitch_level_t pitch_mode)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (net_player) {
        net_player->music_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(net_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

//倍速播放接口
int net_file_speed_up(struct net_file_player *net_player)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (!net_player) {
        return -1;
    }

    if (++net_player->music_speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        net_player->music_speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }

    log_info("play speed up+++%d", net_player->music_speed_mode);

    int err = net_file_set_speed(net_player, net_player->music_speed_mode);
    return !err ? net_player->music_speed_mode : -1;
}

//慢速播放接口
int net_file_speed_down(struct net_file_player *net_player)
{
    if (!net_player) {
        return -1;
    }

    if (--net_player->music_speed_mode < 0) {
        net_player->music_speed_mode = 0;
    }
    log_info("play speed down---%d", net_player->music_speed_mode);

    int err = net_file_set_speed(net_player, net_player->music_speed_mode);
    return !err ? net_player->music_speed_mode : -1;
}

//设置播放速度
int net_file_set_speed(struct net_file_player *net_player, speed_level_t speed_mode)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }
    pitch_speed_param_tool_set speed_param = {
        .pitch = 0,
        .speed = speed_param_table[speed_mode],
    };
    if (net_player) {
        log_info("set play speed %d, %d", net_player->music_speed_mode, (int)(speed_param_table[net_player->music_speed_mode] * 100));
        net_player->music_speed_mode = speed_mode;
        return jlstream_node_ioctl(net_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&speed_param);
    }
    return -1;
}

#if FILE_DEC_AB_REPEAT_EN

#define AUDIO_AB_REPEAT_CODING_TYPE		(AUDIO_CODING_MP3 | AUDIO_CODING_WMA | AUDIO_CODING_WAV | AUDIO_CODING_FLAC | AUDIO_CODING_APE | AUDIO_CODING_DTS)

enum {
    AB_REPEAT_STA_NON = 0,
    AB_REPEAT_STA_ASTA,
    AB_REPEAT_STA_BSTA,
};

/*----------------------------------------------------------------------------*/
/**@brief    设置AB点复读命令
   @param    ab_cmd: 命令
   @param    ab_mode: 参数
   @return   0: 设置成功 -1:设置失败
   @note
*/
/*----------------------------------------------------------------------------*/
static int net_file_ab_repeat_set(int ab_cmd, int ab_mode, struct net_file_player *net_player)
{
    int err = -1;

    if (!net_player) {
        return err;
    }

    log_info("ab repat, cmd:0x%x, mode:%d", ab_cmd, ab_mode);

    struct audio_ab_repeat_mode_param rpt = {0};
    rpt.value = ab_mode;

    switch (ab_cmd) {
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A:
        log_info("SET BREAKPOINT A");
        err = jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_A, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B:
        log_info("SET BREAKPOINT B");
        err = jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_B, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE:
        log_info("CANCEL AB REPEAT");
        err = jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_AB_REPEAT, (int)&rpt);
        break;
    default:
        break;
    }

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    切换AB点复读状态
   @param
   @return   0: 设置成功 -1:设置失败
   @note
*/
/*----------------------------------------------------------------------------*/
int net_file_ab_repeat_switch(struct net_file_player *net_player)
{
    int err = -1;

    if (!net_player) {
        return err;
    }

    switch (net_player->ab_repeat_status) {
    case AB_REPEAT_STA_NON:
        err = net_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A, 0, net_player);
        if (!err) {
            net_player->ab_repeat_status = AB_REPEAT_STA_ASTA;
        }
        break;
    case AB_REPEAT_STA_ASTA:
        err = net_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, net_player);
        if (!err) {
            net_player->ab_repeat_status = AB_REPEAT_STA_BSTA;
        }
        break;
    case AB_REPEAT_STA_BSTA:
        err = net_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, net_player);
        if (!err) {
            net_player->ab_repeat_status = AB_REPEAT_STA_NON;
        }
        break;
    }

    log_info("music_file_ab_repeat_switch = %d", net_player->ab_repeat_status);

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭AB点复读
   @param
   @return   0: 设置成功 -1:设置失败
   @note
*/
/*----------------------------------------------------------------------------*/
int net_file_ab_repeat_close(struct net_file_player *net_player)
{
    int err = -1;

    if (!net_player) {
        return err;
    }

    if (net_player->ab_repeat_status == AB_REPEAT_STA_NON) {
        return 0;
    }

    if (net_player->ab_repeat_status == AB_REPEAT_STA_ASTA) {
        struct stream_fmt fmt;
        jlstream_node_ioctl(net_player->stream, NODE_UUID_SOURCE, NODE_IOC_GET_FMT, (int)&fmt);

        switch (fmt.coding_type) {
        case AUDIO_CODING_FLAC:
        case AUDIO_CODING_DTS:
        case AUDIO_CODING_APE:
            net_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, net_player);
            break;
        }
    }

    err = net_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, net_player);
    net_player->ab_repeat_status = AB_REPEAT_STA_NON;

    return err;
}

#endif /*FILE_DEC_AB_REPEAT_EN*/

int net_file_get_breakpoints(struct audio_dec_breakpoint *bp, struct net_file_player *net_player)
{
    if (net_player && net_player->read_err != 0xff) {
        os_mutex_pend(&g_file_player.mutex, 0);
        if (list_empty(&(g_file_player.head))) {
            os_mutex_post(&g_file_player.mutex);
            return -1;
        }
        os_mutex_post(&g_file_player.mutex);
        return jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_BP, (int)bp);
    }

    return -1;
}

#if FILE_DEC_REPEAT_EN
/*----------------------------------------------------------------------------*/
/**@brief    循环播放回调接口
   @param    *priv: 私有参数
   @return   0：循环播放
   @return   非0：结束循环
   @note
*/
/*----------------------------------------------------------------------------*/
static int net_file_dec_repeat_cb(void *priv)
{
    struct net_file_player *net_player = priv;

    log_info("net_file_dec_repeat_cb");

    if (net_player->repeat_num) {
        net_player->repeat_num--;
    } else {
        log_info("net_file_dec_repeat_cb end");
        return -1;
    }

    return 0;
}

int net_file_dec_repeat_set(struct net_file_player *net_player, u8 repeat_num)
{
    if (!net_player) {
        return -1;
    }

    struct audio_repeat_mode_param rep = {0};

    switch (net_player->stream->coding_type) {
    case AUDIO_CODING_MP3:
    case AUDIO_CODING_WAV:
        net_player->repeat_num = repeat_num;
        /* struct audio_repeat_mode_param rep = {0}; */
        rep.flag = 1; //使能
        rep.headcut_frame = 2; //依据需求砍掉前面几帧，仅mp3格式有效
        rep.tailcut_frame = 2; //依据需求砍掉后面几帧，仅mp3格式有效
        rep.repeat_callback = net_file_dec_repeat_cb;
        rep.callback_priv = net_player;
        rep.repair_buf = &net_player->repair_buf;
        jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_REPEAT, (int)&rep);
        return 0;
    }

    return -1;
}
#endif

#if FILE_DEC_DEST_PLAY
int net_file_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct net_file_player *net_player)
{
    if (!net_player) {
        return -1;
    }

    struct audio_dest_time_play_param param = {0};

    switch (coding_type) {
    case AUDIO_CODING_MP3:
        param.start_time = start_time;
        param.dest_time = dest_time;
        param.callback_func = cb;
        param.callback_priv = cb_priv;
        jlstream_node_ioctl(net_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_DEST_PLAY, (int)&param);
        return 0;
    }

    return -1;
}

int net_file_dec_set_start_play(u32 start_time, u32 coding_type)
{
    return net_file_dec_set_start_dest_play(start_time, 0x7fffffff, NULL, NULL, g_file_player.cur_player->stream->coding_type, g_file_player.cur_player);
}
#endif

static int __net_completion_callback(void *priv, int timeout)
{
    struct net_file_player *player = (struct net_file_player *)priv;

    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        os_mutex_post(&g_file_player.mutex);
        return -1;
    }

    if (timeout || net_download_check_ready(player->net_file) < 0) {
        log_info("net file player start timeout: %d", player->player_id);
        net_player_callback(player, STREAM_EVENT_STOP);
        os_mutex_post(&g_file_player.mutex);
        return 0;
    }

#if TCFG_USER_EMITTER_ENABLE
    extern u8 *get_cur_connect_emitter_mac_addr(void);
    u8 *bt_addr = get_cur_connect_emitter_mac_addr();
    if (bt_addr) {
        jlstream_node_ioctl(player->stream, NODE_UUID_A2DP_TX, NODE_IOC_SET_BTADDR, (int)bt_addr);
    }
#endif

    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, net_file_flen(player));

    if (player->break_point->fptr == 0) {
        net_download_read(player->net_file, player->break_point->header, sizeof(player->break_point->header));
        net_download_seek(player->net_file, 0, SEEK_SET);
    }

    int err = jlstream_start(player->stream);
    if (err) {
        goto __exit;
    }

#if TCFG_LE_AUDIO_STREAM_ENABLE && TCFG_LEA_LOCAL_SYNC_PLAY_EN
    if (player->le_audio) {
        le_audio_player_open(player->le_audio, NULL);
    }
#endif

    if (player->stream->coding_type == AUDIO_CODING_MP3) {
#if TCFG_DEC_ID3_V1_ENABLE
        if (player->p_mp3_id3_v1) {
            id3_obj_post(&player->p_mp3_id3_v1);
        }
        player->p_mp3_id3_v1 = id3_v1_obj_get(player->file);
#endif
#if TCFG_DEC_ID3_V2_ENABLE
        if (player->p_mp3_id3_v2) {
            id3_obj_post(&player->p_mp3_id3_v2);
        }
        player->p_mp3_id3_v2 = id3_v2_obj_get(player->file);
#endif
    }

    player->status = FILE_PLAYER_START;

    os_mutex_post(&g_file_player.mutex);

    return 0;

__exit:
    jlstream_release(player->stream);
    player->stream = NULL;
    player->read_err = 0xff;
    if (player->callback) {
        player->callback(player->priv, player->read_err, STREAM_EVENT_STOP);
    }
    //资源在应用层释放
    /* net_player_free(player); */
    os_mutex_post(&g_file_player.mutex);

    return err;
}

int net_file_player_start(struct net_file_player *player)
{
    int err = -1;

    os_mutex_pend(&g_file_player.mutex, 0);

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"net_music");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_NET_FILE);
    if (!player->stream) {
        goto __exit0;
    }

    jlstream_set_callback(player->stream, player, net_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_NET_FILE, player->break_point->fptr ? (u32)player->break_point->header : 0);
    jlstream_set_dec_file(player->stream, player, &net_file_ops);

#if 0
    //时间戳使能，多设备播放才需要配置，需接入播放同步节点
    u32 timestamp = audio_jiffies_usec() + 30 * 1000;
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif

    if (player->callback) {
        err = player->callback(player->priv, 0, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }

    err = net_download_check_ready(player->net_file);
    if (err < 0) {
        goto __exit1;
    } else if (err == 0) {
        player->wait = wait_completion_timeout_add_to_task("app_core", net_download_check_ready, __net_completion_callback, player, player->net_file, 10000);
        if (!player->wait) {
            err = -1;
            goto __exit1;
        }
    } else {
#if TCFG_USER_EMITTER_ENABLE
        extern u8 *get_cur_connect_emitter_mac_addr(void);
        u8 *bt_addr = get_cur_connect_emitter_mac_addr();
        if (bt_addr) {
            jlstream_node_ioctl(player->stream, NODE_UUID_A2DP_TX, NODE_IOC_SET_BTADDR, (int)bt_addr);
        }
#endif

        jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_FILE_LEN, net_file_flen(player));

        if (player->break_point->fptr == 0) {
            net_download_read(player->net_file, player->break_point->header, sizeof(player->break_point->header));
            net_download_seek(player->net_file, 0, SEEK_SET);
        }

        err = jlstream_start(player->stream);
        if (err) {
            goto __exit1;
        }

        if (player->stream->coding_type == AUDIO_CODING_MP3) {
#if TCFG_DEC_ID3_V1_ENABLE
            if (player->p_mp3_id3_v1) {
                id3_obj_post(&player->p_mp3_id3_v1);
            }
            player->p_mp3_id3_v1 = id3_v1_obj_get(player->file);
#endif
#if TCFG_DEC_ID3_V2_ENABLE
            if (player->p_mp3_id3_v2) {
                id3_obj_post(&player->p_mp3_id3_v2);
            }
            player->p_mp3_id3_v2 = id3_v2_obj_get(player->file);
#endif
        }

        player->status = FILE_PLAYER_START;
    }

    os_mutex_post(&g_file_player.mutex);

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    net_player_free(player);
    os_mutex_post(&g_file_player.mutex);

    return err;
}

static int net_player_init(struct net_file_player *player, void *net_file, struct audio_dec_breakpoint *dbp)
{
    player->ref         = 1;
    player->net_file    = net_file;
    player->scene       = STREAM_SCENE_MUSIC;
    player->player_id   = g_file_player.player_id++;
    player->coexist     = STREAM_COEXIST_AUTO;

    if (dbp == NULL) {
        player->break_point = zalloc(sizeof(struct audio_dec_breakpoint) + BREAKPOINT_DATA_LEN);
        player->break_point->data_len = BREAKPOINT_DATA_LEN;
        player->break_point_flag = 1;
    } else {
        player->break_point = dbp;
        player->break_point_flag = 0;
    }
    player->music_speed_mode  = PLAY_SPEED_1; //固定开始的时候使用1倍速播放 */
    player->music_pitch_mode = PITCH_0;  //固定开始时不变调
#if TCFG_DEC_DECRYPT_ENABLE
    cipher_init(&player->mply_cipher, TCFG_DEC_DECRYPT_KEY);
    cipher_check_decode_file(&player->mply_cipher, net_file);
#endif

    INIT_LIST_HEAD(&player->entry);

    return 0;
}

static struct net_file_player *net_player_create(const char *url, struct audio_dec_breakpoint *dbp)
{
    struct net_file_player *player;
    void *net_file = NULL;
    struct net_download_parm parm = {0};

    if (!url) {
        return NULL;
    }

    parm.url = url;
    //网络缓冲buf大小
    parm.cbuf_size = 500 * 1024;
    //设置网络下载超时
    parm.timeout_millsec = 10000;
    parm.start_play_threshold = 512;
    parm.seek_threshold  = 1024 * 200;	//用户可适当调整

    if (dbp && dbp->len > 0 && dbp->fptr > 0) {
        parm.seek_low_range = dbp->fptr;    //恢复断点时设置网络的开始下载地址
    }

    if (0 != net_download_open(&net_file, &parm)) {
        return NULL;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    net_player_init(player, net_file, dbp);

    return player;
}

struct net_file_player *net_player_add(struct net_file_player *player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        int err = net_file_player_start(player);
        if (err) {
            os_mutex_post(&g_file_player.mutex);
            return NULL;
        }
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        g_file_player.cur_player = player;
#endif
    }
    list_add_tail(&player->entry, &(g_file_player.head));
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct net_file_player *net_file_play(const char *url, struct audio_dec_breakpoint *dbp)
{
    struct net_file_player *player = net_player_create(url, dbp);
    if (!player) {
        return NULL;
    }
    return net_player_add(player);
}

struct net_file_player *net_file_play_callback(const char *url, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp)
{
    struct net_file_player *player = net_player_create(url, dbp);
    if (!player) {
        return NULL;
    }
    player->priv = priv;
    player->callback = callback;
    return net_player_add(player);
}

int net_player_runing(void)
{
    return !list_empty(&(g_file_player.head));
}

void net_file_player_stop_all(void)
{
    struct net_file_player *player, *n;

    os_mutex_pend(&g_file_player.mutex, 0);

    list_for_each_entry_safe(player, n, &(g_file_player.head), entry) {
        __list_del_entry(&player->entry);
        if (player->wait) {
            wait_completion_del(player->wait);
            player->wait = 0;
        }
        if (player->net_file) {
            net_download_buf_inactive(player->net_file);
        }
        if (player->stream) {
            jlstream_stop(player->stream, 50);
            jlstream_release(player->stream);
        }
#if TCFG_DEC_ID3_V1_ENABLE
        if (player->p_mp3_id3_v1) {
            id3_obj_post(&player->p_mp3_id3_v1);
        }
#endif
#if TCFG_DEC_ID3_V2_ENABLE
        if (player->p_mp3_id3_v2) {
            id3_obj_post(&player->p_mp3_id3_v2);
        }
#endif

        net_player_free(player);
    }

    os_mutex_post(&g_file_player.mutex);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"net_music");
}

void net_file_player_stop(struct net_file_player *player)
{
    struct net_file_player *p, *n;

    os_mutex_pend(&g_file_player.mutex, 0);
    list_for_each_entry_safe(p, n, &g_file_player.head, entry) {
        if (p == player) {
            __list_del_entry(&player->entry);
            goto __stop;
        }
    }
    os_mutex_post(&g_file_player.mutex);

    return;

__stop:
    if (player->wait) {
        wait_completion_del(player->wait);
        player->wait = 0;
    }
    if (player->net_file) {
        net_download_buf_inactive(player->net_file);
    }
    if (player->stream) {
        jlstream_stop(player->stream, 50);
        jlstream_release(player->stream);
    }
#if TCFG_DEC_ID3_V1_ENABLE
    if (player->p_mp3_id3_v1) {
        id3_obj_post(&player->p_mp3_id3_v1);
    }
#endif
#if TCFG_DEC_ID3_V2_ENABLE
    if (player->p_mp3_id3_v2) {
        id3_obj_post(&player->p_mp3_id3_v2);
    }
#endif

    net_player_free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"net_music");

    os_mutex_post(&g_file_player.mutex);
}

struct net_file_player *get_net_file_player(void) //返回第一个打开的音乐播放器指针
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        os_mutex_post(&g_file_player.mutex);
        return NULL;
    }
    struct net_file_player *player = list_first_entry(&(g_file_player.head), struct net_file_player, entry);
    os_mutex_post(&g_file_player.mutex);
    return player;
}

int net_music_lrc_analysis_start(void *lrc_info, const char *url, struct net_file_player *net_player)
{
#if (defined(TCFG_LRC_LYRICS_ENABLE) && (TCFG_LRC_LYRICS_ENABLE))
    struct net_download_parm parm = {0};
    if (!url || !net_player || !lrc_info) {
        return -1;
    }

    parm.url = url;
    //网络缓冲buf大小
    parm.cbuf_size = 4 * 1024;
    parm.net_buf = malloc(parm.cbuf_size);
    //设置网络下载超时
    parm.timeout_millsec = 10000;

    if (net_player->net_lrc_file) {
        net_download_close(net_player->net_lrc_file);
        net_player->net_lrc_file = NULL;
    }

    if (0 != net_download_open(&net_player->net_lrc_file, &parm)) {
        return -1;
    }

    log_info("net music lrc analysis start");

    if (net_player->net_lrc_file) {
        return net_lrc_analysis_api(lrc_info, net_player->net_lrc_file) ? 0 : -1;
    }
#endif

    return -1;
}

static int __net_player_init(void)
{
    INIT_LIST_HEAD(&g_file_player.head);
    os_mutex_create(&g_file_player.mutex);
    return 0;
}
__initcall(__net_player_init);

#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN || TCFG_LEA_CIG_CENTRAL_EN || TCFG_LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
static int le_audio_net_file_player_start(struct net_file_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music_le_audio");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_NET_FILE);
    if (!player->stream) {
        goto __exit0;
    }
    int player_id = player->player_id;
    jlstream_set_callback(player->stream, player, net_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_NET_FILE,
                        player->break_point->fptr ? (u32)player->break_point->header : 0);
    jlstream_set_dec_file(player->stream, player, &net_file_ops);
    jlstream_node_ioctl(player->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)player->le_audio);
    jlstream_ioctl(player->stream, NODE_IOC_SET_ENC_FMT, (int)player->le_audio_fmt);
#if 0
//时间戳使能，多设备播放才需要配置，需接入播放同步节点
    u32 timestamp = audio_jiffies_usec() + 30 * 1000;
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif

    if (player->callback) {
        err = player->callback(player->priv, 0, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }

    err = net_download_check_ready(player->net_file);
    if (err < 0) {
        goto __exit1;
    } else if (err == 0) {
        player->wait = wait_completion_timeout_add_to_task("app_core", net_download_check_ready, __net_completion_callback, player, player->net_file, 10000);
        if (!player->wait) {
            err = -1;
            goto __exit1;
        }
    } else {
        jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_FILE_LEN, net_file_flen(player));

        if (player->break_point->fptr == 0) {
            net_download_read(player->net_file, player->break_point->header, sizeof(player->break_point->header));
            net_download_seek(player->net_file, 0, SEEK_SET);
        }

        err = jlstream_start(player->stream);
        if (err) {
            goto __exit1;
        }

#if TCFG_LEA_LOCAL_SYNC_PLAY_EN
        if (player->le_audio) {
            le_audio_player_open(player->le_audio, NULL);
        }
#endif

        if (player->stream->coding_type == AUDIO_CODING_MP3) {
#if TCFG_DEC_ID3_V1_ENABLE
            if (player->p_mp3_id3_v1) {
                id3_obj_post(&player->p_mp3_id3_v1);
            }
            player->p_mp3_id3_v1 = id3_v1_obj_get(player->file);
#endif
#if TCFG_DEC_ID3_V2_ENABLE
            if (player->p_mp3_id3_v2) {
                id3_obj_post(&player->p_mp3_id3_v2);
            }
            player->p_mp3_id3_v2 = id3_v2_obj_get(player->file);
#endif
        }

        player->status = FILE_PLAYER_START;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    net_player_free(player);

    return err;
}

static struct net_file_player *le_audio_net_music_player_add(struct net_file_player *player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        int err = le_audio_net_file_player_start(player);
        if (err) {
            os_mutex_post(&g_file_player.mutex);
            return NULL;
        }
    }
    list_add_tail(&player->entry, &(g_file_player.head));
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct net_file_player *le_audio_net_file_play_callback(const char *url, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp, void *le_audio, void *fmt)
{
    struct net_file_player *player;
    player = net_player_create(url, dbp);
    if (!player) {
        return NULL;
    }
    player->priv            = priv;
    player->callback        = callback;
    player->le_audio        = le_audio;
    player->le_audio_fmt    = fmt;
    return le_audio_net_music_player_add(player);
}
#endif

#endif
