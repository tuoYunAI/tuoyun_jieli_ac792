#ifndef SYS_TIME_H
#define SYS_TIME_H

#include "generic/typedef.h"

struct sys_time {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u8 sec;
};

u32 timer_get_sec(void);

u32 timer_get_ms(void);

unsigned int time_lapse(unsigned int *handle, unsigned int time_out);//2^32/1000/60/60/24 后超时

void set_local_time_init_flag(u8 flag);

void set_time_compensate_ms(u32 ms);

void set_time_compensate_sec(u32 sec);

void set_sys_source_timer(time_t time);

void set_rtc_time_base_on_unix_timestamp(time_t time);

#endif
