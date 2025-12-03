#include "server/video_server.h"
#include "server/server_core.h"
#include "lcd_config.h"

#define LOG_TAG             "[USER_VIDEO_DISP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

static struct server *video_display[8];

#if defined CONFIG_USR_VIDEO_ENABLE && defined CONFIG_UI_ENABLE

int user_video_disp_start(int video_id, int sub_id, const struct video_window *win)
{
    union video_req req = {0};
    char dev_name[20];
    char fb_name[4];

    log_info("video_disp_start: %d, %d x %d", video_id, win->width, win->height);

    if (video_id == 2) {

        sprintf(dev_name, "video%d.%d", 10, sub_id);
    } else {

        sprintf(dev_name, "video%d.%d", video_id, sub_id);
    }

#ifdef CONFIG_VIDEO2_ENABLE
    int uvc_host_online(void);
    if (video_id == 2 && uvc_host_online() != sub_id) {
        return -EFAULT;
    }
#endif

    video_display[video_id] = server_open("video_server", (void *)dev_name);
    if (!video_display[video_id]) {
        log_error("open video_server: faild, video_id = %d", video_id);
        return -1;
    }

    sprintf(fb_name, "fb%d", video_id + 1);

    req.display.fb 		            = fb_name;
    req.display.left  	            = win->left;
    req.display.top 	            = win->top;
    req.display.width 	            = win->width;
    req.display.height 	            = win->height;
    req.display.border_left         = win->border_left;
    req.display.border_top          = win->border_top;
    req.display.border_right        = win->border_right;
    req.display.border_bottom       = win->border_bottom;
    req.display.mirror   	        = win->mirror;
    req.display.jaggy		        = 0;	// IMC 抗锯齿
    req.display.state               = VIDEO_STATE_START;
    req.display.pctl                = NULL;

    if (video_id != 2) {
        req.display.camera_config   = NULL;
        req.display.camera_type     = VIDEO_CAMERA_NORMAL;
    } else {
        req.display.uvc_id          = sub_id;
        req.display.camera_config   = NULL;
        req.display.camera_type     = VIDEO_CAMERA_UVC;
        req.display.src_w           = 640;
        req.display.src_h           = 480;
        //旋转参数配置:
        //0:不旋转,不镜像 (原图)
        //1:逆时针旋转90度,不镜像
        //2:逆时针旋转270度,不镜像
        //3:逆时针旋转90度后,再垂直镜像
        //4:逆时针旋转90度后,再水平镜像
        req.display.rotate          = 0; //usb后视频图像旋转显示
    }

    int err = server_request(video_display[video_id], VIDEO_REQ_DISPLAY, &req);
    if (err) {
        log_error("display req err = %d!!", err);
        server_close(video_display[video_id]);
        video_display[video_id] = NULL;
    }

    return err;
}

void user_video_disp_stop(int video_id)
{
    union video_req req = {0};

    if (video_display[video_id]) {
        req.display.state = VIDEO_STATE_STOP;
        server_request(video_display[video_id], VIDEO_REQ_DISPLAY, &req);
        server_close(video_display[video_id]);
        video_display[video_id] = NULL;
    }
}

void user_video_disp_switch(void)
{
    static u8 curr_display_id = 0xff;

    struct video_window win = {0};

    win.width = LCD_W;
    win.height = LCD_H;

    if (curr_display_id == 0xff) {
#ifdef USE_LVGL_V8_UI_DEMO
        void lvgl_ui_suspend(void);
        lvgl_ui_suspend();
#endif
    } else {
        user_video_disp_stop(curr_display_id);
    }

    if (++curr_display_id >= ARRAY_SIZE(video_display)) {
        curr_display_id = 0xff;
#ifdef USE_LVGL_V8_UI_DEMO
        void lvgl_ui_resume(void);
        lvgl_ui_resume();
#endif
    } else {
        user_video_disp_start(curr_display_id, 0, &win);
    }
}

#endif
