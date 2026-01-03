#include "app_config.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/includes.h"
#include "app_event.h"
#include "app_wifi.h"
#include "app_protocol.h"
#include "session.h"
#include "sip/sip.h"
#include "traffic/traffic.h"

#include "json_c/json_object.h"
#include "json_c/json_tokener.h"

/**
 * 用来表示被修饰的指针必须使用malloc申请, 将由函数移动所有权（即函数内部会释放该指针），
 * 调用者在调用后不应再使用该指针。
 */
#define MOVE 


#define LOG_TAG             "[SESION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


#define REGISTER_EXPIRE_SECOND  300     //注册间隔5分钟   
#define COMMAND_TIMEOUT_MS 30000        //命令超时时间
#define SESSION_SUPORT_MCP       1       //是否支持MCP扩展
#define SESSION_OPUS_CBR         1       //opus是否使用cbr编码
#define SESSION_AUDIO_FRAME_GAP  10      //音频帧间隔ms

extern uint32_t get_system_ms(void);
static void send_invite_ack();

static session_state_machine_t m_session_state = {
    .protocol_inited = 0,
    .uid = {0},
    .session_status = SESSION_STATUS_IDLE,
    .seq = 0,
    .last_keepalive_ms = 0,
    .invite_200_ok_resp_message = NULL,
    .device_ip = {0}
};


static OS_MUTEX mutex;

int check_if_session_in_call(){
    return m_session_state.session_status == SESSION_STATUS_IN_CALL;
}

void test_print_session_state(){
    log_info("+++++++++++++++++++++session status+++++++++++++++\r\n"
        "        protocol_inited: %d\r\n"
        "        session_status: %d\r\n"
        "        seq: %u\r\n"
        "        last_keepalive_ms: %u\r\n"
        "        last_req_message_ms: %u\r\n"
        "        last_req_message_seq: %u\r\n"
        "        invite_200_ok_resp_message: %p\r\n"
        "        device_ip: %s\r\n"
        "++++++++++++++++++++++++++++++++++++++++++++++++\r\n",
         m_session_state.protocol_inited,
         m_session_state.session_status,
         m_session_state.seq,
         m_session_state.last_keepalive_ms,
         m_session_state.last_req_message_ms,
         m_session_state.last_req_message_seq,
         m_session_state.invite_200_ok_resp_message,
         m_session_state.device_ip[0] ? m_session_state.device_ip : "null"         
    );    
}


static int transmit_sip(char *message){
    if (!message){
        return -1;
    }

    json_object *root = json_object_new_object();
    json_object *protocol = json_object_new_string("SIP");
    json_object *payload = json_object_new_string(message); 
    
    json_object_object_add(root, "protocol", protocol);
    json_object_object_add(root, "payload", payload);
    
    const char* json_string = json_object_to_json_string(root);
    transmit_mqtt_message((char*)json_string);
    json_object_put(root);
    return 0;
}



static void proc_response_register(MOVE received_sip_message_ptr  message){

    log_info("Received REGISTER response with status code: %d\n", message->status_code);
    os_mutex_pend(&mutex, 0);
    do{
        if (m_session_state.session_status != SESSION_STATUS_REGISTERING){
            log_info("unexpected register response, current state: %d\n", m_session_state.session_status);
            break;
        }
        if (is_response_ok(message)){ 
            log_info("Register successful\n");
            m_session_state.last_keepalive_ms = get_system_ms();
        } else {
            log_info("Register failed with status code: %d\n", message->status_code);
        }
        m_session_state.session_status = SESSION_STATUS_IDLE;
        m_session_state.last_keepalive_ms = m_session_state.last_req_message_ms;
        m_session_state.last_req_message_ms = 0;
        m_session_state.last_req_message_seq = 0;
    }while(0);
    test_print_session_state();
    os_mutex_post(&mutex);
    free(message);
    
    return;
}

static int clear_session(){
    os_mutex_pend(&mutex, 0);
    m_session_state.session_status = SESSION_STATUS_IDLE;
    m_session_state.last_req_message_ms = 0;
    m_session_state.last_req_message_seq = 0;
    
    if (m_session_state.invite_200_ok_resp_message){
        free(m_session_state.invite_200_ok_resp_message);
        m_session_state.invite_200_ok_resp_message = NULL;
    }
    os_mutex_post(&mutex);
    return 0;
}



static int proc_response_invite(MOVE received_sip_message_ptr message){

    os_mutex_pend(&mutex, 0);
    int ret = 0;
    do
    {
        if (m_session_state.session_status != SESSION_STATUS_INVITING){
            log_info("unexpected invite response, current state: %d\n", m_session_state.session_status);
            ret = -1;
            break;
        }
        
        if (is_response_ok(message) && message->cseq_num == m_session_state.last_req_message_seq) {
            
            log_info("Invite accepted\n");
            
            // 进入通话状态
            m_session_state.invite_200_ok_resp_message = message;

            m_session_state.session_status = SESSION_STATUS_IN_CALL;
            m_session_state.last_keepalive_ms = get_system_ms();
            m_session_state.last_req_message_ms = 0;
            m_session_state.last_req_message_seq = 0;
            strncpy(m_session_state.session_id, message->call_id, sizeof(m_session_state.session_id)-1);

            downlink_sdp_parameter_t sdp = {0};
            if (message->body_length > 0 && message->message_body) {
                // 解析 SDP
                if (parse_sdp(message->message_body, &sdp) == 0) {
                    log_info("SDP accepted\n");
                    // 这里可以根据解析结果配置音频参数
                    media_parameter_t media_param = {0};
                    strncpy(media_param.ip, sdp.ip, sizeof(media_param.ip)-1);
                    media_param.port = sdp.port;
                    strncpy(media_param.codec, sdp.codec, sizeof(media_param.codec)-1);
                    strncpy(media_param.transport, sdp.transport, sizeof(media_param.transport)-1);
                    media_param.sample_rate = sdp.sample_rate;
                    media_param.channels = sdp.channels;
                    media_param.frame_duration = sdp.frame_duration;
                    strncpy(media_param.encryption, sdp.encryption, sizeof(media_param.encryption)-1);
                    memcpy(media_param.nonce, sdp.nonce, sizeof(media_param.nonce));
                    memcpy(media_param.aes_key, sdp.aes_key, sizeof(media_param.aes_key));

                    if (start_traffic_tunnel(&media_param) == 0){
                        struct app_event event = {
                            .event = APP_EVENT_CALL_ESTABLISHED,
                            .arg = &media_param
                        };
                        app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
                        // 发送 ACK 确认
                        send_invite_ack();
                        break;
                    }
                }
            }
        }else{
            log_info("Invite request rejected with status code: %d", message->status_code);
            if(message->status_code == 403){
                log_info("Invite request rejected");
                if (message->x_reason_code == CALL_ERROR_MEMBERSHIP_INVALID){

                    log_info("Reason: membership expired");
                    message_notify_event_t notify = {       
                        .event = CTRL_EVENT_EXPIRE
                    };
                    struct app_event event = {
                        .event = APP_EVENT_SERVER_NOTIFY,
                        .arg = &notify
                    };
                    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
                }
                
            }
        }

        ret = -1;
    } while (0);

    if (ret != 0){
        m_session_state.session_status = SESSION_STATUS_IDLE;
        m_session_state.last_req_message_ms = 0;
        m_session_state.last_req_message_seq = 0;
        m_session_state.invite_200_ok_resp_message = NULL;     
        free(message);      
    }
    os_mutex_post(&mutex);
    return ret;
}


static void proc_response_info(MOVE received_sip_message_ptr message){
    log_info("Received %s response with status code: %d\n", message->method, message->status_code);
    
    if (message->cseq_num != m_session_state.last_req_message_seq){
        log_info("Unexpected CSeq in INFO response, expected: %u, got: %u\n", m_session_state.last_req_message_seq, message->cseq_num);
        return;
    }
    if (is_response_ok(message)) {
        log_info("INFO request acknowledged\n");    
    }
    m_session_state.last_req_message_ms = 0;
    m_session_state.last_req_message_seq = 0;  
    free(message);
    return;
}

static void proc_response_bye(MOVE received_sip_message_ptr message){
    log_info("Received BYE response with status code: %d\n", message->status_code);
    if (is_response_ok(message)) {
        log_info("Call terminated successfully\n");
    } 

    clear_session();
    clear_traffic_tunnel();
    struct app_event event = {
        .event = APP_EVENT_CALL_TERMINATE_ACK,
        .arg = NULL
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);  
    free(message);
    return;
}

static void proc_message(json_object* parse, message_notify_event_ptr notify){

    char ev[16] = {0};
    strncpy(ev, json_get_string(parse, "event"), sizeof(ev)-1);
    char *val = json_get_string(parse, "command");
    if (val){
        strncpy(notify->command, val, sizeof(notify->command)-1);
    }
    val = json_get_string(parse, "text");
    if (val){
        strncpy(notify->message, val, sizeof(notify->message)-1);
    }
    val = json_get_string(parse, "emotion");
    if (val){
        strncpy(notify->emotion, val, sizeof(notify->emotion)-1);
    }

    log_info("Parsed MESSAGE - event: %d, status: %s, message: %s, emotion: %s\n",
                        notify->event, notify->command, notify->message, notify->emotion);
    app_event_t event = {
        .event = APP_EVENT_SERVER_NOTIFY,
        .arg = notify
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

static void proc_message_alert(json_object* data){
    message_notify_event_t notify = {
        .event = CTRL_EVENT_ALERT
    };
    proc_message(data, &notify);
}

static void proc_message_activate(json_object* data){
    message_notify_event_t notify = {
        .event = CTRL_EVENT_ACTIVATE
    };
    proc_message(data, &notify);
}


static void proc_message_expire(json_object* data){
    message_notify_event_t notify = {
        .event = CTRL_EVENT_EXPIRE
    };
    proc_message(data, &notify);
}

static void proc_request_message(MOVE received_sip_message_ptr  message){
    log_info("Received MESSAGE request\n");
    
    if (message->body_length > 0 && message->message_body) {

        log_info("MESSAGE body, len: %d: %s\n", message->body_length, message->message_body);

        // 解析 JSON
        json_object *parse = NULL;
        parse = json_tokener_parse(message->message_body);
        if (parse) {
            message_notify_event_t notify = {0};
            char ev[16] = {0};
            strncpy(ev, json_get_string(parse, "event"), sizeof(ev)-1);
            if (strcmp(ev, DEVICE_CTRL_EVENT_ALERT) == 0){
                proc_message_alert(parse);
            }else if (strcmp(ev, DEVICE_CTRL_EVENT_ACTIVATE) == 0){ 
                proc_message_activate(parse);
            }else if (strcmp(ev, DEVICE_CTRL_EVENT_EXPIRE) == 0){
                proc_message_expire(parse);
            }else {     
                log_info("unknown event type: %s\n", ev);                    
                json_object_put(parse);
                free(message);
                return;
            }
            
            json_object_put(parse);

            char *out_msg = NULL;
            size_t out_len = 0;
            int ret = build_200_ok_response(message, &out_msg, &out_len);
            if (ret != 0 || out_msg == NULL || out_len == 0){
                log_info("failed to response to MESSAGE: ret %d, out_len %d", ret, out_len);
                free(message);
                return;
            }
            transmit_sip(out_msg);
            free_sip_message(out_msg);
            out_msg = NULL;
        }else{
            log_info("failed to parse MESSAGE body to JSON\n");
        }
    }
    free(message);
    return;
}

static void proc_request_bye(MOVE received_sip_message_ptr message){
    log_info("Received BYE request\n");

    os_mutex_pend(&mutex, 0);
    if (strncmp(message->call_id, m_session_state.session_id, sizeof(m_session_state.session_id)) != 0){
        log_info("BYE call_id does not match current session, ignoring\n");
        os_mutex_post(&mutex);
        free(message);
        return;
    }
    // 处理 BYE 请求
    m_session_state.session_status = SESSION_STATUS_IDLE;
    
    char *out_msg = NULL;
    size_t out_len = 0;
    int ret = build_200_ok_response(message, &out_msg, &out_len);
    if (ret != 0 || out_len == NULL || out_len == 0){
        log_info("failed to response to BYE");        
    }else{
        transmit_sip(out_msg);
        free_sip_message(out_msg);
        out_msg = NULL;
    }
    
    clear_session();
    clear_traffic_tunnel();
    struct app_event event = {
        .event = APP_EVENT_CALL_SERVER_TERMINATED,
        .arg = NULL
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);

    os_mutex_post(&mutex);
    free(message);
    return;
}

static void proc_request_info(MOVE received_sip_message_ptr message){
    
    //log_info("Received INFO request\n");
    // 处理 INFO 请求
    if (m_session_state.session_status != SESSION_STATUS_IN_CALL){
        log_info("unexpected INFO request, current state: %d\n", m_session_state.session_status);
        free(message);
        return;
    }

    if (strncmp(message->call_id, m_session_state.session_id, sizeof(m_session_state.session_id)) != 0){
        log_info("Info call_id does not match current session, ignoring\n");
        os_mutex_post(&mutex);
        free(message);
        return;
    }

    if (message->body_length > 0 && message->message_body) {
        // 解析 JSON
        json_object *parse = NULL;
        parse = json_tokener_parse(message->message_body);
        //log_info("INFO body, len: %d: %s\n", out_body_len, out_body);
        if (parse) {
            message_session_event_t session_event = {0};
            char ev[16] = {0};
            char* val = json_get_string(parse, "event");
            if (val){
                strncpy(ev, val, sizeof(ev)-1);
            }
            if (strcmp(ev, DEVICE_CTRL_EVENT_STT) == 0){
                session_event.event = CTRL_EVENT_SHOW_TEXT;
            }else if (strcmp(ev, DEVICE_CTRL_EVENT_SPEAKER) == 0){
                session_event.event = CTRL_EVENT_SPEAKER;
            }else {
                log_info("unknown event type: %s\n", ev);
                json_object_put(parse);
                free(message);
                return;
            }
            char st[33] = {0};
            val = json_get_string(parse, "command");
            if (val){
                strncpy(st, val, sizeof(st)-1);
            }
            if (strcmp(st, WORKING_CMD_START) == 0){
                session_event.status = WORKING_STATUS_START;
            }else if (strcmp(st, WORKING_CMD_STOP) == 0){
                session_event.status = WORKING_STATUS_STOP;
            }else if (strcmp(st, WORKING_CMD_TEXT) == 0 || strcmp(st, WORKING_CMD_SENTENCE_START) == 0){
                session_event.status = WORKING_STATUS_TEXT;
            }else {
                session_event.status = WORKING_STATUS_INVALID;
                //log_info("unknown status type: %s\n", st);
            }
            val = json_get_string(parse, "text");
            if (val){
                strncpy(session_event.text, val, sizeof(session_event.text)-1);
            }
            
            val = json_get_string(parse, "emotion");
            if (val){
                strncpy(session_event.emotion, val, sizeof(session_event.emotion)-1);
            }
            struct app_event event = {
                .event = APP_EVENT_CALL_UPDATED,
                .arg = &session_event
            };
            app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
            json_object_put(parse);

            char *out_msg = NULL;
            size_t out_len = 0;
            int ret = build_200_ok_response(message, &out_msg, &out_len);
            if (ret != 0 || out_msg == NULL || out_len == 0){
                log_info("failed to response to INFO: ret %d, out_len %d", ret, out_len);
                free(message);
                return;
            }
            transmit_sip(out_msg);
            free_sip_message(out_msg);
            out_msg = NULL;
        }else{
            log_info("failed to parse INFO body to JSON\n");
        }
    }

    free(message);
    return;
}


void send_register(){

    int ret = os_mutex_pend(&mutex, 0);
    if (ret != 0){
        log_info("os_mutex_pend fail");
        return;
    }

    do{

        if (m_session_state.session_status > SESSION_STATUS_REGISTERING){
            log_info("send_register client is busy");
            break;
        }

        uint32_t ms = get_system_ms();
        if (m_session_state.last_keepalive_ms > 0 && (m_session_state.last_keepalive_ms + REGISTER_EXPIRE_SECOND * 1000) > ms){
            log_debug("need to wait more time to register");
            break;
        }
                
        sip_register_param_t register_param = {
            .uid = m_session_state.uid,
            .device_ip = m_session_state.device_ip,
            .expires_sec = REGISTER_EXPIRE_SECOND,
            .cseq_num = (int)(m_session_state.seq)
        };              
        char* message = NULL;
        size_t message_len = 0;
        int ret = build_register(&register_param, &message,  &message_len);
        if (ret != 0){
            log_info("build_register fail\n");  
            break;
        }    
        //log_info("register msg:\n%s\n", message);
        m_session_state.session_status = SESSION_STATUS_REGISTERING;
        m_session_state.last_req_message_ms = ms;
        m_session_state.last_req_message_seq = m_session_state.seq;
        transmit_sip(message);

        m_session_state.seq++;
        free_sip_message(message);           
    }while (0);
    
    os_mutex_post(&mutex);
    return;
}

int init_call(char* wake_up_word){
    int ret = os_mutex_pend(&mutex, 0);
    if (ret != 0){
        log_info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ os_mutex_pend fail");
        return -1;
    }
    do{

        if (m_session_state.session_status > SESSION_STATUS_INVITING){
            ret = -1;
            log_info("device is busy");
            break;
        }
        
        uint32_t ms = get_system_ms();
        char sdp_buf[320] = {0};

        extern media_parameter_t g_audio_enc_media_param;
        uplink_sdp_parameter_t sdp_param = {
            .uid = m_session_state.uid,
            .device_ip = m_session_state.device_ip,
            .codec = g_audio_enc_media_param.codec,
            .sample_rate = g_audio_enc_media_param.sample_rate,
            .channels = g_audio_enc_media_param.channels,
            .frame_duration_ms = OPUS_FRAME_DURATION_MS,
            .support_mcp = SESSION_SUPORT_MCP,
            .cbr = SESSION_OPUS_CBR,
            .frame_gap = SESSION_AUDIO_FRAME_GAP,
            .wake_up_word = wake_up_word
        };

        sip_invite_param_t invite = {
            .uid = m_session_state.uid,
            .device_ip = m_session_state.device_ip,
            .cseq_num = (int)(m_session_state.seq),
            .sdp = &sdp_param

        };
        char* message = NULL;
        size_t message_len = 0;           
        int ret = build_invite(&invite, &message,  &message_len);
        if (ret != 0){
            log_info("build invite fail\n");  
            break;
        }    
        log_info("invite msg:\n%s\n", message);
        m_session_state.session_status = SESSION_STATUS_INVITING;
        m_session_state.last_req_message_ms = ms;
        m_session_state.last_req_message_seq = m_session_state.seq;
        m_session_state.seq++;
        transmit_sip(message);
        free_sip_message(message);                     

    }while(0);
    
    os_mutex_post(&mutex);
    return ret;
}


int finish_call(){
    os_mutex_pend(&mutex, 0);
    int ret = 0;
    do{

        if (m_session_state.session_status != SESSION_STATUS_IN_CALL){
            ret = -1;
            log_info("device is not in call");
            break;
        }
        
        char* message = NULL;
        size_t message_len = 0;  

        ret = build_bye(m_session_state.invite_200_ok_resp_message, 
                m_session_state.uid,
                m_session_state.device_ip,
                m_session_state.seq,
                &message,
                &message_len
            );
        if (ret != 0){
            log_info("build bye fail\n");  
        }else{
            log_info("invite msg:\n%s\n", message);
            m_session_state.seq++;
            transmit_sip(message);
            free_sip_message(message);
        }   
        clear_session();
        clear_traffic_tunnel();

    }while(0);
    
    os_mutex_post(&mutex);
    return ret;
}

static void send_invite_ack(){
    if (!m_session_state.invite_200_ok_resp_message){
        return;
    }

    int ret = -1;
    char *out_msg = NULL;
    size_t out_len = 0;

    ret = build_ack(m_session_state.invite_200_ok_resp_message,
                    m_session_state.uid,
                    m_session_state.device_ip,
                    m_session_state.seq++,
                    &out_msg,
                    &out_len);
    if (ret != 0 || !out_msg) {
        log_info("build_ack failed\n");
        goto cleanup;
    }

    transmit_sip(out_msg);

cleanup:
    if (out_msg) free_sip_message(out_msg);
    return;
}

static int send_listening_status(char* cmd, char* mode){

    if(!cmd || (strcmp(cmd, WORKING_CMD_START) != 0 && strcmp(cmd, WORKING_CMD_STOP) != 0)){
        log_info("invalid listening status: %s", cmd ? cmd : "null");
        return -1;
    }
    info_param_t info = {0};
    strncpy(info.event, "listen", sizeof(info.event)-1);
    if (cmd && strlen(cmd) > 0){
        strncpy(info.command, cmd, sizeof(info.command)-1);
    }
    if (mode && strlen(mode) > 0){
        strncpy(info.mode, mode, sizeof(info.mode)-1);
    }

    os_mutex_pend(&mutex, 0);
    int ret = 0;
    do{

        if (m_session_state.session_status != SESSION_STATUS_IN_CALL){
            ret = -1;
            log_info("call status is incorrect: %d", m_session_state.session_status);
            break;
        }
        
        if (!m_session_state.invite_200_ok_resp_message){
            ret = -1;
            log_info("no 200 ok response message");
            break;
        }

        char* message = NULL;
        size_t message_len = 0;           
        int ret = build_listen_info(
            m_session_state.invite_200_ok_resp_message,
            m_session_state.uid, 
            m_session_state.device_ip,
            m_session_state.seq, 
            &info,
            &message, 
            &message_len);
        if (ret != 0){
            log_info("build info fail\n");  
            break;
        }    
        //log_info("info msg:\n%s\n", message);

        uint32_t ms = get_system_ms();
        m_session_state.last_req_message_ms = ms;
        m_session_state.last_req_message_seq = m_session_state.seq;
        m_session_state.seq++;
        transmit_sip(message);
        free_sip_message(message);                     

    }while(0);
    
    os_mutex_post(&mutex);
    return ret;

}

void send_start_listening(){
    log_info("send_start_listening");
    send_listening_status(WORKING_CMD_START, "auto");
    message_session_event_t session_event = {
        .event = CTRL_EVENT_LISTEN, 
        .status = WORKING_STATUS_START
    };
    struct app_event event = {
        .event = APP_EVENT_CALL_UPDATED,
        .arg = &session_event
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void send_stop_listening(){
    send_listening_status(WORKING_CMD_STOP, NULL);
    message_session_event_t session_event = {
        .event = CTRL_EVENT_LISTEN, 
        .status = WORKING_STATUS_STOP
    };

    struct app_event event = {
        .event = APP_EVENT_CALL_UPDATED,
        .arg = &session_event
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}


void send_abort_speaking(abort_reason_t reason){

}



// 使用示例
void handle_received_sip(const char *data, size_t len)
{
    received_sip_message_ptr msg_info = NULL;
    int result = sip_parse_incoming_message(data, len, &msg_info);
    
    if(result == 0){
        //log_info("Successfully parsed SIP message\n");
        
        if (!is_response_message(msg_info)) {
            // 设备只会收到如下几种服务器下发的
            if (strcmp(msg_info->method, "MESSAGE") == 0) {
                // 服务器下发事件
                proc_request_message(msg_info);

            } else if (strcmp(msg_info->method, "BYE") == 0) {
                // 服务器释放会话
                proc_request_bye(msg_info);
            } else if (strcmp(msg_info->method, "INFO") == 0) {
                // 会话中服务器下发更新消息
                proc_request_info(msg_info);
            }else{
                log_info("Received unsupported request method: %s\n", msg_info->method);
                free(msg_info);
            }
        } else{
            // 处理响应消息
            log_info("Received response: %d\n", msg_info->status_code);
            
            if (strcmp(msg_info->method, "REGISTER") == 0) {
                proc_response_register(msg_info);
            } else if (strcmp(msg_info->method, "INVITE") == 0) {
                proc_response_invite(msg_info);
            } else if (strcmp(msg_info->method, "INFO") == 0) {
                proc_response_info(msg_info);
            }  else if (strcmp(msg_info->method, "BYE") == 0) {
                proc_response_bye(msg_info);
            } else {
                log_info("Received response for unsupported method: %s\n", msg_info->method);
                free(msg_info);
            }
        }

    }else{
        log_info("Unsupported SIP message: %d\n", result);
    }
}



void session_checking(){
    if (!m_session_state.protocol_inited){
        return;
    }

    os_mutex_pend(&mutex, 0);
    uint32_t ms = get_system_ms();

    // 检查注册状态
    if (m_session_state.session_status == SESSION_STATUS_REGISTERING){
        if ((m_session_state.last_req_message_ms + COMMAND_TIMEOUT_MS) <= ms){
            log_info("register timeout\n");
            m_session_state.session_status = SESSION_STATUS_IDLE;
            m_session_state.last_req_message_ms = 0;
            m_session_state.last_req_message_seq = 0;
        }
    }else 
    if (m_session_state.session_status == SESSION_STATUS_INVITING){
        if ((m_session_state.last_req_message_ms + COMMAND_TIMEOUT_MS) <= ms){
            log_info("invite timeout\n");
            m_session_state.session_status = SESSION_STATUS_IDLE;
            m_session_state.last_req_message_ms = 0;
            m_session_state.last_req_message_seq = 0;
            struct app_event event = {
                .event = APP_EVENT_CALL_NO_ANSWER,
                .arg = NULL
            };
            app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
        }
    }else 
    if (m_session_state.session_status == SESSION_STATUS_TERMINATING){
       
        if ((m_session_state.last_req_message_ms + COMMAND_TIMEOUT_MS) <= ms){
            log_info("wait response timeout\n");
            m_session_state.session_status = SESSION_STATUS_IDLE;
            m_session_state.last_req_message_ms = 0;
            m_session_state.last_req_message_seq = 0;
        }
    }

    os_mutex_post(&mutex);

    return;
}

void init_session_module(char* uid, char* device_ip){
    os_mutex_create(&mutex);
    m_session_state.protocol_inited = 1;
    size_t len = sizeof(m_session_state.uid);
    strncpy(m_session_state.uid, uid, len);
    m_session_state.uid[len - 1];

    strncpy(m_session_state.device_ip, device_ip, sizeof(m_session_state.device_ip)-1);
    log_info("got ip addr : %s\n", m_session_state.device_ip);

    m_session_state.seq = get_system_ms()%1000;
    log_info("protocol seq : %u\n", m_session_state.seq);

    init_sip();

}