#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".kws_event.data.bss")
#pragma data_seg(".kws_event.data")
#pragma const_seg(".kws_event.text.const")
#pragma code_seg(".kws_event.text")
#endif
#include "app_config.h"
#include "system/timer.h"
#include "kws_event.h"
#include "asr/jl_kws.h"
#include "key/key_driver.h"

#define LOG_TAG_CONST SMART_VOICE
#define LOG_TAG     "[SMART_VOICE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "system/debug.h"

#if ((defined TCFG_SMART_VOICE_ENABLE) && TCFG_SMART_VOICE_ENABLE)
extern int config_audio_kws_event_enable;

typedef struct {
    u8 kws_event;
    u8 key_action;
    u8 key_value;
} kws_key_t;

static const kws_key_t kws_key_table[] = {
    { KWS_EVENT_XIAOJIE, KEY_EVENT_CLICK, KEY_ENC },
    { KWS_EVENT_PLAY_MUSIC, KEY_EVENT_CLICK, KEY_PLAY },
    { KWS_EVENT_STOP_MUSIC, KEY_EVENT_CLICK, KEY_PAUSE },
    { KWS_EVENT_PAUSE_MUSIC, KEY_EVENT_CLICK, KEY_PAUSE },
    { KWS_EVENT_VOLUME_UP, KEY_EVENT_CLICK, KEY_VOLUME_INC },
    { KWS_EVENT_VOLUME_DOWN, KEY_EVENT_CLICK, KEY_VOLUME_DEC },
    { KWS_EVENT_PREV_SONG, KEY_EVENT_LONG, KEY_PREV },
    { KWS_EVENT_NEXT_SONG, KEY_EVENT_LONG, KEY_NEXT },
};

static const u8 kws_wake_word_event[] = {
    KWS_EVENT_NULL,
    KWS_EVENT_HEY_KEYWORD,
};

static const u8 kws_multi_command_event[] = {
    KWS_EVENT_NULL,
    KWS_EVENT_NULL,
    KWS_EVENT_XIAOJIE,
    KWS_EVENT_XIAOJIE,
    KWS_EVENT_PLAY_MUSIC,
    KWS_EVENT_STOP_MUSIC,
    KWS_EVENT_PAUSE_MUSIC,
    KWS_EVENT_VOLUME_UP,
    KWS_EVENT_VOLUME_DOWN,
    KWS_EVENT_PREV_SONG,
    KWS_EVENT_NEXT_SONG,
    KWS_EVENT_ANC_ON,
    KWS_EVENT_ANC_OFF,
    KWS_EVENT_TRANSARENT_ON,
};

static const u8 kws_call_command_event[] = {
    KWS_EVENT_NULL,
    KWS_EVENT_NULL,
    KWS_EVENT_CALL_ACTIVE,
    KWS_EVENT_CALL_HANGUP,
};

static const u8 *const kws_model_events[3] = {
    kws_wake_word_event,
    kws_multi_command_event,
    kws_call_command_event,
};

__attribute__((weak))
int get_ai_listen_state(void)
{
    return 0;
}

int smart_voice_kws_event_handler(u8 model, int kws)
{
    if (!config_audio_kws_event_enable || kws < 0) {
        return 0;
    }

    int event = kws_model_events[model][kws];

    if (event == KWS_EVENT_NULL) {
        return -EINVAL;
    }

    struct key_event key = {0};
    key.type = KEY_DRIVER_TYPE_ASR;
    int i;

    for (i = 0; i < ARRAY_SIZE(kws_key_table); ++i) {
        if (kws_key_table[i].kws_event == event) {
            key.action = kws_key_table[i].key_action;
            key.value = kws_key_table[i].key_value;
            break;
        }
    }

    if (i == ARRAY_SIZE(kws_key_table)) {
        return 0;
    }

    if (get_ai_listen_state() && event != KWS_EVENT_XIAOJIE) {
        return 0;
    }

    return key_event_notify_no_filter(KEY_EVENT_FROM_KEY, &key);
}

static const char *const kws_dump_words[] = {
    "no words",
    "hey siri",
    "xiao jie",
    "xiao du",
    "bo fang yin yue",
    "ting zhi bo fang",
    "zan ting bo fang",
    "zeng da yin liang",
    "jian xiao yin liang",
    "shang yi shou",
    "xia yi shou",
    "jie ting dian hua",
    "gua duan dian hua",
    "anc on",
    "anc off",
    "transarent on",
};

struct kws_result_context {
    u16 timer;
    u32 result[0];
};

static void smart_voice_kws_dump_timer(void *arg)
{
    struct kws_result_context *ctx = (struct kws_result_context *)arg;
    int i = 0;

    int kws_num = ARRAY_SIZE(kws_dump_words);
    log_info("\n===============================================\nResults:\n");
    for (i = 1; i < kws_num; i++) {
        log_info("%s : %u\n", kws_dump_words[i], ctx->result[i]);
    }
    log_info("\n===============================================\n");
}

void *smart_voice_kws_dump_open(int period_time)
{
    if (!config_audio_kws_event_enable) {
        return NULL;
    }

    struct kws_result_context *ctx = NULL;
    ctx = zalloc(sizeof(struct kws_result_context) + (sizeof(u32) * ARRAY_SIZE(kws_dump_words)));

    if (ctx) {
        ctx->timer = sys_timer_add(ctx, smart_voice_kws_dump_timer, period_time);
    }
    return ctx;
}

void smart_voice_kws_dump_result_add(void *_ctx, u8 model, int kws)
{
    if (!config_audio_kws_event_enable || kws < 0) {
        return;
    }

    struct kws_result_context *ctx = (struct kws_result_context *)_ctx;
    int event = kws_model_events[model][kws];
    ctx->result[event]++;
}

void smart_voice_kws_dump_close(void *_ctx)
{
    struct kws_result_context *ctx = (struct kws_result_context *)_ctx;
    if (config_audio_kws_event_enable) {
        if (ctx->timer) {
            sys_timer_del(ctx->timer);
            free(ctx);
        }
    }
}

#endif
