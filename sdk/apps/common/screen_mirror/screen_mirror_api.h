#ifndef __NET_SCR_API_H__
#define __NET_SCR_API_H__

#include "sock_api/sock_api.h"
#include "server/rt_stream_pkg.h"



struct __JPG_INFO {
    u32 src_w;
    u32 src_h;
    u32 buf_len;
    u8  buf[];
};

struct __NET_SCR_CFG {
    struct sockaddr_in cli_addr;
    int fps;
    int src_w;
    int src_h;
    int prot;   //prot:协议类型 0->tcp 1->udp
    int ack;    //ack:规则类型 0->以帧率为准 1->以回包为准
    void *ack_cb;   //回包回调函数
};

u8 get_net_scr_status(void);
int net_scr_init(struct __NET_SCR_CFG *cfg);
int net_scr_uninit(struct __NET_SCR_CFG *cfg);

#endif

