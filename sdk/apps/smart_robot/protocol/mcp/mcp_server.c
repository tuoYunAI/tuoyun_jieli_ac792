#include "app_config.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/includes.h"
#include "app_event.h"
#include "app_mcp_server.h"
#include "app_audio.h"

#include "osipparser2/osip_list.h"
#include "json_c/json_object.h"
#include "json_c/json_tokener.h"


#define LOG_TAG             "[MCP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"



typedef struct {
    char name[MCP_NAME_LEN];
    char* description;
    mcp_call_fn mcp_call_fn;
    int property_count;
    property_t properties[0]
}mcp_tool_t, *mcp_tool_ptr;





static osip_list_t m_mcp_tools_list;
static int m_mcp_tools_count = -1;
static OS_MUTEX mutex;

void add_mcp_tool(char* name, 
     char* description, 
     mcp_call_fn callback, 
     property_ptr props, 
     size_t len){
    if (!name || !callback){
        log_info("invalid mcp tool info\n");
        return;
    }
    if (len > 0 && !props){
        log_info("prop is null");
        return;
    }

    log_info("register mcp tool: %s, %s\n", name, description ? description : "");

    os_mutex_pend(&mutex, 0);
    
    /* Allocate mcp_tool_t + space for properties */
    size_t alloc_size = sizeof(mcp_tool_t) + (len ? (len * sizeof(property_t)) : 0);
    mcp_tool_ptr tool = malloc(alloc_size);
    if (!tool){
        log_info("failed to alloc mem for mcp tool\n");
        os_mutex_post(&mutex);
        return;
    }

    /* Initialize fixed fields */
    memset(tool, 0, alloc_size);
    strncpy(tool->name, name, sizeof(tool->name) - 1);
    tool->name[sizeof(tool->name) - 1] = '\0';

    if (description) {
        char *description_copy = malloc(strlen(description) + 1);
        if (!description_copy) {
            log_info("failed to alloc mem for mcp tool description\n");
            free(tool);
            os_mutex_post(&mutex);
            return;
        }
        strcpy(description_copy, description);
        tool->description = description_copy;
    } else {
        tool->description = NULL;
    }

    tool->mcp_call_fn = callback;
    tool->property_count = (int)len;

    /* 如果提供了 properties，复制到紧随结构体之后的 flexible array */
    if (len > 0 && props) {
        memcpy(tool->properties, props, len * sizeof(property_t));
    }

    if (osip_list_add(&m_mcp_tools_list, tool, -1) < 0){
        log_info("failed to add mcp tool to list\n");
        if (tool->description) free(tool->description);
        free(tool);
        os_mutex_post(&mutex);
        return;
    }

    m_mcp_tools_count++;

    os_mutex_post(&mutex);
    log_info("mcp tool registered successfully, total cnt: %d\n", m_mcp_tools_count);
    return;
}




static int transmit_mcp(char*id, char *message){
    if (!id || !message){
        return -1;
    }
    int len = 200 + strlen(message);
    char* buf = malloc(len);
    if (!buf){
        log_info("failed to alloc mem for mcp response\n");
        return -1;
    }
    memset(buf, 0, len);
    snprintf(buf, len -1, "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":%s}", id, message);
    
    json_object *root = json_object_new_object();
    json_object *protocol = json_object_new_string("MCP");
    json_object *payload = json_object_new_string(buf);
    
    json_object_object_add(root, "protocol", protocol);
    json_object_object_add(root, "payload", payload);
    const char* json_string = json_object_to_json_string(root);
    transmit_mqtt_message((char*)json_string);
    json_object_put(root);
    free(buf);
    return 0;
}




static void handle_initialize(json_object *request){

    log_info("handle_initialize\n");
    char id[32] = {0};
    const char* val = json_get_string(request, "id");
    if (val){
        strncpy(id, val, sizeof(id)-1);
    }else{
        log_info("no id in MCP message\n");
        return;     
    }

    json_object *experimental = NULL;
    experimental = json_object_object_get(request, "experimental");
    if (experimental){
        char* url = json_get_string(experimental, "url");
        char* token = json_get_string(experimental, "token");
        log_info("url: %s, token: %s\n", url ? url : "null", token ? token : "null");   
    }

    char *resp =  "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\""FIRMWARE_VERSION"\"}}";
    transmit_mcp(id, resp);
    
    return;    
}




static void handle_tools_list(json_object *request){

    log_info("handle_tools_list\n");

    char id[32] = {0};
    const char* val = json_get_string(request, "id");
    if (val){
        strncpy(id, val, sizeof(id)-1);
    }else{
        log_info("no id in MCP message\n");
        return; 
    }

    // 读取分页参数
    int start = 0;
    int limit = 20; // 默认每页 20
    json_object* params = json_object_object_get(request, "params");
    if (params) {
        const char* cursor = json_get_string(params, "cursor"); // 期望为数字字符串
        if (cursor && *cursor) {
            char *endp = NULL;
            unsigned long v = strtoul(cursor, &endp, 10);
            if (endp != cursor && *endp == '\0') start = (int)v;
        }
        json_object* limit_obj = json_object_object_get(params, "limit");
        if (limit_obj && json_object_is_type(limit_obj, json_type_int)) {
            int l = json_object_get_int(limit_obj);
            if (l > 0 && l <= 100) limit = l; // 简单上限
        }
    }
    if (start < 0) start = 0;
    log_info("tools_list: start=%d, limit=%d, total: %d", start, limit, m_mcp_tools_count);

    // 组装结果
    json_object *result = json_object_new_object();
    json_object *tools = json_object_new_array();

    int added = 0;
    for (int i = start; i < m_mcp_tools_count && added < limit; i++) {
        mcp_tool_ptr tool = (mcp_tool_ptr)osip_list_get(&m_mcp_tools_list, i);
        if (!tool) continue;


        json_object* tool_obj = json_object_new_object();
        json_object_object_add(tool_obj, "name", json_object_new_string(tool->name ? tool->name : ""));
        json_object_object_add(tool_obj, "description", json_object_new_string(tool->description ? tool->description : ""));
        json_object* input_schema = json_object_new_object();
        json_object_object_add(tool_obj, "inputSchema", input_schema);
        
        json_object_object_add(input_schema, "type", json_object_new_string("object"));
        json_object *required = json_object_new_array();
        json_object *properties = json_object_new_object();
        json_object_object_add(input_schema, "properties", properties);

        for (int j = 0; j < tool->property_count; j++) {
            property_ptr prop = &tool->properties[j];
            if (!prop) continue;

            json_object* prop_obj = json_object_new_object();
            switch (prop->type) {
                case kPropertyTypeBoolean:
                    json_object_object_add(prop_obj, "type", json_object_new_string("boolean"));
                    break;
                case kPropertyTypeInteger:
                    json_object_object_add(prop_obj, "type", json_object_new_string("integer"));
                    json_object_object_add(prop_obj, "minimum", json_object_new_int(prop->min_int_value));
                    json_object_object_add(prop_obj, "maximum", json_object_new_int(prop->max_int_value));
                    break;
                case kPropertyTypeString:
                    json_object_object_add(prop_obj, "type", json_object_new_string("string"));
                    break;
                default:
                    break;
            }
            if (prop->description) {
                json_object_object_add(prop_obj, "description", json_object_new_string(prop->description));
            }
            json_object_object_add(properties, prop->name, prop_obj);
            if (prop->required) {
                json_object_array_add(required, json_object_new_string(prop->name));
            }
        }

        /* 如果 required 数组中有元素，再把它加入 inputSchema；否则释放该空数组 */
        if (required && json_object_array_length(required) > 0) {
            json_object_object_add(input_schema, "required", required);
        } else {
            if (required) json_object_put(required);
        }
        
        json_object_array_add(tools, tool_obj);
        added++;
    }

    // 计算 nextCursor
    if (start + added < m_mcp_tools_count) {
        char next_buf[16];
        snprintf(next_buf, sizeof(next_buf), "%d", start + added);
        json_object_object_add(result, "nextCursor", json_object_new_string(next_buf));
    } 

    json_object_object_add(result, "tools", tools);

    // 发送响应
    const char* result_str = json_object_to_json_string(result);
    log_info("handle_tools_list: start=%d, limit=%d, returned=%d, nextCursor=%s\n",
             start, limit, added,
             json_object_get_type(json_object_object_get(result, "nextCursor")) == json_type_null ? "null" :
             json_object_get_string(json_object_object_get(result, "nextCursor")));

    transmit_mcp(id, (char*)result_str);

    json_object_put(result);
    return;

}


json_object* make_error_result(u32 code, char* msg){
    json_object *result = json_object_new_object();
    json_object *is_error_obj = json_object_new_boolean(true);
    json_object_object_add(result, "isError", is_error_obj);
    json_object *content = json_object_new_object();
    json_object *type = json_object_new_string("text");
    json_object *message = json_object_new_string(msg ? msg : "unknown error");
    json_object *code_obj = json_object_new_int((int)code);
    json_object_object_add(content, "type", type);
    json_object_object_add(content, "message", message);
    json_object_object_add(content, "code", code_obj);
    json_object *contents = json_object_new_array();
    json_object_array_add(contents, content);
    json_object_object_add(result, "content", contents);
    return result;
}


static void do_tool_call(json_object *request){

    char id[32] = {0};
    const char* val = json_get_string(request, "id");
    if (val){
        strncpy(id, val, sizeof(id)-1);
    }else{
        log_info("no id in MCP message\n");
        return; 
    }

    json_object* params = json_object_object_get(request, "params");
    if(params == NULL){
        log_info("no params in tool call\n");
        return;   
    }
    char* tool_name = json_get_string(params, "name");
    if (!tool_name){
        log_info("no tool name in tool call\n");
        return;   
    }
    log_info("do_tool_call: %s\n", tool_name);

    // 查找工具
    mcp_tool_ptr tool = NULL;
    for (int i = 0; i < m_mcp_tools_count; i++) {
        mcp_tool_ptr t = (mcp_tool_ptr)osip_list_get(&m_mcp_tools_list, i);
        if (t && t->name && strcmp(t->name, tool_name) == 0) {
            tool = t;
            break;
        }
    }

    if (!tool) {
        log_info("tool not found: %s\n", tool_name);
        json_object* ret = make_error_result(-32601, "Tool not found");
        const char* json_string = json_object_to_json_string(ret);
        transmit_mcp(id, json_string);  
        json_object_put(ret);
        return;
    }

    json_object *args = json_object_object_get(params, "arguments");
    int arg_count = 0;
    property_t *props = NULL;
    int parsed_count = 0;

    if (args && json_object_is_type(args, json_type_object)) {
        arg_count = tool->property_count;
        if (arg_count > 0) {
            props = malloc(arg_count * sizeof(property_t));
            if (!props) {
                log_info("failed to alloc props for arguments\n");
                json_object* ret = make_error_result(-32000, "internal error");
                const char* json_string = json_object_to_json_string(ret);
                transmit_mcp(id, json_string);  
                json_object_put(ret);
                return;
            }
            memset(props, 0, arg_count * sizeof(property_t));
            
            for (int i = 0; i < tool->property_count; i++) {
                property_ptr pdef = &tool->properties[i];
                json_object *valobj = json_object_object_get(args, pdef->name);
                if (!valobj) continue;

                property_t *p = &props[i];
                strncpy(p->name, pdef->name, MCP_NAME_LEN - 1);
                p->name[MCP_NAME_LEN - 1] = '\0';
                p->type = pdef->type;
                parsed_count++;
                if (p->type == kPropertyTypeInteger) {
                    if (valobj && json_object_is_type(valobj, json_type_int)) {
                        p->value.int_value = json_object_get_int(valobj);
                    } 
                } else if (p->type == kPropertyTypeBoolean) {
                    if (valobj && json_object_is_type(valobj, json_type_boolean)) {
                        p->value.bool_value = json_object_get_boolean(valobj) ? 1 : 0;
                    } 
                } else {

                    const char *valstr = json_get_string(args, pdef->name);
                    if (valstr) {
                        strncpy(p->value.str_value, valstr, sizeof(p->value.str_value) - 1);
                        p->value.str_value[sizeof(p->value.str_value) - 1] = '\0';
                    }
                }
            }
        }
    }else{
        log_info("no arguments in tool call\n");
    }

    // 调用工具的回调
    json_object *out = NULL;
    if (tool->mcp_call_fn) {
        out = tool->mcp_call_fn(props, (size_t)parsed_count);
    }
    
    if (!out) {
        log_info("tool call failed: %s\n", tool->name);
        json_object* ret = make_error_result(-32000, "internal error");
        const char* json_string = json_object_to_json_string(ret);
        transmit_mcp(id, json_string);  
        json_object_put(ret);
    } else {
        
        json_object *result = json_object_new_object();
        json_object *is_error = json_object_new_boolean(false);
        
        json_object *contents = json_object_new_array();
        json_object_array_add(contents, out);
        json_object_object_add(result, "content", contents);

        json_object_object_add(result, "isError", is_error);
        const char* json_string = json_object_to_json_string(result);
        transmit_mcp(id, json_string);  
        log_info("tool call succeeded: %s\n", json_string);
        json_object_put(result);
        out = NULL;        
    }
    
    // 释放临时为 arguments 分配的描述字段与数组
    if (props) {
        for (int i = 0; i < arg_count; i++) {
            if (props[i].description) free(props[i].description);
        }
        free(props);
    }

    return;
}





void handle_received_mcp_request(const char *data, size_t len){
    /*
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    */

    if (!data || len == 0){
        log_info("invalid request data");
        return;
    }
    log_info("Received MCP message (%d bytes):\n", len);
    log_info("%.512s%s\n", data, len > 512 ? "..." : "");
    json_object *parse = NULL;
    parse = json_tokener_parse(data);
    if (!parse){
        log_info("failed to parse MCP message to JSON\n");
        return;     
    }
    char *version = (char*)json_get_string(parse, "jsonrpc");
    if (!version){
        log_info("no version in MCP message\n");
        json_object_put(parse);
        return;
    }  
    if (strcmp(version, "2.0") != 0){
        log_info("unsupported MCP version: %s\n", version);
        json_object_put(parse);
        return;     
    }   
    
    char *method = (char*)json_get_string(parse, "method");
    if (!method){
        log_info("no method in MCP message\n");
        json_object_put(parse);
        return;
    }   
    log_info("MCP method: %s\n", method);
    
    if (strcmp(method, "initialize") == 0){
        handle_initialize(parse);
        
    }else if (strcmp(method, "tools/list") == 0){
        handle_tools_list(parse);
    }else if (strcmp(method, "tools/call") == 0)
    {
        do_tool_call(parse);
    }else if(strcmp(method, "notifications/initialized") == 0){
        log_info("MCP client initialized\n");
    } else{
        log_info("unknown MCP method: %s\n", method);
    }

    json_object_put(parse);
    return;     
            
}


json_object* mcp_call_get_device_status(property_ptr props, size_t len){
/*
     * 返回设备状态JSON
     * 
     * 返回的JSON结构如下：
     * {
     *     "audio_speaker": {
     *         "volume": 70
     *     },
     *     "screen": {
     *         "brightness": 100,
     *         "theme": "light"
     *     },
     *     "battery": {
     *         "level": 50,
     *         "charging": true
     *     },
     *     "network": {
     *         "type": "wifi",
     *         "ssid": "Xiaozhi",
     *         "rssi": -60
     *     },
     *     "chip": {
     *         "temperature": 25
     *     }
     * }
     */

    /* 获取音量和扬声器状态 */
    int volume = tuoyun_audio_player_get_volume();

    char device_status[64] = {0};
    snprintf(device_status, sizeof(device_status - 1), "current volume: %d", volume);
    json_object *result = json_object_new_object();
    json_object *type = json_object_new_string("text");
    log_info("mcp_call_get_device_status: %s\n", device_status);
    json_object *text = json_object_new_string(device_status);
    
    json_object_object_add(result, "type", type);
    json_object_object_add(result, "text", text);
    return result;
}

void mcp_init(void *priv){
    if (os_mutex_create(&mutex) != OS_NO_ERR) {
        log_error("os_mutex_create mutex fail\n");
        return;
    }

    osip_list_init(&m_mcp_tools_list);
    m_mcp_tools_count = 0;

    add_mcp_tool("get_device_status",
        "Provides the real-time information of the device, including the current status of the audio speaker, screen, battery, network, etc.\n"
        "Use this tool for: \n"
        "1. Answering questions about current condition (e.g. what is the current volume of the audio speaker?)\n"
        "2. As the first step to control the device (e.g. turn up / down the volume of the audio speaker, etc.)",
        mcp_call_get_device_status,
        NULL,
        0);
}

__initcall(mcp_init);