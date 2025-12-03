#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "asm/dma.h"
#include "asm/clock.h"
#include "asm/irq.h"
#include "system/timer.h"

#ifdef USE_DMA_TEST_DEMO

#define BUF_LEN 1024 * 7
/* #define BUF_LEN 1024 * 30 */

#define TASK_CNT 100
#define TIMER_CYCLE_CNT 4000000000


/* ~~~~~~~~~~~~~~~~~~~~~~配置定时器，用于测试dma copy时间~~~~~~~~~~~~~~~~~~~~~~ */
#define TSEC SEC_USED(.volatile_ram_code)
static JL_TIMER_TypeDef *TMR = JL_TIMER4;
static u8 timer_irq = IRQ_TIMER4_IDX;
static u32 timer_cycle;                    ///< 定时器进中断次数
static u32 timer_cycle_temp;               ///< 记录stop时,定时器已经进中断次数
static u32 timer_reg_start;                ///< 记录start时,定时器CNT寄存器的值
static u32 timer_reg_stop;                 ///< 记录stop时,定时器CNT寄存器的值
static u32 time_dma_copy_us;               ///< dma copy时间
static u32 timer_clk;

static ___interrupt TSEC void timer_isr(void)
{
    if (TMR->CON & BIT(15)) {
        TMR->CON |= BIT(14);
        timer_cycle++;
        /* putchar('^'); */
    }
}

static void timer_init(void)
{
    timer_clk = clk_get("timer");
    request_irq(timer_irq, 3, timer_isr, 0);
    TMR->CON = 0;
    TMR->CNT = 0;
    SFR(TMR->CON, 10, 4, 1);///< 选择LSB作为TIMER时钟源
    TMR->PRD = TIMER_CYCLE_CNT;
    TMR->CON |= BIT(14);///<清中断pnding
    TMR->CON |= BIT(0);///<设置为定时/计数模式
}

static void timer_start(void)
{
    TMR->CON &= ~(BIT(0) | BIT(1)); ///<关定时器
    TMR->CNT = 0; ///<清空计时
    timer_cycle = 0;
    TMR->CON |= BIT(0);///<重新开始计时
    timer_reg_start = TMR->CNT;
}

static void timer_stop(void)
{
    timer_reg_stop = TMR->CNT;
    timer_cycle_temp = timer_cycle;
    /* printf("timer_reg_stop: %u\n",timer_reg_stop); */
    /* printf("timer_cycle_temp: %u\n",timer_cycle_temp); */
    time_dma_copy_us = (TIMER_CYCLE_CNT * timer_cycle_temp + (timer_reg_stop - timer_reg_start)) * 1000000 / timer_clk;
    printf("dma_copy time: %uus\n", time_dma_copy_us);
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */



// 回调函数
static void dma_irq_cb(dma_execute_status_t exec_sta)
{
    printf("dma_irq_cb() get execute status:%d\n", exec_sta);
}


static void dma_test_task(void *arg)
{
    char *buf_src = malloc(BUF_LEN);
    char *buf_dst = malloc(BUF_LEN);
    char c = 'a'; ///< 测试用的字符数据

    memset(buf_dst, '%', BUF_LEN); ///< 测试程序中buf_dst作为目标地址，设置初值，便于确定dma是否完成copy。
    printf("The buf len is: %dByte\n", BUF_LEN);

    timer_init();
    printf("The timer clock is %dHz\n", timer_clk);

#if 1
    // 1.测试dma_copy_sync() or dma_copy_async()
    while (1) {
        printf("\n1.dma_copy_sync() or dma_copy_async() test~~~~~~~~~~~~~~~~\n");
        if (c > 'z') {
            c = 'a';
        }
        memset(buf_src, c++, BUF_LEN);

        /*
         * 异步dma copy接口。观察下第一次buf_dst的打印，buf数据量较大的时候，copy还没
         * 有完成，就将buf_dst打印出来，因此会出现打印'%'的情况。
         */
        dma_copy_async(buf_dst, buf_src, BUF_LEN);
        /* dma_copy_sync(buf_dst,buf_src,BUF_LEN); */
        /* dma_copy_async_with_execute_status(buf_dst, buf_src, BUF_LEN, &dma_irq_cb); */

        printf("The buf_dst data: %c\n\n", buf_dst[BUF_LEN - 1]);
        os_time_dly(200);
    }

#endif


#if 0
    // 2.测试不同len阈值，对dma_copy时间的影响
    u8 flag = 0;
    int get_len_threshold = 0;

    while (1) {
        printf("\n2.len_threshold test~~~~~~~~~~~~~~~~\n");
        if (c > 'z') {
            c = 'a';
        }
        memset(buf_src, c++, BUF_LEN);

        flag = !flag;
        if (flag) {
            dma_set_len_threshold(1024 * 30);
        } else {
            dma_set_len_threshold(1024 * 6);
        }
        get_len_threshold = dma_get_len_threshold();
        printf("dma get len threshold:%d\n", get_len_threshold);

        /*
         * 同步的dma_copy_sync()会对比任务的len和len阈值大小来判断是否在等待copy过程中释放cpu:
         * a.小于len阈值,驱动死等copy完成,期间不释放cpu
         * b.大于len阈值,驱动os_pend copy完成,期间释放cpu。
         * 因此相同条件下设置不同len阈值复制的时间会有所不同。
         */
        timer_start();
        dma_copy_sync(buf_dst, buf_src, BUF_LEN);
        /* memcpy(buf_dst,buf_src,BUF_LEN);///< 对比cpu copy时间 */
        timer_stop();


        printf("The buf_dst data is: %c\n", buf_dst[BUF_LEN - 1]);
        os_time_dly(200);
    }

#endif


#if 0
    // 3.测试不同内部node数量，对dma_task_list_copy()时间的影响。
    char flag = 0;///< 设置不同len阈值的标志
    u32 get_node_num = 0;///< 获取到的内部节点数量
    dma_user_task_t task_list[TASK_CNT];

    // 配置TASK_CNT个任务，实现数据流：buf_src->buf_dst->buf_src->buf_dst...
    for (int i = 0; i < TASK_CNT; i++) {
        if (i % 2) {
            task_list[i].dst_addr = buf_src;
            task_list[i].src_addr = buf_dst;
        } else {
            task_list[i].dst_addr = buf_dst;
            task_list[i].src_addr = buf_src;
        }
        task_list[i].len = BUF_LEN;
    }

    while (1) {
        printf("\n3.task node count test~~~~~~~~~~~~~~~~\n");
        if (c > 'z') {
            c = 'a';
        }
        memset(buf_src, c++, BUF_LEN);

        flag = ~flag;
        if (flag) {
            dma_set_internal_node_cnt(3);
        } else {
            dma_set_internal_node_cnt(100);
        }
        get_node_num = dma_get_internal_node_cnt();
        printf("Get task node num: %d\n", get_node_num);

        /*
         * dma_task_list_copy()内部调用异步dma copy实现。在节点数足够配置所有任务的情况
         * 下，函数配置完节点开启dma后直接退出，耗时较短。节点数不够的情况下，函数内部
         * 会以循环的方式来实现全部task的copy，耗时较长，效率低。建议使用该函数前，先调
         * 用dma_set_internal_node_cnt()设置内部节点数多于任务数。
         */
        timer_start();
        dma_task_list_copy(task_list, TASK_CNT);
        /* dma_copy_async_wait(); */
        timer_stop();

        printf("The buf_dst data is: %c\n", buf_dst[BUF_LEN - 1]);

        os_time_dly(200);
    }

#endif

#if 0
    // 4.测试硬件memset：dma_memset_sync() or dma_memset_async()
    volatile dma_execute_status_t exec_sta;

    while (1) {
        printf("\n4.dma_memset_sync() or dma_memset_async() test~~~~~~~~~~~~~~~~\n");
        if (c > 'z') {
            c = 'a';
        }

        printf("dst buf data (memset before):\n");
        put_buf(buf_dst, BUF_LEN);

        dma_memset_async(buf_dst, c++, BUF_LEN);
        char temp_c = buf_dst[BUF_LEN - 1];///< 如果数据量大，copy还没完成，则可以看到该地址的数据没有变化
        putchar(temp_c);

        /* exec_sta = dma_memset_sync(buf_dst, c++, BUF_LEN); */
        /* printf("dma_execute_status:%u\n", exec_sta); */
        /* exec_sta = DMA_EXEC_STATUS_NULL; */

        printf("dst buf data (memset after):\n");
        put_buf(buf_dst, BUF_LEN);

        os_time_dly(200);
    }

#endif

    dma_copy_close();
}

static int c_main_dma(void)
{
    os_task_create(dma_test_task, NULL, 10, 1000, 0, "dma_test_task");
    return 0;
}

late_initcall(c_main_dma);

#endif // USE_DMA_TEST_DEMO
