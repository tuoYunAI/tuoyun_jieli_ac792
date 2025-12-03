#include "dev_manager.h"
#include "app_config.h"
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
#include "dev_multiplex_api.h"
#endif
#include "fs/nor_fs.h"
#include "dev_update.h"
#include "system/init.h"
#include "event/event.h"
#include "event/device_event.h"

#define TCFG_DEV_MANAGER_ENABLE 1

#if (TCFG_DEV_MANAGER_ENABLE)

#define LOG_TAG         "[DEV_MANAGER]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ASSERT_ENABLE
#include "system/debug.h"


#include "dev_reg.c"

#define DEV_MANAGER_TASK_NAME							"dev_mg"
#define DEV_MANAGER_SCAN_DISK_MAX_DEEPTH				9

///设备管理总控制句柄
struct __dev_manager {
    struct list_head list;
    OS_MUTEX mutex;
    volatile u32 counter;
};
static struct __dev_manager dev_mg;
#define __this	(&dev_mg)

extern struct imount *fmount_get(const char *mount_path);

static u32 __dev_manager_get_time_stamp(void)
{
    u32 counter = __this->counter;
    __this->counter ++;
    return counter;
}

static int __dev_manager_add(const char *logo, u8 need_mount)
{
    if (logo == NULL) {
        return DEV_MANAGER_ADD_ERR_PARM;
    }

    log_info("%s add start", logo);

    const struct __dev_reg *p = NULL;
    const struct __dev_reg *n;

    for (n = dev_reg; n->logo != NULL; n++) {
        if (!strcmp(n->logo, logo)) {
            p = n;
            break;
        }
    }

    if (p) {
        ///挂载文件系统
        if (dev_manager_list_check_by_logo(logo)) {
            log_error("dev online aready, err!!!");
            return DEV_MANAGER_ADD_IN_LIST_AREADY;
        }

        struct __dev *dev = (struct __dev *)zalloc(sizeof(struct __dev));
        if (dev == NULL) {
            return DEV_MANAGER_ADD_ERR_NOMEM;
        }

        os_mutex_pend(&__this->mutex, 0);
        if (need_mount) {
            dev->fmnt = fmount_get(p->storage_path);
            if (!dev->fmnt) {
                dev->need_unmount = 1;
                dev->fmnt = mount(p->name, p->storage_path, p->fs_type, 32, NULL);
            }
        }
        dev->parm = p;
        dev->valid = (dev->fmnt ? 1 : 0);
        dev->active_stamp = __dev_manager_get_time_stamp();
        list_add_tail(&dev->entry, &__this->list);
        os_mutex_post(&__this->mutex);

        if (dev->fmnt == NULL) {
            return DEV_MANAGER_ADD_ERR_MOUNT_FAIL;
        }

        log_info("%s, %s add ok, %d", __FUNCTION__, logo, dev->active_stamp);

        return DEV_MANAGER_ADD_OK;
    }

    log_error("dev_manager_add can not find logo %s", logo);

    return DEV_MANAGER_ADD_ERR_NOT_FOUND;
}

static int __dev_manager_del(const char *logo)
{
    if (logo == NULL) {
        return -1;
    }

    struct __dev *dev, *n;

    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry_safe(dev, n, &__this->list, entry) {
        if (!strcmp(dev->parm->logo, logo)) {
            ///卸载文件系统
            if (dev->fmnt && dev->need_unmount) {
                unmount(dev->parm->storage_path);
            }
            list_del(&dev->entry);
            free(dev);
            log_info("%s, %s del ok", __FUNCTION__, logo);
            break;
        }
    }
    os_mutex_post(&__this->mutex);
    return 0;
}

int dev_manager_add(const char *logo)
{
    if (logo == NULL) {
        return -1;
    }

    int ret = 0;
#if TCFG_RECORD_FOLDER_DEV_ENABLE
    char rec_dev_logo[16];
    sprintf(rec_dev_logo, "%s%s", logo, "_rec");

    ret = __dev_manager_add(rec_dev_logo, 1);
    if (ret == DEV_MANAGER_ADD_OK) {
        ret = __dev_manager_add(logo, 1);
        if (ret) {
            __dev_manager_del(logo);
            __dev_manager_del(rec_dev_logo);
        }
    } else if (ret == DEV_MANAGER_ADD_ERR_NOT_FOUND) {
        ret = __dev_manager_add(logo, 1);
    } else {
        ret = __dev_manager_add(logo, 0);
    }
#else
    ret = __dev_manager_add(logo, 1);
#endif//TCFG_RECORD_FOLDER_DEV_ENABLE

    return ret;
}

int dev_manager_del(const char *logo)
{
    if (logo == NULL) {
        return -1;
    }

    __dev_manager_del(logo);

#if TCFG_RECORD_FOLDER_DEV_ENABLE
    char rec_dev_logo[16];
    sprintf(rec_dev_logo, "%s%s", logo, "_rec");
    __dev_manager_del(rec_dev_logo);
#endif//TCFG_RECORD_FOLDER_DEV_ENABLE

    return 0;
}

struct __dev *dev_manager_check(struct __dev *dev)
{
    if (dev == NULL) {
        return NULL;
    }

    struct __dev *p;
    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(p, &__this->list, entry) {
        if (!(p->fmnt)) {
            continue;
        }
        if (dev == p) {
            os_mutex_post(&__this->mutex);
            return p;
        }
    }
    os_mutex_post(&__this->mutex);

    return NULL;
}

struct __dev *dev_manager_check_by_logo(const char *logo)
{
    if (logo == NULL) {
        return NULL;
    }

    struct __dev *dev;
    list_for_each_entry(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (!strcmp(dev->parm->logo, logo)) {
            return dev;
        }
    }
    return NULL;
}

u32 dev_manager_get_total(u8 valid)
{
    u32 total_valid = 0;
    u32 total = 0;
    struct __dev *dev;
    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (dev->valid) {
            total_valid++;
        }
        total++;
    }
    os_mutex_post(&__this->mutex);
    return (valid ? total_valid : total);
}

struct __dev *dev_manager_find_first(u8 valid)
{
    struct __dev *dev = NULL;
    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (valid) {
            if (dev->valid) {
                os_mutex_post(&__this->mutex);
                return dev;
            }
        } else {
            os_mutex_post(&__this->mutex);
            return dev;
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

struct __dev *dev_manager_find_last(u8 valid)
{
    struct __dev *dev = NULL;
    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry_reverse(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (valid) {
            if (dev->valid) {
                os_mutex_post(&__this->mutex);
                return dev;
            }
        } else {
            os_mutex_post(&__this->mutex);
            return dev;
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

struct __dev *dev_manager_find_prev(struct __dev *dev, u8 valid)
{
    if (dev == NULL) {
        return NULL;
    }
    os_mutex_pend(&__this->mutex, 0);
    if (dev_manager_check(dev) == NULL) {
        os_mutex_post(&__this->mutex);
        ///传入的参数无效，返回活动设备
        return dev_manager_find_active(valid);
    }

    struct __dev *p = NULL;
    list_for_each_entry(p, &dev->entry, entry) {
        if ((void *)p == (void *)(&__this->list)) {
            continue;
        }
        if (!(p->fmnt)) {
            continue;
        }
        if (valid) {
            if (p->valid) {
                os_mutex_post(&__this->mutex);
                return p;
            }
        } else {
            os_mutex_post(&__this->mutex);
            return p;
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

struct __dev *dev_manager_find_next(struct __dev *dev, u8 valid)
{
    if (dev == NULL) {
        return NULL;
    }
    os_mutex_pend(&__this->mutex, 0);
    if (dev_manager_check(dev) == NULL) {
        os_mutex_post(&__this->mutex);
        ///传入的参数无效， 返回活动设备
        return dev_manager_find_active(valid);
    }

    struct __dev *p = NULL;
    list_for_each_entry_reverse(p, &dev->entry, entry) {
        if ((void *)p == (void *)(&__this->list)) {
            continue;
        }
        if (!(p->fmnt)) {
            continue;
        }
        if (valid) {
            if (p->valid) {
                os_mutex_post(&__this->mutex);
                return p;
            }
        } else {
            os_mutex_post(&__this->mutex);
            return p;
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

struct __dev *dev_manager_find_active(u8 valid)
{
    struct __dev *dev = NULL;
    struct __dev *active = NULL;

    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (valid) {
            if (dev->valid) {
                if (active) {
                    if (active->active_stamp < dev->active_stamp) {
                        active = dev;
                    }
                } else {
                    active = dev;
                }
            }
        } else {
            if (active) {
                if (active->active_stamp < dev->active_stamp) {
                    active = dev;
                }
            } else {
                active = dev;
            }

        }
    }
    os_mutex_post(&__this->mutex);
    return active;
}

struct __dev *dev_manager_find_spec(const char *logo, u8 valid)
{
    if (logo == NULL) {
        return NULL;
    }

    struct __dev *dev = NULL;
    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (!strcmp(dev->parm->logo, logo)) {
            if (valid) {
                if (dev->valid) {
                    os_mutex_post(&__this->mutex);
                    return dev;
                }
            } else {
                os_mutex_post(&__this->mutex);
                return dev;
            }
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

struct __dev *dev_manager_find_by_index(u32 index, u8 valid)
{
    struct __dev *dev = NULL;
    u32 i = 0;

    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!(dev->fmnt)) {
            continue;
        }
        if (valid) {
            if (dev->valid) {
                if (i == index) {
                    os_mutex_post(&__this->mutex);
                    return dev;
                }
                i++;
            }
        } else {
            if (i == index) {
                os_mutex_post(&__this->mutex);
                return dev;
            }
            i++;
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

void dev_manager_set_valid(struct __dev *dev, u8 flag)
{
    os_mutex_pend(&__this->mutex, 0);
    if (dev_manager_check(dev)) {
        dev->valid = flag;
    }
    os_mutex_post(&__this->mutex);
}

void dev_manager_set_valid_by_logo(const char *logo, u8 flag)
{
    if (logo == NULL) {
        return;
    }

    struct __dev *dev = NULL;
    os_mutex_pend(&__this->mutex, 0);
    dev = dev_manager_check_by_logo(logo);
    if (dev) {
        dev->valid = flag;
    }
    os_mutex_post(&__this->mutex);
}

void dev_manager_set_active(struct __dev *dev)
{
    os_mutex_pend(&__this->mutex, 0);
    if (dev_manager_check(dev)) {
        dev->active_stamp = __dev_manager_get_time_stamp();
    }
    os_mutex_post(&__this->mutex);
}

void dev_manager_set_active_by_logo(const char *logo)
{
    if (logo == NULL) {
        return;
    }

    struct __dev *dev = NULL;
    os_mutex_pend(&__this->mutex, 0);
    dev = dev_manager_check_by_logo(logo);
    if (dev) {
        dev->active_stamp = __dev_manager_get_time_stamp();
    }
    os_mutex_post(&__this->mutex);
}

const char *dev_manager_get_logo(struct __dev *dev)
{
    os_mutex_pend(&__this->mutex, 0);
    if (dev_manager_check(dev) == NULL) {
        os_mutex_post(&__this->mutex);
        return NULL;
    }
    const char *logo = dev->parm->logo;
    os_mutex_post(&__this->mutex);
    return logo;
}

const char *dev_manager_get_phy_logo(struct __dev *dev)
{
    const char *logo = dev_manager_get_logo(dev);
    char phy_dev_logo[16] = {0};
    if (logo) {
        const char *str = strstr(logo, "_rec");
        if (str) {
            strncpy(phy_dev_logo, logo, strlen(logo) - strlen(str));
            struct __dev *phy_dev = dev_manager_find_spec(phy_dev_logo, 0);
            return dev_manager_get_logo(phy_dev);
        }
    }
    return logo;
}

const char *dev_manager_get_rec_logo(struct __dev *dev)
{
    const char *logo = dev_manager_get_logo(dev);
    char rec_dev_logo[16] = {0};
    if (logo) {
        const char *str = strstr(logo, "_rec");
        if (!str) {
            sprintf(rec_dev_logo, "%s%s", logo, "_rec");
            struct __dev *rec_dev = dev_manager_find_spec(rec_dev_logo, 0);
            return dev_manager_get_logo(rec_dev);
        }
    }
    return logo;
}

const char *dev_manager_get_root_path(struct __dev *dev)
{
    os_mutex_pend(&__this->mutex, 0);
    if (dev_manager_check(dev) == NULL) {
        os_mutex_post(&__this->mutex);
        return NULL;
    }
    const char *path = dev->parm->root_path;
    os_mutex_post(&__this->mutex);
    return path;
}

const char *dev_manager_get_root_path_by_logo(const char *logo)
{
    os_mutex_pend(&__this->mutex, 0);
    struct __dev *dev = dev_manager_check_by_logo(logo);
    if (dev == NULL) {
        os_mutex_post(&__this->mutex);
        return NULL;
    }
    const char *path = dev->parm->root_path;
    os_mutex_post(&__this->mutex);
    return path;
}

struct imount *dev_manager_get_mount_hdl(struct __dev *dev)
{
    if (dev == NULL) {
        return NULL;
    } else {
        return dev->fmnt;
    }
}

bool dev_manager_online_check_by_logo(const char *logo, u8 valid)
{
    struct __dev *dev = dev_manager_find_spec(logo, valid);
    return (dev ? true : false);
}

bool dev_manager_online_check(struct __dev *dev, u8 valid)
{
    if (dev_manager_check(dev) == NULL) {
        return 0;
    } else {
        if (valid) {
            return (dev->valid ? true : false);
        } else {
            return true;
        }
    }
}

struct __dev *dev_manager_list_check_by_logo(const char *logo)
{
    if (logo == NULL) {
        return 0;
    }

    struct __dev *dev;
    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!strcmp(dev->parm->logo, logo)) {
            os_mutex_post(&__this->mutex);
            return dev;
        }
    }
    os_mutex_post(&__this->mutex);
    return NULL;
}

void dev_manager_list_check_mount(void)
{
    struct __dev *dev;

    os_mutex_pend(&__this->mutex, 0);
    list_for_each_entry(dev, &__this->list, entry) {
        if (!strcmp(dev->parm->logo, "virfat_flash")) {
            continue;
        }

        if (dev->fmnt && dev->need_unmount) {
            unmount(dev->parm->storage_path);
        }

        dev->fmnt = NULL;

        const struct __dev_reg *p = dev->parm;
        dev->fmnt = mount(p->name, p->storage_path, p->fs_type, 32, NULL);
        dev->valid = (dev->fmnt ? 1 : 0);
        dev->need_unmount = (dev->fmnt ? 1 : 0);
    }
    os_mutex_post(&__this->mutex);
}

static int __dev_manager_mount(const char *logo)
{
    int ret = 0;
    os_mutex_pend(&__this->mutex, 0);
    struct __dev *dev = dev_manager_list_check_by_logo(logo);
    if (dev == NULL) {
        os_mutex_post(&__this->mutex);
        return -1;
    }
    if (dev->fmnt == NULL) {
        const struct __dev_reg *p = dev->parm;
        dev->fmnt = mount(p->name, p->storage_path, p->fs_type, 32, NULL);
        dev->valid = (dev->fmnt ? 1 : 0);
        dev->need_unmount = (dev->fmnt ? 1 : 0);
    }
    ret = (dev->valid ? 0 : -1);
    os_mutex_post(&__this->mutex);
    return ret;
}

static int __dev_manager_unmount(const char *logo)
{
    os_mutex_pend(&__this->mutex, 0);
    struct __dev *dev = dev_manager_check_by_logo(logo);
    if (dev == NULL) {
        os_mutex_post(&__this->mutex);
        return -1;
    }
    if (dev->fmnt) {
        if (dev->need_unmount) {
            unmount(dev->parm->storage_path);
            dev->need_unmount = 0;
        }
        dev->fmnt = NULL;
    }
    dev->valid = 0;
    os_mutex_post(&__this->mutex);
    return 0;
}

int dev_manager_mount(const char *logo)
{
    int ret = __dev_manager_mount(logo);
    if (ret == 0) {
#if TCFG_RECORD_FOLDER_DEV_ENABLE
        char rec_dev_logo[16] = {0};
        sprintf(rec_dev_logo, "%s%s", logo, "_rec");
        ret = __dev_manager_mount(rec_dev_logo);
        if (ret) {
            __dev_manager_unmount(logo);
        }
#endif
    }
    return ret;
}

int dev_manager_unmount(const char *logo)
{
    int err = 0;
#if TCFG_RECORD_FOLDER_DEV_ENABLE
    char rec_dev_logo[16] = {0};
    sprintf(rec_dev_logo, "%s%s", logo, "_rec");
    __dev_manager_unmount(rec_dev_logo);
#endif
    err = __dev_manager_unmount(logo);
    return err;
}

static void dev_manager_var_init(void)
{
    memset(__this, 0, sizeof(struct __dev_manager));
    INIT_LIST_HEAD(&__this->list);
    os_mutex_create(&__this->mutex);
}

static void dev_manager_in_event_handler(struct sys_event *e)
{
    struct device_event *event = (struct device_event *)e->payload;

    if (e->from == DEVICE_EVENT_FROM_SD || e->from == DEVICE_EVENT_FROM_UDISK) {
        if (event->event == DEVICE_EVENT_IN) {
            dev_manager_add(event->arg);
        }
    }
}

static void dev_manager_out_event_handler(struct sys_event *e)
{
    struct device_event *event = (struct device_event *)e->payload;

    if (e->from == DEVICE_EVENT_FROM_SD || e->from == DEVICE_EVENT_FROM_UDISK) {
        if (event->event == DEVICE_EVENT_OUT) {
            dev_manager_del(event->arg);
        }
    }
}

static int dev_manager_init(void)
{
    dev_manager_var_init();

    register_sys_event_handler(SYS_DEVICE_EVENT, DEVICE_EVENT_FROM_SD, 1, dev_manager_in_event_handler);
    register_sys_event_handler(SYS_DEVICE_EVENT, DEVICE_EVENT_FROM_UDISK, 1, dev_manager_in_event_handler);
    register_sys_event_handler(SYS_DEVICE_EVENT, DEVICE_EVENT_FROM_SD, 0, dev_manager_out_event_handler);
    register_sys_event_handler(SYS_DEVICE_EVENT, DEVICE_EVENT_FROM_UDISK, 0, dev_manager_out_event_handler);

#if TCFG_SD_ALWAY_ONLINE_ENABLE
    int err;
#if (defined(TCFG_SD0_ENABLE) && (TCFG_SD0_ENABLE))
    extern void force_set_sd_online(char *sdx);
    force_set_sd_online("sd0");
    err = dev_manager_add("sd0");
#elif (defined(TCFG_SD1_ENABLE) && (TCFG_SD1_ENABLE))
    extern void force_set_sd_online(char *sdx);
    force_set_sd_online("sd1");
    err = dev_manager_add("sd1");
#endif
    if (err != 0) {
        log_error("sd add fail");
    } else {
#if (TCFG_SD_ALWAY_ONLINE_ENABLE && (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE))
        extern void sdx_dev_detect_timer_del();
        sdx_dev_detect_timer_del();
#endif
    }
#endif

    dev_manager_add("sdfile");

#if TCFG_NOR_REC
    dev_manager_add("rec_nor");
#endif

#if TCFG_NOR_FAT
    dev_manager_add("fat_nor");
    dev_manager_set_valid_by_logo("fat_nor", 0);///将设备设置为无效设备
#endif

#if TCFG_NOR_FS
    dev_manager_add("res_nor");
#endif

#if TCFG_VIR_UDISK_ENABLE
    dev_manager_add("vir_udisk");
#endif

#if TCFG_VIRFAT_FLASH_ENABLE
    dev_manager_add("virfat_flash");
    dev_manager_set_valid_by_logo("virfat_flash", 0);///将设备设置为无效设备
#endif

#if TCFG_FLASH_INSIDE_REC_ENABLE
    set_rec_capacity(512 * 1024); //需要先设置容量,注意要小于Ini文件设置大小.
    _sdfile_rec_init();
    dev_manager_add("rec_sdfile");
#endif

    return 0;
}
late_initcall(dev_manager_init);

#endif/*TCFG_DEV_MANAGER_ENABLE*/
