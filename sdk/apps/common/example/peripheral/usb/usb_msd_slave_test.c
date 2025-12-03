
/**************************************************************************************
* File:             usb_msd_slave_test.c
*
* Author:
* Created:          09/12/23
* Description:      usb msd从机读写测试，搭配杰理上位机使用(测试脚本是msd.lua)
*                   读写测试原理：
*                   上位机枚举msd设备，随机生成一组序列下发到设备，
*                   设备收到后,发送回给主机，主机进行数据对比，数据不匹配时退出测试.
*                   开启方法：
*                   1. board_xxx.h:
                    #define TCFG_PC_ENABLE  1 开启从机功能
                    #define USB_DEVICE_CLASS_CONFIG             (MASSSTORAGE_CLASS)
                    #define TCFG_VIR_UDISK_ENABLE               1     //使能虚拟U盘
                    #define TCFG_SD0_ENABLE                     0
                    2. msd_upgrade.c :
                    #define TCFG_MSD_BULK_TEST_EN        1  //msd读写测试(搭配上位机)
*************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"

