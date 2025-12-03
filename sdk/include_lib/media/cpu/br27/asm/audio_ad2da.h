#ifndef _AUDIO_AD2DA_H_
#define _AUDIO_AD2DA_H_

#include "generic/typedef.h"
/*
*********************************************************************
*                  AUDIO AD2DA OPEN
* Description: 打开AD2DA模块
* Arguments  : adc_index        ADC通道(AUDIO_ADC_LINE0/AUDIO_ADC_LINE1/
*                               AUDIO_ADC_LINE2/AUDIO_ADC_LINE3(单选)
*              adc_ch_sel       ADC通道的模拟接口(AUDIO_LINEIN0_CH0/
*                               AUDIO_LINEIN1_CH0/AUDIO_LINEIN2_CH0...)
*                               (需要与adc_index的通道对应)
*              dac_out_mapping  AD2DA输出的通道(DAC_CH_FL/DAC_CH_FR/
*                               DAC_CH_RL/DAC_CH_RR/)(可通过或运算多
*                               选)
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_ad2da_open(u8 adc_index, u8 adc_ch_sel, u8 dac_out_mapping);
/*
*********************************************************************
*                  AUDIO AD2DA CLOSE
* Description: 关闭AD2DA模块
* Arguments  : adc_index        ADC通道(ADC_INDEX_CH0/ADC_INDEX_CH1/
*                               ADC_INDEX_CH2/ADC_INDEX_CH3)(单选)
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_ad2da_close(u8 adc_index);
/*
*********************************************************************
*                  AUDIO AD2DA GAIN
* Description: 设置AD2DA的增益
* Arguments  : ad2da_ch         设置的AD2DA通道(多选)
*              digital_gain     设置通道的增益
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_ad2da_gain(u8 ad2da_ch, int digital_gain);

#endif // #ifndef _AUDIO_AD2DA_H_
