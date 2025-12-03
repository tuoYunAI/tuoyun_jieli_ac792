
#include "dma2d_quick_sort.h"

#define AUTORUNNER_ON_PC    0


#if AUTORUNNER_ON_PC
#include "dma2d_driver.h"
#include "jldma2d.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "dirent.h"
#define NO_CACHE_ADDR(x)    (x)
#else
#include "asm/dma2d_driver.h"
#include "asm/jldma2d.h"
#include "fs/fs.h"
#define CONFIG_STORAGE_PATH  "storage/sd0"
#endif

#define DMA2D_TEST_NUM_MAX 1024
static char g_utest_names[DMA2D_TEST_NUM_MAX][16];
static char *g_utest_names_ptr[DMA2D_TEST_NUM_MAX];
static char *g_utest_failed[DMA2D_TEST_NUM_MAX];
static int ut_total;
static int ut_failed_count;

typedef struct {
    uint8_t *fb;
    int fb_w;
    int fb_h;
    int fb_stride;
    int image_buf_size;
} header_info_t;



#if AUTORUNNER_ON_PC

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

int image_compare(char *outbuf, char *golden_buf, int stride, int height)
{

    for (int i = 0; i < height * stride; i++) {

        if (outbuf[i] != golden_buf[i]) {
            return -1;
        }
    }
    return 0;
}

static int run(char *infile, char *goldenfile, char *outfile)
{
    FILE *infp = NULL;
    FILE *goldenfp = NULL;
    FILE *outfp = NULL;
    int infp_size = 0;
    int goldenfp_size = 0;
    int parser_offset = 0;

    uint8_t *param_buf = NULL;
    uint8_t *image_buf_start = NULL;

    uint8_t *golden_buf = NULL;

    struct dma2d_rcf *cur_task = NULL;

    header_info_t *header_info = NULL;

    int ret = 0;

    infp = fopen(infile, "rb");
    if (!infp) {
        printf("open infile failed\n");
        return -1;
    }

    fseek(infp, 0, SEEK_END);
    infp_size = ftell(infp);

    fseek(infp, 0, SEEK_SET);

    param_buf = malloc(infp_size);

    fread(param_buf, 1, infp_size, infp);
    fclose(infp);
#if !AUTORUNNER_ON_PC
    DcuFlushRegion(param_buf, infp_size);
#endif

    goldenfp = fopen(goldenfile, "rb");
    if (!goldenfp) {
        free(param_buf);
        printf("open goldenfile failed\n");
        return -1;
    }

    fseek(goldenfp, 0, SEEK_END);
    goldenfp_size = ftell(goldenfp);

    fseek(goldenfp, 0, SEEK_SET);

    golden_buf = malloc(goldenfp_size);

    fread(golden_buf, 1, goldenfp_size, goldenfp);
    fclose(goldenfp);
#if !AUTORUNNER_ON_PC
    DcuFlushRegion(golden_buf, goldenfp_size);
#endif
    dma2d_init();



    parser_offset = 0;

    header_info = (header_info_t *)param_buf;

    //printf("w=%d,h=%d,stride=%d,size=%d\n", header_info->fb_w, header_info->fb_h, header_info->fb_stride, header_info->image_buf_size);

    parser_offset += sizeof(header_info_t);

    image_buf_start = (uint8_t *)(param_buf + parser_offset);

    header_info->fb += (unsigned int)image_buf_start;

    parser_offset += header_info->image_buf_size;
#if !AUTORUNNER_ON_PC
    //DcuFlushRegion(image_buf_start, header_info->image_buf_size);
#endif
    while (parser_offset < infp_size) {
        cur_task = dma2d_create_task();

        memcpy(cur_task, &param_buf[parser_offset], sizeof(struct dma2d_rcf));
        //cur_task->public_con0l.width = 31;
        //cur_task->public_con0l.high = 32;

        //cur_task->bg_xyinith.instride = 64;
        //cur_task->out_con0l.outstride = cur_task->bg_xyinith.instride;
        //cur_task->bg_con0l.color_mode = 20;
        //cur_task->out_con0l.color_mode = 20;
        printf("m2m_mode=%d,blend_en=%d,blend_mode=%d,paint_mode=%d\n",
               cur_task->public_con0h.m2m_mode,
               cur_task->public_con0h.blend_en,
               cur_task->public_con0h.blend_mode,
               cur_task->fg_con0l.paint_mode);
        printf("width=%d,height=%d,fg_stride=%d,bg_stride=%d,out_stride=%d\n",
               cur_task->public_con0l.width,
               cur_task->public_con0l.high,
               cur_task->fg_xyinith.instride,
               cur_task->bg_xyinith.instride,
               cur_task->out_con0l.outstride);
        //printf("fg_alpha_mode=%d,bg_alpha_mode=%d\n", cur_task->fg_con0l.alpha_mode, cur_task->fg_con0l.alpha_mode);
        printf("fg_alpha_inv=%d,fg_print_mode=%d,fg_color_mode=%d,fg_endian=%d,fg_lr=%d,fg_rbs=%d,fg_rle_en=%d,fg_alpha=%d,fg_alpha_mode=%d,fg_color_key=%d\n",
               cur_task->fg_con0l.alpha_inv,
               cur_task->fg_con0l.paint_mode,
               cur_task->fg_con0l.color_mode,
               cur_task->fg_con0l.endian,
               cur_task->fg_con0l.inpfc_lr,
               cur_task->fg_con0l.inpfc_rbs,
               cur_task->fg_con0l.rle_en,
               cur_task->fg_con0l.alpha,
               cur_task->fg_con0l.alpha_mode,
               cur_task->fg_con0l.color_key);
        printf("bg_alpha_inv=%d,bg_print_mode=%d,bg_color_mode=%d,bg_endian=%d,bg_lr=%d,bg_rbs=%d,bg_alpha=%d,bg_alpha_mode=%d,bg_color_key=%d\n",
               cur_task->bg_con0l.alpha_inv,
               cur_task->bg_con0l.paint_mode,
               cur_task->bg_con0l.color_mode,
               cur_task->bg_con0l.endian,
               cur_task->bg_con0l.inpfc_lr,
               cur_task->bg_con0l.inpfc_rbs,
               cur_task->bg_con0l.alpha,
               cur_task->bg_con0l.alpha_mode,
               cur_task->bg_con0l.color_key);
        printf("out_alpha_inv=%d,out_color_mode=%d,out_den=%d,out_lr=%d,out_rbs=%d,out_stride=%d\n",
               cur_task->out_con0l.alpha_inv,
               cur_task->out_con0l.color_mode,
               cur_task->out_con0l.dth_en,
               cur_task->out_con0l.outpfc_lr,
               cur_task->out_con0l.outpfc_rbs,
               cur_task->out_con0l.outstride);
        printf("fg_lut_color_mode=%d,fg_lut_en=%d,fg_lut_len=%d\n",
               cur_task->fg_clutl.color_mode,
               cur_task->fg_clutl.en,
               cur_task->fg_clutl.len);
        printf("bg_lut_color_mode=%d,bg_lut_en=%d,bg_lut_len=%d\n",
               cur_task->bg_clutl.color_mode,
               cur_task->bg_clutl.en,
               cur_task->bg_clutl.len);
        //relocation
        cur_task->out_con0h.base_adr += (unsigned int)image_buf_start;
        if (cur_task->public_con0h.blend_en == 0
            && cur_task->public_con0h.m2m_mode == 0
            && cur_task->fg_con0l.paint_mode == 1) {
            printf("fg_rbs=%d", cur_task->fg_con0l.inpfc_rbs);
            //cur_task->fg_con0l.inpfc_rbs = 0;
            uint32_t color = cur_task->fg_con0h.color;
            uint8_t b = color & 0xff;
            uint8_t g = (color >> 8) & 0xff;
            uint8_t r = (color >> 16) & 0xff;
            uint8_t a = (color >> 24) & 0xff;
            printf("a=%d,r=%d,g=%d,b=%d\n", a, r, g, b);
            printf("fg_color_mode=%d,bg_color_mode=%d,out_color_mode=%d", cur_task->fg_con0l.color_mode, cur_task->bg_con0l.color_mode, cur_task->out_con0l.color_mode);
            //fill mode
            printf("color_mode=%d,color=%d", cur_task->out_con0l.color_mode, cur_task->fg_con0h.color);

        } else if (cur_task->public_con0h.blend_en == 0
                   && cur_task->public_con0h.m2m_mode == 1) {
            printf("bg_color_mode=%d,out_color_mode=%d", cur_task->bg_con0l.color_mode, cur_task->out_con0l.color_mode);
            printf("bg_stride=%d,out_stride=%d\n",
                   cur_task->bg_xyinith.instride,
                   cur_task->out_con0l.outstride);
            //copy mode
            cur_task->bg_base_adr.base_adr += (unsigned int)image_buf_start;

        } else if (cur_task->public_con0h.blend_en == 0
                   && cur_task->public_con0h.m2m_mode == 0) {
            //int convert_color[4] = { 0,1,4,5 };
            //cur_task->fg_clutl.color_mode = convert_color[cur_task->fg_clutl.color_mode];
            printf("fg_lut_color_mode=%d,fg_color_mode=%d,out_color_mode=%d", cur_task->fg_clutl.color_mode, cur_task->fg_con0l.color_mode, cur_task->out_con0l.color_mode);
            printf("fg_stride=%d,out_stride=%d\n",
                   cur_task->fg_xyinith.instride,
                   cur_task->out_con0l.outstride);
            //convert mode
            cur_task->fg_base_adr.base_adr += (unsigned int)image_buf_start;

        } else {
            //int convert_color[4] = { 0,1,4,5 };
            //cur_task->fg_clutl.color_mode = convert_color[cur_task->fg_clutl.color_mode];
            //cur_task->bg_clutl.color_mode = convert_color[cur_task->bg_clutl.color_mode];
            printf("fg_lut_color_mode=%d,fg_color_mode=%d,bg_lut_color_mode=%d,bg_color_mode=%d,out_color_mode=%d",
                   cur_task->fg_clutl.color_mode,
                   cur_task->fg_con0l.color_mode,
                   cur_task->bg_clutl.color_mode,
                   cur_task->bg_con0l.color_mode,
                   cur_task->out_con0l.color_mode);
            printf("fg_stride=%d,bg_stride=%d,out_stride=%d\n",
                   cur_task->fg_xyinith.instride,
                   cur_task->bg_xyinith.instride,
                   cur_task->out_con0l.outstride);
            //blend mode
            cur_task->fg_base_adr.base_adr += (unsigned int)image_buf_start;
            cur_task->bg_base_adr.base_adr += (unsigned int)image_buf_start;


        }

        if (cur_task->fg_cluth.adr) {
            cur_task->fg_cluth.adr += (unsigned int)image_buf_start;
        }

        if (cur_task->bg_cluth.adr) {
            cur_task->bg_cluth.adr += (unsigned int)image_buf_start;
        }
        //add
        //printf("cur_task->out_con0h.base_adr0=%x", cur_task->out_con0h.base_adr);
        cur_task->out_con0h.base_adr = NO_CACHE_ADDR(cur_task->out_con0h.base_adr);
        //printf("cur_task->out_con0h.base_adr1=%x", cur_task->out_con0h.base_adr);
        cur_task->bg_base_adr.base_adr = NO_CACHE_ADDR(cur_task->bg_base_adr.base_adr);
        cur_task->fg_base_adr.base_adr = NO_CACHE_ADDR(cur_task->fg_base_adr.base_adr);
        cur_task->fg_cluth.adr = NO_CACHE_ADDR(cur_task->fg_cluth.adr);
        cur_task->bg_cluth.adr = NO_CACHE_ADDR(cur_task->bg_cluth.adr);

#if !AUTORUNNER_ON_PC
        DcuFlushRegion(header_info, sizeof(header_info));
#endif
        cur_task->task_adr.task_adr = NULL;

        parser_offset += sizeof(struct dma2d_rcf);
    }
#if !AUTORUNNER_ON_PC
    DcuFlushinvRegion(header_info->fb, header_info->fb_stride * header_info->fb_h);
#endif
    dma2d_run();
    dma2d_wait_done();

    //write output to file
    outfp = fopen(outfile, "wb");
    printf("outfile :%s\n", outfile);
    if (!outfp) {

        free(param_buf);
        free(golden_buf);

        printf("open outfile failed\n");
        return -1;
    }
#if 0//!AUTORUNNER_ON_PC
    printf("header_info->fb_addr0=%x", header_info->fb);
    DcuInvalidRegion(header_info->fb, header_info->fb_stride * header_info->fb_h);
    //DcuFlushRegion(header_info->fb, header_info->fb_stride * header_info->fb_h);
    printf("header_info->fb_addr1=%x", header_info->fb);
    printf("dam2d finish\n");
#endif
    fwrite(header_info->fb, 1, header_info->fb_stride * header_info->fb_h, outfp);
    fclose(outfp);
    //printf("write finish\n");
    ret = image_compare(header_info->fb, golden_buf, header_info->fb_stride, header_info->fb_h);

    free(param_buf);
    free(golden_buf);

    if (ret < 0) {
        printf("image compare failed\n");
        return -1;
    }

    return 0;
}

static void autorunner(char *indir, char *outdir)
{
    char name[64];
    char param_file[64];
    char golden_file[64];
    char output_file[64];

    int ret = 0;

    get_all_utests(indir);

    dma2d_quick_sort((void **)g_utest_names_ptr, 0, ut_total - 1, dma2d_compare_string);


    for (int i = 0; i < ut_total; i++) {
        printf("sorted name:%s\n", g_utest_names_ptr[i]);
    }

    ut_failed_count = 0;
    //dma2d_init();
    for (int i = 0; i < ut_total; i++) {

        strcpy(name, g_utest_names_ptr[i]);
        sprintf((char *)param_file, "%s/%s/param.bin", indir, name);
        sprintf((char *)golden_file, "%s/%s/golden.bin", indir, name);
        sprintf((char *)output_file, "%s/%s_out.bin", outdir, name);

        printf("param_file:%s\n", param_file);
        ret = run(param_file, golden_file, output_file);

        if (ret < 0) {
            g_utest_failed[ut_failed_count++] = g_utest_names_ptr[i];
        }
    }
    printf("utest failed count: %d \n", ut_failed_count);
    for (int i = 0; i < ut_failed_count; i++) {
        printf("utest failed name: %s\n", g_utest_failed[i]);
    }

}

#if AUTORUNNER_ON_PC
int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("parameters missing\n");
        return -1;
    }

    char *indir = argv[1];
    char *outdir = argv[2];
    autorunner(indir, outdir);

    return 0;
}
#else
int dma2d_verify()
{
    printf("dma2d_verify start\n");
    char *indir = CONFIG_STORAGE_PATH"/C/ISPUT_PURECOLOR";
    char *outdir = CONFIG_STORAGE_PATH"/C/ISPUT_PURECOLOR";
    autorunner(indir, outdir);
    printf("dma2d_verify exit\n");
    return 0;
}
#endif
