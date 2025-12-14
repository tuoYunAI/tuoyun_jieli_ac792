#include "app_config.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "mqtt/MQTTClient.h"
#include "system/includes.h"
#include "app_event.h"
#include "app_wifi.h"
#include "app_protocol.h"
#include "session/session.h"

#include "json_c/json_object.h"
#include "json_c/json_tokener.h"

#include "osipparser2/osip_list.h"

#include "mbedtls/aes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define LOG_TAG             "[PROTOCOL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


  
#define COMMAND_TIMEOUT_MS 30000        //命令超时时间
#define MQTT_TIMEOUT_MS  30           //接收阻塞时间
#define MQTT_KEEPALIVE_TIME 30000       //心跳时间
#define SEND_BUF_SIZE 4096              //接收buf大小
#define READ_BUF_SIZE 2048              //接收buf大小
static char read_buf[READ_BUF_SIZE];    //接收buf
static char send_buf[SEND_BUF_SIZE];    //接收buf

static OS_MUTEX mutex;
static int m_protocol_inited = 0;

static mqtt_connection_parameter_ptr m_mqtt_info = NULL;

static Client m_mqtt_client;

extern uint32_t get_system_ms(void);



void transmit_mqtt_message(char* message){
    
    if (!m_protocol_inited || !m_mqtt_client.isconnected){
        log_info("protocol not inited or mqtt not online\n");
        return;
    }

    MQTTMessage mqtt;
    mqtt.qos = QOS1;
    mqtt.retained = 0;
    mqtt.payload = message;
    mqtt.payloadlen = strlen(message);
    //发布消息
    os_mutex_pend(&mutex, 0);
    memset(send_buf, 0, sizeof(send_buf));
    if(!m_mqtt_client.isconnected){
        log_error("mqtt not online\n");
        os_mutex_post(&mutex);
        return;
    }
    
    int err = MQTTPublish(&m_mqtt_client, m_mqtt_info->topic_pub, &mqtt);
    if (err != 0) {
        log_info("MQTTPublish fail, err : 0x%x\n", err);
    }

    os_mutex_post(&mutex);
    //log_info("MQTTPublish payload:(%.30s)\n", message);
}


static osip_list_t m_received_sip_list;
static OS_MUTEX m_sip_mutex;

//接收回调，当订阅的主题有信息下发时，在这里接收
static void message_arrived(MessageData *data)
{
    if (!data || data->topicName == NULL || data->message == NULL){
        log_info("message_arrived data err\n");
        return;
    }
    
    int len = data->message->payloadlen + 1;
    u8* buf = malloc(len);
    if (!buf){
        log_info("message_arrived malloc err: %d\n", data->message->payloadlen);
        return;
    }

    memset(buf, 0, len);
    strncpy(buf, data->message->payload, len - 1);
    
    os_mutex_pend(&m_sip_mutex, 0);
    
    if (osip_list_add(&m_received_sip_list, buf, -1) < 0){
        log_info("failed to add received message to list\n");
        free(buf);
        os_mutex_post(&m_sip_mutex);
        return;
    }
    os_mutex_post(&m_sip_mutex);
    return;   
}


void protocol_mqtt_proc_task(){
    while (1) {
        os_mutex_pend(&m_sip_mutex, 0);
        int list_size = osip_list_size(&m_received_sip_list);
        if (list_size == 0) {
            os_mutex_post(&m_sip_mutex);
            os_time_dly(5);
            continue;
        }
        u8* msg = (char*)osip_list_get(&m_received_sip_list, 0);
        if (msg) {
            osip_list_remove(&m_received_sip_list, 0);
        }
        os_mutex_post(&m_sip_mutex);

        if (msg) {
            // 处理接收到的消息
            json_object *root = json_tokener_parse(msg);
            if(root != NULL){
                char protocol[10] = {0};
                strcpy(protocol, json_get_string(root, "protocol"));
                char* payload = json_get_string(root, "payload");
                if (strcmp("SIP", protocol) == 0){
                    handle_received_sip(payload, strlen(payload));
                }else 
                if (strcmp("MCP", protocol) == 0){
                    handle_received_mcp_request(payload, strlen(payload));
                }else{
                    
                    log_info("received invalid protocol: %s", protocol);
                }
                
                json_object_put(root);
                root = NULL;    
                
            }
            free(msg);
        }
    }
}



static Network network;

void protocol_mqtt_recv_task(void)
{
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    struct app_event event = {
        .event = APP_EVENT_MQTT_CONNECTION_STATUS,
        .arg = MQTT_STATUS_INTERRUPTED
    };
    

    do{
    
        //初始化网络接口
        NewNetwork(&network);
        SetNetworkRecvTimeout(&network, 1000);
        log_info("Network inited\n");
        //初始化客户端
        MQTTClient(&m_mqtt_client, &network, COMMAND_TIMEOUT_MS, send_buf, sizeof(send_buf), read_buf, sizeof(read_buf));
        
        //tcp层连接服务器
        log_info("Connecting to %s:%d\n", m_mqtt_info->end_point, m_mqtt_info->port);
        int err = ConnectNetwork(&network, m_mqtt_info->end_point, m_mqtt_info->port);
        if (err != 0) {
            log_info("ConnectNetwork fail\n");
            os_time_dly(3000);
            continue;;
        }

        connectData.willFlag = 0;
        connectData.MQTTVersion = 3;                                   //mqtt版本号
        connectData.clientID.cstring = m_mqtt_info->client_id;         //客户端id
        connectData.username.cstring = m_mqtt_info->user_name;         //连接时的用户名
        connectData.password.cstring = m_mqtt_info->password;          //连接时的密码
        connectData.keepAliveInterval = MQTT_KEEPALIVE_TIME / 1000;    //心跳时间
        connectData.cleansession = 1;                                  //是否使能服务器的cleansession，0:禁止, 1:使能

        //mqtt层连接,向服务器发送连接请求
        err = MQTTConnect(&m_mqtt_client, &connectData);
        if (err != 0) {
            network.disconnect(&network);
            log_info("MQTTConnect fail, err : 0x%x\n", err);
            os_time_dly(3000);
            continue;;
        }
        
        log_info("MQTTConnect success\n");

        //订阅主题
        err = MQTTSubscribe(&m_mqtt_client, m_mqtt_info->topic_sub, QOS1, message_arrived);
        if (err != 0) {
            MQTTDisconnect(&m_mqtt_client);
            network.disconnect(&network);
            log_info("MQTTSubscribe fail, err : 0x%x\n", err);
            os_time_dly(3000);
            continue;
        }
        log_info("MQTTSubscribe success, topic : %s\n", m_mqtt_info->topic_sub);
        //取消主题订阅
        //MQTTUnsubscribe(&client, subscribeTopic);
        event.arg = MQTT_STATUS_ENSTABLISHED;
        app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
        send_register();

        for (;;) {

            os_mutex_pend(&mutex, 0);
            err = MQTTYield(&m_mqtt_client, MQTT_TIMEOUT_MS);
            if (err != 0) {
                if (m_mqtt_client.isconnected) {
                    //断开mqtt层连接
                    err = MQTTDisconnect(&m_mqtt_client);
                    if (err != 0) {
                        log_info("MQTTDisconnect fail\n");
                    }

                    //断开tcp层连接
                    network.disconnect(&network);
                }
                log_info("MQTT : Reconnecting\n");
                event.arg = MQTT_STATUS_INTERRUPTED;
                app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
                os_mutex_post(&mutex);
                break;
            }
            os_mutex_post(&mutex);
            os_time_dly(5);
        }
        
    }while (1);

    return;
}


int start_protocol(mqtt_connection_parameter_ptr mqtt_info_ptr){
    if (m_protocol_inited) {
        log_info("protocol already inited\n");
        return 0;
    }

    osip_list_init(&m_received_sip_list);
    os_mutex_create(&m_sip_mutex);
    os_mutex_create(&mutex);
    m_protocol_inited = 1;
    m_mqtt_info = mqtt_info_ptr;

    u32 addr = get_wifi_ip();
    if (addr == 0) {
        log_info("no local ip got");
        return -1;
    }
    const char *str = inet_ntoa(addr);
    
    init_session_module(m_mqtt_info->user_name, str);

    log_info("start to init protocol\n");

    if (thread_fork("protocol_mqtt_receive", 25, 1024, 0, NULL, protocol_mqtt_recv_task, NULL) != OS_NO_ERR) {
        log_info("thread fork fail\n");
        return -1;
    }

    if (thread_fork("protocol_mqtt_proc", 25, 2028, 0, NULL, protocol_mqtt_proc_task, NULL) != OS_NO_ERR) {
        log_info("thread fork fail\n");
        return -1;
    }
    
    

    sys_timer_add_to_task("protocol_mqtt_proc", NULL, session_checking, 500 * 100);
    
    sys_timer_add_to_task("protocol_mqtt_proc", NULL, send_register, 20 * 1000);

    return 0;
}
