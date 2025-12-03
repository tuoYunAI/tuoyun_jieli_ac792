#include "drive.h"
#include "timerscfg.h"
#include "applicfg.h"
#include "can.h"
#include "asm/can.h"
#include "os/os_api.h"
#include "device/device.h"
#include "bsp_canopen.h"
#include "canopen_def.h"
#include "timers.h"

#ifdef CANOPEN_MASTER_ENABLE
#include "TestMaster.h"
#endif

#ifdef CANOPEN_SLAVE_ENABLE
#include "TestSlave.h"
#endif

static unsigned int TimeCNT = 0;
static unsigned int NextTime = 0;
static unsigned int TIMER_MAX_COUNT = 70000;
static TIMEVAL last_time_set = TIMEVAL_MAX;

static void canopen_receive_task(void *offset)
{
    static Message m;
    void *canopen_hdl = get_canopen_hdl();
    can_data_t can_rxbuf;
    memset(&can_rxbuf, 0x0, sizeof(can_rxbuf));
    while (1) {
        if (dev_read(canopen_hdl, &can_rxbuf, 1) < 0) {
            printf("(error) %s:[CANOPEN]-->CANOPEN receive error!!!", __func__);
        }
        m.cob_id = can_rxbuf.id;
        m.rtr = can_rxbuf.rtr;
        m.len = can_rxbuf.dlc;
        memcpy(m.data, can_rxbuf.data, m.len);
#ifdef CANOPEN_MASTER_ENABLE
        canDispatch(&TestMaster_Data, &m);
#endif

#ifdef CANOPEN_SLAVE_ENABLE
        canDispatch(&TestSlave_Data, &m);
#endif
    }
}

//定时中断信号量pend
static void canopen_timer_dispatch(void *offset)
{
    while (1) {
        canopen_time_sem_pend();
        TimeCNT++;
        if (TimeCNT >= TIMER_MAX_COUNT) {
            TimeCNT = 0;
        }
        if (TimeCNT == NextTime) {
            last_time_set = TimeCNT;
            TimeDispatch();
        }
    }
}

void canopen_driver_init(void)
{
    if (thread_fork("canopen_receive_task", 5, 1024, 0, 0, canopen_receive_task, NULL) != 0) {
        printf("canopen_receive_task creat failed");
        return;
    }
    if (thread_fork("canopen_timer_dispatch", 9, 1024, 0, 0, canopen_timer_dispatch, NULL) != 0) {
        printf("canopen_timer_dispatch creat failed");
        return;
    }
}

/// canfestival的can发送函数 不建议修改
unsigned char canSend(CAN_PORT notused, Message *m)
{
    can_data_t can_txdata;
    void *canopen_hdl = get_canopen_hdl();
    memset(&can_txdata, 0x0, sizeof(can_txdata));

    can_txdata.data_format 	= CAN_FRAME_STANDARD_FORMAT;
    can_txdata.dlc  		= m->len;
    can_txdata.rtr			= m->rtr;
    can_txdata.id			= m->cob_id;
    memcpy(can_txdata.data, m->data, m->len);
    if (dev_write(canopen_hdl, &can_txdata, 1) < 0) {
        printf("(error) %s:[CANOPEN]-->CANOPEN write error!!!", __func__);
        return 0xFF;
    } else {
        return 0;
    }

}

///设置下一次报警的计时器
void setTimer(TIMEVAL value)
{
    NextTime = (TimeCNT + value) % TIMER_MAX_COUNT;
}

///返回运行时间，告诉堆栈自上次调用以来花费了多少时间
TIMEVAL getElapsedTime(void)
{
    int ret = 0;
    ret = (TimeCNT > last_time_set) ? (TimeCNT - last_time_set) : (TimeCNT + TIMER_MAX_COUNT - last_time_set);
    return ret;
}

