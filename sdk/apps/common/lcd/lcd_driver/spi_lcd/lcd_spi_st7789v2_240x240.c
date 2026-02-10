/**
 * @file lcd_spi_st7789v2_240x240.c
 * @brief 金逸晨 1.54寸 10PIN SPI ST7789V2 LCD驱动
 *        从左往右，从上往下 240*240 16bit 5-6-5 RGB 模式 高位模式
 */
#include "lcd_driver.h"
#include "lcd_config.h"
#include "asm/exti.h"
#include "device/gpio.h"


#if TCFG_LCD_SPI_ST7789V2_240X240

/* #define LCD_TEST_MODE ///< 纯色测试模式。将一帧图像buf地址替换成准备好的纯色buf的地址。 */

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

#define REGFLAG_DELAY     0x45

// 显示区域偏移配置
#define TFT_COLUMN_OFFSET 0
#define TFT_LINE_OFFSET   0

typedef struct {
    u8 cmd;
    u8 cnt;
    u8 dat[32];
} InitCode;

static const InitCode lcd_spi_st7789v2_240x240_code[] = {
    // 软复位
    {0x01, 0},
    {REGFLAG_DELAY, 120},

    // 退出睡眠模式
    {0x11, 0},
    {REGFLAG_DELAY, 120},

    // 内存数据访问控制
    // 0x00: 从左到右，从上到下
    {0x36, 1, {0x00}},

    // RGB 5-6-5 16bit模式
    {0x3A, 1, {0x05}},

    // 帧率控制 (正常模式)
    {0xB2, 5, {0x0C, 0x0C, 0x00, 0x33, 0x33}},

    // 门控制
    {0xB7, 1, {0x35}},

    // VCOM设置
    {0xBB, 1, {0x19}},

    // LCM控制
    {0xC0, 1, {0x2C}},

    // VDV和VRH命令使能
    {0xC2, 1, {0x01}},

    // VRH设置
    {0xC3, 1, {0x12}},

    // VDV设置
    {0xC4, 1, {0x20}},

    // 帧率控制 (正常模式)
    {0xC6, 1, {0x0F}},

    // 电源控制1
    {0xD0, 2, {0xA4, 0xA1}},

    // 正极性伽玛校正
    {0xE0, 14, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}},

    // 负极性伽玛校正
    {0xE1, 14, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}},

    // 设置显示区域 (240x240)
    {0x2A, 4, {0x00, TFT_COLUMN_OFFSET, 0x00, TFT_COLUMN_OFFSET + 0xEF}}, // 列地址设置
    {0x2B, 4, {0x00, TFT_LINE_OFFSET, 0x00, TFT_LINE_OFFSET + 0xEF}},     // 行地址设置

    // 反显开启 (ST7789特有，部分屏需要)
    {0x21, 0},

    // 开启显示
    {0x29, 0},
    {REGFLAG_DELAY, 50},
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

static void lcd_spi_st7789v2_enter_sleep(void)
{
    WriteCOM(__LCD_ID, 0x28); // Display OFF
    lcd_delay(120);
    WriteCOM(__LCD_ID, 0x10); // Sleep In
    lcd_delay(50);
}

static void lcd_spi_st7789v2_exit_sleep(void)
{
    WriteCOM(__LCD_ID, 0x11); // Sleep Out
    lcd_delay(120);
    WriteCOM(__LCD_ID, 0x29); // Display ON
}

static void lcd_spi_240x240_st7789v2_reset(void)
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

static int lcd_spi_240x240_st7789v2_init(struct lcd_board_cfg *bd_cfg)
{
    printf("[LCD]ST7789V2 240x240 spi lcd init start\n");
    lcd_spi_240x240_st7789v2_reset();
    return 0;
}

#ifdef LCD_TEST_MODE
static u8 *test_buf;
static u8 *lcd_240x240_st7789v2_test_buf_prepra(void)
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

static void lcd_spi_240x240_st7789v2_set_window(u16 x1, u16 y1, u16 x2, u16 y2)
{
    // 设置列地址
    WriteCOM(__LCD_ID, 0x2A);
    WriteDAT_8(__LCD_ID, (x1 + TFT_COLUMN_OFFSET) >> 8);
    WriteDAT_8(__LCD_ID, (x1 + TFT_COLUMN_OFFSET) & 0xFF);
    WriteDAT_8(__LCD_ID, (x2 + TFT_COLUMN_OFFSET) >> 8);
    WriteDAT_8(__LCD_ID, (x2 + TFT_COLUMN_OFFSET) & 0xFF);

    // 设置行地址
    WriteCOM(__LCD_ID, 0x2B);
    WriteDAT_8(__LCD_ID, (y1 + TFT_LINE_OFFSET) >> 8);
    WriteDAT_8(__LCD_ID, (y1 + TFT_LINE_OFFSET) & 0xFF);
    WriteDAT_8(__LCD_ID, (y2 + TFT_LINE_OFFSET) >> 8);
    WriteDAT_8(__LCD_ID, (y2 + TFT_LINE_OFFSET) & 0xFF);

    // 写入RAM
    WriteCOM(__LCD_ID, 0x2C);
}

static int lcd_spi_240x240_st7789v2_draw_page(void *data)
{
    WriteCOM(__LCD_ID, 0x2c);

#ifdef LCD_TEST_MODE
    if (!test_buf) {
        test_buf = lcd_240x240_st7789v2_test_buf_prepra();
    }
    WriteDAT_one_page(__LCD_ID, test_buf, __LCD_W * __LCD_H * 2);
#else
    WriteDAT_one_page(__LCD_ID, (u8 *)data, __LCD_W * __LCD_H * 2);
#endif

    return 0;
}

static void lcd_spi_240x240_st7789v2_bl_ctrl(struct lcd_board_cfg *bd_cfg, u8 onoff)
{
    if (onoff) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

static int lcd_spi_240x240_st7789v2_send_init_code(struct lcd_board_cfg *bd_cfg)
{
    lcd_init_code(lcd_spi_st7789v2_240x240_code, ARRAY_SIZE(lcd_spi_st7789v2_240x240_code));
    return 0;
}

#define ST7789V2_CMD_ID1 0xDA
#define ST7789V2_CMD_ID2 0xDB
#define ST7789V2_CMD_ID3 0xDC
#define ST7789V2_ID1    0x00
#define ST7789V2_ID2    0x85
#define ST7789V2_ID3    0x52
static int lcd_spi_240x240_st7789v2_check_id(struct lcd_board_cfg *bd_cfg)
{
#if 0 // 默认关闭
    u8 data[3] = {0};
    ReadDAT(__LCD_ID, ST7789V2_CMD_ID1, &data[0], 1);
    ReadDAT(__LCD_ID, ST7789V2_CMD_ID2, &data[1], 1);
    ReadDAT(__LCD_ID, ST7789V2_CMD_ID3, &data[2], 1);
    printf("read ST7789V2 ID :\n");
    put_buf(data, sizeof(data));
    if ((data[0] != ST7789V2_ID1) || (data[1] != ST7789V2_ID2) || (data[2] != ST7789V2_ID3)) {
        printf("ST7789V2 check fail!\n");
        return -1;
    }
#endif
    return 0;
}


REGISTER_LCD_SPI_DEVICE_BEGIN(lcd_spi_240x240_st7789v2_dev) = {
    .info = {
        .target_xres 	 = __LCD_W,
        .target_yres 	 = __LCD_H,
        .rotate          = ROTATE_0,
        .in_fmt          = TCFG_LCD_INPUT_FORMAT,
        .out_fmt         = LCD_OUT_RGB565,
    },

    .data_out_endian  = MODE_BE, ///< 大端数据，lcd_driver内部会malloc一个buf专门用来转换。
},
REGISTER_LCD_SPI_DEVICE_END()

REGISTER_LCD_DEVICE_DRIVE(lcd_spi_240x240_st7789v2)  = {
    .logo            = "SPI_240x240_ST7789V2",
    .id              = __LCD_ID,
    .type		     = LCD_SPI,
    .dev    	     = &lcd_spi_240x240_st7789v2_dev,
    .init		     = lcd_spi_240x240_st7789v2_init,
    .draw            = lcd_spi_240x240_st7789v2_draw_page,
    .bl_ctrl	     = lcd_spi_240x240_st7789v2_bl_ctrl,
    .check           = lcd_spi_240x240_st7789v2_check_id,
    .send_init_code  = lcd_spi_240x240_st7789v2_send_init_code,
};

#endif // TCFG_LCD_SPI_ST7789V2_240X240
