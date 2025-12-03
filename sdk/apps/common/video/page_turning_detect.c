#include "app_config.h"
#include "get_yuv_data.h"
#include "asm/jpeg_codec.h"

#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE

#include "motion_detect.h"

#define LOG_TAG             "[PAGE_TURNING]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


extern int page_turning_recognition(void *data, u32 length, u8 buf_copy);
extern u8 page_turning_get_mode(void);
extern u8 get_turing_ocr_flag(void);
extern u8 get_turing_finger_flag(void);
extern u32 timer_get_ms(void);

#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
static MotionDetection md;
#endif

static void get_one_frame(u8 *inputFrame, u32 len, int width, int height)
{
#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
    u8 ocr_flag = get_turing_ocr_flag();
    u8 finger_flag = get_turing_finger_flag();
    unsigned char *outImg = getMotionResult(&md, inputFrame, width, height, 1, TL_CV_YUV2GRAY_420, timer_get_ms(), ocr_flag, finger_flag);
    if (outImg) {
        struct jpeg_encode_req req = {0};
        u32 buf_len = ocr_flag ? 60 * 1024 : 30 * 1024;
        u8 *jpg_img = malloc(buf_len);
        if (!jpg_img) {
            log_error("jpeg buf not enough !!!");
            return;
        }

        //q >=2 , size  = w*h*q*0.32
        req.q = 5;	//jepg编码质量(范围0-13),质量越好,下面的编码buf需要适当增大
        req.format = JPG_SAMP_FMT_YUV420;
        req.data.buf = jpg_img;
        req.data.len = buf_len;
        req.width = ocr_flag ? width : (width / 2);
        req.height = ocr_flag ? height : (height / 2);
        req.y = outImg;
        req.u = req.y + req.width * req.height;
        req.v = req.u + req.width * req.height / 4;

        log_info("%s width : %d, height : %d", __FUNCTION__, width, height);

        if (0 == jpeg_encode_one_image(&req)) {
            log_info("jpeg_encode_one_image ok");
            if (0 != page_turning_recognition(req.data.buf, req.data.len, 0)) {
                free(jpg_img);
            }
        }
    }
#endif
}

int page_turning_det_init(void)
{
    log_info("%s", __FUNCTION__);

#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
    int min_thr = 30;
    int max_thr = 100;
    int minThrWaitTime = 100;
    int dstHeight = 100;
    int cropRate = 33;
    int serverWaitTime = 0; // 说明：如果室1000ms内只能向服务器发送1次
    int timer = 100;// 单位毫秒
    long long nowTime = timer_get_ms();// 毫秒级的时间戳
    initMotionDetection(&md, min_thr, max_thr, minThrWaitTime, dstHeight, cropRate, serverWaitTime, timer, nowTime);
    jpeg_codec_init();
#endif

    get_yuv_init(get_one_frame);

    return 0;
}

int page_turning_det_uninit(void)
{
    log_info("%s", __FUNCTION__);

    get_yuv_uninit();
#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
    cvFreeImageTL(md.frame1);
#endif
    return 0;
}

#endif
