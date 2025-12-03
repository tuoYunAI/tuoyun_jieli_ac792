
/**************************************************************************************
* File:             usb_hid_slave_test.c
*
* Author:
* Created:          09/12/23
* Description:      usb hid从机读写测试，搭配杰理上位机使用(测试脚本是hid.lua)
*                   读写测试原理：
*                   上位机枚举hid设备，随机生成一组序列下发到设备，
*                   设备收到后,发送回给主机，主机进行数据对比，数据不匹配时退出测试.
*                   开启方法：
*                   1. board_xxx.h:
                    #define TCFG_PC_ENABLE  1 开启从机功能
                    #define USB_DEVICE_CLASS_CONFIG             (CUSTOM_HID_CLASS)
                    2. task_pc.c :
                    #define TCFG_USB_CUSTOM_HID_TEST_EN  //hid读写测试(搭配上位机)
*************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"

/* #ifdef USE_USB_HID_SLAVE_TEST_DEMO */
#if 0

static void hid_send_test(void *priv)
{
    int ret = 0;
    char buf[] = "{123!@#$%^&*()_+ABC}\r\n";
#if USB_HID_KEYBOARD_ENABLE
    extern int hid_keyboard_send(const u8 * data, u32 len);
    ret = hid_keyboard_send((u8 *)buf, strlen(buf));
    printf("hid_keyboard, ret = %d\n", ret);
#elif USB_HID_POS_ENABLE
    int hid_pos_send(u8 * data, u32 len, u8 force);
    ret = hid_pos_send((u8 *)buf, strlen(buf), 0);
    printf("hid_pos, ret = %d\n", ret);
#endif
}

void usb_hid_slave_test(void)
{
    sys_timer_add(NULL, hid_send_test, 1000);
}

late_initcall(usb_hid_slave_test);

#endif
