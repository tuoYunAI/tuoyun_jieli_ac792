/******************************************************************************
* File:             fb_rotate.c
*
* Author:
* Created:          06/12/24
* Description:      1. fb显存数据处理(使用GPU) 旋转、缩放、镜像
*****************************************************************************/


#include "asm/dma2d_driver.h"
#include "app_config.h"


#include "asm/hwApi.h"
#include "asm/jlgpu_driver.h"
#include "asm/gpu/jlvg.h"
#include "asm/jlgpu_driver.h"

#if TCFG_LCD_ENABLE

static unsigned char gRectSeg[] = {
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0xfa, 0x43,
    0xcd, 0x01, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x01,
    0x03, 0x00, 0xf4, 0x01, 0x04, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0xf4, 0x01,
    0xff
};

static VGHW_FragImage_TypeDef gFragImage = {0};
static VGHW_CMD_TypeDef gCmd = {0};
struct gFrag_ckey_t {
    uint8_t ck_en;
    uint8_t ck_red;
    uint8_t ck_green;
    uint8_t ck_blue;
};
static struct gFrag_ckey_t g_ckey = {0};

#define MATRIX_COEFF_TO_FIXED(m)  (*(VGHWuint*)&(m))

static u32 run_time = 0;
static int __gpu_out_cb_func(void)
{
    run_time = get_system_us() - run_time;
}
static void convertMatrixX9(float *matrix, VGHW_FragImage_TypeDef *gFragImage)
{
    float t;

    t = matrix[0] / matrix[8];
    gFragImage->img_im00 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[1] / matrix[8];
    gFragImage->img_im01 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[2] / matrix[8];
    gFragImage->img_im02 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[3] / matrix[8];
    gFragImage->img_im10 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[4] / matrix[8];
    gFragImage->img_im11 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[5] / matrix[8];
    gFragImage->img_im12 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[6] / matrix[8];
    gFragImage->img_im20 = MATRIX_COEFF_TO_FIXED(t);

    t = matrix[7] / matrix[8];
    gFragImage->img_im21 = MATRIX_COEFF_TO_FIXED(t);

}

static void convertMatrixX6(float *matrix, VGHW_SEG_TypeDef *seg)
{
    seg->trans_m00 = MATRIX_COEFF_TO_FIXED(matrix[0]);
    seg->trans_m01 = MATRIX_COEFF_TO_FIXED(matrix[1]);
    seg->trans_m02 = MATRIX_COEFF_TO_FIXED(matrix[2]);
    seg->trans_m10 = MATRIX_COEFF_TO_FIXED(matrix[3]);
    seg->trans_m11 = MATRIX_COEFF_TO_FIXED(matrix[4]);
    seg->trans_m12 = MATRIX_COEFF_TO_FIXED(matrix[5]);

}

static void setFragImage(VGHW_FragImage_TypeDef *gFragImage, float *matrix, VGHWuint adr,
                         VGHWuint w, VGHWuint h, VGHWuint stride,
                         VGHWuint format, VGHWuint adr_mode, VGHWuint premult, VGHWuint rb_swap, VGHWuint quality)
{
    memset(gFragImage, 0, sizeof(VGHW_FragImage_TypeDef));

    convertMatrixX9(matrix, gFragImage);

    gFragImage->ckey_en = g_ckey.ck_en;
    gFragImage->ckey_red = g_ckey.ck_red;
    gFragImage->ckey_green = g_ckey.ck_green;
    gFragImage->ckey_blue = g_ckey.ck_blue;

    gFragImage->clut_format = 0;
    gFragImage->img_adr_mode = adr_mode;
    gFragImage->img_a_color = 0;
    gFragImage->img_base_adr = adr;
    gFragImage->img_color_fmt = format;
    gFragImage->img_height = h;
    gFragImage->img_width = w;
    gFragImage->img_stride = stride;
    gFragImage->premult = premult;
    gFragImage->rb_swap = rb_swap;
    gFragImage->img_lut_adr = 0;
    /* gFragImage->quality = VGHW_IMAGE_QUALITY_NONANTIALIASED; */
    gFragImage->quality = quality;
    gFragImage->color_ext = 0; //fixed cfg
    gFragImage->margin_mode = 0;
    gFragImage->rle_mode = 0;
    if (adr_mode == 0) {
        gFragImage->rle_limit = (gFragImage->img_stride >> 3) * gFragImage->img_height;
    } else {
        gFragImage->rle_limit = gFragImage->img_stride * gFragImage->img_height;
    }
}
static void setRectSeg(VGHW_SEG_TypeDef *seg, float *matrix, VGHW_FillRule_TypeDef fill_rule, int x, int y, int w, int h)
{
    int coord = 0;

    convertMatrixX6(matrix, seg);

    seg->fill_rule = fill_rule;

    int offset = sizeof(VGHW_SEG_TypeDef) - 3;

    //move to
    coord = x;
    gRectSeg[offset + 1] = (coord >> 0) & 0xff;
    gRectSeg[offset + 2] = (coord >> 8) & 0xff;
    gRectSeg[offset + 3] = (coord >> 16) & 0xff;

    coord = y + h;
    gRectSeg[offset + 4] = (coord >> 0) & 0xff;
    gRectSeg[offset + 5] = (coord >> 8) & 0xff;
    gRectSeg[offset + 6] = (coord >> 16) & 0xff;

    //hline to
    offset += 7;
    coord = x + w;
    gRectSeg[offset + 1] = (coord >> 0) & 0xff;
    gRectSeg[offset + 2] = (coord >> 8) & 0xff;
    gRectSeg[offset + 3] = (coord >> 16) & 0xff;

    //vline to
    offset += 4;
    coord = y;
    gRectSeg[offset + 1] = (coord >> 0) & 0xff;
    gRectSeg[offset + 2] = (coord >> 8) & 0xff;
    gRectSeg[offset + 3] = (coord >> 16) & 0xff;

    //hline to
    offset += 4;
    coord = x;
    gRectSeg[offset + 1] = (coord >> 0) & 0xff;
    gRectSeg[offset + 2] = (coord >> 8) & 0xff;
    gRectSeg[offset + 3] = (coord >> 16) & 0xff;

    //vline to
    offset += 4;
    coord = y + h;
    gRectSeg[offset + 1] = (coord >> 0) & 0xff;
    gRectSeg[offset + 2] = (coord >> 8) & 0xff;
    gRectSeg[offset + 3] = (coord >> 16) & 0xff;

}

static void getCmd(VGHW_CMD_TypeDef *cmd, VGHW_SEG_TypeDef *seg, int seg_len, VGHW_FragImage_TypeDef *fragimage,
                   unsigned int win_x_min, unsigned int win_x_max, unsigned int win_y_min, unsigned int win_y_max
                  )
{
    cmd->msaa_lvl = VGHW_RENDERING_QUALITY_NONANTIALIASED;
    cmd->seg_base_adr = (VGHWuint)seg;
    cmd->seg_cmd_len = seg_len;
    cmd->paint_mode = VGHW_FRAG_IMAGE;
    cmd->act_x_min = win_x_min;
    cmd->act_y_min = win_y_min;
    cmd->act_x_max = win_x_max;
    cmd->act_y_max = win_y_max;
    cmd->blend_mode = VGHW_BLEND_SRC;
    cmd->gb_alpha = 128;
    cmd->frag_base_adr = (VGHWuint)fragimage;
    cmd->next_inst_adr = 0;
}

static int image_rotate(uint8_t *dst, int dst_w, int dst_h, int dst_stride, int dst_xoffset, int dst_yoffset,
                        uint8_t *src, int src_w, int src_h, int src_stride, int src_format, int in_rbs,
                        int scx, int scy, int dcx, int dcy, float degree, int dst_format, int out_rbs, uint8_t mirror)
{
    float matrix[9] = { 1.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 1.0f
                      };

    int quality = VGHW_IMAGE_QUALITY_NONANTIALIASED;

    jlgpu_init();

    jlgpu_reset_all_regs();

    jlgpu_set_out_window((VGHWuint)dst, 0, dst_w, 0, dst_h, dst_stride);

    jlgpu_set_out_format(dst_format, 1, 1, out_rbs, 1);

    jlgpu_set_bg_mode(1, 0);
    jlgpu_set_max_bez_div(7);
    /* jlgpu_set_max_msaa_lvl(VGHW_RENDERING_QUALITY_MSAA_8X); */
    jlgpu_set_max_msaa_lvl(VGHW_RENDERING_QUALITY_NONANTIALIASED);

    VGHW_SEG_TypeDef *seg = (VGHW_SEG_TypeDef *)&gRectSeg;
    VGHW_CMD_TypeDef *cmd = (VGHW_CMD_TypeDef *)&gCmd;
    VGHW_FragImage_TypeDef *fragimage = (VGHW_FragImage_TypeDef *)&gFragImage;

    float userToSurface[9] = { 1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f
                             };

    float surfaceToImage[9];

    int seg_len = sizeof(gRectSeg);

    float rad = degree * 3.14159f / 180.0f;
    float cos_rad = 0.0f;
    float sin_rad = 0.0f;
    if (degree == 90.0f) {
        sin_rad = 1.0f;
    } else if (degree == 270.0f) {
        sin_rad = -1.0f;
    } else if (degree == 180.0f) {
        cos_rad = -1.0f;
    } else {
        cos_rad = cosf(rad);
        sin_rad = sinf(rad);
    }
    /* printf("cos_rad:%f\n",cos_rad ); */
    /* printf("sin_rad:%f\n",sin_rad ); */

    surfaceToImage[0] = cos_rad;
    surfaceToImage[1] = -sin_rad;
    surfaceToImage[2] = (-dcx * cos_rad + dcy * sin_rad + scx) + dst_xoffset;
    surfaceToImage[3] = sin_rad;
    surfaceToImage[4] = cos_rad;
    surfaceToImage[5] = (-dcx * sin_rad - dcy * cos_rad + scy) + dst_yoffset;
    surfaceToImage[6] = 0.0f;
    surfaceToImage[7] = 0.0f;
    surfaceToImage[8] = 1.0f;

    //旋转后缩放
    if (degree == 90.0f || degree == 270.0f) {
        if (src_w != dst_h || src_h != dst_w) {
            float zoom_x = src_h * 1.0f * 256 / dst_w / 256;
            float zoom_y = src_w * 1.0f * 256 / dst_h / 256;
            jlvg_matrix_scale(surfaceToImage, zoom_y, zoom_x);
            quality = VGHW_IMAGE_QUALITY_BILINEAR; //双线性插值
        }
    } else if (degree == 180.0f) {
        if (src_w != dst_w || src_h != dst_h) {
            float zoom_x = src_w * 1.0f * 256 / dst_w / 256;
            float zoom_y = src_h * 1.0f * 256 / dst_h / 256;
            jlvg_matrix_scale(surfaceToImage, zoom_x, zoom_y);
            quality = VGHW_IMAGE_QUALITY_BILINEAR; //双线性插值
        }
    }

    if (mirror == 1) {
        //水平镜像
        float matrix_out[9];
        memcpy(matrix_out, surfaceToImage, sizeof(matrix_out));
        matrix[0] = -1.0f;
        matrix[1] = 0.0f;
        matrix[2] = dst_w;
        matrix[3] = 0.0f;
        matrix[4] = 1.0f;
        matrix[5] = 0.0f;
        jlvg_matrix_multiply(&surfaceToImage, &matrix_out, &matrix);
    }

    setRectSeg(seg, userToSurface, VGHW_NON_ZERO,
               (VGHWuint)0 << 8,
               (VGHWuint)0 << 8,
               ((VGHWuint)dst_w) << 8,
               ((VGHWuint)dst_h) << 8);

    setFragImage(fragimage, surfaceToImage, src,
                 src_w,
                 src_h,
                 src_stride, src_format,
                 1, 1,
                 in_rbs, quality);

    getCmd(cmd, seg, seg_len, fragimage,
           0, dst_w, 0, dst_h);

    jlgpu_add_layer(cmd);

    jlgpu_set_out_callback(__gpu_out_cb_func);
    run_time = get_system_us();

    jlgpu_run();
    jlgpu_wait_done();
    jlgpu_set_out_callback(NULL);

    /* printf("image_rotate use=%dus\n", run_time); */

    /* jlgpu_free(); */

    return run_time;
}
static int image_scale(uint8_t *dst, int dst_w, int dst_h, int dst_stride, int dst_format,
                       uint8_t *src, int src_w, int src_h, int src_stride, int src_format, int rbs, uint8_t mirror)
{
    float matrix[9] = { 1.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 1.0f
                      };

    int quality = VGHW_IMAGE_QUALITY_BILINEAR;

    jlgpu_init();

    jlgpu_reset_all_regs();

    jlgpu_set_out_window((VGHWuint)dst, 0, dst_w, 0, dst_h, dst_stride);

    jlgpu_set_out_format(dst_format, 1, 0, 0, 1);

    jlgpu_set_bg_mode(1, 0);
    jlgpu_set_max_bez_div(7);
    //jlgpu_set_max_msaa_lvl(VGHW_RENDERING_QUALITY_MSAA_8X);
    jlgpu_set_max_msaa_lvl(VGHW_RENDERING_QUALITY_NONANTIALIASED);

    VGHW_SEG_TypeDef *seg = (VGHW_SEG_TypeDef *)&gRectSeg;
    VGHW_CMD_TypeDef *cmd = (VGHW_CMD_TypeDef *)&gCmd;
    VGHW_FragImage_TypeDef *fragimage = (VGHW_FragImage_TypeDef *)&gFragImage;

    float userToSurface[9] = { 1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f
                             };

    int seg_len = sizeof(gRectSeg);

    float surfaceToImage[9] = { 1.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f
                              };
    float zoom_x = src_w * 1.0f * 256 / dst_w / 256;
    float zoom_y = src_h * 1.0f * 256 / dst_h / 256;
#if 1
    //jlvg_matrix_set_identity(surfaceToImage);
    jlvg_matrix_translate(surfaceToImage, -src_w / 2, -src_h / 2);
    jlvg_matrix_scale(surfaceToImage, zoom_x, zoom_y);
    jlvg_matrix_translate(surfaceToImage, src_w / 2 * zoom_x, src_h / 2 * zoom_y);

    if (mirror == 1) {
        //垂直镜像
        float matrix_out[9];
        memcpy(matrix_out, surfaceToImage, sizeof(matrix_out));
        matrix[0] = 1.0f;
        matrix[1] = 0.0f;
        matrix[2] = 0.0f;
        matrix[3] = 0.0f;
        matrix[4] = -1.0f;
        matrix[5] = dst_h;
        jlvg_matrix_multiply(&surfaceToImage, &matrix_out, &matrix);
    }
#endif


    setRectSeg(seg, userToSurface, VGHW_NON_ZERO,
               (VGHWuint)0 << 8,
               (VGHWuint)0 << 8,
               ((VGHWuint)dst_w) << 8,
               ((VGHWuint)dst_h) << 8);

    setFragImage(fragimage, surfaceToImage, src,
                 src_w,
                 src_h,
                 src_stride, src_format,
                 1, 1,
                 rbs, quality);

    getCmd(cmd, seg, seg_len, fragimage,
           0, dst_w, 0, dst_h);

    jlgpu_add_layer(cmd);

    jlgpu_set_out_callback(__gpu_out_cb_func);
    run_time = get_system_us();
    jlgpu_run();
    jlgpu_wait_done();
    jlgpu_set_out_callback(NULL);

    /* j1 = get_system_us() - j; */
    /* printf("use=%dus\n", j1); */

    return run_time;
}

void fb_frame_buf_rotate_set_colorkey(u8 ckey_en, uint8_t ckey_red, uint8_t ckey_green, uint8_t ckey_blue)
{
    g_ckey.ck_en = ckey_en;
    g_ckey.ck_red = ckey_red;
    g_ckey.ck_green = ckey_green;
    g_ckey.ck_blue = ckey_blue;
}

/**
 * @brief     图层帧buffer旋转
 * @param:    image_src  : 源头图层地址
 * @param:    image_dst  : 目标图层地址
 * @param:    src_width  : 输入图层宽度
 * @param:    src_height : 输入图层高度
 * @param:    src_stride : 输入图层跨度
 * @param:    dst_width  : 输出图层宽度
 * @param:    dst_height : 输出图层高度
 * @param:    dst_stride : 输出图层跨度
 * @param:    degree : 图层旋转角度
 * @param:    xoffset : 图层旋转后x偏移
 * @param:    yoffset : 图层旋转后y偏移
 * @param:    in_format : 输入图层格式
 * @param:    out_format : 输出图层格式
 * @param:    mirror : 输出镜像,1:垂直镜像
 * @return:   -1:失败 其他:rotate耗费时间(us)
 **/
int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int src_stride,
                        int dst_width, int dst_height, int dst_stride, int degree, int xoffset, int yoffset,
                        int in_format, int out_format, uint8_t mirror)
{

    /* printf("rotate=%d x %d --> %d x %d\n", src_width,src_height,dst_width,dst_height); */
    int image_src_w, image_src_h;
    int image_dst_w, image_dst_h;
    int image_src_stride = 0;
    int image_dst_stride = 0;
    int image_dst_size = 0;
    int scx, scy, dcx, dcy;

    u8 in_rbs = 0;
    if (in_format == VGHW_FORMAT_YUV422_BT601 || in_format == VGHW_FORMAT_YUV422_BT709) {
        in_rbs = 1;
    }
    int out_rbs = 0;
    if (out_format == VGHW_FORMAT_YUV422_BT601 || out_format == VGHW_FORMAT_YUV422_BT709) {
        out_rbs = 1;
    }

    uint32_t in_bpp = dma2d_get_format_bpp(in_format) >> 3;
    uint32_t out_bpp = dma2d_get_format_bpp(out_format) >> 3;
    image_src_w = src_width;
    image_src_h = src_height;
    image_src_stride = (src_stride == 0) ? image_src_w * in_bpp : src_stride;
    image_dst_w = dst_width;
    image_dst_h = dst_height;
    image_dst_stride = (dst_stride == 0) ? image_dst_w * out_bpp : dst_stride;
    image_dst_size = image_dst_w * image_dst_h * out_bpp;

    /* 旋转中心点 */
    scx = image_src_w >> 1; // /2
    scy = image_src_h >> 1; // /2

    /* 旋转中心点 */
    dcx = image_dst_w >> 1;//  /2
    dcy = image_dst_h >> 1;//  /2

    if (degree != 180) {
        if (dcy - scx) {
            xoffset -= (dcy - scx);
        }
        if (dcx - scy) {
            yoffset -= (dcx - scy);
        }
    } else {
        if (dcx - scx) {
            xoffset -= (dcx - scx);
        }
        if (dcy - scy) {
            yoffset -= (dcy - scy);
        }
    }

    return image_rotate(image_dst, image_dst_w, image_dst_h, image_dst_stride, -xoffset, -yoffset,
                        image_src, image_src_w, image_src_h, image_src_stride, in_format, in_rbs,
                        scx, scy, dcx, dcy, degree, out_format, out_rbs, mirror);

}

/**
 * @brief     图层帧buffer镜像处理
 * @param:    image_src  : 源头图层地址
 * @param:    image_dst  : 目标图层地址
 * @param:    width  : 图层宽度
 * @param:    height : 图层高度
 * @param:    mirror : 图层镜像 1:水平镜像 2:垂直镜像 3:水平+垂直镜像(相当于旋转180)
 * @param:    src_format : 输入图层格式
 * @param:    dst_format : 输出图层格式
 * @return:   0: 成功 -1:失败
 **/
int fb_frame_buf_mirror(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int dst_width, int dst_height, int mirror, int src_format, int dst_format)
{

    int src_stride = 0;
    int dst_stride = 0;
    int dst_size = 0;

    int in_rbs = 1;
    int out_rbs = 0;

    int quality = VGHW_IMAGE_QUALITY_NONANTIALIASED;

    float matrix[9];

    uint32_t in_bpp = dma2d_get_format_bpp(src_format) >> 3;
    uint32_t out_bpp = dma2d_get_format_bpp(dst_format) >> 3;
    src_stride = src_width * in_bpp;
    dst_stride = dst_width * out_bpp;

    jlgpu_init();

    jlgpu_reset_all_regs();

    jlgpu_set_out_window((VGHWuint)image_dst, 0, dst_width, 0, dst_height, dst_stride);

    jlgpu_set_out_format(dst_format, 1, 0, out_rbs, 1);

    jlgpu_set_bg_mode(1, 0);
    jlgpu_set_max_bez_div(7);
    /* jlgpu_set_max_msaa_lvl(VGHW_RENDERING_QUALITY_MSAA_8X); */
    jlgpu_set_max_msaa_lvl(VGHW_RENDERING_QUALITY_NONANTIALIASED);

    VGHW_SEG_TypeDef *seg = (VGHW_SEG_TypeDef *)&gRectSeg;
    VGHW_CMD_TypeDef *cmd = (VGHW_CMD_TypeDef *)&gCmd;
    VGHW_FragImage_TypeDef *fragimage = (VGHW_FragImage_TypeDef *)&gFragImage;

    float userToSurface[9] = { 1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f
                             };

    float surfaceToImage[9];

    int seg_len = sizeof(gRectSeg);

    if (mirror == 1) {
        //水平镜像
        surfaceToImage[0] = -1.0f;
        surfaceToImage[1] = 0.0f;
        surfaceToImage[2] = src_width;
        surfaceToImage[3] = 0.0f;
        surfaceToImage[4] = 1.0f;
        surfaceToImage[5] = 0.0f;
    } else if (mirror == 2) {
        //垂直镜像
        surfaceToImage[0] = 1.0f;
        surfaceToImage[1] = 0.0f;
        surfaceToImage[2] = 0.0f;
        surfaceToImage[3] = 0.0f;
        surfaceToImage[4] = -1.0f;
        surfaceToImage[5] = src_height;
    } else if (mirror == 3) {
        //水平+垂直镜像
        surfaceToImage[0] = -1.0f;
        surfaceToImage[1] = 0.0f;
        surfaceToImage[2] = src_width;
        surfaceToImage[3] = 0.0f;
        surfaceToImage[4] = -1.0f;
        surfaceToImage[5] = src_height;
    }
    surfaceToImage[6] = 0.0f;
    surfaceToImage[7] = 0.0f;
    surfaceToImage[8] = 1.0f;


    setRectSeg(seg, userToSurface, VGHW_NON_ZERO,
               (VGHWuint)0 << 8,
               (VGHWuint)0 << 8,
               ((VGHWuint)dst_width) << 8,
               ((VGHWuint)dst_height) << 8);

    setFragImage(fragimage, surfaceToImage, image_src,
                 src_width,
                 src_height,
                 src_stride, src_format,
                 1, 1,
                 in_rbs, quality);

    getCmd(cmd, seg, seg_len, fragimage,
           0, dst_width, 0, dst_height);

    jlgpu_add_layer(cmd);

    jlgpu_run();
    jlgpu_wait_done();

    return 0;
}

int fb_frame_buf_scale(uint8_t *dst, int dst_format, int dst_w, int dst_h,
                       uint8_t *src, int src_format, int src_w, int src_h, uint8_t mirror)
{
    u8 image_rbs = 0;
    if (src_format == VGHW_FORMAT_YUV422_BT601 || src_format == VGHW_FORMAT_YUV422_BT709) {
        image_rbs = 1;
    }
    image_scale(dst, dst_w, dst_h, dst_w * jlvg_get_image_format_bpp(dst_format) >> 3, dst_format,
                src, src_w, src_h, src_w * jlvg_get_image_format_bpp(src_format) >> 3, src_format, image_rbs, mirror);

    return 0;
}


#if 0
/*****************************测试用例如下*********************************/
#include "fs/fs.h"
static void rotate_test_task(void *p)
{
    int size = 0;

    os_time_dly(500);
    FILE *_file = fopen(CONFIG_ROOT_PATH"out.yuv", "rb");
    if (!_file) {
        printf("fopen file1 faild!\n");
        return;
    }
    size = flen(_file);
    uint8_t *image_src = zalloc(size);
    printf("size0=%d\n", size);
    fread(image_src, size, 1, _file);
    fclose(_file);
    _file = NULL;

    int w = 800;
    int h = 480;
    uint8_t *image_dst = malloc(w * h * 2);


    float degree = 270.0f;
    int format = VGHW_FORMAT_YUV422_BT601;//0;
    u32 j = 0 ;
    j = get_system_us();
    fb_frame_buf_rotate(image_src, image_dst, w, h, 0, h, w, 0, degree, 0, 0, format);
    printf("jj=%dus\n", get_system_us() - j);

    _file = fopen(CONFIG_ROOT_PATH"outr.bin", "wb");
    fwrite(image_dst, w * h * 2, 1, _file);
    fclose(_file);

    if (image_src) {
        free(image_src);
    }
    if (image_dst) {
        free(image_dst);
    }

    while (1) {
        os_time_dly(100);
    }
}
static void _test_init(void)
{
    thread_fork("rotate_task", 30, 1000, 0, 0, rotate_test_task, NULL);
}
/* late_initcall(_test_init); */
#endif

#endif
