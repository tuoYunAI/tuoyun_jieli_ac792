#include "device/device.h"
#include "event/event.h"
#include "event/device_event.h"
#include "asm/sfc_norflash_api.h"
#include "fs/fs.h"
#include "system/init.h"
#include "sdio/sdmmc.h"
#include "generic/ascii.h"
#include "app_config.h"

#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
#include "usb_stack.h"
#include "msd.h"
#include "device/hid.h"
#include "uac_audio.h"
#include "usb_host.h"
#include "audio.h"
#include "hid.h"
#include "adb.h"
#include "usbnet.h"
#include "usb_host_cdc.h"
#include "hub.h"
#endif

#define LOG_TAG         "[DEVICE_MOUNT]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ASSERT_ENABLE
#include "system/debug.h"

#ifdef CONFIG_JLFAT_ENABLE
#define FAT_FILE_SYSTEM_NAME "jlfat"
#else
#define FAT_FILE_SYSTEM_NAME "fat"
#endif

#ifndef FAT_CACHE_NUM
#define FAT_CACHE_NUM 32
#endif

#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE

enum {
    SD_UNMOUNT = 0,
    SD_MOUNT_SUSS,
    SD_MOUNT_FAILD,
};

static u8 sd_mount[2];
static OS_MUTEX sd_mutex;

static int __sd_mutex_init()
{
    return os_mutex_create(&sd_mutex);
}
early_initcall(__sd_mutex_init);

static int mount_sd_to_fs(const char *sd_name)
{
    int err = 0;
    u32 class = 0, capacity = 0, block_size = 0;
    struct imount *mt;
    int id = sd_name[2] - '0';
    const char *dev = sd_name;
    void *fd = NULL;

    err = os_mutex_pend(&sd_mutex, 0);
    if (err) {
        return -EFAULT;
    }

    if (sd_mount[id] == SD_MOUNT_SUSS) {
        goto __exit;
    }
    if (sd_mount[id] == SD_MOUNT_FAILD) {
        err = -EFAULT;
        goto __exit;
    }
    if (!dev_online(dev)) {
        err = -EFAULT;
        goto __exit;
    }

#ifdef CONFIG_DMSDX_ENABLE
    u8 i = 0;
    char sub_dev_name[7] = {0};
    char mount_path_name[16] = {0};

    strcpy(sub_dev_name, dev);
    strcpy(mount_path_name, id ? "storage/sd1" : "storage/sd0");

    do {
        sub_dev_name[strlen(dev)] = '.';
        sub_dev_name[strlen(dev) + 1] = '0' + i;
        fd = dev_open(sub_dev_name, 0);
        if (fd) {
            dev_close(fd);
            mount_path_name[11] = '.';
            mount_path_name[12] = '0' + i;
            ++i;
            mt = mount(sub_dev_name, mount_path_name, FAT_FILE_SYSTEM_NAME, FAT_CACHE_NUM, NULL);
            if (!mt) {
                log_error("%s mount %s %s fail, format...", sub_dev_name, mount_path_name, FAT_FILE_SYSTEM_NAME);
                f_format(sub_dev_name, FAT_FILE_SYSTEM_NAME, 32 * 1024);
                mt = mount(sub_dev_name, id ? "storage/sd1" : "storage/sd0", FAT_FILE_SYSTEM_NAME, FAT_CACHE_NUM, NULL);
            } else {
                log_info("%s mount %s %s suss", sub_dev_name, mount_path_name, FAT_FILE_SYSTEM_NAME);
            }
        }
    } while (fd != NULL && i < 10);

    if (i == 0)
#endif
    {
        mt = mount(dev, id ? "storage/sd1" : "storage/sd0", FAT_FILE_SYSTEM_NAME, FAT_CACHE_NUM, NULL);
        if (!mt) {
            log_error("mount %s fail", sd_name);
            err = -EFAULT;
            goto __err;
        } else {
            log_info("mount %s suss", sd_name);
        }
    }

    fd = dev_open(dev, 0);
    if (!fd) {
        err = -EFAULT;
        goto __err;
    }

    dev_ioctl(fd, IOCTL_GET_CLASS, (u32)&class);
    if (class == SD_CLASS_10) {
        log_info("%s card class: 10", sd_name);
    } else {
        log_info("%s card class: %d", sd_name, class * 2);
    }

    dev_close(fd);

__err:
    sd_mount[id] = err ? SD_MOUNT_FAILD : SD_MOUNT_SUSS;
__exit:
    os_mutex_post(&sd_mutex);

    return err;
}

static void unmount_sd_to_fs(const char *sd_name)
{
    int id = sd_name[2] - '0';
    os_mutex_pend(&sd_mutex, 0);
#ifdef CONFIG_DMSDX_ENABLE
    u8 i = 0;
    char mount_path_name[16] = {0};

    strcpy(mount_path_name, id ? "storage/sd1" : "storage/sd0");
    mount_path_name[11] = '.';

    while (1) {
        mount_path_name[12] = '0' + i;
        if (!fmount_exist(mount_path_name)) {
            break;
        }
        unmount(mount_path_name);
        log_info("unmount %s", mount_path_name);
        ++i;
    }

    if (i == 0)
#endif
    {
        unmount(id ? "storage/sd1" : "storage/sd0");
    }
    sd_mount[id] = SD_UNMOUNT;
    log_info("unmount %s", sd_name);
    os_mutex_post(&sd_mutex);
}
#endif

int storage_device_ready(void)
{
#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
    if (!dev_online(SDX_DEV)) {
        return false;
    }
    if (SDX_DEV[2] - '0' < 2 && sd_mount[SDX_DEV[2] - '0'] == SD_UNMOUNT) {
        mount_sd_to_fs(SDX_DEV);
    }

    return fmount_exist(CONFIG_STORAGE_PATH);
#else
    return false;
#endif
}

int sdcard_storage_device_ready(const char *sd_name)
{
#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
    int id = sd_name[2] - '0';

    if (!dev_online(sd_name)) {
        return false;
    }
    if (sd_mount[id] == SD_UNMOUNT) {
        mount_sd_to_fs(sd_name);
    }

    return fmount_exist(id ? "storage/sd1" : "storage/sd0");
#else
    return false;
#endif
}

int sdcard_storage_device_format(const char *sd_name)
{
#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
    int err;

    if (!sd_name || strlen(sd_name) != 3) {
        return -EPERM;
    }

    unmount_sd_to_fs(sd_name);

    err = f_format(sd_name, FAT_FILE_SYSTEM_NAME, 32 * 1024);
    if (err == 0) {
        mount_sd_to_fs(sd_name);
    }

    return err;
#else
    return 0;
#endif
}

#ifdef CONFIG_DMSDX_ENABLE
int sdcard_storage_subdevice_format(const char *dev_name)
{
    int err;
    char sd_name[6] = {0};

    if (!dev_name || strlen(dev_name) != 5) {
        return -EPERM;
    }

    memcpy(sd_name, dev_name, 3);

    unmount_sd_to_fs(sd_name);

    err = f_format(dev_name, FAT_FILE_SYSTEM_NAME, 32 * 1024);
    if (err == 0) {
        mount_sd_to_fs(sd_name);
    }

    return err;
}
#endif

#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
/*
 * sd卡插拔事件处理
 */
static void sd_event_handler(struct device_event *event)
{
    switch (event->event) {
    case DEVICE_EVENT_IN:
        log_info("%s: in", (const char *)event->arg);
        mount_sd_to_fs(event->arg);
        break;
    case DEVICE_EVENT_OUT:
        log_info("%s: out", (const char *)event->arg);
        unmount_sd_to_fs(event->arg);
        break;
    }
}
#endif


#if TCFG_UDISK_ENABLE

enum {
    UDISK_UNMOUNT = 0,
    UDISK_MOUNT_SUSS,
    UDISK_MOUNT_FAILD,
};

static OS_MUTEX udisk_mutex;
static u8 udisk_mount[2];
static u8 udisk_online[2];

static int __udisk_mutex_init()
{
    return os_mutex_create(&udisk_mutex);
}
early_initcall(__udisk_mutex_init);

static int mount_udisk_to_fs(int id)
{
    int err = 0;
    struct imount *mt;
    u32 sta = 0;
    const char *name = id ? "udisk1" : "udisk0";

    err = os_mutex_pend(&udisk_mutex, 0);
    if (err) {
        return -EFAULT;
    }

    if (udisk_mount[id] == UDISK_MOUNT_SUSS) {
        goto __exit;
    }
    if (udisk_mount[id] == UDISK_MOUNT_FAILD) {
        err = -EFAULT;
        goto __exit;
    }

    sta = dev_online(name);
    if (!sta) {
        err = -EFAULT;
        goto __exit;
    }

    mt = mount(name, id ? "storage/udisk1" : "storage/udisk0", FAT_FILE_SYSTEM_NAME, FAT_CACHE_NUM, NULL);
    if (!mt) {
        log_error("mount %s fail", name);
        err = -EFAULT;
    } else {
        log_info("mount %s suss", name);
    }

    udisk_mount[id] = err ? UDISK_MOUNT_FAILD : UDISK_MOUNT_SUSS;
__exit:
    os_mutex_post(&udisk_mutex);

    return err;
}

static int unmount_udisk_to_fs(int id)
{
    os_mutex_pend(&udisk_mutex, 0);
    if (udisk_mount[id] == UDISK_MOUNT_FAILD) {
        udisk_mount[id] = UDISK_UNMOUNT;
        os_mutex_post(&udisk_mutex);
        return -EFAULT;
    }
    unmount(id ? "storage/udisk1" : "storage/udisk0");
    udisk_mount[id] = UDISK_UNMOUNT;
    log_info("unmount udisk%d", id);
    os_mutex_post(&udisk_mutex);
    return 0;
}

int udisk_storage_device_ready(int id)
{
    u32 sta = 0;

    sta = dev_online(id ? "udisk1" : "udisk0");
    if (!sta) {
        return false;
    }
    if (udisk_mount[id] == UDISK_UNMOUNT) {
        mount_udisk_to_fs(id);
    }
    return fmount_exist(id ? "storage/udisk1" : "storage/udisk0");
}

int udisk_storage_device_format(int id)
{
    int err;

    unmount_udisk_to_fs(id);

    err = f_format(id ? "udisk1" : "udisk0", FAT_FILE_SYSTEM_NAME, 32 * 1024);
    if (err == 0) {
        mount_udisk_to_fs(id);
    }

    return err;
}


#endif

static u8 incharge;

int sys_power_is_charging(void)
{
    return incharge;
}

void otg_event_handler(struct device_event *event)
{
    if (event->event == DEVICE_EVENT_IN) {
        if (((const char *)event->value)[0] == 'h') {
            int usb_id = ((const char *)event->value)[2] - '0';
            log_info("usb%d connect, host mode", usb_id);
#if TCFG_USB_HOST_ENABLE
            usb_host_mount(usb_id, 0, MOUNT_RETRY, MOUNT_RESET, MOUNT_TIMEOUT);
#endif
        } else if (((const char *)event->value)[0] == 'c') {
            incharge = 1;
            //插入充电，插入检测会有2秒延迟
        }
    } else if (event->event == DEVICE_EVENT_OUT) {
        if (((const char *)event->value)[0] == 'h') {
            int usb_id = ((const char *)event->value)[2] - '0';
            log_info("usb%d disconnect, host mode", usb_id);
#if TCFG_USB_HOST_ENABLE
            usb_host_unmount(usb_id, 0);
#endif
        } else if (((const char *)event->value)[0] == 'c') {
            incharge = 0;
            //拔出充电，拔出检测会有5秒延迟
        }
    }
}

#if TCFG_USB_HOST_ENABLE
void usb_host_event_handler(struct device_event *event)
{
    int usb_id;

#if TCFG_HOST_AUDIO_ENABLE
    if (event->event == DEVICE_EVENT_ONLINE) {
        usb_id = ((const char *)event->arg)[8] - '0';
        log_info("APP usb host%d audio start", usb_id);
        if (!strncmp((const char *)event->value, "audio", 5)) {
            usb_audio_start_process(usb_id);
        }
    } else if (event->event == DEVICE_EVENT_OFFLINE) {
        usb_id = ((const char *)event->arg)[8] - '0';
        log_info("APP usb host%d audio stop", usb_id);
        if (!strncmp((const char *)event->value, "audio", 5)) {
            usb_audio_stop_process(usb_id);
        }
    }
#endif

    if (event->event == DEVICE_EVENT_IN) {
        usb_id = ((const char *)event->arg)[8] - '0';
        log_info("usb host%d mount succ", usb_id);
#if TCFG_UDISK_ENABLE
        if (!strncmp((const char *)event->value, "udisk", 5)) {
            //上电发现U盘原本挂在USB则出现不进中断，无法挂载文件系统，U盘断电就解决
            if (0 == mount_udisk_to_fs(usb_id)) {
                struct device_event udisk_event = {0};
                udisk_event.arg = (void *)event->value;
                udisk_event.event = event->event;
                device_event_notify(DEVICE_EVENT_FROM_UDISK, &udisk_event);
            }
        }
#endif
#if TCFG_HOST_UVC_ENABLE
        extern int usb_host_video_init(const usb_dev usb_id, const u8 sub_id);
        if (!strncmp((const char *)event->value, "uvc", 3)) {
            int sub_id = ((const char *)event->value)[3] - '0';
            if (usb_id == 1) {
                if (uvc_host_is_support_h264_fmt()) { /* 判断UVC是否支持h264格式 */
                    u8 uvc_vaild_nums = uvc_host_is_vaild();
                    for (int i = 0; i < uvc_vaild_nums; i++) {
                        usb_host_video_init(1, 1 + i); /* 只接在usb1上 */
                    }
                } else if (sub_id >= 2) {
                    usb_host_video_init(1, sub_id - 1); /* hub */
                } else {
                    usb_host_video_init(usb_id, usb_id);
                }
            } else {
                usb_host_video_init(usb_id, usb_id);
            }
        }
#endif
#if TCFG_HOST_AUDIO_ENABLE
#if USB_HOST_NO_APP_MODE
        if (!strncmp((const char *)event->value, "audio", 5)) {
            usb_audio_start_process(usb_id);
        }
#endif
#endif
#if TCFG_HID_HOST_ENABLE
        if (!strncmp((const char *)event->value, "hid", 3)) {
            hid_process(usb_id);
        }
#endif
#if TCFG_HOST_CDC_ENABLE
        if (!strncmp((const char *)event->value, "cdc", 3)) {
            usb_cdc_init(usb_id);
            //host_cdc_send_demo(usb_id);
        }
#endif
#if TCFG_HOST_HUB_ENABLE
        if (!strncmp((const char *)event->value, "hub", 3)) {
            usb_hub_process(usb_id);
        }
#endif
    } else if (event->event == DEVICE_EVENT_OUT) {
        usb_id = ((const char *)event->arg)[8] - '0';
        log_info("usb host%d unmount", usb_id);
#if TCFG_UDISK_ENABLE
        if (!strncmp((const char *)event->value, "udisk", 5)) {
            if (0 == unmount_udisk_to_fs(usb_id)) {
                struct device_event udisk_event = {0};
                udisk_event.arg = (void *)event->value;
                udisk_event.event = event->event;
                device_event_notify(DEVICE_EVENT_FROM_UDISK, &udisk_event);
            }
        }
#endif
#if TCFG_HOST_UVC_ENABLE
        extern int uvc_host_camera_out(u8 usb_id);
        if (!strncmp((const char *)event->value, "uvc", 3)) {
            uvc_host_camera_out(usb_id);
        }
#endif
#if TCFG_HOST_AUDIO_ENABLE
#if USB_HOST_NO_APP_MODE
        if (!strncmp((const char *)event->value, "audio", 5)) {
            usb_audio_stop_process(usb_id);
        }
#endif
#endif
#if TCFG_HID_HOST_ENABLE
        if (!strncmp((const char *)event->value, "hid", 3)) {
            hid_stop_process(usb_id);
        }
#endif
#if TCFG_ADB_ENABLE
        if (!strncmp((const char *)event->value, "adb", 3)) {
            adb_stop_process(usb_id);
        }
#endif
#if TCFG_HOST_WIRELESS_ENABLE
        if (!strncmp((const char *)event->value, "wireless", 8)) {
            usb_net_stop_process(usb_id);
        }
        if (!strncmp((const char *)event->value, "at_port", 7)) {
            usb_net_at_port_stop_process(usb_id);
        }
#endif
#if TCFG_HOST_RNDIS_ENABLE
        if (!strncmp((const char *)event->value, "rndis", 5)) {
            usb_net_stop_process(usb_id);
        }
#endif
#if TCFG_HOST_CDC_ENABLE
        if (!strncmp((const char *)event->value, "cdc", 3)) {
            usb_cdc_exit(usb_id);
        }
#endif
#if TCFG_HOST_HUB_ENABLE
        if (!strncmp((const char *)event->value, "hub", 3)) {
            /* hub_mount = 0; */
            usb_hub_exit(usb_id);
            log_info("usb_hub offline %d", usb_id);
        }
#endif
    }
}
#endif

#if TCFG_USB_SLAVE_AUDIO_ENABLE && (TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE)
static int pc_user_event_handler(struct device_event *event)
{
    u32 state;
    int usb_id = event->value;

    if (event->event == DEVICE_EVENT_ONLINE) {
        state = usb_otg_online(usb_id);
        if ((state == SLAVE_MODE) || (state == SLAVE_MODE_WAIT_CONFIRMATION)) {
#if TCFG_USB_CDC_BACKGROUND_RUN
            usb_pause(usb_id);
#endif
            usb_start(usb_id, 0);
        }
    } else if (event->event == DEVICE_EVENT_OFFLINE) {
        state = usb_otg_online(usb_id);
        if ((state == SLAVE_MODE) || (state == SLAVE_MODE_WAIT_CONFIRMATION)) {
            usb_pause(usb_id);
#if TCFG_USB_CDC_BACKGROUND_RUN
            usb_start(usb_id, CDC_CLASS);
#endif
        }
    }

    return false;
}
#endif

static int device_event_handler(struct sys_event *e)
{
    struct device_event *event = (struct device_event *)e->payload;

    if (e->from == DEVICE_EVENT_FROM_OTG) {
#if TCFG_USB_SLAVE_ENABLE
        extern int pc_device_event_handler(struct sys_event * e);
        if (pc_device_event_handler(e)) {
            return 1;
        }
#endif
        otg_event_handler(event);
#if TCFG_USB_SLAVE_AUDIO_ENABLE && (TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE)
    } else if (e->from == DEVICE_EVENT_FROM_PC) {
        return pc_user_event_handler(event);
#endif
#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
    } else if (e->from == DEVICE_EVENT_FROM_SD) {
        sd_event_handler(event);
#endif
#if TCFG_USB_SLAVE_AUDIO_ENABLE && defined CONFIG_AUDIO_ENABLE
    } else if (e->from == DEVICE_EVENT_FROM_UAC) {
        extern int usb_audio_event_handler(struct device_event * event);
        return usb_audio_event_handler(event);
#endif
#if TCFG_USB_HOST_ENABLE
    } else if (e->from == DEVICE_EVENT_FROM_USB_HOST) {
        usb_host_event_handler(event);
#endif
#if TCFG_EQ_ENABLE && TCFG_EQ_ONLINE_ENABLE && defined CONFIG_AUDIO_ENABLE && \
        defined TCFG_COMM_TYPE && (TCFG_COMM_TYPE == TCFG_USB_COMM)
    } else if (e->from == DEVICE_EVENT_FROM_CFG_TOOL) {
        extern void usb_cdc_read_data_handler(struct device_event * event);
        usb_cdc_read_data_handler(event);
#endif
    }

    return 0;
}

/*
 * 静态注册设备事件回调函数，优先级为0
 */
SYS_EVENT_STATIC_HANDLER_REGISTER(device_event, 0) = {
    .event_type     = SYS_DEVICE_EVENT,
    .prob_handler   = device_event_handler,
    .post_handler   = NULL,
};

extern int sdfile_ext_init(const char *dev_name, u32 sdfile_ext_base_addr, u32 sdfile_ext_end_addr);
extern u32 boot_info_get_sfc_base_addr(void);
extern u32 ex_app_info_get_base_addr(void);

#ifdef CONFIG_SDFILE_EXT_ENABLE
void sdfile_ext_mount_init(void)
{
    while (!dev_online(SDX_DEV)) {
        os_time_dly(1);
    }

    /* sdfile_ext_init(SDX_DEV, boot_info_get_sfc_base_addr(), __FLASH_SIZE__); */
    sdfile_ext_init(SDX_DEV, ex_app_info_get_base_addr(), CONFIG_SDNAND_HFS_LEN);
}
#endif


#ifdef CONFIG_EXFLASH_ENABLE
static int sdfile_fat_mount_init(void)
{
    mount("virfat_flash", "storage/virfat_flash", "sdfile_fat", 32, NULL);

    return 0;
}
platform_initcall(sdfile_fat_mount_init);
#endif

