/**
 * @file lv_draw_dma2d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jldma2d_private.h"
#if LV_USE_DRAW_JLDMA2D
#include "../../lv_draw_buf_private.h"
#include "../../sw/lv_draw_sw.h"
#include "../../../misc/lv_area_private.h"


/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_JLDMA2D 13

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int32_t evaluate_cb(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);
static int32_t dispatch_cb(lv_draw_unit_t *draw_unit, lv_layer_t *layer);
static int32_t delete_cb(lv_draw_unit_t *draw_unit);
static void lv_draw_buf_jldma2d_init_handlers(void);
#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
static void thread_cb(void *arg);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
static lv_draw_jldma2d_unit_t *g_unit;
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void lv_draw_buf_jldma2d_init_handlers(void)
{

    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = lv_draw_jldma2d_buf_invalidate_cache_cb;
}

void lv_draw_jldma2d_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    uint32_t stride = draw_buf->header.stride;
    uint8_t *address = draw_buf->data;
    int32_t i = 0;
    uint32_t bytes_per_pixel = lv_color_format_get_size(draw_buf->header.cf);
    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);
    uint32_t bytes_to_flush_per_line = (int32_t)width * (int32_t)bytes_per_pixel;;


    /* Stride is in bytes, not pixels */
    address = address + (area->x1 * (int32_t)bytes_per_pixel) + (stride * (int32_t)area->y1);
    for (i = 0; i < lines; i++) {
        DcuFlushinvRegion((u32 *)address, bytes_to_flush_per_line);   //帧buff,清除 area 大小的相对地址空间
        address += stride;
    }

}


void lv_draw_jldma2d_init(void)
{
    lv_draw_buf_jldma2d_init_handlers();

    LV_LOG("jldma2d render hw init.\n");

    lv_draw_jldma2d_unit_t *draw_unit = lv_draw_create_unit(sizeof(lv_draw_jldma2d_unit_t));
    draw_unit->base_unit.evaluate_cb = evaluate_cb;
    draw_unit->base_unit.dispatch_cb = dispatch_cb;
    draw_unit->base_unit.delete_cb = delete_cb;
    draw_unit->base_unit.name = "JLDMA2D";


#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
    g_unit = draw_unit;

    lv_result_t res = lv_thread_init(&draw_unit->thread, "lv_draw_jldma2d", LV_THREAD_PRIO_HIGH, thread_cb, 2 * 1024, draw_unit);
    LV_ASSERT(res == LV_RESULT_OK);
#endif

    /* enable the DMA2D clock */
    dma2d_init();

}

void lv_draw_jldma2d_deinit(void)
{
    /* JL dma2d hw deinit */
    dma2d_free();

#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
    lv_result_t res = lv_thread_delete(&g_unit->thread);
    LV_ASSERT(res == LV_RESULT_OK);

    res = lv_thread_sync_delete(&g_unit->sync);
    LV_ASSERT(res == LV_RESULT_OK);

    g_unit = NULL;
#endif


}

#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
void lv_draw_jldma2d_transfer_complete_interrupt_handler(void)
{
    if (g_unit && g_unit->inited) {
        lv_thread_sync_signal_isr(&g_unit->sync);
    }
}
#endif

uint16_t lv_draw_jldma2d_cf_to_dma2d_output_cf(lv_color_format_t cf)
{
    switch (cf) {
    case LV_COLOR_FORMAT_ARGB8888:
    case LV_COLOR_FORMAT_XRGB8888:
        return JLDMA2D_FORMAT_ARGB8888;
    case LV_COLOR_FORMAT_RGB888:
        return JLDMA2D_FORMAT_RGB888;
    case LV_COLOR_FORMAT_RGB565:
        return JLDMA2D_FORMAT_RGB565;
    case LV_COLOR_FORMAT_ARGB8565:
        return JLDMA2D_FORMAT_ARGB8565;
    case LV_COLOR_FORMAT_I8:
        return JLDMA2D_FORMAT_L8;
    case LV_COLOR_FORMAT_I4:
        return JLDMA2D_FORMAT_L4;
    case LV_COLOR_FORMAT_I2:
        return JLDMA2D_FORMAT_L2;
    case LV_COLOR_FORMAT_I1:
        return JLDMA2D_FORMAT_L1;
    case LV_COLOR_FORMAT_A1:
        return JLDMA2D_FORMAT_A1;
    case LV_COLOR_FORMAT_A2:
        return JLDMA2D_FORMAT_A2;
    case LV_COLOR_FORMAT_A4:
        return JLDMA2D_FORMAT_A4;
    case LV_COLOR_FORMAT_A8:
        return JLDMA2D_FORMAT_A8;
    default:
        LV_ASSERT_MSG(false, "unsupported output color format");
    }
    return JLDMA2D_FORMAT_RGB565;
}

uint32_t lv_draw_jldma2d_color_to_dma2d_color(uint16_t cf, lv_color_t color)
{
    switch (cf) {
    case JLDMA2D_FORMAT_ARGB8888:
    case JLDMA2D_FORMAT_RGB888:
        return lv_color_to_u32(color);
    case JLDMA2D_FORMAT_RGB565:
        return (uint32_t)lv_color_to_u16(color);
    default:
        LV_ASSERT_MSG(false, "unsupported output color format");
    }
    return 0;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/
/* 判断是不是JL dma2d可以处理的图像操作
 * @note 只支持ARGB8888, RGB888, RGB565, I8, I4, I2, I1, A1, A2, A4, A8纯贴图，
         不支持mask,不支持recolor,不支持skew,不支持rotation,不支持scale,不支持tile,不支持sup
 * @note 如果是I系列支持，支持recolor
 */
static bool __is_jldma2d_image_normal_map(lv_draw_image_dsc_t *dsc)
{
    if (dsc->header.cf < LV_COLOR_FORMAT_PROPRIETARY_START
        && dsc->clip_radius == 0
        && dsc->bitmap_mask_src == NULL
        && dsc->sup == NULL
        && dsc->tile == 0
        && dsc->blend_mode == LV_BLEND_MODE_NORMAL
        && dsc->skew_y == 0
        && dsc->skew_x == 0
        && dsc->scale_x == 256
        && dsc->scale_y == 256
        && dsc->rotation == 0
        && (lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_VARIABLE
            || lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_BIN)
        && (dsc->base.layer->color_format == LV_COLOR_FORMAT_ARGB8888
            || dsc->base.layer->color_format == LV_COLOR_FORMAT_ARGB8565
            || dsc->base.layer->color_format == LV_COLOR_FORMAT_XRGB8888
            || dsc->base.layer->color_format == LV_COLOR_FORMAT_RGB888
            || dsc->base.layer->color_format == LV_COLOR_FORMAT_RGB565)) {
        return true;
    }
    return false;
}
static int execute_drawing(lv_draw_jldma2d_unit_t *u)
{
    /*Render the draw task*/
    lv_draw_task_t *t = u->task_act;
    void *dest;


    lv_layer_t *layer = t->target_layer;
    lv_area_t clipped_area, clipped_coords;
    if (!lv_area_intersect(&clipped_area, &t->area, &t->clip_area)) {
        return LV_DRAW_UNIT_IDLE;
    }
    lv_area_copy(&clipped_coords, &clipped_area);

    int32_t x = 0 - layer->buf_area.x1;
    int32_t y = 0 - layer->buf_area.y1;
    lv_area_move(&clipped_area, x, y);
    /* Invalidate cache */
    lv_draw_buf_invalidate_cache(layer->draw_buf, &clipped_area);

    switch (t->type) {
    case LV_DRAW_TASK_TYPE_FILL: {
        lv_draw_fill_dsc_t *dsc = t->draw_dsc;
        dest = lv_draw_layer_go_to_xy(layer, clipped_coords.x1 - layer->buf_area.x1, clipped_coords.y1 - layer->buf_area.y1);
        if (dsc->opa < LV_OPA_MAX) {
            lv_draw_dma2d_opaque_fill(u,
                                      dest,
                                      lv_area_get_width(&clipped_coords),
                                      lv_area_get_height(&clipped_coords),
                                      lv_draw_buf_width_to_stride(lv_area_get_width(&layer->buf_area), dsc->base.layer->color_format));

        } else {
            lv_draw_jldma2d_fill(u,
                                 dest,
                                 lv_area_get_width(&clipped_coords),
                                 lv_area_get_height(&clipped_coords),
                                 lv_draw_buf_width_to_stride(lv_area_get_width(&layer->buf_area), dsc->base.layer->color_format));
        }
    }
    break;
#if 0
    case LV_DRAW_TASK_TYPE_BG_IMG:

        break;
#endif
    case LV_DRAW_TASK_TYPE_LABEL:

        break;
    case LV_DRAW_TASK_TYPE_IMAGE:
        lv_draw_jldma2d_image(u, t->draw_dsc, &t->area);
        break;
    default:
        break;
    }


    return 0;

}
static int32_t evaluate_cb(lv_draw_unit_t *draw_unit, lv_draw_task_t *task)
{
    switch (task->type) {
    case LV_DRAW_TASK_TYPE_FILL: {
        lv_draw_fill_dsc_t *dsc = task->draw_dsc;
        if ((dsc->radius == 0
             && dsc->grad.dir == LV_GRAD_DIR_NONE
             && (dsc->base.layer->color_format == LV_COLOR_FORMAT_ARGB8888
                 || dsc->base.layer->color_format == LV_COLOR_FORMAT_XRGB8888
                 || dsc->base.layer->color_format == LV_COLOR_FORMAT_RGB888
                 || dsc->base.layer->color_format == LV_COLOR_FORMAT_RGB565))) {

            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLDMA2D;
            task->preference_score = 0;
        }
    }
    break;
    case LV_DRAW_TASK_TYPE_IMAGE: {
        lv_draw_image_dsc_t *dsc = task->draw_dsc;
        if (lv_get_compress_type(dsc->src) != LV_COMPRESS_NONE) {
            //压缩
            if (__is_jldma2d_image_normal_map(dsc)
                && dsc->recolor_opa <= LV_OPA_MIN
                && (dsc->header.cf == LV_COLOR_FORMAT_ARGB8888
                    || dsc->header.cf == LV_COLOR_FORMAT_ARGB8565
                    || dsc->header.cf == LV_COLOR_FORMAT_XRGB8888
                    || dsc->header.cf == LV_COLOR_FORMAT_RGB888
                    || dsc->header.cf == LV_COLOR_FORMAT_RGB565
                    || (dsc->header.cf >= LV_COLOR_FORMAT_I1 && dsc->header.cf <= LV_COLOR_FORMAT_I8))) {
                task->preferred_draw_unit_id = DRAW_UNIT_ID_JLDMA2D;
                task->preference_score = 0;
            } else if (__is_jldma2d_image_normal_map(dsc)
                       && dsc->header.cf >= LV_COLOR_FORMAT_A1 && dsc->header.cf <= LV_COLOR_FORMAT_A8) {
                task->preferred_draw_unit_id = DRAW_UNIT_ID_JLDMA2D;
                task->preference_score = 0;
            } else {
                //dma2d 不支持的图片渲染操作，先解压
                //TODO

            }

        }/* 非压缩C数组/bin */
        else  if (__is_jldma2d_image_normal_map(dsc)
                  && dsc->recolor_opa <= LV_OPA_MIN
                  && (dsc->header.cf == LV_COLOR_FORMAT_ARGB8888
                      || dsc->header.cf == LV_COLOR_FORMAT_ARGB8565
                      || dsc->header.cf == LV_COLOR_FORMAT_XRGB8888
                      || dsc->header.cf == LV_COLOR_FORMAT_RGB888
                      || dsc->header.cf == LV_COLOR_FORMAT_RGB565
                      || (dsc->header.cf >= LV_COLOR_FORMAT_I1 && dsc->header.cf <= LV_COLOR_FORMAT_I8))) {

            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLDMA2D;
            task->preference_score = 0;
        } else if (dsc->header.cf >= LV_COLOR_FORMAT_A1 && dsc->header.cf <= LV_COLOR_FORMAT_A8) {
            if (__is_jldma2d_image_normal_map(dsc)) {
                task->preferred_draw_unit_id = DRAW_UNIT_ID_JLDMA2D;
                task->preference_score = 0;
            }
        } else {
            return 0; // not supported
        }

    }
    break;
    default:
        return 0;
    }

    return 0;
}

static int32_t dispatch_cb(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_jldma2d_unit_t *dma2d_unit = (lv_draw_jldma2d_unit_t *) draw_unit;

    if (dma2d_unit->task_act) {
        /*Return immediately if it's busy with draw task*/
        return 0;
    }

    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_JLDMA2D);
    while (t && t->preferred_draw_unit_id != DRAW_UNIT_ID_JLDMA2D) {
        t->state = LV_DRAW_TASK_STATE_READY;
        t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_JLDMA2D);
    }
    if (t == NULL) {
        return LV_DRAW_UNIT_IDLE;
    }

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL) {
        return LV_DRAW_UNIT_IDLE;
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    dma2d_unit->task_act = t;

#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
    /*Let the render thread work*/
    if (dma2d_unit->inited) {
        lv_thread_sync_signal(&dma2d_unit->sync);
    }
#else
    if (execute_drawing(dma2d_unit) == LV_DRAW_UNIT_IDLE) {
        return LV_DRAW_UNIT_IDLE;
    }
    dma2d_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
    dma2d_unit->task_act = NULL;

    /*The draw unit is free now. Request a new dispatching as it can get a new task*/
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t delete_cb(lv_draw_unit_t *draw_unit)
{
    return 0;
}

#if LV_DRAW_JLDMA2D_ASYNC && LV_USE_OS
static void thread_cb(void *arg)
{
    lv_draw_jldma2d_unit_t *u = arg;

    lv_thread_sync_init(&u->sync);

    LV_LOG("jldma2d render async thread run\n");
    u->inited = true;

    while (1) {

        while (u->task_act == NULL) {
            lv_thread_sync_wait(&u->sync);
        }
        //LV_LOG("jldma2d render.\n");
        execute_drawing(u);

        u->task_act->state = LV_DRAW_TASK_STATE_READY;
        u->task_act = NULL;

        /*The draw unit is free now. Request a new dispatching as it can get a new task*/
        lv_draw_dispatch_request();
    }
}
#endif

#endif /*LV_USE_DRAW_JLDMA2D*/

