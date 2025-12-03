#ifndef CPU_AUDIO_ADC_H
#define CPU_AUDIO_ADC_H


#define LADC_STATE_INIT			1
#define LADC_STATE_OPEN      	2
#define LADC_STATE_START     	3
#define LADC_STATE_STOP      	4

#define FPGA_BOARD          	0

#define LADC_MIC                0
#define LADC_LINEIN             1

/************************************************
  				Audio ADC Features
Notes:以下为芯片规格定义，不可修改，仅供引用
************************************************/
/* ADC 最大通道数 */
#define AUDIO_ADC_MAX_NUM           (4)
#define AUDIO_ADC_MIC_MAX_NUM       (4)
#define AUDIO_ADC_LINEIN_MAX_NUM    (4)

/* 通道选择 */
#define AUDIO_ADC_MIC(x)					BIT(x)
#define AUDIO_ADC_MIC_0					    BIT(0)
#define AUDIO_ADC_MIC_1					    BIT(1)
#define AUDIO_ADC_MIC_2					    BIT(2)
#define AUDIO_ADC_MIC_3					    BIT(3)
#define AUDIO_ADC_LINE(x) 					BIT(x)
#define AUDIO_ADC_LINE0 					BIT(0)
#define AUDIO_ADC_LINE1  					BIT(1)
#define AUDIO_ADC_LINE2  					BIT(2)
#define AUDIO_ADC_LINE3  					BIT(3)
#define PLNK_MIC		            		BIT(6)
#define ALNK_MIC				            BIT(7)


/*******************************应用层**********************************/
/* 通道选择 */
#define AUDIO_ADC_MIC_CH		            AUDIO_ADC_MIC_0

/********************************************************************************
                MICx  输入IO配置(要注意IO与mic bias 供电IO配置互斥)
 ********************************************************************************/
// TCFG_AUDIO_MIC0_AIN_SEL
#define AUDIO_MIC0_CH0                ADC_AIN_PORT0   // PB9
#define AUDIO_MIC0_CH1                ADC_AIN_PORT1   // PB7
#define AUDIO_MIC0_CH2                ADC_AIN_PORT2   // PA0  (复用 MIC BIAS)
// 当mic0模式选择差分模式时，输入N端引脚固定为  PB5

// TCFG_AUDIO_MIC1_AIN_SEL
#define AUDIO_MIC1_CH0                ADC_AIN_PORT0    // PB10
#define AUDIO_MIC1_CH1                ADC_AIN_PORT1    // PB8
#define AUDIO_MIC1_CH2                ADC_AIN_PORT2    // PA1  (复用 MIC BIAS)
// 当mic1模式选择差分模式时，输入N端引脚固定为  PB6

// TCFG_AUDIO_MIC2_AIN_SEL
#define AUDIO_MIC2_CH0                ADC_AIN_PORT0    // PC9
#define AUDIO_MIC2_CH1                ADC_AIN_PORT1    // PC10 (复用 MIC BIAS)
// 当mic2模式选择差分模式时，输入N端引脚固定为  PC11

// TCFG_AUDIO_MIC3_AIN_SEL
#define AUDIO_MIC3_CH0                ADC_AIN_PORT0    // PA2
#define AUDIO_MIC3_CH1                ADC_AIN_PORT1    // PA3
// 当mic3模式选择差分模式时，输入N端引脚固定为  PA4

/********************************************************************************
                MICx  mic bias 供电IO配置(要注意IO与micin IO配置互斥)
 ********************************************************************************/
// TCFG_AUDIO_MICx_BIAS_SEL
#define AUDIO_MIC_BIAS_NULL           (0)       // no bias
#define AUDIO_MIC_BIAS_CH0            BIT(0)    // PA0
#define AUDIO_MIC_BIAS_CH1            BIT(1)    // PA1
#define AUDIO_MIC_BIAS_CH2            BIT(2)    // PC10
#define AUDIO_MIC_BIAS_CH3            BIT(3)    // PA5
#define AUDIO_MIC_LDO_PWR             BIT(4)    // PA0

/*MIC LDO index输出定义*/
#define MIC_LDO					BIT(0)	//PA0输出原始MIC_LDO
#define MIC_LDO_BIAS0			BIT(1)	//Pxx输出经过内部上拉电阻分压的偏置
#define MIC_LDO_BIAS1			BIT(2)	//Pxx输出经过内部上拉电阻分压的偏置
#define MIC_LDO_BIAS2			BIT(3)	//Pxx输出经过内部上拉电阻分压的偏置
#define MIC_LDO_BIAS3			BIT(4)	//Pxx输出经过内部上拉电阻分压的偏置

#define AUD_MIC_GB_0dB      (0)
#define AUD_MIC_GB_6dB      (1)
/********************************************************************************
                         LINEINx  输入IO配置
 ********************************************************************************/
// TCFG_AUDIO_LINEIN0_AIN_SEL
#define AUDIO_LINEIN0_CH0             ADC_AIN_PORT0    // PB9
#define AUDIO_LINEIN0_CH1             ADC_AIN_PORT1    // PB7
#define AUDIO_LINEIN0_CH2             ADC_AIN_PORT2    // PA0  (复用 MIC BIAS)

// TCFG_AUDIO_LINEIN1_AIN_SEL
#define AUDIO_LINEIN1_CH0             ADC_AIN_PORT0    // PB10
#define AUDIO_LINEIN1_CH1             ADC_AIN_PORT1    // PB8
#define AUDIO_LINEIN1_CH2             ADC_AIN_PORT2    // PA1  (复用 MIC BIAS)

// TCFG_AUDIO_LINEIN2_AIN_SEL
#define AUDIO_LINEIN2_CH0             ADC_AIN_PORT0    // PC9
#define AUDIO_LINEIN2_CH1             ADC_AIN_PORT1    // PC10 (复用 MIC BIAS)

// TCFG_AUDIO_LINEIN3_AIN_SEL
#define AUDIO_LINEIN3_CH0             ADC_AIN_PORT0    // PA2
#define AUDIO_LINEIN3_CH1             ADC_AIN_PORT1    // PA3

#define AUD_LINEIN_GB_0dB      (0)
#define AUD_LINEIN_GB_6dB      (1)

struct audio_adc_output_hdl {
    struct list_head entry;
    void *priv;
    void (*handler)(void *, s16 *, int);
};

struct audio_adc_private_param {
    u8 micldo_vsel;			// default value=3
};

struct mic_open_param {
    u8 mic_ain_sel;       // 0/1/2
    u8 mic_bias_sel;      // A(PA0)/B(PA1)/C(PC10)/D(PA5)
    u8 mic_bias_rsel;     // 单端隔直电容mic bias rsel
    u8 mic_mode : 4;      // MIC工作模式
    u8 mic_dcc : 4;       // DCC level
};

struct linein_open_param {
    u8 linein_ain_sel;       // 0/1/2
    u8 linein_mode : 4;      // LINEIN 工作模式
    u8 linein_dcc : 4;       // DCC level
};

struct audio_adc_hdl {
    struct list_head head;
    struct audio_adc_private_param *private;
    spinlock_t lock;
    u8 adc_sel[AUDIO_ADC_MAX_NUM];
    u8 adc_dcc[AUDIO_ADC_MAX_NUM];
    struct mic_open_param mic_param[AUDIO_ADC_MAX_NUM];
    struct linein_open_param linein_param[AUDIO_ADC_MAX_NUM];
    u8 mic_ldo_state;
    u8 state;
    u8 channel;
    u8 channel_num;
    u8 mic_num;
    u8 linein_num;
    s16 *hw_buf;   //ADC 硬件buffer的地址
    u8 max_adc_num; //默认打开的ADC通道数
    u8 buf_fixed;  //是否固定adc硬件使用的buffer地址
    u8 bit_width;
    OS_MUTEX mutex;
    u32 timestamp;
};

struct adc_mic_ch {
    struct audio_adc_hdl *adc;
    u8 gain[AUDIO_ADC_MIC_MAX_NUM];
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_mic_ch *, s16 *, u16);
    u8 ch_map;
};

struct adc_linein_ch {
    struct audio_adc_hdl *adc;
    u8 gain[AUDIO_ADC_LINEIN_MAX_NUM];
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_linein_ch *, s16 *, u16);
};

typedef struct {
    int a;
} audio_adc_mic_mana_t;

#endif/*AUDIO_ADC_H*/

