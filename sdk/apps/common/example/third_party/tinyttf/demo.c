#include "app_config.h"
#include "system/includes.h"
#include "storage_device.h"
#include "lcd_config.h"
#include "fs/fs.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_HEAP_FACTOR_SIZE_32 50
#define STBTT_HEAP_FACTOR_SIZE_128 20
#define STBTT_HEAP_FACTOR_SIZE_DEFAULT 10
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u) ((void)(u), free(x))
#define TTF_MALLOC(x) (malloc(x))
#define TTF_FREE(x) (malloc(x))

/*********************
 *      DEFINES
 *********************/
#include "asm/gpu/jlvg.h"
#include "asm/jlgpu_driver.h"

static jlvg_t *jlvg;
static uint32_t begin_time;
static uint32_t end_time;
static uint32_t hw_cfgtime_begin;
static uint32_t hw_cfgtime;
static uint32_t sw_blendtime;
static uint32_t cfg_path_time;

// 需要生成字库文件并烧写到 FLASH
#define CONFIG_FONT_TTF_SUB0 "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub0.ttf"       // 风急天高猿啸哀渚清沙白鸟飞回无边落木萧萧下不尽长江滚滚来万里悲秋常作客百年多病独登台艰难苦恨繁霜鬓潦倒新停浊酒杯
#define CONFIG_FONT_TTF_SUB1 "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub1.ttf"       // 龘
#define CONFIG_FONT_TTF_SUB2 "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub2.ttf"       // 841 个汉字
#define CONFIG_FONT_TTF_TEST "mnt/sdfile/EXT_RESERVED/uipackres/ui/test_zh.ttf"    // 杰理科技股份有限公司

#define GPU_DRAW 1

#define GLYPH_MAX_SEGMENTS 1024 // 字形最大的轮廓数，如果超了就要改大绘制修改字形

#define   BLACK     0x0000       //   黑色
#define   NAVY      0x000F       //    深蓝色
#define   DGREEN    0x03E0       //  深绿色
#define   DCYAN     0x03EF       //   深青色
#define   MAROON    0x7800       //   深红色
#define   PURPLE    0x780F       //   紫色
#define   OLIVE     0x7BE0       //   橄榄绿
#define   LGRAY     0xC618       //  灰白色
#define   DGRAY     0x7BEF       //  深灰色
#define   BLUE      0x001F       //  蓝色
#define   GREEN     0x07E0       //  绿色
#define   CYAN      0x07FF       //  青色
#define   RED       0xF800       //  红色
#define   MAGENTA   0xF81F       //  品红
#define   YELLOW    0xFFE0       //  黄色
#define   WHITE     0xFFFF       //  白色

static u16 char_color[16] = {
    BLACK,
    NAVY,
    DGREEN,
    DCYAN,
    MAROON,
    PURPLE,
    OLIVE,
    LGRAY,
    DGRAY,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE,
};

static u16 back_color[16] = {
    BLACK,
    NAVY,
    DGREEN,
    DCYAN,
    MAROON,
    PURPLE,
    OLIVE,
    LGRAY,
    DGRAY,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE,
};

/* a hydra stream that can be in memory or from a file*/
typedef struct _ttf_cb_stream_t {
    FILE *file;
    const void *data;
    size_t size;
    size_t position;
} ttf_cb_stream_t;

void ttf_cb_stream_read(ttf_cb_stream_t *stream, void *data, size_t to_read);
void ttf_cb_stream_seek(ttf_cb_stream_t *stream, size_t position);

/* for stream support */
#define STBTT_STREAM_TYPE ttf_cb_stream_t *
#define STBTT_STREAM_SEEK(s, x) ttf_cb_stream_seek(s, x);
#define STBTT_STREAM_READ(s, x, y) ttf_cb_stream_read(s, x, y);

/**********************
 *      TYPEDEFS
 **********************/

#include "stb_rect_pack.h"
#include "stb_truetype_htcw.h"

typedef struct _ttf_font_desc_t {
    FILE *file;
    ttf_cb_stream_t stream;
    stbtt_fontinfo info;
    float scale;
    int ascent;
    int descent;
    uint16_t line_height;         /**< The real line height where any text fits*/
    uint16_t base_line;           /**< Base line measured from the top of the line_height*/
    uint8_t *buffer;
} ttf_font_desc_t;

/** Describes the properties of a glyph.*/
typedef struct _font_glyph_dsc_t {
    /*Pointer to the font in a font pack (must have the same line height)*/
    uint16_t adv_w; /**< The glyph needs this space. Draw the next glyph after this width.*/
    uint16_t box_w; /**< Width of the glyph's bounding box*/
    uint16_t box_h; /**< Height of the glyph's bounding box*/
    int16_t ofs_x;  /**< x offset of the bounding box*/
    int16_t ofs_y;  /**< y offset of the bounding box*/
    uint8_t bpp: 4;  /**< Bit-per-pixel: 1, 2, 4, 8*/
    uint8_t is_placeholder: 1; /** Glyph is missing. But placeholder will still be displayed */
} font_glyph_dsc_t;

/* a hydra stream that can be in memory or from a file*/
void ttf_cb_stream_read(ttf_cb_stream_t *stream, void *data, size_t to_read)
{
    if (stream->file != NULL) {
        uint32_t br;
        br = fread(data, 1, to_read, stream->file);
    } else {
        if (to_read + stream->position >= stream->size) {
            to_read = stream->size - stream->position;
        }
        memcpy(data, ((const unsigned char *)stream->data + stream->position), to_read);
        stream->position += to_read;
    }
}

void ttf_cb_stream_seek(ttf_cb_stream_t *stream, size_t position)
{
    if (stream->file != NULL) {
        fseek(stream->file, position, SEEK_SET);
    } else {
        if (position > stream->size) {
            stream->position = stream->size;
        } else {
            stream->position = position;
        }
    }
}

static bool ttf_get_glyph_dsc(ttf_font_desc_t *font_des, font_glyph_dsc_t *glyph_dsc, uint32_t unicode_letter,
                              uint32_t unicode_letter_next)
{
    if (unicode_letter < 0x20 ||
        unicode_letter == 0xf8ff || /*LV_SYMBOL_DUMMY*/
        unicode_letter == 0x200c) { /*ZERO WIDTH NON-JOINER*/
        glyph_dsc->box_w = 0;
        //glyph_dsc->adv_w = 0;
        glyph_dsc->box_h = 0; /*height of the bitmap in [px]*/
        glyph_dsc->ofs_x = 0; /*X offset of the bitmap in [pf]*/
        glyph_dsc->ofs_y = 0; /*Y offset of the bitmap in [pf]*/
        glyph_dsc->bpp = 0;
        glyph_dsc->is_placeholder = false;
        return true;
    }

    int g1 = stbtt_FindGlyphIndex(&(font_des->info), (int)unicode_letter);
    if (g1 == 0) {
        /* Glyph not found */
        return false;
    }
    int x1, y1, x2, y2;

    stbtt_GetGlyphBitmapBox(&(font_des->info), g1, font_des->scale, font_des->scale, &x1, &y1, &x2, &y2);
    int g2 = 0;
    if (unicode_letter_next != 0) {
        g2 = stbtt_FindGlyphIndex(&(font_des->info), (int)unicode_letter_next);
    }
#if 0
    int advw, lsb;
    stbtt_GetGlyphHMetrics(&(font_des->info), g1, &advw, &lsb);
    int k = stbtt_GetGlyphKernAdvance(&(font_des->info), g1, g2);
    glyph_dsc->adv_w = (uint16_t)floor((((float)advw + (float)k) * font_des->scale) +
                                       0.5f); /*Horizontal space required by the glyph in [px]*/
#endif
    glyph_dsc->box_w = (x2 - x1 + 1);         /*width of the bitmap in [px]*/
    glyph_dsc->box_h = (y2 - y1 + 1);         /*height of the bitmap in [px]*/
    //printf("box_w = %d; box_h = %d.", glyph_dsc->box_w, glyph_dsc->box_h);
    glyph_dsc->ofs_x = x1;                    /*X offset of the bitmap in [pf]*/
    glyph_dsc->ofs_y = -y2;                   /*Y offset of the bitmap measured from the as line*/
    //printf("ofs_x = %d; ofs_y = %d.", glyph_dsc->ofs_x, glyph_dsc->ofs_y);
    glyph_dsc->bpp = 8;                       /*Bits per pixel: 1/2/4/8*/
    glyph_dsc->is_placeholder = false;
    return true; /*true: glyph found; false: glyph was not found*/
}

static void ttf_get_glyph_bitmap(ttf_font_desc_t *font_des, uint32_t unicode_letter)
{
    const stbtt_fontinfo *info = (const stbtt_fontinfo *) & (font_des->info);
    int g1 = stbtt_FindGlyphIndex(info, (int)unicode_letter);
    if (g1 == 0) {
        /* Glyph not found */
        return;
    }

    int x1, y1, x2, y2;
    stbtt_GetGlyphBitmapBox(info, g1, font_des->scale, font_des->scale, &x1, &y1, &x2, &y2);

    int32_t w = x2 - x1 + 1;
    int32_t h = y2 - y1 + 1;
    uint32_t stride = w;
    // size_t szb = h * stride;
    //printf("w = %d; h = %d; scale = %f; g1 = %d;", w, h, font_des->scale, g1);
    /*Render into cache*/
    stbtt_MakeGlyphBitmap(info, font_des->buffer, w, h, stride, font_des->scale, font_des->scale, g1);

    return;
}

static void ttf_get_vertex_path(ttf_font_desc_t *font_des, font_glyph_dsc_t *glyph_dsc, uint32_t unicode_letter, uint8_t *segments, float *points, uint32_t *segments_ofs, uint32_t *points_ofs)
{
    const stbtt_fontinfo *info = (const stbtt_fontinfo *) & (font_des->info);
    int g1 = stbtt_FindGlyphIndex(info, (int)unicode_letter);
    if (g1 == 0) {
        /* Glyph not found */
        return;
    }

    float cx1;
    float cy1;
    float cx2;
    float cy2;
    float x;
    float y;

    int advw, lsb;
    stbtt_GetGlyphHMetrics(info, g1, &advw, &lsb);
    int glyph_height;
    int h_ofs = font_des->ascent - advw;
    //printf("ascent = %d; descent = %d, advw = %d; ofs = %d.", font_des->ascent, font_des->descent, advw, ofs);
    if (h_ofs > 0) {
        glyph_height = advw - h_ofs;
    } else {
        glyph_height = advw + h_ofs;
    }

    uint32_t points_num = 0;

    stbtt_vertex *vertices;
    uint32_t get_path_begin = get_system_us();
    int num_verts = stbtt_GetGlyphShape(info, g1, &vertices);

    //printf("ofs_x = %d; ofs_y = %d; box_w = %d; box_h = %d.", glyph_dsc->ofs_x, glyph_dsc->ofs_y, glyph_dsc->box_w, glyph_dsc->box_h);

    for (int i = 0; i < num_verts; i++) {
        if (vertices[i].type == STBTT_vmove) {
            x = (float)(vertices[i].x * font_des->scale);
            y = (float)(glyph_height - vertices[i].y) * font_des->scale;
            points[points_num++] = x;
            points[points_num++] = y;
            segments[i] = VGHW_MOVE_TO;
            //printf("[%d] (x, y) = (%f, %f).", i, x, y);
        } else if (vertices[i].type == STBTT_vline) {
            x = (float)(vertices[i].x * font_des->scale);
            y = (float)(glyph_height - vertices[i].y) * font_des->scale;
            points[points_num++] = x;
            points[points_num++] = y;
            segments[i] = VGHW_LINE_TO;
            //printf("[%d] (x, y) = (%f, %f).", i, x, y);
        } else if (vertices[i].type == STBTT_vcurve) {
            cx1 = (float)(vertices[i].cx * font_des->scale);
            cy1 = (float)(glyph_height - vertices[i].cy) * font_des->scale;
            x = (float)(vertices[i].x * font_des->scale);
            y = (float)(glyph_height - vertices[i].y) * font_des->scale;
            points[points_num++] = cx1;
            points[points_num++] = cy1;
            points[points_num++] = x;
            points[points_num++] = y;
            segments[i] = VGHW_QUAD_TO;
            //printf("[%d] (x, y) = (%f, %f).", i, x, y);
        } else if (vertices[i].type == STBTT_vcubic) {
            cx1 = (float)(vertices[i].cx * font_des->scale);
            cy1 = (float)(glyph_height - vertices[i].cy) * font_des->scale;
            cx2 = (float)(vertices[i].cx1 * font_des->scale);
            cy2 = (float)(glyph_height - vertices[i].y) * font_des->scale;
            x = (float)(vertices[i].x * font_des->scale);
            y = (float)(glyph_height - vertices[i].y) * font_des->scale;
            points[points_num++] = cx1;
            points[points_num++] = cy1;
            points[points_num++] = cx2;
            points[points_num++] = cy2;
            points[points_num++] = x;
            points[points_num++] = y;
            segments[i] = VGHW_CUBIC_TO;
            //printf("[%d] (x, y) = (%f, %f).", i, x, y);
        } else {
            printf("Path resolution error");
            goto exit;
        }
    }

    uint32_t get_path_end = get_system_us();
    cfg_path_time += (get_path_end - get_path_begin);

    *points_ofs = points_num;
    *segments_ofs = num_verts;

exit:
    STBTT_free(vertices, info->userdata);
    return;
}

static void tiny_ttf_set_size(ttf_font_desc_t *font_des, uint16_t font_size)
{
    if (font_size <= 0) {
        printf("invalid font size: %d.", font_size);
        return;
    }

    font_des->scale = stbtt_ScaleForMappingEmToPixels(&(font_des->info), font_size);
    int line_gap = 0;
    stbtt_GetFontVMetrics(&(font_des->info), &font_des->ascent, &font_des->descent, &line_gap);
    font_des->line_height = (uint16_t)(font_des->scale * (font_des->ascent - font_des->descent + line_gap));
    font_des->base_line = (uint16_t)(font_des->scale * (line_gap - font_des->descent));
    //printf("line_gap = %d; line_height = %d; base_line = %d.", line_gap, font_des->line_height, font_des->base_line);
}

static ttf_font_desc_t *tiny_ttf_create(const char *path, const void *data, size_t data_size, uint16_t font_size)
{
    if ((path == NULL && data == NULL) || 0 >= font_size) {
        printf("tiny_ttf: invalid argument\n");
        return NULL;
    }
    ttf_font_desc_t *dsc = (ttf_font_desc_t *)TTF_MALLOC(sizeof(ttf_font_desc_t));
    if (dsc == NULL) {
        printf("tiny_ttf: out of memory\n");
        return NULL;
    }

    if (path != NULL) {
        if (path[0] == 'm') {
            u32 addr;
            u32 data_len;
            FILE *profile_fp = fopen(path, "r");
            if (profile_fp == NULL) {
                printf("tiny_ttf: unable to open %s\n", path);
                goto err_after_dsc;
            }
            struct vfs_attr file_attr;
            fget_attrs(profile_fp, &file_attr);
            addr = file_attr.sclust;
            data_len = flen(profile_fp);
            fclose(profile_fp);

            dsc->stream.file = NULL;
            dsc->stream.data = (const uint8_t *)addr;
            dsc->stream.size = data_len;
            dsc->stream.position = 0;
        } else {
            dsc->file = fopen(path, "r");
            if (dsc->file == NULL) {
                printf("tiny_ttf: unable to open %s\n", path);
                goto err_after_dsc;
            }

            dsc->stream.file = dsc->file;
        }
    } else {
        dsc->stream.file = NULL;
        dsc->stream.data = (const uint8_t *)data;
        dsc->stream.size = data_size;
        dsc->stream.position = 0;
    }

    if (0 == stbtt_InitFont(&(dsc->info), &(dsc->stream), stbtt_GetFontOffsetForIndex(&(dsc->stream), 0))) {
        printf("tiny_ttf: init failed\n");
        goto err_after_dsc;
    }

    tiny_ttf_set_size(dsc, font_size);
    dsc->buffer = malloc(font_size * font_size);
    return dsc;

err_after_dsc:
    TTF_FREE(dsc);
    return NULL;
}

static void tiny_ttf_destroy(ttf_font_desc_t *font_des)
{
    if (font_des != NULL) {
        if (font_des->stream.file != NULL) {
            fclose(font_des->file);
        }
        TTF_FREE(font_des->buffer);
        TTF_FREE(font_des);
    }
}

static void gpu_draw_bitmap(font_glyph_dsc_t *glyph_dsc, u8 char_size, u16 sx, u16 sy, uint8_t *segments, float *points, uint32_t segments_ofs, uint32_t points_ofs, u8 *showbuf)
{
    u16 color;

    static u8 run_time = 0;
    color = char_color[run_time];

#if 0 // fill test
#if 0 // sw fill
    for (uint32_t i = 0; i < LCD_H; i++) {
        for (uint32_t j = 0; j < LCD_W; j++) {

            showbuf[LCD_W * i * 2 + j * 2] = color >> 8;
            showbuf[LCD_W * i * 2 + j * 2 + 1] = color;
        }
    }
#else // gpu fill
    jlvg_surface_t surface;
    jlvg_color_t fill_color = { 0xff, 0xff, 0xff, 0xff };

    u16 color_tmp;
    color_tmp = (color << 3);
    fill_color.r = color_tmp;
    color_tmp = (color >> 5) << 2;
    fill_color.g = color_tmp;
    color_tmp = (color >> 11) << 3;
    fill_color.b = color_tmp;

    jlvg_rect_t draw_area;

#if 0 // vg 开窗大小和屏幕大小一致，耗时大
    int width = LCD_W;
    int height = LCD_H;
    int Bpp = 2;    //RGB565
    int fb_size = width * height * Bpp;
    uint32_t dest_stride;
    u8 *dest_buf;

    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = width;
    draw_area.h = height;
    dest_stride = width * Bpp;
    dest_buf = showbuf;
#else
    int width = char_size;
    int height = char_size;
    int Bpp = 2;    //RGB565
    int fb_size = width * height * Bpp;
    uint32_t dest_stride;
    u8 *dest_buf;

    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = width;
    draw_area.h = height;
    dest_stride = LCD_W * Bpp;
    dest_buf = showbuf + (sx * Bpp) + (sy * dest_stride);
#endif

    jlvg_surface_init(&surface, dest_buf, fb_size, width, height, dest_stride, VGHW_FORMAT_RGB565, 0, 1, 1, 0);

    jlvg_start_frame(jlvg, &surface);

    jlvg_clear(jlvg, &draw_area, &fill_color);

    jlvg_end_frame(jlvg);

    jlvg_surface_deinit(&surface);
#endif
#endif

#if 1
    // 配置 VG
    jlvg_surface_t surface;
    jlvg_color_t fill_color = { 0xff, 0xff, 0xff, 0xff };

    u16 color_tmp;
    color_tmp = (color << 3);
    fill_color.r = color_tmp;
    color_tmp = (color >> 5) << 2;
    fill_color.g = color_tmp;
    color_tmp = (color >> 11) << 3;
    fill_color.b = color_tmp;

    jlvg_rect_t draw_area;

    jlvg_path_t rect_path;
    jlvg_paint_color_t paint;

    jlvg_matrix_t user2surface;

#if 0 // vg 开窗大小和屏幕大小一致，耗时大
    int width = LCD_W;
    int height = LCD_H;
    int Bpp = 2;    //RGB565
    int fb_size = width * height * Bpp;
    uint32_t dest_stride;
    u8 *dest_buf;

    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = width;
    draw_area.h = height;
    dest_stride = width * Bpp;
    dest_buf = showbuf;
#else
    int width = char_size;
    int height = char_size;
    int Bpp = 2;    //RGB565
    int fb_size = width * height * Bpp;
    uint32_t dest_stride;
    u8 *dest_buf;

    draw_area.x = 0;
    draw_area.y = 0;
    draw_area.w = width;
    draw_area.h = height;
    dest_stride = LCD_W * Bpp;
    dest_buf = showbuf + (sx * Bpp) + (sy * dest_stride);
#endif

    jlvg_surface_init(&surface, dest_buf, fb_size, width, height, dest_stride, VGHW_FORMAT_RGB565, 0, 1, 1, 0);

    jlvg_start_frame(jlvg, &surface);

    jlvg_matrix_set_identity(&user2surface);

    jlvg_paint_color_init(&paint, fill_color);

    jlvg_path_init(&rect_path, JLVG_PATH_DATATYPE_F32, segments_ofs, segments, points_ofs, points);
    jlvg_path_update_fill(&rect_path);

    jlvg_fill_path(jlvg, &draw_area, &rect_path, (jlvg_paint_t *)&paint, &user2surface, NULL, VGHW_BLEND_SRC_OVER, VGHW_NON_ZERO);

    hw_cfgtime = hw_cfgtime + (get_system_us() - hw_cfgtime_begin);
    jlvg_end_frame(jlvg);

    jlvg_path_deinit(&rect_path);
    jlvg_paint_deinit((jlvg_paint_t *)&paint);
    jlvg_surface_deinit(&surface);
#endif

    run_time++;
    if (run_time == 15) {
        run_time = 0 ;
    }
}

static void draw_bitmap(ttf_font_desc_t *font_des, font_glyph_dsc_t *glyph_dsc, int16_t sx, int16_t sy, u8 *showbuf)
{
    int32_t i, j, p, q;
    int32_t x = sx + glyph_dsc->ofs_x;
    int32_t y = sy + (font_des->line_height - font_des->base_line) - glyph_dsc->box_h - glyph_dsc->ofs_y;
    int32_t x_max = x + glyph_dsc->box_w;
    int32_t y_max = y + glyph_dsc->box_h;

    unsigned int red, green, blue;
    u16 color;

    static u8 run_time = 0;

#if 0
    putchar('\n');
    putchar('\n');
    printf(">>>>	  deviation x = %d", x); //文字左边空白边距
    printf(">>>>	  deviation y = %d", y); //文字最上方空白边距
    printf(">>>>	        x_max = %d", x_max); //文字最右下角坐标即文字结束时的坐标x
    printf(">>>>	        y_max = %d", y_max); //文字最右下角坐标即文字结束时的坐标y
    printf(">>>>	        width = %d", glyph_dsc->box_w); //文字实体宽度//不包含空白边距
    printf(">>>>	         rows = %d", glyph_dsc->box_h); //文字实体高度//不包含空白边距
    putchar('\n');
    putchar('\n');
#endif

    /*uart_prtintf_img_char(glyph_dsc->box_w, glyph_dsc->box_h, font_des->buffer);*/
    uint32_t sw_blendtime_begin = get_system_us();

    for (j = y, q = 0; j < y_max; j++, q++) { //j:lcd 的y轴
        for (i = x, p = 0; i < x_max; i++, p++) { //i:lcd 的x轴
            if (i < 0 || j < 0 || i >= LCD_W || j >= LCD_H) {
                printf("i = %d; j = %d; w = %d; h = %d.", i, j, LCD_W, LCD_H);
                printf(">>>>>>>>>>>>>>error");
                continue;
            }
            if (font_des->buffer[q * glyph_dsc->box_w + p] == 0) { //背景颜色
                /*color = back_color[15]; */
            } else {//字符颜色
                color = char_color[run_time];
                showbuf[LCD_W * j * 2 + i * 2] = color >> 8;
                showbuf[LCD_W * j * 2 + i * 2 + 1] = color;
            }
            /*原生出来的数据是有做边缘优化显示的字体会更好看的我这里做测试就简单显示颜色处理 */
            /* color = font_des->buffer[q * glyph_dsc->box_w + p]; */
            /* red = (color >> 16) & 0xff; */
            /* green = (color >> 8) & 0xff; */
            /* blue = (color >> 0) & 0xff; */
            /* color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3); */
            /* showbuf[LCD_W * j * 2 + i * 2] = color >> 8; */
            /* showbuf[LCD_W * j * 2 + i * 2 + 1] = color; */
        }
    }

    sw_blendtime = sw_blendtime + (get_system_us() - sw_blendtime_begin);

    run_time++;
    if (run_time == 15) {
        run_time = 0 ;
    }
}

static void JLVG_TinyTTF_lcd_show_char(ttf_font_desc_t *font_des, u8 char_size, int16_t sx, int16_t sy, wchar_t *show_char)
{
    int16_t pen_x = sx;
    int16_t pen_y = sy;
    uint8_t segments[GLYPH_MAX_SEGMENTS];  //绘制完整的路径动作
    float points[GLYPH_MAX_SEGMENTS * 3];      //绘制结束需要的路径坐标
    uint32_t segments_ofs;
    uint32_t points_ofs;

    for (u8 i = 0; i < wcslen(show_char); i++) {
        hw_cfgtime_begin = get_system_us();
        font_glyph_dsc_t g;
        uint32_t letter = show_char[i];
        bool g_ret = ttf_get_glyph_dsc(font_des, &g, letter, '\0');
        if (g_ret == false) {
            /*Add warning if the dsc is not found
             *but do not print warning for non printable ASCII chars (e.g. '\n')*/
            if (letter >= 0x20 &&
                letter != 0xf8ff && /*LV_SYMBOL_DUMMY*/
                letter != 0x200c) { /*ZERO WIDTH NON-JOINER*/
                printf("lv_draw_letter: glyph dsc. not found for U+%X.", letter);
            }
            return;
        }

        ttf_get_vertex_path(font_des, &g, letter, segments, points, &segments_ofs, &points_ofs);
        //printf("segments_ofs = %d; points_ofs = %d.", segments_ofs, points_ofs);

        gpu_draw_bitmap(&g, char_size, pen_x, pen_y, segments, points, segments_ofs, points_ofs, lcd_get_draw_buf());

        pen_x += char_size;
        //pen_y += char_size;
    }
}

static void JL_TinyTTF_lcd_show_char(ttf_font_desc_t *font_des, u8 char_size, int16_t sx, int16_t sy, wchar_t *show_char)
{
    int16_t pen_x = sx;
    int16_t pen_y = sy;

    for (u8 i = 0; i < wcslen(show_char); i++) {
        font_glyph_dsc_t g;
        uint32_t letter = show_char[i];
        bool g_ret = ttf_get_glyph_dsc(font_des, &g, letter, '\0');
        if (g_ret == false) {
            /*Add warning if the dsc is not found
             *but do not print warning for non printable ASCII chars (e.g. '\n')*/
            if (letter >= 0x20 &&
                letter != 0xf8ff && /*LV_SYMBOL_DUMMY*/
                letter != 0x200c) { /*ZERO WIDTH NON-JOINER*/
                printf("lv_draw_letter: glyph dsc. not found for U+%X.", letter);
            }
            return;
        }

        ttf_get_glyph_bitmap(font_des, letter);
        draw_bitmap(font_des, &g, pen_x, pen_y, lcd_get_draw_buf());

        pen_x += char_size;
        //pen_y += char_size;
    }
}

static void tinyttf64_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相";
    wchar_t *wstr2  = L"广袤春夏秋冬昼";
    wchar_t *wstr3  = L"梅兰竹菊松柏常";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄";
    wchar_t *wstr5  = L"习进步勤奋努力";
    wchar_t *wstr6  = L"人湖光山色美不";
    wchar_t *wstr7  = L"起潮落人生如梦";

    const u8 char_size = 64;

    ttf_font_desc_t *font_des = tiny_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
        //JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
#else
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    tiny_ttf_destroy(font_des);
}

static void tinyttf48_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜";
    wchar_t *wstr5  = L"习进步勤奋努力自强";
    wchar_t *wstr6  = L"人湖光山色美不胜收";
    wchar_t *wstr7  = L"起潮落人生如梦岁月";
    wchar_t *wstr8  = L"印永结同古往今来历";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰";

    const u8 char_size = 48;

    ttf_font_desc_t *font_des = tiny_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
        //JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
#else
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    tiny_ttf_destroy(font_des);
}

static void tinyttf32_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日月星辰风雨";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交替岁月如梭";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂馥兰金银铜";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜吉祥如意心";
    wchar_t *wstr5  = L"习进步勤奋努力自强不息厚德载";
    wchar_t *wstr6  = L"人湖光山色美不胜收人间仙境世";
    wchar_t *wstr7  = L"起潮落人生如梦岁月匆高山流水";
    wchar_t *wstr8  = L"印永结同古往今来历史长河英雄";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰宝艺术瑰宝";
    wchar_t *wstr10 = L"生机勃勃绿意盎然花团锦簇五彩";
    wchar_t *wstr11 = L"河山万里长城巍峨壮家国情怀赤";
    wchar_t *wstr12 = L"舍生取尊师重道礼仪之邦和为贵";
    wchar_t *wstr13 = L"智信五常之道诚信为本以德服人";
    wchar_t *wstr14 = L"程万里前程似锦未来可期梦想成";

    const u8 char_size = 32;

    ttf_font_desc_t *font_des = tiny_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
        //JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
#else
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    tiny_ttf_destroy(font_des);
}

static void tinyttf24_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日月星辰风雨雷电山川草";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交替岁月如梭时光荏书声";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂馥兰金银铜铁玉石珠宝";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜吉祥如意心想事成万事";
    wchar_t *wstr5  = L"习进步勤奋努力自强不息厚德载物博学多才";
    wchar_t *wstr6  = L"人湖光山色美不胜收人间仙境世外桃清风明";
    wchar_t *wstr7  = L"起潮落人生如梦岁月匆高山流水知音难觅情";
    wchar_t *wstr8  = L"印永结同古往今来历史长河英雄辈出才子佳";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰宝艺术瑰宝文明之光照";
    wchar_t *wstr10 = L"生机勃勃绿意盎然花团锦簇五彩缤江山如画";
    wchar_t *wstr11 = L"河山万里长城巍峨壮家国情怀赤子之心精忠";
    wchar_t *wstr12 = L"舍生取尊师重道礼仪之邦和为贵和谐共处天";
    wchar_t *wstr13 = L"智信五常之道诚信为本以德服人天下为公大";
    wchar_t *wstr14 = L"程万里前程似锦未来可期梦想成学海无涯书";
    wchar_t *wstr15 = L"而不舍金石可镂水虫苒香门民安略源匆心海";
    wchar_t *wstr16 = L"龙腾虎跃马到成功步步高升日新月异蒸蒸日";
    wchar_t *wstr17 = L"好河山锦绣江山壮丽辉煌青山绿水碧波荡漾";
    wchar_t *wstr18 = L"仙境历史悠久源远流长博大精深璀璨文化文";
    wchar_t *wstr19 = L"你我得会里后自能着用们在下出就分说地主";

    const u8 char_size = 24;

    ttf_font_desc_t *font_des = tiny_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
        //JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 14 * char_size, wstr15);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 15 * char_size, wstr16);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 16 * char_size, wstr17);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 17 * char_size, wstr18);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 18 * char_size, wstr19);
#else
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 14 * char_size, wstr15);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 15 * char_size, wstr16);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 16 * char_size, wstr17);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 17 * char_size, wstr18);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 18 * char_size, wstr19);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    tiny_ttf_destroy(font_des);
}

static void tinyttf16_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日月星辰风雨雷电山川草木花鸟鱼江河湖海森林";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交替岁月如梭时光荏书声琅琅琴棋书画诗酒花茶";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂馥兰金银铜铁玉石珠宝锦绣文章龙飞凤舞麒麟";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜吉祥如意心想事成万事如意家和万事兴国泰学";
    wchar_t *wstr5  = L"习进步勤奋努力自强不息厚德载物博学多才文韬武山川壮丽景色宜";
    wchar_t *wstr6  = L"人湖光山色美不胜收人间仙境世外桃清风明月云卷云舒花开花落潮";
    wchar_t *wstr7  = L"起潮落人生如梦岁月匆高山流水知音难觅情深意长海誓山盟心心相";
    wchar_t *wstr8  = L"印永结同古往今来历史长河英雄辈出才子佳人红颜薄命情深似诗词";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰宝艺术瑰宝文明之光照耀四春夏秋冬季节更迭";
    wchar_t *wstr10 = L"生机勃勃绿意盎然花团锦簇五彩缤江山如画美景如画锦绣江山大好";
    wchar_t *wstr11 = L"河山万里长城巍峨壮家国情怀赤子之心精忠报国忠肝义胆英勇无畏";
    wchar_t *wstr12 = L"舍生取尊师重道礼仪之邦和为贵和谐共处天下大同世界一家仁义礼";
    wchar_t *wstr13 = L"智信五常之道诚信为本以德服人天下为公大道壮志凌云雄心勃勃鹏";
    wchar_t *wstr14 = L"程万里前程似锦未来可期梦想成学海无涯书山有路勤为径苦作舟锲";
    wchar_t *wstr15 = L"而不舍金石可镂水虫苒香门民安略源匆心海方纷观义之行真滴石穿";
    wchar_t *wstr16 = L"龙腾虎跃马到成功步步高升日新月异蒸蒸日上万里长城巍峨壮观大";
    wchar_t *wstr17 = L"好河山锦绣江山壮丽辉煌青山绿水碧波荡漾湖光山色美不胜收人间";
    wchar_t *wstr18 = L"仙境历史悠久源远流长博大精深璀璨文化文明之光书声琅琅欢声笑";
    wchar_t *wstr19 = L"你我得会里后自能着用们在下出就分说地主么去好我们看听想知做";
    wchar_t *wstr20 = L"学用玩吃睡说爱笑走跑跳飞高远近大小多少前后左右东西南北上下";
    wchar_t *wstr21 = L"男女老少师友亲朋同学同事工友客敌友敌我他你我它金木水火土日";
    wchar_t *wstr22 = L"月星云风雨雷电雪霜露冰春秋夏秋冬花鸟鱼虫马牛羊狗猪鸡鸭鹅猫";
    wchar_t *wstr23 = L"兔鼠龙虎蛇猴狮象熊狼狐兔牛马羊鸡鱼鸟虫红黄蓝绿黑白灰紫橙棕";
    wchar_t *wstr24 = L"青粉绿青紫蓝灰棕黑橙黄红绿青蓝紫红橙黄绿青灰白黑紫粉绿声香";
    wchar_t *wstr25 = L"色味触感思听看想读写算说画唱跳笑哭打闹玩跑走游飞游泳游唱跳";
    wchar_t *wstr26 = L"跑跳游唱跳画龙点睛一鸣惊人狐假虎威杯弓蛇影对牛弹琴狗尾续貂";
    wchar_t *wstr27 = L"画蛇添足井底之蛙刻舟求剑滥竽充数掩耳盗铃亡羊补牢守株待兔画";
    wchar_t *wstr28 = L"饼充饥破釜沉舟破镜重圆盲人摸象杯水车薪水中捞月纸上谈兵指鹿";
    wchar_t *wstr29 = L"腰缠万贯为马闭月羞花沉鱼落雁惊弓之鸟草木皆兵瓮中捉鳖望梅止";

    const u8 char_size = 16;

    ttf_font_desc_t *font_des = tiny_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
        //JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0, L"我");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 14 * char_size, wstr15);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 15 * char_size, wstr16);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 16 * char_size, wstr17);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 17 * char_size, wstr18);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 18 * char_size, wstr19);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 19 * char_size, wstr20);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 20 * char_size, wstr21);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 21 * char_size, wstr22);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 22 * char_size, wstr23);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 23 * char_size, wstr24);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 24 * char_size, wstr25);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 25 * char_size, wstr26);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 26 * char_size, wstr27);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 27 * char_size, wstr28);
        JLVG_TinyTTF_lcd_show_char(font_des, char_size, 0, 28 * char_size, wstr29);
#else
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 14 * char_size, wstr15);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 15 * char_size, wstr16);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 16 * char_size, wstr17);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 17 * char_size, wstr18);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 18 * char_size, wstr19);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 19 * char_size, wstr20);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 20 * char_size, wstr21);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 21 * char_size, wstr22);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 22 * char_size, wstr23);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 23 * char_size, wstr24);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 24 * char_size, wstr25);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 25 * char_size, wstr26);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 26 * char_size, wstr27);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 27 * char_size, wstr28);
        JL_TinyTTF_lcd_show_char(font_des, char_size, 0, 28 * char_size, wstr29);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    tiny_ttf_destroy(font_des);
}

static void tinyttf_demo_task(void *priv) //测试显示不同大小文字
{
    printf("LCD_W = %d; LCD_H = %d.", LCD_W, LCD_H);

    const char *filename = CONFIG_FONT_TTF_SUB2;
    jlvg = jlvg_create();

    //tinyttf16_test(filename);
    //tinyttf24_test(filename);
    //tinyttf32_test(filename);
    //tinyttf48_test(filename);
    tinyttf64_test(filename);

    jlvg_free(jlvg);
    while (1) {
        os_time_dly(1);
    }
}

static int tinyttf_demo_task_init(void)
{
    puts("tinyttf_demo_task_init \n\n");
    thread_fork("tinyttf_demo_task", 30, 1024 * 10, 64, 0, tinyttf_demo_task, NULL);
    while (1) {
        os_time_dly(1111);
    }
    return 0;
}
module_initcall(tinyttf_demo_task_init);
