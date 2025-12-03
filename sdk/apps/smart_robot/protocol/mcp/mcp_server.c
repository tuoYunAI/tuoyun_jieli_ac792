#include "app_config.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/includes.h"
#include "app_event.h"
#include "mcp_server.h"

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
    char* name;
    char* tool_json;
}mcp_tool_t, *mcp_tool_ptr;

static osip_list_t m_mcp_tools_list;
static int m_mcp_tools_count = -1;
static OS_MUTEX mutex;

void register_mcp_service(char* name, char* tool_json){
    if (!name || !tool_json){
        log_info("invalid mcp tool info\n");
        return;
    }
    log_info("register mcp tool: %s, %s\n", name, tool_json);

    os_mutex_pend(&mutex, 0);
    if (m_mcp_tools_count < 0){
        osip_list_init(&m_mcp_tools_list);
        m_mcp_tools_count = 0;
    }
    do{

        mcp_tool_ptr tool = malloc(sizeof(mcp_tool_t));
        if (!tool){
            log_info("failed to alloc mem for mcp tool\n");
            break;
        }
        char* name_copy = malloc(strlen(name)+1);
        if (!name_copy){
            log_info("failed to alloc mem for mcp tool name\n");
            free(tool);
            break;
        }
        strcpy(name_copy, name);
        char* json_copy = malloc(strlen(tool_json)+1);
        if (!json_copy){
            log_info("failed to alloc mem for mcp tool json\n");
            free(name_copy);
            free(tool);
            break;
        }
        strcpy(json_copy, tool_json);
        tool->name = name_copy;
        tool->tool_json = json_copy;
        if (osip_list_add(&m_mcp_tools_list, tool, -1) != 0){
            log_info("failed to add mcp tool to list\n");
            free(json_copy);
            free(name_copy);    
            free(tool);    
        }
        m_mcp_tools_count++;
        log_info("mcp tool added, total count: %d\n", m_mcp_tools_count);
    }while(0);
    
    os_mutex_post(&mutex);
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
    snprintf(buf, len, "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":%s}", id, message);
    
    json_object *root = json_object_new_object();
    json_object *protocol = json_object_new_string("MCP");
    json_object *payload = json_object_new_string(buf);
    
    json_object_object_add(root, "protocol", protocol);
    json_object_object_add(root, "payload", payload);
    
    const char* json_string = json_object_to_json_string(root);
    transmit_mqtt_message((char*)json_string);
    json_object_put(root);
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

    // 确保列表初始化
    if(m_mcp_tools_count < 0){
        os_mutex_pend(&mutex, 0);
        osip_list_init(&m_mcp_tools_list);
        m_mcp_tools_count = 0;
        os_mutex_post(&mutex);
    }

    // 组装结果
    json_object *result = json_object_new_object();
    json_object *tools_arr = json_object_new_array();

    int added = 0;
    for (int i = start; i < m_mcp_tools_count && added < limit; i++) {
        mcp_tool_ptr tool = (mcp_tool_ptr)osip_list_get(&m_mcp_tools_list, i);
        if (!tool) continue;

        // tool_json 应该是一个 JSON 对象字符串
        json_object *tool_obj = json_tokener_parse(tool->tool_json);
        if (!tool_obj) {
            // 兜底：构造一个最小对象
            tool_obj = json_object_new_object();
            json_object_object_add(tool_obj, "name", json_object_new_string(tool->name ? tool->name : ""));
            json_object_object_add(tool_obj, "definition", json_object_new_string(tool->tool_json ? tool->tool_json : "{}"));
        } else {
            // 若缺少 name，用注册名补齐
            json_object *name_field = json_object_object_get(tool_obj, "name");
            if (!name_field && tool->name) {
                json_object_object_add(tool_obj, "name", json_object_new_string(tool->name));
            }
        }

        json_object_array_add(tools_arr, tool_obj);
        added++;
    }

    // 计算 nextCursor
    if (start + added < m_mcp_tools_count) {
        char next_buf[16];
        snprintf(next_buf, sizeof(next_buf), "%d", start + added);
        json_object_object_add(result, "nextCursor", json_object_new_string(next_buf));
    } 

    json_object_object_add(result, "tools", tools_arr);

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
        char resp[64];
        snprintf(resp, sizeof(resp), "{\"error\":{\"code\":-32601,\"message\":\"Tool not found: %s\"}}", tool_name);
        transmit_mcp(id, resp);
        return;
    }

    // 模拟调用工具，实际应用中应根据工具定义执行相应操作
    log_info("Invoking tool: %s with definition: %s\n", tool->name, tool->tool_json);

    // 构造模拟结果
    char resp[128];
    snprintf(resp, sizeof(resp), "{\"output\":\"Tool %s executed successfully.\"}", tool->name);
    transmit_mcp(id, resp);

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

