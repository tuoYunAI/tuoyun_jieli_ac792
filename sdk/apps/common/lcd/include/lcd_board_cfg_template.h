/**
 * @file lcd_board_cfg_template.h
 * @brief lcd板级配置的模版。供开发者参考，也方便内部开发调试。
 * @date 2024-12-12
 */
#ifndef __LCD_BOARD_CFG_TEMPLATE_H__
#define __LCD_BOARD_CFG_TEMPLATE_H__

#ifndef TCFG_LCD_INPUT_FORMAT
#define TCFG_LCD_INPUT_FORMAT                   LCD_IN_RGB565
#endif


//*******************************屏驱配置使能*******************************//
//MIPI LCD
/* #define TCFG_LCD_MIPI_ST7701S_480x800           0 ///< WL82开发板标配屏，已直接放板级配置文件中 */
/* #define TCFG_LCD_MIPI_ST7701S_480x800_ROTATE_90 0 ///< WL82开发板标配屏，已直接放板级配置文件中*/
#define TCFG_LCD_MIPI_ST7701S_480x640_ROTATE_90 0
#define TCFG_LCD_MIPI_ST7703_720x720            0
#define TCFG_LCD_MIPI_ILI9881C_800x1280         0
#define TCFG_LCD_MIPI_FL7707_480x1280           0
#define TCFG_LCD_MIPI_JD9165BA_1024x600         0
#define TCFG_LCD_MIPI_ST77922_480x480           0

//MCU LCD
#define TCFG_LCD_MCU_ILI9488_320X480            0
#define TCFG_LCD_MCU_ILI9481_320X480            0

//SPI LCD
#define TCFG_LCD_SPI_GC9307_240X296             0
#define TCFG_LCD_SPI_GC9307_172X320             0
#define TCFG_LCD_SPI_NV3006A_142X428            0

//RGB_LCD
#define TCFG_LCD_RGB_XXXX_480x272               0
#define TCFG_LCD_RGB_XXXX_1280x800              0
#define TCFG_LCD_RGB_SSD2828_720x1280           0
#define TCFG_LCD_RGB_ST7701S_480x800            0
#define TCFG_LCD_RGB_HX8264_800x480             0



//*******************************屏驱IO配置*******************************//
/*
 * #if TCFG_LCD_MIPI_ST7701S_480x800
 * #define TCFG_LCD_DEVICE_NAME                "MIPI_480x800_ST7701S"
 * #define TCFG_LCD_BL_VALUE                   1
 * #define TCFG_LCD_RESET_IO                   IO_PORTB_00
 * #define TCFG_LCD_BL_IO                      IO_PORTB_01
 * #define TCFG_LCD_RS_IO                      -1
 * #define TCFG_LCD_CS_IO                      -1
 * #define TCFG_LCD_TE_ENABLE                  0
 * #define TCFG_LCD_TE_IO                      -1
 * #define TCFG_LCD_SPI_INTERFACE              NULL
 * #endif
 *
 * #if TCFG_LCD_MIPI_ST7701S_480x800_ROTATE_90
 * #define TCFG_LCD_DEVICE_NAME                "MIPI_480x800_ST7701S_ROTATE90"
 * #define TCFG_LCD_BL_VALUE                   1
 * #define TCFG_LCD_RESET_IO                   IO_PORTB_00
 * #define TCFG_LCD_BL_IO                      IO_PORTB_01
 * #define TCFG_LCD_RS_IO                      -1
 * #define TCFG_LCD_CS_IO                      -1
 * #define TCFG_LCD_TE_ENABLE                  0
 * #define TCFG_LCD_TE_IO                      -1
 * #define TCFG_LCD_SPI_INTERFACE              NULL
 * #endif
 */

#if TCFG_LCD_MIPI_ST7701S_480x640_ROTATE_90
#define TCFG_LCD_DEVICE_NAME                "MIPI_480x640_ST7701S_ROTATE90"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_01
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MIPI_ST7703_720x720
#define TCFG_LCD_DEVICE_NAME                "MIPI_720x720_ST7703"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_01
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MIPI_ILI9881C_800x1280
#define TCFG_LCD_DEVICE_NAME                "MIPI_800x1280_ILI9881C"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_03
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MIPI_FL7707_480x1280
#define TCFG_LCD_DEVICE_NAME                "MIPI_480x1280_FL7707"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTA_10
#define TCFG_LCD_BL_IO                      IO_PORTA_12
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MIPI_JD9165BA_1024x600
#define TCFG_LCD_DEVICE_NAME                "MIPI_1024x600_JD9165BA"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_01
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MIPI_ST77922_480x480
#define TCFG_LCD_DEVICE_NAME                "MIPI_480x480_ST77922"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_01
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MCU_ILI9488_320X480
#define TCFG_LCD_DEVICE_NAME                "MCU_320x480_ILI9488"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      IO_PORTA_13
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  1
#define TCFG_LCD_TE_IO                      IO_PORTA_11
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MCU_ILI9481_320X480
#define TCFG_LCD_DEVICE_NAME                "MCU_320x480_ILI9481"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      IO_PORTA_13
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  1
#define TCFG_LCD_TE_IO                      IO_PORTA_11
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_SPI_GC9307_240X296
#define TCFG_LCD_DEVICE_NAME                "SPI_240x296_GC9307"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTA_15
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      IO_PORTB_01
#define TCFG_LCD_CS_IO                      IO_PORTB_00
#define TCFG_LCD_SPI_INTERFACE              "spi2"
#define TCFG_LCD_TE_ENABLE                  1
#define TCFG_LCD_TE_IO                      IO_PORTA_14
#endif

#if TCFG_LCD_SPI_GC9307_172X320
#define TCFG_LCD_DEVICE_NAME                "SPI_172x320_GC9307"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTA_15
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      IO_PORTB_01
#define TCFG_LCD_CS_IO                      IO_PORTB_00
#define TCFG_LCD_SPI_INTERFACE              "spi2"
#define TCFG_LCD_TE_ENABLE                  1
#define TCFG_LCD_TE_IO                      IO_PORTA_14
#endif

#if TCFG_LCD_SPI_NV3006A_142X428
#define TCFG_LCD_DEVICE_NAME                "SPI_142x428_NV3006A"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTA_15
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      IO_PORTB_01
#define TCFG_LCD_CS_IO                      IO_PORTB_00
#define TCFG_LCD_SPI_INTERFACE              "spi2"
#define TCFG_LCD_TE_ENABLE                  1
#define TCFG_LCD_TE_IO                      IO_PORTA_14
#endif

#if TCFG_LCD_RGB_XXXX_480x272
#define TCFG_LCD_DEVICE_NAME                "RGB_480x272_XXXX"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      IO_PORTC_02
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#define TCFG_LCD_RGB_IO_HD_LEVEL            1
#endif

#if TCFG_LCD_RGB_XXXX_1280x800
#define TCFG_LCD_DEVICE_NAME                "RGB_1280x800_XXXX"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1
#define TCFG_LCD_BL_IO                      IO_PORTC_02
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#define TCFG_LCD_RGB_IO_HD_LEVEL            1
#endif

#if TCFG_LCD_RGB_SSD2828_720x1280
#define TCFG_LCD_DEVICE_NAME                "RGB_720x1280_SSD2828"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1
#define TCFG_LCD_BL_IO                      IO_PORTC_02
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#define TCFG_LCD_RGB_IO_HD_LEVEL            1
#endif

#if TCFG_LCD_RGB_ST7701S_480x800
#define TCFG_LCD_DEVICE_NAME                "RGB_480x800_ST7701S"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1          ///< 该屏幕复位脚与TP复用
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      IO_PORTC_02
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#define TCFG_LCD_RGB_ST7701S_SDA            IO_PORTC_00 ///< 该屏需单独通信口发送Init code
#define TCFG_LCD_RGB_ST7701S_SCL            IO_PORTC_01 ///< 该屏需单独通信口发送Init code
#define TCFG_LCD_RGB_IO_HD_LEVEL            1
#endif

#if TCFG_LCD_RGB_HX8264_800x480
#define TCFG_LCD_DEVICE_NAME                "RGB_800x480_HX8264"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   -1
#define TCFG_LCD_BL_IO                      -1
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      IO_PORTC_02
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#define TCFG_LCD_RGB_IO_HD_LEVEL            1
#endif

#endif

