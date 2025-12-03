#include "device/iic.h"
#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "asm/isp_alg.h"

#define TVP_DVP_OUTPUT_W    700
#define TVP_DVP_OUTPUT_H    579

//奇/偶场高度减半
#define TVP_DVP_REAL_H  (TVP_DVP_OUTPUT_H/2)

static u8 tvp_isp_dev = 0xff;

typedef struct {
    void *iic;
    u8 reset_io;
    u8 power_down_io;
    u32 cur_again;
    u32 cur_dgain;
    u32 cur_expline;
} camera_iic;

static camera_iic camera[4];

//视频
#define WRCMD 0xB8
#define RDCMD 0xB9

typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_ini;

Sensor_reg_ini TVP_DVP_INI_REG[] = {
    /* {0x00, 0x00},   //输入源AIP1A, default:0x00 */
    {0x03, 0x0d},   //打开HSYNC/VSYNC输出, default:0x01
    {0x04, 0xc0},   //全打开自动切换格式
    /* {0x18, 0x00},   //垂直消隐开始, default:0x00 */
    /* {0x19, 0x00},   //垂直消隐结束, default:0x00 */
    {0x08, 0x40},   //亮度处理控制, default:0x00
    {0x09, 0x70},   //明度, default:0x80
    /* {0x0a, 0x80},   //色度饱和度, default:0x80 */
    /* {0x0b, 0x00},   //色度色调, default:0x00 */
    {0x0c, 0x50},   //亮度对比度, default:0x80
    /* {0x0d, 0x4f}, */
};

unsigned char wrTVP_DVP_Reg(void *iic, u8 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    delay_us(110);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        puts("1 err\n");
        goto __wend;
    }
    delay_us(110);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID)) {
        ret = 0;
        puts("2 err\n");
        goto __wend;
    }
    delay_us(110);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        puts("3 err\n");
        goto __wend;
    }

    delay_us(110);
__wend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;

}

unsigned char rdTVP_DVP_Reg(void *iic, u8 regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        puts("4 \n");
        goto __rend;
    }

    delay_us(110);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regID)) {
        ret = 0;
        puts("5\n");
        goto __rend;
    }

    delay_us(110);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, RDCMD)) {
        ret = 0;
        puts("6\n");
        goto __rend;
    }

    delay_us(110);

    if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat)) {
        puts("7\n");
        ret = 0;
        goto __rend;
    }

    delay_us(110);
__rend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


/*************************************************************************************************
    sensor api
*************************************************************************************************/
static void TVP_DVP_config_SENSOR(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    void *iic = camera[isp_dev].iic;

    for (int i = 0; i < sizeof(TVP_DVP_INI_REG) / sizeof(Sensor_reg_ini); i++) {
        wrTVP_DVP_Reg(iic, TVP_DVP_INI_REG[i].addr, TVP_DVP_INI_REG[i].value);
    }

    s16 avid_start = 16;
    s16 avid_stop = 0;
    printf("start = %d  stop = %d\n\r", avid_start, avid_stop);
    wrTVP_DVP_Reg(iic, 0x11, avid_start >> 2);                  //AVID Start
    wrTVP_DVP_Reg(iic, 0x12, BIT(2) | (avid_start & 0x03));
    wrTVP_DVP_Reg(iic, 0x13, avid_stop >> 2);                   //AVID Stop
    wrTVP_DVP_Reg(iic, 0x14, (avid_stop & 0x03));

#if 1
    //等待模块状态正常输出, 如果想不接摄像头输出黑色, if 0跳过这一步
    u32 timeout = 1000;
    u8 status;
    while (1) {
        timeout--;
        if (!timeout) {
            break;
        }
        rdTVP_DVP_Reg(iic, 0x88, &status);
        put_u8hex(status);
        if ((status & 0x06) == 0x06) {
            puts("\n0x88:");
            put_u8hex(status);
            rdTVP_DVP_Reg(iic, 0x89, &status);
            puts("\n0x89:");
            put_u8hex(status);
            rdTVP_DVP_Reg(iic, 0x8C, &status);
            puts("\n0x8c:");
            put_u8hex(status);
            break;
        }
        os_time_dly(1);
    }
#endif

    return;
}

static s32 TVP_DVP_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 TVP_DVP_ID_check(void *iic)
{
    u8 pid = 0x00, pid1 = 0x00;
    u16 id = 0;

    //需要加延时，否则前面几个byte读得不正确
    os_time_dly(10);

    //soft reset
    wrTVP_DVP_Reg(iic, 0x05, 0x01);
    wrTVP_DVP_Reg(iic, 0x05, 0x00);

    u8 retry = 0;
    os_time_dly(1);
    while (retry <= 3) {
        rdTVP_DVP_Reg(iic, 0x80, &pid);
        rdTVP_DVP_Reg(iic, 0x81, &pid1);
        printf("retry: %d pid = 0x%02x 0x%02x\n\r", retry, pid, pid1);

        id = pid << 8 | pid1;
        if (id == 0x5150) {
            u16 version = 0;
            rdTVP_DVP_Reg(iic, 0x82, &pid);
            rdTVP_DVP_Reg(iic, 0x83, &pid1);
            version = pid << 8 | pid1;
            printf("\n----hello TVP5150A_DVP id:0x%04x version:0x%04x-----\n", id, version);
            return 0;
        }
        retry++;
    }

    printf("\n----not TVP5150A_DVP 0x%04x-----\n", id);

    return -1;
}

static void TVP_DVP_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    printf("%s, isp_dev = %d\n\r", __func__, isp_dev);

    res_io = camera[isp_dev].reset_io;
    powd_io = camera[isp_dev].power_down_io;

    if (powd_io != (u8) - 1) {
        gpio_direction_output(powd_io, 1);
        os_time_dly(10);
    }

    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 0);
        os_time_dly(10);
        gpio_direction_output(res_io, 1);
    }
}

static s32 TVP_DVP_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
{
    printf("\n\n %s, tvp check isp_dev = %d\n\n", __func__, isp_dev);

    if (!camera[isp_dev].iic) {
        camera[isp_dev].iic = dev_open(iic_name, 0);
        camera[isp_dev].reset_io = (u8)reset_gpio;
        camera[isp_dev].power_down_io = (u8)pwdn_gpio;
    }

    if (!camera[isp_dev].iic) {
        printf("%s tvp iic open err!!!\n\n", __func__);
        return -1;
    }

    TVP_DVP_reset(isp_dev);

    if (0 != TVP_DVP_ID_check(camera[isp_dev].iic)) {
        dev_close(camera[isp_dev].iic);
        camera[isp_dev].iic = NULL;
        return -1;
    }

    tvp_isp_dev = isp_dev;

    return 0;
}

static s32 TVP_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n TVP_DVP_init \n\n");

    camera[isp_dev].cur_again = -1;
    camera[isp_dev].cur_dgain = -1;
    camera[isp_dev].cur_expline = -1;

    TVP_DVP_config_SENSOR(isp_dev, width, height, format, frame_freq);

    printf("width = %d, height = %d\n", *width, *height);

    return 0;
}

static void TVP_DVP_W_Reg(u8 isp_dev, u16 addr, u16 val)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    wrTVP_DVP_Reg(iic, (u16)addr, (u8)val);
}

static u16 TVP_DVP_R_Reg(u8 isp_dev, u16 addr)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);
    u8 val;

    rdTVP_DVP_Reg(iic, (u16)addr, &val);
    return val;
}

static u8 TVP_DVP_avin_fps(void *parm)
{
    u8 status ;
    void *iic = parm;

    rdTVP_DVP_Reg(iic, 0x88, &status);
    if (status & BIT(5)) {
        return 1;   //场50HZ
    }
    return 0;   //场60HZ
}

static s32 TVP_DVP_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    void *iic = camera[tvp_isp_dev].iic;
    ASSERT(iic);

    if (TVP_DVP_avin_fps(iic)) {
        *freq = 25;
    } else {
        *freq = 30;
    }
    //todo
    *width = TVP_DVP_OUTPUT_W;
    *height = TVP_DVP_REAL_H;

    return 0;
}

REGISTER_CAMERA(TVP_DVP) = {
    .logo 				= 	"TVP5150A",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_UYVY,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_BT656,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B | SEN_MBUS_PCLK_SAMPLE_FALLING,
    .fps         		= 	25,

    .sen_size 			= 	{TVP_DVP_OUTPUT_W, TVP_DVP_REAL_H},
    .isp_size 			= 	{TVP_DVP_OUTPUT_W, TVP_DVP_REAL_H},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{TVP_DVP_OUTPUT_W, TVP_DVP_REAL_H},
    .isp_cap_size 			= 	{TVP_DVP_OUTPUT_W, TVP_DVP_REAL_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	TVP_DVP_check,
        .init 		        = 	TVP_DVP_init,
        .set_size_fps 		=	TVP_DVP_set_output_size,
        .power_ctrl         =   TVP_DVP_power_ctl,

        .get_ae_params  	=	NULL,
        .get_awb_params 	=	NULL,
        .get_iq_params      =	NULL,

        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	TVP_DVP_W_Reg,
        .read_reg 		    =	TVP_DVP_R_Reg,

    }
};
