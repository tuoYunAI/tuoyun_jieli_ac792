#include "jlstream.h"
#include "system/init.h"
#include "effects/audio_echo.h"
#include "effects/effects_adj.h"
#include "media/audio_def.h"
#include "volume_node.h"
#include "mic_effect.h"
#include "audio_config_def.h"
#include "media_config.h"
#include "syscfg/syscfg_id.h"
#include "app_config.h"
#include "user_cfg_id.h"


#if TCFG_MIC_EFFECT_ENABLE

#define LOG_TAG             "[MIC_EFFECT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


#define MIC_EFFECT_CHANNEL_MAX	AUDIO_ADC_MAX_NUM

struct mic_effect_player {
    struct jlstream *stream[MIC_EFFECT_CHANNEL_MAX];
    s16 dvol;
    u8 dvol_index;
    unsigned int echo_delay;                      //回声的延时时间 0-300ms
    unsigned int echo_decayval;                   // 0-70%
};

static const u8 vol_table[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
static struct mic_effect_player *g_mic_effect_player;
static u16 mic_irq_point_unit = AUDIO_ADC_IRQ_POINTS;
static s16 mic_eff_volume = 100;
static u8 pause_mark = 0;

void mic_effect_set_irq_point_unit(u16 point_unit)
{
    mic_irq_point_unit = point_unit;
}

static void mic_effect_player_callback(void *private_data, int event)
{
    switch (event) {
    case STREAM_EVENT_START:
        const char *vol_name = "VolEff";
        struct volume_cfg cfg = {0};
        cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
        cfg.cur_vol = mic_eff_volume;
        jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, vol_name, (void *)&cfg, sizeof(struct volume_cfg));
        break;
    }
}

static void mic_effect_player_callback_ext(void *private_data, int event)
{
    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

static int mic_effect_volume_init(void)
{
    syscfg_read(CFG_MIC_EFF_VOLUME_INDEX, &mic_eff_volume, sizeof(mic_eff_volume));

    return 0;
}
__initcall(mic_effect_volume_init);

/*
 * 输入源节点命名规则：MicEff、MicEff1、MicEff2、MicEff3
 * 这样音频流构建的时候，即可不用修改程序，自动遍历打开并创建数据流
 *
 */
int mic_effect_player_open(void)
{
#if TCFG_APP_FM_EN
    if (g_mic_effect_player || get_fm_scan_status()) { //fm搜台中不能开混响
#else
    if (g_mic_effect_player) {
#endif
        return 0;
    }

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mic_effect");
    if (uuid == 0) {
        return -EFAULT;
    }

    struct mic_effect_player *player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    char mic_effect_source_name[8] = "MicEff";
    char mic_effect_stream_err = 1;

    for (int i = 0; i < MIC_EFFECT_CHANNEL_MAX; i++) {
        if (i) {
            sprintf(mic_effect_source_name, "MicEff%d", i);
        }
        log_info("mic_effect_source[%d]_name: %s", i, mic_effect_source_name);
        player->stream[i] = jlstream_pipeline_parse_by_node_name(uuid, mic_effect_source_name);
        if (player->stream[i]) {
            mic_effect_stream_err = 0;
            log_info("mic_stream[%d] succ", i);
        }
    }
    if (mic_effect_stream_err) {
        goto __exit0;
    }

    char mic_effect_thread_name[12] = "mic_effect";
    char mic_effect_thread_name_idx = 1;

    for (int i = 0; i < MIC_EFFECT_CHANNEL_MAX; i++) {
        if (player->stream[i]) {
            jlstream_node_ioctl(player->stream[i], NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, mic_irq_point_unit);
            jlstream_node_ioctl(player->stream[i], NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, mic_irq_point_unit);
            if (i == 0) {
                jlstream_set_callback(player->stream[i], player, mic_effect_player_callback);
            } else {
                jlstream_set_callback(player->stream[i], player, mic_effect_player_callback_ext);
            }
            jlstream_set_scene(player->stream[i], STREAM_SCENE_MIC_EFFECT);
            sprintf(mic_effect_thread_name, "mic_effect%d", mic_effect_thread_name_idx);
            mic_effect_thread_name_idx++;
            log_info("mic_effect stream[%d] thread_name:%s", i, mic_effect_thread_name);
            jlstream_add_thread(player->stream[i], mic_effect_thread_name);
#if CPU_CORE_NUM > 1
            if (config_jlstream_multi_thread_enable) {
                sprintf(mic_effect_thread_name, "mic_effect%d", mic_effect_thread_name_idx);
                mic_effect_thread_name_idx++;
                log_info("mic_effect stream[%d] thread_name:%s", i, mic_effect_thread_name);
                jlstream_add_thread(player->stream[i], mic_effect_thread_name);
            }
#endif
            jlstream_start(player->stream[i]);
        }
    }

    //记录echo、Dvol节点的参数 供按键调节使用
    echo_param_tool_set echo_cfg = {0};
    const char *node_name = "EchoEff";     //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    int ret = jlstream_read_form_data(0, node_name, 0, &echo_cfg);
    if (ret) {
        log_info("read echo parm delay %d", echo_cfg.parm.delay);
        player->echo_delay = echo_cfg.parm.delay;
        player->echo_decayval = echo_cfg.parm.decayval;
    }

    player->dvol = mic_eff_volume;
    player->dvol_index = player->dvol / 10;
    if (player->dvol_index >= sizeof(vol_table) / sizeof(vol_table[0])) {
        player->dvol_index = sizeof(vol_table) / sizeof(vol_table[0]) - 1;
    }
    log_info("mic dvol %d", player->dvol);

    g_mic_effect_player = player;

    return 0;

__exit0:
    free(player);

    return -1;
}

bool mic_effect_player_runing(void)
{
    return g_mic_effect_player != NULL;
}

void mic_effect_player_close(void)
{
    struct mic_effect_player *player = g_mic_effect_player;

    if (!player) {
        return;
    }

    for (int i = 0; i < MIC_EFFECT_CHANNEL_MAX; i++) {
        if (player->stream[i]) {
            jlstream_stop(player->stream[i], 0);
            jlstream_release(player->stream[i]);
        }
    }

    free(player);
    g_mic_effect_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"mic_effect");
}

//混响暂停恢复接口
void mic_effect_player_pause(u8 mark)
{
    if (mark) { //暂停
        //混响在运行时才暂停（关闭）
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
            pause_mark = 1;
        }
    } else {
        if (pause_mark) {
            mic_effect_player_open();
        }
        pause_mark = 0;
    }
}

//echo 调节接口
void mic_effect_set_echo_delay(u32 delay)
{
    if (!g_mic_effect_player) {
        return;
    }

    echo_param_tool_set cfg = {0};
    /*
     *解析配置文件内效果配置
     * */
    char mode_index = 0;                //模式序号（当前节点无多参数组，mode_index是0）
    const char *node_name = "EchoEff";  //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    char cfg_index  = 0;                //目标配置项序号（当前节点无多参数组，cfg_index是0）
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        return;
    }

    g_mic_effect_player->echo_delay = delay;
    cfg.parm.delay = g_mic_effect_player->echo_delay;

    /*
     *将配置文件内获取得到的参数更新到目标节点
     * */
    jlstream_set_node_param(NODE_UUID_ECHO, node_name, &cfg, sizeof(cfg));
}

u32 mic_effect_get_echo_delay(void)
{
    if (g_mic_effect_player) {
        return g_mic_effect_player->echo_delay;
    }

    return 0;
}

//mic音量调节接口
void mic_effect_set_dvol(u8 vol)
{
    const char *vol_name = "VolEff";
    struct volume_cfg cfg = {0};
    cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
    cfg.cur_vol = vol;

    if (g_mic_effect_player) {
        jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, vol_name, (void *)&cfg, sizeof(struct volume_cfg));
        g_mic_effect_player->dvol = vol;
        mic_eff_volume = vol;
        syscfg_write(CFG_MIC_EFF_VOLUME_INDEX, &mic_eff_volume, sizeof(mic_eff_volume));
    }
}

u8 mic_effect_get_dvol(void)
{
    if (g_mic_effect_player) {
        return g_mic_effect_player->dvol;
    }
    return 0;
}

void mic_effect_dvol_up(void)
{
    if (g_mic_effect_player) {
        if (g_mic_effect_player->dvol_index < sizeof(vol_table) / sizeof(vol_table[0]) - 1) {
            g_mic_effect_player->dvol_index++;
            mic_effect_set_dvol(vol_table[g_mic_effect_player->dvol_index]);
        }
    }
}

void mic_effect_dvol_down(void)
{
    if (g_mic_effect_player) {
        if (g_mic_effect_player->dvol_index) {
            g_mic_effect_player->dvol_index--;
            mic_effect_set_dvol(vol_table[g_mic_effect_player->dvol_index]);
        }
    }
}

#endif
