#include "lcd_driver.h"
#include "lcd_config.h"
#include "asm/exti.h"
#include "device/gpio.h"


#if TCFG_LCD_SPI_NV3006A_142X428

/* #define LCD_TEST_MODE ///< 纯色测试模式。将一帧图像buf地址替换成准备好的纯色buf的地址。 */

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

#define REGFLAG_DELAY     0x45

typedef struct {
    u8 cmd;
    u8 cnt;
    u8 dat[4];
} InitCode;

static const InitCode lcd_spi_nv3006a_142x428_code[] = {
    {0xff, 1, {0xa5}},
    {0x9a, 1, {0x08}},
    {0x9b, 1, {0x08}},
    {0x9c, 1, {0xb0}},
    {0x9d, 1, {0x16}},
    {0x9e, 1, {0xc4}},
    {0x8f, 2, {0x55, 0x04}},
    {0x84, 1, {0x90}},
    {0x83, 1, {0x7b}},
    {0x85, 1, {0x33}},
    {0x60, 1, {0x00}},
    {0x70, 1, {0x00}},
    {0x61, 1, {0x02}},
    {0x71, 1, {0x02}},
    {0x62, 1, {0x04}},
    {0x72, 1, {0x04}},
    {0x6c, 1, {0x29}},
    {0x7c, 1, {0x29}},
    {0x6d, 1, {0x31}},
    {0x7d, 1, {0x31}},
    {0x6e, 1, {0x0f}},
    {0x7e, 1, {0x0f}},
    {0x66, 1, {0x21}},
    {0x76, 1, {0x21}},
    {0x68, 1, {0x3a}},
    {0x78, 1, {0x3a}},
    {0x63, 1, {0x07}},
    {0x73, 1, {0x07}},
    {0x64, 1, {0x05}},
    {0x74, 1, {0x05}},
    {0x65, 1, {0x02}},
    {0x75, 1, {0x02}},
    {0x67, 1, {0x23}},
    {0x77, 1, {0x23}},
    {0x69, 1, {0x08}},
    {0x79, 1, {0x08}},
    {0x6a, 1, {0x13}},
    {0x7a, 1, {0x13}},
    {0x6b, 1, {0x13}},
    {0x7b, 1, {0x13}},
    {0x6f, 1, {0x00}},
    {0x7f, 1, {0x00}},
    {0x50, 1, {0x00}},
    {0x52, 1, {0xd6}},
    {0x53, 1, {0x08}},
    {0x54, 1, {0x08}},
    {0x55, 1, {0x1e}},
    {0x56, 1, {0x1c}},
    {0xa0, 3, {0x2b, 0x24, 0x00}},
    {0xa1, 1, {0x87}},
    {0xa2, 1, {0x86}},
    {0xa5, 1, {0x00}},
    {0xa6, 1, {0x00}},
    {0xa7, 1, {0x00}},
    {0xa8, 1, {0x36}},
    {0xa9, 1, {0x7e}},
    {0xaa, 1, {0x7e}},
    {0xb9, 1, {0x85}},
    {0xba, 1, {0x84}},
    {0xbb, 1, {0x83}},
    {0xbc, 1, {0x82}},
    {0xbd, 1, {0x81}},
    {0xbe, 1, {0x80}},
    {0xbf, 1, {0x01}},
    {0xc0, 1, {0x02}},
    {0xc1, 1, {0x00}},
    {0xc2, 1, {0x00}},
    {0xc3, 1, {0x00}},
    {0xc4, 1, {0x33}},
    {0xc5, 1, {0x75}},
    {0xc6, 1, {0x75}},
    {0xc8, 2, {0x33, 0x33}},
    {0xc9, 1, {0x68}},
    {0xca, 1, {0x69}},
    {0xcb, 1, {0x6a}},
    {0xcc, 1, {0x6b}},
    {0xcd, 2, {0x33, 0x33}},
    {0xce, 1, {0x6c}},
    {0xcf, 1, {0x6d}},
    {0xd0, 1, {0x6e}},
    {0xd1, 1, {0x6f}},
    {0xab, 2, {0x03, 0x67}},
    {0xac, 2, {0x03, 0x6b}},
    {0xad, 2, {0x03, 0x68}},
    {0xae, 2, {0x03, 0x6c}},
    {0xb3, 1, {0x00}},
    {0xb4, 1, {0x00}},
    {0xb5, 1, {0x00}},
    {0xb6, 1, {0x32}},
    {0xb7, 1, {0x7e}},
    {0xb8, 1, {0x7e}},
    {0xe0, 1, {0x00}},
    {0xe1, 2, {0x03, 0x0f}},
    {0xe2, 1, {0x04}},
    {0xe3, 1, {0x01}},
    {0xe4, 1, {0x0e}},
    {0xe5, 1, {0x01}},
    {0xe6, 1, {0x19}},
    {0xe7, 1, {0x10}},
    {0xe8, 1, {0x10}},
    {0xea, 1, {0x12}},
    {0xeb, 1, {0xd0}},
    {0xec, 1, {0x04}},
    {0xed, 1, {0x07}},
    {0xee, 1, {0x07}},
    {0xef, 1, {0x09}},
    {0xf0, 1, {0xd0}},
    {0xf1, 1, {0x0e}},
    {0xf2, 4, {0x2e, 0x1b, 0x0b, 0x20}},
    {0xf9, 1, {0x17}},
    {0xe9, 1, {0xa8}},
    {0xec, 1, {0x04}},
    {0x35, 1, {0x00}},
    {0x44, 2, {0x00, 0x10}},
    {0x46, 1, {0x10}},
    {0xff, 1, {0x00}},
    {0x3a, 1, {0x05}},
    {0x11, 0},
    /* {REGFLAG_DELAY, 80},  */
    {REGFLAG_DELAY, 12},
    {0x29, 0},
    /* {REGFLAG_DELAY, 20}, */
    {0x2a, 4, {0x00, 0x0c, 0x00, 0x99}},
    {0x2b, 4, {0x00, 0x00, 0x01, 0xab}},
    {0x2c, 0},
};

static void lcd_init_code(const InitCode *code, u8 cnt)
{
    for (u8 i = 0; i < cnt; i++) {
        if (code[i].cmd == REGFLAG_DELAY) {
            lcd_delay(code[i].cnt * 10);
            continue;
        }

        WriteCOM(__LCD_ID, code[i].cmd);
        for (u8 j = 0; j < code[i].cnt; j++) {
            WriteDAT_8(__LCD_ID, code[i].dat[j]);
        }
    }
}

static void lcd_spi_nv3006a_enter_sleep(void)
{
    WriteCOM(__LCD_ID, 0x28);
    lcd_delay(120);
    WriteCOM(__LCD_ID, 0x10);
    lcd_delay(50);
}

static void lcd_spi_nv3006a_exit_sleep(void)
{
    WriteCOM(__LCD_ID, 0x11);
    lcd_delay(120) ;
    WriteCOM(__LCD_ID, 0x29);
}

static void lcd_spi_142x428_nv3006a_reset(void)
{
    /* printf("[LCD]nv3006a 142x428 spi lcd reset\n"); */
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

static int lcd_spi_142x428_nv3006a_init(struct lcd_board_cfg *bd_cfg)
{
    printf("[LCD]nv3006a 142x428 spi lcd init start\n");
    lcd_spi_142x428_nv3006a_reset();
    return 0;
}

#ifdef LCD_TEST_MODE
static u8 *test_buf;
static u8 *lcd_142x428_nv3006a_test_buf_prepra(void)
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

static int lcd_spi_142x428_nv3006a_draw_page(void *data)
{
    WriteCOM(__LCD_ID, 0x2c);

#ifdef LCD_TEST_MODE
    if (!test_buf) {
        test_buf = lcd_142x428_nv3006a_test_buf_prepra();
    }
    WriteDAT_one_page(__LCD_ID, test_buf, __LCD_W * __LCD_H * 2);
#else
    WriteDAT_one_page(__LCD_ID, (u8 *)data, __LCD_W * __LCD_H * 2);
#endif

    return 0;
}

static void lcd_spi_142x428_nv3006a_bl_ctrl(struct lcd_board_cfg *bd_cfg, u8 onoff)
{
    if (onoff) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

static int lcd_spi_142x428_nv3006a_send_init_code(struct lcd_board_cfg *bd_cfg)
{
    lcd_init_code(lcd_spi_nv3006a_142x428_code, ARRAY_SIZE(lcd_spi_nv3006a_142x428_code));
    return 0;
}

#define NV3006A_CMD_ID1 0xDA
#define NV3006A_CMD_ID2 0xDB
#define NV3006A_CMD_ID3 0xDC
#define NV3006A_ID1     0x30
#define NV3006A_ID2     0x07
#define NV3006A_ID3     0x01
static int lcd_spi_142x428_nv3006a_check_id(struct lcd_board_cfg *bd_cfg)
{
#if 0 // 默认关闭
    u8 data[3] = {0};
    ReadDAT(__LCD_ID, NV3006A_CMD_ID1, &data[0], 1);
    ReadDAT(__LCD_ID, NV3006A_CMD_ID2, &data[1], 1);
    ReadDAT(__LCD_ID, NV3006A_CMD_ID3, &data[2], 1);
    printf("read NV3006A ID :\n");
    put_buf(data, sizeof(data));
    if ((data[0] != NV3006A_ID1) || (data[1] != NV3006A_ID2) || (data[2] != NV3006A_ID3)) {
        printf("NV3006A check fail!\n");
        return -1;
    }
#endif
    return 0;
}

REGISTER_LCD_SPI_DEVICE_BEGIN(lcd_spi_142x428_nv3006a_dev) = {
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

REGISTER_LCD_DEVICE_DRIVE(lcd_spi_142x428_nv3006a)  = {
    .logo            = "SPI_142x428_NV3006A",
    .id              = __LCD_ID,
    .type		     = LCD_SPI,
    .dev    	     = &lcd_spi_142x428_nv3006a_dev,
    .init		     = lcd_spi_142x428_nv3006a_init,
    .draw            = lcd_spi_142x428_nv3006a_draw_page,
    .bl_ctrl	     = lcd_spi_142x428_nv3006a_bl_ctrl,
    .check           = lcd_spi_142x428_nv3006a_check_id,
    .send_init_code  = lcd_spi_142x428_nv3006a_send_init_code,
};

#endif // TCFG_LCD_SPI_NV3006A_142X428
