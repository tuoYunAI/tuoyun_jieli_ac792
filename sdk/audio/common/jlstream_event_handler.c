#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".jlstream_event_handler.data.bss")
#pragma data_seg(".jlstream_event_handler.data")
#pragma const_seg(".jlstream_event_handler.text.const")
#pragma code_seg(".jlstream_event_handler.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "media/audio_general.h"
#include "effects/effects_adj.h"
#include "app_core.h"
#include "classic/tws_api.h"
#include "app_config.h"
#if TCFG_LOCAL_TWS_ENABLE
#include "local_tws.h"
#endif

#define PIPELINE_UUID_TONE_NORMAL   0x7674
#define PIPELINE_UUID_A2DP          0xD96F
#define PIPELINE_UUID_ESCO          0xBA11
#define PIPELINE_UUID_LINEIN        0x1207
#define PIPELINE_UUID_SPDIF         0x96BE
#define PIPELINE_UUID_MUSIC         0x2A09
#define PIPELINE_UUID_FM            0x05FB
#define PIPELINE_UUID_MIC_EFFECT    0x9C2D
#define PIPELINE_UUID_MEDIA         0x1408
#define PIPELINE_UUID_A2DP_DUT      0xC9DB
#define PIPELINE_UUID_PC_AUDIO      0xDC8D
#define PIPELINE_UUID_RECORDER      0x49EC
#define PIPELINE_UUID_AI_VOICE      0x1BA8
#define PIPELINE_UUID_LE_AUDIO      0x99AA
#define PIPELINE_UUID_ACOUSTIC_COMMUNICAT 0x92C8
#define PIPELINE_UUID_VIDEO_REC     0xF7A2
#define PIPELINE_UUID_VIDEO_DEC     0xBC14
#define PIPELINE_UUID_VIR_VOICE     0x8026

void a2dp_energy_detect_handler(int *arg);

int get_system_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_TONE_NORMAL, par);
}

int get_media_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_MEDIA, par);
}

int get_esco_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_ESCO, par);
}

int get_mic_eff_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_MIC_EFFECT, par);
}

int get_usb_audio_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_PC_AUDIO, par);
}

static int get_pipeline_uuid(const char *name)
{
    if (!strcmp(name, "tone")) {
        return PIPELINE_UUID_TONE_NORMAL;
    }
    if (!strcmp(name, "tws_tone")) {
        return PIPELINE_UUID_TONE_NORMAL;
    }
    if (!strcmp(name, "tts")) {
        return PIPELINE_UUID_TONE_NORMAL;
    }
    if (!strcmp(name, "ring")) {
        return PIPELINE_UUID_TONE_NORMAL;
    }
    if (!strcmp(name, "esco")) {
        return PIPELINE_UUID_ESCO;
    }

    if (!strcmp(name, "mic_effect")) {
        return PIPELINE_UUID_MIC_EFFECT;
    }

    if (!strcmp(name, "pc_mic")) {
        return PIPELINE_UUID_PC_AUDIO;
    }
    if (!strcmp(name, "mix_recorder")) {
        return PIPELINE_UUID_RECORDER;
    }
    if (!strcmp(name, "acoustic_communicat")) {
        return PIPELINE_UUID_ACOUSTIC_COMMUNICAT;
    }
    if (!strcmp(name, "ai_voice")) {
        return PIPELINE_UUID_AI_VOICE;
    }

    if (!strcmp(name, "video_rec")) {
        return PIPELINE_UUID_VIDEO_REC;
    }

    if (!strcmp(name, "video_dec")) {
        return PIPELINE_UUID_VIDEO_DEC;
    }

    if (!strcmp(name, "a2dp")) {
#if TCFG_AUDIO_DUT_ENABLE
        if (audio_dec_dut_en_get(0)) {
            return PIPELINE_UUID_A2DP_DUT;
        }
#endif
    }

    if (!strcmp(name, "vir_voice")) {
        return PIPELINE_UUID_VIR_VOICE;
    }

    if (!strcmp(name, "a2dp_le_audio") || \
        !strcmp(name, "music_le_audio") || \
        !strcmp(name, "linein_le_audio") || \
        !strcmp(name, "fm_le_audio") || \
        !strcmp(name, "spdif_le_audio") || \
        !strcmp(name, "iis_le_audio") || \
        !strcmp(name, "mic_le_audio") || \
        !strcmp(name, "pc_le_audio") || \
        !strcmp(name, "le_audio")) {
        return PIPELINE_UUID_LE_AUDIO;
    }

    return PIPELINE_UUID_MEDIA;
}

static void player_close_handler(const char *name)
{

}

#if TCFG_HI_RES_AUDIO_ENEBALE || TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE
//调整解码器输出帧长
static const int frame_unit_size[] = { 64, 128, 256, 384, 512, 1024, 2048, 4096, 8192 };

int decoder_check_frame_unit_size(int dest_len)
{
    for (int i = 0; i < ARRAY_SIZE(frame_unit_size); i++) {
        if (dest_len <= frame_unit_size[i]) {
            dest_len = frame_unit_size[i];
            return dest_len;
        }
    }
    dest_len = 8192;
    return dest_len;
}

#endif

static int load_decoder_handler(struct stream_decoder_info *info)
{
    if (info->scene == STREAM_SCENE_A2DP) {
        info->task_name = "a2dp_dec";

#if TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE
        info->frame_time = 16;
#endif
    }

    if (info->coding_type == AUDIO_CODING_LHDC || info->coding_type == AUDIO_CODING_LDAC) {
        info->task_name = "a2dp_dec";
    }
    if (info->scene == STREAM_SCENE_ESCO) {
    }
    if (info->scene == STREAM_SCENE_MUSIC) {
        info->task_name = "file_dec";
    }

    return 0;
}

static void unload_decoder_handler(u32 coding_type)
{
    if (coding_type == AUDIO_CODING_AAC) {
    }
}

static int load_encoder_handler(struct stream_encoder_info *info)
{
    if (info->scene == STREAM_SCENE_ESCO) {
    }

    return 0;
}

static void unload_encoder_handler(struct stream_encoder_info *info)
{
    if (info->scene == STREAM_SCENE_ESCO) {
    }
}

/*
 *获取需要指定得默认配置
 * */
static int get_node_parm(int arg)
{
    return get_eff_default_param(arg);
}

/*
*获取ram内在线音效参数
*/
static int get_eff_online_parm(int arg)
{
    int ret = 0;
#if TCFG_CFG_TOOL_ENABLE
    struct eff_parm {
        int uuid;
        char name[16];
        u8 data[0];
    };
    struct eff_parm *parm = (struct eff_parm *)arg;
    /* printf("eff_online_uuid %x, %s\n", parm->uuid, parm->name); */
    ret = get_eff_online_param(parm->uuid, parm->name, (void *)arg);
#endif
    return ret;
}

#if TCFG_LOCAL_TWS_ENABLE
static int tws_switch_get_status(void)
{
    struct application *app = __get_current_app();
    if (app == NULL || !strcmp(app->name, "bt") || !strcmp(app->name, "sink_music")) {
        return 0;
    }

    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED && tws_api_data_trans_connect() && (local_tws_get_role() == LOCAL_TWS_ROLE_SOURCE)) {
        return 1;
    }

    return 0;
}
#endif

#if TCFG_USER_TWS_ENABLE
static int tws_get_output_channel(void)
{
    int channel = (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR) ? AUDIO_CH_LR : AUDIO_CH_MIX;
    if (tws_api_is_connect()) {
        channel = tws_api_get_local_channel() == 'L' ? AUDIO_CH_L : AUDIO_CH_R;
    }
    return channel;
}

static int get_merge_node_callback(const char *arg)
{
    return (int)tws_get_output_channel;
}
#endif

static int a2dp_switch_get_status(void)
{
#if TCFG_USER_EMITTER_ENABLE
    extern u8 *get_cur_connect_emitter_mac_addr(void);
    bool a2dp_sbc_encoder_status_check_ready(u8 * addr);
    if (get_cur_connect_emitter_mac_addr() && a2dp_sbc_encoder_status_check_ready(get_cur_connect_emitter_mac_addr())) {
        return 1;
    }
#endif
    return 0;
}

static int dac_switch_get_status(void)
{
    if (!a2dp_switch_get_status()) {
        return 1;
    }
    return 0;
}

#if TCFG_SWITCH_NODE_ENABLE
static int get_switch_node_callback(const char *arg)
{
    printf("get_switch_node_callback, node name:%s, need add yourself switch_node's callback!\n", arg);

#if TCFG_MIX_RECORD_ENABLE
    if (!strncmp(arg, "Switch_rec", strlen("Switch_rec"))) {
        return (int)get_mix_recorder_status;
    }
#endif // TCFG_MIX_RECORD_ENABLE

#if TCFG_LOCAL_TWS_ENABLE
    if (!strncmp(arg, "Switch_TWS", strlen("Switch_TWS"))) {
        return (int)tws_switch_get_status;
    }
#endif

    if (!strncmp(arg, "Switch_a2dp", strlen("Switch_a2dp"))) {
        return (int)a2dp_switch_get_status;
    }
    if (!strncmp(arg, "Switch_dac", strlen("Switch_dac"))) {
        return (int)dac_switch_get_status;
    }

    return 0;
}
#endif

int jlstream_event_notify(enum stream_event event, int arg)
{
    int ret = 0;

    switch (event) {
    case STREAM_EVENT_LOAD_DECODER:
        ret = load_decoder_handler((struct stream_decoder_info *)arg);
        break;
    case STREAM_EVENT_UNLOAD_DECODER:
        unload_decoder_handler((u32)arg);
        break;
    case STREAM_EVENT_LOAD_ENCODER:
        ret = load_encoder_handler((struct stream_encoder_info *)arg);
        break;
    case STREAM_EVENT_UNLOAD_ENCODER:
        unload_encoder_handler((struct stream_encoder_info *)arg);
        break;
    case STREAM_EVENT_GET_PIPELINE_UUID:
        ret = get_pipeline_uuid((const char *)arg);
        break;
    case STREAM_EVENT_CLOSE_PLAYER:
        player_close_handler((const char *)arg);
        break;
    case STREAM_EVENT_INC_SYS_CLOCK:
        break;
    case STREAM_EVENT_GET_NODE_PARM:
        ret = get_node_parm(arg);
        break;
    case STREAM_EVENT_GET_EFF_ONLINE_PARM:
        ret = get_eff_online_parm(arg);
        break;
    case STREAM_EVENT_A2DP_ENERGY:
        //a2dp_energy_detect_handler((int *)arg);
        break;
#if TCFG_SWITCH_NODE_ENABLE
    case STREAM_EVENT_GET_SWITCH_CALLBACK:
        ret = get_switch_node_callback((const char *)arg);
        break;
#endif
#if TCFG_USER_TWS_ENABLE
    case STREAM_EVENT_GET_MERGER_CALLBACK:
        ret = get_merge_node_callback((const char *)arg);
        break;
#endif
    default:
        break;
    }

    return ret;
}
