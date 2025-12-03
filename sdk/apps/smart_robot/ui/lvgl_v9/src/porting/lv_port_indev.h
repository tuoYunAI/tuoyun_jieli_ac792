
/**
 * @file lv_port_indev_templ.h
 *
 */

/*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_INDEV_TEMPL_H
#define LV_PORT_INDEV_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl_v9/lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_indev_init(void);
void lv_indev_timer_read_touch(void *user_data);
void lv_indev_set_touch_timer_en(int en);
void lv_indev_timer_read_key(void *user_data);
bool lv_indev_set_touch_timer_check(void);
void lv_indev_timer_read_touch(void *user_data);

#if LV_USE_SIM_INERTIAL_SLIDE
//动态使能模拟滑动坐标
//status = 1 enable;status = 0 disable.
void lv_set_sim_inertial_slide_status(u8 status);
#endif
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_TEMPL_H*/

#endif /*Disable/Enable content*/
