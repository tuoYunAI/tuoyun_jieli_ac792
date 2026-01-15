#include "adapter.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/includes.h"
#include "mqtt/MQTTClient.h"
#include "system/includes.h"
#include "app_event.h"


#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "traffic/traffic.h"
#include "app_mcp_server.h"
#include "app_audio.h"



#define TAG             "[SIP-ADAPTER]"
#define LOG_TAG            TAG
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

uint32_t adapter_get_system_ms(void){
    extern uint32_t get_system_ms(void);    
    return get_system_ms();
}



bool adapter_start_thread(void (*task_func)(void*), const char* name, int stack_size, int priority){

    if (thread_fork(name, priority, stack_size, 0, NULL, task_func, NULL) != OS_NO_ERR) {
        log_info("thread fork fail\n");
        return false;
    }
        
    return true;
}

bool adapter_start_periodic_task(void (*task_func)(void *), int period_ms, int stack_size, void* arg){
    static int index = 0;
    char task_name[32];
    snprintf(task_name, sizeof(task_name), "sip_periodic_task_%d", index++);
    sys_timer_add_to_task(task_name, NULL, task_func, period_ms);

    return true;
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
        .arg = &media_param
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}


void on_call_ack_error(session_call_error_t error_code){
    session_call_error_event_t notify = {       
        .event = error_code
    };
    struct app_event event = {
        .event = APP_EVENT_CALL_REJECTED,
        .arg = &notify
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

void on_server_message_notify(server_message_notify_ptr notify){
    app_event_t event = {
        .event = APP_EVENT_SERVER_NOTIFY,
        .arg = notify
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_server_session_update_notify(message_session_event_ptr session_event){
    struct app_event event = {
        .event = APP_EVENT_CALL_UPDATED,
        .arg = session_event
    };
    app_event_notify(APP_EVENT_FROM_PROTOCOL, &event);
}

void on_server_mcp_call(const char* message){
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
}

late_initcall(init_adapter);