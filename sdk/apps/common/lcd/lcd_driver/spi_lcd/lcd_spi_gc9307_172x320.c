#include "lcd_driver.h"
#include "lcd_config.h"
#include "asm/exti.h"
#include "device/gpio.h"

#if TCFG_LCD_SPI_GC9307_172X320

/* #define LCD_TEST_MODE ///< 纯色测试模式。将一帧图像buf地址替换成准备好的纯色buf的地址。 */

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

#define REGFLAG_DELAY     0x45

typedef struct {
    u8 cmd;
    u8 cnt;
    u8 dat[6];
} InitCode;

static const InitCode lcd_spi_gc9307_172x320_code[] = {
    {0xfe, 0},
    {0xef, 0},
    {0x36, 1, {0x48}},
    {0x3a, 1, {0x05}},

    /* {0x84, 1, {0x40}},//b6 en */
    {0x86, 1, {0x98}},
    {0x89, 1, {0x33}},
    {0x8b, 1, {0x80}},
    {0x8d, 1, {0x33}},
    {0x8e, 1, {0x0f}},

    /* {0xb6, 3, {0x00, 0x00, 0x24}}, */
    {0xe8, 2, {0x12, 0x00}},
    {0xec, 3, {0x33, 0x02, 0x8d}},
    {0xed, 2, {0x18, 0x09}},
    {0xff, 1, {0x62}},

    {0x99, 1, {0x3e}},
    {0x9d, 1, {0x4b}},
    {0x98, 1, {0x3e}},//V1 ADD	bvdd 3x
    {0x9c, 1, {0x4b}},//V1 ADD 20211004
    {0xc3, 1, {0x29}},//vbp for vreg1a/1b
    {0xc4, 1, {0x45}},//vbn for 2a/2b
    {0xc9, 1, {0x34}},

    //gamma
    {0xf0, 6, {0x10, 0x15, 0x09, 0x09, 0x07, 0x2f}},
    {0xf1, 6, {0x42, 0x73, 0x6f, 0x36, 0x35, 0x2f}},
    {0xf2, 6, {0x12, 0x10, 0x0a, 0x09, 0x06, 0x2f}},
    {0xf3, 6, {0x42, 0x73, 0x6f, 0x36, 0x35, 0x2f}},

    //分辨率
    {0x2a, 4, {0x00, 0x22, 0x00, 0xcd}},
    {0x2b, 4, {0x00, 0x00, 0x01, 0x3f}},

    {0x2c, 0},
    {0x35, 1, {0x00}},
    {0x44, 2, {0x00, 0x0a}},

    {0x11, 0},
    {REGFLAG_DELAY, 120},
    {0x29, 0},
    {0x2c, 0},
};

static void lcd_init_code(const InitCode *code, u8 cnt)
{
    for (u8 i = 0; i < cnt; i++) {
        if (code[i].cmd == REGFLAG_DELAY) {
            lcd_delay(code[i].cnt);
            continue;
        }

        WriteCOM(__LCD_ID, code[i].cmd);
        for (u8 j = 0; j < code[i].cnt; j++) {
            WriteDAT_8(__LCD_ID, code[i].dat[j]);
        }
    }
}

static void lcd_spi_gc9307_enter_sleep(void)
{
    WriteCOM(__LCD_ID, 0x28);
    lcd_delay(120);
    WriteCOM(__LCD_ID, 0x10);
    lcd_delay(50);
}

static void lcd_spi_gc9307_exit_sleep(void)
{
    WriteCOM(__LCD_ID, 0x11);
    lcd_delay(120) ;
    WriteCOM(__LCD_ID, 0x29);
}

static void lcd_spi_172x320_gc9307_reset(void)
{
    lcd_rst_pinstate(__LCD_ID, 1);
    lcd_rs_pinstate(__LCD_ID, 0);
    lcd_cs_pinstate(__LCD_ID, 1);

    lcd_rst_pinstate(__LCD_ID, 1);
    lcd_delay(50);
    lcd_rst_pinstate(__LCD_ID, 0);
    lcd_delay(50);
    lcd_rst_pinstate(__LCD_ID, 1);
    lcd_delay(120);
}

static int lcd_spi_172x320_gc9307_init(struct lcd_board_cfg *bd_cfg)
{
    printf("[LCD]gc9307 172x320 spi lcd init start\n");
    lcd_spi_172x320_gc9307_reset();
    return 0;
}

#ifdef LCD_TEST_MODE
static u8 *test_buf;
static u8 *lcd_172x320_gc9307_test_buf_prepra(void)
{
    static u8 *buf = NULL;
    buf = malloc(__LCD_W * __LCD_H * 2);
    if (!buf) {
        printf("[LCD]test mode malloc buf error\n");
        return NULL;
    }

    u32 color = RED;
    for (u32 i = 0; i < __LCD_W * __LCD_H * 2; i += 2) {
        buf[i] = (color >> 8) & 0xff;
        buf[i + 1] = color & 0xff;
    }

    return buf;
}
#endif

static int lcd_spi_172x320_gc9307_draw_page(void *data)
{
    WriteCOM(__LCD_ID, 0x2c);

#ifdef LCD_TEST_MODE
    if (!test_buf) {
        test_buf = lcd_172x320_gc9307_test_buf_prepra();
    }
    WriteDAT_one_page(__LCD_ID, test_buf, __LCD_W * __LCD_H * 2);
#else
    WriteDAT_one_page(__LCD_ID, (u8 *)data, __LCD_W * __LCD_H * 2);
#endif

    return 0;
}

static void lcd_spi_172x320_gc9307_bl_ctrl(struct lcd_board_cfg *bd_cfg, u8 onoff)
{
    if (onoff) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

static int lcd_spi_172x320_gc9307_send_init_code(struct lcd_board_cfg *bd_cfg)
{
    lcd_init_code(lcd_spi_gc9307_172x320_code, ARRAY_SIZE(lcd_spi_gc9307_172x320_code));
    return 0;
}

#define GC9307_CMD_ID1 0xDA
#define GC9307_CMD_ID2 0xDB
#define GC9307_CMD_ID3 0xDC
#define GC9307_ID1    0x00
#define GC9307_ID2    0x93
#define GC9307_ID3    0x07
static int lcd_spi_172x320_gc9307_check_id(struct lcd_board_cfg *bd_cfg)
{
#if 0 // 默认关闭
    u8 data[3] = {0};
    ReadDAT(__LCD_ID, GC9307_CMD_ID1, &data[0], 1);
    ReadDAT(__LCD_ID, GC9307_CMD_ID2, &data[1], 1);
    ReadDAT(__LCD_ID, GC9307_CMD_ID3, &data[2], 1);
    printf("read GC9307 ID :\n");
    put_buf(data, sizeof(data));
    if ((data[0] != GC9307_ID1) || (data[1] != GC9307_ID2) || (data[2] != GC9307_ID3)) {
        printf("GC9307 check fail!\n");
        return -1;
    }
#endif
    return 0;
}

REGISTER_LCD_SPI_DEVICE_BEGIN(lcd_spi_172x320_gc9307_dev) = {
    .info = {
        .target_xres 	 = __LCD_W,
        .target_yres 	 = __LCD_H,
        .rotate          = ROTATE_0,
        .in_fmt          = TCFG_LCD_INPUT_FORMAT,
        .out_fmt         = LCD_OUT_RGB565,
    },

    .data_out_endian  = MODE_BE,///< 大端数据，lcd_driver内部会malloc一个buf专门用来转换。
},
REGISTER_LCD_SPI_DEVICE_END()

REGISTER_LCD_DEVICE_DRIVE(lcd_spi_172x320_gc9307)  = {
    .logo            = "SPI_172x320_GC9307",
    .id              = __LCD_ID,
    .type		     = LCD_SPI,
    .dev    	     = &lcd_spi_172x320_gc9307_dev,
    .init		     = lcd_spi_172x320_gc9307_init,
    .draw            = lcd_spi_172x320_gc9307_draw_page,
    .bl_ctrl	     = lcd_spi_172x320_gc9307_bl_ctrl,
    .check           = lcd_spi_172x320_gc9307_check_id,
    .send_init_code  = lcd_spi_172x320_gc9307_send_init_code,
};

#endif // TCFG_LCD_SPI_GC9307_172X320
