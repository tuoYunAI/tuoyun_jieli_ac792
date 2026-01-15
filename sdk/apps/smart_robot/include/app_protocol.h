#ifndef __SIP_PROTOCOL_H__
#define __SIP_PROTOCOL_H__




#ifdef __cplusplus
extern "C" {
#endif


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
 * Device session error codes
 */
typedef enum {
    /**
     * The server did not answer
     */
    CALL_ERROR_SERVER_NO_ANSWER = 1000,
    /**
     * Device not authorized
     */
    CALL_ERROR_MEMBERSHIP_INVALID = 1001
} session_call_error_t;

typedef enum{
    SERVER_CMD_REBOOT = 1,
} server_op_command_t;

typedef enum{
    SERVER_MESSAGE_STATUS = 0,
    SERVER_MESSAGE_MSG
} server_message_type_t;

typedef enum{
    SERVER_STATUS_MEMBERSHIP_INVALID = 1,
    SERVER_STATUS_ACTIVATED
} server_message_status_t;

typedef enum{
    MESSAGE_INFO = 1,
    MESSAGE_ALERT
} server_message_level_t;

/**
 * Device control events corresponding to the definition of which sent by the server, 
 */
typedef enum{
    CTRL_EVENT_USER_TEXT,
    CTRL_EVENT_SPEAKER,
    CTRL_EVENT_LISTEN
}session_update_cmd_t;



/**
 * Device control message notification event parameters from the server
 */
typedef struct {
    session_call_error_t event;
    char message[64];
}session_call_error_event_t, *session_call_error_event_ptr;


/**
 * Device control message notification event parameters from the server
 */
typedef struct {
    server_message_type_t event;
    union
    {
        server_message_status_t status;
        server_message_level_t level;
    };
    
    char command[10];
    char message[64];
    char emotion[32];
}server_message_notify_t, *server_message_notify_ptr;


/**
 * Device session status change event parameters from the server
 */
typedef struct{
    session_update_cmd_t event;
    device_working_status_t status;
    char emotion[32];
    char text[1024];
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
    unsigned char nonce[16];   // AES 初始向量/nonce
    unsigned char aes_key[16]; // AES 128位密钥
}media_parameter_t, *media_parameter_ptr;



/**
 * @brief  Initialize session
 * @param  wake_up_word: Wake-up word string; if wake-up word detection is not needed, pass NULL.
 * @return 0: Initialization successful
 *     Other: Initialization failed
 */
int init_call(const char* wake_up_word);



/**
 * @brief  End current session
 * @return 0: End successful
 *     Other: End failed
 */
int finish_call();


/**
 * @brief  Send start listening command to server
 * @return 0: Send successful
 *     Other: Send failed
 */
void send_start_listening(listening_mode_t mode);

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
 * @brief  Initialize session module
 * @param  uid: Device UID
 * @param  device_ip: Device IP address
 */
void init_session_module(const char* uid, const char* device_ip);


/**
 * @brief  Handle received MQTT message containing SIP data
 * @param  data: Pointer to the received data
 * @param  len: Length of the received data
 */
void handle_received_mqtt_message(const char *data, size_t len);     

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
 * @brief  Start the protocol module, including MQTT connection initialization, etc.
 *         If the MQTT connection parameters change, the device needs to be reboot and call this function again.
 * @param  mqtt_info_ptr: Pointer to the MQTT connection parameter structure
 * @return 0: Start successful
 *     Other: Start failed
 */
int start_protocol(mqtt_connection_parameter_ptr mqtt_info_ptr);


/**
 * @brief  Send audio data packet to server
 * @param  packet: Pointer to audio data packet structure
 * @return 0: Send successful
 *     Other: Send failed
 */
int send_audio(audio_stream_packet_ptr packet);         

void test_print_session_state();

#ifdef __cplusplus
}
#endif

#endif