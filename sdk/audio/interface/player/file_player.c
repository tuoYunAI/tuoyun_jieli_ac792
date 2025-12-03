#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".file_player.data.bss")
#pragma data_seg(".file_player.data")
#pragma const_seg(".file_player.text.const")
#pragma code_seg(".file_player.text")
#endif
#include "file_player.h"
#include "music/music_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "fs/fs.h"
#include "effects/audio_pitchspeed.h"
#include "audio_decoder.h"
#include "jldemuxer.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#include "le_audio_player.h"
#include "scene_switch.h"
#if AUDIO_EQ_LINK_VOLUME
#include "effects/eq_config.h"
#endif
#include "app_config.h"


#if TCFG_APP_MUSIC_EN


#define LOG_TAG             "[MUSIC_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


struct music_file_player_hdl {
    u8 player_id;
    OS_MUTEX mutex;
    struct list_head head;
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
    struct file_player *cur_player; //当前的音乐播放器句柄
#endif
};

static struct music_file_player_hdl g_file_player;

static const struct stream_file_ops music_file_ops;

static void music_player_free(struct file_player *player)
{
    if (--player->ref == 0) {
        if (player->break_point_flag == 1) {
            free(player->break_point);
            player->break_point = NULL;
            player->break_point_flag = 0;
        }
        free(player);
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        if (player == g_file_player.cur_player) {
            g_file_player.cur_player = NULL;
        }
#endif
    }
}

static void music_player_callback(void *_player_id, int event)
{
    struct file_player *player;

    log_info("music_callback: 0x%x, %d", event, (u8)_player_id);

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
        //先判断是否为空防止触发异常
        if (list_empty(&(g_file_player.head))) {
            break;
        }
        os_mutex_pend(&g_file_player.mutex, 0);
        player = list_first_entry(&(g_file_player.head), struct file_player, entry);
        if (player->player_id != (u8)_player_id) {
            os_mutex_post(&g_file_player.mutex);
            log_info("player_id_not_match: %d", player->player_id);
            break;
        }
        os_mutex_post(&g_file_player.mutex);
        if (player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_START);
        }
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        //先判断是否为空防止触发异常
        if (list_empty(&(g_file_player.head))) {
            break;
        }
        os_mutex_pend(&g_file_player.mutex, 0);
        player = list_first_entry(&(g_file_player.head), struct file_player, entry);
        if (player->player_id != (u8)_player_id) {
            os_mutex_post(&g_file_player.mutex);
            log_info("player_id_not_match: %d", player->player_id);
            break;
        }
        os_mutex_post(&g_file_player.mutex);

        if (player->callback) {
            player->callback(player->priv, player->read_err, STREAM_EVENT_STOP);
        }
        break;
    }
}

static int music_file_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    struct file_player *player = (struct file_player *)file;

    while (len) {
        if (!player->file) {
            break;
        }

        int rlen = 0;

#if TCFG_DEC_DECRYPT_ENABLE
        u32 addr;
        addr = ftell(player->file);
        rlen = fread(buf + offset, len, 1, player->file);
        if ((rlen > 0) && (rlen <= len)) {
            cryptanalysis_buff(&player->mply_cipher, buf + offset, addr, rlen); //解密了
        }
#else
        rlen = fread(buf + offset, len, 1, player->file);
#endif

        if (rlen < 0 || rlen == 0) {
            if (rlen == -1) {
                player->read_err  = 1; //file error
            } else {
                if (rlen != 0) {
                    player->read_err  = 2;    //disk error
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

static int music_file_seek(void *file, int offset, int fromwhere)
{
    struct file_player *player = (struct file_player *)file;
    return fseek(player->file, offset, fromwhere);
}

static int music_file_flen(void *file)
{
    struct file_player *player = (struct file_player *)file;
    u32 len = 0;
    if (player->file) {
        len = flen(player->file);
    }
    return len;
}

static int music_file_close(void *file)
{
    struct file_player *player = (struct file_player *)file;

    if (player->file) {
        if (player->priv && player->callback == music_player_decode_event_callback && music_player_get_file_hdl(player->priv)) {
            music_player_remove_file_hdl(player->priv);
            fclose(player->file);
        }
        player->file = NULL;
    }

    return 0;
}

static int music_file_get_fmt(void *file, struct stream_fmt *fmt)
{
    u8 name[16];
    struct file_player *player = (struct file_player *)file;

    fget_name(player->file, name, sizeof(name) - 1);

    struct stream_file_info info = {
        .file = player,
        .fname = (char *)name,
        .ops  = &music_file_ops,
        .scene = player->scene,
    };

    int ret = jldemuxer_get_tone_file_fmt(&info, fmt);

    if (fmt->coding_type == AUDIO_CODING_SPEEX) {
        fmt->quality = CONFIG_SPEEX_DEC_FILE_QUALITY;
        if (!fmt->with_head_data) {
            fmt->sample_rate = CONFIG_SPEEX_DEC_FILE_SAMPLERATE;
        }
    }

    if (fmt->coding_type == AUDIO_CODING_OPUS) {
        fmt->quality = CONFIG_OPUS_DEC_FILE_TYPE;
        if (fmt->quality == AUDIO_ATTR_OPUS_CBR_PKTLEN_TYPE) {
            fmt->opus_pkt_len = CONFIG_OPUS_DEC_PACKET_LEN;
        }
    }

    if (fmt->coding_type == AUDIO_CODING_PCM) {
        fmt->sample_rate    = CONFIG_PCM_DEC_FILE_SAMPLERATE;
        fmt->channel_mode   = AUDIO_CH_MIX;
    }

    return ret;
}

static const struct stream_file_ops music_file_ops = {
    .read       = music_file_read,
    .seek       = music_file_seek,
    .close      = music_file_close,
    .get_fmt    = music_file_get_fmt,
};

int music_file_player_pp(struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        if (music_player->status == FILE_PLAYER_PAUSE) {
#if 0
            //时间戳使能，多设备播放才需要配置，需接入播放同步节点
            u32 timestamp = audio_jiffies_usec() + 30 * 1000;
            jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif
        }


        if (music_player->status == FILE_PLAYER_START) {
            if (music_player->stream->coding_type == AUDIO_CODING_PCM) {
                jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_PARAM, (int)&music_player->pcm_addr);
            }
        }

        jlstream_pp_toggle(music_player->stream, 50);

        if (music_player->status == FILE_PLAYER_PAUSE) {
            if (music_player->stream->coding_type == AUDIO_CODING_PCM) {
                jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_PARAM, music_player->pcm_addr);
            }
        }

        if (music_player->status != FILE_PLAYER_STOP) {
            music_player->status = ((music_player->status == FILE_PLAYER_START) ? FILE_PLAYER_PAUSE : FILE_PLAYER_START);
        }
        return 0;
    } else {
        return -1;
    }
}

int music_file_player_ff(u16 step_s, struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FF, step_s);
    } else {
        return -1;
    }
}

int music_file_player_fr(u16 step_s, struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FR, step_s);
    } else {
        return -1;
    }
}

int music_file_get_cur_time(struct file_player *music_player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {          //先判断是否为空
        os_mutex_post(&g_file_player.mutex);
        return -1;
    }

    if (!music_player) {
        music_player = list_first_entry(&(g_file_player.head), struct file_player, entry);
    }
    if (music_player && music_player->stream) {
        int ret = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_CUR_TIME, 0);
        os_mutex_post(&g_file_player.mutex);
        return ret;
    }
    os_mutex_post(&g_file_player.mutex);

    return -1;
}

int music_file_get_total_time(struct file_player *music_player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {          //先判断是否为空
        os_mutex_post(&g_file_player.mutex);
        return -1;
    }

    if (!music_player) {
        music_player = list_first_entry(&(g_file_player.head), struct file_player, entry);
    }
    if (music_player && music_player->stream) {
        int ret = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_TOTAL_TIME, 0);
        os_mutex_post(&g_file_player.mutex);
        return ret;
    }

    return -1;
}

int music_file_get_player_status(struct file_player *music_player)
{
    enum play_status status = FILE_PLAYER_STOP;  //播放结束

    os_mutex_pend(&g_file_player.mutex, 0);
    if (!(list_empty(&(g_file_player.head))) && music_player && music_player->stream) {
        status = music_player->status;
        os_mutex_post(&g_file_player.mutex);
        return status;
    }
    os_mutex_post(&g_file_player.mutex);

    return status;
}

//变调接口
int music_file_pitch_up(struct file_player *music_player)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (!music_player) {
        return -1;
    }

    if (++music_player->music_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        music_player->music_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }

    log_info("play pitch up+++%d", music_player->music_pitch_mode);

    int err = music_file_set_pitch(music_player, music_player->music_pitch_mode);
    return !err ? music_player->music_pitch_mode : -1;
}

int music_file_pitch_down(struct file_player *music_player)
{
    if (!music_player) {
        return -1;
    }

    if (--music_player->music_pitch_mode < 0) {
        music_player->music_pitch_mode = 0;
    }

    log_info("play pitch down---%d", music_player->music_pitch_mode);

    int err = music_file_set_pitch(music_player, music_player->music_pitch_mode);
    return !err ? music_player->music_pitch_mode : -1;
}

int music_file_set_pitch(struct file_player *music_player, pitch_level_t pitch_mode)
{
    const float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};

    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (music_player) {
        music_player->music_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

//倍速播放接口
int music_file_speed_up(struct file_player *music_player)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (!music_player) {
        return -1;
    }

    if (++music_player->music_speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        music_player->music_speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }

    log_info("play speed up+++%d", music_player->music_speed_mode);

    int err = music_file_set_speed(music_player, music_player->music_speed_mode);
    return !err ? music_player->music_speed_mode : -1;
}

//慢速播放接口
int music_file_speed_down(struct file_player *music_player)
{
    if (!music_player) {
        return -1;
    }

    if (--music_player->music_speed_mode < 0) {
        music_player->music_speed_mode = 0;
    }
    log_info("play speed down---%d", music_player->music_speed_mode);

    int err = music_file_set_speed(music_player, music_player->music_speed_mode);
    return !err ? music_player->music_speed_mode : -1;
}

//设置播放速度
int music_file_set_speed(struct file_player *music_player, speed_level_t speed_mode)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }
    pitch_speed_param_tool_set speed_param = {
        .pitch = 0,
        .speed = speed_param_table[speed_mode],
    };
    if (music_player) {
        log_info("set play speed %d, %d", music_player->music_speed_mode, (int)(speed_param_table[music_player->music_speed_mode] * 100));
        music_player->music_speed_mode = speed_mode;
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&speed_param);
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
static int music_file_ab_repeat_set(int ab_cmd, int ab_mode, struct file_player *music_player)
{
    int err = -1;

    if (!music_player) {
        return err;
    }

    log_info("ab repat, cmd:0x%x, mode:%d", ab_cmd, ab_mode);

    struct audio_ab_repeat_mode_param rpt = {0};
    rpt.value = ab_mode;

    switch (ab_cmd) {
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A:
        log_info("SET BREAKPOINT A");
        err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_A, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B:
        log_info("SET BREAKPOINT B");
        err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_B, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE:
        log_info("CANCEL AB REPEAT");
        err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_AB_REPEAT, (int)&rpt);
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
int music_file_ab_repeat_switch(struct file_player *music_player)
{
    int err = -1;

    if (!music_player) {
        return err;
    }

    switch (music_player->ab_repeat_status) {
    case AB_REPEAT_STA_NON:
        err = music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A, 0, music_player);
        if (!err) {
            music_player->ab_repeat_status = AB_REPEAT_STA_ASTA;
        }
        break;
    case AB_REPEAT_STA_ASTA:
        err = music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, music_player);
        if (!err) {
            music_player->ab_repeat_status = AB_REPEAT_STA_BSTA;
        }
        break;
    case AB_REPEAT_STA_BSTA:
        err = music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, music_player);
        if (!err) {
            music_player->ab_repeat_status = AB_REPEAT_STA_NON;
        }
        break;
    }

    log_info("music_file_ab_repeat_switch = %d", music_player->ab_repeat_status);

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭AB点复读
   @param
   @return   0: 设置成功 -1:设置失败
   @note
*/
/*----------------------------------------------------------------------------*/
int music_file_ab_repeat_close(struct file_player *music_player)
{
    int err = -1;

    if (!music_player) {
        return err;
    }

    if (music_player->ab_repeat_status == AB_REPEAT_STA_NON) {
        return 0;
    }

    if (music_player->ab_repeat_status == AB_REPEAT_STA_ASTA) {
        struct stream_fmt fmt;
        jlstream_node_ioctl(music_player->stream, NODE_UUID_SOURCE, NODE_IOC_GET_FMT, (int)&fmt);

        switch (fmt.coding_type) {
        case AUDIO_CODING_FLAC:
        case AUDIO_CODING_DTS:
        case AUDIO_CODING_APE:
            music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, music_player);
            break;
        }
    }

    err = music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, music_player);
    music_player->ab_repeat_status = AB_REPEAT_STA_NON;

    return err;
}

#endif /*FILE_DEC_AB_REPEAT_EN*/

//获取解码文件的码率,采样率和解码格式
int music_file_get_fmt_api(struct file_player *music_player, struct stream_fmt *fmt)
{
    if (!music_player || !fmt) {
        return -1;
    }
    int err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_PRIV_FMT, (int)fmt);
    //printf("coding_type:%x,sample_rate:%d,bit_rate:%d kb/s\n",fmt->coding_type,fmt->sample_rate,fmt->bit_rate);
    return err;
}

int music_file_get_breakpoints(struct audio_dec_breakpoint *bp, struct file_player *music_player)
{
    if (music_player) {
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_BP, (int)bp);
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
    struct file_player *music_player = priv;

    log_info("file_dec_repeat_cb");

    if (music_player->repeat_num) {
        music_player->repeat_num--;
    } else {
        log_info("file_dec_repeat_cb end");
        return -1;
    }

    return 0;
}

int file_dec_repeat_set(struct file_player *music_player, u8 repeat_num)
{
    if (!music_player) {
        return -1;
    }

    struct audio_repeat_mode_param rep = {0};

    switch (music_player->stream->coding_type) {
    case AUDIO_CODING_MP3:
    case AUDIO_CODING_WAV:
        music_player->repeat_num = repeat_num;
        /* struct audio_repeat_mode_param rep = {0}; */
        rep.flag = 1; //使能
        rep.headcut_frame = 2; //依据需求砍掉前面几帧，仅mp3格式有效
        rep.tailcut_frame = 2; //依据需求砍掉后面几帧，仅mp3格式有效
        rep.repeat_callback = file_dec_repeat_cb;
        rep.callback_priv = music_player;
        rep.repair_buf = &music_player->repair_buf;
        jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_REPEAT, (int)&rep);
        return 0;
    }

    return -1;
}
#endif

#if FILE_DEC_DEST_PLAY
int file_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct file_player *music_player)
{
    if (!music_player) {
        return -1;
    }

    struct audio_dest_time_play_param param = {0};

    switch (coding_type) {
    case AUDIO_CODING_MP3:
    case AUDIO_CODING_WAV:
        param.start_time = start_time;
        param.dest_time = dest_time;
        param.callback_func = cb;
        param.callback_priv = cb_priv;
        jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_DEST_PLAY, (int)&param);
        return 0;
    }

    return -1;
}

int file_dec_set_start_play(u32 start_time, u32 coding_type)
{
    return file_dec_set_start_dest_play(start_time, 0x7fffffff, NULL, NULL, g_file_player.cur_player->stream->coding_type, g_file_player.cur_player);
}
#endif


#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
static int file_name_match_mp3(const char *file_name)
{
    const char *ext_name;
    for (int i = 0; file_name[i] != '\0'; i++) {
        if (file_name[i] == '.') {
            ext_name = file_name + i + 1;
            goto __match;
        }
    }
    return 0;
__match:
    if (!strncasecmp(ext_name, "mp3", 3)) {
        return 1;
    }
    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    获取id3信息
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int file_dec_id3_post(struct file_player *player)
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
    if ((ret == 0) && (info.data.len_list)) {
        for (int i = 0; i < info.set.n_items; i++) {
            if ((info.data.len_list[i] != 0) && info.data.mptr[i] != 0) {
                printf("%d\t :  %s\n", i, info.data.mptr[i]);        //显示字符串.
                put_buf(info.data.mptr[i], info.data.len_list[i]);  //HEX
            }
        }
    }
    return true;
}

int music_file_player_start(struct file_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_MUSIC);
    if (!player->stream) {
        goto __exit0;
    }

#if TCFG_USER_EMITTER_ENABLE
    extern u8 *get_cur_connect_emitter_mac_addr(void);
    u8 *bt_addr = get_cur_connect_emitter_mac_addr();
    if (bt_addr) {
        jlstream_node_ioctl(player->stream, NODE_UUID_A2DP_TX, NODE_IOC_SET_BTADDR, (int)bt_addr);
    }
#endif

    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, music_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, (int)music_file_flen(player));

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

    jlstream_set_dec_file(player->stream, player, &music_file_ops);

#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
    u8 mp3_flag = 0;
    u8 name[12 + 1] = {0}; //8.3+\0
    fget_name(player->file, name, sizeof(name));
    if (file_name_match_mp3((const char *)name)) {
        mp3_flag = 1;
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
#endif
    err = jlstream_start(player->stream);

#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
    if (!mp3_flag) {
        file_dec_id3_post(player);
    }
#endif

    if (err) {
        goto __exit1;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);

    if (player->callback) {
        /* err = player->callback(player->priv, 0,STREAM_EVENT_NONE); */
    }
    music_player_free(player);

    return err;
}

static int music_player_init(struct file_player *player, void *file, struct audio_dec_breakpoint *dbp)
{
    player->ref         = 1;
    player->file        = file;
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
    cipher_check_decode_file(&player->mply_cipher, file);
#endif

    INIT_LIST_HEAD(&player->entry);

    return 0;
}

static struct file_player *music_player_create(void *file, struct audio_dec_breakpoint *dbp)
{
    struct file_player *player;

    if (!file) {
        return NULL;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    music_player_init(player, file, dbp);

    return player;
}

struct file_player *music_player_add(struct file_player *player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        int err = music_file_player_start(player);
        if (err) {
            os_mutex_post(&g_file_player.mutex);
            return NULL;
        }
        player->status = FILE_PLAYER_START;
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        g_file_player.cur_player = player;
#endif
    }
    list_add_tail(&player->entry, &(g_file_player.head));
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct file_player *music_file_play(FILE *file, struct audio_dec_breakpoint *dbp)
{
    struct file_player *player;
    player = music_player_create(file, dbp);
    if (!player) {
        return NULL;
    }
    return music_player_add(player);
}

struct file_player *music_file_play_callback(FILE *file, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp)
{
    struct file_player *player;
    player = music_player_create(file, dbp);
    if (!player) {
        return NULL;
    }
    player->priv        = priv;
    player->callback    = callback;
    return music_player_add(player);
}

int music_player_runing(void)
{
    return !list_empty(&(g_file_player.head));
}

void music_file_player_stop(struct file_player *player)
{
    if (!player) {
        return;
    }

    os_mutex_pend(&g_file_player.mutex, 0);
    __list_del_entry(&player->entry);
    os_mutex_post(&g_file_player.mutex);

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

    music_player_free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"music");
}

void music_file_player_stop_all(void)
{
    struct file_player *player, *n;

    os_mutex_pend(&g_file_player.mutex, 0);

    list_for_each_entry_safe(player, n, &(g_file_player.head), entry) {
        __list_del_entry(&player->entry);
#if TCFG_LE_AUDIO_STREAM_ENABLE && TCFG_LEA_LOCAL_SYNC_PLAY_EN
        if (player->le_audio) {
            le_audio_player_close(player->le_audio);
        }
#endif
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

        music_player_free(player);
    }

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"music");
    os_mutex_post(&g_file_player.mutex);
}

struct file_player *get_music_file_player(void) //返回第一个打开的音乐播放器指针
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        os_mutex_post(&g_file_player.mutex);
        return NULL;
    }
    struct file_player *player = list_first_entry(&(g_file_player.head), struct file_player, entry);
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct midi_player *midi_ctrl_player_open(void)
{
    struct midi_player *player = zalloc(sizeof(struct midi_player));
    if (!player) {
        return NULL;
    }

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music");
    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ZERO_ACTIVE);
    if (!player->stream) {
        goto __exit0;
    }

    jlstream_set_callback(player->stream, NULL, NULL);
    jlstream_set_scene(player->stream, STREAM_SCENE_MIDI);
    jlstream_set_coexist(player->stream, 0);

    int err = jlstream_start(player->stream);
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

void midi_ctrl_player_close(struct midi_player *player)
{
    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);
    free(player);
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"music");
}

static int __music_player_init(void)
{
    INIT_LIST_HEAD(&g_file_player.head);
    os_mutex_create(&g_file_player.mutex);
    return 0;
}
__initcall(__music_player_init);


#if TCFG_LE_AUDIO_STREAM_ENABLE
static int le_audio_music_file_player_start(struct file_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music_le_audio");
    /* u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music"); */

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_MUSIC);
    if (!player->stream) {
        goto __exit0;
    }
    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, music_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, (int)music_file_flen(player));
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                        NODE_IOC_SET_TASK, (int)"music_file");
    jlstream_node_ioctl(player->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)player->le_audio);
    jlstream_ioctl(player->stream, NODE_IOC_SET_ENC_FMT, (int)player->fmt);
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

    jlstream_set_dec_file(player->stream, player, &music_file_ops);
#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
    u8 name[12 + 1] = {0}; //8.3+\0
    fget_name(player->file, name, sizeof(name));
    if (file_name_match_mp3((char *)name)) {
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
#endif
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    if (player->callback) {
        /* err = player->callback(player->priv, 0,STREAM_EVENT_NONE); */
    }
    music_player_free(player);
    return err;
}

static struct file_player *le_audio_music_player_add(struct file_player *player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        int err = le_audio_music_file_player_start(player);
        if (err) {
            os_mutex_post(&g_file_player.mutex);
            return NULL;
        }
#if TCFG_LEA_LOCAL_SYNC_PLAY_EN
        if (player->le_audio) {
            err = le_audio_player_open(player->le_audio, NULL);
            if (err != 0) {
                ASSERT(0, "player open fail");
            }
        }
#endif
        player->status = FILE_PLAYER_START;
    }
    list_add_tail(&player->entry, &(g_file_player.head));
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct file_player *le_audio_music_file_play_callback(FILE *file, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp, void *le_audio, void *fmt)
{
    struct file_player *player;
    player = music_player_create(file, dbp);
    if (!player) {
        return NULL;
    }
    player->priv        = priv;
    player->callback    = callback;
    player->le_audio    = le_audio;
    player->fmt         = fmt;
    return le_audio_music_player_add(player);
}
#endif

#endif
