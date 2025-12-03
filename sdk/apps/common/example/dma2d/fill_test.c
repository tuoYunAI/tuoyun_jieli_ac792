#include "asm/dma2d_driver.h"
#include "perf_counter.h"

//推屏显示需要加上 apps/common/example/peripheral/lcd 编译

#define IMG_WIDTH       480
#define IMG_HEIGHT      272

void fill_test()
{
    uint32_t fill_color;
    uint8_t r, g, b, a;

    uint8_t *image_fill = lcd_get_draw_buf();
    uint8_t format = JLDMA2D_FORMAT_RGB565;


    if (!image_fill) {
        return ;
    }

    memset(image_fill, 0, IMG_WIDTH * IMG_HEIGHT * 16 / 8);

    /* dma2d_init(); */

    for (int i = 0; i < 4; i++) {
        if (i == 0) {
            a = 255;
            r = 255;
            g = 0;
            b = 0;
        } else if (i == 1) {
            a = 255;
            r = 0;
            g = 255;
            b = 0;
        } else if (i == 3) {
            a = 255;
            r = 0;
            g = 0;
            b = 255;
        } else {
            a = 128;
            r = 0;
            g = 255;
            b = 255;
        }

        fill_color = dma2d_pack_pixel(a, r, g, b, JLDMA2D_FORMAT_RGB565);

        dma2d_reset_all_regs();
        dma2d_create_task();
        dma2d_fill(image_fill, IMG_WIDTH * 16 / 8, format, 0, fill_color, IMG_WIDTH, IMG_HEIGHT);
        dma2d_run();
        dma2d_wait_done();
        /* printf("fill_color=0x%x\n", fill_color); */
        /* printf("output=%x, %x, %x, %x\n", image_fill[0], image_fill[1], image_fill[2], image_fill[3]); */
        lcd_lvgl_full(0, IMG_WIDTH, 0, IMG_HEIGHT, image_fill);

        os_time_dly(50);

        /* break; */
    }


    //free(image_fill);
}

static void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     uint32_t fill_color)
{
    uint32_t dest_stride = IMG_WIDTH;
    u8 *dst_buf = lcd_get_draw_buf();
#if 0
    //fill_color : RGB565
    uint8_t *p = &dst_buf[(y * dest_stride + x) * 2];
    uint16_t r = ((fill_color >> 11) & 0x1f) << 3;
    uint16_t g = ((fill_color >> 5) & 0x3f) << 2;
    uint16_t b = ((fill_color) & 0x1f) << 3;
    uint32_t color = dma2d_pack_pixel(0xff, r, g, b, JLDMA2D_FORMAT_ARGB8888);
    dma2d_create_task();
    dma2d_fill(p, dest_stride * 2, JLDMA2D_FORMAT_RGB565, 0, color, w, h);
    JL_IOMC->OCH_CON0 = (0 << 0) | (1 << 5) | (2 << 10) | (3 << 15);
    JL_OMAP->PD8_OUT = FO_GP_OCH0;
    JL_OMAP->PD9_OUT = FO_GP_OCH1;
    JL_OMAP->PD14_OUT = FO_GP_OCH2;
    JL_PORTD->DIR &= ~BIT(8);
    JL_PORTD->DIR &= ~BIT(9);
    JL_PORTD->DIR &= ~BIT(14);
#else
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            uint8_t *p = &dst_buf[(yy * dest_stride + xx) * 2];
            *p++ = (fill_color) & 0xff;
            *p = (fill_color >> 8) & 0xff;
        }
    }
#endif
}

static void dma2d_fill_test(void)
{
    u32 iCycleResult;

    dma2d_init();

    /* measure cycles and store it in a dedicated variable without printf */
    __cycleof__("dma2d_fill_test",
                /* insert code to __cycleof__ body, "{}" can be omitted  */
    {
        iCycleResult = __cycle_count__;   /*< "__cycle_count__" stores the result */
    }) {

        // 填充背景色
        FillRect(0,   0,   454, 454,  0xFFFF);
#if 0
        // 绘制数据条
        FillRect(100,  120,  20,  120,  0x001f);
        FillRect(160, 100, 20,  140,    0x001f);
        FillRect(220, 40,  20,  200,    0x001f);
        FillRect(280, 60,  20,  180,    0x001f);
        FillRect(340, 180, 20,  60,     0x001f);
        /* // 绘制X轴 */
        FillRect(40,  240, 340, 1,    0x001f);

#endif
        /* dma2d_dump_all_tasks(); */
        /* dma2d_run(); */
        /* dma2d_wait_done(); */
        //数据准备就绪刷cache
        /*DcuFlushRegion(lcd_get_draw_buf(), IMG_WIDTH * IMG_HEIGHT * 16 / 8);*/
    }
    printf("dma2d_fill_test cost %d us \r\n", perfc_convert_ticks_to_us(iCycleResult));


}

static void dma2d_fill_test_task(void *p)
{
    os_time_dly(50);

    while (1) {
        dma2d_fill_test();
        lcd_draw_finish();

        /*os_time_dly(1);*/
    }

}

static int dma2d_fill_main(void)
{
    thread_fork("dma2d_fill_test_task", 28, 8 * 1024, 0, 0, dma2d_fill_test_task, NULL);
    return 0;
}
late_initcall(dma2d_fill_main);
