#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"

#ifdef USE_CANOPEN_TEST_DEMO
#include "canopen_def.h"
#include "bsp_canopen.h"
#include "drive/drive.h"
#include "canfestival.h"

#ifdef CANOPEN_MASTER_ENABLE
#include "TestMaster.h"
#endif

#ifdef CANOPEN_SLAVE_ENABLE
#include "TestSlave.h"
#endif

static void canopen_main_task(void *p)
{
    canopen_can_init();//can硬件初始化
#ifdef CANOPEN_TIMER_10MS_ENABLE
    canopen_sys_timer_enable();//系统定时器初始化
#else
    canopen_solo_timer_enable();//独立定时器初始化
#endif
    canopen_driver_init(); //can接收任务与定时中断调用任务创建

#ifdef CANOPEN_MASTER_ENABLE
    unsigned char nodeID = 0x01;
    setNodeId(&TestMaster_Data, nodeID);//设置canfestival主机ID
    setState(&TestMaster_Data, Initialisation);//初始化canopen NMT网络主机设备
    setState(&TestMaster_Data, Operational);//进入操作状态
#endif

#ifdef CANOPEN_SLAVE_ENABLE
    unsigned char nodeID = 0x03;
    setNodeId(&TestSlave_Data, nodeID);//设置canfestival从机ID
    setState(&TestSlave_Data, Initialisation);//初始化canopen NMT网络从机设备
    setState(&TestSlave_Data, Operational);//进入操作状态
#endif

    while (1) {
        /*应用层，可在此处添加canopen协议条件发送报文。SDO或PDO*/
        os_time_dly(50);
    }
}

static int canopen_task_init(void)
{
    printf("------------canfestival running------------\n");
    return thread_fork("canopen_main_task", 5, 1024, 0, 0, canopen_main_task, NULL);
}

late_initcall(canopen_task_init);

#endif //USE_CANOPEN_TEST_DEMO

