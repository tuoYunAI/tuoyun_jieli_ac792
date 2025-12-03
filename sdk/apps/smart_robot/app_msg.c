#include "app_msg.h"
#include "os/os_api.h"
#include "system/app_core.h"
#include "app_config.h"

#define LOG_TAG             "[APP_MSG]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
//#define LOG_DEBUG_ENABLE
//#define LOG_DUMP_ENABLE
//#define LOG_CHAR_ENABLE
#include "system/debug.h"


typedef struct app_mode_table {
    const char *app_name;
    u16 app_msg_bottom;
    u16 app_msg_top;
    app_mode_t app_mode;
} app_mode_table_t;

static const app_mode_table_t app_mode_table[] = {
#if TCFG_APP_BT_EN
    { "bt",             APP_MSG_BT_BOTTOM,              APP_MSG_TWS_TOP,            APP_MODE_BT },
#endif
#if TCFG_APP_NET_MUSIC_EN
    { "net_music",      APP_MSG_NET_MUSIC_BOTTOM,       APP_MSG_NET_MUSIC_TOP,      APP_MODE_NET },
#endif
#if TCFG_APP_MUSIC_EN
    { "local_music",    APP_MSG_LOCAL_MUSIC_BOTTOM,     APP_MSG_LOCAL_MUSIC_TOP,    APP_MODE_LOCAL },
#endif
#if TCFG_APP_LINEIN_EN
    { "linein_music",   APP_MSG_LINEIN_MUSIC_BOTTOM,    APP_MSG_LINEIN_MUSIC_TOP,   APP_MODE_LINEIN },
#endif
#if TCFG_APP_PC_EN
    { "pc_music",       APP_MSG_PC_MUSIC_BOTTOM,        APP_MSG_PC_MUSIC_TOP,       APP_MODE_PC },
#endif
#if TCFG_APP_RECORD_EN
    { "recorder",       APP_MSG_RECORDER_BOTTOM,        APP_MSG_RECORDER_TOP,       APP_MODE_RECORDER },
#endif
#if TCFG_LOCAL_TWS_ENABLE
    { "sink_music",     APP_MSG_SINK_MUSIC_BOTTOM,      APP_MSG_SINK_MUSIC_TOP,     APP_MODE_SINK },
#endif
};

__attribute__((weak))
void ai_app_local_event_notify(u8 status)
{

}

int app_send_message(int msg, int argc, ...)
{
    int argv[8];
    va_list argptr;

    va_start(argptr, argc);

    argv[0] = 0;

    for (int i = 0; i < ARRAY_SIZE(app_mode_table); ++i) {
        if (msg > app_mode_table[i].app_msg_bottom && msg < app_mode_table[i].app_msg_top) {
            argv[0] = (int)app_mode_table[i].app_name;
            break;
        }
    }

    argv[1] = msg;

    for (int i = 0; i < argc && i < ARRAY_SIZE(argv) - 2; i++) {
        argv[i + 2] = va_arg(argptr, int);
    }

    va_end(argptr);

    int ret = os_taskq_post_type("app_core", MSG_FROM_APP, argc + 2, argv);
    if (ret != OS_NO_ERR)  {
        log_error("send msg 0x%x fail, ret = %d", msg, ret);
    }

    return ret;
}

bool current_app_in_mode(app_mode_t mode)
{
    struct application *app = __get_current_app();
    if (!app) {
        return FALSE;
    }

    for (int i = 0; i < ARRAY_SIZE(app_mode_table); ++i) {
        if (mode == app_mode_table[i].app_mode) {
            return !strcmp(app->name, app_mode_table[i].app_name) ? TRUE : FALSE;
        }
    }

    return FALSE;
}

app_mode_t get_current_app_mode(void)
{
    struct application *app = __get_current_app();
    if (!app) {
        return APP_MODE_IDLE;
    }

    for (int i = 0; i < ARRAY_SIZE(app_mode_table); ++i) {
        if (!strcmp(app->name, app_mode_table[i].app_name)) {
            return app_mode_table[i].app_mode;
        }
    }

    return APP_MODE_IDLE;
}

void app_mode_change(app_mode_t mode)
{
    struct intent it;
    init_intent(&it);

    for (int i = 0; i < ARRAY_SIZE(app_mode_table); ++i) {
        if (mode == app_mode_table[i].app_mode) {
            it.name = app_mode_table[i].app_name;
            start_app(&it);
            return;
        }
    }
}

void app_mode_change_replace(app_mode_t mode)
{
    app_mode_t curr_mode = get_current_app_mode();

    struct intent it;
    init_intent(&it);

    for (int i = 0; i < ARRAY_SIZE(app_mode_table); ++i) {
        if (mode == app_mode_table[i].app_mode) {
            it.name = app_mode_table[i].app_name;
            if (curr_mode != APP_MODE_BT && curr_mode != APP_MODE_NET) {
                it.action = ACTION_REPLACE;
            }
            start_app(&it);
            return;
        }
    }
}

void app_mode_change_next(void)
{
    struct intent it;
    init_intent(&it);

    app_mode_t curr_mode = get_current_app_mode();

    if (curr_mode > APP_MODE_BT) {
        it.action = ACTION_REPLACE;
    }

    do {
        if (++curr_mode == APP_MODE_SINK || curr_mode < APP_MODE_LOCAL) {
            curr_mode = APP_MODE_LOCAL;
        }

        for (int i = 0; i < ARRAY_SIZE(app_mode_table); ++i) {
            if (curr_mode == app_mode_table[i].app_mode) {
                it.name = app_mode_table[i].app_name;
                //清掉上一个app的恢复断点
                app_msg_handler(NULL, APP_MSG_STOP);
                if (0 == start_app(&it)) {
                    return;
                }
            }
        }
    } while (1);
}

void app_mode_go_back(void)
{
    struct intent it;
    init_intent(&it);
    it.action = ACTION_BACK;
    start_app(&it);
}
