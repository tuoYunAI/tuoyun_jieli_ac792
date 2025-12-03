#ifndef __CPU_DAC_H__
#define __CPU_DAC_H__


#include "generic/typedef.h"
#include "os/os_api.h"
#include "audio_src.h"
#include "media/audio_cfifo.h"
#include "system/spinlock.h"
#include "audio_def.h"
#include "audio_general.h"

/***************************************************************************
  							Audio DAC Features
Notes:以下为芯片规格定义，不可修改，仅供引用
***************************************************************************/
#define AUDIO_DAC_CHANNEL_NUM				4	//DAC通道数
#define AUDIO_ADDA_IRQ_MULTIPLEX_ENABLE			//DAC和ADC中断入口复用使能


/************************************************************************************************************
                                       AUDIO DAC 相关配置

1.  DAC 的输出声道数和输出方式由 TCFG_AUDIO_DAC_CONNECT_MODE 和 TCFG_AUDIO_DAC_MODE 来决定。
    下表是不同组合的支持情况，如果支持该组合配置表格中为使用的DAC输出引脚，不支持的则为 not support。

    +-------------------------------------+------------------+---------------+----------------------------+
    |                                     | DAC_MODE_SINGLE  | DAC_MODE_DIFF | DAC_MODE_VCMO (VCMO就是RL) |
    +-------------------------------------+------------------+---------------+----------------------------+
    |DAC_OUTPUT_MONO_L           (1 声道) | FL               |  FL-FR        |  FL-VCMO                   |
    |DAC_OUTPUT_LR               (2 声道) | FL   FR          |  FL-FR  RL-RR |  FL-VCMO  FR-VCMO          |
    |DAC_OUTPUT_FRONT_LR_REAR_L  (3 声道) | FL   FR   RL     |  not support  |  not support               |
    |DAC_OUTPUT_FRONT_LR_REAR_R  (3 声道) |   not support    |  not support  |  FL-VCMO  FR-VCMO  RR-VCMO |
    |DAC_OUTPUT_FRONT_LR_REAR_LR (4 声道) | FL   FR   RL  RR |  not support  |  not support               |
    +-------------------------------------+------------------+---------------+----------------------------+

2.  DAC 支持高性能模式（DAC_MODE_HIGH_PERFORMANCE）和低功耗模式（DAC_MODE_LOW_POWER ），
    用户可依据实际场景配置 DAC_PERFORMANCE_MODE 选择。

3.  TCFG_DAC_POWER_LEVEL 控制DAC供电电压，外部硬件VCM引脚有挂电容的选择 AUDIO_VCM_CAP_LEVEL1/2/3/4/5，
    VCM引脚没挂电容的选择 AUDIO_VCM_CAPLESS_LEVEL1/2/3/4，数字越大输出幅度越大，功耗越高。
************************************************************************************************************/

// TCFG_AUDIO_DAC_CONNECT_MODE
//#define DAC_OUTPUT_MONO_L                  (0)   //左声道
//#define DAC_OUTPUT_LR                      (2)   //立体声
//#define DAC_OUTPUT_FRONT_LR_REAR_L         (7)   //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
//#define DAC_OUTPUT_FRONT_LR_REAR_R         (8)   //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
//#define DAC_OUTPUT_FRONT_LR_REAR_LR        (9)   //四声道单端输出

//#define DAC_OUTPUT_MONO_R                  (1)   // 未使用
//#define DAC_OUTPUT_MONO_LR_DIFF            (3)   // 未使用
//#define DAC_OUTPUT_DUAL_LR_DIFF            (6)   // 未使用


// TCFG_AUDIO_DAC_MODE
#define DAC_MODE_SINGLE                    (0)
#define DAC_MODE_DIFF                      (1)
#define DAC_MODE_VCMO                      (2)

/************************************
             dac性能模式
************************************/
// TCFG_DAC_PERFORMANCE_MODE
#define	DAC_MODE_HIGH_PERFORMANCE          (0)
#define	DAC_MODE_LOW_POWER		           (1)

/************************************
             dac 供电模式
************************************/
// TCFG_DAC_POWER_LEVEL
#define	AUDIO_VCM_CAP_LEVEL1	           (0)	// VCM-cap, VCM = 1.2v
#define	AUDIO_VCM_CAP_LEVEL2	           (1)  // VCM-cap, VCM = 1.3v
#define	AUDIO_VCM_CAP_LEVEL3	           (2)	// VCM-cap, VCM = 1.4v
#define	AUDIO_VCM_CAP_LEVEL4	           (3)	// VCM-cap, VCM = 1.5v
#define	AUDIO_VCM_CAP_LEVEL5	           (4)	// VCM-cap, VCM = 1.6v
#define	AUDIO_VCM_CAPLESS_LEVEL1           (5)	// VCM-capless, DACLDO = 2.4v
#define	AUDIO_VCM_CAPLESS_LEVEL2           (6)	// VCM-capless, DACLDO = 2.6v
#define	AUDIO_VCM_CAPLESS_LEVEL3           (7)	// VCM-capless, DACLDO = 2.8v
#define	AUDIO_VCM_CAPLESS_LEVEL4           (8)	// VCM-capless, DACLDO = 3.0v


#define AUDIO_DAC_SYNC_IDLE             0
#define AUDIO_DAC_SYNC_PROBE            1
#define AUDIO_DAC_SYNC_START            2
#define AUDIO_DAC_SYNC_NO_DATA          3
#define AUDIO_DAC_ALIGN_NO_DATA         4
#define AUDIO_DAC_SYNC_ALIGN_COMPLETE   5
#define AUDIO_DAC_SYNC_KEEP_RATE_DONE   6

#define AUDIO_SRC_SYNC_ENABLE   1
#define SYNC_LOCATION_FLOAT      1
#if SYNC_LOCATION_FLOAT
#define PCM_PHASE_BIT           0
#else
#define PCM_PHASE_BIT           8
#endif

#define DA_LEFT        0
#define DA_RIGHT       1
#define DA_REAR_RIGHT  2
#define DA_REAR_LEFT   3

#define DA_SOUND_NORMAL                 0x0
#define DA_SOUND_RESET                  0x1
#define DA_SOUND_WAIT_RESUME            0x2

#define DACR32_DEFAULT		8192
#define DA_SYNC_INPUT_BITS              20
#define DA_SYNC_MAX_NUM                 (1 << DA_SYNC_INPUT_BITS)


#define DAC_ANALOG_OPEN_PREPARE         (1)
#define DAC_ANALOG_OPEN_FINISH          (2)
#define DAC_ANALOG_CLOSE_PREPARE        (3)
#define DAC_ANALOG_CLOSE_FINISH         (4)

#define DAC_TRIM_SEL_FL_P               0
#define DAC_TRIM_SEL_FL_N               0
#define DAC_TRIM_SEL_FR_P               1
#define DAC_TRIM_SEL_FR_N               1
#define DAC_TRIM_SEL_RL_P               2
#define DAC_TRIM_SEL_RL_N               2
#define DAC_TRIM_SEL_RR_P               3
#define DAC_TRIM_SEL_RR_N               3
#define DAC_TRIM_SEL_FL                 4
#define DAC_TRIM_SEL_FR                 4
#define DAC_TRIM_SEL_RL                 5
#define DAC_TRIM_SEL_RR                 5
#define DAC_TRIM_SEL_VCM                6

#define DAC_TRIM_PA_FL                  0
#define DAC_TRIM_PA_FR                  1
#define DAC_TRIM_PA_RL                  2
#define DAC_TRIM_PA_RR                  3

#define TYPE_DAC_AGAIN      (0x01)
#define TYPE_DAC_DGAIN      (0x02)

//void audio_dac_power_state(u8 state)
//在应用层重定义 audio_dac_power_state 函数可以获取dac模拟开关的状态
struct audio_dac_hdl;

struct dac_platform_data {
    void (*analog_open_cb)(struct audio_dac_hdl *);
    void (*analog_close_cb)(struct audio_dac_hdl *);
    void (*analog_light_open_cb)(struct audio_dac_hdl *);
    void (*analog_light_close_cb)(struct audio_dac_hdl *);
    float fixed_pns;            // 固定pns,单位ms
    u16 dma_buf_time_ms; 	    // DAC dma buf 大小
    u8 output;              	// DAC输出模式
    u8 output_mode;         	// single/diff/vcmo
    u8 performance_mode;    	// low_power/high_performance
    u8 l_ana_gain;				// 前左声道模拟增益
    u8 r_ana_gain;				// 前右声道模拟增益
    u8 rl_ana_gain;				// 后左声道模拟增益
    u8 rr_ana_gain;				// 后右声道模拟增益
    u8 light_close;         	// 使能轻量级关闭，最低功耗保持dac开启
    u32 max_sample_rate;    	// 支持的最大采样率
    u8 dcc_level;				// DAC去直流滤波器档位, 0~7:关闭    8~15：开启(档位越大，高通截止点越小)
    u8 bit_width;             	// DAC输出位宽
    u8 power_level;				// 电源挡位
    u8 pa_isel0;                // 电流档位:4~6
    u8 pa_isel1;                // 电流档位:3~6
    u8 pa_isel2;                // 电流档位:3~6
};

struct analog_module {
    /*模拟相关的变量*/
    u8 inited;
};

struct trim_init_param_t {
    u8 clock_mode;
    u8 power_level;
};

struct audio_dac_trim {
    s16 left_p;
    s16 left_n;
    s16 right_p;
    s16 right_n;
    s16 vcomo;
};

// *INDENT-OFF*
struct audio_dac_sync {
    u32 channel : 3;
    u32 start : 1;
    u32 fast_align : 1;
    u32 connect_sync : 1;
    u32 release_by_bt : 1;
    u32 resevered : 1;
    u32 input_num : DA_SYNC_INPUT_BITS;
    int fast_points;
    int keep_points;
    int phase_sub;
    int in_rate;
    int out_rate;
    int bt_clkn;
    int bt_clkn_phase;
#if AUDIO_SRC_SYNC_ENABLE
    struct audio_src_sync_handle *src_sync;
    void *buf;
    int buf_len;
    void *filt_buf;
    int filt_len;
#else
    struct audio_src_base_handle *src_base;
#endif
#if SYNC_LOCATION_FLOAT
    float pcm_position;
#else
    u32 pcm_position;
#endif
    void *priv;
    int (*handler)(void *priv, u8 state);
    void *correct_priv;
    void (*correct_cabllback)(void *priv, int diff);
};

struct audio_dac_fade {
    u8 enable;
    volatile u8 ref_L_gain;
    volatile u8 ref_R_gain;
    int timer;
};

struct audio_dac_sync_node {
    u8 triggered;
    u8 network;
    u32 timestamp;
    void *hdl;
    struct list_head entry;
    void *ch;
};

struct audio_dac_channel_attr {
    u8  write_mode;         /*DAC写入模式*/
    u16 delay_time;         /*DAC通道延时*/
    u16 protect_time;       /*DAC延时保护时间*/
};

struct audio_dac_channel {
    u8  state;              /*DAC状态*/
    u8  pause;
    u8  samp_sync_step;     /*数据流驱动的采样同步步骤*/
    struct audio_dac_channel_attr attr;     /*DAC通道属性*/
    struct audio_sample_sync *samp_sync;    /*样点同步句柄*/
    struct audio_dac_hdl *dac;              /* DAC设备*/
    struct audio_cfifo_channel fifo;        /*DAC cfifo通道管理*/

    //struct audio_dac_sync sync;
    // struct list_head sync_list;
};

struct audio_dac_hdl {
    struct analog_module analog;
    const struct dac_platform_data *pd;
    OS_SEM sem;
    struct audio_dac_trim trim;
    void (*fade_handler)(u8 left_gain, u8 right_gain);
    void (*update_frame_handler)(u8 channel_num, void *data, u32 len);
    volatile u8 mute;
    volatile u8 state;
    volatile u8 light_state;
    volatile u8 agree_on;
    u8 ng_threshold;
    u8 gain;
    u8 vol_l;
    u8 vol_r;
    u8 channel;
    u16 max_d_volume;
    u16 d_volume[4];
    u32 sample_rate;
    u32 output_buf_len;
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让DAC真正跑之前写入的PCM点数
    u16 irq_points;
    s16 protect_time;
    s16 protect_pns;
    s16 fadein_frames;
    s16 fade_vol;
    u8 protect_fadein;
    u8 vol_set_en;
    u8 volume_enhancement;
    u8 sound_state;
    unsigned long sound_resume_time;
    s16 *output_buf;

    u8 anc_dac_open;
    u8 dac_read_flag;	//AEC可读取DAC参考数据的标志

    u8 fifo_state;
    u16 unread_samples;             /*未读样点个数*/
    struct audio_cfifo fifo;        /*DAC cfifo结构管理*/
    struct audio_dac_channel main_ch;

	u8 dvol_mute;             //DAC数字音量是否mute
#if 0
    struct audio_dac_sync sync;
    struct list_head sync_list;
    u8 sync_step;
#endif

	u8 active;
    u8 clk_fre;     // 0:6M   1:6.6666M

    void *resample_ch;
    void *resample_buf;
    int resample_buf_len;
	void *feedback_priv;
    void (*underrun_feedback)(void *priv);
/*******************************************/
/**sniff退出时，dac模拟提前初始化，避免模拟初始化延时,影响起始同步********/
	u8 power_on;
    u8 need_close;
    OS_MUTEX mutex;
    OS_MUTEX mutex_power_off;
    OS_MUTEX dvol_mutex;
/*******************************************/
    spinlock_t lock;
	u32 dac_dvol;             //记录DAC 停止前数字音量寄存器DAC_VL0的值
	u32 dac_dvol1;            //记录DAC 停止前数字音量寄存器DAC_VL1的值

/*******************************************/
	struct list_head sync_list;
    struct audio_dac_noisegate ng;
};



#endif

