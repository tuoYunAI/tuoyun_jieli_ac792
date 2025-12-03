#include "device/iic.h"
#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "GC1084_DVP.h"
#include "asm/isp_alg.h"

typedef struct {
    void *iic;
    u8 reset_io;
    u8 power_down_io;
    u32 cur_again;
    u32 cur_dgain;
    u32 cur_expline;
} camera_iic;

static camera_iic camera[2];


extern void *GC1084_DVP_get_ae_params();
extern void *GC1084_DVP_get_awb_params();
extern void *GC1084_DVP_get_iq_params();
extern void GC1084_DVP_ae_ev_init(u8 isp_dev, u32 fps);

#define INPUT_CLK  24
#define PCLK  	32

#define LINE_LENGTH_CLK     1600
#define FRAME_LENGTH30      750
#define FRAME_LENGTH25      900
#define FRAME_LENGTH27      833

#define WRCMD 0x6e
#define RDCMD 0x6f

static u32 line_length_clk = LINE_LENGTH_CLK;
static u32 uframelen = FRAME_LENGTH30;


typedef struct {
    u16 addr;
    u8 value;
} Sensor_reg_ini;

Sensor_reg_ini GC1084_DVP_INI_REG[] = {
    /******************
    Mclk=27MHz
    Pclk=32MHz
    //window_size=1280*720
    //Rowtime = 44.4444us
    //VTS=750,HTS=1442
    ***************/
    /******cisctl rst******/
    0x03fe, 0xf0,
    0x03fe, 0xf0,
    0x03fe, 0xf0,
    0x03fe, 0x00,
    /******PLL*,*****/
    0x03f2, 0x00,
    0x03f3, 0x0f,
    0x03f4, 0x36,
    0x03f5, 0xc0,
    0x03f6, 0x13,
    0x03f7, 0x01,
    0x03f8, 0x10,
    0x03f9, 0x43,
    0x03fa, 0x00,
    0x03fc, 0x8e,
    /******fram,e structure******/
    0x0d05, 0x05,
    0x0d06, 0x8e,
    0x0d07, 0x00,
    0x0d08, 0x10,
    0x0d0a, 0x03,
    0x000c, 0x03,
    0x0d0d, 0x02,
    0x0d0e, 0xd0,
    0x000f, 0x05,
    0x0010, 0x08,
    0x0017, 0x0c,
    0x0d73, 0x92,
    0x0d76, 0x00,
    0x0d41, 0x02,
    0x0d42, 0xee,
    0x0d7a, 0x0c,
    /****** out, ******/
    0x0153, 0x50,
    0x0190, 0x01,
    0x0192, 0x00,
    0x0194, 0x00,
    0x0195, 0x02,
    0x0196, 0xd0,
    0x0197, 0x05,
    0x0198, 0x00,
    /****** Ana,log ******/
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
    0x0058, 0x70,
    0x00d6, 0x66,
    0x00d7, 0x1b,
    0x0093, 0x00,
    0x00d9, 0x04,
    0x00da, 0xc1,
    0x0d2a, 0x00,
    0x0d28, 0x04,
    0x0d53, 0x12,
    0x0050, 0x30,
    0x008c, 0x05,
    0x008d, 0xa8,
    /****** dar,k sun ******/
    0x0077, 0x01,
    0x0078, 0x04,
    0x0079, 0x65,
    0x0067, 0xc0,
    0x0054, 0xff,
    0x0055, 0x02,
    0x0056, 0x00,
    0x0057, 0x04,
    0x005a, 0xff,
    0x005b, 0x07,
    /****** sra,m ******/
    0x00d5, 0x03,
    0x0102, 0xa9,
    /****** exp, & gain ******/
    0x0d03, 0x02,
    0x0d04, 0xd0,
    0x007a, 0x60,
    0x04e0, 0x01,
    /****** blk, *******/
    0x0414, 0x7e,
    0x0415, 0x7e,
    0x0416, 0x7e,
    0x0417, 0x7e,
    /****** mip,i ******/
    0x00dc, 0x05,
    0x00dd, 0xea,
    0x0201, 0x20,
    0x0202, 0x07,
    0x0212, 0x05,
    0x0213, 0x00,
    0x023a, 0x01,
    0x0290, 0x0b,
    0x023e, 0x40,
};


unsigned char wrGC1084_DVP_Reg(void *iic, u16 regID, unsigned char regDat)
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
        printf("wreg iic fail\n");
    }
    return ret;

}

unsigned char rdGC1084_DVP_Reg(void *iic, u16 regID, unsigned char *regDat)
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
        printf("rreg iic fail\n");
    }
    return ret;
}


/*************************************************************************************************
    sensor api
*************************************************************************************************/

void GC1084_DVP_config_SENSOR(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    void *iic = camera[isp_dev].iic;

    GC1084_DVP_set_output_size(isp_dev, width, height, frame_freq);

    for (int i = 0; i < sizeof(GC1084_DVP_INI_REG) / sizeof(Sensor_reg_ini); i++) {
        wrGC1084_DVP_Reg(iic, GC1084_DVP_INI_REG[i].addr, GC1084_DVP_INI_REG[i].value);
    }

    GC1084_DVP_ae_ev_init(isp_dev, *frame_freq);

    if (*frame_freq == 25) {
        wrGC1084_DVP_Reg(iic, 0x0d41, 0x03);
        wrGC1084_DVP_Reg(iic, 0x0d42, 0x84);
        uframelen = FRAME_LENGTH25;

    } else if (*frame_freq == 27) {
        wrGC1084_DVP_Reg(iic, 0x0d41, 0x03);
        wrGC1084_DVP_Reg(iic, 0x0d42, 0x41);
        uframelen = FRAME_LENGTH27;
    } else {
        wrGC1084_DVP_Reg(iic, 0x0d41, 0x02);
        wrGC1084_DVP_Reg(iic, 0x0d42, 0xee);
        uframelen = FRAME_LENGTH30;
    }

    *format = SEN_IN_FORMAT_BGGR;
    return;
}


s32 GC1084_DVP_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq)
{
    *width = GC1084_DVP_OUTPUT_W;
    *height = GC1084_DVP_OUTPUT_H;

    return 0;
}


s32 GC1084_DVP_power_ctl(u8 isp_dev, u8 is_work)
{

    return 0;
}

s32 GC1084_DVP_ID_check(void *iic)
{
    u8 pid = 0x00;
    u8 ver = 0x00;

    u8 retry = 0;
    while (retry <= 3) {
        rdGC1084_DVP_Reg(iic, 0x03f0, &pid);
        rdGC1084_DVP_Reg(iic, 0x03f1, &ver);

        if (pid == 0x10 && ver == 0x84) {

            puts("\n----hello GC1084_DVP-----\n");
            return 0;
        }
        retry++;
        delay(500);
    }
    puts("\n----not GC1084_DVP-----\n");
    return -1;
}

void GC1084_DVP_reset(u8 isp_dev)
{
    u32 reset_gpio = camera[isp_dev].reset_io;
    u32 pwdn_gpio = camera[isp_dev].power_down_io;

    gpio_direction_output(reset_gpio, 1);
    gpio_direction_output(pwdn_gpio, 1);
    delay(40000);
    delay(40000);
    delay(40000);
    gpio_direction_output(reset_gpio, 0);
    gpio_direction_output(pwdn_gpio, 0);
    delay(40000);
    delay(40000);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    gpio_direction_output(pwdn_gpio, 1);
    delay(40000);
    delay(40000);
    delay(40000);
}

void GC1084_DVP_change_fps(u8 sence, u16 lv)
{
    static s8 curSence = -1;

    //printf("%s: %d %d\n", __FUNCTION__, sence, lv);
}

static s32 GC1084_DVP_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
{
    if (isp_dev > 1) {
        return -1;
    }
    printf("\n\n %s, isp_dev = %d\n\n", __func__, isp_dev);

    if (!camera[isp_dev].iic) {
        camera[isp_dev].iic = dev_open(iic_name, 0);
        camera[isp_dev].reset_io = (u8)reset_gpio;
        camera[isp_dev].power_down_io = (u8)pwdn_gpio;
    }

    if (!camera[isp_dev].iic) {
        printf("%s iic open err!!!\n\n", __func__);
        return -1;
    }

    // ===============================
    //DVDD 1.2
    /* gpio_direction_output(IO_PORTG_13, 0); */
    //2.8 AVDD
    //avdd28_ctrl(AVDD28_28, true);

    GC1084_DVP_reset(isp_dev);
    if (0 != GC1084_DVP_ID_check(camera[isp_dev].iic)) {
        dev_close(camera[isp_dev].iic);
        camera[isp_dev].iic = NULL;
        return -1;
    }
    // ================================

    return 0;
}

void resetStatic();
static s32 GC1084_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC1084_DVP_init22 \n\n");

    camera[isp_dev].cur_again = -1;
    camera[isp_dev].cur_dgain = -1;
    camera[isp_dev].cur_expline = -1;

    GC1084_DVP_config_SENSOR(isp_dev, width, height, format, frame_freq);

    return 0;
}

u8 regValTable2083[29][6] = {
    {0x00, 0x00, 0x00, 0x80, 0x01, 0x00},
    {0x0a, 0x00, 0x00, 0x80, 0x01, 0x0b},
    {0x00, 0x01, 0x00, 0x80, 0x01, 0x19},
    {0x0a, 0x01, 0x00, 0x80, 0x01, 0x2a},
    {0x00, 0x02, 0x00, 0x80, 0x02, 0x00},
    {0x0a, 0x02, 0x00, 0x80, 0x02, 0x17},
    {0x00, 0x03, 0x00, 0x80, 0x02, 0x33},
    {0x0a, 0x03, 0x00, 0x80, 0x03, 0x14},
    {0x00, 0x04, 0x00, 0x90, 0x04, 0x00},
    {0x0a, 0x04, 0x00, 0x90, 0x04, 0x2f},
    {0x00, 0x05, 0x00, 0x90, 0x05, 0x26},
    {0x0a, 0x05, 0x00, 0x90, 0x06, 0x28},
    {0x00, 0x06, 0x00, 0xa0, 0x08, 0x00},
    {0x0a, 0x06, 0x00, 0xa0, 0x09, 0x1e},
    {0x12, 0x46, 0x00, 0xa0, 0x0b, 0x0c},
    {0x19, 0x66, 0x00, 0xa0, 0x0d, 0x10},
    {0x00, 0x04, 0x01, 0xa0, 0x10, 0x00},

    {0x0a, 0x04, 0x01, 0xa0, 0x12, 0x3d},
    {0x00, 0x05, 0x01, 0xb0, 0x16, 0x19},
    {0x0a, 0x05, 0x01, 0xc0, 0x1a, 0x23},
    {0x00, 0x06, 0x01, 0xc0, 0x20, 0x00},

    {0x0a, 0x06, 0x01, 0xc0, 0x25, 0x3b},
    {0x12, 0x46, 0x01, 0xc0, 0x2c, 0x30},
    {0x19, 0x66, 0x01, 0xd0, 0x35, 0x01},
    {0x20, 0x06, 0x01, 0xe0, 0x3f, 0x3f},

    {0x25, 0x06, 0x01, 0xe0, 0x3f, 0x3f},
    {0x29, 0x26, 0x01, 0xe0, 0x3f, 0x3f},
    {0x2c, 0xc6, 0x01, 0xf0, 0x3f, 0x3f},
    {0x30, 0x06, 0x01, 0xf0, 0x3f, 0x3f},
};
u32 gainLevelTable[30] = {
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
    716,
    848,
    1024,
    1214,
    1434,
    1699,
    2048,

    2427,
    2865,
    3393,
    4096,

    4855,
    5730,
    6809,
    8192,
    0xffffffff,
};

static void set_again(u8 isp_dev, u32 again)
{
    if (camera[isp_dev].cur_again == again) {
        return;
    }
    void *iic = camera[isp_dev].iic;

    camera[isp_dev].cur_again = again;
    {
        int i;
        u32 tol_dig_gain = 0;
        u32 gain = again / 16;

        int total = sizeof(gainLevelTable) / sizeof(u32);
        for (i = 0; i < total; i++) {
            if ((gainLevelTable[i] <= gain) && (gain < gainLevelTable[i + 1])) {
                break;
            }
        }

        tol_dig_gain = gain * 64 / gainLevelTable[i];

        wrGC1084_DVP_Reg(iic, 0x00d1, regValTable2083[i][0]);
        wrGC1084_DVP_Reg(iic, 0x00d0, regValTable2083[i][1]);

        wrGC1084_DVP_Reg(iic, 0x031d, 0x2e),
                         wrGC1084_DVP_Reg(iic, 0x0dc1, regValTable2083[i][2]),
                         wrGC1084_DVP_Reg(iic, 0x031d, 0x28),
                         wrGC1084_DVP_Reg(iic, 0x0155, regValTable2083[i][3]);
        wrGC1084_DVP_Reg(iic, 0x00b8, regValTable2083[i][4]);
        wrGC1084_DVP_Reg(iic, 0x00b9, regValTable2083[i][5]);

        wrGC1084_DVP_Reg(iic, 0x00b1, (tol_dig_gain >> 6));
        wrGC1084_DVP_Reg(iic, 0x00b2, ((tol_dig_gain & 0x3f) << 2));

    }
    return;
}

static void set_dgain(u8 isp_dev, u32 dgain)
{
    if (camera[isp_dev].cur_dgain == dgain) {
        return;
    }
    camera[isp_dev].cur_dgain = dgain;
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


static void set_shutter(u8 isp_dev, u32 texp)
{
    if (camera[isp_dev].cur_expline == texp) {
        return;
    }
    void *iic = camera[isp_dev].iic;

    camera[isp_dev].cur_expline  = texp;
    wrGC1084_DVP_Reg(iic, 0x0d03, (texp >> 8) & 0xff);
    wrGC1084_DVP_Reg(iic, 0x0d04, (texp >> 0) & 0xff);
}


u32 GC1084_DVP_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp;
    u32 texp_align;
    u32 ratio;
    u32 fLen;
    static u8 bLow = 0;

    fLen = uframelen;
    if (exp_time_us > 40000) {
        fLen = uframelen * 12;
        printf("ex= %d\n", exp_time_us);
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
        //printf("ratio = %d\n",ratio);
    } else {
        ratio = (1 << 10);
    }

    shutter->ae_exp_line =  texp;
    shutter->ae_gain = (gain * ratio) >> 10;
    shutter->ae_exp_clk = 0;

    //printf("exp_time_us=%d, texp=%d, gain=%d->%d  gain_val = %d\n", exp_time_us, texp, gain,shutter->ae_gain,gain>>10);
    return 0;

}
static void set_log()
{

}

u32 GC1084_DVP_set_shutter(isp_ae_shutter_t *shutter, u8 isp_dev)
{
    u32 again, dgain;

    calc_gain((shutter->ae_gain), &again, &dgain);
    set_shutter(isp_dev, shutter->ae_exp_line);
    set_again(isp_dev, again);
    set_dgain(isp_dev, dgain);

    return 0;
}


void GC1084_DVP_sleep()
{


}

void GC1084_DVP_wakeup()
{


}

void GC1084_DVP_W_Reg(u8 isp_dev, u16 addr, u16 val)
{
    void *iic = camera[isp_dev].iic;
    /*printf("update reg%x with %x\n", addr, val);*/
    wrGC1084_DVP_Reg(iic, (u16)addr, (u8)val);
}
u16 GC1084_DVP_R_Reg(u8 isp_dev, u16 addr)
{
    void *iic = camera[isp_dev].iic;
    u8 val;

    rdGC1084_DVP_Reg(iic, (u16)addr, &val);
    //printf("read reg%x val %x\n", addr, val);
    return val;
}

REGISTER_CAMERA(GC1084_DVP) = {
    .logo 				= 	"GC1084D",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_GBRG,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_PCLK_SAMPLE_FALLING,
    .fps         		= 	25,

    .sen_size 			= 	{GC1084_DVP_OUTPUT_W, GC1084_DVP_OUTPUT_H},
    .isp_size 			= 	{GC1084_DVP_OUTPUT_W, GC1084_DVP_OUTPUT_H},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{GC1084_DVP_OUTPUT_W, GC1084_DVP_OUTPUT_H},
    .isp_cap_size 			= 	{GC1084_DVP_OUTPUT_W, GC1084_DVP_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC1084_DVP_check,
        .init 		        = 	GC1084_DVP_init,
        .set_size_fps 		=	GC1084_DVP_set_output_size,
        .power_ctrl         =   GC1084_DVP_power_ctl,

        .get_ae_params  	=	GC1084_DVP_get_ae_params,
        .get_awb_params 	=	GC1084_DVP_get_awb_params,
        .get_iq_params 	    	=	GC1084_DVP_get_iq_params,

        .sleep 		        =	GC1084_DVP_sleep,
        .wakeup 		    =	GC1084_DVP_wakeup,
        .write_reg 		    =	GC1084_DVP_W_Reg,
        .read_reg 		    =	GC1084_DVP_R_Reg,

    }
};
