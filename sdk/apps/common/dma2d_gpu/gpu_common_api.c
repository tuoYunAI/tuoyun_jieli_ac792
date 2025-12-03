
#include "asm/dma2d_driver.h"
#include "gpu_common_api.h"
#include "app_config.h"


#include "asm/hwApi.h"
#include "asm/jlgpu_driver.h"


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

#define MATRIX_COEFF_TO_FIXED(m)  (*(VGHWuint*)&(m))

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
                         VGHWuint format, VGHWuint adr_mode, VGHWuint premult, VGHWuint rb_swap)
{
    memset(gFragImage, 0, sizeof(VGHW_FragImage_TypeDef));

    convertMatrixX9(matrix, gFragImage);

    gFragImage->ckey_blue = 0;
    gFragImage->ckey_en = 0;
    gFragImage->ckey_green = 0;
    gFragImage->ckey_red = 0;
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
    gFragImage->quality = VGHW_IMAGE_QUALITY_NONANTIALIASED;
    /* gFragImage->quality = VGHW_IMAGE_QUALITY_BILINEAR; */
    gFragImage->color_ext = 1; //fixed cfg
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

static int image_rotate(uint8_t *dst, int dst_w, int dst_h, int dst_stride,
                        uint8_t *src, int src_w, int src_h, int src_stride,
                        int scx, int scy, int dcx, int dcy, float degree, int format, int rbs)
{
    float matrix[9];

    jlgpu_init();

    jlgpu_reset_all_regs();

    jlgpu_set_out_window((VGHWuint)dst, 0, dst_w, 0, dst_h, dst_stride);

    jlgpu_set_out_format(format, 1, 0, rbs, 1);

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
    float cos_rad = cosf(rad);
    float sin_rad = sinf(rad);

    surfaceToImage[0] = cos_rad;
    surfaceToImage[1] = -sin_rad;
    surfaceToImage[2] = -dcx * cos_rad + dcy * sin_rad + scx;
    surfaceToImage[3] = sin_rad;
    surfaceToImage[4] = cos_rad;
    surfaceToImage[5] = -dcx * sin_rad - dcy * cos_rad + scy;
    surfaceToImage[6] = 0.0f;
    surfaceToImage[7] = 0.0f;
    surfaceToImage[8] = 1.0f;


    setRectSeg(seg, userToSurface, VGHW_NON_ZERO,
               (VGHWuint)0 << 8,
               (VGHWuint)0 << 8,
               ((VGHWuint)dst_w) << 8,
               ((VGHWuint)dst_h) << 8);

    setFragImage(fragimage, surfaceToImage, src,
                 src_w,
                 src_h,
                 src_stride, format,
                 1, 1,
                 rbs);

    getCmd(cmd, seg, seg_len, fragimage,
           0, dst_w, 0, dst_h);

    jlgpu_add_layer(cmd);

    /* u32 _j = jiffies; */
    /* JL_PORTC->DIR &= ~BIT(11); */
    /* JL_PORTC->OUT |= BIT(11); */
    jlgpu_run();
    jlgpu_wait_done();
    /* JL_PORTC->OUT &= ~BIT(11); */
    /* printf("\n[ debug ]--func=%s line=%d %dms\n", __func__, __LINE__, jiffies_to_msecs(jiffies - _j)); */

    jlgpu_free();

    return 0;
}

/**
 * @brief     图层帧buffer旋转
 * @param:    image_src  : 源头图层地址
 * @param:    image_dst  : 目标图层地址
 * @param:    width  : 图层宽度
 * @param:    height : 图层高度
 * @param:    degree : 图层旋转角度
 * @param:    format : 图层格式
 * @return:   0: 成功 -1:失败
 **/
int gpu_rotate_frame_buf(uint8_t *image_src, uint8_t *image_dst, int src_width, int src_height, int dst_width, int dst_height, int degree, int format)
{

    int image_src_w, image_src_h;
    int image_dst_w, image_dst_h;
    int image_src_stride = 0;
    int image_dst_stride = 0;
    int image_dst_size = 0;
    int scx, scy, dcx, dcy;

    int rbs = 1;

    uint32_t bpp = dma2d_get_format_bpp(format) / 8;
    image_src_w = src_width;
    image_src_h = src_height;
    image_src_stride = image_src_w * bpp;
    image_dst_w = dst_width;
    image_dst_h = dst_height;
    image_dst_stride = image_dst_w * bpp;
    image_dst_size = image_dst_w * image_dst_h * bpp;

    /* 旋转中心点 */
    scx = image_src_w / 2;
    scy = image_src_h / 2;

    /* 旋转中心点 */
    dcx = image_dst_w / 2;
    dcy = image_dst_h / 2;

    return image_rotate(image_dst, image_dst_w, image_dst_h, image_dst_stride,
                        image_src, image_src_w, image_src_h, image_src_stride,
                        scx, scy, dcx, dcy, degree, format, rbs);

}


/*****************************测试用例如下*********************************/
#include "fs/fs.h"
static void gpu_api_task(void *p)
{
    int size = 0;

    os_time_dly(300);
    FILE *_file = fopen(CONFIG_ROOT_PATH"GPU/1280x720.yuv", "rb");
    /* FILE *_file = fopen(CONFIG_ROOT_PATH"GPU/720p.bin", "rb"); */
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

    int w = 1280;
    int h = 720;
    uint8_t *image_dst = malloc(w * h * 4);


    float degree = 90.0f;
    int format = VGHW_FORMAT_YUV422_BT601;//0;
    /* int format = VGHW_FORMAT_ARGB8888; */
    printf("\n[ debug ]--func=%s line=%d\n", __func__, __LINE__);
    /* DcuFlushRegion(image_src, w * h * 2);  */
    /* DcuFlushinvRegion(image_dst, w * h * 2);  */
    gpu_rotate_frame_buf(image_src, image_dst, w, h, h, w, degree, format);

    _file = fopen(CONFIG_ROOT_PATH"GPU/out.bin", "wb");
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
static void gpu_api_test_init(void)
{
    thread_fork("gpu_api_task", 30, 1000, 0, 0, gpu_api_task, NULL);
}
/* late_initcall(gpu_api_test_init); */
