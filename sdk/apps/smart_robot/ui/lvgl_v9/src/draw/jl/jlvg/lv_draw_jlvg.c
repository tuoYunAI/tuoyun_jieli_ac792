/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jlvg.h"

#if LV_USE_DRAW_JLVG

#include "../../lv_draw_buf_private.h"
#include "../../sw/lv_draw_sw.h"


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

#define DRAW_UNIT_ID_JLVG 102

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
jlvg_t *g_jlvg = NULL;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/*
 * Evaluate a task and set the score and preferred JL GPU unit.
 * Return 1 if task is preferred, 0 otherwise (task is not supported).
 */
static int32_t _jlvg_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

/*
 * Dispatch a task to the JL GPU unit.
 * Return 1 if task was dispatched, 0 otherwise (task not supported).
 */
static int32_t _jlvg_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

/*
 * Delete the JL GPU draw unit.
 */
static int32_t _jlvg_delete(lv_draw_unit_t *draw_unit);

#if LV_DRAW_JLVG_ASYNC && LV_USE_OS
static void _jlvg_render_thread_cb(void *ptr);
#endif

static void _jlvg_execute_drawing(lv_draw_jlvg_unit_t *u);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool check_image_cf_is_supported(lv_color_format_t cf)
{
    bool cf_flag = false;
    lv_jlvg_get_color_format(cf, &cf_flag);
    return cf_flag;
}

static void lv_draw_buf_jlvg_init_handlers(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = _jlvg_buf_invalidate_cache_cb;
}

void _jlvg_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    uint8_t *dest_buf;
    int32_t i = 0;
    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);

    uint8_t *buf = draw_buf->data;
    uint32_t stride = draw_buf->header.stride;
    lv_color_format_t color_format = draw_buf->header.cf;
    uint32_t bytes_per_pixel = lv_color_format_get_size(color_format);
    uint32_t width_bytes = width * bytes_per_pixel;
    //printf("flush area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d). stride = %d.", area->x1, area->y1, area->x2, area->y2, stride);

    dest_buf = buf + (area->y1 * stride) + (area->x1 * bytes_per_pixel);
    for (i = 0; i < lines; i++) {
        DcuFlushinvRegion((u32 *)dest_buf, width_bytes);    //帧buff,清除 area 大小的相对地址空间
        dest_buf += stride;
    }
}

static int32_t _jlvg_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task)
{
    //log_debug("%s()", __func__);

    lv_draw_jlvg_unit_t *draw_jlvg_unit = (lv_draw_jlvg_unit_t *) draw_unit;

    if ((lv_area_get_size(&(task->area)) <= LV_JL_GPU_MIN_AREA_SIZE) && (task->type != LV_DRAW_TASK_TYPE_LABEL)) {
        return 0;   /* 需要渲染的区域太小则放弃使用 GPU */
    }

    switch (task->type) {

    case LV_DRAW_TASK_TYPE_FILL: {
#if (LV_USE_DRAW_JLVG_FILL_ENABLE == 0)
        return 0;
#endif

        lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *)task->draw_dsc;

        if (draw_dsc->grad.dir != (lv_grad_dir_t)LV_GRAD_DIR_NONE) {
            // TODO
            return 0;
        }

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLVG;
        }
        return 1;
    }
    case LV_DRAW_TASK_TYPE_IMAGE: {
#if (LV_USE_DRAW_JLVG_IMG_ENABLE == 0)
        return 0;
#endif
        lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *)task->draw_dsc;
        lv_image_header_t img_header;
        lv_result_t res = lv_image_decoder_get_info(draw_dsc->src, &img_header);
        if (res != LV_RESULT_OK) {
            //LV_LOG_WARN("get image info failed");
            return 0;
        }

        lv_color_format_t img_cf = img_header.cf;

        if (!check_image_cf_is_supported(img_cf)) {     // 判断图像源的颜色格式是否支持
            return 0;
        }

        bool mode_flag;
        lv_jlvg_get_blend_mode(draw_dsc->blend_mode, &mode_flag);
        if (!mode_flag) {
            return 0;
        }

        /* not support skew */
        if (draw_dsc->skew_x != 0 || draw_dsc->skew_y != 0) {
            return 0;
        }

        bool transformed = draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE ||
                           draw_dsc->scale_y != LV_SCALE_NONE ? true : false;

        bool masked = draw_dsc->bitmap_mask_src != NULL;

        if (masked && transformed) {
            //TODO
            return 0;
        }

        lv_color_format_t cf = draw_dsc->header.cf;
        if (masked && (cf == LV_COLOR_FORMAT_A8 || cf == LV_COLOR_FORMAT_RGB565A8)) {
            //TODO
            return 0;
        }

        if (draw_dsc->tile) {
            //TODO
            return 0;
        }

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLVG;
        }
        return 1;
    }
    case LV_DRAW_TASK_TYPE_LABEL: {
#if (LV_USE_DRAW_JLVG_LABEL_ENABLE == 0)
        return 0;
#endif
        lv_draw_label_dsc_t *draw_dsc = (lv_draw_label_dsc_t *)task->draw_dsc;

        if (lv_font_jlvg_draw_check_is_vector(draw_dsc->font) == false) {
            return 0;   // 非矢量绘制不走 jlvg 加速
        }

        if (task->preference_score > 80) {
            task->preference_score = 80;
            task->preferred_draw_unit_id = DRAW_UNIT_ID_JLVG;
        }
        return 1;
    }
    default:
        return 0;
    }

    return 0;
}

static int32_t _jlvg_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_jlvg_unit_t *draw_jlvg_unit = (lv_draw_jlvg_unit_t *) draw_unit;

    /* Return immediately if it's busy with draw task. */
    if (draw_jlvg_unit->task_act) {
        return 0;
    }

    /* Try to get an ready to draw. */
    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_JLVG);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_JLVG) {
        return -1;
    }

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL) {
        return -1;
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_jlvg_unit->task_act = t;

#if LV_DRAW_JLVG_ASYNC && LV_USE_OS
    /* Let the render thread work. */
    if (draw_jlvg_unit->inited) {
        lv_thread_sync_signal(&draw_jlvg_unit->sync);
    }
#else
    _jlvg_execute_drawing(draw_jlvg_unit);

    draw_jlvg_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
    draw_jlvg_unit->task_act = NULL;

    /* The draw unit is free now. Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _jlvg_delete(lv_draw_unit_t *draw_unit)
{
#if LV_DRAW_JLVG_ASYNC && LV_USE_OS
    lv_draw_jlvg_unit_t *draw_jlvg_unit = (lv_draw_jlvg_unit_t *) draw_unit;

    LV_LOG_INFO("Cancel JL GPU draw thread.");
    draw_jlvg_unit->exit_status = true;

    if (draw_jlvg_unit->inited) {
        lv_thread_sync_signal(&draw_jlvg_unit->sync);
    }

    lv_result_t res = lv_thread_delete(&draw_jlvg_unit->thread);

    return res;
#else
    LV_UNUSED(draw_unit);

    return 0;
#endif
}

static void _jlvg_execute_drawing(lv_draw_jlvg_unit_t *u)
{
    //log_debug("%s()", __func__);

    lv_draw_task_t *t = u->task_act;
    lv_draw_unit_t *draw_unit = (lv_draw_unit_t *)u;
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    /* Set target buffer */
    lv_area_t clip_area;
    lv_area_copy(&clip_area, &t->clip_area);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t draw_area;
    lv_area_copy(&draw_area, &t->area);
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

    if (!lv_area_intersect(&draw_area, &draw_area, &clip_area)) {
        return;    /*Fully clipped, nothing to do*/
    }

    /* Invalidate the drawing area */
    lv_draw_buf_invalidate_cache(draw_buf, &draw_area);

    switch (t->type) {
    case LV_DRAW_TASK_TYPE_FILL:
#if (LV_USE_DRAW_JLVG_FILL_ENABLE == 1)
        lv_draw_jlvg_fill(draw_unit, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_IMAGE:
#if (LV_USE_DRAW_JLVG_IMG_ENABLE == 1)
        lv_draw_jlvg_img(draw_unit, t->draw_dsc, &t->area);
#endif
        break;
    case LV_DRAW_TASK_TYPE_LABEL:
#if (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1)
        lv_draw_jlvg_label(draw_unit, t->draw_dsc, &t->area);
#endif
        break;
    default:
        break;
    }
}

#if LV_DRAW_JLVG_ASYNC && LV_USE_OS
static void _jlvg_render_thread_cb(void *ptr)
{
    lv_draw_jlvg_unit_t *u = ptr;

    lv_thread_sync_init(&u->sync);
    LV_LOG("jlvg render async thread run\n");
    u->inited = true;

    while (1) {
        /* Wait for sync if there is no task set. */
        while (u->task_act == NULL) {
            if (u->exit_status) {
                break;
            }

            lv_thread_sync_wait(&u->sync);
        }

        if (u->exit_status) {
            LV_LOG_INFO("Ready to exit JL DMA2D draw thread.");
            break;
        }

        _jlvg_execute_drawing(u);

        /* Signal the ready state to dispatcher. */
        u->task_act->state = LV_DRAW_TASK_STATE_READY;

        /* Cleanup. */
        u->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new task. */
        lv_draw_dispatch_request();
    }

    u->inited = false;
    lv_thread_sync_delete(&u->sync);
    LV_LOG_INFO("Exit JL GPU draw thread.");
}
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_draw_jlvg_init(void)
{
    /* lv_draw_buf_jlvg_init_handlers(); */

    lv_draw_jlvg_unit_t *draw_jlvg_unit = lv_draw_create_unit(sizeof(lv_draw_jlvg_unit_t));
    draw_jlvg_unit->base_unit.evaluate_cb = _jlvg_evaluate;
    draw_jlvg_unit->base_unit.dispatch_cb = _jlvg_dispatch;
    draw_jlvg_unit->base_unit.delete_cb = _jlvg_delete;
    draw_jlvg_unit->base_unit.name = "JLVG";

    /* JL gpu hw init */
    g_jlvg = jlvg_create();

    log_info("jl gpu hw init. >>>>>>>>>>>");

#if LV_DRAW_JLVG_ASYNC && LV_USE_OS
    lv_thread_init(&draw_jlvg_unit->thread, "lv_draw_jlvg", LV_THREAD_PRIO_HIGH, _jlvg_render_thread_cb, 8 * 1024, draw_jlvg_unit);
#endif
    return;
}

void lv_draw_jlvg_deinit(void)
{
    /* JL gpu hw deinit */
    jlvg_free(g_jlvg);

    log_info("jl gpu hw deinit.");

    return;
}

#endif /*LV_USE_DRAW_JLVG*/
