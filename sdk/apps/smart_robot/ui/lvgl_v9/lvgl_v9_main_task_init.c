#include "app_config.h"
#include "system/includes.h"
#include "lv_conf.h"
#include "lvgl.h"

#ifdef USE_LVGL_V9_UI


#define LOG_TAG             "[UI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


static void lv_example_img_bin(void)
{
    LV_IMAGE_DECLARE(img_benchmark_cogwheel_argb);
    static lv_style_t style_common;

    lv_style_reset(&style_common);

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_obj_remove_style_all(img);
    lv_obj_add_style(img, &style_common, 0);

    //Img
    lv_obj_set_pos(img, 330, 189);
    //C数组
    /* lv_img_set_src(img, &img_benchmark_cogwheel_argb); */
    //bin文件
    lv_img_set_src(img, "mnt/sdfile/EXT_RESERVED/uipackres/ui/logoargb.bin");

    //Font
    /* lv_font_t *font_bin = lv_font_load_bin("mnt/sdfile/EXT_RESERVED/uipackres/ui/font1.bin"); //不压缩字体 */
    lv_font_t *font_bin = lv_font_load_bin("mnt/sdfile/EXT_RESERVED/uipackres/ui/font1.rle");//压缩字体
    lv_obj_set_style_text_font(label, font_bin, LV_PART_MAIN);//
    /* lv_obj_set_style_text_font(label,&lv_font_montserrat_24, LV_PART_MAIN);//C数组 */

    lv_label_set_text(label, "123");

    lv_obj_set_pos(label, 100, 100);
}

void lvgl_v9_gui_init(void)
{
    log_info("lvgl_v9_gui_init");

#if 0
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
        puts("waitting sd on... ");
    }
#endif

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_fs_init();
    lv_tick_set_cb(&timer_get_ms);

#if LV_USE_MONKEY != 0
    /*Create pointer monkey test*/
    lv_monkey_config_t monkey_pointer_config;
    monkey_pointer_config.type = LV_INDEV_TYPE_POINTER;
    monkey_pointer_config.period_range.min = 20;
    monkey_pointer_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_pointer_config), true);

    /*Create keypad monkey test*/
    lv_monkey_config_t monkey_keypad_config;
    monkey_keypad_config.type = LV_INDEV_TYPE_KEYPAD;
    monkey_keypad_config.period_range.min = 20;
    monkey_keypad_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_keypad_config), true);

    /*Create encoder monkey test*/
    lv_monkey_config_t monkey_encode_config;
    monkey_encode_config.type = LV_INDEV_TYPE_ENCODER;
    monkey_encode_config.period_range.min = 20;
    monkey_encode_config.period_range.max = 20;
    monkey_encode_config.input_range.min = -5;
    monkey_encode_config.input_range.max = 5;
    lv_monkey_set_enable(lv_monkey_create(&monkey_encode_config), true);
#endif

    /*lv_demo_benchmark();*/
    //lv_demo_widgets();
    /*lv_demo_keypad_encoder();*/
}

static int lvgl_v9_main_init(void)
{
    log_info("lvgl_v9_main_init");
    int lvgl_v9_main_task_init(void);
    lvgl_v9_main_task_init();
    return 0;
}
__initcall(lvgl_v9_main_init);

#endif


