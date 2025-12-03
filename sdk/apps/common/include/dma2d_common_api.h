
#ifndef __DMA2D_COMM_API_H__
#define __DMA2D_COMM_API_H__

#include "generic/typedef.h"

typedef struct {
    uint8_t *addr; /* 图层数据地址 */
    uint8_t format; /* 图层数据格式 */
    uint16_t x; /* 图层起始x坐标 */
    uint16_t y; /* 图层起始y坐标 */
    uint16_t width; /* 图层宽度 */
    uint16_t height; /* 图层高度 */
    uint8_t alpha;
    uint8_t alpha_mode;
    uint8_t rbs;
    uint8_t color_key;
    uint32_t color;
} dma2d_frame_t;

int dma2d_copy_frame_buf(dma2d_frame_t *fg, dma2d_frame_t *bg, dma2d_frame_t *out);
int dma2d_merge_frame_buf(dma2d_frame_t *fg, dma2d_frame_t *bg, dma2d_frame_t *out);

#endif
