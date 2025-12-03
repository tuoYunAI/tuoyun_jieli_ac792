#ifndef __CANOPEN_DEFINE_H__
#define __CANOPEN_DEFINE_H__


// #define CANOPEN_MASTER_ENABLE			///< CANOPEN协议栈作主站模式，尽量不要主站从站同时打开
#define CANOPEN_SLAVE_ENABLE			///< CANOPEN协议栈作从站模式，尽量不要主站从站同时打开

#define CANOPEN_TIMER_10MS_ENABLE		///< 使能选择使用系统定时器最小支持10ms心跳时间，失能选择使用独立定时器最小支持1ms心跳时间

#endif
