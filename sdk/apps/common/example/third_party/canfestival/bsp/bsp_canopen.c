#include "irq.h"
#include "drive.h"
#include "asm/can.h"
#include "asm/gpio.h"
#include "os/os_api.h"
#include "asm/clock.h"
#include "bsp_canopen.h"
#include "canopen_def.h"
#include "system/timer.h"
#include "device/device.h"

//TIMER初始化相关
#define CANOPEN_TIMER 			JL_TIMER4

static JL_TIMER_TypeDef *TMR = CANOPEN_TIMER;//选择定时器4
#define CANOPEN_TIMER_IRQ		IRQ_TIMER4_IDX//选择定时器4

static u32 timer_clk = 0;

struct canopen_timer_hdl {
    u32 ticks;
    int prd;
    u8 index;
    u8 run;
    u32 precesion;
};

static OS_SEM canopen_timer_sem;
static struct canopen_timer_hdl hdl;
#define __this	(&hdl)

//CAN初始化相关
static void *canopen_hdl = NULL;

static const u32 canopen_timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};

#define CANOPEN_TIMER_CLK			24000000
#define MAX_CANOPEN_TIME_CNT		0x7fff
#define MIN_CANOPEN_TIME_CNT		0x100

inline void canopen_time_sem_pend(void)
{
    os_sem_pend(&canopen_timer_sem, 0);
}
inline void canopen_time_sem_post(void)
{
    os_sem_post(&canopen_timer_sem);
}
void *get_canopen_hdl(void)
{
    return (void *)canopen_hdl;
}
//TIME
void canopen_timer_resume(void)
{
    if (__this->run == 0) {
        return;
    }
    u32 prd_cnt;
    u8 index;
    //precesion = PRD * index / CLK
    for (index = 0; index < (sizeof(canopen_timer_div) / sizeof(canopen_timer_div[0])); index++) {
        prd_cnt = __this->precesion * (CANOPEN_TIMER_CLK / 10000) / canopen_timer_div[index];
        if (prd_cnt > MIN_CANOPEN_TIME_CNT && prd_cnt < MAX_CANOPEN_TIME_CNT) {
            break;
        }
    }

    __this->index   = index;
    __this->prd     = prd_cnt;

    CANOPEN_TIMER->CNT = __this->ticks;
    CANOPEN_TIMER->PRD = prd_cnt; //0.1ms
    CANOPEN_TIMER->CON = (0x6 << 10) | (index << 4) | BIT(0);
}

void canopen_timer_stop(void)
{
    CANOPEN_TIMER->CON = 0;
    __this->run = 0;
    __this->precesion = 0;
}

void canopen_timer_run(void)
{
    u8 precesion;
    precesion = 10;//单位0.1ms

    CANOPEN_TIMER->CON = BIT(14);
    __this->precesion = precesion;
    __this->run 	  = 1;
    __this->ticks 	  = 0;
    canopen_timer_resume();
}

__attribute__((interrupt("")))
static void canopen_solo_timer_isr(void)
{
    //中断函数
    CANOPEN_TIMER->CON |= BIT(14);
    canopen_time_sem_post();
    /* os_sem_post(&canopen_timer_sem); */
}

void canopen_solo_timer_enable(void)
{
    os_sem_create(&canopen_timer_sem, 0);
    canopen_timer_stop();
    request_irq(CANOPEN_TIMER_IRQ, 1, canopen_solo_timer_isr, 0);
    canopen_timer_run();
}

static void canopen_sys_timer_isr(void *offset)
{
    canopen_time_sem_post();
}

void canopen_sys_timer_enable(void)
{
    os_sem_create(&canopen_timer_sem, 0);
    sys_hi_timer_add(NULL, canopen_sys_timer_isr, 10);
}

//CAN
//CAN接收中断
///没用上
__attribute__((interrupt("")))
void canopen_can_isr(void)
{
    dev_ioctl(canopen_hdl, IOCTL_CAN_GET_INTERRUPT_SOURCE, 0);
    dev_ioctl(canopen_hdl, IOCTL_CAN_SET_INTERRUPT_DISABLE, CAN_EVENT_CLEAR_INTERRUPT);
    /* dev_ioctl(canopen_hdl, IOCTL_CAN_SET_INTERRUPT_DISABLE, CAN_EVENT_ALL_INTERRUPT_DISABLE); */
    printf(">>>%s:  can interrupt occured.", __func__);

}

void canopen_can_init(void)
{
    can_mode_t can_mode;
    canopen_hdl = dev_open("can", NULL);
    can_mode = CAN_MODE_ENHANCED;
    dev_ioctl(canopen_hdl, IOCTL_CAN_SET_DMA_FRAMES, 1);
    dev_ioctl(canopen_hdl, IOCTL_CAN_SET_RECV_WAIT_SEM, 0);
    dev_ioctl(canopen_hdl, IOCTL_CAN_SET_MODE, &can_mode);

//使能中断
    /* request_irq(IRQ_CAN_IDX, 6, canopen_can_isr, 1); */
    /* dev_ioctl(canopen_hdl, IOCTL_CAN_INTERRUPT_ENABLE, RECEIVE_INTERRUPT); */
}



