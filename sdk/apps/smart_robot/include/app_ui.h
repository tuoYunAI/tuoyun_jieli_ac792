#ifndef __APP_UI_H__
#define __APP_UI_H__

/* 表情 gif 枚举 */
typedef enum {
    EMOTION_GIF_WINK = 0,
    EMOTION_GIF_ANGRY,
    EMOTION_GIF_BLINK,
    EMOTION_GIF_DIZZY,
    EMOTION_GIF_HAPPY,
    EMOTION_GIF_SAD,
    EMOTION_GIF_SLEEP,
    EMOTION_GIF_MAX
} emotion_gif_e;

void ui_set_status_text(const char *text);

void ui_set_emotion_text(const char *text);

void ui_set_content_text(const char *text);

void ui_set_emotion_gif(emotion_gif_e emotion);

#endif