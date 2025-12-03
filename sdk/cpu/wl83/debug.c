#include "asm/debug.h"
#include "asm/cache.h"
#include "asm/wdt.h"
#include "app_config.h"

#define LOG_TAG_CONST       DEBUG
#define LOG_TAG             "[DEBUG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

extern void sdtap_init(u32 ch);
extern void debug_register_isr(void);

static void pc0_rang_limit(void *low_addr, void *high_addr)
{
    log_info("pc0 :%x---%x", (u32)low_addr, (u32)high_addr);

    q32DSP(current_cpu_id())->LIM_PC0_H = (u32)high_addr;
    q32DSP(current_cpu_id())->LIM_PC0_L = (u32)low_addr;
}

static void pc1_rang_limit(void *low_addr, void *high_addr)
{
    log_info("pc1 :%x---%x", (u32)low_addr, (u32)high_addr);

    q32DSP(current_cpu_id())->LIM_PC1_H = (u32)high_addr;
    q32DSP(current_cpu_id())->LIM_PC1_L = (u32)low_addr;
}

static void pc2_rang_limit(void *low_addr, void *high_addr)
{
    log_info("pc2 :%x---%x", (u32)low_addr, (u32)high_addr);

    q32DSP(current_cpu_id())->LIM_PC2_H = (u32)high_addr;
    q32DSP(current_cpu_id())->LIM_PC2_L = (u32)low_addr;
}

void pc_rang_limit(void *low_addr0, void *high_addr0, void *low_addr1, void *high_addr1, void *low_addr2, void *high_addr2)
{
    if (low_addr2 && high_addr2 && high_addr2 > low_addr2) {
        pc2_rang_limit(low_addr2, high_addr2);
    }

    if (low_addr1 && high_addr1 && high_addr1 > low_addr1) {
        pc1_rang_limit(low_addr1, high_addr1);
    }

    if (low_addr0 && high_addr0 && high_addr0 > low_addr0) {
        pc0_rang_limit(low_addr0, high_addr0);
    }
}

void cpu_etm_range_value_limit_detect(void *low_addr, void *high_addr, u32 low_limit_value, u32 high_limit_value, ETM_DETECT_MODE mode, int limit_range_out)
{
    int cpu_id = current_cpu_id();

    q32DSP(cpu_id)->ETM_CON &= ~(0x7 << 9);

    if (limit_range_out) {
        q32DSP(cpu_id)->ETM_CON |= BIT(8);
    } else {
        q32DSP(cpu_id)->ETM_CON &= ~BIT(8);
    }

    q32DSP(cpu_id)->WP0_DATL = low_limit_value;
    q32DSP(cpu_id)->WP0_DATH = high_limit_value;
    q32DSP(cpu_id)->WP0_ADRL = (u32)low_addr;
    q32DSP(cpu_id)->WP0_ADRH = (u32)high_addr;

    q32DSP(cpu_id)->ETM_CON |= mode << 9;

    log_debug("CPU%d ETM_CON:%x, WP0_DATL:0x%x, WP0_DATH:0x%x, WP0_ADRL:0x%x, WP0_ADRH:0x%x",
              cpu_id, q32DSP(cpu_id)->ETM_CON, q32DSP(cpu_id)->WP0_DATL, q32DSP(cpu_id)->WP0_DATH, q32DSP(cpu_id)->WP0_ADRL, q32DSP(cpu_id)->WP0_ADRH);
}

//====================================================================//
//                    MPU权限越界检测设置                             //
//====================================================================//
#define MPU_MAX_SIZE    		8 //MPU区域限制框个数

#define TZMPU_LOCK              (1u<<31)
#define TZMPU_HIDE              (1u<<30)
#define TZMPU_SSMODE            (1u<<29)
#define TZMPU_VCHECK            (1u<<28)
#define TZMPU_INV               (1u<<27)

static int mpu_privilege(int idx, u8 type, u8 did, u8 x, u8 r, u8 w, u8 ns)
{
    u8 i;

    switch (type) {
    case 'C':
        log_info("C[%d] : x(%d) : r(%d) : w(%d)", idx, x, r, w);
        if (ns) {
            return ((x) ? TZMPU_U_XEN : 0) | ((r) ? TZMPU_U_REN : 0) | ((w) ? TZMPU_U_WEN : 0);
        } else {
            return ((x) ? TZMPU_S_XEN : 0) | ((r) ? TZMPU_S_REN : 0) | ((w) ? TZMPU_S_WEN : 0);
        }
        break;
    case 'P':
        log_info("P[%d] : r(%d) : w(%d)", idx, r, w);
        if (ns) {
            return ((r) ? TZMPU_U_PREN : 0) | ((w) ? TZMPU_U_PWEN : 0);
        } else {
            return ((r) ? TZMPU_S_PREN : 0) | ((w) ? TZMPU_S_PWEN : 0);
        }
        break;
    case '0':
    case '1':
    case '2':
    case '3':
        i = type - '0';
        log_info("PID[%d][%d] : did 0x%x : r(%d) : w(%d)", idx, i, did, r, w);
        JL_TZMPU->CID[idx] |= TZMPU_IDx_cfg(i, did);
        if (ns) {
            return TZMPU_UID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        } else {
            return TZMPU_SID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        }
        break;
    default:
        break;
    }

    return 0;
}

// [MPU format]
// begin | end | privilege | inv | pid0_privilege |
static int __parser(int idx, const char *format, va_list argptr)
{
    u8 type = 0;
    u8 did = 0; //device id
    u8 privilege = 0;
    int opt = 0;

    while (*format) {
        switch (*format) {
        case 'C':
            opt |= mpu_privilege(idx, type, did, privilege & BIT(2), privilege & BIT(1), privilege & BIT(0), privilege & BIT(3)); //set last device privilege
            did = 0;
            type = *format;
            privilege = 0; //dev init, clear privilege
            break;
        case 'P':
            opt |= mpu_privilege(idx, type, did, privilege & BIT(2), privilege & BIT(1), privilege & BIT(0), privilege & BIT(3)); //set last device privilege
            did = 0;
            type = *format;
            privilege = 0;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
            opt |= mpu_privilege(idx, type, did, privilege & BIT(2), privilege & BIT(1), privilege & BIT(0), privilege & BIT(3)); //set last device privilege
            did = va_arg(argptr, int);
            type = *format;
            privilege = 0;
            break;

        case 'w':
            privilege |= BIT(0); //cpu/peripheral write en
            break;
        case 'r':
            privilege |= BIT(1); //cpu/peripheral read en
            break;
        case 'x':
            privilege |= BIT(2); //cpu/peripheral execute en
            break;
        case 'n':
            privilege |= BIT(3); //no secure
            break;
        case 'l':
            opt |= TZMPU_LOCK;
            break;
        case 'h':
            opt |= TZMPU_HIDE;
            break;
        case 'v':
            opt |= TZMPU_VCHECK;
            break;
        default:
            break;
        }
        format++;
    }
    opt |= mpu_privilege(idx, type, did, privilege & BIT(2), privilege & BIT(1), privilege & BIT(0), privilege & BIT(3)); //set the last device privilege

    return opt;
}

//=================================================================================//
//@brief: 内存权限保护接口, 可用于对某区域地址配置访问权限, 超出权限则触发系统异常
//@input:
// 		idx: 内存保护框索引, 0 ~ 7, 其中系统保留使用限制框6~7, 用户可以使用限制框0~5, 用于调试;
// 		begin: 保护内存开始地址，4字节对齐;
//      end:   保护内存结束地址，4字节对齐-1;
//      inv:   0: 内存权限检测框内, 1: 内存权限检测框外;
//      format:  设置设备权限, 最多支持cpu + 4个外设读写权限;
//@return:  NULL;
//@note: 当允许所有外设可读写时Prw, 传入的id0/1/2/3变为不允许访问
//@example: 设置限制框2, cpu可读写权限, USB可以读写, BT可读
//        mpu_enable_by_index(2, begin, end, 0, "Crw0rw1r", get_debug_dev_id("DBG_USB"), get_debug_dev_id("DBG_BT"));
//=================================================================================//
void mpu_enable_by_index(int idx, u32 begin, u32 end, u32 inv, const char *format, ...)
{
    int opt = 0;

    if (idx >= MPU_MAX_SIZE) {
        return;
    }

    if (JL_TZMPU->CON[idx] & TZMPU_LOCK) {
        return;
    }

    JL_TZMPU->CON[idx] = 0;
    JL_TZMPU->END[idx] = 0;
    JL_TZMPU->BEG[idx] = 0;
    JL_TZMPU->CID[idx] = 0;

    va_list argptr;
    va_start(argptr, format);

    opt = __parser(idx, format, argptr);

    va_end(argptr);

    if (begin >= 0x400000 && end <= 0x800000) {
        opt |= TZMPU_VCHECK;
    }
    if (inv) {
        opt |= TZMPU_INV;
    }

    JL_TZMPU->BEG[idx] = begin;
    JL_TZMPU->END[idx] = end;

    log_info("JL_TZMPU->CID[%d] 0x%x", idx, JL_TZMPU->CID[idx]);
    log_info("JL_TZMPU->BEG[%d] 0x%x", idx, JL_TZMPU->BEG[idx]);
    log_info("JL_TZMPU->END[%d] 0x%x", idx, JL_TZMPU->END[idx]);

    JL_TZMPU->CON[idx] = opt;

    log_info("JL_TZMPU->CON[%d] 0x%x", idx, JL_TZMPU->CON[idx]);
}

void mpu_disable_by_index(u8 idx)
{
    if (JL_TZMPU->CON[idx] & TZMPU_LOCK) {
        return;
    }
    JL_TZMPU->CON[idx] = 0;
    JL_TZMPU->END[idx] = 0;
    JL_TZMPU->BEG[idx] = 0;
    JL_TZMPU->CID[idx] = 0;
}

void mpu_diasble_all(void)
{
    for (u8 idx = 0; idx < MPU_MAX_SIZE; idx++) {
        if (JL_TZMPU->CON[idx] & TZMPU_LOCK) {
            continue;
        }
        JL_TZMPU->CON[idx] = 0;
        JL_TZMPU->END[idx] = 0;
        JL_TZMPU->BEG[idx] = 0;
        JL_TZMPU->CID[idx] = 0;
    }
}

//====================================================================//
//                   TZASC权限越界检测设置                            //
//====================================================================//
#define TZASC_MAX_SIZE          4 //TZASC区域限制框个数

#define TZASC_LOCK              (1u<<31)
#define TZASC_HIDE              (1u<<30)
#define TZASC_U_WEN             (1u<<11)
#define TZASC_U_REN             (1u<<10)
#define TZASC_S_WEN             (1u<<1)
#define TZASC_S_REN             (1u<<0)
#define TZASC_SID_pen(n, pr, pw) ((pr<<(2+n)) | (pw<<(6+n)))
#define TZASC_UID_pen(n, pr, pw) ((pr<<(12+n)) | (pw<<(16+n)))

static int tzasc_privilege(int idx, u8 type, u16 did, u8 r, u8 w, u8 ns)
{
    u8 i = type - '0';

    switch (type) {
    case 'P':
        log_info("P[%d] : r(%d) : w(%d)", idx, r, w);
        if (ns) {
            return ((r) ? TZASC_U_REN : 0) | ((w) ? TZASC_U_WEN : 0);
        } else {
            return ((r) ? TZASC_S_REN : 0) | ((w) ? TZASC_S_WEN : 0);
        }
        break;
    case '0':
        log_info("PID[%d][%d] : did 0x%x : r(%d) : w(%d)", idx, i, did, r, w);
        JL_TZASC->IDA[idx] |= did;
        if (ns) {
            return TZASC_UID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        } else {
            return TZASC_SID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        }
        break;
    case '1':
        log_info("PID[%d][%d] : did 0x%x : r(%d) : w(%d)", idx, i, did, r, w);
        JL_TZASC->IDA[idx] |= did << 16;
        if (ns) {
            return TZASC_UID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        } else {
            return TZASC_SID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        }
        break;
    case '2':
        log_info("PID[%d][%d] : did 0x%x : r(%d) : w(%d)", idx, i, did, r, w);
        JL_TZASC->IDB[idx] |= did;
        if (ns) {
            return TZASC_UID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        } else {
            return TZASC_SID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        }
        break;
    case '3':
        log_info("PID[%d][%d] : did 0x%x : r(%d) : w(%d)", idx, i, did, r, w);
        JL_TZASC->IDB[idx] |= did << 16;
        if (ns) {
            return TZASC_UID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        } else {
            return TZASC_SID_pen(i, ((r) ? 1 : 0), ((w) ? 1 : 0));
        }
        break;
    default:
        break;
    }

    return 0;
}

// [TZASC format]
// begin | end | privilege | pid0_privilege |
static int __tzasc_parser(int idx, const char *format, va_list argptr)
{
    u8 type = 0;
    u16 did = 0; //axi id
    u8 privilege = 0;
    int opt = 0;

    while (*format) {
        switch (*format) {
        case 'P':
            opt |= tzasc_privilege(idx, type, did, privilege & BIT(1), privilege & BIT(0), privilege & BIT(2)); //set last device privilege
            did = 0;
            type = *format;
            privilege = 0;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
            opt |= tzasc_privilege(idx, type, did, privilege & BIT(1), privilege & BIT(0), privilege & BIT(2)); //set last device privilege
            did = va_arg(argptr, int);
            type = *format;
            privilege = 0;
            break;

        case 'w':
            privilege |= BIT(0); //cpu/peripheral write en
            break;
        case 'r':
            privilege |= BIT(1); //cpu/peripheral read en
            break;
        case 'n':
            privilege |= BIT(2); //no secure
            break;
        case 'l':
            opt |= TZASC_LOCK;
            break;
        case 'h':
            opt |= TZASC_HIDE;
            break;
        default:
            break;
        }
        format++;
    }
    opt |= tzasc_privilege(idx, type, did, privilege & BIT(1), privilege & BIT(0), privilege & BIT(2)); //set the last device privilege

    return opt;
}

//=================================================================================//
//@brief: 外存权限保护接口, 可用于对某区域地址配置访问权限, 超出权限则触发系统异常
//@input:
// 		idx: 外存保护框索引, 0 ~ 3, 其中系统保留使用限制框3和框0（背景区域）, 用户可以使用限制框1~2, 用于调试;
// 		begin: 保护外存开始地址，4字节对齐;
//      end:   保护外存结束地址，4字节对齐-1;
//      format:  设置设备权限, 最多支持4个外设读写权限;
//@return:  NULL;
//@note: 当允许所有外设可读写时Prw, 传入的id0/1/2/3变为不允许访问
//@example: 设置限制框1, 只允许JPG访问
//        tzasc_enable_by_index(1, begin, end, "0rw", AXI_MASTER_JPG);
//=================================================================================//
void tzasc_enable_by_index(int idx, u32 begin, u32 end, const char *format, ...)
{
    int opt = 0;

    if (idx >= TZASC_MAX_SIZE) {
        return;
    }

    if (JL_TZASC->CON[idx] & TZASC_LOCK) {
        return;
    }

    JL_TZASC->CON[idx] = 0;
    JL_TZASC->RGN[idx] = 0;
    JL_TZASC->IDA[idx] = 0;
    JL_TZASC->IDB[idx] = 0;

    va_list argptr;
    va_start(argptr, format);

    opt = __tzasc_parser(idx, format, argptr);

    va_end(argptr);

    u32 size = end - begin + 1;
    u32 sub_region_size, base_addr, region_size = 32 * 1024;
    u8 index = 0;

    while (size > region_size || (begin / region_size) != (end / region_size)) {
        region_size <<= 1;
        ++index;
    }

    base_addr = begin / region_size * region_size;
    sub_region_size = region_size >> 3;

    for (u8 i = 0; i < 8; ++i) {
        if (begin >= base_addr + sub_region_size * (i + 1) || end < base_addr + sub_region_size * i) {
            JL_TZASC->RGN[idx] |= BIT(i);
        }
    }

    JL_TZASC->RGN[idx] |= base_addr;
    JL_TZASC->RGN[idx] |= index << 8;

    if (begin % sub_region_size) {
        log_error("JL_TZASC%d begin addr 0x%x not align %d", idx, begin, sub_region_size);
    }
    if ((end + 1) % sub_region_size) {
        log_error("JL_TZASC%d end addr 0x%x not align %d", idx, end, sub_region_size);
    }

    log_info("JL_TZASC%d base:0x%x, region_size:%d, sub_region_size:%d", idx, base_addr, region_size, sub_region_size);
    log_info("JL_TZASC->RGN[%d] 0x%x", idx, JL_TZASC->RGN[idx]);
    log_info("JL_TZASC->IDA[%d] 0x%x", idx, JL_TZASC->IDA[idx]);
    log_info("JL_TZASC->IDB[%d] 0x%x", idx, JL_TZASC->IDB[idx]);

    JL_TZASC->CON[idx] = opt;

    log_info("JL_TZASC->CON[%d] 0x%x", idx, JL_TZASC->CON[idx]);
}

void tzasc_disable_by_index(u8 idx)
{
    if (JL_TZASC->CON[idx] & TZASC_LOCK) {
        return;
    }
    JL_TZASC->CON[idx] = 0;
    JL_TZASC->RGN[idx] = 0;
    JL_TZASC->IDA[idx] = 0;
    JL_TZASC->IDB[idx] = 0;
}

void tzasc_diasble_all(void)
{
    for (u8 idx = 0; idx < TZASC_MAX_SIZE; idx++) {
        if (JL_TZASC->CON[idx] & TZASC_LOCK) {
            continue;
        }
        JL_TZASC->CON[idx] = 0;
        JL_TZASC->RGN[idx] = 0;
        JL_TZASC->IDA[idx] = 0;
        JL_TZASC->IDB[idx] = 0;
    }
}


#ifdef CONFIG_CPU_EFFICIENCY_CALCULATE_ENABLE
void cpu_efficiency_calculate_show(void)
{
    for (int id = 0; id < CPU_CORE_NUM; id++) {
        //读取统计值
        u32 cnt_chit = q32DSP(id)->CNT_CHIT;
        u32 cnt_cmis = q32DSP(id)->CNT_CMIS;
        u32 cnt_fill = q32DSP(id)->CNT_FILL;
        u32 cnt_ihit = q32DSP(id)->CNT_IHIT;
        u32 cnt_imis = q32DSP(id)->CNT_IMIS;
        u32 cnt_rhit = q32DSP(id)->CNT_RHIT;
        u32 cnt_rmis = q32DSP(id)->CNT_RMIS;
        u32 cnt_whit = q32DSP(id)->CNT_WHIT;
        u32 cnt_wmis = q32DSP(id)->CNT_WMIS;
        //重新统计
        //q32DSP(id)->ESU_CON |= BIT(31);

        log_debug("cpu%d c_hist = 0x%x", id, cnt_chit);
        log_debug("cpu%d i_hist = 0x%x", id, cnt_ihit);
        log_debug("cpu%d r_hist = 0x%x", id, cnt_rhit);
        log_debug("cpu%d w_hist = 0x%x", id, cnt_whit);

        log_debug("cpu%d c_mis  = 0x%x", id, cnt_cmis);
        log_debug("cpu%d i_mis  = 0x%x", id, cnt_imis);
        log_debug("cpu%d r_mis  = 0x%x", id, cnt_rmis);
        log_debug("cpu%d w_mis  = 0x%x", id, cnt_wmis);
        log_debug("cpu%d cnt_fill = 0x%x", id, cnt_fill);

        if (!cnt_chit || !cnt_ihit) {
            q32DSP(id)->ESU_CON |= BIT(31);
            continue;
        }

        u64 all;
        all	= cnt_chit + cnt_cmis;
        //内核流水线工作成功的占比
        log_info("cpu%d c_hist = %d%%", id, (u32)((u64)cnt_chit * 100 / all));
        //内核流水线工作等待的占比
        log_info("cpu%d c_mis  = %d%%", id, (u32)((u64)cnt_cmis * 100 / all));

        all = cnt_ihit + cnt_imis;
        //内核取指令总线真正访问总线的占比
        log_info("cpu%d i_hist = %d%%", id, (u32)((u64)cnt_ihit * 100 / all));
        //内核取指令总线等待访问总线的占比
        log_info("cpu%d i_mis  = %d%%", id, (u32)((u64)cnt_imis * 100 / all));

        all = cnt_rhit + cnt_rmis;
        //内核读数据总线真正访问总线的占比
        log_info("cpu%d r_hist = %d%%", id, (u32)((u64)cnt_rhit * 100 / all));
        //内核读数据总线等待访问总线的占比
        log_info("cpu%d r_mis  = %d%%", id, (u32)((u64)cnt_rmis * 100 / all));

        all = cnt_whit + cnt_wmis;
        //内核写数据总线真正访问总线的占比
        log_info("cpu%d w_hist = %d%%", id, (u32)((u64)cnt_whit * 100 / all));
        //内核写数据总线等待访问内存的占比
        log_info("cpu%d w_mis  = %d%%", id, (u32)((u64)cnt_wmis * 100 / all));

        log_info("cpu%d c_fill = %d%%", id, (u32)((u64)cnt_fill * 100 / cnt_cmis));
    }
}

void cpu_efficiency_calculate_init(void)
{
    q32DSP(current_cpu_id())->ESU_CON = 0xffffffff;
}

void cpu_efficiency_calculate_uninit(void)
{
    q32DSP(current_cpu_id())->ESU_CON = BIT(31);
}
#endif

void efficiency_calculate_show(void)
{
#ifdef CONFIG_CPU_EFFICIENCY_CALCULATE_ENABLE
    cpu_efficiency_calculate_show();
#endif
#ifdef CONFIG_ICACHE_EFFICIENCY_CALCULATE_ENABLE
    IcuReportPrintf();
#endif
#ifdef CONFIG_DCACHE_EFFICIENCY_CALCULATE_ENABLE
    DcuReportPrintf();
#endif
}


extern u32 cpu0_sstack_begin[];
extern u32 cpu0_sstack_end[];
extern u32 cpu1_sstack_begin[];
extern u32 cpu1_sstack_end[];

__attribute__((noinline))
void sp_ovf_unen(void)
{
    int cpu_id = current_cpu_id();

    q32DSP(cpu_id)->EMU_CON &= ~BIT(3);
#if defined CONFIG_TRUSTZONE_ENABLE
    q32DSP(cpu_id)->EMU_SSP_H = 0xffffffff;
    q32DSP(cpu_id)->EMU_SSP_L = 0;
#endif
    __asm_csync();	//不能屏蔽，否则会有流水线问题
}

__attribute__((noinline))
void sp_ovf_en(void)
{
    int cpu_id = current_cpu_id();
#if defined CONFIG_TRUSTZONE_ENABLE
    q32DSP(cpu_id)->EMU_SSP_L = cpu_id ? (u32)cpu1_sstack_begin : (u32)cpu0_sstack_begin;
    q32DSP(cpu_id)->EMU_SSP_H = cpu_id ? (u32)cpu1_sstack_end : (u32)cpu0_sstack_end;
#endif
#ifndef SDTAP_DEBUG
    q32DSP(cpu_id)->EMU_CON |= BIT(3); //如果用户使用setjmp longjmp, 或者使用sdtap gdb调试 务必要删掉这句话
    __asm_csync();
#endif
}


#if defined CONFIG_TRUSTZONE_ENABLE

extern u32 _TRUSTZONE_CODE_START;
extern u32 _TRUSTZONE_CODE_END;

//安全模式切换时如果有外设同时访问dcache，可能会出现问题？
AT(.trustzone)
void trustzone_enter(void)
{
    OS_ENTER_CRITICAL();

    IcuFlushinvAll();
    DcuFlushinvAll();

    NSMODE(0);

    __asm_csync();

    OS_EXIT_CRITICAL();
}

AT(.trustzone)
void trustzone_exit(void)
{
    OS_ENTER_CRITICAL();

    IcuFlushinvAll();
    DcuFlushinvAll();

    NSMODE(1);

    __asm_csync();

    OS_EXIT_CRITICAL();
}

void trustzone_init(void)
{
    int cpu_id = current_cpu_id();

    q32DSP(cpu_id)->NSC_ADRL = (u32)&_TRUSTZONE_CODE_START;
    q32DSP(cpu_id)->NSC_ADRH = (u32)&_TRUSTZONE_CODE_END;
    q32DSP(cpu_id)->NSC_CON0 = 1;
}

void trustzone_init_all_cpu(void)
{
    q32DSP(0)->NSC_ADRL = (u32)&_TRUSTZONE_CODE_START;
    q32DSP(0)->NSC_ADRH = (u32)&_TRUSTZONE_CODE_END;
    q32DSP(0)->NSC_CON0 = 1;

    q32DSP(1)->NSC_ADRL = (u32)&_TRUSTZONE_CODE_START;
    q32DSP(1)->NSC_ADRH = (u32)&_TRUSTZONE_CODE_END;
    q32DSP(1)->NSC_CON0 = 1;
}

#else

void trustzone_enter(void)
{

}

void trustzone_exit(void)
{

}

#endif


void debug_msg_clear(void)
{
#ifdef CONFIG_NET_ENABLE
    if (!(JL_WEMU->WREN & BIT(0))) {
        JL_WEMU->WREN = 0xe7;
    }
    JL_WEMU->MSG0 = 0xffffffff;
    JL_WEMU->MSG1 = 0xffffffff;
    JL_WEMU->MSG2 = 0xffffffff;
    JL_WEMU->MSG3 = 0xffffffff;
    if (JL_WEMU->WREN & BIT(0)) {
        JL_WEMU->WREN = 0xe7;
    }
#endif

#ifdef CONFIG_VIDEO_ENABLE
    if (!(JL_VEMU->WREN & BIT(0))) {
        JL_VEMU->WREN = 0xe7;
    }
    JL_VEMU->MSG0 = 0xffffffff;
    JL_VEMU->MSG1 = 0xffffffff;
    JL_VEMU->MSG2 = 0xffffffff;
    JL_VEMU->MSG3 = 0xffffffff;
    if (JL_VEMU->WREN & BIT(0)) {
        JL_VEMU->WREN = 0xe7;
    }
#endif

#ifdef CONFIG_AUDIO_ENABLE
    if (!(JL_AEMU->WREN & BIT(0))) {
        JL_AEMU->WREN = 0xe7;
    }
    JL_AEMU->MSG0 = 0xffffffff;
    JL_AEMU->MSG1 = 0xffffffff;
    JL_AEMU->MSG2 = 0xffffffff;
    JL_AEMU->MSG3 = 0xffffffff;
    if (JL_AEMU->WREN & BIT(0)) {
        JL_AEMU->WREN = 0xe7;
    }
#endif

    if (!(JL_LEMU->WREN & BIT(0))) {
        JL_LEMU->WREN = 0xe7;
    }
    JL_LEMU->MSG0 = 0xffffffff;
    JL_LEMU->MSG1 = 0xffffffff;
    JL_LEMU->MSG2 = 0xffffffff;
    JL_LEMU->MSG3 = 0xffffffff;
    if (JL_LEMU->WREN & BIT(0)) {
        JL_LEMU->WREN = 0xe7;
    }

    if (!(JL_HEMU->WREN & BIT(0))) {
        JL_HEMU->WREN = 0xe7;
    }
    JL_HEMU->MSG0 = 0xffffffff;
    JL_HEMU->MSG1 = 0xffffffff;
    JL_HEMU->MSG2 = 0xffffffff;
    JL_HEMU->MSG3 = 0xffffffff;
    if (JL_HEMU->WREN & BIT(0)) {
        JL_HEMU->WREN = 0xe7;
    }

    if (!(JL_CEMU->WREN & BIT(0))) {
        JL_CEMU->WREN = 0xe7;
    }
    JL_CEMU->MSG0 = 0xffffffff;
    JL_CEMU->MSG1 = 0xffffffff;
    JL_CEMU->MSG2 = 0xffffffff;
    JL_CEMU->MSG3 = 0xffffffff;
    if (JL_CEMU->WREN & BIT(0)) {
        JL_CEMU->WREN = 0xe7;
    }

    JL_L1P->EMU_MSG = 0xffffffff;
    q32DSP_icu(0)->EMU_MSG = 0xffffffff;
    q32DSP_icu(1)->EMU_MSG = 0xffffffff;
    q32DSP(0)->EMU_MSG = 0xffffffff;
    q32DSP(1)->EMU_MSG = 0xffffffff;
}

static int wwdg_isr_callback(void)
{
    return 0;
}

void debug_init(void)
{
    int cpu_id = current_cpu_id();

    debug_register_isr();

#ifdef SDTAP_DEBUG
    sdtap_init(2);
#endif

    q32DSP(cpu_id)->EMU_CON = 0;

    if (cpu_id == 0) {
        debug_msg_clear();

        //初始化窗口看门狗
        wwdg_init(0x76, 0x7f, wwdg_isr_callback);
        /* wwdg_enable(); */
    }

    pc_rang_limit(&rom_text_code_begin, &rom_text_code_end,
                  (void *)0xf0000, &ram_text_code_end,
                  /* &ram_text_code_begin, &ram_text_code_end, */
#ifndef CONFIG_NO_SDRAM_ENABLE
                  &sdram_text_code_begin, &sdram_text_code_end
#else
                  NULL, NULL
#endif
                 );

    q32DSP(cpu_id)->EMU_CON &= ~BIT(3);
    q32DSP(cpu_id)->EMU_SSP_L = cpu_id ? (u32)cpu1_sstack_begin : (u32)cpu0_sstack_begin;
    q32DSP(cpu_id)->EMU_SSP_H = cpu_id ? (u32)cpu1_sstack_end : (u32)cpu0_sstack_end;
    q32DSP(cpu_id)->EMU_USP_L = 0;
    q32DSP(cpu_id)->EMU_USP_H = 0xffffffff;
    q32DSP(cpu_id)->EMU_CON |= BIT(3);

    log_info("cpu %d usp limit %x %x", cpu_id, q32DSP(cpu_id)->EMU_USP_L, q32DSP(cpu_id)->EMU_USP_H);
    log_info("cpu %d ssp limit %x %x", cpu_id, q32DSP(cpu_id)->EMU_SSP_L, q32DSP(cpu_id)->EMU_SSP_H);

#ifdef CONFIG_FLOAT_DEBUG_ENABLE
    q32DSP(cpu_id)->EMU_CON |= (0x1f << 16);
#endif
    q32DSP(cpu_id)->EMU_CON |= BIT(4);
    q32DSP(cpu_id)->EMU_CON |= BIT(0) | BIT(1) | BIT(2) | BIT(5) | BIT(8);
    q32DSP(cpu_id)->EMU_CON |= BIT(29) | BIT(30) | BIT(31);
    q32DSP(cpu_id)->ETM_CON |= BIT(0);

    if (cpu_id == 0) {
        log_debug("JL_CEMU->CON0 : 0x%x", JL_CEMU->CON0);
        log_debug("JL_CEMU->CON1 : 0x%x", JL_CEMU->CON1);
        log_debug("JL_CEMU->CON2 : 0x%x", JL_CEMU->CON2);
        log_debug("JL_HEMU->CON0 : 0x%x", JL_HEMU->CON0);
        log_debug("JL_HEMU->CON1 : 0x%x", JL_HEMU->CON1);
        log_debug("JL_HEMU->CON2 : 0x%x", JL_HEMU->CON2);
        log_debug("JL_LEMU->CON0 : 0x%x", JL_LEMU->CON0);
        log_debug("JL_LEMU->CON1 : 0x%x", JL_LEMU->CON1);
        log_debug("JL_LEMU->CON2 : 0x%x", JL_LEMU->CON2);
        log_debug("JL_AEMU->CON0 : 0x%x", JL_AEMU->CON0);
        log_debug("JL_AEMU->CON1 : 0x%x", JL_AEMU->CON1);
        log_debug("JL_AEMU->CON2 : 0x%x", JL_AEMU->CON2);
        log_debug("JL_VEMU->CON0 : 0x%x", JL_VEMU->CON0);
        log_debug("JL_VEMU->CON1 : 0x%x", JL_VEMU->CON1);
        log_debug("JL_VEMU->CON2 : 0x%x", JL_VEMU->CON2);
        log_debug("JL_WEMU->CON0 : 0x%x", JL_WEMU->CON0);
        log_debug("JL_WEMU->CON1 : 0x%x", JL_WEMU->CON1);
        log_debug("JL_WEMU->CON2 : 0x%x", JL_WEMU->CON2);
    }

    log_debug("ICU_EMU_CON->CON0 : 0x%x", q32DSP_icu(cpu_id)->EMU_CON);
    log_debug("DCU_EMU_CON->CON0 : 0x%x", JL_L1P->EMU_CON);

#ifdef CONFIG_CPU_EFFICIENCY_CALCULATE_ENABLE
    cpu_efficiency_calculate_init();
#endif
#ifdef CONFIG_ICACHE_EFFICIENCY_CALCULATE_ENABLE
    IcuReportEnable();
#endif
#ifdef CONFIG_DCACHE_EFFICIENCY_CALCULATE_ENABLE
    DcuReportEnable();
#endif

#if defined CONFIG_TRUSTZONE_ENABLE
    //该步骤应该在跳转到SDK前实现
    trustzone_init();
    trustzone_exit();
#endif

    if (cpu_id == 0) {
#if defined CONFIG_MPU_DEBUG_ENABLE
        //保护ram code不被篡改，只允许CPU执行
#if defined CONFIG_TRUSTZONE_ENABLE
        mpu_enable_by_index(7, (u32)&ram_text_code_begin, (u32)&ram_text_code_end - 1, 0, "Crxn");
#else
        mpu_enable_by_index(7, (u32)&ram_text_code_begin, (u32)&ram_text_code_end - 1, 0, "Crx");
#endif
#if !defined CONFIG_NO_SDRAM_ENABLE
#if defined CONFIG_SFC_ENABLE
        //保护sdram code不被外设篡改，但没办法捕捉CPU读写，因为CPU经过cache没有覆盖MPU功能
#if defined CONFIG_TRUSTZONE_ENABLE
        mpu_enable_by_index(6, (u32)&sdram_text_code_begin, (u32)&sdram_text_code_end - 1, 0, "Crxn");
        tzasc_enable_by_index(3, NO_CACHE_ADDR((u32)&sdram_text_code_begin), NO_CACHE_ADDR((u32)&sdram_text_code_end - 1), "0rn1rn", AXI_MASTER_ICACHE, AXI_MASTER_DCACHE);
#else
        mpu_enable_by_index(6, (u32)&sdram_text_code_begin, (u32)&sdram_text_code_end - 1, 0, "Crx");
        tzasc_enable_by_index(3, NO_CACHE_ADDR((u32)&sdram_text_code_begin), NO_CACHE_ADDR((u32)&sdram_text_code_end - 1), "0r1r", AXI_MASTER_ICACHE, AXI_MASTER_DCACHE);
#endif
#endif
#endif
#endif
    }
}

