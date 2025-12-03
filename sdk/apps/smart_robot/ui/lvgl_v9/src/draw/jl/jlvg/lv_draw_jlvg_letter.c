/**
 * @file lv_draw_jlvg_letter.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlvg.h"

#if LV_USE_DRAW_JLVG
#if (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1)

//#define LOG_TAG_CONST       UI
#define LOG_TAG             "[lv_gpu]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

#include "../../lvgl.h"
#include "../../../lvgl_private.h"

/*********************
 *      DEFINES
 *********************/

//extern uint32_t get_system_us(void);

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void /* LV_ATTRIBUTE_FAST_MEM */ draw_letter_jlvg_cb(lv_draw_unit_t *draw_unit, \
        lv_draw_glyph_dsc_t *glyph_draw_dsc, \
        lv_draw_fill_dsc_t *fill_draw_dsc, \
        const lv_area_t *fill_area);
static void lv_jlvg_label_draw_task_run(lv_draw_unit_t *draw_unit, const lv_draw_label_dsc_t *dsc, lv_jlvg_label_info_t *curr_label_info);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_jlvg_label(lv_draw_unit_t *draw_unit, const lv_draw_label_dsc_t *dsc, const lv_area_t *coords)
{
    if (dsc->opa <= LV_OPA_MIN) {
        return;
    }

    LV_PROFILER_BEGIN;
    //printf("coords : (x1, y1) = (%d, %d); (x2, y2) = (%d, %d).", coords->x1, coords->y1, coords->x2, coords->y2);

    lv_jlvg_label_info_t *curr_label_info = (lv_jlvg_label_info_t *)lv_malloc_zeroed(sizeof(lv_jlvg_label_info_t));
    if (curr_label_info == NULL) {
        return;
    }

    // 初始化一个 label 绘制任务
    if (lv_jlvg_label_draw_task_create(draw_unit, dsc->font, coords, curr_label_info) == false) {
        lv_free(curr_label_info);
        return;
    }

    // 初始化字形链表
    lv_ll_init(&curr_label_info->glyph_info_list, sizeof(lv_jlvg_glyph_info_t));

    //uint32_t draw_start = get_system_us();

    // 收集文本全部字形的路径
    lv_draw_label_jlvg_path_collection(draw_unit, dsc, coords, draw_letter_jlvg_cb, curr_label_info);

    //uint32_t get_path_end = get_system_us();

    // 执行 label 绘制任务
    lv_jlvg_label_draw_task_run(draw_unit, dsc, curr_label_info);

    //uint32_t draw_end = get_system_us();

    //printf("get_path time = %d; draw time = %d.", (get_path_end - draw_start), (draw_end - get_path_end));

    // 清理绘制现场
    // 清理每个字形的路径缓存
    //printf("gpu over");
    lv_jlvg_glyph_info_t *curr_glyph_info = lv_ll_get_tail(&curr_label_info->glyph_info_list);
    while (curr_glyph_info != NULL) {
        // 释放路径信息
        lv_free(curr_glyph_info->segments);
        lv_free(curr_glyph_info->points);

        /*Get next glyph_info*/
        lv_jlvg_glyph_info_t *next_glyph_info = lv_ll_get_prev(&curr_label_info->glyph_info_list, curr_glyph_info);

        /*Move on to next glyph_info*/
        curr_glyph_info = next_glyph_info;
    }

    // 清理字形列表
    lv_ll_clear(&(curr_label_info->glyph_info_list));

    lv_free(curr_label_info);

    LV_PROFILER_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void draw_letter_jlvg_cb(lv_draw_unit_t *draw_unit, lv_draw_glyph_dsc_t *glyph_draw_dsc,
                                lv_draw_fill_dsc_t *fill_draw_dsc, const lv_area_t *fill_area)
{
    //printf("%s() glyph_draw_dsc->format : 0x%02x.", __func__, glyph_draw_dsc->format);

    if (glyph_draw_dsc->format != LV_FONT_GLYPH_FORMAT_VECTOR) {
        printf("curr jlvg glyph format only supply LV_FONT_GLYPH_FORMAT_VECTOR!!!");
        return;
    }

    const lv_font_glyph_dsc_t *g = glyph_draw_dsc->g;
    const lv_point_t *curr_pos = g->pos;
    lv_jlvg_label_info_t *curr_label_info = g->label_info;

    lv_jlvg_glyph_ori_info_t *curr_glyph_ori_info = (lv_jlvg_glyph_ori_info_t *)glyph_draw_dsc->glyph_data;

    lv_jlvg_glyph_info_t *curr_glyph_info = lv_ll_ins_head(&curr_label_info->glyph_info_list);
    if (curr_glyph_info == NULL) {
        LV_LOG_ERROR("Glyph info lv_ll_ins_head failed");
        return;
    }

    // 将字形句柄记录到标签句柄中
    uint16_t segments_num_tmp = sizeof(uint8_t) * ((curr_glyph_ori_info->segments_number & (~0xf)) + 16);  // 16 个字节对齐
    //uint16_t segments_num_tmp = 1024;
    curr_glyph_info->segments = (uint8_t *)lv_malloc(segments_num_tmp);
    if (curr_glyph_info->segments == NULL) {
        printf("curr_glyph_info->segments alloc is fail !!! >> %s %d", __func__, __LINE__);
    }

    uint16_t points_num_tmp = sizeof(float) * segments_num_tmp * 3;
    curr_glyph_info->points = (float *)lv_malloc(points_num_tmp);
    if (curr_glyph_info->points == NULL) {
        printf("curr_glyph_info->points alloc is fail !!! >> %s %d", __func__, __LINE__);
    }
    //printf("segments_num_tmp = %d; points_num_tmp = %d.", segments_num_tmp, points_num_tmp);

    curr_glyph_info->segments_ofs = 0;
    curr_glyph_info->points_ofs = 0;
    //curr_glyph_info->label_info = curr_label_info;
    curr_glyph_info->pos.x = (curr_pos->x - curr_label_info->effective_coords.x1) / curr_label_info->original_scale;
    curr_glyph_info->pos.y = (curr_pos->y - curr_label_info->effective_coords.y1) / curr_label_info->original_scale;

    //printf("curr_glyph_info->pos.x = %d; curr_glyph_info->pos.y = %d.", curr_glyph_info->pos.x, curr_glyph_info->pos.y);

    // 装载原始路径到 JLVG 的路径格式
    for (int i = 0; i < curr_glyph_ori_info->segments_number; i++) {
        jlvg_hw_pathtype_t type = curr_glyph_ori_info->path_segments[i].type;
        curr_glyph_info->segments[i] = type;
        //printf("type[%d] = 0x%02x.", i, type);
        switch (type) {
        case VGHW_CLOSE_PATH:
            // nothing
            break;
        case VGHW_MOVE_TO:
        case VGHW_LINE_TO:
            // 终点
            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_glyph_ori_info->path_segments[i].to.x + \
                    (float)curr_glyph_info->pos.x;
            curr_glyph_info->points_ofs++;

            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_label_info->height - \
                    (float)(curr_glyph_ori_info->path_segments[i].to.y - \
                            curr_label_info->descender) + \
                    (float)curr_glyph_info->pos.y;
            curr_glyph_info->points_ofs++;
            break;
        case VGHW_CUBIC_TO:
            // 控制点 1
            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_glyph_ori_info->path_segments[i].control1.x + \
                    (float)curr_glyph_info->pos.x;
            curr_glyph_info->points_ofs++;

            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_label_info->height - \
                    (float)(curr_glyph_ori_info->path_segments[i].control1.y - \
                            curr_label_info->descender) + \
                    (float)curr_glyph_info->pos.y;
            curr_glyph_info->points_ofs++;

            // 控制点 2
            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_glyph_ori_info->path_segments[i].control2.x + \
                    (float)curr_glyph_info->pos.x;
            curr_glyph_info->points_ofs++;

            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_label_info->height - \
                    (float)(curr_glyph_ori_info->path_segments[i].control2.y - \
                            curr_label_info->descender) + \
                    (float)curr_glyph_info->pos.y;
            curr_glyph_info->points_ofs++;

            // 终点
            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_glyph_ori_info->path_segments[i].to.x + \
                    (float)curr_glyph_info->pos.x;
            curr_glyph_info->points_ofs++;

            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_label_info->height - \
                    (float)(curr_glyph_ori_info->path_segments[i].to.y - \
                            curr_label_info->descender) + \
                    (float)curr_glyph_info->pos.y;
            curr_glyph_info->points_ofs++;
            break;
        case VGHW_QUAD_TO:
            // 控制点 1
            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_glyph_ori_info->path_segments[i].control1.x + \
                    (float)curr_glyph_info->pos.x;
            curr_glyph_info->points_ofs++;

            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_label_info->height - \
                    (float)(curr_glyph_ori_info->path_segments[i].control1.y - \
                            curr_label_info->descender) + \
                    (float)curr_glyph_info->pos.y;
            curr_glyph_info->points_ofs++;

            // 终点
            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_glyph_ori_info->path_segments[i].to.x + \
                    (float)curr_glyph_info->pos.x;
            curr_glyph_info->points_ofs++;

            curr_glyph_info->points[curr_glyph_info->points_ofs] = (float)curr_label_info->height - \
                    (float)(curr_glyph_ori_info->path_segments[i].to.y - \
                            curr_label_info->descender) + \
                    (float)curr_glyph_info->pos.y;
            curr_glyph_info->points_ofs++;
            break;
        default:
            LV_LOG_ERROR("unknown point type: %d", type);
            LV_ASSERT(false);
            break;
        }
    }

    curr_glyph_info->segments_ofs = curr_glyph_ori_info->segments_number;
    //printf("curr_glyph_info->segments_ofs = %d; curr_glyph_info->points_ofs = %d.", curr_glyph_info->segments_ofs, curr_glyph_info->points_ofs);
    //printf("curr_glyph_info->segments = %p; curr_glyph_info->points = %p.", curr_glyph_info->segments, curr_glyph_info->points);
}

static void lv_jlvg_label_draw_task_run(lv_draw_unit_t *draw_unit, const lv_draw_label_dsc_t *dsc, lv_jlvg_label_info_t *curr_label_info)
{
    //printf("[%s]", __func__);
    // 启动 GPU 绘制
    jlvg_matrix_t curr_tf_matrix;

    jlvg_surface_t surface;     // 绘制的窗口
    jlvg_rect_t surface_area;   // 绘制的窗口区域:实际就是 layer->buf_area 所在的区域
    jlvg_rect_t draw_area;      // 在窗口内实际要绘制的区域:实际就是 blend_area

    //初始化单位变换矩阵
    jlvg_matrix_set_identity(&curr_tf_matrix);

    jlvg_matrix_scale(&curr_tf_matrix, curr_label_info->original_scale, curr_label_info->original_scale);

    //窗口初始化
    jlvg_hw_imageformat_t jlvg_dest_cf = lv_jlvg_get_color_format(curr_label_info->dest_cf, NULL);
    jlvg_surface_init(&surface, curr_label_info->dest_buf, curr_label_info->fb_size, curr_label_info->fb_width, curr_label_info->fb_height, curr_label_info->dest_stride, jlvg_dest_cf, 0, 1, 1, 0);

    //初始化要绘制的区域

    // 即 blend_area 区域
    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = curr_label_info->fb_width;
    draw_area.h = curr_label_info->fb_height;
    //printf("draw_area (x,y) = (%d,%d); w = %d, y = %d.", draw_area.x, draw_area.y, draw_area.w, draw_area.h);

    // 暂时只支持填充纯黑色
    jlvg_paint_color_t paint_color;
    jlvg_color_t fill_color = {0x00, 0x00, 0x00, 0x00};
    jlvg_color_t draw_color;

    fill_color.a = dsc->opa;
    fill_color.r = dsc->color.red;
    fill_color.g = dsc->color.green;
    fill_color.b = dsc->color.blue;

    //printf("fill_color: a = 0x%02x; r = 0x%02x; g = 0x%02x; b = 0x%02x.", fill_color.a, fill_color.r, fill_color.g, fill_color.b);

    jlvg_color_premultiply(&fill_color);    // 填充像素预乘
    draw_color = fill_color;
    jlvg_paint_color_init(&paint_color, draw_color); //初始化填充的颜色内容
    jlvg_paint_t *draw_paint;
    draw_paint = &paint_color;

    // 启动 GPU 硬件绘制
    jlvg_start_frame(g_jlvg, &surface);

    // 添加标签中全部字形的路径绘制任务

    //uint32_t draw_begin = get_system_us();

    /*Iterate over glyph_info*/
    lv_ll_t path_list;
    lv_ll_init(&path_list, sizeof(jlvg_path_t));
    lv_jlvg_glyph_info_t *curr_glyph_info = lv_ll_get_head(&curr_label_info->glyph_info_list);
    while (curr_glyph_info != NULL) {
        jlvg_path_t *curr_path = lv_ll_ins_head(&path_list);
        if (curr_path == NULL) {
            printf("error %s %d", __func__, __LINE__);
            LV_ASSERT(false);
        }

#if 0
        // 填充路径绘制任务
        for (int i = 0; i < curr_glyph_info->segments_ofs; i++) {
            printf("curr_glyph_info->segments[%d] = 0x%02x.", i, curr_glyph_info->segments[i]);
        }

        for (int i = 0; i < curr_glyph_info->points_ofs; i++) {
            printf("curr_glyph_info->points[%d] = %f.", i, curr_glyph_info->points[i]);
        }
#endif

        jlvg_path_init(curr_path, JLVG_PATH_DATATYPE_F32, \
                       curr_glyph_info->segments_ofs, \
                       curr_glyph_info->segments, \
                       curr_glyph_info->points_ofs, \
                       curr_glyph_info->points);

        jlvg_path_update_fill(curr_path);  //更新路径信息

        jlvg_fill_path(g_jlvg, &draw_area, curr_path, draw_paint, &curr_tf_matrix, NULL, VGHW_BLEND_SRC_OVER, VGHW_NON_ZERO);

        /*Get next glyph_info*/
        lv_jlvg_glyph_info_t *next_glyph_info = lv_ll_get_next(&curr_label_info->glyph_info_list, curr_glyph_info);

        /*Move on to next glyph_info*/
        curr_glyph_info = next_glyph_info;
    }

    //uint32_t draw_start = get_system_us();

    jlvg_end_frame(g_jlvg);

    //uint32_t draw_end = get_system_us();

    // 清理GPU现场
    // 清理路径信息
    jlvg_path_t *curr_clean_path = lv_ll_get_head(&path_list);
    while (curr_clean_path != NULL) {
        jlvg_path_deinit(curr_clean_path);

        /*Get next path*/
        jlvg_path_t *next_clean_path = lv_ll_get_next(&path_list, curr_clean_path);

        /*Move on to next path*/
        curr_clean_path = next_clean_path;
    }
    lv_ll_clear(&path_list);

    jlvg_paint_deinit((jlvg_paint_t *)&paint_color);    // 暂时默认都是一个颜色效果
    jlvg_surface_deinit(&surface);

    //uint32_t draw_exit = get_system_us();

    //printf("path updata time = %d; draw time = %d; draw exit = %d.", (draw_start - draw_begin), (draw_end - draw_start), (draw_exit - draw_end));
}

#endif /*LV_USE_DRAW_JLVG_LABEL_ENABLE*/
#endif /*LV_USE_DRAW_JLVG*/
