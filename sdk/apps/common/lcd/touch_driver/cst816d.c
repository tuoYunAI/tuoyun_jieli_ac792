#include "lcd_config.h"
#include "tp_driver.h"
#include "asm/exti.h"
#include "device/gpio.h"
#include "perf_counter.h"
#include "iic.h"

#if TCFG_TP_CST816D_ENABLE

#define LOG_TAG_CONST       TP
#define LOG_TAG             "[TP]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"
#undef  _LOG_TAG
#define _LOG_TAG "[TP]CST816D>"


#define CST816D_READ_XY_MODE     1              ///< 获取CST816D坐标的模式 0:轮询，1:中断
#define CST816D_POLLING_XY_TIME  30             ///< 轮询获取坐标的timer时间，单位：ms
#define CST816D_INT_EDGE         EDGE_NEGATIVE  ///< 中断触发边沿选择

#define print_error_and_line()   log_error("iic write err!!! line : %d", __LINE__)

static void(*tp_drdy_sem_post_func)(void);
static void *iic;
static int cst816d_int_id;
static u16 cst816d_timer_id;


#define CST816D_ADDR                            0X2A // CST816D IIC地址
#define CST816D_CHIP_ID                         0xB5

//CST816寄存器
#define CST_REG_GESTURE_ID                      0x01 //手势寄存器
#define CST_REG_FINGERNUM                       0x02 //手指数量
#define CST_REG_X_POS_H                         0x03 //x高四位
#define CST_REG_X_POS_L                         0x04 //x低八位
#define CST_REG_Y_POS_H                         0x05 //y高四位
#define CST_REG_Y_POS_L                         0x06 //y低八位
#define CST_REG_CHIP_ID                         0xA7 //芯片型号
#define	CST_REG_MOTION_MASK                     0xEC //触发动作
#define CST_REG_AUTO_SLEEP_TIME	                0xF9 //自动休眠
#define CST_REG_IRQ_CTL                         0xFA //中断控制
#define CST_REG_AUTO_RESET                      0xFB //无手势休眠
#define CST_REG_LONG_PRESS_TIME	                0xFC //长按休眠
#define CST_REG_DIS_AUTO_SLEEP                  0xFE //使能低功耗模式
#define CST_XY_DATA_SIZE                        4    //数据大小


static int cst816d_reg_buf_write(u8 reg, u8 *data, u8 len)
{
    int ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST816D_ADDR)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)reg)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    int i = 0;
    for (; i < (len - 1); i++) {
        if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, data[i])) {
            ret = -EIO;
            print_error_and_line();
            goto exit;
        }
        /* delay(100); */
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, data[i])) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static int cst816d_write_reg(u8 reg, u8 data)
{
    return cst816d_reg_buf_write(reg, &data, 1);
}


static int cst816d_buf_read(u8 reg, u8 *buf, u8 len)
{
    u8 ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST816D_ADDR)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)reg)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST816D_ADDR + 1)) {
        ret = -EIO;
        goto exit;
    }
    /* delay(100); */

    int i = 0;
    for (; i < (len - 1); i++) {
        if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_ACK, (u32)(buf + i))) {
            ret = -EIO;
            print_error_and_line();
            goto exit;
        }
        /* delay(100); */
    }

    if (dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)(buf + i))) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static int cst816d_read_reg(u8 reg, u8 *data)
{
    int ret = cst816d_buf_read(reg, data, 1);
    return ret;
}


static int cst816d_get_fig1_xy(u16 *x, u16 *y)
{
    u8 buf[CST_XY_DATA_SIZE];
    int ret = cst816d_buf_read(CST_REG_X_POS_H, buf, CST_XY_DATA_SIZE);

    *x = ((buf[0] & 0x0F) << 8) | buf[1];
    *y = ((buf[2] & 0x0F) << 8) | buf[3];

    return ret;
}


static void cst816d_interrupt(void *arg, u32 parm)
{
    tp_drdy_sem_post_func();
}


static void cst816d_get_xy(tp_info_t *data)
{
    u8 cst816d_status;
    tp_info_t temp = {0};
    static tp_press_status_t cst816d_last_nomal_status = TP_STATUS_RELEASE;

    cst816d_read_reg(CST_REG_FINGERNUM, &cst816d_status);
    log_debug("status: %d", cst816d_status);

    if (cst816d_status) { //有触摸值
        cst816d_get_fig1_xy(&temp.x, &temp.y);
        temp.status = TP_STATUS_PRESS;
        cst816d_last_nomal_status = TP_STATUS_PRESS;
    } else {
        if (cst816d_last_nomal_status == TP_STATUS_PRESS) {
            cst816d_get_fig1_xy(&temp.x, &temp.y);
            temp.status = TP_STATUS_RELEASE;
            cst816d_last_nomal_status = TP_STATUS_RELEASE;
        } else {
            temp.status = TP_STATUS_NULL;
        }
    }

    data->x = temp.x;
    data->y = temp.y;
    data->status = temp.status;
}


static int cst816d_check_pid(void)
{
    u8 touch_id = 0;
    cst816d_read_reg(CST_REG_CHIP_ID, &touch_id);
    log_debug("get touch id : 0x%x", touch_id);
    if (touch_id != CST816D_CHIP_ID) {
        return -ENODEV;
    }
    return 0;
}


static void cst816d_reset(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>reset!!!");
    /* gpio_direction_output(pd_data->rst_pin, 1); */
    /* os_time_dly(1); */
    gpio_direction_output(pd_data->rst_pin, 0);
    os_time_dly(1);
    gpio_direction_output(pd_data->rst_pin, 1);
    os_time_dly(20);
}


static int cst816d_init(const tp_platform_data_t *pd_data, void(*post_func)(void))
{
    log_info(">>>>>init start !!!");
    cst816d_reset(pd_data);

    iic = dev_open(pd_data->interface, NULL);
    if (!iic) {
        log_error("open iic error!");
        return -EINVAL;
    }

    if (cst816d_check_pid()) {
        log_error(">>>>>ID mismatch!!!");
        dev_close(iic);
        return -ENODEV;
    }

    tp_drdy_sem_post_func = post_func;

    if (cst816d_int_enable(pd_data)) {
        dev_close(iic);
        return -EINVAL;
    }

    return 0;
}


static int cst816d_deinit(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>deinit!!!");

    cst816d_int_disable(pd_data);

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


static int cst816d_sleep(const tp_platform_data_t *pd_data)
{
    /* log_info(">>>>>sleep!!!"); */

    return 0;
}


static int cst816d_wakeup(const tp_platform_data_t *pd_data)
{
    /* log_info(">>>>>wakeup!!!"); */

    return 0;
}


static int cst816d_int_enable(const tp_platform_data_t *pd_data)
{
#if CST816D_READ_XY_MODE
    if (cst816d_int_id < 0) {
        cst816d_int_id = exti_init(pd_data->int_pin, CST816D_INT_EDGE, cst816d_interrupt, NULL);
        if (cst816d_int_id < 0) {
            cst816d_int_id = -1;
            return -EINVAL;
        }
    }
#else
    if (cst816d_timer_id == 0) {
        cst816d_timer_id = sys_s_hi_timer_add(NULL, cst816d_interrupt, CST816D_POLLING_XY_TIME);
        if (cst816d_timer_id == 0) {
            return -EINVAL;
        }
    }
#endif
    return 0;
}


static int cst816d_int_disable(const tp_platform_data_t *pd_data)
{
#if CST816D_READ_XY_MODE
    if (cst816d_int_id >= 0) {
        exti_uninit(cst816d_int_id);
        cst816d_int_id = -1;
    }
#else
    if (cst816d_timer_id > 0) {
        sys_s_hi_timer_del((u16)cst816d_timer_id);
        cst816d_timer_id = 0;
    }
#endif
    return 0;
}


REGISTER_TP_DRIVER(cst816d_driver) = {
    .ic_name                = "CST816D",
    .init                   = cst816d_init,
    .get_xy_and_status      = cst816d_get_xy,
    .check_online           = cst816d_check_pid,
    .deinit                 = cst816d_deinit,
    /* .sleep                  = cst816d_sleep, */
    /* .wakeup                 = cst816d_wakeup, */
    .reset                  = cst816d_reset,
};

#endif // TCFG_TP_CST816D_ENABLE
