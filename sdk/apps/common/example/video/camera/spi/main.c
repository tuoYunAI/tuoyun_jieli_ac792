#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"
#include "camera.h"
#include "device/gpio.h"
#include "spi_video.h"
#include "device/device.h"//u8
#include "storage_device.h"//SD
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "asm/clock.h"
#include "asm/pwm.h"
#include "lcd_config.h"//LCD_h
#include "system/includes.h"//late_initcall
#include "yuv_soft_scalling.h"//YUV420p_Soft_Scaling
#include "lcd_te_driver.h"//set_lcd_show_data_mode
#include "get_yuv_data.h"//get_yuv_init
#include "lcd_drive.h"

#ifdef USE_CAMERA_SPI_SHOW_TO_LCD_DEMO

/* 说明 需要将isd_config_rule.c 配置改1*/
/* LSB_DIV=1; [低速总线时钟分频系数 LSB_DIV+1] */

static struct spi_video *hdl;

struct lbuf_test_head {
    u8 data[0];
};

static void *lbuf_ptr = NULL;
static struct lbuff_head *lbuf_handle = NULL;
static struct lbuff_head *lib_system_lbuf_init(u32 buf_size)
{
    struct lbuff_head *lbuf = NULL;
    lbuf_ptr = malloc(buf_size);
    if (lbuf_ptr == NULL) {
        printf("lbuf malloc buf err");
        return NULL;
    }
    lbuf = lbuf_init(lbuf_ptr, buf_size, 4, sizeof(struct lbuf_test_head));
    /* lbuf = lbuf_init(lbuf_ptr, buf_size, 4, 1); */

    return lbuf;
}

static ___interrupt void spi_irq_cb(void); //中断函数，指定在内部sram
static void Calculation_frame(void)
{
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;
    fps_cnt++ ;
    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;
        if (tdiff >= 1000) {
            printf("\n [MSG]spi_video_fps_count = %d\n", fps_cnt *  1000 / tdiff);
            tstart = 0;
            fps_cnt = 0;
        }
    }
}

/********保存YUV数据到SD卡中************/
static void save_YUV_date_ontime(u8 *buf, u32 len)
{
    if (storage_device_ready()) {
        char file_name[64];//定义路径存储
        snprintf(file_name, 64, CONFIG_ROOT_PATH"YUV/YUV***.bin");
        FILE *fd = fopen(file_name, "wb+");
        fwrite(buf, 1, len, fd);
        fclose(fd);
        printf("YUV save ok name=YUV\r\n");
    }
}
static void debug_io_init(void)
{
#ifdef SPI_IO_DBG
    DBG_PORT->DIR &= ~DBG_PORT_BIT;
    DBG_PORT->PU &= ~DBG_PORT_BIT;
    DBG_PORT->PD &= ~DBG_PORT_BIT;
    DBG_PORT->OUT &= ~DBG_PORT_BIT;
#endif
}
static void spi_dma_recv_data(u32 *addr, u32 size)
{
    JL_SPI->ADR = addr;
    JL_SPI->CNT = size;
}

static void *pwm_dev = NULL;

static void spi_xclk_open(void)
{
    struct pwm_platform_data pwm = {0};
    if (!pwm_dev) {
        pwm_dev = dev_open("pwm1", &pwm);
    }
    if (!pwm_dev) {
        printf("open spi_xclk pwm fail\r\n");
        return;
    }
}

static void spi_xclk_close(void)
{
    struct pwm_platform_data pwm = {0};

    if (!pwm_dev) {
        printf("spi_xclk pwm no open\r\n");
        return;
    }
    dev_ioctl(pwm_dev, PWM_REMOV_CHANNEL, (u32)&pwm);
    dev_ioctl(pwm_dev, PWM_STOP, (u32)&pwm);
    dev_close(pwm_dev);
    pwm_dev = NULL;
}

static int spi_camera_open(void)
{
    //1.申请全局结构体内存
    hdl = ram_malloc(sizeof(struct spi_video));
    if (hdl == NULL) {
        printf("[error]>>>>>>>>>>>>malloc fail");
        goto exit;
    }
    os_sem_create(&hdl->sem, 0);
    os_sem_create(&hdl->rgb1, 0);
    os_sem_create(&hdl->rgb2, 0);

    os_sem_create(&hdl->rgb1_ok, 0);
    os_sem_create(&hdl->rgb2_ok, 0);

    hdl->buf = ram_malloc(LINE_LBUF_SIZE);
    if (!hdl->buf) {
        printf("err spi no buf \n");
        goto exit;
    }

    lbuf_handle = lib_system_lbuf_init(YUV422_LBUF_LEN + 128);
    if (!lbuf_handle) {
        printf("err lbuf_handle no buf \n");
        goto exit;
    }


    hdl->yuv422_buf = malloc(YUV422_LBUF_LEN);
    if (!hdl->buf) {
        printf("err spi no buf \n");
        goto exit;
    }

    hdl->recv_move_cnt = 0;
    hdl->frame_addr_move_cnt = 0;

    //这里需要注意顺序 需要先开摄像头 再开spi
    /* gpio_output_channle(CAM_XCLK_PORT, CH1_PLL_24M); */
    spi_xclk_open();
    dev_open("video1.0", NULL);
    os_time_dly(20);//过滤异常数据包

    hdl->spi = dev_open(JL_SPI_NAME, NULL);
    if (!hdl->spi) {
        printf("spi open err \n");
        goto exit;
    } else {
        dev_ioctl(hdl->spi, IOCTL_SPI_SET_IRQ_FUNC, (u32)spi_irq_cb);
        JL_SPI->CON &= ~BIT(2);
        JL_SPI->CON &= ~BIT(11);
        JL_SPI->CON |= BIT(10);
        JL_SPI->CON |= BIT(12);//接收模式
        JL_SPI->CON |= BIT(13);//开启中断
        JL_SPI->CON |= BIT(1);//从机模式
        JL_SPI->CON |= BIT(0);//开启SPI
    }

//用于触发逻辑分析仪检测
#ifdef SPI_IO_DBG
    DBG_PORT->OUT ^= DBG_PORT_BIT;
    DBG_PORT->OUT ^= DBG_PORT_BIT;
    DBG_PORT->OUT ^= DBG_PORT_BIT;
    DBG_PORT->OUT ^= DBG_PORT_BIT;
#endif
    printf("camera open ok\n");
    return 0;
exit:
    return 0;
}
static int spi_camera_close(void)
{
    lbuf_clear(lbuf_handle);
    gpio_clear_output_channle(CAM_XCLK_PORT, CH1_PLL_24M);
    dev_close(hdl->video);
    return 0;
}

//该中断的所有数据均要在内部ram进行处理 在SDRAM可能概率收错
//并将将中断函数里面要尽可能精简 高效 行间隔只有100us只有 PCLK24M
//PCLK80M xclk40M FPS20fps 两线spi
static ___interrupt void spi_irq_cb(void) sec(.volatile_ram_code)
{
    static u16 recv_cnt = 2;//从第二行开始处理
    u32 recv_len;
    JL_SPI->CON |= BIT(14);//清除中断标记
    /* u32 id = 0; */

#ifdef SPI_IO_DBG
    DBG_PORT->OUT ^= DBG_PORT_BIT;
#endif

    //////////处理上一次收到的数据///////////
    if (recv_cnt == 1) { //开始新的一轮帧接收
        if (*(u32 *)hdl->irq_recv_buf != CAM_LSTART_VALUE) { //校验数据
            goto recv_err;
        }
        hdl->cpy_addr = hdl->next_frame_addr;
        hdl->real_data_addr = hdl->irq_last_recv_buf + CAM_FHEAD_SIZE;
        /* 告诉lbuf这个数据包完整了  */
        /* id = dma_copy_async(hdl->cpy_addr, hdl->real_data_addr, CAM_LINE_SIZE); */
        /* dma_copy_async_wait();//硬件DMA的要等待DMA是否完成 */
        lbuf_push(hdl->next_frame_addr, BIT(0));//把数据块推送更新到lbuf的通道0
        /* 开始行的一轮buf */
        if (lbuf_free_space(lbuf_handle) < YUV422_PACK_LEN) { //查询LBUF空闲数据块是否有足够长度
            //如果内存不足直接读一个buf块释放
            hdl->next_frame_addr = lbuf_pop(lbuf_handle, BIT(0));//从lbuf的通道0读取数据块
            lbuf_free(hdl->next_frame_addr);
        }
        hdl->next_frame_addr = lbuf_alloc(lbuf_handle, YUV422_PACK_LEN); //lbuf内申请一块空间
        os_sem_post(&hdl->sem); //这里是允许使用操作系统的 帧间隔比较大有时间处理
    } else if (recv_cnt == 2) { //只有第一行有帧头 取余都是去行头
        hdl->cpy_addr = hdl->next_frame_addr;
        hdl->real_data_addr = hdl->irq_last_recv_buf + CAM_FHEAD_SIZE + CAM_LHEAD_SIZE;
    } else {
        hdl->cpy_addr = hdl->cpy_addr + CAM_LINE_SIZE;
        hdl->real_data_addr = hdl->irq_last_recv_buf + CAM_LHEAD_SIZE;
    }

    //使用DMA拷贝数据减缓CPU开销
    //使用行缓存拷贝需要十分注意一点 需要确保行间隔足够长 不然会追尾
    //使用行缓存拷贝会存在一个问题 就是图像模糊
    //dma 拷贝速度 128us 拷贝 1280个数据极限速度
    //这个地方使用了DMA拷贝别的地方就不能使用只有一个DMA拷贝模块
    dma_copy_async(hdl->cpy_addr, hdl->real_data_addr, CAM_LINE_SIZE);


    //////////处理下一次数据的接收///////////
    //行缓存
    hdl->irq_recv_buf = hdl->buf + hdl->recv_move_cnt * DATA_FIRST_PACK_SIZE;
    hdl->recv_move_cnt = ++hdl->recv_move_cnt % RECV_DATA_LINE_PACK_TIME;
    if (recv_cnt == 1) {            //接收第1行
        JL_SPI->CNT = DATA_FIRST_PACK_SIZE;
    } else if (recv_cnt < CAMERA_H) { //接收2~479行
        JL_SPI->CNT = DATA_LINE_PACK_SIZE;
    } else if (recv_cnt = CAMERA_H) { //接收480行
        JL_SPI->CNT =  DATA_END_PACK_SIZE;
    }// 0-479 一共480行
    JL_SPI->ADR = hdl->irq_recv_buf;

    //保存一下当前接收地址下一次进中断就能知道上一次的接收地址
    hdl->irq_last_recv_buf = hdl->irq_recv_buf;

    recv_cnt = ++recv_cnt % (CAMERA_H + 1); //0~481循环
    if (recv_cnt == 0) { //去除0 从1开始计数 就是1~480循环
        recv_cnt = 1;
    }
    return;

recv_err:
    printf("\n [ERROR] %s -[recv_err] %d\n", __FUNCTION__, __LINE__);
}

void get_spi_data(void)
{

}
#define R2(y,v)   ((298*y + 411*v - 57344)>>8)
#define G2(y,u,v) ((298*y - 101*u - 211*v + 34739)>>8)
#define B2(y,u)   ((298*y + 519*u - 71117)>>8)
#define TIME_DEBUG 0
void yuyv422_soft_rgb565(u8 *in_yuyv422, u8 *out_rgb_565, int sw, int sh, int dw, int dh) sec(.volatile_ram_code)
{
#if TIME_DEBUG
    u32 time1 = timer_get_ms();
#endif

    float Qw = sw / dh;
    float Qh = sh / dw;
    u8 *yuyv422;
    u8 *rgb565;
    u8 y, u, v;
    int r, g, b, i, j;
    int index;

    yuyv422 = in_yuyv422;
    rgb565  = out_rgb_565;

    for (j = 0; j < dh / 2; j++) {
        for (i = 0; i < dw; i++) {
            y = yuyv422[(sw * (sh - (u16)(i * Qh) - 1) + 1 + (u16)(j * Qw)) * 2];
            u = yuyv422[(sw * (sh - (u16)(i * Qh) - 1) + 1 + (u16)(j * Qw) - j % 2) * 2 + 1];
            v = yuyv422[(sw * (sh - (u16)(i * Qh) - 1) + 1 + (u16)(j * Qw) - j % 2) * 2 + 3];
            r = (R2(y, u) & 0x80000000) ? 0 : ((R2(y, u) & 0x7fffff00) ? 255 : R2(y, u));
            g = (G2(y, u, v) & 0x80000000) ? 0 : ((G2(y, u, v) & 0x7fffff00) ? 255 : G2(y, u, v));
            b = (B2(y, v) & 0x80000000) ? 0 : ((B2(y, v) & 0x7fffff00) ? 255 : B2(y, v));
            *rgb565++ = ((r & 0xf8) | ((g & 0xe0) >> 5));
            *rgb565++ = (((g & 0x1c) << 3) | (b & 0xf8) >> 3);
        }
    }

#if TIME_DEBUG
    u32 time2 = timer_get_ms();
    printf("[msg]>>>>>>>>>>>time2-time1=%d", time2 - time1);
#endif
}

void yuyv422_soft_rgb565_1(u8 *in_yuyv422, u8 *out_rgb_565, int sw, int sh, int dw, int dh) sec(.volatile_ram_code)
{
#if TIME_DEBUG
    u32 time1 = timer_get_ms();
#endif

    float Qw = sw / dh;
    float Qh = sh / dw;
    u8 *yuyv422;
    u8 *rgb565;
    u8 y, u, v;
    int r, g, b, i, j;
    int index;

    yuyv422 = in_yuyv422;
    rgb565  = out_rgb_565;

    rgb565 = rgb565 + dh * dw;
    for (j = dh / 2; j < dh; j++) {
        for (i = 0; i < dw; i++) {
            y = yuyv422[(sw * (sh - (u16)(i * Qh) - 1) + 1 + (u16)(j * Qw)) * 2];
            u = yuyv422[(sw * (sh - (u16)(i * Qh) - 1) + 1 + (u16)(j * Qw) - j % 2) * 2 + 1];
            v = yuyv422[(sw * (sh - (u16)(i * Qh) - 1) + 1 + (u16)(j * Qw) - j % 2) * 2 + 3];
            r = (R2(y, u) & 0x80000000) ? 0 : ((R2(y, u) & 0x7fffff00) ? 255 : R2(y, u));
            g = (G2(y, u, v) & 0x80000000) ? 0 : ((G2(y, u, v) & 0x7fffff00) ? 255 : G2(y, u, v));
            b = (B2(y, v) & 0x80000000) ? 0 : ((B2(y, v) & 0x7fffff00) ? 255 : B2(y, v));
            *rgb565++ = ((r & 0xf8) | ((g & 0xe0) >> 5));
            *rgb565++ = (((g & 0x1c) << 3) | (b & 0xf8) >> 3);
        }
    }

#if TIME_DEBUG
    u32 time2 = timer_get_ms();
    printf("[msg]>>>>>>>>>>>time2-time1=%d", time2 - time1);
#endif
}

static u8 test_buf1[640 * 480 * 2];

static void rgb565_task1(void *p)
{
    while (1) {
        os_sem_pend(&hdl->rgb1, 0);
        yuyv422_soft_rgb565(hdl->get_frame, test_buf1, 640, 480, 320, 480);
        os_sem_post(&hdl->rgb1_ok);
    }
}

static void rgb565_task2(void *p)
{
    while (1) {
        os_sem_pend(&hdl->rgb2, 0);
        yuyv422_soft_rgb565_1(hdl->get_frame, test_buf1, 640, 480, 320, 480);
        os_sem_post(&hdl->rgb2_ok);
    }
}

static void spi_video_task(void *p)
{
    debug_io_init();
    if (spi_camera_open()) {
        goto exit;
    }

    hdl->irq_recv_buf = hdl->buf;
    hdl->irq_last_recv_buf = hdl->irq_recv_buf;
    //第一次buf使用可以不用检测里面有无内存 肯定是有的 刚刚申请
    hdl->next_frame_addr = lbuf_alloc(lbuf_handle, YUV422_PACK_LEN); //lbuf内申请一块空间

    spi_dma_recv_data(hdl->irq_recv_buf, DATA_FIRST_PACK_SIZE);
    lcd_bl_pinstate(BL_ON);

    os_task_create(rgb565_task1, NULL, 1, 1024, 0, "rgb565_task1");
    os_task_create(rgb565_task2, NULL, 1, 1024, 0, "rgb565_task2");
    while (1) {
        os_sem_pend(&hdl->sem, 0);
        if (!lbuf_empty(lbuf_handle)) { //查询LBUF内是否有数据帧
            hdl->get_frame = lbuf_pop(lbuf_handle, BIT(0));//从lbuf的通道0读取数据块
            lcd_interface_non_block_wait();
            os_sem_post(&hdl->rgb1);
            os_sem_post(&hdl->rgb2);
            os_sem_pend(&hdl->rgb1_ok, 0);
            os_sem_pend(&hdl->rgb2_ok, 0);
            lcd_show_frame_to_dev(test_buf1, 320 * 480 * 2);
            Calculation_frame();
            lbuf_free(hdl->get_frame);
        }
    }
exit:
    spi_camera_close();
    while (1) { //等待其他任务杀此任务
        os_time_dly(100);//延时100个tick，防止看门狗
    }
}

static int spi_video_task_kill(void)
{
    printf("---> spi_video_task_kill\n");
    os_sem_post(&hdl->sem);
    while (hdl->kill) {
        os_time_dly(2);
    }
    task_kill("spi_video_task");
    return 0;
}


static int spi_video_task_init(void)
{
    os_task_create(spi_video_task, NULL, 20, 1024, 0, "spi_video_task");
    return 0;
}

static int c_main(void)
{
    /******ui_lcd_一起初始化数据***********/
    extern void user_ui_lcd_init(void);
    user_ui_lcd_init();
    /*****默认为UI模式*****/
    set_lcd_show_data_mode(camera);

    spi_video_task_init();


    return 0;
}

late_initcall(c_main);
#endif

