#ifndef __GET_YUV_DATA_H__
#define __GET_YUV_DATA_H__

#include "generic/typedef.h"

void get_yuv_init(void (*cb)(u8 *data, u32 size, int width, int height));

void get_yuv_uninit(void);

void set_get_yuv_data_cb(void (*cb)(u8 *data, u32 size, int width, int height));

#endif //_GET_YUV_DATA_H__
