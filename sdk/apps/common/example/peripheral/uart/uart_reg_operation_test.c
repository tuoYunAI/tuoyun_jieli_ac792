/**@file        uart_reg_operation_test.c
  * @brief      直接操作uart寄存器,进行收发数据的测试例程
  * @author     jieli
  * @date       2024-01-10
  * @version    V1.0
  */


/********************************测试例程说明************************************
 *	概述：    文件为直接操作uart寄存器进行收发数据的例程。
 *
 *	程序功能：循环通过UART发送指定buf数据。如果UART接收到数据，将收到的数据通过
 *	          log打印出。UART在中断中接收数据到buf中。
 *
 *	注意：    1.对BUF寄存器读和写，功能不一样。读->返回RX接收的数据；写->启动一
 *	            次TX发送。
 *	          2.例程使用中断接收数据，非DMA方式，因此不应在中断中执行while()、打
 *	          印log等耗时的操作，否则可能出现上一个Byte数据的中断还未处理完，下
 *	          一Byte数据又来了，数据会丢失。如果需要使用DMA收发数据，需要使用SDK_wl83
 *	          的spec_uart驱动。
 ********************************************************************************/

#include "system/includes.h"
#include "app_config.h"
#include "asm/clock.h"
#include "uart.h"
#include "gpio.h"


#ifdef USE_UART_REG_OPERATION_TEST_DEMO


static u32 receive_cnt;                     ///< 接收到多少数据
static u8 buf[512];                         ///< 数据接收buf

static void uart1_usr_isr(void);

/* 配置串口参数的结构体 */
typedef struct {
    char *name;  			            	///< 串口名
    u8 irq;                  				///< 中断号
    u8 irq_cpu;	                        	///< 中断注册在哪个cpu核
    u8 irq_priority;	                	///< 中断优先级
    void (*irq_func)(void);		            ///< 中断函数
    JL_UART_TypeDef *reg;	               	///< 串口对应的寄存器组
    int tx_pin;               				///< 发送引脚，不配置需设置-1
    int rx_pin;               				///< 接收引脚，不配置需设置-1
    u32 baudrate;             				///< 波特率设置。理论范围：CLK/3 ~ CLK/4/(0xFFFF+1)
    gpio_drive_strength_t tx_pin_hd_level;	///< tx_pin强驱等级。
} uart_config_t;

static uart_config_t uart_o = {
    .name =							"uart1",
    .irq =       					IRQ_UART1_IDX,
    .irq_cpu =       				0,
    .irq_priority =       			1,
    .irq_func =						uart1_usr_isr,
    .reg = 							JL_UART1,
    .tx_pin =						IO_PORTA_06,
    .rx_pin =           			IO_PORTA_07,
    .baudrate =         			1000000,
    .tx_pin_hd_level =				GPIO_DRIVE_STRENGTH_8p0mA,
};


// 串口初始化
static void uart_usr_init(uart_config_t *p_uart)
{
    p_uart->reg->CON0 = 0;
    p_uart->reg->CON1 = 0;
    p_uart->reg->CON2 = 0;
    p_uart->reg->CON0 |= BIT(12) | BIT(13); ///<清RX、TX pnd
    p_uart->reg->CON1 |= BIT(5);///<RX接收数据通路直通使能
    /* p_uart->reg->CON1 &= ~BIT(4);///<receive enable */

    // 注册串口中断
    request_irq(p_uart->irq, p_uart->irq_priority, p_uart->irq_func, p_uart->irq_cpu);

    // 设置时钟源
    u32 clk_rate = 24000000;
    /* u32 clk_rate = clk_get("lsb"); */
    SFR(JL_CLOCK->PRP_CON0, 0, 3, 2);///< 1:48M, 2:24M, 3:EXT_CLK, 4:LSB_CLK

    // 设置波特率，波特率计算公式：baudrate = Freq / ((JL_BAUD + 1) x div)
    u32 baud = p_uart->baudrate;
    p_uart->reg->CON0 &= ~BIT(4);///<设置分频 0:4分频, 1:3分频
    p_uart->reg->BAUD = (clk_rate / baud) / 4 - 1;
    /* p_uart->reg->BAUD = (clk_rate / baud) / 3 - 1; */

    if (p_uart->tx_pin != -1) {
        gpio_enable_function(p_uart->tx_pin, GPIO_FUNC_UART0_TX + p_uart->name[4] - '0', 1);///<设置IO功能为UART TX
        gpio_set_hd(p_uart->tx_pin, p_uart->tx_pin_hd_level);///<设置强驱档位
        /* p_uart->reg->CON0 |= BIT(2);///<TX irq enable */
        p_uart->reg->CON0 |= BIT(0);///<使能串口发送功能
    }
    if (p_uart->rx_pin != -1) {
        gpio_enable_function(p_uart->rx_pin, GPIO_FUNC_UART0_RX + p_uart->name[4] - '0', 1);///<设置IO功能为UART RX
        p_uart->reg->CON0 |= BIT(3);///<RX irq enable
        p_uart->reg->CON0 |= BIT(1);///<使能串口接收功能
    }
}

// 关闭串口
static void uart_usr_close(uart_config_t *p_uart)
{
    if (p_uart->tx_pin != -1) {
        p_uart->reg->CON0 &= ~BIT(0); ///<关闭串口发送功能
        /* p_uart->reg->CON0 &= ~BIT(2);///<TX irq disable */
        gpio_disable_function(p_uart->tx_pin, GPIO_FUNC_UART0_TX + p_uart->name[4] - '0', 1);
    }
    if (p_uart->rx_pin != -1) {
        p_uart->reg->CON0 &= ~BIT(1); ///<关闭串口接收功能
        p_uart->reg->CON0 &= ~BIT(3);///<RX irq disable
        gpio_disable_function(p_uart->rx_pin, GPIO_FUNC_UART0_RX + p_uart->name[4] - '0', 1);
    }
}

// 串口发送一Byte数据
static void uart_usr_send_byte(uart_config_t *p_uart, u8 data)
{
    p_uart->reg->BUF = data;///<往BUF寄存器写数据，可启动一次发送
    while (!(p_uart->reg->CON0 & BIT(15))); ///<等待发送完一个Byte的pnd
    p_uart->reg->CON0 |= BIT(13);///<发送完成清TX pnd
}

// 串口发送指定buf长度的数据
static void uart_usr_send_buf(uart_config_t *p_uart, u8 *buf, u32 cnt)
{
    for (int i = 0; i < cnt; i++) {
        uart_usr_send_byte(p_uart, buf[i]);
    }
}

// UART接收
static void uart_usr_receive(void)
{
    JL_UART_TypeDef *reg = uart_o.reg;
    buf[receive_cnt] = reg->BUF;///<从UART BUF寄存器读数，可得到RX接收到的数据
    receive_cnt++;
}

// 中断函数
___interrupt
static void uart1_usr_isr(void)
{

    u32 con0 = uart_o.reg->CON0;

    /* if((con0 & BIT(15)) && (con0 & BIT(2))) {///<TX irq enable ,and TX pnd */
    /* uart_o.reg->CON0 |= BIT(13);///<TX触发中断,清RX的pnd */
    /* printf("isr TX \n"); */
    /* } */

    if ((con0 & BIT(14)) && (con0 & BIT(3))) { ///<RX irq enable ,and RX pnd
        uart_o.reg->CON0 |= BIT(12);///<RX触发中断,清RX的pnd
        /* printf("isr RX \n"); */
        uart_usr_receive();
    }
    return;
}

// 主程序
static void uart_reg_opt_test_task(void *arg)
{

    printf("uart reg opt test start!!!!\n");
    char *str = "uart send string\n";
    // 传入配置，初始化串口
    uart_usr_init(&uart_o);

    while (1) {
        printf("\n\n~~~~~~~~~~~~~new circle~~~~~~~~~~~~\n");

        if (receive_cnt > 0) { ///<uart收到数据，通过log将其打印出来
            printf("uart get data cnt:%d", receive_cnt);
            put_buf(buf, receive_cnt);
            receive_cnt = 0;
        }

        // UART发送
        uart_usr_send_buf(&uart_o, (u8 *)str, strlen(str));

        os_time_dly(200);
    }
    uart_usr_close(&uart_o);
}



static int c_main_uart_reg_operation(void)
{
    os_task_create(uart_reg_opt_test_task, NULL, 10, 1000, 0, "uart_reg_opt_test_task");
    return 0;
}

late_initcall(c_main_uart_reg_operation);

#endif ///< USE_UART_REG_OPERATION_TEST_DEMO

