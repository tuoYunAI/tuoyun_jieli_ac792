/**
 * @file lv_draw_dma2d_image.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jldma2d_private.h"
#if LV_USE_DRAW_JLDMA2D
#include "../../lv_image_decoder_private.h"
#include "../../lv_draw_image_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void img_draw_core(lv_draw_task_t *t, const lv_draw_image_dsc_t *draw_dsc,
                          const lv_image_decoder_dsc_t *decoder_dsc, lv_draw_image_sup_t *sup,
                          const lv_area_t *img_coords, const lv_area_t *clipped_img_area);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_jldma2d_image(lv_draw_jldma2d_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                           const lv_area_t *coords)
{
    lv_draw_image_normal_helper(draw_unit->task_act, draw_dsc, coords, img_draw_core);
}
/**********************
 *   STATIC FUNCTIONS
 **********************/
static void img_draw_core(lv_draw_task_t *t, const lv_draw_image_dsc_t *draw_dsc,
                          const lv_image_decoder_dsc_t *decoder_dsc, lv_draw_image_sup_t *sup,
                          const lv_area_t *img_coords, const lv_area_t *clipped_img_area)
{

    (void)sup; //remove warning about unused parameter

    //src img
    rle_info_t *rle_info;
    const lv_draw_buf_t *decoded = decoder_dsc->decoded;
    const uint8_t *src_buf = decoded->data;
    const lv_image_header_t *header = &decoded->header;
    lv_color_format_t image_cf = decoded->header.cf;
    uint32_t image_cf_size = LV_COLOR_FORMAT_GET_SIZE(image_cf);
    uint32_t image_stride = decoded->header.stride;
    if (image_stride == 0) {
        image_stride = image_cf_size * header->w;
    }
    lv_opa_t opa = draw_dsc->opa;
    bool has_indexed = (image_cf >= LV_COLOR_FORMAT_I1 && image_cf <= LV_COLOR_FORMAT_I8);

    uint8_t compress_type = LV_COMPRESS_NONE;
    if (decoder_dsc && decoder_dsc->user_data && ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed) {
        compress_type = ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed;
    }

    bool recolor = (draw_dsc->recolor_opa > LV_OPA_MIN);
    uint32_t packed_color = 0;
    if (recolor) {
        lv_color32_t col32 = lv_color_to_32(draw_dsc->recolor, LV_OPA_MIX2(draw_dsc->recolor_opa, draw_dsc->opa));
        packed_color = dma2d_pack_pixel(col32.alpha, col32.red, col32.green, col32.blue, JLDMA2D_FORMAT_ARGB8888);
    }

    if (src_buf) {
        DcuFlushRegion(src_buf, header->h * image_stride);
    }


    //out
    lv_layer_t *layer = t->target_layer;
    //lv_draw_buf_t* draw_buf = layer->draw_buf;
    uint8_t *dest = lv_draw_layer_go_to_xy(layer, clipped_img_area->x1 - layer->buf_area.x1, clipped_img_area->y1 - layer->buf_area.y1);
    int32_t dest_w = lv_area_get_width(clipped_img_area);
    int32_t dest_h = lv_area_get_height(clipped_img_area);
    lv_color_format_t output_cf = layer->color_format;
    int32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&layer->buf_area), output_cf);


    //dma2d render
    dma2d_out_layer_params_t out_layer_param = { 0 };
    dma2d_input_layer_params_t fg_layer_param = { 0 };
    dma2d_input_layer_params_t bg_layer_param = { 0 };
    if (opa >= LV_OPA_MAX && recolor == 0
        && ((image_cf == output_cf))) {
        // 不透明，不recolor，颜色格式相同, 直接copy
        fg_layer_param.data = src_buf;
        fg_layer_param.stride = image_stride;
        fg_layer_param.format = lv_draw_jldma2d_cf_to_dma2d_output_cf(image_cf);
        if (compress_type == LV_COMPRESS_RLE) {
            rle_info = (rle_info_t *)src_buf;
            fg_layer_param.rle_en = 1;
            fg_layer_param.data = src_buf + rle_info->addr;
            fg_layer_param.clut = src_buf + rle_info->lut_addr;
            fg_layer_param.clut_size = rle_info->lut_len;
            fg_layer_param.clut_format = rle_info->lut_format;
            fg_layer_param.image_size = rle_info->len;

            fg_layer_param.stride = lv_rle_get_stride(rle_info, NULL);
            fg_layer_param.format = rle_info->format;
        }
        fg_layer_param.x = clipped_img_area->x1 - img_coords->x1;
        fg_layer_param.y = clipped_img_area->y1 - img_coords->y1;
        fg_layer_param.alpha = 0;
        fg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
        fg_layer_param.rbs = 0;

        out_layer_param.format = lv_draw_jldma2d_cf_to_dma2d_output_cf(output_cf);
        out_layer_param.data = dest;
        out_layer_param.w = dest_w;
        out_layer_param.h = dest_h;
        out_layer_param.stride = dest_stride;
        out_layer_param.alpha_inv = 0;
        out_layer_param.rbs = 0;
        out_layer_param.dither_en = 1;

        dma2d_reset_all_regs();
        dma2d_create_task();
        dma2d_set_fg_layer(&fg_layer_param);
        dma2d_set_out_layer(&out_layer_param);
        dma2d_set_mode(JLDMA2D_M2M_PFC);
        dma2d_run();
        dma2d_wait_done();

    } else {
        out_layer_param.data = dest;
        out_layer_param.w = dest_w;
        out_layer_param.h = dest_h;
        out_layer_param.stride = dest_stride;
        out_layer_param.format = lv_draw_jldma2d_cf_to_dma2d_output_cf(output_cf);
        out_layer_param.rbs = 0;
        out_layer_param.endian = 0;
        out_layer_param.dither_en = 1;

        fg_layer_param.data = (uint32_t)src_buf;
        fg_layer_param.x = clipped_img_area->x1 - img_coords->x1;
        fg_layer_param.y = clipped_img_area->y1 - img_coords->y1;
        fg_layer_param.stride = image_stride;
        fg_layer_param.format = lv_draw_jldma2d_cf_to_dma2d_output_cf(image_cf);
        fg_layer_param.alpha = opa;
        fg_layer_param.alpha_mode = JLDMA2D_COMBINE_ALPHA;
        fg_layer_param.rbs = 0;
        fg_layer_param.color = packed_color;
        fg_layer_param.colorkey = 0;
        if (compress_type == LV_COMPRESS_RLE) {
            rle_info = (rle_info_t *)src_buf;
            fg_layer_param.data = (uint8_t *)src_buf + rle_info->addr;
            fg_layer_param.rle_en = 1;
            fg_layer_param.clut = src_buf + rle_info->lut_addr;
            fg_layer_param.clut_size = rle_info->lut_len;
            fg_layer_param.clut_format = rle_info->lut_format;
            fg_layer_param.image_size = rle_info->len;
            fg_layer_param.stride = lv_rle_get_stride(rle_info, NULL);
            fg_layer_param.format = rle_info->format;
        } else {
            if (has_indexed) {
                fg_layer_param.clut_format = JLDMA2D_CLUT_FORMAT_ARGB8888;// 默认色表格式ARGB8888
                fg_layer_param.clut = (uint32_t)src_buf;
                fg_layer_param.clut_size = (1 << lv_color_format_get_bpp(image_cf)) * 4;
                fg_layer_param.data = (uint32_t)src_buf + fg_layer_param.clut_size;
            }
        }

        bg_layer_param.data = (uint32_t)dest;
        bg_layer_param.stride = dest_stride;
        bg_layer_param.format = lv_draw_jldma2d_cf_to_dma2d_output_cf(output_cf);
        bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
        bg_layer_param.rbs = 0;

        dma2d_reset_all_regs();
        dma2d_create_task();
        dma2d_set_fg_layer(&fg_layer_param);
        dma2d_set_bg_layer(&bg_layer_param);
        dma2d_set_out_layer(&out_layer_param);
        dma2d_set_mode(JLDMA2D_M2M_BLEND);
        dma2d_run();
        dma2d_wait_done();
    }
}

#endif /*LV_USE_DRAW_JLDMA2D*/

