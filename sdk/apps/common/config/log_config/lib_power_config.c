#include "app_config.h"
#include "generic/typedef.h"
#include "gpio.h"

//-------------------------------------------------------------------
/*
 * 调试pdown进不去的场景，影响低功耗流程
 * 打印蓝牙和系统分别可进入低功耗的时间(msec)
 * 打印当前哪些模块处于busy,用于蓝牙已经进入sniff但系统无法进入低功耗的情况
 */
const char debug_is_idle = 0;

//-------------------------------------------------------------------
/* 调试快速起振信息，不影响低功耗流程
 */
const bool pdebug_xosc_resume = 0;

//-------------------------------------------------------------------
/* 调试低功耗流程
 */
//出pdown打印信息，不影响低功耗流程
const bool pdebug_pdown_info = 1;

//使能串口调试低功耗
const u32 pdebug_uart_lowpower = 1;
#ifdef TCFG_DEBUG_PORT
const u32 pdebug_uart_port = TCFG_DEBUG_PORT;
#else
const u32 pdebug_uart_port = -1;
#endif

//使能调试pdown流程打印
const bool pdebug_putbyte_pdown = 0;

//使能调试soff流程打印
const bool pdebug_putbyte_soff = 1;

//使能串口pdown/poff/soff打印所有的寄存器
const bool pdebug_lp_dump_ram = 0;

//使能uart_flowing
const bool pdebug_uart_flowing = 0;
