#include "product_main.h"
#include "video_dec_server.h"
#include "video_server.h"

#ifdef PRODUCT_TEST_ENABLE

static struct product_port_type {
    OS_SEM lcd_sem;
    OS_SEM pir_sem;
    OS_SEM gsensor_sem;
    OS_SEM touchpanel_sem;
    u8 touchpanel_mflag;
} *__THIS = NULL;


#if 1
int _flash_erase(u32 addr, u32 size)
{
    int ret;
    u8 *reserve_4k_area = NULL;
#define FLASH_BLOCK 4096

    int beginBlock = addr / FLASH_BLOCK;
    int endBlock = (addr + size) / FLASH_BLOCK;
    int head = addr % FLASH_BLOCK;  // 起始地址在块内的偏移
    int tail = (addr + size) % FLASH_BLOCK;  // 结束地址在块内的偏移

    printf("head :%d", head);
    printf("begin block:%d, endblock:%d, head:%d, tail:%d, size:%d", beginBlock, endBlock, head, tail, size);

    reserve_4k_area = malloc(FLASH_BLOCK);
    if (reserve_4k_area == NULL) {
        return -1;  // 分配失败直接返回
    }

    if (beginBlock == endBlock) {
        norflash_read(NULL, reserve_4k_area, FLASH_BLOCK, beginBlock * FLASH_BLOCK);
        memset(reserve_4k_area + head, 0xFF, size);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, beginBlock * FLASH_BLOCK);
        norflash_write(NULL, reserve_4k_area, FLASH_BLOCK, beginBlock * FLASH_BLOCK);
        free(reserve_4k_area);
        return 0;
    }

    norflash_read(NULL, reserve_4k_area, FLASH_BLOCK, beginBlock * FLASH_BLOCK);
    memset(reserve_4k_area + head, 0xFF, FLASH_BLOCK - head);
    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, beginBlock * FLASH_BLOCK);
    norflash_write(NULL, reserve_4k_area, FLASH_BLOCK, beginBlock * FLASH_BLOCK);

    for (int i = beginBlock + 1; i < endBlock; i++) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, i * FLASH_BLOCK);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, i * FLASH_BLOCK);
    }

    if (tail > 0) {
        norflash_read(NULL, reserve_4k_area, FLASH_BLOCK, endBlock * FLASH_BLOCK);
        memset(reserve_4k_area, 0xFF, tail);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, endBlock * FLASH_BLOCK);
        norflash_write(NULL, reserve_4k_area, FLASH_BLOCK, endBlock * FLASH_BLOCK);
    } else {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, endBlock * FLASH_BLOCK);
    }

    free(reserve_4k_area);
    reserve_4k_area = NULL;
    return 0;
}
#endif // 0

void product_port_init(void)
{
    if (!__THIS) {
        __THIS = zalloc(sizeof(struct product_port_type));
        ASSERT(__THIS);
    }
}


u8 product_uuid_wr(u8 *uuid, u8 is_write)
{
    u8 exist[] = "NULL";
    u8 temp[PRODUCT_UUID_SIZE] = {0};

    if (!uuid || (is_write && strlen(uuid) > PRODUCT_UUID_SIZE)) {
        return ERR_PARAMS;
    }

    if (is_write) {
        if (syscfg_read(CFG_PRODUCT_UUID_INDEX, temp, PRODUCT_UUID_SIZE) > 0) {
            if (strcmp(temp, exist) && strcmp(uuid, exist)) {
                return ERR_ALREADY_EXIST;
            }
            if (!memcmp(temp, uuid, PRODUCT_UUID_SIZE) && strcmp(exist, uuid)) {
                return ERR_SAME_DATA;
            }
        }
        return ((syscfg_write(CFG_PRODUCT_UUID_INDEX, uuid, strlen(uuid)) == strlen(uuid)) ? ERR_NULL : ERR_DEV_FAULT);
    } else {
        return (syscfg_read(CFG_PRODUCT_UUID_INDEX, uuid, PRODUCT_UUID_SIZE) ? ERR_NULL : ERR_DEV_FAULT);
    }
}


u8 product_sn_wr(u8 *sn, u8 is_write)
{
    u8 exist[] = "NULL";
    u8 temp[PRODUCT_SN_SIZE] =  {0};

    if (!sn || (is_write && strlen(sn) > PRODUCT_SN_SIZE)) {
        return ERR_PARAMS;
    }

    if (is_write) {
        if (syscfg_read(CFG_PRODUCT_SN_INDEX, temp, PRODUCT_SN_SIZE) > 0) {
            if (strcmp(temp, exist) && strcmp(sn, exist)) {
                return ERR_ALREADY_EXIST;
            }
            if (!memcmp(temp, sn, PRODUCT_SN_SIZE) && strcmp(exist, sn)) {
                return ERR_SAME_DATA;
            }
        }
        return ((syscfg_write(CFG_PRODUCT_SN_INDEX, sn, strlen(sn)) == strlen(sn)) ? ERR_NULL : ERR_DEV_FAULT);
    } else {
        return (syscfg_read(CFG_PRODUCT_SN_INDEX, sn, PRODUCT_SN_SIZE) ? ERR_NULL : ERR_DEV_FAULT);
    }
}


u8 product_rf_mac_wr(u8 *type, u8 *mac, u8 is_write)
{
    u8 exist[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8 rscorr, idx, *str[] = {"EDR", "BLE", "WIFI"}, temp[PRODUCT_MAC_SIZE];

    for (idx = 0; idx < ARRAY_SIZE(str); idx++) {
        if (!strcmp(type, str[idx])) {
            break;
        }
    }

    switch (idx) {
    case 0:
    case 2:
        if (is_write) {
            if (syscfg_read(CFG_BT_MAC_ADDR, temp, PRODUCT_MAC_SIZE) == PRODUCT_MAC_SIZE) {
                if (memcmp(temp, exist, PRODUCT_MAC_SIZE) && memcmp(exist, mac, PRODUCT_MAC_SIZE)) {
                    rscorr = ERR_ALREADY_EXIST;
                    break;
                }

                if (!memcmp(temp, mac, PRODUCT_MAC_SIZE) && memcmp(exist, mac, PRODUCT_MAC_SIZE)) {
                    rscorr = ERR_SAME_DATA;
                    break;
                }
            }
            rscorr = (syscfg_write(CFG_BT_MAC_ADDR, mac, PRODUCT_MAC_SIZE) == PRODUCT_MAC_SIZE) ? ERR_NULL : ERR_DEV_FAULT;
        } else {
            rscorr = (syscfg_read(CFG_BT_MAC_ADDR, mac, PRODUCT_MAC_SIZE) == PRODUCT_MAC_SIZE) ? ERR_NULL : ERR_DEV_FAULT;
        }
        break;

    case 1:
        if (is_write) {
            if (syscfg_read(CFG_BLE_MAC_ADDR, temp, PRODUCT_MAC_SIZE) == PRODUCT_MAC_SIZE) {
                if (memcmp(temp, exist, PRODUCT_MAC_SIZE) && memcmp(exist, mac, PRODUCT_MAC_SIZE)) {
                    rscorr = ERR_ALREADY_EXIST;
                    break;
                }

                if (!memcmp(temp, mac, PRODUCT_MAC_SIZE) && memcmp(exist, mac, PRODUCT_MAC_SIZE)) {
                    rscorr = ERR_SAME_DATA;
                    break;
                }
            }
            rscorr = (syscfg_write(CFG_BLE_MAC_ADDR, mac, PRODUCT_MAC_SIZE) == PRODUCT_MAC_SIZE) ? ERR_NULL : ERR_DEV_FAULT;
        } else {
            rscorr = (syscfg_read(CFG_BLE_MAC_ADDR, mac, PRODUCT_MAC_SIZE) == PRODUCT_MAC_SIZE) ? ERR_NULL : ERR_DEV_FAULT;
        }
        break;

    default:
        rscorr = ERR_NO_SUPPORT_DEV_CMD;
        break;
    }

    return rscorr;
}


u8 product_license_flag_wr(u8 *flag, u8 is_write)
{
    u32 temp;
    u8 rscorr = ERR_NULL, *data;

    if (!(data = zalloc(PRODUCT_LICENSE_INFO_SIZE))) {
        return ERR_MALLOC_FAIL;
    }

    if (is_write) {

    } else {
        *flag = 0;
        if (syscfg_read(CFG_PRODUCT_LICENSE_INDEX, data, PRODUCT_LICENSE_INFO_SIZE)) {
            memcpy(&temp, data, sizeof(temp));
            product_info("%s, license_len = %d\n", __FUNCTION__, temp);
            if (temp) {
                *flag = 1;
            }
        }
    }

    free(data);
    return rscorr;
}


u8 product_write_license(u8 idx, u8 *buf, u32 len, u32 file_size)
{
    u8 *data;
    u32 ret = 0, flag = file_size, temp;

    product_info("%s, idx = %d, len = %d, file_size = %d\n", __func__, idx, len, file_size);

    if (file_size > PRODUCT_LICENSE_INFO_SIZE) {
        return ERR_FILE_WR_NOSPACE;
    }

    if (!(data = zalloc(PRODUCT_LICENSE_INFO_SIZE))) {
        return ERR_MALLOC_FAIL;
    }

    if (syscfg_read(CFG_PRODUCT_LICENSE_INDEX, data, PRODUCT_LICENSE_INFO_SIZE)) {
        memcpy(&temp, data, sizeof(temp));
        if (temp) {
            free(data);
            return ERR_ALREADY_EXIST;
        }
    }

    memcpy(data, &flag, sizeof(flag));
    memcpy(data + sizeof(flag), buf, len);
    ret = syscfg_write(CFG_PRODUCT_LICENSE_INDEX, data, sizeof(flag) + len);
    free(data);
    return ((ret == (sizeof(flag) + len)) ? ERR_NULL : ERR_FILE_WR);
}


u8 product_read_license(u8 *idx, u8 *buf, u32 *len)
{
    u8 *data;
    u32 ret = 0, flag;

    *len = 0;

    if (!(data = zalloc(PRODUCT_LICENSE_INFO_SIZE))) {
        return ERR_MALLOC_FAIL;
    }

    if ((ret = syscfg_read(CFG_PRODUCT_LICENSE_INDEX, data, PRODUCT_LICENSE_INFO_SIZE))) {
        memcpy(&flag, data, sizeof(flag));
        product_info("%s, license_len = %d\n", __FUNCTION__, flag);
        if (flag) {
            *idx = 0;
            *len = flag;
            memcpy(buf, data + sizeof(flag), *len);
            free(data);
            return ERR_NULL;
        }
    }

    free(data);
    return ERR_FILE_WR;
}


u8 product_erase_license(void)
{
    u32 flag = 0;
    return syscfg_write(CFG_PRODUCT_LICENSE_INDEX, &flag, sizeof(flag)) == sizeof(flag) ? ERR_NULL : ERR_FILE_WR;
}


u8 product_erase_screens(void)
{
    int err = f_format("extflash", "jlfat", 32 * 1024);
    printf("format err : %d", err);
    return ERR_NULL;
}


u8 product_write_firmware(u32 idx, u8 *buf, u32 len, u32 file_size)
{
    product_info("%s, idx = %d, len = %d, file_size = %d\n", __func__, idx, len, file_size);
    return ERR_NULL;
}

u8 product_write_bootscreens(u8 idx, u8 *buf, u32 len, u32 file_size)
{
    product_info("%s, idx = %d, len = %d, file_size = %d\n", __func__, idx, len, file_size);
    static char *start_img = NULL;
    u32 num = file_size / 2048;
    char *img_buf = NULL;
    if (start_img == NULL) {
        start_img = zalloc(file_size);
    }

    if (idx == 0) {
        memcpy(start_img + (num - idx) * 2048, buf, len);
        printf("num - idx:%d", num - idx);
        FILE *fp;
        char logo_buffer[32];
        if (mount("extflash", "mnt/extflash", "jlfat", 32, NULL)) {
            printf("extflash mount succ");
            fp = fopen("mnt/extflash/C/poweron.jpg", "w+");
            if (fp) {
                printf("fopen succ\n");
                fwrite(start_img, file_size, 1, fp);
                fclose(fp);
                /* memset(logo_buffer, 0, sizeof(logo_buffer)); */
                /* fp = fopen("mnt/extflash/C/poweron.jpg", "r"); */
                /* fread(logo_buffer, sizeof(logo_buffer), 1, fp); */
                /* printf("photo head : \n"); */
                /* put_buf(logo_buffer, 32); */
                /* fclose(fp); */
            } else {
                printf("fopen poweron jpg err");
            }
        } else {
            printf("extflash mount failed!!!");
        }
        free(start_img);
        start_img = NULL;
    } else {
        memcpy(start_img + (num - idx) * 2048, buf, len);
        printf("num - idx:%d", num - idx);
    }

    return ERR_NULL;
}


u8 product_read_bootscreens(u8 *buf, u32 len)
{
    if (mount("extflash", "mnt/extflash", "jlfat", 32, NULL)) {
        printf("extflash mount succ");
        FILE *fp = fopen("mnt/extflash/C/poweron.jpg", "r");
        if (fp) {
            buf = malloc(flen(fp));
            len = flen(fp);
            if (buf == NULL) {
                printf("malloc error!");
                return -1;
            }
            fread(buf, len, 1, fp);
            fclose(fp);
        } else {
            printf("fopen poweron jpg err");
        }
    } else {
        printf("extflash mount failed!!!");
    }
    return ERR_NULL;
}


u8 product_write_shutdown_screens(u8 idx, u8 *buf, u32 len, u32 file_size)
{
    /* u32 lfs_addr, lfs_space; */
    product_info("%s, idx = %d, len = %d, file_size = %d\n", __func__, idx, len, file_size);
    static char *end_img = NULL;
    u32 num = file_size / 2048;
    char *img_buf = NULL;
    if (end_img == NULL) {
        end_img = malloc(file_size);
        memset(end_img, 0, file_size);
    }

    if (idx == 0) {
        memcpy(end_img + (num - idx) * 2048, buf, len);
        printf("num - idx:%d", num - idx);
        FILE *fp;
        char logo_buffer[32];
        if (mount("extflash", "mnt/extflash", "jlfat", 32, NULL)) {
            printf("extflash mount succ");
            fp = fopen("mnt/extflash/C/poweroff.jpg", "w+");
            if (fp) {
                printf("fopen succ\n");
                fwrite(end_img, file_size, 1, fp);
                fclose(fp);

                /* memset(logo_buffer, 0, sizeof(logo_buffer)); */
                /* fp = fopen("mnt/extflash/C/poweroff.jpg", "r"); */
                /* fread(logo_buffer, sizeof(logo_buffer), 1, fp); */
                /* printf("photo head : \n"); */
                /* put_buf(logo_buffer, 32); */
                /* fclose(fp); */
            } else {
                printf("fopen poweroff jpg err");
            }
        } else {
            printf("extflash mount failed!!!");
        }
        free(end_img);
        end_img = NULL;
    } else {
        memcpy(end_img + (num - idx) * 2048, buf, len);
        printf("num - idx:%d", num - idx);
    }

    return ERR_NULL;
}


u8 product_read_shutdown_screens(u8 *buf, u32 len)
{
    if (mount("extflash", "mnt/extflash", "jlfat", 32, NULL)) {
        printf("extflash mount succ");
        FILE *fp = fopen("mnt/extflash/C/poweroff.jpg", "r");
        if (fp) {
            buf = malloc(flen(fp));
            len = flen(fp);
            if (buf == NULL) {
                printf("malloc error!");
                return -1;
            }
            fread(buf, len, 1, fp);
            fclose(fp);
        } else {
            printf("fopen poweroff jpg err");
        }
    } else {
        printf("extflash mount failed!!!");
    }
    return ERR_NULL;
}


u8 product_read_user_data(u8 *idx, u8 *buf, u32 *len)
{
    u8 *data;
    u32 ret = 0, flag;

    *len = 0;

    if (!(data = zalloc(PRODUCT_USER_DATA_SIZE))) {
        return ERR_MALLOC_FAIL;
    }

    if ((ret = syscfg_read(CFG_PRODUCT_USER_INDEX, data, PRODUCT_USER_DATA_SIZE))) {
        memcpy(&flag, data, sizeof(flag));
        product_info("%s, user_data_len = %d\n", __FUNCTION__, flag);
        if (flag) {
            *idx = 0;
            *len = flag;
            memcpy(buf, data + sizeof(flag), *len);
            free(data);
            return ERR_NULL;
        }
    }

    free(data);
    return ERR_FILE_WR;
}


u8 product_write_user_data(u8 idx, u8 *buf, u32 len, u32 file_size)
{
    u8 *data;
    u32 ret = 0, flag = file_size, temp;

    product_info("%s, idx = %d, len = %d, file_size = %d\n", __func__, idx, len, file_size);

    if (file_size > PRODUCT_USER_DATA_SIZE) {
        return ERR_FILE_WR_NOSPACE;
    }

    if (!(data = zalloc(PRODUCT_USER_DATA_SIZE))) {
        return ERR_MALLOC_FAIL;
    }

#if 0
    if (syscfg_read(CFG_PRODUCT_USER_INDEX, data, PRODUCT_USER_DATA_SIZE)) {
        memcpy(&temp, data, sizeof(temp));
        if (temp) {
            free(data);
            return ERR_ALREADY_EXIST;
        }
    }
#endif

    memcpy(data, &flag, sizeof(flag));
    memcpy(data + sizeof(flag), buf, len);
    ret = syscfg_write(CFG_PRODUCT_USER_INDEX, data, sizeof(flag) + len);
    free(data);
    return ((ret == (sizeof(flag) + len)) ? ERR_NULL : ERR_FILE_WR);
}


u8 product_res_write(u8 *data, u32 len)
{
    return ((syscfg_write(CFG_PRODUCT_RES_INDEX, data, len) == len) ? ERR_NULL : ERR_FILE_WR);
}


u8 product_res_read(u8 *data, u32 len)
{
    return ((syscfg_read(CFG_PRODUCT_RES_INDEX, data, len) == len) ? ERR_NULL : ERR_FILE_WR);
}


u8 product_rtc_default_wr(struct product_rtc_time *time_, u8 is_write)
{
    if (is_write) {
        return (syscfg_write(CFG_PRODUCT_RTC_INDEX, time_, sizeof(struct sys_time)) == sizeof(struct sys_time)) ? ERR_NULL : ERR_DEV_FAULT;
    } else {
        /* default_time_get(time); */
        time_t now;
        struct tm *timeNow;
        time(&now);
        timeNow = localtime(&now);
        time_->year  = timeNow->tm_year;
        time_->month = timeNow->tm_mon;
        time_->day   = timeNow->tm_mday;
        time_->hour  = timeNow->tm_hour;
        time_->min   = timeNow->tm_min;
        time_->sec   = timeNow->tm_sec;
    }
    return ERR_NULL;
}


u8 product_write_options(json_object *options_obj)
{
    u16 array_cnt;
    u8 rscorr = ERR_NULL;
    json_object *sub_obj, *key_obj, *value_obj, *type_obj;

    if (!(array_cnt = json_object_array_length(options_obj))) {
        return ERR_PARAMS;
    }

    for (u16 i = 0; i < array_cnt; i++) {
        sub_obj   = json_object_array_get_idx(options_obj, i);
        key_obj   = json_object_object_get(sub_obj, "key");
        value_obj = json_object_object_get(sub_obj, "value");
        type_obj  = json_object_object_get(sub_obj, "type");
        //for test
        product_info("key = %s, value = %s, type = %s\n", json_object_get_string(key_obj), \
                     json_object_get_string(value_obj), \
                     json_object_get_string(type_obj));
        //根据配置项的具体内容进行处理
    }
    return rscorr;
}


u8 product_sd_get_info(u32 *status, u32 *cap_size, u32 *block_size)
{
    void *hdl = NULL;

    if (!hdl) {
        if (!(hdl = dev_open(SDX_DEV, NULL))) {
            return ERR_DEV_FAULT;
        }
    }

    dev_ioctl(hdl, IOCTL_GET_STATUS, status);
    if (*status) {
        dev_ioctl(hdl, IOCTL_GET_CAPACITY, cap_size);
        dev_ioctl(hdl, IOCTL_GET_BLOCK_SIZE, block_size);
    } else {
        *cap_size = 0;
        *block_size = 0;
    }

    dev_close(hdl);
    return ERR_NULL;
}


u8 product_sd_testfile_wr_check(void)
{
    FILE *fd;
    u8 rscorr;
    u8 r_data[64] = {0};
    u8 w_data[] = "product tool sd testfile write/read test";

    if (!storage_device_ready()) {
        return ERR_NO_SUPPORT_DEV;
    }

    if (!(fd = fopen(CONFIG_STORAGE_PATH"/C/testfile.txt", "w+"))) {
        return ERR_FILE_WR;
    }

    if (fwrite(w_data, sizeof(w_data), 1, fd) != sizeof(w_data)) {
        rscorr = ERR_FILE_WR;
        goto _sd_wr_check_exit_;
    }

    fclose(fd);
    fd = NULL;

    if (!(fd = fopen(CONFIG_STORAGE_PATH"/C/testfile.txt", "w+"))) {
        return ERR_FILE_WR;
    }

    if (fread(r_data, sizeof(w_data), 1, fd) != sizeof(w_data)) {
        rscorr = ERR_FILE_WR;
        goto _sd_wr_check_exit_;
    }

    product_info("read_data = %s\n", r_data);
    rscorr = strcmp(r_data, w_data) ? ERR_FILE_CHECK : ERR_NULL;

_sd_wr_check_exit_:
    fclose(fd);
    fd = NULL;

    return rscorr;
}


#if 0
u8 product_sd_write_file(u8 idx, u8 *buf, u32 len)
{
    u8 rscorr, *path;
    static FILE *fd = NULL;

    if (!storage_device_ready()) {
        return ERR_NO_SUPPORT_DEV;
    }

    if (!fd) {
        asprintf(&path, "%s/C/%s", CONFIG_STORAGE_PATH, PRODUCT_TESTFILE_NAME);
        fd = fopen(path, "w+");
        free(path);
        if (!fd) {
            return ERR_FILE_WR;
        }
    }

    rscorr = (fwrite(buf, len, 1, fd) == len) ? ERR_NULL : ERR_FILE_WR;
    if (!idx || (rscorr == ERR_FILE_WR)) {
        fclose(fd);
        fd = NULL;
    }

    return rscorr;
}


u8 product_sd_check_file(u16 crc)
{
    u32 total_size;
    FILE *fd = NULL;
    u8 rscorr, *data = NULL, *path = NULL;

    if (!storage_device_ready()) {
        return ERR_NO_SUPPORT_DEV;
    }

    asprintf(&path, "%s/C/%s", CONFIG_STORAGE_PATH, PRODUCT_TESTFILE_NAME);
    if (!(fd = fopen(path, "w+"))) {
        rscorr = ERR_FILE_CHECK;
        goto _sd_check_exit_;
    }

    if (!(total_size = flen(fd))) {
        rscorr = ERR_FILE_CHECK;
        goto _sd_check_exit_;
    }

    if (!(data = zalloc(total_size))) {
        rscorr = ERR_MALLOC_FAIL;
        goto _sd_check_exit_;
    }

    if (fread(data, total_size, 1, fd) != total_size) {
        rscorr = ERR_FILE_CHECK;
        goto _sd_check_exit_;
    }

    rscorr = (CRC16(data, total_size) == crc) ? ERR_NULL : ERR_FILE_CHECK;

_sd_check_exit_:
    if (path) {
        free(path);
    }

    if (data) {
        free(data);
    }

    if (fd) {
        fclose(fd);
    }

    return rscorr;
}
#endif


u8 product_battery_get_info(u8 *power_percent)
{
    *power_percent = 100;
    return ERR_NULL;
}


#ifdef CONFIG_UI_ENABLE

u8 product_lcd_get_info(u32 *width, u32 *height)
{
#if 0
    struct lcd_info info = {0};

    if (!(lcd_get_hdl()) || !(lcd_get_hdl()->get_screen_info)) {
        product_err("lcd interface NULL\n");
        return ERR_DEV_FAULT;
    }

    lcd_get_hdl()->get_screen_info(&info);
    *width = info.width;
    *height = info.height;
#endif // 0
    *width = LCD_W;
    *height = LCD_H;
    return ERR_NULL;
}

#if TCFG_LCD_INPUT_FORMAT==LCD_IN_RGB565
#define PIXEL_COLOR_T u16
#elif TCFG_LCD_INPUT_FORMAT==LCD_IN_RGB888
#define PIXEL_COLOR_T u32
#elif TCFG_LCD_INPUT_FORMAT==LCD_IN_ARGB888
#define PIXEL_COLOR_T u32
#elif TCFG_LCD_INPUT_FORMAT==LCD_IN_YUV422
#define PIXEL_COLOR_T u16
#endif

#define IMG_WIDTH       LCD_W
#define IMG_HEIGHT      LCD_H

static void *lcd_dev;
static u16 rotate;
static PIXEL_COLOR_T fb_buf[2][IMG_WIDTH * IMG_HEIGHT] __attribute__((aligned(32)));
static PIXEL_COLOR_T fb_to_rotate[IMG_WIDTH * IMG_HEIGHT] __attribute__((aligned(32))); ///< 旋转buf
volatile static void *next_disp_fb;   ///< next_fb地址
static void *cur_disp_fb;             ///< 指向当前LCD的显示显存
static void *cur_draw_fb = fb_buf[0]; ///< 指向当前绘制中的显示显存
static void *lcd_frame_end_hook_func(void)
{
    if (!next_disp_fb) {
        return NULL; ///< 软件还未渲染完整一帧，继续显示上一帧显存
    }
    cur_disp_fb = (void *)next_disp_fb;
    next_disp_fb = NULL;
    return cur_disp_fb;
}

void lcd_calc_draw_rate(void)
{
    static u32 time_lapse_hdl;
    static u8 fps_cnt;
    ++fps_cnt;
    u32	tdiff = time_lapse(&time_lapse_hdl, 1000);
    if (tdiff) {
        printf("lcd draw %d fps\n", fps_cnt *  1000 / tdiff);
        fps_cnt = 0;
    }
}

static void *lcd_get_draw_buf(void)
{
    return cur_draw_fb;
}

static void lcd_draw_finish(void)
{
    static u8 start_flag;
#if 0
    // 计算图像绘制帧率
    lcd_calc_draw_rate();
#endif

    // 一帧渲染完，可以同步cur_draw_fb到SDRAM,如果调用该函数前已经同步过这里就不需要
    DcuFlushRegion(cur_draw_fb, sizeof(fb_buf) / 3);

    if (!start_flag) {
        cur_disp_fb = fb_buf[0];///< LCD第一轮渲染在BUF0，因此设置为第一个推屏显存
        cur_draw_fb = fb_buf[1];///< 切换LCD第二轮渲染在BUF1
        dev_ioctl(lcd_dev, IOCTL_LCD_RGB_START_DISPLAY, (u32)cur_disp_fb); ///< 启动推屏显示
        start_flag = 1;
    }

    void *prev_disp_fb = cur_disp_fb;
    next_disp_fb = cur_draw_fb;
    dev_ioctl(lcd_dev, IOCTL_LCD_RGB_WAIT_FB_SWAP_FINISH, 0);
    cur_draw_fb = prev_disp_fb;///< 把上一帧显示完成的显存配置为下一帧绘制的显存
}

static void lcd_color_switch(void *priv)
{
    static unsigned int time_out = 0;
    const u8 lcd_in_format[] = {
        FB_COLOR_FORMAT_YUV422,
        FB_COLOR_FORMAT_RGB565,
        FB_COLOR_FORMAT_RGB888,
        FB_COLOR_FORMAT_ARGB8888
    };

    PIXEL_COLOR_T *buf_to_draw; ///< 要绘制图像的buf地址
    PIXEL_COLOR_T *buf_idle;
    struct lcd_dev_drive *lcd;

    // 打开lcd
    if (lcd_dev == NULL) {
        lcd_dev = dev_open("lcd", NULL);
        dev_ioctl(lcd_dev, IOCTL_LCD_RGB_SET_ISR_CB, (u32)lcd_frame_end_hook_func);
        ASSERT(lcd_dev != NULL);

        // 获取旋转配置
        dev_ioctl(lcd_dev, IOCTL_LCD_RGB_GET_LCD_HANDLE, (u32)&lcd);
        rotate = lcd->dev->imd.info.rotate;
        printf("rotate = %d\n", rotate);
    }

    for (;;) {
        time_out++;
        buf_idle = (PIXEL_COLOR_T *)lcd_get_draw_buf();

        if (rotate != ROTATE_0) {
            buf_to_draw = fb_to_rotate; ///< 旋转图像需要多一块buf
        } else {
            buf_to_draw = buf_idle;
        }

        // 绘制图像(等效UI绘制)
        if (time_out <= 40) {
            for (int y = 0; y < IMG_HEIGHT; y++) {
                for (int x = 0; x < IMG_WIDTH; x++) {
                    int index = y * IMG_WIDTH + x;
                    int r = 0;
                    int g = 255;
                    int b = 0;
                    PIXEL_COLOR_T color = (r << 11) | ((g >> 2) << 5) | (b);
                    buf_to_draw[index] = color;
                }
            }
        } else if (time_out <= 80) {
            for (int y = 0; y < IMG_HEIGHT; y++) {
                for (int x = 0; x < IMG_WIDTH; x++) {
                    int index = y * IMG_WIDTH + x;
                    int r = 255;
                    int g = 0;
                    int b = 0;
                    PIXEL_COLOR_T color = (r << 11) | ((g >> 2) << 5) | (b);
                    buf_to_draw[index] = color;
                }
            }
        } else if (time_out <= 120) {
            for (int y = 0; y < IMG_HEIGHT; y++) {
                for (int x = 0; x < IMG_WIDTH; x++) {
                    int index = y * IMG_WIDTH + x;
                    int r = 0;
                    int g = 0;
                    int b = 255;
                    PIXEL_COLOR_T color = (r << 11) | ((g >> 2) << 5) | (b);
                    buf_to_draw[index] = color;
                }
            }
        } else {
            time_out = 0;
        }

        // 旋转buf空间
        if (rotate != ROTATE_0) {
            extern int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width,
                                           int src_height, int src_stride, int dst_width,
                                           int dst_height, int dst_stride, int degree, int xoffset,
                                           int yoffset, int in_format, int out_format, uint8_t mirror);

            fb_frame_buf_rotate((u8 *)buf_to_draw,                    ///< 源头图层地址
                                (u8 *)buf_idle,                       ///< 目标图层地址
                                IMG_WIDTH,                            ///< 输入图层宽度
                                IMG_HEIGHT,                           ///< 输入图层高度
                                0,                                    ///< 输入图层跨度
                                IMG_HEIGHT,                           ///< 输出图层宽度
                                IMG_WIDTH,                            ///< 输出图层高度
                                0,                                    ///< 输出图层跨度
                                rotate,                               ///< 图层旋转角度
                                0,                                    ///< 图层旋转后x偏移
                                0,                                    ///< 图层旋转后y偏移
                                lcd_in_format[TCFG_LCD_INPUT_FORMAT], ///< 输入图层格式
                                lcd_in_format[TCFG_LCD_INPUT_FORMAT], ///< 输出图层格式
                                0                                     ///< 输出镜像,1:垂直镜像
                               );
        }

        lcd_draw_finish();
        if (os_sem_accept(&__THIS->lcd_sem) > 0) {
            for (int i = 0; i < 2; i++) {
                buf_idle = (PIXEL_COLOR_T *)lcd_get_draw_buf();
                if (rotate != ROTATE_0) {
                    buf_to_draw = fb_to_rotate; ///< 旋转图像需要多一块buf
                } else {
                    buf_to_draw = buf_idle;
                }
                for (int y = 0; y < IMG_HEIGHT; y++) {
                    for (int x = 0; x < IMG_WIDTH; x++) {
                        int index = y * IMG_WIDTH + x;
                        int r = 0;                    // 红色关闭
                        int g = 0;                    // 绿色
                        int b = 0;                    // 蓝色关闭
                        PIXEL_COLOR_T color = (r << 11) | ((g >> 2) << 5) | (b);
                        buf_to_draw[index] = color;
                    }
                }

                // 旋转buf空间
                if (rotate != ROTATE_0) {
                    extern int fb_frame_buf_rotate(uint8_t *image_src, uint8_t *image_dst, int src_width,
                                                   int src_height, int src_stride, int dst_width,
                                                   int dst_height, int dst_stride, int degree, int xoffset,
                                                   int yoffset, int in_format, int out_format, uint8_t mirror);

                    fb_frame_buf_rotate((u8 *)buf_to_draw,                    ///< 源头图层地址
                                        (u8 *)buf_idle,                       ///< 目标图层地址
                                        IMG_WIDTH,                            ///< 输入图层宽度
                                        IMG_HEIGHT,                           ///< 输入图层高度
                                        0,                                    ///< 输入图层跨度
                                        IMG_HEIGHT,                           ///< 输出图层宽度
                                        IMG_WIDTH,                            ///< 输出图层高度
                                        0,                                    ///< 输出图层跨度
                                        rotate,                               ///< 图层旋转角度
                                        0,                                    ///< 图层旋转后x偏移
                                        0,                                    ///< 图层旋转后y偏移
                                        lcd_in_format[TCFG_LCD_INPUT_FORMAT], ///< 输入图层格式
                                        lcd_in_format[TCFG_LCD_INPUT_FORMAT], ///< 输出图层格式
                                        0                                     ///< 输出镜像,1:垂直镜像
                                       );
                }
                lcd_draw_finish();
//                os_time_dly(10);
            }
            return;
        }
//        os_time_dly(10);
    }
#if 0
    u8 *buf = (u8 *)priv;
    struct lcd_info info = {0};
    u16 color[] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFE0};

    os_sem_set(&__THIS->lcd_sem, 0);

    if (!(lcd_get_hdl()) || \
        !(lcd_get_hdl()->backlight_ctrl) || \
        !(lcd_get_hdl()->get_screen_info)) {
        product_err("lcd interface NULL\n");
        return;
    }

    lcd_get_hdl()->backlight_ctrl(true);
    lcd_get_hdl()->get_screen_info(&info);

    if (!info.width || !info.height) {
        product_err("lcd info NULL\n");
        return ;
    }

    for (;;) {
        for (u8 i = 0; i < ARRAY_SIZE(color); i++) {
            for (u32 j = 0; j < info.width * info.height; j++) {
                buf[2 * j] = (color[i] >> 8) & 0xff;
                buf[2 * j + 1] = color[i] & 0xff;
            }
            lcd_show_frame_to_dev(buf, info.width * info.height * 2);
            if (os_sem_accept(&__THIS->lcd_sem) > 0) {
                free(buf);
                return;
            }
//             os_time_dly(100);
        }
    }
#endif // 0
}




u8 product_lcd_color_test(u8 on)
{
#if 1
    u8 *buf;
    static int pid = 0;
//     struct lcd_info info = {0};

    if (on) {
        if (pid) {
            return ERR_NULL;
        }

//         if (!(lcd_get_hdl()) || !(lcd_get_hdl()->get_screen_info)) {
//             product_err("lcd interface NULL\n");
//             return ERR_DEV_FAULT;
//         }
//
//         lcd_get_hdl()->get_screen_info(&info);
//         if (!(buf = zalloc(info.width * info.height * 2))) {
//             return ERR_MALLOC_FAIL;
//         }
        thread_fork("lcd_color_switch", 6, 512, 0, &pid, lcd_color_switch, (void *)buf);
    } else {
        os_sem_post(&__THIS->lcd_sem);
        pid = 0;
    }
    return ERR_NULL;
#endif // 0
}


u8 product_lcd_init(void)
{
#if 0
    struct lcd_info info = {0};
    extern const struct ui_devices_cfg ui_cfg_data;

    if (!(lcd_get_hdl()) || !(lcd_get_hdl()->init) || !(lcd_get_hdl()->get_screen_info)) {
        product_err("lcd interface NULL\n");
        return ERR_DEV_FAULT;
    }

    if (lcd_get_hdl()->init(&ui_cfg_data)) {
        product_err("lcd interface NULL\n");
        return ERR_DEV_FAULT;
    }

    lcd_get_hdl()->get_screen_info(&info);
    if (!info.width || !info.height) {
        product_err("lcd info NULL\n");
        return ERR_DEV_FAULT;
    }
#endif // 0
    os_sem_create(&__THIS->lcd_sem, 0);
    return ERR_NULL;

}


#endif


u8 product_camera_get_info(struct procudt_camera_info *info)
{
#if 0
    //void *video_dev;
    static void *video_dev = NULL;
    struct camera_device_info camera_info;

    if (!video_dev) {
        video_dev = dev_open("video0.0", NULL);
        if (!video_dev) {
            return ERR_DEV_FAULT;
        }
    }

    dev_ioctl(video_dev, CAMERA_GET_SENSOR_TYPE, (u32)info->name);
    dev_ioctl(video_dev, CAMERA_GET_SENSOR_INFO, (u32)&camera_info);

    info->fps    = camera_info.fps;
    info->width  = camera_info.width;
    info->height = camera_info.height;

    //dev_close(video_dev); //此处关闭会导致UVC断开
#endif
    return ERR_NULL;
}


u8 product_camera_reg_wr(u32 addr, u32 *value, u8 is_write, u8 off)
{
#if 0
    u32 cmd, wr_data = 0;
    static void *video_dev = NULL;

//    product_info("%s, addr = %x\n", __func__, addr);
    if (!video_dev) {
        video_dev = dev_open("video0.0", NULL);
        if (!video_dev) {
            return ERR_DEV_FAULT;
        }
    }

    if (off) {
        if (video_dev) {
            //dev_close(video_dev); //此处关闭会导致UVC断开
        }
        video_dev = NULL;
        return ERR_NULL;
    }

    if (is_write) {
        cmd = CAMERA_WITE_BIT | addr;
        wr_data = *value;
        dev_ioctl(video_dev, cmd, (u32)wr_data);
        wr_data = 0;
    } else {
        cmd = CAMERA_READ_BIT | addr;
        dev_ioctl(video_dev, cmd, (u32)&wr_data);
        *value = wr_data;
    }
#endif
    return ERR_NULL;
}


u8 product_camera_ntv_ctl(u8 on)
{
    return ERR_NULL;
}


u8 product_camera_light_ctl(u8 on)
{
    return ERR_NULL;
}


u8 product_pir_init(void)
{
    os_sem_create(&__THIS->pir_sem, 0);

    //添加PIR初始化
    //
    //
    return ERR_NULL;
}


static u8 product_pir_status_get(void)
{
    //PIR 触发返回DEV_STATUS_ON, 否则返回DEV_STATUS_OFF
    //for test
    static u8 status = 0;
    status ^= 1;
    return status;
}


static void pir_monitor_task(void *priv)
{
    u8 *pirstr;
    static u8 status = 0, last_status = 0;

    for (;;) {
        last_status = status;
        status = product_pir_status_get();
        if (status != last_status) {
            asprintf(&pirstr, "{\"opcode\":\"%d\",\"rscorr\":\"%d\",\"params\":{\"id\":\"%d\",\"type\":\"%d\",\"cmd\":\"%d\",\"args\":{\"pir_status\":\"%d\"}}}", \
                     OPC_DEV_CTL, ERR_NULL, 0, DEV_TYPE_PIR, CTL_DEV_MONITOR_RSP, status);

            product_info("--->resp pir_status\n\n%s\n\n", pirstr);
            data_respond(0, DATA_TYPE_OPCODE, pirstr, strlen(pirstr));

            if (pirstr) {
                free(pirstr);
            }
        }

        if (os_sem_accept(&__THIS->pir_sem) > 0) {
            return;
        }

        os_time_dly(DEV_MONITOR_PERIOD);
    }
}


u8 product_pir_monitor_ctl(u8 on)
{
    static int pid = 0;
    if (on && !pid) {
        os_sem_set(&__THIS->pir_sem, 0);
        thread_fork("pir_monitor_task", 6, 512, 0, &pid, pir_monitor_task, NULL);
    } else {
        os_sem_post(&__THIS->pir_sem);
        pid = 0;
    }
    return ERR_NULL;
}


u8 product_motor_init(void)
{
    return ERR_NULL;
}


u8 product_motor_ctl(u8 id, u8 cmd, int flag, int step)
{
    u8 rscorr = ERR_NULL;
    //flag == TRUE,为步进电机,否则为普通电机
    //id == 0, 马达设备
    //id == 1, IRCUT电机
    product_info("%s, id = %d, cmd = %d, flag = %d, step = %d\n", __FUNCTION__, id, cmd, flag, step);

    switch (cmd) {
    case CTL_MOTOR_LEFT:
        break;

    case CTL_MOTOR_RIGHT:
        break;

    case CTL_MOTOR_UP:
        break;

    case CTL_MOTOR_DOWN:
        break;

    case CTL_MOTOR_STOP:
        break;

    default:
        rscorr = ERR_NO_SUPPORT_DEV_CMD;
        break;
    }
    return rscorr;
}


u8 product_gsensor_init(void)
{
    os_sem_create(&__THIS->gsensor_sem, 0);

    //添加Gsensor初始化
    //
    //
    return ERR_NULL;
}


static u8 product_gsensor_status_get(void)
{
    //达到gsensor设定阈值触发返回DEV_STATUS_ON, 否则返回DEV_STATUS_OFF
    //for test
    static u8 status = 0;
    status ^= 1;
    return status;
}


static void gsensor_monitor_task(void *priv)
{
    u8 *str;
    static u8 status = 0, last_status = 0;

    for (;;) {
        last_status = status;
        status = product_gsensor_status_get();
        if (status != last_status) {
            asprintf(&str, "{\"opcode\":\"%d\",\"rscorr\":\"%d\",\"params\":{\"id\":\"%d\",\"type\":\"%d\",\"cmd\":\"%d\",\"args\":{\"gsensor_status\":\"%d\"}}}", \
                     OPC_DEV_CTL, ERR_NULL, 0, DEV_TYPE_GSENSOR, CTL_DEV_MONITOR_RSP, status);

            product_info("--->resp gsensor_status\n\n%s\n\n", str);
            data_respond(0, DATA_TYPE_OPCODE, str, strlen(str));

            if (str) {
                free(str);
            }
        }

        if (os_sem_accept(&__THIS->gsensor_sem) > 0) {
            return;
        }

        os_time_dly(DEV_MONITOR_PERIOD);
    }
}


u8 product_gesnsor_monitor_ctl(u8 on)
{
    static int pid = 0;
    if (on && !pid) {
        os_sem_set(&__THIS->gsensor_sem, 0);
        thread_fork("gsensor_monitor_task", 6, 512, 0, &pid, gsensor_monitor_task, NULL);
    } else {
        os_sem_post(&__THIS->gsensor_sem);
        pid = 0;
    }
    return ERR_NULL;
}


//for test
static void product_touchpanel_test(void *priv)
{
    static int x = 0, y = 0;
    x++;
    y--;
    product_touchpanel_coord_post(x, y);
}


u8 product_touchpanel_init(void)
{
    os_sem_create(&__THIS->touchpanel_sem, 0);
    //添加Gsensor初始化
    //
    //for test, delete
    sys_timer_add(NULL, product_touchpanel_test, 20);
    return ERR_NULL;
}


void product_touchpanel_coord_post(int x, int y)
{
    if (!__THIS->touchpanel_mflag) {
        return;
    }

    int msg[2];
    msg[0] = x;
    msg[1] = y;
    os_taskq_post_type("touchpanel_monitor_task", 0, ARRAY_SIZE(msg), msg);
}


static void touchpanel_monitor_task(void *priv)
{
    u8 *str;
    int msg[3];

    for (;;) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        asprintf(&str, "{\"opcode\":\"%d\",\"rscorr\":\"%d\",\"params\":{\"id\":\"%d\",\"type\":\"%d\",\"cmd\":\"%d\",\"args\":{\"x\":\"%d\",\"y\":\"%d\"}}}", \
                 OPC_DEV_CTL, ERR_NULL, 0, DEV_TYPE_TOUCHPANEL, CTL_DEV_MONITOR_RSP, msg[1], msg[2]);

        product_info("--->resp touchpannel_status\n\n%s\n\n", str);
        data_respond(0, DATA_TYPE_OPCODE, str, strlen(str));

        if (str) {
            free(str);
        }

        if (os_sem_accept(&__THIS->touchpanel_sem) > 0) {
            return;
        }
        os_time_dly(DEV_MONITOR_PERIOD);
    }
}


u8 product_touchpanel_monitor_ctl(u8 on)
{
    int msg[2] = {0};
    static int pid = 0;
    if (on && !pid) {
        os_sem_set(&__THIS->touchpanel_sem, 0);
        thread_fork("touchpanel_monitor_task", 6, 512, 512, &pid, touchpanel_monitor_task, NULL);
        __THIS->touchpanel_mflag = 1;
    } else {
        __THIS->touchpanel_mflag = 0;
        os_sem_post(&__THIS->touchpanel_sem);
        os_taskq_post_type("touchpanel_monitor_task", 0, ARRAY_SIZE(msg), msg);
        pid = 0;
    }
    return ERR_NULL;
}


int product_key_event_handler(struct sys_event *event)
{
    char *str, *value_str = NULL;
    struct key_event *key = (struct key_event *)event->payload;
    const struct {
        u8 key_value;
        char value_str[16];
    } product_keyvalue_str[] = {
        {
            .key_value = 13,
            .value_str = "K1",
        },
        {
            .key_value = 6,
            .value_str = "K2",
        },
        {
            .key_value = 4,
            .value_str = "K3",
        },
        {
            .key_value = 5,
            .value_str = "K4",
        },
        {
            .key_value = 12,
            .value_str = "K5",
        },
        {
            .key_value = 10,
            .value_str = "K6",
        },
        {
            .key_value = 8,
            .value_str = "K7",
        },
        {
            .key_value = 0,
            .value_str = "K8",
        }
    };

    const char *product_keyaction_str[] = {
        "KEY_CLICK",
        "KEY_LONG_PRESS",
        "KEY_HOLD_PRESS",
        "KEY_UP",
        "KEY_DOUBLE_CLICK",
        "KEY_TRIPLE_CLICK",
        "KEY_FOURTH_CLICK",
        "KEY_FIRTH_CLICK",
    };

    if (event->type == SYS_KEY_EVENT) {
        for (u8 idx = 0; idx < ARRAY_SIZE(product_keyvalue_str); idx++) {
            if (product_keyvalue_str[idx].key_value == key->value) {
                value_str = product_keyvalue_str[idx].value_str;
                break;
            }
        }
        asprintf(&str, "{\"opcode\":\"%d\",\"rscorr\":\"%d\",\"params\":{\"id\":\"%d\",\"type\":\"%d\",\"cmd\":\"%d\",\"args\":{\"key_value\":\"%s\",\"key_action\":\"%s\"}}}", \
                 OPC_DEV_CTL, ERR_NULL, 0, DEV_TYPE_KEYPAD, CTL_DEV_MONITOR_RSP, value_str ? value_str : "NO DEFINED KEY", product_keyaction_str[key->action]);

        product_info("--->resp keypad_status\n\n%s\n\n", str);
        data_respond(0, DATA_TYPE_OPCODE, str, strlen(str));

        if (str) {
            free(str);
        }
    }
}


u8 product_led_ctl(u8 cmd)
{
    u8 rscorr = ERR_NULL;
    product_info("%s, cmd = %d\n", __FUNCTION__, cmd);

    switch (cmd) {
    case CTL_LED_ON:
        break;

    case CTL_LED_OFF:
        break;

    default:
        rscorr = ERR_NO_SUPPORT_DEV_CMD;
        break;
    }
    return rscorr;
}
#include "action.h"
#include "net_server.h"
int product_tcp_video_start()
{
    struct intent it;
    init_intent(&it);
    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO0_OPEN_RT_STREAM;

    char mark = 2;
    struct rt_stream_app_info info;

    info.width = 640;
    info.height = 480;

    info.fps    = NET_VIDEO_REC_FPS0;//NET_VIDEO_REC_FPS1
    info.type = NET_VIDEO_FMT_AVI;
    info.priv = 0;

    it.data = (const char *)&mark;//打开视频
    it.exdata = (u32) &info; //视频参数

    start_app(&it);
}

#endif


