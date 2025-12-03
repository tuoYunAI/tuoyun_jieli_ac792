#include "lcd_driver.h"
#include "lcd_config.h"

/* 测试屏幕好坏开这个宏 */
/*#define USE_TESE_MODE     //推纯色  */

#if TCFG_LCD_MIPI_FL7707_480x1280

#include "generic/typedef.h"
#include "asm/dsi.h"
#include "asm/dsi_api.h"
#include "gpio.h"
//#include "lcd_backlight_pwm.h"

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

//------------------------------------------------------//
// lcd command initial
//------------------------------------------------------//
//command list
const static u8 init_cmd_list[] = {
    /* _W, DELAY(0), PACKET_DCS, SIZE(2), 0xCD,0xAA, */
    /* _W, DELAY(120), PACKET_DCS, SIZE(1), 0x11, */
    /* _R, DELAY(0), PACKET_DCS, SIZE(2), 0xFA,0x01, */
    _W, DELAY(0), PACKET_DCS, SIZE(4), 0xB9, 0xF1, 0x12, 0x87,
    _W, DELAY(0), PACKET_DCS, SIZE(4), 0xB2, 0x40, 0x05, 0x78,
    _W, DELAY(0), PACKET_DCS, SIZE(11), 0xB3, 0x10, 0x10, 0x28, 0x28, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB4, 0x80,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xB5, 0x0F, 0x0F,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xB6, 0x1E, 0x1E,
    _W, DELAY(0), PACKET_DCS, SIZE(5), 0xB8, 0x26, 0x22, 0xF0, 0x13,//0x73for3PowerMode,0x23forPowerICMode
    _W, DELAY(0), PACKET_DCS, SIZE(28), 0xBA, 0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x91, 0x0A,
    0x00, 0x00, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x37,//2Lane:0x31 3Lane:0x32 4Lane:0x33
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xBC, 0x47,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xBF, 0x02, 0x10, 0x00, 0x80, 0x04,
    _W, DELAY(0), PACKET_DCS, SIZE(10), 0xC0, 0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x12, 0x73, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(18), 0xC1, 0x54, 0x80, 0x32, 0x32, 0x77, 0xE4, 0x77, 0x77, 0xCC, 0xCC, 0xFF, 0xFF, 0x11, 0x11, 0x00, 0x00, 0x32,
    _W, DELAY(0), PACKET_DCS, SIZE(13), 0xC7, 0x10, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0xED, 0xC5, 0x00, 0xA5,
    _W, DELAY(0), PACKET_DCS, SIZE(5), 0xC8, 0x10, 0x40, 0x1E, 0x03,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xCC, 0x07, //forward:0x0Bbackward:0x07
    _W, DELAY(0), PACKET_DCS, SIZE(35), 0xE0, 0x00, 0x0D, 0x11, 0x29, 0x3A, 0x3F, 0x48, 0x3E, 0x06,
    0x08, 0x0E, 0x13, 0x14, 0x12, 0x13, 0x12, 0x19, 0x00, 0x0D,
    0x11, 0x29, 0x3A, 0x3F, 0x48, 0x3E, 0x06, 0x08, 0x0E, 0x13,
    0x14, 0x12, 0x13, 0x12, 0x19, //SetGamma2.2

    _W, DELAY(0), PACKET_DCS, SIZE(8), 0xE1, 0x11, 0x11, 0x91, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(15), 0xE3, 0x07, 0x07, 0x0B, 0x0B, 0x0B, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x04, 0xC0, 0x10,
    _W, DELAY(0), PACKET_DCS, SIZE(64), 0xE9, 0xC8, 0x10, 0x07, 0x04, 0xFE, 0x80, 0x81, 0x12, 0x31,
    0x23, 0x47, 0x83, 0x01, 0x81, 0x27, 0x38, 0x0C, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x0C, 0x00, 0x03, 0x00, 0x00, 0x00, 0x75,
    0x75, 0x31, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x13, 0x88,
    0x64, 0x64, 0x20, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x02,
    0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(62), 0xEA, 0x02, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x46, 0x02, 0x88, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x64, 0x88, 0x13, 0x57, 0x13, 0x88, 0x88, 0x88,
    0x88, 0x88, 0x88, 0x75, 0x88, 0x23, 0x30, 0x00, 0x00, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x81, 0x00, 0x00,
    0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(4), 0xEF, 0xFF, 0xFF, 0x01,
    //_W, DELAY(0), PACKET_DCS, SIZE(2), 0x36,0xB2,
    _W, DELAY(250), PACKET_DCS, SIZE(1), 0x11,
    _W, DELAY(50), PACKET_DCS, SIZE(1), 0x29,
};

#define freq 384

/*
 *  bpp_num
 *  16: PIXEL_RGB565_COMMAND/PIXEL_RGB565_VIDEO
 *  18: PIXEL_RGB666/PIXEL_RGB666_LOOSELY
 *  24: PIXEL_RGB888
 */
#define bpp_num  24
#define lane_num 4

#define vsa_line 4//4//2
#define vbp_line 20//8//4
#define vda_line 1280//854//800
#define vfp_line 20//20//10

#define hsa_pixel  110//8//8
#define hbp_pixel  110//84//42
#define hda_pixel  480//480
#define hfp_pixel  110//88//44

REGISTER_MIPI_DEVICE_BEGIN(mipi_dev_t) = {
    .info = {
#ifdef USE_TESE_MODE
        .test_mode 		= true,
#else
        .test_mode 	    = false,
#endif
        .xres 			= __LCD_W,
        .yres 			= __LCD_H,
        .target_xres 	= __LCD_W,
        .target_yres 	= __LCD_H,

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
        .x0_lane = MIPI_LANE_EN | MIPI_LANE_D3,
        .x1_lane = MIPI_LANE_EN | MIPI_LANE_D2,
        .x2_lane = MIPI_LANE_EN | MIPI_LANE_CLK,
        .x3_lane = MIPI_LANE_EN | MIPI_LANE_D1,
        .x4_lane = MIPI_LANE_EN | MIPI_LANE_D0,

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
    .debug_mode = true,
},
REGISTER_MIPI_DEVICE_END()

static int dsi_vdo_4lane_fl7707_init(struct lcd_board_cfg *bd_cfg)
{
    int lcd_reset = bd_cfg->lcd_io.lcd_reset;

    printf("FL7707 init...\n");
    // lcd reset
    if (-1 != lcd_reset) {
        gpio_direction_output(lcd_reset, 0);
        msleep(10);
        gpio_direction_output(lcd_reset, 1);
        msleep(10);
    }
    return 0;
}

static void mipi_backlight_ctrl(struct lcd_board_cfg *bd_cfg, u8 on)
{
    if (-1 == bd_cfg->lcd_io.backlight) {
        return;
    }
#ifdef TCFG_LCD_BACKLIGHT_PWM_ENABLE
    if (lcd_backlight_pwm_get_vaild()) {
        if (on) {
            lcd_backlight_pwm_set_on();
        } else {
            lcd_backlight_pwm_set_off();
        }
    } else {
        if (on) {
            gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
        } else {
            gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
        }
    }
#else
    if (on) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
#endif
}

static int fl7707_check_id(struct lcd_board_cfg *bd_cfg)
{
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

REGISTER_LCD_DEVICE_DRIVE(dev)  = {
    .logo 	         = "MIPI_480x1280_FL7707",
    .id              = __LCD_ID,
    .type 	         = LCD_MIPI,
    .dev  	         = &mipi_dev_t,
    .init 	         = dsi_vdo_4lane_fl7707_init,
    .bl_ctrl         = mipi_backlight_ctrl,
    .check           = fl7707_check_id,
    .send_init_code  = NULL, ///< MIPI屏由lcd_driver读cmd_list自动发送init_code
};

#endif


