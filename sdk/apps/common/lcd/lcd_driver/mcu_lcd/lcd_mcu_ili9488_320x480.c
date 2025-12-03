#include "lcd_driver.h"
#include "lcd_config.h"
#include "asm/exti.h"
#include "gpio.h"

#if TCFG_LCD_MCU_ILI9488_320X480

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

#define REGFLAG_DELAY 0x45

// 设置ram刷新方向。0-旋转180, 1-旋转0
static void ili9488_set_direction(u8 dir)
{
    if (dir == 0) {
        WriteCOM(__LCD_ID, 0x36);
        WriteDAT_8(__LCD_ID, 0xd8);
    } else {
        WriteCOM(__LCD_ID, 0x36);
        WriteDAT_8(__LCD_ID, 0x08);
    }
}

static void ili9488_reset(void)
{
    printf("ili9488 mcu lcd reset\n");
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
    u8 dat[10];
} InitCode;

static const InitCode ili9488_code[] = {
    {0x21, 0},//开启数据翻转模式
    {0x13, 1, {0x00}},//开始正常显示模式
#if TCFG_LCD_TE_ENABLE
    {0x35, 1, {0x00}},//开启te
#else
    {0x34, 0},//关TE
#endif
    {0xB1, 2, {0x90, 0x11}}, //A0 11 60fps
    /* {0x36, 1, {0xd8}},//可以控制旋转/flip/rota: D7-Y D6-X B5-V B4-L B3-RGB B2-H */
    {0xC1, 1, {0x41}},
    {0XF7, 4, {0xA9, 0x51, 0x2C, 0x82}},
    {0xC1, 1, {0x41}},
    {0xC0, 2, {0x0f, 0x0f}},
    {0xC2, 1, {0x22}},
    {0XC5, 3, {0x00, 0x53, 0x80}},
    {0xB4, 1, {0x02}},
    {0xB7, 1, {0xc6}},
    {0xB6, 2, {0x02, 0x42}},
    {0xBE, 2, {0x00, 0x04}},
    {0xE9, 1, {0x00}},
    {0x3a, 1, {0x55}},
    {0x11, 0},//Sleep out
    {REGFLAG_DELAY, 200},
    {0x29, 0},//开显示
};

static void ili9488_init_code(const InitCode *code, u8 cnt)
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

static int ili9488_show_page(void *data)
{
    WriteCOM(__LCD_ID, 0x2c);
    WriteDAT_one_page(__LCD_ID, (u8 *)data, __LCD_W * __LCD_H * 2);
    return 0;
}

static void ili9488_test(void)
{
    static u8 *buf = NULL;
    u32 color;
    u32 i;
    buf = malloc(__LCD_W * __LCD_H * 2);
    if (buf == NULL) {
        printf("[ili9488] malloc buf error\n");
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
        printf("ili9488_test");
        for (int j = 0;  j < __LCD_W * __LCD_H * 2; j++) {
            WriteDAT_8(__LCD_ID, buf[j]);
        }
        os_time_dly(100);
    }
}

static int ili9488_send_init_code(struct lcd_board_cfg *bd_cfg)
{
    ili9488_init_code(ili9488_code, ARRAY_SIZE(ili9488_code));
    return 0;
}

#define ILI9488_REG_ID1 0xDA
#define ILI9488_REG_ID2 0xDB
#define ILI9488_REG_ID3 0xDC
#define ILI9488_ID1 0x54
#define ILI9488_ID2 0x80
#define ILI9488_ID3 0x66
static int ili9488_check_id(struct lcd_board_cfg *bd_cfg)
{
#if 0 // 默认关闭。如果需要使用，注意板级中pap的RD使能以及IO口需要配置。
    u8 data[2] = {0};  ///< 根据ILI9488手册，data0为dummy，data1为ID值
    ReadDAT(__LCD_ID, ILI9488_REG_ID1, data, sizeof(data));
    /* put_buf(data, 2); */
    if (data[1] != ILI9488_ID1) {
        return -1;
    }
    ReadDAT(__LCD_ID, ILI9488_REG_ID2, data, sizeof(data));
    /* put_buf(data, 2); */
    if (data[1] != ILI9488_ID2) {
        return -1;
    }
    ReadDAT(__LCD_ID, ILI9488_REG_ID3, data, sizeof(data));
    /* put_buf(data, 2); */
    if (data[1] != ILI9488_ID3) {
        return -1;
    }
#endif
    return 0;
}

static void ili9488_backctrl(struct lcd_board_cfg *bd_cfg, u8 onoff)
{
    if (onoff) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

static int ili9488_init(struct lcd_board_cfg *bd_cfg)
{
    printf("LCD_ili9488 init_start\n");
    ili9488_reset();
    ili9488_set_direction(0);
    /* thread_fork("ili9488_test", 28, 1024, 0, 0, ili9488_test, NULL); */
    printf("LCD_ili9488 config succes\n");
    return 0;
}

REGISTER_IMD_DEVICE_BEGIN(lcd_mcu_dev) = {
    .info = {
        .test_mode 	     = false,
        .test_mode_color = 0x0000ff,
        .bg_color   	 = 0x00ff00,
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

    .data_out_endian = MODE_BE,//RGB565模式生效 RGB888调节swap
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
    .logo            = "MCU_320x480_ILI9488",
    .id              = __LCD_ID,
    .type		     = LCD_MCU_SINGLE_FRAME,
    /* .type		     = LCD_MCU,  */
    .dev    	     = &lcd_mcu_dev,
    .init		     = ili9488_init,
    .draw            = ili9488_show_page,
    .bl_ctrl	     = ili9488_backctrl,
    .check           = ili9488_check_id,
    .send_init_code  = ili9488_send_init_code,
};

#endif // TCFG_LCD_MCU_ILI9488_320X480
