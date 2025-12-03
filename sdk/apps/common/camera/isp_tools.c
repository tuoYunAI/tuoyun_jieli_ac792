
#include "system/includes.h"
#include "usb/device/msd.h"
#include "usb/scsi.h"
#include "usb_config.h"
#include "app_config.h"
#include "cpu.h"
#include "asm/debug.h"
#include "video/isp.h"

#if TCFG_USB_SLAVE_MSD_ENABLE

#define PACKET_LEN  128

#define ISPT_PACKET_PID     0x4A

#define ISPT_READ           0xF0
#define ISPT_WRITE          0xF1

struct loader_arg_list {
    void *sram_to_usb;
    void *usb_to_sram;
    void **pHook;
    u32 dlmode;
    void **hid_hook;
};

typedef void (*CALL_LOADER)(struct loader_arg_list *arg_list);


enum {
    ISP_CUS_GET_CAM_READY = 0x07,  //获取摄像头是否在线状态
//efuse调试
    ISP_CUS_GET_EFUSE_PARAM = 0xd0, //获取小机的efuse配置,返回struct efuse_cfg结构体
    ISP_CUS_SET_EFUSE_PARAM = 0xdf, //将efuse参数设置生效, usb会重新上线，此时需要主机重新枚举usb设备
//eeprom 烧写
    ISP_CUS_WRITE_EEEPROM = 0xe0, //将eeprom烧写bin文件通过改命令下发
    ISP_CUS_WRITE_MEM = 0xe0,    //(ID+LEN+ADDR+DATA)
    ISP_CUS_RUN_ADDR = 0xe1,     //(ID+LEN+ADDR)
};

struct ispt_packet_header {
    u8 pid;
    u8 syntax;
    u8 number;
    u8 last_number;
    u16 version;
    u16 reserved0;
    u32 reserved1;
    u16 length;
    u16 crc;
};

struct isp_tool_fh {
    u8 inited;
    u32 usb_id;
    u8 *buf;
    u32 buf_size;
    u8 *packet_buf;
    u8 *isp_buf;
    int offset;
    int pkt_id;
    //void *dev;
    void *(*get_video_device)(void);
};

static struct isp_tool_fh isp_tool = {0};

#define __this   (&isp_tool)

static u8 isp_tool_buf[2 * 1024] ALIGNE(4);

static u32(* hook_usb_ispt)(struct isp_tool_fh *tool, void *ptr, u32 len);

static u32 _Msd_Usb2Mcu(u8 *buf, u32 len)
{
    struct usb_device_t *usb_device = usb_id2device(__this->usb_id);
    return msd_usb2mcu(usb_device, buf, len);
}
static u32 _Msd_Mcu2Usb(u8 *buf, u32 len)
{
    struct usb_device_t *usb_device = usb_id2device(__this->usb_id);
    return msd_mcu2usb(usb_device, buf, len);
}


static void ispt_write_ram(u8 *buf, u32 addr, u32 len)
{
    memcpy((u8 *)addr, buf, len);
}

static void ispt_run_addr(u32 addr)
{
    struct loader_arg_list arg_list = {0};
    arg_list.sram_to_usb = _Msd_Mcu2Usb;
    arg_list.usb_to_sram = _Msd_Usb2Mcu;
    arg_list.pHook = (void **)&hook_usb_ispt;
    arg_list.dlmode = 0;
    arg_list.hid_hook = NULL;
    CALL_LOADER ispt_loader = (CALL_LOADER)addr;

    printf_buf((void *)ispt_loader, 16);
    ispt_loader(&arg_list);
}

void isp_tool_init(usb_dev usb_id, struct ispt_param *cfg)
{
    memset(__this, 0, sizeof(struct isp_tool_fh));
    if (cfg->buf && cfg->buf_size) {
        __this->buf = cfg->buf;
        __this->buf_size = cfg->buf_size;
    } else {
        __this->buf = isp_tool_buf;
        __this->buf_size = sizeof(isp_tool_buf);
    }
    __this->usb_id = usb_id;
    __this->isp_buf = __this->buf;
    __this->packet_buf = __this->buf + (__this->buf_size / 2);
    __this->get_video_device = cfg->get_video_device;
    __this->inited = 1;
}

static int ispt_parse_write_config(struct isp_tool_fh *tool, u8 *cmd, u32 cmd_len)
{
    u8 channel = isp0_get_id();
    if (channel == 0xff || __this->inited == 0) {
        return -EINVAL;
    }
    struct ispt_packet_header packet_header;
    int parse_len = 0;
    int data_len;
    u8 *data;
    u8 *tool_buf;
    void *dev;
    u8 byte;
    u32 remain_len;

    memcpy(&data_len, cmd + 6, 2);
    data_len = be16_to_cpu(data_len);
    remain_len = data_len;
    if (data_len > 1024) {
        data_len = 1024;
    }
    _Msd_Usb2Mcu(tool->packet_buf, data_len);
    remain_len -= data_len;
    if (remain_len) {
        _Msd_Usb2Mcu(NULL, remain_len);
    }
    /* printf("isp_pkt wlen %d\n", data_len); */
    /* printf_buf(cmd, cmd_len); */
    /* printf_buf(tool->packet_buf, data_len); */
    data = (u8 *)tool->packet_buf;
    while (parse_len < data_len) {
        do {
            byte = data[parse_len];
            if (byte == ISPT_PACKET_PID) {
                break;
            }
            parse_len++;
            if (parse_len > data_len) {
                return -EINVAL;
            }
        } while (1);

        memcpy(&packet_header, data + parse_len, sizeof(packet_header));
        parse_len += sizeof(packet_header);

        tool_buf = tool->isp_buf + tool->offset;
        if (tool->pkt_id == packet_header.number) {
            memcpy(tool_buf, data + parse_len, packet_header.length);
        } else {
            tool->pkt_id = 0;
            tool->offset = 0;
            continue;
        }

        tool->offset += packet_header.length;
        tool->pkt_id++;
        if (packet_header.number == packet_header.last_number) {
            if (tool->isp_buf[0] == ISP_CUS_SET_EFUSE_PARAM) {
            } else if (tool->isp_buf[0] == ISP_CUS_WRITE_MEM) {
                u32 addr;
                u8 *code = tool->isp_buf + 8;
                u32 code_size = tool->offset - 8;
                memcpy(&addr, tool->isp_buf + 4, 4);
                ispt_write_ram(code, addr, code_size);
            } else if (tool->isp_buf[0] == ISP_CUS_RUN_ADDR) {
                u32 addr;
                memcpy(&addr, tool->isp_buf + 4, 4);
                ispt_run_addr(addr);
            } else {
                struct ispt_customize_cfg user_cfg;

                user_cfg.channel = channel;
                user_cfg.mode = ISP_CUSTOMIZE_MODE_TOOL;
                user_cfg.data = tool->isp_buf;
                user_cfg.len  = tool->offset;

                isp_ioctl(ISP_IOCTL_SET_CUSTOMIZE_CFG, (unsigned int)&user_cfg);
            }
            tool->pkt_id = 0;
            tool->offset = 0;
        }

        parse_len += packet_header.length;
    }

    return 0;
}


static int ispt_parse_read_config(struct isp_tool_fh *tool, u8 *cmd, u32 cmd_len)
{
    void *dev;
    int data_len = 0;
    u8 *data;

    u8 channel = isp0_get_id();
    if (channel == 0xff || __this->inited == 0) {
        return -EINVAL;
    }
    //cmd -- 0 : isp_cmd, 1 -- read/write, 2 -- sub cmd
    data = tool->isp_buf;
    memcpy(data, cmd + 4, 2); //读模式特殊处理
    memcpy(&data_len, cmd + 6, 2);
    data_len = be16_to_cpu(data_len);
    /* printf("isp_read_cmd:\n"); */
    /* printf_buf(cmd, cmd_len); */
    if (cmd[2] == ISP_CUS_GET_CAM_READY) {  //获取设备状态
        memset(data, 0, data_len);
        data[0] = 1;
    } else if (cmd[2] == ISP_CUS_GET_EFUSE_PARAM) {
    } else {
        struct ispt_customize_cfg user_cfg = {0};

        user_cfg.cmd = cmd[2];
        user_cfg.channel = channel;
        user_cfg.mode = ISP_CUSTOMIZE_MODE_TOOL;
        user_cfg.data = data;
        user_cfg.len = data_len;

        isp_ioctl(ISP_IOCTL_GET_CUSTOMIZE_CFG, (unsigned int)&user_cfg);
    }
    /* printf("isp_pkt rlen %d\n", data_len); */
    /* printf_buf(tool->isp_buf, data_len); */
    _Msd_Mcu2Usb(data, data_len);

    return 0;
}

u32 isp_tool_parser(struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw)
{
    int err = 0;
    u8 *cmd;

    cmd = (u8 *)&cbw->operationCode;

    if (hook_usb_ispt && hook_usb_ispt(&isp_tool, cmd, 16)) {
        return 1;
    }

    switch (cmd[1]) {
    case 0x0:
        err = ispt_parse_write_config(&isp_tool, cmd, 16);
        break;
    case 0x1:
        err = ispt_parse_read_config(&isp_tool, cmd, 16);
        break;
    default:
        return 0;
    }

    return ((err == 0) ? 1 : 0);
}

#endif
