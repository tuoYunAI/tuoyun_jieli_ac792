#ifndef _VAD_NODE_H_
#define _VAD_NODE_H_

#include "generic/typedef.h"

enum vad_event {
    VAD_EVENT_SPEAK_START,
    VAD_EVENT_SPEAK_STOP,
};

typedef struct {
    u8 auto_refresh_disable;    ///<  VAD单次识别使能
    u16 start_threshold;        ///<  VAD开始阈值
    u16 stop_threshold;         ///<  VAD停止阈值
    int (*vad_callback)(enum vad_event event);  ///<  VAD回调
} vad_node_priv_t;

#endif/*VAD_NODE_H*/

