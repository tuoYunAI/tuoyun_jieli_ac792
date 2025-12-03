#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "fs/fs.h"
#include "asm/gpio.h"
#include "system/timer.h"

#if 0//use open it

//only choose one to test at the same time
#define TEST_SD0_SPEED 1
#define TEST_UDISK0_SPEED 0
#define TEST_UDISK1_SPEED 0

#define TOTAL_TEST_DATA (6 * 1024 * 1024) //每次文件总数据量
#define ONCE_DATA_SIZE (128*1024)        //单次读写数据量
#define TOTAL_FILE_NUMS 10  //创建文件次数

char test_data[ONCE_DATA_SIZE];//单次读写buf

static JL_TIMER_TypeDef *pTimer;
static u32 timer_irq_idx;
static u32 t_counter;
static u32 wr_blocks;
static u32 rd_blocks;
static u32 wr_time;
static u32 rd_time;
static u32 wr_KBps_min;
static u32 rd_KBps_min;
static u32 wr_KBps_max;
static u32 rd_KBps_max;
static u32 wr_KBps_avg;
static u32 rd_KBps_avg;
static u32 wr_KBps_min_total;
static u32 rd_KBps_min_total;
static u32 wr_KBps_max_total;
static u32 rd_KBps_max_total;
static u32 wr_KBps_avg_total;
static u32 rd_KBps_avg_total;
static u32 total_nums;
static u32 total_file_nums;

___interrupt
void speed_test_irq()
{
    pTimer->CON |= BIT(14);
    t_counter++;
}
void speed_test_init()
{
    int i = 0;
    JL_TIMER_TypeDef *tregs[] = {
        JL_TIMER0, JL_TIMER1, JL_TIMER2, JL_TIMER3,
        JL_TIMER4, JL_TIMER5,
    };
    u32 tmr_irq_idx_tbl[6] = {
        IRQ_TIMER0_IDX, IRQ_TIMER1_IDX, IRQ_TIMER2_IDX, IRQ_TIMER3_IDX,
        IRQ_TIMER4_IDX, IRQ_TIMER5_IDX,
    };
    wr_blocks = 0;
    rd_blocks = 0;
    wr_time = 0;
    rd_time = 0;
    t_counter = 0;
    wr_KBps_min = 0xffffffff;
    rd_KBps_min = 0xffffffff;
    wr_KBps_max = 0;
    rd_KBps_max = 0;
    wr_KBps_avg = 0;
    rd_KBps_avg = 0;
    timer_irq_idx = tmr_irq_idx_tbl[5];
    for (i = 0; i < 6; i++) {
        pTimer = tregs[5 - i];
        if ((pTimer->CON & 3) == 0) {
            timer_irq_idx = tmr_irq_idx_tbl[5 - i];
            break;
        }
    }
    if (i == 6) {
        printf("find idle timer fail\n");
        pTimer = NULL;
        return;
    }
    printf("lsb clock %d\n", clk_get("lsb"));
    printf("timer%d is found\n", 5 - i);
    request_irq(timer_irq_idx, 7, speed_test_irq, 0);
    pTimer->CNT = 0;
    pTimer->PRD = clk_get("lsb") / 4 * 10 / 1000; //10ms
    pTimer->CON = (1 << 4) | (1 << 10) | (1 << 14);  //sel lsb_clk, div4
}
void speed_test_uninit()
{
    if (pTimer) {
        unrequest_irq(timer_irq_idx, 0);
        pTimer->CON = BIT(14);
        pTimer = NULL;
    }
}
void speed_test_probe()
{
    if (pTimer) {
        t_counter = 0;
        pTimer->CNT = 0;
        pTimer->CON |= BIT(0) | BIT(14);
    }
}
u32 speed_test_capture()
{
    u32 time = 0;
    if (pTimer) {
        u32 t_prd = pTimer->PRD;
        u32 t_cnt = pTimer->CNT;
        time = (t_counter * t_prd + t_cnt) / (clk_get("lsb") / 400 / 100 / 100);
        pTimer->CON &= ~BIT(0);
    }

    return time;
}
void write_speed_dump(u32 cur_time)
{
    u32 KBps = 0;
    if (cur_time == 0) {
        r_printf("========div00000000========%s======%d=========yuring===\n\r", __func__, __LINE__);
    }
    KBps = ((ONCE_DATA_SIZE / 16) * 15625) / cur_time;
    /* r_printf("wd: %d data_size, t %d us, %d KB/s\n", ONCE_DATA_SIZE, cur_time, KBps); */
    if (KBps < wr_KBps_min) {
        wr_KBps_min = KBps;
    }
    if (KBps > wr_KBps_max) {
        wr_KBps_max = KBps;
    }
    wr_KBps_avg = wr_KBps_avg + KBps;
}
void read_speed_dump(u32 cur_time)
{
    u32 KBps = 0;
    KBps = ((ONCE_DATA_SIZE / 16) * 15625) / cur_time;
    if (KBps < rd_KBps_min) {
        rd_KBps_min = KBps;
    }
    if (KBps > rd_KBps_max) {
        rd_KBps_max = KBps;
    }
    rd_KBps_avg = rd_KBps_avg + KBps;
}

void speed_summary_dump()
{
    wr_KBps_avg = wr_KBps_avg / total_nums;
    rd_KBps_avg = rd_KBps_avg / total_nums;
    r_printf("w: min %d, max %d, avg %d\n", wr_KBps_min, wr_KBps_max, wr_KBps_avg);
    r_printf("r: min %d, max %d, avg %d\n", rd_KBps_min, rd_KBps_max, rd_KBps_avg);

    if (rd_KBps_min < rd_KBps_min_total) {
        rd_KBps_min_total = rd_KBps_min;
    }
    if (rd_KBps_max > rd_KBps_max_total) {
        rd_KBps_max_total = rd_KBps_max;
    }

    if (wr_KBps_min < wr_KBps_min_total) {
        wr_KBps_min_total = wr_KBps_min;
    }
    if (wr_KBps_max > wr_KBps_max_total) {
        wr_KBps_max_total = wr_KBps_max;
    }

    rd_KBps_avg_total += rd_KBps_avg;
    wr_KBps_avg_total += wr_KBps_avg;
}

void speed_summary_dump_total()
{
    rd_KBps_avg_total /= TOTAL_FILE_NUMS;
    wr_KBps_avg_total /= TOTAL_FILE_NUMS;

    r_printf("w_t: min %d, max %d, avg %d\n", wr_KBps_min_total, wr_KBps_max_total, wr_KBps_avg_total);
    r_printf("r_t: min %d, max %d, avg %d\n", rd_KBps_min_total, rd_KBps_max_total, rd_KBps_avg_total);
}

static void sd_usb_test(void)
{
    u32 total_size = TOTAL_TEST_DATA;
    u32 once_size = ONCE_DATA_SIZE;
    u32 w_xus = 0;
    u32 r_xus = 0;
    u32 w_nums;
    u32 r_nums;
    u32 check_wnum;
    u32 check_rnum;

    total_nums = total_size / once_size; //读或写总次数
    total_file_nums = TOTAL_FILE_NUMS;

    wr_KBps_min_total = 0xffffffff;
    rd_KBps_min_total = 0xffffffff;
    wr_KBps_max_total = 0;
    rd_KBps_max_total = 0;

    while (total_file_nums) { //创建多次文件进行读写
        r_printf("============total_file_nums=%d======%s======%d======\n\r", total_file_nums, __func__, __LINE__);

        w_nums = total_nums;
        r_nums = total_nums;

        speed_test_init();

#if TEST_SD0_SPEED
        FILE *fd = fopen("storage/sd0/C/test.txt", "w+");
#elif TEST_UDISK0_SPEED
        FILE *fd = fopen("storage/udisk0/C/test.txt", "w+");
#elif TEST_UDISK1_SPEED
        FILE *fd = fopen("storage/udisk1/C/test.txt", "w+");
#endif
        if (fd) {
            while (w_nums) {
                speed_test_probe();
                check_wnum = fwrite(test_data, ONCE_DATA_SIZE, 1, fd);
                if (check_wnum == ONCE_DATA_SIZE) {
                    check_wnum = 0;
                    w_xus = speed_test_capture();
                    /* if(w_xus != 0) */
                    if (w_xus > 1) {
                        write_speed_dump(w_xus);
                        w_nums--;
                    }
                }
            }
            fclose(fd);
        } else {
            r_printf("======fopen_w+_fail==========%s======%d=========yuring===\n\r", __func__, __LINE__);
            return;
        }


        memset(test_data, 0, ONCE_DATA_SIZE);
#if TEST_SD0_SPEED
        FILE *fd1 = fopen("storage/sd0/C/test.txt", "r");
#elif TEST_UDISK0_SPEED
        FILE *fd1 = fopen("storage/udisk0/C/test.txt", "r");
#elif TEST_UDISK1_SPEED
        FILE *fd1 = fopen("storage/udisk1/C/test.txt", "r");
#endif
        if (fd1) {
            while (r_nums) {
                speed_test_probe();
                check_rnum = fread(test_data, ONCE_DATA_SIZE, 1, fd1);
                if (check_rnum == ONCE_DATA_SIZE) {
                    check_rnum = 0;
                    r_xus = speed_test_capture();
                    /* if(r_xus != 0) */
                    if (r_xus > 1) {
                        read_speed_dump(r_xus);
                        r_nums--;
                    }
                }
            }
            fclose(fd1);
        } else {
            r_printf("====fopen_r_fail============%s======%d=========yuring===\n\r", __func__, __LINE__);
            return;
        }


        speed_summary_dump();
        r_printf("============num_result=======%s======%d======\n\r", __func__, __LINE__);
        speed_test_uninit();
        w_nums = total_nums;
        r_nums = total_nums;

        total_file_nums--;
    }

    speed_summary_dump_total();
    r_printf("============total_result=======%s======%d======\n\r", __func__, __LINE__);
    while (1) {
        os_time_dly(2);
    }

}

static int c_main(void)
{
#if TEST_SD0_SPEED
    extern int sdcard_storage_device_ready(const char *sd_name);
    while (!sdcard_storage_device_ready("sd0")) {
        os_time_dly(2);
    }
#elif TEST_UDISK0_SPEED
    extern int udisk_storage_device_ready(int id);
    while (!udisk_storage_device_ready(0)) {
        os_time_dly(2);
    }
#elif TEST_UDISK1_SPEED
    extern int udisk_storage_device_ready(int id);
    while (!udisk_storage_device_ready(1)) {
        os_time_dly(2);
    }
#endif
    thread_fork("sd_usb_test", 10, 512, 0, NULL, sd_usb_test, NULL);

    return 0;
}
late_initcall(c_main);

#endif //if 0
