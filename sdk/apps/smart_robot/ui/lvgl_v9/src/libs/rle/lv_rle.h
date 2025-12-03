/**
 * @file lv_rle.h
 *
 */

#ifndef LV_RLE_H
#define LV_RLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
enum {
    ARGB8888 = 0,
    ARGB8565,
    ARGB1555,
    ARGB4444,
    RGB888,
    RGB565,
    AL88,
    AL44,
    L8,
    A8,
};

typedef struct rle_info {
    uint32_t width;
    uint32_t height;
    uint16_t format;
    // 0 : ARGB8888,
    // 1 : ARGB8565
    // 2 : ARGB1555
    // 3 : ARGB4444
    // 4 : RGB888
    // 5 : RGB565
    // 8 : AL88
    // 9 : AL44
    // 10 : L8
    // 11 : A8

    uint16_t block;
    // 0 : block_64x1
    // 1 : block_8x8
    // 2 : block_256x1
    // 3 : block_16x16

    uint16_t lut_format;
    // 0 : ARGB8888
    // 1 : ARGB8565
    // 2 : RGB888
    // 3 : RGB565
    uint16_t nop;

    uint32_t stride;
    uint32_t lut_addr;
    uint32_t lut_len;
    uint32_t addr;
    uint32_t len;
} rle_info_t;

static inline uint16_t lv_rle_get_stride(rle_info_t *rle_info, uint8_t *adr_mode)
{
    uint16_t stride = 0;
    switch (rle_info->block) {
    case 0:
        stride = ((rle_info->width + 63) / 64) * 4;
        if (adr_mode) {
            *adr_mode = 1;
        }
        break;
    case 1:
        stride = ((rle_info->width + 7) / 8) * 4;
        if (adr_mode) {
            *adr_mode = 0;
        }
        break;
    case 2:
        stride = ((rle_info->width + 255) / 256) * 4;
        if (adr_mode) {
            *adr_mode = 1;
        }
        break;
    case 3:
        stride = ((rle_info->width + 15) / 16) * 4;
        if (adr_mode) {
            *adr_mode = 0;
        }
        break;
    }

    return stride;
}

#if LV_USE_RLE

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

uint32_t lv_rle_decompress(const uint8_t *input,
                           uint32_t input_buff_len, uint8_t *output,
                           uint32_t output_buff_len, uint8_t blk_size);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_RLE*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_RLE_H*/
