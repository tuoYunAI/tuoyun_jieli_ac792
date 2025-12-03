#ifndef _AUDIO_AUTOWAH_API_H_
#define _AUDIO_AUTOWAH_API_H_

#include "effects/effect_wah_api.h"
// #include "media/audio_stream.h"
//
struct autowah_update_parm {
    int wahType;         //从下拉框选，AWAH_TYPE0,AWAH_TYPE1
    int moderate;         //0到15
    int freq_min;        //10到20000
    int freq_max;         //10到20000
    int wet;             //0到200
    int dry;             //0到100
    int reserve;        //保留位
};

typedef struct _autowah_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct autowah_update_parm parm;
} autowah_param_tool_set;

struct autowah_open_parm {
    wah_parm_context param;
    u32 sample_rate;
    u8 ch_num;
    u8 bypass;
};

typedef struct _autowah_hdl {
    EFFECT_WAH_FUNC_API *ops;
    void *workbuf;
    struct autowah_open_parm parm;
    u8 update;
    u8 status;
} autowah_hdl;

/*
 * 打开
 */
autowah_hdl *audio_autowah_open(struct autowah_open_parm *param);
/*
 *  关闭
 */
void audio_autowah_close(autowah_hdl *hdl);
/*
 *  参数更新
 */
void audio_autowah_update_parm(autowah_hdl *hdl, struct autowah_update_parm *parm);
/*
 *  数据处理
 */
int audio_autowah_run(autowah_hdl *hdl, s16 *indata, s16 *outdata, int len);
/*
 *  暂停处理
 */
void audio_autowah_bypass(autowah_hdl *hdl, u8 bypass);
#endif

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

