#ifndef _AUDIO_LLNS_H_
#define _AUDIO_LLNS_H_

#include "jlsp_llns.h"

typedef struct _LLNS_TOOL_SET {
    int is_bypass;
    int div;
    llns_param_t parm;
} llns_param_tool_set;

struct llns_open_parm {
    llns_param_t parm;
    u32 samplerate;
    u8 div;
    u8 bit_width;
};

typedef struct _llns_hdl {
    struct llns_open_parm llns_parm;
    u8 update;
    u8 status;

    u16 frame_size;   //处理的每包数据长度，单位points
    int out_buf_size; //llns输出的每包数据长度，单位points

    char *share_buf;
    char *private_buf;
    int share_head_size;
    int private_head_size;
    void *llns; //llns操作句柄
} llns_hdl;

//打开
llns_hdl *audio_llns_open(struct llns_open_parm *param);

//关闭
void audio_llns_close(llns_hdl *hdl);

//参数更新
void audio_llns_update_parm(llns_hdl *hdl, llns_param_t *parm);

//数据处理
int audio_llns_run(llns_hdl *hdl, s16 *indata, s16 *outdata, int len);

//暂停处理
void audio_llns_bypass(llns_hdl *hdl, u8 bypass);

#endif
