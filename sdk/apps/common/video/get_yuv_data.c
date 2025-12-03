#include "os/os_api.h"
#include "device/device.h"
#include "video/video.h"
#include "app_config.h"

#define LOG_TAG             "[YUV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

#ifdef CONFIG_VIDEO_ENABLE

typedef struct {
    int pid;
    void (*cb)(u8 *data, u32 size, int width, int height);
    char init;
} get_yuv_cfg_t;

static get_yuv_cfg_t __info;

static void get_yuv_task(void *priv)
{
    int ret;
    void *video_dev_fd = NULL;
    void *fb5 = NULL;
    struct fb_map_user map;

    //数据来源video，输出到fb2
    struct video_format f = {0};
    struct fb_var_screeninfo fb2_info = {0};
    const char *dev_name = TCFG_YUV_DATA_SOURCE;
    const char *fb_name = "fb5";//fb5 之后不是合成

    f.type  = VIDEO_BUF_TYPE_VIDEO_OVERLAY;
    /* f.src_width = TCFG_YUV_DATA_IN_WIDTH; */
    /* f.src_height = TCFG_YUV_DATA_IN_HEIGHT; */
    f.win.left = 0;
    f.win.top    = 0;
    f.win.width  = TCFG_YUV_DATA_OUT_WIDTH;
    f.win.height = TCFG_YUV_DATA_OUT_HEIGHT;
    f.win.border_left   = 0;
    f.win.border_top    = 0;
    f.win.border_right  = 0;
    f.win.border_bottom = 0;
    f.pixelformat = VIDEO_PIX_FMT_YUV420;
    f.private_data = (void *)fb_name;

    video_dev_fd = dev_open(dev_name, NULL);
    if (video_dev_fd == NULL) {
        log_error("%s test open device %s faild", fb_name, dev_name);
        return;
    }

    ret = dev_ioctl(video_dev_fd, VIDIOC_SET_FMT, (u32)&f);
    if (ret) {
        log_error("VIDIOC_SET_FMT faild");
        dev_close(video_dev_fd);
        return;
    }

    ret = dev_ioctl(video_dev_fd, VIDIOC_OVERLAY, 1);
    if (ret) {
        log_error("VIDIOC_OVERLAY faild");
        dev_close(video_dev_fd);
        return;
    }

    //打开显示通道
    fb5 = dev_open(f.private_data, (void *)FB_COLOR_FORMAT_YUV422);
    if (!fb5) {
        log_error(" ||| %s ,,, %d", __FUNCTION__, __LINE__);
        dev_close(video_dev_fd);
        return;
    }

    while (1) {
        if (thread_kill_req()) {
            break;
        }

        dev_ioctl(fb5, FBIOGET_FBUFFER_INUSED, (int)&map);
        if ((u32)map.baddr) {
            //需要使用YUV数据的任务的回调
            __info.cb(map.baddr, map.width * map.height * 3 / 2, map.width, map.height);
            //获取到才释放
            dev_ioctl(fb5, FBIOPUT_FBUFFER_INUSED, (int)&map);
        } else {
            //获取不到重试
            continue;
        }
    }

    dev_close(fb5);
    dev_ioctl(video_dev_fd, VIDIOC_OVERLAY, 0);
    dev_close(video_dev_fd);
}

void get_yuv_init(void (*cb)(u8 *data, u32 size, int width, int height))
{
    if (__info.init) {
        return;
    }
    __info.init = 1;
    __info.cb = cb;
#ifdef CONFIG_QR_CODE_NET_CFG
    extern u8 is_in_config_network_state(void);
    if (is_in_config_network_state()) {
        //二维码配网时才需要大堆栈
        thread_fork("get_yuv_task", 8, 5120, 0, &__info.pid, get_yuv_task, NULL);
    } else
#endif
    {
        thread_fork("get_yuv_task", 10, 1024, 0, &__info.pid, get_yuv_task, NULL);
    }
}

void get_yuv_uninit(void)
{
    if (!__info.init) {
        return;
    }
    thread_kill(&__info.pid, KILL_WAIT);
    __info.cb = NULL;
    __info.init = 0;
}

void set_get_yuv_data_cb(void (*cb)(u8 *data, u32 size, int width, int height))
{
    __info.cb = cb;
}

#endif
