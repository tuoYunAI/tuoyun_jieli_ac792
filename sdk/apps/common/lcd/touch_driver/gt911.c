#include "lcd_config.h"
#include "tp_driver.h"
#include "asm/exti.h"
#include "device/gpio.h"
#include "perf_counter.h"
#include "iic.h"

#if TCFG_TP_GT911_ENABLE

#define LOG_TAG_CONST       TP
#define LOG_TAG             "[TP]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
/* #define LOG_CLI_ENABLE */
#include "system/debug.h"
#undef  _LOG_TAG
#define _LOG_TAG "[TP]GT911>"

#define print_error_and_line()   log_error("iic write err!!! line : %d", __LINE__)


#define GT911_READ_XY_MODE     1              ///< 获取GT911坐标的模式 0:轮询，1:中断
#define GT911_POLLING_XY_TIME  30             ///< 轮询获取坐标的timer时间，单位：ms
#define GT911_INT_EDGE         EDGE_NEGATIVE  ///< 中断触发边沿选择
#define GT911_SET_IIC_ADDR     0              ///< 设置GT911 IIC地址: 0-0x28/0x29, 1-0xBA/0xBB


/*
 * AC792N开发板的RGB屏的触摸板GT911，中断跳变电平较缓，会出现误触发中断，测试
 * 发现误触发与正常触发的间隔在500us左右，因此在读到status为0x00时延时1ms后重新读取，以规避。
 * 外部中断函数里则需要过滤掉irq间隔过短(500us左右)的触发。
 */
#define GT911_FILTER_FALSE_TRIG_ENABLE 1 ///< 中断读取坐标模式下，是否开启误触发滤波。0：否，1：是
#define GT911_TRIG_GAP_TIME_MIN        2 ///< 误触发滤波，最小中断间隔时间。单位：ms

// #ifndef USE_LVGL_V8_UI_DEMO
// extern int ui_touch_msg_post(struct touch_event *event);
// #endif
static void(*tp_drdy_sem_post_func)(void);
static void *iic;
static int gt911_int_id = -1;
static u16 gt911_timer_id = 0;


//I2C读写命令
//GT911 IIC地址可通过RST+INT的时序配置，我们驱动程序配置为下面的地址。
#if GT911_SET_IIC_ADDR == 0
#define GT911_WRCMD 			0X28     	///< 写命令
#define GT911_RDCMD 			0X29		///< 读命令
#else
#define GT911_WRCMD 			0XBA     	///< 写命令
#define GT911_RDCMD 			0XBB		///< 读命令
#endif

//GT911 部分寄存器定义
#define GT_CTRL_REG 			0X8040   	///< GT911控制寄存器
#define GT_CFGS_REG 			0X8047   	///< GT911配置起始地址寄存器
#define GT_CHECK_REG		 	0X80FF   	///< GT911校验和寄存器

#define GT_CMD_RD_POS	        0           ///< 读坐标状态
#define GT_CMD_SLEEP			5           ///< 关屏

#define GT_PID_REG0				0X8140   	///< GT911产品ID寄存器
#define GT_PID_REG1				0X8141   	///< GT911产品ID寄存器
#define GT_PID_REG2				0X8142   	///< GT911产品ID寄存器
#define GT_PID_REG3				0X8143   	///< GT911产品ID寄存器

#define GT_GSTID_REG 	        0X814E   	///< GT911当前检测到的触摸情况
#define GT_TP1_REG 		        0X8150  	///< 第一个触摸点数据地址 //每个点4四个地址控制GT_TP1_X_L_REG
#define GT_TP2_REG 				0X8158		///< 第二个触摸点数据地址
#define GT_TP3_REG 				0X8160		///< 第三个触摸点数据地址
#define GT_TP4_REG 				0X8168		///< 第四个触摸点数据地址
#define GT_TP5_REG 				0X8170		///< 第五个触摸点数据地址

#define GT_INT_GAP	            0X8056   	///< 坐标上报率：5ms+N。BIT[0:3]


static u8 wrGT911Reg(u32 regID, u8 regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, GT911_WRCMD)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, regDat)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static u8 rdGT911Reg(u16 regID, u8 *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, GT911_WRCMD)) {
        ret = 0;
        print_error_and_line();
        goto exit;
    }
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID >> 8)) {
        ret = 0;;
        print_error_and_line();
        goto exit;
    }
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, regID & 0xff)) {
        ret = 0;;
        print_error_and_line();
        goto exit;
    }
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, GT911_RDCMD)) {
        ret = 0;
        goto exit;
    }
    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)regDat);
exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
    return ret;
}


static void get_GT911_xy(u16 addr, u16 *x, u16 *y)
{
    u8 buf[4];
    for (u8 i = 0; i < 4; i++) {
        rdGT911Reg((addr + i), &buf[i]); ///< 读取XY坐标值
    }
    *x = buf[0] + buf[1] * 255;
    *y = buf[2] + buf[3] * 255;
}


static void GT911_test(void)
{
    u8 status = 0;
    u16 x = 0;
    u16 y = 0;
    rdGT911Reg(GT_GSTID_REG, &status);
    if (status & 0x80) { //有触摸值
        if (status > 127) { //有一个触摸点按下
            get_GT911_xy(GT_TP1_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 1]x = %d, y = %d", x, y);
        }
        if (status > 128) { //有两个触摸点按下
            get_GT911_xy(GT_TP2_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 2]x = %d, y = %d", x, y);
        }
        if (status > 129) { //有三个触摸点按下
            get_GT911_xy(GT_TP3_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 3]x = %d, y = %d", x, y);
        }
        if (status > 130) { //有四个触摸点按下
            get_GT911_xy(GT_TP4_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 4]x = %d, y = %d", x, y);
        }
        if (status > 131) { //有五个触摸点按下
            get_GT911_xy(GT_TP5_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 5]x = %d, y = %d", x, y);
        }
        status = 0;
        wrGT911Reg(GT_GSTID_REG, status); //清标志
    }
}


static void GT911_interrupt(void *arg, u32 parm)
{
#if GT911_FILTER_FALSE_TRIG_ENABLE
    static u32 last_trig_time;
    u32 current_trig_time = timer_get_ms();
    u32 trig_gap_time;
    trig_gap_time = current_trig_time - last_trig_time;
    last_trig_time = current_trig_time;
    if (trig_gap_time < GT911_TRIG_GAP_TIME_MIN) {
        return;
    }
#endif

    // putchar('I');
    tp_drdy_sem_post_func();
}


static int gt911_int_enable(const tp_platform_data_t *pd_data)
{
#if GT911_READ_XY_MODE
    if (gt911_int_id < 0) {
        gt911_int_id = exti_init(pd_data->int_pin, GT911_INT_EDGE, GT911_interrupt, NULL);
        if (gt911_int_id < 0) {
            gt911_int_id = -1;
            return -EINVAL;
        }
    }
#else
    if (gt911_timer_id == 0) {
        gt911_timer_id = sys_s_hi_timer_add(NULL, GT911_interrupt, GT911_POLLING_XY_TIME);
        if (gt911_timer_id == 0) {
            return -EINVAL;
        }
    }
#endif
    return 0;
}


static int gt911_int_disable(const tp_platform_data_t *pd_data)
{
#if GT911_READ_XY_MODE
    if (gt911_int_id >= 0) {
        exti_uninit(gt911_int_id);
        gt911_int_id = -1;
    }
#else
    if (gt911_timer_id > 0) {
        sys_s_hi_timer_del((u16)gt911_timer_id);
        gt911_timer_id = 0;
    }
#endif
    return 0;
}


static void gt911_get_xy(tp_info_t *data)
{
    u8 gt911_status;
    tp_info_t temp = {0};
    static tp_press_status_t gt911_last_nomal_status = TP_STATUS_RELEASE;

#if GT911_FILTER_FALSE_TRIG_ENABLE
    u8 gt911_false_trig_redo_flag = 0;
__gt911_get_xy_redo:
#endif

    // 读取触摸点的状态。BIT7表示有数据, bit0-3表示触摸点个数。
    rdGT911Reg(GT_GSTID_REG, &gt911_status);
    /* log_debug("status: %d", gt911_status); */

    if (gt911_status > 0x80) { //有触摸值
        get_GT911_xy(GT_TP1_REG, &temp.x, &temp.y);
        temp.status = TP_STATUS_PRESS;
        gt911_last_nomal_status = TP_STATUS_PRESS;
        wrGT911Reg(GT_GSTID_REG, 0x00); //清标志
    } else if (gt911_status == 0x80) {
        // 实测gt911上电/抬起时，会连续触发三次抬起中断，在此过滤掉
        if (gt911_last_nomal_status == TP_STATUS_PRESS) {
            get_GT911_xy(GT_TP1_REG, &temp.x, &temp.y);
            temp.status = TP_STATUS_RELEASE;
            gt911_last_nomal_status = TP_STATUS_RELEASE;
        } else {
            temp.status = TP_STATUS_NULL;
        }
        wrGT911Reg(GT_GSTID_REG, 0x00); //清标志
    } else {

#if GT911_FILTER_FALSE_TRIG_ENABLE
        if (gt911_false_trig_redo_flag == 0) {
            gt911_false_trig_redo_flag = 1;
            delay_ms(1);
            log_debug("redo get xy");
            goto __gt911_get_xy_redo;
        }
#endif

        temp.status = TP_STATUS_NULL;
    }

    data->x = temp.x;
    data->y = temp.y;
    data->status = temp.status;
}


static void gt911_set_addr(int rst_pin, int int_pin)
{
#if GT911_SET_IIC_ADDR == 0
    // 设置触摸IC的IIC地址为0x28 0x29
    gpio_direction_output(int_pin, 0);
    gpio_direction_output(rst_pin, 0);
    os_time_dly(1);
    gpio_direction_output(int_pin, 1);
    os_time_dly(1);
    gpio_direction_output(rst_pin, 1);
    os_time_dly(1);
    gpio_direction_output(int_pin, 0);
    os_time_dly(10);
#else
    // 设置触摸IC的IIC地址为0xBA 0xBB
    gpio_set_mode(int_pin, GPIO_OUTPUT_LOW);
    gpio_set_mode(rst_pin, GPIO_OUTPUT_LOW);
    os_time_dly(1);
    gpio_direction_output(rst_pin, 1);
    os_time_dly(10);
#endif
}


static int gt911_check_pid(void)
{
    u8 temp = 0;

    rdGT911Reg(GT_PID_REG0, &temp);
    if (temp != '9') {
        return -ENODEV;
    }
    rdGT911Reg(GT_PID_REG1, &temp);
    if (temp != '1') {
        return -ENODEV;
    }
    rdGT911Reg(GT_PID_REG2, &temp);
    if (temp != '1') {
        return -ENODEV;
    }

    return 0;
}


static int gt911_init(const tp_platform_data_t *pd_data, void(*post_func)(void))
{
    log_info(">>>>>init start !!!");
    if ((pd_data->rst_pin != -1) && (pd_data->int_pin != -1)) {
        gt911_set_addr(pd_data->rst_pin, pd_data->int_pin);
    } else {
        log_error("The reset pin and interrupt pin of the gt911 must be configured!");
        return -EINVAL;
    }

    iic = dev_open(pd_data->interface, NULL);
    if (!iic) {
        log_error("open iic error!");
        return -EINVAL;
    }

    if (gt911_check_pid()) {
        log_error(">>>>>ID mismatch!!!");
        dev_close(iic);
        return -ENODEV;
    }

    tp_drdy_sem_post_func = post_func;

    if (gt911_int_enable(pd_data)) {
        dev_close(iic);
        return -EINVAL;
    }

    return 0;
}


static int gt911_deinit(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>deinit!!!");

    gt911_int_disable(pd_data);

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


static int gt911_sleep(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>sleep!!!");
    // 先拉低INT电平，然后发送sleep命令，让gt911进入sleep mode
    gpio_set_die(pd_data->int_pin, 0);
    gpio_set_mode(pd_data->int_pin, GPIO_OUTPUT_LOW);
    os_time_dly(1);
    wrGT911Reg(GT_CTRL_REG, (u8)GT_CMD_SLEEP);

    return 0;
}


static int gt911_wakeup(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>wakeup!!!");
    // 规格书唤醒时序：拉高INT电平2~5ms，唤醒gt911并进入normal mode
    gpio_set_die(pd_data->int_pin, 0);
    gpio_set_mode(pd_data->int_pin, GPIO_OUTPUT_HIGH);
    os_time_dly(1);
    gpio_direction_output(pd_data->int_pin, 0); ///< 但实测需要再次拉低INT才能进入normal模式。
    os_time_dly(1);
#if GT911_INT_EDGE == EDGE_NEGATIVE
    gpio_set_mode(pd_data->int_pin, GPIO_INPUT_PULL_UP_10K);
#else
    gpio_set_mode(pd_data->int_pin, GPIO_INPUT_PULL_DOWN_10K);
#endif

    return 0;
}


static void gt911_reset(const tp_platform_data_t *pd_data)
{
    log_info(">>>>>reset!!!");
    gpio_direction_output(pd_data->rst_pin, 0);
    os_time_dly(1);
    gpio_direction_output(pd_data->rst_pin, 1);
    os_time_dly(10);
}


REGISTER_TP_DRIVER(gt911_driver) = {
    .ic_name                = "GT911",
    .init                   = gt911_init,
    .get_xy_and_status      = gt911_get_xy,
    .check_online           = gt911_check_pid,
    .deinit                 = gt911_deinit,
    .sleep                  = gt911_sleep,
    .wakeup                 = gt911_wakeup,
    .reset                  = gt911_reset,
};

#endif

