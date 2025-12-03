#include "lcd_driver.h"
#include "lcd_config.h"
#include "asm/exti.h"
#include "gpio.h"

#if TCFG_LCD_MCU_ILI9481_320X480

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

#define REGFLAG_DELAY 0x45

static void ili9481_set_direction(u8 dir)
{
    if (dir == 0) {
        WriteCOM(__LCD_ID, 0x36);
        WriteDAT_8(__LCD_ID, 0x58);
    } else {
        WriteCOM(__LCD_ID, 0x36);
        WriteDAT_8(__LCD_ID, 0x98);
    }
}

static void ili9481_reset(void)
{
    printf("ili9481 mcu lcd reset\n");
    lcd_rst_pinstate(__LCD_ID, 1);
    lcd_rs_pinstate(__LCD_ID, 1);
    lcd_cs_pinstate(__LCD_ID, 1);

    lcd_rst_pinstate(__LCD_ID, 1);
    lcd_delay(60);
    lcd_rst_pinstate(__LCD_ID, 0);
    lcd_delay(10);
    lcd_rst_pinstate(__LCD_ID, 1);
    lcd_delay(100);
}

typedef struct {
    u8 cmd;
    u8 cnt;
    u8 dat[16];
} InitCode;

static const InitCode ili9481_code[] = {
    {0x01, 0},
    {REGFLAG_DELAY, 200},
    {0x13, 1, {0x00}},//开始正常显示模式
    {0x35, 1, {0x00}},//开TE 关TE 0x34
    {0x44, 2, {0x01, 0x50}}, //有关TE时间控制
    {0xc5, 1, {0x04}},//帧率控制寄存器
    {0xc5, 1, {0x07}},
    {0xe4, 1, {0xa0}},
    //屏幕和触摸共用一组3.3v屏拉电流导致触摸不灵敏
    //这里寄存器没有调到最佳 注释一下
    //显示存色的时候可能会出现像素点闪烁
    {0xd0, 3, {0x05, 0x40, 0x08}},
    {0xd1, 3, {0x00, 0x00, 0x10}},
    {0xd2, 2, {0x01, 0x00}},

    //电流够使用这组参数
    /* {0xd0, 3, {0x07, 0x41, 0x16}}, */
    /* {0xd1, 3, {0x00, 0x04, 0x1f}}, */
    /* {0xd2, 2, {0x01, 0x00}}, */

    {0xc0, 5, {0x00, 0x3b, 0x00, 0x02, 0x11}},
    /* {0xc8, 12, {0x00, 0x26, 0x21, 0x00, 0x00, 0x1f, 0x65, 0x23, 0x77, 0x00, 0x0f, 0x00}}, */
    {0xc8, 12, {0x00, 0x01, 0x47, 0x60, 0x04, 0x16, 0x03, 0x67, 0x67, 0x06, 0x0f, 0x00}},
    /* {0xc8, 12, {0x00, 0x37, 0x25, 0x06, 0x04, 0x1e, 0x26, 0x42, 0x77, 0x44, 0x0f, 0x12}}, */
    /* {0xc8, 12, {0x00, 0x30, 0x36, 0x45, 0x04, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0f, 0x00}}, */
    {0x2a, 4, {0x00, 0x00, 0x01, 0x3f}},
    {0x2b, 4, {0x00, 0x00, 0x01, 0xdf}},
    {0x3a, 1, {0x55}},
    {0x11, 0},//Sleep out
    {REGFLAG_DELAY, 200},

    {0x29, 0},
};

static void ili9481_init_code(const InitCode *code, u8 cnt)
{
    for (u8 i = 0; i < cnt; i++) {
        if (code[i].cmd == REGFLAG_DELAY) {
            lcd_delay(code[i].cnt);
        } else {
            WriteCOM(__LCD_ID, code[i].cmd);
            for (u8 j = 0; j < code[i].cnt; j++) {
                WriteDAT_8(__LCD_ID, code[i].dat[j]);
            }
        }
    }
}

static int ili9481_show_page(void *data)
{
    WriteCOM(__LCD_ID, 0x2c);
    WriteDAT_one_page(__LCD_ID, (u8 *)data, __LCD_W * __LCD_H * 2);

    return 0;
}

static void ili9481_test(void)
{
    static u8 *buf = NULL;
    u32 color;
    u32 i;
    buf = malloc(__LCD_W * __LCD_H * 2);
    if (buf == NULL) {
        printf("[ili9481] malloc buf error\n");
        return;
    }

    color = RED;
    for (i = 0; i < __LCD_W * __LCD_H * 1 / 4; i++) {
        buf[2 * i] = (color >> 8) & 0xff;
        buf[2 * i + 1] = color & 0xff;
    }

    color = GREEN;
    for (; i < __LCD_W * __LCD_H * 2 / 4; i++) {
        buf[2 * i] = (color >> 8) & 0xff;
        buf[2 * i + 1] = color & 0xff;
    }

    color = BLUE;
    for (; i < __LCD_W * __LCD_H * 3 / 4; i++) {
        buf[2 * i] = (color >> 8) & 0xff;
        buf[2 * i + 1] = color & 0xff;
    }

    color = YELLOW;
    for (; i < __LCD_W * __LCD_H * 4 / 4; i++) {
        buf[2 * i] = (color >> 8) & 0xff;
        buf[2 * i + 1] = color & 0xff;
    }

    while (1) {
        printf("ili9481_test");
        for (int j = 0;  j < __LCD_W * __LCD_H * 2; j++) {
            WriteDAT_8(__LCD_ID, buf[j]);
        }
        os_time_dly(100);
    }
}

static int ili9481_send_init_code(struct lcd_board_cfg *bd_cfg)
{
    ili9481_init_code(ili9481_code, ARRAY_SIZE(ili9481_code));
    return 0;
}

static int ili9481_check_id(struct lcd_board_cfg *bd_cfg)
{
    // 如果需要使用，注意板级中pap的RD使能以及IO口需要配置。
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

static void ili9481_backctrl(struct lcd_board_cfg *bd_cfg, u8 onoff)
{
    if (onoff) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}


static int ili9481_init(struct lcd_board_cfg *bd_cfg)
{
    printf("LCD_ili9481 init_start\n");
    ili9481_reset();
    ili9481_set_direction(0);
    /* thread_fork("ili9481_test", 28, 1024, 0, 0, ili9481_test, NULL); */
    printf("LCD_ili9481 config succes\n");
    return 0;
}

REGISTER_IMD_DEVICE_BEGIN(lcd_mcu_dev) = {
    .info = {
        .test_mode       = false,
        .test_mode_color = 0x0000ff,
        .bg_color    	 = 0x00ff00,
        .xres 			 = __LCD_W,
        .yres 			 = __LCD_H,
        .target_xres 	 = __LCD_W,
        .target_yres 	 = __LCD_H,
        .sample          = SAMP_YUV420,
        .format          = FORMAT_RGB565,
        .len 			 = LEN_256,
        .interlaced_mode = INTERLACED_NONE,
        .rotate          = ROTATE_0,

        .adjust = {
            .r_gain = 1.0,
            .g_gain = 1.0,
            .b_gain = 1.0,
            .bright_gain = 1.0,
            .contrast_gain = 1.0,
            .saturation_gain = 1.0,
            .mean = 128,
            .angle = 0,
            .mode = 0,
        },

        .in_rbs         = false, //输入数据RB交换
        .out_rbs        = false, //输出数据RB交换
        .in_fmt         = TCFG_LCD_INPUT_FORMAT,
        .out_fmt        = LCD_OUT_RGB565,
        .dither_en      = false, //使能dither 0低位截断 1dither
        .in_bt601       = false, //yuv422输入 转换0 bt601, 1 bt709
        .out_bt601      = false, //yuv422输出 转换0 bt601, 1 bt709
    },

    .data_out_endian = MODE_BE,  //RGB565模式生效 RGB888调节swap
    .set_io_hd       = 1,
    .ncycle          = CYCLE_TWO,
    .data_width      = PORT_8BITS,

    .dclk_set        = CLK_DIS,
    .sync0_set       = CLK_EN  | SIGNAL_DEN,
    .sync1_set       = CLK_DIS | SIGNAL_HSYNC,
    .sync2_set       = CLK_DIS | SIGNAL_VSYNC,

    .lcd_group       = PORT_GROUP_AA,
    .clk_cfg	 	 = PLL2_CLK_192M   | DIVA_3 | DIVB_2,
    .dclk_cfg        = DPI_CLK_STD_48M | DIVA_1 | DIVB_1,

    .timing = {
        .hori_total 		    = (320 + 40) * 2,
        .hori_sync 		        = 20 * 2,
        .hori_back_porth 	    = 20 * 2,
        .hori_pixel 	        = 320,

        .vert_total 		    = 480 + 20,
        .vert_sync 		        = 10,
        .vert_back_porth_odd 	= 10,
        .vert_back_porth_even 	= 0,
        .vert_pixel 	        = 480,
    },

},
REGISTER_IMD_DEVICE_END()

REGISTER_LCD_DEVICE_DRIVE(lcd_dev_mcu)  = {
    .logo            = "MCU_320x480_ILI9481",
    .id              = __LCD_ID,
    .type		     = LCD_MCU_SINGLE_FRAME,
    .dev    	     = &lcd_mcu_dev,
    .init		     = ili9481_init,
    .draw            = ili9481_show_page,
    .bl_ctrl	     = ili9481_backctrl,
    .check           = ili9481_check_id,
    .send_init_code  = ili9481_send_init_code,
};

#endif
