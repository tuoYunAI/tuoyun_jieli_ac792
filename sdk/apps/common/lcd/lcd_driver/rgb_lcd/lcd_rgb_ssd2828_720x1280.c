#include "lcd_driver.h"
#include "lcd_config.h"
#include "gpio.h"

/* 测试屏幕好坏开这个宏 */
/* #define USE_TEST_MODE     //推纯色 */

#if TCFG_LCD_RGB_SSD2828_720x1280

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

static void lcd_720x1280_ssd2828_backctrl(struct lcd_board_cfg *bd_cfg, u8 on)
{
    if (-1 == bd_cfg->lcd_io.backlight) {
        return;
    }
    if (on) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

static int lcd_720x1280_ssd2828_init(struct lcd_board_cfg *bd_cfg)
{
    printf("lcd 720x1280_ssd2828 init ...\n");

    return 0;
}

static int lcd_720x1280_ssd2828_check_id(struct lcd_board_cfg *bd_cfg)
{
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

REGISTER_IMD_DEVICE_BEGIN(lcd_720x1280_ssd2828_dev) = {
    .info = {
#ifdef USE_TEST_MODE
        .test_mode 	     = true,
#else
        .test_mode 	     = false,
#endif
        .test_mode_color = 0xff0000,
        .bg_color   	 = 0x00ff00,//背景颜色
        .xres 			 = __LCD_W,
        .sample          = SAMP_YUV420,
        .yres 			 = __LCD_H,
        .target_xres     = __LCD_W,
        .target_yres     = __LCD_H,
        .format          = FORMAT_RGB888,
        .interlaced_1st_filed = EVEN_FILED,
        .interlaced_mode = INTERLACED_NONE,
        .len 			 = LEN_256,
        .rotate = ROTATE_0,
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
        .out_fmt        = LCD_OUT_RGB888,
        .dither_en      = false, //使能dither 0低位截断 1dither
        .in_bt601       = false, //yuv422输入 转换0 bt601, 1 bt709
        .out_bt601      = false, //yuv422输出 转换0 bt601, 1 bt709
    },

    .data_out_endian = MODE_LE, //RGB565模式生效 RGB888调节swap

    .dclk_set 		= CLK_NEGATIVE | CLK_EN,
    .sync0_set      = SIGNAL_DEN | CLK_EN,
    .sync1_set      = SIGNAL_HSYNC | CLK_NEGATIVE | CLK_EN,
    .sync2_set      = SIGNAL_VSYNC | CLK_NEGATIVE | CLK_EN,

    .set_io_hd      = TCFG_LCD_RGB_IO_HD_LEVEL,
    .lcd_group      = PORT_GROUP_AA,
    .ncycle         = CYCLE_ONE,
    .data_width     = PORT_24BITS,
    .clk_cfg    	= PLL2_CLK_192M | DIVA_5 | DIVB_8,
    .dclk_cfg       = DPI_CLK_STD_48M | DIVA_1 | DIVB_1,
    .timing = {

#define  HBP                    5     //行同步信号后肩 单位：clk
#define  HFP                    10    //行同步信号前肩 单位：clk
#define  HSPW                   1     //水平信号宽度   单位：clk
#define  VSPW                   10    //垂直信号宽度   单位：行
#define  VBP                    60    //帧同步信号后肩 单位：行
#define  VFP                    25    //帧同步信号前肩 单位：行

        //以下信号均为高有效1 以下为固定公式勿动
        .hori_total				= __LCD_W * 1 + HBP + HFP + HSPW, //行总长 : 行像素*3 + 行同步信号后肩 + 行同步信号前肩 + 水平信号宽度
        .hori_sync				= __LCD_W * 1 + HBP + HFP, 		//行同步时间:  行像素*3 + 行同步信号后肩 + 行同步信号前肩
        .hori_back_porth		= HBP,							//行同步信号后肩 (+HSPW?)
        .vert_total				= VSPW + VBP + __LCD_H + VFP, 	//帧总长: 垂直信号宽度 + 帧同步信号后肩 + 像素高 + 帧同步信号后肩
        .vert_sync				= VSPW,							//帧于帧之间间隔多少个行中断?
        .vert_back_porth_odd	= VBP + VSPW, 					//帧信号开始的时候经过多少个行中断开始de: 帧同步信号后肩 + 水平信号宽度
        .hori_pixel				= __LCD_W,                        //像素宽
        .vert_pixel				= __LCD_H,                        //像素高
        .vert_back_porth_even	= 0,                            //推隔行数据才需要配置
    },
},
REGISTER_IMD_DEVICE_END()


REGISTER_LCD_DEVICE_DRIVE(lcd_dev)  = {
    .logo            = "RGB_720x1280_SSD2828",
    .id              = __LCD_ID,
    .type		     = LCD_RGB,
    .dev    	     = &lcd_720x1280_ssd2828_dev,
    .init		     = lcd_720x1280_ssd2828_init,
    .bl_ctrl	     = lcd_720x1280_ssd2828_backctrl,
    .check           = lcd_720x1280_ssd2828_check_id,
    .send_init_code  = NULL,
};

#endif
