#ifndef __GC1084_DVP_H__
#define __GC1084_DVP_H__

#include "typedef.h"

//#define GC1084_DVP   1

#define GC1084_DVP_OUTPUT_W    1280
#define GC1084_DVP_OUTPUT_H    720



s32 GC1084_DVP_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq);
s32 GC1084_DVP_power_ctl(u8 isp_dev, u8 is_work);

s32 GC1084_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq);


void GC1084_DVP_sleep();
void GC1084_DVP_wakeup();
void GC1084_DVP_W_Reg(u8 isp_dev, u16 addr, u16 val);
u16 GC1084_DVP_R_Reg(u8 isp_dev, u16 addr);


#endif


