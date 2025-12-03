#if 	0

#include "mt_fe_common_tc6930.h"
#include "mt_fe_def_tc6930.h"
#include "system/includes.h"
#include "app_config.h"
#include "os/os_api.h"
#include "device/device.h"
#include "asm/gpio.h"
#include "fs/fs.h"


MT_FE_TC6930_SETTINGS *tc6930;
U8 *demod_index_1;

static int tc6930_main(void *offset)
{
    MT_FE_TC6930_SETTINGS mt_fe_tc6960_settings;
    U8 demod_index;
    MT_FE_RET ret;
    U32 freq_KHz;
    U16 symbol_rate_KSs;
    U16 qam;
    U8 inverted;

    U32 ulData = 0;
    MT_FE_LOCK_STATE stat;
    U32 error_bits;
    U32 total_bits;
    U8 signal_snr;
    U8 p_percent;
    U8 signal_strength;

    demod_index     = 0;
    freq_KHz        = 419 * 1000;
    symbol_rate_KSs = 6875;
    qam             = 64;
    inverted        = 0;
    ret = mt_fe_dmd_tc6930_config_default(&mt_fe_tc6960_settings, 0);
    ret = mt_fe_dmd_tc6930_connect(&mt_fe_tc6960_settings, demod_index, freq_KHz, symbol_rate_KSs, qam, inverted);
    while (1) {
        os_time_dly(300);
        mt_fe_dmd_tc6930_get_lock_state(&mt_fe_tc6960_settings, demod_index, &stat);
        if (stat == MtFeLockState_Locked) {
            printf("tuner Locked\n");
        } else {
            printf("tuner unLock\n");
        }
        mt_fe_dmd_tc6930_get_ber(&mt_fe_tc6960_settings, demod_index, &error_bits, &total_bits);
        mt_fe_dmd_tc6930_get_snr(&mt_fe_tc6960_settings, demod_index, &signal_snr);
        mt_fe_dmd_tc6930_get_quality(&mt_fe_tc6960_settings, demod_index, &p_percent);
        mt_fe_dmd_tc6930_get_strength(&mt_fe_tc6960_settings, demod_index, &signal_strength);
        printf("ber=[%d|%d] snr=%d quality=%d strength=%d\n", error_bits, total_bits, signal_snr, p_percent, signal_strength);
        printf("wrdma0 ch%d cnt = %d\n", JL_DVB0->PIDMUX0 & 0x7, JL_DVB0->WR_CNT3 & 0xFFFF);
        printf("wrdma1 ch%d cnt = %d\n", JL_DVB1->PIDMUX0 & 0x7, JL_DVB1->WR_CNT3 & 0xFFFF);
    }
    return 0;
}

static void set_dvb0_test_pid(u8 enable)
{
    // configure pid table
    JL_DVB0->PID_SEL  = 0;
    JL_DVB0->PID_DATA = 0x0;
    JL_DVB0->PID_SEL  = 1;
    JL_DVB0->PID_DATA = 0x205;
    JL_DVB0->PID_SEL  = 2;
    JL_DVB0->PID_DATA = 0x106;
    JL_DVB0->PID_SEL  = 3;
    JL_DVB0->PID_DATA = 0x2bc;

    // enable pid table setting
    if (enable) {
        JL_DVB0->PID_EN = 0x0f;
    } else {
        //PID remain
        JL_DVB0->PID_EN = 0x0;
        SFR(JL_DVB0->REMAINS, 8, 3, 6);//如果修改了wrdma的通道，务必也要修改此处
        SFR(JL_DVB0->REMAINS, 2, 1, 1);
        SFR(JL_DVB0->REMAINS, 4, 1, 1);
        SFR(JL_DVB0->REMAINS, 6, 1, 1);
        SFR(JL_DVB0->REMAINS, 0, 1, 1);
    }
}

static u16 get_dvb0_wrdma_cnt(void)
{
    return JL_DVB0->WR_CNT3 & 0xFFFF;//上下两行的差异仅仅是为了测试不同WRDMA通道
    /* return JL_DVB0->WR_CNT3>>16; */
}

static void set_dvb0_cw_key(u8 key_num)
{
    // odd_key setting
    JL_DVB0->KEY_SEL	= 1;
    JL_DVB0->KEY_ODD	= 0xc7710452;//奇
    JL_DVB0->KEY_ODD	= 0xf0d08996;

    JL_DVB0->KEY_EVEN 	= 0xc7710452;//偶
    JL_DVB0->KEY_EVEN 	= 0xefd08996;

    JL_DVB0->KEY_SEL	= 2;
    JL_DVB0->KEY_ODD	= 0xc7710452;//奇
    JL_DVB0->KEY_ODD	= 0xf1d08996;

    JL_DVB0->KEY_EVEN 	= 0xc7710452;//偶
    JL_DVB0->KEY_EVEN 	= 0xefd08996;
    //enable pid decypher
    JL_DVB0->KEY_EN   = 0x0f;

    //key mux
    if (key_num) {
        JL_DVB0->KEY_MUX0 = 0x1111;     //key_num = 1---测试文件使用2cw时用这个
    } else {
        JL_DVB0->KEY_MUX0 = 0x2010;     //key_num = 0---测试文件使用pid+cw时使用这个
    }
}

#define DVB0_MODE_TSIN			0	//dvb模式-TSIN
#define DVB0_MODE_RDDMA			0	//dvb模式-rddma
#define DVB0_MODE_RDDMA_SINGLE	1	//dvb模式-rddma单次采样

#if DVB0_MODE_TSIN
//tsin buffer size
//如果DVB0和DVB1同步测试，需要根据实际SDRAM/DDR的大小将buffer size减小
#define DVB0_BUFFER_SIZE_SRAM				(200 * 188)
#define DVB0_BUFFER_SIZE_DDR				(14 * 1024 * 1024)

#endif

#if DVB0_MODE_RDDMA || DVB0_MODE_RDDMA_SINGLE
//rddma buffer size
#define DVB0_BUFFER_SIZE_SRAM				(200 * 188)
#define DVB0_BUFFER_SIZE_DDR				(2 * 1024 * 1024)

static u8 read_buf[DVB0_BUFFER_SIZE_DDR] ALIGNED(4);
#endif

//SD卡读数据缓存数组
//DMA输出数据缓存数组
static u8 buffer_ddr[DVB0_BUFFER_SIZE_DDR] ALIGNED(4);
static u8 dvb_buf[DVB0_BUFFER_SIZE_SRAM] SEC_USED(.sram) ALIGNED(4);//wrdma buffer in sram
/* static u8 dvb_buf[DVB0_BUFFER_SIZE_SRAM] ALIGNED(4);//wrdma buffer in ddr */
//写sd数据次数标志位
static int dvb_flag;
static u16 read_flag = 0;

//该函数主要集合了wrdma通道相关的配置，还有一个是get_dvb0_wrdma_cnt
static void set_dvb0_wrdma_ch(void)
{
    // pid mux to wr channel
    JL_DVB0->PIDMUX0 = 0x6666;

    // configure wr dma channel
    /* DcuFlushinvRegion(dvb_buf, sizeof(dvb_buf));//修改dvb_buf不走cache */
    /* JL_DVB0->WR_BASE3 = NO_CACHE_ADDR (dvb_buf); */
    JL_DVB0->WR_BASE6 = (u32)dvb_buf;
    printf("JL_DVB0->WR_BASE = 0x%x,size is 0x%x", (u32)dvb_buf, sizeof(dvb_buf));
    // configure ch6 length
    SFR(JL_DVB0->WR_LEN3, 0, 16, 199); // length ch6 = 199+1
    // configure wrdma set pending register
    SFR(JL_DVB0->WR_SPND3, 0, 16, 199);

    /* SFR(JL_DVB0->WR_INT, 0, 8, BIT(6));//wrdma 通道中断使能 */
}

static void dvb0_fs_test(void *offset)
{
    os_time_dly(800);//延时的目的是保证两个DVB模块同时运行时，tuner板子或者另一个DVB模块已经完成初始化并持续为DVB硬件灌输数据，确保两个DVB在硬件上是同时工作的，单独运行一个DVB可以关闭延时。
#if DVB0_MODE_RDDMA || DVB0_MODE_RDDMA_SINGLE
    FILE *rd_handle = NULL;
    FILE *wr_handle = NULL;
    u16 wrdmaOldCnt = 0;
    u16 wrdmaNowCnt = 0;
    u8 *ptr = buffer_ddr;
    memset(buffer_ddr, 0, sizeof(buffer_ddr));

    set_dvb0_test_pid(1);//1使能测试PID过滤，0使能测试remain
    set_dvb0_cw_key(0);//1对应测试2cw文件的密钥，0对应测试pid+cw文件的密钥
    set_dvb0_wrdma_ch();//修改wrdma输出通道相关，一方面需要修改该函数里的，另一方面需要修改get_dvb0_wrdma_cnt

    // clear pnd
    SFR(JL_DVB0->WR_INT, 8, 8, 0xff);
    /* SFR(JL_DVB0->WR_INT, 0, 8, BIT(1));//wrdma 通道中断使能 */

    /* JL_DVB0->ERR_IE = BIT(3); */
    /* JL_DVB0->ADP_IE = BIT(3); */
    /* JL_DVB0->DCW_IE = BIT(3); */
    /* JL_DVB0->INT_IE = BIT(3); */

    //rddma基本配置
    SFR(JL_DVB0->RD_CON, 8, 16, 199);//传输次数--一般只需要修改这个
    SFR(JL_DVB0->RD_CON, 24, 8, 188);//偏移量
    SFR(JL_DVB0->RD_CON, 6, 1, 1);
    /* DcuFlushRegion(read_buf, sizeof(read_buf)); */
    /* JL_DVB0->RD_BASE = NO_CACHE_ADDR (read_buf); */
    JL_DVB0->RD_BASE = (u32)read_buf;
    printf("JL_DVB0->RD_CON = 0x%x", JL_DVB0->RD_CON);
    printf("JL_DVB0->RD_BASE = 0x%x,size is 0x%x", JL_DVB0->RD_BASE, sizeof(read_buf));

    // tsin && enable
    __asm_csync();
    JL_DVB0->CON0 = 0xbc0e4203; //清流、RDDMA
    /* SFR(JL_DVB0->CON0, 10, 2, 0b11);//忽略错误包错误位使能 */
    /* JL_PORTE->DIR = 0;//测试dma模式下，dvb解扰/清流速率 */
    /* JL_PORTE->OUT = ~(0XFFFF); */

    /* #if 	0 */


    //等待文件系统挂载完毕
    extern int storage_device_ready(void);
    while (!storage_device_ready()) {
        os_time_dly(20);
    }

    //打开文件，设置追加写入模式
    /* wr_handle = fopen(CONFIG_ROOT_PATH"DVB0_TEST/dvtest.ts", "w"); */
    wr_handle = fopen(CONFIG_ROOT_PATH"DVB0_TEST/tstest.ts", "w");

    //打开（读）文件，rddma数据来源
    rd_handle = fopen(CONFIG_ROOT_PATH"dvbfile.ts", "r");//加扰文件
    /* rd_handle = fopen(CONFIG_ROOT_PATH"tsfile.ts", "r");//无加扰文件 */
    /* rd_handle = fopen(CONFIG_ROOT_PATH"csafile.ts", "r");//bit 中断测试文件 */
    //将文件数据存入ddr buffer中
    if (rd_handle) {
        for (dvb_flag = 0; dvb_flag < sizeof(read_buf) / (1024 * 1024); dvb_flag++) {
            fread((read_buf + (dvb_flag * 1024 * 1024)), 1 * 1024, 1 * 1024, rd_handle);
        }
        fclose(rd_handle);
        printf("dvb0 read file over");
    } else {
        printf("tsfile open fail!!(read)");
    }
    if (wr_handle) {

        g_printf("dvb0 %d", sizeof(read_buf) / sizeof(dvb_buf));//debug需要读中断多少次
        /* JL_PORTE->OUT = 0XFFFF; */
#if DVB0_MODE_RDDMA
        SFR(JL_DVB0->RD_CON, 5, 1, 1);//使能读中断
        SFR(JL_DVB0->RD_CON, 0, 1, 1);//发起读请求
        //获取dvb输出的数据
        while (1) {
            //检测写入长度是否接近buffer的长度，是则停止，避免越界
            if (ptr >= (buffer_ddr + sizeof(buffer_ddr) - (188 * 512 * 1))) {
                g_printf("DVB0 wrdma complete!!");
                break;
            }
            __asm_csync();
            wrdmaNowCnt = get_dvb0_wrdma_cnt();
            if (wrdmaOldCnt > wrdmaNowCnt) {
                //将前一次TS包cnt到数组末尾的数据写入ddr的buffer中
                memcpy(ptr, dvb_buf + (wrdmaOldCnt * 188), sizeof(dvb_buf) - (wrdmaOldCnt * 188));
                ptr += sizeof(dvb_buf) - (wrdmaOldCnt * 188);
                wrdmaOldCnt = wrdmaNowCnt;
                //避免dma_copy的len传入形参0
                if (wrdmaOldCnt == 0) {
                    continue;
                }
                //将数组头到本次TS包cnt的数据写入到ddr的buffer中
                memcpy(ptr, dvb_buf, wrdmaOldCnt * 188);
                ptr += wrdmaOldCnt * 188;
            } else {
                //避免dma_copy的len传入形参0
                if (wrdmaOldCnt == wrdmaNowCnt) {
                    continue;
                }
                //将前一次TS包cnt的数据到本次TS包cnt的数据写入ddr的buffer中
                memcpy(ptr, dvb_buf + (wrdmaOldCnt * 188), (wrdmaNowCnt - wrdmaOldCnt) * 188);
                ptr += (wrdmaNowCnt - wrdmaOldCnt) * 188;
                wrdmaOldCnt = wrdmaNowCnt;
            }
        }
        //将ddr的buffer数据写入SD卡
        for (dvb_flag = 0; dvb_flag < (sizeof(buffer_ddr) / (1024 * 1024)); dvb_flag++) {
            fwrite(buffer_ddr + (dvb_flag * 1024 * 1024), 1 * 1024, 1 * 1024, wr_handle);
        }

#endif

#if DVB0_MODE_RDDMA_SINGLE

        SFR(JL_DVB0->RD_CON, 0, 1, 1);
        os_time_dly(10);
        fwrite(dvb_buf, 1, sizeof(dvb_buf), wr_handle);
        printf("JL_DVB0->REMAINS = %x", JL_DVB0->REMAINS);

#endif
        fclose(wr_handle);
    } else {
        r_printf("tsfile open fail!!(write)");
    }
    /* #endif */
    //当log中看到这句就意味着可以拔卡对照数据了
    g_printf("dvb0 write complete!\n");
    while (1) {
        /* SFR(JL_DVB0->RD_CON, 0, 1, 1); */
        os_time_dly(1);
    }
#endif
}

static void ts0_fs_test(void *offset)
{
#if DVB0_MODE_TSIN
    os_time_dly(800);//延时的目的是确保两个DVB工作在TSIN模式下，tuner板子能够正常初始化。单独运行一个DVB时可以把延时关闭。
    FILE *wr_handle = NULL;
    dvb_flag = 0;
    u8 flag1 = 0;
    u16 wrdmaOldCnt = 0;
    u16 wrdmaNowCnt = 0;
    u8 *ptr = buffer_ddr;
    memset(dvb_buf, 0, sizeof(dvb_buf));

    set_dvb0_test_pid(1);//1使能测试PID全通，0使能测试remain
    set_dvb0_cw_key(1);//1对应测试2cw文件的密钥，0对应测试pid+cw文件的密钥
    set_dvb0_wrdma_ch();//修改wrdma输出通道相关，一方面需要修改该函数里的，另一方面需要修改get_dvb0_wrdma_cnt

    //IV设置，测试的时候直接打开就好
    /* JL_DVB0->IV = 0x87654321; */
    /* JL_DVB0->IV = 0x22222222; */

    //ie测试
    /* JL_DVB0->ERR_IE = BIT(3); */
    /* JL_DVB0->ADP_IE = BIT(3); */
    /* JL_DVB0->DCW_IE = BIT(3); */
    /* JL_DVB0->INT_IE = BIT(3); */


    // clear pnd
    SFR(JL_DVB0->WR_INT, 8, 8, 0xff);
    SFR(JL_DVB0->WR_INT, 25, 1, 1);
    /* SFR(JL_DVB0->WR_INT, 24, 1, 1); */


    /* SFR(JL_DVB0->CON3, 0, 1, 1);//输入异常中断使能 */
    // tsin && enable
    __asm_csync();
    JL_DVB0->CON0 = 0xbc0e41f3; //disable dsm_en
    /* SFR(JL_DVB0->CON0, 10, 2, 0b11); */
    /* SFR(JL_DVB0->CON0, 21, 1, 1);//tsin error io IE */
    printf("JL_DVB0->CON0 = 0x%x", JL_DVB0->CON0);

    //配置IO输入模式
    gpio_direction_input(IO_PORTA_02);
    gpio_direction_input(IO_PORTA_03);
    gpio_direction_input(IO_PORTA_04);
    gpio_direction_input(IO_PORTA_05);
    gpio_direction_input(IO_PORTA_06);
    /* JL_PORTE->DIR = 0;//tsin 模式测试dvb解扰/清流速率 */

    /* #if 	0 */

    //等待文件系统挂载完毕
    extern int storage_device_ready(void);
    while (!storage_device_ready()) {
        os_time_dly(20);
    }
    //打开文件，设置追加写入模式
    wr_handle = fopen(CONFIG_ROOT_PATH"DVB0_TEST/dvtest.ts", "w");
    /* wr_handle = fopen(CONFIG_ROOT_PATH"DVB0_TEST/tstest.ts", "w"); */
    if (wr_handle) {
        //将wrdma输出的数据写到ddr buffer中
        while (1) {
            //检测写入长度是否接近ddr buffer的长度，是则停止，避免越界
            if (ptr >= (buffer_ddr + sizeof(buffer_ddr) - (188 * 256))) {
                g_printf("DVB0 tsin complete\n");
                break;
            }
            __asm_csync();
            wrdmaNowCnt = get_devb0_wrdma_cnt();
            if (wrdmaOldCnt > wrdmaNowCnt) {
                //将前一次TS包cnt到数组末尾的数据写入ddr buffer
                memcpy(ptr, dvb_buf + (wrdmaOldCnt * 188), sizeof(dvb_buf) - (wrdmaOldCnt * 188));
                ptr += sizeof(dvb_buf) - (wrdmaOldCnt * 188);
                wrdmaOldCnt = wrdmaNowCnt;
                //避免dma_copy的len传入形参0
                if (wrdmaOldCnt == 0) {
                    continue;
                }
                //将数组头到本次TS包cnt的数据写入ddr buffer中
                memcpy(ptr, dvb_buf, wrdmaOldCnt * 188);
                ptr += wrdmaOldCnt * 188;
            } else {
                //避免dma_copy的len传入形参0
                if (wrdmaOldCnt == wrdmaNowCnt) {
                    continue;
                }
                //将前一次TS包cnt的数据到本次TS包cnt的数据写入ddr buffer
                memcpy(ptr, dvb_buf + (wrdmaOldCnt * 188), (wrdmaNowCnt - wrdmaOldCnt) * 188);
                ptr += (wrdmaNowCnt - wrdmaOldCnt) * 188;
                wrdmaOldCnt = wrdmaNowCnt;
            }
        }

        if (buffer_ddr[0] == 1) {
            r_printf("buffer_ddr error!! %x", buffer_ddr[0]);
        } else {
            g_printf("buffer_ddr[0] = %x", buffer_ddr[0]);
        }
        //将ddr buffer的数据写入SD卡
        os_time_dly(10);
        for (dvb_flag = 0; dvb_flag < (sizeof(buffer_ddr) / (1024 * 1024)); dvb_flag++) {
            fwrite((buffer_ddr + (dvb_flag * 1024 * 1024)), 1 * 1024, 1 * 1024, wr_handle);
            os_time_dly(10);
        }
        fclose(wr_handle);
    } else {
        printf("tsfile open fail!!(write)");
    }
    //当log中看到这句就意味着可以拔卡对照数据了
    g_printf("DVB0 SD0 write complete!\n");
    /* SFR(JL_DVB0->WR_INT, 0, 8, BIT(6));//wrdma 通道中断使能 */
    /* #endif */
    while (1) {
        os_time_dly(300);
        r_printf("dvb0-bandwidth->%x", JL_DVB0->WR_INT & BIT(26));
        /* printf("dvb have data"); */
    }
#endif
}

void continue_cnt_poll()
{
    while (1) {
        while (!((JL_DVB0->DCW_L >> 16) || (JL_DVB0->DCW_H >> 16))) {
            ;
        }
        printf("continue cnt bit interrupt!DCW_L = 0X%x;\rDCW_H = 0X%x\n\n", JL_DVB0->DCW_L >> 16, JL_DVB0->DCW_H >> 16);
        SFR(JL_DVB0->DCW_L, 0, 16, 0xffff);
        SFR(JL_DVB0->DCW_H, 0, 16, 0xffff);
        os_time_dly(1);
    }
}

void monitor_task()
{
    while (1) {
        /* printf("wrdma_int = 0x%x",JL_DVB0->WR_INT); */
        /* printf("rddma_con = 0x%x",JL_DVB0->RD_CON); */
        /* printf("error_l = 0x%x",JL_DVB0->ERR_L>>16); */
        //error monitor
        /* while(!(JL_DVB0->WR_INT&BIT(25))){ */
        /* } */
        /* SFR(JL_DVB0->WR_INT, 24, 1, 1); */
        /* printf("\nwrdma interrupt\n\n"); */

        //remains error  monitor
        /* while(!(JL_DVB0->REMAINS&BIT(3))){ */
        /* } */
        /* SFR(JL_DVB0->REMAINS, 2, 1, 1); */
        /* printf("\nremains error interrupt\n\n"); */

        //wrdma error
        /* while(!(JL_DVB0->WR_INT&BIT(25))); */
        r_printf("con3 = %x,wr_bardwith = %x", JL_DVB0->CON3, JL_DVB0->WR_INT & BIT(26));
        /* SFR(JL_DVB0->CON3, 1, 1, 1); */
        SFR(JL_DVB0->WR_INT, 25, 1, 1);
        /* SFR(JL_DVB0->WR_INT, 24, 1, 1); */
        /* printf("DVB0->RD_CON = 0X%x",JL_DVB0->RD_CON&(0xff)); */
        os_time_dly(100);
    }
    /* while(1){	 */
    /* while(!(JL_DVB0->RD_CON&BIT(7))); */
    /* printf("RDdma interrupt while ok!!"); */
    /* SFR(JL_DVB0->RD_CON, 5, 1, 1); */
    /* } */
}

void error_poll()
{
    while (1) {
        while (!((JL_DVB0->ERR_H >> 16) || (JL_DVB0->ERR_L >> 16))) {
            ;
        }
        r_printf("JL_DVB0->ERR_H>>16 = %x;JL_DVB0->ERR_L>>16 = %x\n\n", JL_DVB0->ERR_H >> 16, JL_DVB0->ERR_L >> 16);
        SFR(JL_DVB0->ERR_L, 0, 16, 0xffff);
        SFR(JL_DVB0->ERR_H, 0, 16, 0xffff);
        r_printf("error bit interrupt\n\n");
        os_time_dly(1);
    }
}

__attribute__((interrupt("")))
void dvb_isr()
{
    /* JL_PORTE->OUT ^= 0XFFFF; */
    /* r_printf("interrupt!!! \n"); */
    /* SFR(JL_DVB0->WR_INT, 8, 8, 0xff); */
    /* SFR(JL_DVB0->CON3, 1, 1, 1); */
    /* SFR(JL_DVB0->CON0, 22, 1, 1); */

    /* if(JL_DVB0->ADP_L&(BIT(3)<<16)){ */
    /* printf("adp pnd"); */
    /* SFR(JL_DVB0->ADP_L, 0, 16, BIT(3)); */
    /* } */
    /* if(JL_DVB0->DCW_L&(BIT(3)<<16)){ */
    /* printf("continue pnd"); */
    /* SFR(JL_DVB0->DCW_L, 0, 16, BIT(3)); */
    /* } */
    /* if(JL_DVB0->INT_L&(BIT(3)<<16)){ */
    /* printf("pid mat pnd"); */
    /* SFR(JL_DVB0->INT_L, 0, 16, BIT(3)); */
    /* } */
    /* if(JL_DVB0->ERR_L&(BIT(3)<<16)){ */
    /* printf("error bit pnd"); */
    /* SFR(JL_DVB0->ERR_L, 0, 16, BIT(3)); */
    /* } */

#if DVB0_MODE_RDDMA

    SFR(JL_DVB0->RD_CON, 6, 1, 1);
    if (read_flag < (sizeof(read_buf) / sizeof(dvb_buf))) {
        read_flag++;
        JL_DVB0->RD_BASE = (u32)(read_buf + (read_flag * sizeof(dvb_buf)));
        SFR(JL_DVB0->RD_CON, 0, 1, 1);
        /* SFR(JL_DVB1->RD_CON, 0, 1, 1);	//测试双DVB dma模式打开 */
        printf("dvb0 read enable complete!! frequency = %d", read_flag);
    }
#endif

    if (JL_DVB0->WR_INT & BIT(26)) {
        r_printf("bandwidth error!!");
        SFR(JL_DVB0->WR_INT, 25, 1, 1);
    }
}

static void DVB0_main(void)
{
    request_irq(IRQ_DVB0_IDX, 5, dvb_isr, 1);
    printf("\n\n-----------------DVB0_SRAM_TO_DDR_TEST--------------------\n\n");
    /* os_task_create(continue_cnt_poll, NULL, 5, 512, 0, "continue_cnt_poll"); */
    /* os_task_create(error_poll, NULL, 5, 512, 0, "error_poll"); */
    /* os_task_create(monitor_task, NULL, 20, 512, 0, "monitor_task"); */
#if DVB0_MODE_RDDMA_SINGLE || DVB0_MODE_RDDMA
    os_task_create(dvb0_fs_test, NULL, 28, 8 * 1024, 0, "dvb0_fs_test");
#endif

#if DVB0_MODE_TSIN
    os_task_create(ts0_fs_test, NULL, 10, 10 * 1024, 0, "ts0_fs_test");
    os_task_create(tc6930_main, NULL, 12, 8 * 1024, 0, "tc6930_main");
#endif

    return;
}
late_initcall(DVB0_main);

#endif
