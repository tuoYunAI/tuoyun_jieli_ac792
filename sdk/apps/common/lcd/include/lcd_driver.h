#ifndef __LCD_DRIVER_H__
#define __LCD_DRIVER_H__

#include "asm/dpi.h"
#include "asm/pap.h"
#include "asm/dsi.h"

#include "system/includes.h"
#include "device/device.h"
#include "os/os_api.h"

#include "device/iic.h"
#include "touch_event.h"

#define   BLACK     0x0000       //黑色
#define   NAVY      0x000F       //深蓝色
#define   DGREEN    0x03E0       //深绿色
#define   DCYAN     0x03EF       //深青色
#define   MAROON    0x7800       //深红色
#define   PURPLE    0x780F       //紫色
#define   OLIVE     0x7BE0       //橄榄绿
#define   LGRAY     0xC618       //灰白色
#define   DGRAY     0x7BEF       //深灰色
#define   BLUE      0x001F       //蓝色
#define   GREEN     0x07E0       //绿色
#define   CYAN      0x07FF       //青色
#define   RED       0xF800       //红色
#define   MAGENTA   0xF81F       //品红
#define   YELLOW    0xFFE0       //黄色
#define   WHITE     0xFFFF       //白色

#define IOCTL_LCD_RGB_SET_ISR_CB                _IOW('F', 1, sizeof(void *(*)(void)))
#define IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH       _IOW('F', 2, sizeof(int))
#define IOCTL_LCD_RGB_START_DISPLAY             _IOW('F', 3, sizeof(void *))
#define IOCTL_LCD_RGB_GET_LCD_HANDLE            _IOR('F', 4, sizeof(struct lcd_dev_drive **))
#define IOCTL_LCD_RGB_SET_LINE_ISR_CB           _IOW('F', 5, sizeof(int (*)(int)))
#define IOCTL_LCD_RGB_WAIT_LINE_FINISH          _IOW('F', 6, sizeof(int))

struct te_mode_ctrl {
    u8 te_mode_en;              ///< TE功能使能
    u8 edge;                    ///< TE触发边沿
    int gpio;                   ///< TE脚IO
};

struct lcd_board_cfg {
    char *lcd_name;                ///< 屏驱配置名(用于与屏驱配置匹配)
    struct {
        u8 backlight_value;              ///< 背光控制使能电平
        int backlight;                   ///< 背光控制IO
        int lcd_reset;                   ///< 复位脚
        int lcd_cs;                      ///< 片选脚
        int lcd_rs;                      ///< 屏幕命令/数据脚
    } lcd_io;
    struct te_mode_ctrl te_mode;         ///< TE功能配置(仅MCU/SPI类型的屏需要)
    char *spi_lcd_interface;             ///< spi屏的推屏接口(仅SPI屏需要)
};

struct lcd_platform_data {
    u8 cfg_num;                          ///< 板级配置数量
    struct lcd_board_cfg *config_ptr;    ///< 板级配置地址
};

#define LCD_PLATFORM_DATA_BEGIN(data) \
static struct lcd_board_cfg data[] = { \
    {

#define LCD_PLATFORM_DATA_ADD() \
    },\
    {

#define LCD_PLATFORM_DATA_END() \
    },\
};


struct esd_deal {
    u8  en;
    u16 count;
    u16 interval;
    void (*esd_check_isr)(struct esd_deal *esd);
    int timer;
};

struct spi_dev {
    struct basic_info info;
    enum LCD_ENDIAN data_out_endian;
};

union lcd_dev_info {
    struct imd_dev imd;
    struct mipi_dev mipi;
    struct spi_dev spi;
};

#define REGISTER_IMD_DEVICE_BEGIN(info) \
    static union lcd_dev_info info SEC_USED(.lcd_device) = { \
        .imd
#define REGISTER_IMD_DEVICE_END() \
    };

#define REGISTER_MIPI_DEVICE_BEGIN(info) \
    static union lcd_dev_info info SEC_USED(.lcd_device) = { \
        .mipi
#define REGISTER_MIPI_DEVICE_END() \
    };

#define REGISTER_LCD_SPI_DEVICE_BEGIN(info) \
    static union lcd_dev_info info SEC_USED(.lcd_device) = { \
        .spi
#define REGISTER_LCD_SPI_DEVICE_END() \
    };

struct lcd_dev_drive {
    const char *logo;
    unsigned int id;
    enum LCD_IF type;
    int (*init)(struct lcd_board_cfg *bd_cfg);
    int (*draw)(void *data);
    int (*uninit)(struct lcd_board_cfg *bd_cfg);
    void (*bl_ctrl)(struct lcd_board_cfg *bd_cfg, u8 onoff);
    int (*check)(struct lcd_board_cfg *bd_cfg);
    int (*send_init_code)(struct lcd_board_cfg *bd_cfg);
    struct esd_deal esd;
    union lcd_dev_info *dev;
};

#define REGISTER_LCD_DEVICE_DRIVE(dev) \
	static const struct lcd_dev_drive dev##_drive SEC_USED(.lcd_device_drive)

#define list_for_each_lcd_device_drive(p) \
	for (p=lcd_device_drive_begin; p < lcd_device_drive_end; p++)

#define lcd_delay(x) msleep(x)

extern struct lcd_dev_drive lcd_device_drive_begin[];
extern struct lcd_dev_drive lcd_device_drive_end[];
extern const struct device_operations lcd_dev_ops;

void lcd_cs_pinstate(u8 lcd_id, u8 state);
void lcd_rs_pinstate(u8 lcd_id, u8 state);
void lcd_rst_pinstate(u8 lcd_id, u8 state);
void ReadDAT(u8 lcd_id, u8 cmd, u8 *buf, u8 len);
void WriteCOM(u8 lcd_id, u8 cmd);
void WriteDAT_8(u8 lcd_id, u8 dat);
void WriteDAT_one_page(u8 lcd_id, u8 *dat, int len);
u16 lcd_get_rotate(u8 lcd_id);
int lcd_touch_width_height_rotate(u8 lcd_id, u16 *w, u16 *h);
int lcd_touch_xy_coord_rotate(u8 lcd_id, u16 *x, u16 *y, u8 status);

#endif

