#ifndef _AUDIO_DAC_H_
#define _AUDIO_DAC_H_


#include "asm/cpu_dac.h"

// DAC IO
struct audio_dac_io_param {
    /*
     *       state 通道初始状态
     * 使能单左/单右声道，初始状态为高电平：state[0] = 1
     * 使能双声道，左声道初始状态为高，右声道初始状态为低：state[0] = 1，state[1] = 0。
     */
    u8 state[4];
    /*
     *       irq_points 中断点数
     * 申请buf的大小为 buf_len = irq_points * channel_num * 4
     */
    u16 irq_points;
    /*
     *       channel 打开的通道
     * 可配 “BIT(0)、BIT(1)、BIT(2)、BIT(3)” 对应 “FL FR RL RR”
     * 打开多通道时使用或配置：channel = BIT(0) | BIT(1) | BIT(2) | BIT(3);
     *
     * 注意，不支持以下配置类型：
     * channel = BIT(1)                             "MONO FR"
     * channel = DAC_CH(0) | DAC_CH(1) | DAC_CH(3)  "FL FR RR"
     */
    u8 channel;
    /*
     *       digital_gain 增益
     * 影响输出电平幅值，-8192~8192可配
     */
    u16 digital_gain;

    /*
     *       ldo_volt 电压
     * 影响输出电平幅值，0~3可配
     */
    u8 ldo_volt;
    /*
     *       clk_sel 时钟源选择
     * 差分晶振时钟/单端数字时钟
     */
    u8 clk_sel;
};

/*
*********************************************************************
*              audio_dac_init
* Description: DAC 初始化
* Arguments  : dac      dac 句柄
*			   pd		dac 参数配置结构体
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_init(struct audio_dac_hdl *dac, const struct dac_platform_data *pd);

void *audio_dac_get_pd_data(void);
/*
*********************************************************************
*              audio_dac_get_hdl
* Description: 获取 DAC 句柄
* Arguments  :
* Return	 : DAC 句柄
* Note(s)    :
*********************************************************************
*/
struct audio_dac_hdl *audio_dac_get_hdl(void);

int audio_dac_trim_value_check(struct audio_dac_trim *dac_trim);

/*
*********************************************************************
*              audio_dac_do_trim
* Description: DAC 直流偏置校准
* Arguments  : dac      	dac 句柄
*			   dac_trim		存放 trim 值结构体的地址
*              fast_trim 	快速 trim 使能
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_do_trim(struct audio_dac_hdl *dac, struct audio_dac_trim *dac_trim, struct trim_init_param_t *trim_init);

/*
*********************************************************************
*              audio_dac_set_trim_value
* Description: 将 DAC 直流偏置校准值写入 DAC 配置
* Arguments  : dac      	dac 句柄
*			   dac_trim		存放 trim 值结构体的地址
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_trim_value(struct audio_dac_hdl *dac, struct audio_dac_trim *dac_trim);

/*
*********************************************************************
*              audio_dac_set_delay_time
* Description: 设置 DAC DMA fifo 的启动延时和最大延时。
* Arguments  : dac      	dac 句柄
*              start_ms     启动延时，DAC 开启时候的 DMA fifo 延时
*              max_ms 	    DAC DMA fifo 的最大延时
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_delay_time(struct audio_dac_hdl *dac, int start_ms, int max_ms);

/*
*********************************************************************
*              audio_dac_irq_handler
* Description: DAC 中断回调函数
* Arguments  : dac      	dac 句柄
* Return	 :
* Note(s)    :
*********************************************************************
*/
void audio_dac_irq_handler(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_set_buff
* Description: 设置 DAC 的 DMA buff
* Arguments  : dac      	dac 句柄
*              buf  		应用层分配的 DMA buff 起始地址
*              len  		DMA buff 的字节长度
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_buff(struct audio_dac_hdl *dac, s16 *buf, int len);

/*
*********************************************************************
*              audio_dac_write
* Description: 将数据写入默认的 dac channel cfifo。等同于调用 audio_dac_channel_write 函数 private_data 传 NULL
* Arguments  : dac      	dac 句柄
*              buf  		数据的起始地址
*              len  		数据的字节长度
* Return	 : 成功写入的数据字节长度
* Note(s)    :
*********************************************************************
*/
int audio_dac_write(struct audio_dac_hdl *dac, void *buf, int len);

int audio_dac_get_write_ptr(struct audio_dac_hdl *dac, s16 **ptr);

int audio_dac_update_write_ptr(struct audio_dac_hdl *dac, int len);

/*
*********************************************************************
*              audio_dac_set_sample_rate
* Description: 设置 DAC 的输出采样率
* Arguments  : dac      		dac 句柄
*              sample_rate  	采样率
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_sample_rate(struct audio_dac_hdl *dac, int sample_rate);

/*
*********************************************************************
*              audio_dac_get_sample_rate
* Description: 获取 DAC 的输出采样率
* Arguments  : dac      		dac 句柄
* Return	 : 采样率
* Note(s)    :
*********************************************************************
*/
int audio_dac_get_sample_rate(struct audio_dac_hdl *dac);

int audio_dac_get_sample_rate_base_reg(void);

u32 audio_dac_select_sample_rate(u32 sample_rate);

int audio_dac_clk_switch(u8 clk);

int audio_dac_clk_get(void);

int audio_dac_set_channel(struct audio_dac_hdl *dac, u8 channel);

int audio_dac_get_channel(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_set_digital_vol
* Description: 设置 DAC 的数字音量
* Arguments  : dac      	dac 句柄
*              vol  		需要设置的数字音量等级
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_digital_vol(struct audio_dac_hdl *dac, u16 vol);

/*
*********************************************************************
*              audio_dac_set_analog_vol
* Description: 设置 DAC 的模拟音量
* Arguments  : dac      	dac 句柄
*              vol  		需要设置的模拟音量等级
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_analog_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_ch_analog_gain_set(struct audio_dac_hdl *dac, u32 ch, u32 again);

int audio_dac_ch_analog_gain_get(struct audio_dac_hdl *dac, u32 ch);

int audio_dac_ch_digital_gain_set(struct audio_dac_hdl *dac, u32 ch, u32 dgain);

int audio_dac_ch_digital_gain_get(struct audio_dac_hdl *dac, u32 ch);

/*
*********************************************************************
*              audio_dac_try_power_on
* Description: 根据设置好的参数打开 DAC 的模拟模块和数字模块。与 audio_dac_start 功能基本一样，但不设置 PNS
* Arguments  : dac      	dac 句柄
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_try_power_on(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_start
* Description: 根据设置好的参数打开 DAC 的模拟模块和数字模块。
* Arguments  : dac      	dac 句柄
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_start(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_stop
* Description: 关闭 DAC 数字部分。所有 DAC channel 都关闭后才能调用这个函数
* Arguments  : dac      	dac 句柄
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_stop(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_idle
* Description: 获取 DAC 空闲状态
* Arguments  : dac      	dac 句柄
* Return	 : 0：非空闲  1：空闲
* Note(s)    :
*********************************************************************
*/
int audio_dac_idle(struct audio_dac_hdl *dac);

void audio_dac_mute(struct audio_dac_hdl *hdl, u8 ch, u8 mute);

u8  audio_dac_digital_mute_state(struct audio_dac_hdl *hdl);

void audio_dac_digital_mute(struct audio_dac_hdl *dac, u8 mute);

int audio_dac_open(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_close
* Description: 关闭 DAC 模拟部分。audio_dac_stop 之后才可以调用
* Arguments  : dac      	dac 句柄
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_close(struct audio_dac_hdl *dac);

/*
*********************************************************************
*              audio_dac_set_volume
* Description: 设置音量等级记录变量，但是不会直接修改音量。只有当 DAC 关闭状态时，第一次调用 audio_dac_channel_start 打开 dac fifo 后，会根据 audio_dac_set_fade_handler 设置的回调函数来设置系统音量，回调函数的传参就是 audio_dac_set_volume 设置的音量值。
* Arguments  : dac      	dac 句柄
			   gain			记录的音量等级
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_volume(struct audio_dac_hdl *dac, u8 gain);

int audio_dac_set_L_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_R_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_RL_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_RR_digital_vol(struct audio_dac_hdl *dac, u16 vol);

void audio_dac_ch_high_resistance(struct audio_dac_hdl *dac, u8 ch, u8 en);

/*
*********************************************************************
*              audio_dac_ch_mute
* Description: 将某个通道静音，用于降低底噪，或者做串扰隔离的功能
* Arguments  : dac      	dac 句柄
*              ch			需要操作的通道，BIT(n)代表操作第n个通道，可以多个通道或上操作
*			   mute		    0:解除mute  1:mute
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
void audio_dac_ch_mute(struct audio_dac_hdl *dac, u8 ch, u8 mute);

void audio_dac_ch_set_mute_status(u8 ch, u8 mute);

/*
*********************************************************************
*              audio_dac_set_fade_handler
* Description: DAC 关闭状态时，第一次调用 audio_dac_channel_start 打开 dac fifo 后，会根据 audio_dac_set_fade_handler 设置的回调函数来设置系统音量
* Arguments  : dac      		dac 句柄
*              priv				暂无作用
*			   fade_handler 	回调函数的地址
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
void audio_dac_set_fade_handler(struct audio_dac_hdl *dac, void *priv, void (*fade_handler)(u8, u8));

/*
*********************************************************************
*              audio_dac_set_bit_mode
* Description: 设置 DAC 的数据位宽
* Arguments  : dac      		dac 句柄
*              bit_width	0:16bit  1:24bit
* Return	 : 0：成功  -1：失败
* Note(s)    :
*********************************************************************
*/
int audio_dac_set_bit_mode(struct audio_dac_hdl *dac, u8 bit_width);

int audio_dac_sound_reset(struct audio_dac_hdl *dac, u32 msecs);

int audio_dac_get_max_channel(void);

int audio_dac_get_status(struct audio_dac_hdl *dac);

u8 audio_dac_is_working(struct audio_dac_hdl *dac);

int audio_dac_set_irq_time(struct audio_dac_hdl *dac, int time_ms);

int audio_dac_data_time(struct audio_dac_hdl *dac);

void audio_dac_anc_set(struct audio_dac_hdl *dac, u8 toggle);

int audio_dac_irq_enable(struct audio_dac_hdl *dac, int time_ms, void *priv, void (*callback)(void *));

int audio_dac_set_protect_time(struct audio_dac_hdl *dac, int time, void *priv, void (*feedback)(void *));

int audio_dac_buffered_frames(struct audio_dac_hdl *dac);

void audio_dac_add_syncts_handle(struct audio_dac_hdl *dac, void *syncts);

void audio_dac_remove_syncts_handle(struct audio_dac_channel *ch, void *syncts);

void audio_dac_count_to_syncts(struct audio_dac_channel *ch, int frames);
void audio_dac_syncts_latch_trigger(struct audio_dac_channel *ch);
int audio_dac_add_syncts_with_timestamp(struct audio_dac_channel *ch, void *syncts, u32 timestamp);
void audio_dac_syncts_trigger_with_timestamp(struct audio_dac_channel *ch, u32 timestamp);
/*
 * 音频同步
 */
void *audio_dac_resample_channel(struct audio_dac_hdl *dac);

int audio_dac_sync_resample_enable(struct audio_dac_hdl *dac, void *resample);

int audio_dac_sync_resample_disable(struct audio_dac_hdl *dac, void *resample);

void audio_dac_set_input_correct_callback(struct audio_dac_hdl *dac,
        void *priv,
        void (*callback)(void *priv, int diff));

int audio_dac_set_sync_buff(struct audio_dac_hdl *dac, void *buf, int len);

int audio_dac_set_sync_filt_buff(struct audio_dac_hdl *dac, void *buf, int len);

int audio_dac_sync_open(struct audio_dac_hdl *dac);

int audio_dac_sync_set_channel(struct audio_dac_hdl *dac, u8 channel);

int audio_dac_sync_set_rate(struct audio_dac_hdl *dac, int in_rate, int out_rate);

int audio_dac_sync_auto_update_rate(struct audio_dac_hdl *dac, u8 on_off);

int audio_dac_sync_flush_data(struct audio_dac_hdl *dac);

int audio_dac_sync_fast_align(struct audio_dac_hdl *dac, int in_rate, int out_rate, int fast_output_points, float phase_diff);

#if SYNC_LOCATION_FLOAT
float audio_dac_sync_pcm_position(struct audio_dac_hdl *dac);
#else
u32 audio_dac_sync_pcm_position(struct audio_dac_hdl *dac);
#endif

int audio_dac_sync_keep_rate(struct audio_dac_hdl *dac, int points);

int audio_dac_sync_pcm_input_num(struct audio_dac_hdl *dac);

void audio_dac_sync_input_num_correct(struct audio_dac_hdl *dac, int num);

void audio_dac_set_sync_handler(struct audio_dac_hdl *dac, void *priv, int (*handler)(void *priv, u8 state));

int audio_dac_sync_start(struct audio_dac_hdl *dac);

int audio_dac_sync_stop(struct audio_dac_hdl *dac);

int audio_dac_sync_reset(struct audio_dac_hdl *dac);

int audio_dac_sync_data_lock(struct audio_dac_hdl *dac);

int audio_dac_sync_data_unlock(struct audio_dac_hdl *dac);

void audio_dac_sync_close(struct audio_dac_hdl *dac);

u32 local_audio_us_time_set(u16 time);

int local_audio_us_time(void);

int audio_dac_start_time_set(void *_dac, u32 us_timeout, u32 cur_time, u8 on_off);

u32 audio_dac_sync_pcm_total_number(void *_dac);

void audio_dac_sync_set_pcm_number(void *_dac, u32 output_points);

u32 audio_dac_pcm_total_number(void *_dac, int *pcm_r);

u8 audio_dac_sync_empty_state(void *_dac);

void audio_dac_sync_empty_reset(void *_dac, u8 state);

void audio_dac_set_empty_handler(void *_dac, void *empty_priv, void (*handler)(void *priv, u8 empty));

void audio_dac_set_dcc(u8 dcc);

u8 audio_dac_ana_gain_mapping(u8 level);
/*
*********************************************************************
*          audio_dac_volume_enhancement_mode_set
* Description: DAC 音量增强模式切换
* Arguments  : dac      dac 句柄
*			   mode		1：音量增强模式  0：普通模式
* Return	 : 0：成功  -1：失败
* Note(s)    : 不能在中断调用
*********************************************************************
*/
int audio_dac_volume_enhancement_mode_set(struct audio_dac_hdl *dac, u8 mode);

/*
*********************************************************************
*          audio_dac_volume_enhancement_mode_get
* Description: DAC 音量增强模式切换
* Arguments  : dac      dac 句柄
* Return	 : 1：音量增强模式  0：普通模式
* Note(s)    : None.
*********************************************************************
*/
u8 audio_dac_volume_enhancement_mode_get(struct audio_dac_hdl *dac);

void audio_dac_channel_start(void *private_data);
void audio_dac_channel_close(void *private_data);
int audio_dac_channel_write(void *private_data, struct audio_dac_hdl *dac, void *buf, int len);
int audio_dac_channel_set_attr(struct audio_dac_channel *ch, struct audio_dac_channel_attr *attr);
int audio_dac_new_channel(struct audio_dac_hdl *dac, struct audio_dac_channel *ch);

void audio_dac_add_update_frame_handler(struct audio_dac_hdl *dac, void (*update_frame_handler)(u8, void *, u32));
void audio_dac_del_update_frame_handler(struct audio_dac_hdl *dac);
int audio_dac_adapter_link_to_syncts_check(struct audio_dac_hdl *dac, void *syncts);
/*AEC参考数据软回采接口*/
int audio_dac_read_reset(void);
int audio_dac_read(s16 points_offset, void *data, int len, u8 read_channel);
int audio_dac_vol_set(u8 type, u32 ch, u16 gain, u8 fade_en);

int audio_dac_noisefloor_optimize_onoff(u8 onoff);
void audio_dac_noisefloor_optimize_reset(void);

void audio_dac_io_init(struct audio_dac_io_param *param);

void audio_dac_io_uninit(struct audio_dac_io_param *param);

/*
 * ch：通道
 *    可配 “BIT(0)、BIT(1)、BIT(2)、BIT(3)” 对应 “FL FR RL RR”
 *    多通道时使用或配置：channel = BIT(0) | BIT(1) | BIT(2) | BIT(3);
 * val：电平
 *    高电平 val = 1
 *    低电平 val = 0
 */
void audio_dac_io_set(u8 ch, u8 val);

#endif

