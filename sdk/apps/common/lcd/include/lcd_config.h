#ifndef __LCD_CONFIG_H__
#define __LCD_CONFIG_H__

#include "app_config.h"

//*****************************************//
//                定义屏配置               //
//*****************************************//
//屏幕宏定义规则TCFG_LCD_屏幕类型_屏幕驱动_屏幕尺寸 只能开一个屏幕宏
#if TCFG_LCD_ENABLE

//*****************************************//
//                定义屏尺寸               //
//*****************************************//
// 多屏驱支持
#if TCFG_LCD_SUPPORT_MULTI_DRIVER_EN == 0

//MCU LCD
#if TCFG_LCD_MCU_ILI9488_320X480
#define LCD_W 320
#define LCD_H 480
#endif

#if TCFG_LCD_MCU_ILI9481_320X480
#define LCD_W 320
#define LCD_H 480
#endif

//MIPI LCD
#if TCFG_LCD_MIPI_FL7707_480x1280
#define LCD_W 1280
#define LCD_H 480
#endif

#if TCFG_LCD_MIPI_ST7701S_480x800
#define LCD_W 480
#define LCD_H 800
#endif

#if TCFG_LCD_MIPI_ST7703_720x720
#define LCD_W 720
#define LCD_H 720
#endif

#if TCFG_LCD_MIPI_ST7701S_480x800_ROTATE_90
#define LCD_W 800
#define LCD_H 480
#endif

#if TCFG_LCD_MIPI_ST7701S_480x640_ROTATE_90
#define LCD_W 640
#define LCD_H 368
#endif

#if TCFG_LCD_MIPI_ST77922_480x480
#define LCD_W 480
#define LCD_H 480
#endif

#if TCFG_LCD_MIPI_ILI9881C_800x1280
#define LCD_W 800
#define LCD_H 1280
#endif

#if TCFG_LCD_MIPI_JD9165BA_1024x600
#define LCD_W 1024
#define LCD_H 600
#endif

//RGB_LCD
#if TCFG_LCD_RGB_XXXX_480x272
#define LCD_W 480
#define LCD_H 272
#endif

#if TCFG_LCD_RGB_SSD2828_720x1280
#define LCD_W 720
#define LCD_H 1280
#endif

#if TCFG_LCD_RGB_XXXX_1280x800
#define LCD_W 1280
#define LCD_H 800
#endif

#if TCFG_LCD_RGB_ST7701S_480x800
#define LCD_W 480
#define LCD_H 800
#endif

#if TCFG_LCD_RGB_HX8264_800x480
#define LCD_W 800
#define LCD_H 480
#endif

//SPI LCD
#if TCFG_LCD_SPI_GC9307_240X296
#define LCD_W 240
#define LCD_H 296
#endif

#if TCFG_LCD_SPI_GC9307_172X320
#define LCD_W 172
#define LCD_H 320
#endif

#if TCFG_LCD_SPI_NV3006A_142X428
#define LCD_W 142
#define LCD_H 428
#endif

#define LCD_ID  0

#else /* TCFG_LCD_SUPPORT_MULTI_DRIVER_EN */
//LCD0 宽高和ID号定义
#define LCD_W  800// 480
#define LCD_H  480// 800
#define LCD_ID  0
/***** 不同分辨率的屏定义 *****/
//LCD1 宽高和ID号定义
#define LCD1_W   296//240
#define LCD1_H   240//296
#define LCD1_ID  1
#endif // TCFG_LCD_SUPPORT_MULTI_DRIVER_EN

//*****************************************//
//              定义数据类型长度           //
//*****************************************//
#define LCD_YUV420_DATA_SIZE    LCD_W*LCD_H*3/2
#define LCD_YUV422_DATA_SIZE    LCD_W*LCD_H*2
#define LCD_YUV444_DATA_SIZE    LCD_W*LCD_H*3
#define LCD_RGB565_DATA_SIZE    LCD_W*LCD_H*2
#define LCD_RGB888_DATA_SIZE    LCD_W*LCD_H*3
#define LCD_ARGB888_DATA_SIZE   LCD_W*LCD_H*4


#else

#define LCD_W 0
#define LCD_H 0
#define LCD_YUV420_DATA_SIZE    LCD_W*LCD_H*3/2
#define LCD_YUV422_DATA_SIZE    LCD_W*LCD_H*2
#define LCD_YUV444_DATA_SIZE    LCD_W*LCD_H*3
#define LCD_RGB565_DATA_SIZE    LCD_W*LCD_H*2
#define LCD_RGB888_DATA_SIZE    LCD_W*LCD_H*3
#define LCD_ARGB888_DATA_SIZE   LCD_W*LCD_H*4

#endif// TCFG_LCD_ENABLE

#endif

