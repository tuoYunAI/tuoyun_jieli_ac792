#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dev_flow_player.data.bss")
#pragma data_seg(".dev_flow_player.data")
#pragma const_seg(".dev_flow_player.text.const")
#pragma code_seg(".dev_flow_player.text")
#endif
#include "jlstream.h"
#include "sdk_config.h"
#include "audio_config_def.h"
#include "dev_flow_player.h"
#include "effects/audio_pitchspeed.h"

struct dev_flow_player {
    struct jlstream *stream;
    s8 music_speed_mode; //播放倍速
};

static struct dev_flow_player *g_dev_flow_player = NULL;


static void dev_flow_player_callback(void *private_data, int event)
{
    struct dev_flow_player *player = g_dev_flow_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

//设置播放速度
int flow_set_speed(u8 speed_mode)
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }
    pitch_speed_param_tool_set speed_param = {
        .pitch = 0,
        .speed = speed_param_table[speed_mode],
    };
    if (g_dev_flow_player) {
        g_dev_flow_player->music_speed_mode = speed_mode;
        printf("set play speed %d, %d", g_dev_flow_player->music_speed_mode, (int)(speed_param_table[g_dev_flow_player->music_speed_mode] * 100));
        return jlstream_node_ioctl(g_dev_flow_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&speed_param);
    }
    return -1;
}

//倍速播放接口
int flow_speed_up()
{
    const float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (!g_dev_flow_player) {
        return -1;
    }

    if (++g_dev_flow_player->music_speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        g_dev_flow_player->music_speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }

    printf("play speed up+++%d", g_dev_flow_player->music_speed_mode);

    int err = flow_set_speed(g_dev_flow_player->music_speed_mode);
    return !err ? g_dev_flow_player->music_speed_mode : -1;
}

//慢速播放接口
int flow_speed_down()
{
    if (!g_dev_flow_player) {
        return -1;
    }

    if (--g_dev_flow_player->music_speed_mode < 0) {
        g_dev_flow_player->music_speed_mode = 0;
    }
    printf("play speed down---%d", g_dev_flow_player->music_speed_mode);

    int err = flow_set_speed(g_dev_flow_player->music_speed_mode);
    return !err ? g_dev_flow_player->music_speed_mode : -1;
}



int dev_flow_player_pp()
{
    if (g_dev_flow_player && g_dev_flow_player->stream) {
        jlstream_pp_toggle(g_dev_flow_player->stream, 50);
        return 0;
    } else {
        return -1;
    }
}

//自定义数据流播放器 启动
int dev_flow_player_open(u32 code_type, u8 ch_mode, u32 sr)
{
    int err;
    struct dev_flow_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"dev_flow");
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
    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SOURCE_DEV0);
    /* player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SOURCE_DEV1); */

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

    jlstream_set_callback(player->stream, player->stream, dev_flow_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_DEV_FLOW);
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    g_dev_flow_player = player;

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

//自定义数据流播放器 查询是否活动
bool dev_flow_player_runing(void)
{
    return g_dev_flow_player != NULL;
}


//自定义数据流播放器 关闭
void dev_flow_player_close(void)
{
    struct dev_flow_player *player = g_dev_flow_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);
    g_dev_flow_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"dev_flow");
}





#define PCM_FLOW_TEST 0
#if PCM_FLOW_TEST

static const s16 test1_data_sin44100[441] = {
    0x0000, 0x122d, 0x23fb, 0x350f, 0x450f, 0x53aa, 0x6092, 0x6b85, 0x744b, 0x7ab5, 0x7ea2, 0x7fff, 0x7ec3, 0x7af6, 0x74ab, 0x6c03,
    0x612a, 0x545a, 0x45d4, 0x35e3, 0x24db, 0x1314, 0x00e9, 0xeeba, 0xdce5, 0xcbc6, 0xbbb6, 0xad08, 0xa008, 0x94fa, 0x8c18, 0x858f,
    0x8181, 0x8003, 0x811d, 0x84ca, 0x8af5, 0x9380, 0x9e3e, 0xaaf7, 0xb969, 0xc94a, 0xda46, 0xec06, 0xfe2d, 0x105e, 0x223a, 0x3365,
    0x4385, 0x5246, 0x5f5d, 0x6a85, 0x7384, 0x7a2d, 0x7e5b, 0x7ffa, 0x7f01, 0x7b75, 0x7568, 0x6cfb, 0x6258, 0x55b7, 0x4759, 0x3789,
    0x2699, 0x14e1, 0x02bc, 0xf089, 0xdea7, 0xcd71, 0xbd42, 0xae6d, 0xa13f, 0x95fd, 0x8ce1, 0x861a, 0x81cb, 0x800b, 0x80e3, 0x844e,
    0x8a3c, 0x928c, 0x9d13, 0xa99c, 0xb7e6, 0xc7a5, 0xd889, 0xea39, 0xfc5a, 0x0e8f, 0x2077, 0x31b8, 0x41f6, 0x50de, 0x5e23, 0x697f,
    0x72b8, 0x799e, 0x7e0d, 0x7fee, 0x7f37, 0x7bed, 0x761f, 0x6ded, 0x6380, 0x570f, 0x48db, 0x392c, 0x2855, 0x16ad, 0x048f, 0xf259,
    0xe06b, 0xcf20, 0xbed2, 0xafd7, 0xa27c, 0x9705, 0x8db0, 0x86ab, 0x821c, 0x801a, 0x80b0, 0x83da, 0x8988, 0x919c, 0x9bee, 0xa846,
    0xb666, 0xc603, 0xd6ce, 0xe86e, 0xfa88, 0x0cbf, 0x1eb3, 0x3008, 0x4064, 0x4f73, 0x5ce4, 0x6874, 0x71e6, 0x790a, 0x7db9, 0x7fdc,
    0x7f68, 0x7c5e, 0x76d0, 0x6ed9, 0x64a3, 0x5863, 0x4a59, 0x3acc, 0x2a0f, 0x1878, 0x0661, 0xf42a, 0xe230, 0xd0d0, 0xc066, 0xb145,
    0xa3bd, 0x9813, 0x8e85, 0x8743, 0x8274, 0x8030, 0x8083, 0x836b, 0x88da, 0x90b3, 0x9acd, 0xa6f5, 0xb4ea, 0xc465, 0xd515, 0xe6a3,
    0xf8b6, 0x0aee, 0x1ced, 0x2e56, 0x3ecf, 0x4e02, 0x5ba1, 0x6764, 0x710e, 0x786f, 0x7d5e, 0x7fc3, 0x7f91, 0x7cc9, 0x777a, 0x6fc0,
    0x65c1, 0x59b3, 0x4bd3, 0x3c6a, 0x2bc7, 0x1a41, 0x0833, 0xf5fb, 0xe3f6, 0xd283, 0xc1fc, 0xb2b7, 0xa503, 0x9926, 0x8f60, 0x87e1,
    0x82d2, 0x804c, 0x805d, 0x8303, 0x8833, 0x8fcf, 0x99b2, 0xa5a8, 0xb372, 0xc2c9, 0xd35e, 0xe4da, 0xf6e4, 0x091c, 0x1b26, 0x2ca2,
    0x3d37, 0x4c8e, 0x5a58, 0x664e, 0x7031, 0x77cd, 0x7cfd, 0x7fa3, 0x7fb4, 0x7d2e, 0x781f, 0x70a0, 0x66da, 0x5afd, 0x4d49, 0x3e04,
    0x2d7d, 0x1c0a, 0x0a05, 0xf7cd, 0xe5bf, 0xd439, 0xc396, 0xb42d, 0xa64d, 0x9a3f, 0x9040, 0x8886, 0x8337, 0x806f, 0x803d, 0x82a2,
    0x8791, 0x8ef2, 0x989c, 0xa45f, 0xb1fe, 0xc131, 0xd1aa, 0xe313, 0xf512, 0x074a, 0x195d, 0x2aeb, 0x3b9b, 0x4b16, 0x590b, 0x6533,
    0x6f4d, 0x7726, 0x7c95, 0x7f7d, 0x7fd0, 0x7d8c, 0x78bd, 0x717b, 0x67ed, 0x5c43, 0x4ebb, 0x3f9a, 0x2f30, 0x1dd0, 0x0bd6, 0xf99f,
    0xe788, 0xd5f1, 0xc534, 0xb5a7, 0xa79d, 0x9b5d, 0x9127, 0x8930, 0x83a2, 0x8098, 0x8024, 0x8247, 0x86f6, 0x8e1a, 0x978c, 0xa31c,
    0xb08d, 0xbf9c, 0xcff8, 0xe14d, 0xf341, 0x0578, 0x1792, 0x2932, 0x39fd, 0x499a, 0x57ba, 0x6412, 0x6e64, 0x7678, 0x7c26, 0x7f50,
    0x7fe6, 0x7de4, 0x7955, 0x7250, 0x68fb, 0x5d84, 0x5029, 0x412e, 0x30e0, 0x1f95, 0x0da7, 0xfb71, 0xe953, 0xd7ab, 0xc6d4, 0xb725,
    0xa8f1, 0x9c80, 0x9213, 0x89e1, 0x8413, 0x80c9, 0x8012, 0x81f3, 0x8662, 0x8d48, 0x9681, 0xa1dd, 0xaf22, 0xbe0a, 0xce48, 0xdf89,
    0xf171, 0x03a6, 0x15c7, 0x2777, 0x385b, 0x481a, 0x5664, 0x62ed, 0x6d74, 0x75c4, 0x7bb2, 0x7f1d, 0x7ff5, 0x7e35, 0x79e6, 0x731f,
    0x6a03, 0x5ec1, 0x5193, 0x42be, 0x328f, 0x2159, 0x0f77, 0xfd44, 0xeb1f, 0xd967, 0xc877, 0xb8a7, 0xaa49, 0x9da8, 0x9305, 0x8a98,
    0x848b, 0x80ff, 0x8006, 0x81a5, 0x85d3, 0x8c7c, 0x957b, 0xa0a3, 0xadba, 0xbc7b, 0xcc9b, 0xddc6, 0xefa2, 0x01d3, 0x13fa, 0x25ba,
    0x36b6, 0x4697, 0x5509, 0x61c2, 0x6c80, 0x750b, 0x7b36, 0x7ee3, 0x7ffd, 0x7e7f, 0x7a71, 0x73e8, 0x6b06, 0x5ff8, 0x52f8, 0x444a,
    0x343a, 0x231b, 0x1146, 0xff17, 0xecec, 0xdb25, 0xca1d, 0xba2c, 0xaba6, 0x9ed6, 0x93fd, 0x8b55, 0x850a, 0x813d, 0x8001, 0x815e,
    0x854b, 0x8bb5, 0x947b, 0x9f6e, 0xac56, 0xbaf1, 0xcaf1, 0xdc05, 0xedd3
};


static void wcbuf_task_test(void)
{
    /* sys_timer_add(NULL, dev_flow_player_pp, 2*1000); */
    /* sys_timeout_add(NULL, dev_flow_player_close, 10*1000); */
    int wlen;
    while (1) {
        os_time_dly(1);
        wlen = pcm_stream_data_write((u8 *)test1_data_sin44100, sizeof(test1_data_sin44100));
        if (wlen == 0) {
            printf("pcm_stream_data_write_cbuf_err");
        }
    }
}

//dev_flow_player_open打开以后，在pcm_stream_data_write接口写入数据
static int dev_flow_player_test(void)
{
    dev_flow_player_open(AUDIO_CODING_PCM, AUDIO_CH_L, 44100);

    return thread_fork("wcbuf_task", 12, 1024 * 2, 0, 0, wcbuf_task_test, 0);
}
late_initcall(dev_flow_player_test);

#endif //PCM_FLOW_TEST
