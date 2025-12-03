#include "lcd_driver.h"
#include "lcd_config.h"

/* 测试屏幕好坏开这个宏 */
/* #define USE_TEST_MODE     //推纯色 */

#if TCFG_LCD_MIPI_JD9165BA_1024x600

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
const static u8 init_cmd_list[] = {
//------------------------------------------------//
//IOVCC=1.8/3.3V, AVDD=9.8V, VGH=20V, VGL=-8, VCOM=3.5V
//1+2Dot inversion
//------------------------------------------------//
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(5),  0xF7, 0x49, 0x61, 0x02, 0x00,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x01,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x04, 0x0C,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x05, 0x08,
    /* _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x05, 0x09, ///< BIST测试模式选择BIT0。 0-HW 1-REG */
    /* _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x06, 0x40, ///< BIST测试模式使能BIT6。 0-normal 1-BIST */
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x20, 0x04, //r_lansel_sel_reg  //A2 add
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x0B, 0x13, //0x11(2lanes),0x12(3lanes),0x13(4lanes)
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x1F, 0x05, //mipi_hs_settle  //0x05->0x00 (P7_r01=04)
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x23, 0x38,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x28, 0x18,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x29, 0x29,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x2A, 0x01,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x2B, 0x29,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x2C, 0x01,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x02,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x00, 0x05,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x01, 0x22,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x02, 0x08,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x03, 0x12,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x04, 0x16,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x05, 0x64,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x06, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x07, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x08, 0x78,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x09, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x0A, 0x04,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x0B, 0x16, 0x17, 0x0B, 0x0D, 0x0D, 0x0D, 0x11, 0x10, 0x07, 0x07, 0x09,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x0C, 0x09, 0x1E, 0x1E, 0x1C, 0x1C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x0D, 0x0A, 0x05, 0x0B, 0x0D, 0x0D, 0x0D, 0x11, 0x10, 0x06, 0x06, 0x08,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x0E, 0x08, 0x1F, 0x1F, 0x1D, 0x1D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x0F, 0x0A, 0x05, 0x0D, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x1D, 0x1D, 0x1F,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x10, 0x1F, 0x08, 0x08, 0x06, 0x06, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x11, 0x16, 0x17, 0x0D, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x1C, 0x1C, 0x1E,
    _W, DELAY(0),   PACKET_DCS, SIZE(12), 0x12, 0x1E, 0x09, 0x09, 0x07, 0x07, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
    _W, DELAY(0),   PACKET_DCS, SIZE(5),  0x13, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(5),  0x14, 0x00, 0x00, 0x41, 0x41,
    _W, DELAY(0),   PACKET_DCS, SIZE(5),  0x15, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x17, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x18, 0x85,
    _W, DELAY(0),   PACKET_DCS, SIZE(3),  0x19, 0x06, 0x09,
    _W, DELAY(0),   PACKET_DCS, SIZE(3),  0x1A, 0x05, 0x08,
    _W, DELAY(0),   PACKET_DCS, SIZE(3),  0x1B, 0x0A, 0x04,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x26, 0x00,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x27, 0x00,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x06,
    _W, DELAY(0),   PACKET_DCS, SIZE(15), 0x12, 0x3F, 0x26, 0x27, 0x35, 0x2D, 0x34, 0x3F, 0x3F, 0x3F, 0x35, 0x2A, 0x20, 0x16, 0x08,
    _W, DELAY(0),   PACKET_DCS, SIZE(15), 0x13, 0x3F, 0x26, 0x28, 0x35, 0x27, 0x29, 0x29, 0x2F, 0x35, 0x2F, 0x26, 0x20, 0x16, 0x08,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x0A,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x02, 0x4F,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x0B, 0x40,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x0D,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x0D, 0x04, //mipi add  //0x0C, 0x04
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x10, 0x0C,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x11, 0x0C,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x12, 0x0C,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x13, 0x0C,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x07,
    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x0D, 0x01,

    _W, DELAY(0),   PACKET_DCS, SIZE(2),  0x30, 0x00,
    _W, DELAY(120), PACKET_DCS, SIZE(1),  0x11,
    _W, DELAY(20),  PACKET_DCS, SIZE(1),  0x29,
};

#define freq 312

/*
 *  bpp_num
 *  16: PIXEL_RGB565_COMMAND/PIXEL_RGB565_VIDEO
 *  18: PIXEL_RGB666/PIXEL_RGB666_LOOSELY
 *  24: PIXEL_RGB888
 */
#define bpp_num  24

#define vsa_line 2
#define vbp_line 21
#define vda_line 600
#define vfp_line 12

#define hsa_pixel  24
#define hbp_pixel  136
#define hda_pixel  1024
#define hfp_pixel  160

REGISTER_MIPI_DEVICE_BEGIN(mipi_dev_t) = {
    .info = {
#ifdef USE_TEST_MODE
        .test_mode 		    = true,
#else
        .test_mode 	        = false,
#endif
        .xres 			    = __LCD_W,
        .yres 			    = __LCD_H,
        .target_xres 	    = __LCD_W,
        .target_yres 	    = __LCD_H,
        .interpolation_en 	= 0,

        .test_mode_color    = 0x0000ff,
        .bg_color           = 0xff0000,
        .format 		    = FORMAT_RGB888,
        .len 			    = LEN_256,
        .interlaced_mode    = INTERLACED_NONE,
        .rotate             = ROTATE_0,

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
        .in_rbs             = false, //输入数据RB交换
        .out_rbs            = false, //输出数据RB交换
        .in_fmt             = TCFG_LCD_INPUT_FORMAT,
        .out_fmt            = LCD_OUT_RGB888,
        .dither_en          = false, //使能dither 0低位截断 1dither
        .in_bt601           = false, //yuv422输入 转换0 bt601, 1 bt709
        .out_bt601          = false, //yuv422输出 转换0 bt601, 1 bt709


    },
    .lane_mapping = {
        .x0_lane            = MIPI_LANE_EN | MIPI_LANE_D0,
        .x1_lane            = MIPI_LANE_EN | MIPI_LANE_CLK,
        .x2_lane            = MIPI_LANE_EN | MIPI_LANE_D1,
        .x3_lane            = MIPI_LANE_EN | MIPI_LANE_D2,
        .x4_lane            = MIPI_LANE_EN | MIPI_LANE_D3,
    },

    .video_timing = {
        .video_mode         = VIDEO_STREAM_VIDEO,
        .sync_mode          = SYNC_PULSE_MODE,
        .color_mode         = COLOR_FORMAT_RGB888,
        .pixel_type         = PIXEL_RGB888,
        .virtual_ch         = 0,
        .hs_eotp_en         = true,

        .dsi_vdo_vsa_v      = vsa_line,
        .dsi_vdo_vbp_v      = vbp_line,
        .dsi_vdo_vact_v     = vda_line,
        .dsi_vdo_vfp_v      = vfp_line,

        .dsi_vdo_hsa_v      = ((bpp_num * hsa_pixel) / 8) - 10,
        .dsi_vdo_hbp_v      = ((bpp_num * hbp_pixel) / 8) - 10,
        .dsi_vdo_hact_v     = ((bpp_num * hda_pixel) / 8),
        .dsi_vdo_hfp_v      = ((bpp_num * hfp_pixel) / 8) - 6,

        .dsi_vdo_bllp0_v    = ((bpp_num * (hbp_pixel + hda_pixel + hfp_pixel) / 8) - 10),
        .dsi_vdo_bllp1_v    = ((bpp_num * hda_pixel) / 8),
    },
    .timing = {
        /* 以下参数只需修改freq */
        .tval_lpx   = ((80     * freq / 1000) / 2 - 1),
        .tval_wkup  = ((100000 * freq / 1000) / 8 - 1),
        .tval_c_pre = ((128    * freq / 1000) / 2 - 1),
        .tval_c_sot = ((300    * freq / 1000) / 2 - 1),
        .tval_c_eot = ((100    * freq / 1000) / 2 - 1),
        .tval_c_brk = ((150    * freq / 1000) / 2 - 1),
        .tval_d_pre = ((128    * freq / 1000) / 2 - 1),
        .tval_d_sot = ((160    * freq / 1000) / 2 - 1),
        .tval_d_eot = ((100    * freq / 1000) / 2 - 1),
        .tval_d_brk = ((150    * freq / 1000) / 2 - 1),
        .tval_c_rdy = 400,
    },
    .pll4 = {
        .pll_freq = freq,
    },

    .cmd_list               = init_cmd_list,
    .cmd_list_item          = sizeof(init_cmd_list),
    .debug_mode             = false,
},
REGISTER_MIPI_DEVICE_END()


static int dsi_vdo_4lane_jd9165ba_init(struct lcd_board_cfg *bd_cfg)
{
    int lcd_reset = bd_cfg->lcd_io.lcd_reset;

    printf("JD9165BA init...\n");
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

    if (on) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

static int jd9165ba_check_id(struct lcd_board_cfg *bd_cfg)
{
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

REGISTER_LCD_DEVICE_DRIVE(dev)  = {
    .logo 	         = "MIPI_1024x600_JD9165BA",
    .id              = __LCD_ID,
    .type 	         = LCD_MIPI,
    .dev  	         = &mipi_dev_t,
    .init 	         = dsi_vdo_4lane_jd9165ba_init,
    .bl_ctrl         = mipi_backlight_ctrl,
    .check           = jd9165ba_check_id,
    .send_init_code  = NULL, ///< MIPI屏由lcd_driver读cmd_list自动发送init_code
};

#endif // TCFG_LCD_MIPI_JD9165BA_1024x600
