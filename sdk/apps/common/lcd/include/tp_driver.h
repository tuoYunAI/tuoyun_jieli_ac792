/**
 * @file tp_driver.h
 * @brief touch panel driver framework
 * @version
 * @date 2024-07-11
 */

#ifndef __TP_DRIVER_H__
#define __TP_DRIVER_H__
#include "system/includes.h"
#include "touch_event.h"

/**
 * \name TP press status
 * \{
 */
typedef enum {
    TP_STATUS_RELEASE,          ///< 释放状态
    TP_STATUS_PRESS,            ///< 按压状态
    TP_STATUS_NULL,             ///< 获取状态出错
} tp_press_status_t;
/* \} name */


/**
 * \name TP platform data
 * \{
 */
typedef struct {
    char *interface;               ///< 通讯接口。比如"iic0"
    int int_pin;                   ///< 外部中断脚
    int rst_pin;                   ///< 复位脚
    u8 x_mirror;                   ///< x坐标是否镜像。0:否, 1:是。
    u8 y_mirror;                   ///< y坐标是否镜像。0:否, 1:是。
    u8 swap_x_y;                   ///< 是否对调x和y的坐标值。0:否, 1:是。
} tp_platform_data_t;
/* \} name */


/**
 * \name Coordinates and status of TP
 * \{
 */
typedef struct {
    u16 x;                    ///< X坐标
    u16 y;                    ///< Y坐标
    u8 status;                ///< TP按压状态。0:抬起, 1:按下
    touch_action_t act;       ///< 触摸动作event
} tp_info_t;
/* \} name */


/**
 * \name TP device drive interface
 * \{
 */
typedef struct {
    char *ic_name;                                                  ///< 使用的触摸IC型号
    int (*init)(const tp_platform_data_t *pd_data, void(*post_func)(void));    ///< 触摸初始化
    void (*get_xy_and_status)(tp_info_t *);                         ///< 获取当前触摸坐标
    int (*check_online)(void);                                      ///< 检查的触摸是否在线
    int (*deinit)(const tp_platform_data_t *pd_data);               ///< 触摸反初始化
    int (*sleep)(const tp_platform_data_t *pd_data);                ///< 进入睡眠
    int (*wakeup)(const tp_platform_data_t *pd_data);               ///< 唤醒
    void (*reset)(const tp_platform_data_t *pd_data);               ///< 复位
} tp_driver_t;
/* \} name */


/// \cond DO_NOT_DOCUMENT
#define REGISTER_TP_DRIVER(name) \
	static const tp_driver_t name SEC_USED(.tp_driver)

#define list_for_each_tp_driver(p) \
	for (p = tp_driver_begin; p < tp_driver_end; p++)


#define TP_PLATFORM_DATA_BEGIN(tp_pd_data) \
    static const tp_platform_data_t tp_pd_data = { \

#define TP_PLATFORM_DATA_END() \
    };


extern const tp_driver_t tp_driver_begin[];
extern const tp_driver_t tp_driver_end[];
/// \endcond

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_get_platform_data 获取board.c里的touch panel的platform data结构体地址。
 *
 * @return  platform data结构体地址。
 */
/* ------------------------------------------------------------------------------------*/
const tp_platform_data_t *tp_get_platform_data(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_init 初始化touch panel驱动。该接口会查找注册的具体型号触摸驱动ic的驱动，并对其
 *                进行初始化。创建tp_driver任务来获取触摸坐标&状态，并将数据发送给UI。
 *
 * @return 0-init成功; 小于0-init失败。
 */
/* ------------------------------------------------------------------------------------*/
int tp_init(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_deinit 卸载touch panel驱动。
 *
 * @return 0-卸载成功; 小于0-卸载失败。
 */
/* ------------------------------------------------------------------------------------*/
int tp_deinit(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_check_online 查询touch panel设备是否在线。
 *
 * @return 0-online; 小于0-offline。
 */
/* ------------------------------------------------------------------------------------*/
int tp_check_online(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_reset 复位touch panel。
 *
 * @return 0-复位成功; 小于0-复位失败。
 */
/* ------------------------------------------------------------------------------------*/
int tp_reset(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_sleep touch panel 进入休眠。
 *
 * @return 0-进入休眠成功; 小于0-进入休眠失败。
 */
/* ------------------------------------------------------------------------------------*/
int tp_sleep(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_wakeup 唤醒休眠的touch panel。
 *
 * @return 0-唤醒成功; 小于0-唤醒失败。
 */
/* ------------------------------------------------------------------------------------*/
int tp_wakeup(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_enable TP使能发送数据给系统和UI
 */
/* ------------------------------------------------------------------------------------*/
void tp_enable(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_disable TP关闭发送数据给系统和UI
 */
/* ------------------------------------------------------------------------------------*/
void tp_disable(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_post_xy_ui_enable 使能发送坐标数据给UI
 */
/* ------------------------------------------------------------------------------------*/
void tp_post_xy_ui_enable(void);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief tp_post_xy_ui_disable 关闭发送坐标数据给UI
 */
/* ------------------------------------------------------------------------------------*/
void tp_post_xy_ui_disable(void);

#endif
