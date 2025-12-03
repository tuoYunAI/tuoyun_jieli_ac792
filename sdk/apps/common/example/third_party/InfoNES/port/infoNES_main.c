#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "InfoNES.h"

#ifdef CONFIG_NES_DEMO

static void nes_main_task(void *priv)
{
    user_lcd_init();

#if 0
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
        puts("waitting sd on... ");
    }
#endif

    if (InfoNES_Load(NULL) == 0) {
        InfoNES_Main();
    }
}

static int nes_main_task_init(void)
{
    puts("nes_main_task_init \n\n");
    return thread_fork("nes_main_task", 25, 1024, 0, 0, nes_main_task, NULL);
}
late_initcall(nes_main_task_init);

#endif



