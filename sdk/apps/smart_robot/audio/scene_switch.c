#include "jlstream.h"
#include "effects/effects_adj.h"
#include "node_param_update.h"
#include "scene_switch.h"
#include "effects/audio_vocal_remove.h"
#include "system/app_core.h"
#include "syscfg/syscfg_id.h"
#include "user_cfg_id.h"
#include "app_config.h"

#define LOG_TAG             "[SCENE]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

#define MEDIA_MODULE_NODE_UPDATE_EN  (TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE || TCFG_3D_PLUS_MODULE_NODE_ENABLE)// Media模式添加模块子节点更新

static u8 music_scene = 0; //记录音乐场景序号
static u8 music_eq_preset_index = 0; //记录 Eq0Media EQ配置序号
static u8 mic_scene = 0; //记录混响场景序号
static u8 vocla_remove_mark = 0xff;

/* 命名规则：节点名+模式名，如 SurBt、CrossAux、Eq0File */
#if defined(MEDIA_UNIFICATION_EN)&&MEDIA_UNIFICATION_EN
static const char *const music_mode[] = {"Media", "Media", "Media", "Media", "Media", "Media"};
#else
static const char *const music_mode[] = {"Bt", "Aux", "File", "Fm", "Spd", "Net"};
#endif
static const char *const father_name[] = {"VSPro", "3dPlus", "VBassPro"};//模块节点名,如添加新的模块节点名，需对宏定义MEDIA_MODULE_NODE_UPDATE_EN进行配置使能
static const char *const sur_name[] = {"Sur"};
static const char *const crossover_name[] = {"Cross", "LRCross"};
static const char *const band_merge_name[] = {"Band", "LRBand", "LR3Band", "LSCBand", "RSCBand", "RLSCBand", "RRSCBand", "MixerGain"};
static const char *const two_band_merge_name[] = {"Bandt"};
static const char *const bass_treble_name[] = {"Bass"};
static const char *const smix_name[] = {"Smix0", "Smix1", "MidSMix", "LowSMix"};
static const char *const eq_name[] = {"Eq0", "Eq1", "Eq2", "Eq3", "CEq", "LRSEq"};
static const char *const sw_eq_name[] = {"HPEQ", "PEAKEQ"};
static const char *const drc_name[] = {"Drc0", "Drc1", "Drc2", "Drc3", "Drc4"};
static const char *const vbass_name[] = {"VBass"};
static const char *const multi_freq_gen_name[] = {"MFreqGen"};
static const char *const gain_name[] = {"Gain", "LRGain"};
static const char *const harmonic_exciter_name[] = {"Hexciter"};
static const char *const dy_eq_name[] = {"DyEq"};
static const char *const limiter_name[] = {"PreLimiter", "LRLimiter", "CLimiter", "LRSLimiter"};
static const char *const multiband_limiter_name[] = {"MBLimiter0"};
static const char *const pcm_delay_name[] = {"LRPcmDly"};
static const char *const drc_adv_name[] = {"CDrcAdv", "LRSDrcAdv", "HPDRC"};
static const char *const multiband_drc_adv_name[] = {"MDrcAdv"};
static const char *const noise_gate_name[] = {"LRSNsGate"};
static const char *const upmix_name[] = {"UpMix2to5"};
static const char *const effect_dev0_name[] = {"effdevx"};
static const char *const effect_dev1_name[] = {"effdevx"};
static const char *const effect_dev2_name[] = {"effdevx"};
static const char *const effect_dev3_name[] = {"effdevx"};
static const char *const effect_dev4_name[] = {"effdevx"};
static const char *const stereo_spatial_wider_name[] = {"SPWider"};

/* 混响模块命名 */
static const char *const mic_name = "Eff";
static const char *const mic_bass_treble_name[] = {"BassTre"};
static const char *const mic_noisegate_name[] = {"NoiseGate"};
static const char *const mic_crossover_name[] = {"Crossover"};
static const char *const mic_band_merge_name[] = {"BMerge1", "BMerge2"};
static const char *const mic_2band_merge_name[] = {"BMerge4"};
static const char *const mic_howling_fs_name[] = {"Fshift"};
static const char *const mic_howling_supress_name[] = {"Hspress"};
static const char *const mic_voice_changer_name[] = {"Vchanger"};
static const char *const mic_autotune_name[] = {"Atune"};
static const char *const mic_plate_reverb_advance_name[] = {"PReverb"};
static const char *const mic_reverb_name[] = {"Reverb"};
static const char *const mic_echo_name[] = {"Echo"};
static const char *const mic_eq_name[] = {"Eq1", "Eq2", "Eq3", "Eq4", "Eq5", "Eq6"};
static const char *const mic_drc_name[] = {"Drc1", "Drc2", "Drc3", "Drc4", "Drc5", "Drc6", "Drc7"};

typedef enum {
    BT_MODE,
    AUX_MODE,
    FILE_MODE,
    FM_MODE,
    SPDIF_MODE,
    PC_MODE,
    NET_MODE,
    NOT_SUPPORT_MODE,
} music_mode_t;

typedef struct {
    const char *app_name;
    const char *uuid_name;
    music_mode_t scene_mode;
} app_scene_table_t;

static const app_scene_table_t app_scene_table[] = {
    { "bt",             "a2dp",     BT_MODE },
    { "linein_music",   "linein",   AUX_MODE },
    { "local_music",    "music",    FILE_MODE },
    { "fm_music",       "fm",       FM_MODE },
    { "spdif_music",    "spdif",    SPDIF_MODE },
    { "pc_music",       "pc_spk",   PC_MODE },
    { "net_music",      "net",      NET_MODE },
};

/* 获取音乐模式场景序号 */
u8 get_current_scene(void)
{
    return music_scene;
}

/* 获取mic混响模式场景序号 */
u8 get_mic_current_scene(void)
{
    return mic_scene;
}

void set_default_scene(u8 index)
{
    music_scene = index;
}

/* 获EQ0 配置序号 */
u8 get_music_eq_preset_index(void)
{
    return music_eq_preset_index;
}

void set_music_eq_preset_index(u8 index)
{
    music_eq_preset_index = index;
    syscfg_write(CFG_EQ_INDEX, &music_eq_preset_index, sizeof(music_eq_preset_index));
}

/* 获取当前处于的模式 */
static u8 get_current_scene_mode_index(void)
{
    struct application *app = get_current_app();
    if (!app) {
        return NOT_SUPPORT_MODE;
    }

    for (int i = 0; i < ARRAY_SIZE(app_scene_table); ++i) {
        if (!strcmp(app->name, app_scene_table->app_name)) {
            return app_scene_table->scene_mode;
        }
    }

    log_error("mode not support scene switch %s", app->name);

    return NOT_SUPPORT_MODE;
}

/* 获取当前模式中场景个数 */
static int get_mode_scene_num(void)
{
    struct application *app = get_current_app();
    if (!app) {
        return -1;
    }

    for (int i = 0; i < ARRAY_SIZE(app_scene_table); ++i) {
        if (!strcmp(app->name, app_scene_table->app_name)) {
            u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)app_scene_table->uuid_name);
            u8 scene_num;
            jlstream_read_pipeline_param_group_num(uuid, &scene_num);
            return scene_num;
        }
    }

    log_error("mode not support scene switch %s", app->name);

    return -1;
}


static void effects_name_sprintf(char *out, const char *name0, const char *name1)
{
    memset(out, '\0', 16);
    memcpy(out, name0, strlen(name0));
    memcpy(&out[strlen(name0)], name1, strlen(name1));
}

static void effects_name_sprintf_to_hash(char *out, const char *name_son, const char *name_father)
{
    jlstream_module_node_get_name((char *)name_son, (char *)name_father, out);
    /* printf("name: %s , 0x%x\n", name ,hash); */
}

/*
 *媒体模块节点参数更新
 * */
static int module_node_update_parm(int (*node_update)(u8 mode_index, char *node_name, u8 cfg_index), u8 mode_index, char *son_node_name, u8 cfg_index)
{
    int ret;
    char tar_name[16];
    for (int j = 0; j < ARRAY_SIZE(father_name); j++) {
        effects_name_sprintf_to_hash(tar_name, son_node_name, father_name[j]);
        if (node_update) {
            ret = node_update(mode_index, tar_name, cfg_index);
            if (ret < 0) {
                continue;
            } else {
                break;
            }
        }
    }
    return ret;
}

/* 音乐模式：根据参数组序号进行场景切换 */
void effect_scene_set(u8 scene)
{
    int scene_num = get_mode_scene_num();
    if (scene >= scene_num) {
        log_error("err : without this scene %d", scene);
        return;
    }

    music_scene = scene;
    log_info("current music scene : %d", scene);

    syscfg_write(CFG_SCENE_INDEX, &music_scene, sizeof(music_scene));

    u8 cur_mode = get_current_scene_mode_index();
    if (cur_mode >= NOT_SUPPORT_MODE) {
        return;
    }

    //cppcheck-suppress unusedVariable
    char tar_name[16];
    //cppcheck-suppress unusedVariable
    int ret;

#if TCFG_PCM_DELAY_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(pcm_delay_name); i++) {
        effects_name_sprintf(tar_name,  pcm_delay_name[i], music_mode[cur_mode]);
        ret = pcm_delay_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(pcm_delay_update_parm, scene, pcm_delay_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_NOISEGATE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(noise_gate_name); i++) {
        effects_name_sprintf(tar_name,  noise_gate_name[i], music_mode[cur_mode]);
        ret = noisegate_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(noisegate_update_parm, scene, noise_gate_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_WDRC_ADVANCE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(drc_adv_name); i++) {
        effects_name_sprintf(tar_name,  drc_adv_name[i], music_mode[cur_mode]);
        ret = wdrc_advance_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(wdrc_advance_update_parm, scene, drc_adv_name[i], 0);
#endif
        }
    }
#endif

#if  TCFG_MULTI_BAND_DRC_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(multiband_drc_adv_name); i++) {
        effects_name_sprintf(tar_name,  multiband_drc_adv_name[i], music_mode[cur_mode]);
        ret = multiband_drc_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(wdrc_advance_update_parm, scene, multiband_drc_adv_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_LIMITER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(limiter_name); i++) {
        effects_name_sprintf(tar_name, limiter_name[i], music_mode[cur_mode]);
        ret = limiter_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(limiter_update_parm, scene, limiter_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_MULTI_BAND_LIMITER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(multiband_limiter_name); i++) {
        effects_name_sprintf(tar_name, multiband_limiter_name[i], music_mode[cur_mode]);
        ret = multiband_limiter_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(multiband_limiter_update_parm, scene, multiband_limiter_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_VIRTUAL_SURROUND_PRO_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(upmix_name); i++) {
        effects_name_sprintf(tar_name, upmix_name[i], music_mode[cur_mode]);
        ret = virtual_surround_pro_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(virtual_surround_pro_update_parm, scene, upmix_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_EFFECT_DEV0_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(effect_dev0_name); i++) {
        effects_name_sprintf(tar_name, effect_dev0_name[i], music_mode[cur_mode]);
        ret = effect_dev0_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(effect_dev0_update_parm, scene, effect_dev0_name[i], 0);
#endif
        }
    }
#endif
#if TCFG_EFFECT_DEV1_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(effect_dev1_name); i++) {
        effects_name_sprintf(tar_name, effect_dev1_name[i], music_mode[cur_mode]);
        ret = effect_dev1_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(effect_dev1_update_parm, scene, effect_dev1_name[i], 0);
#endif
        }
    }
#endif
#if TCFG_EFFECT_DEV2_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(effect_dev2_name); i++) {
        effects_name_sprintf(tar_name, effect_dev2_name[i], music_mode[cur_mode]);
        ret = effect_dev2_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(effect_dev2_update_parm, scene, effect_dev2_name[i], 0);
#endif
        }
    }
#endif
#if TCFG_EFFECT_DEV3_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(effect_dev3_name); i++) {
        effects_name_sprintf(tar_name, effect_dev3_name[i], music_mode[cur_mode]);
        ret = effect_dev3_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(effect_dev3_update_parm, scene, effect_dev3_name[i], 0);
#endif
        }
    }
#endif
#if TCFG_EFFECT_DEV4_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(effect_dev4_name); i++) {
        effects_name_sprintf(tar_name, effect_dev4_name[i], music_mode[cur_mode]);
        ret = effect_dev4_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(effect_dev4_update_parm, scene, effect_dev4_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_SURROUND_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(sur_name); i++) {
        effects_name_sprintf(tar_name, sur_name[i], music_mode[cur_mode]);
        ret = surround_effect_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(surround_effect_update_parm, scene, sur_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_CROSSOVER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(crossover_name); i++) {
        effects_name_sprintf(tar_name, crossover_name[i], music_mode[cur_mode]);
        ret = crossover_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(crossover_update_parm, scene, crossover_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_3BAND_MERGE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(band_merge_name); i++) {
        effects_name_sprintf(tar_name, band_merge_name[i], music_mode[cur_mode]);
        ret = band_merge_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(band_merge_update_parm, scene, band_merge_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_2BAND_MERGE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(two_band_merge_name); i++) {
        effects_name_sprintf(tar_name, two_band_merge_name[i], music_mode[cur_mode]);
        ret = two_band_merge_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(two_band_merge_update_parm, scene, two_band_merge_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_BASS_TREBLE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(bass_treble_name); i++) {
        effects_name_sprintf(tar_name, bass_treble_name[i], music_mode[cur_mode]);
        ret = bass_treble_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(bass_treble_update_parm, scene, bass_treble_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_STEROMIX_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(smix_name); i++) {
        effects_name_sprintf(tar_name, smix_name[i], music_mode[cur_mode]);
        ret = stero_mix_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(stero_mix_update_parm, scene, smix_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_EQ_ENABLE
    for (int i = 0; i < ARRAY_SIZE(eq_name); i++) {
        effects_name_sprintf(tar_name,  eq_name[i], music_mode[cur_mode]);
        if (i == 0) {
            ret = eq_update_parm(scene, tar_name, music_eq_preset_index);
            if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
                module_node_update_parm(eq_update_parm, scene, eq_name[i], music_eq_preset_index);
#endif
            }
        } else {
            ret = eq_update_parm(scene, tar_name, 0);
            if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
                module_node_update_parm(eq_update_parm, scene, eq_name[i], 0);
#endif
            }
        }
    }
#endif
#if TCFG_SOFWARE_EQ_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(sw_eq_name); i++) {
        effects_name_sprintf(tar_name,  sw_eq_name[i], music_mode[cur_mode]);
        ret = sw_eq_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(sw_eq_update_parm, scene, sw_eq_name[i], 0);
#endif
        }
    }
#endif
#if TCFG_WDRC_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(drc_name); i++) {
        effects_name_sprintf(tar_name,  drc_name[i], music_mode[cur_mode]);
        ret = drc_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(drc_update_parm, scene, drc_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_VBASS_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(vbass_name); i++) {
        effects_name_sprintf(tar_name,  vbass_name[i], music_mode[cur_mode]);
        ret = virtual_bass_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(virtual_bass_update_parm, scene, vbass_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_VIRTUAL_BASS_CLASSIC_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(multi_freq_gen_name); i++) {
        effects_name_sprintf(tar_name,  multi_freq_gen_name[i], music_mode[cur_mode]);
        ret = virtual_bass_classic_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(virtual_bass_classic_update_parm, scene, multi_freq_gen_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_GAIN_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(gain_name); i++) {
        effects_name_sprintf(tar_name,  gain_name[i], music_mode[cur_mode]);
        ret = gain_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(gain_update_parm, scene, gain_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_HARMONIC_EXCITER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(harmonic_exciter_name); i++) {
        effects_name_sprintf(tar_name,  harmonic_exciter_name[i], music_mode[cur_mode]);
        ret = harmonic_exciter_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(harmonic_exciter_update_parm, scene, harmonic_exciter_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_DYNAMIC_EQ_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(dy_eq_name); i++) {
        effects_name_sprintf(tar_name,  dy_eq_name[i], music_mode[cur_mode]);
        ret = dynamic_eq_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(dynamic_eq_update_parm, scene, dy_eq_name[i], 0);
#endif
        }
    }
#endif

#if TCFG_STEREO_SPATIAL_WIDER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(stereo_spatial_wider_name); i++) {
        effects_name_sprintf(tar_name,  stereo_spatial_wider_name[i], music_mode[cur_mode]);
        ret = stereo_spatial_wider_update_parm(scene, tar_name, 0);
        if (ret < 0) {
#if MEDIA_MODULE_NODE_UPDATE_EN
            module_node_update_parm(stereo_spatial_wider_update_parm, scene, stereo_spatial_wider_name[i], 0);
#endif
        }
    }
#endif
}

/* 音乐模式：根据参数组个数顺序切换场景 */
void effect_scene_switch(void)
{
    int scene_num = get_mode_scene_num();
    if (!scene_num) {
        log_error("scene switch err");
        return;
    }
    music_scene++;
    if (music_scene >= scene_num) {
        music_scene = 0;
    }
    effect_scene_set(music_scene);
}

/* mic混响：获取场景个数 */
static u8 get_mic_effect_scene_num(void)
{
    u8 scene_num;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mic_effect");
    jlstream_read_pipeline_param_group_num(uuid, &scene_num);
    return scene_num;
}

/* mic混响：根据参数组序号进行场景切换 */
void mic_effect_scene_set(u8 scene)
{
    u8 scene_num = get_mic_effect_scene_num();
    if (scene >= scene_num) {
        log_error("err : without this scene %d", scene);
        return;
    }

    mic_scene = scene;
    //cppcheck-suppress unusedVariable
    char tar_name[16];

#if TCFG_NOISEGATE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_noisegate_name); i++) {
        effects_name_sprintf(tar_name,  mic_noisegate_name[i], mic_name);
        noisegate_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_CROSSOVER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_crossover_name); i++) {
        effects_name_sprintf(tar_name,  mic_crossover_name[i], mic_name);
        crossover_update_parm(scene, tar_name, 0);
    }
#endif

#if (TCFG_3BAND_MERGE_ENABLE)
    for (int i = 0; i < ARRAY_SIZE(mic_band_merge_name); i++) {
        effects_name_sprintf(tar_name,  mic_band_merge_name[i], mic_name);
        band_merge_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_2BAND_MERGE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_2band_merge_name); i++) {
        effects_name_sprintf(tar_name,  mic_2band_merge_name[i], mic_name);
        two_band_merge_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_howling_fs_name); i++) {
        effects_name_sprintf(tar_name,  mic_howling_fs_name[i], mic_name);
        howling_frequency_shift_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_NOTCH_HOWLING_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_howling_supress_name); i++) {
        effects_name_sprintf(tar_name,  mic_howling_supress_name[i], mic_name);
        howling_suppress_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_VOICE_CHANGER_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_voice_changer_name); i++) {
        effects_name_sprintf(tar_name,  mic_voice_changer_name[i], mic_name);
        voice_changer_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_AUTOTUNE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_autotune_name); i++) {
        effects_name_sprintf(tar_name,  mic_autotune_name[i], mic_name);
        autotune_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_plate_reverb_advance_name); i++) {
        effects_name_sprintf(tar_name,  mic_plate_reverb_advance_name[i], mic_name);
        reverb_advance_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_REVERB_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_reverb_name); i++) {
        effects_name_sprintf(tar_name,  mic_reverb_name[i], mic_name);
        reverb_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_ECHO_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_echo_name); i++) {
        effects_name_sprintf(tar_name,  mic_echo_name[i], mic_name);
        echo_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_EQ_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_eq_name); i++) {
        effects_name_sprintf(tar_name,  mic_eq_name[i], mic_name);
        eq_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_drc_name); i++) {
        effects_name_sprintf(tar_name,  mic_drc_name[i], mic_name);
        drc_update_parm(scene, tar_name, 0);
    }
#endif

#if TCFG_BASS_TREBLE_NODE_ENABLE
    for (int i = 0; i < ARRAY_SIZE(mic_bass_treble_name); i++) {
        effects_name_sprintf(tar_name,  mic_bass_treble_name[i], mic_name);
        bass_treble_update_parm(scene, tar_name, 0);
    }
#endif
}

/* mic混响：根据参数组个数顺序切换场景 */
void mic_effect_scene_switch(void)
{
    int scene_num = get_mic_effect_scene_num();
    if (!scene_num) {
        log_error("scene switch err");
        return;
    }
    mic_scene++;
    if (mic_scene >= scene_num) {
        mic_scene = 0;
    }
    mic_effect_scene_set(mic_scene);
}

//实时更新media数据流中人声消除bypass参数,启停人声消除功能
void music_vocal_remover_switch(void)
{
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
    vocal_remover_param_tool_set cfg = {0};
    const char *vocal_node_name = "VocalRemovMedia";
    int ret = jlstream_read_form_data(0, vocal_node_name, 0, &cfg);
    if (!ret) {
        log_error("read parm err, %s, %s", __func__, vocal_node_name);
        return;
    }
    if (vocla_remove_mark == 0xff) {
        vocla_remove_mark = cfg.is_bypass;
    }
    vocla_remove_mark ^= 1;
    cfg.is_bypass = vocla_remove_mark | USER_CTRL_BYPASS;
    jlstream_set_node_param(NODE_UUID_VOCAL_REMOVER, vocal_node_name, &cfg, sizeof(cfg));
#endif
}

//media数据流启动后更新人声消除bypass参数
void musci_vocal_remover_update_parm(void)
{
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
    vocal_remover_param_tool_set cfg = {0};
    char *vocal_node_name = "VocalRemovMedia";
    int ret = jlstream_read_form_data(0, vocal_node_name, 0, &cfg);
    if (!ret) {
        log_error("read parm err, %s, %s", __func__, vocal_node_name);
        return;
    }
    if (vocla_remove_mark == 0xff) {
        vocla_remove_mark = cfg.is_bypass;
    }
    cfg.is_bypass = vocla_remove_mark | USER_CTRL_BYPASS;
    jlstream_set_node_param(NODE_UUID_VOCAL_REMOVER, vocal_node_name, &cfg, sizeof(cfg));
#endif
}

u8 get_music_vocal_remover_status(void)
{
    return vocla_remove_mark;
}
