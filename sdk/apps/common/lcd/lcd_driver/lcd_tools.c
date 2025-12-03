#include "app_config.h"
#include "lcd_tools.h"

#ifdef CONFIG_VIDEO_IQ_TOOLS_ENABLE
#ifdef CONFIG_UI_ENABLE
#include "os/os_api.h"
#if (defined USE_LVGL_V8_UI_DEMO)
#include "lv_conf.h"
#endif
#endif

static struct lcd_tools_hdl_type {
    u8 *recv_buf;
    u8 *resp_buf;
    u8 enter_flag;
    OS_MUTEX rsp_mutex, list_mutex;
    struct list_head msg_list;

    struct server *display;
    struct server *photo;

} *__THIS = NULL;

static OS_SEM cdc_sem;
static u8 lcd_usb_id;
static int lcd_tools_event_handler(struct sys_event *e)
{
    struct device_event *event = (struct device_event *)e->payload;
    const char *usb_msg = (const char *)event->value;
    if (e->from == DEVICE_EVENT_FROM_OTG) {
        if (usb_msg[0] == 's') {
            if (event->event == DEVICE_EVENT_IN) {
                lcd_tools_info("lcd tools usb %c online", usb_msg[2]);
                lcd_usb_id = usb_msg[2] - '0';
            } else if (event->event == DEVICE_EVENT_OUT) {
                lcd_tools_info("lcd tools usb %c offline", usb_msg[2]);
            }
        }
    }
    return 0;
}

SYS_EVENT_STATIC_HANDLER_REGISTER(lcd_tools_event, 0) = {
    .event_type     = SYS_DEVICE_EVENT,
    .prob_handler   = lcd_tools_event_handler,
    .post_handler   = NULL,
};


static int lcd_tools_stop_ui(void)
{
#ifdef USE_LVGL_V8_UI_DEMO
    if (os_task_get_handle(LVGL_TASK_NAME)) {
        void lvgl_ui_suspend(void);
        lvgl_ui_suspend();
    }
#endif
    return 0;
}
static int lcd_tools_start_ui(void)
{
#ifdef USE_LVGL_V8_UI_DEMO
    if (os_task_get_handle(LVGL_TASK_NAME)) {
        void lvgl_ui_resume(void);
        lvgl_ui_resume();
    }
#endif
    return 0;
}


u8 lcd_tools_cdc_post(void)
{
    if (__THIS->enter_flag) {
        os_sem_post(&cdc_sem);
    }
    return __THIS->enter_flag;
}

static u32 lcd_tools_dev_read(u8 *data, u32 size)
{
    os_sem_pend(&cdc_sem, 0);
    return cdc_read_data(lcd_usb_id, data, size);
}


static u32 lcd_tools_dev_write(u8 *data, u32 size)
{
    return cdc_write_data(lcd_usb_id, data, size);
}

static const gpcrc_algorithm_cfg_t crc_algorithm[] = {
    /* crc_name             init        poly            rev_in          rev_out           pol_size   xorout */
    /* {"CRC-7-MMC",           0x00,       0x09,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL7,   0x00      }, */
    /* {"CRC-8",               0x00,       0x07,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL8,   0x00      }, */
    /* {"CRC-8-ITU",           0x00,       0x07,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL8,   0x55      }, */
    /* {"CRC-8-ROHC",          0xFF,       0x07,       GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL8,   0x00      }, */
    /* {"CRC-8-MAXIM",         0x00,       0x31,       GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL8,   0x00      }, */
    /* {"CRC-16-IBM",          0x0000,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-MAXIM",        0x0000,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    /* {"CRC-16-USB",          0xFFFF,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    {"CRC-16-MODBUS",       0xFFFF,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    },
    /* {"CRC-16-CCITT",        0x0000,     0x1021,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-CCITT-FALSE",  0xFFFF,     0x1021,     GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-X25",          0xFFFF,     0x1021,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    /* {"CRC-16-XMODEM",       0x0000,     0x1021,     GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-XMODEM2",      0x0000,     0x8408,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-DNP",          0x0000,     0x3D65,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    /* {"CRC-32",              0xFFFFFFFF, 0x04C11DB7, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF}, */
    /* {"CRC-32-C",            0xFFFFFFFF, 0x1EDC6F41, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF}, */
    /* {"CRC-32-KOOPMAN",      0xFFFFFFFF, 0x741B8CD7, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF}, */
    /* {"CRC-32-MPEG-2",       0xFFFFFFFF, 0x04C11DB7, GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL32,  0x00000000}, */
};

static void display_stop()
{
    union video_dec_req dec_req = {0};
    union video_req req = {0};

    if (__THIS) {
        if (__THIS->display) {
            req.display.state 	= VIDEO_STATE_STOP;
            server_request(__THIS->display, VIDEO_REQ_DISPLAY, &req);
            server_close(__THIS->display);
            __THIS->display = NULL;
        }

        if (__THIS->photo) {
            server_request(__THIS->photo, VIDEO_REQ_DEC_STOP, &dec_req);
            server_close(__THIS->photo);
            __THIS->photo = NULL;
        }
    }
}

static void display_photo(u8 *p_buf, u32 image_size)
{

    int err = 0;
    union video_dec_req dec_req;

    memset(&dec_req, 0x0, sizeof(dec_req));

    struct video_dec_arg arg = {0};
    arg.dev_name = "video_dec";
    arg.video_buf_size = 100 * 1024;
    __THIS->photo = server_open("video_dec_server", &arg);
    if (!__THIS->photo) {
        log_e("open video_dec_server fail");
        return;
    }

    lcd_tools_stop_ui();

    dec_req.dec.fb = "fb1";
    dec_req.dec.left = 0;
    dec_req.dec.top = 0;
    dec_req.dec.width = LCD_W;
    dec_req.dec.height = LCD_H;
    dec_req.dec.preview = 1;
    dec_req.dec.image.buf = p_buf;
    dec_req.dec.image.size = image_size;
    err = server_request(__THIS->photo, VIDEO_REQ_DEC_IMAGE, &dec_req);
    if (err) {
        log_e("request video_dec_server fail");
        return;
    }
}

static void video_display()
{
    union video_req req = {0};

    __THIS->display = server_open("video_server", "video0.0");
    if (!__THIS->display) {
        printf("video server open error!");
        return;
    }

    lcd_tools_stop_ui();
    req.display.fb 		        = "fb1";
    req.display.left  	        = 0;
    req.display.top 	        = 0;
    req.display.width 	        = LCD_W;
    req.display.height 	        = LCD_H;
    req.display.border_left     = 0;
    req.display.border_top      = 0;
    req.display.border_right    = 0;
    req.display.border_bottom   = 0;
    req.display.win_type        = 0;
    req.display.src_crop_enable = 0;
    req.display.camera_config   = NULL;
    req.display.camera_type     = 0;
    req.display.mirror          = 0;  //1: 显示镜像
    req.display.state 	        = VIDEO_STATE_START;
    req.display.pctl            = NULL;
    int err = server_request(__THIS->display, VIDEO_REQ_DISPLAY, &req);
    if (err) {
        printf("video server request error!");
        return;
    }
}

static void data_respond(u32 idx, u8 type, u8 *data, u32 len)
{
    u32 crc_len;
    u16 *crc_value;
    /* const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[8]; */
    const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[0];

    struct display_head *head = (struct display_head *)__THIS->resp_buf;

    os_mutex_pend(&__THIS->rsp_mutex, 0);

    memset(__THIS->resp_buf, 0, RESP_BUFFER_SIZE);
    head->mark[0] = 'J';
    head->mark[1] = 'L';
    head->type    = type;
    head->idx     = idx;

    head->len = (len + DATA_BUFFER_ALIGN - 1) / DATA_BUFFER_ALIGN * DATA_BUFFER_ALIGN;
    memcpy(__THIS->resp_buf + sizeof(struct display_head), data, len);

    crc_len = sizeof(struct display_head) + head->len;
    crc_value = (u16 *)(__THIS->resp_buf + crc_len);
    *crc_value = gpcrc_calc_u8_buf((gpcrc_algorithm_cfg_t *)algo_cfg, head, crc_len);

    if (lcd_tools_dev_write(__THIS->resp_buf, crc_len + sizeof(u16)) != crc_len + sizeof(u16)) {
        printf("%s error\n", __func__);
    }

    os_mutex_post(&__THIS->rsp_mutex);
}


static void msg_handle_task(void *priv)
{
    u32 len;
    struct display_msg *msg, *n;
    json_object *new_obj, *opcode_obj, *rscorr_obj, *params_obj;
    u8 *str = NULL, *_str = NULL, opcode, rscorr = 0;
    u8 rscorr_str[4] = {0};
    dmm_ccmf_param_t ccm = {0};
    dmm_gamma_param_t gamma = {0};
    json_object *ccm_a;
    json_object *gamma_a;
    int array_len;
    float r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle;
    int mode, rbs, line_type;
    dmm_param_t default_dmm;

    for (int i = 0; i < 256; i++) {
        default_dmm.rgb_gamma_param.lut[i] = i;
    }

    float cmm_param[12] = {0.999, 0.0004, 0.001, 0, -0.00185, 1.00453, -0.00228, 0, -0.00182, -0.00218, 1.0044, 0};

    for (int i = 0; i < 12; i++) {
        default_dmm.ccmf_param.matrix[i] = cmm_param[i];
    }

    dmm_blend_param_t default_param = {
        .r_gain = 1.0,
        .g_gain = 1.0,
        .b_gain = 1.0,
        .bright_gain = 1.0,
        .contrast_gain = 1.0,
        .mean = 128.0,
        .saturation_gain = 1.0,
        .angle = 0,
        .mode = 0,
    };

    r_gain = default_param.r_gain;
    g_gain = default_param.g_gain;
    b_gain = default_param.b_gain;
    bright_gain = default_param.bright_gain;
    contrast_gain = default_param.contrast_gain;
    saturation_gain = default_param.saturation_gain;
    mean = default_param.mean;
    mode = default_param.mode;
    angle = default_param.angle;

    for (;;) {
        os_mutex_pend(&__THIS->list_mutex, 0);
        list_for_each_entry_safe(msg, n, &__THIS->msg_list, entry) {
            lcd_tools_info("--->recv cjson\n\n%s\n\n", msg->data);
            if (!(new_obj = json_tokener_parse(msg->data))) {
                goto _exit_list_;
            }

            opcode_obj = json_object_object_get(new_obj, "opcode");
            rscorr_obj = json_object_object_get(new_obj, "rscorr");
            params_obj = json_object_object_get(new_obj, "params");

            if (!opcode_obj || !rscorr_obj || !params_obj) {
                goto _exit_list_;
            }

            str = NULL;
            opcode = json_object_get_int(opcode_obj);

            switch (opcode) {
            case IMD_CUS_CONNECT_DEV: //连接设备
                lcd_tools_info("IMD_CUS_CONNECT_DEV\n");
                asprintf(&str, "{\"opcode\":\"%d\",\"rscorr\":\"%d\",\"params\":{}}", IMD_CUS_CONNECT_DEV, 0);
                break;

            case IMD_CUS_DISCONNECT_DEV: //断开连接
                lcd_tools_info("IMD_CUS_DISCONNECT_DEV\n");
                lcd_tools_start_ui();
                display_stop();
                break;

            case IMD_CUS_SHOW_VIDEO_IMAGE: //显示摄像头
                lcd_tools_info("IMD_CUS_SHOW_VIDEO_IMAGE\n");
                display_stop();
                video_display();
                break;

            case IMD_CUS_GET_DEFAULT_PARAM://获取设备默认参数
                lcd_tools_info("IMD_CUS_GET_DEFAULT_PARAM\n");
//                dmm_default_blend_param_get(&default_param);
//                dmm_param_get(&default_dmm);
                r_gain = default_param.r_gain;
                g_gain = default_param.g_gain;
                b_gain = default_param.b_gain;
                bright_gain = default_param.bright_gain;
                contrast_gain = default_param.contrast_gain;
                saturation_gain = default_param.saturation_gain;
                mean = default_param.mean;
                mode = default_param.mode;
                angle = default_param.angle;
                rbs = 0;
                line_type = 3;
                asprintf(&str, "{\"opcode\":\"%d\",\"params\":{\"r_gain\":\"%f\",\"g_gain\":\"%f\",\
                         \"b_gain\":\"%f\",\"bright_gain\":\"%f\",\"contrast_gain\":\"%f\",\"saturation_gain\":\"%f\",\
                         \"mean\":\"%f\",\"angle\":\"%f\",\"mode\":\"%d\",\"rbs\":\"%d\",\"line_type\":\"%d\"},\"rscorr\":\"%d\"}", \
                         IMD_CUS_GET_DEFAULT_PARAM, r_gain, g_gain, b_gain, bright_gain, contrast_gain, \
                         saturation_gain, mean, angle, mode, rbs, line_type, rscorr);

                json_object *root = json_tokener_parse(str);
                if (!root) {
                    goto _exit_list_;
                }

                json_object *params_obj_ = json_object_object_get(root, "params");

                json_object *gamma_array = json_object_new_array();
                for (int i = 0; i < 256; i++) {
                    char buffer[32];
                    gamma.lut[i] = i;
                    snprintf(buffer, sizeof(buffer), "%d", i);
                    json_object_array_add(gamma_array, json_object_new_string(buffer));
                }
                json_object_object_add(params_obj_, "gamma", gamma_array);

                json_object *ccm_array = json_object_new_array();
                for (int i = 0; i < 12; i++) {
                    char buffer[32];
                    ccm.matrix[i] = cmm_param[i];
                    snprintf(buffer, sizeof(buffer), "%0.4f", ccm.matrix[i]);
                    json_object_array_add(ccm_array, json_object_new_string(buffer));
                }
                json_object_object_add(params_obj_, "ccm", ccm_array);

                char *json_ = json_object_to_json_string(root);
                asprintf(&str, json_);
                json_object_put(root);
                set_final_ccm_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode, &ccm);
                set_dmm_rgb_gamma_param(&gamma);

                break;

            case IMD_CUS_SET_ALL_PARAM:  //设置所有参数
                lcd_tools_info("IMD_CUS_SET_ALL_PARAM\n");
                r_gain = json_object_get_double(json_object_object_get(params_obj, "r_gain"));
                g_gain = json_object_get_double(json_object_object_get(params_obj, "g_gain"));
                b_gain = json_object_get_double(json_object_object_get(params_obj, "b_gain"));
                bright_gain = json_object_get_double(json_object_object_get(params_obj, "bright_gain"));
                contrast_gain = json_object_get_double(json_object_object_get(params_obj, "contrast_gain"));
                saturation_gain = json_object_get_double(json_object_object_get(params_obj, "saturation_gain"));
                mean = json_object_get_double(json_object_object_get(params_obj, "mean"));
                angle = json_object_get_double(json_object_object_get(params_obj, "angle"));
                mode = json_object_get_double(json_object_object_get(params_obj, "mode"));

                json_object_object_get_ex(params_obj, "ccm", &ccm_a);
                array_len = json_object_array_length(ccm_a);
                for (int i = 0; i < array_len; i++) {
                    json_object *element = json_object_array_get_idx(ccm_a, i);
                    ccm.matrix[i] = json_object_get_double(element);
                }

                json_object_object_get_ex(params_obj, "gamma", &gamma_a);
                array_len = json_object_array_length(gamma_a);
                for (int i = 0; i < array_len; i++) {
                    json_object *element = json_object_array_get_idx(gamma_a, i);
                    gamma.lut[i] = json_object_get_double(element);
                }
                set_dmm_rgb_gamma_param(&gamma);

                rbs = json_object_get_int(json_object_object_get(params_obj, "rbs"));
                if (rbs == 1) {
                    set_final_ccm_param(b_gain, g_gain, r_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode, &ccm);
                } else {
                    set_final_ccm_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode, &ccm);
                }
                break;

            case IMD_CUS_SET_RBS:   //设置r分量和b分量对调
                lcd_tools_info("IMD_CUS_SET_RBS\n");
                int rbs = json_object_get_int(json_object_object_get(params_obj, "rbs"));
                if (rbs == 1) {
                    set_blend_ccmf_param(b_gain, g_gain, r_gain, bright_gain, contrast_gain,  saturation_gain,  mean,  angle,  mode);
                } else {
                    set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain,  saturation_gain,  mean,  angle,  mode);
                }
                break;

            case IMD_CUS_SET_CCM:   //颜色矩阵
                lcd_tools_info("IMD_CUS_SET_CCM\n");
                json_object *ccm_a;
                json_object_object_get_ex(params_obj, "ccm", &ccm_a);
                array_len = json_object_array_length(ccm_a);
                for (int i = 0; i < array_len; i++) {
                    json_object *element = json_object_array_get_idx(ccm_a, i);
                    ccm.matrix[i] = json_object_get_double(element);
                }
                set_final_ccm_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain,  saturation_gain,  mean,  angle, mode, &ccm);
                break;

            case IMD_CUS_SET_GAMMA: //伽马矩阵
                lcd_tools_info("IMD_CUS_SET_GAMMA\n");
                json_object *gamma_a;
                json_object_object_get_ex(params_obj, "gamma", &gamma_a);
                array_len = json_object_array_length(gamma_a);
                for (int i = 0; i < array_len; i++) {
                    json_object *element = json_object_array_get_idx(gamma_a, i);
                    gamma.lut[i] = json_object_get_double(element);
                }
                set_dmm_rgb_gamma_param(&gamma);
                break;

            case IMD_CUS_SET_R_GAIN://r_gain
                lcd_tools_info("IMD_CUS_SET_R_GAIN\n");
                r_gain = json_object_get_double(json_object_object_get(params_obj, "r_gain"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_G_GAIN://g_gain
                lcd_tools_info("IMD_CUS_SET_G_GAIN\n");
                g_gain = json_object_get_double(json_object_object_get(params_obj, "g_gain"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_B_GAIN://b_gain
                lcd_tools_info("IMD_CUS_SET_B_GAIN\n");
                b_gain = json_object_get_double(json_object_object_get(params_obj, "b_gain"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_BRIGHT://亮度 bright_gain
                lcd_tools_info("IMD_CUS_SET_BRIGHT\n");
                bright_gain = json_object_get_double(json_object_object_get(params_obj, "bright_gain"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_CONTRAST://对比度 contrast_gain
                lcd_tools_info("IMD_CUS_SET_CONTRAST\n");
                contrast_gain = json_object_get_double(json_object_object_get(params_obj, "contrast_gain"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_CONTRAST_MEAN://对比度均值
                lcd_tools_info("IMD_CUS_SET_CONTRAST_MEAN\n");
                mean = json_object_get_double(json_object_object_get(params_obj, "mean"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_SATURATION://饱和度
                lcd_tools_info("IMD_CUS_SET_SATURATION\n");
                saturation_gain = json_object_get_double(json_object_object_get(params_obj, "saturation_gain"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            case IMD_CUS_SET_ANGLE: //色相
                lcd_tools_info("IMD_CUS_SET_ANGLE\n");
                angle = json_object_get_double(json_object_object_get(params_obj, "angle"));
                set_blend_ccmf_param(r_gain, g_gain, b_gain, bright_gain, contrast_gain, saturation_gain, mean, angle, mode);
                break;

            default:
                break;
            }

            if (str) {

            } else {
                sprintf(rscorr_str, "%d", rscorr);
                json_object_string_set(rscorr_obj, rscorr_str, strlen(rscorr_str));
                json_object_object_add(new_obj, "params", json_object_new_object());
                _str = json_object_get_string(new_obj);
            }

            if (str) {
                lcd_tools_info("--->resp cjson\n\n%s\n\n", str);
                data_respond(0, DATA_TYPE_OPCODE, str, strlen(str));
            } else {
                lcd_tools_info("--->resp cjson\n\n%s\n\n", _str);
                data_respond(0, DATA_TYPE_OPCODE, _str, strlen(_str));
            }

            if (str) {
                free(str);
                str = NULL;
            }

_exit_list_:
            json_object_put(new_obj);
            list_del(&msg->entry);
            free(msg->data);
            free(msg->self);
            break;
        }
        os_mutex_post(&__THIS->list_mutex);
        os_time_dly(1);
    }
}

static void data_handle_task(void *priv)
{
    u32 len;
    u16 *crc_value;
    u8 rscorr, *data, *read_addr, *l_data, l_idx;
    u32 crc_len, recv_size, total_size, l_len;
    u16 crc_result;
    struct display_msg *msg;
    struct display_head *head;
    /* const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[8]; */
    const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[0];
    head = (struct display_head *)__THIS->recv_buf;
    data = __THIS->recv_buf + sizeof(struct display_head);
    char *str;
    u32 cdc_bulk_size = 0;

    for (;;) {
        cdc_bulk_size = (lcd_usb_id == 0) ? MAXP_SIZE_CDC_BULKIN_FS : MAXP_SIZE_CDC_BULKIN_HS;
        len = lcd_tools_dev_read(__THIS->recv_buf, cdc_bulk_size);

        if (len < sizeof(struct display_head)) {
            continue;
        }

        if (head->mark[0] != 'J' || head->mark[1] != 'L') {
            continue;
        }

        recv_size  = len;
        read_addr  = __THIS->recv_buf + len;
        total_size = sizeof(struct display_head) + head->len + sizeof(u16);

        if (total_size > RECV_BUFFER_SIZE) {
            int rscorr = 1;
            lcd_tools_err("The photo size > 400kb!\n");
            asprintf(&str, "{\"opcode\":\"%d\",\"params\":{},\"rscorr\":\"%d\"}", IMD_CUS_SHOW_JPG_IMAGE, rscorr);
            data_respond(0, IMD_CUS_SHOW_JPG_IMAGE, str, strlen(str));
            continue;
        }

        while (recv_size < total_size) {
            if ((len = lcd_tools_dev_read(read_addr, cdc_bulk_size)) <= 0) {
                break;
            }
            recv_size += len;
            read_addr += len;
        }

        if (recv_size < total_size) {
            continue;
        }

        crc_len = sizeof(struct display_head) + head->len;
        crc_result = gpcrc_calc_u8_buf((gpcrc_algorithm_cfg_t *)algo_cfg, __THIS->recv_buf, crc_len);
        u16 value;
        memcpy(&value, __THIS->recv_buf + crc_len, sizeof(u16));
        if (value != crc_result) {
            printf("error crc is not equality\n");
            continue;
        }

        switch (head->type) {
        case DATA_TYPE_OPCODE:
            msg = zalloc(sizeof(struct display_msg));
            msg->data = zalloc(head->len);
            if (!msg || !msg->data) {
                break;
            }
            msg->len = head->len;
            msg->self = msg;
            memcpy(msg->data, data, head->len);
            os_mutex_pend(&__THIS->list_mutex, 0);
            list_add_tail(&msg->entry, &__THIS->msg_list);
            os_mutex_post(&__THIS->list_mutex);
            break;

        case IMD_CUS_SHOW_JPG_IMAGE:
            lcd_tools_info("IMD_CUS_SHOW_JPG_IMAGE\n");
            display_stop();
            display_photo(data, head->len);
            break;

        default:
            break;
        }
    }
}


u8 lcd_tools_main(void)
{

    if (!__THIS) {
        __THIS = zalloc(sizeof(struct lcd_tools_hdl_type));
        ASSERT(__THIS);
    }

    lcd_tools_info("\r\n================ENTER LCD TOOLS MODE================\r\n");

    __THIS->enter_flag = 1;
    __THIS->recv_buf = zalloc(RECV_BUFFER_SIZE);
    __THIS->resp_buf = zalloc(RESP_BUFFER_SIZE);
    ASSERT(__THIS->recv_buf && __THIS->recv_buf, "lcd tool zalloc buffer err");

    INIT_LIST_HEAD(&__THIS->msg_list);  //链表头部初始化

    os_sem_create(&cdc_sem, 0);

    os_mutex_create(&__THIS->rsp_mutex);
    os_mutex_create(&__THIS->list_mutex);
    thread_fork("msg_handle_task", 10, 1024, 0, NULL, msg_handle_task, NULL);
    thread_fork("data_handle_task", 30, 1024, 0, NULL, data_handle_task, NULL);
    return 1;
}

#endif


