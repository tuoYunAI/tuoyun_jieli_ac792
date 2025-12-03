#include "asm/debug.h"
#include "asm/cache.h"
#include "app_config.h"
#include "fs/fs.h"
#include "init.h"

#define LOG_TAG_CONST       DEBUG_EXCP
#define LOG_TAG             "[DEBUG_EXCP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

#define EXPCPTION_IN_SRAM    0  //exception代码放在sram中。如果出现死机不
//                                复位的情况务必需要打开，防止flash挂了,
//                                进入异常函数失败复位不了
#define DEBUG_UART           JL_UART0

#if EXPCPTION_IN_SRAM && defined CONFIG_SAVE_EXCEPTION_LOG_IN_FLASH
#error "CONFIG_EXCEPTION_IN_SRAM and CONFIG_SAVE_EXCEPTION_LOG_IN_FLASH cannot be enabled simultaneously."
#endif


#if EXPCPTION_IN_SRAM
#pragma bss_seg(".debug_exception.data.bss")
#pragma data_seg(".debug_exception.data")
#pragma const_seg(".debug_exception.debug_const")
#pragma code_seg(".debug_exception.debug_text")
#pragma str_literal_override(".debug_exception.debug_const")
#endif


extern void exception_irq_handler();
extern const char *os_current_task_rom();
extern void log_output_release_deadlock(void);
extern void __wdt_clear(void);
extern void __cpu_reset(void);


/*===========================================================================================
  异常管理单元结构:
 ---------          ---------          ---------           ---------
|   LSB   |        |         |        |         |         |         |
|ASS/VIDEO|  --->  |   HSB   |  --->  |  HCORE  |  --->   |   CPU   |
|   WL    |        |         |        |         |         |         |
 ---------          ---------          ---------           ---------
     |                  |                  |                   |
     |                  |                  |                   |
      -------------------------------------
	                  system                                  内核

1. CPU: 内核级异常
   1) 对应异常开关控制寄存器: q32DSP(n)->EMU_CON;
   2) 对应异常查询寄存器:     q32DSP(n)->EMU_MSG;

2. HCORE: 系统(system)级异常, 该级别通常与内存(Memory)和MMU异常有关
   1) 对应异常开关控制寄存器: JL_CEMU->CON0, JL_CEMU->CON1, JL_CEMU->CON2, JL_CEMU->CON3;
   2) 对应异常查询寄存器:     JL_CEMU->MSG0, JL_CEMU->MSG1, JL_CEMU->MSG2, JL_CEMU->MSG3;
   3) 异常外设查询寄存器:  	  JL_CEMU->LOG0;
   4) HCORE异常类型:
			MSG0和MSG1异常页触发模块的源头相同, 其中
				MSG0异常原因有:
					A)异常模块访问到hmem空白区域;
					B)访问到MMU的TLB_BEG和TLB_END范围;
				MSG1异常原因有:
					A)异常模块访问到已被释放的MMU地址;
					B)MMU TLB配置为空白区;
					C)访问 mmu 虚拟地址引起读 tlb1 的操作，tlb1 的索引范围大于 tlb1_beg/tlb1_end 分配的空间;

3. HSB: 系统(system)级异常, 通常与一些系统高速设备触发的异常有关
   1) 对应异常开关控制寄存器: JL_HEMU->CON0, JL_HEMU->CON1, JL_HEMU->CON2, JL_HEMU->CON3;
   2) 对应异常查询寄存器:     JL_HEMU->MSG0, JL_HEMU->MSG1, JL_HEMU->MSG2, JL_HEMU->MSG3;
   3) 异常外设查询寄存器:  	  JL_HEMU->LOG0;

4. LSB: 系统(system)级异常, 通常与一些系统低速设备触发的异常有关
   1) 对应异常开关控制寄存器: JL_LEMU->CON0, JL_LEMU->CON1, JL_LEMU->CON2, JL_LEMU->CON3;
   2) 对应异常查询寄存器:     JL_LEMU->MSG0, JL_LEMU->MSG1, JL_LEMU->MSG2, JL_LEMU->MSG3;
   3) 异常外设查询寄存器:  	  JL_LEMU->LOG0;

5. ASS: 系统(system)级异常, 通常与系统音频设备触发的异常有关
   1) 对应异常开关控制寄存器: JL_AEMU->CON0, JL_AEMU->CON1, JL_AEMU->CON2, JL_AEMU->CON3;
   2) 对应异常查询寄存器:     JL_AEMU->MSG0, JL_AEMU->MSG1, JL_AEMU->MSG2, JL_AEMU->MSG3;
   3) 异常外设查询寄存器:  	  JL_AEMU->LOG0;

6. VIDEO: 系统(system)级异常, 通常与系统视频设备触发的异常有关
   1) 对应异常开关控制寄存器: JL_VEMU->CON0, JL_VEMU->CON1, JL_VEMU->CON2, JL_VEMU->CON3;
   2) 对应异常查询寄存器:     JL_VEMU->MSG0, JL_VEMU->MSG1, JL_VEMU->MSG2, JL_VEMU->MSG3;
   3) 异常外设查询寄存器:  	  JL_VEMU->LOG0;

7. WL: 系统(system)级异常, 通常与系统RF设备触发的异常有关
   1) 对应异常开关控制寄存器: JL_WEMU->CON0, JL_WEMU->CON1, JL_WEMU->CON2, JL_WEMU->CON3;
   2) 对应异常查询寄存器:     JL_WEMU->MSG0, JL_WEMU->MSG1, JL_WEMU->MSG2, JL_WEMU->MSG3;
   3) 异常外设查询寄存器:  	  JL_WEMU->LOG0;
============================================================================================*/


//====================================================================//
//                         设备ID号                                   //
//====================================================================//
struct dev_id_str {
    const char *name;
    u8 id;
};

static const struct dev_id_str dev_id_list[] = {
    {"DBG_REVERSE",         0x00},
    {"DBG_ALNK",            0x01},
    {"DBG_AUDIO_DAC",       0x03},
    {"DBG_AUDIO_ADC",       0x04},
    {"DBG_ISP",             0x06},
    {"DBG_PLNK",            0x08},
    {"DBG_SBC",             0x09},
    {"DBG_HADC",            0x0b},
    {"DBG_SD0",             0x0c},
    {"DBG_SD1",             0x0d},
    {"DBG_SPI0",            0x0e},
    {"DBG_SPI1",            0x0f},
    {"DBG_SPI2",            0x10},
    {"DBG_UART0",           0x11},
    {"DBG_UART1",           0x12},
    {"DBG_UART2",           0x13},
    {"DBG_LEDC",            0x14},
    {"DBG_GPADC",           0x15},
    {"DBG_PAP",             0x16},
    {"DBG_CTM",             0x17},
    {"DBG_ANC",             0x18},
    {"DBG_SRC_SYNC",        0x19},
    {"DBG_IIC1",            0x1a},
    {"DBG_UART3",           0x1b},
    {"DBG_TDM",             0x1c},
    {"DBG_SHA",             0x1e},
    {"DBG_ETHMAC",          0x1f},
    {"DBG_Q32S_IF",         0x20},
    {"DBG_Q32S_RW",         0x21},
    {"DBG_SPI3",            0x22},
    {"DBG_IIC2",            0x23},
    {"DBG_CAN",             0x24},
    {"DBG_FLASH_SPI",       0x25},
    {"DBG_UART4",           0x26},
    {"DBG_DVB0",            0x27},
    {"DBG_DVB1",            0x28},

    {"DBG_AXI_M1",          0x80},
    {"DBG_AXI_M2",          0x81},
    {"DBG_AXI_M3",          0x82},
    {"DBG_AXI_M4",          0x83},
    {"DBG_AXI_M5",          0x84},
    {"DBG_AXI_M6",          0x85},
    {"DBG_AXI_M7",          0x86},
    {"DBG_AXI_M8",          0x87},
    {"DBG_AXI_M9",          0x88},
    {"DBG_AXI_MA",          0x89},
    {"DBG_AXI_MB",          0x8a},
    {"DBG_AXI_MC",          0x8b},
    {"DBG_AXI_MD",          0x8c},
    {"DBG_AXI_ME",          0x8d},
    {"DBG_AXI_MF",          0x8e},

    {"DBG_AXI_1F",          0x9f},

    {"DBG_USB",             0xa0},
    {"DBG_FM",              0xa1},
    {"DBG_BT",              0xa2},
    {"DBG_FFT",             0xa3},
    {"DBG_EQ",              0xa4},
    {"DBG_SRC",             0xa5},
    {"DBG_DMA_COPY",        0xa6},
    {"DBG_JPG",             0xa7},
    {"DBG_IMB",             0xa8},
    {"DBG_AES",             0xa9},
    {"DBG_BT_RF",           0xaa},
    {"DBG_BT_BLE",          0xab},
    {"DBG_BT_BREDR",        0xac},
    {"DBG_WF_SDC",          0xad},
    {"DBG_WF_SDD",          0xae},

    {"DBG_UDMA_00",         0xe0},
    {"DBG_UDMA_01",         0xe1},
    {"DBG_UDMA_02",         0xe2},
    {"DBG_UDMA_03",         0xe3},
    {"DBG_UDMA_04",         0xe4},
    {"DBG_UDMA_05",         0xe5},
    {"DBG_UDMA_06",         0xe6},
    {"DBG_UDMA_07",         0xe7},
    {"DBG_UDMA_08",         0xe8},
    {"DBG_UDMA_09",         0xe9},
    {"DBG_UDMA_0A",         0xea},
    {"DBG_UDMA_0B",         0xeb},
    {"DBG_UDMA_0C",         0xec},
    {"DBG_UDMA_0D",         0xed},
    {"DBG_UDMA_0E",         0xee},
    {"DBG_UDMA_0F",         0xef},

    {"DBG_CPU0_WR",         0xf0},
    {"DBG_CPU0_RD",         0xf1},
    {"DBG_CPU0_IF",         0xf2},
    {"DBG_CPU1_WR",         0xf3},
    {"DBG_CPU1_RD",         0xf4},
    {"DBG_CPU1_IF",         0xf5},
    {"DBG_CPU2_WR",         0xf6},
    {"DBG_CPU2_RD",         0xf7},
    {"DBG_CPU2_IF",         0xf8},
    {"DBG_CPU3_WR",         0xf9},
    {"DBG_CPU3_RD",         0xfa},
    {"DBG_CPU3_IF",         0xfb},

    {"DBG_SDTAP",           0xff},
    {"MSG_NULL",            0xff}
};

static const struct dev_id_str tzasc_dev_id_list[] = {
    {"TZASC_DCACHE",        0x00},
    {"TZASC_ICACHE",        0x01},
    {"TZASC_DMA_COPY",      0x02},
    {"TZASC_JPG0",          0x03},
    {"TZASC_JPG1",          0x04},
    {"TZASC_IMB",           0x05},
    {"TZASC_IMD",           0x06},
    {"TZASC_IMG",           0x07},
};

//====================================================================//
//                        CPU异常检测部分                             //
//====================================================================//
static const char *const emu_msg[32] = {
    "misalign_err",     //0
    "illeg_err",        //1
    "div0_err",         //2
    "stackoverflow",    //3

    "pc_limit_err",     //4
    "nsc_err",          //5
    "reserved",         //6
    "reserved",         //7

    "etm_wp0_err",      //8
    "reserved",         //9
    "reserved",         //10
    "reserved",         //11

    "reserved",         //12
    "reserved",         //13
    "reserved",         //14
    "reserved",         //15

    "fpu_ine_err",      //16
    "fpu_huge_err",     //17
    "fpu_tiny",         //18
    "fpu_inf_err",      //19

    "fpu_inv_err",      //20
    "reserved",         //21
    "reserved",         //22
    "reserved",         //23

    "reserved",         //24
    "reserved",         //25
    "reserved",         //26
    "reserved",         //27

    "reserved",         //28
    "dcu_emu_err",      //29
    "icu_emu_err",      //30
    "sys_emu_err",      //31
};

//====================================================================//
//                      HCORE异常检测部分                             //
//====================================================================//
static const char *const hcore_emu_msg0[] = {
    "hmem access mpu virtual exception",   //[0]: TZMPU检测到虚拟地址访问内存权限越界，需要读 EMU_ID 分析外设号;
    "hmem access mpu physical exception",  //[1]: TZMPU检测到物理地址访问内存权限越界，需要读 EMU_ID 分析外设号;
    "hmem access mmu exception",           //[2]: MMU访问TLB1异常，检测到TLB1的valid位为0或TLB1寻址范围超过设定的范围，需要读 EMU_ID 分析外设号;
    "hmem access inv exception",           //[3]: hemm内存操作访问到空白区或MMU的TLB1保护的空间或MMU的TLB1设置在空白区，需要读 EMU_ID 分析外设号;
    "sfr_wr_inv or csfr_wr_inv",           //[4]: 写到 syfssr 或者csfr 空白区，需要读 EMU_ID 分析外设号;
    "sfr_rd_inv or csfr_rd_inv",           //[5]: 读到 syfssr 或者csfr 空白区，需要读 EMU_ID 分析外设号;
    "sfr_ass_inv",                         //[6]: 在音频子系统未复位时访问到音频的SFR，需要读 EMU_ID 分析外设号;
    "sfr_video_inv",                       //[7]: 在视频子系统未复位时访问到视频的SFR，需要读 EMU_ID 分析外设号;
    "sfr_wl_inv",                          //[8]: 在WL子系统未复位时访问到WL的SFR，需要读 EMU_ID 分析外设号;
    "lsb emu err",                         //[9]: 非HCORE异常, 属于LSB异常, 需要分析LSB相关EMU_MSG;
    "hsb emu err",                         //[10]: 非HCORE异常, 属于HSB异常, 需要分析HSB相关EMU_MSG;
    "ass emu err",                         //[11]: 非HCORE异常, 属于ASS异常, 需要分析ASS相关EMU_MSG;
    "video emu err",                       //[12]: 非HCORE异常, 属于VIDEO异常, 需要分析VIDEO相关EMU_MSG;
    "wl emu err",                          //[13]: 非HCORE异常, 属于WL异常, 需要分析WL相关EMU_MSG;
    "ilock_err",                           //[14]: 多核互锁使用异常，同一个核连续调用lockset或者另外一个核调用lockclr;
    "l1p request write rocache err",       //[15]: 有写请求访问rocache，需要读 EMU_ID 分析外设号
    "l1p dcache emu err",                  //[16]: 需要调用DcuEmuMessage()分析异常原因
    "l1i ram mapping access excption",     //[17]: 访问到l1icache sfr或mapping预留区域
    "l1d ram mapping access excption",     //[18]: 访问到l1dcache sfr或mapping预留区域
    "l2i ram mapping access excption",     //[19]: 不存在L2I缓存时，访问到l2icache sfr或mapping预留区域
    "l2d ram mapping access excption",     //[20]: 不存在L2D缓存时，访问到l2dcache sfr或mapping预留区域
    "prp ram mapping access excption",     //[21]: 访问外设专用RAM异常，需要读 EMU_ID 分析外设号;
};

//====================================================================//
//                      HSB异常检测部分                               //
//====================================================================//
static const char *const hsb_emu_msg0[] = {
    "device write hsb sfr reserved memory",//[0]: hsb_sfr_wr_inv, 写到 HSB SFR 空白区，需要读 EMU_ID 分析外设号;
    "device read hsb sfr reserved memory", //[1]: hsb_sfr_rd_inv, 读到 HSB SFR 空白区，需要读 EMU_ID 分析外设号;
    "axi write reserved memory",           //[2]: axis_winv, Axi 主机，写到空白区，需要读 EMU_ID 分析外设号;
    "axi read reserved memory",            //[3]: axis_rinv, Axi 主机，读到空白区，需要读 EMU_ID 分析外设号;
    "hsb tzasc exception",                 //[4]: TZASC模块异常
    "sfc0 mmu exception",                  //[5]: SFC0_MMU模块异常
    "sfc1 mmu exception",                  //[6]: SFC1_MMU模块异常
    "watchdog time out",                   //[7]: watchdog_exception, 看门狗超时异常;
    "vlvd exception",                      //[8]: vlvd异常
    "dmc exception",                       //[9]: DMC控制器异常(主机端rready bready被拉低或dmc权限控制框越界)
};

//====================================================================//
//                      LSB异常检测部分                               //
//====================================================================//
static const char *const lsb_emu_msg0[] = {
    "device write lsb sfr reserved memory",//[0]: lsb_sfr_wr_inv, 写到 LSB SFR 空白区，需要读 EMU_ID 分析外设号;
    "device read lsb sfr reserved memory", //[1]: lsb_sfr_rd_inv, 读到 LSB SFR 空白区，需要读 EMU_ID 分析外设号;
};

//====================================================================//
//                      ASS异常检测部分                               //
//====================================================================//
static const char *const ass_emu_msg0[] = {
    "device write ass sfr reserved memory",//[0]: ass_sfr_wr_inv, 写到 ASS SFR 空白区，需要读 EMU_ID 分析外设号;
    "device read ass sfr reserved memory", //[1]: ass_sfr_rd_inv, 读到 ASS SFR 空白区，需要读 EMU_ID 分析外设号;
};

//====================================================================//
//                     VIDEO异常检测部分                              //
//====================================================================//
static const char *const video_emu_msg0[] = {
    "device write video sfr reserved memory",//[0]: video_sfr_wr_inv, 写到 VIDEO SFR 空白区，需要读 EMU_ID 分析外设号;
    "device read video sfr reserved memory", //[1]: video_sfr_rd_inv, 读到 VIDEO SFR 空白区，需要读 EMU_ID 分析外设号;
};

//====================================================================//
//                       WL异常检测部分                               //
//====================================================================//
static const char *const wl_emu_msg0[] = {
    "device write wl sfr reserved memory",//[0]: wl_sfr_wr_inv, 写到 WL SFR 空白区，需要读 EMU_ID 分析外设号;
    "device read wl sfr reserved memory", //[1]: wl_sfr_rd_inv, 读到 WL SFR 空白区，需要读 EMU_ID 分析外设号;
};

//====================================================================//
//                      CACHE异常检测部分                             //
//====================================================================//
static const char *const icache_emu_msg[] = {
    "icache emu error -> iway_rhit_err",   //[0]: 访问ICACHE时，命中了多块WAY，ITAG记录出错
    "icache emu error -> iway_lock_err",   //[1]: 访问ICACHE时，所有WAY都被锁定，无法缓存刚取的指令
    "icache emu error -> ireq_rack_inv",   //[2]: ICACHE关闭时，程序访问了ICACHE区域
    "icache emu error -> islv_inv",        //[3]: 访问到ICACHE_MAPPING的空白区，或ICACHE使用过程中，触发了ICACHE的写保护，需要读EMU_ID分析外设号
    "icache emu error -> icmd_wkst_err",   //[4]: ITAG命令正在进行时，又启动另一个命令
    "icache emu error -> icmd_lock_err",   //[5]: ICACHE锁定代码将所有WAY都锁定了
};

static const char *const dcache_emu_msg[] = {
    "dcache emu error -> rreq_err",        //[0]: 读请求访问DCACHE时，发现DTAG记录出错，命中了多块WAY，或读请求传输的CACHE属性不支持
    "dcache emu error -> wreq_err",        //[1]: 写请求访问DCACHE时，发现DTAG记录出错，命中了多块WAY，或写请求传输的CACHE属性不支持
    "dcache emu error -> snoop_err",       //[2]: 监听器访问DCACHE时，发现DTAG记录出错，命中了多块WAY，或监听器的CACHE属性不支持
    "dcache emu error -> miss_err",        //[3]: MISS通道返回的读响应信息出错
    "dcache emu error -> dcu_ack_inv",     //[4]: DCACHE在DCU_EN未打开时，接受到读写请求或维护请求
    "dcache emu error -> dslv_inv",        //[5]: 访问到DCACHE_MAPPING的空白区，或DCACHE使用的过程中，触发了DCACHE的写保护，需要读EMU_ID分析外设号
};

//====================================================================//
//                        MMU异常检测部分                             //
//====================================================================//
static const char *const sfc_mmu_err_msg[] = {
    "sfc mmu no error",                    //[0]: 无异常
    "sfc mmu close",                       //[1]: 在MMU_EN=0的情况下访问虚拟空间
    "sfc mmu virtual addr error",          //[2]: 在地址转换过程中访问TLB1，但访问的地址不属于TLB1空间
    "sfc mmu TLB1 is invalid",             //[3]: 在地址转换过程中访问TLB1，读到的TLB1数据是无效的
};


const char *get_debug_dev_name(u32 id)
{
    for (int i = 0; i < ARRAY_SIZE(dev_id_list); ++i) {
        if (dev_id_list[i].id == (id & 0xff)) {
            return dev_id_list[i].name;
        }
    }
    return NULL;
}

const char *get_debug_tzasc_dev_name(u32 id)
{
    for (int i = 0; i < ARRAY_SIZE(tzasc_dev_id_list); ++i) {
        if (tzasc_dev_id_list[i].id == (id & 0xff)) {
            return tzasc_dev_id_list[i].name;
        }
    }
    return NULL;
}

u32 get_debug_dev_id(const char *name)
{
    for (int i = 0; i < ARRAY_SIZE(dev_id_list); ++i) {
        if (!strcmp(dev_id_list[i].name, name)) {
            return dev_id_list[i].id;
        }
    }

    return -1;
}


#if EXPCPTION_IN_SRAM
static u32 deb_power(u8 x, u8 n)
{
    u64 result = 1;
    for (int i = 0; i < n; i++) {
        result *= x;
    }
    return (u32)result;
}
static void deb_putchar(char c)
{
    if (DEBUG_UART->CON0 & BIT(0)) {
        if (c == '\r') {
            return;
        }
        if (c == '\n') {
            DEBUG_UART->CON0 |= BIT(13);
            DEBUG_UART->BUF = '\r';
            __asm_csync();
            while ((DEBUG_UART->CON0 & BIT(15)) == 0);
        }
        DEBUG_UART->CON0 |= BIT(13);
        DEBUG_UART->BUF = c;
        __asm_csync();
        while ((DEBUG_UART->CON0 & BIT(15)) == 0);
    }
}
static void deb_putu4hex(u8 dat)
{
    dat = 0xf & dat;
    if (dat > 9) {
        deb_putchar(dat - 10 + 'a');
    } else {
        deb_putchar(dat + '0');
    }
}
static void deb_puthex(u32 value)
{
    u8 start_flag = 0;
    if (value) {
        for (u8 i = 8; i > 0; i--) {
            u8 tmp = 4 * (i - 1);
            u8 dat = ((value & 0xf << tmp) >> tmp);
            if (dat || start_flag) {
                deb_putu4hex(dat);
                start_flag = 1;
            }
        }
    } else  {
        deb_putu4hex(0);
    }
}
static void deb_putdec(u32 value)
{
    u8 start_flag = 0;
    if (value) {
        for (u8 i = 9; i > 0; i--) {
            u32 tmp = deb_power(10, i - 1);
            u8 dat = (value / tmp % 10);
            if (dat || start_flag) {
                deb_putu4hex(dat);
                start_flag = 1;
            }
        }
    } else  {
        deb_putu4hex(0);
    }
}
static void deb_putstr(char *str)
{
    if (str == NULL) {
        return;
    }
    while (*str != '\0') {
        deb_putchar(*str);
        str++;
    }
}
static void deb_putbuf(u8 *buf, u32 len)
{
    u32 tmp = len;
    while (len--) {
        deb_putu4hex((*buf >> 4) & 0xf);
        deb_putu4hex(*buf & 0xf);
        deb_putchar(' ');
        buf++;

        if (!((tmp - len) % 16)) {
            deb_putchar('\n');
        }
    }
}
static void deb_printf(char *format, ...)
{
    va_list args;

    va_start(args, format);

    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
            if (*format == 's') {
                char *s = (char *)va_arg(args, int);
                deb_putstr(s ? s : "(null)");
                continue;
            }
            if (*format == 'd') {
                deb_putdec(va_arg(args, int));
                continue;
            }
            if (*format == 'x') {
                deb_puthex(va_arg(args, int));
                continue;
            }

        } else {
            deb_putchar(*format);
        }
    }

    va_end(args);
}
#endif


#ifdef CONFIG_SAVE_EXCEPTION_LOG_IN_FLASH

#define LOG_PATH "mnt/sdfile/EXT_RESERVED/log"
#define EXCEPTION_LOG_BUF_SIZE 1500

static u32 exception_log_flash_addr sec(.volatile_ram);
static char exception_log_buf[EXCEPTION_LOG_BUF_SIZE];
static u32 exception_log_cnt sec(.volatile_ram);

static int get_exception_log_flash_addr(void)
{
    struct vfs_attr file_attr;

    FILE *fp = fopen(LOG_PATH, "r");
    if (fp == NULL) {
        return -1;
    }
    fget_attrs(fp, &file_attr);
    fclose(fp);
    exception_log_flash_addr = file_attr.sclust;
    exception_log_buf[0] = -1;
    sdfile_reserve_zone_read(exception_log_buf, exception_log_flash_addr, sizeof(exception_log_buf), 0);
    if (exception_log_buf[0] != -1) {
        printf("last exception log : \n%s\n", exception_log_buf);
        sdfile_reserve_zone_erase(exception_log_flash_addr, SDFILE_SECTOR_SIZE, 0);
    }
    memset(exception_log_buf, 0, sizeof(exception_log_buf));
    return 0;
}
platform_initcall(get_exception_log_flash_addr);

static void write_exception_log_to_flash(void *data, u32 len)
{
    if (exception_log_flash_addr == 0) {
        return;
    }
    extern void set_os_init_flag(u8 value);
    set_os_init_flag(0);
    extern void norflash_set_write_cpu_hold(u8 hold_en);
    norflash_set_write_cpu_hold(0);
    sdfile_reserve_zone_erase(exception_log_flash_addr, SDFILE_SECTOR_SIZE, 0);
    sdfile_reserve_zone_write(data, exception_log_flash_addr, len, 0);
}
#endif


#ifdef CONFIG_SAVE_EXCEPTION_LOG_IN_FLASH
static int exception_printf(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vprintf(format, args);
    va_end(args);

    if (ret < 0) {
        return ret;
    }

    va_start(args, format);
    exception_log_cnt += vsnprintf(\
                                   exception_log_buf + exception_log_cnt, \
                                   sizeof(exception_log_buf) - exception_log_cnt, \
                                   format,
                                   args);
    va_end(args);

    return exception_log_cnt;
}

#define EXCEPTION_PRINTF(fmt, ...) exception_printf(fmt"\r\n", ##__VA_ARGS__)

#else

#if EXPCPTION_IN_SRAM
#define EXCEPTION_PRINTF(fmt, ...) deb_printf(fmt"\r\n", ##__VA_ARGS__)
#else
#define EXCEPTION_PRINTF(fmt, ...) printf(fmt"\r\n", ##__VA_ARGS__)
#endif

#endif

static void trace_call_stack(void)
{
    for (int c = 0; c < CPU_CORE_NUM; c++) {
        EXCEPTION_PRINTF("CPU%d %x-->%x-->%x-->%x-->%x-->%x", c,
                         q32DSP(c)->ETM_PC5, q32DSP(c)->ETM_PC4,
                         q32DSP(c)->ETM_PC3, q32DSP(c)->ETM_PC2,
                         q32DSP(c)->ETM_PC1, q32DSP(c)->ETM_PC0);
    }
}

#if (defined TCFG_DEBUG_DLOG_ENABLE && TCFG_DEBUG_DLOG_ENABLE == 1)
void dlog_exception_log_to_flash(void)
{
#if (TCFG_DEBUG_DLOG_FLASH_SEL == 0)      // 内置flash
    extern void norflash_set_write_cpu_hold(u8 hold_en);
    norflash_set_write_cpu_hold(0);
#endif

    int dlog_flush_all_cache_2_flash();
    dlog_flush_all_cache_2_flash();
}
#endif

void exception_analyze(int *sp)
{
    log_output_release_deadlock();

    u32 cpu_id = current_cpu_id();
    q32DSP(!cpu_id)->CMD_PAUSE = -1;
    q32DSP(!cpu_id)->PMU_CON0 &= ~BIT(0);
    __asm_csync();

#if (defined TCFG_DEBUG_DLOG_ENABLE && TCFG_DEBUG_DLOG_ENABLE == 1)
    void dlog_uart_set_isr_mode(u8 mode);
    dlog_uart_set_isr_mode(0);
    extern void set_os_init_flag(u8 value);
    set_os_init_flag(0);
    OS_ENTER_CRITICAL(); //禁止操作系统
#endif
    EXCEPTION_PRINTF("\r\n\r\n---------------------exception error ------------------------\r\n\r\n");
    EXCEPTION_PRINTF("CPU%d %s, EMU_MSG = 0x%x", cpu_id, __func__, q32DSP(cpu_id)->EMU_MSG);
    EXCEPTION_PRINTF("CPU%d run addr = 0x%x", !cpu_id, q32DSP(!cpu_id)->PCRS);

    EXCEPTION_PRINTF("JL_CEMU->MSG0:0x%x, MSG1:0x%x, MSG2:0x%x, ID:0x%x, 0x%x, 0x%x, 0x%x", JL_CEMU->MSG0, JL_CEMU->MSG1, JL_CEMU->MSG2, JL_CEMU->LOG0, JL_CEMU->LOG1, JL_CEMU->LOG2, JL_CEMU->LOG3);
    EXCEPTION_PRINTF("JL_HEMU->MSG0:0x%x, MSG1:0x%x, MSG2:0x%x, ID:0x%x, 0x%x, 0x%x, 0x%x", JL_HEMU->MSG0, JL_HEMU->MSG1, JL_HEMU->MSG2, JL_HEMU->LOG0, JL_HEMU->LOG1, JL_HEMU->LOG2, JL_HEMU->LOG3);
    EXCEPTION_PRINTF("JL_LEMU->MSG0:0x%x, MSG1:0x%x, MSG2:0x%x, ID:0x%x, 0x%x, 0x%x, 0x%x", JL_LEMU->MSG0, JL_LEMU->MSG1, JL_LEMU->MSG2, JL_LEMU->LOG0, JL_LEMU->LOG1, JL_LEMU->LOG2, JL_LEMU->LOG3);
    EXCEPTION_PRINTF("JL_AEMU->MSG0:0x%x, MSG1:0x%x, MSG2:0x%x, ID:0x%x, 0x%x, 0x%x, 0x%x", JL_AEMU->MSG0, JL_AEMU->MSG1, JL_AEMU->MSG2, JL_AEMU->LOG0, JL_AEMU->LOG1, JL_AEMU->LOG2, JL_AEMU->LOG3);
    EXCEPTION_PRINTF("JL_VEMU->MSG0:0x%x, MSG1:0x%x, MSG2:0x%x, ID:0x%x, 0x%x, 0x%x, 0x%x", JL_VEMU->MSG0, JL_VEMU->MSG1, JL_VEMU->MSG2, JL_VEMU->LOG0, JL_VEMU->LOG1, JL_VEMU->LOG2, JL_VEMU->LOG3);
    EXCEPTION_PRINTF("JL_WEMU->MSG0:0x%x, MSG1:0x%x, MSG2:0x%x, ID:0x%x, 0x%x, 0x%x, 0x%x", JL_WEMU->MSG0, JL_WEMU->MSG1, JL_WEMU->MSG2, JL_WEMU->LOG0, JL_WEMU->LOG1, JL_WEMU->LOG2, JL_WEMU->LOG3);

    trace_call_stack();

    unsigned int reti = sp[16];
    unsigned int rete = sp[17];
    unsigned int retx = sp[18];
    unsigned int rets = sp[19];
    unsigned int psr  = sp[20];
    unsigned int icfg = sp[21];
    unsigned int usp  = sp[22];
    unsigned int ssp  = sp[23];
    unsigned int _sp  = sp[24];

    EXCEPTION_PRINTF("exception cpu %d info : ", cpu_id);

    for (int r = 0; r < 16; r++) {
        EXCEPTION_PRINTF("R%d: %x", r, sp[r]);
    }

    EXCEPTION_PRINTF("icfg: %x", icfg);
    EXCEPTION_PRINTF("psr:  %x", psr);
    EXCEPTION_PRINTF("rets: 0x%x", rets);
    EXCEPTION_PRINTF("reti: 0x%x", reti);
    EXCEPTION_PRINTF("usp : %x, ssp : %x sp: %x", usp, ssp, _sp);

    const volatile u32 *rm = &(q32DSP(!cpu_id)->DR00);
    EXCEPTION_PRINTF("other cpu %d info : ", !cpu_id);

    for (int r = 0; r < 16; r++) {
        EXCEPTION_PRINTF("R%d: %x", r, rm[r]);
    }

    EXCEPTION_PRINTF("icfg: %x", q32DSP(!cpu_id)->ICFG);
    EXCEPTION_PRINTF("psr:  %x", q32DSP(!cpu_id)->PSR);
    EXCEPTION_PRINTF("rets: 0x%x", q32DSP(!cpu_id)->RETS);
    EXCEPTION_PRINTF("reti: 0x%x", q32DSP(!cpu_id)->RETI);
    EXCEPTION_PRINTF("usp : %x, ssp : %x sp: %x", q32DSP(!cpu_id)->USP, q32DSP(!cpu_id)->SSP, q32DSP(!cpu_id)->SP);

    // CPU -> HCORE -> HSB -> LSB ASS VIDEO WL

    /*------------------------------------
      CPU 内核
      ------------------------------------*/
    for (int i = 0; i < 32; ++i) {
        if (q32DSP(cpu_id)->EMU_MSG & BIT(i)) {
            EXCEPTION_PRINTF("[0-CPU] cpu%d emu err msg : %s", cpu_id, emu_msg[i]);

            if (i == 3) {
                if (os_current_task_rom()) {
                    EXCEPTION_PRINTF("current_task : %s", os_current_task_rom());
                }
                EXCEPTION_PRINTF("usp limit %x %x, ssp limit %x %x",
                                 q32DSP(cpu_id)->EMU_USP_L, q32DSP(cpu_id)->EMU_USP_H, q32DSP(cpu_id)->EMU_SSP_L, q32DSP(cpu_id)->EMU_SSP_H);
            } else if (i == 4) {
                EXCEPTION_PRINTF("PC_LIMIT0_L:0x%x, PC_LIMIT0_H:0x%x, PC_LIMIT1_L:0x%x, PC_LIMIT1_H:0x%x, PC_LIMIT2_L:0x%x, PC_LIMIT2_H:0x%x",
                                 q32DSP(cpu_id)->LIM_PC0_L, q32DSP(cpu_id)->LIM_PC0_H,
                                 q32DSP(cpu_id)->LIM_PC1_L, q32DSP(cpu_id)->LIM_PC1_H,
                                 q32DSP(cpu_id)->LIM_PC2_L, q32DSP(cpu_id)->LIM_PC2_H);
            } else if (i == 8) {
                EXCEPTION_PRINTF("ETM limit exception datl:0x%x, dath:0x%x, addrl:0x%x, addrh:0x%x",
                                 q32DSP(cpu_id)->WP0_DATL, q32DSP(cpu_id)->WP0_DATH, q32DSP(cpu_id)->WP0_ADRL, q32DSP(cpu_id)->WP0_ADRH);
            } else if (i == 29) {
                DcuEmuMessage();
                EXCEPTION_PRINTF("except at dev : %s %s", get_debug_dev_name(JL_L1P->EMU_ID & 0xff), (JL_L1P->EMU_ID & BIT(31)) ? "not safe" : "safe");
            } else if (i == 30) {
                for (int j = 0; j < ARRAY_SIZE(icache_emu_msg); ++j) {
                    if (q32DSP_icu(cpu_id)->EMU_MSG & BIT(j)) {
                        EXCEPTION_PRINTF("[X-ICACHE] emu err msg : %s", icache_emu_msg[j]);
                        if (j == 3) {
                            EXCEPTION_PRINTF("except at dev : %s %s", get_debug_dev_name(q32DSP_icu(cpu_id)->EMU_ID & 0xff), (q32DSP_icu(cpu_id)->EMU_ID & BIT(31)) ? "not safe" : "safe");
                        }
                    }
                }
            } else if (i == 31) {
                /*------------------------------------
                  HCORE
                  ------------------------------------*/
                for (int j = 0; j < ARRAY_SIZE(hcore_emu_msg0); ++j) {
                    if (JL_CEMU->MSG0 & BIT(j)) {
                        EXCEPTION_PRINTF("[1-HCORE] emu err msg : %s", hcore_emu_msg0[j]);
                        if (j == 14) {
                            DcuEmuMessage();
                            EXCEPTION_PRINTF("except at dev : %s %s", get_debug_dev_name(JL_L1P->EMU_ID & 0xff), (JL_L1P->EMU_ID & BIT(31)) ? "not safe" : "safe");
                        }
                        if (j < 8 || j == 13 || j == 19) {
                            EXCEPTION_PRINTF("except at dev : %s", get_debug_dev_name(JL_CEMU->LOG0 & 0xff));
                        }
                        if (j < 4) {
                            if (JL_CEMU->LOG1 && (j == 0 || j == 2)) {
                                EXCEPTION_PRINTF("%s virtual memory err, addr : 0x%x", (JL_CEMU->LOG0 & BIT(30)) ? "write" : "read", JL_CEMU->LOG1);
                            }
                            if (JL_CEMU->LOG2 && (j == 1 || j == 3)) {
                                EXCEPTION_PRINTF("%s physical memory err, addr : 0x%x", (JL_CEMU->LOG0 & BIT(29)) ? "write" : "read", JL_CEMU->LOG2);
                            }
                        } else if (j < 6 || j == 19) {
                            EXCEPTION_PRINTF("except memory addr : 0x%x, %s", JL_CEMU->LOG1, (JL_CEMU->LOG0 & BIT(31)) ? "not safe" : "safe");
                        }
                        if (j < 2) {
                            int k = 0;
                            int id = JL_CEMU->LOG3 & 0xff;
                            while (!(id & BIT(k)) && ++k < 8) {};
                            EXCEPTION_PRINTF("tzmpu limit range index : %d", k);
                        }
                    }
                }
                /*------------------------------------
                  HSB
                  ------------------------------------*/
                if (JL_CEMU->MSG0 & BIT(10)) {
                    for (int j = 0; j < ARRAY_SIZE(hsb_emu_msg0); ++j) {
                        if (JL_HEMU->MSG0 & BIT(j)) {
                            EXCEPTION_PRINTF("[2-HSB] emu err msg : %s", hsb_emu_msg0[j]);
                            if (j < 4) {
                                EXCEPTION_PRINTF("except at dev : %s, sfr addr : 0x%x", get_debug_dev_name(JL_HEMU->LOG0 & 0xff), hs_base | (JL_HEMU->LOG1 & 0xffff));
                            } else if (j == 4) {
                                EXCEPTION_PRINTF("except at dev : %s, %s, %s, if over 4KB %s, err len %d, err each size %d, mem addr : 0x%x(align 32 bytes)",
                                                 get_debug_tzasc_dev_name((JL_HEMU->LOG0 & 0x7fff) >> 8),
                                                 (JL_HEMU->LOG0 & BIT(30)) ? "write" : "read",
                                                 (JL_HEMU->LOG0 & BIT(31)) ? "not safe" : "safe",
                                                 (JL_HEMU->LOG0 & BIT(15)) ? "true" : "flase",
                                                 (JL_HEMU->LOG0 >> 16) & 0x7f,
                                                 (JL_HEMU->LOG0 >> 24) & 0x7,
                                                 JL_HEMU->LOG1);
                                int k = 0;
                                int id = JL_HEMU->LOG2 & 0x1f;
                                while (!(id & BIT(k)) && ++k < 4) {};
                                EXCEPTION_PRINTF("tzasc limit range index : %d", k);
                            } else if (j < 7) {
                                EXCEPTION_PRINTF("sfc mmu err msg : %s", sfc_mmu_err_msg[j == 5 ? (JL_SFC0_MMU->CON0 >> 30) : (JL_SFC1_MMU->CON0 >> 30)]);
                            }
                        }
                    }
                }
                /*------------------------------------
                  LSB
                  ------------------------------------*/
                if (JL_CEMU->MSG0 & BIT(9)) {
                    for (int j = 0; j < ARRAY_SIZE(lsb_emu_msg0); ++j) {
                        if (JL_LEMU->MSG0 & BIT(j)) {
                            EXCEPTION_PRINTF("[3-LSB] emu err msg : %s", lsb_emu_msg0[j]);
                            EXCEPTION_PRINTF("except at dev : %s, sfr addr : 0x%x", get_debug_dev_name(JL_LEMU->LOG0 & 0xff), ls_base | (JL_LEMU->LOG1 & 0xffff));
                        }
                    }
                }
                /*------------------------------------
                  ASS
                  ------------------------------------*/
                if (JL_CEMU->MSG0 & BIT(11)) {
                    for (int j = 0; j < ARRAY_SIZE(ass_emu_msg0); ++j) {
                        if (JL_AEMU->MSG0 & BIT(j)) {
                            EXCEPTION_PRINTF("[4-ASS] emu err msg : %s", ass_emu_msg0[j]);
                            EXCEPTION_PRINTF("except at dev : %s, sfr addr : 0x%x", get_debug_dev_name(JL_AEMU->LOG0 & 0xff), as_base | (JL_AEMU->LOG1 & 0xffff));
                        }
                    }
                }
                /*------------------------------------
                  VIDEO
                  ------------------------------------*/
                if (JL_CEMU->MSG0 & BIT(12)) {
                    for (int j = 0; j < ARRAY_SIZE(video_emu_msg0); ++j) {
                        if (JL_VEMU->MSG0 & BIT(j)) {
                            EXCEPTION_PRINTF("[5-VIDEO] emu err msg : %s", video_emu_msg0[j]);
                            EXCEPTION_PRINTF("except at dev : %s, sfr addr : 0x%x", get_debug_dev_name(JL_VEMU->LOG0 & 0xff), JL_VEMU->LOG1 & 0xffff);
                        }
                    }
                }
                /*------------------------------------
                  WL
                  ------------------------------------*/
                if (JL_CEMU->MSG0 & BIT(13)) {
                    for (int j = 0; j < ARRAY_SIZE(wl_emu_msg0); ++j) {
                        if (JL_WEMU->MSG0 & BIT(j)) {
                            EXCEPTION_PRINTF("[6-WL] emu err msg : %s", wl_emu_msg0[j]);
                            EXCEPTION_PRINTF("except at dev : %s, sfr addr : 0x%x", get_debug_dev_name(JL_WEMU->LOG0 & 0xff), wl_base | (JL_WEMU->LOG1 & 0x1ffff));
                        }
                    }
                }
            }
        }
    }

#ifdef CONFIG_SAVE_EXCEPTION_LOG_IN_FLASH
    printf("exception log buf len : %d", exception_log_cnt);
    write_exception_log_to_flash(exception_log_buf, exception_log_cnt + 1);
#endif

    EXCEPTION_PRINTF("system_reset...\r\n\r\n\r\n");
    log_flush();    //只能在异常中断里调用
    __wdt_clear();
#if (defined TCFG_DEBUG_DLOG_ENABLE && TCFG_DEBUG_DLOG_ENABLE == 1)
    dlog_exception_log_to_flash();
#endif
#ifdef SDTAP_DEBUG
    sdtap_init(2);//防止IO被占据,重新初始化SDTAP
    while (1);
#endif
#ifndef CONFIG_FPGA_ENABLE
    __cpu_reset();
    while (1);
#else
    while (1);
#endif
}

void debug_register_isr(void)
{
    request_irq(IRQ_EXCEPTION_IDX, 7, exception_irq_handler, 0xff);
}
