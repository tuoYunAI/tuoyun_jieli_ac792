#include <assert.h>
#include "os/os_api.h"
#include "init.h"
#include "spinlock.h"
#include "lcd_driver.h"
#include "lcd_config.h"
#include "asm/cache.h"
#include "perf_counter.h"
#include "asm/dmm.h"
#include "video/fb.h"

#ifdef USE_LCD_TEST_DEMO

/* #define CONFIG_LCD_ENABLE_3FB ///< 利用3FB加速，防止FB渲染完要等待LCD垂直同步才能够切换BUF */

#if TCFG_LCD_INPUT_FORMAT==LCD_IN_RGB565
#define PIXEL_COLOR_T u16
#elif TCFG_LCD_INPUT_FORMAT==LCD_IN_RGB888
#define PIXEL_COLOR_T u32
#elif TCFG_LCD_INPUT_FORMAT==LCD_IN_ARGB888
#define PIXEL_COLOR_T u32
#elif TCFG_LCD_INPUT_FORMAT==LCD_IN_YUV422
#define PIXEL_COLOR_T u16
#endif

#define IMG_WIDTH       LCD_W
#define IMG_HEIGHT      LCD_H

#ifdef CONFIG_LCD_ENABLE_3FB
static PIXEL_COLOR_T fb_buf[3][IMG_WIDTH * IMG_HEIGHT] __attribute__((aligned(32)));
static void *next_offline_fb;         ///< 上一帧online_fb，也是下一帧offline_fb
static spinlock_t fb_lock;            ///< 双核互斥锁
#else
static PIXEL_COLOR_T fb_buf[2][IMG_WIDTH * IMG_HEIGHT] __attribute__((aligned(32)));
#endif
static PIXEL_COLOR_T fb_to_rotate[IMG_WIDTH * IMG_HEIGHT] __attribute__((aligned(32))); ///< 旋转buf
volatile static void *next_disp_fb;   ///< next_fb地址
static void *cur_disp_fb;             ///< 指向当前LCD的显示显存
static void *cur_draw_fb = fb_buf[0]; ///< 指向当前绘制中的显示显存
static void *lcd_dev;
static u16 rotate;


void lcd_calc_draw_rate(void)
{
    static u32 time_lapse_hdl;
    static u8 fps_cnt;
    ++fps_cnt;
    u32	tdiff = time_lapse(&time_lapse_hdl, 1000);
    if (tdiff) {
        printf("lcd draw %d fps\n", fps_cnt *  1000 / tdiff);
        fps_cnt = 0;
    }
}

void *lcd_get_draw_buf(void)
{
    return cur_draw_fb;
}

void *lcd_get_displaying_buf(void)
{
    return cur_disp_fb;
}

static void *lcd_frame_end_hook_func(void)
{
    if (!next_disp_fb) {
        return NULL; ///< 软件还未渲染完整一帧，继续显示上一帧显存
    }
#ifdef CONFIG_LCD_ENABLE_3FB
    spin_lock(&fb_lock);
    // next_offline_fb 变量保存上一帧的 online_fb 地址
    next_offline_fb = cur_disp_fb;
    // 调用系统API把 online_fb 的地址设置为当前 LCD 的显示显存
    cur_disp_fb = (void *)next_disp_fb;
    next_disp_fb = NULL;
    spin_unlock(&fb_lock);
    return cur_disp_fb;
#else
    cur_disp_fb = (void *)next_disp_fb;
    next_disp_fb = NULL;
    return cur_disp_fb;
#endif
}

static void lcd_dmm_cca_gamma_test(void)
{
    static u32 time_lapse_hdl;
    static u8 fps_cnt;
    static u32 variable = 0;
    static u8 change = 0;
    static dmm_ccmf_param_t special_param;
    special_param.matrix[0]  = 1.0;
    special_param.matrix[1]  = 0;
    special_param.matrix[2]  = 0;
    special_param.matrix[3]  = 0;
    special_param.matrix[4]  = 0;
    special_param.matrix[5]  = 1.0;
    special_param.matrix[6]  = 0;
    special_param.matrix[7]  = 0;
    special_param.matrix[8]  = 0;
    special_param.matrix[9]  = 0;
    special_param.matrix[10] = 1.0;
    special_param.matrix[11] = 0;
    ++fps_cnt;
    u32	tdiff = time_lapse(&time_lapse_hdl, 1000);
    if (tdiff) {
        printf("lcd_dmm_cca_gamma_test %d fps\n", fps_cnt *  1000 / tdiff);
        fps_cnt = 0;
        set_blend_ccmf_param(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 128, variable, 0); ///< 这里的值是原来的除以256:ok
        variable = variable + 10;
        dmm_gamma_param_t gamma_test = {0};
        if (change) {
            set_a_gamma_param(&gamma_test);
            set_dmm_rgb_gamma_param(&gamma_test);///< 写
            change = 0;
        } else {
            set_a_gamma_default_param(&gamma_test);
            set_dmm_rgb_gamma_param(&gamma_test);///< 写
            change = 1;
        }
    }
}

void lcd_open(void)
{
    if (lcd_dev) {
        return;
    }
    lcd_dev = dev_open("lcd", NULL);
    dev_ioctl(lcd_dev, IOCTL_LCD_RGB_SET_ISR_CB, (u32)lcd_frame_end_hook_func);
    ASSERT(lcd_dev != NULL);
}

void lcd_draw_finish(void)
{
    static u8 start_flag;
#if 1
    // 计算图像绘制帧率
    lcd_calc_draw_rate();
#endif

#if 0
    // 测试dmm调色模块ccm&gamma
    lcd_dmm_cca_gamma_test();
#endif

    // 一帧渲染完，可以同步cur_draw_fb到SDRAM,如果调用该函数前已经同步过这里就不需要
    DcuFlushRegion(cur_draw_fb, sizeof(fb_buf) / 3);

    if (!start_flag) {
        cur_disp_fb = fb_buf[0];///< LCD第一轮渲染在BUF0，因此设置为第一个推屏显存
        cur_draw_fb = fb_buf[1];///< 切换LCD第二轮渲染在BUF1
        dev_ioctl(lcd_dev, IOCTL_LCD_RGB_START_DISPLAY, (u32)cur_disp_fb); ///< 启动推屏显示
        start_flag = 1;
    }

#ifdef CONFIG_LCD_ENABLE_3FB
    if (next_offline_fb == NULL) {
        // 这个时刻是LCD第二轮渲染，由于有3FB，因此不需要等待LCD Vsync SWAP完成就可以渲染下一个FB
        next_offline_fb = fb_buf[2];//准备LCD第三轮渲染在BUF3
    } else {
        // 第三轮渲染以后都要等待LCD Vsync SWAP FB完成
        /* while (next_disp_fb != NULL) {}; */
        dev_ioctl(lcd_dev, IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, 0);
        // 等到s_next_fb空闲同步已完成
        ASSERT(next_disp_fb == NULL);
    }

    spin_lock(&fb_lock);
    // 将绘制好的 offline_fb 设置为 next_fb
    next_disp_fb = cur_draw_fb;
    // 把上一帧的 online_fb 作为下一帧的 offline_fb
    cur_draw_fb = next_offline_fb;
    spin_unlock(&fb_lock);
#else
    void *prev_disp_fb = cur_disp_fb;
    next_disp_fb = cur_draw_fb;
    dev_ioctl(lcd_dev, IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, 0);
    cur_draw_fb = prev_disp_fb;///< 把上一帧显示完成的显存配置为下一帧绘制的显存
#endif
}

// 显示彩色例子
static void lcd_colorful_test_task(void *p)
{
    const u8 lcd_in_format[] = {
        FB_COLOR_FORMAT_YUV422,
        FB_COLOR_FORMAT_RGB565,
        FB_COLOR_FORMAT_RGB888,
        FB_COLOR_FORMAT_ARGB8888
    };

    PIXEL_COLOR_T *buf_to_draw; ///< 要绘制图像的buf地址
    PIXEL_COLOR_T *buf_idle;
    struct lcd_dev_drive *lcd;

    // 打开lcd
    lcd_open();

    // 获取旋转配置
    dev_ioctl(lcd_dev, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
    rotate = lcd->dev->imd.info.rotate;
    printf("rotate = %d\n", rotate);

    while (1) {
        buf_idle = (PIXEL_COLOR_T *)lcd_get_draw_buf();

        if (rotate != ROTATE_0) {
            buf_to_draw = fb_to_rotate; ///< 旋转图像需要多一块buf
        } else {
            buf_to_draw = buf_idle;
        }

        // 绘制图像(等效UI绘制)
        for (int y = 0; y < IMG_HEIGHT; y++) {
            for (int x = 0; x < IMG_WIDTH; x++) {
                int index = y * IMG_WIDTH + x;
                int r = (x * 255) / IMG_WIDTH;
                int g = (y * 255) / IMG_HEIGHT;
                int b = 128;
                PIXEL_COLOR_T color = (r << 11) | (g << 5) | b;
                buf_to_draw[index] = color;
            }
        }

        // 旋转buf空间
        if (rotate != ROTATE_0) {
            extern int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width,
                                           int src_height, int src_stride, int dst_width,
                                           int dst_height, int dst_stride, int degree, int xoffset,
                                           int yoffset, int in_format, int out_format, uint8_t mirror);

            fb_frame_buf_rotate((u8 *)buf_to_draw,                    ///< 源头图层地址
                                (u8 *)buf_idle,                       ///< 目标图层地址
                                IMG_WIDTH,                            ///< 输入图层宽度
                                IMG_HEIGHT,                           ///< 输入图层高度
                                0,                                    ///< 输入图层跨度
                                IMG_HEIGHT,                           ///< 输出图层宽度
                                IMG_WIDTH,                            ///< 输出图层高度
                                0,                                    ///< 输出图层跨度
                                rotate,                               ///< 图层旋转角度
                                0,                                    ///< 图层旋转后x偏移
                                0,                                    ///< 图层旋转后y偏移
                                lcd_in_format[TCFG_LCD_INPUT_FORMAT], ///< 输入图层格式
                                lcd_in_format[TCFG_LCD_INPUT_FORMAT], ///< 输出图层格式
                                0                                     ///< 输出镜像,1:垂直镜像
                               );
        }

        lcd_draw_finish();
    }
}

static int lcd_main(void)
{
    lcd_colorful_test_task(NULL);
    return 0;
}
module_initcall(lcd_main);

#endif // USE_LCD_TEST_DEMO
