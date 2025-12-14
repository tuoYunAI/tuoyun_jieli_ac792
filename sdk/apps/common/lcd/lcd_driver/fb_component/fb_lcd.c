/******************************************************************************
* File:             fb_lcd.c
*
* Author:
* Created:          02/28/24
* Description:      1.fb lcd 输出端口功能,实现驱屏相关功能
*****************************************************************************/
#include "system/init.h"
#include "lcd_driver.h"
#include "video/video.h"
#include "video/fb.h"
#include "asm/dma2d_driver.h"
#include "asm/jldma2d.h"
#include "fs/fs.h"
#include "asm/gpio.h"
#include "asm/dcache.h"
#include "asm/dmm.h"
#include "app_config.h"
#include "lcd_config.h"
#include "perf_counter/perf_counter.h"

#if TCFG_LCD_ENABLE

#define LOG_TAG_CONST       FB
#define LOG_TAG             "[FB]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_ERROR_ENABLE
#include "system/debug.h"
#if (defined USE_LVGL_V8_UI_DEMO || define USE_LVGL_V9_UI)
#include "lv_conf.h"
#endif

#ifndef FB_LCD_BUF_NUM /*防止app_config.h 未定义*/
#error "Please define FB_LCD_BUF_NUM 2 in app_config.h"
#endif

#define FB_LCD_FRAME_RATE_DEBUG_EN         1   //打印lcd数据更新帧率

struct fb_lcd_t {
    void *lcd;
    void *next_disp;/* 下一帧待显示的next_fb地址 */
    volatile u8 lcd_kick_start;
    u8 lcd_type;
    u8 interpolation_en;
    u16 out_buf_x; //显存x偏移
    u16 out_buf_y; //显存y偏移
    u16 out_buf_w; //显存宽
    u16 out_buf_h; //显存高
    u16 out_w;     //实际的推屏宽
    u16 out_h;     //实际的推屏高
    u32 out_buf[2]; //显存buffer
    u8  out_buf_num;
    u16 out_format;
    u16 rotate;
    volatile u8 out_buf_index;
    volatile u8 lcd_push_task_run;
    OS_SEM lcd_async_sem;
    OS_SEM lcd_async_busy_sem;
};

static u8 lcd_buf_num = FB_LCD_BUF_NUM;

static volatile u8  g_dmm_line_period; //dmm读取一行所需时间
static volatile u32 g_last_vsync_trig_time;
static volatile u64 g_vsync_start_time;
static volatile u16 g_dmm_line;

static struct fb_lcd_t _fb_lcd[FB_MAX_OUT_NUM];
#define  __this   (&_fb_lcd[id])

enum {
    FB_LCD_MSG_ASYNC_WAIT_VSYNC,
    FB_LCD_MSG_ASYNC_UPDATE,
};
int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int src_stride,
                        int dst_width, int dst_height, int dst_stride, int degree, int xoffset, int yoffset,
                        int in_format, int out_format, uint8_t mirror);
void fb_lcd_frame_buf_update(u8 id, u8 *frame_buffer);


static void __fb_buf_clear(u8 *frame_buffer, u32 frame_size, u32 format)
{
    if (format == FB_COLOR_FORMAT_YUV422) {
        for (int i = 0; i < frame_size; i += 4) {
            frame_buffer[i] = 0x00;
            frame_buffer[i + 1] = 0x80;
            frame_buffer[i + 2] = 0x00;
            frame_buffer[i + 3] = 0x80;
        }
        DcuFlushRegion((void *)frame_buffer, frame_size);
    }
}
static void __fb_lcd_set_buf_num(u8 num)
{
    lcd_buf_num = num;
}

static int __fb_lcd_line_done_hook_func(int line)
{
    return 0;
}

static u16 __fb_abs(int x, int y, u8 *flag)
{
    if ((x) > (y)) {
        if (flag) {
            *flag = 0;
        }
        return ((x) - (y)) ;
    }
    if (flag) {
        *flag = 1;
    }
    return ((y) - (x));
}

static int __fb_lcd_line_done_wait(u8 id, u8 comp, u8 wait_line)
{
    u32 cur_line = 0;
    if (!wait_line) {
#if 0
        // TODO : cur_line 计算受中断影响，不同场景会出现计算有误差,导致cur_line不准影响判断
        //只要当前dmm读的行数大于最小被追上的行,则不用等line pend
        if (g_dmm_line_period) {
            cur_line = (get_system_us() - g_vsync_start_time) / g_dmm_line_period ;
        }
        /* printf("cur line=%d\n",cur_line ); */
        if (comp == 0) {
            if (cur_line > g_dmm_line) {
                return 0;
            }
        }
#endif
    }
    if (__this->lcd) {
        dev_ioctl(__this->lcd, IOCTL_LCD_RGB_WAIT_LINE_FINISH, (u32)0);
    }
    return 0;
}

static void *__fb_lcd_frame_end_hook_func(void)
{
    u8 id = 0;
    if (!__this->next_disp) {
        return NULL; //软件还未渲染完整一帧，继续显示上一帧显存
    }
    void *cur_fb = __this->next_disp;
    __this->next_disp = NULL;

    return cur_fb;
}

//数据与lcd显存交换
static int __fb_lcd_frame_swap_flush(u8 id, u8 *frame_buffer)
{
    if (__this->lcd_kick_start == 0) {
        if (__this->lcd) {
            dev_ioctl(__this->lcd, IOCTL_LCD_RGB_START_DISPLAY, (u32)frame_buffer);
            __this->lcd_kick_start = 1;
        }
        return 0;
    }

    __this->next_disp = frame_buffer;
    dev_ioctl(__this->lcd, IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, (u32)frame_buffer);
    return 0;
}

//数据直接拷贝到lcd显存
static int __fb_lcd_frame_copy_flush(u8 id, u8 wait_line, u8 *frame_buffer, u16 x, u16 y)
{
#define CALC_CNT  60
    static u8 statistics_cnt = CALC_CNT;
    static u32 *dma2d_copy_use_times = NULL;
    static u16 lcd_vert_total = 0;
    struct lcd_dev_drive *lcd = NULL;
    struct imd_dev *imd;
    struct mipi_dev *mipi;

    u32 dma2d_copy_start_time;
    //1.等lcd line pending
    if (wait_line) {
        __fb_lcd_line_done_wait(id, 0, statistics_cnt);
    }
    dma2d_copy_start_time = get_system_us();
    //2.dma2d copy 推屏
    if (__this->interpolation_en == 0) {
        extern void dma2d_combine_copy_frame(u8 * dst, u8 * src, u16 x, u16 y, u16 src_w, u16 src_h, u16 dst_w, u16 dst_h, u16 format);
        dma2d_combine_copy_frame((u8 *)__this->out_buf[0], frame_buffer, __this->out_buf_x, __this->out_buf_y, __this->out_buf_w, __this->out_buf_h, __this->out_w, __this->out_h, __this->out_format);
    } else {
        //插值
        fb_frame_buf_scale((u8 *)__this->out_buf[0], __this->out_format, __this->out_w, __this->out_h, frame_buffer, __this->out_format, __this->out_buf_w, __this->out_buf_h, 0);
    }

    u32 _use_time = get_system_us() - dma2d_copy_start_time;
    if (statistics_cnt > 0) {
        if (wait_line && g_dmm_line_period) {
            if (dma2d_copy_use_times == NULL) {
                dma2d_copy_use_times = (u32 *)malloc(CALC_CNT * sizeof(u32));
            }
            dev_ioctl(__this->lcd, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
            lcd_vert_total = __this->out_h;
            if (__this->lcd_type == LCD_MIPI) {
                mipi = &lcd->dev->mipi;
                lcd_vert_total = mipi->video_timing.dsi_vdo_vsa_v + mipi->video_timing.dsi_vdo_vbp_v + mipi->video_timing.dsi_vdo_vact_v + mipi->video_timing.dsi_vdo_vfp_v;
            } else if (__this->lcd_type  == LCD_RGB) {
                imd = &lcd->dev->imd;
                lcd_vert_total = imd->timing.vert_total;
            } else {
                statistics_cnt--;
                return 0;
            }
            dma2d_copy_use_times[statistics_cnt - 1]  = _use_time;
            g_dmm_line = __fb_abs(lcd_vert_total, (_use_time / g_dmm_line_period), NULL);
            dmm_line_pend_init(g_dmm_line);
            statistics_cnt--;
            if (g_dmm_line_period && statistics_cnt == 0) {
                //去掉最大最小值并求平均
                int time_sum = 0;
                u32 max_time = 0;
                u32 min_time = 0xffffffff;
                for (int i = 0; i < CALC_CNT; i++) {
                    if (min_time > dma2d_copy_use_times[i]) {
                        min_time = dma2d_copy_use_times[i];
                    } else if (max_time < dma2d_copy_use_times[i]) {
                        max_time = dma2d_copy_use_times[i];
                    }
                    time_sum += dma2d_copy_use_times[i];
                }
                time_sum -= (min_time + max_time);
                g_dmm_line = __fb_abs(lcd_vert_total, (time_sum / (CALC_CNT - 2) / g_dmm_line_period), NULL);
                log_info("dma2d_copy_ava_use_time==%dus g_dmm_line_period=%d,g_dmm_line=%d\n", time_sum / (CALC_CNT - 2), g_dmm_line_period, g_dmm_line);
                dmm_line_pend_init(g_dmm_line);

                if (dma2d_copy_use_times) {
                    free(dma2d_copy_use_times);
                    dma2d_copy_use_times = NULL;
                }
            }
        }
    } else {
        if (g_dmm_line_period) {
            u32 dma2d_copy_line = __fb_abs(lcd_vert_total, (_use_time / g_dmm_line_period), NULL);
            static u8 err_cnt = 0;
            if (dma2d_copy_line > g_dmm_line + 3) { //允许有几行的误差
                if (++err_cnt > 2) {
                    log_warn("fb lcd dma2d copy and screen data rear-end! dma2d_copy_line=%d g_dmm_line=%d\n", dma2d_copy_line, g_dmm_line);
                    g_dmm_line = dma2d_copy_line;
                    dmm_line_pend_init(g_dmm_line);
                }
            } else {
                err_cnt = 0;
            }
        }
    }

    return 0;
}

//数据旋转至lcd显存
static int __fb_lcd_frame_rotate_copy_flush(u8 id, u8 wait_line, u8 *frame_buffer, u8 *out_buffer)
{
    static u8 rotate_slow = 0;  //0:旋转速度比推屏快  1:旋转速度比推屏慢 旋转90度出现耗时比推1帧时间多
#define CALC_CNT  10
    static u8 statistics_cnt = CALC_CNT;
    u16 src_w = __this->interpolation_en ? __this->out_buf_h : __this->out_h;
    u16 src_h = __this->interpolation_en ? __this->out_buf_w : __this->out_w;
    static u16 lcd_vert_total = 0;
    struct lcd_dev_drive *lcd = NULL;
    struct imd_dev *imd;
    struct mipi_dev *mipi;
    u32 rotate_use_time = 0;

    static u32 *rotate_times = NULL;

    //1.等lcd line pending
    if (wait_line) {
        __fb_lcd_line_done_wait(id, rotate_slow, statistics_cnt);
    }


    //2.旋转并推屏
    if (__this->rotate == ROTATE_180) {
        src_w = __this->out_w;
        src_h = __this->out_h;
    }
    rotate_use_time = fb_frame_buf_rotate(frame_buffer, (u8 *)out_buffer, src_w, src_h, 0, __this->out_w, __this->out_h, 0, __this->rotate, __this->out_buf_x, __this->out_buf_y, __this->out_format, __this->out_format, 0);

    //下面是旋转时间统计
    if (statistics_cnt > 0) {
        if (rotate_times == NULL) {
            rotate_times = (u32 *)malloc(CALC_CNT * sizeof(u32));
        }
        dev_ioctl(__this->lcd, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
        if (g_dmm_line_period) {
            lcd_vert_total = __this->out_h;
            if (__this->lcd_type == LCD_MIPI) {
                mipi = &lcd->dev->mipi;
                lcd_vert_total = mipi->video_timing.dsi_vdo_vsa_v + mipi->video_timing.dsi_vdo_vbp_v + mipi->video_timing.dsi_vdo_vact_v + mipi->video_timing.dsi_vdo_vfp_v;
            } else if (__this->lcd_type  == LCD_RGB) {
                imd = &lcd->dev->imd;
                lcd_vert_total = imd->timing.vert_total;
            } else {
                statistics_cnt--;
                return 0;
            }

            rotate_times[statistics_cnt - 1]  = rotate_use_time;
            g_dmm_line = __fb_abs(lcd_vert_total, (rotate_use_time / g_dmm_line_period), &rotate_slow);
            dmm_line_pend_init(g_dmm_line);
            //printf("lcd_vert_total=%d rotate_use_time====%dus g_dmm_line_period=%d g_dmm_line=%d\n", lcd_vert_total,rotate_use_time, g_dmm_line_period,g_dmm_line);
        }
        statistics_cnt--;
        if (g_dmm_line_period && statistics_cnt == 0) {
            //去掉最大最小值并求平均
            int rotate_time_sum = 0;
            u32 max_rotate_time = 0;
            u32 min_rotate_time = 0xffffffff;
            for (int i = 0; i < CALC_CNT; i++) {
                if (min_rotate_time > rotate_times[i]) {
                    min_rotate_time = rotate_times[i];
                } else if (max_rotate_time < rotate_times[i]) {
                    max_rotate_time = rotate_times[i];
                }
                rotate_time_sum += rotate_times[i];
            }
            rotate_time_sum -= (min_rotate_time + max_rotate_time);
            g_dmm_line = __fb_abs(lcd_vert_total, (rotate_time_sum / (CALC_CNT - 2) / g_dmm_line_period), &rotate_slow);
            log_info("rotate_ava_use_time==%dus g_dmm_line_period=%d,g_dmm_line=%d\n", rotate_time_sum / (CALC_CNT - 2), g_dmm_line_period, g_dmm_line);
            dmm_line_pend_init(g_dmm_line);

            if (rotate_times) {
                free(rotate_times);
                rotate_times = NULL;
            }
        }
    } else {
        //实时监测rotate有没和推屏追尾
        if (g_dmm_line_period) {
            u32 rotate_line = __fb_abs(lcd_vert_total, (rotate_use_time / g_dmm_line_period), &rotate_slow);
            static u8 err_cnt = 0;
            /* printf("rotate_time==%d\n",rotate_use_time); */
            /* printf("rotate_line==%d\n",rotate_line); */
            if (rotate_line > g_dmm_line + 3) { //允许有几行的误差
                if (++err_cnt > 2) {
                    log_info("fb lcd rotate and screen data rear-end! rotate_line=%d g_dmm_line=%d\n", rotate_line, g_dmm_line);
                    g_dmm_line = rotate_line;
                    dmm_line_pend_init(g_dmm_line);
                }
            } else {
                err_cnt = 0;
            }
        }
    }

    return 0;
}

#if FB_LCD_FRAME_RATE_DEBUG_EN
static int __fb_lcd_framerate_calc(u8 id)
{
    static u32 time[2];
    static u16 frames[2];
    u32 cur_time = 0;

    frames[id]++;
    cur_time = get_system_ms();
    if (time_after(cur_time, time[id])) {
        log_info("lcd[%d] update: %dfps", id, frames[id] * 1000 / (1000 + (cur_time - time[id])));
        time[id] = get_system_ms() + 1000;
        frames[id] = 0;
    }
    return 0;
}
#endif

#if (defined USE_LVGL_V8_UI_DEMO)
void dmm_vsync_int_handler(void)
{
    struct lcd_dev_drive *lcd = NULL;
    struct imd_dev *imd;
    struct mipi_dev *mipi;
    u8 id = 0;
#if 1 //跑几次，取最小值即可，然后可屏蔽代码写死行周期变量
    static u32 dmm_frame_period = 16000; //先给一个最小周期的值,单位微秒
    static u8 statistics_cnt = 0;
    u32 lcd_vert_total = 0;
    if (statistics_cnt < 10) {
        /* 计算dmm 读取一行最小周期 */
        ++statistics_cnt;
        dmm_frame_period = get_system_us() - g_last_vsync_trig_time;
        g_last_vsync_trig_time = get_system_us();
        if (__this->lcd && __this->out_buf_num == 1) {
            dev_ioctl(__this->lcd, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
            lcd_vert_total = __this->out_h;
            if (__this->lcd_type == LCD_MIPI) {
                mipi = &lcd->dev->mipi;
                lcd_vert_total = mipi->video_timing.dsi_vdo_vsa_v + mipi->video_timing.dsi_vdo_vbp_v + mipi->video_timing.dsi_vdo_vact_v + mipi->video_timing.dsi_vdo_vfp_v;
            } else if (__this->lcd_type == LCD_RGB) {
                imd = &lcd->dev->imd;
                lcd_vert_total = imd->timing.vert_total;
            }
            if (lcd_vert_total) {
                /* if (dmm_frame_period / lcd_vert_total < g_dmm_line_period || statistics_cnt <= 3) { */
                //g_dmm_line_period =  dmm_frame_period / lcd_vert_total;
                g_dmm_line_period = (dmm_frame_period + lcd_vert_total - 1) / lcd_vert_total; //向前取整
                /* } */
            }
            /* printf("dmm frame_period = %dus, %d line_period = %dus\n", dmm_frame_period, lcd_vert_total, g_dmm_line_period); */
        }
    }
#else
    g_dmm_line_period = 28;
#endif
    g_vsync_start_time = get_system_us(); //记录Vsync起始时间点
}
#endif

static void dump_frame_buffer(uint8_t *buffer, int buffer_size)
{
    static int fcnt = 0;
    char name[64];
    sprintf(name, CONFIG_ROOT_PATH"frame%d.dat", fcnt);
    fcnt++;
    /* if(fcnt == 100) { */
    if (fcnt >= 100 && fcnt <= 120) {
        FILE *f = fopen(name, "w+");
        if (f) {
            printf("write framw buffer %s\n", name);
            fwrite(buffer, buffer_size, 1, f);
            fclose(f);
        }
    }
}

static void *__fb_lcd_open_device(u8 id)
{
    struct lcd_dev_drive *lcd = NULL;
    //yuv422,rgb565,rgb888,argb888
    const u8 lcd_in_format[] = {
        FB_COLOR_FORMAT_YUV422,
        FB_COLOR_FORMAT_RGB565,
        FB_COLOR_FORMAT_RGB888,
        FB_COLOR_FORMAT_ARGB8888
    };
    if (__this->lcd == NULL) {
        __this->lcd = dev_open("lcd", (void *)&id);
        if (__this->lcd) {
            dev_ioctl(__this->lcd, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
            dev_ioctl(__this->lcd, IOCTL_LCD_RGB_SET_ISR_CB, (u32)__fb_lcd_frame_end_hook_func);
            __this->lcd_type = lcd->type;
            __this->out_buf_index = 0;

            __this->rotate = lcd->dev->imd.info.rotate;
            __this->out_buf_x = lcd->dev->imd.info.x;
            __this->out_buf_y = lcd->dev->imd.info.y;
            __this->interpolation_en = lcd->dev->imd.info.interpolation_en;
            if (__this->rotate == ROTATE_90 || __this->rotate == ROTATE_270) {
                __this->out_buf_w = lcd->dev->imd.info.yres;
                __this->out_buf_h = lcd->dev->imd.info.xres;
                __this->out_w = lcd->dev->imd.info.target_yres;
                __this->out_h = lcd->dev->imd.info.target_xres;
            } else {
                __this->out_buf_w = lcd->dev->imd.info.xres;
                __this->out_buf_h = lcd->dev->imd.info.yres;
                __this->out_w = lcd->dev->imd.info.target_xres;
                __this->out_h = lcd->dev->imd.info.target_yres;
            }
            __this->out_format = lcd_in_format[lcd->dev->imd.info.in_fmt];
            log_info(">>> lcd rotate en:%d\n", __this->rotate);
        }
    } else {
        dev_ioctl(__this->lcd, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
        __this->lcd_type = lcd->type;
        __this->out_buf_index = 0;
        __this->out_buf_x = lcd->dev->imd.info.x;
        __this->out_buf_y = lcd->dev->imd.info.y;
        __this->interpolation_en = lcd->dev->imd.info.interpolation_en;
        if (__this->rotate == ROTATE_90 || __this->rotate == ROTATE_270) {
            __this->out_buf_w = lcd->dev->imd.info.yres;
            __this->out_buf_h = lcd->dev->imd.info.xres;
            __this->out_w = lcd->dev->imd.info.target_yres;
            __this->out_h = lcd->dev->imd.info.target_xres;
        } else {
            __this->out_buf_w = lcd->dev->imd.info.xres;
            __this->out_buf_h = lcd->dev->imd.info.yres;
            __this->out_w = lcd->dev->imd.info.target_xres;
            __this->out_h = lcd->dev->imd.info.target_yres;
        }
        __this->out_format = lcd_in_format[lcd->dev->imd.info.in_fmt];
    }
    return __this->lcd;
}

static void __fb_lcd_async_push_task(void *p)
{
    int ret = 0;
    int msg[3] = {0};
    log_info("fb_lcd_async_push_task run...\n");
    u32 max_frame_period_us = 0;
    u32 frame_period_start_us = 0;
    u32 frame_period_us = 0;
    u8 id = (u8)p;
    while (1) {
        ret = os_taskq_pend_timeout(msg, ARRAY_SIZE(msg), 0);
        /* printf("ret=%d\n",ret ); */
        if (ret == OS_TASKQ)  {
            switch (msg[1]) {
            case FB_LCD_MSG_ASYNC_WAIT_VSYNC:
                /* printf("swap addr=%x\n",msg[2] ); */
                frame_period_start_us = get_system_us();
                __fb_lcd_frame_swap_flush(id, (u8 *)msg[2]);
#if FB_LCD_FRAME_RATE_DEBUG_EN
                frame_period_us = get_system_us() - frame_period_start_us;;
                if (max_frame_period_us < frame_period_us) {
                    max_frame_period_us = frame_period_us;
                    log_info("fb lcd max frame period : %dus", max_frame_period_us);
                }
                __fb_lcd_framerate_calc(id);
#endif
                break;
            case FB_LCD_MSG_ASYNC_UPDATE:
                fb_lcd_frame_buf_update(id, (u8 *)(msg[2]));
                os_sem_post(&__this->lcd_async_sem);
                if (os_sem_valid(&__this->lcd_async_busy_sem)) {
                    os_sem_set(&__this->lcd_async_busy_sem, 0);
                    os_sem_post(&__this->lcd_async_busy_sem);
                }
                break;
            }
        }
    }
}

/**
 * @brief   检查当前buf是否lcd正在显示的buf冲突
 * @param:  需要检查的buf地址
 * @return: 0:不冲突 非0: 冲突
 **/
int fb_lcd_buf_is_busy(u8 id, u8 *buf)
{
    u32 dmm_addr = 0;
    if (__this->out_buf_num > 1) {
        dmm_addr = dmm_addr_base;
        dmm_addr = CPU_ADDR(dmm_addr);
        if (buf == (u8 *)dmm_addr) {
            //合成输出地址和推屏地址冲突
            printf("fb combine outaddr:%x dmm_addr:%x\n", (unsigned int)buf, dmm_addr);
            __this->out_buf_index = !__this->out_buf_index;
            return -1;
        }
    }
    return 0;
}

/**
 * @brief   打开lcd显存并初始化lcd
 * @param:  out_info: 输出参数配置
 * @return: lcd 设备句柄
 **/
void *fb_lcd_device_open(struct fb_out_info *out_info)
{
    void *fd = NULL;
    u32 lcd_out_addr = 0;
    u32 out_buf_size;
    u8 id = out_info ? out_info->out_id : 0;
    if (__this->lcd == NULL) {
        // 打开lcd 设备
        fd = __fb_lcd_open_device(id);

        lcd_out_addr = __this->out_buf[0];
        if (__this->rotate) {
#if (FB_LCD_BUF_NUM <= 2)
            __fb_lcd_set_buf_num(1);
#else
            __fb_lcd_set_buf_num(2);
            if (__this->lcd_push_task_run == 0) {
                //创建线程,用于异步等待vsync的动作
                char async_lcd_task_name[20];
                sprintf(async_lcd_task_name, "async_lcd_task%d", id);
                thread_fork(async_lcd_task_name, 20, 1024, 256, 0, __fb_lcd_async_push_task, (void *)id);
                __this->lcd_push_task_run = 1;

            }
#endif
        } else {
            if (__this->interpolation_en || __this->out_buf_x || __this->out_buf_y) {
                __fb_lcd_set_buf_num(1);
            }
        }
        if (out_info) {
            __this->out_buf_num = (out_info->out_buf_num == 0) ? lcd_buf_num : out_info->out_buf_num;
            if (out_info->out_addr) {
                lcd_out_addr = (__this->out_buf[0]) ? __this->out_buf[0] : (u32)out_info->out_addr;
            }
        } else {
            __this->out_buf_num =  lcd_buf_num;
        }
        if (__this->out_buf_num == 1) {
            /* dev_ioctl(__this->lcd, IOCTL_LCD_RGB_SET_LINE_ISR_CB, (u32)__fb_lcd_line_done_hook_func); */
            if (g_dmm_line == 0) {
                g_dmm_line = __this->out_h - 16;
            }
            dmm_line_pend_init(g_dmm_line);
        }
        out_buf_size = __this->out_w * __this->out_h * dma2d_get_format_bpp(__this->out_format) / 8;

        // 申请lcd 显存
        for (int i = 0; i < __this->out_buf_num; i++) {
            if (!__this->out_buf[i]) {
                __this->out_buf[i] = (u32)zalloc(out_buf_size);
                log_info("fb_lcd_buffer_open out_buf[%d] malloc buf:%x size:%d\n", i, __this->out_buf[i], out_buf_size);
                ASSERT(__this->out_buf[i], "fb lcd out buffer malloc err!");
                lcd_out_addr = __this->out_buf[i];
            }
        }

        if (__this->lcd_type == LCD_MIPI || __this->lcd_type == LCD_RGB) {
            if (lcd_out_addr) {
                __fb_buf_clear((u8 *)lcd_out_addr, out_buf_size, __this->out_format);
                dev_ioctl(__this->lcd, IOCTL_LCD_RGB_START_DISPLAY, (u32)lcd_out_addr); //开始推屏显示
                __this->lcd_kick_start = 1;
            }
        }

    } else {
        return __this->lcd;
    }
    return fd;
}

/**
 * @brief   关闭lcd设备
 * @param:  none
 * @return:
 **/
int fb_lcd_device_close(u8 id)
{
    if (__this->lcd) {
        dev_close(__this->lcd);
        __this->lcd = NULL;
        __this->lcd_kick_start = 0;
    }
    return 0;
}
/**
 * @brief   异步推屏
 * @param:  id: 输出id
 * @param:  frame_buffer: 待更新帧数据
 * @return: none
 **/
void fb_lcd_frame_buf_async_flush(u8 id, u8 *frame_buffer)
{
    char async_lcd_task_name[20];
    int msg[3];
    msg[0] = FB_LCD_MSG_ASYNC_WAIT_VSYNC;
    msg[1] = (int)frame_buffer;
    sprintf(async_lcd_task_name, "async_lcd_task%d", id);
    int err =  os_taskq_post_type(async_lcd_task_name, Q_USER, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("fb_lcd_async_flush post err=%d\n", err);
    }
}

/**
 * @brief   更新lcd显示数据
 * @param:  id: lcd id号
 * @param:  frame_buffer: 待更新帧数据
 * @return: none
 **/
void fb_lcd_frame_buf_update(u8 id, u8 *frame_buffer)
{
#if 0 ////显存写卡调试
    int out_buf_size = __this->out_w * __this->out_h * dma2d_get_format_bpp(__this->out_format) / 8;
    dump_frame_buffer(frame_buffer, out_buf_size);
#endif

    static u32 max_frame_period_us = 0;
    u32 frame_period_us = 0;
    u32 frame_period_start_us = get_system_us();
    if (__this->out_buf_num == 1) {
        if (__this->rotate) {
            if (__this->lcd_type == LCD_MIPI || __this->lcd_type == LCD_RGB) {
                __fb_lcd_frame_rotate_copy_flush(id, 1, frame_buffer, (u8 *)__this->out_buf[0]);
            } else {
                //mcu/spi lcd
                __fb_lcd_frame_rotate_copy_flush(id, 0, frame_buffer, (u8 *)__this->out_buf[0]);
                __fb_lcd_frame_swap_flush(id, (u8 *)__this->out_buf[0]);
                goto _exit;
            }
        } else {
            if (__this->lcd_type == LCD_MIPI || __this->lcd_type == LCD_RGB) {
                __fb_lcd_frame_copy_flush(id, 1, frame_buffer, __this->out_buf_x, __this->out_buf_y);
            } else {
                //mcu/spi lcd
                __fb_lcd_frame_copy_flush(id, 0, frame_buffer, __this->out_buf_x, __this->out_buf_y);
                __fb_lcd_frame_swap_flush(id, (u8 *)__this->out_buf[0]);
                goto _exit;
            }
        }
    } else {
        if (__this->out_buf_num != 0) {
            if (__this->rotate) {
                u32 dmm_addr = dmm_addr_base;
                dmm_addr = CPU_ADDR(dmm_addr);
                if (__this->out_buf[__this->out_buf_index] == (u8 *)dmm_addr) {
                    __this->out_buf_index = !__this->out_buf_index;
                }
                __fb_lcd_frame_rotate_copy_flush(id, 0, frame_buffer, (u8 *)__this->out_buf[__this->out_buf_index]);
                fb_lcd_frame_buf_async_flush(id, (u8 *)__this->out_buf[__this->out_buf_index]);
                return;
            }
        }
        __fb_lcd_frame_swap_flush(id, frame_buffer);
    }
#if FB_LCD_FRAME_RATE_DEBUG_EN
    frame_period_us = get_system_us() - frame_period_start_us;;
    if (max_frame_period_us < frame_period_us) { //统计最大帧间隔
        max_frame_period_us = frame_period_us;
        log_info("fb lcd max frame period : %dus", max_frame_period_us);
    }
#endif

_exit:
#if FB_LCD_FRAME_RATE_DEBUG_EN
    __fb_lcd_framerate_calc(id);
#endif
}

int fb_lcd_frame_async_wait(u8 id)
{
    if (!os_sem_valid(&__this->lcd_async_sem)) {
        return 0;
    }
    if (!os_sem_valid(&__this->lcd_async_busy_sem)) {
        os_sem_create(&__this->lcd_async_busy_sem, 1);
    }

    return os_sem_pend(&__this->lcd_async_busy_sem, 50);
}
/**
 * @brief   异步lcd显示数据
 * @param:     id:输出id
 * @param:     frame_buffer:显示数据
 * @return:    0: ok  非0: error
 **/
int fb_lcd_frame_buf_update_async(u8 id, u8 *frame_buffer)
{
    char async_lcd_task_name[20];
    sprintf(async_lcd_task_name, "async_lcd_task%d", id);
    if (__this->lcd_push_task_run == 0) {
        //创建线程,用于异步推屏线程
        thread_fork(async_lcd_task_name, 20, 1024, 256, 0, __fb_lcd_async_push_task, (void *)id);
        __this->lcd_push_task_run = 1;

    }
    if (!os_sem_valid(&__this->lcd_async_sem)) {
        os_sem_create(&__this->lcd_async_sem, 1);
    }

    os_sem_pend(&__this->lcd_async_sem, 50);

    int err;
    int msg[3];
    msg[0] = FB_LCD_MSG_ASYNC_UPDATE;
    msg[1] = frame_buffer;
    err =  os_taskq_post_type(async_lcd_task_name, Q_USER, ARRAY_SIZE(msg), msg);
    if (err) {
        os_sem_post(&__this->lcd_async_sem);
        if (os_sem_valid(&__this->lcd_async_busy_sem)) {
            os_sem_post(&__this->lcd_async_busy_sem);
        }
    }

    return err;
}

/**
 * @brief   lcd显存交换
 * @param:  id: 输出id
 * @param:  frame_buffer:待交互的显存
 * @return: none
 **/
int fb_lcd_frame_buf_swap(u8 id, u8 *frame_buffer)
{
    int ret = 0;
    ret = __fb_lcd_frame_swap_flush(id, frame_buffer);
#if FB_LCD_FRAME_RATE_DEBUG_EN
    __fb_lcd_framerate_calc(id);
#endif
}
/**
 * @brief   获取lcd是否插值显示
 * @param:  none
 * @return: lcd插值与否
 **/
u8 fb_lcd_get_interpolation(u8 id)
{
    return __this->interpolation_en;
}
/**
 * @brief   获取lcd显存水平方向起始
 * @param:  none
 * @return: lcd显存水平起始坐标
 **/
u16 fb_lcd_get_buf_xoffset(u8 id)
{
    return __this->out_buf_x;
}

/**
 * @brief   获取lcd显存垂直方向起始
 * @param:  none
 * @return: lcd显存垂直起始坐标
 **/
u16 fb_lcd_get_buf_yoffset(u8 id)
{
    return __this->out_buf_y;
}

/**
 * @brief   获取lcd屏幕宽度
 * @param:  none
 * @return: lcd屏幕宽度
 **/
u16 fb_lcd_get_width(u8 id)
{
    return __this->out_w;
}
/**
 * @brief   获取lcd屏幕高度
 * @param:  none
 * @return: lcd屏幕高度
 **/
u16 fb_lcd_get_height(u8 id)
{
    return __this->out_h;
}

/**
 * @brief   获取lcd显存宽度
 * @param:  none
 * @return: lcd显存宽度
 **/
u16 fb_lcd_get_buf_width(u8 id)
{
    return __this->out_buf_w;
}
/**
 * @brief   获取lcd显存高度
 * @param:  none
 * @return: lcd显存高度
 **/
u16 fb_lcd_get_buf_height(u8 id)
{
    return __this->out_buf_h;
}

/**
 * @brief   获取lcd屏显输入格式
 * @param:  none
 * @return: lcd屏显输入格式
 **/
u16 fb_lcd_get_format(u8 id)
{
    return __this->out_format;
}
/**
 * @brief   获取lcd屏显旋转角度
 * @param:  none
 * @return: lcd屏显旋转角度
 **/
u16 fb_lcd_get_rotate(u8 id)
{
    return __this->rotate;
}

/**
 * @brief   获取lcd屏显显存buffer0
 * @param:  none
 * @return: lcd屏显显存buffer0
 **/
u32 fb_lcd_get_buf0(u8 id)
{
    return __this->out_buf[0];
}
/**
 * @brief   设置lcd屏显显存buffer0
 * @param:  id
 * @param:  buffer
 * @return: none
 **/
void fb_lcd_set_buf0(u8 id, u8 *buffer)
{
    __this->out_buf[0] = buffer;
}
/**
 * @brief   设置lcd屏显显存buffer1
 * @param:  id
 * @param:  buffer
 * @return: none
 **/
void fb_lcd_set_buf1(u8 id, u8 *buffer)
{
    __this->out_buf[1] = buffer;
}
/**
 * @brief   获取lcd屏显显存buffer1
 * @param:  none
 * @return: lcd屏显显存buffer1
 **/
u32 fb_lcd_get_buf1(u8 id)
{
    return __this->out_buf[1];
}
/**
 * @brief   获取lcd 空闲的显存buffer
 * @param:  none
 * @return: lcd 空闲的显存buffer
 **/
u32 fb_lcd_get_idle_buf(u8 id)
{
    return __this->out_buf[__this->out_buf_index];
}

/**
 * @brief   获取lcd 显存个数
 * @param:  none
 * @return: lcd 显存个数
 **/
u8 fb_lcd_get_buf_num(u8 id)
{
    return __this->out_buf_num;
}


/**
 * @brief   lcd双buf情况下，显存buf切换
 * @param:  none
 * @return: 切换后可用的lcd显存buf
 **/
u8 *fb_lcd_buf_index_swap(u8 id)
{
    /* 双输出缓存切换 */
    __this->out_buf_index = !__this->out_buf_index;
    return (u8 *)__this->out_buf[__this->out_buf_index];
}

/**
 * @brief   清除lcd显示数据
 * @param:  id: 输出id
 * @param:  none
 * @return: none
 **/
void fb_lcd_buf_clear(u8 id)
{
    u16 format = fb_lcd_get_format(id);
    u8 *addr = (u8 *)fb_combine_get_outbuf(); //获取fb合成输出的地址
    u16 out_width  = fb_lcd_get_width(id);
    u16 out_height = fb_lcd_get_height(id);
    uint32_t fb_get_format_bpp(uint32_t format);
    u32 size = out_width * out_height * fb_get_format_bpp(format) / 8;
    u32 ysize = out_width * out_height;
    u32 color = 0x008080;

    if (addr && (addr != 0x5a)) {
        switch (format) {
        case FB_COLOR_FORMAT_ARGB8888:
        case FB_COLOR_FORMAT_ARGB8565:
        case FB_COLOR_FORMAT_ARGB1555:
        case FB_COLOR_FORMAT_RGB888:
        case FB_COLOR_FORMAT_RGB565:
            memset(addr, 0x0, size);
            break;
        case FB_COLOR_FORMAT_YUV422:
            //UYVY
            for (int i = 0; i < size; i += 4) {
                addr[i] = 0x80;
                addr[i + 1] = 0x00;
                addr[i + 2] = 0x80;
                addr[i + 3] = 0x00;
            }
            break;
        case FB_COLOR_FORMAT_YUV422P:
            memset(addr, (color >> 16) & 0xff, ysize);
            memset(addr + ysize, (color >> 8) & 0xff, ysize / 2);
            memset(addr + ysize + ysize / 2, (color >> 0) & 0xff, ysize / 2);
            break;
        case FB_COLOR_FORMAT_YUV420P:
            memset(addr, (color >> 16) & 0xff, ysize);
            memset(addr + ysize, (color >> 8) & 0xff, ysize / 4);
            memset(addr + ysize + ysize / 4, (color >> 0) & 0xff, ysize / 4);
            break;
        }

        log_info("no fb ! clear lcd buf...\n");
        DcuFlushinvRegion((void *)addr, size);

        fb_lcd_frame_buf_update(id, addr);
    }
}
#endif

