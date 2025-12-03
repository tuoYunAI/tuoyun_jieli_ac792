/*这是H63P DVP驱动，不是H63*/
/*H63P ID: 0x08 0x48*/
/*H63  ID: 0x0A 0x63*/

#include "device/iic.h"
#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "h63_dvp.h"
#include "asm/isp_alg.h"

extern void *H63_DVP_get_ae_params();
extern void *H63_DVP_get_awb_params();
extern void *H63_DVP_get_iq_params();
extern void H63_DVP_ae_ev_init(u8 isp_dev, u32 fps);


#define FRAME_W  1920//1600
#define PCLK       43//36

#define FRAME_H  750
#define H63_FPS  30

#define INPUT_CLK       24

#define EXTERNAL_LDO_1V5 1
//如果系统VDDIO使用3.3V请定义此宏定义值为1
//如果系统VDDIO使用1.8V请定义此宏定义值为0
#define	VDDIO_VOLTAGE_3V3 0
#define DSC_ADDR_BASE 0x0400

#define H63_WRCMD 0x80
#define H63_RDCMD 0x81

typedef struct {
    void *iic;
    u8 reset_io;
    u8 power_down_io;
    u32 cur_again;
    u32 cur_dgain;
    u32 cur_expline;
} camera_iic;
static camera_iic camera[2];    //dvp0 dvp1

typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_ini;

static Sensor_reg_ini H63_DVP_INI_REG[] = {
    0x12, 0x40,
    0x48, 0x85,
    0x48, 0x05,
    0x0E, 0x11,
    0x0F, 0x84,
    0x10, 0x24,
    0x11, 0x80,
    0x57, 0x60,
    0x58, 0x18,
    0x61, 0x10,
    0x46, 0x00,
    0x0D, 0xA0, //a0
    0x20, 0xc0,
    0x21, 0x03,
    0x22, 0xEE,
    0x23, 0x02,
    0x24, 0x80,
    0x25, 0xD0,
    0x26, 0x22,
    0x27, 0x36,
    0x28, 0x15,
    0x29, 0x03,
    0x2A, 0x2B,
    0x2B, 0x13,
    0x2C, 0x00,
    0x2D, 0x00,
    0x2E, 0xBA,
    0x2F, 0x60,
    0x41, 0x84,
    0x42, 0x02,
    0x47, 0x42,
    0x76, 0x40,
    0x77, 0x06,
    0x80, 0x01,
    0xAF, 0x22,
    0x8A, 0x00,
    0xA6, 0x00,
    0xAB, 0x00,
    0x1D, 0xFF,
    0x1E, 0x1F,
    0x6C, 0xC0,
    0x9E, 0xF8,
    0x9C, 0xE1,
    0x3A, 0xAC,
    0x3B, 0x18,
    0x3C, 0x5D,
    0x3D, 0x80,
    0x3E, 0x6E,
    0x31, 0x07,
    0x32, 0x14,
    0x33, 0x12,
    0x34, 0x1C,
    0x35, 0x1C,
    0x56, 0x12,
    0x59, 0x20,
    0x85, 0x14,
    0x64, 0xD2,
    0x8D, 0x41,
    0x8F, 0x90,
    0xA4, 0x87,
    0xA7, 0x80,
    0xA9, 0x48,
    0x45, 0x01,
    0x5B, 0xA0,
    0x5C, 0x6C,
    0x5D, 0x44,
    0x5E, 0x81,
    0x63, 0x0F,
    0x65, 0x12,
    0x66, 0x43,
    0x67, 0x79,
    0x68, 0x04,
    0x69, 0x78,
    0x6A, 0x28,
    0x7A, 0x66,
    0xA5, 0x03,
    0x94, 0xC0,
    0x13, 0x81,
    0x96, 0x84,
    0xB7, 0x4A,
    0x4A, 0x01,
    0xB5, 0x0C,
    0xA1, 0x0F,
    0xA3, 0x40,
    0xB1, 0x00,
    0x93, 0x00,
    0x7E, 0x4C,
    0x50, 0x02,
    0x49, 0x10,
    0x8E, 0x40,
    0x7F, 0x56,
    0x0C, 0x00,
    0xBC, 0x11,
    0x82, 0x00,
    0x19, 0x20,
    0x1F, 0x10,
    0x1B, 0x4F,
    0x12, 0x00,
};

unsigned char wrH63_DVPReg(void *iic, unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, H63_WRCMD)) {
        ret = 0;
        //  puts("1 err\n");
        goto __wend;
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID)) {
        ret = 0;
        //   puts("2 err\n");

        goto __wend;
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        //    puts("3 err\n");

        goto __wend;
    }

__wend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}

unsigned char rdH63_DVPReg(void *iic, unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, H63_WRCMD)) {
        ret = 0;
        //puts("4 \n");

        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regID)) {
        ret = 0;
        //puts("5\n");

        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, H63_RDCMD)) {
        ret = 0;
        // puts("6\n");

        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat)) {
        //puts("7\n");

        ret = 0;
        goto __rend;
    }
__rend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;

}
#if 0
unsigned char rdH63_DVPReg_All()
{
    u16 i, val;
    for (i = 0; i < 0x100; i ++) {
        rdH63_DVPReg(iic, i, &val);
        printf("0x%x:0x%x \n", i, val);
    }
    return 0;
}
#endif
/*************************************************************************************************
 sensor api
 *************************************************************************************************/
void H63_DVP_config_SENSOR(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    u16 dsc_addr;
    u8 pid;

    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    for (i = 0; i < sizeof(H63_DVP_INI_REG) / sizeof(Sensor_reg_ini); i++) {
        wrH63_DVPReg(iic, H63_DVP_INI_REG[i].addr, H63_DVP_INI_REG[i].value);
    }

    printf("\n[ debug ]--func=%s line=%d Frame_freq==%d\n", __func__, __LINE__, *frame_freq);

    H63_DVP_ae_ev_init(isp_dev, *frame_freq);

    if (*frame_freq == 25) {
        wrH63_DVPReg(iic, 0x22, 0x84);
        wrH63_DVPReg(iic, 0x23, 0x03);
    }
    if (*frame_freq == 5) {
        wrH63_DVPReg(iic, 0x22, 0x94);
        wrH63_DVPReg(iic, 0x23, 0x11);
    }

    if (*frame_freq == 10) {
        wrH63_DVPReg(iic, 0x22, 0xca);
        wrH63_DVPReg(iic, 0x23, 0x8);
    }
    if (*frame_freq == 15) {
        wrH63_DVPReg(iic, 0x22, 0xdc);
        wrH63_DVPReg(iic, 0x23, 0x5);
    }
    if (*frame_freq == 20) {
        wrH63_DVPReg(iic, 0x22, 0x65);
        wrH63_DVPReg(iic, 0x23, 0x4);
    }
    return;
}

s32 H63_DVP_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    if (5 == *frame_freq) {
        wrH63_DVPReg(iic, 0x21, 0x2B);
        wrH63_DVPReg(iic, 0x20, 0x66);
    } else if (25 == *frame_freq) {
        wrH63_DVPReg(iic, 0x21, 0x03);
        wrH63_DVPReg(iic, 0x20, 0xEE);
    }
    return 0;
}

s32 H63_DVP_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

void H63_DVP_xclk_set(u8 isp_dev)
{

}

s32 H63_DVP_ID_check(void *iic)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i;

    for (i = 0; i < 3; i++) {
        rdH63_DVPReg(iic, 0x0a, &pid);
        rdH63_DVPReg(iic, 0x0b, &ver);

        printf("Sensor H63_DVP PID %x %x\n", pid, ver);
        if (pid == 0x08 && ver == 0x48) {
            break;
        }
    }

    if (pid != 0x08 || ver != 0x48) {
        puts("\n----not H63_DVP-----\n");
        return -1;
    }
    puts("\n----hello H63_DVP-----\n");
    return 0;
}

void H63_DVP_reset(u8 isp_dev)
{
    puts("H63_DVP reset \n");

    u32 reset_gpio = (u32)camera[isp_dev].reset_io;
    u32 pwdn_gpio = (u32)camera[isp_dev].power_down_io;

    printf("gpio=%x %x\n", reset_gpio, pwdn_gpio);
    gpio_direction_output(pwdn_gpio, 0);
    gpio_direction_output(reset_gpio, 0);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    delay(40000);
}

s32 H63_DVP_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
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

    H63_DVP_reset(isp_dev);

    if (0 != H63_DVP_ID_check(camera[isp_dev].iic)) {
        dev_close(camera[isp_dev].iic);
        camera[isp_dev].iic = NULL;
        return -1;
    }

    return 0;
}

s32 H63_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\nH63_DVP_init \n\n");

    camera[isp_dev].cur_again = -1;
    camera[isp_dev].cur_dgain = -1;
    camera[isp_dev].cur_expline = -1;

    H63_DVP_config_SENSOR(isp_dev, width, height, format, frame_freq);

    return 0;
}

static void set_again(u8 isp_dev, u32 again)
{
    if (camera[isp_dev].cur_again == again) {
        return;
    }
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    camera[isp_dev].cur_again = again;
#if 1
    wrH63_DVPReg(iic, 0x00, again);
#else
    wrH63_DVPReg(iic, 0xC4, 0x00);
    wrH63_DVPReg(iic, 0xC5, again);

    wrH63_DVPReg(iic, 0x1F, 0x80);   //group write enable
#endif
    return;
}

static void set_dgain(u8 isp_dev, u32 dgain)
{
    if (camera[isp_dev].cur_dgain == dgain) {
        return;
    }
    camera[isp_dev].cur_dgain = dgain;
}

#define MAX_AGAIN (32*1024 - 512)
static void calc_gain(u32 gain, u32 *_again, u32 *_dgain)
{
    u32 gain0;
    u32 reg0, reg1;

    if (gain < 1024) {
        gain = 1024;
    }
    if (gain > 31 * 1024) {
        gain = 31 * 1024 ;
    }

    gain0 = gain;
    reg0 = 0;
    while (gain0 > MAX_AGAIN) {
        gain0 /= 2;
        reg0 *= 2;
        reg0++;
    }
    *_dgain = reg0;
    reg0 = 0;
    while (gain0 >= 2048) {
        reg0++;
        gain0 /= 2;
    }
    reg1 = ((gain0 - 1024) >> 6);


    *_again = (reg0 << 4) | reg1;
}

static void set_shutter(u8 isp_dev, u32 texp, u32 texp_mck)
{
    if (camera[isp_dev].cur_expline == texp) {
        return;
    }
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    /* printf("texp %d \n",texp ); */
    if (texp > FRAME_H - 1) {
        texp = FRAME_H - 1;
    }
#if 1
    wrH63_DVPReg(iic, 0x01, texp & 0xff);

    wrH63_DVPReg(iic, 0x02, (texp >> 8) & 0xff);
#else//使用预加载功能

    wrH63_DVPReg(iic, 0xC0, 0x01);
    wrH63_DVPReg(iic, 0xC1, texp & 0xff);

    wrH63_DVPReg(iic, 0xC2, 0x02);
    wrH63_DVPReg(iic, 0xC3, (texp >> 8) & 0xff);

    wrH63_DVPReg(iic, 0x1F, 0x80);   //group write enable

#endif
    camera[isp_dev].cur_expline = texp;

}

u32 H63_DVP_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp =  0;
    u32 texp_align = 0;
    u32 ratio = 0;
    texp = exp_time_us * PCLK / FRAME_W;
    if (texp < 4) {
        texp = 4;
    }
    texp_align = texp * FRAME_W / PCLK;
    if (texp_align < exp_time_us) {
        ratio = (exp_time_us) * (1 << 10) / texp_align;

    } else {
        ratio = (1 << 10);
    }

    shutter->ae_exp_line =  texp;
    shutter->ae_gain = (gain * ratio) >> 10;
    shutter->ae_exp_clk = 0;
    return 0;

}

u32 H63_DVP_set_shutter(isp_ae_shutter_t *shutter, u8 isp_dev)
{
    u32 again, dgain;
    calc_gain(shutter->ae_gain, &again, &dgain);

    set_again(isp_dev, again);
    set_shutter(isp_dev, shutter->ae_exp_line, shutter->ae_exp_clk);

    return 0;
}

void H63_DVP_sleep()
{

}

void H63_DVP_wakeup()
{

}

void H63_DVP_W_Reg(u8 isp_dev, u16 addr, u16 val)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    wrH63_DVPReg(iic, (u8) addr, (u8) val);
}

u16 H63_DVP_R_Reg(u8 isp_dev, u16 addr)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);
    u8 val;

    rdH63_DVPReg(iic, (u8) addr, &val);
    return val;
}

REGISTER_CAMERA(H63_DVP) = {
    .logo 				= 	"H63P_DVP",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			=   SEN_IN_FORMAT_BGGR,
    //.in_format 			=   SEN_IN_FORMAT_GRBG,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_PARALLEL, //SEN_MBUS_CSI2,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_HSYNC_ACTIVE_HIGH | SEN_MBUS_PCLK_SAMPLE_RISING,
    //.mbus_config        =   SEN_MBUS_DATA_WIDTH_8B | SEN_MBUS_HSYNC_ACTIVE_HIGH | SEN_MBUS_PCLK_SAMPLE_RISING,
    .fps         		= 	25, // 30,

    .sen_size 			= 	{H63_DVP_OUTPUT_W, H63_DVP_OUTPUT_H},
    .isp_size 			= 	{H63_DVP_OUTPUT_W, H63_DVP_OUTPUT_H},
    //.isp_size 			= 	{160, 128},
    //.isp_size 			= 	{640/2, 480/2},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{H63_DVP_OUTPUT_W, H63_DVP_OUTPUT_H},
    .isp_cap_size 			= 	{H63_DVP_OUTPUT_W, H63_DVP_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	H63_DVP_check,
        .init 		        = 	H63_DVP_init,
        .set_size_fps 		=	H63_DVP_set_output_size,
        .power_ctrl         =   H63_DVP_power_ctl,

        .get_ae_params 	    =	H63_DVP_get_ae_params,
        .get_awb_params 	=	H63_DVP_get_awb_params,
        .get_iq_params 	    =	H63_DVP_get_iq_params,

        .sleep 		        =	H63_DVP_sleep,
        .wakeup 		    =	H63_DVP_wakeup,
        .write_reg 		    =	H63_DVP_W_Reg,
        .read_reg 		    =	H63_DVP_R_Reg,
    }
};

