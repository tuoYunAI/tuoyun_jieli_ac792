#include "device/iic.h"
#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "N5_DVP.h"
#include "asm/isp_alg.h"

//todo
#define VIDEO_BIN_INSIDE_PATH   "mnt/sdfile/res/cfg/video.bin"

static u8 n5_isp_dev = 0xff;

typedef struct {
    void *iic;
    u8 reset_io;
    u8 power_down_io;
    u32 cur_again;
    u32 cur_dgain;
    u32 cur_expline;
} camera_iic;

static camera_iic camera[4];

#define WRCMD 0x64
#define RDCMD 0x65

typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_ini;

//25pfs
Sensor_reg_ini N5_DVP_INI_REG[] = {
    {0xff, 0x00},
    {0x80, 0x00},
    {0x00, 0x10},
    {0x01, 0x10},
    {0x08, 0x00},
    {0x09, 0x00},
    {0x18, 0x3f},
    {0x19, 0x3f},
    {0x22, 0x0b},
    {0x23, 0x41},
    {0x26, 0x0b},
    {0x27, 0x41},
    {0x34, 0x00},
    {0x35, 0x00},
    {0x54, 0x01},
    {0x55, 0x10},
    {0x58, 0x74},
    {0x59, 0x74},
    {0x5c, 0x80},
    {0x5d, 0x80},
    {0x64, 0x01},
    {0x65, 0x01},
    {0x78, 0x00},//0x88
    {0x81, 0x07},
    {0x82, 0x07},
    {0x85, 0x00},
    {0x86, 0x00},
    {0x89, 0x00},
    {0x8a, 0x00},
    {0x8e, 0x00},
    {0x8f, 0x00},
    {0x30, 0x12},
    {0x31, 0x12},
    {0xa0, 0x05},
    {0xa1, 0x05},
    {0xff, 0x01},
    {0x84, 0x08},
    {0x85, 0x08},
    {0x8c, 0x08},
    {0x8D, 0x08},
    {0x7A, 0x0f},
    {0xff, 0x05},
    {0x00, 0xf0},
    {0x01, 0x22},
    {0x05, 0x04},
    {0x08, 0x55},
    {0x1b, 0x08},
    {0x20, 0x84},
    {0x25, 0xdc},
    {0x28, 0x80},
    {0x2b, 0xa8},
    {0x2f, 0x00},
    {0x30, 0xe0},
    {0x31, 0x43},
    {0x32, 0xa2},
    {0x47, 0x04},
    {0x50, 0x84},
    {0x57, 0x00},
    {0x58, 0x70},
    {0x5b, 0x41},
    {0x5c, 0x78},
    {0x5f, 0x00},
    {0x7b, 0x11},
    {0x7c, 0x01},
    {0x7d, 0x80},
    {0x80, 0x00},
    {0x90, 0x01},
    {0xa9, 0x00},
    {0xb5, 0x00},
    {0xb8, 0xb9},
    {0xb9, 0x72},
    {0xd1, 0x00},
    {0xd5, 0x80},
    {0xff, 0x09},
    {0x50, 0x30},
    {0x51, 0x6f},
    {0x52, 0x67},
    {0x53, 0x48},
    {0x54, 0x30},
    {0x55, 0x6f},
    {0x56, 0x67},
    {0x57, 0x48},
    {0x96, 0x00},
    {0x9e, 0x00},
    {0xb6, 0x00},
    {0xbe, 0x00},
    {0xff, 0x0a},
    {0x25, 0x10},
    {0x27, 0x1e},
    {0x30, 0xac},
    {0x31, 0x78},
    {0x32, 0x17},
    {0x33, 0xc1},
    {0x34, 0x40},
    {0x35, 0x00},
    {0x36, 0xc3},
    {0x37, 0x0a},
    {0x38, 0x00},
    {0x39, 0x02},
    {0x3a, 0x00},
    {0x3b, 0xb2},
    {0xa5, 0x10},
    {0xa7, 0x1e},
    {0xb0, 0xac},
    {0xb1, 0x78},
    {0xb2, 0x17},
    {0xb3, 0xc1},
    {0xb4, 0x40},
    {0xb5, 0x00},
    {0xb6, 0xc3},
    {0xb7, 0x0a},
    {0xb8, 0x00},
    {0xb9, 0x02},
    {0xba, 0x00},
    {0xbb, 0xb2},
    {0x77, 0x8F},
    {0xF7, 0x8F},
    {0xff, 0x13},
    {0x07, 0x47},
    {0x12, 0x04},
    {0x1e, 0x1f},
    {0x1f, 0x27},
    {0x2e, 0x10},
    {0x2f, 0xc8},
    {0x30, 0x00},
    {0x31, 0xff},
    {0x32, 0x00},
    {0x33, 0x00},
    {0x3a, 0xff},
    {0x3b, 0xff},
    {0x3c, 0xff},
    {0x3d, 0xff},
    {0x3e, 0xff},
    {0x3f, 0x0f},
    {0x70, 0x00},
    {0x72, 0x05},
    {0x7A, 0xf0},
    {0xff, 0x01},
    {0x97, 0x00},
    {0x97, 0x03},
    {0xA0, 0x20},
    {0xA1, 0x20},
    {0xC0, 0x10},
    {0xC1, 0x10},
    {0xC2, 0x10},
    {0xC3, 0x10},
    {0xC8, 0x22},
#ifdef DDR_SAMPLING
    {0xCC, 0x0a}, //74.25M
    {0xCD, 0x0a},
#else
    {0xCC, 0x4c}, //148.5M
    {0xCD, 0x4c},
#endif
    {0xCA, 0x11}, //port0 enabled
};


unsigned char wrN5_DVP_Reg(void *iic, u8 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        puts("1 err\n");
        goto __wend;
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID)) {
        ret = 0;
        puts("2 err\n");
        goto __wend;
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        puts("3 err\n");
        goto __wend;
    }

__wend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;

}

unsigned char rdN5_DVP_Reg(void *iic, u8 regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, WRCMD)) {
        ret = 0;
        puts("4 \n");
        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regID)) {
        ret = 0;
        puts("5\n");
        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, RDCMD)) {
        ret = 0;
        puts("6\n");
        goto __rend;
    }

    delay(100);

    if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat)) {
        puts("7\n");
        ret = 0;
        goto __rend;
    }
__rend:

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}

static int n5_dvp_set_video_parameters(u8 isp_dev)
{
    void *iic = camera[isp_dev].iic;
    u8 ch_index;
    int ret = 0;
    void *fd = NULL;
    int len;

    fd = fopen(VIDEO_BIN_INSIDE_PATH, "rb");
    if (fd != NULL) {
        printf(">>>>read chip video.bin file\r\n\r\n");
        //read chip video.bin file
        len = flen(fd);

        if (len <= 16) {
            char *p_buf = NULL;
            p_buf = (char *)malloc(len);
            if (p_buf == NULL) {
                printf("malloc fail(%d)!\n", len);
                goto exit;
            }
            memset(p_buf, 0, len);
            fread(p_buf, len, 1, fd);
            fclose(fd);

            for (ch_index = 0; ch_index < 2; ch_index++) {
                ret = wrN5_DVP_Reg(iic, 0xff, 0);//bank 0
                ret += wrN5_DVP_Reg(iic, 0x0C + ch_index, p_buf[0]);//brightness
                ret += wrN5_DVP_Reg(iic, 0x10 + ch_index, p_buf[1]);//contrast
                ret += wrN5_DVP_Reg(iic, 0x3C + ch_index, p_buf[2]);//saturation 1
                ret += wrN5_DVP_Reg(iic, 0x40 + ch_index, p_buf[3]);//hue
                ret += wrN5_DVP_Reg(iic, 0x44 + ch_index, p_buf[6]);//U gain
                ret += wrN5_DVP_Reg(iic, 0x48 + ch_index, p_buf[7]);//V gain
                ret += wrN5_DVP_Reg(iic, 0x4C + ch_index, p_buf[8]);//U offset
                ret += wrN5_DVP_Reg(iic, 0x50 + ch_index, p_buf[9]);//V offset
                ret += wrN5_DVP_Reg(iic, 0x14 + ch_index, p_buf[10]);//sharpness
                //ret = wrN5_DVP_Reg(iic, 0xff, 5 + ch_index);//bank 5/6
                //ret = wrN5_DVP_Reg(iic, 0x2B, video_parameters_buf[4]);//saturation 2
                //ret = wrN5_DVP_Reg(iic, 0x20, video_parameters_buf[5]);//black level
            }
            // for(int i=0;i<len;i++){
            //     printf("file value: %d, 0x%02x\n", i, p_buf[i]);
            // }
            // u8 tmp_value = 0;
            // rdN5_DVP_Reg(iic, 0x0C, &tmp_value);
            // printf("value:0x%02x\n",tmp_value);

            free(p_buf);
            if (ret == 10) {
                printf("------write n5 video parameters success------\n");
                goto exit;
            }
            printf("------write n5 video parameters err!!!------ ret(%d)\n", ret);
        } else {
            printf("------n5 video parameters file size err!!!------\n");
        }
    }

exit:
    return ret;
}

/*************************************************************************************************
    sensor api
*************************************************************************************************/

void N5_DVP_config_SENSOR(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    void *iic = camera[isp_dev].iic;

    N5_DVP_set_output_size(isp_dev, width, height, frame_freq);

    for (int i = 0; i < sizeof(N5_DVP_INI_REG) / sizeof(Sensor_reg_ini); i++) {
        wrN5_DVP_Reg(iic, N5_DVP_INI_REG[i].addr, N5_DVP_INI_REG[i].value);
    }

    //N5_DVP_ae_ev_init(isp_dev, *frame_freq);

//   *format = SEN_IN_FORMAT_UYVY;
    return;
}


s32 N5_DVP_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq)
{
    //todo
    *width = N5_DVP_OUTPUT_W;
    *height = N5_DVP_OUTPUT_H;

    return 0;
}


s32 N5_DVP_power_ctl(u8 isp_dev, u8 is_work)
{

    return 0;
}

s32 N5_DVP_ID_check(void *iic)
{
    u8 pid = 0x00;

    u8 retry = 0;
    while (retry <= 3) {
        wrN5_DVP_Reg(iic, 0xff, 0x00);
        rdN5_DVP_Reg(iic, 0xf4, &pid);

        if (pid == 0xe0) {
            printf("\n----hello N5_DVP id:0x%02x -----\n", pid);
            return 0;
        }
        retry++;
        delay_ms(120);
    }

    printf("\n----not N5_DVP %x-----\n", pid);
    return -1;
}

void N5_DVP_reset(u8 isp_dev)
{
    u32 reset_gpio = camera[isp_dev].reset_io;
    // u32 pwdn_gpio = camera[isp_dev].power_down_io;

    gpio_direction_output(reset_gpio, 1);
    // gpio_direction_output(pwdn_gpio, 1);
    delay_ms(120);
    gpio_direction_output(reset_gpio, 0);
    // gpio_direction_output(pwdn_gpio, 0);
    delay_ms(120);

    gpio_direction_output(reset_gpio, 1);
    //  gpio_direction_output(pwdn_gpio, 1);
    delay_ms(120);
}

void N5_DVP_change_fps(u8 sence, u16 lv)
{
    static s8 curSence = -1;

    //printf("%s: %d %d\n", __FUNCTION__, sence, lv);
}

static s32 N5_DVP_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio, char *iic_name)
{
    // if (isp_dev > 1) {
    // printf("n5 isp_dev err! \n\r");
    // return -1;
    // }
    printf("\n\n %s, n5 check isp_dev = %d\n\n", __func__, isp_dev);
    if (!camera[isp_dev].iic) {
        camera[isp_dev].iic = dev_open(iic_name, 0);
        camera[isp_dev].reset_io = (u8)reset_gpio;
        camera[isp_dev].power_down_io = (u8)pwdn_gpio;
    }

    if (!camera[isp_dev].iic) {
        printf("%s n5 iic open err!!!\n\n", __func__);
        return -1;
    }

    // ===============================
    //DVDD 1.2
    /* gpio_direction_output(IO_PORTG_13, 0); */
    //2.8 AVDD
    //avdd28_ctrl(AVDD28_28, true);

    N5_DVP_reset(isp_dev);

    if (0 != N5_DVP_ID_check(camera[isp_dev].iic)) {
        dev_close(camera[isp_dev].iic);
        camera[isp_dev].iic = NULL;
        return -1;
    }
    // ================================

    n5_isp_dev = isp_dev;
    printf("set n5_isp_dev = %d\n", n5_isp_dev);

    return 0;
}


static s32 N5_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n N5_DVP_init \n\n");

    camera[isp_dev].cur_again = -1;
    camera[isp_dev].cur_dgain = -1;
    camera[isp_dev].cur_expline = -1;

    N5_DVP_config_SENSOR(isp_dev, width, height, format, frame_freq);

    printf("width = %d, height = %d\n", *width, *height);
    //todo 文件配置
    n5_dvp_set_video_parameters(isp_dev);

    return 0;
}

void N5_DVP_sleep()
{
}

void N5_DVP_wakeup()
{
}

void N5_DVP_W_Reg(u8 isp_dev, u16 addr, u16 val)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);

    wrN5_DVP_Reg(iic, (u16)addr, (u8)val);
}

u16 N5_DVP_R_Reg(u8 isp_dev, u16 addr)
{
    void *iic = camera[isp_dev].iic;
    ASSERT(iic);
    u8 val;

    rdN5_DVP_Reg(iic, (u16)addr, &val);
    return val;
}

bool get_n5_iic_status(void)
{
    if (n5_isp_dev != 0xff && camera[n5_isp_dev].iic != NULL) {
        printf(">>>>>>>>>>n5_isp_dev: %d\n", n5_isp_dev);
        return true;
    } else {
        printf("Error: n5_isp_dev is invalid or iic is NULL\n");
        printf("n5_isp_dev: %d\n", n5_isp_dev);
        printf("iic: %p\n", camera[n5_isp_dev].iic);
        return false;
    }
}

REGISTER_CAMERA(N5_DVP) = {
    .logo 				= 	"N5",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_UYVY,
    .out_format 		= 	ISP_OUT_FORMAT_YUV,
    .mbus_type          =   SEN_MBUS_BT656,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B,
    .fps         		= 	25,

    .sen_size 			= 	{N5_DVP_OUTPUT_W, N5_DVP_OUTPUT_H},
    .isp_size 			= 	{N5_DVP_OUTPUT_W, N5_DVP_OUTPUT_H},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{N5_DVP_OUTPUT_W, N5_DVP_OUTPUT_H},
    .isp_cap_size 			= 	{N5_DVP_OUTPUT_W, N5_DVP_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	N5_DVP_check,
        .init 		        = 	N5_DVP_init,
        .set_size_fps 		=	N5_DVP_set_output_size,
        .power_ctrl         =   N5_DVP_power_ctl,

        .get_ae_params  	=	NULL,
        .get_awb_params 	=	NULL,
        .get_iq_params      =	NULL,

        .sleep 		        =	N5_DVP_sleep,
        .wakeup 		    =	N5_DVP_wakeup,
        .write_reg 		    =	N5_DVP_W_Reg,
        .read_reg 		    =	N5_DVP_R_Reg,

    }
};
