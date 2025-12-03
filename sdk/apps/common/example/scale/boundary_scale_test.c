
#define SCALE_BOUNDARY_TEST_PC  0

#if SCALE_BOUNDARY_TEST_PC
#include "jl_scale_driver.h"
#include<stdio.h>
#include<stdlib.h>
#else
#include "asm/jl_scale_driver.h"
#include"fs/fs.h"
#define CONFIG_STORAGE_PATH  "storage/sd0"
#endif


#define SCALE_BOUNDAR_TEST_NUM 2
static int src_w[SCALE_BOUNDAR_TEST_NUM] = { (1 << 14), 2 };
static int src_h[SCALE_BOUNDAR_TEST_NUM] = { 2, 2 };
static int dst_w[SCALE_BOUNDAR_TEST_NUM] = { 2, 1 << 14 };
static int dst_h[SCALE_BOUNDAR_TEST_NUM] = { 2, 2 };
static int x_step[SCALE_BOUNDAR_TEST_NUM] = { (1 << 28) - 1, 1};
static int y_step[SCALE_BOUNDAR_TEST_NUM] = { 1 << 14, 1 << 14 };
static int channel = 1;
void boundary_test_scaler()
{
    printf("boundary_test_scaler\n");
    char outfile[128];
    for (int k = 0; k < SCALE_BOUNDAR_TEST_NUM; k++) {
        uint8_t *src_img_adr = (uint8_t *)malloc(src_w[k] * src_h[k] * channel);
        uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w[k] * dst_h[k] * channel);

        memset(src_img_adr, 0, src_w[k] * src_h[k] * channel);
        memset(dst_img_adr, 0, dst_w[k] * dst_h[k] * channel);

        if (k == 0) {
            src_img_adr[src_w[k] - 1] = 255;
            //src_img_adr[src_w[k] - 2] = 255;
            src_img_adr[2 * src_w[k] - 1] = 255;
            // src_img_adr[2 * src_w[k] - 2] = 255;
        } else {
            src_img_adr[src_w[k] - 1] = 0;
            src_img_adr[src_w[k] - 2] = 255;
            src_img_adr[2 * src_w[k] - 1] = 0;
            src_img_adr[2 * src_w[k] - 2] = 255;
        }
#if !SCALE_BOUNDARY_TEST_PC
        DcuFlushRegion(src_img_adr, src_w[k]* src_h[k]);
#endif

        int32_t x_offset = 0;
        int32_t y_offset = 0;


        scale_init();
#if SCALE_BOUNDARY_TEST_PC
        reset_all_regs();
#endif

        uint32_t interp = 1;// JLSCALER_INTERP_NEAREST;// JLSCALER_INTERP_BILINEAR;

        set_scale_param(src_img_adr, src_w[k], src_h[k], src_w[k] * channel,
                        dst_img_adr, dst_w[k], dst_h[k], dst_w[k] * channel, 0, x_step[k], y_step[k], x_offset, y_offset, interp);

#if !SCALE_BOUNDARY_TEST_PC
        DcuFlushinvRegion(dst_img_adr, dst_w[k] * dst_h[k]);
#endif
        scale_run();
        scale_wait_done();
#if !SCALE_BOUNDARY_TEST_PC
        //DcuInvalidRegion(dst_img_adr, dst_w[k]* dst_h[k]);
        //os_time_dly(50);
#endif
# if SCALE_BOUNDARY_TEST_PC
        sprintf((char *)outfile, "out_%d.bin", k);
#else
        char *indir = CONFIG_STORAGE_PATH"/C";
        sprintf((char *)outfile, "%s/out_%d.bin", indir, k);
#endif
        FILE *fp = fopen(outfile, "wb");
        if (fp == NULL) {
            printf("file is not exist\n");
        }
        fwrite(dst_img_adr, 1, dst_w[k] * dst_h[k] * channel, fp);
        fclose(fp);

        scale_free();

        free(src_img_adr);
        free(dst_img_adr);
    }
    printf("boundary_test finish\n");
}

#if SCALE_BOUNDARY_TEST_PC
int main()
{
    boundary_test_scaler();

    return 0;
}
#endif
