/*****************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd                                    */
/* MONTAGE PROPRIETARY AND CONFIDENTIAL                                      */
/* Copyright (c) 2020 Montage Technology Group Limited. All Rights Reserved. */
/*****************************************************************************/
/*
 * Filename:		mt_fe_tn_tc6930.c
 *
 * Description:		Montage TC6930 Digital Cable Tuner IC driver.
 *
 *
 * Author:			Roy.xu
 *
 * Version:			0.02.03
 * Date:			2022-08-09

 * History:
 * Description		Version		Date			Author
 *---------------------------------------------------------------------------
 * File Create		0.00.00		2017.06.14		Youzhong.Huang
 * Modify			0.00.01		2017.06.20		Roy.Xu
 * Modify			0.00.02		2017.06.21		Roy.Xu
 * Modify			0.00.03		2017.06.22		Roy.Xu
 * Modify			0.00.04		2017.06.23		Roy.Xu
 * Modify			0.00.05		2017.06.23		Roy.Xu
 * Modify			0.00.06		2017.06.26		Roy.Xu
 * Modify			0.00.07		2017.06.27		Roy.Xu
 * Modify			0.00.08		2017.07.05		Roy.Xu
 * Modify			0.00.09		2017.07.06		Roy.Xu
 * Modify			0.00.10		2017.07.10		Roy.Xu
 * Modify			0.00.11		2017.07.24		Roy.Xu
 * Modify			0.00.12		2017.08.01		Roy.Xu
 * Modify			0.00.13		2017.08.28		Roy.Xu
 * Modify			0.00.14		2017.09.22		Roy.Xu
 * Modify			0.00.15		2017.10.11		Daniel Zhu
 * Modify			0.00.16		2017.11.11		Jerock Qin
 * Modify			0.00.17		2017.11.22		Jerock Qin
 * Modify			0.00.18		2018.01.03		Jerock Qin
 * Modify			0.00.19		2018.01.09		Roy.Xu
 * Modify			0.01.01		2018.01.12		Roy.Xu
 * Modify			0.01.02		2018.01.12		Roy.Xu
 * Modify			0.01.03		2018.01.23		Roy.Xu
 * Modify			0.01.04		2019.02.15		YZ.Huang
 * Modify			0.01.05		2019.03.06		YZ.Huang
 * Modify			0.01.06		2019.11.07		Daniel.Zhu
 * Modify			0.01.07		2019.11.25		Daniel.Zhu
 * Modify			0.01.08		2019.12.05		Daniel.Zhu
 * Modify			0.01.09		2020.04.03		YZ.Huang
 * Modify			0.01.10		2020.08.12		YZ.Huang
 * Modify			0.01.11		2020.08.24		YZ.Huang
 * Modify			0.01.12		2020.09.16		Daniel.Zhu
 * Modify			0.01.13		2020.12.22		Daniel.Zhu
 * Modify			0.02.00		2021.04.07		YZ.Huang
 * Modify			0.02.01		2021.07.29		YZ.Huang
 * Modify			0.02.02		2021.08.04		YZ.Huang
 * Modify			0.02.03		2022.08.05		J.Qin
 *---------------------------------------------------------------------------
 *****************************************************************************/
#include "mt_fe_i2c_tc6930.h"
#include "mt_fe_fw_tn_tc6930.h"



S32 _mt_fe_dsp_set_reg_bit_tc6930(U8 iIndex, U8 reg_addr, U8 data, U8 ucBitHigh, U8 ucBitLow)
{
    U8 tmp = 0, value = 0;

    if ((ucBitHigh > 7) || (ucBitLow > 7)) {
        return -1;
    }

    if (ucBitHigh < ucBitLow) {
        tmp = ucBitHigh;
        ucBitHigh = ucBitLow;
        ucBitLow = tmp;
    }

    data <<= (7 + ucBitLow - ucBitHigh);
    data &= 0xFF;
    data >>= (7 - ucBitHigh);
    data &= 0xFF;

    tmp = 0xFF;
    tmp <<= (7 + ucBitLow - ucBitHigh);
    tmp &= 0xFF;
    tmp >>= (7 - ucBitHigh);
    tmp &= 0xFF;

    _mt_fe_dsp_get_reg_tc6930(iIndex, reg_addr, &value);
    value &= ~tmp;
    value |= data;
    _mt_fe_dsp_set_reg_tc6930(iIndex, reg_addr, value);

    return 0;
}

S32 _mt_fe_tn_set_reg_16bit_tc6930_bits(U16 uiAddr, U16 uiData, U8 ucBitHigh, U8 ucBitLow)
{
    U16 uiTmp = 0, uiVal = 0;
    U8 ucTmp;

    if ((ucBitHigh > 15) || (ucBitLow > 15)) {
        return -1;
    }

    if (ucBitHigh < ucBitLow) {
        ucTmp = ucBitHigh;
        ucBitHigh = ucBitLow;
        ucBitLow = ucTmp;
    }

    uiData <<= (15 + ucBitLow - ucBitHigh);
    uiData &= 0xFFFF;
    uiData >>= (15 - ucBitHigh);
    uiData &= 0xFFFF;

    uiTmp = 0xFFFF;
    uiTmp <<= (15 + ucBitLow - ucBitHigh);
    uiTmp &= 0xFFFF;
    uiTmp >>= (15 - ucBitHigh);
    uiTmp &= 0xFFFF;

    _mt_fe_tn_get_reg_16bit_tc6930(uiAddr, &uiVal);
    uiVal &= ~uiTmp;
    uiVal |= uiData;
    _mt_fe_tn_set_reg_16bit_tc6930(uiAddr, uiVal);

    return 0;
}

void _mt_fe_tn_set_RF_front_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz)
{
    U16 buf;
    U16 buf1, buf2;

    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    if (handle->demod_device[demod_index].tuner_settings.tuner_int_im == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930(0x4700, 0x0156);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4701, 0x1d70);
        buf = 0x0022;
        _mt_fe_tn_set_reg_16bit_tc6930(0x4234, 0x04bf);
    } else if (handle->demod_device[demod_index].tuner_settings.tuner_int_im == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930(0x4700, 0x002e);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4701, 0x1af0);
        buf = 0x0022;
        _mt_fe_tn_set_reg_16bit_tc6930(0x4234, 0x04bf);
    } else {
        _mt_fe_tn_set_reg_16bit_tc6930(0x4700, 0x00ce);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4701, 0x1cf0);

        buf = 0x0055;
        _mt_fe_tn_set_reg_16bit_tc6930(0x4234, 0x0b7f);
    }

    if (((handle->demod_device[demod_index].tuner_settings.tuner_mode == 1) || (handle->demod_device[demod_index].tuner_settings.tuner_mode == 2)) && (handle->demod_device[demod_index].tuner_settings.tuner_int_lt >= 2)) {
        _mt_fe_tn_get_reg_16bit_tc6930(0x4770, &buf);

        buf1 = (buf & 0x0e00) >> 5;
        buf2 = (buf & 0x01c0) >> 6;
        buf = (buf1 + buf2) | 0x0088;

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42be, buf, 11, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4001, 0, 3, 3);
    } else {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42be, buf, 11, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4001, 1, 3, 3);
    }


    if (tuner_index_in == 0) {	// RXA
        if (freq_KHz <= 379000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x475a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4759, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4758, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4757, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4756, 0x4c4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4755, 0x393c);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4754, 0x2629);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4753, 0x1316);
        } else if (freq_KHz <= 562000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x475a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4759, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4758, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4757, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4756, 0x4b4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4755, 0x383b);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4754, 0x2528);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4753, 0x1114);
        } else if (freq_KHz <= 690000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x475a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4759, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4758, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4757, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4756, 0x4b4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4755, 0x373b);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4754, 0x2226);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4753, 0x0011);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930(0x475a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4759, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4758, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4757, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4756, 0x494e);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4755, 0x3438);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4754, 0x1223);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4753, 0x0000);
        }
    } else if (tuner_index_in == 1) {	// RXB
        if (freq_KHz <= 379000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4762, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4761, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4760, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475f, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475e, 0x4c4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475d, 0x393c);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475c, 0x2629);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475b, 0x1316);
        } else if (freq_KHz <= 562000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4762, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4761, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4760, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475f, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475e, 0x4b4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475d, 0x383b);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475c, 0x2528);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475b, 0x1114);
        } else if (freq_KHz <= 690000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4762, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4761, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4760, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475f, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475e, 0x4b4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475d, 0x373b);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475c, 0x2226);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475b, 0x0011);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4762, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4761, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4760, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475f, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475e, 0x494e);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475d, 0x3438);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475c, 0x1223);
            _mt_fe_tn_set_reg_16bit_tc6930(0x475b, 0x0000);
        }
    } else if (tuner_index_in == 2) {	// RXC
        if (freq_KHz <= 379000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x476a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4769, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4768, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4767, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4766, 0x4c4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4765, 0x393c);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4764, 0x2629);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4763, 0x1316);
        } else if (freq_KHz <= 562000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x476a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4769, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4768, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4767, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4766, 0x4b4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4765, 0x383b);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4764, 0x2528);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4763, 0x1114);
        } else if (freq_KHz <= 690000) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x476a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4769, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4768, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4767, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4766, 0x4b4f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4765, 0x373b);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4764, 0x2226);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4763, 0x0011);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930(0x476a, 0xbfcf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4769, 0x9faf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4768, 0x7f8f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4767, 0x5f6f);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4766, 0x494e);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4765, 0x3438);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4764, 0x1223);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4763, 0x0000);
        }
    }

    return;
}



void _mt_fe_tn_set_Mixer_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz, U8 mixer_sel)
{
    U16	buf0 = 0;

    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    if (tuner_index_in == 0) {	// RXA
        if (mixer_sel == 2) {
            buf0 = 2;

            if (freq_KHz < 125000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 12, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x0000, 11, 8);
            } else if (freq_KHz < 167000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0001, 12, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x0000, 11, 8);
            } else if (freq_KHz < 250000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0002, 12, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x0004, 11, 8);
            } else if (freq_KHz < 333000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0003, 12, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x0008, 11, 8);
            } else {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0004, 12, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x000c, 11, 8);
            }
        } else if (mixer_sel == 3) {
            buf0 = 3;
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 12, 10);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x0000, 11, 8);
        } else {
            buf0 = 1;
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 12, 10);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4707, 0x0000, 11, 8);
        }

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4017, buf0, 1, 0);
    } else if (tuner_index_in == 1) {	// RXB
        if (mixer_sel == 2) {
            buf0 = 2;

            if (freq_KHz < 125000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 9, 7);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x0000, 11, 8);
            } else if (freq_KHz < 167000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0001, 9, 7);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x0000, 11, 8);
            } else if (freq_KHz < 250000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0002, 9, 7);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x0004, 11, 8);
            } else if (freq_KHz < 333000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0003, 9, 7);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x0008, 11, 8);
            } else {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0004, 9, 7);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x000c, 11, 8);
            }
        } else if (mixer_sel == 3) {
            buf0 = 3;
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 9, 7);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x0000, 11, 8);
        } else {
            buf0 = 1;
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 9, 7);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470a, 0x0000, 11, 8);
        }

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4018, buf0, 1, 0);
    } else if (tuner_index_in == 2) {	// RXC
        if (mixer_sel == 2) {
            buf0 = 2;

            if (freq_KHz < 125000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 6, 4);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x0000, 11, 8);
            } else if (freq_KHz < 167000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0001, 6, 4);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x0000, 11, 8);
            } else if (freq_KHz < 250000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0002, 6, 4);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x0004, 11, 8);
            } else if (freq_KHz < 333000) {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0003, 6, 4);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x0008, 11, 8);
            } else {
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0004, 6, 4);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x000c, 11, 8);
            }
        } else if (mixer_sel == 3) {
            buf0 = 3;
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 6, 4);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x0000, 11, 8);
        } else {
            buf0 = 1;
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c7, 0x0000, 6, 4);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470d, 0x0000, 11, 8);
        }

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4019, buf0, 1, 0);
    }

    return;
}


void _mt_fe_tn_set_LO_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz, U8 mixer_sel)
{
    U32 LDIV = 0;
    U16	buf0;


    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    if ((mixer_sel == 2) && (freq_KHz < 500000)) {
        if (freq_KHz < 63000) {
            LDIV = 128;
            buf0 = 139;
        } else if (freq_KHz < 83000) {
            LDIV = 96;
            buf0 = 74;
        } else if (freq_KHz < 125000) {
            LDIV = 64;
            buf0 = 10;
        } else if (freq_KHz < 167000) {
            LDIV = 48;
            buf0 = 13;
        } else if (freq_KHz < 250000) {
            LDIV = 32;
            buf0 = 9;
        } else if (freq_KHz < 333000) {
            LDIV = 24;
            buf0 = 12;
        } else {
            LDIV = 16;
            buf0 = 8;
        }
    } else if ((mixer_sel == 3) && (freq_KHz >= 250000)) {
        if (freq_KHz < 333000) {
            LDIV = 24;
            buf0 = 200;
        } else if (freq_KHz < 500000) {
            LDIV = 16;
            buf0 = 136;
        } else if (freq_KHz < 666000) {
            LDIV = 12;
            buf0 = 68;
        } else {
            LDIV = 8;
            buf0 = 4;
        }
    } else {
        if (freq_KHz < 56000) {
            LDIV = 128;
            buf0 = 11;
        } else if (freq_KHz < 83000) {
            LDIV = 96;
            buf0 = 74;
        } else if (freq_KHz < 125000) {
            LDIV = 64;
            buf0 = 10;
        } else if (freq_KHz < 167000) {
            LDIV = 48;
            buf0 = 73;
        } else if (freq_KHz < 250000) {
            LDIV = 32;
            buf0 = 9;
        } else if (freq_KHz < 333000) {
            LDIV = 24;
            buf0 = 72;
        } else if (freq_KHz < 500000) {
            LDIV = 16;
            buf0 = 8;
        } else if (freq_KHz < 667000) {
            LDIV = 12;
            buf0 = 68;
        } else {
            LDIV = 8;
            buf0 = 4;
        }
    }

    handle->demod_device[demod_index].tuner_settings.tuner_int_ldiv = LDIV;
    handle->demod_device[demod_index].tuner_settings.tuner_int_fvco_tg = freq_KHz * LDIV;


    if (((buf0 >> 6) & 0x0001) == 1) {
        if (handle->demod_device[demod_index].tuner_settings.tuner_int_fvco_tg < 6700000) {
            buf0 = buf0 + 48;
        } else if (handle->demod_device[demod_index].tuner_settings.tuner_int_fvco_tg < 7500000) {
            buf0 = buf0 + 32;
        } else if (handle->demod_device[demod_index].tuner_settings.tuner_int_fvco_tg < 8000000) {
            buf0 = buf0 + 16;
        } else {
            buf0 = buf0 + 0;
        }
    } else {
        buf0 = buf0 + 0;
    }



    if (handle->demod_device[demod_index].tuner_settings.tuner_int_fvco_tg < 6700000) {
        buf0 = buf0 + 0x2400;
    } else {
        buf0 = buf0 + 0x3c00;
    }

    if (tuner_index_in == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930(0x473b, buf0);
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930(0x473d, buf0);
    } else if (tuner_index_in == 2) {
        _mt_fe_tn_set_reg_16bit_tc6930(0x473f, buf0);
    }

    return;
}


void _mt_fe_tn_set_PLL_freq_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 Delta_sign, U8 Num_step)
{
    U32	LDIV = 0x00;
    U32 Fvco_KHz = 0x00;
    U32 Fvco_in_KHz = 0x00;
    U32	pll_ref = 48000;				//KHz
    U8	pll_fdiv_bit_sel = 4;			// value list: 0/1/2/3/4;
    U32 buf32 = 0x00;
    U32 NS4 = 0x00;
    U32 fin = 0x00;

    U8	pll_sdm_en = 1;
    U8  XTAL_DF_enb = 0;
    U8  spur_opti_by_dsm = 0;
    U32	FDIV_F = 0x00;
    U32 FDIV_N = 0x00;
    U32 FDIV_FIN = 0x00;

    U8	ICP_CODE = 5;
    U8	kvco_tune = 0;
    U8	buf = 0;
    U32 freq_LO_KHz = 0;
    U32 harmK = 1;
    U8 Icp_scale_en = 0;

    U8 PFD_SET;

    U8 R2 = 0;
    U8 R3 = 0;
    U8 RDIV = 0;

    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;


    LDIV = handle->demod_device[demod_index].tuner_settings.tuner_int_ldiv;
    Fvco_in_KHz = handle->demod_device[demod_index].tuner_settings.tuner_int_fvco_tg;

    freq_LO_KHz = Fvco_in_KHz / LDIV;
    pll_ref = 1 * handle->demod_device[demod_index].tuner_settings.tuner_crystal / 1;
    harmK = (U32)(((Fvco_in_KHz / (handle->demod_device[demod_index].tuner_settings.tuner_crystal >> 1)) & (0x01)) + (Fvco_in_KHz / handle->demod_device[demod_index].tuner_settings.tuner_crystal));
    Icp_scale_en = 0;

    if ((pll_ref == 27000) || (pll_ref == 9000)) {
        NS4 = (U32)(2 * Fvco_in_KHz / (pll_ref >> 3));
        fin = (U32)(4096 * (2 * Fvco_in_KHz - (pll_ref >> 3) * NS4) / (pll_ref >> 3));
    } else if (pll_ref == 13500) {
        NS4 = (U32)(4 * Fvco_in_KHz / (pll_ref >> 2));
        fin = (U32)(4096 * (4 * Fvco_in_KHz - (pll_ref >> 2) * NS4) / (pll_ref >> 2));
    } else {
        NS4 = (U32)(Fvco_in_KHz / (pll_ref >> 4));
        fin = (U32)(4096 * (Fvco_in_KHz - (pll_ref >> 4) * NS4) / (pll_ref >> 4));
    }

    FDIV_N = (NS4 >> 4) & 0x3ff;
    FDIV_F = ((NS4 >> (4 - pll_fdiv_bit_sel)) << (4 - pll_fdiv_bit_sel)) & (0x0f);
    FDIV_FIN = (((((NS4 & 0x0f) << 12) | fin) >> (4 - pll_fdiv_bit_sel)) &  0x0fff);

    if ((Num_step == 0) && ((FDIV_FIN <= 32) || (FDIV_FIN >= (4096 - 32)))) {
        pll_sdm_en = 0;
    }

    if (pll_sdm_en == 0) {
        if (fin & 0x800) {
            NS4  = NS4 + 0x01;
        }

        FDIV_N = (NS4 >> 4) & 0x3ff;
        FDIV_F = ((NS4 >> (4 - pll_fdiv_bit_sel)) << (4 - pll_fdiv_bit_sel)) & 0x0f;
        FDIV_FIN = 0x00;
    }

    if (spur_opti_by_dsm && (pll_sdm_en == 0)) {
        if ((((FDIV_N & 0x03) == 0x01) && (FDIV_F == 0x01)) || (((FDIV_N & 0x03) == 0x03) && (FDIV_F == 0x0f))) {
            pll_sdm_en = 1;
            FDIV_FIN = 0x03;
        }
    }

    Fvco_KHz = FDIV_N * pll_ref + ((FDIV_F * pll_ref) >> 4) + ((FDIV_FIN * pll_ref) >> 16);

    //handle->demod_device[demod_index].tuner_settings.tuner_Fvco_val = 1.0 * Fvco_KHz / 1000;		//output, MHz

    if (8000000 > Fvco_KHz) {
        buf32 = 4 * 64 + ((8 - 4) * (8000000 - Fvco_KHz) / (2600000 / 64));
    } else {
        buf32 = 4 * 64 - ((8 - 4) * (Fvco_KHz - 8000000) / (2600000 / 64));
    }

    ICP_CODE = (U8)((buf32 >> 6) + ((buf32 >> 5) & 0x01));

    if (pll_ref == handle->demod_device[demod_index].tuner_settings.tuner_crystal) {
        ICP_CODE = (U8)((buf32 >> 5) + ((buf32 >> 4) & 0x01));
        XTAL_DF_enb = 1;
        RDIV = 0x01;
        R2 = 0x02;
        R3 = 0x03;
    } else if (pll_ref == (2 * handle->demod_device[demod_index].tuner_settings.tuner_crystal / 3)) {
        ICP_CODE = (U8)((buf32 >> 5) + ((buf32 >> 4) & 0x01)) + (U8)((buf32 >> 6) + ((buf32 >> 5) & 0x01));
        XTAL_DF_enb = 0;
        RDIV = 0x03;
        R2 = 0x08;
        R3 = 0x03;
    } else if (pll_ref == (1 * handle->demod_device[demod_index].tuner_settings.tuner_crystal / 3)) {
        ICP_CODE = (U8)((buf32 >> 4) + ((buf32 >> 3) & 0x01)) + (U8)((buf32 >> 5) + ((buf32 >> 4) & 0x01));
        XTAL_DF_enb = 1;
        RDIV = 0x03;
        R2 = 0x0C;
        R3 = 0x03;
    } else if (pll_ref == (1 * handle->demod_device[demod_index].tuner_settings.tuner_crystal / 2)) {
        ICP_CODE = (U8)((buf32 >> 4) + ((buf32 >> 3) & 0x01));
        XTAL_DF_enb = 1;
        RDIV = 0x02;
        R2 = 0x02;
        R3 = 0x03;
    } else { // Fref = 2xFxtal (48M)
        ICP_CODE = (U8)((buf32 >> 6) + ((buf32 >> 5) & 0x01));
        XTAL_DF_enb = 0;
        RDIV = 0x01;
        R2 = 0x02;
        R3 = 0x03;
    }

    if (pll_sdm_en) {
        ICP_CODE = ICP_CODE - 1;
        PFD_SET = 0x21;
    } else {
        ICP_CODE = ICP_CODE - 0;
        PFD_SET = 0x00;
    }

    if (Icp_scale_en == 1) {
        ICP_CODE = (ICP_CODE >> 1) & 0xff;
    } else if (Icp_scale_en == 2) {
        ICP_CODE = (ICP_CODE >> 2) & 0xff;
    }
#if 0
    else {
        ICP_CODE = ICP_CODE;
    }
#endif

    if (ICP_CODE >= 15) {
        ICP_CODE = 15;
    }

    if (ICP_CODE <= 2) {
        ICP_CODE = 2;
    }

    if (kvco_tune == 1) {
        ICP_CODE = (ICP_CODE >> 2) & 0xff;
    }


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4023, XTAL_DF_enb, 6, 6);

    if (tuner_index_in == 0) {
        buf = R3 & 0x01;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471a, buf, 15, 15);
        buf = R3 >> 1;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4719, buf, 1, 0);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471a, ICP_CODE, 3, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471a, 0x03, 11, 10);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471C, 0x02, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4719, RDIV, 11, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471a, R2, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471b, 0x17, 9, 5);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471c, kvco_tune, 13, 13);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4717, (U16)FDIV_N, 9, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4718, (U16)FDIV_F, 15, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4718, (U16)FDIV_FIN, 11, 0);
        buf = 7 - pll_fdiv_bit_sel;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4717, buf, 14, 12);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471b, 0, 12, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471d, 0X02, 1, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471d, 0X03, 1, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4040, 0, 10, 10);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4041, 0X01, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4041, 0X03, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4040, 1, 10, 10);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471b, pll_sdm_en, 12, 12);
    } else if (tuner_index_in == 1) {
        buf = R3 & 0x01;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4721, buf, 15, 15);
        buf = R3 >> 1;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4720, buf, 1, 0);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4721, ICP_CODE, 3, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4721, 0x03, 11, 10);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4723, 0x02, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4720, RDIV, 11, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4721, R2, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4722, 0x17, 9, 5);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4723, kvco_tune, 13, 13);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471e, (U16)FDIV_N, 9, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471f, (U16)FDIV_F, 15, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471f, (U16)FDIV_FIN, 11, 0);
        buf = 7 - pll_fdiv_bit_sel;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x471e, buf, 14, 12);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4722, 0, 12, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4724, 0X02, 1, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4724, 0X03, 1, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4040, 0, 9, 9);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4046, 0X01, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4046, 0X03, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4040, 1, 9, 9);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4722, pll_sdm_en, 12, 12);
    } else if (tuner_index_in == 2) {
        buf = R3 & 0x01;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4728, buf, 15, 15);
        buf = R3 >> 1;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4727, buf, 1, 0);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4728, ICP_CODE, 3, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4728, 0x03, 11, 10);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472a, 0x02, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4727, RDIV, 11, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4728, R2, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4729, 0x17, 9, 5);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472a, kvco_tune, 13, 13);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4725, (U16)FDIV_N, 9, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4726, (U16)FDIV_F, 15, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4726, (U16)FDIV_FIN, 11, 0);
        buf = 7 - pll_fdiv_bit_sel;
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4725, buf, 14, 12);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4729, 0, 12, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472b, 0X02, 1, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472b, 0X03, 1, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4040, 0, 8, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x404b, 0X01, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x404b, 0X03, 3, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4040, 1, 8, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4729, pll_sdm_en, 12, 12);
    }

    return;
}


/*****************************************************************************/
/*Function: 	_mt_fe_tn_freq_fine_tune_tc6930  							 */
/*****************************************************************************/
void _mt_fe_tn_freq_fine_tune_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 Delta_sign, U8 Num_step)
{
    _mt_fe_tn_set_PLL_freq_tc6930(handle, demod_index, Delta_sign, Num_step);

    return;
}


void _mt_fe_tn_set_BB_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U32  bandwidth = 0;

    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    bandwidth = handle->demod_device[demod_index].tuner_settings.tuner_bandwidth;

    if (tuner_index_in == 0) {	// RXA
        if (bandwidth == 6000) {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000a, 11, 8);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472e, 0x000a, 7, 4);
        } else if (bandwidth == 7000) {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000b, 11, 8);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472e, 0x0008, 7, 4);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000c, 11, 8);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472e, 0x0008, 7, 4);
        }
    } else if (tuner_index_in == 1) { // RXB
        if (bandwidth == 6000) {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000a, 7, 4);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4731, 0x000a, 7, 4);
        } else if (bandwidth == 7000) {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000b, 7, 4);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4731, 0x0008, 7, 4);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000c, 7, 4);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4731, 0x0008, 7, 4);
        }
    } else if (tuner_index_in == 2) { // RXC
        if (bandwidth == 6000) {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000a, 3, 0);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4734, 0x000a, 7, 4);
        } else if (bandwidth == 7000) {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000b, 3, 0);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4734, 0x0008, 7, 4);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4036, 0x000c, 3, 0);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4734, 0x0008, 7, 4);
        }
    }


    return;
}


void _mt_fe_tn_set_BB_Cal_tc6930(MT_FE_TC6930_Device_Handle handle, U32 bandwidth)
{
    return;
}


void _mt_fe_tn_set_DAC_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz)
{
    U32		DACFreq_KHz = handle->demod_device[demod_index].tuner_settings.tuner_dac;
    U32		fc = 0, fadc = 0, fsd = 0, f2d = 0;
    U8		tempnumber = 0;
    U16		flt_bit = 0, gain_bit = 0, R_4745 = 0;
    U32		FreqTrue108_Hz = 0;
    S32		f1 = 0, f2 = 0, delta1 = 0, Totalnum1 = 0;
    S32		cntT = 0, cntin = 0, NCOI = 0, z0 = 0, z1 = 0, z2 = 0, tmp = 0;
    S32		f1f2number = 0;

    if (demod_index != 2) {
        return;
    }

    if (DACFreq_KHz <= 4500) {
        flt_bit = 0x0001;
    } else {
        flt_bit = 0x0002;
    }

    gain_bit = handle->demod_device[demod_index].tuner_settings.tuner_dac_gain;
    R_4745 = flt_bit + (gain_bit << 8) + 0x08a8;
    _mt_fe_tn_set_reg_16bit_tc6930(0x4745, R_4745);

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4746, 0x0003, 1, 0);

    FreqTrue108_Hz = 115200000;

    _mt_sleep_tc6930(2);

    f1f2number = ((DACFreq_KHz * 1000 * 512) / FreqTrue108_Hz) * 128
                 + (((DACFreq_KHz * 1000 * 512) % FreqTrue108_Hz) * 32 / FreqTrue108_Hz) * 4;

    _mt_fe_dsp_set_reg_tc6930(2, 0xfa, (U8)((f1f2number & 0xff00) >> 8));
    _mt_fe_dsp_set_reg_tc6930(2, 0xfb, (U8)((f1f2number & 0x00ff)));

    f1 = 4096;
    fc = FreqTrue108_Hz;
    fadc = fc / 4;
    fsd = 27000000;


    f2d = handle->demod_device[demod_index].tuner_settings.tuner_bandwidth / 2 - 100;

    f2 = (fsd / 250) * f2d / ((fc + 500) / 1000);
    delta1 = ((f1 - f2) << 15) / f2;
    Totalnum1 = ((f1 - f2) << 15) - delta1 * f2;
    cntT = f2;
    cntin = Totalnum1;
    NCOI = delta1;
    z0 = cntin;
    z1 = cntT;
    z2 = NCOI;

    tempnumber = (U8)((z0 & 0xff00) >> 8);
    _mt_fe_dsp_set_reg_tc6930(2, 0xe0, (U8)(tempnumber & 0x0f));
    tempnumber = (U8)(z0 & 0xff);
    _mt_fe_dsp_set_reg_tc6930(2, 0xe1, tempnumber);
    tempnumber = (U8)((z1 & 0xff00) >> 8);
    _mt_fe_dsp_set_reg_tc6930(2, 0xe2, tempnumber);
    tempnumber = (U8)(z1 & 0xff);
    _mt_fe_dsp_set_reg_tc6930(2, 0xe3, tempnumber);
    tempnumber = (U8)((z2 & 0xff00) >> 8);
    _mt_fe_dsp_set_reg_tc6930(2, 0xe4, tempnumber);
    tempnumber = (U8)(z2 & 0xff);
    _mt_fe_dsp_set_reg_tc6930(2, 0xe5, tempnumber);

    tmp = f1;
    f1 = f2;
    f2 = tmp / 2;
    delta1 = ((f1 - f2) << 15) / f2;
    Totalnum1 = ((f1 - f2) << 15) - delta1 * f2;
    NCOI = (f1 << 15) / f2 - (1 << 15);
    cntT = f2;
    cntin = Totalnum1;
    z0 = cntin;
    z1 = cntT;
    z2 = NCOI;

    tempnumber = (U8)((z0 & 0xff00) >> 8);
    _mt_fe_dsp_set_reg_tc6930(2, 0xef, (U8)(tempnumber & 0x0f));
    tempnumber = (U8)(z0 & 0xff);
    _mt_fe_dsp_set_reg_tc6930(2, 0xf0, tempnumber);
    tempnumber = (U8)((z1 & 0xff00) >> 8);
    _mt_fe_dsp_set_reg_tc6930(2, 0xf1, tempnumber);
    tempnumber = (U8)(z1 & 0xff);
    _mt_fe_dsp_set_reg_tc6930(2, 0xf2, tempnumber);
    tempnumber = (U8)((z2 & 0xff00) >> 8);
    _mt_fe_dsp_set_reg_tc6930(2, 0xf3, tempnumber);
    tempnumber = (U8)(z2 & 0xff);
    _mt_fe_dsp_set_reg_tc6930(2, 0xf4, tempnumber);

    return;
}



void _mt_fe_tn_preset_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    //U16 buf;

    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    if ((handle->demod_device[demod_index].tuner_settings.tuner_mode == 1) || (handle->demod_device[demod_index].tuner_settings.tuner_mode == 2)) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0x07f0, 10, 0);
    } else {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0x06f0, 10, 0);
    }

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4017, 0x03, 7, 6);

    //_mt_fe_tn_get_reg_16bit_tc6930(0x4081, &buf);
    //buf = buf & 0x000f;
    //_mt_fe_tn_set_reg_16bit_tc6930_bits(0x4081, buf, 7, 0);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4081, 0x0c, 7, 0);

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x408d, 0x3f, 14, 8);


    if (tuner_index_in == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4017, 0x3d, 5, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4090, 0x0f, 12, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4750, 0x00, 5, 5);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4750, 0x00, 2, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4751, 0x00, 11, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4752, 0x0f, 11, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4092, 0x0f, 10, 5);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4100, 0x0079);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4104, 0x00, 5, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4107, 0x00, 12, 6);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4106, 0x5a, 7, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4038, 0x00, 13, 13);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x423a, 0x00, 8, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4050, 0x00, 14, 7);
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4018, 0x3d, 5, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4095, 0x0f, 12, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4750, 0x00, 4, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4750, 0x00, 1, 1);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4751, 0x00, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4752, 0x0f, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4097, 0x0f, 10, 5);
        _mt_fe_tn_set_reg_16bit_tc6930(0x411c, 0x0079);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4120, 0x00, 5, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4123, 0x00, 12, 6);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4122, 0x5a, 7, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4038, 0x00, 6, 6);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x427a, 0x00, 8, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x405d, 0x00, 14, 7);
    } else if (tuner_index_in == 2) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4019, 0x3d, 5, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x409a, 0x0f, 12, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4750, 0x00, 3, 3);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4750, 0x00, 0, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4751, 0x00, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4752, 0x0f, 7, 4);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x409c, 0x0f, 10, 5);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4138, 0x0079);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x413c, 0x00, 5, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x413f, 0x00, 12, 6);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x413e, 0x5a, 7, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4039, 0x00, 6, 6);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42ba, 0x00, 8, 8);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x406a, 0x00, 14, 7);
    }


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4032, 0, 7, 6);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4025, 0x4480);

    // dco

    return;
}

void _mt_fe_tn_run_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U16 buf = 0;

    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    if (((handle->demod_device[demod_index].tuner_settings.tuner_mode == 1) || (handle->demod_device[demod_index].tuner_settings.tuner_mode == 2)) && (handle->demod_device[demod_index].tuner_settings.tuner_int_lt >= 2)) {
        _mt_fe_tn_get_reg_16bit_tc6930(0x4081, &buf);
        buf = buf | 0x0010;
        _mt_fe_tn_set_reg_16bit_tc6930(0x4081, buf);
    }


    if (tuner_index_in == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4006, 0x01, 4, 3);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4007, 0x80, 7, 0);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4008, 0x30ff);
        _mt_fe_tn_set_reg_16bit_tc6930(0x400d, 0x0000);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4012, 0x0000);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4160, 0xa234);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4003, 0x01, 2, 2);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4003, 0x00, 2, 2);
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x400b, 0x01, 4, 3);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x400c, 0x80, 7, 0);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4008, 0x0c00);
        _mt_fe_tn_set_reg_16bit_tc6930(0x400d, 0x00ff);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4012, 0x0000);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4163, 0xa234);


        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4003, 0x01, 1, 1);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4003, 0x00, 1, 1);
    } else if (tuner_index_in == 2) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4010, 0x01, 4, 3);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4011, 0x80, 7, 0);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4008, 0x0300);
        _mt_fe_tn_set_reg_16bit_tc6930(0x400d, 0x0000);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4012, 0x00ff);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4166, 0xa234);

        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4003, 0x01, 0, 0);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4003, 0x00, 0, 0);
    }

    _mt_sleep_tc6930(10);

    if (((handle->demod_device[demod_index].tuner_settings.tuner_mode == 1) || (handle->demod_device[demod_index].tuner_settings.tuner_mode == 2)) && (handle->demod_device[demod_index].tuner_settings.tuner_int_lt >= 2)) {
        buf = buf & 0xffef;
        _mt_fe_tn_set_reg_16bit_tc6930(0x4081, buf);
    }

    _mt_fe_tn_get_reg_16bit_tc6930(0x4016, &buf);
    buf = (buf & 0x7000) >> 12;
    if ((buf == 0x0001) || (buf == 0x0002) || (buf == 0x0004)) {	// only one rx is on
        _mt_fe_tc6930_set_reg_32bit_bits(0x7c04, 0x0001, 3, 3);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4602, 0x0001);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4602, 0x0000);

        _mt_fe_tc6930_set_reg_32bit_bits(0x7c04, 0x0000, 3, 3);
    }

    _mt_sleep_tc6930(20);

    if (((handle->demod_device[demod_index].tuner_settings.tuner_mode == 1) || (handle->demod_device[demod_index].tuner_settings.tuner_mode == 2)) && (handle->demod_device[demod_index].tuner_settings.tuner_int_lt == 2)) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4001, 1, 3, 3);
        _mt_fe_tn_get_reg_16bit_tc6930(0x42be, &buf);
        buf = buf & 0xf77f;
        _mt_fe_tn_set_reg_16bit_tc6930(0x42be, buf);
    }

    // ---------- LT Mode control
    if (handle->demod_device[demod_index].tuner_settings.tuner_mode == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 10, 10);	// lt_sleepb_bp=0
    } else if (handle->demod_device[demod_index].tuner_settings.tuner_mode == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 10, 10);	// lt_sleepb_bp=0
    } else if (handle->demod_device[demod_index].tuner_settings.tuner_mode == 2) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 1, 10, 10);	// lt_sleepb_bp=1
    } else {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 10, 10);	// lt_sleepb_bp=0
    }

    // dco timer
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4050, 0x0049, 14, 7);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x405d, 0x0049, 14, 7);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x406a, 0x0049, 14, 7);

    // bb timer
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4100, 0x0003, 2, 0);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x411c, 0x0003, 2, 0);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4138, 0x0003, 2, 0);

    return;
}


void _mt_fe_tn_set_tune_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz, U8 mixer_sel, U32 WaitTime)
{
    _mt_fe_tn_preset_tc6930(handle, demod_index);

    _mt_fe_tn_set_RF_front_tc6930(handle, demod_index, freq_KHz);

    _mt_fe_tn_set_Mixer_tc6930(handle, demod_index, freq_KHz, mixer_sel);

    _mt_fe_tn_set_LO_tc6930(handle, demod_index, freq_KHz, mixer_sel);

    _mt_fe_tn_set_PLL_freq_tc6930(handle, demod_index, 0, 0);

    _mt_fe_tn_set_BB_tc6930(handle, demod_index);

    _mt_fe_tn_set_DAC_tc6930(handle, demod_index, freq_KHz);

    _mt_fe_tn_run_tc6930(handle, demod_index);

    _mt_sleep_tc6930(WaitTime);

    return;
}

void _mt_fe_tn_set_sysmon_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    return;
}

void _mt_fe_tn_ADC_calibration_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U16	data1_a = 0, data2_a = 0, data3_a = 0;
    U16	data1_b = 0, data2_b = 0, data3_b = 0;
    U16	data1_c = 0, data2_c = 0, data3_c = 0;
    U8	adc_cal_suc_a = 0, adc_cal_suc_b = 0, adc_cal_suc_c = 0;
    U8	i;
    U16	buf;

    U8	b_cal_a = 0, b_cal_b = 0, b_cal_c = 0;

    // RC6800, demod 0, tuner 1
    // TC6920, demod 0 + 2, tuner 1, 2
    // TC6930, demod 0 + 1, tuner 1, 2
    // TC6960, RC6900, all 3

    // demod 0
    switch (handle->demod_device[0].tuner_settings.tuner_index) {
    case 1:
        b_cal_b = 1;
        break;
    case 2:
        b_cal_c = 1;
        break;
    case 0:
    default:
        b_cal_a = 1;
        break;
    }

    if ((handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6930) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6960) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_RC6900)) {
        // demod 1, tuner 2
        switch (handle->demod_device[1].tuner_settings.tuner_index) {
        case 1:
            b_cal_b = 1;
            break;
        case 2:
            b_cal_c = 1;
            break;
        case 0:
        default:
            b_cal_a = 1;
            break;
        }
    }

    if ((handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6920) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6960) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_RC6900)) {
        // demod 2, tuner 0
        switch (handle->demod_device[2].tuner_settings.tuner_index) {
        case 1:
            b_cal_b = 1;
            break;
        case 2:
            b_cal_c = 1;
            break;
        case 0:
        default:
            b_cal_a = 1;
            break;
        }
    }

#if TC6930_ENABLE_PRINT
    tc6930_dbg_print(("%s[%d] ---- demod_type[%d], tuner_list[%d, %d, %d], cal_status[%d, %d, %d]\n", __FUNCTION__, __LINE__,
                      handle->demod_device[0].demod_type,
                      handle->demod_device[0].tuner_settings.tuner_index,
                      handle->demod_device[1].tuner_settings.tuner_index,
                      handle->demod_device[2].tuner_settings.tuner_index,
                      b_cal_a, b_cal_b, b_cal_c));
#endif

    _mt_fe_tn_set_tune_tc6930(handle, demod_index, 997375, 1, 100);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4741, 0x4d4d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4740, 0x014d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4743, 0x924a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4742, 0x0a24);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4744, 0x75bc);

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473b, 0x7b, 7, 0);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473d, 0x7b, 7, 0);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473f, 0x7b, 7, 0);

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473a, 0x01, 9, 9);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473c, 0x01, 9, 9);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473e, 0x01, 9, 9);


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4107, 0x01, 10, 10);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4106, 0x03, 7, 6);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4123, 0x01, 10, 10);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4122, 0x03, 7, 6);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x413f, 0x01, 10, 10);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x413e, 0x03, 7, 6);


    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x000a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x000d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x000a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x000d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x000a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x000d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0000);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0014);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0040);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0014);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0040);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0014);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0040);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0003);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x005a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0004);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x00c4);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0003);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x005a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0004);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x00c4);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0003);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x005a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0004);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x00c4);


    // power switch for necessary tuner(s)
    // TODO
    adc_cal_suc_a = 1;
    adc_cal_suc_b = 1;
    adc_cal_suc_c = 1;

    i = 0;
    do {
        i++;

        if ((adc_cal_suc_a == 1) && (b_cal_a == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0001);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x00ff);
        }

        if ((adc_cal_suc_b == 1) && (b_cal_b == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0001);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x00ff);
        }

        if ((adc_cal_suc_c == 1) && (b_cal_c == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0001);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x00ff);
        }

        _mt_sleep_tc6930(2);

        if ((adc_cal_suc_a == 1) && (b_cal_a == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0002);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4305, 0x01, 6, 6);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4302, 0x0000);
            _mt_fe_tn_get_reg_16bit_tc6930(0x4302, &buf);
        }

        if ((adc_cal_suc_b == 1) && (b_cal_b == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0002);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4405, 0x01, 6, 6);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4402, 0x0000);
            _mt_fe_tn_get_reg_16bit_tc6930(0x4402, &buf);
        }

        if ((adc_cal_suc_c == 1) && (b_cal_c == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0000);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0002);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4505, 0x01, 6, 6);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4502, 0x0000);
            _mt_fe_tn_get_reg_16bit_tc6930(0x4502, &buf);
        }

        _mt_sleep_tc6930(2);

        if ((adc_cal_suc_a == 1) && (b_cal_a == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0002);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4305, 0x00, 6, 6);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4302, 0x0000);
            _mt_fe_tn_get_reg_16bit_tc6930(0x4302, &buf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0001);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x003a);
        }

        if ((adc_cal_suc_b == 1) && (b_cal_b == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0002);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4405, 0x00, 6, 6);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4402, 0x0000);
            _mt_fe_tn_get_reg_16bit_tc6930(0x4402, &buf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0001);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x003a);
        }

        if ((adc_cal_suc_c == 1) && (b_cal_c == 1)) {
            _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0002);
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4505, 0x00, 6, 6);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4502, 0x0000);
            _mt_fe_tn_get_reg_16bit_tc6930(0x4502, &buf);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0001);
            _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x003a);
        }


        _mt_sleep_tc6930(20);

        _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0006);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4305, &data1_a);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4302, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4302, &buf);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0006);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4405, &data1_b);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4402, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4402, &buf);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0006);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4505, &data1_c);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4502, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4502, &buf);

        _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0008);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4305, &data2_a);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4302, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4302, &buf);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0008);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4405, &data2_b);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4402, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4402, &buf);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0008);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4505, &data2_c);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4502, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4502, &buf);

        _mt_fe_tn_get_reg_16bit_tc6930(0x4301, &data3_a);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4302, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4302, &buf);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4401, &data3_b);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4402, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4402, &buf);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4501, &data3_c);
        _mt_fe_tn_set_reg_16bit_tc6930(0x4502, 0x0000);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4502, &buf);

        adc_cal_suc_a = 0;
        adc_cal_suc_b = 0;
        adc_cal_suc_c = 0;

        if (((((U8)data1_a) != 0x2d) && (((U8)data1_a) != 0x2e) && (((U8)data1_a) != 0x2c)) ||
            ((((U8)data2_a) != 0x2d) && (((U8)data2_a) != 0x2e) && (((U8)data2_a) != 0x2c)) ||
            ((((U8)data3_a) != 0xc5) && (((U8)data3_a) != 0xc6) && (((U8)data3_a) != 0xc7) && (((U8)data3_a) != 0xc8))) {
            adc_cal_suc_a = 1;
        }

        if (((((U8)data1_b) != 0x2d) && (((U8)data1_b) != 0x2e) && (((U8)data1_b) != 0x2c)) ||
            ((((U8)data2_b) != 0x2d) && (((U8)data2_b) != 0x2e) && (((U8)data2_b) != 0x2c)) ||
            ((((U8)data3_b) != 0xc5) && (((U8)data3_b) != 0xc6) && (((U8)data3_b) != 0xc7) && (((U8)data3_b) != 0xc8))) {
            adc_cal_suc_b = 1;
        }

        if (((((U8)data1_c) != 0x2d) && (((U8)data1_c) != 0x2e) && (((U8)data1_c) != 0x2c)) ||
            ((((U8)data2_c) != 0x2d) && (((U8)data2_c) != 0x2e) && (((U8)data2_c) != 0x2c)) ||
            ((((U8)data3_c) != 0xc5) && (((U8)data3_c) != 0xc6) && (((U8)data3_c) != 0xc7) && (((U8)data3_c) != 0xc8))) {
            adc_cal_suc_c = 1;
        }
    } while (((adc_cal_suc_a == 1) || (adc_cal_suc_b == 1) || (adc_cal_suc_c == 1)) && (i < 20));

    _mt_fe_tn_set_reg_16bit_tc6930(0x4741, 0x0909);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4740, 0x0109);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4743, 0x9242);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4742, 0x0024);

    _mt_fe_tn_set_reg_16bit_tc6930(0x473b, 0x2438);
    _mt_fe_tn_set_reg_16bit_tc6930(0x473d, 0x2438);
    _mt_fe_tn_set_reg_16bit_tc6930(0x473f, 0x2438);

    _mt_fe_tn_set_reg_16bit_tc6930(0x473a, 0x000e);
    _mt_fe_tn_set_reg_16bit_tc6930(0x473c, 0x000e);
    _mt_fe_tn_set_reg_16bit_tc6930(0x473e, 0x000e);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4107, 0x0088);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4106, 0x2f5a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4123, 0x0088);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4122, 0x2f5a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x413f, 0x0088);
    _mt_fe_tn_set_reg_16bit_tc6930(0x413e, 0x2f5a);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x000a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0080);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x000d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0080);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0014);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0040);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0003);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4304, 0x0004);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4305, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x000a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0080);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x000d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0080);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0014);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0040);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0003);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4404, 0x0004);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4405, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x000a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0080);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x000d);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0080);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0014);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0040);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0003);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0000);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4504, 0x0004);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4505, 0x0000);
}

void _mt_fe_tn_poweron_set_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U8	i = 0;

#if TC6930_ENABLE_PRINT
    tc6930_dbg_print(("TC6930 power on.\n"));
#endif

    // wakeup necessary tuners
    // TODO

    // RC6800, demod 0, tuner 1
    // TC6920, demod 0 + 2, tuner 1, 2
    // TC6930, demod 0 + 1, tuner 1, 2
    // TC6960, RC6900, all 3


    mt_fe_tn_wakeup_tc6930(handle, 0);

#if TC6930_ENABLE_PRINT
    tc6930_dbg_print(("%s[%d] ---- wakeup demod[0], tuner[%d]\n", __FUNCTION__, __LINE__,
                      handle->demod_device[0].tuner_settings.tuner_index));
#endif

    if ((handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6930) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6960) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_RC6900)) {
        mt_fe_tn_wakeup_tc6930(handle, 1);

#if TC6930_ENABLE_PRINT
        tc6930_dbg_print(("%s[%d] ---- wakeup demod[1], tuner[%d]\n", __FUNCTION__, __LINE__,
                          handle->demod_device[1].tuner_settings.tuner_index));
#endif
    }

    if ((handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6920) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6960) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_RC6900)) {
        mt_fe_tn_wakeup_tc6930(handle, 2);

#if TC6930_ENABLE_PRINT
        tc6930_dbg_print(("%s[%d] ---- wakeup demod[2], tuner[%d]\n", __FUNCTION__, __LINE__,
                          handle->demod_device[2].tuner_settings.tuner_index));
#endif
    }

    _mt_fe_tn_set_reg_16bit_tc6930(0x4744, 0x75bc);

    if (handle->demod_device[demod_index].tuner_settings.tuner_clock_out == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4020, 0x01, 5, 5);
    } else {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4020, 0x00, 5, 5);
    }

    _mt_fe_tn_set_reg_16bit_tc6930(0x4051, 0x0ff1);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4057, 0x0c00);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4058, 0x1af8);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4059, 0x0220);

    _mt_fe_tn_set_reg_16bit_tc6930(0x405e, 0x0ff1);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4064, 0x0c00);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4065, 0x1af8);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4066, 0x0220);

    _mt_fe_tn_set_reg_16bit_tc6930(0x406b, 0x0ff1);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4071, 0x0c00);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4072, 0x1af8);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4073, 0x0220);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4101, 0x2ab2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4102, 0x546e);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4103, 0x2ab2);

    _mt_fe_tn_set_reg_16bit_tc6930(0x411d, 0x2ab2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x411e, 0x546e);
    _mt_fe_tn_set_reg_16bit_tc6930(0x411f, 0x2ab2);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4139, 0x2ab2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x413a, 0x546e);
    _mt_fe_tn_set_reg_16bit_tc6930(0x413b, 0x2ab2);

    _mt_fe_tn_set_reg_16bit_tc6930(0x42bd, 0x0924);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42be, 0x03, 15, 12);
    _mt_fe_tn_set_reg_16bit_tc6930(0x42c5, 0x2b24);
    _mt_fe_tn_set_reg_16bit_tc6930(0x42c8, 0x006a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4704, 0x0006);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x472e, 0, 8, 8);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4731, 0, 8, 8);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4734, 0, 8, 8);

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4081, 0xb5, 15, 8);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4091, 0xf684);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4096, 0xf684);
    _mt_fe_tn_set_reg_16bit_tc6930(0x409b, 0xf684);


    _mt_fe_tn_set_reg_16bit_tc6930(0x401d, 0x0001);


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4181, 0x0010, 15, 7);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4185, 0x0010, 15, 7);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4189, 0x0010, 15, 7);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4606, 0x0080);


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4093, 0x000a, 9, 5);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4098, 0x000a, 9, 5);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x409d, 0x000a, 9, 5);


    _mt_fe_tn_set_reg_16bit_tc6930(0x4171, 0x0090);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4172, 0x0201);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4173, 0x0201);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4174, 0x0201);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4170, 0x600a);


    _mt_fe_tn_set_reg_16bit_tc6930(0x4119, 0x07b3);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4118, 0x07b2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4117, 0x07aa);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4116, 0x07a2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4115, 0x079a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4114, 0x075a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4113, 0x055a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4112, 0x035a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4111, 0x031a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4110, 0x02da);
    _mt_fe_tn_set_reg_16bit_tc6930(0x410f, 0x029a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x410e, 0x025a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x410d, 0x005a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x410c, 0x0052);
    _mt_fe_tn_set_reg_16bit_tc6930(0x410b, 0x004a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x410a, 0x0042);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4109, 0x0041);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4108, 0x0040);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4135, 0x07b3);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4134, 0x07b2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4133, 0x07aa);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4132, 0x07a2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4131, 0x079a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4130, 0x075a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x412f, 0x055a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x412e, 0x035a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x412d, 0x031a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x412c, 0x02da);
    _mt_fe_tn_set_reg_16bit_tc6930(0x412b, 0x029a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x412a, 0x025a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4129, 0x005a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4128, 0x0052);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4127, 0x004a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4126, 0x0042);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4125, 0x0041);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4124, 0x0040);

    _mt_fe_tn_set_reg_16bit_tc6930(0x4151, 0x07b3);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4150, 0x07b2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x414f, 0x07aa);
    _mt_fe_tn_set_reg_16bit_tc6930(0x414e, 0x07a2);
    _mt_fe_tn_set_reg_16bit_tc6930(0x414d, 0x079a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x414c, 0x075a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x414b, 0x055a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x414a, 0x035a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4149, 0x031a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4148, 0x02da);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4147, 0x029a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4146, 0x025a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4145, 0x005a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4144, 0x0052);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4143, 0x004a);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4142, 0x0042);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4141, 0x0041);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4140, 0x0040);


    _mt_fe_tn_set_reg_16bit_tc6930(0x408a, 0x033f);


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c1, 0x04, 11, 8);


    _mt_fe_tn_ADC_calibration_tc6930(handle, demod_index);

    for (i = 0; i <= 2; i++) {
        if (demod_index != i) {
            mt_fe_tn_sleep_tc6930(handle, i);
        }
    }


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4107, 1, 5, 5);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4123, 1, 5, 5);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x413f, 1, 5, 5);


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4713, 1, 12, 12);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4713, 0,  4,  4);

    return;
}


void _mt_fe_tn_set_appendix_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    return;
}


void mt_fe_tn_wakeup_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473b, 0x03, 5, 4);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473d, 0x03, 5, 4);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x473f, 0x03, 5, 4);

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 1, 15, 15);

    if (tuner_index_in == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 1, 14, 14);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4607, 1, 0, 0);		// debug_to_mcu1, rxa is on
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 1, 13, 13);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4607, 1, 1, 1);		// debug_to_mcu1, rxb is on
    } else if (tuner_index_in == 2) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 1, 12, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4607, 1, 2, 2);		// debug_to_mcu1, rxc is on
    }

    handle->demod_device[demod_index].tuner_settings.tuner_sleep_status = 1;

    return;
}


void mt_fe_tn_sleep_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U8 tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    if (tuner_index_in == 0) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 14, 14);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4607, 0, 0, 0);		// debug_to_mcu1, rxa is off
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 13, 13);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4607, 0, 1, 1);		// debug_to_mcu1, rxb is off
    } else if (tuner_index_in == 2) {
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 12, 12);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4607, 0, 2, 2);		// debug_to_mcu1, rxc is off
    }

    handle->demod_device[demod_index].tuner_settings.tuner_sleep_status = 0;

    return;
}


S32 mt_fe_tn_download_fw_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    U32 i = 0, fw_size = 0;
    U16	data0 = 0, data1 = 0, tmp;
    S32	ret = 0;
    const U8 *p_fw = m88tc6930_fm;
    fw_size = sizeof(m88tc6930_fm);

    _mt_fe_tn_get_reg_16bit_tc6930(0x4747, &tmp);
    if (((tmp & 0x4000) == 0) || (handle->tuner_mcu_status != 1)) {
        _mt_fe_tn_set_reg_16bit_tc6930(0x4602, 0x01);

        for (i = 0; i < fw_size; i += 2) {
            ret = _mt_fe_tn_set_reg_16bit_tc6930(0x4600, p_fw[i]);
            ret = _mt_fe_tn_set_reg_16bit_tc6930(0x4601, p_fw[i + 1]);
        }

        _mt_fe_tn_set_reg_16bit_tc6930(0x4602, 0x00);

        _mt_fe_tn_get_reg_16bit_tc6930(0x4608, &data0);
        _mt_fe_tn_get_reg_16bit_tc6930(0x4600, &data1);

        tmp |= 0x4000;
        _mt_fe_tn_set_reg_16bit_tc6930(0x4747, tmp);

        handle->tuner_mcu_status = 1;

#if TC6930_ENABLE_PRINT
        tc6930_dbg_print(("TC6930: Download firmware 0x%x, 0x%x OK!\n", data0, data1));
#endif
    }

    return 0;
}


void mt_fe_tn_init_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    if (handle->demod_device[demod_index].tuner_settings.tuner_init_OK != 1) {
        U8 iIndex = 0;

#if TC6930_ENABLE_PRINT
        tc6930_dbg_print(("TC6930 initialize.\n"));
#endif

        for (iIndex = 0; iIndex < 3; iIndex++) {
            handle->demod_device[iIndex].tuner_settings.tuner_dev_addr			 = TUNER_I2C_ADDR_TC6930;

            handle->demod_device[iIndex].tuner_settings.tuner_application		 = 0;
            handle->demod_device[iIndex].tuner_settings.tuner_mode				 = 0;		// 0: disable loop-through
            handle->demod_device[iIndex].tuner_settings.tuner_int_lt			 = 2;


            handle->demod_device[iIndex].tuner_settings.tuner_bandwidth			 = 8000;
            handle->demod_device[iIndex].tuner_settings.tuner_crystal			 = 24000;
            handle->demod_device[iIndex].tuner_settings.tuner_dac				 = 7200;
            handle->demod_device[iIndex].tuner_settings.tuner_dac_gain			 = 4;

            handle->demod_device[iIndex].tuner_settings.tuner_crystal_cap		 = 0x18;
            handle->demod_device[iIndex].tuner_settings.tuner_clock_out			 = 1;

            handle->demod_device[iIndex].tuner_settings.tuner_int_im			 = 2;

            handle->demod_device[iIndex].tuner_settings.tuner_custom_cfg		 = MT_TC6930_CUSTOM_CONFIG;
            handle->demod_device[iIndex].tuner_settings.tuner_region_cfg		 = 0;
            handle->demod_device[iIndex].tuner_settings.tuner_version			 = 202;
            handle->demod_device[iIndex].tuner_settings.tuner_fw_version		 = 0x0c;
            handle->demod_device[iIndex].tuner_settings.tuner_time				 = 21080411;

            handle->demod_device[iIndex].tuner_settings.tuner_init_OK			 = 1;

            handle->demod_device[iIndex].tuner_settings.tuner_signal_delay_en	 = 0;

            handle->demod_device[iIndex].tuner_settings.tuner_signal_delay_ns	 = 0;

            handle->demod_device[iIndex].tuner_settings.tuner_mixer_type		 = 0;
            handle->demod_device[iIndex].tuner_settings.tuner_harmonic_imp		 = 1;

            handle->demod_device[iIndex].tuner_settings.tuner_gpio_out			 = 0;

            handle->demod_device[iIndex].tuner_settings.tuner_sleep_status		 = 0;
        }
    }

    mt_fe_tn_download_fw_tc6930(handle);

    return;
}

int _mt_fe_tn_check_freq_tc6930(int iFreqKHz, int iFreqH, int iFreqL)
{
    int bOK = TRUE;

    if (iFreqH < iFreqL) {
        int iTmp;

        iTmp = iFreqH;
        iFreqH = iFreqL;
        iFreqL = iTmp;
    }

    if ((iFreqKHz >= iFreqL) && (iFreqKHz <= iFreqH)) {
        bOK = TRUE;
    } else {
        bOK = FALSE;
    }

    return bOK;
}

int _mt_fe_tn_harm_comb_hitornot_tc6930(int clock_demo, int *clk_demo_list_round, int iDemoCnt, int clock_rgmii, int *clk_rgmii_list_round, int iRgmiiCnt, int *tuner_fl, int *tuner_fh)
{
    int flag_hit = FALSE, condition_in_a, condition_in_b, condition_in_c;

    int freq_harm_comb;

    int i, j, sign;

    for (i = 0; i < iRgmiiCnt; i++) {
        for (j = 0; j < iDemoCnt; j++) {
            for (sign = 0; sign < 3; sign++) {
                if (sign == 0) {
                    freq_harm_comb = clk_demo_list_round[j] * clock_demo + clk_rgmii_list_round[i] * clock_rgmii;
                } else if (sign == 1) {
                    freq_harm_comb = clk_demo_list_round[j] * clock_demo - clk_rgmii_list_round[i] * clock_rgmii;
                } else {
                    freq_harm_comb = clk_rgmii_list_round[i] * clock_rgmii - clk_demo_list_round[j] * clock_demo;
                }

                if ((freq_harm_comb < 100000) || (freq_harm_comb > 900000)) {
                    continue;
                }

                condition_in_a = ((freq_harm_comb > tuner_fl[0]) && (freq_harm_comb < tuner_fh[0]));
                condition_in_b = ((freq_harm_comb > tuner_fl[1]) && (freq_harm_comb < tuner_fh[1]));
                condition_in_c = ((freq_harm_comb > tuner_fl[2]) && (freq_harm_comb < tuner_fh[2]));

                if (condition_in_a || condition_in_b || condition_in_c) {
                    flag_hit = TRUE;
                    break;
                }
            }

            if (flag_hit) {
                break;
            }
        }

        if (flag_hit) {
            break;
        }
    }

    return flag_hit;
}

void _mt_fe_tn_CPU_freq_adjust_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz)
{
    int iIndex, i, j, iCnt;

    U32 uiTunerFreqKHz[3];
    U32 uiBandWidth[3];

    U32 tuner_fh[3], tuner_fl[3];

    int div_ratio, out_div_ratio;
    int div_ratio_max = 275;
    int div_ratio_min = 190;
    int div_ratio_step = 5;

    int freq_pll = 1152000;

    int freq_clock;

    int condition_in[3];
    int harm_max, num_harm;
    int freq_clock_harm;

    int flag_hit = FALSE;

    U8 N = 6, F = 3;


    for (i = 0; i < 3; i++) {
        iIndex = handle->demod_device[i].tuner_settings.tuner_index;

        uiTunerFreqKHz[iIndex] = handle->demod_device[i].tuner_settings.tuner_freq;
        if (handle->demod_device[i].tuner_settings.tuner_sleep_status == 0) {
            uiTunerFreqKHz[iIndex] = 0;
        }

        uiBandWidth[iIndex] = handle->demod_device[i].tuner_settings.tuner_bandwidth; // + 400;


        tuner_fh[iIndex] = uiTunerFreqKHz[iIndex] + uiBandWidth[iIndex] / 2;
        tuner_fl[iIndex] = uiTunerFreqKHz[iIndex] - uiBandWidth[iIndex] / 2;
    }

    iCnt = sizeof(mDataList_FAST_SWITCH) / sizeof(MT_FE_TN_TC6930_FREQ_ADJUST);

    for (i = 0; i < iCnt; i++) {	// search the table to find the matched line
        U32 uiFreqMHz[3] = {0, 0, 0};
        int bFreqOK[3] = {FALSE, FALSE, FALSE};

        for (j = 0; j < 3; j++) {	// check 3 frequencies
            uiFreqMHz[j] = mDataList_FAST_SWITCH[i].iFreqMHz[j];

            if ((uiFreqMHz[j] == 0) || (uiFreqMHz[j] * 1000 == uiTunerFreqKHz[j])) {	// 0 or equal
                bFreqOK[j] = TRUE;
            }
        }

        if (bFreqOK[0] && bFreqOK[1] && bFreqOK[2]) {	// frequencies matched in the table
            break;
        }
    }

    if (i != iCnt) {	// frequencies matched, the index is valid
        N = mDataList_FAST_SWITCH[i].iDataR4712;
        F = mDataList_FAST_SWITCH[i].iDataR4713;
    } else {
        for (div_ratio = div_ratio_max; div_ratio >= div_ratio_min; div_ratio -= div_ratio_step) {
            flag_hit = FALSE;

            freq_clock = (freq_pll * 10 + div_ratio / 2) / div_ratio;
            harm_max = 1000000 / freq_clock;

            for (num_harm = 1; num_harm <= harm_max; num_harm++) {
                freq_clock_harm = freq_clock * num_harm;

                for (i = 0; i < 3; i++) {
                    if (uiTunerFreqKHz[i] != 0) {
                        condition_in[i] = _mt_fe_tn_check_freq_tc6930(freq_clock_harm, tuner_fh[i], tuner_fl[i]);
                    } else {
                        condition_in[i] = FALSE;
                    }
                }

                if (condition_in[0] || condition_in[1] || condition_in[2]) {
                    flag_hit = TRUE;
                    break;
                } else {
                    flag_hit = FALSE;
                }
            }

            if (!flag_hit) {
                break;
            }
        }

        if (!flag_hit) {
            out_div_ratio = div_ratio;

            N = out_div_ratio / 40;
            F = (out_div_ratio % 40) * 2 / 10;
        }
    }

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4712, N, 3, 0);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4713, F, 15, 13);

    _mt_fe_tc6930_set_reg_32bit_bits(0x7C04, 1, 10, 10);
    _mt_fe_tc6930_set_reg_32bit_bits(0x7C04, 0, 10, 10);

    return;
}

void mt_fe_tn_set_freq_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz)
{
    U8 tuner_index_in = 0;
    U16 buf = 0;


    U8 mixer_sel = 0;

#if TC6930_ENABLE_PRINT
    tc6930_dbg_print(("TC6930 set freq, tuner_app is %d\n", handle->demod_device[demod_index].tuner_settings.tuner_application));
#endif

    handle ->demod_device[demod_index].tuner_settings.tuner_freq = freq_KHz;

    mt_fe_tn_init_tc6930(handle, demod_index);

    tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    mt_fe_tn_wakeup_tc6930(handle, demod_index);

    _mt_fe_tn_CPU_freq_adjust_tc6930(handle, demod_index, freq_KHz);

    _mt_sleep_tc6930(1);

    if (handle->demod_device[demod_index].tuner_settings.tuner_mixer_type == 1) {
        mixer_sel = 1;
    } else if (handle->demod_device[demod_index].tuner_settings.tuner_mixer_type == 2) {
        mixer_sel = 2;
    } else if (handle->demod_device[demod_index].tuner_settings.tuner_mixer_type == 3) {
        mixer_sel = 3;
    } else {
        if (freq_KHz <= 334000) {
            mixer_sel = 2;
        } else if (handle->demod_device[demod_index].tuner_settings.tuner_harmonic_imp == 1) {
            if (handle->demod_device[demod_index].tuner_settings.tuner_application != 0) {
                mixer_sel = 3;
            } else {
                if (((freq_KHz >= 800000) && (freq_KHz <= 830000)) ||
                    ((freq_KHz >= 610000) && (freq_KHz <= 624000)) ||
                    ((freq_KHz >= 480000) && (freq_KHz <= 492000)) ||
                    ((freq_KHz >= 366000) && (freq_KHz <= 374000))) {
                    mixer_sel = 3;
                } else {
                    mixer_sel = 1;
                }
            }
        } else {
            mixer_sel = 1;
        }
    }


    if (demod_index == 0) {
        _mt_fe_dsp_set_reg_bit_tc6930(0, 0xc0, 0x03, 1, 0);
        _mt_fe_dsp_set_reg_bit_tc6930(0, 0xc1, 0x01, 7, 7);
    } else if (demod_index == 1) {
        _mt_fe_dsp_set_reg_bit_tc6930(1, 0xc0, 0x03, 1, 0);
        _mt_fe_dsp_set_reg_bit_tc6930(1, 0xc1, 0x01, 7, 7);
    } else if (demod_index == 2) {
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xc0, 0x03, 1, 0);
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xc1, 0xff, 7, 0);
    }


    _mt_fe_tn_get_reg_16bit_tc6930(0x4747, &buf);
    buf = buf & 0x8000;

    if (buf == 0x0000) {
        _mt_fe_tn_poweron_set_tc6930(handle, demod_index);
        _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4747, 0x01, 15, 15);
    }


    _mt_fe_tn_preset_tc6930(handle, demod_index);


    if (handle->demod_device[demod_index].tuner_settings.tuner_custom_cfg == 1) {
        if (((freq_KHz == 794000)) || ((freq_KHz == 810000))) { //20191107
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42be, 0x03, 15, 12);
            _mt_fe_tn_set_reg_16bit_tc6930(0x42c8, 0x006a);
        } else {
            _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42be, 0x01, 15, 12);
            _mt_fe_tn_set_reg_16bit_tc6930(0x42c8, 0x005a);
        }

        if ((freq_KHz == 266000) || (freq_KHz == 402000) || ((freq_KHz >= 342000) && (freq_KHz <= 356000))) {
            if (tuner_index_in == 0) {	// RXA
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 10, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4708, 0x09, 6, 0);
            } else if (tuner_index_in == 1) {	// RXB
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 9, 9);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470b, 0x09, 6, 0);
            } else if (tuner_index_in == 2) {	// RXC
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 8, 8);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470e, 0x09, 6, 0);
            }
        } else if ((freq_KHz >= 800000) && (freq_KHz <= 830000)) {
            if (tuner_index_in == 0) {	// RXA
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 10, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4708, 0x00, 6, 0);
            } else if (tuner_index_in == 1) {	// RXB
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 9, 9);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470b, 0x00, 6, 0);
            } else if (tuner_index_in == 2) {	// RXC
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 8, 8);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470e, 0x00, 6, 0);
            }
        } else if ((freq_KHz >= 480000) && (freq_KHz <= 498000)) {
            if (tuner_index_in == 0) {	// RXA
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 10, 10);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4708, 0x02, 6, 0);
            } else if (tuner_index_in == 1) {	// RXB
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 9, 9);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470b, 0x02, 6, 0);
            } else if (tuner_index_in == 2) {	// RXC
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x01, 8, 8);
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x470e, 0x02, 6, 0);
            }
        } else {
            if (tuner_index_in == 0) {	// RXA
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x00, 10, 10);
            } else if (tuner_index_in == 1) {	// RXB
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x00, 9, 9);
            } else if (tuner_index_in == 2) {	// RXC
                _mt_fe_tn_set_reg_16bit_tc6930_bits(0x42c6, 0x00, 8, 8);
            }
        }
    }


    _mt_fe_tn_set_RF_front_tc6930(handle, demod_index, freq_KHz);

    _mt_fe_tn_set_Mixer_tc6930(handle, demod_index, freq_KHz, mixer_sel);

    _mt_fe_tn_set_LO_tc6930(handle, demod_index, freq_KHz, mixer_sel);

    _mt_fe_tn_set_PLL_freq_tc6930(handle, demod_index, 0, 0);

    _mt_fe_tn_set_BB_tc6930(handle, demod_index);

    _mt_fe_tn_set_DAC_tc6930(handle, demod_index, freq_KHz);

    _mt_fe_tn_run_tc6930(handle, demod_index);


    if (demod_index == 0) {
        _mt_fe_dsp_set_reg_bit_tc6930(0, 0xc0, 0x00, 1, 0);
        _mt_fe_dsp_set_reg_bit_tc6930(0, 0xc1, 0x00, 7, 7);
    } else if (demod_index == 1) {
        _mt_fe_dsp_set_reg_bit_tc6930(1, 0xc0, 0x00, 1, 0);
        _mt_fe_dsp_set_reg_bit_tc6930(1, 0xc1, 0x00, 7, 7);
    } else if (demod_index == 2) {
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xc0, 0x00, 1, 0);
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xc1, 0x00, 7, 0);
    }


#if TC6930_ENABLE_PRINT
    tc6930_dbg_print(("TC6930 set finished.\n"));
#endif

    return;
}


S32 mt_fe_tn_get_signal_strength_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    static S32 xm_list_dBm[16][16] = {
        {   0,    0,  602,  954, 1204, 1398, 1556, 1690, 1806, 1908, 2000, 2083, 2158, 2228, 2292, 2352},
        {2408, 2461, 2511, 2558, 2602, 2644, 2685, 2723, 2760, 2796, 2830, 2863, 2894, 2925, 2954, 2983},
        {3010, 3037, 3063, 3088, 3113, 3136, 3160, 3182, 3204, 3226, 3246, 3267, 3287, 3306, 3326, 3344},
        {3362, 3380, 3398, 3415, 3432, 3449, 3465, 3481, 3496, 3512, 3527, 3542, 3556, 3571, 3585, 3599},
        {3612, 3626, 3639, 3652, 3665, 3678, 3690, 3703, 3715, 3727, 3738, 3750, 3762, 3773, 3784, 3795},
        {3806, 3817, 3828, 3838, 3849, 3859, 3869, 3879, 3889, 3899, 3908, 3918, 3928, 3937, 3946, 3955},
        {3965, 3974, 3982, 3991, 4000, 4009, 4017, 4026, 4034, 4042, 4051, 4059, 4067, 4075, 4083, 4091},
        {4098, 4106, 4114, 4121, 4129, 4136, 4144, 4151, 4158, 4166, 4173, 4180, 4187, 4194, 4201, 4208},
        {4214, 4221, 4228, 4235, 4241, 4248, 4254, 4261, 4267, 4273, 4280, 4286, 4292, 4298, 4305, 4311},
        {4317, 4323, 4329, 4335, 4341, 4346, 4352, 4358, 4364, 4369, 4375, 4381, 4386, 4392, 4397, 4403},
        {4408, 4414, 4419, 4424, 4430, 4435, 4440, 4445, 4451, 4456, 4461, 4466, 4471, 4476, 4481, 4486},
        {4491, 4496, 4501, 4506, 4511, 4515, 4520, 4525, 4530, 4534, 4539, 4544, 4548, 4553, 4558, 4562},
        {4567, 4571, 4576, 4580, 4585, 4589, 4593, 4598, 4602, 4606, 4611, 4615, 4619, 4624, 4628, 4632},
        {4636, 4640, 4644, 4649, 4653, 4657, 4661, 4665, 4669, 4673, 4677, 4681, 4685, 4689, 4693, 4697},
        {4700, 4704, 4708, 4712, 4716, 4720, 4723, 4727, 4731, 4735, 4738, 4742, 4746, 4749, 4753, 4757},
        {4760, 4764, 4768, 4771, 4775, 4778, 4782, 4785, 4789, 4792, 4796, 4799, 4803, 4806, 4810, 4813}
    };

    U8 bufH, bufL;
    U16 buf;

    S32 level = -107;
    S32 gain_x10;
    S32 gain_ref_x10;
    S32 pd_x10;
    S32 pdtref_x10 = -390;

    U32 freq_KHz;
    U8 tuner_index_in;// = handle->demod_device[demod_index].tuner_settings.tuner_index;


    if (handle == NULL) {
        return -107;
    }

    if (!handle->demod_device[demod_index].tuner_settings.tuner_init_OK) {
        return -107;
    }

    tuner_index_in = handle->demod_device[demod_index].tuner_settings.tuner_index;

    freq_KHz = handle->demod_device[demod_index].tuner_settings.tuner_freq;

    pdtref_x10 = -430;

    gain_ref_x10 = 560;

    if (tuner_index_in == 0) {	// RXA
        _mt_fe_tn_get_reg_16bit_tc6930(0x4162, &buf);
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_get_reg_16bit_tc6930(0x4165, &buf);
    } else { // if (tuner_index_in == 2)
        _mt_fe_tn_get_reg_16bit_tc6930(0x4168, &buf);
    }
    buf = buf >> 1;
    buf = buf & 0x00ff;


    gain_x10 = gain_ref_x10 + 300 - 1350 + (buf * 10);

    gain_x10 = gain_x10 + 150;

    if (tuner_index_in == 0) {	// RXA
        _mt_fe_tn_get_reg_16bit_tc6930(0x42be, &buf);
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_get_reg_16bit_tc6930(0x42bf, &buf);
    } else { // if (tuner_index_in == 2)
        _mt_fe_tn_get_reg_16bit_tc6930(0x42c0, &buf);
    }

    buf = buf & 0x0ff0;
    buf = buf >> 4;


    if (tuner_index_in == 0) {	// RXA
        _mt_fe_tn_get_reg_16bit_tc6930(0x4183, &buf);
    } else if (tuner_index_in == 1) {
        _mt_fe_tn_get_reg_16bit_tc6930(0x4187, &buf);
    } else { // if (tuner_index_in == 2)
        _mt_fe_tn_get_reg_16bit_tc6930(0x418b, &buf);
    }

    buf = buf & 0xff;

    bufH = buf >> 4;
    bufL = buf & 0x0f;

    pd_x10 = xm_list_dBm[bufH][bufL] / 10 + 5 + pdtref_x10;


    if (freq_KHz < 339000) {
        level = (pd_x10 - gain_x10) / 10 + 22 - freq_KHz / 70000;
    } else {
        level = (pd_x10 - gain_x10) / 10 + 18 + freq_KHz / 100000;
    }

    if (level < -107) {
        level = -107;
    }

    return level;
}

void mt_fe_tn_get_signal_delay_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U32 delay_sel, signal_delay;
    U8 bufH, bufL, delay_en = 0;

    delay_en = handle->demod_device[demod_index].tuner_settings.tuner_signal_delay_en;
    signal_delay = handle->demod_device[demod_index].tuner_settings.tuner_signal_delay_ns;

    delay_sel = ((signal_delay - 4360) + 74) / 148;
    bufH = (delay_sel >> 8) & 0x01;
    bufL = delay_sel & 0xff;

    if (delay_en == 1) {
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xf6, 1, 1, 1);
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xf6, bufH, 0, 0);
        _mt_fe_dsp_set_reg_tc6930(2, 0xf7, bufL);
    } else {
        _mt_fe_dsp_set_reg_bit_tc6930(2, 0xf6, 0, 1, 1);
    }

}

/*****************************************************************************/
/*API: 	mt_fe_tn_set_GPIOout_high_tc6930									 */
/*****************************************************************************/
void mt_fe_tn_set_GPIOout_high_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    _mt_fe_dsp_set_reg_bit_tc6930(2, 0x37, 0x01, 2, 0);

    return;
}

/*****************************************************************************/
/*API: 	mt_fe_tn_set_GPIOout_lowt_tc6930									 */
/*****************************************************************************/
void mt_fe_tn_set_GPIOout_low_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    _mt_fe_dsp_set_reg_bit_tc6930(2, 0x37, 0x00, 2, 0);

    return;
}

void mt_fe_tn_get_diagnose_info_tc6930(MT_FE_TC6930_Device_Handle handle, U32 *data1, U32 *data2)
{
#if TC6930_ENABLE_PRINT
    U8	reg_data0 = 0, reg_data1 = 0, reg_data2 = 0, reg_data3 = 0, reg_data4 = 0;
    U32	value0 = 0, value1 = 0;

    tc6930_dbg_print(("TC6930 -- cg: %d, ci: %d, if: %d\n", reg_data0, reg_data1, reg_data4));

    tc6930_dbg_print(("TC6930 -- tia: %d, bb: %d, vga: %d, dac: %d, fr: %d\n", reg_data0, reg_data1, reg_data2, reg_data3, reg_data4));

    tc6930_dbg_print(("TC6930 -- pll_lock: %d, bpflt_c: %d, lna_cc: %d, cc_final: %d\n", reg_data0, reg_data1, value0, value1));

    tc6930_dbg_print(("TC6930 -- 0x01: 0x%x, 0xc5: 0x%x, 0xc6: 0x%x, 0xc8: 0x%x\n", reg_data0, reg_data1, reg_data2, reg_data3));

    tc6930_dbg_print(("TC6930 -- 0x6e: 0x%x, 0x6f: 0x%x, 0x70: 0x%x, 0x71: 0x%x, 0x72: 0x%x\n", reg_data0, reg_data1, reg_data2, reg_data3, reg_data4));

    tc6930_dbg_print(("TC6930 -- 0x73: 0x%x, 0x74: 0x%x, 0x75: 0x%x\n", reg_data0, reg_data1, reg_data2));

#endif

    return;
}


void mt_fe_tn_sleep_all_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    mt_fe_tn_sleep_tc6930(handle, 0);
    mt_fe_tn_sleep_tc6930(handle, 1);
    mt_fe_tn_sleep_tc6930(handle, 2);


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4016, 0, 4, 4);
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4017, 0, 6, 6);

    return;
}

void mt_fe_tn_set_tuner_index_tc6930(MT_FE_TC6930_Device_Handle handle)
{
    U8 iTunerSel1, iTunerSel2, iTunerSel3, iTmp = 0;

    iTunerSel1 = handle->demod_device[0].tuner_settings.tuner_index;
    iTunerSel2 = handle->demod_device[1].tuner_settings.tuner_index;
    iTunerSel3 = handle->demod_device[2].tuner_settings.tuner_index;

    iTunerSel1 %= 3;
    iTunerSel2 %= 3;
    iTunerSel3 %= 3;

    if ((iTunerSel1 == 2) && (iTunerSel2 == 1) && (iTunerSel3 == 0))	{
        iTmp = 1;
    } else if ((iTunerSel1 == 0) && (iTunerSel2 == 2) && (iTunerSel3 == 1))	{
        iTmp = 2;    // iTmp = 7;
    } else if ((iTunerSel1 == 1) && (iTunerSel2 == 2) && (iTunerSel3 == 0))	{
        iTmp = 3;    // iTmp = 6;
    } else if ((iTunerSel1 == 1) && (iTunerSel2 == 0) && (iTunerSel3 == 2))	{
        iTmp = 4;
    } else if ((iTunerSel1 == 2) && (iTunerSel2 == 0) && (iTunerSel3 == 1))	{
        iTmp = 5;
    } else {
        iTmp = 0;
    }

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4017, iTunerSel3, 9, 8);
    _mt_fe_tc6930_set_reg_32bit_bits(0x7C04, iTmp, 18, 16);		// register 0x3F010

    return;
}

