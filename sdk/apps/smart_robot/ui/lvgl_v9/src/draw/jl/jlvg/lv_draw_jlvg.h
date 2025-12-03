#ifndef LV_DRAW_JLVG_H
#define LV_DRAW_JLVG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../lv_conf_internal.h"

#if LV_USE_DRAW_JLVG
#include "../../../display/lv_display_private.h"
#include "../../lv_image_decoder_private.h"
#include "../../sw/lv_draw_sw.h"
#include "../../lv_draw.h"
#include "../../../misc/lv_types.h"
#include "src/draw/lv_draw_buf_private.h"
#include "src/draw/lv_draw_private.h"
#include "src/draw/sw/lv_draw_sw_private.h"
#include "../../../libs/jl_bin/lv_jl_bin.h"

#include "lv_draw_jlvg_common.h"

#include "asm/gpu/jlvg.h"
#include "asm/jlgpu_driver.h"

/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_DEPTH == 8
#error "Can't use 2.5D GPU with LV_COLOR_DEPTH == 8"
#endif

#if LV_COLOR_DEPTH == 16
#define LV_GPU_COLOR_FORMAT VGHW_FORMAT_RGB565
#elif LV_COLOR_DEPTH == 24
#define LV_GPU_COLOR_FORMAT VGHW_FORMAT_RGB888
#elif LV_COLOR_DEPTH == 32
#define LV_GPU_COLOR_FORMAT VGHW_FORMAT_ARGB8888
#else
/*Can't use GPU with other formats*/
#endif

#define LV_JL_GPU_MIN_AREA_SIZE 0    // 交付给 GPU 渲染的最小区域大小，小于这个大小则不采用 GPU，脏块太小的渲染动作反而影响整体的渲染速度

#define JLVG_DRAW_FULL_FB_ENABLE 0      // 使用小窗绘制速度会更快，默认关闭这个宏

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_draw_jlvg_unit_t {
    lv_draw_unit_t base_unit;
    lv_draw_task_t *task_act;
    //lv_draw_sw_thread_dsc_t thread_dscs[LV_DRAW_SW_DRAW_UNIT_CNT];
#if LV_USE_OS
    lv_thread_sync_t sync;
    lv_thread_t thread;
    volatile bool inited;
    volatile bool exit_status;
#endif
} lv_draw_jlvg_unit_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern jlvg_t *g_jlvg;

void lv_draw_jlvg_init(void);

void lv_draw_jlvg_deinit(void);

void _jlvg_buf_invalidate_cache_cb(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

#if (LV_USE_DRAW_JLVG_FILL_ENABLE == 1)
void lv_draw_jlvg_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLVG_IMG_ENABLE == 1)
void lv_draw_jlvg_img(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords);
#endif

#if (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1)
void lv_draw_jlvg_label(lv_draw_unit_t *draw_unit, const lv_draw_label_dsc_t *dsc, const lv_area_t *coords);
#endif

/**********************
 *      MACROS
 **********************/
#endif /*LV_USE_DRAW_JLVG*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_JLVG_H*/
