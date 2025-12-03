#ifndef __MCP_SERVER_H__
#define __MCP_SERVER_H__
#include "protocol.h"

void register_mcp_service(char* name, char* tool_json);

void handle_received_mcp_request(const char *data, size_t len);

#endif