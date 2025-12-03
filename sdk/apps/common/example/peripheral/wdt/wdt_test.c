#include "app_config.h"
#include "system/includes.h"
#include "asm/wdt.h"

#ifdef USE_WDT_TEST_DEMO

// 重写覆盖库内弱函数,可以选择不再使用库内清看门狗
/* void wdt_clear(void) */
/* { */

/* } */


static void wdt_test_task(void *arg)
{
    printf("\r\n\r\n -----------wdt_test_task run begin %s -------------\r\n\r\n", __TIME__);
    while (1); ///< 迫使看门狗溢出复位
}


static void user_wdt_clear_task(void *arg)
{
    while (1) {
        wdt_clear();
        printf("wdt_clear\n");
        os_time_dly(100);
    }
}


static int c_main_wdt(void)
{
    /* wdt_close();// 关闭看门狗,程序跑飞不会复位 */

    // 用户自己外部清狗线程，防止死机复位，注释会死机
    os_task_create(user_wdt_clear_task, NULL, 31, 1000, 0, "user_wdt_clear_task");

    os_task_create(wdt_test_task, NULL, 30, 1000, 0, "wdt_test_task");

    return 0;
}

late_initcall(c_main_wdt);

#endif // USE_WDT_TEST_DEMO
