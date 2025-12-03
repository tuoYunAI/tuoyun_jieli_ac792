#include "app_config.h"
#include "app_event.h"
#include "os/os_api.h"
#include "system/app_core.h"


#define LOG_TAG             "[EVENT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
// #define LOG_DEBUG_ENABLE
// #define LOG_DUMP_ENABLE
// #define LOG_CHAR_ENABLE
#include "system/debug.h"

int app_event_notify(enum app_event_from from, struct app_event *event){
    return sys_event_notify(SYS_APP_EVENT, from, event, sizeof(struct app_event));
}