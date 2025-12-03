#include "lcd_driver.h"
#include "lcd_config.h"

/* 测试屏幕好坏开这个宏 */
/* #define USE_TEST_MODE     //推纯色 */

#if TCFG_LCD_MIPI_ST77922_480x480

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
    _W, DELAY(0),   PACKET_GENERIC, SIZE(1),  0x28,
    _W, DELAY(120), PACKET_GENERIC, SIZE(1),  0x10,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0xD0, 0x02,

    //======================CMD2======================
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0xF1, 0x00,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(4),  0x60, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0x65, 0x80,
    _W, DELAY(0),   PACKET_DCS,     SIZE(3),  0x66, 0x02, 0x3F,
    _W, DELAY(0),   PACKET_DCS,     SIZE(4),  0xBE, 0x24, 0x00, 0xED,
    // VFP, VBP, Gate line
    _W, DELAY(0),   PACKET_DCS,     SIZE(13), 0x70, 0x11, 0x9D, 0x11, 0xE0, 0xE0, 0x00, 0x08, 0x75, 0x00, 0x00, 0x00, 0x1A,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0x71, 0xD3, //video mode
    _W, DELAY(0),   PACKET_GENERIC, SIZE(4),  0x7B, 0x00, 0x08, 0x08,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(12), 0x80, 0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(5),  0x81, 0x26, 0x52, 0x72, 0x27,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0x84, 0x92, 0x25,
    _W, DELAY(0),   PACKET_DCS,     SIZE(15), 0x86, 0xC6, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x10, 0x13, 0x01, 0xA5, 0x00, 0xA5, 0xA5,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(7),  0x87, 0x10, 0x10, 0x58, 0x00, 0x02, 0x3A,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(16), 0x88, 0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x06,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(4),  0x89, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(12), 0x8A, 0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19,
    // VGL Pump
    _W, DELAY(0),   PACKET_GENERIC, SIZE(10), 0x8B, 0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E,
    // VGH Pump
    _W, DELAY(0),   PACKET_GENERIC, SIZE(14), 0x8C, 0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50, 0x0F, 0x01, 0xC5, 0x12, 0x09,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0x8D, 0x0C,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(7),  0x8E, 0x33, 0x01, 0x0C, 0x13, 0x01, 0x01,
    _W, DELAY(0),   PACKET_DCS,     SIZE(10), 0x90, 0x00, 0x44, 0x55, 0x7A, 0x00, 0x40, 0x40, 0x3F, 0x3F,   // 0125
    _W, DELAY(0),   PACKET_DCS,     SIZE(10), 0x91, 0x00, 0x44, 0x55, 0x7B, 0x00, 0x40, 0x7F, 0x3F, 0x3F,   // 0125
    _W, DELAY(0),   PACKET_DCS,     SIZE(11), 0x92, 0x00, 0x44, 0x55, 0x2F, 0x00, 0x30, 0x00, 0x05, 0x3F, 0x3F,
    _W, DELAY(0),   PACKET_DCS,     SIZE(11), 0x93, 0x00, 0x43, 0x11, 0x3F, 0x00, 0x3F, 0x00, 0x05, 0x3F, 0x3F,
    _W, DELAY(0),   PACKET_DCS,     SIZE(7),  0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS,     SIZE(6),  0x95, 0x9D, 0x1D, 0x00, 0x00, 0xFF,
    _W, DELAY(0),   PACKET_DCS,     SIZE(13), 0x96, 0x44, 0x44, 0x07, 0x16, 0x3A, 0x3B, 0x01, 0x00, 0x3F, 0x3F, 0x00, 0x40,
    _W, DELAY(0),   PACKET_DCS,     SIZE(13), 0x97, 0x44, 0x44, 0x25, 0x34, 0x3C, 0x3D, 0x1F, 0x1E, 0x3F, 0x3F, 0x00, 0x40,
    _W, DELAY(0),   PACKET_DCS,     SIZE(6),  0xBA, 0x55, 0x3F, 0x3F, 0x3F, 0x3F,
    _W, DELAY(0),   PACKET_DCS,     SIZE(8),  0x9A, 0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS,     SIZE(8),  0x9B, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS,     SIZE(14), 0x9C, 0x40, 0x12, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS,     SIZE(9),  0x9D, 0x80, 0x53, 0x00, 0x00, 0x00, 0x80, 0x64, 0x01,
    _W, DELAY(0),   PACKET_DCS,     SIZE(8),  0x9E, 0x53, 0x00, 0x00, 0x00, 0x80, 0x64, 0x01,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(5),  0x9F, 0xA0, 0x09, 0x00, 0x57,
    _W, DELAY(0),   PACKET_DCS,     SIZE(8),  0xB3, 0x00, 0x30, 0x0F, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0),   PACKET_DCS,     SIZE(13), 0xB4, 0x10, 0x09, 0x0B, 0x02, 0x00, 0x19, 0x18, 0x13, 0x1E, 0x1D, 0x1C, 0x1E,  // 0125
    _W, DELAY(0),   PACKET_DCS,     SIZE(13), 0xB5, 0x08, 0x12, 0x03, 0x0A, 0x19, 0x01, 0x11, 0x18, 0x1D, 0x1E, 0x1E, 0x1C,  // 0125
    _W, DELAY(0),   PACKET_DCS,     SIZE(8),  0xB6, 0xFF, 0xFF, 0x00, 0x07, 0xFF, 0x0B, 0xFF,
    // GammaP
    _W, DELAY(0),   PACKET_GENERIC, SIZE(17), 0xB7, 0x00, 0x0B, 0x12, 0x0A, 0x0B, 0x06, 0x37, 0x00, 0x02, 0x4D, 0x08, 0x14, 0x14, 0x30, 0x36, 0x0F,
    // GammaN
    _W, DELAY(0),   PACKET_GENERIC, SIZE(17), 0xB8, 0x00, 0x0B, 0x11, 0x09, 0x09, 0x06, 0x37, 0x06, 0x05, 0x4D, 0x08, 0x13, 0x13, 0x2F, 0x36, 0x0F,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0xB9, 0x23, 0x23,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0xBB, 0x00, 0x00,
    // VGHP/VGLP
    _W, DELAY(0),   PACKET_GENERIC, SIZE(7),  0xBF, 0x0F, 0x13, 0x13, 0x09, 0x09, 0x09,

    // ======================CMD3======================
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0xF2, 0x00,
    // VOP= 5v
    _W, DELAY(0),   PACKET_GENERIC, SIZE(6),  0x73, 0x04, 0xBA, 0x12, 0x5E, 0x55,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(6),  0x77, 0x6B, 0x5B, 0xFD, 0xC3, 0xC5,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0x7A, 0x15, 0x27,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0x7B, 0x04, 0x57,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0x7E, 0x01, 0x0E,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0xBF, 0x36,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(3),  0xE3, 0x40, 0x40, // VMF

    // ======================CMD1======================
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0xF0, 0x00,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(1),  0x21,
    _W, DELAY(120), PACKET_DCS,     SIZE(1),  0x11,
    _W, DELAY(0),   PACKET_DCS,     SIZE(1),  0x29,
    _W, DELAY(0),   PACKET_GENERIC, SIZE(2),  0x35, 0x00,
};

#define freq 480

/*
 *  bpp_num
 *  16: PIXEL_RGB565_COMMAND/PIXEL_RGB565_VIDEO
 *  18: PIXEL_RGB666/PIXEL_RGB666_LOOSELY
 *  24: PIXEL_RGB888
 */
#define bpp_num  24
/* #define lane_num 2 */

#define vsa_line 2
#define vbp_line 6
#define vda_line 480
#define vfp_line 117

#define hsa_pixel  2
#define hbp_pixel  40
#define hda_pixel  480
#define hfp_pixel  40

REGISTER_MIPI_DEVICE_BEGIN(mipi_dev_t) = {
    .info = {
#ifdef USE_TEST_MODE
        .test_mode 		 = true,
#else
        .test_mode 	     = false,
#endif
        .xres 			 = __LCD_W,
        .yres 			 = __LCD_H,
        .target_xres 	 = __LCD_W,
        .target_yres 	 = __LCD_H,

        .test_mode_color = 0x0000ff,
        .bg_color        = 0xff0000,
        .format 		 = FORMAT_RGB888,
        .len 			 = LEN_256,
        .interlaced_mode = INTERLACED_NONE,
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
    .lane_mapping = {
        .x0_lane = MIPI_LANE_EN | MIPI_LANE_D0,
        .x1_lane = MIPI_LANE_EN | MIPI_LANE_CLK,
        .x2_lane = MIPI_LANE_DIS,
        .x3_lane = MIPI_LANE_DIS,
        .x4_lane = MIPI_LANE_DIS,
    },

    .video_timing = {
        .video_mode = VIDEO_STREAM_VIDEO,
        .sync_mode  = SYNC_EVENT_MODE,
        .color_mode = COLOR_FORMAT_RGB888,
        .pixel_type = PIXEL_RGB888,
        .virtual_ch = 0,
        .hs_eotp_en = false,//true,

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

static int dsi_vdo_1lane_st77922_init(struct lcd_board_cfg *bd_cfg)
{
    int lcd_reset = bd_cfg->lcd_io.lcd_reset;

    printf("ST77922 init...\n");
    // lcd reset
    if (-1 != lcd_reset) {
        gpio_direction_output(lcd_reset, 0);
        msleep(10);
        gpio_direction_output(lcd_reset, 1);
        msleep(120);
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

static int st77922_check_id(struct lcd_board_cfg *bd_cfg)
{
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

REGISTER_LCD_DEVICE_DRIVE(dev)  = {
    .logo 	         = "MIPI_480x480_ST77922",
    .id              = __LCD_ID,
    .type 	         = LCD_MIPI,
    .dev  	         = &mipi_dev_t,
    .init 	         = dsi_vdo_1lane_st77922_init,
    .bl_ctrl         = mipi_backlight_ctrl,
    .check           = st77922_check_id,
    .send_init_code  = NULL, ///< MIPI屏由lcd_driver读cmd_list自动发送init_code
};

#endif // TCFG_LCD_MIPI_ST77922_480x480
