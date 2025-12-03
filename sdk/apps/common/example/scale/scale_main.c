#include "app_config.h"
#include "system/includes.h"

#include "gpio.h"
#include "asm/clock.h"
//extern int dma2d_verify();
#ifdef USE_SCALE_MAIN_TEST_DEMO

extern void test_scale_demo();
extern void yuv_test_code_by_line_420p();
extern void yuv_test_code_by_line_422p();

void scale_test_demo()
{

    /*printf("\n[lyc][this file] %s -[this function] %s -[this lnie] %d\n", __FILE__, __FUNCTION__, __LINE__);*/

    printf("scale_test\n");
    video_clock_early_init();
    os_time_dly(50);

    while (!storage_device_ready()) { //
        printf("scale sd card");
        os_time_dly(50);
    }



#if 1
    int mode_num = 6;
    for (int mode = 0; mode < mode_num; mode++) {
        test_scale_demo(1, mode); //通道1
        printf("\n----channel:1 mode:%d----done\n", mode);
    }

    for (int mode = 0; mode < mode_num; mode++) {
        test_scale_demo(3, mode); //通道3
        printf("\n----channel:3 mode:%d----done\n", mode);
    }

    for (int mode = 0; mode < mode_num; mode++) {
        test_scale_demo(4, mode); //通道4
        printf("\n----channel:4 mode:%d----done\n", mode);
    }

    for (int mode = 0; mode < 3; mode++) { //yuv420p
        yuv_test_code_by_line_420p(mode);
        printf("\n[yuv420]----channel:1 mode:%d----done\n", mode);
    }

    for (int mode = 0; mode < 3; mode++) { //yuv420p
        yuv_test_code_by_line_422p(mode);
        printf("\n[yuv422]----channel:1 mode:%d----done\n", mode);
    }

    /*0://vga->2k */

    /*1://vga->4k*/

    /*2://vga->vga/2*/

#endif

}

static int scale_main_task(void)
{
    puts("scale_main_task_init \n\n");
    //说明由于LVGL没有加延时让出CPU需要配置为最低优先级 高优先级不能长时间占用CPU不然LVGL运行卡顿
    return thread_fork("scale_test_demo", 100, 8 * 1024, 0, 0, scale_test_demo, NULL);
}
late_initcall(scale_main_task);


#endif
