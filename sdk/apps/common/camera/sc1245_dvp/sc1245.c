#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "sc1245.h"
#include "asm/isp_alg.h"
#include "device/iic.h"

extern void *sc1245_dvp_get_ae_params();
extern void *sc1245_dvp_get_awb_params();
extern void *sc1245_dvp_get_iq_params();
extern void sc1245_dvp_ae_ev_init(u8 isp_dev, u32 fps);


#define LINE_LENGTH_CLK     1600
#define FRAME_LENGTH        900
#define ROW_TIME_NS         44444
#define INPUT_CLK  24
#define PCLK  36

#define LINE_LENGTH_CLK_25FPS     0xb40
#define LINE_LENGTH_CLK_20FPS     0xe10
#define LINE_LENGTH_CLK_15FPS     0x12c0
#define ROW_TIME_NS_25FPS         35556
#define ROW_TIME_NS_20FPS         44445
#define ROW_TIME_NS_15FPS         59260

/* #define OUT_33_3FPS */

#define WRCMD 0x60
#define RDCMD 0x61

typedef struct {
    void *iic;
    u8 reset_io;
    u8 power_down_io;
    u32 cur_again;
    u32 cur_dgain;
    u32 cur_expline;
    u8 dType;
} camera_iic;
static camera_iic camera[2];    //dvp0 dvp1

typedef struct {
    u16 addr;
    u8 value;
} sensor_ini_regs_t;

const sensor_ini_regs_t SC1245_INI_REG03[] = {
    0x0103, 0x01,
    0x0100, 0x00,

    0x4500, 0x51,

//0x320e,0x02, //30fps  25:900 30:750
//0x320f,0xee,

    0x3e01, 0x38,
    0x3635, 0xa0,

    0x3620, 0x28,
    0x3309, 0xa0,
    0x331f, 0x9d,
    0x3321, 0x9f,
    0x3306, 0x65,
    0x330b, 0x66,

    0x3e08, 0x03,
    0x3e07, 0x00,
    0x3e09, 0x10,

    0x3367, 0x18, //pchtx min

    0x335e, 0x01, //ana dithering
    0x335f, 0x03,
    0x337c, 0x04,
    0x337d, 0x06,
    0x33a0, 0x05,
    0x3301, 0x05,
    0x3302, 0x7d,
    0x3633, 0x43,

    0x3908, 0x11,


    0x3621, 0x28,
    0x3622, 0x02,

//0714
    0x3637, 0x1c,
    0x3308, 0x1c,
    0x3367, 0x10,

    0x337f, 0x03, //new auto precharge  330e in 3372   [7:6] 11: close div_rst 00:open div_rst
    0x3368, 0x04, //auto precharge
    0x3369, 0x00,
    0x336a, 0x00,
    0x336b, 0x00,
    0x3367, 0x08,
    0x330e, 0x30,

    0x3366, 0x7c, // div_rst gap


//0805
    0x3635, 0xc4, //fpn
    0x3621, 0x20, //col_fpn
    0x3334, 0x40,
    0x3333, 0x10,
    0x3302, 0x3d,

//0807
    0x335e, 0x01, //ana dithering
    0x335f, 0x02,
    0x337c, 0x03,
    0x337d, 0x05,
    0x33a0, 0x04,
    0x3301, 0x03,
    0x3633, 0x48,

    0x3670, 0x0a, //[1]:3630 logic ctrl  real value in 3681
    0x367c, 0x07, //gain0
    0x367d, 0x0f, //gain1
    0x3674, 0x20, //<gain0
    0x3675, 0x2f, //gain0 - gain1
    0x3676, 0x2f, //>gain1


    0x3631, 0x84,
    0x3622, 0x06,
    0x3630, 0x20,

    0x3620, 0x08, //col_fpn


//0x330b,0x66,

    0x3638, 0x1f, //ramp by clk
    0x3625, 0x02,
    0x3637, 0x1e,
    0x3636, 0x25,

    0x3306, 0x1c, //5.0k
    0x3637, 0x1a,
    0x331e, 0x0c,
    0x331f, 0x9c,
    0x3320, 0x0c,
    0x3321, 0x9c,

    0x366e, 0x08, // ofs auto en [3]
    0x366f, 0x2a, // ofs+finegain  real ofs in 0x3687[4:0]

    0x3622, 0x16, //sig clamp
    0x363b, 0x0c, //hvdd

    0x3639, 0x0a,
    0x3632, 0x28,
    0x3038, 0x84,

//0814
    0x3635, 0xc0,

//0819

//digital ctrl

    0x3f00, 0x07, // bit[2] = 1
    0x3f04, 0x02,
    0x3f05, 0xfc, // hts / 2 - 0x24

    0x3802, 0x01,
    0x3235, 0x07,
    0x3236, 0x06, // vts x 2 - 2

//0831
    0x3639, 0x09,
    0x3670, 0x02,

//0904
    0x3635, 0x80,

//0906
#ifdef OUT_33_3FPS
    0x320e, 0x02,
    0x320f, 0xee,
#else
    0x320e, 0x03,
    0x320f, 0x84,
#endif
    0x3235, 0x05,
    0x3236, 0xda,

#ifdef OUT_33_3FPS
    0x3039, 0x58,
    0x303a, 0xC2,
#else
    0x3039, 0x51,
    0x303a, 0xba,
#endif
    0x3034, 0x06,
    0x3035, 0xe2,

//for AE control per frame
    0x3633, 0x53,
    0x3802, 0x00,

    0x3e03, 0x03, //gain mode --->0908

    0x3640, 0x03,
    0x3221, 0x00,
    0x0100, 0x01,

};

const sensor_ini_regs_t SC1245_INI_REG02[] = {
    0x0103, 0x01,
    0x0100, 0x00,

    0x4500, 0x51,

#ifdef OUT_33_3FPS
    0x320e, 0x02, //25fps
    0x320f, 0xee,
#else
    0x320e, 0x03, //25fps
    0x320f, 0x84,
#endif

    0x3e01, 0x38,
    0x3635, 0xa0,

    0x3620, 0x28,
    0x3309, 0xa0,
    0x331f, 0x9d,
    0x3321, 0x9f,
    0x3306, 0x65,
    0x330b, 0x66,

    0x3e08, 0x03,
    0x3e07, 0x00,
    0x3e09, 0x10,

    0x3367, 0x18, //pchtx min

    0x335e, 0x01, //ana dithering
    0x335f, 0x03,
    0x337c, 0x04,
    0x337d, 0x06,
    0x33a0, 0x05,
    0x3301, 0x05,
    0x3302, 0x7d,
    0x3633, 0x43,

    0x3908, 0x11,


    0x3621, 0x28,
    0x3622, 0x02,

//0714
    0x3637, 0x1c,
    0x3308, 0x1c,
    0x3367, 0x10,

    0x337f, 0x03, //new auto precharge  330e in 3372   [7:6] 11: close div_rst 00:open div_rst
    0x3368, 0x04, //auto precharge
    0x3369, 0x00,
    0x336a, 0x00,
    0x336b, 0x00,
    0x3367, 0x08,
    0x330e, 0x30,

    0x3366, 0x7c, // div_rst gap


//0805
    0x3635, 0xc4, //fpn
    0x3621, 0x20, //col_fpn
    0x3334, 0x40,
    0x3333, 0x10,
    0x3302, 0x3d,

//0807
    0x335e, 0x01, //ana dithering
    0x335f, 0x02,
    0x337c, 0x03,
    0x337d, 0x05,
    0x33a0, 0x04,
    0x3301, 0x03,
    0x3633, 0x48,

    0x3670, 0x0a, //[1]:3630 logic ctrl  real value in 3681
    0x367c, 0x07, //gain0
    0x367d, 0x0f, //gain1
    0x3674, 0x20, //<gain0
    0x3675, 0x2f, //gain0 - gain1
    0x3676, 0x2f, //>gain1


    0x3631, 0x84,
    0x3622, 0x06,
    0x3630, 0x20,

    0x3620, 0x08, //col_fpn


//0x330b,0x66,

    0x3638, 0x1f, //ramp by clk
    0x3625, 0x02,
    0x3637, 0x1e,
    0x3636, 0x25,

    0x3306, 0x1c, //5.0k
    0x3637, 0x1a,
    0x331e, 0x0c,
    0x331f, 0x9c,
    0x3320, 0x0c,
    0x3321, 0x9c,

    0x366e, 0x08, // ofs auto en [3]
    0x366f, 0x2a, // ofs+finegain  real ofs in 0x3687[4:0]

    0x3622, 0x16, //sig clamp
    0x363b, 0x0c, //hvdd

    0x3639, 0x0a,
    0x3632, 0x28,
    0x3038, 0x84,

//0814
    0x3635, 0xc0,

//0819

//digital ctrl

    0x3f00, 0x07, // bit[2] = 1
    0x3f04, 0x02,
    0x3f05, 0xfc, // hts / 2 - 0x24

    0x3802, 0x01,
    0x3235, 0x07,
    0x3236, 0x06, // vts x 2 - 2

//0831
    0x3639, 0x09,
    0x3670, 0x02,

//0904
    0x3635, 0x80,
#ifdef OUT_33_3FPS
//0906
    0x320e, 0x02,
    0x320f, 0xee,
#else
    0x320e, 0x03,
    0x320f, 0x84,
#endif

    0x3235, 0x05,
    0x3236, 0xda,

#ifdef OUT_33_3FPS
    0x3039, 0x58,
    0x303a, 0xc2,
#else
    0x3039, 0x51,
    0x303a, 0xba,
#endif
    0x3034, 0x06,
    0x3035, 0xe2,

    0x3208, 0x05,
    0x3209, 0x08,
    0x320a, 0x02,
    0x320b, 0xd8,

    0x3211, 0x04,
    0x3213, 0x04,

    0x3302, 0x7d, //20171027  add 3301 margin

//1103
    0x3357, 0x51,

//1106
    0x3326, 0x00,
    0x3303, 0x20,
    0x3320, 0x07,
    0x331e, 0x11,
    0x331f, 0x91,

    0x366f, 0x2c,


    0x3333, 0x30,
    0x331b, 0x83,

    0x330b, 0x5e,

    0x3620, 0x28,
    0x3631, 0x86,

    0x3633, 0x42,
    0x3301, 0x04,
    0x3622, 0xd6,

//20171108
    0x3635, 0x84, //trade off between FPN and Lag
    0x3e01, 0x5d, //20171112
    0x3e02, 0x80,

//20171219
    0x3802, 0x00,
    0x3e03, 0x03,
    0x0100, 0x01,
};

unsigned char wr_sc1245_reg(void *iic, u16 regID, unsigned char regDat)
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

unsigned char rd_sc1245_reg(void *iic, u16 regID, unsigned char *regDat)
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
void sc1245_dvp_config_SENSOR(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 sc1245_frame_rate = 30;
    u32 i;

    void *iic = camera[isp_dev].iic;

    rd_sc1245_reg(iic, 0x3020, &camera[isp_dev].dType);
    printf("sc1245 ver:%d\n", camera[isp_dev].dType);
    sc1245_dvp_set_output_size(isp_dev, width, height, frame_freq);

    if (0x03 == camera[isp_dev].dType) {
        for (i = 0; i < sizeof(SC1245_INI_REG03) / sizeof(sensor_ini_regs_t); i++) {
            wr_sc1245_reg(iic, SC1245_INI_REG03[i].addr, SC1245_INI_REG03[i].value);
        }
    }
    if (0x02 == camera[isp_dev].dType) {
        for (i = 0; i < sizeof(SC1245_INI_REG02) / sizeof(sensor_ini_regs_t); i++) {
            wr_sc1245_reg(iic, SC1245_INI_REG02[i].addr, SC1245_INI_REG02[i].value);
        }
    }

    sc1245_frame_rate = *frame_freq;
#ifdef OUT_33_3FPS
    /* if(sc1245_frame_rate==30){ */
    /* //output format 36M 1600X750 30fps */
    /* wr_sc1245_reg(iic, 0x320e, 0x02); */
    /* wr_sc1245_reg(iic, 0x320f, 0xee); */
    /* }else if(sc1245_frame_rate==25){ */
    /* //output format 36M 1600X900 25fps */
    /* wr_sc1245_reg(iic, 0x320e, 0x03); */
    /* wr_sc1245_reg(iic, 0x320f, 0x84); */
    /* }else if(sc1245_frame_rate==33){ */
    /* //output format 36M 1600X750 33.3fps */
    wr_sc1245_reg(iic, 0x320c, 0x06);
    wr_sc1245_reg(iic, 0x320d, 0x40);
    wr_sc1245_reg(iic, 0x320e, 0x02);
    wr_sc1245_reg(iic, 0x320f, 0xEE);
    /* } */
#else
    if (sc1245_frame_rate == 30) {
        //output format 36M 1600X750 30fps
        wr_sc1245_reg(iic, 0x320e, 0x02);
        wr_sc1245_reg(iic, 0x320f, 0xee);
    } else if (sc1245_frame_rate == 25) {
        //output format 36M 1600X900 25fps
        wr_sc1245_reg(iic, 0x320e, 0x03);
        wr_sc1245_reg(iic, 0x320f, 0x84);
    }
#endif
    sc1245_dvp_ae_ev_init(isp_dev, *frame_freq);

    return;
}


s32 sc1245_dvp_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq)
{
    return 0;
}


s32 sc1245_dvp_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

s32 sc1245_dvp_ID_check(void *iic)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i ;

    for (i = 0; i < 3; i++) { //
        rd_sc1245_reg(iic, 0x3107, &pid);
        rd_sc1245_reg(iic, 0x3108, &ver);
    }

    printf("Sensor PID %02x %02x\n", pid, ver);

    // if (pid != 0x22 && ver != 0x35)
    if (pid != 0x12 && ver != 0x45) {
        puts("----not sc1245_dvp-----\n");
        return -1;
    }
    puts("----hello sc1245_dvp-----\n");
    return 0;
}

void sc1245_dvp_reset(u8 isp_dev)
{
    u32 reset_gpio = camera[isp_dev].reset_io;
    u32 pwdn_gpio = camera[isp_dev].power_down_io;

    /*printf("pwdn_gpio=%d\n", pwdn_gpio);*/
    gpio_direction_output(pwdn_gpio, 1);

    /* if (reset_gpio == -1){ */
    /* while(1){ */

    /* puts("=====>>>\n");		 */
    /* gpio_direction_output(reset_gpio, 0); */
    /* DAA_CON0 &= ~(BIT(2)); */
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    delay(40000);
    /* DAA_CON0 |= (BIT(2)); */
    //gpio_direction_output(pwdn_gpio, 0);
    delay(40000);
    delay(40000);
    /* delay(40000); */
    /* delay(40000); */
    /* } */
    /* }else{ */
    gpio_direction_output(reset_gpio, 0);
    delay(40000);
    gpio_direction_output(reset_gpio, 1);
    /* //gpio_direction_output(pwdn_gpio, 0); */
    /* delay(40000); */
    /* } */
}

s32 sc1245_dvp_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
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

    sc1245_dvp_reset(isp_dev);

    if (0 != sc1245_dvp_ID_check(camera[isp_dev].iic)) {
        dev_close(camera[isp_dev].iic);
        camera[isp_dev].iic = NULL;
        return -1;
    }

    return 0;
}

void resetStatic();
s32 sc1245_dvp_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n sc1245_dvp_init22 \n\n");

    camera[isp_dev].cur_again = -1;
    camera[isp_dev].cur_dgain = -1;
    camera[isp_dev].cur_expline = -1;

    sc1245_dvp_config_SENSOR(isp_dev, width, height, format, frame_freq);

    return 0;
}

static u32 updatedcgain(u32 val)
{
    if (val >= 16) {
        return 16;
    }
    if (val >= 8) {
        return 8;
    }
    if (val >= 4) {
        return 4;
    }
    if (val >= 2) {
        return 2;
    }
    return 1;
}

static u32 convertdcgain2cfg(u32 val)
{
    if (val == 16) {
        return 0x0f;
    }
    if (val == 8) {
        return 0x07;
    }
    if (val == 4) {
        return 0x03;
    }
    if (val == 2) {
        return 0x01;
    }
    return 0;
}

static void updateforshutter()
{
//    if (val3e01 < 0x05)
//        wrSC1245Reg(0x3314, 0x12);
//    if (val3e01 > 0x10)
//        wrSC1245Reg(0x3314, 0x02);
}

static void updateforgain02(u8 isp_dev, u32 dgain)
{
    u8 data2, data1;
    void *iic = camera[isp_dev].iic;

    data1 = dgain & 0xff;
    data2 = (dgain >> 8) & 0xff;

    wr_sc1245_reg(iic, 0x3903, 0x84);
    wr_sc1245_reg(iic, 0x3903, 0x04);
    wr_sc1245_reg(iic, 0x3812, 0x00);

    if (data2 == 0) { //<2
        wr_sc1245_reg(iic, 0x3633, 0x42); //20171212
        wr_sc1245_reg(iic, 0x3301, 0x05); //20171106 [3,4]
        wr_sc1245_reg(iic, 0x3622, 0xd6);
        updateforshutter();
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    if (data2 <= 4) { //<4
        wr_sc1245_reg(iic, 0x3633, 0x42); //20171212
        wr_sc1245_reg(iic, 0x3301, 0x06); //20171106 [3,4]
        wr_sc1245_reg(iic, 0x3622, 0xd6);
        updateforshutter();
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    if (data2 <= 0x0c) { //<8
        wr_sc1245_reg(iic, 0x3633, 0x42); //20171212
        wr_sc1245_reg(iic, 0x3301, 0x06); //20171106 [3,4]
        wr_sc1245_reg(iic, 0x3622, 0xd6);
        updateforshutter();
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    if (data2 <= 0x1c) { //<15.5
        wr_sc1245_reg(iic, 0x3633, 0x42); //20171212
        wr_sc1245_reg(iic, 0x3301, 0x07); //20171106 [3,4]
        wr_sc1245_reg(iic, 0x3622, 0x16);
        updateforshutter();
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    if (data1 == 0x1f) { //>=15.5
        wr_sc1245_reg(iic, 0x3633, 0x42); //20171212
        wr_sc1245_reg(iic, 0x3301, 0x32); //20171106 [3,4]
        wr_sc1245_reg(iic, 0x3622, 0x16);
        updateforshutter();
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    } else {
        wr_sc1245_reg(iic, 0x3633, 0x42); //20171212
        wr_sc1245_reg(iic, 0x3301, 0x07); //20171106 [3,4]
        wr_sc1245_reg(iic, 0x3622, 0x16);
        updateforshutter();
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
}

static void updateforgain03(u8 isp_dev, u32 gain)
{
    void *iic = camera[isp_dev].iic;

    wr_sc1245_reg(iic, 0x3903, 0x84);
    wr_sc1245_reg(iic, 0x3903, 0x04);
    wr_sc1245_reg(iic, 0x3812, 0x00);

    if (gain < 2) {
        wr_sc1245_reg(iic, 0x3301, 0x07);
        wr_sc1245_reg(iic, 0x3631, 0x84);
        wr_sc1245_reg(iic, 0x3620, 0x28);
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    if (gain < 4) {
        wr_sc1245_reg(iic, 0x3301, 0x08);
        wr_sc1245_reg(iic, 0x3631, 0x86);
        wr_sc1245_reg(iic, 0x3620, 0x08);
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    if (gain < 8) {
        wr_sc1245_reg(iic, 0x3301, 0x09);
        wr_sc1245_reg(iic, 0x3631, 0x86);
        wr_sc1245_reg(iic, 0x3620, 0x08);
        wr_sc1245_reg(iic, 0x3812, 0x30);
        return;
    }
    wr_sc1245_reg(iic, 0x3301, 0x20);
    wr_sc1245_reg(iic, 0x3631, 0x86);
    wr_sc1245_reg(iic, 0x3620, 0x08);
    wr_sc1245_reg(iic, 0x3812, 0x30);

}

static void set_again(u8 isp_dev, u32 again)
{
    if (camera[isp_dev].cur_again == again) {
        return;
    }
    void *iic = camera[isp_dev].iic;

    camera[isp_dev].cur_again = again;
//    calc_gain(aGain,&gainval);
//
//    wrSC1245Reg(0x3e07, (gainval >> 16)&0xff);
//    wrSC1245Reg(0x3e08, ((gainval >> 8)&0xff)|(0x03));
//    wrSC1245Reg(0x3e09, gainval & 0xff);

    wr_sc1245_reg(iic, 0x3e08, ((again >> 14) & 0xff));
    wr_sc1245_reg(iic, 0x3e09, (again >> 6) & 0xff);
    if (0x03 == camera[isp_dev].dType) {
        updateforgain03(isp_dev, again / 1024);
    }
    if (0x02 == camera[isp_dev].dType) {
        updateforgain02(isp_dev, again);
    }
    return;
}

static void set_dgain(u8 isp_dev, u32 dgain, u32 gain)
{
    if (camera[isp_dev].cur_dgain == dgain) {
        return;
    }
    void *iic = camera[isp_dev].iic;
    camera[isp_dev].cur_dgain = dgain;

    wr_sc1245_reg(iic, 0x3e0f, 0x84 | ((dgain << 5) & 0xf0)); // ppk add dgain on 20151015
}

static void calc_gain(u32 gain, u32 *_again, u32 *_dgain)
{
    /* int i; */
    /* u32 reg0, reg1, reg2; */

    /* if(gain <1024) gain = 1024; */
    /* if(gain > 63*1024) gain = 63*1024; */

    /* reg0 = gain /8/1024; */

    /* reg1 = gain /1024; */
    /* if (reg1>8) */
    /* reg1 = 8; */
    /* reg1=updatedcgain(reg1); */
    /* reg0=updatedcgain(reg0); */

    /* reg2 = gain /reg0/reg1/64; */

    /* *_again = 0; */
    /* *_dgain = (convertdcgain2cfg(reg0) <<13)|(convertdcgain2cfg(reg1)<<10)|(reg2); */
    //printf("gain = %d dg = %d; cg= %d; fg= 0x%02x\n ",gain,reg0,reg1,reg2);
    //


    if (gain < 1024) {
        gain = 1024;
    }
    if (gain > 62 * 1024) {
        gain = 62 * 1024;
    }

    *_again = gain;
    *_dgain = 0;

}

static void set_shutter(u8 isp_dev, u32 texp)
{

    if (camera[isp_dev].cur_expline == texp) {
        return;
    }
    void *iic = camera[isp_dev].iic;
    camera[isp_dev].cur_expline  = texp;

    wr_sc1245_reg(iic, 0x3e01, texp >> 4);
    wr_sc1245_reg(iic, 0x3e02, (texp << 4) & 0xf0);
}


u32 sc1245_dvp_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp;
    u32 texp_align;
    u32 ratio;
    u32 line_length_clk = LINE_LENGTH_CLK;

#if SC1245_FPS_VARIABLE
    if (exp_time_us <= 30000) {
        line_length_clk = LINE_LENGTH_CLK;
        texp = exp_time_us * 1000 * 2 / ROW_TIME_NS;
    } else if (exp_time_us <= 40000) {
        line_length_clk = LINE_LENGTH_CLK_25FPS;
        texp = exp_time_us * 1000 * 2 / ROW_TIME_NS_25FPS;
    } else if (exp_time_us <= 50000) {
        line_length_clk = LINE_LENGTH_CLK_20FPS;
        texp = exp_time_us * 1000 * 2 / ROW_TIME_NS_20FPS;
    } else {
        line_length_clk = LINE_LENGTH_CLK_15FPS;
        texp = exp_time_us * 1000 * 2 / ROW_TIME_NS_15FPS;
    }
#else


    texp = exp_time_us * 1000 * 2 / ROW_TIME_NS;
#endif // SC1245_FPS_VARIABLE
    if (texp < 1) {
        texp = 1;
    }
    if (texp > FRAME_LENGTH * 2 - 4) {
        texp = FRAME_LENGTH * 2 - 4;
    }
    texp_align = (texp) * ROW_TIME_NS / (1000 * 2);

    if (texp_align < exp_time_us) {
        ratio = (exp_time_us) * (1 << 10) / texp_align;
        //printf("ratio = %d\n",ratio);
    } else {
        ratio = (1 << 10);
    }

    shutter->ae_exp_line =  texp;
    shutter->ae_gain = (gain * ratio) >> 10;
    shutter->ae_exp_clk = 0;

    // printf("exp_time_us=%d, texp=%d, gain=%d->%d\n", exp_time_us, texp, gain, shutter->ae_gain);
    return 0;

}

u32 sc1245_dvp_set_shutter(isp_ae_shutter_t *shutter, u8 isp_dev)
{
    /* static int ffff = 0; */
    /* static u8 aaaa = 0; */
    static u32 lastdgain = -1;
    static u32 lastexline = -1;
    u32 again, dgain;

    calc_gain((shutter->ae_gain), &again, &dgain);


    set_again(isp_dev, again);
    set_dgain(isp_dev, dgain, shutter->ae_gain / 1024);

    set_shutter(isp_dev, shutter->ae_exp_line);

    if ((lastdgain != dgain)) { //||(lastexline!=shutter->ae_exp_line))
//        updateforgain(shutter->ae_gain/1024);
//        wr_sc1245_reg(iic, 0x3812,0x30);
    }


    lastdgain = dgain;
    lastexline = shutter->ae_exp_line;
    return 0;
}

void sc1245_dvp_sleep()
{


}

void sc1245_dvp_wakeup()
{


}

void sc1245_dvp_wr_reg(u8 isp_dev, u16 addr, u16 val)
{
    void *iic = camera[isp_dev].iic;
    printf("update reg%x with %x\n", addr, val);
    wr_sc1245_reg(iic, (u16)addr, (u8)val);
}

u16 sc1245_dvp_rd_reg(u8 isp_dev, u16 addr)
{
    void *iic = camera[isp_dev].iic;
    u8 val;
    rd_sc1245_reg(iic, (u16)addr, &val);
    return val;
}

REGISTER_CAMERA(sc1245_dvp) = {
    .logo 				= 	"sc1245d",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_BGGR,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_10B | SEN_MBUS_PCLK_SAMPLE_FALLING,

#ifdef OUT_33_3FPS
    .fps         		= 	30,
    .real_fps           = (u32)(33.333333 * 65536),
#else
    /* .fps         		= 	30, */
    .fps         		= 	25,
#endif

    .sen_size 			= 	{SC1245_DVP_OUTPUT_W, SC1245_DVP_OUTPUT_H},
    .isp_size 			= 	{SC1245_DVP_OUTPUT_W, SC1245_DVP_OUTPUT_H},

#ifdef OUT_33_3FPS
    .cap_fps         		= 	30,
#else
    /* .cap_fps         		= 	30, */
    .cap_fps         		= 	25,
#endif
    .sen_cap_size 			= 	{SC1245_DVP_OUTPUT_W, SC1245_DVP_OUTPUT_H},
    .isp_cap_size 			= 	{SC1245_DVP_OUTPUT_W, SC1245_DVP_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	sc1245_dvp_check,
        .init 		        = 	sc1245_dvp_init,
        .set_size_fps 		=	sc1245_dvp_set_output_size,
        .power_ctrl         =   sc1245_dvp_power_ctl,

        .get_ae_params  	=	sc1245_dvp_get_ae_params,
        .get_awb_params 	=	sc1245_dvp_get_awb_params,
        .get_iq_params 	    	=	sc1245_dvp_get_iq_params,

        .sleep 		        =	sc1245_dvp_sleep,
        .wakeup 		    =	sc1245_dvp_wakeup,
        .write_reg 		    =	sc1245_dvp_wr_reg,
        .read_reg 		    =	sc1245_dvp_rd_reg,

    }
};



