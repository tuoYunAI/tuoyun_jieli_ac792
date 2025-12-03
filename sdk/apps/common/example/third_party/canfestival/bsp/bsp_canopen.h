#ifndef __canopen_bsp_canopen_H__
#define __canopen_bsp_canopen_H__

void canopen_solo_timer_enable(void);
void canopen_sys_timer_enable(void);
void canopen_can_init(void);
void canopen_time_sem_pend(void);
void canopen_time_sem_post(void);
void *get_canopen_hdl(void);

#endif
