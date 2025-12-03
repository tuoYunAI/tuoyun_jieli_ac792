#include "generic/typedef.h"
#include "asm/adc_api.h"
#include "device/gpio.h"
#include "system/includes.h"
#include "app_config.h"

#ifdef USE_ADC_TEST_DEMO

static void adc_test_task(void *arg)
{
    u16 adc_val;
    u16 io_vol;

    // 测量AD通道
    u32 adc_io = IO_PORTD_00;
    u32 adc_port = ADC_IO_CH_PD00;

    adc_init();                     //初始化adc。一般在板级board_init()里初始化，在此处调用是为了方便演示

    gpio_set_die(adc_io, 0);        //关闭数字输入，将io设为模拟功能
    // 浮空输入
    gpio_set_direction(adc_io, 1);  //方向设为输入
    gpio_set_pull_up(adc_io, 0);    //关上拉10K
    gpio_set_pull_down(adc_io, 0);  //关下拉10K

    adc_add_sample_ch(adc_port);    //添加通道

    while (1) {
        os_time_dly(100);
        adc_val = adc_get_value(adc_port);
        io_vol = adc_get_voltage(adc_port);
        printf("adc_val = %d  >>>  %d mv\n", adc_val, io_vol);
    }

    return;
}

static int c_main_adc()
{
    os_task_create(adc_test_task, NULL, 10, 1000, 0, "adc_test_task");
    return 0;
}

late_initcall(c_main_adc);

#endif // USE_ADC_TEST_DEMO
