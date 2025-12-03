#include "lcd_config.h"
#include "tp_driver.h"
#include "asm/exti.h"
#include "device/gpio.h"
#include "perf_counter.h"
#include "iic.h"

#if TCFG_TP_FT6236_ENABLE

#define LOG_TAG_CONST       TP
#define LOG_TAG             "[TP]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"
#undef  _LOG_TAG
#define _LOG_TAG "[TP]FT6236>"

#define print_error_and_line()   log_error("iic write err!!! line : %d", __LINE__)


#define FT6236_READ_XY_MODE       1              ///< 获取FT6236坐标的模式 0:轮询，1:中断
#define FT6236_POLLING_XY_TIME    30             ///< 轮询获取坐标的timer时间，单位：ms
#define FT6236_INT_EDGE           EDGE_NEGATIVE  ///< 中断触发边沿选择

/* #ifndef USE_LVGL_UI_DEMO */
/* extern int ui_touch_msg_post(struct touch_event *event); */
/* #endif */

static void(*tp_drdy_sem_post_func)(void);
static void *iic;
static volatile int ft6236_int_id = -1;
static u16 ft6236_timer_id = 0;


//I2C读写命令
#define FT6236_WRCMD                        0X70//写命令
#define FT6236_RDCMD                        0X71//读命令

#define FTS_REG_INT_CNT                     0x8F
#define FTS_REG_FLOW_WORK_CNT               0x91
#define FTS_REG_WORKMODE                    0x00
#define FTS_REG_WORKMODE_FACTORY_VALUE      0x40
#define FTS_REG_WORKMODE_WORK_VALUE         0x00
#define FTS_REG_ESDCHECK_DISABLE            0x8D
#define FTS_REG_CHIP_ID                     0xA3
#define FTS_REG_CHIP_ID2                    0x9F
#define FTS_REG_POWER_MODE                  0xA5
#define FTS_REG_POWER_MODE_SLEEP_VALUE      0x03
#define FTS_REG_FW_VER                      0xA6
#define FTS_REG_VENDOR_ID                   0xA8
#define FTS_REG_LCD_BUSY_NUM                0xAB
#define FTS_REG_FACE_DEC_MODE_EN            0xB0
#define FTS_REG_FACE_DEC_MODE_STATUS        0x01
#define FTS_REG_IDE_PARA_VER_ID             0xB5
#define FTS_REG_IDE_PARA_STATUS             0xB6
#define FTS_REG_GLOVE_MODE_EN               0xC0
#define FTS_REG_COVER_MODE_EN               0xC1
#define FTS_REG_CHARGER_MODE_EN             0x8B
#define FTS_REG_GESTURE_EN                  0xD0
#define FTS_REG_GESTURE_OUTPUT_ADDRESS      0xD3
#define FTS_REG_MODULE_ID                   0xE3
#define FTS_REG_LIC_VER                     0xE4
#define FTS_REG_ESD_SATURATE                0xED

//I2C读写命令
#define FT_CMD_WR 			            	0X70//写命令
#define FT_CMD_RD 			            	0X71//读命令
//FT6236 部分寄存器定义
#define FT_DEVIDE_MODE 		            	0x00//FT6236模式控制寄存器
#define FT_REG_NUM_FINGER                   0x02//触摸状态寄存器
#define FT_TP1_REG 			            	0X03//第一个触摸点数据地址

#define	FT_ID_G_LIB_VERSION	            	0xA1//版本
#define FT_ID_G_MODE 		            	0xA4//FT6236中断模式控制寄存器
#define FT_ID_G_THGROUP		            	0x80//触摸有效值设置寄存器
#define FT_ID_G_PERIODACTIVE            	0x88//激活状态周期设置寄存器
#define Chip_Vendor_ID                      0xA3//芯片ID(0x36)
#define ID_G_FT6236ID		            	0xA8//0x11

//由FT6236芯片手册查询得到各部分寄存器地址
#define FI_DEVIDE_MODE                      0x00//FT6236模式控制寄存器
#define FI_REG_NUM_FINGER                   0x02//触摸状态寄存器

#define FI_TP1_REG                          0x03//第一个触摸点数据地址
#define FI_TP2_REG                          0x09//第一个触摸点数据地址
#define FI_TP3_REG                          0x0F//第一个触摸点数据地址
#define FI_TP4_REG                          0x15//第一个触摸点数据地址
#define FI_TP5_REG                          0x1B//第一个触摸点数据地址


static unsigned char wrFT6236Reg(u8 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, FT6236_WRCMD)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static unsigned char rdFT6236Reg(u8 regID, u8 *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, FT6236_WRCMD)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, FT6236_RDCMD)) {
        ret = 0;
        goto exit;
    }
    /* delay(100); */
    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat);
exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static void get_ft6236_xy(u16 addr, u16 *x, u16 *y)
{
    u8 buf[4];
    for (u8 i = 0; i < 4; i++) {
        rdFT6236Reg((addr + i), &buf[i]);	//读取XY坐标值
    }
    *y = (((u16)(buf[2] & 0X0F) << 8) + buf[3]);
    *x = ((u16)(buf[0] & 0X0F) << 8) + buf[1];
}


static void ft6236_interrupt(void *arg, u32 parm)
{
    tp_drdy_sem_post_func();
}


static int ft6236_int_enable(const tp_platform_data_t *pd_data)
{
#if FT6236_READ_XY_MODE
    if (ft6236_int_id < 0) {
        ft6236_int_id = exti_init(pd_data->int_pin, FT6236_INT_EDGE, ft6236_interrupt, NULL);
        if (ft6236_int_id < 0) {
            ft6236_int_id = -1;
            return -EINVAL;
        }
    }
#else
    if (ft6236_timer_id == 0) {
        ft6236_timer_id = sys_s_hi_timer_add(NULL, ft6236_interrupt, FT6236_POLLING_XY_TIME);
        if (ft6236_timer_id == 0) {
            return -EINVAL;
        }
    }
#endif
    return 0;
}


static int ft6236_int_disable(const tp_platform_data_t *pd_data)
{
#if FT6236_READ_XY_MODE
    if (ft6236_int_id >= 0) {
        exti_uninit(ft6236_int_id);
        ft6236_int_id = -1;
    }
#else
    if (ft6236_timer_id > 0) {
        sys_s_hi_timer_del((u16)ft6236_timer_id);
        ft6236_timer_id = 0;
    }
#endif
    return 0;
}


static void ft6236_get_xy(tp_info_t *data)
{
    u8 ft6236_status;
    static tp_info_t temp = {0};
    static tp_press_status_t ft6236_last_nomal_status = TP_STATUS_RELEASE;

    rdFT6236Reg(FT_REG_NUM_FINGER, &ft6236_status);
    /* log_debug("status: %d", ft6236_status); */

    if (ft6236_status) { //有触摸值
        get_ft6236_xy(FT_TP1_REG, &temp.x, &temp.y);
        temp.status = TP_STATUS_PRESS;
        ft6236_last_nomal_status = TP_STATUS_PRESS;
    } else {
        if (ft6236_last_nomal_status == TP_STATUS_PRESS) {
            temp.status = TP_STATUS_RELEASE;
            ft6236_last_nomal_status = TP_STATUS_RELEASE;
        } else {
            temp.status = TP_STATUS_NULL;
        }
    }
    data->x = temp.x;
    data->y = temp.y;
    data->status = temp.status;
}


static int ft6236_check_pid(void)
{
    u8 temp = 0;

    rdFT6236Reg(FTS_REG_CHIP_ID, &temp);
    if (temp != 100) {
        return -ENODEV;
    }

    return 0;
}


static int ft6236_init(const tp_platform_data_t *pd_data, void(*post_func)(void))
{
    log_info(">>>>>init start !!!");

    iic = dev_open(pd_data->interface, NULL);
    if (!iic) {
        log_error("open iic error!");
        return -EINVAL;
    }

    /* ft6236_reset(pd_data); */

    wrFT6236Reg(FT_DEVIDE_MODE, 0);
    wrFT6236Reg(FT_ID_G_THGROUP, 22);
    /* rdFT6236Reg(FT_ID_G_THGROUP, &read_data); */
    /* log_debug("[FT_ID_G_THGROUP]>>>>>>>>>>>read_data=%d", read_data); */
    wrFT6236Reg(FT_ID_G_PERIODACTIVE, 13);
    /* rdFT6236Reg(FT_ID_G_PERIODACTIVE, &read_data); */
    /* log_debug("[FT_ID_G_PERIODACTIVE]>>>>>>>>>>>read_data=%d", read_data); */
    wrFT6236Reg(FT_ID_G_MODE, 1);
    /* rdFT6236Reg(FT_ID_G_MODE, &read_data); */
    /* log_debug("[FT_ID_G_MODE]>>>>>>>>>>>read_data=%d", read_data); */

    if (ft6236_check_pid()) {
        log_error(">>>>>ID mismatch!!!");
        dev_close(iic);
        return -ENODEV;
    }

    tp_drdy_sem_post_func = post_func;

    if (ft6236_int_enable(pd_data)) {
        dev_close(iic);
        return -EINVAL;
    }

    return 0;
}


static int ft6236_deinit(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>deinit!!!");

    ft6236_int_disable(pd_data);

    if (pd_data->rst_pin != -1) {
        gpio_set_mode(pd_data->rst_pin, GPIO_DEINIT);
    }

    if (pd_data->int_pin != -1) {
        gpio_set_mode(pd_data->int_pin, GPIO_DEINIT);
    }

    if (!iic) {
        dev_close(iic);
        iic = NULL;
    }

    return 0;
}


static int ft6236_sleep(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>sleep!!!");

    return 0;
}


static int ft6236_wakeup(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>wakeup!!!");

    return 0;
}


static void ft6236_reset(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>reset!!!");
    gpio_direction_output(pd_data->rst_pin, 0);
    os_time_dly(6);
    gpio_direction_output(pd_data->rst_pin, 1);
    os_time_dly(20);
}


REGISTER_TP_DRIVER(ft6236_driver) = {
    .ic_name                = "FT6236",
    .init                   = ft6236_init,
    .get_xy_and_status      = ft6236_get_xy,
    .check_online           = ft6236_check_pid,
    .deinit                 = ft6236_deinit,
    /* .sleep                  = ft6236_sleep, */
    /* .wakeup                 = ft6236_wakeup, */
    .reset                  = ft6236_reset,
};

#endif

