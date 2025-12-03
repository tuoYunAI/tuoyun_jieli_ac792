#include "app_config.h"
#include "system/includes.h"
#include "gpio.h"

#ifdef  USE_TIMER_CAPTURE_TEST
/*
 * timer capture测试例程
 *
 * 1.例程逻辑：
 * 使用timer的capture功能测量信号脉宽
 *
 * 2.脉宽测试原理：
 * Timer的capture功能会捕获上升沿/下降沿信号，触发时会将当前的CNT寄存器值
 * 存放到PRD寄存器，并触发中断。以测试高电平脉宽为例，在检测到上升沿触发中断
 * 时将CNT和PRD寄存器清0，并设置下一个触发边沿为下降沿，等下降沿到来时获得的
 * PRD值便是高电平脉宽cnt值，结合timer的clk频率可以计算得到脉宽时间。
 *
 * 3.注意事项：
 *     a.用户只能选择：JL_TIMER2、JL_TIMER3、JL_TIMER4、JL_TIMER5
 *     b.使用定时器2或者3映射PWM时，不能用于定时
 *
 * 4.其它：
 * 本例程以STD_12M的4分频作为timer时钟源，即3M。
 */


//=========================debug code start=======================
// 打印调试(打印耗时较长，不适合捕获脉宽较短的信号的调试)
#define LOG_ENABLE 1
#if LOG_ENABLE
#define PRINTF_USER         printf
#else
#define PRINTF_USER         my_printf
#endif
int my_printf(const char *format, ...)
{
    return 0;
}


// IO电平翻转方式调试
#define DGB_ENABLE 1
#define DBG_PORT_BIT_A      BIT(0)
#define DBG_PORT_A          JL_PORTC ///< pend sem
#define DBG_PORT_BIT_B      BIT(1)
#define DBG_PORT_B          JL_PORTC ///< capture start
#define DBG_PORT_BIT_C      BIT(2)
#define DBG_PORT_C          JL_PORTC ///< capture end

#if DGB_ENABLE
#define DBG_IO_FLIP(PORT,BIT) \
    do { \
        PORT->OUT ^= BIT; \
        PORT->OUT ^= BIT; \
    }while(0)

#define DBG_IO_INIT(PORT,BIT) \
    do{ \
        PORT->DIR &= ~BIT; \
        PORT->PU0 &= ~BIT; \
        PORT->PU1 &= ~BIT; \
        PORT->PD0 &= ~BIT; \
        PORT->PD1 &= ~BIT; \
        PORT->OUT &= ~BIT; \
        PORT->HD0 |=  BIT; \
        PORT->HD1 |=  BIT; \
    }while(0)
#else
#define DBG_IO_FLIP(PORT,BIT)
#define DBG_IO_INIT(PORT,BIT)
#endif
//=========================debug code end=======================






#define TMR                 JL_TIMER5                    ///< 测试使用的定时器
#define TMR_IRQ_ID          IRQ_TIMER5_IDX               ///< 使用的定时器的中断号
#define TMR_INPUT_CH_CAP    INPUT_CH_SIGNAL_TIMER5_CAP   ///< 使用的定时器CAP信号channel
#define TMR_CAPTURE_IO      IO_PORTA_04                  ///< 映射TIMER的CAP信号的IO
#define TMR_CAPTURE_LEVEL   LEVEL_HIGH_WIDTH             ///< 捕获的电平类型
/* #define TMR_CAPTURE_LEVEL   LEVEL_LOW_WIDTH              ///< 捕获的电平类型 */
#define TMR_CLK             3000000                      ///< timer时钟(分频后的)


typedef enum {
    EDGE_RISE,
    EDGE_FALL,
} edge_mode_t;

typedef enum {
    LEVEL_HIGH_WIDTH, ///< 捕获高电平脉宽
    LEVEL_LOW_WIDTH,  ///< 捕获低电平脉宽
} cap_lev_mode_t;

cap_lev_mode_t cap_mode;
OS_SEM tmr_cap_sem;  ///< 捕获脉宽完成信号量。如果信号脉宽很短，要while(var)
/* static volatile u32 tmr_cap_cycle;      ///< timer超时循环次数 */
/* static volatile u32 tmr_cap_start_cnt;  ///< 捕获开始时的cnt值 */
static volatile u32 tmr_cap_end_cnt;       ///< 捕获结束时的cnt值
static volatile u8 flag_cap_start = FALSE; ///< 开始捕获标志

static void timer_set_cap_edge(edge_mode_t mode)
{
    if (mode == EDGE_RISE) {
        SFR(TMR->CON, 0, 2, 2); ///< 上升沿捕获
    } else {
        SFR(TMR->CON, 0, 2, 3); ///< 下降沿捕获
    }
}

static void timer_set_zero(void)
{
    TMR->CNT = 0;
    TMR->PRD = 0;
}

static void timer_cap_width_start(void)
{
    DBG_IO_FLIP(DBG_PORT_B, DBG_PORT_BIT_B); ///< 翻转电平

    // 修改下一个捕获边沿
    if (cap_mode == LEVEL_LOW_WIDTH) {
        timer_set_cap_edge(EDGE_RISE);
    } else {
        timer_set_cap_edge(EDGE_FALL);
    }
    timer_set_zero();

    /* tmr_cap_cycle = 0; */
    /* tmr_cap_start_cnt = TMR->PRD; */
    tmr_cap_end_cnt = 0;
}

static void timer_cap_width_end(void)
{
    DBG_IO_FLIP(DBG_PORT_C, DBG_PORT_BIT_C); ///< 翻转电平

    if (cap_mode == LEVEL_LOW_WIDTH) {
        timer_set_cap_edge(EDGE_FALL);
    } else {
        timer_set_cap_edge(EDGE_RISE);
    }
    tmr_cap_end_cnt = TMR->PRD;
    timer_set_zero();
    os_sem_post(&tmr_cap_sem);
}

static void timer_timeout_deal(void)
{
#if LOG_ENABLE
    putchar('T');
#endif

    timer_set_zero();
    /* tmr_cap_cycle ++; */
}

___interrupt
static void timer_isr(void)
{
    TMR->CON |= BIT(14);
    if (TMR->PRD) {
        // 捕获信号
        if (flag_cap_start == FALSE) {
            timer_cap_width_start();
            flag_cap_start = TRUE;
        } else {
            timer_cap_width_end();
            flag_cap_start = FALSE;
        }

    } else {
        // timer超时
        timer_timeout_deal();
    }
#if LOG_ENABLE
    putchar('%');
#endif
}

// 获取捕获的脉宽的时间，单位us
static u32 timer_get_cap_time(void)
{
    // return (u64)(tmr_cap_end_cnt - tmr_cap_start_cnt) * 1000000 / TMR_CLK;
    return (u64)tmr_cap_end_cnt * 1000000 / TMR_CLK;
}

// 获取捕获的脉宽的计数值
static u32 timer_get_cap_cnt(void)
{
    // return tmr_cap_end_cnt - tmr_cap_start_cnt;
    return tmr_cap_end_cnt;
}

// 定时器初始化
static void timer_init(cap_lev_mode_t mode)
{
    cap_mode = mode;
    os_sem_create(&tmr_cap_sem, 0);

    TMR->CON = 0;
    TMR->CNT = 0;
    TMR->PRD = 0;
    request_irq(TMR_IRQ_ID, 1, timer_isr, 0);

    /* TMR->CON |= BIT(16); ///< 使能双边沿触发 */

    // 选择TIMER时钟源:
    // 0-NULL;    1-LSB;     2-RC250K;  3-RC16M;    4-LRC_CLK;  5-STD_12M;   6-STD_24M;
    // 7-STD_48M; 8-pat_clk; 9-clkout1; 10-clkout2; 11-RTC_OSC; 12-XOSC_24M; 13-HSB_CLK
    SFR(TMR->CON, 10, 4, 5);

    // 选择TIMER分频:
    //  0-1div;        1-4div;       2-16div;       3-64div;
    //  4-2*1div;      5-2*4div;     6-2*16div;     7-2*64div;
    //  8-256*1div;    9-256*4div;  10-256*16div;  11-256*64div;
    // 12-512*1div;   13-512*4div;  14-512*16div;  15-512*64div;
    SFR(TMR->CON, 4, 4, 1); ///< STD_12M的4分频

    if (mode == LEVEL_LOW_WIDTH) {
        timer_set_cap_edge(EDGE_FALL);
        gpio_set_mode(TMR_CAPTURE_IO, GPIO_INPUT_PULL_UP_10K);
    } else {
        timer_set_cap_edge(EDGE_RISE);
        gpio_set_mode(TMR_CAPTURE_IO, GPIO_INPUT_PULL_DOWN_10K);
    }
    gpio_ich_sel_input_signal(TMR_CAPTURE_IO, TMR_INPUT_CH_CAP, INPUT_CH_TYPE_GP_ICH);
}

static void timer_uninit(void)
{
    TMR->CON = 0;
    os_sem_del(&tmr_cap_sem, 0);
}

static void timer_capture_task(void *arg)
{
    DBG_IO_INIT(DBG_PORT_A, DBG_PORT_BIT_A); ///< IO初始化
    DBG_IO_INIT(DBG_PORT_B, DBG_PORT_BIT_B); ///< IO初始化
    DBG_IO_INIT(DBG_PORT_C, DBG_PORT_BIT_C); ///< IO初始化

    timer_init(TMR_CAPTURE_LEVEL);

    while (1) {
        PRINTF_USER("\n~~~~~~~~~~~~~~~~~%s~~~~~~~~~~~~~~~~~\n", __func__);
        os_sem_pend(&tmr_cap_sem, 0);
        // TODO

        DBG_IO_FLIP(DBG_PORT_A, DBG_PORT_BIT_A); ///< 翻转电平
        // 打印耗时较长，触发捕获的周期较短时无法正常打印结果。
        // 如果触发捕获的周期较长，可以将捕获结果打印出来。
        PRINTF_USER("Capture cnt = %d\n", timer_get_cap_cnt());
        PRINTF_USER("Width time  = %dus\n", (u32)timer_get_cap_time());

        os_sem_set(&tmr_cap_sem, 0);
    }

    timer_uninit();

    while (1) {
        os_time_dly(100);
    }
}


static int c_main_timer_capture(void)
{
    os_task_create(timer_capture_task, NULL, 22, 1000, 0, "timer_capture_task");
    return 0;
}

late_initcall(c_main_timer_capture);

#endif // USE_TIMER_CAPTURE_TEST
