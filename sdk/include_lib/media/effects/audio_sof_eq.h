#ifndef _SOF_EQ_API_H_
#define _SOF_EQ_API_H_

#include "generic/typedef.h"
#include "system/timer.h"
#include "effects/audio_eq.h"
#include "effects/audio_sof_eq.h"
#include "effects/effects_adj.h"
#include "effects/fix_iir_filter_api.h"


struct audio_sof_eq_param {
    struct eq_seg_info *seg; //系数表
    float global_gain;       //总增益
    u32 sample_rate;         //采样率，更根据当前数据实际采样率填写
    u8 in_bit_width;         //输入数据位宽 0:short  1:int  2:float
    u8 out_bit_width;        //输出数据位宽 0:short  1:int  2:float
    u8 nsection;             //滤波器段数
    u8 channel;              //通道数,1：单声道  2：立体声
    u8 qval;                 //数据饱和位宽
};

struct audio_sof_eq {
    void *work_buf;                          //sof eq句柄
    float *coeff;                            //系数
    struct audio_eq_fade *fade;
    struct eq_fade_parm fade_parm;
    // struct audio_sof_eq_param oparam;
    struct iir_filter_param iir_param;
    struct eq_seg_info *cur_seg;
    void *file;
    float global_gain;
    int sample_rate;
    u64 mask;
    u8 channel;
    u8 cur_seg_nsection;//滤波器个数
    u8 cur_coeff_nsection;//滤波器计算后的段数（多阶滤波器段数可变，导致系数段数与滤波器个数会存在不一样的情况）
    u8 update;
};


struct audio_sof_eq *audio_sof_eq_open(struct audio_sof_eq_param *param);
int audio_sof_eq_close(struct audio_sof_eq *hdl);
int audio_sof_eq_run(struct audio_sof_eq *hdl, s16 *indata, s16 *outdata, u32 len);
void audio_sof_eq_update(struct audio_sof_eq *hdl, struct eq_fade_parm fade_parm, struct eq_tool *update_param);
void audio_sof_eq_hp_lp_adv_reset(struct eq_seg_info *seg, u8 seg_num);

#define debug_digital(x)  __builtin_abs((int)((x - (int)x) * 100)) //此处使用该宏定义，获取浮点两位小数，转int型后打印

#endif

