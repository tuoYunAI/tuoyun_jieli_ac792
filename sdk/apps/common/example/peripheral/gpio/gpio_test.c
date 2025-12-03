#include "asm/gpio.h"
#include "system/includes.h"
#include "app_config.h"
#include "asm/exti.h"
#include "device/gpio.h"

#ifdef USE_GPIO_TEST_DEMO

static const u32 gpio_regs[] = {
    (u32) JL_PORTA,
    (u32) JL_PORTB,
    (u32) JL_PORTC,
    (u32) JL_PORTD,
    (u32) JL_PORTE,
    (u32) JL_PORTF,
};
static spinlock_t locks[ARRAY_SIZE(gpio_regs)] = {0};

static void io_exti_handler(void *priv, unsigned int gpio)   //外部中断处理
{
    printf("hello,jl\n");
}

static void gpio_test_task(void *arg)
{
    int ret;
    // gpio中断模式使用（i/o口作外部中断，外部中断引脚，上升沿触发，中断响应函数）
    // ret = exti_init(IO_PORTD_06, EDGE_POSITIVE, io_exti_handler, NULL);
    // printf("exti_init:%d\n", ret);

    // 设置GPIO输入，使用--互斥接口。
    /*
     * 非互斥接口存在的问题：
     * 对一组PORT的某个IO进行配置流程：读寄存器--对指定IO的位逻辑运算--运算结果写回寄存器。
     * 如果A程序的IO操作执行到-读寄存器后，被不同线程或者中断打断（这里且称为B程序），并
     * 且B程序操作了同一组PORT的不同IO，这时cpu返回A程序继续执行，A程序写回的寄存器值是不
     * 包含B程序前面的操作的，那么B程序的IO操作约等于没有执行到。
     *
     * 解决：考虑到这个问题，互斥的接口里，在读寄存器前上了锁，写回寄存器后解锁。这样就能避
     * 免上述情况。
     */
    gpio_direction_input(IO_PORTA_04);        // GPIO输入
    gpio_set_pull_down(IO_PORTA_04, 1);       // 下拉配置
    gpio_set_die(IO_PORTA_04, 1);             // 使能数字输入


    // 设置GPIO输入，使用--非互斥接口。
    /*
     * 同一组引脚之间没有互斥, 针对用户有高速度要求, 并且不需要考虑同一组引脚之间操作互斥才允许使用
     */
    // __gpio_direction_input(IO_PORTA_04);   // GPIO输入
    // __gpio_set_pull_up(IO_PORTA_04, 1);    // 上拉配置
    // __gpio_set_die(IO_PORTA_04, 1);        // 使能数字输入


    ret = gpio_read(IO_PORTA_04);
    printf("ret:%d\n", ret);


    // 设置GPIO输出。
    gpio_direction_output(IO_PORTA_05, 1);  // GPIO输出
    gpio_set_hd(IO_PORTA_05, 1);            // 强驱配置


    // 特殊引脚如USB/配置为GPIO使用方法
    gpio_direction_output(IO_PORT_USB_DPA, 1);
    gpio_direction_output(IO_PORT_USB_DMA, 1);


    /********************************以下为GPIO直接操作寄存器示例*********************************/

    /* gpio_direction_output(IO_PORTC_03, 1);  // GPIO输出 */
    spin_lock(&locks[2]);        // 对PORTC引脚上锁，0、1、2，分别代表A、B、C 组引脚,以此类推下去
    JL_PORTC->OUT |= BIT(3);     // 将引脚PC03设置为高电平,0、1、2对应引脚号，对应bit位置1则为高电平
    JL_PORTC->DIR &= ~BIT(3);    // 将引脚PCO3设置为输出模式,0、1、2对应引脚号，对应bit位置0为输出模式
    spin_unlock(&locks[2]);      // 对PORTC引脚解锁，0、1、2，分别代表A、B、C 组引脚,以此类推下去
    os_time_dly(300);


    /* gpio_set_hd(IO_PORTC_03, 1);            // 强驱配置 */
    spin_lock(&locks[2]);
    JL_PORTC->HD0 |= BIT(3);
    JL_PORTC->HD1 |= BIT(3);
    spin_unlock(&locks[2]);
    os_time_dly(300);


    /* gpio_direction_output(IO_PORTC_03, 0);  // GPIO输出 */
    spin_lock(&locks[2]);
    JL_PORTC->OUT &= ~BIT(3);
    JL_PORTC->DIR &= ~BIT(3);
    spin_unlock(&locks[2]);
    os_time_dly(300);


    /* gpio_direction_intput(IO_PORTC_02, 1);  // GPIO输入 */
    spin_lock(&locks[2]);
    JL_PORTC->DIR |= BIT(2);
    spin_unlock(&locks[2]);
    os_time_dly(300);


    // gpio_set_pull_up(IO_PORTC_02, 1);       // GPIO上拉
    spin_lock(&locks[2]);
    JL_PORTC->PU0 |= BIT(2);
    JL_PORTC->PU1 |= BIT(2);
    spin_unlock(&locks[2]);
    os_time_dly(300);


    /* gpio_set_pull_down(IO_PORTC_02, 1);     // GPIO下拉 */
    spin_lock(&locks[2]);
    JL_PORTC->PD0 |= BIT(2);
    JL_PORTC->PD1 |= BIT(2);
    spin_unlock(&locks[2]);
    os_time_dly(300);


    /* gpio_read(IO_PORTC_02); 	     		   // 读GPIO */
    spin_lock(&locks[2]);
    ret = !!(JL_PORTC->IN & BIT(2));
    printf("ret:%d\n", ret);
    spin_unlock(&locks[2]);
    os_time_dly(300);


    /* gpio_set_die(IO_PORTC_02, 1);           // GPIO设置为数字输入 */
    spin_lock(&locks[2]);
    JL_PORTC->DIE |= BIT(2);
    spin_unlock(&locks[2]);
    os_time_dly(300);

    while (1) {
        os_time_dly(10);
    }
}


static int c_main_gpio()
{
    os_task_create(gpio_test_task, NULL, 10, 1000, 0, "gpio_test_task");
    return 0;
}

late_initcall(c_main_gpio);

#endif // USE_GPIO_TEST_DEMO
