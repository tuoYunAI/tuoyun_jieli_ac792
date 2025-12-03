#include "typedef.h"
#include "app_config.h"
#include "fs/sdfile.h"
#include "fs/resfile.h"
#include "boot.h"
#include "asm/sfc_norflash_api.h"
#include "system/includes.h"
#include "generic/dlog.h"
#include "asm/rtc.h"
#include "asm/spi.h"
#include "device/gpio.h"
/* #include "norflash.h" */

#if TCFG_DEBUG_DLOG_ENABLE
extern u32 __attribute__((weak)) dlog_log_data_start_addr[];

struct sys_time dlog_sys_time = {0};

__attribute__((weak))
int dlog_output_direct(void *buf, u16 len)
{
    return 0;
}

#if 0
void dlog_print_test1(void *priv)
{
    static int cnt[3] = {0};
    char time[15] = {0};
    int index = (int)priv;

    extern int log_print_time_to_buf(char *time);
    log_print_time_to_buf(time);

    /* printf("%stask:%s, cnt:%d\n", time, os_current_task(), cnt[index]); */
    dlog_printf("%stask:%s, cnt:%d\n", time, os_current_task(), cnt[index]);
    /* dlog_printf("%stask:%s, %s, %s, %s, %s, cnt:%d\n", time, os_current_task(), __func__, __FILE__, __FILE__, __FILE__, cnt[index]); */
    /* printf("%stask:%s, %s, %s, %s, %s, cnt:%d\n", time, os_current_task(), __func__, __FILE__, __FILE__, __FILE__, cnt[index]); */
    cnt[index]++;
}

void dlog_print_in_bttask()
{
    printf("%s, %d", __FUNCTION__, __LINE__);
    sys_timer_add((void *)1, dlog_print_test1, 188);
}

void dlog_print_in_btctrler()
{
    printf("%s, %d", __FUNCTION__, __LINE__);
    sys_timer_add((void *)2, dlog_print_test1, 115);
}

void dlog_print_test(void *priv)
{
    extern void dlog_uart_init();
    dlog_uart_init();
    sys_timer_add((void *)0, dlog_print_test1, 200);
#if 1
    int msg[] = {(int)dlog_print_in_bttask, 0};
    os_taskq_post_type("btstack", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    int msg2[] = {(int)dlog_print_in_btctrler, 0};
    os_taskq_post_type("btctrler", Q_CALLBACK, ARRAY_SIZE(msg2), msg2);
#endif
}
#endif

#if TCFG_DEBUG_DLOG_FLASH_SEL // 外置flash获取其它自定义的区域

struct _dlog_ext_flash_ {
    void *spi;
    int cs_gpio;
};

static struct _dlog_ext_flash_ hdl = {0};

#define FLASH_PAGE_SIZE 256

#define WINBOND_READ_DATA	        0x03
#define WINBOND_DUAL_READ_DATA	    0x3b
#define WINBOND_DUAL_READ	        0xbb
#define WINBOND_READ_SR1            0x05
#define DUMMY_BYTE                  0xff
#define WINBOND_CHIP_ERASE          0xC7
#define W25X_JedecDeviceID          0x9f
#define WINBOND_WRITE_ENABLE        0x06
#define WINBOND_PAGE_PROGRAM_QUAD	0x32
#define W25X_SectorErase		    0x20
#define W25X_PageProgram         	0x02

static void cs_gpio(int sta)
{
    gpio_direction_output(hdl.cs_gpio, sta);
}
static void spiflash_send_write_enable(void)
{
    cs_gpio(0);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, WINBOND_WRITE_ENABLE);
    cs_gpio(1);
}
static int W25X_GetChipID(void)
{
    u8 id[0x3] = {0, 0, 0};
    int id_mub = 0;
    int err = 0;

    cs_gpio(0);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, W25X_JedecDeviceID);

    for (u8 i = 0; i < sizeof(id); i++) {
        err = dev_ioctl(hdl.spi, IOCTL_SPI_READ_BYTE, (u32)&id[i]);
        printf(">>>>>>>>>>id = 0x%x", id[i]);
        if (err) {
            printf(">>>>>>>>>>>ERR");
            id[0] = id[1] = id[2] = 0xff;
            break;
        }
    }

    cs_gpio(1);

    id_mub = id[0] << 16 | id[1] << 8 | id[2];

#define W25Q16 0XEF4015
#define W25Q32 0XEF4016
#define W25Q64 0XEF4017
#define W25Q128 0XEF4018
#define W25Q256 0XEF4019

    switch (id_mub) {
    case W25Q16:
        printf("\n>>>>>flash IC is W25Q16\n");
        break;

    case W25Q32:
        printf("\n>>>>>flash IC is W25Q32\n");
        break;

    case W25Q64:
        printf("\n>>>>>flash IC is W25Q64\n");
        break;

    case W25Q128:
        printf("\n>>>>>flash IC is W25Q128\n");
        break;

    case W25Q256:
        printf("\n>>>>>flash IC is W25Q256\n");
        break;

    default:
        printf("\n>>>>>no flash\n");
        /* err = -1; */
        break;
    }

    return err;
}

static int spiflash_wait_ok(void)
{
    u8 state = 0;

    cs_gpio(0);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, WINBOND_READ_SR1);
    dev_ioctl(hdl.spi, IOCTL_SPI_READ_BYTE, (u32)&state);
    cs_gpio(1);
    os_time_dly(1);
    while ((state & 0x01) == 0x01) {
        cs_gpio(0);
        dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, WINBOND_READ_SR1);
        dev_ioctl(hdl.spi, IOCTL_SPI_READ_BYTE, (u32)&state);
        cs_gpio(1);
        os_time_dly(1);
    }
    return 0;
}

static void spiflash_send_addr(u32 addr)
{
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, addr >> 16);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, addr >> 8);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, addr);
}

static void SectorErase(u32 addr)
{
    spiflash_send_write_enable();
    cs_gpio(0);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, W25X_SectorErase);//擦除当前页
    addr &= ~(4096 - 1); //4k对齐
    spiflash_send_addr(addr) ;
    cs_gpio(1);
    spiflash_wait_ok();
}

static void write_data(u8 *buf, u32 addr, u32 len)
{
    spiflash_send_write_enable();
    cs_gpio(0);
    dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, W25X_PageProgram);//向当前页写入数据
    spiflash_send_addr(addr);
    dev_write(hdl.spi, buf, len);
    cs_gpio(1);
    spiflash_wait_ok();
}

static s32 spiflash_write(u8 *buf, u32 addr, u32 len)
{

    s32 pages = len >> 8;
    u32 waddr = addr;

    //每次最多写入SPI_FLASH_PAGE_SIZE字节
    SectorErase(waddr);
    do {

        write_data(buf, waddr, FLASH_PAGE_SIZE);
        waddr += FLASH_PAGE_SIZE;
        buf	 += FLASH_PAGE_SIZE;
    } while (pages--);

    return 0;
}

static int spiflash_read(u8 *buf, u32 addr, u16 len)
{

    int err = 0;
    cs_gpio(0);
    err = dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, WINBOND_READ_DATA);

    if (err == 0) {
        spiflash_send_addr(addr);
        /* dev_bulk_read(hdl.spi, buf, addr, len); */
        for (int i = 0; i < len; i++) {
            dev_ioctl(hdl.spi, IOCTL_SPI_READ_BYTE, (u32)(buf + i));
        }
    }
    cs_gpio(1);
    return err ? -EFAULT : len;
}

//spi双线使用这个接口读
static int spiflash_dual_read(u8 *buf, u32 addr, u16 len)
{
    int err = 0;
    cs_gpio(0);
    err = dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, WINBOND_DUAL_READ_DATA);
    if (err == 0) {
        spiflash_send_addr(addr);
        dev_ioctl(hdl.spi, IOCTL_SPI_SEND_BYTE, WINBOND_DUAL_READ);
        dev_bulk_read(hdl.spi, buf, addr, len);
    }
    cs_gpio(1);
    return err ? -EFAULT : len;
}
static void spi_open(void)
{
    hdl.cs_gpio = IO_PORTC_05;
    hdl.spi = dev_open("spi1", NULL);

    if (hdl.spi == NULL) {
        printf(">>>>>>>>>>open_fail");
        hdl.cs_gpio = -1;
        return;
    }
    cs_gpio(1);
    dev_ioctl(hdl.spi, IOCTL_SPI_SET_USE_SEM, 0); //不使用信号量
    dev_ioctl(hdl.spi, IOCTL_SPI_SET_ASYNC_SEND, 0); //同步模式
}
static void spi_close(void)
{
    if (hdl.spi) {
        dev_close(hdl.spi);
        hdl.spi = NULL;
    }
}

u8 dlog_use_ex_flash = 0;

/*----------------------------------------------------------------------------*/
/**@brief 获取保存dlog数据的flash起始地址和大小
  @param  addr: 返回起始地址
  @param  len: 返回长度
  @param  offset: 数据需要写入的flash地址
  @return 等于0表示成功; 小于0表示失败
  @note   起始地址和长度必需 4K 对齐
 */
/*----------------------------------------------------------------------------*/
static int dlog_get_ex_flash_zone(u32 *addr, u32 *len)
{
    // 需要实现
    if (addr) {
        *addr = TCFG_DLOG_FLASH_START_ADDR + (u32)dlog_log_data_start_addr;
    }
    if (len) {
        *len = TCFG_DLOG_FLASH_REGION_SIZE - (u32)dlog_log_data_start_addr;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief 将flash的指定扇区擦除
  @param  erase_sector: 要擦除的扇区偏移(偏移是相对于保存dlog数据的flash区域起始地址)
  @param  sector_num: 要擦除的扇区个数
  @param  offset: 数据需要写入的flash地址
  @return 等于0表示成功; 小于0表示失败
  @note
 */
/*----------------------------------------------------------------------------*/
static int dlog_ex_flash_zone_erase(u16 erase_sector, u16 sector_num)
{
    // 需要实现
    /* printf("%s, %d, %d", __func__, erase_sector, sector_num); */

    if (dlog_use_ex_flash) {
        for (int i = 0; i < sector_num; i++) {
            /* norflash_erase(0, IOCTL_ERASE_SECTOR, (erase_sector + i) * LOG_BASE_UNIT_SIZE + TCFG_DLOG_FLASH_START_ADDR + (u32)dlog_log_data_start_addr); */
            SectorErase((erase_sector + i) * LOG_BASE_UNIT_SIZE + TCFG_DLOG_FLASH_START_ADDR + (u32)dlog_log_data_start_addr);
        }
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief 将dlog数据写入flash
  @param  buf: 要写入的数据
  @param  len: 要写入的数据长度
  @param  offset: 数据需要写入的flash地址
  @return 返回写入的长度,返回值小于0表示写入失败
  @note
 */
/*----------------------------------------------------------------------------*/
static int dlog_ex_flash_write(void *buf, u16 len, u32 offset)
{
    // 需要实现
    if (!len) {
        return 0;
    }

    if (dlog_use_ex_flash) {
        /* norflash_write(NULL, buf, len, offset); */
        u32 waddr = offset;
        u8 *data = (u8 *)buf;
        s32 pages = len >> 8;
        //每次最多写入SPI_FLASH_PAGE_SIZE字节
        do {
            /* SectorErase(waddr); */
            write_data(data, waddr, FLASH_PAGE_SIZE);
            waddr += FLASH_PAGE_SIZE;
            data += FLASH_PAGE_SIZE;
        } while (pages--);
    }

    /* printf("%s, addr:%x %d", __func__, offset, len); */
    /* put_buf(buf, len); */

    /* u8 *tmp_buf = malloc(len); */
    /* spiflash_read(tmp_buf, offset, len); */
    /* put_buf(tmp_buf, len); */
    /* free(tmp_buf); */
    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief 从flash中读dlog数据
  @param  buf: 返回要读取的数据
  @param  len: 需要读取的数据长度
  @param  offset: 需要读取的flash地址
  @return 返回写入的长度,返回值小于0表示写入失败
  @note
 */
/*----------------------------------------------------------------------------*/
static int dlog_ex_flash_read(void *buf, u16 len, u32 offset)
{
    // 需要实现
    int err = 0;
    if (dlog_use_ex_flash) {
        /* norflash_read(NULL, buf, len, offset); */
        err = spiflash_read(buf, offset, len);
    }
    /* printf("%s, err:%d addr:%x %d", __func__, err, offset, len); */
    /* put_buf(buf, len); */
    return len;
}

// 返回 0 表示初始化成功
static int dlog_ex_flash_init(void)
{
    int err = 0;

    void *fd = dev_open("rtc", NULL);
    if (fd) {
        dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)&dlog_sys_time);
        dev_close(fd);
    }

    printf("dlog ex flash init\n");

    spi_open(); //打开spi 外挂flash设备

    err = W25X_GetChipID(); //获取flashID
    if (err) {
        spi_close();
    } else {
        dlog_use_ex_flash = 1;
        printf("dlog_ex_flash_init ok\n");
#if 0
        //test
        printf("begin test....\n");
        int test_len = 256;
        u8 *buf = malloc(test_len);
        for (u8 i = 1; i < 4; i++) {
            /* 写入数据  */
            memset(buf, i, test_len);
            spiflash_write(buf, 0x00, test_len);

            /* 读出数据  */
            memset(buf, 0, test_len);
            spiflash_read(buf, 0x00, test_len);
            printf(">>>>>>>>> read_spiflash_data addr = 0x00 , len = %d", test_len);
            put_buf(buf, test_len);
        }
        if (buf) {
            free(buf);
        }
#endif
    }
    //sys_timeout_add(NULL, dlog_print_test, 3000);
    return err;
}

REGISTER_DLOG_OPS(ex_flash_op, 1) = {
    .dlog_output_init       = dlog_ex_flash_init,
    .dlog_output_get_zone   = dlog_get_ex_flash_zone,
    .dlog_output_zone_erase = dlog_ex_flash_zone_erase,
    .dlog_output_write      = dlog_ex_flash_write,
    .dlog_output_read       = dlog_ex_flash_read,
    .dlog_output_direct     = dlog_output_direct,
};

#else  // 内置flash


//使用预留区的user空间
#define DLOG_FILE_PATH  		          "mnt/sdfile/EXT_RESERVED/dlog"

struct inside_flash_info_s {
    u32 addr;
    u32 len;
};

static struct inside_flash_info_s flash_info;

static int dlog_get_inside_flash_zone(u32 *addr, u32 *len)
{
    if (addr) {
        *addr = flash_info.addr;
    }
    if (len) {
        *len = flash_info.len;
    }

    return 0;
}

static int dlog_inside_flash_zone_erase(u16 erase_sector, u16 sector_num)
{
    int ret = -1;

    if ((flash_info.addr == 0) || (flash_info.len == 0)) {
        return ret;
    }

    u32 erase_addr = sdfile_cpu_addr2flash_addr(flash_info.addr + erase_sector * LOG_BASE_UNIT_SIZE);
    u32 erase_unit = LOG_BASE_UNIT_SIZE;
    u32 erase_size = 0;
    u32 total_size = LOG_BASE_UNIT_SIZE * sector_num;
    u32 erase_cmd = IOCTL_ERASE_SECTOR;
    if (boot_info.vm.align == 1) {
        erase_unit = 256;
        erase_cmd = IOCTL_ERASE_PAGE;
    }
    do {
        ret = norflash_ioctl(NULL, erase_cmd, erase_addr);
        if (ret < 0) {
            break;
        }
        erase_addr += erase_unit;
        erase_size += erase_unit;
    } while (erase_size < total_size);

    return ret;
}

static int dlog_inside_flash_init(void)
{
    FILE *fp = fopen(DLOG_FILE_PATH, "r");
    if (fp == NULL) {
        printf("Open %s dlog file fail", DLOG_FILE_PATH);
        return -1;
    }

    struct vfs_attr file_attr = {0};
    fget_attrs(fp, &file_attr);
    flash_info.addr = file_attr.sclust;
    flash_info.len = file_attr.fsize;

    if (fp) {
        fclose(fp);
    }

    printf("dlog flash init ok!\n");
    printf("dlog file addr 0x%x, len %d\n", flash_info.addr, flash_info.len);


    void *fd = dev_open("rtc", NULL);
    if (fd) {
        dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)&dlog_sys_time);
        dev_close(fd);
    }

    return 0;
}

static int dlog_inside_flash_write(void *buf, u16 len, u32 offset)
{
    int ret = -1;

    if ((flash_info.addr == 0) || (flash_info.len == 0)) {
        return ret;
    }

    u32 write_addr = sdfile_cpu_addr2flash_addr(offset); //flash addr
    printf("flash write addr 0x%x\n", write_addr);
    //写flash:
    ret = norflash_write(NULL, (void *)buf, len, write_addr);
    if (ret != len) {
        printf("dlog data save to flash err\n");
    }

    return ret;
}

static int dlog_inside_flash_read(void *buf, u16 len, u32 offset)
{
    memcpy(buf, (u8 *)offset, len);

    return len;
}

REGISTER_DLOG_OPS(inside_flash_op, 0) = {
    .dlog_output_init       = dlog_inside_flash_init,
    .dlog_output_get_zone   = dlog_get_inside_flash_zone,
    .dlog_output_zone_erase = dlog_inside_flash_zone_erase,
    .dlog_output_write      = dlog_inside_flash_write,
    .dlog_output_read       = dlog_inside_flash_read,
    .dlog_output_direct     = dlog_output_direct,
};

#endif


u16 dlog_read_log_data(u8 *buf, u16 len, u32 offset)
{
    u16 ret = -1;
    if ((offset + len) <= (u32)dlog_log_data_start_addr) {
        memset(buf, 0xFF, len);
        return len;
    } else {
        int remain_len = (int)((u32)dlog_log_data_start_addr - offset);
        remain_len = remain_len > 0 ? remain_len : 0;
        if (remain_len) {
            memset(buf, 0xFF, remain_len);
        }
        ret = dlog_read_from_flash(buf + remain_len, len - remain_len, (offset + remain_len) - (u32)dlog_log_data_start_addr);
        if ((len - remain_len) == ret) {
            return len;
        }
    }

    return ret;
}


#if 1  // dlog demo
// 以下是部分离线log接口的使用示例

void dlog_demo(void)
{
    int ret;
    // 1,刷新将离线log数据从ram保存到flash
    // 仅仅只发送一个刷新消息后退出, 可以用于中断和任务
    ret = dlog_flush2flash(0);
    // 一直等待刷新成功, 若返回成功, 从函数返回后已经刷新到flash, 仅可用于任务
    ret = dlog_flush2flash(-1);
    // 等待1000ms, 若返回成功, 从函数返回后已经刷新到flash, 仅可用于任务
    ret = dlog_flush2flash(100);


    // 2,设置log的等级, 主要用于 log_xxx 类接口
    // 设置为 info 等级, 更多等级见 debug.h
    dlog_level_set(__LOG_INFO);


    // 3,设置log输出的方式
    // 设置为仅串口输出离线log
    if (dlog_output_type_get() & DLOG_OUTPUT_2_FLASH) {
        // 如果之前有使能flash输出, 那么关闭flash输出前需要刷新ram的log到flash
        ret = dlog_flush2flash(100);
    }
    dlog_output_type_set(DLOG_OUTPUT_2_UART);
    // 离线log输出加上flash输出(如原本是仅输出到串口,设置后log同时输出到串口和flash)
    dlog_output_type_set(dlog_output_type_get() | DLOG_OUTPUT_2_FLASH);


    // 4,离线log的时间、日期、序号进行同步(当收到手机下发的日期、时间后需要同步一次, 以校准离线log时间)
    dlog_info_sync();


    // 5,读取离线log数据
    FILE *f = fopen(CONFIG_ROOT_PATH"flash_dlog.bin", "w+");
#define TMP_BUF_SIZE         501
    u8 *tmp_buf = malloc(TMP_BUF_SIZE);
    u32 offset = 0;
    u8 flash_log_en = (dlog_output_type_get() & DLOG_OUTPUT_2_FLASH);  // 获取是否有使能log输出到flash
    if (flash_log_en) {
        dlog_output_type_set(dlog_output_type_get() & (~DLOG_OUTPUT_2_FLASH));  // 禁止log输出到flash
    }
    while (1) {
        // 建议使用dlog_read_log_data接口, 而不是dlog_read_from_flash接口
        ret = dlog_read_log_data(tmp_buf, TMP_BUF_SIZE, offset);  // 读取flash中的log, 返回0表示已读取完flash的全部log
        if (0 == ret) {
            // 已经全部读取
            break;
        }
        offset += ret;
        put_buf(tmp_buf, ret > 16 ? 16 : ret);  // 将读取的log数据前16Byte打印出来
        if (f) {
            fwrite(tmp_buf, ret, 1, f);
        }
    }
    if (flash_log_en) {
        dlog_output_type_set(dlog_output_type_get() | DLOG_OUTPUT_2_FLASH);  // 恢复log输出到flash
    }
    free(tmp_buf);
    if (f) {
        fclose(f);
    }


}

// 判断是否为闰年
static int is_leap_year(u16 year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 获取当月的天数
static u8 days_in_month(u16 year, u8 month)
{
    const u8 days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12) {
        return 0;
    }

    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    return days[month - 1];
}

static void add_days(struct sys_time *time, u32 days)
{
    while (days > 0) {
        u8 days_in_current_month = days_in_month(time->year, time->month);
        u8 remaining_days_in_month = days_in_current_month - time->day + 1;

        if (days < remaining_days_in_month) {
            // 在当前月内增加
            time->day += days;
            break;
        } else {
            // 需要进位到下个月
            days -= remaining_days_in_month;
            time->day = 1;
            time->month++;

            if (time->month > 12) {
                // 需要进位到下一年
                time->month = 1;
                time->year++;
            }
        }
    }
}
// 重写弱函数的实现示例
int dlog_get_rtc_time(void *time_p)
{
    // 仅需返回年月日

    struct sys_time *time = time_p;
    time->year = dlog_sys_time.year;
    time->month = dlog_sys_time.month;
    time->day = dlog_sys_time.day;

    return 0;

    /* return -1; */
}

// 重写弱函数的实现示例
u32 dlog_get_rtc_time_ms(void)
{
    // dlog会多次调用这个接口获取系统时间戳, 需要应用层维护一个全局时间戳, 要处理好网络时间和本地时间的同步
    // 返回的时间单位是毫秒, 24小时制
    //如当前时间为 20:13:30.100, 则返回 ((21 * 60 + 13) * 60) + 30) * 1000 + 100 = 76410100毫秒
    /* u32 time_ms = get_system_ms();  // get_sys_time_ms函数需要自行实现, 此处仅示例 */
    //这里get_system_ms 为系统运行时间ms 开机从0开始计数
    u32 base_time_ms = (dlog_sys_time.hour * 3600 + dlog_sys_time.min * 60 + dlog_sys_time.sec) * 1000;
    u32 total_ms = base_time_ms + get_system_ms();

    // 检查是否超过一天
#define MS_PER_DAY (24 * 3600 * 1000)  // 86400000毫秒
    if (total_ms >= MS_PER_DAY) {
        u32 overflow_days = total_ms / MS_PER_DAY;
        u32 remain_ms = total_ms % MS_PER_DAY;

        // 更新日期
        add_days(&dlog_sys_time, overflow_days);

        // 更新时分秒为新的一天的时间
        u32 remain_sec = remain_ms / 1000;
        dlog_sys_time.hour = remain_sec / 3600;
        dlog_sys_time.min = (remain_sec % 3600) / 60;
        dlog_sys_time.sec = remain_sec % 60;

        return remain_ms;
    }

    return total_ms;
}
#endif

#endif
