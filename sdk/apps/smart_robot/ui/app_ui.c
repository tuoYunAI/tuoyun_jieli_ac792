
#include "os/os_api.h"
#include <stdlib.h>
#include "system/includes.h"
#include "app_config.h"
#include "system/init.h"
#include "syscfg_id.h"
#include "app_event.h"
#include "app_ui.h"
#include "lvgl.h"

#define LOG_TAG             "[UI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

static OS_MUTEX mutex;
static lv_obj_t *m_label_status = NULL;
static lv_obj_t *m_label_emotion = NULL;
static lv_obj_t *m_label_content = NULL;
static int m_init  = 0;
static int m_status_updated = 1;
static int m_emotion_updated = 1;
static int m_content_updated = 1;
static char m_status_text[128] = {0};
static char m_emotion_text[128] = {0};
static char m_content_text[512] = {0};

void ui_set_status_text(const char *text)
{

    os_mutex_pend(&mutex, 0);
    if (text && strnlen(text, 10) > 0 ) {
        strncpy(m_status_text, text, sizeof(m_status_text) - 1);
    } else {
        strcpy(m_status_text, " ");
    }
    m_status_updated = 1;
    os_mutex_post(&mutex);
}

void ui_set_emotion_text(const char *text)
{
    os_mutex_pend(&mutex, 0);
    if (text && strnlen(text, 10) > 0 ) {
        strncpy(m_emotion_text, text, sizeof(m_emotion_text) - 1);
    } else {
        strcpy(m_emotion_text, " ");
    }
    m_emotion_updated = 1;
    os_mutex_post(&mutex);
    
}

void ui_set_content_text(const char *text)
{
    os_mutex_pend(&mutex, 0);
    if (text && strnlen(text, 10) > 0 ) {
        strncpy(m_content_text, text, sizeof(m_content_text) - 1);
    } else {
        strcpy(m_content_text, " ");
    }
    m_content_updated = 1;
    os_mutex_post(&mutex);
    
}


int lvgl_v9_main_task_hook()
{
    int ret = 0;
    if (m_init == 0) {
        LV_LOG_INFO("LVGL_SCREEN_INIT");

        if (os_mutex_create(&mutex) != OS_NO_ERR) {
            LV_LOG_ERROR("%s os_mutex_create buf_mutex fail\n", __FILE__);
            return ret;
        }
        lv_obj_t *scr = lv_screen_active();

        /* Change the active screen's background color */
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x003a57), LV_PART_MAIN);

        /* create label once and reuse it; update text on subsequent calls */
        if (!m_label_status) {
            m_label_status = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_status, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_align(m_label_status, LV_ALIGN_CENTER, 0, -50);
        }
        if (!m_label_emotion) {
            m_label_emotion = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_emotion, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_align(m_label_emotion, LV_ALIGN_CENTER, 0, -80);
        }
        if (!m_label_content) {
            m_label_content = lv_label_create(scr);
            lv_obj_align(m_label_content, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(m_label_content, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_width(m_label_content, 300);   // 限制 label 宽度，让它自动换行
            lv_label_set_long_mode(m_label_content, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(m_label_content, LV_TEXT_ALIGN_CENTER, 0);
        }
        
        m_init = 1;
    }
    os_mutex_pend(&mutex, 0);
    if (m_status_updated) {
        lv_label_set_text(m_label_status, m_status_text);
        m_status_updated = 0;
        ret = 1;
    }
    if (m_emotion_updated) {
        lv_label_set_text(m_label_emotion, m_emotion_text);
        m_emotion_updated = 0;
        ret = 1;
    }  
    if (m_content_updated) {
        lv_label_set_text(m_label_content, m_content_text);
        m_content_updated = 0;
        ret = 1;
    }   
    
    os_mutex_post(&mutex);
    return ret;
}
