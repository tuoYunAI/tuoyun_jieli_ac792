#include "app_config.h"
#include "app_mcp_server.h"

#include "osipparser2/osip_list.h"


#define LOG_TAG             "[MCP]"


typedef struct {
    char name[MCP_NAME_LEN];
    char* description;
    mcp_call_fn mcp_call_fn;
    int property_count;
    property_t properties[0]
}mcp_tool_t, *mcp_tool_ptr;





static osip_list_t m_mcp_tools_list;
static int m_mcp_tools_count = -1;

void add_mcp_tool(char* name, 
     char* description, 
     mcp_call_fn callback, 
     property_ptr props, 
     size_t len){
    if (!name || !callback){
        return;
    }
    if (len > 0 && !props){
        return;
    }

    printf("register mcp tool: %s, %s\n", name, description ? description : "");

    adapter_lock_mcp_mutex();
    
    /* Allocate mcp_tool_t + space for properties */
    size_t alloc_size = sizeof(mcp_tool_t) + (len ? (len * sizeof(property_t)) : 0);
    mcp_tool_ptr tool = malloc(alloc_size);
    if (!tool){
        adapter_unlock_mcp_mutex();
        return;
    }

    /* Initialize fixed fields */
    memset(tool, 0, alloc_size);
    strncpy(tool->name, name, sizeof(tool->name) - 1);
    tool->name[sizeof(tool->name) - 1] = '\0';

    if (description) {
        char *description_copy = malloc(strlen(description) + 1);
        if (!description_copy) {
            free(tool);
            adapter_unlock_mcp_mutex();
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
        if (tool->description) free(tool->description);
        free(tool);
        adapter_unlock_mcp_mutex();
        return;
    }

    m_mcp_tools_count++;
    adapter_unlock_mcp_mutex();
    return;
}


static int transmit_mcp(char*id, char *message){
    if (!id || !message){
        return -1;
    }
    int len = 200 + strlen(message);
    char* buf = malloc(len);
    if (!buf){
        return -1;
    }
    memset(buf, 0, len);
    snprintf(buf, len -1, "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":%s}", id, message);
    
    void *root = adapter_create_json_object();
    adapter_put_json_string_value(root, "MCP", id);    
    adapter_put_json_string_value(root, "protocol", "MCP");
    adapter_put_json_string_value(root, "payload", buf);
    const char* json_string = adapter_serialize_json_to_string(root);

    extern void transmit_mqtt_message(char* message);
    transmit_mqtt_message((char*)json_string);
    adapter_delete_json_object(root);
    free(buf);
    return 0;
}




static void handle_initialize(void *request){
    char id[32] = {0};
    const char* val = adapter_get_json_string_value(request, "id");
    if (val){
        strncpy(id, val, sizeof(id)-1);
    }else{
        return;     
    }

    void *experimental = adapter_get_json_node_value(request, "experimental");
    if (experimental){
        char* url = adapter_get_json_string_value(experimental, "url");
        char* token = adapter_get_json_string_value(experimental, "token");   
    }

    char *resp =  "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" BOARD_NAME "\",\"version\":\""FIRMWARE_VERSION"\"}}";
    transmit_mcp(id, resp);
    
    return;    
}


static void handle_tools_list(void *request){

    char id[32] = {0};
    const char* val = adapter_get_json_string_value(request, "id");
    if (val){
        strncpy(id, val, sizeof(id)-1);
    }else{
        return; 
    }

    // 读取分页参数
    int start = 0;
    int limit = 20; // 默认每页 20
    void* params = adapter_get_json_node_value(request, "params");
    if (params) {
        const char* cursor = adapter_get_json_string_value(params, "cursor"); // 期望为数字字符串
        if (cursor && *cursor) {
            char *endp = NULL;
            unsigned long v = strtoul(cursor, &endp, 10);
            if (endp != cursor && *endp == '\0') start = (int)v;
        }
        int l = adapter_get_json_int_value(params, "limit", limit);
        if (l > 0 && l <= 100) limit = l; // 简单上限
    }
    if (start < 0) start = 0;

    // 组装结果
    void *result = adapter_create_json_object();
    void *tools = adapter_create_json_array();

    int added = 0;
    for (int i = start; i < m_mcp_tools_count && added < limit; i++) {
        mcp_tool_ptr tool = (mcp_tool_ptr)osip_list_get(&m_mcp_tools_list, i);
        if (!tool) continue;


        void* tool_obj = adapter_create_json_object();
        adapter_put_json_string_value(tool_obj, "name", tool->name ? tool->name : "");
        adapter_put_json_string_value(tool_obj, "description", tool->description ? tool->description : "");
        void* input_schema = adapter_create_json_object();
        adapter_put_json_object_value(tool_obj, "inputSchema", input_schema);
        
        adapter_put_json_string_value(input_schema, "type", "object");
        void *required = adapter_create_json_array();
        void *properties = adapter_create_json_object();
        adapter_put_json_object_value(input_schema, "properties", properties);

        for (int j = 0; j < tool->property_count; j++) {
            property_ptr prop = &tool->properties[j];
            if (!prop) continue;

            void* prop_obj = adapter_create_json_object();
            switch (prop->type) {
                case PROPERTY_TYPE_BOOLEAN:
                    adapter_put_json_string_value(prop_obj, "type", "boolean");
                    break;
                case PROPERTY_TYPE_INTEGER:
                    adapter_put_json_string_value(prop_obj, "type", "integer");
                    adapter_put_json_object_value(prop_obj, "minimum", adapter_json_object_new_int(prop->min_int_value));
                    adapter_put_json_object_value(prop_obj, "maximum", adapter_json_object_new_int(prop->max_int_value));
                    break;
                case PROPERTY_TYPE_STRING:
                    adapter_put_json_string_value(prop_obj, "type", "string");
                    break;
                default:
                    break;
            }
            if (prop->description) {
                adapter_put_json_string_value(prop_obj, "description", prop->description);
            }
            adapter_put_json_object_value(properties, prop->name, prop_obj);
            if (prop->required) {
                adapter_array_add_json_object(required, adapter_json_object_new_string(prop->name));
            }
        }

        /* 如果 required 数组中有元素，再把它加入 inputSchema；否则释放该空数组 */
        if (required && json_object_array_length(required) > 0) {
            adapter_put_json_object_value(input_schema, "required", required);
        } else {
            if (required) adapter_delete_json_object(required);
        }
        
        adapter_array_add_json_object(tools, tool_obj);
        added++;
    }

    // 计算 nextCursor
    if (start + added < m_mcp_tools_count) {
        char next_buf[16];
        snprintf(next_buf, sizeof(next_buf), "%d", start + added);
        json_object_object_add(result, "nextCursor", adapter_json_object_new_string(next_buf));
    } 

    json_object_object_add(result, "tools", tools);

    // 发送响应
    const char* result_str = adapter_serialize_json_to_string(result);

    transmit_mcp(id, (char*)result_str);
    adapter_delete_json_object(result);
    return;

}


void* make_error_result(u32 code, char* msg){
    void *result = adapter_create_json_object();
    void *is_error_obj = adapter_json_object_new_boolean(true);
    adapter_put_json_object_value(result, "isError", is_error_obj);

    void *content = adapter_create_json_object();
    adapter_put_json_string_value(content, "type", "text");
    adapter_put_json_string_value(content, "message", msg ? msg : "unknown error");
    adapter_put_json_object_value(content, "code", adapter_json_object_new_int((int)code));

    void *contents = adapter_create_json_array();
    adapter_array_add_json_object(contents, content);
    adapter_put_json_object_value(result, "content", contents);
    return result;
}


static void do_tool_call(void *request){

    char id[32] = {0};
    const char* val = adapter_get_json_string_value(request, "id");
    if (val){
        strncpy(id, val, sizeof(id)-1);
    }else{
        return; 
    }

    void* params = adapter_get_json_node_value(request, "params");
    if(params == NULL){
        return;   
    }
    char* tool_name = adapter_get_json_string_value(params, "name");
    if (!tool_name){
        return;   
    }

    printf("do_tool_call: %s\n", tool_name);

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
        void* ret = make_error_result(-32601, "Tool not found");
        const char* json_string = adapter_serialize_json_to_string(ret);
        transmit_mcp(id, json_string);  
        adapter_delete_json_object(ret);
        return;
    }

    void *args = adapter_get_json_node_value(params, "arguments");
    int arg_count = 0;
    property_t *props = NULL;
    int parsed_count = 0;

    if (args) {
        arg_count = tool->property_count;
        if (arg_count > 0) {
            props = malloc(arg_count * sizeof(property_t));
            if (!props) {
                void* ret = make_error_result(-32000, "internal error");
                const char* json_string = json_object_to_json_string(ret);
                transmit_mcp(id, json_string);  
                adapter_delete_json_object(ret);
                return;
            }
            memset(props, 0, arg_count * sizeof(property_t));
            
            for (int i = 0; i < tool->property_count; i++) {
                property_ptr pdef = &tool->properties[i];
                property_t *p = &props[i];
                strncpy(p->name, pdef->name, MCP_NAME_LEN - 1);
                p->name[MCP_NAME_LEN - 1] = '\0';
                p->type = pdef->type;
                parsed_count++;
                if (p->type == PROPERTY_TYPE_INTEGER) {
                    p->value.int_value = adapter_get_json_int_value(args, pdef->name, 0);
                } else if (p->type == PROPERTY_TYPE_BOOLEAN) {
                    p->value.bool_value = adapter_get_json_boolean_value(args, pdef->name, false);
                } else {

                    const char *valstr = adapter_get_json_string_value(args, pdef->name);
                    if (valstr) {
                        strncpy(p->value.str_value, valstr, sizeof(p->value.str_value) - 1);
                        p->value.str_value[sizeof(p->value.str_value) - 1] = '\0';
                    }
                }
            }
        }
    }

    // 调用工具的回调
    void *out = NULL;
    if (tool->mcp_call_fn) {
        out = tool->mcp_call_fn(props, (size_t)parsed_count);
    }
    
    if (!out) {
        void* ret = make_error_result(-32000, "internal error");
        const char* json_string = adapter_serialize_json_to_string(ret);
        transmit_mcp(id, json_string);  
        adapter_delete_json_object(ret);
    } else {
        
        void *result = adapter_create_json_object();
        void *is_error = adapter_json_object_new_boolean(false);
        adapter_put_json_object_value(result, "isError", is_error);

        void *contents = adapter_create_json_array();
        adapter_array_add_json_object(contents, out);
        adapter_put_json_object_value(result, "content", contents);

        const char* json_string = adapter_serialize_json_to_string(result);
        transmit_mcp(id, json_string);  
        adapter_delete_json_object(result);
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
        return;
    }

    void *parse = adapter_parse_json_string(data);
    if (!parse){
        return;     
    }
    char *version = adapter_get_json_string_value(parse, "jsonrpc");
    if (!version){
        adapter_delete_json_object(parse);
        return;
    }  
    if (strcmp(version, "2.0") != 0){
        adapter_delete_json_object(parse);
        return;     
    }   
    
    char *method = (char*)adapter_get_json_string_value(parse, "method");
    if (!method){
        adapter_delete_json_object(parse);
        return;
    }   
    printf("MCP method: %s\n", method);
    
    if (strcmp(method, "initialize") == 0){
        handle_initialize(parse);
        
    }else if (strcmp(method, "tools/list") == 0){
        handle_tools_list(parse);
    }else if (strcmp(method, "tools/call") == 0)
    {
        do_tool_call(parse);
    }else if(strcmp(method, "notifications/initialized") == 0){
        
    } else{
        printf("unknown MCP method: %s\n", method);
    }

    adapter_delete_json_object(parse);
    return;     
            
}


void* mcp_call_get_device_status(property_ptr props, size_t len){
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
    int volume = adapter_get_audio_volume();
    size_t size = 256;
    char* device_status = malloc(size);
    memset(device_status, 0, size);
    snprintf(device_status, size - 1, "current volume: %d", volume);
    void *result = adapter_create_json_object();
    adapter_put_json_string_value(result, "type", "text");
    adapter_put_json_string_value(result, "text", device_status);
    free(device_status);
    return result;
}

void mcp_init(void *priv){

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
