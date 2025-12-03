/**@file        uart_device_test.c
  * @brief      uart测试例程
  * @details    uart测试例程，程序使用spec_uart.c驱动程序
  * @author     jieli
  * @date       2024-01-09
  * @version    V1.0
  */


/********************************测试例程说明************************************
 *	本测试例程分两个demo程序，通过定义DEMO_0和DEMO_1来打开对应的程序。
 * 	DEMO_0：测试UART收发数据功能，以及使用IOCTL命令对UART进行一些配置操作。
 * 	        程序打开UART串口并配置完成后循环读取RX数据，读到数据后将的数据
 * 	        返回给发送方，并统计接收到的字节数。
 *
 * 	DEMO_1：测试CTS和RTS功能。通过定义TEST_CTS和TEST_RTS来打开对应程序。
 *          TEST_CTS：循环发送数据，当CTS被串口工具拉低时，UART会停止发送。
 *          TEST_RTS：程序逻辑与DEMO_0基本一致，不同的是程序增加了RTS_SUSPEND
 *          		  RTS_RESUME控制RTS，RESUME 2秒，SUSPEND 5秒，不断循环，
 *          		  可通过逻辑分析仪观察RTS脚idle/busy状态下，串口工具的数
 *          		  据发送情况。
 ********************************************************************************/

#include "system/includes.h"
#include "app_config.h"
#include "uart.h"

#ifdef USE_UART_DEVICE_TEST_DEMO

#define DEMO_0
/* #define DEMO_1 */


/* 将下面的UART模块配置放到板级文件中,并注册 */
/*
UART2_PLATFORM_DATA_BEGIN(uart2_data)
    .flow_ctl_enable        = 1,
    .rts_hw_enable          = 0,///<软件RTS
    .baudrate               = 1000000,
    .tx_pin                 = IO_PORTA_06,
    .rx_pin                 = IO_PORTA_07,
    .rts_pin                = IO_PORTA_05,
    .cts_pin                = IO_PORTA_04,
    .rts_idle_level         = 0,
    .cts_idle_level         = 0,
    .rx_thresh              = 80,
    .max_continue_recv_cnt  = 1024,
	.tx_pin_hd_level		= GPIO_DRIVE_STRENGTH_8p0mA,
    .idle_sys_clk_cnt       = 500000,
    .clk_src                = STD_24M,
    .parity                 = UART_PARITY_DISABLE,
UART2_PLATFORM_DATA_END();

REGISTER_DEVICES(device_table) = {
    {"uart2", &uart_dev_ops, (void *)&uart2_data },
};
*/


/* +++++++++++++++++++++++++++++++++++++++ DEMO_0 start +++++++++++++++++++++++++++++++++++++++ */
#ifdef DEMO_0

#define BUF_SIZE 512
static u8 recv_buf[BUF_SIZE];
static int send_buf[BUF_SIZE];
static u8 buf[1 * 1024] __attribute__((aligned(4))); ///<用于串口接收缓存数据的循环buf


/* 串口中断事件回调函数，该函数在中断中调用，程序执行时间应尽量短 */
static void uart_irq_cb(uart_irq_event_t event)
{
    switch (event) {
    case UART_IRQ_EVENT_TX:
        printf("I am UART_IRQ_EVENT_TX\n");
        break;
    case UART_IRQ_EVENT_RX:
        printf("I am UART_IRQ_EVENT_RX\n");
        break;
    case UART_IRQ_EVENT_OT:
        printf("I am UART_IRQ_EVENT_OT\n");
        break;
    default:
        printf("I am %d\n", event);
        break;
    }
}


/* 串口RX接收超时回调函数 */
static int read_timeout_cb(void)
{
    printf("I am read timeout callback\n");
    return 1;
}


/* DEMO_0 主程序 */
static void uart_device_test_task(void *arg)
{
    int len;
    int cnt = 0;
    u32 cbuf_data_cnt = 0;

    void *hdl = dev_open("uart2", NULL);
    if (!hdl) {
        printf("open uart err !!!\n");
        return ;
    }

    // 设置串口接收缓存数据的循环buf地址(若使用RX,必须)
    dev_ioctl(hdl, IOCTL_UART_SET_CIRCULAR_BUFF_ADDR, (u32)buf);

    // 设置串口接收缓存数据的循环buf长度(若使用RX,必须)
    dev_ioctl(hdl, IOCTL_UART_SET_CIRCULAR_BUFF_LENTH, sizeof(buf));

    // 设置发送数据为阻塞方式(非必须)
    dev_ioctl(hdl, IOCTL_UART_SET_SEND_BLOCK, 1);

    // 设置接收数据为阻塞方式(非必须),建议加上超时设置
    dev_ioctl(hdl, IOCTL_UART_SET_RECV_BLOCK, 1);

    // 设置串口接收超时时间(非必须)
    dev_ioctl(hdl, IOCTL_UART_SET_RECV_TIMEOUT, 10 * 1000); ///<超时时间10s

    // 设置串口超时回调函数(非必须)
    dev_ioctl(hdl, IOCTL_UART_SET_RECV_TIMEOUT_CB, (u32)&read_timeout_cb);

    // 设置串口dev_read()需等待接收满指定数量数据才退出(非必须)
    /* dev_ioctl(hdl, IOCTL_UART_SET_RECV_ALL, 1); */

    // 设置串口中断时间回调函数(非必须)
    dev_ioctl(hdl, IOCTL_UART_SET_IRQ_EVENT_CB, (u32)&uart_irq_cb);

    // 使能串口,启动收发数据(必须)
    dev_ioctl(hdl, IOCTL_UART_START, 0);

    // 修改波特率
    /* dev_ioctl(hdl, IOCTL_UART_SET_BAUDRATE, 115200); */

    printf("uart_test_task running \n");

    while (1) {

        // 获取串口循环buf中已接收到未被取走的数据的累计值
        dev_ioctl(hdl, IOCTL_UART_GET_RECV_CNT, (u32)&cbuf_data_cnt);
        printf("UART cbuf now have data count:%u\n", cbuf_data_cnt);

        len = dev_read(hdl, recv_buf, BUF_SIZE);
        if (len <= 0) {
            printf("uart recv err len = %d\n", len);
            if (len == UART_CIRCULAR_BUFFER_WRITE_OVERLAY) {
                printf("UART_CIRCULAR_BUFFER_WRITE_OVERLAY...\n");
                // 如果循环buf接收到的数据未来得及取走,出现回卷覆盖,则冲掉循环buf所有数据重新接收
                dev_ioctl(hdl, IOCTL_UART_FLUSH, 0);
            } else if (len == UART_RECV_TIMEOUT) {
                printf("UART_RECV_TIMEOUT...\n");
            } else if (len == UART_RECV_EXIT) {
                printf("UART_RECV_EXIT...\n");
            }
            continue;
        }
        printf("uart recv len = %d\n", len);
        // 把串口接收到的数据发送回去
        dev_write(hdl, recv_buf, len);

        // 统计串口接收到的数据,并发送回去
        cnt += len;
        len = sprintf((char *)send_buf, "\nRx_Cnt = %d\n", cnt);
        dev_write(hdl, send_buf, len);

        os_time_dly(10);
    }

    // 串口停止
    /* dev_ioctl(hdl, IOCTL_UART_STOP, 0); */

    dev_close(hdl);
}

#endif ///< DEMO_0
/* +++++++++++++++++++++++++++++++++++++++ DEMO_0 end +++++++++++++++++++++++++++++++++++++++ */





/* +++++++++++++++++++++++++++++++++++++++ DEMO_1 start +++++++++++++++++++++++++++++++++++++++ */
#ifdef DEMO_1

#define BUF_SIZE 1 * 1024
static u8 recv_buf[BUF_SIZE];
static u8 send_buf[BUF_SIZE];
static u8 buf[2 * 1024] __attribute__((aligned(4))); ///< 用于串口接收缓存数据的循环buf


// DEMO_1 主程序
static void uart_test_main(void *priv)
{

    void *hdl = dev_open("uart2", NULL);
    if (!hdl) {
        printf("open uart err !!!\n");
        return ;
    }

    // 设置串口接收缓存数据的循环buf地址
    dev_ioctl(hdl, IOCTL_UART_SET_CIRCULAR_BUFF_ADDR, (u32)buf);

    // 设置串口接收缓存数据的循环buf长度
    dev_ioctl(hdl, IOCTL_UART_SET_CIRCULAR_BUFF_LENTH, sizeof(buf));

    // 设置接收数据为阻塞方式
    dev_ioctl(hdl, IOCTL_UART_SET_RECV_BLOCK, 1);

    // 使能串口,启动收发数据
    dev_ioctl(hdl, IOCTL_UART_START, 0);


#define TEST_CTS
    /* #define TEST_RTS */

    /* ~~~~~~~~~~~~~~~~~~TEST_CTS~~~~~~~~~~~~~~~~~~~~~ */
#ifdef TEST_CTS
    /*
     * 使用带流控功能的串口工具(比如CH343)+串口调试助手(比如友善串口调试助手),
     * 上位机设置RTS为busy时，那么我们uart将停止发送,可用逻辑分析仪观察
     */
    memset(send_buf, 'a', BUF_SIZE);
    while (1) {
        printf("UART send test_buf_a\n");
        dev_write(hdl, send_buf, BUF_SIZE);
        os_time_dly(200);
    }
#endif ///< TEST_CTS
    /* ~~~~~~~~~~~~~~~~~~TEST_CTS~~~~~~~~~~~~~~~~~~~~~ */


    /* ~~~~~~~~~~~~~~~~~~TEST_RTS~~~~~~~~~~~~~~~~~~~~~ */
#ifdef TEST_RTS
    /*
     * 使用带流控功能的串口工具(比如CH343)+串口调试助手(比如友善串口调试助手)。
     * 设置串口调试助手自动重复发送数据，用逻辑分析仪观察,在UART被设置为SUSPEND
     * 时，RTS脚会处busy电平，串口工具将不会发送数据。
     */
    int len;
    int cnt = 0;

    while (1) {
        dev_ioctl(hdl, IOCTL_UART_FLOW_CTL_RTS_RESUME, 0);
        printf("UART FLOW RESUME\n");
        os_time_dly(200);


        len = dev_read(hdl, recv_buf, BUF_SIZE);
        if (len <= 0) {
            if (len == UART_CIRCULAR_BUFFER_WRITE_OVERLAY) {
                printf("UART_CIRCULAR_BUFFER_WRITE_OVERLAY...\n");
                // 如果循环buf接收到的数据未来得及取走,出现回卷覆盖,则冲掉循环buf所有数据重新接收
                dev_ioctl(hdl, IOCTL_UART_FLUSH, 0);
            }
            continue;
        }
        printf("uart recv len = %d\n", len);
        // 把串口接收到的数据发送回去
        dev_write(hdl, recv_buf, len);

        // 统计串口接收到的数据,并发送回去
        cnt += len;
        len = sprintf((char *)send_buf, "\nRx_Cnt = %d\n", cnt);
        dev_write(hdl, send_buf, len);


        dev_ioctl(hdl, IOCTL_UART_FLOW_CTL_RTS_SUSPEND, 0);
        printf("UART FLOW SUSPEND\n");
        os_time_dly(500);
    }
#endif ///< TEST_RTS
    /* ~~~~~~~~~~~~~~~~~~TEST_RTS~~~~~~~~~~~~~~~~~~~~~ */
}

#endif ///< DEMO_1
/* +++++++++++++++++++++++++++++++++++++++ DEMO_1 end +++++++++++++++++++++++++++++++++++++++ */





static int c_main_uart_device(void)
{
    os_task_create(uart_device_test_task, NULL, 10, 1000, 0, "uart_device_test_task");
    return 0;
}

late_initcall(c_main_uart_device);

#endif // USE_UART_DEVICE_TEST_DEMO

