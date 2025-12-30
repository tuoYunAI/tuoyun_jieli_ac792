#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

/**
 * OPUS audio frame duration of this device, unit: ms
 */
#define OPUS_FRAME_DURATION_MS 60

/**

 * Device control event name in SIP message payload
 */
#define DEVICE_CTRL_EVENT_ALERT       "alert"
#define DEVICE_CTRL_EVENT_ACTIVATE    "activate"
#define DEVICE_CTRL_EVENT_EXPIRE      "expire"
#define DEVICE_CTRL_EVENT_STT         "stt"
#define DEVICE_CTRL_EVENT_LISTEN      "listen"
#define DEVICE_CTRL_EVENT_SPEAKER     "speaker"


#define WORKING_STATUS_START_TXT      "start"
#define WORKING_STATUS_STOP_TXT       "stop"
#define WORKING_STATUS_TEXT_TXT       "text"
#define WORKING_STATUS_SENTENCE_START   "sentence_start"

/**
 * Device control events corresponding to the definition of which sent by the server, 
 */
typedef enum{
    CTRL_EVENT_ALERT = 1,
    CTRL_EVENT_ACTIVATE,
    CTRL_EVENT_EXPIRE,
    CTRL_EVENT_SHOW_TEXT,
    CTRL_EVENT_SPEAKER,
    CTRL_EVENT_LISTEN
}device_ctrl_event_t;

/**
 * Interruption reasons
 */
typedef enum  {
    ABORT_REASON_NONE = 0,
    ABORT_REASON_WAKE_WORD_DETECTED
}abort_reason_t;


typedef enum  {
    LISTENING_MODE_AUTO_STOP = 0,
    LISTENING_MODE_MANUAL_STOP,
    LISTENING_MODE_REALTIME // 需要 AEC 支持
} listening_mode_t;

/**
 * Working status of device microphone and speaker
 */
typedef enum{
    WORKING_STATUS_STOP_PENDING = 0,
    WORKING_STATUS_STOP = 1,
    WORKING_STATUS_START = 10,
    WORKING_STATUS_TEXT = 20,
    WORKING_STATUS_INVALID = 99
}device_working_status_t;


/**
 * MQTT connection status
 */
typedef enum {

    /**
     * MQTT connection interrupted
     */
    MQTT_STATUS_INTERRUPTED = 0,

    /**
     * MQTT connection established
     */
    MQTT_STATUS_ENSTABLISHED = 1
} mqtt_status_t;

/**
 * Device session error codes
 */
typedef enum {
    /**
     * Device not authorized
     */
    CALL_ERROR_MEMBERSHIP_INVALID = 1001
} session_call_error_t;


/**
 * MQTT connection parameters
 */
typedef struct mqtt_connection_parameter{
    int port;
    char end_point[128];
    char client_id[64];
    char user_name[64];
    char password[64];
    /**
     * Topic name for messages sent by this device
     */
    char topic_pub[64];

    /**
     * This device listens for messages under this topic
     */
    char topic_sub[64];
} mqtt_connection_parameter_t, *mqtt_connection_parameter_ptr;


/**
 * Structure of audio data packet sent to the server via udp
 */
typedef struct  {
    int sample_rate;
    int frame_duration;
    unsigned long timestamp;
    /**
     * Pointer to audio data payload; Caller needs to manage the pointed memory
     */
    char* payload;
    uint16_t payload_len;
} audio_stream_packet_t, *audio_stream_packet_ptr;

/**
 * Device control message notification event parameters from the server
 */
typedef struct {
    device_ctrl_event_t event;
    char status[10];
    char message[64];
    char emotion[32];
}message_notify_event_t, *message_notify_event_ptr;

/**
 * Device session status change event parameters from the server
 */
typedef struct{
    device_ctrl_event_t event;
    device_working_status_t status;
    char emotion[32];
    char text[256];
}message_session_event_t, *message_session_event_ptr;


/**
 * Media parameters
 * Including audio codec parameters and encryption parameters
 * When the session is initiated, the device needs to negotiate media parameters with the server
 */
typedef struct{
    char ip[16];
    int port;
    char codec[16]; // "opus"
    char transport[8]; // "udp"
    int sample_rate; // 24000
    int channels; // 1
    int frame_duration; // 60 ms
    char encryption[16]; // "aes-128-cbc"
    uint8_t nonce[16];   // AES 初始向量/nonce
    uint8_t aes_key[16]; // AES 128位密钥
}media_parameter_t, *media_parameter_ptr;


/**
 * @brief  Start the protocol module, including MQTT connection initialization, etc.
 *         If the MQTT connection parameters change, the device needs to be reboot and call this function again.
 * @param  mqtt_info_ptr: Pointer to the MQTT connection parameter structure
 * @return 0: Start successful
 *     Other: Start failed
 */
int start_protocol(mqtt_connection_parameter_ptr mqtt_info_ptr);

/**
 * @brief  Initialize session
 * @param  wake_up_word: Wake-up word string; if wake-up word detection is not needed, pass NULL.
 * @return 0: Initialization successful
 *     Other: Initialization failed
 */
int init_call(char* wake_up_word);

/**
 * @brief  End current session
 * @return 0: End successful
 *     Other: End failed
 */
int finish_call();

/**
 * @brief  Send audio data packet to server
 * @param  packet: Pointer to audio data packet structure
 * @return 0: Send successful
 *     Other: Send failed
 */
int send_audio(audio_stream_packet_ptr packet);

/**
 * @brief  Send start listening command to server
 * @return 0: Send successful
 *     Other: Send failed
 */
void send_start_listening();

/**
 * @brief  Send stop listening command to server
 * @return 0: Send successful
 *     Other: Send failed
 */
void send_stop_listening();

/**
 * @brief  Send abort current speech synthesis command to server
 * @param  reason: Abort reason
 * @return 0: Send successful
 *     Other: Send failed
 */
void send_abort_speaking(abort_reason_t reason);

/**
 * @brief  Send message to server via MQTT
 * @param  message: Pointer to the message string to be sent
 * @return void
 */
void transmit_mqtt_message(char* message);


void test_print_session_state();
void test_print_traffic_state();
#endif