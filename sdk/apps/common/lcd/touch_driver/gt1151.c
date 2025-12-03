#include "lcd_config.h"
#include "tp_driver.h"
#include "asm/exti.h"
#include "device/gpio.h"
#include "perf_counter.h"
#include "iic.h"

#if TCFG_TP_GT1151_ENABLE

#define LOG_TAG_CONST       TP
#define LOG_TAG             "[TP]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"
#undef  _LOG_TAG
#define _LOG_TAG "[TP]GT1151>"

#define print_error_and_line()   log_error("iic write err!!! line : %d", __LINE__)


#define GT1151_READ_XY_MODE       1              ///< 获取GT1151坐标的模式 0:轮询，1:中断
#define GT1151_POLLING_XY_TIME    30             ///< 轮询获取坐标的timer时间，单位：ms
#define GT1151_INT_EDGE           EDGE_NEGATIVE  ///< 中断触发边沿选择

/* #ifndef USE_LVGL_UI_DEMO */
/* extern int ui_touch_msg_post(struct touch_event *event); */
/* #endif */

static void(*tp_drdy_sem_post_func)(void);
static void *iic;
static volatile int gt1151_int_id = -1;
static u16 gt1151_timer_id = 0;


//I2C读写命令
#define gt1151_WRCMD 			0X28     	//写命令
#define gt1151_RDCMD 			0X29		//读命令

#define GT_PID_REG0				0X8140   	//产品ID寄存器0
#define GT_PID_REG1				0X8141   	//产品ID寄存器1
#define GT_PID_REG2				0X8142   	//产品ID寄存器2
#define GT_PID_REG3				0X8143   	//产品ID寄存器3

#define GT_GSTID_REG 	        0X814E   	//当前检测到的触摸情况
#define GT_TP1_REG 		        0X8150  	//第一个触摸点数据地址 //每个点4四个地址控制GT_TP1_X_L_REG
#define GT_TP2_REG 				0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 				0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 				0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 				0X8170		//第五个触摸点数据地址


static unsigned char wrGT1151Reg(unsigned int regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, gt1151_WRCMD)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(1000); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(1000); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(1000); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(1000); */

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static unsigned char rdGT1151Reg(u16 regID, u8 *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, gt1151_WRCMD)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;;
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
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, gt1151_RDCMD)) {
        ret = 0;
        goto exit;
    }
    /* delay(100); */
    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat);
exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static void get_gt1151_xy(u16 addr, u16 *x, u16 *y)
{
    u8 buf[4];
    for (u8 i = 0; i < 4; i++) {
        rdGT1151Reg((addr + i), &buf[i]);	//读取XY坐标值
    }
    *x = buf[0] + buf[1] * 255;
    *y = buf[2] + buf[3] * 255;
}


static void gt1151_interrupt(void *arg, u32 parm)
{
    tp_drdy_sem_post_func();
}


static int gt1151_int_enable(const tp_platform_data_t *pd_data)
{
#if GT1151_READ_XY_MODE
    if (gt1151_int_id < 0) {
        gt1151_int_id = exti_init(pd_data->int_pin, GT1151_INT_EDGE, gt1151_interrupt, NULL);
        if (gt1151_int_id < 0) {
            gt1151_int_id = -1;
            return -EINVAL;
        }
    }
#else
    if (gt1151_timer_id == 0) {
        gt1151_timer_id = sys_s_hi_timer_add(NULL, gt1151_interrupt, GT1151_POLLING_XY_TIME);
        if (gt1151_timer_id == 0) {
            return -EINVAL;
        }
    }
#endif
    return 0;
}


static int gt1151_int_disable(const tp_platform_data_t *pd_data)
{
#if GT1151_READ_XY_MODE
    if (gt1151_int_id >= 0) {
        exti_uninit(gt1151_int_id);
        gt1151_int_id = -1;
    }
#else
    if (gt1151_timer_id > 0) {
        sys_s_hi_timer_del((u16)gt1151_timer_id);
        gt1151_timer_id = 0;
    }
#endif
    return 0;
}


static void gt1151_get_xy(tp_info_t *data)
{
    u8 gt1151_status;
    tp_info_t temp = {0};
    static tp_press_status_t gt1151_last_nomal_status = TP_STATUS_RELEASE;


    // 读取触摸点的状态。BIT7表示有数据, bit0-3表示触摸点个数。
    rdGT1151Reg(GT_GSTID_REG, &gt1151_status);
    /* log_debug("status: %d", gt1151_status); */

    if (gt1151_status > 0x80) { //有触摸值
        get_gt1151_xy(GT_TP1_REG, &temp.x, &temp.y);
        temp.status = TP_STATUS_PRESS;
        gt1151_last_nomal_status = TP_STATUS_PRESS;
        wrGT1151Reg(GT_GSTID_REG, 0x00); //清标志
    } else if (gt1151_status == 0x80) {
        if (gt1151_last_nomal_status == TP_STATUS_PRESS) {
            get_gt1151_xy(GT_TP1_REG, &temp.x, &temp.y);
            temp.status = TP_STATUS_RELEASE;
            gt1151_last_nomal_status = TP_STATUS_RELEASE;
        } else {
            temp.status = TP_STATUS_NULL;
        }
        wrGT1151Reg(GT_GSTID_REG, 0x00); //清标志
    } else {
        temp.status = TP_STATUS_NULL;
    }

    data->x = temp.x;
    data->y = temp.y;
    data->status = temp.status;
}


static int gt1151_check_pid(void)
{
    u8 temp = 0;

    rdGT1151Reg(GT_PID_REG0, &temp);
    if (temp != '1') {
        return -ENODEV;
    }
    rdGT1151Reg(GT_PID_REG1, &temp);
    if (temp != '1') {
        return -ENODEV;
    }
    rdGT1151Reg(GT_PID_REG2, &temp);
    if (temp != '5') {
        return -ENODEV;
    }

    return 0;
}


static int gt1151_init(const tp_platform_data_t *pd_data, void(*post_func)(void))
{
    log_info(">>>>>init start !!!");

    iic = dev_open(pd_data->interface, NULL);
    if (!iic) {
        log_error("open iic error!");
        return -EINVAL;
    }

    if (gt1151_check_pid()) {
        log_error(">>>>>ID mismatch!!!");
        dev_close(iic);
        return -ENODEV;
    }

    tp_drdy_sem_post_func = post_func;

    if (gt1151_int_enable(pd_data)) {
        dev_close(iic);
        return -EINVAL;
    }

    return 0;
}


static int gt1151_deinit(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>deinit!!!");

    gt1151_int_disable(pd_data);

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


static int gt1151_sleep(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>sleep!!!");

    return 0;
}


static int gt1151_wakeup(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>wakeup!!!");

    return 0;
}


static void gt1151_reset(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>reset!!!");
    gpio_direction_output(pd_data->rst_pin, 0);
    os_time_dly(1);
    gpio_direction_output(pd_data->rst_pin, 1);
    os_time_dly(10);
}


REGISTER_TP_DRIVER(gt1151_driver) = {
    .ic_name                = "GT1151",
    .init                   = gt1151_init,
    .get_xy_and_status      = gt1151_get_xy,
    .check_online           = gt1151_check_pid,
    .deinit                 = gt1151_deinit,
    /* .sleep                  = gt1151_sleep, */
    /* .wakeup                 = gt1151_wakeup, */
    .reset                  = gt1151_reset,
};

#endif

