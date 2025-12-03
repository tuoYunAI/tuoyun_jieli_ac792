/**@file        uart_smartcard_iso7816_test.c
  * @brief      uart智能卡模式测试例程
  * @author     jieli
  * @date       2024-01-12
  * @version    V1.0
  */


/********************************测试例程说明************************************
 *	概述：    例程用于测试uart智能卡模式，通过直接操作寄存器实现。
 *
 *	程序功能：发送复位信号后，接收智能卡复位后的ATR数据包。循环发送命令并将智能卡
 *	          回复的数据通过log打印出。
 *
 *	发数逻辑：向BUF寄存器写一BYTE数据后，芯片硬件启动一次发送，程序等待发送完成。
 *	          程序等到发送完成起pnd后，检查是否发送校验出错，出错重发，如果重发
 *	          次数达到设定的MAX值则放弃发送。
 *
 *	收数逻辑：接收数据通过中断实现。当UART接收到数据，会起RX pnd，中断里检查
 *	          是否是RX起PND，并检查是否数据校验出错，出错则直接丢弃数据, 正常
 *	          则读取BUF寄存器值存放到buf中。
 *
 *	注意：    1.智能卡模式不能打开dma开关，否则会直接系统复位。
 *	          2.本例程的实体IC卡时钟需要外供，如果需要使用芯片提供时钟，参考pwm章节。
 ********************************************************************************/

#include "system/includes.h"
#include "app_config.h"
#include "uart.h"
#include "asm/clock.h"
#include "gpio.h"


#ifdef USE_UART_SMARTCARD_ISO7816_TEST_DEMO


#define SMARTCARD_RST_PIN    IO_PORTC_02    ///< 智能卡复位引脚

static u32 receive_cnt;                     ///< 接收到多少数据
static u8 buf[100];                         ///< 数据接收buf
static const u32 g_TX_FAIL_MAX = 20;        ///< 发送失败最大次数

static void uart1_smartcard_isr(void);

// 配置串口参数的结构体
typedef struct {
    char *name;  			            	///< 串口名
    u8 irq;                  				///< 中断号
    u8 irq_cpu;	                        	///< 中断注册在哪个cpu核
    u8 irq_priority;	                	///< 中断优先级
    void (*irq_func)(void);		            ///< 中断函数
    JL_UART_TypeDef *reg;	               	///< 串口对应的寄存器组
    int io_pin;               				///< 收发送引脚
    u32 baudrate;             				///< 波特率设置。理论范围：CLK/3 ~ CLK/4/(0xFFFF+1)
    uart_parity_t parity;       				///< 奇偶校验位。可选：UART_PARITY_DISABLE、UART_PARITY_EVEN、UART_PARITY_ODD
} uart_config_t;

static uart_config_t uart_o = {
    .name =				"uart1",
    .irq =       		IRQ_UART1_IDX,
    .irq_cpu =       	0,
    .irq_priority =     1,
    .irq_func =			uart1_smartcard_isr,
    .reg = 				JL_UART1,
    .io_pin =			IO_PORTC_01,
    .baudrate =         9600 * 101 / 100,   ///< smartcard模式RX有些问题，Byte数据间隔裕度不够，通过提高1%波特率可规避
    .parity =      		UART_PARITY_EVEN,
};


// 串口初始化
static void uart_smartcard_mode_init(uart_config_t *p_uart)
{
    p_uart->reg->CON0 = 0;
    p_uart->reg->CON1 = 0;
    p_uart->reg->CON2 = 0;
    // 注册串口中断
    request_irq(p_uart->irq, p_uart->irq_priority, p_uart->irq_func, p_uart->irq_cpu);

    // 设置时钟源
    u32 clk_rate = 24000000;
    /* u32 clk_rate = clk_get("lsb"); */
    SFR(JL_CLOCK->PRP_CON0, 0, 3, 2);                    ///< 1:48M, 2:24M, 3:EXT_CLK, 4:LSB_CLK

    // 设置波特率，波特率计算公式：baudrate = Freq / ((JL_BAUD + 1) x div)
    u32 baud = p_uart->baudrate;
    p_uart->reg->CON0 &= ~BIT(4);                        ///<设置分频 0:4分频, 1:3分频
    p_uart->reg->BAUD = (clk_rate / baud) / 4 - 1;
    /* p_uart->reg->BAUD = (clk_rate / baud) / 3 - 1; */

    p_uart->reg->CON0 |= BIT(12) | BIT(13);              ///< 清RX、TX pnd
    p_uart->reg->CON2 |= BIT(7);                         ///< 清校验pnd
    /* p_uart->reg->CON1 &= ~BIT(4);                        ///< receive enable */
    p_uart->reg->CON0 &= ~BIT(6);                        ///< 关dma,智能卡模式不能开dma，否则会导致系统复位。
    p_uart->reg->CON1 |= BIT(5);                         ///< RX接收数据通路直通使能
    p_uart->reg->CON2 |= BIT(9);                         ///< 开智能卡模式

    gpio_set_spl(p_uart->io_pin, 1);                     ///< 智能卡模式必须开启SPL(使得IO输出时保持上下拉功能)
    gpio_enable_function(p_uart->io_pin, GPIO_FUNC_UART0_TX + p_uart->name[4] - '0', 1); ///< 设置IO功能为UART TX
    gpio_enable_function(p_uart->io_pin, GPIO_FUNC_UART0_RX + p_uart->name[4] - '0', 1); ///< 设置IO功能为UART RX

    if (p_uart->parity != UART_PARITY_DISABLE) {
        p_uart->reg->CON2 |= p_uart->parity << 4;        ///< check_mode
        p_uart->reg->CON2 |= BIT(7);                     ///< 清校验pnd
        /* p_uart->reg->CON2 |= BIT(6);                     ///< 校验中断使能 */
        p_uart->reg->CON2 |= BIT(3);                     ///< 校验使能
        p_uart->reg->CON2 |= BIT(0);                     ///< 9BIT模式使能
    }

    /* p_uart->reg->CON0 |= BIT(2);                         ///< TX中断使能 */
    p_uart->reg->CON0 |= BIT(3);                         ///< RX中断使能
    p_uart->reg->CON0 |= BIT(0) | BIT(1);                ///< 使能串口TX/RX功能
}

// 关闭串口
static void uart_smartcard_mode_close(uart_config_t *p_uart)
{
    p_uart->reg->CON2 &= ~BIT(9);                        ///< 关智能卡模式
    p_uart->reg->CON0 &= ~BIT(0);                        ///< 关闭串口发送功能
    p_uart->reg->CON0 &= ~BIT(1);                        ///< 关闭串口接收功能
    /* p_uart->reg->CON0 &= ~BIT(2);                        ///< TX irq disable */
    p_uart->reg->CON0 &= ~BIT(3);                        ///< RX irq disable
    gpio_disable_function(p_uart->io_pin, GPIO_FUNC_UART0_TX + p_uart->name[4] - '0', 1);
    gpio_disable_function(p_uart->io_pin, GPIO_FUNC_UART0_RX + p_uart->name[4] - '0', 1);
    p_uart->reg->CON2 &= ~BIT(3);                        ///< 关闭校验
    /* p_uart->reg->CON2 &= ~BIT(6);                        ///< 关闭校验中断 */
    p_uart->reg->CON2 &= ~BIT(0);                        ///< 关闭9BIT模式
}

static void smarcard_reset(void)
{
    gpio_direction_output(SMARTCARD_RST_PIN, 1);
    os_time_dly(1);
    gpio_direction_output(SMARTCARD_RST_PIN, 0);
    os_time_dly(1);
    gpio_direction_output(SMARTCARD_RST_PIN, 1);
}

// 串口发送一Byte数据
static int  uart_smartcard_send_byte(uart_config_t *p_uart, u8 data)
{
    u32 tx_fail_times = 0;

__redo:
    p_uart->reg->BUF = data;                             ///< 往BUF寄存器写数据，可启动一次发送
    while (!(p_uart->reg->CON0 & BIT(15))) {}            ///< 等待发送完一个Byte的pnd
    p_uart->reg->CON0 |= BIT(13);                        ///< 发送完成清TX pnd
    if (p_uart->reg->CON2 & BIT(11)) {
        tx_fail_times++;
        if (tx_fail_times >= g_TX_FAIL_MAX) {
            return -1;                                   ///< 发送失败次数达到MAX值，返回发送失败
        } else {
            goto __redo;                                 ///< 重发
        }
    }

    return 0;                                            ///< 发送成功
}

// 串口发送指定buf长度的数据
static int uart_smartcard_send_buf(uart_config_t *p_uart, u8 *buf, u32 cnt)
{
    int ret;
    for (int i = 0; i < cnt; i++) {
        ret = uart_smartcard_send_byte(p_uart, buf[i]);
        if (ret == -1) {
            return ret;
        }
    }
    return 0;
}

// UART接收，在中断中被调用
static void uart_smartcard_receive(void)
{
    JL_UART_TypeDef *reg = uart_o.reg;
    buf[receive_cnt] = reg->BUF;///<从UART BUF寄存器读数，可得到RX接收到的数据
    receive_cnt++;
}

// 中断函数
___interrupt
static void uart1_smartcard_isr(void)
{

    u32 con0 = uart_o.reg->CON0;
    u32 con2 = uart_o.reg->CON2;

    /* if((con0 & BIT(15)) && (con0 & BIT(2))) {            ///< TX irq enable ,and TX pnd */
    /* uart_o.reg->CON0 |= BIT(13);                     ///< TX触发中断,清RX的pnd */
    /* printf("[smartcard] isr TX \n"); */
    /* } */

    if ((con0 & BIT(14)) && (con0 & BIT(3))) {           ///< RX irq enable ,and RX pnd
        uart_o.reg->CON0 |= BIT(12);                     ///< RX触发中断,清RX的pnd
        /* printf("[smartcard] isr RX \n"); */
        if (con2 & BIT(8)) {                             ///< 校验出错,丢弃数据,芯片硬件会自动提示发送方校验错误
            uart_o.reg->CON2 |= BIT(7);                  ///< 清校验错误pnd
            return;
        }
        uart_smartcard_receive();
    }
}

// 主程序
static void uart_smartcard_test_main(void *priv)
{
    const u8 cmd_buf1[] = {0x00, 0x84, 0x00, 0x00, 0x08};///< 某SIM卡生成8个随机数命令(即发送“Get Challenge”)
    int ret;                                             ///< 发送是否成功的返回值

    printf("[smartcard] uart smartcard mode test start!!!!\n");

    // 传入配置，初始化串口
    uart_smartcard_mode_init(&uart_o);

    // 复位智能卡后，智能卡会先发送一包ATR数据
    smarcard_reset();
    os_time_dly(40);                                     ///< 等智能卡发送完数据后，再从cbuf里面拿数据。
    if (receive_cnt > 0) { ///<uart收到数据，通过log将其打印出来
        printf("[smartcard] uart get data cnt:%d", receive_cnt);
        put_buf(buf, receive_cnt);
        receive_cnt = 0;
    }

    // 循环发送命令，发送成功后，delay一段时间接收完数据，将收到的数据通过log打印出
    while (1) {
        printf("\n\n~~~~~~~~~~~~~new circle~~~~~~~~~~~~\n");

        // 发送命令
        ret = uart_smartcard_send_buf(&uart_o, cmd_buf1, sizeof(cmd_buf1));
        if (ret != 0) {
            printf("[smartcard] send cmd fail\n");
            os_time_dly(200);
            continue;
        }
        printf("[smartcard] send cmd successful\n");

        // delay一段时间接收完数据，将收到的数据通过log打印出
        os_time_dly(40);
        if (receive_cnt > 0) { ///<uart收到数据，通过log将其打印出来
            printf("[smartcard] uart get data cnt:%d", receive_cnt);
            put_buf(buf, receive_cnt);
            receive_cnt = 0;
        }

        os_time_dly(200);
    }
    uart_smartcard_mode_close(&uart_o);
}


static int c_main(void)
{
    os_task_create(uart_smartcard_test_main, NULL, 10, 1000, 0, "uart_smartcard_test_main");
    return 0;
}

late_initcall(c_main);

#endif ///< USE_UART_SMARTCARD_ISO7816_TEST_DEMO

