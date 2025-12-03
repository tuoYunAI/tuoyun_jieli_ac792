#ifndef __AUDIO_LLNS_DNS_H__
#define __AUDIO_LLNS_DNS_H__

#include "tech_lib/jlsp_llns_dns.h"

typedef struct _LLNS_DNS_TOOL_SET {
    int is_bypass;
    int level; 			//设置降噪等级，分为4档，0：不降噪，1：降噪强度较轻，2：降噪强度适中，3：降噪强度较强；默认选择2
    llns_dns_param_t parm;
    llns_dns_agc_param_t agc_parm;
} llns_dns_param_tool_set;

struct llns_dns_open_parm {
    llns_dns_param_t parm;
    llns_dns_agc_param_t agc_parm;
    u32 samplerate;
    int level;
    u8 bit_width;
};

typedef struct _llns_dns_hdl {
    struct llns_dns_open_parm llns_dns_parm;
    u8 update;
    u8 status;
    u8 first_run;

    u16 frame_size;   //处理的每包数据长度，单位points
    int out_buf_size; //llns_dns输出的每包数据长度，单位points

    char *share_buf;
    char *private_buf;
    int share_head_size;
    int private_head_size;
    void *llns_dns; //llns_dns操作句柄
} llns_dns_hdl;


//打开
llns_dns_hdl *audio_llns_dns_open(struct llns_dns_open_parm *param);

//关闭
void audio_llns_dns_close(llns_dns_hdl *hdl);

//参数更新
void audio_llns_dns_update_parm(llns_dns_hdl *hdl, llns_dns_param_tool_set *cfg);

//数据处理
int audio_llns_dns_run(llns_dns_hdl *hdl, s16 *indata, s16 *outdata, int len);

//暂停处理
void audio_llns_dns_bypass(llns_dns_hdl *hdl, u8 bypass);

#endif
