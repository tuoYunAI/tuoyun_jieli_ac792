
#include "system/includes.h"
#include "pipeline_core.h"
#include "asm/jpeg_codec.h"
#include "app_config.h"
#include "server/video_server.h"//app_struct
#include "server/video_dec_server.h"//dec_struct

#ifdef CONFIG_VIDEO_ENABLE

struct _jpeg_disp_f_t {
    struct server *dec_server;
};
//base
struct _jpeg_disp_f_t jpeg_disp_fh_base = {0};
#define __base   (&jpeg_disp_fh_base)

//top
struct _jpeg_disp_f_t jpeg_disp_fh_top = {0};
#define __top   (&jpeg_disp_fh_top)


#define JPG_MAX_SIZE 100*1024

#define JPG_DISP_TASK_BASE_NAME "jpeg_disp_task_base"
#define JPG_DISP_TASK_TOP_NAME  "jpeg_disp_task_top"


static OS_MUTEX jpeg_disp_mutex;

/*显示层级关系
 *  top
 *  ...
 *  fb4--------JPEG_DISP_LEVEL_4
 *  fb3--------JPEG_DISP_LEVEL_3
 *  lvgl-ui(fb0)
 *  fb1--------JPEG_DISP_LEVEL_1
 *  ...
 *  base
*/
enum {
    JPEG_DISP_LEVEL_1 = 1,
    JPEG_DISP_LEVEL_3 = 3,
    JPEG_DISP_LEVEL_4 = 4,
};

enum {
    JPEG_MSG_DISP,
    JPEG_MSG_INIT,
    JPEG_MSG_UNINIT
};

static const char *fd_imgs[10] = {
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_01.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_02.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_03.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_04.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_05.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_06.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_07.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_08.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_09.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/fd_10.jpg"
};

static const char *sx_imgs[11] = {
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_01.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_02.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_03.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_04.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_05.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_06.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_07.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_08.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_09.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_10.jpg",
    "mnt/sdfile/EXT_RESERVED/uipackres/ui/sx_11.jpg"
};

//base disp
static OS_SEM base_jpeg_disp_sem;
static uint8_t base_running = 0;
static char **base_current_jpegs;
static uint8_t base_jpegs_num = 0;
static uint8_t base_jpegs_period = 0;

//top disp
static OS_SEM top_jpeg_disp_sem;
static uint8_t top_running = 0;
static char **top_current_jpegs;
static uint8_t top_jpegs_num = 0;
static uint8_t top_jpegs_period = 0;



int jpeg_disp_one_frame(u16 left, u16 top, u16 width, u16 height, u8 *buf, int len, uint8_t disp_level)
{

    union video_dec_req dec_req = {0};

    switch (disp_level) {
    case 1:
        dec_req.dec.fb = "fb1";//最底层
        break;
    case 3:
        dec_req.dec.fb = "fb3";//次顶层
        break;
    case 4:
        dec_req.dec.fb = "fb4";//最顶层
        break;
    default:
        dec_req.dec.fb = "fb1";//最底层
        break;
    }

    dec_req.dec.left = left;//显示位置
    dec_req.dec.top = top;
    dec_req.dec.width = width;//显示宽高
    dec_req.dec.height = height;
    dec_req.dec.thm_first = 0;
    dec_req.dec.pctl = NULL;
    dec_req.dec.preview = 0;
    dec_req.dec.image.buf = buf;
    dec_req.dec.image.size = len;


    if (disp_level == JPEG_DISP_LEVEL_4) {
        return server_request(__top->dec_server, VIDEO_REQ_DEC_IMAGE, &dec_req);
    } else {
        return server_request(__base->dec_server, VIDEO_REQ_DEC_IMAGE, &dec_req);
    }

}

int jpeg_disp_frame_init(uint8_t disp_level)
{
    os_mutex_pend(&jpeg_disp_mutex, 0);

    if (disp_level == JPEG_DISP_LEVEL_4) {
        memset(__top, 0x00, sizeof(struct _jpeg_disp_f_t));
        if (!__top->dec_server) {
            struct video_dec_arg arg = {0};
            arg.dev_name = "video_dec";
            arg.video_buf_size = JPG_MAX_SIZE;
            __top->dec_server = server_open("video_dec_server", &arg);
            if (!__top->dec_server) {
                printf("open video_dec_server fail");
                os_mutex_post(&jpeg_disp_mutex);
                return -EFAULT;
            }
        }
    } else {
        memset(__base, 0x00, sizeof(struct _jpeg_disp_f_t));
        if (!__base->dec_server) {
            struct video_dec_arg arg = {0};
            arg.dev_name = "video_dec";
            arg.video_buf_size = JPG_MAX_SIZE;
            __base->dec_server = server_open("video_dec_server", &arg);
            if (!__base->dec_server) {
                printf("open video_dec_server fail");
                os_mutex_post(&jpeg_disp_mutex);
                return -EFAULT;
            }
        }
    }

    os_mutex_post(&jpeg_disp_mutex);

    return 0;
}

int jpeg_disp_frame_uninit(uint8_t disp_level)
{

    os_mutex_pend(&jpeg_disp_mutex, 0);
    union video_dec_req dec_req = {0};


    if (disp_level == JPEG_DISP_LEVEL_4) {
        server_request(__top->dec_server, VIDEO_REQ_DEC_STOP, &dec_req);
        server_close(__top->dec_server);
        __top->dec_server = NULL;
    } else {
        server_request(__base->dec_server, VIDEO_REQ_DEC_STOP, &dec_req);
        server_close(__base->dec_server);
        __base->dec_server = NULL;
    }

    os_mutex_post(&jpeg_disp_mutex);

    return 0;
}


void jpeg_disp_test(void *p)
{
    printf("\n[ debug ]--func=%s line=%d\n", __func__, __LINE__);
    static volatile u8 state = 0;
    u16 disp_width = 800;
    u16 disp_height = 480;
    u16 disp_left = 0;
    u16 disp_top = 0;
    u8 disp_level = JPEG_DISP_LEVEL_1;

    if (state == 0) {
        jpeg_disp_frame_init(disp_level);
        FILE *profile_fp = fopen("mnt/sdfile/EXT_RESERVED/logopackres/logo/poweron.jpg", "r");
        if (profile_fp == NULL) {
            return;
        }
        u32 jpg_len = flen(profile_fp);
        struct vfs_attr file_attr;
        fget_attrs(profile_fp, &file_attr);
        u8 *jpg_buffer = file_attr.sclust;
        fclose(profile_fp);
        jpeg_disp_one_frame(disp_left, disp_top, disp_width, disp_height, jpg_buffer, jpg_len, disp_level);
    } else {
        jpeg_disp_frame_uninit(disp_level);
    }
    state = !state;

}



int jpeg_disp_sem_post(uint8_t disp_level)
{
    int err;
    if (disp_level == JPEG_DISP_LEVEL_4) {
        err = os_taskq_post(JPG_DISP_TASK_TOP_NAME, 1, JPEG_MSG_DISP);
    } else {
        err = os_taskq_post(JPG_DISP_TASK_BASE_NAME, 1, JPEG_MSG_DISP);
    }
    if (err) {
        printf("jpeg_disp_sem_post err = %d\n", err);
        return err;
    }

}

int jpeg_disp_frame_init_post(uint8_t disp_level)
{

    int err;
    if (disp_level == JPEG_DISP_LEVEL_4) {
        err = os_taskq_post(JPG_DISP_TASK_TOP_NAME, 1, JPEG_MSG_INIT);
    } else {
        err = os_taskq_post(JPG_DISP_TASK_BASE_NAME, 1, JPEG_MSG_INIT);
    }
    if (err) {
        printf("jpeg_disp_sem_post err = %d\n", err);
        return err;
    }
}

int jpeg_disp_frame_uninit_post(uint8_t disp_level)
{

    int err;
    if (disp_level == JPEG_DISP_LEVEL_4) {
        err = os_taskq_post(JPG_DISP_TASK_TOP_NAME, 1, JPEG_MSG_UNINIT);
    } else {
        err = os_taskq_post(JPG_DISP_TASK_BASE_NAME, 1, JPEG_MSG_UNINIT);
    }
    if (err) {
        printf("jpeg_disp_sem_post err = %d\n", err);
        return err;
    }

}



void jpeg_disp_set_src_and_play(const void *src[], uint8_t num, uint8_t period, uint8_t disp_level)
{
    if (disp_level == JPEG_DISP_LEVEL_4) {
        top_current_jpegs = src;
        top_jpegs_num = num;
        top_jpegs_period = period;
        jpeg_disp_sem_post(disp_level);
    } else {
        base_current_jpegs = src;
        base_jpegs_num = num;
        base_jpegs_period = period;
        jpeg_disp_sem_post(disp_level);
    }
}


int jpeg_disp_sem_pend(int timeout, uint8_t disp_level)
{
    if (disp_level == JPEG_DISP_LEVEL_4) {
        return os_sem_pend(&top_jpeg_disp_sem, timeout);
    } else {
        return os_sem_pend(&base_jpeg_disp_sem, timeout);
    }
}


void jpeg_disp_task_base_task(void *arg)
{

    //显示位置设置
    u16 disp_width = 800;
    u16 disp_height = 480;
    u16 disp_left = 0;
    u16 disp_top = 0;

    //显示层级位置设置
    u8 disp_level = JPEG_DISP_LEVEL_1;

    int res;
    int msg[16];

    jpeg_disp_frame_init(disp_level);

    while (1) {
        // 等待通知
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));

        if (res != OS_TASKQ) {
            printf("jpeg_disp_task_base err=%d %d\n", res, msg[0]);
            continue;
        }


        switch (msg[1]) {
        case JPEG_MSG_DISP:
            base_running = 1;
            // 模拟连续播放
            for (int i = 0; i < base_jpegs_num && base_running; i++) {

                /*printf("base_current_jpegs[%d] == %s\n",i,base_current_jpegs[i]);*/

                FILE *profile_fp = fopen(base_current_jpegs[i], "r");
                if (profile_fp == NULL) {
                    printf("fopen error!!!!\n");
                    continue;
                }
                u32 jpg_len = flen(profile_fp);
                struct vfs_attr file_attr;
                fget_attrs(profile_fp, &file_attr);
                u8 *jpg_buffer = file_attr.sclust;
                fclose(profile_fp);

                jpeg_disp_one_frame(disp_left, disp_top, disp_width, disp_height, jpg_buffer, jpg_len, disp_level);
                mdelay(base_jpegs_period);//播放间隔
            }
            base_running = 0;
            break;
        case JPEG_MSG_INIT:
            jpeg_disp_frame_init(disp_level);
            break;
        case JPEG_MSG_UNINIT:
            jpeg_disp_frame_uninit(disp_level);
            break;
        }

    }//while

    jpeg_disp_frame_uninit(disp_level);


    os_mutex_del(&jpeg_disp_mutex, OS_DEL_ALWAYS);
    os_sem_del(&base_jpeg_disp_sem, 0);

    return;
}


void jpeg_disp_task_top_task(void *arg)
{

    //显示位置设置
    u16 disp_width = 400;
    u16 disp_height = 480;
    u16 disp_left = 400;
    u16 disp_top = 0;

    //显示层级位置设置
    u8 disp_level = JPEG_DISP_LEVEL_4;

    jpeg_disp_frame_init(disp_level);

    int res;
    int msg[16];

    while (1) {

        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));

        if (res != OS_TASKQ) {
            printf("jpeg_disp_task_top err=%d %d\n", res, msg[0]);
            continue;
        }

        switch (msg[1]) {
        case JPEG_MSG_DISP:
            top_running = 1;
            // 模拟连续播放
            for (int i = 0; i < top_jpegs_num && top_running; i++) {
                FILE *profile_fp = fopen(top_current_jpegs[i], "r");
                if (profile_fp == NULL) {
                    printf("fopen error!!!!\n");
                    continue;
                }
                u32 jpg_len = flen(profile_fp);
                struct vfs_attr file_attr;
                fget_attrs(profile_fp, &file_attr);
                u8 *jpg_buffer = file_attr.sclust;
                fclose(profile_fp);
                jpeg_disp_one_frame(disp_left, disp_top, disp_width, disp_height, jpg_buffer, jpg_len, disp_level);
                mdelay(top_jpegs_period);//播放间隔
            }

            printf("播放结束\n");
            top_running = 0;
            break;
        case JPEG_MSG_INIT:
            jpeg_disp_frame_init(disp_level);
            break;
        case JPEG_MSG_UNINIT:
            jpeg_disp_frame_uninit(disp_level);
            break;
        }

    }//while


    jpeg_disp_frame_uninit(disp_level);

    os_mutex_del(&jpeg_disp_mutex, OS_DEL_ALWAYS);
    os_sem_del(&top_jpeg_disp_sem, 0);

    return;
}


void jpeg_disp_task_init(void)
{

    printf("[%s]", __func__);
    os_sem_create(&base_jpeg_disp_sem, 0);
    os_sem_create(&top_jpeg_disp_sem, 0);

    os_mutex_create(&jpeg_disp_mutex);


    thread_fork(JPG_DISP_TASK_BASE_NAME, 25, 1024, 128, NULL, jpeg_disp_task_base_task, NULL);
    thread_fork(JPG_DISP_TASK_TOP_NAME, 25, 1024, 128, NULL, jpeg_disp_task_top_task, NULL);
}

static void timer_cb(void *p)
{
    static int swatch_flag = 0;
    if (swatch_flag == 0) {
        printf("+++disp base\n");
        jpeg_disp_set_src_and_play(sx_imgs, 11, 10, JPEG_DISP_LEVEL_1); //显示底层
        swatch_flag = 1;
    } else if (swatch_flag == 1) {
        printf("+++disp top\n");
        jpeg_disp_set_src_and_play(fd_imgs, 10, 10, JPEG_DISP_LEVEL_4); //显示顶层
        jpeg_disp_set_src_and_play(sx_imgs, 11, 10, JPEG_DISP_LEVEL_1); //显示底层
        swatch_flag = 2;
    } else {
        printf("+++undisp top\n");
        jpeg_disp_frame_uninit_post(JPEG_DISP_LEVEL_4);//关闭顶层
        jpeg_disp_frame_init_post(JPEG_DISP_LEVEL_4);//开启顶层
        swatch_flag = 0;
    }
}

//play demo
//演示了两个图层同时显示jpeg帧
//注意:jpeg资源需要自行添加,demo仅作演示

void jpeg_disp_frames_test(void)
{
    jpeg_disp_task_init();
    sys_timer_add(NULL, timer_cb, 2000);
}

#endif
