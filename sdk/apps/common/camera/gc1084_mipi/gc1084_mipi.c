/******************************************************************************************
 *  File Name:  gc1084_mipi.c
 *  Created on: 2024/7/20
 *  Author:     xzpin
 ******************************************************************************************/

#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "asm/isp_alg.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"
#include "gc1084_mipi.h"

static u32 reset_gpios[2] = {-1, -1};
static u32 pwdn_gpios[2] = {-1, -1};

extern void *gc1084_get_ae_params();
extern void *gc1084_get_awb_params();
extern void *gc1084_get_iq_params();

extern void gc1084_ae_ev_init(u32 fps);

#define INPUT_CLK  24
#define PCLK    50

#define LINE_LENGTH_CLK     2222
#define FRAME_LENGTH30      750
#define FRAME_LENGTH25      900
#define FRAME_LENGTH27      833

static u32 cur_again = -1;
static u32 cur_dgain = -1;
static u32 cur_expline = -1;
static u32 cur_line_length_clk = -1;

static u32 line_length_clk = LINE_LENGTH_CLK;
static u32 uframelen = FRAME_LENGTH30;

#define WRCMD 0x6e
#define RDCMD 0x6f

typedef struct {
    u16 addr;
    u8 value;
} sensor_reg_ini;

sensor_reg_ini GC1084_INI_REG[] = {
    //window size=1280x720,frame rate=30fps
    //mipi_data_rate=400Mbps/lane
    //HB=2200,FL=750,Rowtime=44.4444us
    0x03fe, 0xf0,
    0x03fe, 0xf0,
    0x03fe, 0xf0,
    0x03fe, 0x00,
    0x03f2, 0x00,
    0x03f3, 0x00,
    0x03f4, 0x36,
    0x03f5, 0xc0,
    0x03f6, 0x13,
    0x03f7, 0x01,
    0x03f8, 0x32,
    0x03f9, 0x21,
    0x03fc, 0xae,
    0x0d05, 0x08,
    0x0d06, 0xae,
    0x0d08, 0x10,
    0x0d0a, 0x02,
    0x000c, 0x03,
    0x0d0d, 0x02,
    0x0d0e, 0xd4,
    0x000f, 0x05,
    0x0010, 0x08,
    0x0017, 0x08,
    0x0d73, 0x92,
    0x0076, 0x00,
    0x0d76, 0x00,
    0x0d41, 0x02,
    0x0d42, 0xee,
    0x0d7a, 0x0a,
    0x006b, 0x18,
    0x0db0, 0x9d,
    0x0db1, 0x00,
    0x0db2, 0xac,
    0x0db3, 0xd5,
    0x0db4, 0x00,
    0x0db5, 0x97,
    0x0db6, 0x09,
    0x00d2, 0xfc,
    0x0d19, 0x31,
    0x0d20, 0x40,
    0x0d25, 0xcb,
    0x0d27, 0x03,
    0x0d29, 0x40,
    0x0d43, 0x20,
    0x0058, 0x60,
    0x00d6, 0x66,
    0x00d7, 0x19,
    0x0093, 0x02,
    0x00d9, 0x14,
    0x00da, 0xc1,
    0x0d2a, 0x00,
    0x0d28, 0x04,
    0x0dc2, 0x84,
    0x0050, 0x30,
    0x0080, 0x07,
    0x008c, 0x05,
    0x008d, 0xa8,
    0x0077, 0x01,
    0x0078, 0xee,
    0x0079, 0x02,
    0x0067, 0xc0,
    0x0054, 0xff,
    0x0055, 0x02,
    0x0056, 0x00,
    0x0057, 0x04,
    0x005a, 0xff,
    0x005b, 0x07,
    0x00d5, 0x03,
    0x0102, 0xa9,
    0x0d03, 0x02,
    0x0d04, 0xd0,
    0x007a, 0x60,
    0x04e0, 0xff,
    0x0414, 0x75,
    0x0415, 0x75,
    0x0416, 0x75,
    0x0417, 0x75,
    0x0122, 0x00,
    0x0121, 0x80,
    0x0428, 0x10,
    0x0429, 0x10,
    0x042a, 0x10,
    0x042b, 0x10,
    0x042c, 0x14,
    0x042d, 0x14,
    0x042e, 0x18,
    0x042f, 0x18,
    0x0430, 0x05,
    0x0431, 0x05,
    0x0432, 0x05,
    0x0433, 0x05,
    0x0434, 0x05,
    0x0435, 0x05,
    0x0436, 0x05,
    0x0437, 0x05,
    0x0153, 0x00,
    0x0190, 0x01,
    0x0192, 0x02,
    0x0194, 0x04,
    0x0195, 0x02,
    0x0196, 0xd0,
    0x0197, 0x05,
    0x0198, 0x00,
    0x0201, 0x23,
    0x0202, 0x53,
    0x0203, 0xce,
    0x0208, 0x39,
    0x0212, 0x06,
    0x0213, 0x40,
    0x0215, 0x12,
    0x0229, 0x05,
    0x023e, 0x98,
    0x031e, 0x3e,
};

static void *iic = NULL;

unsigned char wr_gc1084_reg(u16 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        goto __wend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;
        goto __wend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;
        goto __wend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        goto __wend;
    }
__wend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    if (ret == 0) {
        x_printf("wreg iic fail\n");
    }
    return ret;

}

unsigned char rd_gc1084_reg(u16 regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;
        goto __rend;
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regID & 0xff)) {
        ret = 0;
        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, RDCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(1000);

    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat);

__rend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    if (ret == 0) {
        x_printf("rreg iic fail\n");
    }
    return ret;

}

/*************************************************************************************************
    sensor api
*************************************************************************************************/


void gc1084_config_sensor(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;

    for (i = 0; i < sizeof(GC1084_INI_REG) / sizeof(sensor_reg_ini); i++) {
        wr_gc1084_reg(GC1084_INI_REG[i].addr, GC1084_INI_REG[i].value);
    }
    return;
}

s32 gc1084_set_output_size(u16 *width, u16 *height, u8 *frame_freq)
{
    return 0;
}

s32 gc1084_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

s32 gc1084_id_check(void)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i ;
    for (i = 0; i < 5; i++) {
        rd_gc1084_reg(0x03f0, &pid);
        rd_gc1084_reg(0x03f1, &ver);
        x_printf("%s  0x%x 0x%x\n", __FUNCTION__,  pid, ver);
        if (pid == 0x10 && ver == 0x84) {
            x_puts("\n hello GC1084-----\n");
            return 0;
        }
    }
    x_puts("\n----not GC1084MIPI-----\n");
    return -1;
}

void gc1084_reset(u8 isp_dev)
{
    u32 reset_gpio;
    u32 pwdn_gpio;
    if (isp_dev == ISP_DEV_0) {
        reset_gpio = reset_gpios[0];
        pwdn_gpio = pwdn_gpios[0];
    } else {
        reset_gpio = reset_gpios[1];
        pwdn_gpio = pwdn_gpios[1];
    }
    x_printf("pwdn_gpio=%d\n", pwdn_gpio);
    gpio_direction_output(pwdn_gpio, 1);
    delay(20000);
    gpio_direction_output(reset_gpio, 0);
    delay(20000);
    gpio_direction_output(reset_gpio, 1);
    delay(20000);
}

void GC1084_change_fps(u8 sence, u16 lv)
{
    x_printf("%s: %d %d\n", __FUNCTION__, sence, lv);
}

static u8 cur_sensor_type = 0xff;

s32 gc1084_check(u8 _isp_dev, u32 _reset_gpio, u32 pwdn_gpio, char *iic_name)
{
    x_puts("\n\n gc1084_check \n\n");
    u8 isp_dev = 0;
    if (!iic) {
        iic = dev_open(iic_name, 0);
        if (!iic) {
            return -1;
        }
    } else {
        if (cur_sensor_type != isp_dev) {
            return -1;
        }
    }

    x_printf("\n\n isp_dev =%d\n\n", isp_dev);

    reset_gpios[isp_dev] = _reset_gpio;
    pwdn_gpios[isp_dev] = pwdn_gpio;

    gc1084_reset(isp_dev);

    x_puts("gc1084_id_check\n");
    if (0 != gc1084_id_check()) {

        dev_close(iic);
        iic = NULL;

        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;
}

s32 gc1084_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    x_puts("\n\n gc1084_init22 \n\n");

    cur_expline = -1;
    cur_again = -1;
    cur_dgain = -1;

    gc1084_config_sensor(width, height, format, frame_freq);
    gc1084_ae_ev_init(25);

    return 0;
}

u8 regValTable1084[29][6] = {
    {0x00, 0x00, 0x00, 0x01, 0x00, 0x00},
    {0x0A, 0x00, 0x00, 0x01, 0x0c, 0x00},
    {0x00, 0x01, 0x00, 0x01, 0x1a, 0x00},
    {0x0A, 0x01, 0x00, 0x01, 0x2a, 0x00},
    {0x00, 0x02, 0x00, 0x02, 0x00, 0x00},
    {0x0A, 0x02, 0x00, 0x02, 0x18, 0x00},
    {0x00, 0x03, 0x00, 0x02, 0x33, 0x00},
    {0x0A, 0x03, 0x00, 0x03, 0x14, 0x00},
    {0x00, 0x04, 0x00, 0x04, 0x00, 0x02},
    {0x0A, 0x04, 0x00, 0x04, 0x2f, 0x02},
    {0x00, 0x05, 0x00, 0x05, 0x26, 0x02},
    {0x0A, 0x05, 0x00, 0x06, 0x29, 0x02},
    {0x00, 0x06, 0x00, 0x08, 0x00, 0x02},
    {0x0A, 0x06, 0x00, 0x09, 0x1f, 0x04},
    {0x12, 0x46, 0x00, 0x0b, 0x0d, 0x04},
    {0x19, 0x66, 0x00, 0x0d, 0x12, 0x06},
    {0x00, 0x04, 0x01, 0x10, 0x00, 0x06},
    {0x0A, 0x04, 0x01, 0x12, 0x3e, 0x08},
    {0x00, 0x05, 0x01, 0x16, 0x1a, 0x0a},
    {0x0A, 0x05, 0x01, 0x1a, 0x23, 0x0c},
    {0x00, 0x06, 0x01, 0x20, 0x00, 0x0c},
    {0x0A, 0x06, 0x01, 0x25, 0x3b, 0x0f},
    {0x12, 0x46, 0x01, 0x2c, 0x33, 0x12},
    {0x19, 0x66, 0x01, 0x35, 0x06, 0x14},
    {0x20, 0x06, 0x01, 0x3f, 0x3f, 0x15},
};
u32 gainLevelTable1084[26] = {
    64,
    76,
    90,
    106,
    128,
    152,
    179,
    212,
    256,
    303,
    358,
    425,
    512,
    607,
    717,
    849,
    1024,
    1213,
    1434,
    1699,
    2048,
    2427,
    2867,
    3398,
    4096,
    0xffffffff,
};

static void set_again(u32 again)
{
    if (cur_again == again) {
        return;
    }

    cur_again = again;

    int i;
    u32 tol_dig_gain = 0;
    u32 gain = again / 16;

    int total = sizeof(gainLevelTable1084) / sizeof(u32);
    for (i = 0; i < total; i++) {
        if ((gainLevelTable1084[i] <= gain) && (gain < gainLevelTable1084[i + 1])) {
            break;
        }
    }

    tol_dig_gain = gain * 64 / gainLevelTable1084[i];

    wr_gc1084_reg(0x00d1, regValTable1084[i][0]);
    wr_gc1084_reg(0x00d0, regValTable1084[i][1]);

    wr_gc1084_reg(0x031d, 0x2e),
                  wr_gc1084_reg(0x0dc1, regValTable1084[i][2]),
                  wr_gc1084_reg(0x031d, 0x28),
                  wr_gc1084_reg(0x00b8, regValTable1084[i][3]);
    wr_gc1084_reg(0x00b9, regValTable1084[i][4]);
    wr_gc1084_reg(0x0155, regValTable1084[i][5]);

    wr_gc1084_reg(0x00b1, (tol_dig_gain >> 6));
    wr_gc1084_reg(0x00b2, ((tol_dig_gain & 0x3f) << 2));

    return;
}

static void set_dgain(u32 dgain)
{
    if (cur_dgain == dgain) {
        return;
    }
    cur_dgain = dgain;
}

static void calc_gain(u32 gain, u32 *_again, u32 *_dgain)
{
    if (gain < 1024) {
        gain = 1024;
    }
    if (gain > 128 * 1024) {
        gain = 128 * 1024;
    }

    *_again = gain;
    *_dgain = 1024;
}

static void set_shutter(u32 texp)
{
    if (cur_expline == texp) {
        return;
    }

    cur_expline  = texp;
    wr_gc1084_reg(0x0d03, (texp >> 8) & 0xff);
    wr_gc1084_reg(0x0d04, (texp >> 0) & 0xff);
}

//pclk 48M
u32 gc1084_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp;
    u32 texp_align;
    u32 ratio;
    u32 fLen;
    static u8 bLow = 0;

    fLen = uframelen;
    if (exp_time_us > 40000) {
        fLen = uframelen * 12;
        x_printf("ex= %d\n", exp_time_us);
    }

    texp = exp_time_us * PCLK / LINE_LENGTH_CLK;

    if (texp < 4) {
        texp = 4;
    }
    if (texp > fLen  - 8) {
        texp = fLen  - 8;
    }
    texp_align = (texp) * LINE_LENGTH_CLK / (PCLK);

    if (texp_align < exp_time_us) {
        ratio = (exp_time_us) * (1 << 10) / texp_align;
        //x_printf("ratio = %d\n", ratio);
    } else {
        ratio = (1 << 10);
    }

    shutter->ae_exp_line =  texp;
    shutter->ae_gain = (gain * ratio) >> 10;
    shutter->ae_exp_clk = 0;

    //x_printf("exp_time_us=%d, texp=%d, gain=%d->%d  gain_val = %d\n", exp_time_us, texp, gain, shutter->ae_gain, gain >> 10);
    return 0;
}

u32 gc1084_set_shutter(isp_ae_shutter_t *shutter)
{
    u32 again, dgain;

    calc_gain((shutter->ae_gain), &again, &dgain);
    set_shutter(shutter->ae_exp_line);
    set_again(again);
    set_dgain(dgain);

    return 0;
}

void gc1084_sleep()
{

}

void gc1084_wakeup()
{

}

void gc1084_wr_reg(u8 isp_dev, u16 addr, u16 val)
{
    x_printf("update reg%x with %x\n", addr, val);
    wr_gc1084_reg((u16)addr, (u8)val);
}

u16 gc1084_rd_reg(u8 isp_dev, u16 addr)
{
    u8 val;
    rd_gc1084_reg((u16)addr, &val);
    return val;
}


REGISTER_CAMERA(GC1084) = {
    .logo               = "GC1084M",
    .isp_dev            = ISP_DEV_NONE,
    .in_format          = SEN_IN_FORMAT_GRBG,
    .out_format         = ISP_OUT_FORMAT_YUV,
    .mbus_type          = SEN_MBUS_CSI2,
    .mbus_config        = SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_CSI2_1_LANE,
    .fps                = 30,

    .sen_size           = {GC1084_INPUT_W, GC1084_INPUT_H},
    .isp_size           = {GC1084_OUTPUT_W, GC1084_OUTPUT_H},

    .cap_fps                = 30,
    .sen_cap_size           = {GC1084_INPUT_W, GC1084_INPUT_H},
    .isp_cap_size           = {GC1084_OUTPUT_W, GC1084_OUTPUT_H},

    .ops                =   {
        .avin_fps           = NULL,
        .avin_valid_signal  = NULL,
        .avin_mode_det      = NULL,
        .sensor_check       = gc1084_check,
        .init               = gc1084_init,
        .set_size_fps       = gc1084_set_output_size,
        .power_ctrl         = gc1084_power_ctl,

        .get_ae_params      = gc1084_get_ae_params,
        .get_awb_params     = gc1084_get_awb_params,
        .get_iq_params      = gc1084_get_iq_params,

        .sleep              = gc1084_sleep,
        .wakeup             = gc1084_wakeup,
        .write_reg          = gc1084_wr_reg,
        .read_reg           = gc1084_rd_reg,
    }
};


