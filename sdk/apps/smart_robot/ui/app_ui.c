
#include "os/os_api.h"
#include <stdlib.h>
#include "app_config.h"
#include "syscfg_id.h"
#include "app_event.h"
#include "app_ui.h"
#include "lvgl.h"

static int cnt = 0;
/* reuse the same label to avoid creating multiple labels on repeated calls */
static lv_obj_t *g_label = NULL;
void lv_example_get_started_1(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* Change the active screen's background color */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x003a57), LV_PART_MAIN);

    /* create label once and reuse it; update text on subsequent calls */
    if (!g_label) {
        g_label = lv_label_create(scr);
        lv_obj_set_style_text_color(g_label, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_align(g_label, LV_ALIGN_CENTER, 0, 0);
    }

    switch (cnt) {
    case 0:
        lv_label_set_text(g_label, "拓云 AI");
        cnt++;
        break;
    case 1:
        lv_label_set_text(g_label, "Speaking");
        cnt++;
        break;
    case 2:
        lv_label_set_text(g_label, "Listening");
        cnt++;
        break;
    default:
        cnt = 0;
        break;
    }
}
