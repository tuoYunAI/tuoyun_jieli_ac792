/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlvg.h"

#if LV_USE_DRAW_JLVG
#if (LV_USE_DRAW_JLVG_IMG_ENABLE == 1)

//#define LOG_TAG_CONST       UI
#define LOG_TAG             "[lv_gpu]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

/*********************
*      DEFINES
*********************/
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**********************
*      TYPEDEFS
**********************/

/**********************
*  STATIC PROTOTYPES
**********************/

static void _jlvg_draw_image_normal_helper(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords);
static void _jlvg_draw_image_tiled_helper(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords);
/**********************
*  STATIC VARIABLES
**********************/

/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/

void lv_draw_jlvg_img(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords)
{
    //log_debug("%s()", __func__);
    if (dsc->opa <= LV_OPA_MIN) {
        return;
    }

    // coords 是图像相对于屏幕的位置，基于这个坐标进行变换和分割
    //printf("coords (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", coords->x1, coords->y1, coords->x2, coords->y2);

    LV_PROFILER_BEGIN;

    lv_image_decoder_dsc_t decoder_dsc;
    lv_result_t res = lv_image_decoder_open(&decoder_dsc, dsc->src, NULL);
    if (res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        LV_PROFILER_END;
        return;
    }

    bool has_trasform = (dsc->rotation != 0 || dsc->scale_x != LV_SCALE_NONE || dsc->scale_y != LV_SCALE_NONE);


    if (!dsc->tile) {
        /*The whole image is available, just draw it*/
        _jlvg_draw_image_normal_helper(draw_unit, dsc, &decoder_dsc, coords);
    } else {
        /*Draw in smaller pieces*/
        log_error("unsupported [%s]", __func__);
        //_jlvg_draw_image_tiled_helper(draw_unit, dsc, &decoder_dsc, coords);
    }

    lv_image_decoder_close(&decoder_dsc);
    LV_PROFILER_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void _jlvg_draw_image_normal_helper(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords)
{
    lv_draw_jlvg_unit_t *u = (lv_draw_jlvg_unit_t *)draw_unit;
    lv_draw_task_t *t = u->task_act;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    //printf("buf_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", layer->buf_area.x1, layer->buf_area.y1, layer->buf_area.x2, layer->buf_area.y2);

    /* The coordinates passed in by coords are not transformed,
     * so the transformed area needs to be calculated once.
     */

    lv_area_t image_tf_area;
    lv_area_copy(&image_tf_area, coords);
    bool has_angle = dsc->rotation == 0 ? false : true ;
    bool has_zoom = (dsc->scale_x == LV_SCALE_NONE) && (dsc->scale_y == LV_SCALE_NONE) ? false : true;
    if (has_angle || has_zoom) {
        lv_image_buf_get_transformed_area(
            &image_tf_area,
            lv_area_get_width(coords),
            lv_area_get_height(coords),
            dsc->rotation,
            dsc->scale_x,
            dsc->scale_y,
            &dsc->pivot);

        lv_area_move(&image_tf_area, coords->x1, coords->y1);
    }

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_image_tf_area;
    lv_area_copy(&rel_image_tf_area, &image_tf_area);
    lv_area_move(&rel_image_tf_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;   //这个区域无论是帧buff还是行buff都是相对的新的绘制区域
    if (!lv_area_intersect(&blend_area, &rel_image_tf_area, &rel_clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }
    //printf("blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);

    //开始 JLVG 渲染动作
    // 1、初始化渲染空间
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf;
    rle_info_t *rle_info;
    lv_color_format_t dest_cf = draw_buf->header.cf;
    uint32_t bytes_per_pixel = lv_color_format_get_size(dest_cf);

#if (JLVG_DRAW_FULL_FB_ENABLE == 1)
    fb_width = layer->buf_area.x2 - layer->buf_area.x1 + 1;     // 显存区域的宽度
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;    // 显存区域的高度
    fb_size = fb_width * fb_height * bytes_per_pixel;           // 显存区域的buff大小
    dest_stride = draw_buf->header.stride;
    dest_buf = draw_buf->data;                                  //最终会更新到屏幕上面的 buff 区域
#else
    fb_width = blend_area.x2 - blend_area.x1 + 1;               // 绘制区域的宽度
    fb_height = blend_area.y2 - blend_area.y1 + 1;              // 绘制区域的高度
    fb_size = fb_width * fb_height * bytes_per_pixel;           // 绘制区域的buff大小
    dest_stride = draw_buf->header.stride;
    dest_buf = draw_buf->data + (blend_area.x1 * bytes_per_pixel) + (blend_area.y1 * dest_stride);
#endif
    //printf("fb_width = %d; fb_height = %d; dest_stride = %d.", fb_width, fb_height, dest_stride);

    jlvg_matrix_t surface2image;    // 变换矩阵 surface to image
    jlvg_matrix_t image2surface;    // 变换矩阵 image to surface (surface2image 的逆矩阵)
    jlvg_matrix_t surface2paint;    // 变换矩阵 surface to paint

    jlvg_surface_t surface;     // 绘制的窗口
    //jlvg_rect_t surface_area;   // 绘制的窗口区域:实际就是 layer->buf_area 所在的区域
    jlvg_rect_t draw_area;      // 在窗口内实际要绘制的区域:实际就是 blend_area

    //初始化单位变换矩阵
    jlvg_matrix_set_identity(&surface2image);
    jlvg_matrix_set_identity(&image2surface);
    jlvg_matrix_set_identity(&surface2paint);

    //窗口初始化
    jlvg_hw_imageformat_t jlvg_dest_cf = lv_jlvg_get_color_format(dest_cf, NULL);
    jlvg_surface_init(&surface, dest_buf, fb_size, fb_width, fb_height, dest_stride, jlvg_dest_cf, 0, 1, 1, 0);

    //初始化要绘制的区域
#if (JLVG_DRAW_FULL_FB_ENABLE == 1)
#if 0   /* 允许 VG 绘制整个 FB 空间: 重要，绘制整个 FB 会导致 VG 加速效果变差，耗时会较长 */
    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = fb_width;
    draw_area.h = fb_height;
#else   /* 只允许 VG 绘制 LVGL 限制的交叉裁剪区域 */
    draw_area.x = blend_area.x1;
    draw_area.y = blend_area.y1;
    draw_area.w = blend_area.x2 - blend_area.x1 + 1;
    draw_area.h = blend_area.y2 - blend_area.y1 + 1;
#endif
#else
    // 即 blend_area 区域
    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = fb_width;
    draw_area.h = fb_height;
#endif
    //printf("draw_area (x,y) = (%d,%d); w = %d, y = %d.", draw_area.x, draw_area.y, draw_area.w, draw_area.h);

    //2、配置变换矩阵
    float scale_x;
    float scale_y;
    float rotation;
    lv_point_t pivot_abs;   //变换原点的绝对位置，不会变化
    lv_point_t coords_abs;  //图像(0, 0)原点的绝对位置，不会变化

    if (has_angle || has_zoom) {
        // 计算变换原点的绝对位置
#if (JLVG_DRAW_FULL_FB_ENABLE == 1)
        pivot_abs.x = dsc->pivot.x + rel_coords.x1;
        pivot_abs.y = dsc->pivot.y + rel_coords.y1;
        //printf("0 pivot_abs (x,y) = (%d,%d).", pivot_abs.x, pivot_abs.y);
#else
        //计算变换原点绝对位置在 blend_area 区域的相对位置
        pivot_abs.x = dsc->pivot.x + rel_coords.x1 - blend_area.x1;
        pivot_abs.y = dsc->pivot.y + rel_coords.y1 - blend_area.y1;
        //printf("1 pivot_abs (x,y) = (%d,%d).", pivot_abs.x, pivot_abs.y);
#endif
        if (has_angle && (!has_zoom)) { // 仅旋转
            rotation = (-1.0f) * dsc->rotation * M_PI / 1800.0f;
            //printf("rotation = %f.", rotation);

            // 旋转变换流程说明：
            // openVG 的流程是先平移到相对变换原点位置，再旋转变换之后再平移到绝对原点位置; JLVG 仅旋转时保持一样的动作
            /* ===== rotate begin ===== */
            jlvg_matrix_translate(&image2surface, -pivot_abs.x, -pivot_abs.y);      // 基于变换原点的绝对位置进行偏移

            jlvg_matrix_rotate(&image2surface, rotation);

            jlvg_matrix_translate(&image2surface, dsc->pivot.x, dsc->pivot.y);      // 基于变换原点的相对位置进行偏移
            /* ===== rotate end ===== */
        } else if (has_zoom && (!has_angle)) {  // 仅缩放
            scale_x = dsc->scale_x * 1.0f / LV_SCALE_NONE;
            scale_y = dsc->scale_y * 1.0f / LV_SCALE_NONE;
            //printf("scale_x = %f; scale_y = %f.", scale_x, scale_y);

            // 缩放变换流程说明：
            // openVG 的流程是先平移到相对变换原点位置，再缩放变换之后再平移到绝对原点位置;
            // JLVG 需要通过逆变换：实际上是先缩放变换然后平移到绝对原点位置之后再逆变换，之后再平移回变换原点位置，并且由于逆变换之后平移方向反向
            /* ===== scale begin ===== */
            jlvg_matrix_set_scale(&surface2image, scale_x, scale_y);

            jlvg_matrix_translate(&surface2image, pivot_abs.x, pivot_abs.y);    // 基于变换原点的绝对位置进行偏移

            //矩阵逆运算
            jlvg_matrix_invert(&image2surface, &surface2image);

            jlvg_matrix_translate(&image2surface, dsc->pivot.x, dsc->pivot.y);  // 基于变换原点的相对位置进行偏移,由于逆变换，方向相反，流程上和旋转变换的第一步相对抵消
            /* ===== scale end ===== */
        } else {    // 同时缩放和旋转
            rotation = (-1.0f) * dsc->rotation * M_PI / 1800.0f;
            //printf("rotation = %f.", rotation);
            scale_x = dsc->scale_x * 1.0f / LV_SCALE_NONE;
            scale_y = dsc->scale_y * 1.0f / LV_SCALE_NONE;
            //printf("scale_x = %f; scale_y = %f.", scale_x, scale_y);

            // 缩放变换流程说明：
            // openVG 的流程是先平移到相对变换原点位置，再缩放变换之后再平移到绝对原点位置;
            // JLVG 需要通过逆变换：实际上是先缩放变换然后平移到绝对原点位置之后再逆变换，之后再平移回变换原点位置，并且由于逆变换之后平移方向反向
            /* ===== scale begin ===== */
            jlvg_matrix_set_scale(&surface2image, scale_x, scale_y);

            jlvg_matrix_translate(&surface2image, pivot_abs.x, pivot_abs.y);    // 基于变换原点的绝对位置进行偏移

            //矩阵逆运算
            jlvg_matrix_invert(&image2surface, &surface2image);

            //jlvg_matrix_translate(&image2surface, dsc->pivot.x , dsc->pivot.y); // 基于变换原点的相对位置进行偏移,由于逆变换，方向相反，流程上和旋转变换的第一步相对抵消
            /* ===== scale end ===== */

            // 旋转变换流程说明：
            // openVG 的流程是先平移到相对变换原点位置，再旋转变换之后再平移到绝对原点位置;
            // JLVG 由于和缩放一起走的逆变换流程，所以只需要平移相对原点位置即可
            /* ===== rotate begin ===== */
            //jlvg_matrix_translate(&image2surface, -dsc->pivot.x , -dsc->pivot.y);   // 基于变换原点的相对位置进行偏移，由于逆变换，方向相反，流程上和缩放变换的最后一步互相抵消

            jlvg_matrix_rotate(&image2surface, rotation);

            jlvg_matrix_translate(&image2surface, dsc->pivot.x, dsc->pivot.y);      // 基于变换原点的相对位置进行偏移
            /* ===== rotate end ===== */
        }
    } else {    // 无变换动作
#if (JLVG_DRAW_FULL_FB_ENABLE == 1)
        coords_abs.x = rel_coords.x1;
        coords_abs.y = rel_coords.y1;
        //printf("0 coords_abs (x,y) = (%d,%d).", coords_abs.x, coords_abs.y);
#else
        //计算变换原点绝对位置在 blend_area 区域的相对位置
        coords_abs.x = rel_coords.x1 - blend_area.x1;
        coords_abs.y = rel_coords.y1 - blend_area.y1;
        //printf("1 coords_abs (x,y) = (%d,%d).", coords_abs.x, coords_abs.y);
#endif
        if (coords_abs.x || coords_abs.y) {
            jlvg_matrix_translate(&image2surface, -coords_abs.x, -coords_abs.y);      // 基于图像(0, 0)原点的绝对位置进行偏移
            //printf("coords_abs (x,y) = (%d,%d).", coords_abs.x, coords_abs.y);
        }
    }

    //3、获取原始图像数据信息
    const lv_draw_buf_t *decoded = decoder_dsc->decoded;
    uint8_t adr_mode = 1;
    uint8_t rle_en = 0;
    uint8_t compress_type = LV_COMPRESS_NONE;
    if (decoder_dsc && decoder_dsc->user_data && ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed) {
        compress_type = ((jl_bin_user_data_t *)decoder_dsc->user_data)->compressed;
    }
    const uint8_t *img_buf = decoded->data;
    uint32_t img_stride = decoded->header.stride;
    lv_color_format_t img_cf = decoded->header.cf;
    uint32_t img_bytes_per_pixel = lv_color_format_get_size(img_cf);
    uint16_t clut_format = VGHW_CLUT_FORMAT_ARGB8888;
    uint8_t *src_buf = img_buf;
    uint8_t *src_clut = NULL;
    uint32_t src_width = decoded->header.w;
    uint32_t src_height = decoded->header.h;
    uint32_t src_size = src_width * src_height * img_bytes_per_pixel;
    jlvg_hw_imageformat_t jlvg_img_cf = lv_jlvg_get_color_format(img_cf, NULL);
    bool has_indexed = (jlvg_img_cf >= VGHW_FORMAT_L8 && jlvg_img_cf <= VGHW_FORMAT_L1) ? true : false;
    if (compress_type == LV_COMPRESS_RLE) {
        rle_info = (rle_info_t *)src_buf;
        rle_en = 1;
        img_stride = lv_rle_get_stride(rle_info, &adr_mode);
        src_size = rle_info->len;
        jlvg_img_cf = rle_info->format;
        clut_format = rle_info->lut_format;
        if (has_indexed) {
            src_clut = (uint8_t *)src_buf + rle_info->lut_addr;
        }
        src_buf = (uint8_t *)src_buf + rle_info->addr;

    } else {
        if (has_indexed) {
            src_clut = src_buf;
            src_buf += 1024;//256 * 4
        }
    }
    jlvg_image_t *image = jlvg_image_create(src_buf, src_size, src_width, src_height, img_stride, jlvg_img_cf, rle_en, adr_mode, 1, 0);
    jlvg_image_set_clut(image, src_clut, clut_format);
    uint8_t blend_mode = lv_jlvg_get_blend_mode(dsc->blend_mode, NULL);
    //printf("img info : src_width = %d; src_height = %d; src_size = %d; img_stride = %d.", src_width, src_height, src_size, img_stride);

    //4、硬件渲染图像数据
    jlvg_start_frame(g_jlvg, &surface);

    //根据图像透明度进行渲染动作
    jlvg_paint_color_t paint;
    jlvg_color_t paint_color = { 0xff, 0xff, 0xff, 0xff};
    if (dsc->opa < (lv_opa_t)LV_OPA_MAX) {
        paint_color.a = dsc->opa;
        jlvg_color_premultiply(&paint_color);    // 填充像素预乘
        jlvg_paint_color_init(&paint, paint_color);
        jlvg_draw_image_with_paint(g_jlvg, &draw_area, image, &paint, &image2surface, &surface2paint, blend_mode, 0);
    } else {
        jlvg_draw_image(g_jlvg, &draw_area, image, &image2surface, blend_mode);
    }

    jlvg_end_frame(g_jlvg);

    jlvg_image_free(image);

    jlvg_surface_deinit(&surface);
}

static void _jlvg_draw_image_tiled_helper(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc, const lv_image_decoder_dsc_t *decoder_dsc, const lv_area_t *coords)
{
    log_error("unsupported [%s]", __func__);
    // todo
}

#endif /*LV_USE_DRAW_JLVG_IMG_ENABLE*/
#endif /*LV_USE_DRAW_JLVG*/
