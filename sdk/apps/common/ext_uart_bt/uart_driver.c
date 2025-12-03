#include "system/includes.h"
#include "app_config.h"
#include "uart.h"

#if TCFG_INSTR_DEV_UART_ENABLE

#define LOG_TAG         "[UART_DRIVER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"

/* 宏定义 */
#define uart_name   "uart2"                     /**< 使用的串口设备名 */
#define UART_MAX_RECV_SIZE   (4 * 1024)         /**< 最大接收数据大小 */

/* 全局变量声明 */
static void *uart_hdl = NULL;                   /**< 串口设备句柄 */
static u8 uart_buf[UART_MAX_RECV_SIZE] __attribute__((aligned(32))); /**< 串口接收缓冲区，32字节对齐 */

/*************************************************************************************************/
/**
 * @brief  串口接收任务
 *
 * @param[in] priv 任务私有参数（未使用）
 *
 * @return 无
 *
 * @note 该任务负责持续接收串口数据并进行处理
 */
/*************************************************************************************************/
static void uart_recevice_task(void *priv)
{
    u8 *recv_buf;
    s32 recv_len = 0;

    /* 分配接收缓冲区 */
    recv_buf = zalloc(UART_MAX_RECV_SIZE);
    if (!recv_buf) {
        r_printf("malloc uart err !!!\n");
        return ;
    }

    g_printf("uart_recevice_task\n");

    /* 任务主循环 */
    while (1) {
        memset(recv_buf, 0, UART_MAX_RECV_SIZE);

        /* 从串口读取数据 */
        recv_len = dev_read(uart_hdl, recv_buf, UART_MAX_RECV_SIZE);

        if (recv_len > 0) {
            /* 成功接收到数据 */
            printf("\n uart rx ok!***len:%d*****\n", recv_len);
            pack_parserread(recv_buf, recv_len);
        } else if (recv_len <= 0) {
            /* 接收失败或超时处理 */
            if (recv_len == -1) {
                /* 刷新串口缓冲区 */
                dev_ioctl(uart_hdl, IOCTL_UART_FLUSH, 0);
            }
            os_time_dly(1);
            continue;
        }
        os_time_dly(1);
    }
}

/*************************************************************************************************/
/**
 * @brief  串口驱动初始化函数
 *
 * @return 无
 *
 * @note 初始化串口设备并创建接收任务
 */
/*************************************************************************************************/
void uart_driver_init(void)
{
    int len = sizeof(uart_buf);

    /* 打开串口设备 */
    uart_hdl = dev_open(uart_name, NULL);

    if (!uart_hdl) {
        log_info("open uart err !!!\n");
        return ;
    }

    /* 1. 设置环形缓冲区地址 */
    dev_ioctl(uart_hdl, IOCTL_UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_buf);

    /* 2. 设置环形缓冲区长度 */
    dev_ioctl(uart_hdl, IOCTL_UART_SET_CIRCULAR_BUFF_LENTH, len);

    /* 3. 设置接收模式为阻塞方式 */
    dev_ioctl(uart_hdl, IOCTL_UART_SET_RECV_BLOCK, 1);

    /* 设置接收超时时间 */
    u32 parm = 500;  /**< 接收超时时间（单位：ms），需比发送超时时间小 */
    dev_ioctl(uart_hdl, IOCTL_UART_SET_RECV_TIMEOUT, (u32)parm);

    /* 4. 启动串口设备 */
    dev_ioctl(uart_hdl, IOCTL_UART_START, 0);

    printf("---> uart_recv_task start create!!! \n");

    /* 创建串口接收任务 */
    int err = thread_fork("uart_recevice_task", 20, 2048, 0, 0, uart_recevice_task, NULL);
    if (err != OS_ERR_NONE) {
        r_printf("creat uart_recevice_task fail %x\n", err);
    }
}

/*************************************************************************************************/
/**
 * @brief  串口数据发送函数
 *
 * @param[in] data 待发送数据指针
 * @param[in] len  待发送数据长度
 *
 * @return 无
 *
 * @note 通过串口发送指定长度的数据
 */
/*************************************************************************************************/
void uart_dev_write(u8 *data, u32 len)
{
    if (uart_hdl) {
        dev_write(uart_hdl, data, len);
    }
}

#endif

