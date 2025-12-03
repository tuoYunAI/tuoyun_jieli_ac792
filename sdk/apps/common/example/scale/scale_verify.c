
#include "scale_quick_sort.h"

#define SCALE_VERIFY_ON_PC  0

#if SCALE_VERIFY_ON_PC
#include "jl_scale_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
extern struct scaler_rcf *create_task();
#else
#include "asm/jl_scale_driver.h"
#include "fs/fs.h"
#define CONFIG_STORAGE_PATH  "storage/sd0"
#endif
typedef struct {
    int src_w;
    int src_h;
    int src_stride;
    int dst_w;
    int dst_h;
    int dst_stride;
    int channel;
    int x_step;
    int y_step;
    int x_offset;
    int y_offset;
    int interp;
} scale_param_t;

#define SCALE_TEST_NUM_MAX 512

#define SCALE_TEST_WIDTH_MAX 1920
#define SCALE_TEST_HEIGHT_MAX 1080

#define SCALE_TASK_MAX 1
#define SCALE_TEST_SIZE_MAX (SCALE_TEST_WIDTH_MAX*SCALE_TEST_HEIGHT_MAX*4)

static char g_utest_names[SCALE_TEST_NUM_MAX][16];
static char *g_utest_names_ptr[SCALE_TEST_NUM_MAX];
static char *g_utest_failed[SCALE_TEST_NUM_MAX];
static int ut_total;
static int ut_failed_count;
static hw_channel_map[5] = {
    0, 0, 0, 1, 2
};

static scale_param_t scale_param;
static int task_len;

#if SCALE_VERIFY_ON_PC
static uint8_t input_image[SCALE_TASK_MAX][SCALE_TEST_SIZE_MAX];
static uint8_t output_image[SCALE_TASK_MAX][SCALE_TEST_SIZE_MAX];
static uint8_t expect_image[SCALE_TASK_MAX][SCALE_TEST_SIZE_MAX];

#else
static uint8_t input_image[SCALE_TASK_MAX][SCALE_TEST_SIZE_MAX]__attribute__((aligned(32)));
static uint8_t output_image[SCALE_TASK_MAX][SCALE_TEST_SIZE_MAX]__attribute__((aligned(32)));
static uint8_t expect_image[SCALE_TASK_MAX][SCALE_TEST_SIZE_MAX]__attribute__((aligned(32)));
#endif

static int dst_stride[SCALE_TASK_MAX];
static int dst_h[SCALE_TASK_MAX];


static char file_name_buf0[200];
static char file_name_buf1[200];
static char file_name_buf2[200];
static char file_name_buf3[200];
static char file_name_buf4[200];

#if SCALE_VERIFY_ON_PC
char *indir = "../jl_scale_uinttest/ISPUT_PURECOLOR";
#else
char *indir = CONFIG_STORAGE_PATH"/C/ISPUT_PURECOLOR";
#endif

#if SCALE_VERIFY_ON_PC
#include "dirent.h"
static int get_all_utests(char *dirname)
{
    struct dirent *de;  // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(dirname);

    if (dr == NULL) { // opendir returns NULL if couldn't open directory
        printf("Could not open current directory");
        return 0;
    }

    ut_total = 0;

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    // for readdir()
    while ((de = readdir(dr)) != NULL) {
        printf("%s\n", de->d_name);

        if (strcmp(".", de->d_name) != 0
            && strcmp("..", de->d_name) != 0
            && de->d_type & DT_DIR) {
            strcpy(g_utest_names[ut_total], de->d_name);
            g_utest_names_ptr[ut_total] = g_utest_names[ut_total];

            ut_total++;
        }
    }

    closedir(dr);
    return 0;
}

#else

static int get_all_utests(char *dirname)
{
    char name[64];
    struct vfscan *fs = NULL;
    FILE *fp = NULL;

    fs = fscan(dirname, "-d -sn", 1);
    if (!fs) {
        printf("fscan failed\n");
        return -1;
    }

    fp = fselect(fs, FSEL_FIRST_FILE, 0);

    ut_total = 0;
    while (fp) {

        fget_name(fp, (uint8_t *)name, 16);

        fclose(fp);

        strcpy(g_utest_names[ut_total], name);
        g_utest_names_ptr[ut_total] = g_utest_names[ut_total];

        ut_total++;
        fp = fselect(fs, FSEL_NEXT_FILE, 0);
        printf("name:%s\n", name);
    }

    fscan_release(fs);

    return 0;
}

#endif
static int store_rgb_to_sdcard_by_name_id(uint8_t *buf, uint32_t size, char *name, int id)
{
    FILE *fp;
#if SCALE_VERIFY_ON_PC
    sprintf((char *)file_name_buf4, "../output/out%s_%d.bin", name, id);
    fp = fopen((char *)file_name_buf4, "wb");
#else
    sprintf((char *)file_name_buf4, "%s/out%s_%d.bin", indir, name, id);
    //fp = fopen((char*)file_name_buf4, "w+");
    fp = fopen((char *)file_name_buf4, "wb");
#endif
    printf("outfile :%s\n", file_name_buf4);
    if (fp != 0) {
#if 1//SCALE_VERIFY_ON_PC
        fwrite(buf, 1, size, fp);
#else
        fwrite(fp, buf, size);
#endif
        fclose(fp);
    } else {
        printf("open file failed\n");
    }
    return 0;
}

static int image_compare(uint8_t *expect_out, uint8_t *real_out, int size)
{
    for (int k = 0; k < size; k++) {
        if (expect_out[k] != real_out[k]) {
            return -1;
        }
    }
    return 0;
}


int scale_utest_run_one_test(char *params_file, char *input_file, char *name)
{
    int ret = 0;

    FILE *fp_p;
    FILE *fp_i;
    FILE *fp_e;
#if 1//SCALE_VERIFY_ON_PC
    fp_p = fopen(params_file, "rb");
    if (fp_p == NULL) {
        printf("params file is not exist\n");
    }
    fread(&task_len, 1, 4, fp_p);
#else
    fp_p = fopen(params_file, "r");
    if (fp_p == NULL) {
        printf("params file is not exist\n");
    }
    fread(fp_p, &task_len, 4);
#endif
#if !SCALE_VERIFY_ON_PC
    DcuFlushRegion(&task_len, 4);
#endif
#if 1//SCALE_VERIFY_ON_PC
    fp_i = fopen(input_file, "rb");
#else
    fp_i = fopen(input_file, "r");
#endif
    if (fp_i == NULL) {
        printf("input_file is not exist\n");
    }
    scale_init();
#if SCALE_VERIFY_ON_PC
    reset_all_regs();
#endif
    for (int k = 0; k < task_len; k++) {
        memset(input_image[k], 0, scale_param.src_stride * scale_param.src_h);
        memset(output_image[k], 0, scale_param.dst_stride * scale_param.dst_h);
#if 1//SCALE_VERIFY_ON_PC
        fread(&scale_param, 1, 48, fp_p);
        fread(input_image[k], 1, scale_param.src_stride * scale_param.src_h, fp_i);
#else
        fread(fp_p, &scale_param, 48);
        fread(fp_i, input_image[k], scale_param.src_stride * scale_param.src_h);
#endif
#if !SCALE_VERIFY_ON_PC
        DcuFlushRegion(&scale_param, 48);
        DcuFlushRegion(input_image[k], scale_param.src_stride * scale_param.src_h);
#endif
        printf("src_w=%d,src_h=%d,src_stride=%d\n", scale_param.src_w, scale_param.src_h, scale_param.src_stride);
        printf("dst_w=%d,dst_h=%d,dst_stride=%d\n", scale_param.dst_w, scale_param.dst_h, scale_param.dst_stride);
        printf("x_offset=%d,y_offset=%d\n", scale_param.x_offset, scale_param.y_offset);
        printf("x_step=%d,y_step=%d\n", scale_param.x_step, scale_param.y_step);
        printf("channel=%d,interp=%d\n", scale_param.channel, scale_param.interp);
        set_scale_param(input_image[k], scale_param.src_w, scale_param.src_h, scale_param.src_stride, output_image[k],
                        scale_param.dst_w, scale_param.dst_h, scale_param.dst_stride, hw_channel_map[scale_param.channel], scale_param.x_step,
                        scale_param.y_step, scale_param.x_offset, scale_param.y_offset, scale_param.interp);
        dst_stride[k] = scale_param.dst_stride;
        dst_h[k] = scale_param.dst_h;

#if SCALE_VERIFY_ON_PC
        //task_len always 1
        if (k != task_len - 1) {
            create_task();
        }
#endif
#if !SCALE_VERIFY_ON_PC
        DcuFlushinvRegion(output_image[k], dst_stride[k] * dst_h[k]);
#endif
    }
    fclose(fp_p);
    fclose(fp_i);
    printf("scale run\n");
    printf("scale_con=%d\n", scale_con);
    scale_run();
    printf("scale start\n");
    printf("scale_con=%d\n", scale_con);
    scale_wait_done();
    printf("scale finish\n");
    for (int k = 0; k < task_len; k++) {
        store_rgb_to_sdcard_by_name_id(output_image[k], dst_stride[k] * dst_h[k], name, k + 1);
#if 0//!SCALE_VERIFY_ON_PC
        //DcuInvalidRegion(output_image[k], dst_stride[k] * dst_h[k])
#endif
        //compare with expected image
#if SCALE_VERIFY_ON_PC
        sprintf((char *)file_name_buf3, "%s/%s/expect%d.bin", indir, name, k + 1);
        fp_e = fopen(file_name_buf3, "rb");
        if (fp_e == NULL) {
            printf("expect_file is not exist\n");
        }
        fread(expect_image[k], 1, dst_stride[k] * dst_h[k], fp_e);
#else
        sprintf((char *)file_name_buf3, "%s/%s/expect%d.bin", indir, name, k + 1);
        //fp_e = fopen(file_name_buf3, "r");
        fp_e = fopen(file_name_buf3, "rb");
        if (fp_e == NULL) {
            printf("expect_file is not exist\n");
        }
        //fread(fp_e,expect_image[k],dst_w[k] * dst_h[k] * dst_c[k]);
        fread(expect_image[k], 1, dst_stride[k] * dst_h[k], fp_e);
#endif
        fclose(fp_e);
        ret = image_compare(expect_image[k], output_image[k], dst_stride[k] * dst_h[k]);

        if (ret != 0) {
            printf("%s:unit test failed,task_num=%d\n", name, k + 1);
            return -1;
        }
    }
    scale_free();


    printf("%s:unit test successed!\n", name);

    return 0;
}



int scale_utest_run_from_sdcard()
{
    int ret = 0;
    char name[64];

    get_all_utests(indir);

    scale_quick_sort((void **)g_utest_names_ptr, 0, ut_total - 1, scale_compare_string);


    for (int i = 0; i < ut_total; i++) {
        printf("sorted name:%s\n", g_utest_names_ptr[i]);
    }

    ut_failed_count = 0;
    for (int i = 0; i < ut_total; i++) {
        strcpy(name, g_utest_names_ptr[i]);

        sprintf((char *)file_name_buf0, "%s/%s/params.bin", indir, name);
        sprintf((char *)file_name_buf1, "%s/%s/image.bin", indir, name);
        printf("param_file:%s\n", file_name_buf0);
        ret = scale_utest_run_one_test(file_name_buf0, file_name_buf1, name);

        if (ret < 0) {
            g_utest_failed[ut_failed_count++] = g_utest_names_ptr[i];
        }
    }

    printf("failed test picture count: %d \n", ut_failed_count);
    for (int i = 0; i < ut_failed_count; i++) {
        printf("failed test picture name: %s\n", g_utest_failed[i]);
    }

    return 0;

}



int scale_verify()
{
    int ret = 0;

    printf("scale_verify start\n");

    scale_utest_run_from_sdcard();

    printf("scale_verify exit\n");
    return 0;
}

#if SCALE_VERIFY_ON_PC
void main()
{
    scale_verify();
}
#endif
