#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/includes.h"
#include "mqtt/MQTTClient.h"
#include "app_event.h"


#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "traffic/traffic.h"
#include "app_mcp_server.h"
#include "app_audio.h"

#define ADAPTER_LOG_TAG    "[SIP-ADAPTER]"
#define LOG_LEVEL_ENABLED  LOG_INFO_LEVEL
#include "adapter.h"

extern void dbg_print(const char *format, ...);

void adatper_log(sip_log_level_t level, const char* tag, const char* format, ...){
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    switch (level) {
        case LOG_DEBUG_LEVEL:
            dbg_print("[Verb]: %s%s", tag, buffer); 
            printf("[Verb]: %s%s", tag, buffer); 
            break;
        case LOG_INFO_LEVEL:
            dbg_print("[Info]: %s%s", tag, buffer); 
            printf("[Info]: %s%s", tag, buffer); 
            break;
        case LOG_WARN_LEVEL:
            dbg_print("[Warn]: %s%s", tag, buffer); 
            printf("<Warn>: %s%s", tag, buffer); 
            break;
        case LOG_ERROR_LEVEL:
            dbg_print("[Error]: %s%s", tag, buffer); 
            printf("<Error>: %s%s", tag, buffer); 
            break;
        default:
            dbg_print("[Info]: %s%s", tag, buffer); 
            printf("[Info]: %s%s", tag, buffer); 
            break;
    }
}


void log_error(const char* format, ...){

}   


uint32_t adapter_get_system_ms(void){
    extern uint32_t get_system_ms(void);    
    return get_system_ms();
}



sip_ret_t adapter_start_thread(void (*task_func)(void*), const char* name, int stack_size, int priority){
    int ret = thread_fork(name, priority, stack_size, 0, NULL, task_func, NULL);
    if (ret != OS_NO_ERR) {
        LOG_INFO("thread fork fail: %d\r\n", ret);
        return RET_ERROR;
    }
    LOG_INFO("Start task success: %s", name);
    return RET_OK;
}

sip_ret_t adapter_start_periodic_task(void (*task_func)(void *), int period_ms, int stack_size, void* arg){
    static int index = 0;
    char task_name[32];
    snprintf(task_name, sizeof(task_name), "sip_periodic_task_%d", index++);

    u16 handle = sys_timer_add_to_task(task_name, NULL, task_func, period_ms);
    if (handle == 0) {
        LOG_INFO("Failed to create periodic task: %s", task_name);
        return RET_ERROR;
    }
    LOG_INFO("Start timer task sucess: %s", task_name);
    return RET_OK;
}

void adapter_task_delay(int delay_ms){
    os_time_dly(delay_ms / 10);
}

void* adapter_create_json_object(){
    json_object *root = json_object_new_object();
    return root;
}

void* adapter_create_json_array(){
    json_object *array = json_object_new_array();
    return array;
}

void* adapter_json_object_new_int(int value){
    json_object *int_obj = json_object_new_int(value);
    return int_obj;
}

void* adapter_json_object_new_string(const char* value){
    json_object *str_obj = json_object_new_string(value);
    return str_obj;
}

void* adapter_json_object_new_boolean(bool value){
    json_object *bool_obj = json_object_new_boolean(value ? 1 : 0);
    return bool_obj;
}

void adapter_array_add_json_object(void* array, void* obj){
    if (!array || !obj) return;
    json_object_array_add((json_object *)array, (json_object *)obj);
}

void adapter_put_json_object_value(void* root, char* key, void* obj){
    if (!root || !obj) return;
    json_object_object_add((json_object *)root, key, (json_object *)obj);
}   

void* adapter_put_json_string_value(void* obj, const char* key, const char* value){
    if (!obj || !key || !value) return NULL;
    json_object *payload = json_object_new_string(value); 
    json_object_object_add(obj, key, payload);
    return obj;
}

char* adapter_serialize_json_to_string(void* obj){
    if (!obj) return NULL;
    char* json_string = json_object_to_json_string((json_object *)obj);
    return json_string;
}


void* adapter_parse_json_string(const char* json_str){
    if (!json_str) return NULL;
    json_object *parse = json_tokener_parse(json_str);
    return parse;
}

int adapter_get_json_int_value(void* obj, const char* key, int default_value){
    if (!obj || !key) return 0;
    json_object* valobj = json_object_object_get((json_object *)obj, key);
    if (valobj && json_object_is_type(valobj, json_type_int)) {
        return json_object_get_int(valobj);
    }
    return default_value;
}

char* adapter_get_json_string_value(void* obj, const char* key){
    if (!obj || !key) return NULL;
    return json_get_string(obj, key);
}

bool adapter_get_json_boolean_value(void* obj, const char* key, bool default_value){
    if (!obj || !key) return default_value;
    json_object* valobj = json_object_object_get((json_object *)obj, key);
    if (valobj && json_object_is_type(valobj, json_type_boolean)) {
        return json_object_get_boolean(valobj) ? true : false;
    }
    return default_value;
}

void* adapter_get_json_node_value(void* obj, const char* key){
    if (!obj || !key) return NULL;
    return json_object_object_get((json_object *)obj, key);
}

void adapter_delete_json_object(void* obj){
    if (obj) {
        json_object_put((json_object *)obj);
    }
}

static OS_MUTEX session_mutex;

void adapter_lock_sip_mutex(){
    os_mutex_pend(&session_mutex, 0);
}

void adapter_unlock_sip_mutex(){
    os_mutex_post(&session_mutex);
}


static OS_MUTEX sip_list_mutex;
void adapter_lock_sip_list_mutex(){
    os_mutex_pend(&sip_list_mutex, 0);
}

void adapter_unlock_sip_list_mutex(){
    os_mutex_post(&sip_list_mutex);
}

static OS_MUTEX mcp_tool_list_mutex;
void adapter_lock_mcp_mutex(){
    os_mutex_pend(&mcp_tool_list_mutex, 0);
}

void adapter_unlock_mcp_mutex(){
    os_mutex_post(&mcp_tool_list_mutex);
}

int adapter_start_traffic_tunnel(media_parameter_ptr media_param){

    return start_traffic_tunnel(media_param);
}

void adapter_clear_traffic_tunnel(){
    clear_traffic_tunnel();
}


void adapter_transmit_mqtt_message(char* message){
    extern void transmit_mqtt_message(char* message);
    transmit_mqtt_message(message);
}

void on_call_established(char* session_id, media_parameter_ptr media_param){
    struct app_event event = {
        .event = APP_EVENT_CALL_ESTABLISHED,
        .arg = media_param
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}


void on_call_ack_error(session_call_error_t error_code){
    session_call_error_event_ptr notify = malloc(sizeof(session_call_error_event_t));
    if (!notify) {
        return;
    }
    memset(notify, 0, sizeof(session_call_error_event_t));
    notify->event = error_code;
    struct app_event event = {
        .event = APP_EVENT_CALL_REJECTED,
        .arg = notify
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_call_terminated_by_server(){
    struct app_event event = {
        .event = APP_EVENT_CALL_SERVER_TERMINATED,
        .arg = NULL
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_call_terminated_ack(){
    struct app_event event = {
        .event = APP_EVENT_CALL_TERMINATE_ACK,
        .arg = NULL
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event); 
}

void on_server_notify(MOVE event_system_notification_ptr notify){
    app_event_t event = {
        .event = APP_EVENT_SERVER_NOTIFY,
        .arg = notify
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_server_lifecycle_event(MOVE event_device_lifecycle_ptr lifecycle){
    app_event_t event = {
        .event = APP_EVENT_SERVER_LIFECYCLE_EVENT,
        .arg = lifecycle
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_set_device_mode(MOVE control_device_mode_set_ptr params){
    app_event_t event = {
        .event = APP_EVENT_SET_DEVICE_MODE,
        .arg = params
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_execute_motion(MOVE control_device_motion_execute_ptr params){
    app_event_t event = {
        .event = APP_EVENT_EXECUTE_MOTION,
        .arg = params
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_stop_motion(MOVE control_device_motion_stop_ptr params){
    app_event_t event = {
        .event = APP_EVENT_STOP_MOTION,
        .arg = params
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}   

void on_server_session_input_text_notify(MOVE data_audio_input_text_ptr audio_input_text){
    app_event_t event = {
        .event = APP_EVENT_CALL_INPUT_TEXT_NOTIFY,
        .arg = audio_input_text
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_server_session_output_text_notify(MOVE data_audio_output_text_ptr audio_output_text){
    app_event_t event = {
        .event = APP_EVENT_CALL_OUTPUT_TEXT_NOTIFY,
        .arg = audio_output_text
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_server_session_update_notify(MOVE control_audio_output_state_ptr session_event){
    struct app_event event = {
        .event = APP_EVENT_CALL_UPDATED,
        .arg = session_event
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_server_mcp_call(const char* message){
    /*
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    */
    handle_received_mcp_request(message, strlen(message));
}


int adapter_get_audio_volume(){
    return tuoyun_audio_player_get_volume();
}

void init_adapter(){
    os_mutex_create(&session_mutex);
    os_mutex_create(&sip_list_mutex);
    os_mutex_create(&mcp_tool_list_mutex);
    mcp_init(NULL);
    LOG_INFO("adapter initialized");
}

late_initcall(init_adapter);