#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_setup.data.bss")
#pragma data_seg(".audio_setup.data")
#pragma const_seg(".audio_setup.text.const")
#pragma code_seg(".audio_setup.text")
#endif
/*
 ******************************************************************************************
 *							Audio Setup
 *
 * Discription: 音频模块初始化，配置，调试等
 *
 * Notes:
 ******************************************************************************************
 */
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_adc.h"
#include "asm/audio_common.h"
#include "asm/clock.h"
#include "asm/efuse.h"
#include "adc_file.h"
#include "linein_file.h"
#include "media/audio_general.h"
#include "audio_dac.h"
#include "syscfg/syscfg_id.h"
#include "system/init.h"
#include "update.h"
#include "asm/gpio.h"
#include "asm/power_interface.h"
#include "media/audio_adc.h"

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_dvol.h"
#endif

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif

struct audio_dac_hdl dac_hdl;
struct audio_adc_hdl adc_hdl;

typedef struct {
    atomic_t ref;
} audio_setup_t;

static audio_setup_t audio_setup;
#define __this      (&audio_setup)

___interrupt
static void audio_dac_irq_hdl(void)
{
    if (JL_AUDDAC->DAC_CON0 & BIT(7)) {
        JL_AUDDAC->DAC_CON0 |= BIT(6);
        if (JL_AUDDAC->DAC_CON0 & BIT(5)) {
            audio_dac_irq_handler(&dac_hdl);
        }
    }
    asm("csync");
}

___interrupt
static void audio_adc_irq_hdl(void)
{
    if (JL_AUDADC->ADC_CON0 & BIT(7)) {
        JL_AUDADC->ADC_CON0 |= BIT(6);
        if ((JL_AUDADC->ADC_CON0 & BIT(5)) && (JL_AUDADC->ADC_CON0 & BIT(4))) {
            audio_adc_irq_handler(&adc_hdl);
        }
    }
    asm("csync");
}

struct dac_platform_data dac_data = {
    .output             = TCFG_AUDIO_DAC_CONNECT_MODE,                //DAC输出配置，和具体硬件连接有关，需根据硬件来设置
    .output_mode        = TCFG_AUDIO_DAC_MODE,
    .performance_mode   = TCFG_DAC_PERFORMANCE_MODE,
    .light_close        = TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE,
    .dma_buf_time_ms    = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
    .dcc_level          = 14,
    .pa_mute_en         = TCFG_AUDIO_DAC_PA_MUTE_EN,
    .pa_mute_port       = TCFG_AUDIO_DAC_PA_MUTE_PORT,
    .pa_mute_value      = TCFG_AUDIO_DAC_PA_MUTE_LEVEL,
    .pa_mute_delay_ms   = TCFG_AUDIO_DAC_PA_MUTE_DELAY_MS,
};

#if TCFG_AUDIO_ADC_ENABLE
const struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM] = {
#if TCFG_ADC0_ENABLE
    [0] = {
        .mic_mode             = TCFG_ADC0_MODE,
        .mic_ain_sel          = TCFG_ADC0_AIN_SEL,
        .mic_bias_sel         = TCFG_ADC0_BIAS_SEL,
        .mic_bias_rsel        = TCFG_ADC0_BIAS_RSEL,
        .power_io             = TCFG_ADC0_POWER_IO,
        .mic_dcc              = TCFG_ADC0_DCC_LEVEL,
        .inside_bias_resistor = TCFG_ADC0_INSIDE_BIAS_RESISTOR_ENABLE,
        .dmic_enable          = TCFG_DMIC_ENABLE,
        .dmic_sclk_fre        = TCFG_DMIC_SCLK_FREQUENCY,
        .dmic_io_sclk         = TCFG_DMIC_IO_SCLK,
        .dmic_io_idat0        = TCFG_DMIC_IO_IDAT0,
        .dmic_io_idat1        = TCFG_DMIC_IO_IDAT1,
        .dmic_ch0_mode        = TCFG_DMIC_CH0_MODE,
        .dmic_ch1_mode        = TCFG_DMIC_CH1_MODE,
    },
#endif
#if TCFG_ADC1_ENABLE
    [1] = {
        .mic_mode             = TCFG_ADC1_MODE,
        .mic_ain_sel          = TCFG_ADC1_AIN_SEL,
        .mic_bias_sel         = TCFG_ADC1_BIAS_SEL,
        .mic_bias_rsel        = TCFG_ADC1_BIAS_RSEL,
        .power_io             = TCFG_ADC1_POWER_IO,
        .mic_dcc              = TCFG_ADC1_DCC_LEVEL,
        .inside_bias_resistor = TCFG_ADC1_INSIDE_BIAS_RESISTOR_ENABLE,
        .dmic_enable          = TCFG_DMIC_ENABLE,
        .dmic_sclk_fre        = TCFG_DMIC_SCLK_FREQUENCY,
        .dmic_io_sclk         = TCFG_DMIC_IO_SCLK,
        .dmic_io_idat0        = TCFG_DMIC_IO_IDAT0,
        .dmic_io_idat1        = TCFG_DMIC_IO_IDAT1,
        .dmic_ch0_mode        = TCFG_DMIC_CH0_MODE,
        .dmic_ch1_mode        = TCFG_DMIC_CH1_MODE,
    },
#endif
};
#endif

void audio_dac_power_state(u8 state)
{
    if (state == DAC_ANALOG_OPEN_PREPARE) {

    }
}

int audio_setup_dac_get_sample_rate(void)
{
    return dac_hdl.sample_rate;
}

int get_dac_channel_num(void)
{
    return dac_hdl.channel;
}

static void audio_dac_initcall(void)
{
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif

    audio_common_param_t common_param = {0};

    common_param.clock_mode = AUDIO_COMMON_CLK_DIF_XOSC;

    common_param.vcm_cap_en = TCFG_AUDIO_VCM_CAP_EN;

    common_param.vbg_trim_value = efuse_get_audio_vbg_trim();
    if (common_param.vbg_trim_value > 7) {
        common_param.vbg_trim_value = 3;
    }

    if (common_param.vcm_cap_en) {
        common_param.power_level = TCFG_DAC_POWER_LEVEL;
    } else {
        common_param.power_level = TCFG_DAC_POWER_LEVEL + 5;
        if (common_param.power_level == 9) {
            printf("!!!!if vcm without cap cannot use power_level 5!!!!");
            common_param.power_level = 8;
        }
    }

    audio_common_init(&common_param);

    audio_common_clock_open(); //从audio_dac_init中取出打开时钟,关闭dac时adc可正常使用

#if TCFG_DAC_NODE_ENABLE
    dac_data.power_level = common_param.power_level;
    dac_data.max_sample_rate = AUDIO_DAC_MAX_SAMPLE_RATE;
    dac_data.bit_width = audio_general_out_dev_bit_width();
    audio_dac_init(&dac_hdl, &dac_data);
#endif
    //dac_hdl.ng.threshold = 4;         //DAC底噪优化阈值
    //dac_hdl.ng.detect_interval = 200; //DAC底噪优化检测间隔ms

    audio_common_audio_init((void *)(adc_hdl.mic_param), (void *)(dac_hdl.pd));

    //开机就打开vcm进行上电
    audio_common_power_open();

#if defined(TCFG_AUDIO_DAC_24BIT_MODE) && TCFG_AUDIO_DAC_24BIT_MODE
    audio_dac_set_bit_mode(&dac_hdl, 1);
#endif

    audio_dac_set_analog_vol(&dac_hdl, 0);

    request_irq(IRQ_AUDIO_DAC_IDX, 4, audio_dac_irq_hdl, 0);
    request_irq(IRQ_AUDIO_ADC_IDX, 4, audio_adc_irq_hdl, 0);

    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    /* audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt)); */
}

static const struct audio_adc_private_param adc_private_param = {
    .micldo_vsel   =  3,
};

static void audio_input_initcall(void)
{
    audio_adc_init(&adc_hdl, (struct audio_adc_private_param *)&adc_private_param);
    adc_hdl.bit_width = DATA_BIT_WIDE_16BIT;//WL83 ADC固定使用16bit

#if TCFG_AUDIO_LINEIN_ENABLE
    audio_linein_file_init();
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/

    audio_adc_file_init();

    audio_all_adc_file_init();

#if TCFG_APP_FM_EN
    audio_fm_file_init();
#endif
}

void audio_fast_mode_test()
{
#if 0
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    audio_dac_start(&dac_hdl);
    audio_adc_mic_demo_open(AUDIO_ADC_MIC_CH, 10, 16000, 1);
#endif
}

#ifdef BT_DUT_DAC_INTERFERE
static void write_dac(void *p)
{
    printf("audio_dac_demo_open\n");
    audio_dac_demo_open();
}
#endif

void audio_interfere_bt_dut(void)
{
#ifdef BT_DUT_ADC_INTERFERE
    printf("audio_adc_mic_demo_open\n");
    audio_adc_mic_demo_open(0b0001, 3, 44100, 0);
#endif

#ifdef BT_DUT_DAC_INTERFERE
    os_task_create(write_dac, NULL, 7, 128, 128, "dac_test_task");
#endif
}

static void audio_config_trace(void *priv)
{
    printf(">>Audio_Config_Trace:\n");
    audio_config_dump();
    //audio_adda_dump();
}

int audio_local_time_init(int timer_id);

/*audio模块初始化*/
static int audio_init(void)
{
    puts("audio_init\n");

    audio_clock_early_init();
    audio_general_init();
    audio_input_initcall();
    audio_dac_initcall();
    audio_local_time_init(2);
#if TCFG_AUDIO_DUT_ENABLE
    audio_dut_init();
#endif /*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_AUDIO_CONFIG_TRACE
    sys_timer_add(NULL, audio_config_trace, 3000);
#endif/*TCFG_AUDIO_CONFIG_TRACE*/

    audio_interfere_bt_dut();

    puts("audio_init succ\n");

    return 0;
}
__initcall(audio_init);

#if TCFG_SMART_VOICE_ENABLE
static int audio_smart_voice_init(void)
{
    return audio_smart_voice_detect_init(NULL);
}
late_initcall(audio_smart_voice_init);
#endif

/*关闭audio相关模块使能*/
u8 audio_disable_all(void)
{
    //DAC:DACEN
    JL_AUDDAC->DAC_CON0 &= ~BIT(4);
    //ADC:ADCEN
    JL_AUDADC->ADC_CON0 &= ~BIT(4);
    //EQ:
    JL_EQ->CON0 &= ~BIT(1);
    //FFT:
    JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //ANC:anc_en anc_start
    //JL_ANC->CON0 &= ~(BIT(1) | BIT(29));
    dac_power_off();

    return 0;
}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};

static u8 audio_iis_idle_query(void)
{
    return (JL_ALNK->CON0 & BIT(11)) == 0;
}

REGISTER_LP_TARGET(audio_iis_lp_target) = {
    .name       = "alnk",
    .is_idle    = audio_iis_idle_query,
};

void dac_power_on(void)
{
    /* printf(">>>dac_power_on:%d", __this->ref.counter); */
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }
}

void dac_power_off(void)
{
    /*log_info(">>>dac_power_off:%d", __this->ref.counter);*/
    if (atomic_read(&__this->ref) != 0 && atomic_dec_return(&__this->ref)) {
        return;
    }
#if 0
    app_audio_mute(AUDIO_MUTE_DEFAULT);
    if (dac_hdl.vol_l || dac_hdl.vol_r) {
        u8 fade_time = dac_hdl.vol_l * 2 / 10 + 1;
        os_time_dly(fade_time); //br30进入低功耗无法使用os_time_dly
        printf("fade_time:%d ms", fade_time);
    }
#endif
    audio_dac_close(&dac_hdl);
}

//mic type
typedef struct __MIC_TYPE_CONFIG {
    u8 type;     //0:不省电容模式        1:省电容模式
    //1:16K 2:7.5K 3:5.1K 4:6.8K 5:4.7K 6:3.5K 7:2.9K  8:3K  9:2.5K 10:2.1K 11:1.9K  12:2K  13:1.8K 14:1.6K  15:1.5K 16:1K 31:0.6K
    u8 pull_up;
    //00:2.3v 01:2.5v 10:2.7v 11:3.0v
    u8 ldo_lev;
} _GNU_PACKED_ MIC_TYPE_CONFIG;

int read_mic_type_config(void)
{
    MIC_TYPE_CONFIG mic_type;
    int ret = syscfg_read(CFG_MIC_TYPE_ID, &mic_type, sizeof(MIC_TYPE_CONFIG));
    if (ret > 0) {
        return 0;
    }

    return -1;
}

/*音频模块寄存器跟踪*/
void audio_adda_dump(void) //打印所有的dac,adc寄存器
{
    printf("AUD_CON0:%x", JL_AUD->AUD_CON0);
    printf("AUD_CON1:%x", JL_AUD->AUD_CON1);
    printf("AUD_CON2:%x", JL_AUD->AUD_CON2);
    printf("DAC_CON0:%x", JL_AUDDAC->DAC_CON0);
    printf("DAC_CON1:%x", JL_AUDDAC->DAC_CON1);
    printf("ADC_CON0:%x", JL_AUDADC->ADC_CON0);
    printf("ADC_CON1:%x", JL_AUDADC->ADC_CON1);
    printf("DAC_VOL:%x", JL_AUDDAC->DAC_VOL);
    printf("ADDA_CON0:%x", JL_ADDA->ADDA_CON0);
    printf("ADDA_CON1:%x", JL_ADDA->ADDA_CON1);
    printf("DAA_CON 0:%x\n", JL_ADDA->DAA_CON0);
    printf("ADA_CON 0:%x	1:%x	2:%x	3:%x	4:%x	5:%x\n", JL_ADDA->ADA_CON0, JL_ADDA->ADA_CON1, JL_ADDA->ADA_CON2, JL_ADDA->ADA_CON3, JL_ADDA->ADA_CON4, JL_ADDA->ADA_CON5);
}

/*音频模块配置跟踪*/
void audio_config_dump(void)
{
    u8 dac_bit_width = ((JL_AUDDAC->DAC_CON0 & BIT(20)) ? 24 : 16);
    u8 adc_bit_width = ((JL_AUDADC->ADC_CON0 & BIT(18)) ? 24 : 16);
    int dac_dgain_max = 8192;
    int mic_gain_max = 14;
    u8 dac_dcc = (JL_AUDDAC->DAC_CON0 >> 12) & 0xF;
    u8 mic0_dcc = (JL_AUDADC->ADC_CON1 >> 16) & 0xF;
    u8 mic1_dcc = (JL_AUDADC->ADC_CON1 >> 20) & 0xF;
    u32 dac_dgain_fl = JL_AUDDAC->DAC_VOL & 0xFFFF;
    u32 dac_dgain_fr = (JL_AUDDAC->DAC_VOL >> 16) & 0xFFFF;
    u8 mic0_0_6 = (JL_ADDA->ADA_CON1 >> 11) & 0x1;
    u8 mic1_0_6 = (JL_ADDA->ADA_CON2 >> 11) & 0x1;
    u8 mic0_gain = (JL_ADDA->ADA_CON0 >> 12) & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON1 >> 12) & 0x1F;

    printf("[ADC]BitWidth:%d,DCC:%d,%d\n", adc_bit_width, mic0_dcc, mic1_dcc);
    printf("[ADC]Gain(Max:%d):%d,%d,6dB_Boost:%d,%d\n", mic_gain_max, mic0_gain, mic1_gain, \
           mic0_0_6, mic1_0_6);

    printf("[DAC]BitWidth:%d,DCC:%d\n", dac_bit_width, dac_dcc);
    printf("[DAC],DGain(Max:%d):%d,%d\n", dac_dgain_max, dac_dgain_fl, dac_dgain_fr);
}

