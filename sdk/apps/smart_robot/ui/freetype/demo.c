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

#if 0
u8 *test_char[] = {
    L"A",
    L"B",
    L"C",
    L"D",
    L"E",
    L"F",
};
#else
u8 *test_char[] = {
    L"加",
    L"发",
    L"绣",
    L"霉",
    L"冀",
    L"猫",
};
#endif

static void uart_prtintf_img_char(u16 w, u16 h, u8 *buf)
{
    putchar('\n');

    for (u8 i = 0; i < h; i++) {
        for (u8 j = 0; j < w; j++) {
            putchar(buf[i * w + j] == 0 ? ' '
                    : buf[i * w + j] < 128 ? '+'
                    : '*');
        }

        putchar('\n');
    }

    putchar('\n');
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

    run_time++;
    if (run_time == 15) {
        run_time = 0 ;
    }
}

static void JL_FT_lcd_show_char(FT_Face face, FT_GlyphSlot slot, double angle, u8 char_size,  u16 sx, u16 sy, wchar_t *show_char)
{
    FT_Vector pen;
    FT_Error error;
    FT_Matrix matrix;  //旋转角度结构体

    u32 flags = FT_LOAD_DEFAULT | FT_LOAD_RENDER | FT_LOAD_NO_AUTOHINT | FT_OUTLINE_HIGH_PRECISION;

    if (angle != 0) {
        angle  = (angle / 360) * 3.14159 * 2;        /* use 0 degrees     */
        matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
        matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
        matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
        matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);
    }

    pen.x = sx * 64;//这个64我也不知道是怎么回事 没有搞懂
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

    lcd_draw_finish();
}

static void freetype_demo_task(void *priv) //测试显示不同大小文字
{
    wchar_t *wstr1 = L"这是FT测试";
    wchar_t *wstr2 = L"支持任意缩放";
    wchar_t *wstr3 = L"支持任意旋转字体";
    wchar_t *wstr4 = L"支持自定义字体颜色";
    wchar_t *wstr5 = L"这是旋转字体角度测试";
    wchar_t *wstr6 = L"显示数字测试12345";
    wchar_t *wstr7 = L"#$@!()";


    FT_Library library;
    FT_Face face;
    FT_GlyphSlot slot;
    FT_Glyph glyph;

    FT_Error error;

    int i, j;

    int line_box_ymin = 0;
    int line_box_ymax = 0;
    double        angle;


    const char *filename = "mnt/sdfile/res/cfg/default.ttf";

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

    slot = face->glyph;

    u8 len_size = 0;
    u8 char_size = 48;
    printf(">>>>>>show char 1");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, 0, char_size, 0, 0, wstr1);//行坐标0,0
    len_size += char_size;


    char_size = 40;
    printf(">>>>>>show char 2");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, 0, char_size, 0, len_size, wstr2);////行坐标0,len_size
    len_size += char_size;

    char_size = 30;
    printf(">>>>>>show char 3");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, 0, char_size, 0, len_size, wstr3);
    len_size += char_size;

    char_size = 20;
    printf(">>>>>>show char 4");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, 0, char_size, 0, len_size, wstr4);
    len_size += char_size;

    char_size = 50;
    printf(">>>>>>show char 7");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, 0, char_size, 110, len_size + 20, wstr7);

    len_size += 30;
    char_size = 35;
    printf(">>>>>>show char 5");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, -45, char_size, 40, len_size, wstr5);

    char_size = 35;
    printf(">>>>>>show char 6");
    FT_Set_Pixel_Sizes(face, char_size, 0);
    JL_FT_lcd_show_char(face, slot, 45, char_size, 30, 430, wstr6);

    char_size = 20;
    u8 run_time = 0;

    while (1) {
        JL_FT_lcd_show_char(face, slot, 0, char_size, 150, 430, test_char[run_time]);
        run_time++;
        if (run_time == 6) {
            run_time = 0;
        }
    }



    FT_Done_Face(face);
    FT_Done_FreeType(library);

    while (1) {
        os_time_dly(1);
    }

}
static int freetype_demo_task_init(void)
{
    puts("freetype_demo_task_init \n\n");
    thread_fork("freetype_demo_task", 30, 1024 * 10, 64, 0, freetype_demo_task, NULL);
    while (1) {
        os_time_dly(1111);
    }
    return 0;
}
module_initcall(freetype_demo_task_init);

