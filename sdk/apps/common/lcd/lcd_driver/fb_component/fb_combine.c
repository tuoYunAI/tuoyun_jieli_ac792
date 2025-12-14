/******************************************************************************
* File:             fb_combine.c
*
* Author:
* Created:          02/28/24
* Description:      1.fb 多图层合成功能
*****************************************************************************/
#include "system/init.h"
#include "lcd_driver.h"
#include "video/video.h"
#include "video/fb.h"
#include "asm/dma2d_driver.h"
#include "asm/jldma2d.h"
#include "fs/fs.h"
#include "asm/gpio.h"
#include "app_config.h"
#include "lcd_config.h"

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
#ifdef CONFIG_VIDEO_ENABLE
#include "pipeline_core.h"
#endif

#ifndef FB_LCD_BUF_NUM /*防止app_config.h 未定义*/
#error "Please define FB_LCD_BUF_NUM 2 in app_config.h"
#endif

#define FB_COMBINE_TIME_DEBUG_EN         0   //打印数据合成链路的时间消耗
#define FB_COMBINE_FRAME_DEBUG_EN        1   //打印FB合成丢帧帧数

#define FB_COMBINE_MAX_NUM  (5)
struct fb_combine_t {
    OS_SEM wait_sem;
    OS_MUTEX mutex;
    volatile u8 combine_task_run;
    volatile u8 combine_busy;
    u32 combine_out_buf[2];

    u8  fb_number;
    u16 ui_timer_id;
    u8 *map_backup_baddr;
    int fb_frame_cnt[FB_COMBINE_MAX_NUM];

    struct fb_map_user map_modify[FB_COMBINE_MAX_NUM];
};

static struct fb_combine_t _fb_combine[FB_MAX_OUT_NUM];
#define  __this   (&_fb_combine[id])

static spinlock_t fb_lock[FB_MAX_OUT_NUM] = {0};
static struct list_head head[FB_MAX_OUT_NUM]; //合成fb 链表
static u32 fb_combine_async_run[FB_MAX_OUT_NUM];
int (*fb_combine_out_cb_func)(void *, u32, u32);

#define _LVGL_UI_ENTER_FLUSH_TIMEOUT  (500)

enum {
    FB_COMBINE_STOP = 0, //合成停止
    FB_COMBINE_TASK_RUN, //以单独线程方式合成
    FB_COMBINE_FUNC_RUN, //独立函数,外部调用合成
};

enum {
    FB_COMBINE_OUT_USE_MAX_IMGBUF = 0x5a,
    FB_COMBINE_OUT_USE_ROTATE = 0x5b,
};

#define LAYER_GPU_SCALE  BIT(0)
#define LAYER_GPU_ROTATE BIT(1)
#define LAYER_GPU_MIRROR BIT(2)

u16 fb_lcd_get_rotate(u8 id);
u16 fb_lcd_get_width(u8 id);
u16 fb_lcd_get_height(u8 id);
u16 fb_lcd_get_format(u8 id);
u32 fb_lcd_get_buf0(u8 id);
u32 fb_lcd_get_buf1(u8 id);
u32 fb_lcd_get_idle_buf(u8 id);
u8 fb_lcd_get_buf_num(u8 id);
u16 fb_lcd_get_buf_width(u8 id);
u16 fb_lcd_get_buf_height(u8 id);
u8 fb_lcd_get_interpolation(u8 id);

int fb_lcd_buf_is_busy(u8 id, u8 *buf);
int fb_frame_buf_scale(uint8_t *dst, int dst_format, int dst_w, int dst_h,
                       uint8_t *src, int src_format, int src_w, int src_h, uint8_t mirror);
int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int src_stride,
                        int dst_width, int dst_height, int dst_stride, int degree, int xoffset, int yoffset,
                        int in_format, int out_format, uint8_t mirror);

int fb_frame_buf_mirror(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int dst_width, int dst_height, int mirror, int src_format, int dst_format);
int fb_combine_task(u8 id, void *priv);
/********************** DMA2D 图层合成接口 *****************************/
typedef struct {
    uint8_t *addr; /* 图层数据地址 */
    uint8_t format; /* 图层数据格式 */
    uint16_t x; /* 图层起始x坐标 */
    uint16_t y; /* 图层起始y坐标 */
    uint16_t width; /* 图层显存宽度 */
    uint16_t height; /* 图层显存高度 */
    uint16_t stride;
    uint16_t v_width;/* 图层在屏幕上实际显示宽度 */
    uint16_t v_height;/* 图层在屏幕上实际显示高度 */
    uint16_t rotate;
    uint16_t mirror;
    uint8_t alpha;
    uint8_t alpha_mode;
    uint8_t rbs;
    uint8_t color_key;
    uint32_t color;
} dma2d_layer_t;

/**
 * @brief    帧buffer 属性配置
 * @param:   in: 输入图层
 * @param:   p: fb 图层属性
 * @return:  none
 **/
static void __dma2d_in_layer_conf(dma2d_layer_t *in, struct fb_out_t *p)
{
    if (!in || !p) {
        return;
    }
    memset(in, 0x0, sizeof(*in));
    in->addr = p->map.baddr;
    in->format = p->map.format;
    in->width = p->map.width;
    in->height = p->map.height;
    in->v_width = p->map.real_width;
    in->v_height = p->map.real_height;
    in->rotate = p->map.rotate;
    in->mirror = p->map.mirror;
    in->x = p->map.xoffset;
    in->y = p->map.yoffset;

    if (in->format == JLDMA2D_FORMAT_YUV422_BT601) {
        in->rbs = 1;
    }
    if (in->format <= JLDMA2D_FORMAT_RGB565) {
        //过滤color key
        in->color_key = 1;
        /* in->color = 0; //纯黑色 */
        in->color = 0x52aaa5;//特殊颜色值为透明值
    }

}
/**
 * @brief    帧buffer 属性二次修改
 * @param:   id: lcd ID
 * @param:   in: 输入图层
 * @param:   p: fb 图层属性
 * @return:  none
 **/
static void __dma2d_in_layer_modify(u8 id, dma2d_layer_t *in, struct fb_out_t *p)
{
    struct fb_map_user *map_modify = NULL;
    if (!in || !p) {
        return;
    }

    /* 图层二次修改参数 */
    map_modify = &__this->map_modify[p->fb_name[2] - '0'];
    if ((map_modify->width && map_modify->height) || map_modify->rotate || map_modify->mirror) {
        in->width = map_modify->width;
        in->height = map_modify->height;
        in->v_width = map_modify->real_width;
        in->v_height = map_modify->real_height;
        in->rotate = map_modify->rotate;
        in->mirror = map_modify->mirror;
        in->x = map_modify->xoffset;
        in->y = map_modify->yoffset;
    }
}

/**
 * @brief    帧buffer 清屏
 * @param:   layer: 输入图层
 * @return:  0:成功 非0:失败
 **/
static int __dma2d_frame_buf_clear(dma2d_layer_t *layer)
{
    uint16_t _bpp = dma2d_get_format_bpp(layer->format) >> 3;
    uint8_t *dest = NULL;
    uint32_t clear_color = 0; //黑色
    dma2d_create_task();
    dest = layer->addr + (layer->y * layer->width + layer->x) * _bpp;
    dma2d_fill(dest, layer->width * _bpp, layer->format, 0, clear_color, layer->width, layer->height);

    return 0;
}
/**
 * @brief    帧buffer 混合,输入前景和输入背景用dma2d混合,输出到输出图层上
 * @param:   fg: 输入前景图层
 * @param:   bg: 输入背景图层
 * @param:   out:输出图层
 * @return:  0:成功 非0:失败
 **/
static int __dma2d_frame_buf_combine(dma2d_layer_t *fg, dma2d_layer_t *bg, dma2d_layer_t *out)
{
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};
    dma2d_out_layer_params_t out_layer_param = {0};
    uint16_t x = 0;
    uint16_t y = 0;
    if (!bg || !fg || !out) {
        return -1;
    }

    uint16_t _out_bpp = dma2d_get_format_bpp(out->format) >> 3;
    uint16_t _fg_bpp = dma2d_get_format_bpp(fg->format) >> 3;
    uint16_t _bg_bpp = dma2d_get_format_bpp(bg->format) >> 3;
    x = fg->x;
    y = fg->y;
    dma2d_create_task();
    out_layer_param.data = (uint8_t *)((uint32_t)out->addr + (y * out->width + x) * _out_bpp);
    out_layer_param.w = (fg->width < out->width) ? fg->width : out->width;
    out_layer_param.h = (fg->height < out->height) ? fg->height : out->height;
    out_layer_param.stride = (out->stride == 0) ? out->width * _out_bpp : out->stride;
    out_layer_param.format = out->format;
    out_layer_param.rbs = out->rbs;
    out_layer_param.dither_en = 1;

    fg_layer_param.data = (uint8_t *)fg->addr;
    fg_layer_param.stride = (fg->stride == 0) ? fg->width * _fg_bpp : fg->stride;
    fg_layer_param.format = fg->format;
    fg_layer_param.alpha = fg->alpha;
    fg_layer_param.alpha_mode = fg->alpha_mode;
    fg_layer_param.rbs = fg->rbs;
    fg_layer_param.colorkey = fg->color_key;
    fg_layer_param.color = fg->color;

    if (bg->addr == out->addr) {
        bg_layer_param.x = x;
        bg_layer_param.y = y;
    } else {
        bg_layer_param.x = bg->x;
        bg_layer_param.y = bg->y;
    }
    bg_layer_param.data = (uint8_t *)bg->addr;
    bg_layer_param.stride = (bg->stride == 0) ? bg->width * _bg_bpp : bg->stride;
    bg_layer_param.format = bg->format;
    bg_layer_param.alpha = bg->alpha;
    bg_layer_param.alpha_mode = bg->alpha_mode;
    bg_layer_param.rbs = bg->rbs;

    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);

    return 0;
}

/**
 * @brief    帧buffer 格式转换
 * @param:   in: 输入图层
 * @param:   out:输出图层
 * @return:  0:成功 非0:失败
 **/
static int __dma2d_frame_buf_fpc(dma2d_layer_t *in, dma2d_layer_t *out)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    uint8_t *o_dest = NULL;
    if (!in || !out) {
        return -1;
    }

    uint16_t _out_bpp = dma2d_get_format_bpp(out->format) >> 3;
    uint16_t _in_bpp = dma2d_get_format_bpp(in->format) >> 3;

    //输入和输出格式不同,需要做格式转换
    dma2d_create_task();
    fg_layer_param.data = in->addr;
    fg_layer_param.stride = in->width * _in_bpp;
    fg_layer_param.format = in->format;
    if (in->format == JLDMA2D_FORMAT_YUV422_BT601) {
        fg_layer_param.rbs = 1;
    }

    out_layer_param.data = (uint8_t *)((uint32_t)out->addr + (in->y * out->width + in->x) * _out_bpp);
    out_layer_param.stride = out->width * _out_bpp;
    out_layer_param.format = out->format;
    //imc的16位对齐要求，导致in和out的宽高可能会出现不一致而出现异常，因此这里设置为最小宽高
    out_layer_param.w = (in->width > out->width) ? out->width : in->width;
    out_layer_param.h = (in->height > out->height) ? out->height : in->height;
    out_layer_param.endian = 0;
    out_layer_param.rbs = 0;
    out_layer_param.dither_en = 1;
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_PFC);
    return 0;
}

/**
 * @brief    帧buffer 拷贝
 * @param:   in: 输入源图层
 * @param:   out:输出目标图层
 * @return:  0:成功 非0:失败
 **/
static int __dma2d_frame_buf_copy(dma2d_layer_t *in, dma2d_layer_t *out)
{
    uint8_t *o_dest = NULL;
    if (!in || !out) {
        return -1;
    }
    if (in->addr == out->addr) {
        //拷贝的地址相同不需要拷贝
        return 0;
    }

    if (in->format == JLDMA2D_FORMAT_YUV422_BT601) {
        //yuv需要2对齐
        in->x = in->x & ~1;
        in->y = in->y & ~1;
    }
    uint16_t _bpp = dma2d_get_format_bpp(in->format) >> 3;
    dma2d_create_task();
    o_dest = out->addr + (in->y * out->width + in->x) * _bpp;
    dma2d_copy(o_dest, out->width * _bpp,
               in->addr, in->width * _bpp,
               in->format, in->width, in->height);

    return 0;
}

/**
 * @brief    帧buffer 覆盖,将输入图层直接覆盖到输出图层
 * @param:   in: 输入图层
 * @param:   out:输出图层
 **/
static void __dma2d_frame_buf_cover(dma2d_layer_t *in, dma2d_layer_t *out)
{
    if (in->format != out->format) {
        //格式不一致时格式转换
        __dma2d_frame_buf_fpc(in, out);
    } else {
        //格式一致时启动copy
        __dma2d_frame_buf_copy(in, out);
    }
}

/**
 * @brief    运行dma2d任务
 *           需要和dma2d_reset_all_regs搭配使用
 * @param:   none
 * @return:  none
 **/
static void __dma2d_combine_run(void)
{
    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();
}

/**
 * @brief    判断dma2d合成中是否需要插入其他处理
 * @param:   in: 输入图层
 * @param:   out:输出图层
 * @return:  1: 需要  0:不需要
 **/
static int is_combine_layer_need_process(dma2d_layer_t *in, dma2d_layer_t *out)
{
    u8 need_process = 0;
    if (in->width != in->v_width || in->height != in->v_height) {
        //图层显存尺寸和实际显示尺寸不符,用gpu进行缩放
        need_process |= LAYER_GPU_SCALE;
    }
    if (in->rotate) {
        //图层旋转
        need_process |= LAYER_GPU_ROTATE;
    }
    if (in->mirror & VIDEO_VER_MIRROR) {
        //图层镜像
        need_process |= LAYER_GPU_MIRROR;
    }

    /* printf("need_process=%d\n",need_process); */
    return need_process;
}
/**
 * @brief    合成图层中插入其他处理
 * @param:   in: 输入图层
 * @param:   out:输出图层
 * @param:   process_type:处理类型
 * @return:  1: 处理成功  0:不需要处理
 **/
static int combine_layer_process(dma2d_layer_t *in, dma2d_layer_t *out, u8 process_type)
{
    uint8_t *out_data_addr = NULL;

    if (process_type == 0) {
        return 0;
    }

    int out_bpp = jlvg_get_image_format_bpp(out->format) >> 3;

    if (process_type == LAYER_GPU_MIRROR) {
        if (in->width + in->x > out->width || in->height + in->y > out->height) {
            log_error("layer combine mirror cross over! in:(%d,%d)(%d,%d) out:(%d,%d)", in->x, in->y, in->height, in->width, out->width, out->height);
            return 0;
        }
        out_data_addr = out->addr;
        if (in->x || in->y) {
            out_data_addr = out->addr + (in->y * out->width + in->x) * out_bpp;
        }
        fb_frame_buf_mirror(in->addr, out_data_addr, in->width, in->height, in->width, in->height, 2, in->format, out->format);
        return 1;
    }

    if ((process_type & LAYER_GPU_ROTATE) && (process_type & LAYER_GPU_SCALE)) {
        //旋转缩放
        out_data_addr = out->addr;
        if (in->x || in->y) {
            out_data_addr = out->addr + (in->y * out->width + in->x) * out_bpp;
        }

        fb_frame_buf_rotate(in->addr, out_data_addr, in->v_width, in->v_height, 0, in->width, in->height, out->width * out_bpp, in->rotate, 0, 0, in->format, out->format, in->mirror);
        return 0;
    }
    if (process_type & LAYER_GPU_SCALE) {
        //图层显存尺寸和实际显示尺寸不符,用gpu进行缩放
        u8 mirror = (process_type & LAYER_GPU_MIRROR) ? 1 : 0;
        fb_frame_buf_scale(out->addr, out->format, out->width, out->height, in->addr, in->format, in->width, in->height, mirror);
        return 1;
    }
    if (process_type & LAYER_GPU_ROTATE) {
        u16 w = in->height;
        u16 h = in->width;
        if (in->rotate == 180) {
            w = in->width;
            h = in->height;
        }
        if (w + in->x > out->width || h + in->y > out->height) {
            log_error("layer combine rotate cross over! in:(%d,%d)(%d,%d) out:(%d,%d)", in->x, in->y, in->height, in->width, out->width, out->height);
            return 0;
        }
        u8 mirror = (process_type & LAYER_GPU_MIRROR) ? 1 : 0;
        out_data_addr = out->addr + (in->y * out->width + in->x) * out_bpp;
        fb_frame_buf_rotate(in->addr, out_data_addr, in->width, in->height, 0,
                            w, h, out->width * out_bpp, in->rotate,
                            0, 0, in->format, out->format, mirror);
        return 1;
    }
    return 0;
}

/**
 * @brief    优化dma2d合成方法，只适用于某几个场景
 *           通用性不强所以单独一个函数实现
 * @param:   in:输入图层
 * @param:   out:输出图层
 * @param:   fb_nums:合成图层数
 * @return:  0: 不处理 非0:处理的图层数
 **/
static int dma2d_combine_optimize(dma2d_layer_t in[], dma2d_layer_t *out, int fb_nums)
{
    dma2d_layer_t *layer0; //fb0 ui
    dma2d_layer_t *layer;
    uint8_t *in0_addr;
    uint16_t in0_width;
    int fb_n = fb_nums;
    u8 other_process = 0;

    //场景1: 双路图像左右半屏显示+ UI单独全屏buffer显示
    if (fb_n == 3) {
        if ((in[fb_n - 1].v_width + in[fb_n - 2].v_width == out->width) && (in[fb_n - 1].v_height == out->height) && (in[fb_n - 2].v_height == out->height) &&
            in[0].v_width == out->width) {
            //UI全屏大小，其他fb加起来等于全屏大小的情况
            //将UI拆开左右2边分别和其他图像fb做blend, 最终输出一个全屏buffer大小，速度提升一倍
            layer0 = &in[0];//fb0 ui
            in0_addr = layer0->addr;
            in0_width = layer0->width;
            uint32_t layer0_bpp = dma2d_get_format_bpp(layer0->format) >> 3;
            layer0->stride = in0_width * layer0_bpp;
            for (int i = 0; i < 2; i++) {
                layer = &in[fb_n - i - 1];
                layer0->addr = in0_addr + (layer->y * in0_width + layer->x) * layer0_bpp;
                layer0->width = layer->width;
                layer0->x = layer->x;
                layer0->y = layer->y;

                layer->x = 0;
                layer->y = 0;

                layer->alpha = 253;
                layer->alpha_mode = JLDMA2D_REPLACE_ALPHA;
                __dma2d_frame_buf_combine(layer0, layer, out);
            }
            return fb_n;
        }
    }

#if 0
    //FIX ME: 下面场景需要先合成输出到全屏那一路buffer,当全屏那一路sensor帧间隔过短,容易出现合成模块和图像输出竞争同一块buffer的情况,出现显示闪屏
    //所以如果要使用该加速场景,要求sensor帧间隔尽可能拉长
    if (fb_n >= 3) {  //场景2: UI单独全屏buffer显示, 双路或三路图像同时显示,其中一路全屏显示
        layer = &in[fb_n - 1];//全屏那路非UI图层
        if ((layer->v_width == out->width && layer->v_height == out->height) && (in[0].v_width == out->width && in[0].v_height == out->height)) {
            other_process = is_combine_layer_need_process(layer, out);  //先判断全屏那路图层需不需要插入处理
            if (other_process == 0) {
                //1.将其他fb全部叠加到全屏那块fb图层
                while (fb_n > 2) {
                    other_process = is_combine_layer_need_process(&in[fb_n - 2], out);
                    if (other_process) {
                        //如果有其他插入处理，先输出dma2d任务链表处理
                        __dma2d_combine_run();
                        combine_layer_process(&in[fb_n - 2], layer, other_process);
                        fb_n--;
                        dma2d_reset_all_regs();
                        continue;
                    }
                    __dma2d_frame_buf_cover(&in[fb_n - 2], layer);
                    fb_n--;
                }
                //2.再将UI 图层和全屏的那块fb图层做blend
                __dma2d_frame_buf_combine(&in[0], layer, out);
                return fb_nums;
            }
        }
    }
#endif

    return 0;
}
static int dma2d_combine_method(u8 id, dma2d_layer_t in[], dma2d_layer_t *out, int fb_nums)
{
    int ret = 0;
    u8 other_process = 0;
    dma2d_reset_all_regs();
    if (fb_nums >= 2) {
        other_process = is_combine_layer_need_process(&in[fb_nums - 1], out); //读取是否有其他插入处理
        ret = dma2d_combine_optimize(in, out, fb_nums);
        if (ret) {
            fb_nums -= ret;
            goto __dma2d_second;
        }
        /* 最底层开始，由下至上混合 */
        /* 第一步*/
        if (other_process == 0 && (in[fb_nums - 1].width == out->width && in[fb_nums - 1].height == out->height) && (in[fb_nums - 2].width == out->width && in[fb_nums - 2].height == out->height)) {
            //前景背景和输出同尺寸,前景和背景直接混合输出,可以少做一次copy
            in[fb_nums - 1].alpha = 253;
            in[fb_nums - 1].alpha_mode = JLDMA2D_REPLACE_ALPHA;
            if (out->format == JLDMA2D_FORMAT_YUV422_BT601) {
                out->rbs = 1;
            }
            __dma2d_frame_buf_combine(&in[fb_nums - 2], &in[fb_nums - 1], out);
            in[fb_nums - 1].alpha = 0;
            in[fb_nums - 1].alpha_mode = 0;
            fb_nums -= 2;
        } else {
            //前景背景和输出不同尺寸,第一个图层覆盖到输出
            if (in[fb_nums - 1].width != out->width || in[fb_nums - 1].height != out->height) {
                __dma2d_frame_buf_clear(out);
            }
            if (other_process) {
                //如果有其他插入处理，先输出dma2d任务链表处理
                __dma2d_combine_run();
                combine_layer_process(&in[fb_nums - 1], out, other_process);
                fb_nums--;
                dma2d_reset_all_regs();
                goto __dma2d_second;
            }
            __dma2d_frame_buf_cover(&in[fb_nums - 1], out);
            fb_nums--;
        }
__dma2d_second:
        /* 第二步*/
        /* 图层(fg)逐个和上一次的输出(bg)做混合 */
        while (fb_nums) {
            other_process = is_combine_layer_need_process(&in[fb_nums - 1], out); //读取是否有其他插入处理
            if (in[fb_nums - 1].format == JLDMA2D_FORMAT_YUV422_BT601) {
                if (other_process) {
                    //如果有其他插入处理，先输出dma2d任务链表处理
                    __dma2d_combine_run();
                    combine_layer_process(&in[fb_nums - 1], out, other_process);
                    fb_nums--;
                    dma2d_reset_all_regs();
                    continue;
                }
                //YUV图层不需要混合,直接做格式转换或直接拷贝
                __dma2d_frame_buf_cover(&in[fb_nums - 1], out);
            } else {
                __dma2d_frame_buf_combine(&in[fb_nums - 1], out, out);
            }
            fb_nums--;
        }

    } else if (fb_nums == 1) { //fb图层只有一个时
        other_process = is_combine_layer_need_process(&in[0], out);
        if (__this->combine_task_run == FB_COMBINE_TASK_RUN) {
            if (other_process == 0 && (fb_lcd_get_rotate(id) == ROTATE_90 || fb_lcd_get_rotate(id) == ROTATE_270)) {
                dma2d_layer_t combine_out = {0};
                other_process = LAYER_GPU_ROTATE;
                __dma2d_combine_run();
                memcpy(&combine_out, out, sizeof(dma2d_layer_t));
                u16 tmp = combine_out.width;
                combine_out.width = combine_out.height;
                combine_out.height = tmp;
                in[0].rotate = fb_lcd_get_rotate(id);
                fb_frame_buf_rotate_set_colorkey(1, 0x52, 0xaa, 0xa5); //过滤colorkey
                combine_layer_process(&in[0], &combine_out, other_process);
                fb_frame_buf_rotate_set_colorkey(0, 0, 0, 0);
                return FB_COMBINE_OUT_USE_ROTATE;
            }
        }
        if (in[0].format == JLDMA2D_FORMAT_YUV422_BT601) {
            //YUV图层不需要混合,直接做格式转换或直接拷贝
            if (in[0].width != out->width || in[0].height != out->height) {
                __dma2d_frame_buf_clear(out);
            }
            if (other_process) {
                //如果有其他插入处理，先输出dma2d任务链表处理
                __dma2d_combine_run();
                combine_layer_process(&in[0], out, other_process);
                return 0;
            }
            __dma2d_frame_buf_cover(&in[0], out);
        } else {
            __dma2d_frame_buf_clear(out);
            __dma2d_frame_buf_combine(&in[0], out, out);
        }
    } else if (fb_nums == -1) {
        __dma2d_frame_buf_clear(out);
    }

__exit:
    __dma2d_combine_run();

    return 0;
}

//dma2d copy 接口
void dma2d_combine_copy_frame(u8 *dst, u8 *src, u16 x, u16 y, u16 src_w, u16 src_h, u16 dst_w, u16 dst_h, u16 format)
{
    dma2d_layer_t src_layer = {0};
    dma2d_layer_t dst_layer = {0};
    src_layer.addr = src;
    src_layer.format = format;
    src_layer.x = x;
    src_layer.y = y;
    src_layer.width = src_w;
    src_layer.height = src_h;
    dst_layer.addr = dst;
    dst_layer.width = dst_w;
    dst_layer.height = dst_h;
    dma2d_init();
    dma2d_reset_all_regs();
    __dma2d_frame_buf_copy(&src_layer, &dst_layer);
    __dma2d_combine_run();
}
/*********************************************************/

static u8 __is_need_create_combine_task(u8 open_fb, struct fb_draw_info *info)
{
    u16 lcd_w = 0;
    u16 lcd_h = 0;
    u8 id = info->out_id;

#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
#if (LV_DISP_UI_FB_NUM <= 1)
    __this->combine_task_run = FB_COMBINE_FUNC_RUN;
    return 0;
#elif (LV_DISP_UI_FB_NUM >= 2 && FB_LCD_BUF_NUM)
    return 1;
#endif
#else
    if (fb_lcd_get_buf_num(id) == 1) {
        __this->combine_task_run = FB_COMBINE_FUNC_RUN;
        return 0;
    }
#endif

    if (fb_lcd_get_rotate(id) == ROTATE_90 || fb_lcd_get_rotate(id) == ROTATE_270) {
        lcd_w  = fb_lcd_get_height(id);
        lcd_h = fb_lcd_get_width(id);
    } else {
        lcd_w  = fb_lcd_get_width(id);
        lcd_h = fb_lcd_get_height(id);
    }
    u16 lcd_format = fb_lcd_get_format(id);

    if (open_fb > 1) {
        return 1;
    }
    if (strcmp(info->name, "fb0")) {
        if (lcd_w == info->width && lcd_h == info->height && lcd_format == FB_COLOR_FORMAT_YUV422) {
            //单全屏显示图像,lcd显存格式为yuv422,不需要创建合成任务,可以直接推屏
            return 0;
        }
        return 1;
    }

    if (!strcmp(info->name, "fb0")) {
        if (info->fb_num >= 2 && (lcd_w != info->width || lcd_h != info->height)) {
            //UI渲染尺寸小于屏尺寸
            return 1;
        }
    }

    return 0;
}
/* 检查合成输出的fb分辨率是否全屏 */
static int __check_max_fb_is_support(dma2d_layer_t *layer, u16 lcd_w, u16 lcd_h)
{
    if (layer) {
        /* printf("layer w=%d h=%d out_w:%d out_h:%d\n",layer->width,layer->height,lcd_w,lcd_h); */
        if (layer->width == lcd_w && layer->height == lcd_h) {
            return 1;
        }
    }
    /* ASSERT(0); */
    return 0;
}

static int __fb_buf_is_lock(struct fb_out_t *p, u8 *baddr)
{
    u8 has_unlock = 1;
#if (FB_LCD_BUF_NUM == 1)
    if (!strcmp(p->fb_name, "fb0")) {
        return 0;
    }
    has_unlock = 0;
    for (int i = 0; i < p->buf_num; i++) {
        if (p->buf_addr[i] != baddr) {
            if (p->buf_lock[i] == 0) {
                has_unlock = 1;
                break;
            }
        }
    }
#endif
    return !has_unlock;
}
static int __fb_buf_lock(struct fb_out_t *p, u8 *baddr)
{
#if (FB_LCD_BUF_NUM == 1)
    for (int i = 0; i < p->buf_num; i++) {
        if (p->buf_addr[i]) {
            if (p->buf_addr[i] == baddr) {
                p->buf_lock[i] = 1;
                return 0;
            }
        }
    }
#endif
    return 0;
}
static int __fb_head_buf_unlock(u8 id, u8 *baddr)
{
#if (FB_LCD_BUF_NUM == 1)
    struct fb_out_t *p = NULL;
    spin_lock(&fb_lock[id]);
    list_for_each_entry(p, &head[id], entry) {
        for (int i = 0; i < p->buf_num; i++) {
            if (p->buf_addr[i]) {
                if (p->buf_addr[i] == baddr) {
                    p->buf_lock[i] = 0;
                    break;
                }
            }
        }
    }
    spin_unlock(&fb_lock[id]);
#endif
    return 0;
}

/**
 * @brief    检查fb是否有图层需要合成
 * @param:   num:返回需要合成的图层数
 * @return:  返回最大的fb图层
 **/
static struct fb_out_t *__fb_combine_buf_check(u8 id, int *num)
{
    int max_map_size = 0;
    int fb_n = 0;
    struct fb_out_t *p = NULL;
    struct fb_out_t *p_max = NULL;
    spin_lock(&fb_lock[id]);
    list_for_each_entry(p, &head[id], entry) {
        if (p->fb_name[2] - '0' != 0) {
            if (p->map.baddr) {
                /* 找到最大尺寸的图层 */
                if (max_map_size < p->map.width * p->map.height) {
                    max_map_size =  p->map.width * p->map.height;
                    p_max = p;
                }
                fb_n++;
            }
        }
    }
    spin_unlock(&fb_lock[id]);

    *num = fb_n;

    return p_max;
}

static int fb_check_gpu_plugin_is_busy(void)
{
#ifdef CONFIG_VIDEO_ENABLE
    /* 获取gpu 插件繁忙状态 */
    for (int i = 0; i < 4; i++) {
        char *gpu_source[] = {"gpu0", "gpu1", "gpu2", "gpu3"};
        pipe_plugin_t *gpu_plugin = plugin_factory_find_used(gpu_source[i]);
        if (gpu_plugin) {
            u8 gpu_task_busy = *(u8 *)(((u32 *)gpu_plugin->private_data));
            u32 time = get_system_ms_in_irq() + 1 * 1000;
            while (gpu_task_busy) { /* 如果繁忙则等待 */
                gpu_task_busy = *(u8 *)(((u32 *)gpu_plugin->private_data));
                if (time_after(get_system_ms_in_irq(), time)) {
                    log_error("gpu_plugin task busy timeout!");
                    break;
                }
            }
        }
    }
#endif
}
static int dump_combine_frame_buffer(dma2d_layer_t in[], dma2d_layer_t *out, int fb_nums)
{
    FILE *f = NULL;
    int buffer_size;
    static int fcnt = 0;
    char name[64];
    fcnt++;
    if (fcnt >= 100 && fcnt <= 120) {
        //dump in
        for (int i = 0; i < fb_nums; i++) {
            memset(name, 0, sizeof(name));
            sprintf(name, CONFIG_ROOT_PATH"in%d_frame%d.dat", i, fcnt);
            f = fopen(name, "w+");
            if (f) {
                printf("write frame %s\n", name);
                buffer_size = in[i].width * in[i].height * dma2d_get_format_bpp(in[i].format) / 8;
                fwrite(in[i].addr, buffer_size, 1, f);
                fclose(f);
                f = NULL;
            }
        }

        //dump out
        memset(name, 0, sizeof(name));
        sprintf(name, CONFIG_ROOT_PATH"out_frame%d.dat", fcnt);
        f = fopen(name, "w+");
        if (f) {
            printf("write out frame %s\n", name);
            buffer_size = out->width * out->height * dma2d_get_format_bpp(out->format) / 8;
            fwrite(out->addr, buffer_size, 1, f);
            fclose(f);
            f = NULL;
        }
    }
    return 0;
}

static void ui_set_flush_mode_timer_cb(void *p)
{
    struct fb_out_t *p_max = NULL;
    u8 id = (u8)p;
    int ret = 0;
    u8 mode = 1;
    u8 *buf0 = (u8 *)fb_lcd_get_buf0(id);
    u8 *buf1 = (u8 *)fb_lcd_get_buf1(id);
    if (fb_lcd_get_buf_num(id) == 1) {
        buf0 = (u8 *)__this->combine_out_buf[0];
        buf1 = NULL;
    }
#if (FB_LCD_BUF_NUM == 3)
    if (fb_lcd_get_rotate(id)) {
        buf0 = (u8 *)__this->combine_out_buf[0];
        buf1 = NULL;
    }
#endif
    if (buf0 == NULL && buf1 == NULL) {
        p_max = __fb_combine_buf_check(id, &ret);
        if (ret == 0) {
            mode = 2;//告诉UI需要自己额外申请一块buf
        }
        if (p_max) {
            p_max->ready_combine = 1;
            u8 *cur_map_addr = p_max->map.baddr;
            for (int i = 0; i < p_max->buf_num; i++) {
                if (p_max->buf_addr[i] != cur_map_addr) {
                    __this->map_backup_baddr = p_max->buf_addr[i];
                }
            }
            //确保fb 2块buf有同一帧数据
            log_info("image_num:%d map image backup addr:%x\n", ret, __this->map_backup_baddr);
            /* dma2d_combine_copy_frame(p_max->map.baddr, __this->map_backup_baddr, p_max->map.width, p_max->map.height, p_max->map.format); */
            buf0 = cur_map_addr;
            buf1 = NULL;
        }
    }

    if (__this->ui_timer_id) {
        sys_timer_modify(__this->ui_timer_id, 0xffffff);
    }
    log_info("image not change! ui set self flush mode%d now buf0=%x buf1=%x\n", mode, (unsigned int)buf0, (unsigned int)buf1);
    if (buf0 && buf1) {
        //判断是否和lcd推屏buf冲突
        u32 dmm_addr = 0;
        dmm_addr = dmm_addr_base;
        dmm_addr = CPU_ADDR(dmm_addr);
        if (buf1 == CPU_ADDR(dmm_addr)) {
            dmm_addr = buf1;
            buf1 = buf0;
            buf0 = dmm_addr;
        }
    }
    lvgl_set_ui_flush_mode(mode, (void *)buf1, (void *)buf0); /* lvgl UI自己刷新 */
}

int fb_combine_mutex_enter(u8 id)
{
    return os_mutex_pend(&__this->mutex, 0);
}
void fb_combine_mutex_exit(u8 id)
{
    os_mutex_post(&__this->mutex);
}

static void _fb_combine_async(void *priv)
{
    u8 id = (u8)priv;
    fb_combine_task((u8)id, NULL);
    --fb_combine_async_run[id];
}

/**
 * @brief      异步执行fb合成函数
 * @param:     none
 * @return:    0: ok  非0: error
 **/
int run_fb_combine_async(u8 id)
{
    if (fb_combine_async_run[id]) {
        return -1;
    }

    ++fb_combine_async_run[id];

    int err;
    int msg[3];
    msg[0] = (int)_fb_combine_async;
    msg[1] = 1;
    msg[2] = (int)id;
    err =  os_taskq_post_type("sys_timer", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    if (err) {
        --fb_combine_async_run[id];
    }
    return err;
}

/**
 * @brief      设置fb合成模块输出回调
 * @param:     arg:回调函数地址
 * @return:    none
 **/
void fb_combine_set_out_cb(u32 arg)
{
    fb_combine_out_cb_func = (void *)arg;
}

#if (defined USE_LVGL_V8_UI_DEMO)
static void ui_timer_modify(u8 id)
{
#if (LV_DISP_UI_FB_NUM <= 1)
    if (__this->ui_timer_id) {
        sys_timer_modify(__this->ui_timer_id, _LVGL_UI_ENTER_FLUSH_TIMEOUT);
    }
#endif
}
#endif


/**
 * @brief   fb 合成模块输出设置
 * @param:  none
 * @return: none
 **/
static void fb_combine_set_outbuf(u8 id)
{
    u32 out_buf_size = 0;
    u32 lcd_buf_w, lcd_buf_h;

#if (FB_LCD_BUF_NUM <= 2)
    if (fb_lcd_get_buf_num(id) > 1) { // always no rotate
        __this->combine_out_buf[0] = fb_lcd_get_buf0(id);
        __this->combine_out_buf[1] = fb_lcd_get_buf1(id);
        log_info("fb_combine out buffer :%x %x\n", __this->combine_out_buf[0], __this->combine_out_buf[1]);
    } else if (fb_lcd_get_buf_num(id) == 1) { // always rotate
#if (FB_LCD_BUF_NUM != 1)
        if (__this->combine_out_buf[0] == 0) {
            lcd_buf_w = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_width(id) : fb_lcd_get_width(id);
            lcd_buf_h  = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_height(id) : fb_lcd_get_height(id);
            out_buf_size = lcd_buf_w * lcd_buf_h * dma2d_get_format_bpp(fb_lcd_get_format(id)) / 8;
            __this->combine_out_buf[0] = (u32)zalloc(out_buf_size);
            log_info("fb_combine out buffer alloc :%x size:%d\n", __this->combine_out_buf[0], out_buf_size);
            ASSERT(__this->combine_out_buf[0], "fb_combine out buffer alloc err!");
        }
#endif /*FB_LCD_BUF_NUM != 1*/
    }

#elif (FB_LCD_BUF_NUM == 3)

    if (__this->combine_out_buf[0] == 0) {
        out_buf_size = fb_lcd_get_width(id) * fb_lcd_get_height(id) * dma2d_get_format_bpp(fb_lcd_get_format(id)) / 8;
        __this->combine_out_buf[0] = zalloc(out_buf_size);
        log_info("fb_combine out other buffer alloc :%x size:%d\n", __this->combine_out_buf[0], out_buf_size);
        ASSERT(__this->combine_out_buf[0], "fb_combine out buffer alloc err!");
        __this->combine_out_buf[1] = 0;
    }
#endif
}

/**
 * @brief      获取fb合成模块输出buf地址
 * @param:     id
 * @return:    fb合成模块输出buf地址
 **/
u8 *fb_combine_get_outbuf(u8 id)
{
    if (__this->combine_out_buf[0] && __this->combine_out_buf[1]) {
        return (u8 *)fb_lcd_get_idle_buf(id);
    } else if (__this->combine_out_buf[0]) {
        return (u8 *)__this->combine_out_buf[0];
    }

    return (u8 *)FB_COMBINE_OUT_USE_MAX_IMGBUF; //以最大的imc图像层作为合成输出
}

/**
 * @brief      交换fb合成模块输出buffer和LCD buffer交换
 * @param:     id
 * @return:    fb合成模块输出buf地址
 **/
u8 *fb_combine_swap_outbuf(u8 id, u8 *addr)
{
    __this->combine_out_buf[0] = fb_lcd_get_buf0(id);
    fb_lcd_set_buf0(0, addr);
    return (u8 *)__this->combine_out_buf[0];
}
/**
 * @brief       图层合成任务
 *              当__this->combine_task_run == FB_COMBINE_STOP,该任务不跑
 *              当__this->combine_task_run == FB_COMBINE_FUNC_RUN,该任务当普通函数使用,只跑一次,由外部调用
 *              当__this->combine_task_run == FB_COMBINE_TASK_RUN,该任务是一个线程
 * @param:      id :合成id
 * @param:      priv :合成的输出buffer; NULL时则内部自己选择输出buf
 *
 * @return:     返回合成的图层数
 **/
int fb_combine_task(u8 id, void *priv)
{
    struct fb_out_t *p = NULL;
    struct fb_out_t *p1 = NULL;
    dma2d_layer_t in[FB_COMBINE_MAX_NUM] = {0};
    dma2d_layer_t out = {0};
    struct fb_map_user omap = {0};
    int fb_n = 0;
    int ret = 0;
    int dret = 0;
    int index = -1;
    dma2d_init();
    u32 dmm_addr = 0;

    u32 dma2d_combine_use_time = 0;
    u32 ui_use_time = 0;
    u32 lcd_use_time = 0;
    while (1) {
        fb_combine_mutex_enter(id);
        if (__this->combine_task_run == FB_COMBINE_STOP) {
            fb_combine_mutex_exit(id);
            break;
        }
        if (!out.addr) {
            out.format = fb_lcd_get_format(id);
            if (fb_lcd_get_rotate(id)) {
                if (fb_lcd_get_rotate(id) == ROTATE_90 || fb_lcd_get_rotate(id) == ROTATE_270) {
                    out.width  = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_height(id) : fb_lcd_get_height(id);
                    out.height = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_width(id) : fb_lcd_get_width(id);
                } else {
                    out.width  = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_width(id) : fb_lcd_get_width(id);
                    out.height = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_height(id) : fb_lcd_get_height(id);
                }
            } else {
                out.width  = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_width(id) : fb_lcd_get_width(id);
                out.height = fb_lcd_get_interpolation(id) ? fb_lcd_get_buf_height(id) : fb_lcd_get_height(id);
            }
            if (priv) {
                out.addr = (uint8_t *)priv;
            } else {
                out.addr = (u8 *)fb_combine_get_outbuf(id); //获取fb合成输出的地址
            }
            //检查out地址是否和当前lcd推屏地址冲突
            if (fb_lcd_buf_is_busy(id, out.addr)) {
                goto __combine_exit;
            }
            out.alpha = 253;
            out.alpha_mode = JLDMA2D_REPLACE_ALPHA;
        }

        /* 检查gpu状态是否运行,并且进行繁忙等待 */
        fb_check_gpu_plugin_is_busy();

        __this->combine_busy = 1;

        /* 对需要合成FB进行配置 */
        u8 need_combine = 0;
        int max_map_size = 0;
        spin_lock(&fb_lock[id]);
        fb_n = 0;
        list_for_each_entry(p, &head[id], entry) {
            if (p->map.baddr) {
                if (p->ready_combine) {
                    need_combine++;
#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
#if (LV_DISP_UI_FB_NUM != 0 || FB_LCD_BUF_NUM == 1)
                    p->ready_combine = 0;
#endif
#else
                    p->ready_combine = 0;
#endif
                    /* 找到最大尺寸的非UI图层 */
                    if (p->fb_name[2] - '0' != 0) {
                        if (max_map_size < p->map.width * p->map.height) {
                            max_map_size =  p->map.width * p->map.height;
                            index = fb_n;
                            if (__this->map_backup_baddr) {
                                p->ready_combine = 1;
                            }
                        }
                    }
                    if (out.addr) {
                        __fb_buf_lock(p, p->map.baddr);
                    }
                }
                p1 = p;
                /* printf("%s--%x\n", p->fb_name, p->map.baddr); */
                __this->fb_frame_cnt[p->fb_name[2] - '0']--;
                __dma2d_in_layer_conf(&in[fb_n], p);
                __dma2d_in_layer_modify(id, &in[fb_n], p);
                fb_n++;
            }
        }
        spin_unlock(&fb_lock[id]);
#if 0
        if (__this->fb_number > fb_n) {
            need_combine = 1;
            __this->fb_number = fb_n;
        } else if (__this->fb_number && fb_n == 0) {
            __this->fb_number = 0;
            need_combine = 1;
            fb_n = -1;
        } else {
            __this->fb_number = fb_n;
        }
#endif

        if (out.addr && fb_n && need_combine) {
            if (out.addr == FB_COMBINE_OUT_USE_MAX_IMGBUF) {
                if (fb_n == 1 && p1) {
                    if ((p1->fb_name[2] - '0' == 0) && p1->map.format == out.format) {
                        //只有一个UI图层需要合成时,并且格式和lcd一致可以直推lcd显示
                        out.addr = in[0].addr;
                        goto __data2lcd;
                    } else if (p1->fb_name[2] - '0' == 0) {
                        //格式不相同暂不支持
                        log_error("fb0 format != lcd out format");
                        out.addr = in[0].addr;
                        goto __data2lcd;
                    }
                }
                if (index == -1 || !__check_max_fb_is_support(&in[index], out.width, out.height)) {
                    for (u8 i = 0; i < fb_n; i++) {
                        __fb_head_buf_unlock(id, in[i].addr);
                    }
                    goto __combine_exit;
                }
                out.addr = in[index].addr;
            }
            if (__this->map_backup_baddr) {
                //还原备份的数据图像帧
                dma2d_combine_copy_frame(in[index].addr, __this->map_backup_baddr, 0, 0, in[index].width, in[index].height, in[index].width, in[index].height, in[index].format);
            }

            /* log_info("fb combine task out addr:%x\n", out.addr); */

#if FB_COMBINE_TIME_DEBUG_EN
            dma2d_combine_use_time = get_system_us();
#endif
            fb_lcd_frame_async_wait(id);
            //使用dma2d合成方法
            dret = dma2d_combine_method(id, in, &out, fb_n);
#if FB_COMBINE_TIME_DEBUG_EN
            dma2d_combine_use_time = get_system_us() - dma2d_combine_use_time;
#endif
            //unlock fb in buf
            for (u8 i = 0; i < fb_n; i++) {
                if (in[i].addr == out.addr) {
                    continue;
                }
                __fb_head_buf_unlock(id, in[i].addr);
            }

            //debug
            /* dump_combine_frame_buffer(in, &out, fb_n); */

            /* 合成输出用户回调 */
            if (fb_combine_out_cb_func) {
                //可用于UI直接在合成输出buffer上绘制
                omap.baddr = out.addr;
                omap.width = out.width;
                omap.height = out.height;
                omap.format = out.format;
                if (out.addr != FB_COMBINE_OUT_USE_MAX_IMGBUF) {
                    //如果合成输出buf是单独的buf, 即UI不是直接在图像FB上绘制的情况，可以提前释放锁
                    fb_combine_mutex_exit(id);
                }
#if FB_COMBINE_TIME_DEBUG_EN
                ui_use_time = get_system_us();
#endif
                ret = fb_combine_out_cb_func((void *)&omap, (u32)(in[0].addr), (u32)(in[1].addr));
#if FB_COMBINE_TIME_DEBUG_EN
                ui_use_time = get_system_us() - ui_use_time;
#endif
            }

__data2lcd:
            if (ret == 0) {
#if FB_COMBINE_TIME_DEBUG_EN
                lcd_use_time = get_system_us();
#endif
#if (LV_DISP_UI_FB_NUM == 1 && FB_LCD_BUF_NUM == 2)
                fb_lcd_frame_buf_update_async(id, out.addr);
#else
                if (dret == FB_COMBINE_OUT_USE_ROTATE) {
                    //lcd显存交换
                    fb_lcd_frame_buf_swap(id, out.addr);
                    out.addr = fb_combine_swap_outbuf(id, out.addr);
                } else {
                    //更新lcd显示
                    fb_lcd_frame_buf_update(id, out.addr);
                }
#endif
#if FB_COMBINE_TIME_DEBUG_EN
                lcd_use_time = get_system_us() - lcd_use_time;
#endif
            }

            //unlock fb out buf
            __fb_head_buf_unlock(id, out.addr);

            if (dret != 0xf) {
                if (fb_lcd_get_buf_num(id) > 1) {
                    //lcd 双buf 索引切换
                    out.addr = fb_lcd_buf_index_swap(id);
                } else {
                    out.addr = 0;
                }
            }
#if FB_COMBINE_TIME_DEBUG_EN
            log_info("[%d]dma2d combine:%dus, ui use:%dus, lcd use:%dus", id, dma2d_combine_use_time, ui_use_time, lcd_use_time);
            log_info("[%d]totoal use:%dus", id, dma2d_combine_use_time + ui_use_time + lcd_use_time);
#endif
#if FB_COMBINE_FRAME_DEBUG_EN
            static unsigned int time_lapse_handle = 0;
            if (time_lapse(&time_lapse_handle, 1000)) {
                for (int i = 0; i < FB_COMBINE_MAX_NUM; i++) {
                    if (__this->fb_frame_cnt[i] > 0) {
                        log_info("fb%d drop frames:%d", i, __this->fb_frame_cnt[i]);
                        __this->fb_frame_cnt[i] =  0;
                    }
                }
            }
#endif

        } else if (__this->combine_task_run == FB_COMBINE_TASK_RUN) {
            __this->combine_busy = 0;
            fb_combine_mutex_exit(id);
            os_sem_pend(&__this->wait_sem, 0);
            continue;
        }

__combine_exit:
        __this->combine_busy = 0;

        if (__this->combine_task_run == FB_COMBINE_FUNC_RUN) {
            //单函数，只执行一次
            fb_combine_mutex_exit(id);
            break;
        }
        fb_combine_mutex_exit(id);
    }

    return fb_n;
}
static void __fb_task(void *p)
{
    log_info("fb task run....");
    fb_combine_task((u8)p, NULL);
}

/**
 * @brief   fb 合成模块初始化
 * @param:  none
 * @return: none
 **/
void fb_combine_init(void)
{
    u8 id = 0;
    for (int i = 0; i < sizeof(_fb_combine) / sizeof(_fb_combine[0]); i++) {
        id = i;
        memset(__this, 0, sizeof(struct fb_combine_t));
        INIT_LIST_HEAD(&head[i]);
        os_mutex_create(&__this->mutex);
        os_sem_create(&__this->wait_sem, 0);
        for (int j = 0; j < FB_COMBINE_MAX_NUM; j++) {
            __this->fb_frame_cnt[j] = -1;
        }
    }
}


/**
 * @brief   fb 合成模块准备
 * @param:  info:当前fb信息 open_fb:已经打开的fb个数
 * @return: none
 **/
void fb_combine_prepare(struct fb_draw_info *info, u8 open_fb)
{
    char fb_combine_task_name[20];
    u8 id = info->out_id;
    fb_combine_mutex_enter(id);
    if (__is_need_create_combine_task(open_fb, info)) {
        if (__this->combine_task_run == FB_COMBINE_STOP) {
            __this->combine_task_run = FB_COMBINE_TASK_RUN;
            sprintf(fb_combine_task_name, "fb_combine_task%d", id);
            thread_fork(fb_combine_task_name, 25, 4096, 0, 0, __fb_task, (void *)id);
        }
    }

    fb_combine_set_outbuf(id); //fb 合成输出buf配置

    if (!strcmp(info->name, "fb0")) {
        if (info->fb_num == 1) { //单buffer
            __this->combine_task_run = FB_COMBINE_FUNC_RUN;
        } else if (info->fb_num == 0) {
#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
            u8 mode = 1; //使用lcd显存双buffer
            log_info("lvgl_set_ui_flush_mode: %x %x", __this->combine_out_buf[0], __this->combine_out_buf[1]);
            if (__this->combine_out_buf[0] == 0 && __this->combine_out_buf[1] == 0) {
                mode = 2;
            }
            lvgl_set_ui_flush_mode(mode, (void *)__this->combine_out_buf[0], (void *)__this->combine_out_buf[1]); /* lvgl UI自己刷新 */
#endif

        }
    } else {

#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
#if (LV_DISP_UI_FB_NUM <= 1)
        if (__this->ui_timer_id == 0) {
            char *task_name;
            if (os_task_get_handle(LVGL_TASK_NAME) == NULL) {
                task_name = "app_core";
            } else {
                task_name = LVGL_TASK_NAME;
            }
            /* 创建自刷模式检测定时器 */
            __this->ui_timer_id = sys_timer_add_to_task(task_name, (void *)id, ui_set_flush_mode_timer_cb, _LVGL_UI_ENTER_FLUSH_TIMEOUT);
        }
#endif
#endif
    }

    fb_combine_mutex_exit(id);
}

/**
 * @brief   fb 合成模块链表追加
 * @param:  ep:当前fb 输出节点
 * @return: none
 **/
void fb_combine_list_add(struct fb_out_t *ep)
{
    struct fb_out_t *p0 = NULL;
    u8 id = ep->out_id;
#if 0
    //约定fb3作为次顶层
    if (ep->fb_name[2] - '0' == 3) {
        ep->z_order = 254;
    }
    //约定fb4作为最顶层
    if (ep->fb_name[2] - '0' == 4) {
        ep->z_order = 255;
    }
#endif
    spin_lock(&fb_lock[id]);
    __this->fb_frame_cnt[ep->fb_name[2] - '0'] = 0;
    list_for_each_entry(p0, &head[id], entry) {
        if (p0->z_order <= ep->z_order) { //按照由小到大排序,数字越大越顶层
            __list_add(&ep->entry, p0->entry.prev, &p0->entry);
            spin_unlock(&fb_lock[id]);
            return;
        }
    }
    list_add_tail(&ep->entry, &head[id]);
    spin_unlock(&fb_lock[id]);

}

/**
 * @brief   fb 合成模块链表节点删除
 * @param:  ep:当前fb 输出节点
 * @return: none
 **/
void fb_combine_list_del(struct fb_out_t *ep)
{
    struct fb_out_t _ep = {0};
    u8 id = ep->out_id;
    spin_lock(&fb_lock[id]);
    memcpy(&_ep, ep, sizeof(struct fb_out_t));
    ep->map.baddr = 0;
    ep->map.baddr_bk = 0;
    __this->fb_frame_cnt[ep->fb_name[2] - '0'] = -1;
    if (ep->buf_addr[0] || ep->buf_addr[1]) {
        list_del(&ep->entry);
    }
    memset(&__this->map_modify[ep->fb_name[2] - '0'], 0, sizeof(struct fb_map_user));
    spin_unlock(&fb_lock[id]);
    _ep.ready_combine = 0xff;
    fb_combine_updata(&_ep, NULL); //当前fb图层删除了应该触发一次合成
}


/**
 * @brief   fb 合成模块数据更新
 * @param:  ep:当前fb 输出节点 map: 需要更新的数据
 * @return: 0:更新成功 非0: 更新失败
 **/
int fb_combine_updata(struct fb_out_t *ep, struct fb_map_user *map)
{
    struct fb_out_t *p = NULL;
    int err = 0;
    u8 need_combine = 0;
    u8 fb_index;
    u8 id = ep->out_id;

    if (map && map->transp) {
        //不用合成,数据直推lcd 更新
        fb_lcd_frame_buf_update(id, map->baddr);
        return 0;
    }
    if (map == NULL && ep->ready_combine == 0xff) {
        need_combine = 1;
    }

    spin_lock(&fb_lock[id]);
    list_for_each_entry(p, &head[id], entry) {
        if (p == ep) {
            if (__fb_buf_is_lock(p, map->baddr)) {
                spin_unlock(&fb_lock[id]);
                putchar('D');
                return -1;
            }
            p->map.baddr_bk = p->map.baddr;
            if (map) {
                p->map.baddr = map->baddr;
                p->map.width = map->width;
                p->map.height = map->height;
                p->map.real_width = map->real_width;
                p->map.real_height = map->real_height;
                p->map.rotate = map->rotate;
                p->map.mirror = map->mirror;
                p->map.format = map->format;
                p->map.xoffset = map->xoffset;
                p->map.yoffset = map->yoffset;
            }
            p->ready_combine = 1;
            need_combine = 1;
            for (u8 i = 0; i < sizeof(p->buf_addr) / sizeof(p->buf_addr[0]); i++) {
                if ((u32)p->map.baddr == p->buf_addr[i]) {
                    p->map.buf_index = i;
                    break;
                }
            }
            break;
        }
    }
    spin_unlock(&fb_lock[id]);

    if (need_combine) {
#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
#if (LV_DISP_UI_FB_NUM <= 1)

#if (FB_LCD_BUF_NUM == 1)
        u16 lcd_w = 0;
        u16 lcd_h = 0;
        if (fb_lcd_get_rotate(id) == ROTATE_90 || fb_lcd_get_rotate(id) == ROTATE_270) {
            lcd_w  = fb_lcd_get_height(id);
            lcd_h = fb_lcd_get_width(id);
        } else {
            lcd_w  = fb_lcd_get_width(id);
            lcd_h = fb_lcd_get_height(id);
        }
        if (ep->fb_name[2] - '0' != 0) {
            if (map) {
                if (map->width == lcd_w && map->height == lcd_h) {
                    __this->map_backup_baddr = NULL;
                }
            }
        }
#endif
        fb_index = ep->fb_name[2] - '0';
        if (fb_index != 0) {
            int lvgl_send_fb_combine_event(u8 id);
            err = lvgl_send_fb_combine_event(id);

            __this->fb_frame_cnt[fb_index]++;

            ui_timer_modify(id);
        } else if (__this->ui_timer_id == 0 || fb_index == 0) {
            //ui自刷
            fb_combine_task(id, NULL);
            __this->fb_frame_cnt[fb_index]++;
        }
#endif
#else
        if (fb_lcd_get_buf_num(id) == 1) {
            run_fb_combine_async(id);
        }
#endif
        os_sem_set(&__this->wait_sem, 0);
        os_sem_post(&__this->wait_sem);
    }

    if (__this->combine_task_run == FB_COMBINE_STOP) {
#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
#if (LV_DISP_UI_FB_NUM == 2 && FB_LCD_BUF_NUM == 1)
        if (map) {
            fb_lcd_frame_buf_update_async(id, map->baddr);
        }
        return 0;
#endif
#endif
        if (map) {
            fb_lcd_frame_buf_update(id, map->baddr);
        }
    }

    return 0;
}

/**
 * @brief   fb 合成模块关闭
 * @param:  none
 * @return: 0:关闭成功 非0: 关闭失败
 **/
int fb_combine_close(u8 id)
{
    if (__this->combine_task_run) {
        __this->combine_task_run = FB_COMBINE_STOP;
        __this->combine_busy = 0;
        os_sem_set(&__this->wait_sem, 0);
        os_sem_post(&__this->wait_sem);

    }
    return 0;
}

/**
 * @brief : 动态修改指定fb图层属性
 * @param: id: lcd ID
 * @param: fb_name: fb名
 * @param: map: 修改的图层属性
 * @return: 0:成功  非0:失败
 **/
int fb_combine_layer_modify(u8 id, const char *fb_name, struct fb_map_user *map)
{
    u8 i = 0;
    struct fb_map_user *map_modify = NULL;
    struct fb_out_t *p = NULL;
    if (!fb_name || !map) {
        return -1;
    }
    fb_combine_mutex_enter(id);
    i = fb_name[2] - '0';
    if (i > FB_COMBINE_MAX_NUM) {
        return -1;
    }

    spin_lock(&fb_lock[id]);
    list_for_each_entry(p, &head[id], entry) {
        if (!strcmp(p->fb_name, fb_name)) {
            break;
        }
    }
    spin_unlock(&fb_lock[id]);
    log_info("fb_combine_layer_modify: %d %dx%d r:%d m:%d", id, map->width, map->height, map->rotate, map->mirror);
    map_modify = &__this->map_modify[i];
    memset(map_modify, 0, sizeof(struct fb_map_user));
    /* map_modify->addr = map->baddr; */
    /* map_modify->format = map->format; */
    map_modify->width = map->width;
    map_modify->height = map->height;
    map_modify->real_width = map->real_width;
    map_modify->real_height = map->real_height;
    map_modify->rotate = map->rotate;
    map_modify->mirror = map->mirror;
    map_modify->xoffset = map->xoffset;
    map_modify->yoffset = map->yoffset;
    fb_combine_mutex_exit(id);

    p->ready_combine = 0xff;
    fb_combine_updata(p, NULL); //强制触发一次合成

    return 0;
}











/********************* Demo Test*****************************/

//动态修改FB属性Demo
void fb_modify_test(void *p)
{
    static u8 index = 0;
    struct fb_map_user map = {0};
    const u16 rotate_array[4] = {0, 90, 180, 270};
    //4个方向轮换
    if (++index >= 4) {
        index = 0;
    }

    u16 r = rotate_array[index];
    switch (r) {
    case 0:
    case 180:
        map.real_width = 800; //显示原图宽
        map.real_height = 480;//显示原图高

        map.rotate = r;    //显示旋转角度
        map.width = 800;   //显示目标宽
        map.height = 480;  //显示目标高
        break;
    case 90:
    case 270:
        map.xoffset = 250;    //显示位置x坐标偏移
        map.yoffset = 0;      //显示位置y坐标偏移
        map.real_width = 800; //显示原图宽
        map.real_height = 480;//显示原图高

        map.rotate = r;    //显示旋转角度
        map.width = 288;   //显示目标宽
        map.height = 480;  //显示目标高
        break;
    }
    fb_combine_layer_modify(0, "fb1", &map);

}
#endif
