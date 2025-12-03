#include "lcd_driver.h"
#include "lcd_config.h"

/* 测试屏幕好坏开这个宏 */
// #define USE_TESE_MODE     //推纯色

#if TCFG_LCD_MIPI_ST7701S_480x640_ROTATE_90

#include "generic/typedef.h"
#include "asm/dsi.h"
#include "asm/dsi_api.h"
#include "gpio.h"

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

//------------------------------------------------------//
// lcd command initial
//------------------------------------------------------//
//command list
const static u8 init_cmd_list[] = {
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xEF, 0x08,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xC0, 0x4F, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xC1, 0x07, 0x02,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xC2, 0x31, 0x05,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xCC, 0x10,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xB0, 0x00, 0x0A, 0x11, 0x0C, 0x10, 0x05, 0x00, 0x08, 0x08, 0x1F, 0x07, 0x13, 0x10, 0xA9, 0x30, 0x18,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xB1, 0x00, 0x0B, 0x11, 0x0D, 0x0F, 0x05, 0x02, 0x07, 0x06, 0x20, 0x05, 0x15, 0x13, 0xA9, 0x30, 0x18,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB0, 0x53,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB1, 0x60,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB2, 0x87,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB3, 0x80,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB5, 0x49,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB7, 0x85,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB8, 0x21,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xBB, 0x33,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xBC, 0x33,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xC1, 0x78,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xC2, 0x78,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xD0, 0x88,
    _W, DELAY(0), PACKET_DCS, SIZE(7), 0xEF, 0x08, 0x08, 0x08, 0x45, 0x3F, 0x54,
    _W, DELAY(10), PACKET_DCS, SIZE(4), 0xE0, 0x00, 0x00, 0x02,
    _W, DELAY(0), PACKET_DCS, SIZE(12), 0xE1, 0x03, 0xA0, 0x00, 0x00, 0x02, 0xA0, 0x00, 0x00, 0x00, 0x33, 0x33,
    _W, DELAY(0), PACKET_DCS, SIZE(13), 0xE2, 0x22, 0x22, 0x33, 0x33, 0x88, 0xA0, 0x00, 0x00, 0x87, 0xA0, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(5), 0xE3, 0x00, 0x00, 0x22, 0x22,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xE4, 0x44, 0x44,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xE5, 0x04, 0x84, 0xA0, 0xA0, 0x06, 0x86, 0xA0, 0xA0, 0x08, 0x88, 0xA0, 0xA0, 0x0A, 0x8A, 0xA0, 0xA0,
    _W, DELAY(0), PACKET_DCS, SIZE(5), 0xE6, 0x00, 0x00, 0x22, 0x22,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xE7, 0x44, 0x44,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xE8, 0x03, 0x83, 0xA0, 0xA0, 0x05, 0x85, 0xA0, 0xA0, 0x07, 0x87, 0xA0, 0xA0, 0x09, 0x89, 0xA0, 0xA0,
    _W, DELAY(0), PACKET_DCS, SIZE(8), 0xEB, 0x00, 0x01, 0xE4, 0xE4, 0x88, 0x00, 0x40,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xEC, 0x3C, 0x01,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xED, 0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xE6, 0x16, 0x7C,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xE8, 0x00, 0x0E,
    _W, DELAY(0), PACKET_DCS, SIZE(1), 0x11,
    _W, DELAY(120), PACKET_DCS, SIZE(3), 0xE8, 0x00, 0x0C,
    _W, DELAY(10), PACKET_DCS, SIZE(3), 0xE8, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0x36, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(1), 0x29,
};

#define freq 280

/*
 *  bpp_num
 *  16: PIXEL_RGB565_COMMAND/PIXEL_RGB565_VIDEO
 *  18: PIXEL_RGB666/PIXEL_RGB666_LOOSELY
 *  24: PIXEL_RGB888
 */

#define bpp_num  24
#define lane_num 2

#define vsa_line 8
#define vbp_line 30
#define vda_line 640
#define vfp_line 30

#define hsa_pixel  8
#define hbp_pixel  60
#define hda_pixel  480
#define hfp_pixel  60

REGISTER_MIPI_DEVICE_BEGIN(mipi_dev_t) = {
    .info = {
#ifdef USE_TESE_MODE
        .test_mode 		= true,
#else
        .test_mode 	    = false,
#endif
        .x 			= 0,
        .y 			= 56,
        .xres 			= __LCD_W,
        .yres 			= __LCD_H,
        .target_xres 	= 640,
        .target_yres 	= 480,

        .test_mode_color = 0x0000ff,
        .bg_color   = 0xff0000,
        .format 		= FORMAT_RGB888,
        .len 			= LEN_256,
        .interlaced_mode = INTERLACED_NONE,
        .rotate = ROTATE_90,

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
    .lane_mapping = {
        .x0_lane = MIPI_LANE_EN | MIPI_LANE_D0,
        .x1_lane = MIPI_LANE_EN | MIPI_LANE_D1,
        .x2_lane = MIPI_LANE_EN | MIPI_LANE_CLK,
        .x3_lane = MIPI_LANE_DIS,
        .x4_lane = MIPI_LANE_DIS,
    },

    .video_timing = {
        .video_mode = VIDEO_STREAM_VIDEO,
        .sync_mode  = SYNC_PULSE_MODE,
        .color_mode = COLOR_FORMAT_RGB888,
        .pixel_type = PIXEL_RGB888,
        .virtual_ch = 0,
        .hs_eotp_en = true,

        .dsi_vdo_vsa_v  = vsa_line,
        .dsi_vdo_vbp_v  = vbp_line,
        .dsi_vdo_vact_v = vda_line,
        .dsi_vdo_vfp_v  = vfp_line,

        .dsi_vdo_hsa_v   = ((bpp_num * hsa_pixel) / 8) - 10,
        .dsi_vdo_hbp_v   = ((bpp_num * hbp_pixel) / 8) - 10,
        .dsi_vdo_hact_v  = ((bpp_num * hda_pixel) / 8),
        .dsi_vdo_hfp_v   = ((bpp_num * hfp_pixel) / 8) - 6,

        .dsi_vdo_bllp0_v = ((bpp_num * (hbp_pixel + hda_pixel + hfp_pixel) / 8) - 10),
        .dsi_vdo_bllp1_v = ((bpp_num * hda_pixel) / 8),
    },
    .timing = {
        /* 以下参数只需修改freq */
        .tval_lpx   = ((80     * freq / 1000) / 2 - 1),
        .tval_wkup  = ((100000 * freq / 1000) / 8 - 1),
        .tval_c_pre = ((40     * freq / 1000) / 2 - 1),
        .tval_c_sot = ((300    * freq / 1000) / 2 - 1),
        .tval_c_eot = ((100    * freq / 1000) / 2 - 1),
        .tval_c_brk = ((150    * freq / 1000) / 2 - 1),
        .tval_d_pre = ((60     * freq / 1000) / 2 - 1),
        .tval_d_sot = ((160    * freq / 1000) / 2 - 1),
        .tval_d_eot = ((100    * freq / 1000) / 2 - 1),
        .tval_d_brk = ((150    * freq / 1000) / 2 - 1),
        .tval_c_rdy = 400/* 64 */,
    },
    .pll4 = {
        .pll_freq = freq,
    },

    /* .pll_freq = freq * 2,[> 配置PLL频率的最佳范围为600MHz~1.2GHz,少于600MHz的频率通过二分频获得 <] */
    /* .pll_division = MIPI_PLL_DIV2, */

    .cmd_list = init_cmd_list,
    .cmd_list_item = sizeof(init_cmd_list),
    .debug_mode = false,
},
REGISTER_MIPI_DEVICE_END()

static int dsi_vdo_4lane_st7701s_init(struct lcd_board_cfg *bd_cfg)
{
    int lcd_reset = bd_cfg->lcd_io.lcd_reset;

    printf("ST7701S_360x640 init...\n");
    // lcd reset
    if (-1 != lcd_reset) {
        printf("lcd reset...\n");
        printf("lcd_reset value:%d\n", lcd_reset);
        msleep(1000);
        gpio_direction_output(lcd_reset, 1);
        msleep(100);
        gpio_direction_output(lcd_reset, 0);
        msleep(100);
        gpio_direction_output(lcd_reset, 1);
        msleep(100);
    }
    return 0;
}

static void mipi_backlight_ctrl(struct lcd_board_cfg *bd_cfg, u8 on)
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

static int st7701s_check_id(struct lcd_board_cfg *bd_cfg)
{
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

REGISTER_LCD_DEVICE_DRIVE(dev)  = {
    .logo 	         = "MIPI_480x640_ST7701S_ROTATE90",
    .id              = __LCD_ID,
    .type 	         = LCD_MIPI,
    .dev  	         = &mipi_dev_t,
    .init 	         = dsi_vdo_4lane_st7701s_init,
    .bl_ctrl         = mipi_backlight_ctrl,
    .check           = st7701s_check_id,
    .send_init_code  = NULL, ///< MIPI屏由lcd_driver读cmd_list自动发送init_code
};

#endif
