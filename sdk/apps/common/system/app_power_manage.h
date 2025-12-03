#ifndef __APP_POWER_MANAGE_H__
#define __APP_POWER_MANAGE_H__

#include "generic/typedef.h"
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    POWER_EVENT_POWER_NORMAL,
    POWER_EVENT_POWER_WARNING,
    POWER_EVENT_POWER_LOW,
    POWER_EVENT_POWER_CHANGE,
    POWER_EVENT_POWER_AUTOOFF,
    POWER_EVENT_POWER_CHARGE,
    POWER_EVENT_SYNC_TWS_VBAT_LEVEL,
};

u8 get_cur_battery_level(void);
u8 get_self_battery_level(void);
void power_event_to_user(u8 event);

void sys_power_init(void);
int sys_power_get_battery_voltage(void);
int sys_power_get_battery_persent(void);
int sys_power_is_charging(void);
void sys_power_poweroff(void);
/*
 * @brief 软关机，触发DEVICE_EVENT_POWER_SHUTDOWN事件，app捕获事件释放资源再调用sys_power_poweroff()
 */
void sys_power_shutdown(void);
/*
 * @brief 倒计时自动关机
 * @parm dly_secs 延时关机时间，赋值0为永不关机
 * @return none
 */
void sys_power_auto_shutdown_start(u32 dly_secs);
void sys_power_auto_shutdown_pause(void);
void sys_power_auto_shutdown_resume(void);
void sys_power_auto_shutdown_clear(void);
void sys_power_auto_shutdown_stop(void);

#ifdef __cplusplus
}
#endif

#endif
