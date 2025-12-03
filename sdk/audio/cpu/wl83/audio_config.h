#ifndef _APP_AUDIO_H_
#define _APP_AUDIO_H_

#include "generic/typedef.h"
#include "audio_dvol.h"
#include "audio_config_def.h"
#include "audio_adc.h"

struct adc_platform_cfg {
    u8 mic_mode;          // MIC工作模式
    u8 mic_ain_sel;       // 0/1/2
    u8 mic_bias_sel;      // A(PA0)/B(PA1)/C(PC10)/D(PA5)
    u8 mic_bias_rsel;     // 单端隔直电容mic bias rsel
    u8 mic_dcc;           // DCC level
    u8 inside_bias_resistor; /*!< MIC BIAS是否使用内置电阻 */
    u16 power_io;         // MIC供电IO
    u8 dmic_enable;       /*!< 数字麦通道使能，此时模拟麦通道会被替换 */
    u8 dmic_ch0_mode;     /*!< 通道0输入模式选择 */
    u8 dmic_ch1_mode;     /*!< 通道1输入模式选择 */
    u32 dmic_sclk_fre;    /*!< 数字麦的sclk输出频率 */
    int dmic_io_sclk;     /*!< 数字麦的sclk输出引脚 */
    int dmic_io_idat0;    /*!< 数字麦的dat输入引脚0 */
    int dmic_io_idat1;    /*!< 数字麦的dat输入引脚1 */
};

struct adc_file_param {
    u8 mic_gain;		  // MIC增益
    u8 mic_pre_gain;      // MIC前级增益 0:0dB   1:6dB
} __attribute__((packed));

//MIC电源控制
typedef enum {
    MIC_PWR_INIT = 1,	/*开机状态*/
    MIC_PWR_ON,			/*工作状态*/
    MIC_PWR_OFF,		/*空闲状态*/
    MIC_PWR_DOWN,		/*低功耗状态*/
} audio_mic_pwr_t;

static const char *const audio_vol_str[] = {
    "Vol_BtMusic",
    "Vol_BtCall",
    "Vol_LineinMusic",
    "Vol_FileMusic",
    "Vol_NetMusic",
    "Vol_FmMusic",
    "Vol_Virtual",
    "Vol_SpdMusic",
    "Vol_PcspkMusic",
    "Vol_SysTone",
    "Vol_SysKTone",
    "Vol_SysRing",
    "Vol_SysTTS",
    "Vol_SysTWSTone",
    "NULL",
};

//音量名称index
typedef enum {
    AppVol_BT_MUSIC = 0,
    AppVol_BT_CALL,

    AppVol_LINEIN,
    AppVol_MUSIC,
    AppVol_NET_MUSIC,
    AppVol_FM,
    AppVol_VIRTUAL,
    AppVol_SPDIF,
    AppVol_USB,

    SysVol_TONE,
    SysVol_KEY_TONE,
    SysVol_RING,
    SysVol_TTS,
    SysVol_TWS_TONE,
    Vol_NULL,
} audio_vol_index_t;

/*
*********************************************************************
*                  Audio Volume Get
* Description: 音量获取
* Arguments  : state	要获取音量值的音量状态
* Return	 : 返回指定状态对应得音量值
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_get_volume(u8 state);

/*
*********************************************************************
*                  Audio Mute Get
* Description: mute状态获取
* Arguments  : state	要获取是否mute的音量状态
* Return	 : 返回指定状态对应的mute状态
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_get_mute_state(u8 state);

/*
*********************************************************************
*          			Audio Volume Set
* Description: 音量设置
* Arguments  : state	目标声音通道
*			   volume	目标音量值
*			   fade		是否淡入淡出
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_set_volume(u8 state, s16 volume, u8 fade);

/*
*********************************************************************
*          			Audio Volume Set
* Description: 改变音量状态保存值
* Arguments  : state	目标声音通道
*			   volume	目标音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_change_volume(u8 state, s16 volume);

/*
*********************************************************************
*                  Audio Volume Up
* Description: 增加当前音量通道的音量
* Arguments  : value	要增加的音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_volume_up(u8 value);

/*
*********************************************************************
*                  Audio Volume Down
* Description: 减少当前音量通道的音量
* Arguments  : value	要减少的音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_volume_down(u8 value);

/*
*********************************************************************
*                  Audio State Switch
* Description: 切换声音状态
* Arguments  : state
*			   max_volume
               dvol_hdl
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_state_switch(u8 state, s16 max_volume, dvol_handle *dvol_hdl);

/*
*********************************************************************
*                  Audio State Exit
* Description: 退出当前的声音状态
* Arguments  : state	要退出的声音状态
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_state_exit(u8 state);

/*
*********************************************************************
*                  Audio State Get
* Description: 获取当前声音状态
* Arguments  : None.
* Return	 : 返回当前的声音状态
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_get_state(void);

void app_audio_mute(u8 value);

u8 app_audio_get_dac_digital_mute(void); //获取DAC 是否mute
/*
*********************************************************************
*                  Audio Volume_Max Get
* Description: 获取当前声音通道的最大音量
* Arguments  : None.
* Return	 : 返回当前的声音通道最大音量
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_get_max_volume(void);

/*
*********************************************************************
*                  Audio Set Max Volume
* Description: 设置最大音量
* Arguments  : state		要设置最大音量的声音状态
*			   max_volume	最大音量
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_set_max_volume(u8 state, s16 max_volume);

u8 app_audio_bt_volume_update(u8 *btaddr, u8 state);

void app_audio_bt_volume_save(u8 state);

void app_audio_bt_volume_save_mac(u8 *addr);

void audio_fade_in_fade_out(u8 left_vol, u8 right_vol);

int audio_digital_vol_default_init(void);

void volume_up_down_direct(s16 value);
void audio_combined_vol_init(u8 cfg_en);
void audio_volume_list_init(u8 cfg_en);

void dac_power_on(void);
void dac_power_off(void);

/*打印audio模块的数字模拟增益：DAC/ADC*/
void audio_config_dump(void);
void audio_adda_dump(void); //打印所有的dac,adc寄存器

//MIC静音标志获取
u8 audio_common_mic_mute_en_get(void);

//MIC静音设置接口
void audio_common_mic_mute_en_set(u8 mute_en);

void app_audio_set_volume_def_state(u8 volume_def_state);
/*
*********************************************************************
*          audio_dac_volume_enhancement_mode_set
* Description: DAC 音量增强模式切换
* Arguments  : mode		1：音量增强模式  0：普通模式
* Return	 : NULL
* Note(s)    : None.
*********************************************************************
*/
void app_audio_dac_vol_mode_set(u8 mode);

/*
*********************************************************************
*           app_audio_dac_vol_mode_get
* Description: DAC 音量增强模式状态获取
* Arguments  : None.
* Return	 : 1：音量增强模式  0：普通模式
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_dac_vol_mode_get(void);

/*
*********************************************************************
*           audio_mic_pwr_ctl
* Description: MIC电源普通IO供电管理
* Arguments  : None.
* Return	 : state MIC电源状态
* Note(s)    : None.
*********************************************************************
*/
void audio_mic_pwr_ctl(audio_mic_pwr_t state);

/*
*********************************************************************
*           app_audio_volume_max_query
* Description: 音量最大值查询
* Arguments  : 目标音量index.
* Return	 : 目标音量最大值
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_volume_max_query(audio_vol_index_t index);

/*********************************************************************
*          			Audio Volume MUTE
* Description: 将数据静音或者解开静音
* Arguments  : mute_en	是否使能静音, 0:不使能,1:使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_mute_en(u8 mute_en);

void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_platform_cfg *platform_cfg);

void audio_linein_param_fill(struct linein_open_param *linein_param, const struct adc_platform_cfg *platform_cfg);

void audio_fast_mode_test();

s16 get_music_volume(void);

void set_music_volume(s16 value);

u8 get_opid_play_vol_sync(void);

void set_opid_play_vol_sync(u8 value);

#endif/*_APP_AUDIO_H_*/
