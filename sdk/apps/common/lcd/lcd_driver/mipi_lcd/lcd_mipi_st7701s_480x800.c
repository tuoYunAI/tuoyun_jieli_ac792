#include "lcd_driver.h"
#include "lcd_config.h"

/* 测试屏幕好坏开这个宏 */
/*#define USE_TEST_MODE     //推纯色 */

#if TCFG_LCD_MIPI_ST7701S_480x800

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
    /* _W, DELAY(0), PACKET_DCS, SIZE(2), 0xCD,0xAA, */
    /* _W, DELAY(120), PACKET_DCS, SIZE(1), 0x11, */
    /* _R, DELAY(0), PACKET_DCS, SIZE(2), 0xFA,0x01, */
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xC0, 0x63, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xC1, 0x11, 0x02,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xC2, 0x31, 0x08,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xCC, 0x10,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xB0, 0x00, 0x05, 0x0F, 0x0D, 0x13, 0x07, 0x01, 0x08, 0x09, 0x1E, 0x05, 0x12, 0x10, 0xB2, 0x2F, 0x18,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xB1, 0x00, 0x0F, 0x17, 0x0C, 0x0D, 0x05, 0x01, 0x08, 0x08, 0x1E, 0x05, 0x13, 0x11, 0xB2, 0x2F, 0x18,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB0, 0x8C,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB1, 0x5D,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB2, 0x07,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB3, 0x80,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB7, 0x47,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB7, 0x85,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB8, 0x20,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xB9, 0x10,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xC1, 0x78,
    _W, DELAY(0), PACKET_DCS, SIZE(2), 0xC2, 0x78,
    _W, DELAY(100), PACKET_DCS, SIZE(2), 0xD0, 0x88,
    _W, DELAY(0), PACKET_DCS, SIZE(4), 0xE0, 0x00, 0x00, 0x02,
    _W, DELAY(0), PACKET_DCS, SIZE(12), 0xE1, 0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00, 0x33, 0x33,
    _W, DELAY(0), PACKET_DCS, SIZE(14), 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    _W, DELAY(0), PACKET_DCS, SIZE(5), 0xE3, 0x00, 0x00, 0x33, 0x33,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xE4, 0x44, 0x44,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xE5, 0x0E, 0x60, 0xA0, 0xA0, 0x10, 0x60, 0xA0, 0xA0, 0x0A, 0x60, 0xA0, 0xA0, 0x0C, 0x60, 0xA0, 0xA0,
    _W, DELAY(0), PACKET_DCS, SIZE(5), 0xE6, 0x00, 0x00, 0x33, 0x33,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xE7, 0x44, 0x44,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xE8, 0x0D, 0x60, 0xA0, 0xA0, 0x0F, 0x60, 0xA0, 0xA0, 0x09, 0x60, 0xA0, 0xA0, 0x0B, 0x60, 0xA0, 0xA0,
    _W, DELAY(0), PACKET_DCS, SIZE(8), 0xEB, 0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40,
    _W, DELAY(0), PACKET_DCS, SIZE(3), 0xEC, 0x02, 0x01,
    _W, DELAY(0), PACKET_DCS, SIZE(17), 0xED, 0xAB, 0x89, 0x76, 0x54, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0x45, 0x67, 0x98, 0xBA,
    _W, DELAY(0), PACKET_DCS, SIZE(6), 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00,
    _W, DELAY(120), PACKET_DCS, SIZE(1), 0x11,
    _W, DELAY(0), PACKET_DCS, SIZE(1), 0x29,
};

#define freq 414

/*
 *  bpp_num
 *  16: PIXEL_RGB565_COMMAND/PIXEL_RGB565_VIDEO
 *  18: PIXEL_RGB666/PIXEL_RGB666_LOOSELY
 *  24: PIXEL_RGB888
 */
#define bpp_num   24
#define lane_num  2

#define vsa_line  4
#define vbp_line  8
#define vda_line  854
#define vfp_line  20

#define hsa_pixel 8
#define hbp_pixel 84
#define hda_pixel 480
#define hfp_pixel 88

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
        .x0_lane = MIPI_LANE_DIS,
        .x1_lane = MIPI_LANE_DIS,
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

    /* .pll_freq = freq * 2, // 配置PLL频率的最佳范围为600MHz~1.2GHz,少于600MHz的频率通过二分频获得 */
    /* .pll_division = MIPI_PLL_DIV2, */

    .cmd_list = init_cmd_list,
    .cmd_list_item = sizeof(init_cmd_list),
    .debug_mode = false,
},
REGISTER_MIPI_DEVICE_END()

static int dsi_vdo_4lane_st7701s_init(struct lcd_board_cfg *bd_cfg)
{
    int lcd_reset = bd_cfg->lcd_io.lcd_reset;

    printf("ST7701S init...\n");
#if 0
    //lcd_standby 由硬件接地拉低
    int lcd_standby = bd_cfg->lcd_io.lcd_standby;
    if (-1 != lcd_standby) { /*2 lane*/
        printf("lcd_standby = %d\n", lcd_standby);
        gpio_direction_output((u32)lcd_standby, 0);
    }
#endif

    // lcd reset
    if (-1 != lcd_reset) {
        gpio_direction_output((u32)lcd_reset, 0);
        msleep(10);
        gpio_direction_output((u32)lcd_reset, 1);
        msleep(10);
    }

    return 0;
}

static void mipi_backlight_ctrl(struct lcd_board_cfg *bd_cfg, u8 onoff)
{
    if (-1 == bd_cfg->lcd_io.backlight) {
        return;
    }
    if (onoff) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

#define ST7701S_COMAND_RDDID 0x04
#define ST7701S_ID1          0xFF
#define ST7701S_ID2          0xFF
#define ST7701S_ID3          0xFF
static int st7701s_check_id(struct lcd_board_cfg *bd_cfg)
{
#if 0
    u8 buf[3] = {0};
    dcs_read_parm(ST7701S_COMAND_RDDID, buf, sizeof(buf));
    printf("read st7701s id1 = 0x%x\n", buf[0]);
    printf("read st7701s id2 = 0x%x\n", buf[1]);
    printf("read st7701s id3 = 0x%x\n", buf[2]);
    if ((buf[0] != ST7701S_ID1) || (buf[1] != ST7701S_ID2) || (buf[2] != ST7701S_ID3)) {
        printf("st7701s id check fail!\n");
        return -1;
    }
    printf("st7701s id check pass!\n");
#endif
    return 0;
}

REGISTER_LCD_DEVICE_DRIVE(dev)  = {
    .logo 	         = "MIPI_480x800_ST7701S",
    .id              = __LCD_ID,
    .type 	         = LCD_MIPI,
    .dev  	         = &mipi_dev_t,
    .init 	         = dsi_vdo_4lane_st7701s_init,
    .bl_ctrl         = mipi_backlight_ctrl,
    .check           = st7701s_check_id,
    .send_init_code  = NULL, ///< MIPI屏由lcd_driver读cmd_list自动发送init_code
};

#endif
