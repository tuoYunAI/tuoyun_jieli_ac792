#include "system/includes.h"
#include "app_config.h"
#include "asm/gpcnt.h"
#include "asm/gpio.h"
#include "device/device.h"

#ifdef USE_GPCNT_TEST_DEMO

// #define GPCNT_NON_BOARD			//不使用板级文件配置初始化
// #define GPCNT_MODIFY_CLK_SOURCE	//修改主次时钟源

static void gpcnt_test_task(void *arg)
{
    void *gpcnt_hdl = 0;
    u32 gpcnt = 1;
#ifdef GPCNT_NON_BOARD
    struct gpcnt_platform_data user_data;
    user_data.gpio = IO_PORTB_01;
    user_data.gss_clk_source = GPCNT_GSS_CSS_PLL_96M;
    user_data.css_clk_source = GPCNT_GSS_CSS_STD_48M;
    user_data.cycle = 15;
    gpcnt_hdl = dev_open("gpcnt", &user_data);
#else
    gpcnt_hdl = dev_open("gpcnt", NULL);
#endif

#ifdef GPCNT_MODIFY_CLK_SOURCE
    dev_ioctl(gpcnt_hdl, IOCTL_GPCNT_SET_GSS_CLK_SOURCE, GPCNT_GSS_CSS_STD_48M);
    dev_ioctl(gpcnt_hdl, IOCTL_GPCNT_SET_CSS_CLK_SOURCE, GPCNT_GSS_CSS_PLL_96M);
#endif
    while (1) {
        dev_ioctl(gpcnt_hdl, IOCTL_GPCNT_GET_CNT, (u32)&gpcnt);
        printf(">>>>>>>>>>>gpcnt = %d", gpcnt);
        os_time_dly(300);
    }
}

static int c_main_gpcnt(void)
{
    os_task_create(gpcnt_test_task, NULL, 10, 1000, 0, "gpcnt_test_task");
    return 0;
}

late_initcall(c_main_gpcnt);

#endif // USE_GPCNT_TEST_DEMO
