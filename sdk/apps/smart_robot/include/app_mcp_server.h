#ifndef __MCP_SERVER_H__
#define __MCP_SERVER_H__
#include "app_protocol.h"

#define MCP_NAME_LEN 64

typedef enum property_type {
    PROPERTY_TYPE_BOOLEAN = 0,
    PROPERTY_TYPE_INTEGER,
    PROPERTY_TYPE_STRING
}property_type_e;


typedef struct property{
    char name[MCP_NAME_LEN];
    property_type_e type;
    int required;

    union {
        int int_value;
        char str_value[64];
        int bool_value;
    }value;
    int max_int_value;
    int min_int_value;  
    char *description; 
} property_t, *property_ptr;

/* Function type for calling MCP tool handlers.
 * Parameters:
 *  - property_t *props : pointer to an array of properties (inputs)
 *  - size_t len        : number of elements in the props array
 * Returns:
 *  {
        "jsonrpc": "2.0",
        "id": 2,
        "result": 
    }
 *  - char * : content of key result,  a heap-allocated JSON string (caller-owned) with the result/output
 * 


 */
typedef char *(*mcp_call_fn)(property_ptr props, size_t len);

void add_mcp_tool(char* name, char* description, mcp_call_fn callback, property_ptr props, size_t len);

void handle_received_mcp_request(const char *data, size_t len);

#endif