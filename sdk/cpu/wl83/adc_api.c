#include "generic/typedef.h"
#include "asm/clock.h"
#include "asm/adc_api.h"
#include "system/timer.h"
#include "system/spinlock.h"
#include "asm/efuse.h"
#include "asm/gpio.h"
#include "asm/power_interface.h"
#include "os/os_api.h"

#define LOG_TAG_CONST       GPADC
#define LOG_TAG             "[GPADC]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_ERROR_ENABLE
#include "system/debug.h"

extern const int config_gpadc_use_algo;

static volatile u16 g_adc_res;
static u8 cur_ch = 0;
static volatile u8 adc_scan_busy;
static u16 scan_timer;
static OS_MUTEX adc_mutex;

// adc trim 相关
#define CENTER_VBG_VOL      800  // VBG中心电压mv
#define CENTER_2V5_VAL      775  // 2.5V参考电压的中心值，需与烧写器一致
#define TRIM_LSB            2    // efuse lsb的精度，需与烧写器一致
#define ADC_TRIM_VIO_VOL    3300 // 烧写器trim 2.5V REF时芯片IOVDD档位
#define ADC_TRIM_REF_VOL    2500 // 烧写器2.5V REF 电压值
static u32 adc_trim_2V5_A;       // 烧写器读2.5V REF的adc值
static u8 adc_algo_sel;          // adc_trim算法选择

struct adc_info_t {
    u32 ch;
    u16 value;
};

#define VBG_VBAT_SCAN_CNT       100
#define VBAT_VALUE_ARRAY_SIZE   20

static struct adc_info_t adc_queue[ADC_MAX_CH];

static u16 vbg_adc_value;
static u16 vbat_value_array[VBAT_VALUE_ARRAY_SIZE];

//gpio->adc_ch表
static const u8 adc_ch_io_table[] = {
    IO_PORTD_00,
    IO_PORTD_01,
    IO_PORTD_06,
    IO_PORTD_07,
    IO_PORTD_08,
    IO_PORTD_09,
    IO_PORTD_10,
    IO_PORTE_04,
    IO_PORTE_05,
    IO_PORTE_06,
    IO_PORTE_07,
    IO_PORTE_08,
    IO_PORTE_09,
    IO_PORTC_13,
    IO_PORT_USB_DPA,
    IO_PORT_USB_DPB,
};

adc_io_ch_t adc_io2ch(int gpio)
{
    for (int i = 0; i < ARRAY_SIZE(adc_ch_io_table); ++i) {
        if (adc_ch_io_table[i] == gpio) {
            return i;
        }
    }

    log_error("add_adc_ch io error!!! change other io_port!!!");

    return 0xffffffff;
}

u32 adc_add_sample_ch(u32 ch)
{
    u32 i;

    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            break;
        } else if (adc_queue[i].ch == -1) {
            adc_queue[i].value = 1;
            adc_queue[i].ch = ch;
            log_info("add sample ch %x", ch);
            break;
        }
    }

    return i;
}

u32 adc_remove_sample_ch(u32 ch)
{
    u32 i;

    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            adc_queue[i].ch = -1;
            break;
        }
    }

    return i;
}

static u32 adc_get_next_ch(u32 cur_ch)
{
    for (int i = cur_ch + 1; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch != -1) {
            return i;
        }
    }

    return 0;
}

static u8 adc_adjust_div(void)
{
    u8 adc_clk_div = 0x7f; //adc_clk_div 占7bit
    const u32 lsb_clk = clk_get("lsb");

    for (int i = 1; i < 0x80; i++) {
        if (lsb_clk / i <= 1000000) {
            adc_clk_div = i;
            break;
        }
    }

    return adc_clk_div;
}

static void vbat_value_push(u16 vbat_value)
{
    static u32 pos = 0;
    vbat_value_array[pos] = vbat_value;
    if (++pos == VBAT_VALUE_ARRAY_SIZE) {
        pos = 0;
    }
}

static u16 vbat_value_avg(void)
{
    u32 i, sum = 0;
    for (i = 0; i < VBAT_VALUE_ARRAY_SIZE; i++) {
        sum += vbat_value_array[i];
    }
    return sum / VBAT_VALUE_ARRAY_SIZE;
}

u32 adc_get_value(u32 ch)
{
    if (ch == ADC_PMU_CH_VBAT) {
        return vbat_value_avg();
    }

    if (ch == ADC_PMU_CH_VBG) {
        return vbg_adc_value;
    }

    for (int i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            return adc_queue[i].value;
        }
    }
    return 0;
}

static u32 adc_get_ref_trim_value(void)
{
    u8 adc_trim_efuse = efuse_get_adc_trim();
    if (adc_trim_efuse & BIT(4)) {
        return (u32)(CENTER_2V5_VAL - TRIM_LSB * (adc_trim_efuse & 0xF));
    } else {
        return (u32)(CENTER_2V5_VAL + TRIM_LSB * (adc_trim_efuse & 0xF));
    }
}

// 算法1.认为VBG 800mV准确，参考VBG计算adc值。
u32 adc_value_to_voltage_algo1(u32 adc_vbg, u32 adc_ch_val)
{
    return adc_ch_val * CENTER_VBG_VOL / adc_vbg;
}

u32 adc_get_voltage_algo1(u32 ch)
{
    u32 adc_vbg = adc_get_value(ADC_PMU_CH_VBG);
    u32 adc_res = adc_get_value(ch);
    return adc_value_to_voltage_algo1(adc_vbg, adc_res);
}

// 算法2.烧写器阶段trim好烧写器上的LDO2.5V，参考2.5V的trim值计算adc值。
u32 adc_value_to_voltage_algo2(u32 adc_value_B)
{
    vddiom_vol_t vol_level = get_vddiom_vol_sel();
    u32 vddio_vol;
    if (vol_level == VDDIOM_VOL_240V) {
        vddio_vol = 2400;
    } else {
        vddio_vol = (u32)((vol_level - VDDIOM_VOL_280V) * 100 + 2800);
    }

    u32 voltage = ADC_TRIM_REF_VOL * adc_value_B / adc_trim_2V5_A;
    /* printf("[msg]>>>>no comp voltage= %d\n", voltage); */

    if (vddio_vol <= ADC_TRIM_VIO_VOL) {
        voltage = voltage - (ADC_TRIM_VIO_VOL - vddio_vol) * adc_value_B / 1024;
    } else {
        voltage = voltage + (vddio_vol - ADC_TRIM_VIO_VOL) * adc_value_B / 1024;
    }

    return voltage;
}

u32 adc_get_voltage_algo2(u32 ch)
{
    u32 adc_value = adc_get_value(ch);

    return adc_value_to_voltage_algo2(adc_value);
}

u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_ch_val)
{
    if (adc_algo_sel == 1) {
        return adc_value_to_voltage_algo1(adc_vbg, adc_ch_val);
    } else {
        return adc_value_to_voltage_algo2(adc_ch_val);
    }
}

u32 adc_get_voltage(u32 ch)
{
    if (adc_algo_sel == 1) {
        return adc_get_voltage_algo1(ch);
    } else {
        return adc_get_voltage_algo2(ch);
    }
}

static void adc_audio_ch_select(u32 ch)
{

}

___interrupt
static void adc_isr(void)
{
    if (!(JL_ADC->CON & BIT(30))) {
        return;    //防止刚进中断时pnd被另外一个核清掉
    }

    while (!(JL_ADC->CON & BIT(7))) {}     //WAIT IDLE

    u16 adc_res = JL_ADC->RES;

    if (adc_res == 0xffff) {
        log_error("adc res == 0xffff impossible!!!");
    }

    JL_ADC->CON = BIT(29);  //PND_CLR

    __asm_csync();

    g_adc_res = adc_res;
}

u32 adc_sample(u32 ch, u8 block)
{
    u16 tmp_adc_res = g_adc_res;

    if (!block) {
        g_adc_res = (u16) - 1;
    }

    u32 adc_con = 0;
    SFR(adc_con, 8, 7, adc_adjust_div() / 2);   //ADC_BAUD

    adc_con |= (0xf << 15); //启动延时控制，实际启动延时为此数值*8个ADC时钟
    adc_con |= BIT(0);      //ADC_AEN
    adc_con |= BIT(29);     //PND_CLR
    if (!block) {
        adc_con |= BIT(28); //PND_IE
    }

#ifndef CONFIG_FPGA_ENABLE
    SFR(JL_WL_RFD->WLA_CON0, 14, 1, 0); //TEST_TO_ADC_EN_12v
#endif

    if (ch < ADC_IO_CH_MAX) {
        adc_con |= BIT(2);  //IO_CHL_EN
        SFR(adc_con, 19, 4, ch);                //ADC_CHL
    } else {
        adc_con |= BIT(3);  //TEST_CHL_EN
        SFR(adc_con, 23, 3, ch >> 16);          //ADC_CHL

        if ((ch >> 16) == ADC_PMU_CH) {
            if (ch == ADC_PMU_CH_VBG) {
#ifndef CONFIG_FPGA_ENABLE
                SFR(JL_WL_RFD->WLA_CON0, 14, 1, 1);    //TEST_TO_ADC_EN_12v
#endif
            }
            adc_pmu_detect_en(ch & 0b111);
        } else if ((ch >> 16) == ADC_AUDIO_CH) {
            adc_audio_ch_select(ch & 0b111);
        }
    }

    if (tmp_adc_res == 0xffff || (JL_ADC->CON & ~BIT(7))) {
        log_error("impossible adc_con: 0x%x, g_adc_res: 0x%x", JL_ADC->CON, tmp_adc_res);
    }

    JL_ADC->CON = adc_con;
    JL_ADC->CON |= BIT(31); //ADC_DEN
    __asm_csync();
    JL_ADC->CON |= BIT(6);  //kistart

    if (block) {
        while (!(JL_ADC->CON & BIT(7)));    //WAIT IDLE
        while (!(JL_ADC->CON & BIT(30)));   //WAIT PND
        tmp_adc_res = JL_ADC->RES;
        JL_ADC->CON = BIT(29);              //PND_CLR
    }

    return tmp_adc_res;
}

static u16 vbg_vbat_cnt = VBG_VBAT_SCAN_CNT;
static u8 vbg_vbat_step = 0;
static u16 old_adc_res;
static u16 tmp_vbg_adc_value;
static spinlock_t adc_lock;

static void adc_scan(void *priv)
{
    if ((JL_ADC->CON & BIT(31)) || g_adc_res == 0xffff) {
        return;
    }

#if CPU_CORE_NUM > 1
    spin_lock(&adc_lock);
    if (adc_scan_busy) {
        spin_unlock(&adc_lock);
        return;
    }
    adc_scan_busy = 1;
    spin_unlock(&adc_lock);
#endif

    if (++vbg_vbat_cnt > VBG_VBAT_SCAN_CNT) {
        if (vbg_vbat_step == 0) {
            vbg_vbat_step = 1;
            old_adc_res = g_adc_res;
            adc_sample(ADC_PMU_CH_VBG, 0);
            adc_scan_busy = 0;
            return;
        } else if (vbg_vbat_step == 1) {
            vbg_vbat_step = 2;
            tmp_vbg_adc_value = adc_sample(ADC_PMU_CH_VBAT, 0);
            adc_scan_busy = 0;
            //printf("vbg = %d\n", tmp_vbg_adc_value);
            return;
        } else if (vbg_vbat_step == 2) {
            vbg_vbat_step = 0;
            vbat_value_push(g_adc_res);
            vbg_adc_value = tmp_vbg_adc_value;
            //printf("vbg = %d  vbat = %d\n", vbg_adc_value, vbat_adc_value);
            g_adc_res = old_adc_res;
            vbg_vbat_cnt = 0;
        }
    }

    u8 next_ch = adc_get_next_ch(cur_ch);

    adc_queue[cur_ch].value = adc_sample(adc_queue[next_ch].ch, 0);

    cur_ch = next_ch;

    adc_scan_busy = 0;
}

//获取当前采集ad的通道总数
u8 get_cur_total_ad_ch(void)
{
    u8 total_ch = 0;
    u8 i = 0;

    while (i < ADC_MAX_CH) {
        if (adc_queue[i].ch != -1) {
            total_ch++;
        }
        log_debug("i:%d, ch:%x", i, adc_queue[i].ch);
        i++;
    }

    log_debug("total_ch:%d", total_ch);

    return total_ch;
}

void adc_init(void)
{
#ifndef CONFIG_FPGA_ENABLE
    SFR(JL_WL_RFD->WLA_CON0, 0, 1, 1);    //RWF_BIAS_EN_12v
    SFR(JL_WL_RFD->WLA_CON0, 1, 4, 0x2);  //RWF_BIAS_IGEN_SEL[3:0]_12v
    SFR(JL_WL_RFD->WLA_CON0, 5, 1, 0);    //RWF_BIAS_TEST_OE_12v
    SFR(JL_WL_RFD->WLA_CON0, 6, 2, 0);    //RWF_BIAS_TEST_SEL[1:0]_12v
    SFR(JL_WL_RFD->WLA_CON0, 15, 3, 0b100); //TEST_TO_ADC_S[2:0]_12v
#endif

    os_mutex_create(&adc_mutex);

    memset(adc_queue, 0xff, sizeof(adc_queue));

    JL_ADC->CON = BIT(29);     //PND_CLR

    for (u8 i = 0; i < VBAT_VALUE_ARRAY_SIZE; i++) {
        vbat_value_array[i] = adc_sample(ADC_PMU_CH_VBAT, 1);
    }
    log_info("vbat_adc_value = %d", vbat_value_avg());

    vbg_adc_value = 0;
    for (u8 i = 0; i < 10; i++) {
        vbg_adc_value += adc_sample(ADC_PMU_CH_VBG, 1);
    }
    vbg_adc_value /= 10;
    log_info("vbg_adc_value = %d", vbg_adc_value);

    u8 adc_trim_efuse = efuse_get_adc_trim();
    if (config_gpadc_use_algo == 0) {
        adc_algo_sel = (adc_trim_efuse == 0x1f) ? 1 : 2; // adc未trim则使用算法1
    } else if (config_gpadc_use_algo == 1) {
        adc_algo_sel = 1;
    } else {
        adc_algo_sel = 2;
    }
    log_info("adc_trim_efuse = 0x%x", adc_trim_efuse);

    if (adc_algo_sel == 1) {
        if (config_gpadc_use_algo == 1) {
            log_info("adc using algorithm 1");
        } else {
            log_warn("ADC has not been trimmed. " \
                     "Program will use adc Algorithm 1.\n" \
                     "It is recommended to use a programmer to " \
                     "trim the ADC to reduce sampling errors!");
        }
    } else {
        adc_trim_2V5_A = adc_get_ref_trim_value();
        log_info("adc_trim_2V5 = %d", adc_trim_2V5_A);
        log_info("adc using algorithm 2");
    }

    g_adc_res = 1;

    request_irq(IRQ_SARADC_IDX, 0, adc_isr, 0);

    scan_timer = sys_s_hi_timer_add(NULL, adc_scan, 2);
}

void adc_suspend(void)
{
    if (!os_mutex_valid(&adc_mutex)) {
        return;
    }

    os_mutex_pend(&adc_mutex, 0);

    if (scan_timer) {
        sys_hi_timer_del(scan_timer);
        scan_timer = 0;
    }
    //解决双核互斥问题，等当前采样轮次结束了再关闭ADC
#if CPU_CORE_NUM > 1
__check_busy:
    spin_lock(&adc_lock);
    if (!adc_scan_busy) {
        adc_scan_busy = 1;
        spin_unlock(&adc_lock);
    } else {
        spin_unlock(&adc_lock);
        while (adc_scan_busy) {}
        goto __check_busy;
    }
#endif
    while (!(JL_ADC->CON & BIT(7))) {}  //WAIT IDLE
    while (g_adc_res == 0xffff) {}      //等待上一次的采集结果完成
    JL_ADC->CON = 0;
}

void adc_resume(void)
{
    if (!os_mutex_valid(&adc_mutex)) {
        return;
    }

    if (!scan_timer) {
        adc_scan_busy = 0;
        scan_timer = sys_s_hi_timer_add(NULL, adc_scan, 2);
    }

    os_mutex_post(&adc_mutex);
}

u32 adc_get_value_blocking(u32 ch)
{
    u16 cur_res;

    adc_suspend();

    cur_res = adc_sample(ch, 1);

    adc_resume();

    return cur_res;
}

u32 adc_get_voltage_blocking(u32 ch)
{
    u32 vbg_value = 0;
    u32 vbg_min_value = -1;
    u32 vbg_max_value = 0;

    u32 ch_adc_value = 0;
    u32 ch_min_value = -1;
    u32 ch_max_value = 0;

    u32 adc_value;

    for (int i = 0; i < 12; i++) {
        adc_value = adc_get_value_blocking(ADC_PMU_CH_VBG);
        if (adc_value > vbg_max_value) {
            vbg_max_value = adc_value;
        }
        if (adc_value < vbg_min_value) {
            vbg_min_value = adc_value;
        }

        vbg_value += adc_value;

        if (ch == ADC_PMU_CH_VBG) {
            continue;
        }

        adc_value = adc_get_value_blocking(ch);

        if (adc_value > ch_max_value) {
            ch_max_value = adc_value;
        }
        if (adc_value < ch_min_value) {
            ch_min_value = adc_value;
        }
        ch_adc_value += adc_value;
    }

    vbg_value -= vbg_max_value;
    vbg_value -= vbg_min_value;

    vbg_value /= 10;

    if (ch == ADC_PMU_CH_VBG) {
        ch_adc_value = 1023;
    } else {
        ch_adc_value -= ch_max_value;
        ch_adc_value -= ch_min_value;
        ch_adc_value /= 10;
    }

    log_debug("%s() %d ch: %d min: %d max: %d", __func__, __LINE__, ch_adc_value, ch_min_value, ch_max_value);

    return adc_value_to_voltage(vbg_value, ch_adc_value);
}

static u8 request_gpadc_enter_low_power(u32 timeout)
{
    if (!(JL_ADC->CON & BIT(7))) {
        while (!(JL_ADC->CON & BIT(30))) {} //WAIT PND
        while (!(JL_ADC->CON & BIT(7))) {}  //WAIT IDLE
    }

    return 0;
}

static u8 request_gpadc_exit_low_power(u32 timeout)
{
    //p33_fast_access(P3_PMU_ADC0, BIT(5), 1);
    //p33_fast_access(P3_PMU_ADC0, BIT(0), 1);

    return 0;
}

REGISTER_LP_REQUEST(gpadc_lp_request) = {
    .name           = "gpadc",
    .request_enter  = request_gpadc_enter_low_power,
    .request_exit   = request_gpadc_exit_low_power,
};
