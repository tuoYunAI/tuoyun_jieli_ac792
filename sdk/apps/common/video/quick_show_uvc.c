/******************************************************************************
* File:             quick_show_uvc.c
*
* Author:
* Created:          11/08/24
* Description:      用于主机快速枚举uvc出图
*****************************************************************************/

#include "app_config.h"
#include "event/device_event.h"
#include "fs/fs.h"
#include "system/init.h"

#ifdef CONFIG_VIDEO_ENABLE

#include "pipeline_core.h"


static pipe_core_t *pipe_core[2];

#if TCFG_USB_HOST_ENABLE
extern void usb_host_event_handler(struct device_event *event);
extern void otg_event_handler(struct device_event *event);
#endif
/**
 * @brief    uvc快速开流显示接口
 *
 * @param:   id: usb id号
 * @param:   x: 显示窗口x坐标  y: 显示窗口y坐标
 *           width: 显示窗口宽度 height: 显示窗口高度
 *
 * @return:  0: 成功  -1: 失败
 **/
int start_quick_uvc_disp(u8 id, u32 x, u32 y, u32 width, u32 height)
{
    pipe_filter_t *uvc_filter, *jpeg_dec_filter, *imc_filter, *rep_filter, *disp_filter;
    struct video_format uvc_fmt = {0};
    struct video_format f = {0};
    int err = 0;

    if (pipe_core[id]) {
        printf("%s   %d err", __func__, __LINE__);
        return -1;
    }

    pipe_core[id] = pipeline_init(NULL, NULL);
    if (!pipe_core[id]) {
        printf("%s   %d err", __func__, __LINE__);
        return -1;
    }

    char source_uvc[5];
    char dis_source[6];
    char fb_source[5];
    memset(source_uvc, 0, sizeof(source_uvc));
    memset(dis_source, 0, sizeof(source_uvc));
    memset(fb_source, 0, sizeof(fb_source));
    sprintf(source_uvc, "uvc%d", id);
    sprintf(fb_source, "fb%d", id + 3);
    sprintf(dis_source, "disp%d", id + 3);
    //UVC
    pipe_core[id]->channel = plugin_source_to_channel(source_uvc);
    uvc_filter = pipeline_filter_add(pipe_core[id], source_uvc);
    if (!uvc_filter) {
        return -1;
    }
    //JPEG
    pipeline_param_get(pipe_core[id], uvc_filter, PIPELINE_GET_FORMAT, (int)&uvc_fmt);
    if (uvc_fmt.pixelformat & VIDEO_PIX_FMT_JPEG) {
        jpeg_dec_filter = pipeline_filter_add(pipe_core[id], plugin_factory_find("jpeg_dec"));
        if (!jpeg_dec_filter) {
            return -1;
        }
    }
    //DISP
    disp_filter = pipeline_filter_add(pipe_core[id], dis_source);

    rep_filter = pipeline_filter_add(pipe_core[id], plugin_factory_find("rep"));
    imc_filter = pipeline_filter_add(pipe_core[id], plugin_factory_find("imc"));

    f.uvc_id      = id;
    f.camera_type   = VIDEO_CAMERA_UVC;
    f.type = VIDEO_BUF_TYPE_VIDEO_PLAY;
    f.pixelformat = VIDEO_PIX_FMT_JPEG | VIDEO_PIX_FMT_YUYV;
    f.private_data = fb_source;
    f.win.left 	 = x;
    f.win.top  	 = y;
    f.win.width = width;
    f.win.height = height;
    f.win.combine = 1; //合成显示
    pipeline_param_set(pipe_core[id], NULL, PIPELINE_SET_FORMAT, &f);
    /* pipeline_param_set(pipe_core[id], NULL, PIPELINE_SET_EXTERN_BUFFER, (int)&device->video_q); */
    if (uvc_fmt.pixelformat & VIDEO_PIX_FMT_JPEG) {
        int line_cnt = 16;
        if ((!(uvc_fmt.pixelformat & VIDEO_PIX_FMT_YUV420)) && uvc_fmt.src_width >= 1280) {
            line_cnt = 8;
        }
        pipeline_param_set(pipe_core[id], NULL, PIPELINE_SET_BUFFER_LINE, (int)&line_cnt);
        pipeline_filter_link(uvc_filter, jpeg_dec_filter);
        pipeline_filter_link(jpeg_dec_filter, rep_filter);
    } else {
        //yuyv
        pipeline_filter_link(uvc_filter, rep_filter);
    }
    pipeline_filter_link(rep_filter, imc_filter);
    pipeline_filter_link(imc_filter, disp_filter);


    pipeline_prepare(pipe_core[id]);
    pipeline_start(pipe_core[id]);

    return 0;
}

/**
 * @brief  uvc快速关闭显示接口
 *
 * @param: id : uvc id号
 *
 * @return:  0: 成功  -1: 失败
 **/
int stop_quick_uvc_disp(u8 id)
{
    if (pipe_core[id]) {
        pipeline_stop(pipe_core[id]);
        pipeline_reset(pipe_core[id]);
        pipeline_uninit(pipe_core[id]);
        pipe_core[id] = NULL;
    }

    return 0;
}

#endif /* CONFIG_VIDEO_ENABLE */
