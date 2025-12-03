/**
 * @file lv_draw_dma2d_private.h
 *
 */

#ifndef LV_DRAW_JLDMA2D_PRIVATE_H
#define LV_DRAW_JLDMA2D_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jldma2d.h"
#if LV_USE_DRAW_JLDMA2D

#include "../../lv_draw_private.h"
#include "../../sw/lv_draw_sw.h"
#include "../../../libs/jl_bin/lv_jl_bin.h"
#include "asm/dma2d_driver.h"

/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t base_unit;
    lv_draw_task_t *volatile task_act;
#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
    lv_thread_t thread;
    lv_thread_sync_t sync;
    volatile bool inited;
#endif
} lv_draw_jldma2d_unit_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_draw_jldma2d_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

void lv_draw_dma2d_opaque_fill(lv_draw_jldma2d_unit_t *u, void *first_pixel, int32_t w, int32_t h, int32_t stride);
void lv_draw_jldma2d_fill(lv_draw_jldma2d_unit_t *u, void *first_pixel, int32_t w, int32_t h, int32_t stride);

void lv_draw_jldma2d_image(lv_draw_jldma2d_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                           const lv_area_t *coords);
uint16_t lv_draw_jldma2d_cf_to_dma2d_output_cf(lv_color_format_t cf);
uint32_t lv_draw_jldma2d_color_to_dma2d_color(uint16_t cf, lv_color_t color);


/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_JLDMA2D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_JLDMA2D_PRIVATE_H*/

