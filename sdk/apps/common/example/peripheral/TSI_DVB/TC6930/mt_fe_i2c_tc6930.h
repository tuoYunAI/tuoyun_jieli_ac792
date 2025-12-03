/*****************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd                                    */
/* MONTAGE PROPRIETARY AND CONFIDENTIAL                                      */
/* Copyright (c) 2016 Montage Technology Group Limited. All Rights Reserved. */
/*****************************************************************************/
/*
 * Filename:        mt_fe_i2c_tc6930.h
 *
 * Description:     2-wire bus function prototype for FE module.
 *
 *
 * Author:          YZ.Huang
 *
 * Version:         1.01.00
 * Date:            2014-12-23

 * History:
 * Description      Version     Date            Author
 *----------------------------------------------------------------------------
 * File Create      1.01.00     2012.02.28      YZ.Huang
 *      Modify      1.03.03     2016.04.14      YZ.Huang
 *----------------------------------------------------------------------------
 *****************************************************************************/

#ifndef __MT_FE_I2C_TC6930_H__
#define __MT_FE_I2C_TC6930_H__

#include "mt_fe_def_tc6930.h"

#ifdef __cplusplus
extern "C" {
#endif

MT_FE_RET _mt_fe_dmd_set_reg_tc6930(void *dev_handle, U8 demod_index, U8 reg_index, U8 data);
MT_FE_RET _mt_fe_dmd_get_reg_tc6930(void *dev_handle, U8 demod_index, U8 reg_index, U8 *p_buf);
MT_FE_RET _mt_fe_dsp_set_reg_tc6930(U8 iIndex, U8 reg_index, U8 data);
MT_FE_RET _mt_fe_dsp_get_reg_tc6930(U8 iIndex, U8 reg_index, U8 *p_buf);
MT_FE_RET _mt_fe_tn_write_tc6930(void *dev_handle, U8 iInde, U8 *p_buf, U16 n_byte);
MT_FE_RET _mt_fe_write32_tc6930(U32 reg_addr, U32 reg_data);
MT_FE_RET _mt_fe_read32_tc6930(U32 reg_addr, U32 *p_data);

S32 _mt_fe_tn_get_reg_16bit_tc6930(U16 reg_addr, U16 *reg_data);
S32 _mt_fe_tn_set_reg_16bit_tc6930(U16 reg_addr, U16 reg_data);

S32 _mt_fe_tn_set_reg_16bit_tc6930_bits(U16 uiAddr, U16 uiData, U8 ucBitHigh, U8 ucBitLow);


S32 _mt_fe_tc6930_get_reg_32bit(U32 reg_addr, U32 *reg_data);
S32 _mt_fe_tc6930_set_reg_32bit(U32 reg_addr, U32 reg_data);

S32 _mt_fe_tc6930_set_reg_32bit_bits(U32 ulAddr, U32 ulData, U8 ucBitHigh, U8 ucBitLow);



void _mt_sleep_tc6930(U32 ticks_ms);

#ifdef __cplusplus
};
#endif

#endif /* __MT_FE_I2C_H__ */

