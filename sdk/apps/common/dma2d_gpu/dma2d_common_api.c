
#include "asm/dma2d_driver.h"
#include "dma2d_common_api.h"
#include "app_config.h"

/**
 * @brief     多图层帧buffer数据拷贝
 * @param:    fg  : 前景图层配置
 * @param:    bg  : 背景图层配置
 * @param:    out : 输出图层配置
 * @return:   0: 成功 -1:失败
 **/
int dma2d_copy_frame_buf(dma2d_frame_t *fg, dma2d_frame_t *bg, dma2d_frame_t *out)
{
    uint8_t *o_dest = NULL;
    if (!bg || !fg || !out) {
        return -1;
    }
    if (bg->x + bg->width > out->width || bg->y + bg->height > out->height) {
        printf("bg size err!!!\n");
        return -1;
    }
    if (fg->x + fg->width > out->width || fg->y + fg->height > out->height) {
        printf("bg size err!!!\n");
        return -1;
    }
    dma2d_reset_all_regs();
    dma2d_create_task();
    if (bg != out) {
        /* 先拷贝背景图层 */
        /* 如果背景和前景不是同一块buf,需要拷贝背景先 */
        /* 如果背景和前景是同一块buf,则不需要拷贝背景,直接前景拷贝到输出buf即可 */
        o_dest = out->addr + (bg->y * out->width + bg->x) * dma2d_get_format_bpp(out->format) / 8;
        dma2d_copy(o_dest, out->width * dma2d_get_format_bpp(out->format) / 8, bg->addr, bg->width * dma2d_get_format_bpp(bg->format) / 8,
                   out->format, bg->width, bg->height);
        dma2d_create_task();
    }

    /* 再拷贝前景图层 */
    o_dest = out->addr + (fg->y * out->width + fg->x) * dma2d_get_format_bpp(out->format) / 8;
    dma2d_copy(o_dest, out->width * dma2d_get_format_bpp(out->format) / 8, fg->addr, fg->width * dma2d_get_format_bpp(fg->format) / 8,
               out->format, fg->width, fg->height);

    dma2d_run();
    dma2d_wait_done();

    return 0;
}

/**
 * @brief     多图层帧buffer数据混合
 * @param:    fg  : 前景图层配置
 * @param:    bg  : 背景图层配置
 * @param:    out : 输出图层配置
 * @return:   0: 成功 -1:失败
 **/
int dma2d_merge_frame_buf(dma2d_frame_t *fg, dma2d_frame_t *bg, dma2d_frame_t *out)
{
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};
    dma2d_out_layer_params_t out_layer_param = {0};
    uint8_t *o_dest = NULL;
    if (!bg || !fg || !out) {
        return -1;
    }
    dma2d_reset_all_regs();
    dma2d_create_task();
    out_layer_param.data = (uint32_t)out->addr;
    out_layer_param.w = out->width;
    out_layer_param.h = out->height;
    out_layer_param.stride = out->width * dma2d_get_format_bpp(out->format) / 8;
    out_layer_param.format = out->format;
    out_layer_param.rbs = out->rbs;
    out_layer_param.dither_en = 1;

    fg_layer_param.data = (uint32_t)fg->addr;
    fg_layer_param.stride = fg->width * dma2d_get_format_bpp(fg->format) / 8;
    fg_layer_param.format = fg->format;
    fg_layer_param.alpha = fg->alpha;
    fg_layer_param.alpha_mode = fg->alpha_mode;
    fg_layer_param.rbs = fg->rbs;
    fg_layer_param.colorkey = fg->color_key;
    fg_layer_param.color = fg->color;

    bg_layer_param.data = (uint32_t)bg->addr;
    bg_layer_param.stride = bg->width * dma2d_get_format_bpp(bg->format) / 8;
    bg_layer_param.format = bg->format;
    bg_layer_param.alpha = bg->alpha;
    bg_layer_param.alpha_mode = bg->alpha_mode;
    bg_layer_param.rbs = bg->rbs;

    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);

    /* dma2d_dump_all_tasks(); */
    dma2d_run();
    dma2d_wait_done();

    return 0;
}

/**
 * @brief     dma2d 8字节二维copy
 * @param:    dst  : 目标buf地址
 * @param:    src   : 源buf地址
 * @param:    dst_width  : 目标buf 的总宽度
 * @param:    src_width  : 源buf 的总宽度
 * @param:    width  :  需要拷贝的宽度
 * @param:    height :  需要拷贝的高度
 * @return:   0: 成功 -1:失败
 **/
int dma2d_copy_a8(uint8_t *dst, uint8_t *src, uint16_t dst_width, uint16_t src_width, uint16_t width, uint16_t height)
{
    dma2d_reset_all_regs();
    dma2d_create_task();

    dma2d_copy(dst, dst_width, src, src_width, JLDMA2D_FORMAT_A8, width, height);
    dma2d_run();
    dma2d_wait_done();

    return 0;
}

/**
 * @brief  dma2d 填充区域函数
 * @param: dset_buf:填充区域的首地址
 * @param: dest_stride:区域的偏移步进
 * @param: a:alpha 值
 * @param: r:红色值
 * @param: g:绿色值
 * @param: b:蓝色值
 * @param: out_format:输出颜色格式 (参考jldma2d_format_t)
 * @param: endian:输出大小端
 * @param: w:填充区域宽度
 * @param: h:填充区域高度
 *
 * @return:0
 **/
int jldma2d_fill(uint8_t *dest_buf, uint32_t dest_stride, uint8_t a, uint8_t r, uint8_t g, uint8_t b, uint32_t out_format, uint32_t endian, uint32_t w, uint32_t h)
{
    uint32_t fill_color;
    fill_color = dma2d_pack_pixel(a, r, g, b, JLDMA2D_FORMAT_ARGB8888);

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_fill(dest_buf, dest_stride, out_format, endian, fill_color, w, h);
    dma2d_run();
    dma2d_wait_done();

    return 0;
}

/**
 * @brief  dma2d 区域拷贝函数
 * @param: dest_buf:目标buf地址
 * @param: src_buf: 源buf地址
 * @param: dest_stride:目标区域偏移步进
 * @param: src_stride:源区域偏移步进
 * @param: format:颜色格式 (参考jldma2d_format_t)
 * @param: w:拷贝区域宽度
 * @param: h:拷贝区域高度
 *
 * @return:0
 **/
int jldma2d_copy(uint8_t *dest_buf, uint8_t *src_buf, uint32_t dest_stride, uint32_t src_stride, uint32_t format, uint32_t w, uint32_t h)
{
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_copy(dest_buf, dest_stride, src_buf, src_stride, format, w, h);
    dma2d_run();
    dma2d_wait_done();

    return 0;
}

/**
 * @brief  dma2d 格式转换函数
 * @param: dest_buf:目标buf地址
 * @param: src_buf: 源buf地址
 * @param: dest_stride:目标区域偏移步进
 * @param: src_stride:源区域偏移步进
 * @param: in_format:输入颜色格式 (参考jldma2d_format_t)
 * @param: out_format:输出颜色格式 (参考jldma2d_format_t)
 * @param: w:图像宽度
 * @param: h:图像高度
 * @param: endian:输出大小端
 * @param: out_rbs:输出rbs配置
 *
 * @return:0
 **/
int jldma2d_format(uint8_t *dest_buf, uint8_t *src_buf, uint32_t dest_stride, uint32_t src_stride, uint32_t in_format, uint32_t out_format, uint32_t w, uint32_t h, uint32_t endian, uint32_t out_rbs)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};

    fg_layer_param.data = src_buf;
    fg_layer_param.stride = src_stride;
    fg_layer_param.format = in_format;
    fg_layer_param.rbs = 0;


    out_layer_param.data = dest_buf;
    out_layer_param.stride = dest_stride;
    out_layer_param.format = out_format;
    out_layer_param.w = w;
    out_layer_param.h = h;
    out_layer_param.endian = endian;
    out_layer_param.rbs = out_rbs;
    out_layer_param.dither_en = 1;

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_PFC);
    dma2d_run();
    dma2d_wait_done();

    return 0;
}

/**
 * @brief jldma2d_endian_trans 硬件实现的端序取反
 * @Params dest_buf:目标buf地址
 * @Params src_buf:源buf地址
 * @Params w:图像宽度
 * @Params h:图像高度
 * @Params format:颜色格式 (参考jldma2d_format_t)
 */
void jldma2d_endian_trans(uint8_t *dest_buf, uint8_t *src_buf, uint32_t w, uint32_t h, uint32_t format)
{
    uint32_t stride = w * dma2d_get_format_bpp(format) / 8;
    jldma2d_format(dest_buf, src_buf, stride, stride, format, format, w, h, 1, 0);
}

/**
 * @brief  dma2d 图层混合函数
 * @param: dest_buf:目标输出buf地址
 * @param: fg_buf: 前景图层地址
 * @param: bg_buf: 背景图层地址
 * @param: fg_stride:前景图层的偏移步进
 * @param: bg_stride:背景图层的偏移步进
 * @param: fg_format:前景图层颜色格式 (参考jldma2d_format_t)
 * @param: bg_format:背景图层颜色格式 (参考jldma2d_format_t)
 * @param: out_format:输出图层颜色格式 (参考jldma2d_format_t)
 * @param: fg_rbs:前景rbs配置
 * @param: bg_rbs:背景rbs配置
 * @param: out_rbs:输出rbs配置
 * @param: fg_alpha:前景的alpha值
 * @param: fg_alpha_mode:前景的alpha混合模式
 * @param: bg_alpha:背景的alpha值
 * @param: bg_alpha_mode:背景的alpha混合模式
 * @param: blend_mode:前景和背景的混合方式
 * @param: endian:输出大小端
 * @param: w:图层宽度
 * @param: h:图层高度
 *
 * @return:0
 **/
int jldma2d_blend(uint8_t *dest_buf, uint8_t *fg_buf, uint8_t *bg_buf, uint32_t dest_stride, uint32_t fg_stride, uint32_t bg_stride, uint32_t fg_format, uint32_t bg_format, uint32_t out_format, uint8_t fg_rbs, uint8_t bg_rbs, uint8_t out_rbs, uint8_t fg_alpha, uint8_t bg_alpha, uint8_t fg_alpha_mode, uint8_t bg_alpha_mode, uint8_t blend_mode, uint32_t endian, uint32_t w, uint32_t h)
{
    dma2d_out_layer_params_t out_layer_param = {0};
    dma2d_input_layer_params_t fg_layer_param = {0};
    dma2d_input_layer_params_t bg_layer_param = {0};

    fg_layer_param.data = fg_buf;
    fg_layer_param.stride = fg_stride;
    fg_layer_param.format = fg_format;
    fg_layer_param.rbs = fg_rbs;
    fg_layer_param.alpha = fg_alpha;
    fg_layer_param.alpha_mode = fg_alpha_mode;

    bg_layer_param.data = bg_buf;
    bg_layer_param.stride = bg_stride;
    bg_layer_param.format = bg_format;
    bg_layer_param.rbs = bg_rbs;
    bg_layer_param.alpha = bg_alpha;
    bg_layer_param.alpha_mode = bg_alpha_mode;

    out_layer_param.data = dest_buf;
    out_layer_param.stride = dest_stride;
    out_layer_param.format = out_format;
    out_layer_param.endian = endian;
    out_layer_param.rbs = out_rbs;
    out_layer_param.w = w;
    out_layer_param.h = h;
    out_layer_param.dither_en = 1;


    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_blend_mode(blend_mode);
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_run();
    dma2d_wait_done();
}

int jldma2d_format_common(dma2d_input_layer_params_t *fg_layer_param, dma2d_out_layer_params_t *out_layer_param)
{
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(fg_layer_param);
    dma2d_set_out_layer(out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_PFC);
    dma2d_run();
    dma2d_wait_done();
}

int jldma2d_blend_common(dma2d_input_layer_params_t *fg_layer_param, dma2d_input_layer_params_t *bg_layer_param, dma2d_out_layer_params_t *out_layer_param)
{
    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(fg_layer_param);
    dma2d_set_bg_layer(bg_layer_param);
    dma2d_set_out_layer(out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);
    dma2d_run();
    dma2d_wait_done();
}

/*****************************测试用例如下*********************************/
#include "fs/fs.h"
static void dma2d_api_task(void *p)
{
    int size = 0;

    os_time_dly(300);
    //bg
    FILE *_file = fopen(CONFIG_ROOT_PATH"DMA2D/1280x720.yuv", "rb");
    if (!_file) {
        printf("fopen file1 faild!\n");
        return;
    }
    size = flen(_file);
    uint8_t *bg_buf = zalloc(size);
    printf("size0=%d\n", size);
    fread(bg_buf, size, 1, _file);
    fclose(_file);
    _file = NULL;

    //fg
    _file = fopen(CONFIG_ROOT_PATH"DMA2D/640x480.yuv", "rb");
    if (!_file) {
        printf("fopen file2 faild!\n");
        return;
    }
    size = flen(_file);
    printf("size1=%d\n", size);
    uint8_t *fg_buf = zalloc(size);
    fread(fg_buf, size, 1, _file);
    fclose(_file);
    _file = NULL;

#if 1
    //fg1
    _file = fopen(CONFIG_ROOT_PATH"DMA2D/1280x720.rgb", "rb");
    if (!_file) {
        printf("fopen file3 faild!\n");
        return;
    }
    size = flen(_file);
    printf("size3=%d\n", size);
    uint8_t *fg1_buf = zalloc(size);
    fread(fg1_buf, size, 1, _file);
    fclose(_file);
    _file = NULL;
#endif

    dma2d_frame_t fg = {0};
    dma2d_frame_t bg = {0};
    dma2d_frame_t out = {0};
    bg.addr = bg_buf;
    bg.x = 0;
    bg.y = 0;
    bg.width = 1280;
    bg.height = 720;
    bg.format = JLDMA2D_FORMAT_YUV422_BT601;

    fg.addr = fg_buf;
    fg.x = 10;
    fg.y = 10;
    fg.width = 640;
    fg.height = 480;
    fg.format = JLDMA2D_FORMAT_YUV422_BT601;

    out.width = 1280;
    out.height = 720;
    uint8_t *out_buf = zalloc(out.width * out.height * 2);
    out.addr = out_buf;
    out.format = JLDMA2D_FORMAT_YUV422_BT601;

    dma2d_init();
    DcuFlushRegion((u32 *)fg.addr, fg.width * fg.height * 2);
    DcuFlushinvRegion(out_buf, out.width * out.height * 2);

    //图层拷贝测试
    dma2d_copy_frame_buf(&fg, &bg, &out);

    fg.addr = fg1_buf;
    fg.x = 0;
    fg.y = 0;
    fg.width = 1280;
    fg.height = 720;
    fg.alpha = 50;
    fg.alpha_mode = JLDMA2D_COMBINE_ALPHA;
    fg.format = JLDMA2D_FORMAT_RGB888;
    fg.rbs = 1;

    DcuFlushRegion((u32 *)fg.addr, fg.width * fg.height * 3);
    u32 _j = jiffies;

    //图层混合测试
    dma2d_merge_frame_buf(&fg, &out, &out);
    printf("\n[ debug ]--func=%s line=%d %dms\n", __func__, __LINE__, jiffies_to_msecs(jiffies - _j));

    _file = fopen(CONFIG_ROOT_PATH"DMA2D/out.yuv", "wb");
    fwrite(out_buf, out.width * out.height * 2, 1, _file);
    fclose(_file);

    if (fg_buf) {
        free(fg_buf);
    }
    if (fg1_buf) {
        free(fg1_buf);
    }
    if (bg_buf) {
        free(bg_buf);
    }
    if (out_buf) {
        free(out_buf);
    }

    while (1) {
        os_time_dly(100);
    }
}
static void dma2d_api_test_init(void)
{
    thread_fork("dma2d_api_task", 30, 1000, 0, 0, dma2d_api_task, NULL);
}
/* late_initcall(dma2d_api_test_init); */
