#include "server/video_server.h"
#include "asm/osd.h"
#include "asm/isc.h"
#include "os/os_api.h"
#include "server/net_server.h"
#include "app_config.h"
#include "server/audio_dev.h"

#ifdef PRODUCT_TEST_ENABLE
#include "product_main.h"
#endif

#ifdef CONFIG_USR_VIDEO_ENABLE

#define LOG_TAG             "[USER_VIDEO_REC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


#ifdef CONFIG_XCIOT_ENABLE
void *user_video_handle;

void **get_user_video_handle(void)
{
    return &user_video_handle;
}
#endif

void *user_video_rec_open(const char *video_name, u8 id);
#define USER_VIDEO_PCM  			0x00000000
#define USER_VIDEO_JPEG 			0x00000002
#define USER_VIDEO_H264 			0x00000003 //暂时不支持

struct user_video_hdl {
    u8 state;
    u8 page_turning;
    u8 req_send;
    u8 *user_isc_buf;
    u8 *user_video_buf;
    /* u8 *user_audio_buf; */
    struct server *user_video_rec;
    u32(*video_rt_cb)(void *, u8 *, u32);
    void *cb_priv;
    u8 cur_video_name[10];
    struct lbuff_head *lbuf_handle;
    u8 task_kill;
    int task_pid;
};

struct video_lbuf_test_head {
    u32 len;
    u8 data[0];
};

static struct user_video_hdl *user_handle[2] = {0};

static const unsigned char user_osd_format_buf[] = "yyyy-nn-dd hh:mm:ss";

/*码率控制，根据具体分辨率设置*/
int user_video_rec_get_abr(u32 width)
{
    if (width <= 640) {
        return 2800;
    } else {
        return 4000;
    }
}

void set_video_rt_cb(void *p, u32(*cb)(void *, u8 *, u32), void *priv, u8 id)
{
    struct user_video_hdl *handle = (struct user_video_hdl *)p;

    handle->video_rt_cb = cb;
    handle->cb_priv = priv;
}

static void *xopen(const char *path, void *arg)
{
    return arg;
}

static int xsend(void *fd, char *data, unsigned int len, unsigned int flag)
{
    struct user_video_hdl *handler = (struct user_video_hdl *)fd;
    if (handler->video_rt_cb) {
        if (flag == USER_VIDEO_JPEG) {
            handler->video_rt_cb(handler->cb_priv, (u8 *)data + 8, len - 8);
        }
    }

    return len;
}

static void xclose(void *fd)
{
    log_info("xclose");
}

#ifdef CONFIG_AEC_ENC_ENABLE

/*aec module enable bit define*/
#define AEC_EN              BIT(0)
#define NLP_EN              BIT(1)
#define ANS_EN              BIT(2)

#define AEC_MODE_ADVANCE    (AEC_EN | NLP_EN | ANS_EN)
#define AEC_MODE_REDUCE     (NLP_EN | ANS_EN)
#define AEC_MODE_SIMPLEX    (ANS_EN)

#define AEC_DATA_TO_SD 0 //将mic/dac/aec数据写卡进行查看,3channel(mic,dac,aec)

#endif //CONFIG_AEC_ENC_ENABLE

void *user_video_rec_open(const char *video_name, u8 id)
{
    int err = 0;
    union video_req req = {0};
    char path[48];
    struct user_video_hdl *handle = NULL;
    if (user_handle[id]) {
        handle = user_handle[id];
    } else {
        handle = (struct user_video_hdl *)zalloc(sizeof(struct user_video_hdl));
    }

    if (!handle) {
        return NULL;
    }

    if (handle->state == true || handle->user_video_rec) {
        return handle;
    }

    if (!handle->user_video_buf) {
        handle->user_video_buf = malloc(CONFIG_USER_VIDEO_SBUF_SIZE);
        if (!handle->user_video_buf) {
            log_error("no mem");
            goto __exit;
        }
    }

    int major = 0;
    int mijor = 0;

    sscanf(video_name, "video%d.%d", &major, &mijor);
    memset(handle->cur_video_name, 0, sizeof(handle->cur_video_name));
    strcpy(handle->cur_video_name, video_name);
    if (major == 2) {
        major = 10;
        sprintf(video_name, "video%d.%d", major, mijor);
        log_info("Updated video_name: %s\n", video_name);
    }

    handle->user_video_rec = server_open("video_server", (void *)video_name);
    if (!handle->user_video_rec) {
        goto __exit;
    }

    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.channel = 1;
    req.rec.width   = CONFIG_USER_VIDEO_WIDTH;
    req.rec.src_w   = CONFIG_USER_VIDEO_WIDTH;
    req.rec.height  = CONFIG_USER_VIDEO_HEIGHT;
    req.rec.src_h   = CONFIG_USER_VIDEO_HEIGHT;
    req.rec.state   = VIDEO_STATE_START;
    req.rec.format  = USER_VIDEO_FMT_AVI;
    req.rec.quality = VIDEO_LOW_Q;//VIDEO_MID_Q;
    req.rec.fps     = CONFIG_USER_VIDEO_REC_FPS;
    req.rec.enable_dri = 0;
    req.rec.online = 1;
#ifdef CONFIG_VIDEO2_ENABLE
    req.rec.uvc_id  = uvc_host_online();
#endif

#ifdef PRODUCT_TEST_ENABLE
    req.rec.real_fps = is_product_mode() ? PRODUCT_UVC_FPS : 15;//帧率
#else
    req.rec.real_fps = 20;
#endif
    req.rec.cycle_time = 60;
    //需要音频：请写audio.sample_rate和audio.buf、audio.buf_len
    req.rec.audio.sample_rate = CONFIG_USER_VIDEO_REC_AUDIO_SAMPLE_RATE;//音频采样率
    req.rec.audio.channel = 1;
    req.rec.audio.volume = 100;//音频增益0-100
    req.rec.audio.buf = NULL;//音频BUFF
    req.rec.audio.buf_len = 0;//音频BUFF长度
    req.rec.audio.aud_interval_size = 1024;

    req.rec.abr_kbps = user_video_rec_get_abr(req.rec.width);//JPEG图片码率
    req.rec.buf = handle->user_video_buf;
    req.rec.buf_len = CONFIG_USER_VIDEO_SBUF_SIZE;

#ifdef CONFIG_OSD_ENABLE
    struct video_text_osd text_osd = {0};
    text_osd.font_w = OSD_DEFAULT_WIDTH;//必须16对齐
    text_osd.font_h = OSD_DEFAULT_HEIGHT;//必须16对齐
    text_osd.text_format = user_osd_format_buf;
    text_osd.x = (req.rec.width - text_osd.font_w * strlen(text_osd.text_format) + 15) / 16 * 16;
    text_osd.y = (req.rec.height - text_osd.font_h + 15) / 16 * 16;
    text_osd.osd_yuv = 0xe20095;
    req.rec.text_osd = &text_osd;
#endif

    sprintf(path, "usr://%s", CONFIG_USER_VIDEO_PATH);
    strcpy(req.rec.net_par.netpath, path);

    req.rec.target = VIDEO_TO_OUT;
    /* req.rec.target = VIDEO_TO_OUT | VIDEO_USE_STD; */
    req.rec.out.arg = handle;
    req.rec.out.open = xopen;
    req.rec.out.send = xsend;
    req.rec.out.close = xclose;

#if TCFG_AUDIO_ADC_ALL_CHANNEL_OPEN
    req.rec.audio.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL);
#endif

#ifdef CONFIG_AEC_ENC_ENABLE

#if AEC_DATA_TO_SD
    aec_data_to_sd_open();
#endif //AEC_DATA_TO_SD

    req.rec.audio.aec_enable = 1;
    struct aec_s_attr aec_param = {0};
    req.rec.audio.aec_attr = &aec_param;

    extern void get_cfg_file_aec_config(struct aec_s_attr * aec_param);
    get_cfg_file_aec_config(&aec_param);

    if (aec_param.EnableBit == 0) {
        req.rec.audio.aec_enable = 0;
        req.rec.audio.aec_attr = NULL;
    }
    if (aec_param.EnableBit != AEC_MODE_ADVANCE) {
        aec_param.output_way = 0;
    }

#if defined TCFG_AUDIO_ADC_ALL_CHANNEL_OPEN && defined CONFIG_AISP_LINEIN_ADC_CHANNEL && defined CONFIG_AEC_HW_REC_LINEIN_ENABLE
    if (req.rec.audio.aec_enable) {
        aec_param.output_way = 1;    //1:使用硬件回采 0:使用软件回采
        req.rec.audio.rec_linein_channel_bit_map = BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL); //设置linein的回采通道
        req.rec.audio.channel_bit_map |= BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);  //配置回采硬件通道
        if (CONFIG_AISP_LINEIN_ADC_CHANNEL < CONFIG_PHONE_CALL_ADC_CHANNEL) {
            req.rec.audio.ch_data_exchange = 1;   //如果回采通道使用的硬件channel比MIC通道使用的硬件channel靠前的话处理数据时需要交换一下顺序
        }
    }
#endif

    if (req.rec.audio.sample_rate == 16000) {
        aec_param.wideband = 1;
#ifdef CONFIG_AEC_USE_PLAY_MUSIC_ENABLE
        aec_param.hw_delay_offset = 640;
#else
        aec_param.hw_delay_offset = 30;
#endif
    } else {
        aec_param.wideband = 0;
        aec_param.hw_delay_offset = 75;
    }

#ifdef CONFIG_AEC_USE_PLAY_MUSIC_ENABLE
    if (aec_param.output_way == 0) {
        aec_param.dac_ref_sr = 48000; //aec软件回采dac参考采样率
    }
#endif

#endif//CONFIG_AEC_ENC_ENABLE

    err = server_request(handle->user_video_rec, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_error("user start rec err");
        goto __exit;
    }

#if defined CONFIG_AEC_ENC_ENABLE && !defined CONFIG_FPGA_ENABLE
    if (aec_param.output_way) {
#ifdef TCFG_AUDIO_ADC_ALL_CHANNEL_OPEN
        extern void adc_multiplex_set_gain(const char *source, u8 channel_bit_map, u8 gain);
        adc_multiplex_set_gain("linein", BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL), CONFIG_AISP_LINEIN_ADC_GAIN * 2);
#endif
    }
#endif

#ifdef CONFIG_XCIOT_ENABLE
    extern u32 xc_video_frame_callback(u8 type, void *data, u32 len);
    req.rec.packet_cb = xc_video_frame_callback;//stream_packet_cb;//注册数据包回调函数进行协议转发

    err = server_request(handle->user_video_rec, VIDEO_REQ_SET_PACKET_CALLBACK, &req);
    if (err != 0) {
        log_error("stream_packet_cb set err");//xsend设置了不需要再次设置回调
    }
#endif

    handle->state = true;

    log_info("user video rec open ok");

    user_handle[id] = handle;
    return handle;

__exit:
    if (handle->user_video_rec) {
        memset(&req, 0, sizeof(req));
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        server_request(handle->user_video_rec, VIDEO_REQ_REC, &req);
        server_close(handle->user_video_rec);
        handle->user_video_rec = NULL;
    }
    if (handle->user_video_buf) {
        free(handle->user_video_buf);
        handle->user_video_buf = NULL;
    }

    free(handle);
    user_handle[id] = NULL;

    return NULL;
}

int user_video_rec_stop(void *p)
{
    struct user_video_hdl *handle = (struct user_video_hdl *)p;
    int err;
    union video_req req = {0};

    if (handle->user_video_rec) {
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(handle->user_video_rec, VIDEO_REQ_REC, &req);
        if (err != 0) {
            log_error("stop rec err 0x%x", err);
        }
        server_close(handle->user_video_rec);

        handle->user_video_rec = NULL;
        handle->state = false;
        log_info("user video rec close ok");
    }

    return 0;
}

int user_video_rec_close(void *p, u8 id)
{
    struct user_video_hdl *handle = (struct user_video_hdl *)p;
    int err;
    union video_req req = {0};

    if (handle->user_video_rec) {
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(handle->user_video_rec, VIDEO_REQ_REC, &req);
        if (err != 0) {
            log_error("stop rec err 0x%x", err);
        }
        server_close(handle->user_video_rec);

        handle->user_video_rec = NULL;
        handle->state = false;
        log_info("user video rec close ok");
    }
    if (handle->user_video_buf) {
        free(handle->user_video_buf);
        handle->user_video_buf = NULL;
    }
#if (defined CONFIG_AEC_ENC_ENABLE) && (AEC_DATA_TO_SD)
    aec_data_to_sd_close();
#endif

    if (handle->task_pid != NULL) {
        handle->task_kill = 1;
        thread_kill(&handle->task_pid, KILL_WAIT);
        handle->task_kill = 0;
    }
    free(handle);
    user_handle[id] = NULL;

    return 0;
}

//demo
int user_video_rec_switch(u8 id)
{
    ASSERT(id == 0 || id == 1);

    struct user_video_hdl *handle = (struct user_video_hdl *)user_handle[id];
    int err;
    union video_req req = {0};
    if (!handle || !handle->user_video_rec) {
        log_error("user video rec haven't open.");
        return -1;
    }

    const char *video_name = NULL;
    video_name = strcmp(handle->cur_video_name, TCFG_SLAVE_UVC0_JPEG_DATA_SOURCE) == 0 ? TCFG_SLAVE_UVC1_JPEG_DATA_SOURCE : TCFG_SLAVE_UVC0_JPEG_DATA_SOURCE;

    user_video_rec_stop(handle);
    handle = user_video_rec_open(video_name, id);
    extern void set_uvc_handle_cb(void *handle, u8 id);
    set_uvc_handle_cb(handle, id);

    return 0;
}
//demo
void virtual_video_send_task(void *p)
{

    struct user_video_hdl *handle = (struct user_video_hdl *)p;
    int rlen = 0;
    int wlen = 0;
    int ret = 0;
    int len = 0;
    struct video_lbuf_test_head *wbuf = NULL;
    struct video_lbuf_test_head *rbuf = NULL;
    FILE *fd = NULL;
    while (1) {
        if (handle->task_kill) {
            if (fd != NULL) {
                fd = NULL;
            }
            fclose(fd);
            lbuf_clear(handle->lbuf_handle);
            return;
        }

        fd = fopen(CONFIG_ROOT_PATH"a.jpg", "r");

        if (!fd) {
            log_error("fopen faild");
            break;
        }

        len = flen(fd);
        if (lbuf_free_space(handle->lbuf_handle) > len) {
            wbuf = (struct video_lbuf_test_head *)lbuf_alloc(handle->lbuf_handle, len + sizeof(struct video_lbuf_test_head));  //lbuf内申请一块空间
            if (!wbuf) {
                return;
            }
            wlen =  fread(wbuf->data, 1, len, fd);
            fclose(fd);
            wbuf->len = wlen;
            lbuf_push(wbuf, BIT(0));
        }


        if (!lbuf_empty(handle->lbuf_handle)) {//查询LBUF内是否有数据帧
            rbuf = (struct video_lbuf_test_head *)lbuf_pop(handle->lbuf_handle, BIT(0));
            if (!rbuf) {
                return;
            }

            rlen = rbuf->len;
            if (handle->video_rt_cb) {
                handle->video_rt_cb(handle->cb_priv, (u8 *)rbuf + 4, rlen - 4);
            }

            if (lbuf_free(rbuf) == 0) { //释放lbuf通道0的数据块
                printf("lbuf free fail!!!");
            }

        }

    }
}

#if 0
//demo
void *user_video_rec_open(const char *video_name, u8 id)
{
    int err = 0;
    union video_req req = {0};
    struct user_video_hdl *handle = NULL;
    handle = (struct user_video_hdl *)zalloc(sizeof(struct user_video_hdl));

    if (!handle) {
        return NULL;
    }


    if (!handle->user_video_buf) {
        handle->user_video_buf = malloc(CONFIG_USER_VIDEO_SBUF_SIZE);
        if (!handle->user_video_buf) {
            log_error("no mem");
            return NULL;

        }
    }


    //lbuf初始化:
    handle->lbuf_handle = lbuf_init(handle->user_video_buf, CONFIG_USER_VIDEO_SBUF_SIZE, 4, sizeof(struct video_lbuf_test_head));

    thread_fork("virtual_video_send_task", 20, 1024, 32, &handle->task_pid, virtual_video_send_task, handle);
    return handle;
}
#endif

#endif
