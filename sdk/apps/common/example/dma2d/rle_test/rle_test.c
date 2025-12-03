
#define RLE_TEST_ON_PC  0
#if RLE_TEST_ON_PC
#include <stdio.h>

#include "jldma2d.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "dma2d_driver.h"
#include "rle_header.h"
#include "utils.h"
#else
#include "rle_header.h"
#include "asm/dma2d_driver.h"
#include "asm/jldma2d.h"
#include "fs/fs.h"
#define CONFIG_STORAGE_PATH  "storage/sd0"
#endif
#define IMG_WIDTH       640
#define IMG_HEIGHT      480


static char  *load_image(char *filename)
{
    FILE *fp = NULL;
    uint8_t *data = NULL;

    int image_stride = 0;

    int ret = 0;

    fp = fopen(filename, "rb");
    if (!fp) {
        printf("%s file not exist\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);

    int filesize = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    data = (uint8_t *)malloc(filesize);

    fread(data, 1, filesize, fp);

    if (fp) {
        fclose(fp);
    }
#if !RLE_TEST_ON_PC
    DcuFlushRegion(data, filesize);
#endif
    return data;
}

static void free_image(char *image)
{
    if (image) {
        free(image);
    }
}

static void test_rle(char *bg_filename, char *fg_filename)
{
    dma2d_out_layer_params_t out_layer_param;
    dma2d_input_layer_params_t fg_layer_param;
    dma2d_input_layer_params_t bg_layer_param;

    memset(&out_layer_param, 0, sizeof(out_layer_param));
    memset(&fg_layer_param, 0, sizeof(fg_layer_param));
    memset(&bg_layer_param, 0, sizeof(bg_layer_param));


    dma2d_init();

    dma2d_reset_all_regs();
    dma2d_create_task();

    struct image_info *fg_image_info = (struct image_info *) load_image(fg_filename);
    if (!fg_image_info) {
        return;
    }

    struct image_info *bg_image_info = (struct image_info *) load_image(bg_filename);
    if (!bg_image_info) {
        return;
    }

    printf("fg_w=%d,fg_h=%d,fg_s=%d\n", fg_image_info->width, fg_image_info->height, fg_image_info->stride);
    printf("bg_w=%d,bg_h=%d,bg_s=%d\n", bg_image_info->width, bg_image_info->height, bg_image_info->stride);

    dma2d_rectangle_t rect_fg;
    dma2d_rectangle_t rect_bg;
    dma2d_rectangle_t rect_draw;

    rect_fg.x = (bg_image_info->width - fg_image_info->width) / 2;
    rect_fg.y = (bg_image_info->height - fg_image_info->height) / 2;
    rect_fg.w = fg_image_info->width;
    rect_fg.h = fg_image_info->height;

    rect_bg.x = 0;
    rect_bg.y = 0;
    rect_bg.w = bg_image_info->width;
    rect_bg.h = bg_image_info->width;

    dma2d_rectangle_intersect(&rect_draw, &rect_fg, &rect_bg);

    out_layer_param.data = bg_image_info->addr + ((unsigned int)bg_image_info) + rect_draw.y * bg_image_info->stride + rect_draw.x * 4;
    out_layer_param.w = rect_draw.w;
    out_layer_param.h = rect_draw.h;
    out_layer_param.stride = bg_image_info->stride;
    out_layer_param.format = bg_image_info->format;
    out_layer_param.alpha_inv = 0;
    out_layer_param.rbs = 0;

    if (fg_image_info->rle_format) {
        struct rle_info *rle_info = (struct rle_info *)(fg_image_info->addr + ((unsigned int)fg_image_info));
        unsigned int stride = 0;

        switch (rle_info->block) {
        case 0:
            stride = ((fg_image_info->width + 63) / 64) * 4;
            break;
        case 1:
            stride = ((fg_image_info->width + 7) / 8) * 4;
            break;
        case 2:
            stride = ((fg_image_info->width + 255) / 256) * 4;
            break;
        case 3:
            stride = ((fg_image_info->width + 15) / 16) * 4;
            break;
        }

        fg_layer_param.data = fg_image_info->addr + ((unsigned int)fg_image_info) + sizeof(struct rle_info);
        fg_layer_param.stride = stride;
        fg_layer_param.format = fg_image_info->format;
        fg_layer_param.rle_en = fg_image_info->rle_format;
        fg_layer_param.alpha = 255;
        fg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
        fg_layer_param.alpha_inv = 0;
        if (fg_image_info->format == 6 || fg_image_info->format == 7) {
            fg_layer_param.rbs = fg_image_info->yuv_format;
        } else {
            fg_layer_param.rbs = 0;
        }
        fg_layer_param.color = dma2d_pack_pixel(255, 255, 0, 0, JLDMA2D_FORMAT_ARGB8888);
        fg_layer_param.clut = fg_image_info->lut_addr + ((unsigned int)fg_image_info);
        fg_layer_param.clut_size = fg_image_info->lut_len;
        fg_layer_param.clut_format = fg_image_info->lut_format;
        fg_layer_param.image_size = fg_image_info->len - sizeof(struct rle_info);
    } else {
        fg_layer_param.data = fg_image_info->addr + ((unsigned int)fg_image_info);
        fg_layer_param.stride = fg_image_info->stride;
        fg_layer_param.format = fg_image_info->format;
        fg_layer_param.rle_en = fg_image_info->rle_format;
        fg_layer_param.alpha = 255;
        fg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
        fg_layer_param.alpha_inv = 0;
        if (fg_image_info->format == 6 || fg_image_info->format == 7) {
            fg_layer_param.rbs = fg_image_info->yuv_format;
        } else {
            fg_layer_param.rbs = 0;
        }

        fg_layer_param.color = dma2d_pack_pixel(255, 255, 0, 0, JLDMA2D_FORMAT_ARGB8888);
        fg_layer_param.clut = fg_image_info->lut_addr + ((unsigned int)fg_image_info);
        fg_layer_param.clut_size = fg_image_info->lut_len;
        fg_layer_param.clut_format = fg_image_info->lut_format;
        fg_layer_param.image_size = fg_image_info->len;
    }

    bg_layer_param.data = bg_image_info->addr + ((unsigned int)bg_image_info) + rect_draw.y * bg_image_info->stride + rect_draw.x * 4;
    bg_layer_param.stride = bg_image_info->stride;
    bg_layer_param.format = JLDMA2D_FORMAT_ARGB8888;
    bg_layer_param.alpha = 255;
    bg_layer_param.alpha_mode = JLDMA2D_NO_MODIF_ALPHA;
    bg_layer_param.alpha_inv = 0;
    bg_layer_param.rbs = 0;
    bg_layer_param.color = 0;
    bg_layer_param.clut = 0;
    bg_layer_param.clut_size = 0;
    bg_layer_param.clut_format = 0;

    dma2d_reset_all_regs();
    dma2d_create_task();
    dma2d_set_fg_layer(&fg_layer_param);
    dma2d_set_bg_layer(&bg_layer_param);
    dma2d_set_out_layer(&out_layer_param);
    dma2d_set_mode(JLDMA2D_M2M_BLEND);

    dma2d_set_ypnd(1, 48);

    dma2d_run();
    dma2d_wait_done();

    //snprintf(outfile, 128, "out_%s_%d.png", __func__, fg_layer_param.format);
    //stbi_write_png(outfile, IMG_WIDTH, IMG_HEIGHT, 32 / 8, bg_image_info->addr + ((unsigned int)bg_image_info), IMG_WIDTH * 32 / 8);
#if !RLE_TEST_ON_PC
    DcuInvalidRegion(bg_image_info->addr + ((unsigned int)bg_image_info), bg_image_info->stride * bg_image_info->height);
#endif
    FILE *out_fp = NULL;
#if RLE_TEST_ON_PC
    out_fp = fopen("out_rle.bin", "wb");
#else
    out_fp = fopen(CONFIG_STORAGE_PATH"/C/out_rle.bin", "wb");
#endif
    printf("stride=%d,height=%d", bg_image_info->stride, bg_image_info->height);
    fwrite(bg_image_info->addr + ((unsigned int)bg_image_info), 1, bg_image_info->stride * bg_image_info->height, out_fp);

    fclose(out_fp);

    free_image(fg_image_info);

    free_image(bg_image_info);


}
#if RLE_TEST_ON_PC
int main(int argc, char *argv[])
{
    test_rle(argv[1], argv[2]);

    return 0;
}
#else
int rle_test()
{
    puts(">>> rle test\n");
    //char* fg_file = CONFIG_STORAGE_PATH"/C/fg.bin";
    char *fg_file = CONFIG_STORAGE_PATH"/C/fg.rle";
    char *bg_file = CONFIG_STORAGE_PATH"/C/bg.bin";
    test_rle(bg_file, fg_file);

    os_time_dly(100);

    return 0;
}
#endif
