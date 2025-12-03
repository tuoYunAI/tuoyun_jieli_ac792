#include "lcd_config.h"
#include "tp_driver.h"
#include "asm/exti.h"
#include "device/gpio.h"
#include "perf_counter.h"
#include "iic.h"

#if TCFG_TP_CST3240_ENABLE

#define LOG_TAG_CONST       TP
#define LOG_TAG             "[TP]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"
#undef  _LOG_TAG
#define _LOG_TAG "[TP]CST3240>"

#define print_error_and_line()   log_error("iic write err!!! line : %d", __LINE__)


#define CST3240_READ_XY_MODE       1              ///< 获取CST3240坐标的模式 0:轮询，1:中断
#define CST3240_POLLING_XY_TIME    30             ///< 轮询获取坐标的timer时间，单位：ms
#define CST3240_INT_EDGE           EDGE_NEGATIVE  ///< 中断触发边沿选择

/* #ifndef USE_LVGL_UI_DEMO */
/* extern int ui_touch_msg_post(struct touch_event *event); */
/* #endif */

static void(*tp_drdy_sem_post_func)(void);
static void *iic;
static volatile int cst3240_int_id = -1;
static u16 cst3240_timer_id = 0;


#define CST3240_ADDR 		                0XB4   // CST3240 IIC地址

#define CST_FIG_INFO_SIZE                   5      //每个触摸点数据大小
#define CST_REG_1ST_FIG                     0xD000 //第一个触摸点数据地址
#define CST_REG_2ST_FIG                     0xD007 //第二个触摸点数据地址
#define CST_REG_3ST_FIG                     0xD00C //第三个触摸点数据地址
#define CST_REG_4ST_FIG                     0xD011 //第四个触摸点数据地址
#define CST_REG_5ST_FIG                     0xD016 //第五个触摸点数据地址

#define CST_REG_FIG_NUM                     0xD005 //触摸点数量地址

#define CST_ENTER_NORMAL_MODE               0xD109
#define CST_STOP_READ                       0xD000AB

#define CST_STATUS_PRESS                    0x06
#define CST_STATUS_RELEASE                  0x00

#define CST_CMD_NORMAL_MODE                 0xD109
#define CST_CMD_INFO_MODE                   0xD101

#define CST_REG_IC_TYPE                     0xD204 //芯片类型，固件项目ID


static int cst3240_stop_read(void)
{
    int ret = 0;
    u8 buf[] = {0xD0, 0x00, 0xAB}; ///< CST同步命令，表示完成此次读取

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST3240_ADDR)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    int i = 0;
    for (; i < (sizeof(buf) - 1); i++) {
        if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)buf[i])) {
            ret = -EIO;
            print_error_and_line();
            goto exit;
        }
        /* delay(100); */
    }

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, (u32)buf[i])) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}

static int cst3240_send_cmd(u16 cmd)
{
    int ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST3240_ADDR)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay_us(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)((cmd >> 8) & 0xff))) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay_us(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)(cmd & 0xff))) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay_us(100); */

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static int cst3240_reg_buf_write(u16 reg, u8 *data, u8 len)
{
    int ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST3240_ADDR)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)((reg >> 8) & 0xff))) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)(reg & 0xff))) {
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


static int cst3240_write_reg(u16 reg, u8 data)
{
    return cst3240_reg_buf_write(reg, &data, 1);
}


static int cst3240_buf_read(u16 reg, u8 *buf, u8 len)
{
    u8 ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST3240_ADDR)) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)((reg >> 8) & 0xff))) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)(reg & 0xff))) {
        ret = -EIO;
        print_error_and_line();
        goto exit;
    }
    /* delay(100); */

    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, CST3240_ADDR + 1)) {
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


static int cst3240_read_reg(u16 reg, u8 *data)
{
    int ret = cst3240_buf_read(reg, data, 1);
    cst3240_stop_read();
    return ret;
}


static int cst3240_read_info(u16 reg, u32 *data)
{
    int ret = cst3240_buf_read(reg, (u8 *)data, 4);
    cst3240_stop_read();
    return ret;
}


static int cst3240_get_fig1_xy(u16 *x, u16 *y)
{
    u8 buf[CST_FIG_INFO_SIZE];
    int ret = cst3240_buf_read(CST_REG_1ST_FIG, buf, CST_FIG_INFO_SIZE);
    cst3240_stop_read();

    *x = (buf[1] << 4) | ((buf[3] >> 4) & 0x0F);
    *y = (buf[2] << 4) | (buf[3] & 0x0F);

    return ret;
}


static void cst3240_interrupt(void *arg, u32 parm)
{
    tp_drdy_sem_post_func();
}


static int cst3240_int_enable(const tp_platform_data_t *pd_data)
{
#if CST3240_READ_XY_MODE
    if (cst3240_int_id < 0) {
        cst3240_int_id = exti_init(pd_data->int_pin, CST3240_INT_EDGE, cst3240_interrupt, NULL);
        if (cst3240_int_id < 0) {
            cst3240_int_id = -1;
            return -EINVAL;
        }
    }
#else
    if (cst3240_timer_id == 0) {
        cst3240_timer_id = sys_s_hi_timer_add(NULL, cst3240_interrupt, CST3240_POLLING_XY_TIME);
        if (cst3240_timer_id == 0) {
            return -EINVAL;
        }
    }
#endif
    return 0;
}


static int cst3240_int_disable(const tp_platform_data_t *pd_data)
{
#if CST3240_READ_XY_MODE
    if (cst3240_int_id >= 0) {
        exti_uninit(cst3240_int_id);
        cst3240_int_id = -1;
    }
#else
    if (cst3240_timer_id > 0) {
        sys_s_hi_timer_del((u16)cst3240_timer_id);
        cst3240_timer_id = 0;
    }
#endif
    return 0;
}


static void cst3240_get_xy(tp_info_t *data)
{
    u8 touch_status;
    tp_info_t temp = {0};
    static tp_press_status_t cst3240_last_nomal_status = TP_STATUS_RELEASE;
    cst3240_read_reg(CST_REG_1ST_FIG, &touch_status);//读取触摸点1的状态
    cst3240_get_fig1_xy(&temp.x, &temp.y);
    if ((touch_status & 0x0f) == CST_STATUS_PRESS) {
        temp.status = TP_STATUS_PRESS;
        cst3240_last_nomal_status = TP_STATUS_PRESS;
    } else if ((touch_status & 0x0f) == CST_STATUS_RELEASE) {
        if (cst3240_last_nomal_status == TP_STATUS_PRESS) {
            temp.status = TP_STATUS_RELEASE;
            cst3240_last_nomal_status = TP_STATUS_RELEASE;
        } else {
            temp.status = TP_STATUS_NULL;
            log_debug("touch status error");
        }
    } else {
        temp.status = TP_STATUS_NULL;
        log_debug("touch status error");
    }

    data->x = temp.x;
    data->y = temp.y;
    data->status = temp.status;
}


static int cst3240_check_pid(void)
{
    u32 touch_id;
    cst3240_send_cmd(CST_CMD_INFO_MODE);
    cst3240_send_cmd(CST_CMD_INFO_MODE);
    cst3240_send_cmd(CST_CMD_INFO_MODE);///<应用手册：多次发送命令，确保发送成功
    cst3240_read_info(CST_REG_IC_TYPE, &touch_id);
    cst3240_send_cmd(CST_CMD_NORMAL_MODE);
    touch_id >>= 16;
    log_debug("get touch id : 0x%x", touch_id);
    if (touch_id != 0x3240) {
        return -ENODEV;
    }
    return 0;
}


static int cst3240_init(const tp_platform_data_t *pd_data, void(*post_func)(void))
{
    log_info(">>>>>init start !!!");

    iic = dev_open(pd_data->interface, NULL);
    if (!iic) {
        log_error("open iic error!");
        return -EINVAL;
    }

    if (cst3240_check_pid()) {
        log_error(">>>>>ID mismatch!!!");
        dev_close(iic);
        return -ENODEV;
    }

    tp_drdy_sem_post_func = post_func;

    if (cst3240_int_enable(pd_data)) {
        dev_close(iic);
        return -EINVAL;
    }

    return 0;
}


static int cst3240_deinit(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>deinit!!!");

    cst3240_int_disable(pd_data);

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


static int cst3240_sleep(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>sleep!!!");
    //TODO

    return 0;
}


static int cst3240_wakeup(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>wakeup!!!");
    //TODO

    return 0;
}


static void cst3240_reset(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>reset!!!");
    gpio_direction_output(pd_data->rst_pin, 0);
    os_time_dly(1);
    gpio_direction_output(pd_data->rst_pin, 1);
    os_time_dly(60);
}


REGISTER_TP_DRIVER(cst3240_driver) = {
    .ic_name                = "CST3240",
    .init                   = cst3240_init,
    .get_xy_and_status      = cst3240_get_xy,
    .check_online           = cst3240_check_pid,
    .deinit                 = cst3240_deinit,
    /* .sleep                  = cst3240_sleep, */
    /* .wakeup                 = cst3240_wakeup, */
    .reset                  = cst3240_reset,
};

#endif

