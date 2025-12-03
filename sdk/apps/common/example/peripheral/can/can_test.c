#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/can.h"
#include "asm/gpio.h"
#include "irq.h"


#ifdef	USE_CAN_TEST_DEMO

#define CAN_REINIT_MODIFY_PARAMETERS	///< 演示demo-代码运行后修改can的参数
#define CAN_RECV_BLOCK_ENABLE			///< 演示can阻塞式接收

#define CAN_RX_CNT	5

static void *can_hdl = NULL;

// 定义中断回调函数，非阻塞式接收数据完成会回调该函数
// 回调的can_rx_data指针实际上与IOCTL_CAN_SET_RECV_NON_BLOCK_ENABLE传递的指针是一致的
static int can_irq_cb(void *priv, can_data_t *can_rx_data)
{
    printf("\n\n--------can non block recv run---------\n\n");
    printf("data_format is --%d", can_rx_data->data_format);
    printf("rtr is --%d", can_rx_data->rtr);
    printf("id is --0x%x", can_rx_data->id);
    printf("dlc is --%d", can_rx_data->dlc);
    for (int i = 0; i < can_rx_data->dlc; i++) {
        printf("0x%x", can_rx_data->data[i]);
    }
    return 0;
}

static void can_test_task(void *arg)
{
#ifdef CAN_REINIT_MODIFY_PARAMETERS
    can_mode_t		can_mode;
    can_baudrate_t  reset_rate;
    can_rx_filter_t	reset_filter;
#endif
    can_data_t can_rx_data[CAN_RX_CNT];
    can_data_t can_tx_data[4];
    memset(&can_tx_data, 0x0, sizeof(can_tx_data));
    memset(&can_rx_data, 0x0, sizeof(can_rx_data));
    can_hdl = dev_open("can", NULL);
    if (!can_hdl) {
        r_printf("can open err");
        goto exit;
    }
#ifdef CAN_RECV_BLOCK_ENABLE
    dev_ioctl(can_hdl, IOCTL_CAN_SET_DMA_FRAMES, CAN_RX_CNT);
    /* dev_ioctl(can_hdl, IOCTL_CAN_SET_RECV_WAIT_WHILE, 0); */
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RECV_WAIT_SEM, 0);
#else
    can_cb_t cb;
    cb.cb_func = can_irq_cb;
    cb.cb_priv = NULL;
    dev_ioctl(can_hdl, IOCTL_CAN_SET_IRQ_CB, (u32)&cb);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RECV_NON_BLOCK_ENABLE, (u32)&can_rx_data);
#endif

#if (TCFG_CAN_MODE == 0) //basic can
    can_tx_data[0].id   = 0x233;
    can_tx_data[0].dlc  = 8;
    can_tx_data[0].rtr  = CAN_REQUEST_DATA_TYPE;//数据帧
    // 发送数据
    can_tx_data[0].data[0]  = 0x11;
    can_tx_data[0].data[1]  = 0x11;
    can_tx_data[0].data[2]  = 0x33;
    can_tx_data[0].data[3]  = 0x33;
    can_tx_data[0].data[4]  = 0x55;
    can_tx_data[0].data[5]  = 0x55;
    can_tx_data[0].data[6]  = 0x77;
    can_tx_data[0].data[7]  = 0x77;

    // 数据帧发送
    can_tx_data[1].id  = 0x159;
    can_tx_data[1].dlc  = 8;
    can_tx_data[1].rtr  = CAN_REQUEST_DATA_TYPE;//数据帧
    // 发送数据
    can_tx_data[1].data[0]  = 0x21;
    can_tx_data[1].data[1]  = 0x31;
    can_tx_data[1].data[2]  = 0x43;
    can_tx_data[1].data[3]  = 0x53;
    can_tx_data[1].data[4]  = 0x65;
    can_tx_data[1].data[5]  = 0x75;
    can_tx_data[1].data[6]  = 0x87;
    can_tx_data[1].data[7]  = 0x97;

    // 远程帧发送
    can_tx_data[2].id  = 0x159;
    can_tx_data[2].dlc  = 8;
    can_tx_data[2].rtr  = CAN_REQUEST_REMOTE_TYPE;//远程帧

    dev_write(can_hdl, &can_tx_data, 3);

#endif

#if (TCFG_CAN_MODE == 1) //enhanced can
    // 测试发送标准数据帧
    can_tx_data[0].dlc 		 	= 8;
    can_tx_data[0].rtr 		 	= CAN_REQUEST_DATA_TYPE;
    can_tx_data[0].data_format  = CAN_FRAME_STANDARD_FORMAT;
    can_tx_data[0].id  		 	= 0x233;

    can_tx_data[0].data[0]  =  0x18;
    can_tx_data[0].data[1]  =  0x27;
    can_tx_data[0].data[2]  =  0x36;
    can_tx_data[0].data[3]  =  0x45;
    can_tx_data[0].data[4]  =  0x54;
    can_tx_data[0].data[5]  =  0x63;
    can_tx_data[0].data[6]  =  0x72;
    can_tx_data[0].data[7]  =  0x81;

    // 测试发送标准远程帧
    can_tx_data[1].dlc			 = 2;
    can_tx_data[1].rtr			 = CAN_REQUEST_REMOTE_TYPE;
    can_tx_data[1].data_format   = CAN_FRAME_STANDARD_FORMAT;
    can_tx_data[1].id			 = 0x456;

    // 测试发送扩展数据帧
    can_tx_data[2].dlc			 = 8;
    can_tx_data[2].rtr			 = CAN_REQUEST_DATA_TYPE;
    can_tx_data[2].data_format   = CAN_FRAME_EXTENDED_FORMAT;
    can_tx_data[2].id			 = 0x123456;

    can_tx_data[2].data[0]  =  0x11;
    can_tx_data[2].data[1]  =  0x22;
    can_tx_data[2].data[2]  =  0x33;
    can_tx_data[2].data[3]  =  0x44;
    can_tx_data[2].data[4]  =  0x55;
    can_tx_data[2].data[5]  =  0x66;
    can_tx_data[2].data[6]  =  0x77;
    can_tx_data[2].data[7]  =  0x88;

    // 测试发送扩展远程帧
    can_tx_data[3].dlc			 = 2;
    can_tx_data[3].rtr			 = CAN_REQUEST_REMOTE_TYPE;
    can_tx_data[3].data_format	 = CAN_FRAME_EXTENDED_FORMAT;
    can_tx_data[3].id			 = 0x12345678;

    dev_write(can_hdl, &can_tx_data, 4);

#endif

    /* dev_ioctl(can_hdl, IOCTL_CAN_INTERRUPT_ENABLE, RECEIVE_INTERRUPT); */
    while (1) {

#ifdef CAN_RECV_BLOCK_ENABLE
        os_time_dly(1);
        ///接收数据
        printf("\n\n--------can run---------\n\n");
        dev_read(can_hdl, &can_rx_data, CAN_RX_CNT);

        for (int cnt = 0; cnt < CAN_RX_CNT; cnt++) {
            printf("The CAN RX data-[%d]", cnt);
            printf("data_format is [%d]-%d", cnt, can_rx_data[cnt].data_format);
            printf("rtr is [%d]-%d", cnt, can_rx_data[cnt].rtr);
            printf("id is [%d]-0x%x", cnt, can_rx_data[cnt].id);
            printf("dlc is [%d]-%d", cnt, can_rx_data[cnt].dlc);
            for (int i = 0; i < can_rx_data[cnt].dlc; i++) {
                printf("[%d]-0x%x", cnt, can_rx_data[cnt].data[i]);
            }
        }
#else
        os_time_dly(100);
        dev_write(can_hdl, &can_tx_data[1], 2);
        printf("waiting recv...");
#endif

#ifdef CAN_REINIT_MODIFY_PARAMETERS
        break;
#endif
    }

#ifdef CAN_REINIT_MODIFY_PARAMETERS
    can_mode = CAN_MODE_BASICCAN;

    reset_rate.brp 			= 39;
    reset_rate.sjw 			= TCFG_CAN_BAUD_SJW;
    reset_rate.sam 			= TCFG_CAN_BAUD_SAM;
    reset_rate.tseg1 		= TCFG_CAN_BAUD_PTS_PBS1;
    reset_rate.tseg2 		= TCFG_CAN_BAUD_PBS2;

    reset_filter.accept_id0 = 0x123;
    reset_filter.mask_id0	= 0x00;
    // 演示修改can参数-波特率从1Mhz修改为100khz；模式改为基础can；滤波器修改
    dev_ioctl(can_hdl, IOCTL_CAN_SET_MODE, (u32)&can_mode);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_FILTER, (u32)&reset_filter);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RATE, (u32)&reset_rate);
    while (1) {

#ifdef CAN_RECV_BLOCK_ENABLE
        os_time_dly(1);
        // 接收数据
        printf("\n\n--------modify_parameters_can run---------\n\n");
        dev_read(can_hdl, &can_rx_data, CAN_RX_CNT);
        for (int cnt = 0; cnt < CAN_RX_CNT; cnt++) {
            printf("The CAN RX data-[%d]", cnt);
            printf("data_format is [%d]-%d", cnt, can_rx_data[cnt].data_format);
            printf("rtr is [%d]-%d", cnt, can_rx_data[cnt].rtr);
            printf("id is [%d]-0x%x", cnt, can_rx_data[cnt].id);
            printf("dlc is [%d]-%d", cnt, can_rx_data[cnt].dlc);
            for (int i = 0; i < can_rx_data[cnt].dlc; i++) {
                printf("[%d]-0x%x", cnt, can_rx_data[cnt].data[i]);
            }
        }
#else
        os_time_dly(100);
        dev_write(can_hdl, &can_tx_data[1], 2);
        printf("waiting recv...");
#endif

    }
#endif
exit:
    if (can_hdl) {
        dev_close(can_hdl);
    }
    while (1) {
        os_time_dly(100);
    }
}

__attribute__((interrupt("")))
static void can_isr()
{
    u32 arg;
    r_printf(">>>%s:  can interrupt occure.", __func__);
    dev_ioctl(can_hdl, IOCTL_CAN_GET_INTERRUPT_SOURCE, (u32)&arg);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_INTERRUPT_DISABLE, CAN_EVENT_CLEAR_INTERRUPT);
    /* dev_ioctl(can_hdl, IOCTL_CAN_SET_INTERRUPT_DISABLE, CAN_EVENT_ALL_INTERRUPT_DISABLE); */
    printf("arg = %x", arg);
}

static int c_main_can(void)
{
    /* request_irq(IRQ_CAN_IDX, 5, can_isr, 1); */
    printf("\n\n-----------------CAN_TEST_DEMO run %s---------------- \n\n", __TIME__);
    os_task_create(can_test_task, NULL, 10, 1000, 0, "can_test_task");
    return 0;
}

late_initcall(c_main_can);

#endif // USE_CAN_TEST_DEMO


