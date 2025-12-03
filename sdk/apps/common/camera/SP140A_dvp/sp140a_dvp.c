#include "app_config.h"
#include "device/iic.h"
#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "sp140a_dvp.h"

#include "asm/isp_alg.h"


static u32 reset_gpios[2] = {-1, -1};
static u32 pwdn_gpios[2] = {-1, -1};
static u32 cur_expline = -1;
static u32 cur_again = -1;
static u32 cur_dgain = -1;
extern void *sp140a_dvp_get_ae_params();
extern void *sp140a_dvp_get_awb_params();
extern void *sp140a_dvp_get_iq_params();
extern void delay_2ms(int cnt);


extern void sp140a_dvp_ae_ev_init(u32 fps);

#define ROTALL_180 0

static u32 sensor_id;

#define WRCMD 0x78
#define RDCMD 0x79

typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_ini;

Sensor_reg_ini sp140a_dvp_INI_REG[] = {
    0xfd, 0x00,
    0x2f, 0x20,
    0x2e, 0x01,
    0xfd, 0x01,
    0x06, 0x0b,
    0x09, 0x01,
    0x0a, 0xfd,
    0x03, 0x00,
    0x04, 0xe1,
    0x24, 0x20,
    0x30, 0x01,
    0x31, 0x01,
    0x35, 0x40,
    0x36, 0x80,
    0x37, 0x80,
    0x38, 0x80,
    0x39, 0x80,
    0x01, 0x01,
    0xfb, 0x62,
    0xf9, 0x1c,
    0xfd, 0x01,
    0x11, 0x80,
    0x13, 0x40,
    0x33, 0x30,
    0x16, 0x18,
    0x1c, 0xed,
    0x21, 0x03,
    0x22, 0x47,
    0x28, 0x04,
    0x29, 0x0e,
    0x2a, 0x1d,
    0x2c, 0xe0,
    0xfd, 0x01,
    0x55, 0x10,
    0x58, 0x20,
    0x5b, 0x02,
    0x5d, 0x00,
    0x5e, 0x15,
    0x66, 0x50,
    0x68, 0x52,
    0x69, 0x0a,
    0x71, 0x60,
    0x72, 0x10,
    0x74, 0x60,
    0x75, 0x10,
    0x78, 0x3c,
    0x84, 0x70,
    0x8a, 0x33,
    0x8b, 0x33,
    0x47, 0x04,
    0x48, 0x05,
    0x49, 0x00,
    0x4b, 0x04,
    0x4c, 0x02,
    0x4d, 0xd0,
    0x8f, 0x00,
    0x91, 0xd0,
    0xf7, 0x10,
    0xfd, 0x02,
    0x37, 0x40,
    0x3a, 0xff,
    0xfd, 0x01,
    0xe7, 0x03,
    0xe7, 0x00,
    0xfd, 0x00,
    0x1b, 0x00,
};

static void *iic = NULL;

unsigned char rdsp140a_dvpReg(u16 regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(100);

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
    return ret;
}
unsigned char wrsp140a_dvpReg(u16 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
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
    return ret;

}


/*************************************************************************************************
    sensor api
*************************************************************************************************/


void sp140a_dvp_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;


    if (sensor_id == 0x140a) {
        for (i = 0; i < sizeof(sp140a_dvp_INI_REG) / sizeof(Sensor_reg_ini); i++) {
            wrsp140a_dvpReg(sp140a_dvp_INI_REG[i].addr, sp140a_dvp_INI_REG[i].value);
        }
    }


    return;
}


s32 sp140a_dvp_set_output_size(u16 *width, u16 *height, u8 *frame_freq)
{
    return 0;
}


s32 sp140a_dvp_power_ctl(u8 isp_dev, u8 is_work)
{

    return 0;
}

s32 sp140a_dvp_ID_check(void)
{
    u8 sensor_num;
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i ;
    u8 retry = 0;
    while (retry <= 10) {
        rdsp140a_dvpReg(0x02, &pid);
        rdsp140a_dvpReg(0x03, &ver);

        printf("%s %d 0x%x 0x%x\n", __FUNCTION__, retry, pid, ver);
        sensor_id = (pid << 8) | ver;
        //printf("chenzhenhui ---- sensor_id = %x\n",sensor_id);
        if (sensor_id == 0x140a) {
            puts("\n hello GC140A_dvp-----\n");
            return 0;
        }

        retry++;
        delay(500);
    }
    puts("\n----not SP140A_dvp-----\n");



    return -1;
}

void sp140a_dvp_reset(u8 isp_dev)
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
    printf("pwdn_gpio=%d\n", pwdn_gpio);

    gpio_direction_output(reset_gpio, 0);
    gpio_direction_output(pwdn_gpio, 0);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    delay(20000);
    gpio_direction_output(pwdn_gpio, 1);
    delay(200000);
}

void sp140a_dvp_change_fps(u8 sence, u16 lv)
{
    printf("%s: %d %d\n", __FUNCTION__, sence, lv);
}

static u8 cur_sensor_type = 0xff;

s32 sp140a_dvp_check(u8 isp_dev, u32 _reset_gpio, u32 pwdn_gpio, char *iic_name)
{

    puts("\n\n sp140a_dvp_check \n\n");
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
    printf("\n\n isp_dev =%d\n\n", isp_dev);

    reset_gpios[isp_dev] = _reset_gpio;
    pwdn_gpios[isp_dev] = pwdn_gpio;

    sp140a_dvp_reset(isp_dev);

    puts("sp140a_dvp_id_check\n");
    if (0 != sp140a_dvp_ID_check()) {
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;
}

s32 sp140a_dvp_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n sp140a_dvp_init22 \n\n");

    sp140a_dvp_config_SENSOR(width, height, format, frame_freq);


    return 0;
}

s32 sp140a_dvp_get_mipi_clk(u32 *mipi_clk,  u32 *tval_hstt, u32 *tval_stto, u16 width, u16 height, u8 frame_freq)
{
    *mipi_clk = 348;	//348
    *tval_hstt = 0;
    *tval_stto = 30;
    return 0;
}

//pclk 48M
u32 sp140a_dvp_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 text_align, ratio;
    u32 line = exp_time_us * 36 / 1920;	// clk/H ta

    //gain = (gain - 1024) * 70 / 100 + 1024;
    if (line < 8) {
        line = 8;
    }

    text_align = line * 1920 / 36;
    ratio = exp_time_us * 1024 / text_align;
    if (ratio < 1024) {
        ratio = 1024;
    }
    shutter->ae_exp_line =  line;
    shutter->ae_gain = (gain >> 5) * (ratio >> 5);
    shutter->ae_exp_clk = 0;
    return 0;

}

void sp140a_dvp_W_Reg(u16 addr, u16 val);

static void set_again(u32 again)
{
    if (cur_again == again) {
        return;
    }
    cur_again  = again;

    wrsp140a_dvpReg(0xfd, 0x01);
    wrsp140a_dvpReg(0x24, again);
    wrsp140a_dvpReg(0x01, 0x01);
    return;
}

static void set_dgain(u32 dgain)
{
    if (cur_dgain == dgain) {
        return;
    }
    cur_dgain  = dgain;
    wrsp140a_dvpReg(0xfd, 0x01);
    wrsp140a_dvpReg(0x35, dgain);
//	wrsp140a_dvpReg(0x36,dgain);
//	wrsp140a_dvpReg(0x37,dgain);
//	wrsp140a_dvpReg(0x38,dgain);
//	wrsp140a_dvpReg(0x39,dgain);
    wrsp140a_dvpReg(0x01, 0x01);
}

#define MAX_AGAIN (31*512)

static void calc_gain(u32 gain, u32 *again, u32 *dgain)
{

    u32 curagain, curdgain;
    u32 max_gain = 31 * 1024;

    if (gain >= max_gain) {
        gain = max_gain - 32;
    } else if (gain < 1024) {
        gain = 1024;
    }

    if (gain > MAX_AGAIN) {
        curagain = MAX_AGAIN;
        curdgain = gain * 1024 / MAX_AGAIN;
    } else {
        curagain = gain;
        curdgain = 1024;
    }
    *again = curagain / 64;

    *dgain = curdgain >> 4;


}
static void set_140a_shutter(u32 exp)
{
    if (cur_expline == exp) {
        return;
    }
    cur_expline  = exp;
    wrsp140a_dvpReg(0xfd, 0x01);
    wrsp140a_dvpReg(0x03, (exp >> 8) & 0xff);
    wrsp140a_dvpReg(0x04, (exp >> 0) & 0xff);
    wrsp140a_dvpReg(0x01, 0x01);
}


u32 sp140a_dvp_set_shutter(isp_ae_shutter_t *shutter)
{
    u32 gain;
    u32 again, dgain;
    int i;

    gain = (shutter->ae_gain);

    //decompose_gain(gain, &again, &dgain);

    calc_gain((shutter->ae_gain), &again, &dgain);
    set_again(again);
    set_dgain(dgain);
    set_140a_shutter(shutter->ae_exp_line);


    return 0;
}

void sp140a_dvp_sleep()
{
    //wrsp140a_dvpReg(0xf6, 0x01);
}

void sp140a_dvp_wakeup()
{

    //rsp140a_dvpReg(0xf6, 0x00);
}

void sp140a_dvp_W_Reg(u16 addr, u16 val)
{
    //printf("update reg%x with %x\n", addr, val);
    wrsp140a_dvpReg((u8)addr, (u8)val);
}

u16 sp140a_dvp_R_Reg(u16 addr)
{
    u8 val;
    rdsp140a_dvpReg((u8)addr, &val);
    return val;
}

REGISTER_CAMERA(sp140a_dvp) = {
    .logo 				= 	"sp140a",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_BGGR,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_PCLK_SAMPLE_FALLING,
    .fps         		= 	30,

    .sen_size 			= 	{SP140A_DVP_OUTPUT_W, SP140A_DVP_OUTPUT_H},
    .isp_size 			= 	{SP140A_DVP_OUTPUT_W, SP140A_DVP_OUTPUT_H},

    .cap_fps         		= 	30,
    .sen_cap_size 			= 	{SP140A_DVP_OUTPUT_W, SP140A_DVP_OUTPUT_H},
    .isp_cap_size 			= 	{SP140A_DVP_OUTPUT_W, SP140A_DVP_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	sp140a_dvp_check,
        .init 		        = 	sp140a_dvp_init,
        .set_size_fps 		=	sp140a_dvp_set_output_size,
        .get_mipi_clk       =   sp140a_dvp_get_mipi_clk,
        .power_ctrl         =   sp140a_dvp_power_ctl,

        .get_ae_params  	=	sp140a_dvp_get_ae_params,
        .get_awb_params 	=	sp140a_dvp_get_awb_params,
        .get_iq_params 	    =	sp140a_dvp_get_iq_params,

        .sleep 		        =	sp140a_dvp_sleep,
        .wakeup 		    =	sp140a_dvp_wakeup,
        .write_reg 		    =	sp140a_dvp_W_Reg,
        .read_reg 		    =	sp140a_dvp_R_Reg,

    }
};

