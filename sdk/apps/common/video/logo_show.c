#include "logo_show.h"
#ifdef CONFIG_UI_ENABLE
#include "os/os_api.h"
#if (defined USE_LVGL_V8_UI_DEMO)
#include "lv_conf.h"
#endif

struct logo_dec dec_logo_handler;
#define __this 	(&dec_logo_handler)

/*
    获取logo显示状态
*/
int logo_get_state()
{
    return __this->logo_state;
}

static int logo_play_stop_ui(void)
{
#ifdef USE_LVGL_V8_UI_DEMO
    if (os_task_get_handle(LVGL_TASK_NAME)) {
        void lvgl_ui_suspend(void);
        lvgl_ui_suspend();
    }
#endif
    return 0;
}
/*
    停止视频解码服务 音频解码服务
*/
void logo_stop(void (*func)())
{
    if (__this->video_dec) {
        server_request(__this->video_dec, VIDEO_REQ_DEC_PLAY_PAUSE, &__this->video_req);
        server_request(__this->video_dec, VIDEO_REQ_DEC_STOP, &__this->video_req);
        server_close(__this->video_dec);
        __this->video_dec = NULL;
    }
    if (__this->audio_dec) {
        __this->audio_req.dec.cmd = AUDIO_DEC_STOP;
        server_request(__this->audio_dec, AUDIO_REQ_DEC, &__this->audio_req);
        server_close(__this->audio_dec);
    }
    if (__this->audio_req.dec.file) {
        fclose(__this->audio_req.dec.file);
        __this->audio_req.dec.file = NULL;
    }
    if (__this->video_req.dec.file) {
        fclose(__this->video_req.dec.file);
        __this->video_req.dec.file = NULL;
    }
    if (__this->audio_buf) {
        free(__this->audio_buf);
        __this->audio_buf = NULL;
    }
    if (__this->video_buf) {
        free(__this->video_buf);
        __this->video_buf = NULL;
    }
    __this->logo_state = LOGO_SHOW_END;
    if (func) {
        func();
    } else {
        printf("-------func is empty--------");
    }
}

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case VIDEO_DEC_EVENT_CURR_TIME:
        /*
         *解码中
         */
        int cur_time = argv[1];
        printf("VIDEO_DEC_EVENT_CURR_TIME: %d\n", cur_time);
        break;
    case VIDEO_DEC_EVENT_FIRST_FRAME:
        printf("VIDEO_DEC_EVENT_FIRST_FRAME\n");
        break;
    case VIDEO_DEC_EVENT_LAST_FRAME:
        printf("VIDEO_DEC_EVENT_LAST_FRAME\n");
        break;
    case VIDEO_DEC_EVENT_END:
        printf("VIDEO_DEC_EVENT_END\n");
        /*
         *解码结束
         */
        break;
    case VIDEO_DEC_EVENT_ERR:
        printf("VIDEO_DEC_EVENT_ERR\n");
        /*
         *解码出错，如果存储设备没有被拔出则播放前一个文件
         */
        break;
    }
}
/*
    logo显示
    logo_path 传入需要视频logo或图片logo路径，视频音频只支持解码pcm格式；
    audio_path 传入图片logo播放时的背景音乐；
    time_out 传入显示logo显示时长，当传入时间超过视频时长，显示时间等于视频时长，如果传入0，不暂停显示；
    当time_out为0时，需要手动关闭视频解码服务和音频解码服务，否则会造成下次使用解码服务失败。
*/

int logo_show(char *logo_path, char *audio_path, int time_out, void (*func)())
{
    int err;
    union audio_req req = {0};
    while (__this->logo_state == LOGO_SHOW_START) {
        /* 防止函数重入 */
        os_time_dly(1);
    }
    FILE *ret = fopen(logo_path, "r");
    if (ret == NULL) {
        printf("-------------------open file error !----------------");
        return -EFAULT;
    }
    memset(__this, 0, sizeof(struct logo_dec));
    __this->logo_state = LOGO_SHOW_START;
    /* 读取logo 文件类型 */
    int len = fget_name(ret, __this->fname, MAX_FILE_NAME_LEN);
    if (len) {
        if (!ASCII_StrCmpNoCase((char *)(__this->fname + len - 3), "JPG", 3)) {
            __this->file_type = LOGO_FILE_TYPE_JPG;
        } else {
            __this->file_type = LOGO_FILE_TYPE_MOVE;
        }
    }

    if (__this->file_type == LOGO_FILE_TYPE_MOVE) { /* 视频文件 */
        struct video_dec_arg arg = {0};
        __this->audio_buf = malloc(LOGO_AUDIO_DEC_BUF_SIZE);
        __this->video_buf = malloc(LOGO_VIDEO_DEC_BUF_SIZE);

        arg.dev_name = "video_dec";
        arg.audio_buf_size = LOGO_AUDIO_DEC_BUF_SIZE;
        arg.video_buf_size = LOGO_VIDEO_DEC_BUF_SIZE;
        arg.audio_buf = __this->audio_buf;
        arg.video_buf = __this->video_buf;
        /* arg.ram_dev.enable = 0; */

        __this->video_dec = server_open("video_dec_server", &arg);
        if (!__this->video_dec) {
            printf("error_video_dec");
            __this->logo_state = LOGO_SHOW_END;
            return -EFAULT;
        }

        __this->video_req.dec.file    = (FILE *)ret;
        __this->video_req.dec.fb      = "fb1";
        __this->video_req.dec.left 	  = 0;
        __this->video_req.dec.top     = 0;
        __this->video_req.dec.width   = LCD_W;
        __this->video_req.dec.height  = LCD_H;
        __this->video_req.dec.preview = 0;
        __this->video_req.dec.volume  = 100;
        __this->video_req.dec.pctl    = NULL;

        if (time_out == 0xffff) {
            __this->video_req.dec.dec_cyc  = 1;
        } else {

            logo_play_stop_ui(); //暂停当时UI显示
        }
        server_request(__this->video_dec, VIDEO_REQ_DEC_START, &__this->video_req);

        if (time_out > __this->video_req.dec.info.total_time) {
            time_out = __this->video_req.dec.info.total_time;
        }
        if (time_out <= 0) {
            logo_stop(func);
            return 0;
        }
        server_register_event_handler(__this->video_dec, NULL, dec_server_event_handler);
        if (__this->video_req.dec.dec_cyc == 0) {
            sys_timeout_add_to_task("sys_timer", func, logo_stop, time_out * 1000);
        }
    } else { /* 非视频文件 */
        FILE *voice_file = fopen(audio_path, "r"); /* 先读取logo 音频文件 */
        if (!voice_file) {
            printf("-------------error audio file not open-----------------");
        }
        __this->video_buf = malloc(LOGO_VIDEO_DEC_BUF_SIZE);
        struct video_dec_arg arg = {0};
        arg.dev_name = "video_dec";
        arg.video_buf_size = LOGO_VIDEO_DEC_BUF_SIZE;
        arg.video_buf = __this->video_buf;
        /* arg.ram_dev.enable = 0; */
        __this->video_dec = server_open("video_dec_server", &arg);
        if (!__this->video_dec) {
            printf("error_video_dec");
            /* return -EFAULT; */
        }
        if (voice_file) {
            __this->audio_dec = server_open("audio_server", "dec");
            if (!__this->audio_dec) {
                fclose(voice_file);
                printf("-------------audio server open error--------");
            } else {
                __this->audio_req.dec.cmd             = AUDIO_DEC_OPEN;
                __this->audio_req.dec.volume          = 100;
                __this->audio_req.dec.output_buf      = NULL;
                __this->audio_req.dec.output_buf_len  = 8 * 1024;
                __this->audio_req.dec.file            = voice_file;
                __this->audio_req.dec.channel         = 0;
                err = server_request(__this->audio_dec, AUDIO_REQ_DEC, &__this->audio_req);
                if (err) {
                    printf("--------audio server rquest error --------------------");
                }
            }
        }
        __this->video_req.dec.file = (FILE *)ret;
        /* 加载logo JPEG文件 */
        if (!__this->video_req.dec.file) {
            printf("error req.dec.file");
            server_close(__this->video_dec);
            __this->video_dec = NULL;
            __this->logo_state = LOGO_SHOW_END;
            return -ENOENT;
        }
        __this->video_req.dec.fb        = "fb1";
        __this->video_req.dec.left 	    = 0;
        __this->video_req.dec.top 	    = 0;
        __this->video_req.dec.width 	= LCD_W;
        __this->video_req.dec.height 	= LCD_H;
        __this->video_req.dec.preview   = 1;
        __this->video_req.dec.pctl      = NULL;

        /* logo_play_stop_ui(); //暂停当时UI显示 */
        if (__this->video_dec) {
            err = server_request(__this->video_dec, VIDEO_REQ_DEC_START, &__this->video_req);
            if (err) {
                printf("--------video server rquest error --------------------");
                __this->logo_state = LOGO_SHOW_END;
                return err;
            }
        }
        if (voice_file) {
            if (__this->audio_dec) {
                __this->audio_req.dec.cmd = AUDIO_DEC_START;
                err = server_request(__this->audio_dec, AUDIO_REQ_DEC, &__this->audio_req);
                if (err) {
                    printf("--------audio server rquest error --------------------");
                }
            }
        }
        if (time_out <= 0) {
            logo_stop(func);
            return 0;
        }
        sys_timeout_add_to_task("sys_timer", func, logo_stop, time_out * 1000);
    }
    return 0;
}
#endif
