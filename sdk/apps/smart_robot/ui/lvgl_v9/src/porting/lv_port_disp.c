/**
 * @file lv_port_disp.c
 *
 */
#include "lcd_config.h"

#ifdef USE_LVGL_V9_UI

/*********************
 *      INCLUDES
 *********************/
#define BOOL_DEFINE_CONFLICT
#include "lv_port_disp.h"
#include "src/display/lv_display_private.h"
#include "src/core/lv_global.h"
#include "video/fb.h"
#include "lcd_driver.h"


/*********************
 *      DEFINES
 *********************/

#define LV_DISP_DRV_MAX_NUM  (2)

#define LV_LCD_DISP_BUF_NUM  (1) //屏幕显存数
/**********************
 *      TYPEDEFS
 **********************/
#if LV_COLOR_DEPTH==16
#define LV_PIXEL_COLOR_T lv_color16_t
#elif LV_COLOR_DEPTH==24
#define LV_PIXEL_COLOR_T lv_color_t
#elif LV_COLOR_DEPTH==32
#define LV_PIXEL_COLOR_T lv_color32_t
#endif
/**********************
 *  STATIC PROTOTYPES
 **********************/

static void disp_init(uint8_t id);
static void *lv_lcd_frame_end_hook_func(void);
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

static u32 debug_draw_time_ms; //用于观察从开始渲染一帧开始到渲染完成一帧(推屏之前)需要的时间
static u64 debug_lcd_latency_us;//用于观察每帧推屏引起的延迟,如果有开TE,等TE时间也会计算进入,如果单纯评估测试DMA造成的时延,请关闭TE测试
static u32 debug_draw_max_time_ms;//用于观察记录从绘制完第一帧的第一片后开始推屏到绘制完最后一片消耗的历史最长时间
static uint8_t first_render[LV_DISP_DRV_MAX_NUM];
static uint32_t lcd_rotate_task_pid[LV_DISP_DRV_MAX_NUM];

/**********************
 *      MACROS
 **********************/

#define LV_UI_TRIPLE_BUFFER_EN   0//ui 3buffer使能 适配v9.3.0新特性

struct lv_fb_t {
    lv_display_t *disp;
    LV_PIXEL_COLOR_T *fb;
};
volatile static struct lv_fb_t next_disp[LV_DISP_DRV_MAX_NUM];/* 下一帧待显示的next_fb地址 */

static struct lv_fb_t rotate_disp_fh[LV_DISP_DRV_MAX_NUM];

static void *lcd_dev[LV_DISP_DRV_MAX_NUM];
static lv_display_t *lv_disp[LV_DISP_DRV_MAX_NUM];
static uint8_t *lcd_disp_buffer[LV_DISP_DRV_MAX_NUM][LV_LCD_DISP_BUF_NUM];
static uint16_t lcd_rotate[LV_DISP_DRV_MAX_NUM];
static uint16_t lcd_format[LV_DISP_DRV_MAX_NUM];
static volatile u8  g_dmm_line_period; //dmm读取一行所需时间
static volatile u32 g_last_vsync_trig_time;
static volatile u64 g_vsync_start_time;
static volatile u16 g_dmm_line;
static volatile u16 lcd_vert_total = 0;
static OS_SEM rotate_sem[LV_DISP_DRV_MAX_NUM];

#if LV_UI_TRIPLE_BUFFER_EN
static OS_SEM triple_buf_sem[LV_DISP_DRV_MAX_NUM];
#endif
/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_display_t *lv_port_get_disp(uint8_t id)
{
    lv_display_t *disp = lv_disp[id];
    if (disp == NULL) {
        return lv_disp[0];
    }
    return disp;
}
void lv_port_refr_now(lv_display_t *disp)
{
    lv_timer_t tmr = {0};
    /* lv_anim_refr_now(); */
    if (disp) {
        tmr.user_data = disp;
        lv_display_refr_timer(&tmr);
    } else {
        lv_display_t *d;
        d = lv_display_get_next(NULL);
        while (d) {
            tmr.user_data = d;
            lv_display_refr_timer(&tmr);
            d = lv_display_get_next(d);
        }
    }
}

void lv_port_disp_init(void)
{

    lv_display_t *disp = NULL;
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init(0);

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    disp = lv_display_create(LCD_W, LCD_H);
    disp->disp_id = 0;
    lv_disp[disp->disp_id] = disp;
    first_render[disp->disp_id] = 1;
    lv_display_set_flush_cb(disp, disp_flush);

#if LV_USE_PERF_MONITOR == 0 //只是为了适应LVGL_V9的benchmark帧率统计
    lv_timer_delete(disp->refr_timer);
    disp->refr_timer = NULL;
#endif

    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    static LV_PIXEL_COLOR_T buf_3_1[LCD_W * LCD_H] __attribute__((aligned(32)));            /*A screen sized buffer*/
    static LV_PIXEL_COLOR_T buf_3_2[LCD_W * LCD_H] __attribute__((aligned(32)));            /*Another screen sized buffer*/

    //RGB/MIPI屏幕配置FB必须是屏幕大小，使得LVGL内部直接渲染到FB对应的绝对坐标 && 内部会同步双BUF脏矩阵区域
    lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);


#if LV_UI_TRIPLE_BUFFER_EN
    static lv_draw_buf_t draw_buf3 = {0};
    static LV_PIXEL_COLOR_T buf_3_3[LCD_W * LCD_H] __attribute__((aligned(32)));
    lv_color_format_t cf = lv_display_get_color_format(disp);
    uint32_t stride = lv_draw_buf_width_to_stride(LCD_W, cf);
    lv_draw_buf_init(&draw_buf3, LCD_W, LCD_H, cf, stride, buf_3_3, sizeof(buf_3_3));
    lv_display_set_3rd_draw_buffer(disp, &draw_buf3);
    os_sem_create(&triple_buf_sem[disp->disp_id], 0);
#endif


#if TCFG_LCD_SUPPORT_MULTI_DRIVER_EN //双屏显示
    disp_init(1);
    disp = lv_display_create(LCD1_W, LCD1_H);
    disp->disp_id = 1;
    lv_disp[disp->disp_id] = disp;
    first_render[disp->disp_id] = 1;
    lv_display_set_flush_cb(disp, disp_flush);
#if LV_USE_PERF_MONITOR == 0 //只是为了适应LVGL_V9的benchmark帧率统计
    lv_timer_delete(disp->refr_timer);
    disp->refr_timer = NULL;
#endif
    static LV_PIXEL_COLOR_T buf_2_1[LCD1_W * LCD1_H] __attribute__((aligned(32)));            /*A screen sized buffer*/
    static LV_PIXEL_COLOR_T buf_2_2[LCD1_W * LCD1_H] __attribute__((aligned(32)));            /*Another screen sized buffer*/

    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_DIRECT);

#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static u16 __lcd_abs(int x, int y)
{
    if ((x) > (y)) {
        return ((x) - (y)) ;
    }
    return ((y) - (x));
}

static void *lv_lcd_frame_end_hook_func(void)
{
    u8 id = 0;
    if (!next_disp[id].fb) {
        return NULL; //软件还未渲染完整一帧，继续显示上一帧显存
    }
    lv_draw_buf_t *cur_fb = next_disp[id].fb;
    next_disp[id].fb = NULL;

#if LV_UI_TRIPLE_BUFFER_EN
    os_sem_post(&triple_buf_sem[id]);
#endif
    return cur_fb->data;
}

/**
 * @brief      获取到空闲的lcd显存
 * @param:     id: lcd id号
 * @param:     index: 当前正在使用的lcd buf 索引(0/1)
 *
 * @return:    返回空闲的lcd显存buffer
 **/
static uint8_t *lv_get_lcd_idle_buf(u8 id, u8 index)
{
#if (LV_LCD_DISP_BUF_NUM == 2)
    return lcd_disp_buffer[id][!index];
#else
    return lcd_disp_buffer[id][0];
#endif
}

#if LV_UI_TRIPLE_BUFFER_EN
//判断下一个渲染buffer是否繁忙(和推屏buffer冲突)
static u8 is_display_buffer_busy(u8 id, lv_display_t *disp)
{
    u32 dmm_addr = 0;
    dmm_addr = dmm_addr_base;
    lv_draw_buf_t *next_act;
    if (disp->buf_act == disp->buf_1) {
        next_act = disp->buf_2;
    } else if (disp->buf_act == disp->buf_2) {
        next_act = disp->buf_3 ? disp->buf_3 : disp->buf_1;
    } else {
        next_act = disp->buf_1;
    }
    if (lcd_rotate[id]) {
        if (next_act->data == (uint8_t *)rotate_disp_fh[id].fb) {
            return 1;
        }
    } else {
        if (next_act->data == (uint8_t *)CPU_ADDR(dmm_addr)) {
            return 1;
        }
    }

    return 0;
}
#endif

#if (LV_LCD_DISP_BUF_NUM == 1)
void dmm_vsync_int_handler(void)
{
    struct lcd_dev_drive *lcd = NULL;
    struct imd_dev *imd;
    struct mipi_dev *mipi;
    static u32 dmm_frame_period = 16000; //先给一个最小周期的值,单位微秒
    static u8 statistics_cnt = 0;
    u8 id = 0; //默认mipi/lcd 屏id号是0
    if (statistics_cnt < 10) {
        /* 计算dmm 读取一行最小周期 */
        ++statistics_cnt;
        dmm_frame_period = get_system_us() - g_last_vsync_trig_time;
        g_last_vsync_trig_time = get_system_us();
        if (lcd_dev[id] && lcd_rotate[id]) {
            dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
            lcd_vert_total = lcd->dev->imd.info.target_xres;
            if (lcd->type == LCD_MIPI) {
                mipi = &lcd->dev->mipi;
                lcd_vert_total = mipi->video_timing.dsi_vdo_vsa_v + mipi->video_timing.dsi_vdo_vbp_v + mipi->video_timing.dsi_vdo_vact_v + mipi->video_timing.dsi_vdo_vfp_v;
            } else if (lcd->type == LCD_RGB) {
                imd = &lcd->dev->imd;
                lcd_vert_total = imd->timing.vert_total;
            }
            if (lcd_vert_total) {
                g_dmm_line_period = (dmm_frame_period + (lcd_vert_total / 2)) / lcd_vert_total;//四舍五入
            }
        }
    }
    g_vsync_start_time = get_system_us(); //记录Vsync起始时间点
}
#endif

static void lcd_rotate_task(void *p)
{
#define CALC_CNT  60
    struct lcd_dev_drive *lcd = NULL;
    int ret = 0;
    u32 rotate_start_time, rotate_use_time;
    int msg[3] = {0};
    struct lv_fb_t *fh = (struct lv_fb_t *)p;
    uint8_t *frame_buffer = (uint8_t *)fh->fb;
    u8 statistics_cnt = CALC_CNT;
    u32 *rotate_times = NULL;
    u8 err_cnt = 0;
    lv_display_t *disp = fh->disp;
    u8 id = disp->disp_id;
    lv_draw_buf_t fb = {0};

    u8 lcd_buf_index = 0; //记录当前显示的lcd buf是哪一块

    u16 out_w = (lcd_rotate[id] == ROTATE_180) ? disp->hor_res : disp->ver_res;
    u16 out_h = (lcd_rotate[id] == ROTATE_180) ? disp->ver_res : disp->hor_res;

    dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);

    os_sem_create(&rotate_sem[id], 1);
    printf("lcd_rotate_task%d run...\n", id);
    rotate_start_time = get_system_us();
    fb_frame_buf_rotate(frame_buffer, lv_get_lcd_idle_buf(id, 0), disp->hor_res, disp->ver_res, 0, out_w, out_h, 0, lcd_rotate[id], 0, 0, lcd_format[id], lcd_format[id], 0);
    rotate_use_time = get_system_us() - rotate_start_time;
    if (g_dmm_line_period) {
        g_dmm_line = __lcd_abs(lcd_vert_total, (rotate_use_time / g_dmm_line_period));
    }
    dmm_line_pend_init(g_dmm_line);
    while (1) {
        ret = os_taskq_pend_timeout(msg, ARRAY_SIZE(msg), 0);
        if (ret == OS_TASKQ)  {
            frame_buffer = msg[1];
            rotate_disp_fh[id].fb = frame_buffer;
#if (LV_LCD_DISP_BUF_NUM == 1)
            //1. wait line pend
            if (lcd->type == LCD_MIPI || lcd->type == LCD_RGB) {
                if (lcd_dev[id]) {
                    dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_WAIT_LINE_FINISH, (u32)0);
                }
            }
            rotate_start_time = get_system_us();
#endif
            //2. rotate to lcd buffer
            fb_frame_buf_rotate(frame_buffer, lv_get_lcd_idle_buf(id, lcd_buf_index), disp->hor_res, disp->ver_res, 0, out_w, out_h, 0, lcd_rotate[id], 0, 0, lcd_format[id], lcd_format[id], 0);

#if (LV_LCD_DISP_BUF_NUM == 1)
            rotate_use_time = get_system_us() - rotate_start_time;
            //下面是旋转时间统计
            if (statistics_cnt > 0) {
                if (rotate_times == NULL) {
                    rotate_times = (u32 *)malloc(CALC_CNT * sizeof(u32));
                }
                if (g_dmm_line_period) {
                    rotate_times[statistics_cnt - 1]  = rotate_use_time;
                    g_dmm_line = __lcd_abs(lcd_vert_total, (rotate_use_time / g_dmm_line_period));
                    dmm_line_pend_init(g_dmm_line);
                    statistics_cnt--;
                    if (statistics_cnt == 0) {
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
                        g_dmm_line = __lcd_abs(lcd_vert_total, (rotate_time_sum / (CALC_CNT - 2) / g_dmm_line_period));
                        printf("rotate_ava_use_time==%dus g_dmm_line_period=%d,g_dmm_line=%d\n", rotate_time_sum / (CALC_CNT - 2), g_dmm_line_period, g_dmm_line);
                        dmm_line_pend_init(g_dmm_line);
                        if (rotate_times) {
                            free(rotate_times);
                            rotate_times = NULL;
                        }
                    }
                }
            } else {
                if (g_dmm_line_period) {
                    u32 rotate_line = __lcd_abs(lcd_vert_total, (rotate_use_time / g_dmm_line_period));
                    if (rotate_line > g_dmm_line + 3) {
                        if (++err_cnt > 2) {
                            g_dmm_line = rotate_line;
                            dmm_line_pend_init(g_dmm_line);
                        }
                    } else {
                        err_cnt = 0;
                    }
                }
            }
            if (!(lcd->type == LCD_MIPI || lcd->type == LCD_RGB)) {
                //mcu/spi屏
                if (lcd_dev[id]) {
                    dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, (u32)lcd_disp_buffer[id][0]);
                }
            }
            rotate_disp_fh[id].fb = NULL;
            os_sem_post(&rotate_sem[id]);

#elif (LV_LCD_DISP_BUF_NUM == 2)

            fb.data = lv_get_lcd_idle_buf(id, lcd_buf_index);
            next_disp[id].fb = (LV_PIXEL_COLOR_T *)&fb;
            dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, (u32)fb.data);
            lcd_buf_index = !lcd_buf_index;
            os_sem_post(&rotate_sem[id]);
#endif

        }
    }
}
static void lv_lcd_swap_fb(lv_display_t *disp_drv, const lv_area_t *area, LV_PIXEL_COLOR_T *px_map)
{
    u8 id = disp_drv->disp_id;
    char rotate_task_name[20];
    if (first_render[id] == 1) { //LVGL首次启动渲染
        first_render[id] = 0;
        if (lcd_rotate[id] != 0) {
            rotate_disp_fh[id].disp = disp_drv;
            rotate_disp_fh[id].fb = px_map;
            sprintf(rotate_task_name, "lcd_rotate_task%d", id);
            thread_fork(rotate_task_name, 20, 1024, 256, &lcd_rotate_task_pid[id], lcd_rotate_task, (void *)&rotate_disp_fh[id]);
        } else {
            dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_SET_ISR_CB, (u32)lv_lcd_frame_end_hook_func);
            dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_START_DISPLAY, (u32)LV_GLOBAL_DEFAULT()->disp_refresh->buf_act->data);
        }
        return;
    }
    if (lcd_rotate[id]) {
        int msg[1];
        msg[0] = (int)px_map;
        sprintf(rotate_task_name, "lcd_rotate_task%d", id);
#if LV_UI_TRIPLE_BUFFER_EN
        if (is_display_buffer_busy(id, disp_drv)) {
            os_sem_set(&rotate_sem[id], 0);
            os_sem_pend(&rotate_sem[id], 0);
        }
#else
        os_sem_pend(&rotate_sem[id], 0);
#endif
        os_taskq_post_type(rotate_task_name, Q_USER, ARRAY_SIZE(msg), msg);
        return;
    }

#if LV_UI_TRIPLE_BUFFER_EN
    if (is_display_buffer_busy(id, disp_drv)) {
        os_sem_set(&triple_buf_sem[id], 0);
        os_sem_pend(&triple_buf_sem[id], 0);
    }
#endif
    next_disp[id].fb = LV_GLOBAL_DEFAULT()->disp_refresh->buf_act;
    lv_draw_buf_t *cur_fb = next_disp[id].fb;

#if (LV_UI_TRIPLE_BUFFER_EN == 0)
    dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, (u32)cur_fb->data);
#endif

}
/*Initialize your display and the required peripherals.*/
void disp_init(uint8_t id)
{
    struct lcd_dev_drive *lcd = NULL;
    uint8_t i = 0;
    //yuv422,rgb565,rgb888,argb888
    const u8 lcd_in_format[] = {
        FB_COLOR_FORMAT_YUV422,
        FB_COLOR_FORMAT_RGB565,
        FB_COLOR_FORMAT_RGB888,
        FB_COLOR_FORMAT_ARGB8888
    };
    if (lcd_dev[id] == NULL) {
        lcd_dev[id] = dev_open("lcd", (void *)&id);
        dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
        lcd_rotate[id] = lcd->dev->imd.info.rotate;
        lcd_format[id] = lcd_in_format[lcd->dev->imd.info.in_fmt];
        if (lcd_rotate[id] != 0) {
            for (i = 0; i < LV_LCD_DISP_BUF_NUM; i++) {
                if (lcd_disp_buffer[id][i] == NULL) {
                    lcd_disp_buffer[id][i] = zalloc(lcd->dev->imd.info.target_xres * lcd->dev->imd.info.target_yres * sizeof(LV_PIXEL_COLOR_T));
                    DcuFlushRegion((void *)lcd_disp_buffer[id][i], lcd->dev->imd.info.target_xres * lcd->dev->imd.info.target_yres * sizeof(LV_PIXEL_COLOR_T));
                }
                printf("lvgl v9 lcd disp buffer [%d],%d x %d, malloc %d bytes at 0x%x.", i, lcd->dev->imd.info.target_xres, lcd->dev->imd.info.target_yres, lcd->dev->imd.info.target_xres * lcd->dev->imd.info.target_yres * sizeof(LV_PIXEL_COLOR_T), lcd_disp_buffer[id][i]);
            }
            dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_SET_ISR_CB, (u32)lv_lcd_frame_end_hook_func);
            dev_ioctl(lcd_dev[id], IOCTL_LCD_RGB_START_DISPLAY, (u32)lcd_disp_buffer[id][0]);
            if (i == 1) {
                os_time_dly(50); //为了稳定计算dmm读取一行所需时间
            }
        }
    }
}
void disp_uninit(uint8_t id)
{
    if (lcd_dev[id]) {
        dev_close(lcd_dev[id]);
        lcd_dev[id] = NULL;
    }
    for (int i = 0; i < LV_LCD_DISP_BUF_NUM; i++) {
        if (lcd_disp_buffer[id][i]) {
            free(lcd_disp_buffer[id][i]);
            lcd_disp_buffer[id][i] = NULL;
        }
    }
    if (lcd_rotate_task_pid[id]) {
        thread_kill(&lcd_rotate_task_pid[id], KILL_WAIT);
        lcd_rotate_task_pid[id] = 0;
    }
    first_render[id] = 1;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/

static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    u64 temp_system_us;
    /* printf("[%d]disp_flush->0x%x,%d,%d,%d,%d", disp_drv->disp_id, px_map, area->x1, area->x2, area->y1, area->y2); */
    if (LV_GLOBAL_DEFAULT()->disp_refresh->render_mode != LV_DISPLAY_RENDER_MODE_FULL) {

        if (LV_GLOBAL_DEFAULT()->disp_refresh->flushing_last) {

            u32 tmp_draw_time_ms = timer_get_ms() - lv_get_timer_handler_start_time_ms();
            debug_draw_time_ms += tmp_draw_time_ms;
            if (tmp_draw_time_ms > debug_draw_max_time_ms) {
                debug_draw_max_time_ms = tmp_draw_time_ms;
                printf("debug_draw_max_time_ms=%dms", debug_draw_max_time_ms);//注意由于中途打印,或者被其他高优先级任务抢占导致的统计不准
            }

            temp_system_us = get_system_us();
            DcuFlushRegion(px_map, disp_drv->hor_res * disp_drv->ver_res * sizeof(LV_PIXEL_COLOR_T));//这个地方可以优化，仅flush脏矩形部分的区域即可(需要从LVGL内部获取得到), 但是二维的，因此最好cache那边封装一个接口出来用
            //局部脏矩形刷新策略下，当一帧里面最后一个脏矩形被渲染完毕才切换帧BUF
            lv_lcd_swap_fb(disp_drv, area, px_map);
            debug_lcd_latency_us += (get_system_us() - temp_system_us);
        }
    } else {
        if (lv_get_timer_handler_start_time_ms()) { //过滤第一帧引入初始化屏等东西导致的延时不准
            u32 tmp_draw_time_ms = timer_get_ms() - lv_get_timer_handler_start_time_ms();
            debug_draw_time_ms += tmp_draw_time_ms;
            if (tmp_draw_time_ms > debug_draw_max_time_ms) {
                debug_draw_max_time_ms = tmp_draw_time_ms;
                printf("debug_draw_max_time_ms=%dms", debug_draw_max_time_ms);//注意由于中途打印,或者被其他高优先级任务抢占导致的统计不准
            }
        }
        temp_system_us = get_system_us();
        DcuFlushRegion(px_map, disp_drv->hor_res * disp_drv->ver_res * sizeof(LV_PIXEL_COLOR_T));
        lv_lcd_swap_fb(disp_drv, area, px_map);
        debug_lcd_latency_us += (get_system_us() - temp_system_us);
    }

    if (LV_GLOBAL_DEFAULT()->disp_refresh->flushing_last) {
#if 1
        static u32 time_lapse_hdl;
        static u8 fps_cnt;
        ++fps_cnt;
        u32 tdiff = time_lapse(&time_lapse_hdl, 1000);
        if (tdiff) {//注意由于中途打印,或者被其他高优先级任务抢占导致的统计不准
            printf("lv9 render %u mspf, lcd_latency %u mspf, render+flush %d fps\n", debug_draw_time_ms / fps_cnt, (u32)((debug_lcd_latency_us) / 1000 / fps_cnt), fps_cnt *  1000 / tdiff);
            debug_lcd_latency_us = 0;
            fps_cnt = 0;
            debug_draw_time_ms = 0;
        }
#endif
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp_drv);
}

#endif
