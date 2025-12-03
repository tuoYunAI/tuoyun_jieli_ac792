

#include "asm/jl_scale_driver.h"
#include "fs/fs.h"

#define JLSCALER_FRACTION_WIDTH     (14)
#define CONFIG_STORAGE_PATH  "storage/sd0"

//static uint8_t	src_img_adr[32*128] sec(.volatile_ram);
//static uint8_t	dst_img_adr[160*120] sec(.volatile_ram);

void test_scale()
{
    char outfile[128];

    int src_w = 640;
    int src_h = 480;
    int channel = 4;
#if  1
    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel);

    //DcuFlushRegion(src_img_adr, src_w*src_h*channel);
    if (!src_img_adr) {
        printf("Failed to load src image.\n");
    }
    FILE *fp = fopen(CONFIG_STORAGE_PATH"/C/src.bin", "rb");
    if (fp == NULL) {
        printf("file is not exist\n");
    }
    fread(src_img_adr, 1, src_w * src_h * channel, fp);
    fclose(fp);
    //DcuInvalidRegion(src_img_adr,src_w * src_h * channel);
    //memset(src_img_adr,255,src_w*src_h*channel);
    DcuFlushRegion(src_img_adr, src_w * src_h * channel);
#else
    for (int k = 0; k < src_w * src_h * channel; k++) {
        src_img_adr[k] = 1;//k*2;
    }
#endif
    //memset(src_img_adr,0,src_w*src_h*channel);
    uint32_t dst_w = 1280;
    uint32_t dst_h = 720;

#if 1
    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel);
    if (!dst_img_adr) {
        return;
    }
#endif
    memset(dst_img_adr, 0, dst_w * dst_h * channel);


    uint32_t x_step = 0.3f * (1 << JLSCALER_FRACTION_WIDTH);
    uint32_t y_step = 0.3f * (1 << JLSCALER_FRACTION_WIDTH);
    //uint32_t x_step = 1 * (1 << JLSCALER_FRACTION_WIDTH);
    //uint32_t y_step = 1 * (1 << JLSCALER_FRACTION_WIDTH);

    int32_t x_offset = 0;// -3.0f * (1 << JLSCALER_FRACTION_WIDTH);
    int32_t y_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);

    printf("ssssssssssss");
    scale_init();
    printf("scale_init");
    //reset_all_regs();

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };

    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = 1;//JLSCALER_INTERP_NEAREST;// JLSCALER_INTERP_BILINEAR;
#if 0
    printf("src_adr=%x,dst_adr=%x", src_img_adr, dst_img_adr);
    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                    dst_img_adr, dst_w, dst_h, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);
    printf("set_param finish");
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel);
    scale_run();
    printf("scale_run finish");
    scale_wait_done();
    printf("scale wait done finish");
#elif 0
    int offset = 0;
    int h_num = 16;
    int count = dst_h / h_num;
    for (int k = 0; k < count; k++) {
        offset = k * h_num * dst_w * channel;
        y_offset = k * h_num * y_step;
        set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                        dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);


        DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel);
        scale_run();
        scale_wait_done();
    }
#else
    int offset = 0;
    int w_num = 16;
    int count = dst_w / w_num;
    for (int k = 0; k < count; k++) {
        offset = k * w_num * channel;
        x_offset = k * w_num * x_step;
        set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                        dst_img_adr + offset, w_num, dst_h, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);

        DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel);
        scale_run();
        scale_wait_done();
    }
#endif
    //DcuInvalidRegion(dst_img_adr,dst_w * dst_h * channel);
    //os_time_dly(50);
    printf("cache done");
#if 1
    FILE *out_fp = NULL;

    out_fp = fopen(CONFIG_STORAGE_PATH"/C/out.bin", "wb");
    if (out_fp == NULL) {
        printf("out file is not exist\n");
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel, out_fp);

    fclose(out_fp);
#endif
    printf("save done");
    free(src_img_adr);
    free(dst_img_adr);

    scale_free();
    printf("free done");
    printf("free done");
}


