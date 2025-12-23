#ifndef _APP_EVENT_H_
#define _APP_EVENT_H_

#include "event/event.h"


enum app_event_from {
    APP_EVENT_FROM_USER = 1,
    APP_EVENT_FROM_PROTOCOL = 2,
    APP_EVENT_FROM_AUDIO = 3,
    APP_EVENT_FROM_UI = 4
};

typedef enum {
    APP_EVENT_ACTIVATION,
    APP_EVENT_OTA_START,
    APP_EVENT_OTA_END,
    APP_EVENT_WIFI_CFG_FINISH,
    APP_EVENT_TELNET_DEBUG_STARTED,
    APP_EVENT_MQTT_CONNECTION_PARAM,
    APP_EVENT_DATA,
    APP_EVENT_KEY,
    APP_EVENT_WAKEUP_WORD_DETECTED
}app_event_type_t;

typedef enum{
    APP_EVENT_MQTT_CONNECTION_STATUS,
    APP_EVENT_SERVER_NOTIFY,
    APP_EVENT_CALL_ESTABLISHED,
    APP_EVENT_CALL_REJECTED,
    APP_EVENT_CALL_NO_ANSWER,
    APP_EVENT_CALL_UPDATED,
    APP_EVENT_CALL_SERVER_TERMINATED,
    APP_EVENT_CALL_TERMINATE_ACK,
    APP_EVENT_AUDIO_PLAY_END_NOTIFY,
    APP_EVENT_AUDIO_ENC_NOTIFY
}protocol_event_type_t;

struct app_event {
    u8 event;
    void *arg;
};

int app_event_notify(enum app_event_from from, struct app_event *event);

#endif

