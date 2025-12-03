#include "app_config.h"
#include "system/includes.h"
#include "ft2build.h"
#include "freetype/freetype.h"
#include "storage_device.h"
#include "lcd_config.h"
#include "wchar.h"

#include <freetype/freetype.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

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

#define GPU_DRAW 0

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

#define GLYPH_MAX_SEGMENTS 1024 // 字形最大的轮廓数，如果超了就要改大绘制修改字形

typedef struct _jlvg_glyph_t {
    uint8_t segments[GLYPH_MAX_SEGMENTS];  //绘制完整的路径动作
    float points[GLYPH_MAX_SEGMENTS * 3];      //绘制结束需要的路径坐标
    uint32_t segments_ofs;
    uint32_t points_ofs;
    u8 char_size;   //字形设置的大小
    FT_UShort units_per_EM; // FT 中原像素的比例单位
    FT_Short ascender;  //ascender表示从基线到字体中最高点的距离（向上为正）
    FT_Short descender; //descender表示从基线到字体中最低点的距离（向下为负）
    FT_Short height;    // 表示整个字形框的高度
    FT_Glyph_Metrics metrics;  // FT 中字形的量度信息
} jlvg_glyph_t;

static void outline_push_point(
    jlvg_glyph_t *jlvg_glyph,
    jlvg_hw_pathtype_t type,
    const FT_Vector *control1,
    const FT_Vector *control2,
    const FT_Vector *to)
{
    if (jlvg_glyph->segments_ofs >= GLYPH_MAX_SEGMENTS) {
        printf("error: GLYPH_MAX_SEGMENTS is %d, curr segments_num = %d!!!", GLYPH_MAX_SEGMENTS, jlvg_glyph->segments_ofs);
        assert(0);
    }

    jlvg_glyph->segments[jlvg_glyph->segments_ofs] = type;

    float cx1;
    float cy1;
    float cx2;
    float cy2;
    float x;
    float y;
    int glyph_height;
#if 1
    //int glyph_height = jlvg_glyph->metrics.vertAdvance + jlvg_glyph->descender;
    glyph_height = jlvg_glyph->metrics.vertAdvance;
#else
    int h_ofs = jlvg_glyph->ascender - jlvg_glyph->metrics.vertAdvance;
    if (h_ofs > 0) {
        glyph_height = jlvg_glyph->metrics.vertAdvance - h_ofs;
    } else {
        glyph_height = jlvg_glyph->metrics.vertAdvance + h_ofs;
    }
#endif

    //int w_ofs = jlvg_glyph->metrics.horiBearingX;
    //int h_ofs = glyph_height - jlvg_glyph->metrics.horiBearingY;
    float unit = (float)jlvg_glyph->units_per_EM;

    if ((type == VGHW_MOVE_TO) || (type == VGHW_LINE_TO)) {
        x = (float)(to->x) * jlvg_glyph->char_size / unit;
        y = (float)(glyph_height - to->y) * jlvg_glyph->char_size / unit;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = x;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = y;
    } else if (type == VGHW_QUAD_TO) {
        cx1 = (float)(control1->x) * jlvg_glyph->char_size / unit;
        cy1 = (float)(glyph_height - control1->y) * jlvg_glyph->char_size / unit;
        x = (float)(to->x) * jlvg_glyph->char_size / unit;
        y = (float)(glyph_height - to->y) * jlvg_glyph->char_size / unit;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = cx1;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = cy1;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = x;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = y;
    } else if (type == VGHW_CUBIC_TO) {
        cx1 = (float)(control1->x) * jlvg_glyph->char_size / unit;
        cy1 = (float)(glyph_height - control1->y) * jlvg_glyph->char_size / unit;
        cx2 = (float)(control2->x) * jlvg_glyph->char_size / unit;
        cy2 = (float)(glyph_height - control2->y) * jlvg_glyph->char_size / unit;
        x = (float)(to->x) * jlvg_glyph->char_size / unit;
        y = (float)(glyph_height - to->y) * jlvg_glyph->char_size / unit;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = cx1;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = cy1;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = cx2;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = cy2;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = x;
        jlvg_glyph->points[jlvg_glyph->points_ofs++] = y;
    } else {
        // nothing
    }

    jlvg_glyph->segments_ofs++;
    return;
}

static int outline_move_to_cb(
    const FT_Vector *to,
    void *user)
{
    jlvg_glyph_t *jlvg_glyph = (jlvg_glyph_t *)user;
    outline_push_point(jlvg_glyph, VGHW_MOVE_TO, NULL, NULL, to);
    return FT_Err_Ok;
}

static int outline_line_to_cb(
    const FT_Vector *to,
    void *user)
{
    jlvg_glyph_t *jlvg_glyph = (jlvg_glyph_t *)user;
    outline_push_point(jlvg_glyph, VGHW_LINE_TO, NULL, NULL, to);
    return FT_Err_Ok;
}

static int outline_conic_to_cb(
    const FT_Vector *control,
    const FT_Vector *to,
    void *user)
{
    jlvg_glyph_t *jlvg_glyph = (jlvg_glyph_t *)user;
    outline_push_point(jlvg_glyph, VGHW_QUAD_TO, control, NULL, to);
    return FT_Err_Ok;
}

static int outline_cubic_to_cb(
    const FT_Vector *control1,
    const FT_Vector *control2,
    const FT_Vector *to,
    void *user)
{
    jlvg_glyph_t *jlvg_glyph = (jlvg_glyph_t *)user;
    outline_push_point(jlvg_glyph, VGHW_CUBIC_TO, control1, control2, to);
    return FT_Err_Ok;
}

static FT_Outline_Funcs outline_funcs = {
    .move_to = outline_move_to_cb,
    .line_to = outline_line_to_cb,
    .conic_to = outline_conic_to_cb,
    .cubic_to = outline_cubic_to_cb,
    .shift = 0,
    .delta = 0
};

static void gpu_draw_bitmap(FT_Face face, u8 char_size, u16 sx, u16 sy, u8 *showbuf)
{
    u16 color;

    static u8 run_time = 0;
    color = char_color[run_time];

    // 获取轮廓数据
    FT_Outline *outline = &face->glyph->outline;

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

#if 1 // 绘制字形
    // 获取路径
    FT_Error error;

    uint32_t get_path_begin = get_system_us();
    jlvg_glyph_t jlvg_glyph;
    jlvg_glyph.segments_ofs = 0;
    jlvg_glyph.points_ofs = 0;
    jlvg_glyph.char_size = char_size;
    jlvg_glyph.units_per_EM = face->units_per_EM;
    jlvg_glyph.ascender = face->ascender;
    jlvg_glyph.descender = face->descender;
    jlvg_glyph.height = face->height;
    jlvg_glyph.metrics = face->glyph->metrics;

    /* Run outline decompose again to fill outline data */
    error = FT_Outline_Decompose(outline, &outline_funcs, &jlvg_glyph);
    if (error) {
        printf("FT_Outline_Decompose error = %d.", error);
        return;
    }

    /* close outline */
    outline_push_point(&jlvg_glyph, VGHW_CLOSE_PATH, NULL, NULL, NULL);
    uint32_t get_path_end = get_system_us();
    cfg_path_time += (get_path_end - get_path_begin);

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

    jlvg_path_init(&rect_path, JLVG_PATH_DATATYPE_F32, jlvg_glyph.segments_ofs, jlvg_glyph.segments, jlvg_glyph.points_ofs, jlvg_glyph.points);
    jlvg_path_update_fill(&rect_path);

    jlvg_fill_path(jlvg, &draw_area, &rect_path, (jlvg_paint_t *)&paint, &user2surface, NULL, VGHW_BLEND_SRC_OVER, VGHW_NON_ZERO);

    hw_cfgtime = hw_cfgtime + (get_system_us() - hw_cfgtime_begin);
    jlvg_end_frame(jlvg);

    jlvg_path_deinit(&rect_path);
    jlvg_paint_deinit((jlvg_paint_t *)&paint);
    jlvg_surface_deinit(&surface);
#endif
#endif

    run_time++;
    if (run_time == 15) {
        run_time = 0 ;
    }
}

static void draw_bitmap(FT_Bitmap  *bitmap, FT_Int x, FT_Int y, u8 *showbuf)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;

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
    printf(">>>>	bitmap->width = %d", bitmap->width); //文字实体宽度//不包含空白边距
    printf(">>>>	 bitmap->rows = %d", bitmap->rows); //文字实体高度//不包含空白边距
    putchar('\n');
    putchar('\n');
#endif

    /*uart_prtintf_img_char(bitmap->width, bitmap->rows, bitmap->buffer);*/
    uint32_t sw_blendtime_begin = get_system_us();

    for (j = y, q = 0; j < y_max; j++, q++) { //j:lcd 的y轴
        for (i = x, p = 0; i < x_max; i++, p++) { //i:lcd 的x轴
            if (i < 0 || j < 0 || i >= LCD_W || j >= LCD_H) {
                printf(">>>>>>>>>>>>>>error");
                continue;
            }
            if (bitmap->buffer[q * bitmap->width + p] == 0) { //背景颜色
                /*color = back_color[15]; */
            } else {//字符颜色
                color = char_color[run_time];
                showbuf[LCD_W * j * 2 + i * 2] = color >> 8;
                showbuf[LCD_W * j * 2 + i * 2 + 1] = color;
            }
            /*原生出来的数据是有做边缘优化显示的字体会更好看的我这里做测试就简单显示颜色处理 */
            /* color = bitmap->buffer[q * bitmap->width + p]; */
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

static void JLVG_FT_lcd_show_char(FT_Face face, FT_GlyphSlot slot, u8 char_size,  u16 sx, u16 sy, wchar_t *show_char)
{
    FT_Vector pen;
    FT_Error error;
    FT_Matrix matrix;  //旋转角度结构体
    //printf("sx = %d; sy = %d.", sx, sy);

    pen.x = sx;
    pen.y = sy;

    for (u8 i = 0; i < wcslen(show_char); i++) {
        hw_cfgtime_begin = get_system_us();
        FT_UInt glyph_index;

        // 获取字符的字形索引
        glyph_index = FT_Get_Char_Index(face, show_char[i]);

        // 加载字形
        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
        //error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
        //error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
        //error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_RENDER | FT_LOAD_NO_AUTOHINT | FT_OUTLINE_HIGH_PRECISION);
        if (error) {
            return;
        }

#if 0
        printf(">>>>>>>>> face->units_per_EM = %ld", face->units_per_EM);
        printf("face->glyph->bitmap.rows  = %d", face->glyph->bitmap.rows);
        printf("face->glyph->bitmap.width = %d", face->glyph->bitmap.width);
        printf("face->glyph->bitmap_left  = %d", face->glyph->bitmap_left);
        printf("face->glyph->bitmap_top   = %d", face->glyph->bitmap_top);
        printf("face->glyph->bitmap.rows  = %d", face->glyph->bitmap.rows);
        printf("face->glyph->metrics.width = %d", face->glyph->metrics.width);
        printf("face->glyph->metrics.height = %d", face->glyph->metrics.height);
        printf("face->glyph->metrics.horiBearingX = %d", face->glyph->metrics.horiBearingX);
        printf("face->glyph->metrics.horiBearingY = %d", face->glyph->metrics.horiBearingY);
        printf("face->glyph->metrics.horiAdvance = %d", face->glyph->metrics.horiAdvance);
        printf("face->glyph->metrics.vertBearingX = %d", face->glyph->metrics.vertBearingX);
        printf("face->glyph->metrics.vertBearingY = %d", face->glyph->metrics.vertBearingY);
        printf("face->glyph->metrics.vertAdvance = %d", face->glyph->metrics.vertAdvance);
        printf("face->ascender = %d", face->ascender);
        printf("face->descender = %d", face->descender);
        printf("face->height = %d", face->height);
#endif

        gpu_draw_bitmap(face, char_size, pen.x, pen.y, lcd_get_draw_buf());

        pen.x += char_size;
        //pen.y += char_size;
    }
}

static void JL_FT_lcd_show_char(FT_Face face, FT_GlyphSlot slot, double angle, u8 char_size,  u16 sx, u16 sy, wchar_t *show_char)
{
    FT_Vector pen;
    FT_Error error;
    FT_Matrix matrix;  //旋转角度结构体

    u32 flags = FT_LOAD_DEFAULT | FT_LOAD_RENDER;
    //u32 flags = FT_LOAD_DEFAULT;

    if (angle != 0) {
        angle  = (angle / 360) * 3.14159 * 2;        /* use 0 degrees     */
        matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
        matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
        matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
        matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);
    }

    pen.x = sx * 64;//这个64我也不知道是怎么回事 没有搞懂
    // 解释 ：这个 64 是因为在 FT 中为了提高精度，以26.6固定点格式表示。这意味着该值可以被分解为整数部分（26位）和小数部分（6位），以提供高精度的像素级定位, × 64 在 FT 中相当于对齐了整数部分
    pen.y = (LCD_H - char_size - sy) * 64;

    for (u8 i = 0; i < wcslen(show_char); i++) {
        if (angle != 0) {
            FT_Set_Transform(face, &matrix, &pen);
        } else {
            FT_Set_Transform(face, 0, &pen);
        }

        error = FT_Load_Char(face, show_char[i], flags);
        if (error) {
            printf("FT_Load_Char error\n");
            return ;
        }

        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
#if 0
        printf(">>>>>>>>>pen.x = %ld<<<pen.y = %ld<<<<<", pen.x, pen.y);
        printf(">>>>>>>>slot->advance.x = %ld<<<slot->advance.y= %ld<<<<<", slot->advance.x, slot->advance.y);
        printf(">>>>>>>>>slot->bitmap.width = %d, slot->bitmap.rows = %d, slot->bitmap_left = %d,  slot->bitmap_top = %d", slot->bitmap.width, slot->bitmap.rows, slot->bitmap_left, slot->bitmap_top);
#endif

        draw_bitmap(&slot->bitmap, slot->bitmap_left, LCD_H - slot->bitmap_top, lcd_get_draw_buf());
    }
}

static void freetype64_test(FT_Face face)
{
    FT_GlyphSlot slot = face->glyph;

    wchar_t *wstr1  = L"天地人和心手相";
    wchar_t *wstr2  = L"广袤春夏秋冬昼";
    wchar_t *wstr3  = L"梅兰竹菊松柏常";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄";
    wchar_t *wstr5  = L"习进步勤奋努力";
    wchar_t *wstr6  = L"人湖光山色美不";
    wchar_t *wstr7  = L"起潮落人生如梦";

    const u8 char_size = 64;
    FT_Set_Pixel_Sizes(face, char_size, 0);
    //FT_Set_Char_Size(face, char_size, char_size, LCD_W, LCD_H);
    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();
        //JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0, L"我一");
        //JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0, L"我一");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0  * char_size, wstr1);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 1  * char_size, wstr2);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 2  * char_size, wstr3);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 3  * char_size, wstr4);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 4  * char_size, wstr5);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 5  * char_size, wstr6);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 6  * char_size, wstr7);
#else
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0  * char_size, wstr1);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 1  * char_size, wstr2);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 2  * char_size, wstr3);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 3  * char_size, wstr4);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 4  * char_size, wstr5);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 5  * char_size, wstr6);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 6  * char_size, wstr7);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);
}

static void freetype48_test(FT_Face face)
{
    FT_GlyphSlot slot = face->glyph;

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
    FT_Set_Pixel_Sizes(face, char_size, 0);
    //FT_Set_Char_Size(face, char_size, char_size, LCD_W, LCD_H);
    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();
        //JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0, L"我一");
        //JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0, L"我一");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0  * char_size, wstr1);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 1  * char_size, wstr2);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 2  * char_size, wstr3);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 3  * char_size, wstr4);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 4  * char_size, wstr5);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 5  * char_size, wstr6);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 6  * char_size, wstr7);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 7  * char_size, wstr8);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 8  * char_size, wstr9);
#else
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0  * char_size, wstr1);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 1  * char_size, wstr2);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 2  * char_size, wstr3);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 3  * char_size, wstr4);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 4  * char_size, wstr5);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 5  * char_size, wstr6);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 6  * char_size, wstr7);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 7  * char_size, wstr8);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 8  * char_size, wstr9);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);
}

static void freetype32_test(FT_Face face)
{
    FT_GlyphSlot slot = face->glyph;

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
    FT_Set_Pixel_Sizes(face, char_size, 0);
    //FT_Set_Char_Size(face, char_size, char_size, LCD_W, LCD_H);
    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();
        //JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0, L"我一");
        //JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0, L"我一");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0  * char_size, wstr1);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 1  * char_size, wstr2);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 2  * char_size, wstr3);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 3  * char_size, wstr4);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 4  * char_size, wstr5);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 5  * char_size, wstr6);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 6  * char_size, wstr7);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 7  * char_size, wstr8);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 8  * char_size, wstr9);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 9  * char_size, wstr10);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 10 * char_size, wstr11);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 11 * char_size, wstr12);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 12 * char_size, wstr13);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 13 * char_size, wstr14);
#else
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0  * char_size, wstr1);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 1  * char_size, wstr2);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 2  * char_size, wstr3);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 3  * char_size, wstr4);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 4  * char_size, wstr5);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 5  * char_size, wstr6);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 6  * char_size, wstr7);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 7  * char_size, wstr8);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 8  * char_size, wstr9);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 9  * char_size, wstr10);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 10 * char_size, wstr11);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 11 * char_size, wstr12);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 12 * char_size, wstr13);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 13 * char_size, wstr14);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);
}

static void freetype24_test(FT_Face face)
{
    FT_GlyphSlot slot = face->glyph;

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
    FT_Set_Pixel_Sizes(face, char_size, 0);
    //FT_Set_Char_Size(face, char_size, char_size, LCD_W, LCD_H);
    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();
        //JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0, L"我一");
        //JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0, L"我一");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0  * char_size, wstr1);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 1  * char_size, wstr2);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 2  * char_size, wstr3);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 3  * char_size, wstr4);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 4  * char_size, wstr5);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 5  * char_size, wstr6);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 6  * char_size, wstr7);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 7  * char_size, wstr8);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 8  * char_size, wstr9);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 9  * char_size, wstr10);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 10 * char_size, wstr11);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 11 * char_size, wstr12);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 12 * char_size, wstr13);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 13 * char_size, wstr14);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 14 * char_size, wstr15);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 15 * char_size, wstr16);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 16 * char_size, wstr17);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 17 * char_size, wstr18);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 18 * char_size, wstr19);
#else
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0  * char_size, wstr1);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 1  * char_size, wstr2);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 2  * char_size, wstr3);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 3  * char_size, wstr4);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 4  * char_size, wstr5);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 5  * char_size, wstr6);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 6  * char_size, wstr7);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 7  * char_size, wstr8);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 8  * char_size, wstr9);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 9  * char_size, wstr10);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 10 * char_size, wstr11);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 11 * char_size, wstr12);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 12 * char_size, wstr13);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 13 * char_size, wstr14);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 14 * char_size, wstr15);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 15 * char_size, wstr16);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 16 * char_size, wstr17);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 17 * char_size, wstr18);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 18 * char_size, wstr19);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);
}

static void freetype16_test(FT_Face face)
{
    FT_GlyphSlot slot = face->glyph;

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
    FT_Set_Pixel_Sizes(face, char_size, 0);
    //FT_Set_Char_Size(face, char_size, char_size, LCD_W, LCD_H);
    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();
        //JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0, L"我一");
        //JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0, L"我一");
#if 1
#if GPU_DRAW // GPU 矢量渲染
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 0  * char_size, wstr1);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 1  * char_size, wstr2);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 2  * char_size, wstr3);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 3  * char_size, wstr4);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 4  * char_size, wstr5);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 5  * char_size, wstr6);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 6  * char_size, wstr7);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 7  * char_size, wstr8);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 8  * char_size, wstr9);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 9  * char_size, wstr10);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 10 * char_size, wstr11);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 11 * char_size, wstr12);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 12 * char_size, wstr13);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 13 * char_size, wstr14);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 14 * char_size, wstr15);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 15 * char_size, wstr16);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 16 * char_size, wstr17);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 17 * char_size, wstr18);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 18 * char_size, wstr19);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 19 * char_size, wstr20);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 20 * char_size, wstr21);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 21 * char_size, wstr22);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 22 * char_size, wstr23);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 23 * char_size, wstr24);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 24 * char_size, wstr25);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 25 * char_size, wstr26);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 26 * char_size, wstr27);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 27 * char_size, wstr28);
        JLVG_FT_lcd_show_char(face, slot, char_size, 0, 28 * char_size, wstr29);
#else
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0  * char_size, wstr1);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 1  * char_size, wstr2);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 2  * char_size, wstr3);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 3  * char_size, wstr4);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 4  * char_size, wstr5);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 5  * char_size, wstr6);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 6  * char_size, wstr7);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 7  * char_size, wstr8);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 8  * char_size, wstr9);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 9  * char_size, wstr10);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 10 * char_size, wstr11);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 11 * char_size, wstr12);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 12 * char_size, wstr13);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 13 * char_size, wstr14);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 14 * char_size, wstr15);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 15 * char_size, wstr16);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 16 * char_size, wstr17);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 17 * char_size, wstr18);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 18 * char_size, wstr19);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 19 * char_size, wstr20);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 20 * char_size, wstr21);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 21 * char_size, wstr22);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 22 * char_size, wstr23);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 23 * char_size, wstr24);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 24 * char_size, wstr25);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 25 * char_size, wstr26);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 26 * char_size, wstr27);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 27 * char_size, wstr28);
        JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 28 * char_size, wstr29);
#endif
#endif
        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);
}

static void freetype_demo_task(void *priv) //测试显示不同大小文字
{
    printf("LCD_W = %d; LCD_H = %d.", LCD_W, LCD_H);

    FT_Library library;
    FT_Face face;
    FT_GlyphSlot slot;
    FT_Glyph glyph;

    FT_Error error;

    const char *filename = CONFIG_FONT_TTF_SUB2;
    error = FT_Init_FreeType(&library);
    if (error) {
        printf("FT_Init_FreeType err0!\n");
        return;
    }

    error = FT_New_Face(library, filename, 0, &face);
    if (error) {
        printf("FT_New_Face err, open file[%s] failed!\n", filename);
        return;
    }

    jlvg = jlvg_create();

    //freetype16_test(face);
    //freetype24_test(face);
    //freetype32_test(face);
    //freetype48_test(face);
    freetype64_test(face);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    jlvg_free(jlvg);
    while (1) {
        os_time_dly(1);
    }

}

static int freetype_demo_task_init(void)
{
    puts("freetype_demo_task_init \n\n");
    thread_fork("freetype_demo_task_init", 30, 1024 * 10, 64, 0, freetype_demo_task, NULL);
    while (1) {
        os_time_dly(1111);
    }
    return 0;
}
module_initcall(freetype_demo_task_init);
