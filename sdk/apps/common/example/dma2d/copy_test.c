
#include "asm/dma2d_driver.h"
#include "asm/gpio.h"

#define IMG_WIDTH       454
#define IMG_HEIGHT      454

#define IMG_COPY_WIDTH      32
#define IMG_COPY_HEIGHT     32

#define IMG_OUT_FORMAT  (JLDMA2D_FORMAT_RGB565)
#define IMG_COLOR_DEPTH  16

/* #define IMG_OUT_FORMAT  (JLDMA2D_FORMAT_ARGB8888) */
/* #define IMG_COLOR_DEPTH 32 */

static unsigned char src_buf[IMG_COPY_WIDTH * IMG_COPY_HEIGHT * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;
static unsigned char dst_buf[IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;

static JL_TIMER_TypeDef *pTimer;
volatile u32 t_counter = 0;
static volatile u32 t_init = 0;

static volatile u8 lvgl_start = 0;
static volatile u32 lvgl_start_cnt = 0;
static volatile u8 lvgl_sw_start = 0;
static volatile u32 lvgl_sw_start_cnt = 0;
static volatile u8 lvgl_sw_1_start = 0;
static volatile u32 lvgl_sw_1_start_cnt = 0;
static volatile u8 lvgl_dma2d_start = 0;
static volatile u32 lvgl_dma2d_start_cnt = 0;

unsigned int LVGL_START_ENTER(int f)
{
    if (f == -1) {
        return lvgl_start_cnt;
    }
    /* JL_PORTD->DIR &= ~BIT(9); */
    /* if(f) { */
    /* JL_PORTD->OUT |= BIT(9); */
    /* } else { */
    /* JL_PORTD->OUT &= ~BIT(9); */
    /* } */
    lvgl_start = f;
    return 0;
}

unsigned int LVGL_SW_START_ENTER(int f)
{
    if (f == -1) {
        return lvgl_sw_start_cnt;
    }
    /* JL_PORTD->DIR &= ~BIT(8); */
    /* if(f) { */
    /* JL_PORTD->OUT |= BIT(8); */
    /* } else { */
    /* JL_PORTD->OUT &= ~BIT(8); */
    /* } */
    lvgl_sw_start = f;
    return 0;
}
unsigned int LVGL_SW_1_START_ENTER(int f)
{
    if (f == -1) {
        return lvgl_sw_1_start_cnt;
    }
    /* JL_PORTD->DIR &= ~BIT(14); */
    /* if(f) { */
    /* JL_PORTD->OUT |= BIT(14); */
    /* } else { */
    /* JL_PORTD->OUT &= ~BIT(14); */
    /* } */
    lvgl_sw_1_start = f;
    return 0;
}
unsigned int LVGL_DMA2D_START_ENTER(int f)
{
    if (f == -1) {
        return lvgl_dma2d_start_cnt;
    }
    lvgl_dma2d_start = f;
    return 0;
}
___interrupt
void test_timer_irq()
{
    pTimer->CON |= BIT(14);
    /* t_counter++; */
    if (lvgl_start) {
        lvgl_start_cnt++;
    }
    if (lvgl_sw_start) {
        lvgl_sw_start_cnt++;
    }
    if (lvgl_sw_1_start) {
        lvgl_sw_1_start_cnt++;
    }
    if (lvgl_dma2d_start) {
        lvgl_dma2d_start_cnt++;
    }
}
void timer_test()
{
    u8 timer_irq_idx = 0;
    JL_TIMER_TypeDef *tregs[] = {
        JL_TIMER0, JL_TIMER1, JL_TIMER2, JL_TIMER3,
        /* JL_TIMER4, JL_TIMER5, */
    };
    u32 tmr_irq_idx_tbl[] = {
        IRQ_TIMER0_IDX, IRQ_TIMER1_IDX, IRQ_TIMER2_IDX, IRQ_TIMER3_IDX,
        /* IRQ_TIMER4_IDX, IRQ_TIMER5_IDX, */
    };
    if (t_init) {
        return ;
    }
    u8 i;
    u8 tmr_num = sizeof(tmr_irq_idx_tbl) / sizeof(tmr_irq_idx_tbl[0]);
    timer_irq_idx = tmr_irq_idx_tbl[tmr_num - 1];
    for (i = 0; i < tmr_num; i++) {
        pTimer = tregs[tmr_num - 1 - i];
        if ((pTimer->CON & 3) == 0) {
            timer_irq_idx = tmr_irq_idx_tbl[tmr_num - 1 - i];
            break;
        }
    }

    pTimer->CON = 0;
    request_irq(timer_irq_idx, 7, test_timer_irq, 0);

    t_counter = 0;

    printf("lsb clock %d\n", clk_get("lsb"));
    printf("timer%d is found\n", tmr_num - 1 - i);
    pTimer->CNT = 0;
    pTimer->PRD = clk_get("lsb") / 4 / 100000; //10us
    pTimer->CON = (1 << 4) | (1 << 10) | (1 << 14);  //sel lsb_clk, div4
    pTimer->CON |= BIT(0);
    t_init = 1;

}
void copy_test()
{

    uint8_t *image_fill = src_buf;
    uint8_t *image_copy = dst_buf;

    static uint32_t x = 0;
    static uint32_t y = 0;
    static uint32_t lines = 0;

    uint32_t dest_stride = IMG_COPY_WIDTH;
    uint32_t fill_color = 0x7BE0;
    for (int yy = 0; yy < IMG_COPY_HEIGHT; yy++) {
        for (int xx = 0; xx < IMG_COPY_WIDTH; xx++) {
            uint8_t *p = &image_fill[(yy * dest_stride + xx) * 2];
            *p++ = (fill_color) & 0xff;
            *p = (fill_color >> 8) & 0xff;
        }
    }

    if (lines == 0) {
        /* dest_stride = IMG_WIDTH; */
        /* fill_color = 0xFFFF;  */
        /* for (int yy = 0; yy < IMG_HEIGHT; yy++) { */
        /* for (int xx = 0; xx < IMG_WIDTH; xx++) { */
        /* uint8_t *p = &image_copy[(yy * dest_stride + xx) * 2]; */
        /* *p++ = (fill_color) & 0xff; */
        /* *p = (fill_color >> 8) & 0xff; */
        /* } */
        /* } */
        memset(image_copy, 0xFF, IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8);
        lines = 1;
    }

    /* dma2d_init(); */

    dma2d_reset_all_regs();
    dma2d_create_task();

    x += IMG_COPY_WIDTH;
    if (x > IMG_WIDTH) {
        x = 0;
        y = lines * IMG_COPY_HEIGHT;
        lines++;
        if ((y + IMG_COPY_HEIGHT) > IMG_HEIGHT) {
            memset(image_copy, 0xFF, IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8);
            lines = 0;
            x = 0;
            y = 0;
        }
    }
    printf("x=%d y=%d lines=%d\n", x, y, lines);
    uint8_t *image_dst = image_copy + (y * IMG_WIDTH + x) * (IMG_COLOR_DEPTH / 8);
    dma2d_copy(image_dst, IMG_WIDTH * IMG_COLOR_DEPTH / 8, image_fill, IMG_COPY_WIDTH * IMG_COLOR_DEPTH / 8, IMG_OUT_FORMAT, IMG_COPY_WIDTH, IMG_COPY_HEIGHT);
    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

    /* put_buf(image_copy, IMG_WIDTH * IMG_HEIGHT * IMG_COLOR_DEPTH / 8); */
    //推屏显示
    lcd_lvgl_full(0, IMG_WIDTH, 0, IMG_HEIGHT, image_copy);

    os_time_dly(50);


    //free(image_fill);
    //free(image_copy);
}

/* 单个整张图层拷贝测试 */
#define IMG_SRC_W  454
#define IMG_SRC_H  454
#define IMG_DST_W  454
#define IMG_DST_H  454
static unsigned char image_src[IMG_SRC_W * IMG_SRC_H * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;
static unsigned char image_dst[IMG_DST_W * IMG_DST_H * IMG_COLOR_DEPTH / 8] __attribute__((aligned(32))) ;
__attribute__((noinline))
void lcd_copy_test()
{
    uint32_t fill_color = 0x7BE0;
    for (int yy = 0; yy < IMG_SRC_H; yy++) {
        for (int xx = 0; xx < IMG_SRC_W; xx++) {
            uint8_t *p = &image_src[(yy * IMG_SRC_W + xx) * IMG_COLOR_DEPTH / 8];
            *p++ = (fill_color) & 0xff;
            *p = (fill_color >> 8) & 0xff;
        }
    }

    memset(image_dst, 0xFF, IMG_DST_W * IMG_DST_H * IMG_COLOR_DEPTH / 8);

    timer_test();

    JL_PORTD->DIR &= ~BIT(15);
    JL_PORTD->OUT |= BIT(15);
    u32 _j = jiffies;
    t_counter = 0;
#if 0 /* 当前fpga 环境 系统时钟48M dma2d时钟24M
       * 454x454的图,总时间在23ms,其中前期刷cache 4ms，dma2d处理时间15ms,后期读数据到cache 4ms
       * */
    //数据准备就绪刷cache
    DcuFlushRegion(image_src, IMG_SRC_W * IMG_SRC_H * IMG_COLOR_DEPTH / 8);
    DcuFlushRegion(image_dst, IMG_DST_W * IMG_DST_H * IMG_COLOR_DEPTH / 8);
    //ic 在sof里面拉了一些信号io出来
    dma2d_init();

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_copy(image_dst, IMG_DST_W * IMG_COLOR_DEPTH / 8, image_src, IMG_SRC_W * IMG_COLOR_DEPTH / 8, IMG_OUT_FORMAT, IMG_SRC_W, IMG_SRC_H);

    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

    //从ddr读数据到cache,准备给后续ddr用
    /* DcuInvalidRegion(image_src,IMG_SRC_W * IMG_SRC_H * IMG_COLOR_DEPTH / 8); */
    DcuInvalidRegion(image_dst, IMG_DST_W * IMG_DST_H * IMG_COLOR_DEPTH / 8);
#else
    //纯cpu 拷贝数据
    /* 454x454的图,总时间在33ms */
    memcpy(image_dst, image_src, IMG_SRC_W * IMG_SRC_H * IMG_COLOR_DEPTH / 8);
    /* for(int i = 0; i < IMG_SRC_H; i++) { */
    /* for(int j = 0;j < IMG_SRC_W; j++) { */
    /* image_dst[(i * IMG_DST_W + j) * 2] = image_src[(i * IMG_SRC_W + j) * 2];  */
    /* image_dst[(i * IMG_DST_W + j) * 2 + 1] = image_src[(i * IMG_SRC_W + j) * 2 + 1];  */
    /* } */
    /* } */
    DcuFlushRegion(image_dst, IMG_DST_W * IMG_DST_H * IMG_COLOR_DEPTH / 8);
#endif
    JL_PORTD->OUT &= ~BIT(15);

    /* printf("use time =%dms\n", jiffies_to_msecs(jiffies - _j)); */
    if (!memcmp(image_dst, image_src, IMG_SRC_W * IMG_SRC_H * IMG_COLOR_DEPTH / 8)) {
        printf("copy data is same sucess!\n");
        t_counter = 0;
    } else {
        printf("copy data is not same faild!!!\n");
    }

    os_time_dly(50);

}
