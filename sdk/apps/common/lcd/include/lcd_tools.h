#ifndef LCD_TOOLS_H_INCLUDED
#define LCD_TOOLS_H_INCLUDED

#include "app_config.h"

#ifdef CONFIG_VIDEO_IQ_TOOLS_ENABLE


#include "asm/dmm.h"
#include "lcd_config.h"
#include "system/includes.h"
#include "json_c/json.h"
#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "usb_std_class_def.h"
#include "ioctl_cmds.h"
#include "syscfg_id.h"
#include "gpio.h"
#include "fs/fs.h"
#include "asm/gpcrc.h"
#include "video_server.h"
#include "video_dec_server.h"
#include "event/device_event.h"

#define LCD_TOOLS_DEBUG_EN   1

#define 	DATA_BUFFER_ALIGN	(4)
#define 	RECV_BUFFER_SIZE	(sizeof(struct display_head) + sizeof(u16) + 1024 * 100)
#define 	RESP_BUFFER_SIZE	(sizeof(struct display_head) + sizeof(u16) + 1024 * 3)

#if LCD_TOOLS_DEBUG_EN
#define     lcd_tools_info(x, ...)     printf("[LCD_TOOLS][INFO] " x " ", ## __VA_ARGS__)
#define     lcd_tools_err(x, ...)      printf("[LCD_TOOLS][ERR] " x " ", ## __VA_ARGS__)
#else
#define     lcd_tools_info(x, ...)
#define     lcd_tools_err(x, ...)
#endif

enum imd_tool_cmd {
    DATA_TYPE_OPCODE    = 0x00,
    IMD_CUS_SET_VERSION = 0x36,
    IMD_CUS_GET_VERSION = 0x37,
    IMD_CUS_CONNECT_DEV = 0x38, //连接设备
    IMD_CUS_DISCONNECT_DEV = 0x39,//断开设备
    IMD_CUS_SHOW_JPG_IMAGE = 0x3A, //显示jpeg静态图
    IMD_CUS_SHOW_VIDEO_IMAGE = 0x3B,//显示设备摄像头

    IMD_CUS_GET_DEFAULT_PARAM = 0x20, // 获取设备默认参数
    IMD_CUS_SET_ALL_PARAM = 0x21, // 设置所有参数
    IMD_CUS_SET_RBS = 0x2B, // 设置r分量和b分量对调
    IMD_CUS_SET_CCM = 0x2C, // 颜色矩阵
    IMD_CUS_SET_GAMMA = 0x2D, // 伽马曲线
    IMD_CUS_SET_R_GAIN = 0x2E, // r_gain
    IMD_CUS_SET_G_GAIN = 0x2F, // g_gain
    IMD_CUS_SET_B_GAIN = 0x30, // b_gain
    IMD_CUS_SET_BRIGHT = 0x31, // 亮度
    IMD_CUS_SET_CONTRAST = 0x32, // 对比度
    IMD_CUS_SET_CONTRAST_MEAN = 0x33, // 对比度均值
    IMD_CUS_SET_SATURATION = 0x34, // 饱和度
    IMD_CUS_SET_ANGLE = 0x35, // 色相
};

#pragma pack(1)
struct display_head {
    u8 mark[2];
    u8 idx;
    u8 type;
    u32 len;
    u8 reserved[2];
};
#pragma pack ()

struct display_msg {
    u16 len;
    u8 *data;
    struct display_msg *self;
    struct list_head entry;
};
u8 lcd_tools_main(void);


#endif // LCD_TOOL_ENABLE
#endif // LCD_TOOLS_H_INCLUDED
