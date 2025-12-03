
#ifndef __SC1345_MIPI_H__
#define __SC1345_MIPI_H__

#include "typedef.h"

#define SC1345_MIPI_OUTPUT_W    1280
#define SC1345_MIPI_OUTPUT_H    720

s32 SC1345_mipi_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *freq);
s32 SC1345_mipi_power_ctl(u8 isp_dev, u8 is_work);

//s32 GC1004_check(u8 isp_dev);
s32 SC1345_mipi_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq);


void SC1345_mipi_sleep();
void SC1345_mipi_wakeup();
void SC1345_mipi_W_Reg(u8 isp_dev, u16 addr, u16 val);
u16 SC1345_mipi_R_Reg(u8 isp_dev, u16 addr);


#endif



