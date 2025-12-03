#include "lcd_driver.h"
#include "lcd_config.h"

/* 测试屏幕好坏开这个宏 */
/* #define USE_TEST_MODE     //推纯色 */

#if TCFG_LCD_MIPI_ILI9881C_800x1280

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
    _W, DELAY(0),  PACKET_DCS, SIZE(4), 0xFF, 0x98, 0x81, 0x03,//PAGE3
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x01, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x02, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x03, 0x53,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x04, 0x53,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x05, 0x13,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x06, 0x04,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x07, 0x02,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x08, 0x02,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x09, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x0a, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x0b, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x0c, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x0d, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x0e, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x0f, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x10, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x11, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x12, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x13, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x14, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x15, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x16, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x17, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x18, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x19, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x1a, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x1b, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x1c, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x1d, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x1e, 0xc0,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x1f, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x20, 0x02,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x21, 0x09,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x22, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x23, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x24, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x25, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x26, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x27, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x28, 0x55,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x29, 0x03,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2a, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2b, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2c, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2d, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2e, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2f, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x30, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x31, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x32, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x33, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x34, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x35, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x36, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x37, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x38, 0x3C,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x39, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3a, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3b, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3c, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3d, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3e, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3f, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x40, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x41, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x42, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x43, 0x00,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x44, 0x00,
    // _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x45, 0x00,

    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x50, 0x01,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x51, 0x23,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x52, 0x45,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x53, 0x67,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x54, 0x89,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x55, 0xab,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x56, 0x01,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x57, 0x23,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x58, 0x45,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x59, 0x67,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x5a, 0x89,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x5b, 0xab,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x5c, 0xcd,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x5d, 0xef,

    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x5e, 0x01,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x5f, 0x0A,//FW_CGOUT_L[1] RESE_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x60, 0x02,//FW_CGOUT_L[2] VSSG_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x61, 0x02,//FW_CGOUT_L[3] VSSG_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x62, 0x08,//FW_CGOUT_L[4] STV2_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x63, 0x15,//FW_CGOUT_L[5] VDD2_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x64, 0x14,//FW_CGOUT_L[6] VDD1_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x65, 0x02,//FW_CGOUT_L[7]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x66, 0x11,//FW_CGOUT_L[8] CK11
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x67, 0x10,//FW_CGOUT_L[9] CK9
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x68, 0x02,//FW_CGOUT_L[10]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x69, 0x0F,//FW_CGOUT_L[11] CK7
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6a, 0x0E,//FW_CGOUT_L[12] CK5
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6b, 0x02,//FW_CGOUT_L[13]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6c, 0x0D,//FW_CGOUT_L[14] CK3
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6d, 0x0C,//FW_CGOUT_L[15] CK1
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6e, 0x06,//FW_CGOUT_L[16] STV1_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6f, 0x02,//FW_CGOUT_L[17]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x70, 0x02,//FW_CGOUT_L[18]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x71, 0x02,//FW_CGOUT_L[19]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x72, 0x02,//FW_CGOUT_L[20]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x73, 0x02,//FW_CGOUT_L[21]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x74, 0x02,//FW_CGOUT_L[22]

    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x75, 0x0A,//BW_CGOUT_L[1]   RESE_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x76, 0x02,//BW_CGOUT_L[2]   VSSG_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x77, 0x02,//BW_CGOUT_L[3]   VSSG_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x78, 0x06,//BW_CGOUT_L[4]   STV2_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x79, 0x15,//BW_CGOUT_L[5]   VDD2_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x7a, 0x14,//BW_CGOUT_L[6]   VDD1_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x7b, 0x02,//BW_CGOUT_L[7]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x7c, 0x10,//BW_CGOUT_L[8]   CK11
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x7d, 0x11,//BW_CGOUT_L[9]   CK9
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x7e, 0x02,//BW_CGOUT_L[10]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x7f, 0x0C,//BW_CGOUT_L[11]  CK7
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x80, 0x0D,//BW_CGOUT_L[12]  CK5
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x81, 0x02,//BW_CGOUT_L[13]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x82, 0x0E,//BW_CGOUT_L[14]  CK3
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x83, 0x0F,//BW_CGOUT_L[15]  CK1
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x84, 0x08,//BW_CGOUT_L[16]  STV1_ODD
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x85, 0x02,//BW_CGOUT_L[17]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x86, 0x02,//BW_CGOUT_L[18]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x87, 0x02,//BW_CGOUT_L[19]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x88, 0x02,//BW_CGOUT_L[20]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x89, 0x02,//BW_CGOUT_L[21]
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x8A, 0x02,//BW_CGOUT_L[22]

    _W, DELAY(0),  PACKET_DCS, SIZE(4), 0xFF, 0x98, 0x81, 0x04,//PAGE4
    // _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x00, 0x00,//3L  打开此行
    // _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x2F, 0x01,//BIST
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3B, 0xC0,//ILI4003D sel
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6C, 0x15,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6E, 0x30,//VGH 16V
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x6F, 0x55,//Pump ratio VGH=VSPX4 VGL=VSNX4
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x3A, 0x24,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x8D, 0x1F,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x87, 0xBA,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x26, 0x76,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xB2, 0xD1,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xB5, 0x07,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x35, 0x1F,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x88, 0x0B,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x21, 0x30,

    _W, DELAY(0),  PACKET_DCS, SIZE(4), 0xFF, 0x98, 0x81, 0x01, //PAGE1
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x22, 0x0A,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x31, 0x09,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x40, 0x33,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x53, 0x37,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x55, 0x38,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x50, 0x95,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x51, 0x95,
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x60, 0x30,

    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA0, 0x0F,//VP255Gamma P
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA1, 0x17,//VP251
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA2, 0x22,//VP247
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA3, 0x19,//VP243
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA4, 0x15,//VP239
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA5, 0x28,//VP231
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA6, 0x1C,//VP219
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA7, 0x1C,//VP203
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA8, 0x78,//VP175
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xA9, 0x1C,//VP144
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xAA, 0x28,//VP111
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xAB, 0x69,//VP80
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xAC, 0x1A,//VP52
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xAD, 0x19,//VP36
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xAE, 0x4B,//VP24
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xAF, 0x22,//VP16
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xB0, 0x2A,//VP12
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xB1, 0x4B,//VP8
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xB2, 0x6B,//VP4
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xB3, 0x3F,//VP0

    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC0, 0x01,//VN255 GAMMA N
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC1, 0x17,//VN251
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC2, 0x22,//VN247
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC3, 0x19,//VN243
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC4, 0x15,//VN239
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC5, 0x28,//VN231
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC6, 0x1C,//VN219
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC7, 0x1D,//VN203
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC8, 0x78,//VN175
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xC9, 0x1C,//VN144
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xCA, 0x28,//VN111
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xCB, 0x69,//VN80
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xCC, 0x1A,//VN52
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xCD, 0x19,//VN36
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xCE, 0x4B,//VN24
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xCF, 0x22,//VN16
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xD0, 0x2A,//VN12
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xD1, 0x4B,//VN8
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xD2, 0x6B,//VN4
    _W, DELAY(0),  PACKET_DCS, SIZE(2), 0xD3, 0x3F,//VN0

    _W, DELAY(0),  PACKET_DCS, SIZE(4), 0xFF, 0x98, 0x81, 0x00, //PAGE0
    // _W, DELAY(0),  PACKET_DCS, SIZE(2), 0x35, 0x00,//TE OUT
    _W, DELAY(120), PACKET_DCS, SIZE(2), 0x11, 0x00, //sleep out
    _W, DELAY(5),  PACKET_DCS, SIZE(2), 0x29, 0x00,//display on
};

#define freq 400
/* #define freq 370 */

/*
 *  bpp_num
 *  16: PIXEL_RGB565_COMMAND/PIXEL_RGB565_VIDEO
 *  18: PIXEL_RGB666/PIXEL_RGB666_LOOSELY
 *  24: PIXEL_RGB888
 */
#define bpp_num   24

#define vsa_line  6
#define vbp_line  16
#define vda_line  1280
#define vfp_line  15

#define hsa_pixel  8
#define hbp_pixel  48
#define hda_pixel  800
#define hfp_pixel  52

REGISTER_MIPI_DEVICE_BEGIN(mipi_dev_t) = {
    .info = {
#ifdef USE_TEST_MODE
        .test_mode 		= true,
#else
        .test_mode 	    = false,
#endif
        .xres 			= __LCD_W,
        .yres 			= __LCD_H,
        .target_xres 	= __LCD_W,
        .target_yres 	= __LCD_H,

        .test_mode_color = 0x0000ff,
        .bg_color       = 0xff0000,
        .format 		= FORMAT_RGB888,
        .len 			= LEN_256,
        .interlaced_mode = INTERLACED_NONE,

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
        .x3_lane = MIPI_LANE_EN | MIPI_LANE_D2,
        .x4_lane = MIPI_LANE_EN | MIPI_LANE_D3,
    },

    .video_timing = {
        .video_mode = VIDEO_STREAM_VIDEO,
        .sync_mode  = SYNC_EVENT_MODE,
        // .sync_mode  = SYNC_PULSE_MODE,
        // .sync_mode  = BURST_MODE,

        .color_mode = COLOR_FORMAT_RGB888,
        .pixel_type = PIXEL_RGB888,
        .virtual_ch = 0,
        .hs_eotp_en = false,
        /* .hs_eotp_en = true, */

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
        .tval_c_rdy = 400,
    },
    .pll4 = {
        .pll_freq = freq,
    },

    .cmd_list = init_cmd_list,
    .cmd_list_item = sizeof(init_cmd_list),
    .debug_mode = false,
},
REGISTER_MIPI_DEVICE_END()

static int dsi_vdo_4lane_ili9881c_init(struct lcd_board_cfg *bd_cfg)
{
    int lcd_reset = bd_cfg->lcd_io.lcd_reset;

    printf("ILI9881C init...\n");

    // reset
    if (-1 != lcd_reset) {
        gpio_direction_output(lcd_reset, 1);
        msleep(5);
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

static int ili9881c_check_id(struct lcd_board_cfg *bd_cfg)
{
    // TODO: check id
    // if check fail, return -1.
    return 0;
}

REGISTER_LCD_DEVICE_DRIVE(dev)  = {
    .logo 	         = "MIPI_800x1280_ILI9881C",
    .id              = __LCD_ID,
    .type 	         = LCD_MIPI,
    .dev  	         = &mipi_dev_t,
    .init 	         = dsi_vdo_4lane_ili9881c_init,
    .bl_ctrl         = mipi_backlight_ctrl,
    .check           = ili9881c_check_id,
    .send_init_code  = NULL, ///< MIPI屏由lcd_driver读cmd_list自动发送init_code
};


#endif
