
#ifndef __GPU_COMM_API_H__
#define __GPU_COMM_API_H__

#include "generic/typedef.h"

int gpu_rotate_frame_buf(uint8_t *src_buf, uint8_t *dst_buf, int src_width, int src_height, int dst_width, int dst_height, int degree, int format);

#endif
