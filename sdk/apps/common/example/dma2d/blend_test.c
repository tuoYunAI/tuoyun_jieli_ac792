
#include "asm/dma2d_driver.h"
#include "asm/gpio.h"

#define IMG_WIDTH       454
#define IMG_HEIGHT      454

#define IMG_OUT_FORMAT  (JLDMA2D_FORMAT_RGB565)
#define IMG_COLOR_DEPTH  16

/* static unsigned char fg_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] sec(.volatile_ram) __attribute__((aligned(32))) ; */
/* static unsigned char bg_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] sec(.volatile_ram) __attribute__((aligned(32))) ; */
/* static unsigned char out_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] sec(.volatile_ram) __attribute__((aligned(32))) ; */
static unsigned char fg_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;
static unsigned char bg_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;
static unsigned char out_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;

static void fill_buffer_rgb565(unsigned char *buffer, int x, int y, int w, int h, uint32_t fill_color)
{
    uint32_t dest_stride = IMG_WIDTH;
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            uint8_t *p = &buffer[(yy * dest_stride + xx) * 2];
            *p++ = (fill_color) & 0xff;
            *p = (fill_color >> 8) & 0xff;
        }
    }
}

static void blend_buffer(unsigned char *bg_buf, unsigned char *fg_buf, unsigned char *dst_buf, uint8_t opa)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    out_layer_param.data = dst_buf;
    out_layer_param.w = IMG_WIDTH;
    out_layer_param.h = IMG_HEIGHT;
    out_layer_param.stride = IMG_WIDTH * 2;
    out_layer_param.format = IMG_OUT_FORMAT;
    out_layer_param.rbs = 0;

    fg_layer_param.data = (uint32_t)fg_buf;
    fg_layer_param.stride = IMG_WIDTH * 2;
    fg_layer_param.format = IMG_OUT_FORMAT;
    fg_layer_param.alpha = opa;
    fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
    fg_layer_param.rbs = 0;

    bg_layer_param.data = (uint32_t)bg_buf;
    bg_layer_param.stride = IMG_WIDTH * 2;
    bg_layer_param.format = IMG_OUT_FORMAT;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
}

#include "src/misc/lv_color.h"
static void soft_blend_buffer(lv_color_t *bg_buf, lv_color_t *fg_buf, lv_color_t *dst_buf, uint8_t opa)
{
    int w = IMG_WIDTH;
    int h = IMG_HEIGHT;
    u32 src_stride = w;
    u32 dest_stride = w;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            dst_buf[x] = lv_color_mix(fg_buf[x], bg_buf[x], opa);
        }
        bg_buf += dest_stride;
        fg_buf += src_stride;
        dst_buf += dest_stride;
    }

}
void blend_test()
{
    static u8 buffer_inited = 0;
    if (buffer_inited == 0) {
        fill_buffer_rgb565(bg_buf, 0, 0, IMG_WIDTH, IMG_HEIGHT, 0xFFFF);
        fill_buffer_rgb565(fg_buf, 0, 0, IMG_WIDTH, IMG_HEIGHT, 0x780F);
        buffer_inited = 1;
    }

    JL_PORTD->DIR &= ~BIT(15);
    JL_PORTD->OUT |= BIT(15);

    /* 透明度渐变 */
    static uint8_t opa = 255;
    if (opa == 0) {
        opa = 255;
    }
#if 1
    //数据准备就绪刷cache
    DcuFlushRegion(bg_buf, IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8);
    DcuFlushRegion(fg_buf, IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8);
    JL_IOMC->OCH_IOMC0 = (0 << 0) | (1 << 5) | (2 << 10) | (3 << 15);
    JL_OMAP->PD8_OUT = FO_GP_OCH0;
    JL_OMAP->PD9_OUT = FO_GP_OCH1;
    JL_OMAP->PD14_OUT = FO_GP_OCH2;
    JL_PORTD->DIR &= ~BIT(8);
    JL_PORTD->DIR &= ~BIT(9);
    JL_PORTD->DIR &= ~BIT(14);
    dma2d_init();
    blend_buffer(bg_buf, fg_buf, out_buf, opa);
    dma2d_run();
    dma2d_wait_done();
    DcuInvalidRegion(out_buf, IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8);
#else
    soft_blend_buffer(bg_buf, fg_buf, out_buf, opa);
#endif
    JL_PORTD->OUT &= ~BIT(15);
    opa -= 4;
    printf("opa=%d\n", opa);

    //推屏显示
    lcd_lvgl_full(0, IMG_WIDTH, 0, IMG_HEIGHT, out_buf);

    /* os_time_dly(3); */

}
