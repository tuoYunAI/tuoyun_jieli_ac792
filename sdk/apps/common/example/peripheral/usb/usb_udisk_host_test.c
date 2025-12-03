
/**************************************************************************************
* File:             usb_disk_host_test.c
*
* Author:
* Created:          09/12/23
* Description:      u盘主机读写测试
*                   1. 修改board_xxx.h:
*                   #define TCFG_PC_ENABLE                      0
                    #define TCFG_UDISK_ENABLE                   1     //U盘主机功能
                    2.有2种测试例子:
                    2.1 #define USE_USB_UDISK_HOST_TEST_DEMO    //u盘文件读写测试
                        该测试例子会在u盘上线状态下，在u盘里创建一个test文本,再读取出来
                    2.2 修改usb_storage.c
                        #define USE_USB_UDISK_HOST_SPEED_TEST_EN //u盘读写速度测试
                        该测试例子会在u盘上线状态下，对u盘随机读写，测试读写速度
**************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"
#include "fs/fs.h"

/* #define USE_USB_UDISK_HOST_TEST_DEMO    //u盘文件读写测试 */

#ifdef USE_USB_UDISK_HOST_TEST_DEMO
void filename_test(void)
{
    u8 *test_data = "filename test123"; //写入文件数据
    u8 read_buf[32];//读取数据buf
    int len;//文件大小
    char path[256] = "storage/udisk1/C/";//路径，高速USB口，开源板需要自己接出来。


    //创建并写文件

    FILE *fd = fopen(CONFIG_UDISK_ROOT_PATH"test/test.txt", "w+");

    if (fd) {
        fwrite(test_data, 1, strlen(test_data), fd);//写入文件数据
        fclose(fd);//关闭文件
        printf("write file : %s \n", read_buf);
    }

    //读文件
    fd = fopen(CONFIG_UDISK_ROOT_PATH"test/test.txt", "r");
    if (fd) {
        len = flen(fd);//获取整个文件大小
        fread(read_buf, 1, len, fd);//读取文件
        fclose(fd);//关闭文件
        printf("read file : %s \n", read_buf);
    }
}


late_initcall(filename_test);
#endif
