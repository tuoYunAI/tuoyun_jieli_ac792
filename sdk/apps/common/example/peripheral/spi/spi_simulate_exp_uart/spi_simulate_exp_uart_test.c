#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"
#include "device/gpio.h"

#ifdef USE_SPI_SIMULATE_EXP_UART_TEST_DEMO

/*
 * 本例程，用于演示使用spi接口，三个字节SPI信号模拟特殊编码的uart信号（0用01编码，1用11编码）
 * 注意：
 * 1.AC792N的SPI固定为MSB发送
 * 2.本例程使用spi1接口，改用其他接口，务必修改本例程中所有包含"spi1"字符的命名
 * 3.dev_open的spi接口应在在板级里有注册，否则会open失败。
 * 4.spi clk为19200时的配置：SYS_CLK = 270000000; LSB_CLK = 4900000; SPI_CLK = 19200;
 */


#define UART_ODD   0
#define UART_EVEN  1
#define ENCODE_LSB  0
#define ENCODE_MSB  1

#define SPI_SIM_EXP_UART_PARITY  UART_ODD  ///< 串口奇校验
/* #define SPI_SIM_EXP_UART_PARITY  UART_EVEN ///< 串口偶校验 */

#define ENCODE_MODE ENCODE_LSB             ///< 编码模式：LSB
/* #define ENCODE_MODE ENCODE_MSB             ///< 编码模式：MSB */


//参考板级配置:
// *INDENT-OFF*
SPI1_PLATFORM_DATA_BEGIN(spi_data_usr)
    .io = {
        .cs_pin             = -1,
        .di_pin             = -1,
        .do_pin             = IO_PORTC_01,
        .clk_pin            = IO_PORTC_02,
        .d2_pin             = -1,
        .d3_pin             = -1,
    },
    /* .clk                    = 500000, */
    .clk                    = 19200,
    .mode                   = SPI_1WIRE_MODE,
    .priority               = 6,
    .cpu_id                 = 1,
    .hd_level               = GPIO_DRIVE_STRENGTH_18p5mA,
    /* .attr                   = SPI_SCLK_H_UPL_SMPH, */
    .attr                   = SPI_SCLK_H_UPH_SMPL,
    .role                   = SPI_MODE_MASTER,
SPI1_PLATFORM_DATA_END()


void *spi_hdl = NULL;
static int spi_sim_exp_uart_init(const struct spi_platform_data *data)
{
    spi_hdl = dev_open("spi1", (void *)data);
    if (!spi_hdl) {
        printf("spi open err \n");
        return -1;
    }
    // 解决SPI DMA发送后发送脚的空闲电平受到BUF寄存器BIT7影响的问题
    JL_SPI1->CON &= ~BIT(0);///< 关闭SPI后，再配置BUF值(SPI驱动启动发送前会重新使能SPI模块)
    JL_SPI1->BUF = 0x80;    ///< 将BIT7置1，使得DO空闲时为高电平

    // 在同步收/发时，使用信号量等待(未配置的话默认while等待)
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_USE_SEM, 1);
    // 异步发送
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_ASYNC_SEND, 1);
    return 0;
}

static int spi_sim_exp_uart_deinit(void)
{
    if (spi_hdl) {
        dev_close(spi_hdl);
        spi_hdl = NULL;
    }
    return 0;
}

static int spi_sim_uart_send_buf(u8 *buf, u32 len)
{
    int ret = -1;
    if (spi_hdl) {
        ret = dev_write(spi_hdl, buf, len);
    }
    return ret ? -1 : 0;
}

static void spi_sim_exp_uart_encode_byte(u32 byte, u8 *save_buf)
{
    save_buf[0] = 0x55;
    save_buf[1] = 0x55;
    /* save_buf[2] = 0x5C; ///< 编码数据的最后两个BIT填充00 */
    save_buf[2] = 0x5F; ///< 编码数据的最后两个BIT填充11

#if ENCODE_MODE == ENCODE_LSB
    if(byte & BIT(0))save_buf[0] |= BIT(5);
    if(byte & BIT(1))save_buf[0] |= BIT(3);
    if(byte & BIT(2))save_buf[0] |= BIT(1);
    if(byte & BIT(3))save_buf[1] |= BIT(7);
    if(byte & BIT(4))save_buf[1] |= BIT(5);
    if(byte & BIT(5))save_buf[1] |= BIT(3);
    if(byte & BIT(6))save_buf[1] |= BIT(1);
    if(byte & BIT(7))save_buf[2] |= BIT(7);
#elif ENCODE_MODE == ENCODE_MSB
    if(byte & BIT(7))save_buf[0] |= BIT(5);
    if(byte & BIT(6))save_buf[0] |= BIT(3);
    if(byte & BIT(5))save_buf[0] |= BIT(1);
    if(byte & BIT(4))save_buf[1] |= BIT(7);
    if(byte & BIT(3))save_buf[1] |= BIT(5);
    if(byte & BIT(2))save_buf[1] |= BIT(3);
    if(byte & BIT(1))save_buf[1] |= BIT(1);
    if(byte & BIT(0))save_buf[2] |= BIT(7);
#endif

    u8 odd = 0;
    for(u8 i = 0; i < 8; i++)
    {
        if((byte >> i) & 0x01) odd = !odd;
    }

#if SPI_SIM_EXP_UART_PARITY == UART_ODD
    save_buf[2] |= odd ? 0x00 : BIT(5);
#elif SPI_SIM_EXP_UART_PARITY == UART_EVEN
    save_buf[2] |= odd ? BIT(5) : 0x00;
#endif
}

static void spi_sim_exp_uart_encode_buf(u8 *src_buf, u8 *save_buf, u32 src_len)
{
    for (u32 i = 0; i < src_len; i++) {
        spi_sim_exp_uart_encode_byte(src_buf[i], (save_buf + 3 * i));
    }
}


static void spi_simulate_exp_uart_task(void *arg)
{
    /* u8 data[] = {0x01, 0x03, 0x07 ,0x0F, 0x1F, 0x3F, 0x7F ,0xFF}; */
    /* u8 data[] = {0x11, 0x22, 0x33 ,0x44, 0x55, 0x66, 0x77 ,0x88}; */
    u8 data[] = {0x55, 0xAA, 0x00};

    int ret;
    u8 *buf_data;
    u8 *buf_encoded_data;
    u32 data_len;

    buf_data = (u8 *)data;
    data_len = sizeof(data);

    // 准备buf空间
    buf_encoded_data = malloc(data_len * 3);
    if (!buf_encoded_data) {
        printf("%s : malloc fail!!!!\n", __FUNCTION__);
        goto exit;
    }

    // 初始化spi接口
    ret = spi_sim_exp_uart_init(&spi_data_usr);
    if (ret) {
        printf("%s : init fail!!!!\n", __FUNCTION__);
        goto exit;
    }

    // 编码准备数据
    spi_sim_exp_uart_encode_buf(buf_data, buf_encoded_data, data_len);

    // 发送数据
    printf("data:\n");
    put_buf(buf_data, data_len);
    printf("encoded data:\n");
    put_buf(buf_encoded_data, data_len * 3);
    while (1) {
        spi_sim_uart_send_buf(buf_encoded_data, data_len * 3);
        os_time_dly(100);
    }

exit:
    spi_sim_exp_uart_deinit();
    if (buf_encoded_data) {
        free(buf_encoded_data);
        buf_encoded_data = NULL;
    }
    while (1) {
        os_time_dly(100);
    }
}

static int c_main_spi_simulate_exp_uart(void)
{
    os_task_create(spi_simulate_exp_uart_task, NULL, 22, 1000, 0, "spi_simulate_exp_uart_task");
    return 0;
}

late_initcall(c_main_spi_simulate_exp_uart);

#endif // USE_SPI_SIMULATE_EXP_UART_TEST_DEMO
