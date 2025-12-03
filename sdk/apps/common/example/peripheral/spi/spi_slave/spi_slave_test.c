#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"

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
 *     .role                   = SPI_MODE_SLAVE,
 * SPI1_PLATFORM_DATA_END()
 */


#ifdef  USE_SPI_SLAVE_TEST_DEMO

static void spi_slave_test_task(void *p)
{
    u8 *buf = NULL;
    void *spi_hdl = NULL;
    int ret;

#if 0

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~用法2：单次指定接收固定数据量~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /*
     * 注意：此方法spi接收数据比较慢，因此对方主机的spi连续发送字节不能过快，否则丢数据（当数对方主机spi数据过
     * 快，请使用上述：用户指定接收块方法）
     */

    // 打开spi设备
    spi_hdl = dev_open("spi2", NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }

    // 等待数据时，用信号量等待，不用则为抢占查询硬件中断标记，建议应信号量等待
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_USE_SEM, 1);

    // 发送一个字节
    u8 data = 0x12;
    dev_ioctl(spi_hdl, IOCTL_SPI_SEND_BYTE, data);

    // 读取一个字节
    dev_ioctl(spi_hdl, IOCTL_SPI_READ_BYTE, (u32)&data);
    printf("ioctl_spi_read_byte get data:%x\n", data);

    buf = malloc(1024);
    if (!buf) {
        printf("err spi no buf \n");
        goto exit;
    }

    while (1) {
        // 读取1024字节
        ret = dev_read(spi_hdl, buf, 1024);
        if (ret > 0) {//ret为实际读到的数据长度，正常情况下都会与指定的1024长度相等
            // 处理数据
            put_buf(buf, 1024);

        }
        os_time_dly(200);
    }

#else

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~用法1：接收固定数据量~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /*
     * 比如:接收固定的SPI摄像头数据。
     * SPI摄像宽为128，高为224，仅传输Y灰度数据，一帧数据头为4字节，
     * 一帧数据尾为4字节，摄像头进行一行一行数据往SPI发送数据，则当
     * 一帧数据量小于65535时，SPI从机配置如下：
     */
    spi_block_data_cfg_t bl_cfg = {0};
    int read_addr;


    // 计算一帧数据量大小
    int w = 128;
    int h = 224;
    int fhead  = 4;
    int fend = 4;
    // 一行的数据（此处为Y，当数据为Y+U+V的YUV422时，line_size = w * 2）
    int line_size = w;
    // 一帧的数据（此处为Y，若摄像头数据为Y+U+V的YUV422时，frame_size = line_size * h * 2 + fhead + fend）
    int frame_size = line_size * h + fhead + fend;


    // 用户指定接收块大小配置
    /*
     * 由于SPI DMA一次性最大发送数据量为SPI_MAX_SIZE，当一帧数据量<=SPI_MAX_SIZE，则可以接收完整的一帧。
     * 当一帧数据量>SPI_MAX_SIZE，则先接收一部分，再偏移地址接收一部分（内部系统完成，应用层不用关注）。
     */
    // first_dma_size第一次接收数据量：一帧数据量<=SPI_MAX_SIZE，可设置第一次DMA数据量为一帧或者0，当一帧数据量>SPI_MAX_SIZE，第一次接收为：帧头 + 行的整数倍
    bl_cfg.first_dma_size 	= (frame_size > SPI_MAX_SIZE) ? (SPI_MAX_SIZE / line_size * line_size + fhead) : frame_size;
    //dma最大个数为spi的最大dma字节
    bl_cfg.dma_max_cnt 		= SPI_MAX_SIZE;
    //一个块的大小（分块在此处即为一帧大小）
    bl_cfg.block_size 		= frame_size;
    //指定接收的缓冲区为一个块大小（此处为一帧）的2倍以上，此处为2倍
    bl_cfg.buf_size 		= frame_size * 2;
    //申请缓冲区
    bl_cfg.buf = malloc(bl_cfg.buf_size);
    if (!bl_cfg.buf) {
        printf("err spi no buf \n");
        goto exit;
    }

    // 打开spi设备
    spi_hdl = dev_open("spi2", NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        if (bl_cfg.buf) { //释放内存
            free(bl_cfg.buf);
        }
        goto exit;
    }

    // 设置用户指定接收块大小配置信息
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_BLOCK_DATA_CFG, (u32)&bl_cfg);

    while (1) {
        printf("ioctl_spi_read_block_data~~~~~~~~~~~~~~~\n");

        // 读取数据，注意：IOCTL_SPI_READ_BLOCK_DATA启用读取数据之前需保证此时对方SPI还没有发送数据，否则中途接收数据会出错
        ret = dev_ioctl(spi_hdl, IOCTL_SPI_READ_BLOCK_DATA, (u32)&read_addr);

        // 返回值不小于0则已经读取到数据
        if (ret >= 0) {
            // read_addr的值就是硬件存放数据在bl_cfg.buf的可用地址
            put_buf((u8 *)read_addr, frame_size); //打印数据
            // 用完释放数据
            dev_ioctl(spi_hdl, IOCTL_SPI_FREE_BLOCK_DATA, 0);
        }

        os_time_dly(200);
    }

#endif

exit:
    if (buf) {
        free(buf);
    }
    if (spi_hdl) { //关闭spi设备
        dev_close(spi_hdl);
    }
    while (1) {
        os_time_dly(100);
    }
}

static int c_main_spi_slave(void)
{
    os_task_create(spi_slave_test_task, NULL, 22, 1000, 0, "spi_slave_test_task");
    return 0;
}

late_initcall(c_main_spi_slave);

#endif // USE_SPI_SLAVE_TEST_DEMO
