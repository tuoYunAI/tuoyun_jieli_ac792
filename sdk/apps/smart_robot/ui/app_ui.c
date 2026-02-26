#include "os/os_api.h"
#include <stdlib.h>
#include <string.h>
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
static int mutex_ready = 0;

static lv_obj_t *m_label_status = NULL;
static lv_obj_t *m_label_emotion = NULL;
static lv_obj_t *m_label_content = NULL;
static lv_obj_t *m_emotion_gif = NULL;

static int m_init  = 0;
static int m_status_updated = 0;
static int m_emotion_updated = 0;
static int m_content_updated = 0;

static char m_status_text[128] = " ";
static char m_emotion_text[128] = " ";
static char m_content_text[512] = " ";

extern unsigned char emotion_wink_gif[];
extern unsigned int wink_gif_len;

/* =======================  安全锁操作  ======================= */

static inline void ui_lock(void)
{
    if (mutex_ready) {
        os_mutex_pend(&mutex, 0);
    }
}

static inline void ui_unlock(void)
{
    if (mutex_ready) {
        os_mutex_post(&mutex);
    }
}

/* =======================  对外接口  ======================= */

void ui_set_status_text(const char *text)
{
    ui_lock();

    const char *src = (text && text[0]) ? text : " ";
    if (strcmp(m_status_text, src) != 0) {
        strncpy(m_status_text, src, sizeof(m_status_text) - 1);
        m_status_text[sizeof(m_status_text) - 1] = '\0';
        m_status_updated = 1;
        log_info("Status updated: %s", m_status_text);
    }

    ui_unlock();
}

void ui_set_emotion_text(const char *text)
{
    ui_lock();

    const char *src = (text && text[0]) ? text : " ";
    if (strcmp(m_emotion_text, src) != 0) {
        strncpy(m_emotion_text, src, sizeof(m_emotion_text) - 1);
        m_emotion_text[sizeof(m_emotion_text) - 1] = '\0';
        m_emotion_updated = 1;
        log_info("Emotion updated: %s", m_emotion_text);
    }

    ui_unlock();
}

void ui_set_content_text(const char *text)
{
    ui_lock();

    const char *src = (text && text[0]) ? text : " ";
    if (strcmp(m_content_text, src) != 0) {
        strncpy(m_content_text, src, sizeof(m_content_text) - 1);
        m_content_text[sizeof(m_content_text) - 1] = '\0';
        m_content_updated = 1;
        log_info("Content updated: %s", m_content_text);
    }

    ui_unlock();
}

/* =======================  LVGL 主 Hook  ======================= */

int lvgl_v9_main_task_hook(void)
{
    int ret = 0;

    /* ---------- 初始化只执行一次 ---------- */
    if (!m_init) {

        LV_LOG_INFO("LVGL_SCREEN_INIT");

        if (os_mutex_create(&mutex) == OS_NO_ERR) {
            mutex_ready = 1;
        } else {
            LV_LOG_ERROR("mutex create fail");
        }

        lv_obj_t *scr = lv_screen_active();
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

        /* ===== 创建 GIF ===== */
        if (!m_emotion_gif) {
            static lv_image_dsc_t gif_dsc;

            gif_dsc.data = emotion_wink_gif;
            gif_dsc.data_size = wink_gif_len;
            gif_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
            gif_dsc.header.flags = 0;
            gif_dsc.header.cf = LV_COLOR_FORMAT_RAW;
            gif_dsc.header.w = 0;
            gif_dsc.header.h = 0;
            gif_dsc.header.stride = 0;

            m_emotion_gif = lv_gif_create(scr);
            lv_gif_set_src(m_emotion_gif, &gif_dsc);
            lv_obj_align(m_emotion_gif, LV_ALIGN_CENTER, 0, -70);
        }

        /* ===== 状态文字 ===== */
        if (!m_label_status) {
            m_label_status = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_status, lv_color_hex(0xffffff), 0);
            lv_obj_align(m_label_status, LV_ALIGN_CENTER, 0, -20);
            lv_label_set_text(m_label_status, m_status_text);
        }

        /* ===== 表情文字 ===== */
        if (!m_label_emotion) {
            m_label_emotion = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_emotion, lv_color_hex(0xffffff), 0);
            lv_obj_align(m_label_emotion, LV_ALIGN_CENTER, 0, -50);
            lv_label_set_text(m_label_emotion, m_emotion_text);
        }

        /* ===== 内容文字（直接放屏幕，避免裁剪问题）===== */
        if (!m_label_content) {
            m_label_content = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_content, lv_color_hex(0xffffff), 0);
            lv_obj_set_width(m_label_content, lv_pct(90));
            lv_obj_align(m_label_content, LV_ALIGN_BOTTOM_MID, 0, -20);
            lv_label_set_long_mode(m_label_content, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(m_label_content, LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_text(m_label_content, m_content_text);
        }

        m_init = 1;
    }

    /* ---------- 更新 UI ---------- */

    ui_lock();

    if (m_status_updated && m_label_status) {
        lv_label_set_text(m_label_status, m_status_text);
        m_status_updated = 0;
        ret = 1;
    }

    if (m_emotion_updated && m_label_emotion) {
        lv_label_set_text(m_label_emotion, m_emotion_text);
        m_emotion_updated = 0;
        ret = 1;
    }

    if (m_content_updated && m_label_content) {
        lv_label_set_text(m_label_content, m_content_text);
        m_content_updated = 0;
        ret = 1;
    }

    ui_unlock();

    return ret;
}