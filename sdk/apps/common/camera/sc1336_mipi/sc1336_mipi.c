/******************************************************************************************
 *  File Name:  sc1336_mipi.c
 *  Created on: 2024/7/11
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
#include "sc1336_mipi.h"

#define LINE_LENGTH_CLK  1540
#define FRAME_LENGTH     1560
#define FRAME_LENGTH25   1872
#define SENSOR_FPS    30

#define PCLK       72
#define INPUT_CLK  24

static u8  SC1336_WRCMD = 0x60;
static u8  SC1336_RDCMD = 0x61;

static u32 cur_expline = -1;
static u32 cur_again = -1;
static u32 cur_dgain = -1;
static u32 cur_line_length_clk = -1;

static u32 reset_gpios[2] = {-1, -1};
static u32 pwdn_gpios[2] = {-1, -1};

static u8 cur_sensor_type = 0xff;

extern void *sc1336_mipi_get_ae_params();
extern void *sc1336_mipi_get_awb_params();
extern void *sc1336_mipi_get_iq_params();
extern void sc1336_mipi_ae_ev_init(u32 fps);

typedef struct {
    u16 addr;
    u8 value;
} sensor_reg_ini;

struct stVrefh {
    u16 index;
    u16 th_low;
    u16 th_high;
    u8 vrefh;
};

static sensor_reg_ini SC1336_MIPI_INI_REG[] = {
    0x0103, 0x01,
    0x0100, 0x00,
    0x36e9, 0x80,
    0x37f9, 0x80,
    0x3018, 0x1a,
    0x3019, 0x0e,
    0x301f, 0x14,
    0x3200, 0x00,
    0x3201, 0x00,
    0x3202, 0x00,
    0x3203, 0x00,
    0x3204, 0x05,
    0x3205, 0x07,
    0x3206, 0x02,
    0x3207, 0xd7,
    0x3208, 0x05,
    0x3209, 0x00,
    0x320a, 0x02,
    0x320b, 0xd0,
    0x320c, 0x06,
    0x320d, 0x40,
    0x3210, 0x00,
    0x3211, 0x04,
    0x3212, 0x00,
    0x3213, 0x04,
    0x3221, 0x66,// FLIP&MIRROR // 0x00 normal
    0x3248, 0x04,
    0x3249, 0x0b,
    0x3250, 0x40,
    0x3301, 0x03,
    0x3302, 0x10,
    0x3303, 0x10,
    0x3304, 0x40,
    0x3306, 0x38,
    0x3307, 0x02,
    0x3308, 0x08,
    0x3309, 0x60,
    0x330a, 0x00,
    0x330b, 0x70,
    0x330c, 0x16,
    0x330d, 0x10,
    0x330e, 0x10,
    0x3318, 0x02,
    0x331c, 0x01,
    0x331e, 0x39,
    0x331f, 0x59,
    0x3327, 0x0a,
    0x3333, 0x10,
    0x3334, 0x40,
    0x335e, 0x06,
    0x335f, 0x0a,
    0x3364, 0x1f,
    0x337a, 0x02,
    0x337b, 0x06,
    0x337c, 0x02,
    0x337d, 0x0e,
    0x3390, 0x01,
    0x3391, 0x07,
    0x3392, 0x0f,
    0x3393, 0x03,
    0x3394, 0x03,
    0x3395, 0x03,
    0x3396, 0x48,
    0x3397, 0x49,
    0x3398, 0x4f,
    0x3399, 0x02,
    0x339a, 0x04,
    0x339b, 0x10,
    0x339c, 0x90,
    0x33a2, 0x04,
    0x33a3, 0x04,
    0x33ad, 0x0c,
    0x33b1, 0x80,
    0x33b2, 0x50,
    0x33b3, 0x38,
    0x33f9, 0x38,
    0x33fb, 0x48,
    0x33fc, 0x4b,
    0x33fd, 0x4f,
    0x349f, 0x03,
    0x34a6, 0x49,
    0x34a7, 0x4f,
    0x34a8, 0x28,
    0x34a9, 0x00,
    0x34aa, 0x00,
    0x34ab, 0x70,
    0x34ac, 0x00,
    0x34ad, 0x80,
    0x3630, 0xc0,
    0x3631, 0x84,
    0x3632, 0x78,
    0x3633, 0x42,
    0x3637, 0x2a,
    0x363a, 0x88,
    0x363b, 0x03,
    0x363c, 0x08,
    0x3641, 0x3a,
    0x3670, 0x0f,
    0x3674, 0xb0,
    0x3675, 0xc0,
    0x3676, 0xc0,
    0x367c, 0x40,
    0x367d, 0x48,
    0x3690, 0x33,
    0x3691, 0x43,
    0x3692, 0x53,
    0x3693, 0x84,
    0x3694, 0x88,
    0x3695, 0x8c,
    0x3698, 0x89,
    0x3699, 0x92,
    0x369a, 0xa5,
    0x369b, 0xca,
    0x369c, 0x48,
    0x369d, 0x4f,
    0x369e, 0x48,
    0x369f, 0x4b,
    0x36a2, 0x49,
    0x36a3, 0x4b,
    0x36a4, 0x4f,
    0x36a6, 0x49,
    0x36a7, 0x4b,
    0x36ab, 0x74,
    0x36ac, 0x74,
    0x36ad, 0x78,
    0x36d0, 0x01,
    0x36ea, 0x0c,
    0x36eb, 0x1c,
    0x36ec, 0x0c,
    0x36ed, 0x28,
    0x370f, 0x01,
    0x3722, 0x01,
    0x3724, 0x41,
    0x3725, 0xc4,
    0x37b0, 0x01,
    0x37b1, 0x01,
    0x37b2, 0x01,
    0x37b3, 0x4b,
    0x37b4, 0x4f,
    0x37fa, 0x0c,
    0x37fb, 0x35,
    0x37fc, 0x01,
    0x37fd, 0x07,
    0x3900, 0x0d,
    0x3902, 0xdf,
    0x3905, 0xb8,
    0x3908, 0x41,
    0x391b, 0x81,
    0x391c, 0x10,
    0x391f, 0x30,
    0x3933, 0x81,
    0x3934, 0xd9,
    0x3940, 0x70,
    0x3941, 0x00,
    0x3942, 0x01,
    0x3943, 0xdc,
    0x3952, 0x02,
    0x3953, 0x0f,
    0x3e01, 0x5d,
    0x3e02, 0x40,
    0x3e08, 0x1f,
    0x3e1b, 0x14,
    0x4509, 0x1c,
    0x4800, 0x44,
    0x4819, 0x05,
    0x481b, 0x03,
    0x481d, 0x0a,
    0x481f, 0x02,
    0x4821, 0x08,
    0x4823, 0x03,
    0x4825, 0x02,
    0x4827, 0x03,
    0x4829, 0x04,
    0x4831, 0x02,
    0x5799, 0x06,
    0x5ae0, 0xfe,
    0x5ae1, 0x40,
    0x5ae2, 0x30,
    0x5ae3, 0x28,
    0x5ae4, 0x20,
    0x5ae5, 0x30,
    0x5ae6, 0x28,
    0x5ae7, 0x20,
    0x5ae8, 0x3c,
    0x5ae9, 0x30,
    0x5aea, 0x28,
    0x5aeb, 0x3c,
    0x5aec, 0x30,
    0x5aed, 0x28,
    0x5aee, 0xfe,
    0x5aef, 0x40,
    0x5af4, 0x30,
    0x5af5, 0x28,
    0x5af6, 0x20,
    0x5af7, 0x30,
    0x5af8, 0x28,
    0x5af9, 0x20,
    0x5afa, 0x3c,
    0x5afb, 0x30,
    0x5afc, 0x28,
    0x5afd, 0x3c,
    0x5afe, 0x30,
    0x5aff, 0x28,
    0x36e9, 0x20,
    0x37f9, 0x27,
    0x0100, 0x01,
};

static void *iic = NULL;

unsigned char wr_sc1336_mipi_reg(u16 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, SC1336_WRCMD)) {
        ret = 0;
        goto __wend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;
        goto __wend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;
        goto __wend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {

    }
__wend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}

unsigned char rd_sc1336_mipi_reg(u16 regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, SC1336_WRCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regID & 0xff)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, SC1336_RDCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat)) {
        ret = 0;
        goto __rend;
    }
__rend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}

void sc1336_mipi_config_sensor(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    //u8 v;

    for (i = 0; i < sizeof(SC1336_MIPI_INI_REG) / sizeof(sensor_reg_ini); i++) {
        wr_sc1336_mipi_reg(SC1336_MIPI_INI_REG[i].addr, SC1336_MIPI_INI_REG[i].value);
        //rd_sc1336_mipi_reg(SC1336_MIPI_INI_REG[i].addr, &v);
        //x_printf("%02x: %02x->%02x\n", SC1336_MIPI_INI_REG[i].addr, SC1336_MIPI_INI_REG[i].value, v);
    }

    sc1336_mipi_ae_ev_init(*frame_freq);

    return;
}

s32 sc1336_mipi_set_output_size(u16 *width, u16 *height, u8 *frame_freq)
{
    return 0;
}

s32 sc1336_mipi_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

s32 sc1336_mipi_id_check(void)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i;

    for (i = 0; i < 3; i++) {
        rd_sc1336_mipi_reg(0x3107, &pid);
        rd_sc1336_mipi_reg(0x3108, &ver);

        x_printf("sensor PID %x %x %x %x\n", pid, ver, SC1336_WRCMD, SC1336_RDCMD);
        if (pid == 0xca && ver == 0x3f) {
            break;
        }
    }

    if (pid != 0xca || ver != 0x3f) {
        x_puts("\n----not SC1336_MIPI-----\n");
        return -1;
    }
    x_puts("\n----hello SC1336_MIPI-----\n");
    return 0;
}

void sc1336_mipi_reset(u8 isp_dev)
{
    x_puts("SC1336_MIPI reset \n");

    u32 reset_gpio;
    u32 pwdn_gpio;

    x_printf("ISP_DEV_0 = %d, isp_dev = %d\n", ISP_DEV_0, isp_dev);
    if (isp_dev == ISP_DEV_0) {
        reset_gpio = reset_gpios[0];
        pwdn_gpio = pwdn_gpios[0];
    } else {
        reset_gpio = reset_gpios[1];
        pwdn_gpio = pwdn_gpios[1];
    }

    x_printf("gpio reset = %x, pwdn = %x\n", reset_gpio, pwdn_gpio);
    gpio_direction_output(pwdn_gpio, 1);
    gpio_direction_output(reset_gpio, 0);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    delay(40000);

}

s32 sc1336_mipi_check(u8 _isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
{
    u8 isp_dev = 0;
    user_printf("\n <<<<<  sSC1336INI:%s, %d>>>>>>>>>>>>>>>>>>>>>>>>\n ", __FUNCTION__, __LINE__);
    x_printf("\nsc1336 reset:%x, pwdn: %x, _isp_dev: %x\n", reset_gpio, pwdn_gpio, isp_dev);

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
    x_printf("\n\nsc1336 isp_dev = %d\n\n", isp_dev);

    reset_gpios[isp_dev] = reset_gpio;
    pwdn_gpios[isp_dev] = pwdn_gpio;

    sc1336_mipi_reset(isp_dev);

    if (0 != sc1336_mipi_id_check()) {
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;
}

s32 sc1336_mipi_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    user_printf("\n <<<<<  finSC1336INI:%s, %d>>>>>>>>>>>>>>>>>>>>>>>>\n ", __FUNCTION__, __LINE__);
    x_puts("\n\n sc1336_mipi_init \n\n");
    sc1336_mipi_config_sensor(width, height, format, frame_freq);
    return 0;
}

static void set_again(u32 again)
{
    u8 again_s0 = 0;
    u8 again_s1 = 0;

    if (cur_again == again) {
        return;
    }

    cur_again = again;
    wr_sc1336_mipi_reg(0x3e08, 0x3f);
    wr_sc1336_mipi_reg(0x3e09, (again >> 0) & 0xff);

    return;
}

static void set_dgain(u32 dgain)
{
    u8 dgain_s0 = 0;
    u8 dgain_s1 = 0;

    if (cur_dgain == dgain) {
        return;
    }

    cur_dgain = dgain;
    wr_sc1336_mipi_reg(0x3e06, 0x00);
    wr_sc1336_mipi_reg(0x3e07, (dgain >> 0) & 0xff);
}

#define AGAIN_STEP 0x800         //(0x800)
#define MAX_AGAIN (64*1024)      //15.875
#define MAX_DGAIN 31*1024        //4*1024

static void calc_gain(u32 gain, u32 *_again, u32 *_dgain)
{
    int i;
    u32 reg0, reg1, reg2;
    u32 dgain, againTndex, again;
    u8 againMap[] = {0x00, 0x08, 0x09, 0x0b, 0x0f, 0x1f};

    x_printf("xzpin calc_gain ae_gain = %d\n", gain);
    if (gain < 1024) {
        gain = 1024;
    }
    if (gain > 128 * 1024) {
        gain = 128 * 1024;
    }

    againTndex = 0;
    while (gain >= 4096) {
        x_printf("xzpin gain = %d, againTndex = %d\n", gain, againTndex);
        gain *= 1024;
        gain /= AGAIN_STEP;
        againTndex++;

        if (againTndex == 0x05) {
            break;
        }
    }
    x_printf("xzpin againTndex = %x\n", againTndex);
    dgain = 0x00;
    if (gain > MAX_DGAIN) {
        gain = MAX_DGAIN;
    }

    while (gain >= 2048) {          //2048
        gain /= 2;
        dgain *= 2;
        dgain++;
    }
    x_printf("xzpin gain = %d, dgain = %d\n", gain, dgain);

    *_again = againMap[againTndex];
    *_dgain = (dgain << 8) + (gain >> 3);
}

static void set_shutter(u32 texp)
{
    if (cur_expline == texp) {
        return;
    }
    cur_expline  = texp;

    wr_sc1336_mipi_reg(0x3e00, (texp >> 12) & 0xff);
    wr_sc1336_mipi_reg(0x3e01, (texp >> 4) & 0xff);
    wr_sc1336_mipi_reg(0x3e02, (texp << 4) & 0xf0);
}

u32 sc1336_mipi_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp = 0;
    u32 texp_align = 0;
    u32 ratio = 0;

    texp = exp_time_us * PCLK * 2 / LINE_LENGTH_CLK;

    if (texp < 2) {
        texp = 2;
    }
    if (texp > FRAME_LENGTH * 2 - 8) {
        texp = FRAME_LENGTH * 2 - 8;
    }
    texp_align = (texp) * LINE_LENGTH_CLK / (PCLK * 2);

    if (texp_align < exp_time_us) {
        ratio = (exp_time_us) * (1 << 10) / texp_align;
        //printf("ratio = %d\n",ratio);
    } else {
        ratio = (1 << 10);
    }

    shutter->ae_exp_line =  texp;
    shutter->ae_gain = (gain * ratio) >> 10;
    shutter->ae_exp_clk = 0;

    x_printf("exp_time_us=%d, texp=%d, gain=%d->%d\n", exp_time_us, texp, gain, shutter->ae_gain);

    return 0;
}

u32 sc1336_mipi_set_shutter(isp_ae_shutter_t *shutter)
{
    u32 again, dgain;

    calc_gain((shutter->ae_gain), &again, &dgain);
    set_shutter(shutter->ae_exp_line);

    if (shutter->ae_gain < 2048) {
        /* printf("xzpin again dgain < 2048 -------->\n"); */
        wr_sc1336_mipi_reg(0x3e08, 0x1f);
        wr_sc1336_mipi_reg(0x3e09, 0x00);
        set_dgain(dgain);
    } else {
        set_again(again);
        set_dgain(dgain);
    }

    return 0;
}

void sc1336_mipi_sleep()
{

}

void sc1336_mipi_wakeup()
{

}

void sc1336_mipi_wreg(u8 isp_dev, u16 addr, u16 val)
{
    wr_sc1336_mipi_reg((u16)addr, (u8)val);
}

u16 sc1336_mipi_rreg(u8 isp_dev, u16 addr)
{
    u8 val;
    rd_sc1336_mipi_reg((u16)addr, &val);
    return val;
}

REGISTER_CAMERA(SC1336_MIPI) = {
    .logo               =   "SC1336M",
    .isp_dev            =   ISP_DEV_NONE,
    .in_format          =    SEN_IN_FORMAT_BGGR,
    .out_format         =   ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_CSI2,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_CSI2_1_LANE,
    .fps                =   SENSOR_FPS,

    .sen_size           =   {SC1336_MIPI_OUTPUT_W, SC1336_MIPI_OUTPUT_H},
    .isp_size           =   {SC1336_MIPI_OUTPUT_W, SC1336_MIPI_OUTPUT_H},

    .cap_fps                =   SENSOR_FPS,
    .sen_cap_size           =   {SC1336_MIPI_OUTPUT_W, SC1336_MIPI_OUTPUT_H},
    .isp_cap_size           =   {SC1336_MIPI_OUTPUT_W, SC1336_MIPI_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check       =   sc1336_mipi_check,
        .init               =   sc1336_mipi_init,
        .set_size_fps       =   sc1336_mipi_set_output_size,
        .power_ctrl         =   sc1336_mipi_power_ctl,

        .get_ae_params      =   sc1336_mipi_get_ae_params,
        .get_awb_params     =   sc1336_mipi_get_awb_params,
        .get_iq_params      =   sc1336_mipi_get_iq_params,

        .sleep              =   sc1336_mipi_sleep,
        .wakeup             =   sc1336_mipi_wakeup,
        .write_reg          =   sc1336_mipi_wreg,
        .read_reg           =   sc1336_mipi_rreg,
    }
};
