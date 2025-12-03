#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/rtc.h"
#include "asm/adc_api.h"
#include <time.h>
#include <sys/time.h>


#ifdef USE_RTC_TEST_DEMO

static void *rtc_hdl;


#define printf_time(str, time)  printf(str"%d-%d-%d %d:%d:%d\n", time.year, time.month, time.day, \
                                                                 time.hour, time.min, time.sec)



static void cpu_reset_test(void)
{
    /*cpu_reset();*/
    system_soft_reset();
}

static void time_rtc_test_task(void *arg)
{
    static struct sys_time time;

    // adc_init()和rtc_early_init()一般在板级board_init()里初始化。
    // adc_init();             ///< rtc_early_init()里调用adc相关接口，因此需要先初始化ADC。
    // rtc_early_init();       ///< 使用RTC前需要先初始化。

    // 打开RTC设备
    rtc_hdl = dev_open("rtc", NULL);
    if (!rtc_hdl) {
        printf("err: rtc open fail!!!\n");
        return;
    }

    // 获取闹钟信息
    dev_ioctl(rtc_hdl, IOCTL_GET_ALARM, (u32)&time);
    printf_time("get_alarm_time:", time);

    // 获取时间信息
    dev_ioctl(rtc_hdl, IOCTL_GET_SYS_TIME, (u32)&time);
    printf_time("frist_time_get_sys_time:", time);

    // 设置时间信息
    time.sec = 0;
    dev_ioctl(rtc_hdl, IOCTL_SET_SYS_TIME, (u32)&time);
    printf_time("set_sys_time:", time);

    // 设置闹钟
#if 1
    time.sec = 59;
    dev_ioctl(rtc_hdl, IOCTL_SET_ALARM, (u32)&time);
#else
    dev_ioctl(rtc_hdl, IOCTL_SET_COUNTDOWN_ALARM, 30); ///< 设置倒计时闹钟，单位s
#endif

    // 打开闹钟开关
    dev_ioctl(rtc_hdl, IOCTL_SET_ALARM_ENABLE, 1); ///< 闹钟使能。闹钟响铃回调函数接口在板级里配置

    /* sys_timeout_add(NULL, cpu_reset_test, 4000); ///< 可测试系统复位对RTC走时的影响 */

    while (1) {
        dev_ioctl(rtc_hdl, IOCTL_GET_SYS_TIME, (u32)&time);
        printf_time("get_sys_time:", time);

        dev_ioctl(rtc_hdl, IOCTL_GET_ALARM, (u32)&time);
        printf_time("get_alarm_time:", time);

        printf("\n");
        os_time_dly(200);
    }
}

static int c_main_rtc(void)
{
    os_task_create(time_rtc_test_task, NULL, 10, 1000, 0, "time_rtc_test_task");
    return 0;
}

late_initcall(c_main_rtc);

#endif ///< USE_RTC_TEST_DEMO
