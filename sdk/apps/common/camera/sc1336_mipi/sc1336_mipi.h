#ifndef __SC1336_MIPI_H__
#define __SC1336_MIPI_H__

#include "typedef.h"

#define SC1336_MIPI_OUTPUT_W     1280
#define SC1336_MIPI_OUTPUT_H      720

#define DEBUG_CMOS 1

#if (DEBUG_CMOS == 1)
#define x_puts puts
#define x_printf printf
#else
#define x_puts(...)
#define x_printf(...)
#endif

s32 sc1336_mipi_set_output_size(u16 *width, u16 *height, u8 *freq);
s32 sc1336_mipi_power_ctl(u8 isp_dev, u8 is_work);

s32 sc1336_mipi_check(u8 _isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name);
s32 sc1336_mipi_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq);

void sc1336_mipi_sleep();
void sc1336_mipi_wakeup();
void sc1336_mipi_wreg(u8 isp_dev, u16 addr, u16 val);
u16 sc1336_mipi_rreg(u8 isp_dev, u16 addr);

#endif


