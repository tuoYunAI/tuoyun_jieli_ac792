#ifndef _APP_EVENT_H_
#define _APP_EVENT_H_

#include "event/event.h"


enum app_event_from {

    /**
     * 
     * User defined event.
     */
    APP_EVENT_FROM_USER = 1,

    /**
     * Protocol stack event, from protocol module.
     */
    APP_EVENT_FROM_PROTOCOL = 2,

    /**
     * Audio module event, from audio module.
     */
    APP_EVENT_FROM_AUDIO = 3,

    /**
     * UI module event, from UI module.
     */
    APP_EVENT_FROM_UI = 4
};


/**
 * 
 * Definition of user defined events.
 */
typedef enum {
    
    /**
     * Notification of starting to connect to WiFi, used to notify the application 
     * layer the target WiFi SSID.
     */
    APP_EVENT_CONNECTING_TO_WIFI,
    

    /**
     * Notification of connection to WiFi interruption, used to notify the application
     * layer that the WiFi connection has been interrupted.
     */
    APP_EVENT_WIFI_DISCONNECTED,

    /**
     * Device inactive notification, indicating the device is successfully 
     * bound to the user account and cannot perform normal business operations.
     */
    APP_EVENT_ACTIVATION,
    
    /**
     * OTA upgrade start notification, used to notify the application
     * layer to enter the OTA upgrade process.
     */
    APP_EVENT_OTA_START,
    /**
     * OTA upgrade end notification, used to notify the application layer 
     * that the OTA upgrade process is completed and the device can be restarted.
     */
    APP_EVENT_OTA_END,
    /**
     * WiFi configuration completion notification, used to notify the application 
     * layer that WiFi configuration is completed, SSID and password verification 
     * succeeded, and the device can be restarted.
     */
    APP_EVENT_WIFI_CFG_FINISH,
    /**
     * Telnet debug log channel has been initialized, subsequent initialization 
     * can be performed.
     */
    APP_EVENT_TELNET_DEBUG_STARTED,
    /**
     * Received MQTT connection parameter notification, used to notify the application 
     * layer to get the latest MQTT connection parameters.
     */
    APP_EVENT_MQTT_CONNECTION_PARAM,
    
}app_event_type_t;

/**
 * Protocol module event definition.
 */
typedef enum{

    /**
     * When MQTT connection status changes, used to notify the application layer of the 
     * current connection status.
     */
    APP_EVENT_MQTT_CONNECTION_STATUS,

    /**
     * Forward server event notifications, such as device successful binding, tariff recharge 
     * status changes, etc.
     */
    APP_EVENT_SERVER_NOTIFY,

    /**
     * Session established notification, indicating that signaling and media channels have been 
     * successfully established, and the terminal enters session interaction.
     */
    APP_EVENT_CALL_ESTABLISHED,

    /**
     * Session rejected notification, indicating that the other party rejected this session request.
     */
    APP_EVENT_CALL_REJECTED,

    /**
     * Session no answer notification, indicating that the terminal initiated the session but 
     * timed out waiting for the other party to answer.
     */
    APP_EVENT_CALL_NO_ANSWER,

    /**
     * Session status update notification, used to notify the application layer of the current 
     * session status changes, such as start/stop dictation, etc.
     */
    APP_EVENT_CALL_UPDATED,

    /**
     * Server-side session termination notification, indicating that the server side released 
     * the current session.
     */
    APP_EVENT_CALL_SERVER_TERMINATED,

    /**
     * Session termination confirmation notification, indicating that the terminal initiated 
     * termination and the server confirmed the local session release request.
     */
    APP_EVENT_CALL_TERMINATE_ACK
}protocol_event_type_t;


typedef enum{
    /**
     * Wake-up word detected notification, used to notify the application layer that the wake-up 
     * word has been detected.
     */
    APP_EVENT_WAKEUP_WORD_DETECTED,

    /**
     * Audio playback end notification, used to notify the application layer that the current 
     * audio playback has ended and the server can be notified to enter the dictation state.
     */
    APP_EVENT_AUDIO_PLAY_END_NOTIFY,


    APP_EVENT_AUDIO_SPEAK_BEFORE_OPEN,
    APP_EVENT_AUDIO_SPEAK_AFTER_OPEN,
    APP_EVENT_AUDIO_SPEAK_BEFORE_CLOSE,
    APP_EVENT_AUDIO_SPEAK_AFTER_CLOSE,


    APP_EVENT_AUDIO_MIC_BEFORE_OPEN,
    APP_EVENT_AUDIO_MIC_AFTER_OPEN,
    APP_EVENT_AUDIO_MIC_BEFORE_CLOSE,
    APP_EVENT_AUDIO_MIC_AFTER_CLOSE,
}audio_event_type_t;


/**
 * UI module event definition.
 */
typedef enum{

    /**
     * Key event notification, used to forward UI module key events.
     */
    APP_EVENT_KEY,
}ui_event_type_t;


/**
 * Structure for application layer custom event parameters.
 */
typedef struct app_event {
    u8 event;
    void *arg;
}app_event_t, *app_event_ptr;

/**
 * @brief  Notify application layer of event occurrence
 * @param  from: Event source
 * @param  event: Pointer to Event
 * @return 0: Notification successful
 *     Other: Notification failed
 * @note 
 */
int app_event_notify(enum app_event_from from, struct app_event *event);

#endif

