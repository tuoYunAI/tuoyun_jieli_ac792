
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
static lv_obj_t *m_emotion_gif = NULL;
static int m_init  = 0;
static int m_status_updated = 1;
static int m_emotion_updated = 1;
static int m_content_updated = 1;
static char m_status_text[128] = {0};
static char m_emotion_text[128] = {0};
static char m_content_text[512] = {0};

extern unsigned char emotion_wink_gif[];
extern unsigned int wink_gif_len;

extern unsigned char happy_gif[];
extern unsigned int happy_gif_len;

extern unsigned char dizzy_gif[];
extern unsigned int dizzy_gif_len;
    

void ui_set_status_text(const char *text)
{
    char next_text[sizeof(m_status_text)] = {0};
    os_mutex_pend(&mutex, 0);
    if (text && text[0] != '\0') {
        strncpy(next_text, text, sizeof(next_text) - 1);
    } else {
        strcpy(next_text, " ");
    }

    if (strcmp(m_status_text, next_text) != 0) {
        strcpy(m_status_text, next_text);
        m_status_updated = 1;
        log_info("------------@@@@@@@@@@@Status updated@@@@@@@@@@@@@------------: %s", m_status_text);
    }
    os_mutex_post(&mutex);
}

void ui_set_emotion_text(const char *text)
{    
    char next_text[sizeof(m_emotion_text)] = {0};
    os_mutex_pend(&mutex, 0);
    if (text && text[0] != '\0') {
        strncpy(next_text, text, sizeof(next_text) - 1);
    } else {
        strcpy(next_text, " ");
    }

    if (strcmp(m_emotion_text, next_text) != 0) {
        strcpy(m_emotion_text, next_text);
        m_emotion_updated = 1;
        log_info("------------@@@@@@@@@@@Emotion updated@@@@@@@@@@@@@------------: %s", m_emotion_text);
    }
    os_mutex_post(&mutex); 
}

void ui_set_content_text(const char *text)
{
    char next_text[sizeof(m_content_text)] = {0};
    os_mutex_pend(&mutex, 0);
    if (text && text[0] != '\0') {
        strncpy(next_text, text, sizeof(next_text) - 1);
    } else {
        strcpy(next_text, " ");
    }

    if (strcmp(m_content_text, next_text) != 0) {
        strcpy(m_content_text, next_text);
        m_content_updated = 1;
        log_info("------------@@@@@@@@@@@Content updated@@@@@@@@@@@@@------------: %s", m_content_text);
    }
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

        /* Change the active screen's background color 0x003a57 */
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

        if( !m_emotion_gif ){
            static lv_image_dsc_t s_emotion_wink_gif_dsc;
            s_emotion_wink_gif_dsc.data = dizzy_gif;
            s_emotion_wink_gif_dsc.data_size = dizzy_gif_len;
            s_emotion_wink_gif_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
            s_emotion_wink_gif_dsc.header.flags = 0;
            s_emotion_wink_gif_dsc.header.cf = LV_COLOR_FORMAT_RAW;
            s_emotion_wink_gif_dsc.header.w = 0;
            s_emotion_wink_gif_dsc.header.h = 0;
            s_emotion_wink_gif_dsc.header.stride = 0;
#if 1
            m_emotion_gif = lv_gif_create(scr);
            lv_gif_set_src(m_emotion_gif, &s_emotion_wink_gif_dsc);
            lv_obj_center(m_emotion_gif);
            lv_obj_align(m_emotion_gif, LV_ALIGN_CENTER, 0, -70);
#endif            
        }
        

        /* create label once and reuse it; update text on subsequent calls  */
        if (!m_label_status) {
            m_label_status = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_status, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_align(m_label_status, LV_ALIGN_CENTER, 0, -20);
        }
#if 1
        if (!m_label_emotion) {
            m_label_emotion = lv_label_create(scr);
            lv_obj_set_style_text_color(m_label_emotion, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_align(m_label_emotion, LV_ALIGN_CENTER, 0, -50);
        }
#endif
        int32_t screen_width = lv_obj_get_width(scr);
        int32_t screen_height = lv_obj_get_height(scr); 
        if (!m_label_content) {
            /*创建一个容器，放在屏幕中线 */
            lv_obj_t * container = lv_obj_create(scr);
            /* 去掉容器默认样式 */
            lv_obj_remove_style_all(container);
            lv_obj_set_size(container, screen_width, screen_height/2 - 30);   // 下半屏高度
            /* 对齐到父对象底部，使容器占据下半屏 */
            lv_obj_align(container, LV_ALIGN_BOTTOM_LEFT, 0, 0);

            /* 关键：开启裁剪 */
            lv_obj_set_style_clip_corner(container, true, 0);

            m_label_content = lv_label_create(container);
            lv_obj_align(m_label_content, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_text_color(m_label_content, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_opa(m_label_content, LV_OPA_COVER, 0);
            lv_obj_set_width(m_label_content, screen_width-20);   // 限制 label 宽度，让它自动换行
            lv_label_set_long_mode(m_label_content, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(m_label_content, LV_TEXT_ALIGN_CENTER, 0);
        }
        
        m_init = 1;
    }
    os_mutex_pend(&mutex, 0);
    if (m_status_updated) {
        lv_label_set_text(m_label_status, m_status_text);
        log_info("lv_label_set_text ------------m_status_text: %s", m_status_text);
        m_status_updated = 0;
        ret = 1;
    }
    if (m_emotion_updated) {
        lv_label_set_text(m_label_emotion, m_emotion_text);
        log_info("lv_label_set_text ------------m_emotion_text: %s", m_emotion_text);
        m_emotion_updated = 0;
        ret = 1;
    }  
    if (m_content_updated) {
        lv_label_set_text(m_label_content, m_content_text);
        log_info("lv_label_set_text ------------m_content_text: %s", m_content_text);
        lv_obj_invalidate(m_label_content); // 强制刷新label
        m_content_updated = 0;
        ret = 1;
    }
    
    os_mutex_post(&mutex);
    return ret;
}
