#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vir_player.data.bss")
#pragma data_seg(".vir_player.data")
#pragma const_seg(".vir_player.text.const")
#pragma code_seg(".vir_player.text")
#endif
#include "vir_dev_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "fs/fs.h"
#include "effects/audio_pitchspeed.h"
#include "audio_decoder.h"
#include "jldemuxer.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#include "app_config.h"
#include "network_download/net_audio_buf.h"

#define LOG_TAG             "[VIRTUAL_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

struct virtual_dev_player_hdl {
    u32 player_id;
    OS_MUTEX mutex;
    struct list_head head;
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
    struct vir_player *cur_player; //当前的音乐播放器句柄
#endif
};

static struct virtual_dev_player_hdl g_vir_dev_player;
static const struct stream_file_ops virtual_dev_ops;

static void virtual_player_free(struct vir_player *player)
{
    if (--player->ref == 0) {
        if (player->break_point_flag == 1) {
            free(player->break_point);
            player->break_point = NULL;
            player->break_point_flag = 0;
        }
        free(player);
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        if (player == g_vir_dev_player.cur_player) {
            g_vir_dev_player.cur_player = NULL;
        }
#endif
    }
}

static void virtual_player_callback(void *priv, int event)
{
    struct vir_player *player = (struct vir_player *)priv;

    log_info("virtual_player_callback: 0x%x, 0x%x", event, player);

    switch (event) {
    case STREAM_EVENT_START:
        os_mutex_pend(&g_vir_dev_player.mutex, 0);
        if (list_empty(&(g_vir_dev_player.head))) {
            os_mutex_post(&g_vir_dev_player.mutex);
            break;
        }
#if AUDIO_VBASS_LINK_VOLUME
        vbass_link_volume();
#endif
        if (player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_START);
        }
        os_mutex_post(&g_vir_dev_player.mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        //先判断是否为空防止触发异常
        os_mutex_pend(&g_vir_dev_player.mutex, 0);
        if (list_empty(&(g_vir_dev_player.head))) {
            os_mutex_post(&g_vir_dev_player.mutex);
            break;
        }

        if (event == STREAM_EVENT_STOP && player->callback) {
            player->callback(player->priv, player->read_err, STREAM_EVENT_END);
        }

        if (player->callback) {
            player->callback(player->priv, player->read_err, STREAM_EVENT_STOP);
        }

        os_mutex_post(&g_vir_dev_player.mutex);
        break;
    }
}

int virtual_dev_player_pp(struct vir_player *virtual_player)
{
    if (virtual_player && virtual_player->stream) {
        if (virtual_player->status == FILE_PLAYER_PAUSE) {
#if 0
            //时间戳使能，多设备播放才需要配置，需接入播放同步节点
            u32 timestamp = audio_jiffies_usec() + 30 * 1000;
            jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif
        }
        jlstream_pp_toggle(virtual_player->stream, 50);
        if (virtual_player->status != FILE_PLAYER_STOP) {
            virtual_player->status = ((virtual_player->status == FILE_PLAYER_START) ? FILE_PLAYER_PAUSE : FILE_PLAYER_START);
        }
        return 0;
    } else {
        return -1;
    }
}

int virtual_dev_player_ff(u16 step_s, struct vir_player *virtual_player)
{
    if (virtual_player && virtual_player->stream) {
        return jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FF, step_s);
    } else {
        return -1;
    }
}

int virtual_dev_player_fr(u16 step_s, struct vir_player *virtual_player)
{
    if (virtual_player && virtual_player->stream) {
        return jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FR, step_s);
    } else {
        return -1;
    }
}

int virtual_dev_get_cur_time(struct vir_player *virtual_player)
{
    os_mutex_pend(&g_vir_dev_player.mutex, 0);
    if (list_empty(&(g_vir_dev_player.head))) {          //先判断是否为空
        os_mutex_post(&g_vir_dev_player.mutex);
        return -1;
    }

    if (!virtual_player) {
        virtual_player = list_first_entry(&(g_vir_dev_player.head), struct vir_player, entry);
    }
    if (virtual_player && virtual_player->stream) {
        int ret = jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_CUR_TIME, 0);
        os_mutex_post(&g_vir_dev_player.mutex);
        return ret;
    }
    os_mutex_post(&g_vir_dev_player.mutex);

    return -1;
}

int virtual_dev_get_total_time(struct vir_player *virtual_player)
{
    os_mutex_pend(&g_vir_dev_player.mutex, 0);
    if (list_empty(&(g_vir_dev_player.head))) {          //先判断是否为空
        os_mutex_post(&g_vir_dev_player.mutex);
        return -1;
    }

    if (!virtual_player) {
        virtual_player = list_first_entry(&(g_vir_dev_player.head), struct vir_player, entry);
    }
    if (virtual_player && virtual_player->stream) {
        int ret = jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_TOTAL_TIME, 0);
        os_mutex_post(&g_vir_dev_player.mutex);
        return ret;
    }

    return -1;
}

int virtual_dev_get_player_status(struct vir_player *virtual_player)
{
    enum play_status status = FILE_PLAYER_STOP;  //播放结束

    os_mutex_pend(&g_vir_dev_player.mutex, 0);
    if (!(list_empty(&(g_vir_dev_player.head))) && virtual_player && virtual_player->stream) {
        status = virtual_player->status;
        os_mutex_post(&g_vir_dev_player.mutex);
        return status;
    }
    os_mutex_post(&g_vir_dev_player.mutex);

    return status;
}

//变调接口
int virtual_dev_pitch_up(struct vir_player *virtual_player)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (!virtual_player) {
        return -1;
    }

    if (++virtual_player->music_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        virtual_player->music_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }

    log_info("play pitch up+++%d", virtual_player->music_pitch_mode);

    int err = virtual_dev_set_pitch(virtual_player, virtual_player->music_pitch_mode);
    return !err ? virtual_player->music_pitch_mode : -1;
}

int virtual_dev_pitch_down(struct vir_player *virtual_player)
{
    if (!virtual_player) {
        return -1;
    }

    if (--virtual_player->music_pitch_mode < 0) {
        virtual_player->music_pitch_mode = 0;
    }

    log_info("play pitch down---%d", virtual_player->music_pitch_mode);

    int err = virtual_dev_set_pitch(virtual_player, virtual_player->music_pitch_mode);
    return !err ? virtual_player->music_pitch_mode : -1;
}

int virtual_dev_set_pitch(struct vir_player *virtual_player, pitch_level_t pitch_mode)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (virtual_player) {
        virtual_player->music_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(virtual_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

//倍速播放接口
int virtual_dev_speed_up(struct vir_player *virtual_player)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (!virtual_player) {
        return -1;
    }

    if (++virtual_player->music_speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        virtual_player->music_speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }

    log_info("play speed up+++%d", virtual_player->music_speed_mode);

    int err = virtual_dev_set_speed(virtual_player, virtual_player->music_speed_mode);
    return !err ? virtual_player->music_speed_mode : -1;
}

//慢速播放接口
int virtual_dev_speed_down(struct vir_player *virtual_player)
{
    if (!virtual_player) {
        return -1;
    }

    if (--virtual_player->music_speed_mode < 0) {
        virtual_player->music_speed_mode = 0;
    }
    log_info("play speed down---%d", virtual_player->music_speed_mode);

    int err = virtual_dev_set_speed(virtual_player, virtual_player->music_speed_mode);
    return !err ? virtual_player->music_speed_mode : -1;
}

//设置播放速度
int virtual_dev_set_speed(struct vir_player *virtual_player, speed_level_t speed_mode)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }
    pitch_speed_param_tool_set speed_param = {
        .pitch = 0,
        .speed = speed_param_table[speed_mode],
    };
    if (virtual_player) {
        log_info("set play speed %d, %d", virtual_player->music_speed_mode, (int)(speed_param_table[virtual_player->music_speed_mode] * 100));
        virtual_player->music_speed_mode = speed_mode;
        return jlstream_node_ioctl(virtual_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&speed_param);
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
static int virtual_dev_ab_repeat_set(int ab_cmd, int ab_mode, struct vir_player *virtual_player)
{
    int err = -1;

    if (!virtual_player) {
        return err;
    }

    log_info("ab repat, cmd:0x%x, mode:%d", ab_cmd, ab_mode);

    struct audio_ab_repeat_mode_param rpt = {0};
    rpt.value = ab_mode;

    switch (ab_cmd) {
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A:
        log_info("SET BREAKPOINT A");
        err = jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_A, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B:
        log_info("SET BREAKPOINT B");
        err = jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_B, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE:
        log_info("CANCEL AB REPEAT");
        err = jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_AB_REPEAT, (int)&rpt);
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
int virtual_dev_ab_repeat_switch(struct vir_player *virtual_player)
{
    int err = -1;

    if (!virtual_player) {
        return err;
    }

    switch (virtual_player->ab_repeat_status) {
    case AB_REPEAT_STA_NON:
        err = virtual_dev_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A, 0, virtual_player);
        if (!err) {
            virtual_player->ab_repeat_status = AB_REPEAT_STA_ASTA;
        }
        break;
    case AB_REPEAT_STA_ASTA:
        err = virtual_dev_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, virtual_player);
        if (!err) {
            virtual_player->ab_repeat_status = AB_REPEAT_STA_BSTA;
        }
        break;
    case AB_REPEAT_STA_BSTA:
        err = virtual_dev_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, virtual_player);
        if (!err) {
            virtual_player->ab_repeat_status = AB_REPEAT_STA_NON;
        }
        break;
    }

    log_info("virtual_dev_ab_repeat_switch = %d", virtual_player->ab_repeat_status);

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭AB点复读
   @param
   @return   0: 设置成功 -1:设置失败
   @note
*/
/*----------------------------------------------------------------------------*/
int virtual_dev_ab_repeat_close(struct vir_player *virtual_player)
{
    int err = -1;

    if (!virtual_player) {
        return err;
    }

    if (virtual_player->ab_repeat_status == AB_REPEAT_STA_NON) {
        return 0;
    }

    if (virtual_player->ab_repeat_status == AB_REPEAT_STA_ASTA) {
        struct stream_fmt fmt;
        jlstream_node_ioctl(virtual_player->stream, NODE_UUID_SOURCE, NODE_IOC_GET_FMT, (int)&fmt);

        switch (fmt.coding_type) {
        case AUDIO_CODING_FLAC:
        case AUDIO_CODING_DTS:
        case AUDIO_CODING_APE:
            virtual_dev_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, virtual_player);
            break;
        }
    }

    err = virtual_dev_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, virtual_player);
    virtual_player->ab_repeat_status = AB_REPEAT_STA_NON;

    return err;
}

#endif /*FILE_DEC_AB_REPEAT_EN*/

int virtual_dev_get_breakpoints(struct audio_dec_breakpoint *bp, struct vir_player *virtual_player)
{
    if (virtual_player) {
        return jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_BP, (int)bp);
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
static int file_dec_repeat_cb(void *priv)
{
    struct vir_player *virtual_player = priv;

    log_info("file_dec_repeat_cb");

    if (virtual_player->repeat_num) {
        virtual_player->repeat_num--;
    } else {
        log_info("file_dec_repeat_cb end");
        return -1;
    }

    return 0;
}

int vir_dev_dec_repeat_set(struct vir_player *virtual_player, u8 repeat_num)
{
    if (!virtual_player) {
        return -1;
    }

    struct audio_repeat_mode_param rep = {0};

    switch (virtual_player->stream->coding_type) {
    case AUDIO_CODING_MP3:
    case AUDIO_CODING_WAV:
        virtual_player->repeat_num = repeat_num;
        /* struct audio_repeat_mode_param rep = {0}; */
        rep.flag = 1; //使能
        rep.headcut_frame = 2; //依据需求砍掉前面几帧，仅mp3格式有效
        rep.tailcut_frame = 2; //依据需求砍掉后面几帧，仅mp3格式有效
        rep.repeat_callback = file_dec_repeat_cb;
        rep.callback_priv = virtual_player;
        rep.repair_buf = &virtual_player->repair_buf;
        jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_REPEAT, (int)&rep);
        return 0;
    }

    return -1;
}
#endif

#if FILE_DEC_DEST_PLAY
int vir_dev_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct vir_player *virtual_player)
{
    if (!virtual_player) {
        return -1;
    }

    struct audio_dest_time_play_param param = {0};

    switch (coding_type) {
    case AUDIO_CODING_MP3:
        param.start_time = start_time;
        param.dest_time = dest_time;
        param.callback_func = cb;
        param.callback_priv = cb_priv;
        jlstream_node_ioctl(virtual_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_DEST_PLAY, (int)&param);
        return 0;
    }

    return -1;
}

int vir_dev_dec_set_start_play(u32 start_time, u32 coding_type)
{
    return file_dec_set_start_dest_play(start_time, 0x7fffffff, NULL, NULL, g_vir_dev_player.cur_player->stream->coding_type, g_vir_dev_player.cur_player);
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    获取id3信息
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int vir_dev_dec_id3_post(struct vir_player *player)
{
    if (!player) {
        return false;
    }

    u32 id3_buf[8 + 13 + 8];
    struct id3_info info = {0};
    info.set.ptr = id3_buf;
    info.set.max_len = sizeof(id3_buf);
    info.set.n_items = 6;
    info.set.item_limit_len = 64;
    info.set.frame_id_att[0] = 0x3f; // 0x3E;  0x3f;

    int ret = jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_GET_ID3, (int)&info);
    if (ret == 0) {
        for (int i = 0; i < info.set.n_items; i++) {
            if ((info.data.len_list[i] != 0) && info.data.mptr[i] != 0) {
                printf("%d\t :  %s\n", i, info.data.mptr[i]);        //显示字符串.
                put_buf(info.data.mptr[i], info.data.len_list[i]);  //HEX
            }
        }
    }
    return true;
}

int virtual_player_start(struct vir_player *player, const struct stream_file_ops *ops)
{
    int err = -EINVAL;

    if (ops == NULL) {
        ops = &virtual_dev_ops;
    }

    os_mutex_pend(&g_vir_dev_player.mutex, 0);

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"virtual");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_VIR_DATA_RX);
    if (!player->stream) {
        goto __exit0;
    }
    jlstream_set_callback(player->stream, player, virtual_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_NET_FILE, player->break_point->fptr ? (u32)player->break_point->header : 0);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, -1);

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

    jlstream_set_dec_file(player->stream, player, ops);

    if (player->break_point->fptr == 0) {
        ops->read(player, player->break_point->header, sizeof(player->break_point->header));
        ops->seek(player, 0, SEEK_SET);
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
    } else {
#if (TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE)
        vir_dev_dec_id3_post(player);
#endif
    }

    os_mutex_post(&g_vir_dev_player.mutex);
    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    virtual_player_free(player);
    os_mutex_post(&g_vir_dev_player.mutex);

    return err;
}

static int virtual_player_init(struct vir_player *player, void *file, struct audio_dec_breakpoint *dbp)
{
    player->ref         = 1;
    player->file        = file;
    player->scene       = STREAM_SCENE_MUSIC;
    player->player_id   = g_vir_dev_player.player_id++;
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
    cipher_check_decode_file(&player->mply_cipher, file);
#endif

    INIT_LIST_HEAD(&player->entry);

    return 0;
}

static struct vir_player *virtual_player_create(void *file, struct audio_dec_breakpoint *dbp, u32 coding_type)
{
    struct vir_player *player;

    if (!file) {
        return NULL;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    virtual_player_init(player, file, dbp);

    player->coding_type = coding_type;

    return player;
}

struct vir_player *virtual_player_add(struct vir_player *player, struct stream_file_ops *ops)
{
    os_mutex_pend(&g_vir_dev_player.mutex, 0);
    if (list_empty(&(g_vir_dev_player.head))) {
        int err = virtual_player_start(player, ops);
        if (err) {
            os_mutex_post(&g_vir_dev_player.mutex);
            virtual_player_free(player);
            return NULL;
        }
        player->status = FILE_PLAYER_START;
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        g_vir_dev_player.cur_player = player;
#endif
    }
    list_add_tail(&player->entry, &(g_vir_dev_player.head));
    os_mutex_post(&g_vir_dev_player.mutex);
    return player;
}

struct vir_player *virtual_dev_play(FILE *file, struct stream_file_ops *ops, struct audio_dec_breakpoint *dbp, u32 coding_type)
{
    struct vir_player *player = virtual_player_create(file, dbp, coding_type);
    if (!player) {
        return NULL;
    }
    return virtual_player_add(player, ops);
}

struct vir_player *virtual_dev_play_callback(FILE *file, struct stream_file_ops *ops, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp, u32 coding_type)
{
    struct vir_player *player = virtual_player_create(file, dbp, coding_type);
    if (!player) {
        return NULL;
    }
    player->priv        = priv;
    player->callback    = callback;
    return virtual_player_add(player, ops);
}

int virtual_player_runing(void)
{
    return !list_empty(&(g_vir_dev_player.head));
}

void virtual_dev_player_stop(struct vir_player *player)
{
    if (!player) {
        return;
    }

    struct vir_player *p, *n;

    os_mutex_pend(&g_vir_dev_player.mutex, 0);
    list_for_each_entry_safe(p, n, &g_vir_dev_player.head, entry) {
        if (p == player) {
            __list_del_entry(&player->entry);
            goto __stop;
        }
    }
    os_mutex_post(&g_vir_dev_player.mutex);

    return;

__stop:
    if (player->file) {
        net_buf_inactive(player->file);
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

    virtual_player_free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"virtual");

    os_mutex_post(&g_vir_dev_player.mutex);
}

void virtual_dev_player_stop_all(void)
{
    struct vir_player *player, *n;

    os_mutex_pend(&g_vir_dev_player.mutex, 0);

    list_for_each_entry_safe(player, n, &(g_vir_dev_player.head), entry) {
        __list_del_entry(&player->entry);
        if (player->file) {
            net_buf_inactive(player->file);
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

        virtual_player_free(player);
    }

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"virtual");
    os_mutex_post(&g_vir_dev_player.mutex);
}

struct vir_player *get_virtual_dev_player(void) //返回第一个打开的音乐播放器指针
{
    os_mutex_pend(&g_vir_dev_player.mutex, 0);
    if (list_empty(&(g_vir_dev_player.head))) {
        os_mutex_post(&g_vir_dev_player.mutex);
        return NULL;
    }
    struct vir_player *player = list_first_entry(&(g_vir_dev_player.head), struct vir_player, entry);
    os_mutex_post(&g_vir_dev_player.mutex);
    return player;
}

static int __virtual_player_init(void)
{
    INIT_LIST_HEAD(&g_vir_dev_player.head);
    os_mutex_create(&g_vir_dev_player.mutex);
    return 0;
}
__initcall(__virtual_player_init);


static int virtual_dev_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    int rlen;
    struct vir_player *player = (struct vir_player *)file;

    while (len) {
        if (!player->file) {
            break;
        }

        rlen = net_buf_read(buf + offset, len, player->file);
        if (rlen <= 0) {
            if (rlen == -2) {
                player->read_err  = 1; //file error
            } else {
                if (rlen != 0) {
                    player->read_err = 2;    //disk error
                    return -1; //拔卡读不到数
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

static int virtual_dev_seek(void *file, int offset, int fromwhere)
{
    struct vir_player *player = (struct vir_player *)file;
    return net_buf_seek(offset, fromwhere, player->file);
}

static int virtual_dev_close(void *file)
{
    return 0;
}

static int virtual_dev_get_fmt(void *file, struct stream_fmt *fmt)
{
    struct vir_player *player = (struct vir_player *)file;
    u8 buf[80];

    //需要手动填写解码类型
    fmt->coding_type = player->coding_type;

    if (fmt->coding_type == AUDIO_CODING_SPEEX) {
        virtual_dev_ops.seek(player, 0, SEEK_SET);
        virtual_dev_ops.read(player, buf, 4);

        if ((buf[1] == 0x00) && (buf[3] == 0x00) && (buf[2] == 0x54 || buf[2] == 0x53)) {
            fmt->with_head_data = 1;
        }
        if (fmt->with_head_data) {
            fmt->sample_rate = buf[2] == 0x54 ? 16000 : 8000;
        }
        fmt->channel_mode = AUDIO_CH_MIX;
        fmt->quality = CONFIG_SPEEX_DEC_FILE_QUALITY;
        if (!fmt->with_head_data) {
            fmt->sample_rate = CONFIG_SPEEX_DEC_FILE_SAMPLERATE;
        }
        virtual_dev_ops.seek(player, 0, SEEK_SET);
        return 0;
    }

    if (fmt->coding_type == AUDIO_CODING_OPUS || fmt->coding_type == AUDIO_CODING_STENC_OPUS) {
        fmt->quality = CONFIG_OPUS_DEC_FILE_TYPE;
        if (fmt->quality == AUDIO_ATTR_OPUS_CBR_PKTLEN_TYPE) {
            fmt->opus_pkt_len = CONFIG_OPUS_DEC_PACKET_LEN;
        }
        return -EINVAL;
    }

    if (fmt->coding_type == AUDIO_CODING_PCM) {
        fmt->sample_rate   = CONFIG_PCM_DEC_FILE_SAMPLERATE;
        fmt->channel_mode  = AUDIO_CH_LR;
        fmt->pcm_file_mode = 1;
        return 0;
    }

    return -EINVAL;
}

static const struct stream_file_ops virtual_dev_ops = {
    .read       = virtual_dev_read,
    .seek       = virtual_dev_seek,
    .close      = virtual_dev_close,
    .get_fmt    = virtual_dev_get_fmt,
};


#define VIRTUAL_PLAY_TEST 0

#if VIRTUAL_PLAY_TEST

#include "fs/fs.h"

typedef struct {
    struct vir_player *player;
} vir_music_hdl;

static vir_music_hdl vir_hdl;

#define __this (&vir_hdl)


static void virtual_thread(void *net_buf)
{
    FILE *vfd = fopen("storage/sd0/C/2.mp3", "r");
    /* FILE *vfd = fopen("storage/sd0/C/2.opu", "r"); */
    /* FILE *vfd = fopen("storage/sd0/C/2.pcm", "r"); */
    /* FILE *vfd = fopen("storage/sd0/C/test.spx", "r"); */
    if (!vfd) {
        printf("===virtual_thread vfd open fail!");
    }
    u8 wbuf[512];
    u32 wlen, nwlen;
    while (1) {
        if (vfd) {
            wlen = fread(wbuf, 1, 512, vfd);
            if (wlen == 0) {
                fclose(vfd);
                vfd = NULL;
                //写入完数据需要set end,让read那边把最后剩余数据全部读出
                net_buf_set_file_end(net_buf);
                break;
            }
            nwlen = net_buf_write(wbuf, wlen, net_buf);
        }
    }
}

static int virtual_music_player_decode_event_callback(void *priv, int parm, enum stream_event event)
{
    switch (event) {
    case STREAM_EVENT_START:
        log_info("STREAM_EVENT_START");
        break;
    case STREAM_EVENT_STOP:
        log_info("STREAM_EVENT_STOP");
        virtual_dev_player_stop(__this->player);
        break;
    case STREAM_EVENT_END:
        log_info("STREAM_EVENT_END");
        break;
    default:
        break;
    }

    return 0;
}

//如果是只使用pcm的数据，可去掉net_buf进行管理的过程,在virtual_dev_ops的read中进行喂入数据解码即可
//net_buf管理是为了带格式的音频数据可以进行seek解码使用
void virtual_test()
{
    //初始化net buf
    u32 bufsize = 32 * 1024;
    u8 *net_buf = net_buf_init(&bufsize, NULL);

    net_buf = net_buf_init(&bufsize, NULL);
    if (!net_buf) {
        printf("virtual_test net_buf_init fail");
    }
    net_buf_active(net_buf);
    net_buf_set_time_out(100, net_buf);

    extern int storage_device_ready(void);
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }
    //net_buf_write写入数据
    thread_fork("virtual_thread", 20, 1024, 0, 0, virtual_thread, net_buf);

    os_time_dly(10);

    //虚拟源输入读取net buf数据解码
    __this->player = virtual_dev_play_callback((FILE *)net_buf, &virtual_dev_ops, NULL, virtual_music_player_decode_event_callback, NULL, AUDIO_CODING_MP3);
}

#endif //VIRTUAL_PLAY_TEST
