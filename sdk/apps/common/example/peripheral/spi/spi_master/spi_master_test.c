#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"
#include "device/gpio.h"

/*
 * 参考板级配置:
 * SPI1_PLATFORM_DATA_BEGIN(spi1_data)
 *     .io = {
 *         .cs_pin             = -1,
 *         .di_pin             = IO_PORTA_06,
 *         .do_pin             = IO_PORTA_05,
 *         .clk_pin            = IO_PORTA_04,
 *         .d2_pin             = -1,
 *         .d3_pin             = -1,
 *     },
 *     .clk                    = 10000000,
 *     .mode                   = SPI_STD_MODE,
 *     .priority               = 6,
 *     .cpu_id                 = 1,
 *     .hd_level               = 0,
 *     .attr                   = SPI_SCLK_L_UPL_SMPH,
 *     .role                   = SPI_MODE_MASTER,
 * SPI1_PLATFORM_DATA_END()
 */

#ifdef USE_SPI_MASTER_TEST_DEMO

#define SPI_CS  IO_PORTA_00
#define SPI_BUF_LEN  1024

void spi_cs(char val)
{
    gpio_direction_output(SPI_CS, val);
}

static void spi_master_test_task(void *arg)
{
    u8 *buf = NULL;
    void *spi_hdl = NULL;
    int ret;

    // 1.打开spi设备
    spi_hdl = dev_open("spi1", NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }

    // 在同步收/发时，使用信号量等待。(未配置的话默认while等待)
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_USE_SEM, 1);

    // 2.发送一个字节
    u8 data = 0x12;
    spi_cs(0);
    dev_ioctl(spi_hdl, IOCTL_SPI_SEND_BYTE, data);
    spi_cs(1);
    os_time_dly(100);

    // 3.读取一个字节
    spi_cs(0);
    dev_ioctl(spi_hdl, IOCTL_SPI_READ_BYTE, (u32)&data);
    spi_cs(1);
    os_time_dly(100);

    buf = malloc(SPI_BUF_LEN);
    if (!buf) {
        printf("err spi no buf \n");
        goto exit;
    }
    memset(buf, 0xAA, SPI_BUF_LEN);

    // 4.发送1024字节
    spi_cs(0);
    dev_write(spi_hdl, buf, SPI_BUF_LEN);
    spi_cs(1);
    os_time_dly(100);

    while (1) {
        spi_cs(0);
        ret = dev_read(spi_hdl, buf, SPI_BUF_LEN);
        spi_cs(1);
        // ret为实际读到的数据长度，正常情况下都会与指定的1024长度相等
        if (ret > 0) {
            // 5. 处理数据
            put_buf(buf, SPI_BUF_LEN);
        }
        os_time_dly(100);
    }

exit:
    if (buf) {
        free(buf);
    }
    if (spi_hdl) { // 关闭spi设备
        dev_close(spi_hdl);
    }
    while (1) {
        os_time_dly(100);
    }
}

static int c_main_spi_master(void)
{
    os_task_create(spi_master_test_task, NULL, 22, 1000, 0, "spi_master_test_task");
    return 0;
}

late_initcall(c_main_spi_master);

#endif // USE_SPI_MASTER_TEST_DEMO
