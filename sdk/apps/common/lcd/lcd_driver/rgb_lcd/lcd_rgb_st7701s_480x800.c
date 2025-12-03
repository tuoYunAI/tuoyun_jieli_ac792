#include "lcd_driver.h"
#include "lcd_config.h"
#include "device/gpio.h"

/* 测试屏幕好坏开这个宏 */
/* #define USE_TEST_MODE     //推纯色 */

#if TCFG_LCD_RGB_ST7701S_480x800

#define __LCD_W  LCD_W
#define __LCD_H  LCD_H
#define __LCD_ID LCD_ID

#define SDI     TCFG_LCD_RGB_ST7701S_SDA
#define SCL     TCFG_LCD_RGB_ST7701S_SCL

#define delay_ms    msleep

static void lcd_480x800_st7701s_backctrl(struct lcd_board_cfg *bd_cfg, u8 on)
{
    if (-1 == bd_cfg->lcd_io.backlight) {
        return;
    }
    if (on) {
        gpio_direction_output(bd_cfg->lcd_io.backlight, bd_cfg->lcd_io.backlight_value);
    } else {
        gpio_direction_output(bd_cfg->lcd_io.backlight, !bd_cfg->lcd_io.backlight_value);
    }
}

//模拟spi时序
static void SPI_SendData(unsigned short temp)
{
    unsigned char i;
    gpio_set_mode(SCL, GPIO_OUTPUT_LOW);
    gpio_set_mode(SDI, GPIO_OUTPUT_LOW);
    for (i = 0; i < 9; i++) {
        gpio_direction_output(SCL, 0);
        if (temp & 0x0100) {
            gpio_direction_output(SDI, 1);
        } else {
            gpio_direction_output(SDI, 0);
        }
        delay_us(1);
        gpio_direction_output(SCL, 1);
        delay_us(1);
        temp = temp << 1;
    }
}

//命令
static void SPI_WriteComm(u8 c)
{
    unsigned short temp = 0x00;
    temp = temp | c;
    lcd_cs_pinstate(__LCD_ID, 0);
    SPI_SendData(temp);
    lcd_cs_pinstate(__LCD_ID, 1);
    delay_us(1);
}

//数据
static void SPI_WriteData(u8 c)
{
    unsigned short temp = 0x100;
    temp = temp | c;
    lcd_cs_pinstate(__LCD_ID, 0);
    SPI_SendData(temp);
    lcd_cs_pinstate(__LCD_ID, 1);
    delay_us(1);
}

static u8 SPI_ReadData(void)
{
    u8 temp = 0;
    gpio_set_mode(SDI, GPIO_INPUT_FLOATING);
    gpio_set_mode(SCL, GPIO_OUTPUT_LOW);
    for (u8 i = 0; i < 8; i++) {
        gpio_direction_output(SCL, 0);
        delay_us(1);
        gpio_direction_output(SCL, 1);
        delay_us(1);
        temp = temp << 1;
        if (gpio_read(SDI)) {
            temp |= 1;
        }
    }
    return temp;
}

static u8 SPI_Read_Register(u8 cmd)
{
    unsigned short temp = 0;
    u8 data;
    temp = temp | cmd;
    lcd_cs_pinstate(__LCD_ID, 0);
    delay_us(1);
    SPI_SendData(temp);
    delay_us(1);
    data = SPI_ReadData();
    lcd_cs_pinstate(__LCD_ID, 1);
    return data;
}

static void W_COM(uint8_t c)
{
    SPI_WriteComm(c);
}

static void W_D(uint8_t c)
{
    SPI_WriteData(c);
}

static int lcd_480x800_st7701s_init(struct lcd_board_cfg *bd_cfg)
{
    printf("lcd 480x800_st7701s init ...\n");
    lcd_cs_pinstate(__LCD_ID, 1);
    if (bd_cfg->lcd_io.lcd_reset != -1) {
        printf("[sanqi] %s - %d\n", __FUNCTION__, __LINE__);
        lcd_rst_pinstate(__LCD_ID, 1);
        delay_ms(1);
        lcd_rst_pinstate(__LCD_ID, 0);
        delay_ms(1);
        lcd_rst_pinstate(__LCD_ID, 1);
        delay_ms(120);
    } else {
        // lcd复用了TP的reset时，加延时让lcd在reset信号之后再初始化，否则lcd可能不显示。
        os_time_dly(30);
    }

    return 0;
}

static void st7701s_check_self(void)
{
    W_COM(0xFF);
    W_D(0x77);
    W_D(0x01);
    W_D(0x00);
    W_D(0x00);
    W_D(0x12);
    W_COM(0xD1);
    W_D(0x81);
    W_COM(0xD2);
    W_D(0x08);
}

static int lcd_480x800_st7701s_send_init_code(struct lcd_board_cfg *bd_cfg)
{
    W_COM(0x01); // soft reset. st7701s硬复位后发现还是可能出现不显示，加软复位命令可以解决.
    delay_ms(10);

    W_COM(0x11);
    delay_ms(120);
//---------------------------Bank0 Setting---------------------------//
//------------Display Control setting------------//
    /* st7701s_check_self(); */
    W_COM(0xFF);
    W_D(0x77);
    W_D(0x01);
    W_D(0x00);
    W_D(0x00);
    W_D(0x10);
    W_COM(0xC0);
    W_D(0x63);
    W_D(0x00);
    W_COM(0xC1);
    W_D(0x11);
    W_D(0x02);
    W_COM(0xC2);
    W_D(0x31);
    W_D(0x08);
    W_COM(0xC3);
    W_D(0x10);
    W_COM(0xCC);
    W_D(0x10);
//------------Gamma Cluster Setting--------------//
    W_COM(0xB0);
    W_D(0x40);
    W_D(0x01);
    W_D(0x46);
    W_D(0x0D);
    W_D(0x13);
    W_D(0x09);
    W_D(0x05);
    W_D(0x09);
    W_D(0x09);
    W_D(0x1B);
    W_D(0x07);
    W_D(0x15);
    W_D(0x12);
    W_D(0x4C);
    W_D(0x10);
    W_D(0xC8);
    W_COM(0xB1);
    W_D(0x40);
    W_D(0x02);
    W_D(0x86);
    W_D(0x0D);
    W_D(0x13);
    W_D(0x09);
    W_D(0x05);
    W_D(0x09);
    W_D(0x09);
    W_D(0x1F);
    W_D(0x07);
    W_D(0x15);
    W_D(0x12);
    W_D(0x15);
    W_D(0x19);
    W_D(0x08);
//------------- End Gamma Setting--------------------//
//------------End Display Control setting------------//
//-----------------------Bank0 Setting End------------------------//


//---------------------- Bank1 Setting----------------------------//
//----------- Power Control Registers Initial -------- //
    W_COM(0xFF);
    W_D(0x77);
    W_D(0x01);
    W_D(0x00);
    W_D(0x00);
    W_D(0x11);
    W_COM(0xB0);
    W_D(0x50);
//--------------- Vcom Setting------------------ //
    W_COM(0xB1);
    W_D(0x68);
//------------- End Vcom Setting---------------- //
    W_COM(0xB2);
    W_D(0x07);
    W_COM(0xB3);
    W_D(0x80);
    W_COM(0xB5);
    W_D(0x47);
    W_COM(0xB7);
    W_D(0x85);
    W_COM(0xB8);
    W_D(0x21);
    W_COM(0xB9);
    W_D(0x10);
    W_COM(0xC1);
    W_D(0x78);
    W_COM(0xC2);
    W_D(0x78);
    W_COM(0xD0);
    W_D(0x88);
//-----End Power Control Registers Initial ------//
    delay_ms(100);
//----------------GIP Setting--------------------//
    W_COM(0xE0);
    W_D(0x00);
    W_D(0x00);
    W_D(0x02);
    W_COM(0xE1);
    W_D(0x08);
    W_D(0x00);
    W_D(0x0A);
    W_D(0x00);
    W_D(0x07);
    W_D(0x00);
    W_D(0x09);
    W_D(0x00);
    W_D(0x00);
    W_D(0x33);
    W_D(0x33);
    W_COM(0xE2);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);
    W_COM(0xE3);
    W_D(0x00);
    W_D(0x00);
    W_D(0x33);
    W_D(0x33);
    W_COM(0xE4);
    W_D(0x44);
    W_D(0x44);
    W_COM(0xE5);
    W_D(0x0E);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_D(0x10);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_D(0x0A);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_D(0x0C);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_COM(0xE6);
    W_D(0x00);
    W_D(0x00);
    W_D(0x33);
    W_D(0x33);
    W_COM(0xE7);
    W_D(0x44);
    W_D(0x44);
    W_COM(0xE8);
    W_D(0x0D);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_D(0x0F);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_D(0x09);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_D(0x0B);
    W_D(0x2D);
    W_D(0xA0);
    W_D(0xA0);
    W_COM(0xEB);
    W_D(0x02);
    W_D(0x01);
    W_D(0xE4);
    W_D(0xE4);
    W_D(0x44);
    W_D(0x00);
    W_D(0x40);
    W_COM(0xEC);
    W_D(0x02);
    W_D(0x01);
    W_COM(0xED);
    W_D(0xAB);
    W_D(0x89);
    W_D(0x76);
    W_D(0x54);
    W_D(0x01);
    W_D(0xFF);
    W_D(0xFF);
    W_D(0xFF);
    W_D(0xFF);
    W_D(0xFF);
    W_D(0xFF);
    W_D(0x10);
    W_D(0x45);
    W_D(0x67);
    W_D(0x98);
    W_D(0xBA);
//---------------- End GIP Setting------------------- //
//--------- Power Control Registers Initial End-------//
//-------------------------- Bank1 Setting--------------------------- //
    W_COM(0xFF);
    W_D(0x77);
    W_D(0x01);
    W_D(0x00);
    W_D(0x00);
    W_D(0x00);

    W_COM(0x3A);
    W_D(0x77);

    W_COM(0x29);
    return 0;
}

#define ST7701S_REG_ID1      0xDA
#define ST7701S_REG_ID2      0xDB
#define ST7701S_REG_ID3      0xDC
#define ST7701S_ID1          0xFF
#define ST7701S_ID2          0xFF
#define ST7701S_ID3          0xFF
static int lcd_480x800_st7701s_check_id(struct lcd_board_cfg *bd_cfg)
{
#if 0
    u8 data[3] = {0};
    /*
     * u8 temp = SPI_Read_Register(0x0A);
     * printf("read ST7701S 0x0A = 0x%x\n", temp);
     * temp = SPI_Read_Register(0x0C);
     * printf("read ST7701S 0x0C = 0x%x\n", temp);
     */

    data[0] = SPI_Read_Register(ST7701S_REG_ID1);
    data[1] = SPI_Read_Register(ST7701S_REG_ID2);
    data[2] = SPI_Read_Register(ST7701S_REG_ID3);
    printf("read ST7701S ID :\n");
    put_buf(data, sizeof(data));
    if ((data[0] != ST7701S_ID1) || (data[1] != ST7701S_ID2) || (data[2] != ST7701S_ID3)) {
        printf("ST7701S check fail!\n");
        /* return -1; */
    }
#endif
    return 0;
}

REGISTER_IMD_DEVICE_BEGIN(lcd_480x800_st7701s_dev) = {
    .info = {
#ifdef USE_TEST_MODE
        .test_mode 	     = true,
#else
        .test_mode 	     = false,
#endif
        .test_mode_color = 0xff0000,
        .bg_color    	 = 0x00ff00,
        .xres 			 = __LCD_W,
        .yres 			 = __LCD_H,
        .target_xres     = __LCD_W,
        .target_yres     = __LCD_H,
        .sample          = SAMP_YUV420,
        .format          = FORMAT_RGB888,
        .interlaced_1st_filed = EVEN_FILED,
        .interlaced_mode = INTERLACED_NONE,
        .len 			 = LEN_256,
        .rotate = ROTATE_0,				// 旋转配置
        .adjust = {
            .r_gain = 1.0,
            .g_gain = 1.0,
            .b_gain = 1.0,
            .bright_gain = 1.0,
            .contrast_gain = 1.0,
            .saturation_gain = 1.0,
            .mean = 128,
            .angle = 0,
            .mode = 0,
        },

        .in_rbs         = false, //输入数据RB交换
        .out_rbs        = false, //输出数据RB交换
        .in_fmt         = TCFG_LCD_INPUT_FORMAT,
        .out_fmt        = LCD_OUT_RGB888,
        .dither_en      = false, //使能dither 0低位截断 1dither
        .in_bt601       = false, //yuv422输入 转换0 bt601, 1 bt709
        .out_bt601      = false, //yuv422输出 转换0 bt601, 1 bt709
    },

    .data_out_endian = MODE_LE, //RGB565模式生效 RGB888调节swap

    .dclk_set 		= CLK_EN | CLK_NEGATIVE,
    .sync0_set      = SIGNAL_DEN | CLK_EN,
    .sync1_set      = SIGNAL_HSYNC | CLK_EN,
    .sync2_set      = SIGNAL_VSYNC | CLK_NEGATIVE | CLK_EN,

    .set_io_hd      = TCFG_LCD_RGB_IO_HD_LEVEL,
    .lcd_group      = PORT_GROUP_AA,//接口固定勿动
    .ncycle         = CYCLE_ONE,
    .data_width     = PORT_24BITS,
    .dclk_cfg       = DPI_CLK_STD_48M | DIVA_1 | DIVB_2,
    .timing = {
//下面的时间均指多少个时钟周期 调整de 或者行间隔可以调屏的大概效果
#define  HBP          15 //行同步信号后肩 单位：clk
#define  HFP          2  //行同步信号前肩 单位：clk
#define  HSPW         1  //水平信号宽度   单位：clk
#define  VSPW         2  //垂直信号宽度   单位：行
#define  VBP          16 //帧同步信号后肩 单位：行
#define  VFP          1  //帧同步信号前肩 单位：行

        //以下信号均为高有效1 以下为固定公式勿动
        .hori_total				= 480 * 1 + HBP + HFP + HSPW, //行总长 : 行像素*3 + 行同步信号后肩 + 行同步信号前肩 + 水平信号宽度
        .hori_sync				= 480 * 1 + HBP + HFP,        //行同步时间:  行像素*3 + 行同步信号后肩 + 行同步信号前肩
        .hori_back_porth		= HBP,                        //行同步信号后肩 (+HSPW?)
        .vert_total				= VSPW + VBP + 800 + VFP,     //帧总长: 垂直信号宽度 + 帧同步信号后肩 + 像素高 + 帧同步信号后肩
        .vert_sync				= VSPW,                       //帧于帧之间间隔多少个行中断:?
        .vert_back_porth_odd	= VBP + VSPW,                 //帧信号开始的时候经过多少个行中断开始de: 帧同步信号后肩 +  水平信号宽度
        .hori_pixel				= 480,                        //像素宽   480*3 = 1440
        .vert_pixel				= 800,                        //像素高
        .vert_back_porth_even	= 0,                          //推隔行数据才需要配置
    },
},
REGISTER_IMD_DEVICE_END()

REGISTER_LCD_DEVICE_DRIVE(lcd_dev) = {
    .logo            = "RGB_480x800_ST7701S",
    .id              = __LCD_ID,
    .type            = LCD_RGB,
    .dev             = &lcd_480x800_st7701s_dev,
    .init            = lcd_480x800_st7701s_init,
    .bl_ctrl         = lcd_480x800_st7701s_backctrl,
    .check           = lcd_480x800_st7701s_check_id,
    .send_init_code  = lcd_480x800_st7701s_send_init_code,
};

#endif

