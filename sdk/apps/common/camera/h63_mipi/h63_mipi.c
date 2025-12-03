#ifndef H63_MIPI_C
#define  H63_MIPI_C

/*这是H63P MIPI驱动，不是H63*/
/*H63P ID: 0x08 0x48*/
/*H63  ID: 0x0A 0x63*/
#include "app_config.h"

#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "asm/isp_alg.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"
#include "h63_mipi.h"

#define FRAME_W  1920
#define FRAME_H  750
#define H63_FPS  30

#define PCLK       43
#define INPUT_CLK       24

static u8  H63_WRCMD =      0x80;
static u8  H63_RDCMD =      0x81;

static u32 cur_expline = -1;
static u32 cur_again = -1;
static u32 cur_dgain = -1;

static u32 reset_gpios[2] = {-1, -1};
static u32 pwdn_gpios[2] = {-1, -1};

static u8 cur_sensor_type = 0xff;

extern void *H63_MIPI_get_ae_params();
extern void *H63_MIPI_get_awb_params();
extern void *H63_MIPI_get_iq_params();
extern void H63_MIPI_ae_ev_init(u32 fps);

typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_ini;

struct stVrefh {
    u16 index;
    u16 th_low;
    u16 th_high;
    u8 vrefh;
};

//如果系统VDDIO使用3.3V请定义此宏定义值为1
//如果系统VDDIO使用1.8V请定义此宏定义值为0

static Sensor_reg_ini H63_MIPI_INI_REG[] = {
    0x12, 0x40,
    0x48, 0x85,
    0x48, 0x05,
    0x0E, 0x11,
    0x0F, 0x84,
    0x10, 0x24, //0x24
    0x11, 0x80,
    0x57, 0x60,
    0x58, 0x18,
    0x61, 0x10,
    0x46, 0x08,
    0x0D, 0xA0,
    0x20, 0xC0,
    0x21, 0x03,
    0x22, 0xEE,
    0x23, 0x02,
    0x24, 0x80,
    0x25, 0xD0,
    0x26, 0x22,
    0x27, 0xD6,
    0x28, 0x15,
    0x29, 0x02,
    0x2A, 0xCB,
    0x2B, 0x12,
    0x2C, 0x00,
    0x2D, 0x00,
    0x2E, 0xBA,
    0x2F, 0x60,
    0x41, 0x84,
    0x42, 0x02,
    0x47, 0x46,
    0x76, 0x40,
    0x77, 0x06,
    0x80, 0x01,
    0xAF, 0x22,
    0x8A, 0x00,
    0xA6, 0x00,
    0xAB, 0x00,
    0x8D, 0x49,
    0x1D, 0x00,
    0x1E, 0x04,
    0x6C, 0x50, //bit2 &bit3  MIPI驱动能力
    0x9E, 0xF8,
    0x6E, 0x2C,
    0x70, 0x8C,
    0x71, 0x6D,
    0x72, 0x6A,
    0x73, 0x46,
    0x74, 0x00, //0x02
    0x78, 0x8E,
    0x89, 0x01,
    0x6B, 0x20, //20 bit2 &bit3
    0x86, 0x40,
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
    0x66, 0x43, //43 bit4 &bit5 MIPI驱动能力
    0x67, 0x79,
    0x68, 0x00,
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

static void *iic = NULL;

unsigned char wrH63_MIPIReg(unsigned char regID, unsigned char regDat)
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

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regDat)) {
        ret = 0;
        //    puts("3 err\n");

        goto __wend;
    }

__wend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}

unsigned char rdH63_MIPIReg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, H63_WRCMD)) {
        ret = 0;
        puts("4 \n");

        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID)) {
        ret = 0;
        // puts("5\n");

        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, H63_RDCMD)) {
        ret = 0;
        // puts("6\n");

        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat)) {
        // puts("7\n");

        ret = 0;
        goto __rend;
    }
__rend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;

}


/*************************************************************************************************
 sensor api
 *************************************************************************************************/
void H63_MIPI_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    u16 dsc_addr;
    u8 pid;


    for (i = 0; i < sizeof(H63_MIPI_INI_REG) / sizeof(Sensor_reg_ini); i++) {
        wrH63_MIPIReg(H63_MIPI_INI_REG[i].addr, H63_MIPI_INI_REG[i].value);
        rdH63_MIPIReg(H63_MIPI_INI_REG[i].addr, &pid);
        /* printf("%02x: %02x->%02x\n", H63_MIPI_INI_REG[i].addr, H63_MIPI_INI_REG[i].value, pid); */
    }
//    rdH63_MIPIReg(0x6D, &pid);
//    pid |= 0x30;
//    wrH63_MIPIReg(0x6D, pid);

    H63_MIPI_ae_ev_init(*frame_freq);

    if (*frame_freq == 25) {
        wrH63_MIPIReg(0x22, 0x84);
        wrH63_MIPIReg(0x23, 0x03);
    }
    if (*frame_freq == 5) {
        wrH63_MIPIReg(0x22, 0x94);
        wrH63_MIPIReg(0x23, 0x11);
    }

    if (*frame_freq == 10) {
        wrH63_MIPIReg(0x22, 0xca);
        wrH63_MIPIReg(0x23, 0x8);
    }
    if (*frame_freq == 15) {
        wrH63_MIPIReg(0x22, 0xdc);
        wrH63_MIPIReg(0x23, 0x5);
    }
    if (*frame_freq == 20) {
        wrH63_MIPIReg(0x22, 0x65);
        wrH63_MIPIReg(0x23, 0x4);
    }
    return;
}

s32 H63_MIPI_set_output_size(u16 *width, u16 *height, u8 *frame_freq)
{
    return 0;
}

s32 H63_MIPI_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

void H63_MIPI_xclk_set(u8 isp_dev)
{

}

s32 H63_MIPI_ID_check(void)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i;
    for (i = 0; i < 4; i++) {
        rdH63_MIPIReg(0x0a, &pid);
        rdH63_MIPIReg(0x0b, &ver);

        printf("Sensor H63_MIPI PID %x %x %x %x\n", pid, ver, H63_WRCMD, H63_RDCMD);
        if (pid == 0x08 && ver == 0x48) {
            break;
        }
    }

    if (pid != 0x08 || ver != 0x48) {
        puts("\n----not H63_MIPI-----\n");
        return -1;
    }
    puts("\n----hello H63_MIPI-----\n");
    printf("csi_tval_con = %04x\n", csi_tval_con);
    return 0;
}

static void H63_MIPI_reset(u8 isp_dev)
{
    puts("H63_MIPI reset \n");

    u32 reset_gpio;
    u32 pwdn_gpio;

    if (isp_dev == ISP_DEV_0) {
        reset_gpio = reset_gpios[0];
        pwdn_gpio = pwdn_gpios[0];
    } else {
        reset_gpio = reset_gpios[1];
        pwdn_gpio = pwdn_gpios[1];
    }

    printf("gpio reset = %x, pwdn = %x\n", reset_gpio, pwdn_gpio);
    gpio_direction_output(pwdn_gpio, 0);
    gpio_direction_output(reset_gpio, 0);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    delay(40000);

}

s32 H63_MIPI_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
{
    printf("\n\nH63_MIPI_check reset pin :%x\n\n", reset_gpio);
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
    printf("\n\n isp_dev = %d\n\n", isp_dev);

    reset_gpios[isp_dev] = reset_gpio;
    pwdn_gpios[isp_dev] = pwdn_gpio;

    H63_MIPI_reset(isp_dev);

    if (0 != H63_MIPI_ID_check()) {
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;
}

s32 H63_MIPI_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\nH63_MIPI_init \n\n");

    cur_expline = -1;
    cur_again = -1;
    cur_dgain = -1;

    H63_MIPI_config_SENSOR(width, height, format, frame_freq);
    return 0;
}

static void set_again(u32 again)
{
    if (cur_again == again) {
        return;
    }
    cur_again = again;
#if 01
    wrH63_MIPIReg(0x00, again);
#else
    wrH63_MIPIReg(0xC4, 0x00);
    wrH63_MIPIReg(0xC5, again);

    wrH63_MIPIReg(0x1F, 0x80);   //group write enable
#endif
    return;
}

static void set_dgain(u32 dgain)
{
    if (cur_dgain == dgain) {
        return;
    }
    cur_dgain = dgain;
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

static void set_shutter(u32 texp, u32 texp_mck)
{
    if (cur_expline == texp) {
        return ;
    }
    /* printf("texp %d \n",texp ); */
    if (texp > FRAME_H - 1) {
        texp = FRAME_H - 1;
    }
#if 1
    wrH63_MIPIReg(0x01, texp & 0xff);

    wrH63_MIPIReg(0x02, (texp >> 8) & 0xff);
#else//使用预加载功能

    wrH63_MIPIReg(0xC0, 0x01);
    wrH63_MIPIReg(0xC1, texp & 0xff);

    wrH63_MIPIReg(0xC2, 0x02);
    wrH63_MIPIReg(0xC3, (texp >> 8) & 0xff);

    wrH63_MIPIReg(0x1F, 0x80);   //group write enable

#endif
    cur_expline = texp;

}

u32 H63_MIPI_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp =  0;
    u32 texp_align = 0;
    u32 ratio = 0;
    texp = exp_time_us * PCLK / FRAME_W;
    texp_align = texp * FRAME_W / PCLK;
    if (texp < 4) {
        texp = 4;
    }
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

u32 H63_MIPI_set_shutter(isp_ae_shutter_t *shutter)
{
    u32 again, dgain;
    calc_gain(shutter->ae_gain, &again, &dgain);

    set_again(again);
    set_shutter(shutter->ae_exp_line, shutter->ae_exp_clk);

    return 0;
}

void H63_MIPI_sleep()
{

}

void H63_MIPI_wakeup()
{

}

void H63_MIPI_W_Reg(u16 addr, u16 val)
{
    wrH63_MIPIReg((u8) addr, (u8) val);
}

u16 H63_MIPI_R_Reg(u16 addr)
{
    u8 val;
    rdH63_MIPIReg((u8) addr, &val);
    return val;
}

REGISTER_CAMERA(H63_MIPI) = {
    .logo 				= 	"H63P_MIPI",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			=  	 SEN_IN_FORMAT_BGGR,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_CSI2,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_CSI2_1_LANE,
    .fps         		= 	25,

    .sen_size 			= 	{H63_MIPI_OUTPUT_W, H63_MIPI_OUTPUT_H},
    .isp_size 			= 	{H63_MIPI_OUTPUT_W, H63_MIPI_OUTPUT_H},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{H63_MIPI_OUTPUT_W, H63_MIPI_OUTPUT_H},
    .isp_cap_size 			= 	{H63_MIPI_OUTPUT_W, H63_MIPI_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	H63_MIPI_check,
        .init 		        = 	H63_MIPI_init,
        .set_size_fps 		=	H63_MIPI_set_output_size,
        .power_ctrl         =   H63_MIPI_power_ctl,

        .get_ae_params 	    =	H63_MIPI_get_ae_params,
        .get_awb_params 	=	H63_MIPI_get_awb_params,
        .get_iq_params 	    =	H63_MIPI_get_iq_params,

        .sleep 		        =	H63_MIPI_sleep,
        .wakeup 		    =	H63_MIPI_wakeup,
        .write_reg 		    =	H63_MIPI_W_Reg,
        .read_reg 		    =	H63_MIPI_R_Reg,
    }
};

#endif
