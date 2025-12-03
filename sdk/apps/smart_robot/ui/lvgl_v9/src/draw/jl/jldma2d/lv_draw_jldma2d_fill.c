/**
 * @file lv_draw_dma2d_fill.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jldma2d_private.h"
#if LV_USE_DRAW_JLDMA2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_draw_dma2d_opaque_fill(lv_draw_jldma2d_unit_t *u, void *first_pixel, int32_t w, int32_t h, int32_t stride)
{
    lv_draw_fill_dsc_t *dsc = u->task_act->draw_dsc;
    lv_color_format_t cf = dsc->base.layer->color_format;
    uint16_t output_cf = lv_draw_jldma2d_cf_to_dma2d_output_cf(cf);

    lv_opa_t opa = dsc->opa;

    uint32_t packed_color = lv_color_to_u32(dsc->color);

    dma2d_out_layer_params_t out_layer_param = { 0 };
    dma2d_input_layer_params_t fg_layer_param = { 0 };
    dma2d_input_layer_params_t bg_layer_param = { 0 };

    out_layer_param.data = first_pixel;
    out_layer_param.w = w;
    out_layer_param.h = h;
    out_layer_param.stride = stride;
    out_layer_param.format = output_cf;
    out_layer_param.rbs = 0;
    out_layer_param.endian = 0;

    fg_layer_param.paintmode = 1;
    fg_layer_param.data = NULL;
    fg_layer_param.stride = 0;
    fg_layer_param.format = JLDMA2D_FORMAT_ARGB8888;
    if (opa < LV_OPA_MAX) {
        fg_layer_param.alpha_mode = JLDMA2D_REPLACE_ALPHA;
        fg_layer_param.alpha = opa;
    }
    fg_layer_param.rbs = 0;
    fg_layer_param.color = packed_color;

    bg_layer_param.data = (uint32_t)first_pixel;
    bg_layer_param.stride = stride;
    bg_layer_param.format = output_cf;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.rbs = 0;

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

}


void lv_draw_jldma2d_fill(lv_draw_jldma2d_unit_t *u, void *first_pixel, int32_t w, int32_t h, int32_t stride)
{
    lv_draw_fill_dsc_t *dsc = u->task_act->draw_dsc;
    lv_color_format_t cf = dsc->base.layer->color_format;
    uint16_t output_cf = lv_draw_jldma2d_cf_to_dma2d_output_cf(cf);
    //uint32_t packed_color = lv_draw_jldma2d_color_to_dma2d_color(output_cf, dsc->color);
    uint32_t packed_color = lv_color_to_u32(dsc->color);
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_fill(first_pixel, stride, output_cf, 0, packed_color, w, h);
    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_JLDMA2D*/

