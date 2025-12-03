#ifndef _AUDIO_ANALOG_AUX_H_
#define _AUDIO_ANALOG_AUX_H_

#include "asm/audio_analog_aux_cpu.h"

/*
*********************************************************************
*                  Audio AUX Open
* Description: 打开模拟AUX模块
* Arguments  : aux->ch    AUX通道选择: AUX_CH0/1/...(支持CH0|CH1...配置)
*              aux->port0 AUX输入端口选择: AUX_AIN_PORT0/1/2/3/4/...(支持PORTx|PORTy|PORTz...配置)
*              aux->port1 AUX输入端口选择: AUX_AIN_PORT0/1/2/3/4/...(支持PORTx|PORTy|PORTz...配置)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_analog_aux_open(struct audio_aux_hdl *aux);

/*
*********************************************************************
*                  Audio AUX Close
* Description: 关闭模拟AUX模块
* Arguments  : aux->ch    AUX通道选择: AUX_CH0/1/...(支持CH0|CH1...配置)
*              aux->port0 AUX输入端口选择: AUX_AIN_PORT0/1/2/3/4/...(支持PORTx|PORTy|PORTz...配置)
*              aux->port1 AUX输入端口选择: AUX_AIN_PORT0/1/2/3/4/...(支持PORTx|PORTy|PORTz...配置)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_analog_aux_close(struct audio_aux_hdl *aux);

/*
*********************************************************************
*                  Audio AUX Set Gain Boost
* Description: 设置第一级/前级增益
* Arguments  : ch   AUX通道选择: AUX_CH0/1
*              port AUX输入端口选择: AUX_AIN_PORT0/1/2/3/4
*              gain 前级增益档位: 0/1(0dB/6dB@ain0~ain3), 0~3(17dB~26dB, step:3dB@ain4)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_analog_aux_set_gain_boost(u8 ch, u8 port, u8 gain);

/*
*********************************************************************
*                  Audio AUX Get Gain Boost
* Description: 获取第一级/前级增益
* Arguments  : ch   AUX通道选择: AUX_CH0/1
*              port AUX输入端口选择: AUX_AIN_PORT0/1/2/3/4
* Return	 : gain 前级增益档位: 0/1(0dB/6dB@ain0~ain3), 0~3(17dB~26dB, step:3dB@ain4)
* Note(s)    : None.
*********************************************************************
*/
int audio_analog_aux_get_gain_boost(u8 ch, u8 port);

/*
*********************************************************************
*                  Audio AUX Set Gain
* Description: 设置第二级/后级增益
* Arguments  : ch    AUX通道选择: AUX_CH0/1
*              gain AUX增益档位: 0~31(-47dB~0dB, step:1.5dB)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_analog_aux_set_gain(u8 ch, u8 gain);

/*
*********************************************************************
*                  Audio AUX Get Gain
* Description: 设置第二级/后级增益
* Arguments  : ch    AUX通道选择: AUX_CH0/1
* Return	 : gain AUX增益档位: 0~31(-47dB~0dB, step:1.5dB)
* Note(s)    : None.
*********************************************************************
*/
int audio_analog_aux_get_gain(u8 ch);

#endif
