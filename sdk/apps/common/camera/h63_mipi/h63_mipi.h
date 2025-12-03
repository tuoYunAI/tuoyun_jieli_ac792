
#ifndef __H63_MIPI_H__
#define __H63_MIPI_H__

#include "typedef.h"

#define H63_MIPI_OUTPUT_W     1280
#define H63_MIPI_OUTPUT_H      720



s32 H63_MIPI_set_output_size(u16 *width, u16 *height, u8 *freq);
s32 H63_MIPI_power_ctl(u8 isp_dev, u8 is_work);

s32 H63_MIPI_check(u8 _isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name);
s32 H63_MIPI_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq);

void H63_MIPI_sleep();
void H63_MIPI_wakeup();
void H63_MIPI_W_Reg(u16 addr, u16 val);
u16 H63_MIPI_R_Reg(u16 addr);


#endif


