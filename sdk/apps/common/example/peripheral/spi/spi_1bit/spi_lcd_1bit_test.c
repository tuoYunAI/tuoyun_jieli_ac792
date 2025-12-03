#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"
#include "device/gpio.h"


#ifdef USE_SPI_LCD_1BIT_TEST_DEMO

// 说明:该测试例子为spi1线推屏接口测试 最要用于测试接口推屏能力
// 需要将isd_config_rule.c LSB_DIV=1; [低速总线时钟分频系数 LSB_DIV+1]
/*
 * 参考板级配置:
 * SPI1_PLATFORM_DATA_BEGIN(spi1_data)
 *     .io = {
 *         .cs_pin             = -1,
 *         .di_pin             = -1,
 *         .do_pin             = IO_PORTA_05,
 *         .clk_pin            = IO_PORTA_04,
 *         .d2_pin             = -1,
 *         .d3_pin             = -1,
 *     },
 *     .clk                    = 80000000,
 *     .mode                   = SPI_1WIRE_MODE,
 *     .priority               = 6,
 *     .cpu_id                 = 1,
 *     .hd_level               = 0,
 *     .attr                   = SPI_SCLK_L_UPL_SMPH,
 *     .role                   = SPI_MODE_MASTER,
 * SPI1_PLATFORM_DATA_END()
 */

#define SPI_LEN 320 * 240 * 2
static void Calculation_frame(void)
{
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;

        if (tdiff >= 1000) {
            printf("\n [MSG]fps_count = %d\n", fps_cnt *  1000 / tdiff);
            tstart = 0;
            fps_cnt = 0;
        }
    }
}

SEC_USED(.sram) static u8 data[SPI_LEN] ;
static void spi_lcd_1bit_test_task(void *arg)
{
    void *spi_hdl = NULL;

    //1.打开spi设备
    spi_hdl = dev_open("spi1", NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }

    memset(data, 0x55, SPI_LEN);
    // 设置为同步方式发送数据包
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_ASYNC_SEND, 0);

    while (1) {
        dev_write(spi_hdl, data, 1);//发送1bit使用该接口
        dev_write(spi_hdl, data, 1);//发送1bit使用该接口
        dev_write(spi_hdl, data, 1);//发送1bit使用该接口

        /* os_time_dly(1); */
        dev_write(spi_hdl, data, SPI_LEN);
        Calculation_frame();
        /* os_time_dly(1); */

        // clear sys dog
        // yield();
    }

exit:
    if (spi_hdl) { //关闭spi设备
        dev_close(spi_hdl);
    }
    while (1) { //等待其他任务杀此任务
        os_time_dly(100);
    }
}

static int c_main_spi_lcd_1bit(void)
{
    os_task_create(spi_lcd_1bit_test_task, NULL, 22, 1000, 0, "spi_lcd_1bit_test_task");
    return 0;
}

late_initcall(c_main_spi_lcd_1bit);

#endif // USE_SPI_LCD_1BIT_TEST_DEMO
