#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".avi_audio_player.data.bss")
#pragma data_seg(".avi_audio_player.data")
#pragma const_seg(".avi_audio_player.text.const")
#pragma code_seg(".avi_audio_player.text")
#endif
#include "jlstream.h"
#include "audio_config_def.h"
#include "avi_audio_player.h"

struct avi_audio_player {
    struct jlstream *stream;
    s8 music_speed_mode; //播放倍速
};

static struct avi_audio_player *g_avi_audio_player = NULL;

static void avi_audio_player_callback(void *private_data, int event)
{
    struct avi_audio_player *player = (struct avi_audio_player *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int avi_audio_player_pp(void)
{
    if (g_avi_audio_player && g_avi_audio_player->stream) {
        jlstream_pp_toggle(g_avi_audio_player->stream, 50);
        return 0;
    } else {
        return -1;
    }
}

//自定义数据流播放器 启动
int avi_audio_player_open(u32 code_type, u8 ch_mode, u32 sr, u8 * (*get_data_cb)(u32 *))
{
    int err;
    struct avi_audio_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"avi_audio");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    /*
       导入自定义音频流的源节点UUID，如下自定义源节点为NODE_UUID_SOURCE_DEV,
       若是其他源节点，需使用对应节点的UUID，如ADC节点则为NODE_UUID_ADC
     */
    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SOURCE_DEV4);
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置当前数据对应节点的特性
    struct stream_fmt fmt = {
        .channel_mode = ch_mode,
        .sample_rate = sr,
        .coding_type = code_type,
    };

    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, (int)&fmt);
    //设置获取音频数据回调
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PARAM, (int)get_data_cb);
    jlstream_set_callback(player->stream, player, avi_audio_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_DEV_FLOW);

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    g_avi_audio_player = player;

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

//自定义数据流播放器 查询是否活动
bool avi_audio_player_runing(void)
{
    return g_avi_audio_player != NULL;
}

//自定义数据流播放器 关闭
void avi_audio_player_close(void)
{
    struct avi_audio_player *player = g_avi_audio_player;

    if (!player) {
        return;
    }
    g_avi_audio_player = NULL;
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);
    free(player);

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"avi_audio");
}
