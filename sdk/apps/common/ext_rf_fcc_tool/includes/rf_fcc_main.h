#ifndef __RF_FCC_MAIN_H_
#define __RF_FCC_MAIN_H_

#include "app_config.h"
#include "list.h"
#include "asm/gpio.h"
#include "asm/crc16.h"
#include "asm/includes.h"
#include "os/os_api.h"
#include "wireless_ext/rtl_mp.h"
#include "device/uart.h"
#include "device/device.h"
#include "wireless_ext/wifi_dev.h"
#include "wireless_ext/wifi_connect.h"

#include "datatype.h"
#include "wifi_moudle.h"
#include "communication.h"

#define IO_TRIGGER_MODE      (0) //"IO检测"方式触发进入RF_FCC测试模式
#define USER_DEF_MODE        (1) //"用户定义"方式触发进入RF_FCC测试模式
#define AUTO_STARTUP_MODE    (2) //上电进入RF_FCC测试模式


//*********************************************************************************//
//                                  FCC测试相关配置                                //
//*********************************************************************************//
#ifdef TCFG_EXT_RF_FCC_TEST_ENABLE

#define CONFIG_RF_FCC_TRIGGER_MODE   AUTO_STARTUP_MODE //RF_FCC触发方式配置

#define CONFIG_RF_FCC_TRIGGER_IO_PORT   IO_PORTC_01    //RF_FCC"IO检测"IO配置
#define CONFIG_RF_FCC_TRIGGER_IO_STATE  (1)            //RF_FCC"IO检测"IO的状态配置，0:低电平触发，1:高电平触发
#define CONFIG_RF_FCC_TRIGGER_IO_CNT    (10)           //RF_FCC"IO检测"IO的检测次数


#define MAX_DATA_SIZE	(64)
#define	MAX_PARAMS_SIZE	(MAX_DATA_SIZE - sizeof(struct fcc_data) + 1)

#endif //TCFG_EXT_RF_FCC_TEST_ENABLE



//*********************************************************************************//
//                                  FCC相关接口说明                                //
//*********************************************************************************//


/**
 * @brief rf_fcc_tool_init，RF_FCC测试初始化
 *
 * @param
 *
 * @note
 */
void rf_fcc_tool_init(void);


/**
 * @brief fcc_get_uart，获取RF_FCC测试的上位机通信串口号
 *
 * @param
 *
 * @note 可在外部定义同名函数，修改返回的通信串口号("uart0"/"uart1"/"uart2")
 */
__attribute__((weak)) const char *fcc_get_uart(void);


/**
 * @brief fcc_enter_user_def，开机检测是否进入RF_FCC测试
 *
 * @param
 *
 * @note 可在外部定义同名函数，修改触发方式
 */
__attribute__((weak))u8 fcc_enter_user_def(void);


/**
 * @brief fcc_res_handler，RF_FCC测试结果处理
 *
 * @param res == true, 测试PASS
 *        res == false, 测试FAIL
 *
 * @note 可在外部定义同名函数，根据测试结果添加自定义操作
 */
__attribute__((weak))void fcc_res_handler(u8 res);


#endif


