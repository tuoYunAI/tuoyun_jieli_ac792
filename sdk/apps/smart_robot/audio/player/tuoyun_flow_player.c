#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tuoyun_player.data.bss")
#pragma data_seg(".tuoyun_player.data")
#pragma const_seg(".tuoyun_player.text.const")
#pragma code_seg(".tuoyun_player.text")
#endif

#include "generic/typedef.h"
#include "jlstream.h"
#include "music/music_decrypt.h"

#include "tuoyun_flow_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "fs/fs.h"
#include "jldemuxer.h"
#include "audio_config_def.h"
#include "app_config.h"
#include "network_download/net_audio_buf.h"
#include "file_player.h"
#include "volume_node.h"

#include "syscfg/syscfg_id.h"
#include "app_mcp_server.h"

#include "json_c/json_object.h"
#include "json_c/json_tokener.h"

#define LOG_TAG             "[PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

#define NODE_VOLUME_FLOW     "Vol_FlowMusic" 


typedef int (*music_player_cb_t)(void *, int parm, enum stream_event);

typedef enum play_status play_status_enum;

typedef struct tuoyun_player {
    play_status_enum status;    //播放状态
    enum stream_scene scene;
    enum stream_coexist coexist;

    u32 coding_type;
    struct jlstream *stream;
    void *file;

    void *priv;
    music_player_cb_t callback;

    CIPHER mply_cipher;		// 解密播放
    s8 music_speed_mode; //播放倍速
    s8 music_pitch_mode; //变调模式
}tuoyun_player_t, *tuoyun_player_ptr;


typedef struct {
    OS_MUTEX mutex;
    tuoyun_player_ptr player;
} tuoyun_flow_player_hdl_t;

static tuoyun_flow_player_hdl_t tuoyun_flow_player_hdl;

#define __this (&tuoyun_flow_player_hdl)

static const struct stream_file_ops virtual_dev_ops;
static s16 m_volume = 50;


#define CACHE_BUF_LEN (500 * 1024)
static cbuffer_t g_audio_cache_cbuf = {0};


static void tuoyuan_player_free(tuoyun_player_ptr player){
    free(player);
}

static void tuoyun_player_callback(void *priv, int event){

    tuoyun_player_ptr player = (tuoyun_player_ptr)priv;

    log_info("tuoyun_player_callback: 0x%x, 0x%x", event, player);

    switch (event) {
    case STREAM_EVENT_START:
        os_mutex_pend(&__this->mutex, 0);
        if (__this->player == NULL) {
            os_mutex_post(&__this->mutex);
            break;  
        }

        if (player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_START);
        }
        os_mutex_post(&__this->mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        //先判断是否为空防止触发异常
        os_mutex_pend(&__this->mutex, 0);
        if (__this->player == NULL) {
            os_mutex_post(&__this->mutex);
            break;
        }

        if (event == STREAM_EVENT_STOP && player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_END);
        }

        if (player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_STOP);
        }

        os_mutex_post(&__this->mutex);
        break;
    }
}



int tuoyun_player_get_status()
{
    play_status_enum status = FILE_PLAYER_STOP;  //播放结束

    os_mutex_pend(&__this->mutex, 0);
    if (__this->player != NULL) {
        status = __this->player->status;
        os_mutex_post(&__this->mutex);
        return status;
    }
    os_mutex_post(&__this->mutex);

    return status;
}


int start_player(tuoyun_player_ptr player, const struct stream_file_ops *ops)
{
    int err = -EINVAL;
    if (ops == NULL) {
        ops = &virtual_dev_ops;
    }

    

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"virtual");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_VIR_DATA_RX);
    if (!player->stream) {
        goto __exit0;
    }
    jlstream_set_callback(player->stream, player, tuoyun_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_FILE_LEN, -1);

    if (player->callback) {
        err = player->callback(player->priv, 0, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }

    jlstream_set_dec_file(player->stream, player, ops);


    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    __this->player = player;

    log_info("tuoyun player init success");
    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    tuoyuan_player_free(player);

    return err;
}

static tuoyun_player_ptr create_player(void *file, u32 coding_type)
{
    tuoyun_player_ptr player;

    if (!file) {
        log_info("create_player err");
        return NULL;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    player->file        = file;
    player->scene       = STREAM_SCENE_DEV_FLOW;
    player->coexist     = STREAM_COEXIST_AUTO;

    player->coding_type = coding_type;
    return player;
}

static void player_dev_stop()
{

    os_mutex_pend(&__this->mutex, 0);
    if (!__this->player) {
        os_mutex_post(&__this->mutex);
        return;
    }

    if (__this->player->stream) {
        jlstream_stop(__this->player->stream, 50);
        jlstream_release(__this->player->stream);
    }
    
    tuoyuan_player_free(__this->player);
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"virtual");
    __this->player = NULL;
    os_mutex_post(&__this->mutex);
}

static void player_interrupted(void *priv) {
    os_mutex_pend(&__this->mutex, 0);
    if(__this->player && __this->player->priv){
        player_decode_event_callback(__this->player->priv, 0, STREAM_EVENT_STOP);
    }
    
    os_mutex_post(&__this->mutex);
}   

static int player_dev_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    int rlen = 0;
    int wait = 0;
    while (len) {
        int blen = cbuf_get_data_size(&g_audio_cache_cbuf);
        if(blen == 0){
            break;
        }   
        rlen = cbuf_read(&g_audio_cache_cbuf, buf + offset, len);
        if (rlen <= 0) {
            break;
        }
        offset += rlen;
        if ((len -= rlen) == 0) {
            break;
        }
    }

    if (rlen == 0 && offset == 0) {
        log_info("@@@@@@@++++++++++++@@@@@@@@@@player_dev_read no data");        
        thread_fork("stop_player", 20, 1024, 0, 0, player_interrupted, NULL);
        return 0;
    }
    //log_info("++++++++++++player_dev_read resutl: rlen=%d, Offset: %d", rlen, offset);
    return offset;
}

static int player_dev_seek(void *file, int offset, int fromwhere)
{
    log_info("player_dev_seek offset: %d, fromwhere: %d", offset, fromwhere);
    //tuoyun_player_ptr player = (tuoyun_player_ptr)file;
    return  1 ;//net_buf_seek(offset, fromwhere, player->file);
}

static int player_dev_close(void *file)
{
    //log_info("@@@@player_dev_close");
    return 0;
}

static int player_dev_get_fmt(void *file, struct stream_fmt *fmt)
{
    tuoyun_player_ptr player = (tuoyun_player_ptr)file;
    u8 buf[80];

    //需要手动填写解码类型
    fmt->coding_type = player->coding_type;
    //fmt->sample_rate = 24000;   
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
    .read       = player_dev_read,
    .seek       = player_dev_seek,
    .close      = player_dev_close,
    .get_fmt    = player_dev_get_fmt,
};


static int player_decode_event_callback(void *priv, int parm, enum stream_event event)
{

    if (priv) {
        player_event_callback_fn callback = (player_event_callback_fn)priv;
        callback(event);
    }

    switch (event) {
    case STREAM_EVENT_INIT:
        log_info("STREAM_EVENT_INIT");
        break;
    case STREAM_EVENT_START:
        log_info("STREAM_EVENT_START");
        break;
    case STREAM_EVENT_STOP:
        log_info("STREAM_EVENT_STOP");
        player_dev_stop(__this->player);
        break;
    case STREAM_EVENT_END:
        log_info("STREAM_EVENT_END");
        break;
    default:
        break;
    }

    return 0;
}




int tuoyun_audio_write_data(void *buf, int len)
{
    u8 head[8] = {0};
    u32 pkt_len = len;
    head[0] = (u8)((pkt_len >> 24) & 0xff);
    head[1] = (u8)((pkt_len >> 16) & 0xff);
    head[2] = (u8)((pkt_len >> 8) & 0xff);
    head[3] = (u8)(pkt_len & 0xff);
    
    u32 wlen = cbuf_write(&g_audio_cache_cbuf, head, sizeof(head));
    if (wlen == 0) {
        log_info("tuoyun_audio_write_data head cbuf full");
        cbuf_clear(&g_audio_cache_cbuf);
        return 0;
    }
    wlen = cbuf_write(&g_audio_cache_cbuf, buf, len);
    if (wlen == 0) {
        log_info("tuoyun_audio_write_data cbuf full");
        cbuf_clear(&g_audio_cache_cbuf);
    }
    //log_info("tuoyun_audio_write_data write len: %d", wlen+sizeof(head));
    return wlen;
}


void tuoyun_audio_player_start(player_event_callback_fn callback)
{
    os_mutex_pend(&__this->mutex, 0);

    if (__this->player != NULL) {
        log_info("tuoyun_audio_player_start err, player exist");
        os_mutex_post(&__this->mutex);
        return;
    }
    
    tuoyun_player_ptr player  = create_player((FILE *)&g_audio_cache_cbuf, AUDIO_CODING_OPUS);
    if (!player) {
        log_info("virtual_dev_play_callback err");
        os_mutex_post(&__this->mutex);
        return;
    }
    player->priv        = callback;
    player->callback    = player_decode_event_callback;
    start_player(player, &virtual_dev_ops);

    log_info("start the tuoyun player");
    if (m_volume > 0){
        tuoyun_audio_player_set_volume(m_volume);
    }

    os_mutex_post(&__this->mutex);

}

void tuoyun_audio_player_clear(void)
{
    player_dev_stop();
    cbuf_clear(&g_audio_cache_cbuf);
    log_info("clear the tuoyun player");
}


void tuoyun_audio_player_set_volume(s16 volume){   

    if (volume > 100){
        volume = 100;
    }
    m_volume = volume;
    char vol_name[32]={0 };
    int err1;
    struct volume_cfg cfg={0};;

    int err=jlstream_get_node_param(NODE_UUID_VOLUME_CTRLER, NODE_VOLUME_FLOW, &cfg,    sizeof(struct volume_cfg));
    if(!err){
        log_info("audio_digital_vol_node_name_get err:%x", err);
        return;
    }

    if(cfg.cur_vol == volume){
        log_info("volume no change : %d, %d", volume, cfg.cur_vol);
        return;
    }

    cfg.cur_vol=volume;
    cfg.bypass=VOLUME_NODE_CMD_SET_VOL;;

    err1=jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, NODE_VOLUME_FLOW, &cfg, sizeof(struct volume_cfg));

    if(!err1){
        log_error("audio_digital_vol_node_name_set err:%x", err1);
    }
    log_info("volume set : %d", volume);

    s16 save_music_volume = (s16)volume;
    syscfg_write(CFG_MUSIC_VOL, &save_music_volume, 2);

    return;
}

int tuoyun_audio_player_get_volume(){   

    os_mutex_pend(&__this->mutex, 0);
    if(__this->player == NULL) {
        os_mutex_post(&__this->mutex);
        return m_volume;
    }
    os_mutex_post(&__this->mutex);
    
    char vol_name[32]={0 };
    int err1;
    struct volume_cfg cfg={0};;
    log_info("tuoyun_audio_player_get_volume m_volume:%d", m_volume);

    int err=jlstream_get_node_param(NODE_UUID_VOLUME_CTRLER, NODE_VOLUME_FLOW, &cfg,    sizeof(struct volume_cfg));
    if(!err){
        log_info("audio_digital_vol_node_name_get err:%x", err);
        return m_volume;
    }
    log_info("volume get : %d", cfg.cur_vol);
    if (cfg.cur_vol == 0){
        return m_volume;
    }
    m_volume = (int)cfg.cur_vol;
    return cfg.cur_vol;
}

json_object* mcp_call_set_volume(property_ptr props, size_t len) {
    if (len != 1 || strcmp(props[0].name, "volume") != 0) {
        return NULL; // Invalid parameters
    }

    int volume = props[0].value.int_value;
    tuoyun_audio_player_set_volume((s16)volume);
    log_info("MCP set_volume called with volume: %d", volume);
    char buf[32];
    snprintf(buf, sizeof(buf), "Volume set to %d", volume);
    json_object *result = json_object_new_object();
    json_object *type = json_object_new_string("text");
    json_object *text = json_object_new_string(buf);

    json_object_object_add(result, "type", type);
    json_object_object_add(result, "text", text);
    return result;
}

static int __player_init(void)
{
    os_mutex_create(&__this->mutex);
    log_info("tuoyun_player_init\n");

    u8* buf = malloc(CACHE_BUF_LEN);
    if (!buf) {
        log_info(" tuoyun_player_init malloc fail");
        return -1;
    }
    
    cbuf_init(&g_audio_cache_cbuf, buf, CACHE_BUF_LEN);

    s16 vol = 0;
    int ret = syscfg_read(CFG_MUSIC_VOL, &vol, 2);
    if (ret > 0) {
        m_volume = vol;
    }

    property_t volume_property = {
        .name = "volume",
        .type = PROPERTY_TYPE_INTEGER,
        .required = 1,
        .max_int_value = 100,
        .min_int_value = 0,
        .description = "Volume level from 0 to 100"
    };
    
    add_mcp_tool("set_volume", 
        "Set the volume of the audio speaker. If the current volume is unknown, you must call `get_device_status` tool first and then call this tool.",
        mcp_call_set_volume,
        &volume_property, 
        1);

    return 0;
}

late_initcall(__player_init);

