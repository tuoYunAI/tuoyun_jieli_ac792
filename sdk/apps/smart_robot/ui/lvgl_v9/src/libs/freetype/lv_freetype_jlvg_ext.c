/**
 * @file lv_freetype_jlvg_ext.c
 * 为 JLVG 的矢量绘制收集路径信息
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../lvgl.h"
#include "../../lvgl_private.h"

#if LV_USE_FREETYPE

#if LV_USE_DRAW_JLVG
#if (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1)

#include "../../draw/jl/jlvg/lv_draw_jlvg.h"

/*********************
 *      DEFINES
 *********************/
#define LABEL_MAX_SEGMENTS 1024 // 字形最大的轮廓数，如果超了就要改大绘制修改字形

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void freetype_outline_event_cb(lv_event_t *e);
static void freetype_outline_push(const lv_freetype_outline_event_param_t *param);

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

void lv_draw_jlvg_freetype_event_init(void)
{
    static bool is_init = false;
    if (!is_init) {
        lv_freetype_outline_add_event(freetype_outline_event_cb, LV_EVENT_ALL, NULL);
        is_init = true;
    }
}

bool lv_jlvg_label_freetype_draw_task_create(lv_draw_unit_t *draw_unit, const lv_font_t *font_p, const lv_area_t *coords, lv_jlvg_label_info_t *curr_label_info)
{
    //printf("[%s]", __func__);

    lv_draw_jlvg_unit_t *u = (lv_draw_jlvg_unit_t *)draw_unit;
    lv_draw_task_t *t = u->task_act;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;   //这个区域无论是帧buff还是行buff都是相对的新的绘制区域
    if (!lv_area_intersect(&blend_area, &rel_coords, &rel_clip_area)) {
        return false; /*Fully clipped, nothing to do*/
    }

    // 初始化当前的文本句柄 curr_label_info
    const lv_freetype_font_dsc_t *ft_dsc = font_p->dsc;
    const FT_Face face = ft_dsc->cache_node->face;

    curr_label_info->units_per_em = face->units_per_EM;
    curr_label_info->ascender = face->ascender;
    curr_label_info->descender = face->descender;
    curr_label_info->height = face->height;
    curr_label_info->font_size = ft_dsc->size;
    curr_label_info->original_scale = (float)ft_dsc->size / (float)(face->units_per_EM);

    //printf("curr_label_info->units_per_em = %d.", curr_label_info->units_per_em);
    //printf("curr_label_info->ascender = %d.", curr_label_info->ascender);
    //printf("curr_label_info->descender = %d.", curr_label_info->descender);
    //printf("curr_label_info->height = %d.", curr_label_info->height);
    //printf("curr_label_info->font_size = %d.", curr_label_info->font_size);
    //printf("curr_label_info->original_scale = %f.", curr_label_info->original_scale);

    lv_area_copy(&(curr_label_info->effective_coords), &blend_area);

    // 初始化绘制区域
    curr_label_info->dest_cf = draw_buf->header.cf;
    curr_label_info->bytes_per_pixel = lv_color_format_get_size(curr_label_info->dest_cf);

    curr_label_info->fb_width = blend_area.x2 - blend_area.x1 + 1;       // 绘制区域的宽度
    curr_label_info->fb_height = blend_area.y2 - blend_area.y1 + 1;      // 绘制区域的高度
    curr_label_info->fb_size = curr_label_info->fb_width * \
                               curr_label_info->fb_height * \
                               curr_label_info->bytes_per_pixel;   // 绘制区域的buff大小
    curr_label_info->dest_stride = draw_buf->header.stride;
    curr_label_info->dest_buf = draw_buf->data + (blend_area.x1 * curr_label_info->bytes_per_pixel) + \
                                (blend_area.y1 * curr_label_info->dest_stride);

    //printf("fb_width = %d; fb_height = %d; dest_stride = %d.", curr_label_info->fb_width, curr_label_info->fb_height, curr_label_info->dest_stride);

    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void freetype_outline_event_cb(lv_event_t *e)
{
    LV_PROFILER_BEGIN;
    lv_event_code_t code = lv_event_get_code(e);
    lv_freetype_outline_event_param_t *param = lv_event_get_param(e);
    lv_jlvg_glyph_ori_info_t *glyph_ori_info = NULL;
    //printf("code = 0x%02x.", code);
    switch (code) {
    case LV_EVENT_CREATE:
        //printf("path create!!");
        glyph_ori_info = (lv_jlvg_glyph_ori_info_t *)lv_malloc_zeroed(sizeof(lv_jlvg_glyph_ori_info_t));
        if (glyph_ori_info == NULL) {
            printf("glyph_ori_info alloc is fail !!! >> %s %d", __func__, __LINE__);
            LV_ASSERT(false);
        }

        // 创建缓冲区暂存路径
        glyph_ori_info->path_segments = (lv_jlvg_path_segment_t *) \
                                        lv_malloc(sizeof(lv_jlvg_path_segment_t) * \
                                                LABEL_MAX_SEGMENTS);
        if (glyph_ori_info->path_segments == NULL) {
            printf("glyph_ori_info->path_segments alloc is fail !!! >> %s %d", __func__, __LINE__);
            LV_ASSERT(false);
        }

        param->outline = glyph_ori_info;

        break;
    case LV_EVENT_DELETE:
        //printf("path delete");
        glyph_ori_info = param->outline;
        lv_free(glyph_ori_info->path_segments);
        lv_free(glyph_ori_info);

        break;
    case LV_EVENT_INSERT:
        freetype_outline_push(param);
        break;
    default:
        LV_LOG_WARN("unknown event code: %d", code);
        break;
    }
    LV_PROFILER_END;
}

static void freetype_outline_push(const lv_freetype_outline_event_param_t *param)
{
    LV_PROFILER_BEGIN;

    lv_jlvg_glyph_ori_info_t *curr_glyph_ori_info = (lv_jlvg_glyph_ori_info_t *)param->outline;

    LV_ASSERT_NULL(curr_glyph_ori_info);

    //lv_memset(curr_glyph_ori_info->path_segments, 0, sizeof(lv_jlvg_path_segment_t) * LABEL_MAX_SEGMENTS);

    if (curr_glyph_ori_info->segments_number >= LABEL_MAX_SEGMENTS) {
        printf("error: LABEL_MAX_SEGMENTS is %d, curr segments_num = %d!!!", LABEL_MAX_SEGMENTS, curr_glyph_ori_info->segments_number);
        assert(0);
    }

    lv_freetype_outline_type_t type = param->type;

    switch (type) {
    case LV_FREETYPE_OUTLINE_END:
        //printf("outline end, segments_number = %d", curr_glyph_ori_info->segments_number);
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].type = VGHW_CLOSE_PATH;

        //  一个字形的路径记录结束 记录最后一个命令
        curr_glyph_ori_info->segments_number++;

        //printf("curr_glyph_ori_info->path_segments = %p.", curr_glyph_ori_info->path_segments);

#if 1   // 16 字节对齐
        // 重新调整存储路径信息的空间
        uint32_t segments_tmp_size = ((sizeof(lv_jlvg_path_segment_t) * curr_glyph_ori_info->segments_number) & (~0xf)) + 16;// 16 字节对齐
        //printf("segments_tmp_size = %d.", segments_tmp_size);
        lv_jlvg_path_segment_t *segments_tmp = (lv_jlvg_path_segment_t *)lv_realloc(curr_glyph_ori_info->path_segments, \
                                               segments_tmp_size);
        if (segments_tmp == NULL) {
            printf("segments_tmp realloc is fail!!! >> %s %d", __func__, __LINE__);
            LV_ASSERT(false);
        }

        curr_glyph_ori_info->path_segments = segments_tmp;
#endif

        LV_PROFILER_END;
        return;
    case LV_FREETYPE_OUTLINE_MOVE_TO:
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].type = VGHW_MOVE_TO;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.x = param->to.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.y = param->to.y;
        break;
    case LV_FREETYPE_OUTLINE_LINE_TO:
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].type = VGHW_LINE_TO;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.x = param->to.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.y = param->to.y;
        break;
    case LV_FREETYPE_OUTLINE_CUBIC_TO:
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].type = VGHW_CUBIC_TO;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.x = param->to.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.y = param->to.y;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].control1.x = param->control1.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].control1.y = param->control1.y;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].control2.x = param->control2.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].control2.y = param->control2.y;
        break;
    case LV_FREETYPE_OUTLINE_CONIC_TO:
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].type = VGHW_QUAD_TO;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.x = param->to.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].to.y = param->to.y;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].control1.x = param->control1.x;
        curr_glyph_ori_info->path_segments[curr_glyph_ori_info->segments_number].control1.y = param->control1.y;
        break;
    default:
        LV_LOG_ERROR("unknown point type: %d", type);
        LV_ASSERT(false);
        break;
    }

    curr_glyph_ori_info->segments_number++;

    LV_PROFILER_END;
}

#endif /*LV_USE_DRAW_JLVG_LABEL_ENABLE*/
#endif /*LV_USE_DRAW_JLVG*/
#endif /*LV_USE_FREETYPE*/
