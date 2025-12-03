/****************************************************************************
*  Copyright 2022 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
*                                                                           *
*  Licensed under the Apache License, Version 2.0 (the "License");          *
*  you may not use this file except in compliance with the License.         *
*  You may obtain a copy of the License at                                  *
*                                                                           *
*     http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                           *
*  Unless required by applicable law or agreed to in writing, software      *
*  distributed under the License is distributed on an "AS IS" BASIS,        *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
*  See the License for the specific language governing permissions and      *
*  limitations under the License.                                           *
*                                                                           *
****************************************************************************/

/*============================ INCLUDES ======================================*/
#undef __PERF_COUNT_PLATFORM_SPECIFIC_HEADER__
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define BOOL_DEFINE_CONFLICT
#include "app_config.h"
#include "system/includes.h"
#include "asm/clock.h"
#include "spinlock.h"

//static spinlock_t perfc_lock;
#define PERF_COUNTER_LOCK()     //spin_lock(&perfc_lock)
#define PERF_COUNTER_UNLOCK()   //spin_unlock(&perfc_lock)


#define __IMPLEMENT_PERF_COUNTER
#include "perf_counter.h"

#if 1 //沿用操作系统ticktimer
#define TIMER_USE_ISR_SOLVE_OVERFLOW_PROBLEM 1 // 用中断解决溢出问题

#define PERF_COUNTER_TIMER_CON q32DSP(0)->TTMR_CON
#define PERF_COUNTER_TIMER_CNT q32DSP(0)->TTMR_CNT
#define PERF_COUNTER_TIMER_PRD q32DSP(0)->TTMR_PRD
#define g_overflow_cnt jiffies

uint32_t SystemCoreClock;
void perf_counter_init(void)
{
    SystemCoreClock = clk_get("sys");
    update_perf_counter();
}

void perf_counter_update(u32 sys_clk)
{
    SystemCoreClock = sys_clk;
    update_perf_counter();
}

#else //占据一个硬件TIMER

#define TIMER_USE_ISR_SOLVE_OVERFLOW_PROBLEM 1 // 1:用中断解决溢出问题, 0:查pending再使用jiffies纠正

#define PERF_COUNTER_TIMER_CON		 JL_TIMER5->CON
#define PERF_COUNTER_TIMER_CNT		 JL_TIMER5->CNT
#define PERF_COUNTER_TIMER_FLOW_PRD	 JL_TIMER5->PRD
#define PERF_COUNTER_TIMER_ISR_IDX	 IRQ_TIMER5_IDX
#define PERF_COUNTER_TIMER_PRD 0xffffffff

#define PERF_COUNTER_TIMER_PENDSTSET_Msk BIT(15)
#define PERF_COUNTER_TIMER_PENDSTSET_Msk_Clr BIT(14)
//修改TIMER频率，需要同时修改 PERF_CNT_DELAY_US_COMPENSATION, 精度小于1us才可以调用perfc_delay_us
#define TIMER_CLK 24000000
/*#define TIMER_CLK 6000000 */
/*#define TIMER_CLK 750000*/
/*#define TIMER_CLK 93750*/
uint32_t SystemCoreClock = TIMER_CLK;

volatile uint32_t g_overflow_cnt;
#if TIMER_USE_ISR_SOLVE_OVERFLOW_PROBLEM
___interrupt
static void perf_counter_timer_isr(void)
{
    PERF_COUNTER_TIMER_CON |= BIT(14);
    ++g_overflow_cnt;
}
#endif
static int perf_counter_init(void)
{
#if TIMER_USE_ISR_SOLVE_OVERFLOW_PROBLEM
    request_irq(PERF_COUNTER_TIMER_ISR_IDX, 0, perf_counter_timer_isr, 0);
#endif

    PERF_COUNTER_TIMER_CON = BIT(14) | (6 << 10) ;
    PERF_COUNTER_TIMER_FLOW_PRD = -1;

#if TIMER_CLK == 24000000 //精度 1/24=0.0416us,2^32/1000/1000/60/ 24=2.98分钟溢出
#elif TIMER_CLK == 6000000 //精度 4/24=0.16us,2^32/1000/1000/60/ 6=11.9分钟溢出
    PERF_COUNTER_TIMER_CON |= BIT(4);
#elif TIMER_CLK == 750000 //精度 32/24=1.33us,2^32/1000/1000/60/ 0.75=95.4分钟溢出
    PERF_COUNTER_TIMER_CON |= BIT(5) | BIT(6);
#elif TIMER_CLK == 93750 //精度 256/24=10.6us,2^32/1000/1000/60/60 0.09375=31.8小时溢出
    PERF_COUNTER_TIMER_CON |= BIT(7);
#endif

    PERF_COUNTER_TIMER_CON |= BIT(0);
    PERF_COUNTER_TIMER_CNT = 0;

    update_perf_counter();

    return 0;
}
__initcall(perf_counter_init);
#endif





/*============================ MACROS ========================================*/

#ifndef PERF_CNT_COMPENSATION_THRESHOLD
#define PERF_CNT_COMPENSATION_THRESHOLD          16
#endif


#ifndef PERF_CNT_DELAY_US_COMPENSATION
// delay补偿值根据SYS_CLK系统时钟范围选择
//        SYS_CLK >= 360M --------  120
// 360M > SYS_CLK >= 200M --------  130
// 200M > SYS_CLK         --------  140
#if TCFG_SYS_CLK < 200000000
#define PERF_CNT_DELAY_US_COMPENSATION           140
#elif TCFG_SYS_CLK < 360000000
#define PERF_CNT_DELAY_US_COMPENSATION           130
#else
#define PERF_CNT_DELAY_US_COMPENSATION           120
#endif
#endif // PERF_CNT_DELAY_US_COMPENSATION


/*============================ GLOBAL VARIABLES ==============================*/

/*============================ LOCAL VARIABLES ===============================*/
static int64_t g_lLastTimeStamp;
static uint32_t s_lOldTimeCnt, s_lOldOverFlowCnt;
static int32_t s_nUSUnit;
static int32_t s_nMSUnit = 0x7fffffff;


/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/
/*============================ INCLUDES ======================================*/


void update_perf_counter(void)
{
    s_nUSUnit = SystemCoreClock / 1000000ul;
    s_nMSUnit = SystemCoreClock / 1000ul;

    g_lLastTimeStamp = get_system_ticks();

    /* __perf_counter_printf__("update_perf_counter  SystemCoreClock=%dMhz,PERF_CNT_DELAY_US_COMPENSATION=%d", SystemCoreClock / 1000000, PERF_CNT_DELAY_US_COMPENSATION); */

}


/*! \note this function should only be called when irq is disabled
 *        hence SysTick-LOAD and (SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)
 *        won't change.
 */

static __inline int64_t check_systick(void)
{
    int64_t lTemp;

    PERF_COUNTER_LOCK();
    int64_t lOldTimestamp = s_lOldTimeCnt + (int64_t)(s_lOldOverFlowCnt * (int64_t)PERF_COUNTER_TIMER_PRD);
    PERF_COUNTER_UNLOCK();

    uint32_t nTemp = PERF_COUNTER_TIMER_CNT;
    uint32_t nTempOverflowCnt = g_overflow_cnt;


    /*        Since we cannot stop counting temporarily, there are several
     *        conditions which we should take into consideration:
     *        - Condition 1: when assigning nTemp with the register value (LOAD-VAL),
     *            the underflow didn't happen but when we check the PENDSTSET bit,
     *            the underflow happens, for this condition, we should not
     *            do any compensation. When this happens, the (LOAD-nTemp) is
     *            smaller than PERF_CNT_COMPENSATION_THRESHOLD (a small value) as
     *            long as LOAD is bigger than (or equals to) the
     *            PERF_CNT_COMPENSATION_THRESHOLD;
     *        - Condition 2: when assigning nTemp with the register value (LOAD-VAL),
     *            the VAL is zero and underflow happened and the PENDSTSET bit
     *            is set, for this condition, we should not do any compensation.
     *            When this happens, the (LOAD-nTemp) is equals to zero.
     *        - Condition 3: when assigning nTemp with the register value (LOAD-VAL),
     *            the underflow has already happened, hence the PENDSTSET
     *            is set, for this condition, we should compensate the return
     *            value. When this happens, the (LOAD-nTemp) is bigger than (or
     *            equals to) PERF_CNT_COMPENSATION_THRESHOLD.
     *        The following code implements an equivalent logic.
     */
#if !TIMER_USE_ISR_SOLVE_OVERFLOW_PROBLEM
    if (PERF_COUNTER_TIMER_CON & PERF_COUNTER_TIMER_PENDSTSET_Msk) {
        PERF_COUNTER_TIMER_CON |= PERF_COUNTER_TIMER_PENDSTSET_Msk_Clr;

        nTemp = PERF_COUNTER_TIMER_CNT; //还是有概率会造成1次溢出周期的精度的误差

        uint32_t overflow_timer = timer_get_sec() / (PERF_COUNTER_TIMER_PRD / SystemCoreClock);
        if (overflow_timer == 0) {
            overflow_timer = 1;
        }

        g_overflow_cnt += overflow_timer;

        /*if (((int32_t)SysTick->LOAD - nTemp) >= PERF_CNT_COMPENSATION_THRESHOLD) {*/
        /*nTemp += SysTick->LOAD + 1;*/
        /*}*/
    }
#endif

    /* When calling get_system_ticks() in an exception handler that has a
     * higher priority than the SysTick_Handler, in some rare cases, the
     * lTemp might be temporarily smaller than the previous value (i.e.
     * s_lOldTimestamp), to mitigate the adverse effects of this problem,
     * we use the following code to avoid time-rolling-back issue.
     *
     * NOTE: the issue mentioned above doesn't accumulate or have long-lasting
     *       effects.
     */

    lTemp = nTemp + (int64_t)(nTempOverflowCnt * (int64_t)PERF_COUNTER_TIMER_PRD);
    if (lTemp < lOldTimestamp) {
        /*
        	timer计数回滚了，但是还没来得及进中断更新g_overflow_cnt(尤其多核情况会出现), 所以小于上次计数, 属于正常情况,
        	另外一种情况是: timer计数回滚了,并且 TIMER被关闭计数不进中断导致jiffies不更新也会导致一直进来
         */

        /*__perf_counter_printf__("[perf_counter] time-rolling-back happend!!! %lld, %lld, %d, %d\r\n",lOldTimestamp,lTemp,nTemp,nTempOverflowCnt);                 */
        lTemp += PERF_COUNTER_TIMER_PRD;//补偿回滚导致的损失

        /*
        如果这一时刻开始被切走cpu，导致在另外一处地方更新了一个更大的s_lOldOverFlowCnt, 这时候返回的值就是比lOldTimestamp偏小错误的，
        但是没办法避免，即使把整个函数互斥起来，也只能够保证这个函数不被切走cpu，互斥一打开就有可能切走，
        因此不会在这里做额外的处理,下次再进来s_lOldOverFlowCnt会得到修复
        */
    } else {
        /*注意这里 不互斥的情形下，  另外一个线程会重入造成非原子操作，即使分开两个U32变量，仅是为了不造成错误，避免不了改写顺序问题，
        一种特殊的情形是，在这一个时刻，CPU切走，s_lOldOverFlowCnt被更新了一个更大的值,等CPU再回来后,两个全局U32变量中的一个或者两个被覆盖了旧的较小的值,
        但是在下一轮进来会纠正，并且不影响返回结果
        */
        PERF_COUNTER_LOCK();
        s_lOldOverFlowCnt = nTempOverflowCnt;
        s_lOldTimeCnt = nTemp;
        PERF_COUNTER_UNLOCK();
    }

    return lTemp;
}


__attribute__((always_inline))
static void delay_us_use_nop(unsigned int us)
{
    extern int sys_clk_get(void);
    if (JL_SFCTZ->CON & BIT(0)) {
        us *= sys_clk_get() / 1000000 / 8;
    } else {
        us *= sys_clk_get() / 1000000 / 4;
    }

    while (us--) {
        asm volatile("nop");
    }
}

__attribute__((always_inline))
void delay_us_in_irq(uint32_t nUs)
{
    delay_us_use_nop(nUs);
}

void delay_us(uint32_t nUs)
{
    if (cpu_irq_disabled()) {
        delay_us_in_irq(nUs);
        return;
    }

    int64_t lUs = (int64_t)nUs * (int64_t)s_nUSUnit;

    if (lUs <= PERF_CNT_DELAY_US_COMPENSATION) {
        ASSERT(s_nUSUnit);//perf_counter 还未初始化就调用
        return;
    }

    lUs -= PERF_CNT_DELAY_US_COMPENSATION;

    lUs += get_system_ticks();
    while (get_system_ticks() < lUs);
}

void delay_ms(uint32_t nMs)
{
    if (cpu_irq_disabled()) {
        delay_us_use_nop(nMs * 1000);
        return;
    }

    int64_t lUs = (int64_t)nMs * (int64_t)s_nMSUnit;

    if (lUs <= PERF_CNT_DELAY_US_COMPENSATION) {
        ASSERT(s_nMSUnit);//perf_counter 还未初始化就调用
        return ;
    }

    lUs -= PERF_CNT_DELAY_US_COMPENSATION;

    lUs += get_system_ticks();
    while (get_system_ticks() < lUs);
}

__attribute__((noinline))
int64_t get_system_ticks(void)
{
    return check_systick();
}

/*! \note the prototype of this clock() is different from the one defined in
 *!           time.h. As clock_t is usually defined as unsigned int, it is
 *!           not big enough in Cortex-M system to hold a time-stamp. clock()
 *!           defined here returns the timestamp since the begining of main()
 *!           and its unit is clock cycle (rather than 1ms). Hence, for a system
 *!           running under several hundreds MHz or even 1GHz, e.g. RT10xx from
 *!           NXP, it is very easy to see a counter overflow as clock_t is
 *!           defined as uint32_t in timer.h.
 *!           Since we are not allowed to change the defintion of clock_t in
 *!           official header file, i.e. time.h, I use a compatible prototype
 *!           after I checked the AAPCS spec. So, the return of the clock() is
 *!           int64_t, which will use the R0 to store the lower 32bits and R1
 *!           to store the higher 32bits. When you are using the prototype from
 *!           timer.h, caller will only take the lower 32bits stored in R0 and
 *!           the higher 32bits stored in R1 will be ignored.
 *!
 *!           If you want to use the non-overflow version of this clock(), please
 *!           1) define the MACRO: __PERF_CNT_USE_LONG_CLOCK__ in your project
 *!           and 2) do not include system header file <time.h>
 *!
 */
__attribute__((nothrow))
int64_t clock(void)
{
    return get_system_ticks();
}

uint32_t get_system_ms_in_irq(void)
{
    return (check_systick()) / s_nMSUnit;
}

uint32_t get_system_ms(void)
{
    /* return (check_systick()) / s_nMSUnit; */
    return jiffies_msec();
}

uint32_t timer_get_ms(void)
{
    return get_system_ms();
}

uint32_t timer_get_sec(void)
{
    return timer_get_ms() / 1000;
}

uint32_t get_system_us(void)
{
    /* return (check_systick()) / s_nUSUnit; */
    return jiffies_usec();
}

int64_t perfc_convert_ticks_to_ms(int64_t lTick)
{
    return lTick / (int64_t)s_nMSUnit;
}

int64_t perfc_convert_ms_to_ticks(uint32_t wMS)
{
    int64_t lResult = (int64_t)s_nMSUnit * (int64_t)wMS;
    return lResult ? lResult : 1;
}

int64_t perfc_convert_ticks_to_us(int64_t lTick)
{
    return lTick / (int64_t)s_nUSUnit;
}

int64_t perfc_convert_us_to_ticks(uint32_t wMS)
{
    int64_t lResult = (int64_t)s_nUSUnit * (int64_t)wMS;
    return lResult ? lResult : 1;
}

bool __perfc_is_time_out(int64_t lPeriod, int64_t *plTimestamp, bool bAutoReload)
{
    if (NULL == plTimestamp) {
        return false;
    }

    int64_t lTimestamp = get_system_ticks();

    if (0 == *plTimestamp) {
        *plTimestamp = lPeriod;
        *plTimestamp += lTimestamp;

        return false;
    }

    if (lTimestamp >= *plTimestamp) {
        if (bAutoReload) {
            *plTimestamp = lPeriod + lTimestamp;
        }
        return true;
    }

    return false;
}

unsigned int time_lapse(unsigned int *handle, unsigned int time_out)//2^32/1000/60/60/24 后超时
{
    unsigned int t, t2;

    if (*handle == 0) {
        *handle = timer_get_ms();
        return 0;
    }

    t = timer_get_ms();
    t2 = t - *handle;

    if (t2 > time_out) {
        *handle = t;
        return t2;
    }

    return 0;
}

__attribute__((always_inline))
void delay(unsigned int ncycle)
{
    while (ncycle--) {
        asm volatile("nop");
    }
}

__attribute__((always_inline))
void delay_nops(unsigned int ncycle)
{
    while (ncycle--) {
        asm volatile("nop");
    }
}

