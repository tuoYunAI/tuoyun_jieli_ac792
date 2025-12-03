#ifndef __SC1245_DVP_H__
#define __SC1245_DVP_H__

#include "typedef.h"

#define SC1245_DVP_OUTPUT_W    1280
#define SC1245_DVP_OUTPUT_H    720

#define SC1245_FPS_VARIABLE    0


s32 sc1245_dvp_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *freq);
s32 sc1245_dvp_power_ctl(u8 isp_dev, u8 is_work);

s32 sc1245_dvp_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq);


void sc1245_dvp_sleep();
void sc1245_dvp_wakeup();
void sc1245_dvp_wr_reg(u8 isp_dev, u16 addr, u16 val);
u16 sc1245_dvp_rd_reg(u8 isp_dev, u16 addr);


#endif



