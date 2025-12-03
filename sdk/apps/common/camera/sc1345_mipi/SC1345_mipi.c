#include "asm/iic.h"
#include "device/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "SC1345_mipi.h"
#include "asm/isp_alg.h"

static u32 reset_gpios[2] = {-1, -1};
static u32 pwdn_gpios[2] = {-1, -1};

extern void *SC1345_mipi_get_ae_params();
extern void *SC1345_mipi_get_awb_params();
extern void *SC1345_mipi_get_iq_params();
extern void SC1345_mipi_ae_ev_init(u8 isp_dev, u32 fps);


#define LINE_LENGTH_CLK     1664
#define FRAME_LENGTH        750
#define ROW_TIME_NS         44444
#define INPUT_CLK  24
#define PCLK  37


static u32 cur_again = -1;
static u32 cur_dgain = -1;
static u32 cur_expline = -1;
static u32 cur_line_length_clk = -1;

static void *iic = NULL;

#define WRCMD 0x60
#define RDCMD 0x61


typedef struct {
    u16 addr;
    u8 value;
} sensor_ini_regs_t;

sensor_ini_regs_t SC1345_mipi_ini_regs[] = {
    0x0103, 0x01,
    0x0100, 0x00,
    0x36e9, 0x80,
    0x301f, 0x04,
    0x320c, 0x06,
    0x320d, 0x80,
    0x320e, 0x02,
    0x320f, 0xee,
    0x3253, 0x0a,
    0x3301, 0x04,
    0x3306, 0x30,
    0x3309, 0x48,
    0x330b, 0xb0,
    0x330e, 0x18,
    0x331f, 0x41,
    0x3320, 0x05,
    0x3333, 0x10,
    0x3364, 0x17,
    0x3390, 0x08,
    0x3391, 0x18,
    0x3392, 0x38,
    0x3393, 0x06,
    0x3394, 0x10,
    0x3395, 0x40,
    0x3620, 0x08,
    0x3622, 0xc6,
    0x3630, 0xc0,
    0x3633, 0x33,
    0x3637, 0x14,
    0x3638, 0x0e,
    0x363a, 0x00,
    0x363c, 0x05,
    0x3670, 0x1e,
    0x3674, 0x90,
    0x3675, 0x90,
    0x3676, 0x90,
    0x3677, 0x82,
    0x3678, 0x86,
    0x3679, 0x8b,
    0x367c, 0x18,
    0x367d, 0x38,
    0x367e, 0x18,
    0x367f, 0x38,
    0x3690, 0x33,
    0x3691, 0x33,
    0x3692, 0x32,
    0x369c, 0x08,
    0x369d, 0x38,
    0x36a4, 0x08,
    0x36a5, 0x18,
    0x36a8, 0x02,
    0x36a9, 0x04,
    0x36aa, 0x0e,
    0x36ea, 0x0d,
    0x36eb, 0x88,
    0x36ec, 0x1d,
    0x36ed, 0x04,
    0x3e00, 0x00,
    0x3e01, 0x2e,
    0x3e02, 0x00,
    0x3e08, 0x03,
    0x3e09, 0x20,
    0x4509, 0x20,
    0x36e9, 0x37,

//0x3640,0x02, //去横线

    0x0100, 0x01,
};


unsigned char wr_SC1345_reg(u16 regID, unsigned char regDat)
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
    return ret;

}

static unsigned char rd_SC1345_reg(u16 regID, unsigned char *regDat)
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
    return ret;
}


/*************************************************************************************************
    sensor api
*************************************************************************************************/

void SC1345_mipi_config_SENSOR(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    u8 v;


    SC1345_mipi_set_output_size(isp_dev, width, height, frame_freq);

    for (i = 0; i < sizeof(SC1345_mipi_ini_regs) / sizeof(sensor_ini_regs_t); i++) {
        wr_SC1345_reg(SC1345_mipi_ini_regs[i].addr, SC1345_mipi_ini_regs[i].value);
    }

    if (25 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x03);
        wr_SC1345_reg(0x320f, 0x84);
    }

    //SC1345_mipi_ae_ev_init(isp_dev, *frame_freq);

    cur_again = -1;
    cur_dgain = -1;
    cur_expline = -1;
    cur_line_length_clk = -1;
    return;
}


s32 SC1345_mipi_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq)
{
    if (5 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x03);
        wr_SC1345_reg(0x320f, 0x84);
        wr_SC1345_reg(0x320c, 0x21);
        wr_SC1345_reg(0x320d, 0x80);
    } else if (25 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x03);
        wr_SC1345_reg(0x320f, 0x84);
        wr_SC1345_reg(0x320c, 0x06);
        wr_SC1345_reg(0x320d, 0x80);
    } else if (30 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x02);
        wr_SC1345_reg(0x320f, 0xee);
        wr_SC1345_reg(0x320c, 0x06);
        wr_SC1345_reg(0x320d, 0x30);
    } else if (20 == *frame_freq) {
        //20fps
        wr_SC1345_reg(0x320e, 0x02);
        wr_SC1345_reg(0x320f, 0xee);
        wr_SC1345_reg(0x320c, 0x09);
        wr_SC1345_reg(0x320d, 0x60);
    } else if (15 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x02);
        wr_SC1345_reg(0x320f, 0xee);
        wr_SC1345_reg(0x320c, 0x0c); //15fps
        wr_SC1345_reg(0x320d, 0x80);
    } else if (10 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x02);
        wr_SC1345_reg(0x320f, 0xee);
        wr_SC1345_reg(0x320c, 0x12); //10fps
        wr_SC1345_reg(0x320d, 0xc0);
    } else if (3 == *frame_freq) {
        wr_SC1345_reg(0x320e, 0x02);
        wr_SC1345_reg(0x320f, 0xee);
        wr_SC1345_reg(0x320c, 0x3e);
        wr_SC1345_reg(0x320d, 0xfe);
    }
    return 0;
}


s32 SC1345_mipi_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

s32 SC1345_mipi_ID_check(void)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i ;

    for (i = 0; i < 3; i++) { //
        rd_SC1345_reg(0x3107, &pid);
        rd_SC1345_reg(0x3108, &ver);
    }

    printf("sc1345_mipi,read3107=: 0x%x,read3108=: 0x%x \n", pid, ver);

    // if (pid != 0x22 && ver != 0x35)
    if (pid != 0xda || ver != 0x23) {
        puts("----not sc1345_mipi-----\n");
        return -1;
    }
    puts("----hello sc1345_mipi-----\n");
    return 0;
}

void SC1345_mipi_reset(u8 isp_dev)
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

    /*printf("pwdn_gpio=%d\n", pwdn_gpio);*/
    gpio_direction_output(pwdn_gpio, 1);

    gpio_direction_output(reset_gpio, 0);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    //gpio_direction_output(pwdn_gpio, 0);
    delay(40000);
}


static u8 cur_sensor_type = 0xff;

s32 SC1345_mipi_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
{

    puts("\n\n SC1345_mipi_check \n\n");
    if (!iic) {
        iic = dev_open(iic_name, 0);
        /* iic = dev_open("swiic0", &_sw_iic); */
        if (!iic) {
            return -1;
        }
    } else {
        if (cur_sensor_type != isp_dev) {
            return -1;
        }
    }
    printf("\n\n isp_dev =%d\n\n", isp_dev);
    // printf("\n\n isp_dev =%d\n\n", isp_dev);

    reset_gpios[isp_dev] = reset_gpio;
    pwdn_gpios[isp_dev] = pwdn_gpio;

    SC1345_mipi_reset(isp_dev);

    if (0 != SC1345_mipi_ID_check()) {
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;
}

void resetStatic();
s32 SC1345_mipi_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n SC1345_mipi_init22 \n\n");

    SC1345_mipi_config_SENSOR(isp_dev, width, height, format, frame_freq);

    return 0;
}


static void set_again(u32 again)
{
    if (cur_again == again) {
        return;
    }
    cur_again  = again;

    wr_SC1345_reg(0x3e08, (again >> 8) & 0xff);
    wr_SC1345_reg(0x3e09, (again >> 0) & 0xff);
    return;
}

static void set_dgain(u32 dgain)
{
    if (cur_dgain == dgain) {
        return;
    }
    cur_dgain  = dgain;

    wr_SC1345_reg(0x3e06, (dgain >> 8) & 0xff);
    wr_SC1345_reg(0x3e07, (dgain >> 0) & 0xff);
}



#define MAX_AGAIN (16*1024-256)
#define MAX_DGAIN (31*1024-512)

static void calc_gain(u32 gain, u32 *_again, u32 *_dgain)
{
    int i;
    u32 again, dgain;

    if (gain < 1024) {
        gain = 1024;
    }

    if (gain > 32 * 1024) {
        gain = 32 * 1024;    // 32
    }

    if (gain > MAX_AGAIN) {
        again = MAX_AGAIN;
        dgain = gain * 1024 / MAX_AGAIN;
    } else {
        again = gain;
        dgain = 1024;
    }

    //convert again

    u8 anagain = 0x03;
    u8 anafine = 0x20;
    while (again >= 2048) {
        anagain *= 2;
        anagain++;
        again /= 2;
    }
    anafine = again / 32;

    //convert dgain

    u8 diggain = 0x00;
    u8 digfine = 0x20;
    while (dgain > 2048) {
        diggain *= 2;
        diggain++;
        dgain /= 2;
    }
    digfine = dgain / 8;


    *_again = (anagain << 8) + anafine;
    *_dgain = (diggain << 8) + digfine;

    //printf("gain = %d dg = %d; cg= %d; fg= 0x%02x\n ",gain,reg0,reg1,reg2);
}


static void set_shutter(u32 texp)
{
    if (cur_expline == texp) {
        return;
    }
    cur_expline  = texp;

    wr_SC1345_reg(0x3e01, texp >> 4);
    wr_SC1345_reg(0x3e02, (texp << 4) & 0xf0);
}


u32 SC1345_mipi_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp;
    u32 texp_align;
    u32 ratio;

    texp = exp_time_us * PCLK * 2 / LINE_LENGTH_CLK;

    if (texp < 1) {
        texp = 1;
    }
    if (texp > FRAME_LENGTH * 2 - 4) {
        texp = FRAME_LENGTH * 2 - 4;
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

    //printf("exp_time_us=%d, texp=%d, gain=%d->%d\n", exp_time_us, texp, gain,shutter->ae_gain);
    return 0;

}

u32 SC1345_mipi_set_shutter(isp_ae_shutter_t *shutter)
{
    static u32 lastgain = 0;
    u32 again, dgain;

    calc_gain((shutter->ae_gain), &again, &dgain);

    set_again(again);
    set_dgain(dgain);

    set_shutter(shutter->ae_exp_line);
    /*
    set logic
    [gain<2]
    0x363a,0x0c,
    0x3622,0xc6,

    [2=<gain<4]
    0x363a,0x16,
    0x3622,0xc6,

    [4=<gain<8]
    0x363a,0x16,
    0x3622,0xd6,

    [gain>=8]
    0x363a,0x1d,
    0x3622,0xd6,
    */
    if (lastgain != shutter->ae_gain) {
        u32 aAgain = shutter->ae_gain;

        lastgain = shutter->ae_gain;
        //printf("gain = %d",lastgain);
        wr_SC1345_reg(0x3812, 0x00);
        if (aAgain < 2048) {
            wr_SC1345_reg(0x363a, 0x0c);
            wr_SC1345_reg(0x3622, 0xc6);
            //printf("logical 0~2\n");
        } else if (aAgain < 4096) {
            wr_SC1345_reg(0x363a, 0x16);
            wr_SC1345_reg(0x3622, 0xc6);
            //printf("logical 2~4\n");
        } else if (aAgain < 8192) {
            wr_SC1345_reg(0x363a, 0x16);
            wr_SC1345_reg(0x3622, 0xd6);
            //printf("logical 4~8\n");
        } else {
            wr_SC1345_reg(0x363a, 0x1d);
            wr_SC1345_reg(0x3622, 0xd6);
            //printf("logical >8\n");
        }
        wr_SC1345_reg(0x3812, 0x30);
    }
    return 0;
}

void SC1345_mipi_sleep()
{


}

void SC1345_mipi_wakeup()
{


}

void SC1345_mipi_wr_reg(u8 isp_dev, u16 addr, u16 val)
{
    printf("update reg%x with %x\n", addr, val);
    wr_SC1345_reg((u16)addr, (u8)val);
}
u16 SC1345_mipi_rd_reg(u8 isp_dev, u16 addr)
{
    u8 val;
    rd_SC1345_reg((u16)addr, &val);
    printf("read reg%x with %x\n", addr, val);
    return val;
}

REGISTER_CAMERA(SC1345_mipi) = {
    .logo 				= 	"SC1345d",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_BGGR,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_CSI2,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_CSI2_1_LANE,
    .fps         		= 	25,

    .sen_size 			= 	{SC1345_MIPI_OUTPUT_W, SC1345_MIPI_OUTPUT_H},
    .isp_size 			= 	{SC1345_MIPI_OUTPUT_W, SC1345_MIPI_OUTPUT_H},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{SC1345_MIPI_OUTPUT_W, SC1345_MIPI_OUTPUT_H},
    .isp_cap_size 			= 	{SC1345_MIPI_OUTPUT_W, SC1345_MIPI_OUTPUT_H },

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	SC1345_mipi_check,
        .init 		        = 	SC1345_mipi_init,
        .set_size_fps 		=	SC1345_mipi_set_output_size,
        .power_ctrl         =   SC1345_mipi_power_ctl,

        .get_ae_params  	=	SC1345_mipi_get_ae_params,
        .get_awb_params 	=	SC1345_mipi_get_awb_params,
        .get_iq_params 	    	=	SC1345_mipi_get_iq_params,

        .sleep 		        =	SC1345_mipi_sleep,
        .wakeup 		    =	SC1345_mipi_wakeup,
        .write_reg 		    =	SC1345_mipi_wr_reg,
        .read_reg 		    =	SC1345_mipi_rd_reg,

    }
};




