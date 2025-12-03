/*****************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd                                    */
/* MONTAGE PROPRIETARY AND CONFIDENTIAL                                      */
/* Copyright (c) 2020 Montage Technology Group Limited. All Rights Reserved. */
/*****************************************************************************/
/*
 * Filename:            mt_fe_dmd_tc6930.c
 *
 * Description:         Montage QAM demodulator driver of Jazz.
 *
 * Current Version:     0.04.04
 *
 *  History:
 *
 *  Description     Version     Date        Author
 *---------------------------------------------------------------------------
 *  File Create     0.00.01     2017.05.01  YZ.Huang
 *  Modify          0.01.00     2017.07.10  YZ.Huang
 *  Modify          0.01.05     2017.08.29  YZ.Huang
 *  Modify          0.01.06     2017.11.22  YZ.Huang
 *  Modify          0.01.08     2018.01.23  YZ.Huang
 *  Modify          0.01.09     2018.06.13  YZ.Huang
 *  Modify          0.01.10     2019.01.28  YZ.Huang
 *  Modify          0.01.11     2019.02.14  YZ.Huang
 *  Modify          0.01.12     2019.03.06  YZ.Huang
 *  Modify          0.02.00     2019.06.04  YZ.Huang
 *  Modify          0.02.01     2019.10.12  YZ.Huang
 *  Modify          0.02.02     2019.11.16  YZ.Huang
 *  Modify          0.03.00     2020.04.03  YZ.Huang
 *  Modify          0.03.01     2020.04.30  YZ.Huang
 *  Modify          0.03.02     2020.08.12  YZ.Huang
 *  Modify          0.03.03     2020.09.15  YZ.Huang
 *  Modify          0.03.04     2020.09.16  YZ.Huang
 *  Modify          0.04.00     2020.11.20  YZ.Huang
 *  Modify          0.04.01     2021.04.07  YZ.Huang
 *  Modify          0.04.02     2021.07.27  YZ.Huang
 *  Modify          0.04.03     2021.09.10  YZ.Huang
 *  Modify          0.04.04     2022.08.09  YZ.Huang
 *****************************************************************************/


#include "mt_fe_i2c_tc6930.h"

MT_FE_TC6930_SETTINGS tc6930_config;


MT_FE_RET mt_fe_dmd_tc6930_match_tuners(MT_FE_TC6930_Device_Handle handle)
{
    if ((handle->demod_device[0].demod_type == MT_FE_DEMOD_TC6920) ||
        (handle->demod_device[0].demod_type == MT_FE_DEMOD_RC6800)) {
        // TC6920 or RC6800
        handle->demod_device[0].tuner_settings.tuner_index = 1;
        handle->demod_device[1].tuner_settings.tuner_index = 0;
        handle->demod_device[2].tuner_settings.tuner_index = 2;
    } else {
        // TC6930 or TC6960
        handle->demod_device[0].tuner_settings.tuner_index = 1;
        handle->demod_device[1].tuner_settings.tuner_index = 2;
        handle->demod_device[2].tuner_settings.tuner_index = 0;
    }

    return MtFeErr_Ok;
}

#include <stdio.h>
MT_FE_SUPPORTED_DEMOD mt_fe_dmd_tc6930_check_chip_id(MT_FE_TC6930_Device_Handle handle)
{
    // 0x3f01c
    // TC6920	000468A1
    // TC6930	000468A1
    // RC6900	000268A1

    // TC6960	000468A2
    // TC6930	000468A2
    // RC6900	000268A2
    // TC6920	000768A2
    // RC6800	000568A2

    U32 ulData = 0;

    MT_FE_SUPPORTED_DEMOD demod_type = MT_FE_DEMOD_UNDEF;

    _mt_fe_tc6930_get_reg_32bit((0x3F01C - 0x20000) / 4, &ulData);
    switch (ulData) {
    case 0x000268A1:
    case 0x000268A2:
        demod_type = MT_FE_DEMOD_RC6900;
        break;

    case 0x000768A2:
        demod_type = MT_FE_DEMOD_TC6920;
        break;

    case 0x000568A2:
        demod_type = MT_FE_DEMOD_RC6800;
        break;

    case 0x000468A1:
    case 0x000468A2:
        demod_type = MT_FE_DEMOD_TC6960;
        break;

    default:
        break;
    }

    if (demod_type == MT_FE_DEMOD_TC6960) {
        if ((handle->demod_device[0].demod_type != MT_FE_DEMOD_TC6930) &&
            (handle->demod_device[0].demod_type != MT_FE_DEMOD_TC6960)) {
            handle->demod_device[0].demod_type = demod_type;
            handle->demod_device[1].demod_type = demod_type;

            mt_fe_dmd_tc6930_match_tuners(handle);
        }
    } else {
        handle->demod_device[0].demod_type = demod_type;
        handle->demod_device[1].demod_type = demod_type;

        mt_fe_dmd_tc6930_match_tuners(handle);
    }

    return demod_type;
}

MT_FE_RET mt_fe_dmd_tc6930_optimize_tuner(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
#if 0	// demo applications
    if (handle->demod_device[0].demod_type != MT_FE_DEMOD_RC6800) {
        return MtFeErr_Ok;
    }

    if (handle->demod_device[0].channel_param.freq_KHz <= 542000) {
        handle->demod_device[0].tuner_settings.tuner_index = 0;
        handle->demod_device[1].tuner_settings.tuner_index = 1;
        handle->demod_device[2].tuner_settings.tuner_index = 2;
    } else if (handle->demod_device[0].channel_param.freq_KHz <= 650000) {
        handle->demod_device[0].tuner_settings.tuner_index = 1;
        handle->demod_device[1].tuner_settings.tuner_index = 0;
        handle->demod_device[2].tuner_settings.tuner_index = 2;
    } else {
        handle->demod_device[0].tuner_settings.tuner_index = 1;
        handle->demod_device[1].tuner_settings.tuner_index = 2;
        handle->demod_device[2].tuner_settings.tuner_index = 0;
    }

#endif

    return MtFeErr_Ok;
}


MT_FE_RET mt_fe_dmd_tc6930_config_default(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U8 iIndex = 0;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }


    for (iIndex = 0; iIndex < 3; iIndex++) {
        handle->demod_device[iIndex].bInitOK							 = TRUE;
        handle->demod_device[iIndex].dev_index							 = iIndex;
        handle->demod_device[iIndex].demod_dev_addr						 = 0x38;
        handle->demod_device[iIndex].chip_id							 = 0;

        handle->demod_device[iIndex].perf_optimized						 = 0;

        if (iIndex < 2) {
            handle->demod_device[iIndex].demod_type						 = MT_FE_DEMOD_TC6930;
        } else {
            handle->demod_device[iIndex].demod_type						 = MT_FE_DEMOD_M88DC2800;
        }

        handle->demod_device[iIndex].dtv_mode							 = MtFeType_DVBC;

        handle->demod_device[iIndex].channel_param.freq_KHz				 = 762000;
        handle->demod_device[iIndex].channel_param.sym_KSs				 = 6900;
        handle->demod_device[iIndex].channel_param.qam_code				 = 64;
        handle->demod_device[iIndex].channel_param.inverted				 = 0;
        handle->demod_device[iIndex].channel_param.param_reserved		 = 0;

        handle->demod_device[iIndex].tuner_settings.tuner_init_OK		 = FALSE;
        handle->demod_device[iIndex].tuner_settings.tuner_index			 = iIndex;

        handle->demod_device[iIndex].on_board_settings.chip_mode		 = 2;
        handle->demod_device[iIndex].on_board_settings.bSingleXTAL		 = TRUE;
        handle->demod_device[iIndex].on_board_settings.bReadCali		 = FALSE;
        handle->demod_device[iIndex].on_board_settings.iVppSel			 = 0;
        handle->demod_device[iIndex].on_board_settings.iGainStep		 = 0;
        handle->demod_device[iIndex].on_board_settings.xtal_KHz			 = 28800;
        handle->demod_device[iIndex].on_board_settings.chip_version		 = 0xD0;
        handle->demod_device[iIndex].on_board_settings.on_board_reserved = 0;

        handle->demod_device[iIndex].ts_output_settings.active_edge_clk	 = 0;
        handle->demod_device[iIndex].ts_output_settings.polar_sync		 = 0;
        handle->demod_device[iIndex].ts_output_settings.polar_val		 = 0;
        handle->demod_device[iIndex].ts_output_settings.polar_err		 = 0;
        handle->demod_device[iIndex].ts_output_settings.output_mode		 = MtFeTsOutMode_Serial;
        handle->demod_device[iIndex].ts_output_settings.output_clock	 = MtFeTSOut_Max_Clock_Userdefine;
        handle->demod_device[iIndex].ts_output_settings.custom_duty_cycle = 0;
        handle->demod_device[iIndex].ts_output_settings.serial_pin_select = 0;
        handle->demod_device[iIndex].ts_output_settings.output_high_Z	 = 0;
    }

    handle->dmd_set_reg							 = _mt_fe_dmd_set_reg_tc6930;
    handle->dmd_get_reg							 = _mt_fe_dmd_get_reg_tc6930;
    handle->mt_sleep							 = _mt_sleep_tc6930;
    handle->Set32Bits							 = _mt_fe_write32_tc6930;
    handle->Get32Bits							 = _mt_fe_read32_tc6930;

    mt_fe_dmd_tc6930_match_tuners(handle);

    return MtFeErr_Ok;
}


MT_FE_RET mt_fe_dmd_tc6930_reset_all(MT_FE_TC6930_Device_Handle handle)
{
    U8 demod_index = 0;


    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4000, 0x01, 0, 0);    //reset all i2c register

    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4747, 0x00, 14, 14);  //reset fw flag
    _mt_fe_tn_set_reg_16bit_tc6930_bits(0x4747, 0x00, 15, 15);  //reset poweron set flag

    handle->tuner_mcu_status = 0;     //reset fw flag

    for (demod_index = 0; demod_index < 3; demod_index++) {
        handle->demod_device[demod_index].tuner_settings.tuner_init_OK  =  0;    //reset init flag
        mt_fe_dmd_tc6930_init(handle, demod_index);
    }

    return MtFeErr_Ok;
}


MT_FE_RET mt_fe_dmd_tc6930_check_4_reset_all(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    U32 ulData = 0;
    //U16 usData = 0;
    //U8  ucData = 0;


    // Check global reset
    _mt_fe_tc6930_get_reg_32bit((0x3F008 - 0x20000) / 4, &ulData);
    if (ulData != 0xFFFFFFFF) {
        _mt_fe_tc6930_set_reg_32bit((0x3F008 - 0x20000) / 4, 0x00000000);
        _mt_fe_tc6930_set_reg_32bit((0x3F008 - 0x20000) / 4, 0xFFFFFFFF);

        mt_fe_dmd_tc6930_reset_all(handle);

        return MtFeErr_Ok;
    }

    _mt_fe_tc6930_get_reg_32bit((0x3F03C - 0x20000) / 4, &ulData);
    if (((demod_index == 0) && ((ulData & 0x40) == 0)) ||
        ((demod_index == 1) && ((ulData & 0x80) == 0) &&
         (handle->demod_device[0].demod_type != MT_FE_DEMOD_TC6920) &&
         (handle->demod_device[0].demod_type != MT_FE_DEMOD_RC6800))) {
        mt_fe_dmd_tc6930_reset_all(handle);

        return MtFeErr_Ok;
    }

    _mt_fe_tc6930_get_reg_32bit(0x4608, &ulData);	// Firmware version number
    if ((ulData & 0xFF) == 0) {
        mt_fe_dmd_tc6930_reset_all(handle);

        return MtFeErr_Ok;
    }

    return MtFeErr_Ok;
}


MT_FE_RET mt_fe_dmd_tc6930_calibration(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 iMaxTimes)
{
    U8 tmp1, tmp2, tmp3, i, data;

    if ((handle->demod_device[demod_index].dev_index != 2) ||
        (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
        return MtFeErr_Ok;
    }


    handle->dmd_get_reg(handle, demod_index, 0x86, &data);
    data &= 0x77;
    handle->dmd_set_reg(handle, demod_index, 0x86, data); //not invert demod system clock


    handle->dmd_get_reg(handle, demod_index, 0x32, &tmp1);
    tmp1 |= 0x53;//53//5f
    handle->dmd_set_reg(handle, demod_index, 0x32, tmp1); //set clear lms_lock log

    handle->dmd_get_reg(handle, demod_index, 0x31, &tmp1);
    tmp1 |= 0x06;
    handle->dmd_set_reg(handle, demod_index, 0x31, tmp1); //enable calibration & ext control

    handle->dmd_get_reg(handle, demod_index, 0x33, &tmp1);
    tmp1 |= 0x43;//42-div2//43-div4//4b-div8
    handle->dmd_set_reg(handle, demod_index, 0x33, tmp1); //enable ext control & clock div4

    handle->dmd_get_reg(handle, demod_index, 0x34, &tmp1);
    tmp1 |= 0x40;//43//40
    handle->dmd_set_reg(handle, demod_index, 0x34, tmp1); //enable ext control

    for (i = 0; i < iMaxTimes; i++) {
        handle->dmd_get_reg(handle, demod_index, 0x30, &tmp2);
        tmp2 |= 0x0F;
        handle->dmd_set_reg(handle, demod_index, 0x30, tmp2); //reset calibration

        handle->mt_sleep(5);//wait 5ms

        handle->dmd_get_reg(handle, demod_index, 0x32, &tmp1);
        tmp1 &= 0x3F;
        handle->dmd_set_reg(handle, demod_index, 0x32, tmp1);//release clear lms_lock log

        tmp2 &= 0xF0;
        handle->dmd_set_reg(handle, demod_index, 0x30, tmp2);//release calibration

        handle->mt_sleep(10); //wait 10ms

        handle->dmd_get_reg(handle, demod_index, 0x39, &tmp3);  //judge if calibration is success
        if ((tmp3 >= 0x29) && (tmp3 <= 0x2F)) { //after calibration, reg 0x39 should be 0x29 - 0x2F
            break;
        }
    }

    if ((tmp3 >= 0x29) && (tmp3 <= 0x2F)) {
        mt_fe_print(("\tTC6930 calibration OK!\n"));
    } else {
        mt_fe_print(("\tTC6930 calibration failed!\n"));
    }

    handle->dmd_get_reg(handle, demod_index, 0x31, &tmp1);
    tmp1 &= 0x00;
    handle->dmd_set_reg(handle, demod_index, 0x31, tmp1); //disable calibration


    if (handle->demod_device[demod_index].on_board_settings.bReadCali) {
        /*Read Registers*/
        handle->mt_sleep(40); //wait 40ms

        for (i = 0x39; i <= 0x3F; i++) {
            handle->dmd_get_reg(handle, demod_index, i, &tmp1);
        }

        handle->dmd_get_reg(handle, demod_index, 0x36, &tmp2);
        tmp2 |= 0x02;
        handle->dmd_set_reg(handle, demod_index, 0x36, tmp2);

        for (i = 0; i < 28; i++) {
            handle->dmd_get_reg(handle, demod_index, 0x35, &tmp1);
        }

        tmp2 &= 0xFD;
        handle->dmd_set_reg(handle, demod_index, 0x36, tmp2);
    }

    handle->dmd_get_reg(handle, demod_index, 0x86, &data);
    data |= 0x08;
    handle->dmd_set_reg(handle, demod_index, 0x86, data);


    return MtFeErr_Ok;
}


MT_FE_RET mt_fe_dmd_tc6930_init(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    MT_FE_RET ret = MtFeErr_Ok;
    U8 data = 0; //, tmp1 = 0, i = 0, tmp2 = 0;
    U32 reg1 = 0, reg2 = 0, reg3 = 0;

    MT_FE_SUPPORTED_DEMOD demod_type;// = handle->demod_device[0].demod_type;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    demod_type = handle->demod_device[0].demod_type;

    demod_type = mt_fe_dmd_tc6930_check_chip_id(handle);

    if (handle->demod_device[demod_index].dev_index == 2) {
        if ((handle->demod_device[demod_index].on_board_settings.chip_mode == 2) && (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_M88DC2800)) {
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x2a);
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x6a);

            handle->dmd_set_reg(handle, demod_index, 0x80, 0x01);
        } else {
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x40);
        }


        if (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_CS8000_CAB) {
            //set C_RF_AGC input only	1F13C00C[11:8]=_1
            handle->Get32Bits(0xBF13C00C, &reg2);
            // bit[11:8] = _1
            reg2 &= 0xFFFFF0FF;
            reg2 |= 0x00000100;
            handle->Set32Bits(0xBF13C00C, reg2);


            handle->Get32Bits(0xBF5D00D0, &reg1);
            // bit[10:8] = _100
            reg1 &= 0xFFFFF8FF;
            reg1 |= 0x00000400;
            handle->Set32Bits(0xBF5D00D0, reg1);

            handle->Get32Bits(0xBF5D0098, &reg1);
            handle->Get32Bits(0xBF5D009C, &reg2);
            handle->Get32Bits(0xBF5D004C, &reg3);

            handle->Set32Bits(0xBF5D0098, 0xF000281F);
            handle->Set32Bits(0xBF5D009C, 0x00404040);
            handle->Set32Bits(0xBF5D004C, 0x0EA1BB01);
        }
    }

    handle->dmd_get_reg(handle, demod_index, 0x87, &data);
    data &= 0xF8;		// 1V
    if (handle->demod_device[demod_index].on_board_settings.iVppSel == 0) {	// 2V
        data |= 0x03;
    }
    handle->dmd_set_reg(handle, demod_index, 0x87, data);  //Set 1V or 2V Vdpp


    mt_fe_dmd_tc6930_calibration(handle, demod_index, 5);


    if (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_CS8000_CAB) {
        handle->Set32Bits(0xBF5D0098, reg1);
        handle->Set32Bits(0xBF5D009C, reg2);
        handle->Set32Bits(0xBF5D004C, reg3);
    }


    if (demod_index == 0) {
        //Ts0_data0 :	0x3f030[bit 16:18]
        //Ts0_CLK :		0x3f02c[bit  8:10]
        //Ts0_ERR :		0x3f030[bit 24:26]
        //Ts0_SYNC:		0x3f02c[bit 24:26]
        //Ts0_VAL:		0x3f02c[bit 16:18]

        _mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01, 18, 16);
        _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 10,  8);
        //_mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01, 26, 24);

        _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 26, 24);
        _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 18, 16);

        _mt_fe_tc6930_set_reg_32bit_bits((0x3F03C - 0x20000) / 4, 0x01,  6,  6);		// TS 0, pinmux
    } else if (demod_index == 1) {
        if ((handle->demod_device[0].demod_type != MT_FE_DEMOD_TC6920) &&
            (handle->demod_device[0].demod_type != MT_FE_DEMOD_RC6800)) {
            // not TC6920 and RC6800
            //Ts1_data0 :	0x3f028[bit  8:10]
            //Ts1_CLK :		0x3f030[bit  0: 2]
            //Ts1_ERR :		0x3f028[bit 24:26]
            //Ts1_SYNC:		0x3f028[bit 16:18]
            //Ts1_VAL:		0x3f028[bit  0: 2]

#if (MT_PLATFORM_SEL != 0)
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01, 10,  8);		// For Symphony: 0, others: 1
#else
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x00, 10,  8);		// For Symphony: 0, others: 1
#endif
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01,  2,  0);
            //_mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01, 26, 24);

            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01, 18, 16);
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01,  2,  0);

            _mt_fe_tc6930_set_reg_32bit_bits((0x3F03C - 0x20000) / 4, 0x01,  7,  7);		// TS 1, pinmux
        }
    }

    mt_fe_tn_init_tc6930(handle, demod_index);

    mt_fe_tn_set_tuner_index_tc6930(handle);

    handle->demod_device[demod_index].bInitOK = TRUE;

    return ret;
}


MT_FE_RET mt_fe_dmd_tc6930_reinit(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    MT_FE_RET ret = MtFeErr_Ok;
    //U8 tmp1 = 0, i = 0, tmp2 = 0;
    //U32 reg1 = 0, reg2 = 0, reg3 = 0;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    if (demod_index == 0) {
        //Ts0_data0 :	0x3f030[bit 16:18]
        //Ts0_CLK :		0x3f02c[bit  8:10]
        //Ts0_ERR :		0x3f030[bit 24:26]
        //Ts0_SYNC:		0x3f02c[bit 24:26]
        //Ts0_VAL:		0x3f02c[bit 16:18]

        _mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01, 18, 16);
        _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 10,  8);
        //_mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01, 26, 24);

        _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 26, 24);
        _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 18, 16);

        _mt_fe_tc6930_set_reg_32bit_bits((0x3F03C - 0x20000) / 4, 0x01,  6,  6);		// TS 0, pinmux
    } else if (demod_index == 1) {
        if ((handle->demod_device[0].demod_type != MT_FE_DEMOD_TC6920) &&
            (handle->demod_device[0].demod_type != MT_FE_DEMOD_RC6800)) {
            // not TC6920 and RC6800
            //Ts1_data0 :	0x3f028[bit  8:10]
            //Ts1_CLK :		0x3f030[bit  0: 2]
            //Ts1_ERR :		0x3f028[bit 24:26]
            //Ts1_SYNC:		0x3f028[bit 16:18]
            //Ts1_VAL:		0x3f028[bit  0: 2]

#if (MT_PLATFORM_SEL != 0)
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01, 10,  8);		// For Symphony: 0, others: 1
#else
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x00, 10,  8);		// For Symphony: 0, others: 1
#endif
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01,  2,  0);
            //_mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01, 26, 24);

            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01, 18, 16);
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F028 - 0x20000) / 4, 0x01,  2,  0);

            _mt_fe_tc6930_set_reg_32bit_bits((0x3F03C - 0x20000) / 4, 0x01,  7,  7);		// TS 1, pinmux
        }
    }

    mt_fe_tn_init_tc6930(handle, demod_index);

    mt_fe_tn_set_tuner_index_tc6930(handle);

    return ret;
}


/*	FUNCTIN:
**		mt_fe_dmd_tc6930_auto_QAM_debug
**
**	DESCRIPTION:
**		auto get qam, symbol rate, freq offset in  Symphony DVB-C
**		only used when debug
**	IN:
**		MT_FE_TC6930_Device_Handle handle
**		U8 auto_mode:
**			1: auto-qam only,
**			2: auto-qam and auto-symbol rate
**			3: auto-qam and auto-symbol rate and auto frequency offset
**	OUT:
**		U16 *auto_qam	 : QAM mode, 16, 32, 64, 128, 256
**		U32 *auto_symbol : symbol rate, unit: KBaud
**		S32 *auto_offset : frequency offset ,unit: KBaud
**
**	RETURN:
*/
MT_FE_RET mt_fe_dmd_tc6930_auto_QAM_debug(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 auto_mode, U16 *auto_qam, U32 *auto_symbol, S32 *auto_offset)
{
    U8	tmp1 = 0;
    S8	biggest_indicator = 0;
    U8	biggest_index = 0;
    U8	i = 0;
    S8	qam_indicator_value[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    U16	qam_mode = 0;
    U32	symbol_rate_12bit = 0;
    S32	symbol_rate = 0;  //KHz
    S32	frequency_offset_16bit = 0;
    S32	symbol_rate_error = 0;
    S32	frequency_offset = 0; //KHz
    U16	crystal_freq = 28800; //KHz
    S8	smallest_in_max_value = 0x7f;
    U32	count = 0;


    if ((handle->demod_device[demod_index].dev_index == 2) && (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_M88DC2800)) {
        return MtFeErr_NoSupportFunc;
    }

    //auto_mode = 1, do auto-qam only;
    //auto_mode = 2, do auto-qam and auto-symbol rate
    //auto_mode = 3, do auto-qam and auto-symbol rate and auto frequency offset

    handle->dmd_set_reg(handle, demod_index, 0x80, 0x01);
    handle->dmd_set_reg(handle, demod_index, 0xe8, 0x3a);
    handle->dmd_set_reg(handle, demod_index, 0xe5, 0x26);
    handle->dmd_set_reg(handle, demod_index, 0x0e, 0x24);
    handle->dmd_set_reg(handle, demod_index, 0x02, 0x18);
    handle->dmd_set_reg(handle, demod_index, 0xef, 0x44);
    handle->dmd_set_reg(handle, demod_index, 0x18, 0xc0);

    handle->dmd_set_reg(handle, demod_index, 0x53, 0x00);
    handle->dmd_set_reg(handle, demod_index, 0x59, 0x2d);
    handle->dmd_set_reg(handle, demod_index, 0xf9, 0x02);
    handle->dmd_set_reg(handle, demod_index, 0x01, 0x41);

    handle->dmd_set_reg(handle, demod_index, 0xf1, 0xff);
    handle->dmd_set_reg(handle, demod_index, 0xf0, 0xa3);
    handle->dmd_set_reg(handle, demod_index, 0x4c, 0x4c);

    switch (auto_mode) {
    case 1:
        handle->dmd_set_reg(handle, demod_index, 0x0c, 0x80);
        break;
    case 2:
        handle->dmd_set_reg(handle, demod_index, 0x0c, 0xc0);
        break;
    case 3:
        handle->dmd_set_reg(handle, demod_index, 0x0c, 0xe0);
        break;
    default:
        handle->dmd_set_reg(handle, demod_index, 0x0c, 0x80);
        break;
    }

    handle->dmd_set_reg(handle, demod_index, 0x80, 0x00);

    count = 0;
    do {
        handle->mt_sleep(5);

        count++;
        if (count > 200) {
            return MtFeErr_Fail;
        }
        handle->dmd_get_reg(handle, demod_index, 0x0c, &tmp1);
        tmp1 = (U8)(tmp1 & 0x10);
    } while (tmp1 == 0);

    for (i = 0; i < 10; i++) {
        handle->dmd_set_reg(handle, demod_index, 0x18, i);
        tmp1 = 0;
        handle->dmd_get_reg(handle, demod_index, 0x0d, &tmp1);
        qam_indicator_value[i] = tmp1;
    }

    biggest_indicator = qam_indicator_value[0];
    biggest_index = 0;
    for (i = 0; i < 10; i++) {
        if (qam_indicator_value[i] > biggest_indicator) {
            biggest_indicator = qam_indicator_value[i];
            biggest_index = i;
        }
    }

    if (biggest_indicator < smallest_in_max_value) {
        smallest_in_max_value = biggest_indicator;
    }
    switch (biggest_index) {
    case 0:
    case 5:
        if (biggest_indicator < 60) {
            biggest_index = 4;
        } else {
            biggest_index = 0;
        }
        break;

    case 1:
    case 6:
        if (biggest_indicator < 50) {
            biggest_index = 4;
        } else {
            biggest_index = 1;
        }
        break;

    case 2:
    case 7:
        if (biggest_indicator < 50) {
            biggest_index = 4;
        } else {
            biggest_index = 2;
        }
        break;

    case 3:
    case 8:
        if (biggest_indicator < 50) {
            biggest_index = 4;
        } else {
            biggest_index = 3;
        }
        break;

    default:
        biggest_index = 4;
        break;
    }

    switch (biggest_index) {
    case 0:
        qam_mode = 16;
        break;
    case 1:
        qam_mode = 32;
        break;
    case 2:
        qam_mode = 64;
        break;
    case 3:
        qam_mode = 128;
        break;
    case 4:
        qam_mode = 256;
        break;
    case 5:
        qam_mode = 16;
        break;
    case 6:
        qam_mode = 32;
        break;
    case 7:
        qam_mode = 64;
        break;
    case 8:
        qam_mode = 128;
        break;
    case 9:
        qam_mode = 256;
        break;
    default:
        qam_mode = 0;
        break;
    }

    tmp1 = 0;
    handle->dmd_get_reg(handle, demod_index, 0x19, &tmp1);
    symbol_rate_12bit = tmp1 << 4;
    tmp1 = 0;
    handle->dmd_get_reg(handle, demod_index, 0x1a, &tmp1);
    symbol_rate_12bit = symbol_rate_12bit | (tmp1 & 0x0f);

    symbol_rate = symbol_rate_12bit * crystal_freq / 4096;

    tmp1 = 0;
    handle->dmd_get_reg(handle, demod_index, 0x1b, &tmp1);
    frequency_offset_16bit = tmp1 << 8;
    tmp1 = 0;
    handle->dmd_get_reg(handle, demod_index, 0x1c, &tmp1);
    frequency_offset_16bit = frequency_offset_16bit | tmp1;
    if (frequency_offset_16bit > 32767) {
        frequency_offset_16bit = frequency_offset_16bit - 65536;
    }
    frequency_offset = frequency_offset_16bit * crystal_freq / 65536;

    tmp1 = 0;
    handle->dmd_get_reg(handle, demod_index, 0x51, &tmp1);
    symbol_rate_error = tmp1;

    if (symbol_rate_error >= 128) {
        symbol_rate_error = symbol_rate_error - 256;
    }
    symbol_rate = symbol_rate - symbol_rate_error * symbol_rate / 2000;

    *auto_qam = qam_mode;
    *auto_symbol = (U32)symbol_rate;
    *auto_offset = (S32)frequency_offset;

    return MtFeErr_Ok;
}


MT_FE_RET mt_fe_dmd_tc6930_connect(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz, U16 symbol_rate_KSs, U16 qam, U8 inverted)
{
    MT_FE_RET	ret = MtFeErr_Ok;
    U8		tmp1, tmp2, tmp;
    U16		iBandWidth;

    int	bAutoMode = FALSE;
    U16		iQAMMode = 64;
    U32		iSymbolRateKSs = 6888;
    S32		iFreqOffsetKHz = 0;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    mt_fe_dmd_tc6930_check_4_reset_all(handle, demod_index);

    handle->demod_device[demod_index].channel_param.freq_KHz	 = freq_KHz;
    handle->demod_device[demod_index].channel_param.sym_KSs		 = symbol_rate_KSs;
    handle->demod_device[demod_index].channel_param.qam_code	 = qam;
    handle->demod_device[demod_index].channel_param.inverted	 = inverted;

    handle->demod_device[demod_index].perf_optimized = 0;

    if ((qam == 16) || (qam == 32) || (qam == 64) || (qam == 128) || (qam == 256)) {
        bAutoMode = FALSE;
    } else {
        bAutoMode = TRUE;
    }

    if (symbol_rate_KSs == 0) {
        bAutoMode = TRUE;
    }

    if ((handle->demod_device[demod_index].dev_index == 2) && (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_M88DC2800)) {
        bAutoMode = FALSE;

        if (symbol_rate_KSs == 0) {
            symbol_rate_KSs = 6888;
        }
    }

    mt_fe_dmd_tc6930_optimize_tuner(handle, demod_index);

    mt_fe_dmd_tc6930_reinit(handle, demod_index);

#if (MT_PLATFORM_SEL == 0)
    {
        if ((demod_index == 0) && (handle->demod_device[demod_index].tuner_settings.tuner_custom_cfg == 1)) {
            //Ts0_data0 :	0x3f030[bit 16:18]
            //Ts0_CLK :		0x3f02c[bit  8:10]

            if (((freq_KHz < 173000) && (freq_KHz > 167000)) ||
                ((freq_KHz < 293000) && (freq_KHz > 287000))) {
                _mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x00, 18, 16);
                _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x00, 10,  8);
            } else {
                _mt_fe_tc6930_set_reg_32bit_bits((0x3F030 - 0x20000) / 4, 0x01, 18, 16);
                _mt_fe_tc6930_set_reg_32bit_bits((0x3F02C - 0x20000) / 4, 0x01, 10,  8);
            }
        }
    }
#endif

    if (bAutoMode) {
        handle->dmd_get_reg(handle, demod_index, 0xb3, &tmp);
        handle->dmd_set_reg(handle, demod_index, 0xb3, 0x00);
    }

    iBandWidth = symbol_rate_KSs + symbol_rate_KSs * 3 / 20;

    if (handle->demod_device[demod_index].dev_index < 2) {
        _mt_fe_dsp_get_reg_tc6930(demod_index, 0xF5, &tmp1);
        if ((iBandWidth < 6000) && (iBandWidth != 0)) {
            tmp1 |= 0x40;
        } else {
            tmp1 &= ~0x40;
        }
        _mt_fe_dsp_set_reg_tc6930(demod_index, 0xF5, tmp1);
    }

    handle->dmd_get_reg(handle, demod_index, 0x86, &tmp1);
    handle->dmd_get_reg(handle, demod_index, 0x87, &tmp2);

    handle->dmd_set_reg(handle, demod_index, 0x80, 0x01);

    if (handle->demod_device[demod_index].tuner_settings.tuner_init_OK == FALSE) {
        mt_fe_tn_init_tc6930(handle, demod_index);
    }

    //if (handle->demod_device[demod_index].dev_index < 2)
    {
        if ((iBandWidth < 6000) && (iBandWidth != 0)) {
            handle->demod_device[demod_index].tuner_settings.tuner_bandwidth = 6000;
        } else {
            handle->demod_device[demod_index].tuner_settings.tuner_bandwidth = 8000;
        }
    }


    mt_fe_tn_set_freq_tc6930(handle, demod_index, freq_KHz);


    if (handle->demod_device[demod_index].dev_index == 2) {
        if ((handle->demod_device[demod_index].on_board_settings.chip_mode == 2) && (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_M88DC2800)) {
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x2a);
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x6a);

            handle->dmd_set_reg(handle, demod_index, 0x80, 0x01);
        }
    } else {
        //handle->dmd_set_reg(handle, demod_index, 0x84, 0x00);
        //handle->dmd_set_reg(handle, demod_index, 0x84, 0x40);

        // 0x3f008 bit[28: 27]: 0 enable;
        // bit[28] - demodulator1, bit[27] - demodulator0

        if (demod_index == 0) {
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F008 - 0x20000) / 4, 0x00, 27, 27);
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F008 - 0x20000) / 4, 0x01, 27, 27);
        } else if (demod_index == 1) {
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F008 - 0x20000) / 4, 0x00, 28, 28);
            _mt_fe_tc6930_set_reg_32bit_bits((0x3F008 - 0x20000) / 4, 0x01, 28, 28);
        }
    }

    if (bAutoMode) {
        handle->demod_device[demod_index].channel_param.sym_KSs  = 6875;
        handle->demod_device[demod_index].channel_param.qam_code = 64;

        _mt_fe_dmd_tc6930_set_demod(handle, demod_index);
        _mt_fe_dmd_tc6930_set_symbol_rate(handle, demod_index, 6875, handle->demod_device[demod_index].on_board_settings.xtal_KHz);
        _mt_fe_dmd_tc6930_set_QAM(handle, demod_index, 64);
        _mt_fe_dmd_tc6930_set_tx_mode(handle, demod_index, inverted, 0);		/* J83A */
        _mt_fe_dmd_tc6930_set_ts_output(handle, demod_index, handle->demod_device[demod_index].ts_output_settings.output_mode);
        _mt_fe_dmd_tc6930_set_demod_appendix(handle, demod_index, 64);

        ret = mt_fe_dmd_tc6930_auto_QAM_debug(handle, demod_index, 3, &iQAMMode, &iSymbolRateKSs, &iFreqOffsetKHz);

        handle->dmd_set_reg(handle, demod_index, 0xe8, 0x3b);
        handle->dmd_set_reg(handle, demod_index, 0xe5, 0xa6);
        handle->dmd_set_reg(handle, demod_index, 0x0e, 0xa5);
        //handle->dmd_set_reg(handle, demod_index, 0x02, 0x60);
        handle->dmd_set_reg(handle, demod_index, 0xef, 0x88);
        handle->dmd_set_reg(handle, demod_index, 0x18, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x53, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0x59, 0x30);
        handle->dmd_set_reg(handle, demod_index, 0xf9, 0x0a);
        handle->dmd_set_reg(handle, demod_index, 0x01, 0x62);
        handle->dmd_set_reg(handle, demod_index, 0xf1, 0xff);
        handle->dmd_set_reg(handle, demod_index, 0xf0, 0x83);
        handle->dmd_set_reg(handle, demod_index, 0x4c, 0x0c);
        handle->dmd_set_reg(handle, demod_index, 0x0c, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0xb3, tmp);

        if (ret == MtFeErr_Ok) {
            handle->demod_device[demod_index].channel_param.qam_code = iQAMMode;
            handle->demod_device[demod_index].channel_param.sym_KSs = (U16)iSymbolRateKSs;
            //handle->m_device_ctt2.tp_cfg.freq_KHz = handle->m_device_ctt2.input_params.input_freq_kHz - iFreqOffsetKHz;

            //handle->m_device_ctt2.input_params.input_freq_kHz -= iFreqOffsetKHz;

            symbol_rate_KSs = iSymbolRateKSs;
            qam = iQAMMode;
        }

        if (symbol_rate_KSs == 0) {
            symbol_rate_KSs = 6875;
        }
    }

    _mt_fe_dmd_tc6930_set_demod(handle, demod_index);
    _mt_fe_dmd_tc6930_set_symbol_rate(handle, demod_index, symbol_rate_KSs, handle->demod_device[demod_index].on_board_settings.xtal_KHz);
    _mt_fe_dmd_tc6930_set_QAM(handle, demod_index, qam);
    _mt_fe_dmd_tc6930_set_tx_mode(handle, demod_index, inverted, 0);		/* J83A */
    _mt_fe_dmd_tc6930_set_ts_output(handle, demod_index, handle->demod_device[demod_index].ts_output_settings.output_mode);
    _mt_fe_dmd_tc6930_set_demod_appendix(handle, demod_index, qam);


    tmp1 &= 0x7F;
    tmp2 &= 0x07;

    handle->dmd_set_reg(handle, demod_index, 0x86, tmp1);
    handle->dmd_set_reg(handle, demod_index, 0x87, tmp2);


    mt_fe_dmd_tc6930_soft_reset(handle, demod_index);

    handle->mt_sleep(10);

#if 0
    {
        U32 data = 0;

        _mt_fe_tc6930_get_reg_32bit(0x7C04, &data);

        mt_fe_print(("0x7C04 = 0x%04x\n", data));
    }
#endif

    return ret;
}


/***********************************************
 Initialize the internal registers in M88DC2000
************************************************/
MT_FE_RET _mt_fe_dmd_tc6930_set_demod(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    if ((handle->demod_device[demod_index].dev_index < 2) || (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_CS8000_CAB)) {
        handle->dmd_set_reg(handle, demod_index, 0x05, 0x0D);
        handle->dmd_set_reg(handle, demod_index, 0x36, 0x80);
        handle->dmd_set_reg(handle, demod_index, 0x43, 0x40);

        handle->dmd_set_reg(handle, demod_index, 0x61, 0x40);

        handle->dmd_set_reg(handle, demod_index, 0x90, 0x06);
        handle->dmd_set_reg(handle, demod_index, 0xDE, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0xA0, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0xDF, 0x81);
        handle->dmd_set_reg(handle, demod_index, 0xFA, 0x40);
        handle->dmd_set_reg(handle, demod_index, 0x37, 0x10);

        handle->dmd_set_reg(handle, demod_index, 0xF2, 0x9C);
        handle->dmd_set_reg(handle, demod_index, 0xF3, 0x40);

        handle->dmd_set_reg(handle, demod_index, 0x30, 0xFF);

        handle->dmd_set_reg(handle, demod_index, 0x31, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x32, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x33, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x35, 0x32);

        handle->dmd_set_reg(handle, demod_index, 0x39, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x3A, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0xF1, 0xFF);

        handle->dmd_set_reg(handle, demod_index, 0xF5, 0x40);
        handle->dmd_set_reg(handle, demod_index, 0x42, 0x24);

        handle->dmd_set_reg(handle, demod_index, 0x92, 0x7F);

        handle->dmd_set_reg(handle, demod_index, 0x2B, 0x33);

        handle->dmd_set_reg(handle, demod_index, 0x2E, 0x80);

        handle->dmd_set_reg(handle, demod_index, 0x2D, 0xFF);

        handle->dmd_set_reg(handle, demod_index, 0xA4, 0xFF);
        handle->dmd_set_reg(handle, demod_index, 0xA3, 0x0D);

        handle->dmd_set_reg(handle, demod_index, 0xF6, 0x4E);
        handle->dmd_set_reg(handle, demod_index, 0xF7, 0x20);

        handle->dmd_set_reg(handle, demod_index, 0x14, 0x08);

        handle->dmd_set_reg(handle, demod_index, 0x10, 0xFF);
        handle->dmd_set_reg(handle, demod_index, 0x11, 0x00);

        handle->dmd_set_reg(handle, demod_index, 0x13, 0x23);
        handle->dmd_set_reg(handle, demod_index, 0x60, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x69, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x6A, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0xE0, 0x75);

        handle->dmd_set_reg(handle, demod_index, 0x4E, 0xD8);
        handle->dmd_set_reg(handle, demod_index, 0x88, 0x80);
        handle->dmd_set_reg(handle, demod_index, 0x52, 0x79);
        handle->dmd_set_reg(handle, demod_index, 0x53, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0x59, 0x30);
        handle->dmd_set_reg(handle, demod_index, 0x5E, 0x02);
        handle->dmd_set_reg(handle, demod_index, 0x5F, 0x0F);
        handle->dmd_set_reg(handle, demod_index, 0x71, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0x72, 0x12);
        handle->dmd_set_reg(handle, demod_index, 0x73, 0x12);
    } else {
        U8 RegE3H, RegE4H;

        if (handle == NULL) {
            return MtFeErr_Uninit;
        }

        handle->dmd_set_reg(handle, demod_index, 0x00, 0x48);
        handle->dmd_set_reg(handle, demod_index, 0x01, 0x09);
        handle->dmd_set_reg(handle, demod_index, 0xFB, 0x0A);
        handle->dmd_set_reg(handle, demod_index, 0xFC, 0x0B);
        handle->dmd_set_reg(handle, demod_index, 0x02, 0x0B);
        handle->dmd_set_reg(handle, demod_index, 0x03, 0x18);
        handle->dmd_set_reg(handle, demod_index, 0x05, 0x0D);
        handle->dmd_set_reg(handle, demod_index, 0x36, 0x80);
        handle->dmd_set_reg(handle, demod_index, 0x43, 0x40);
        handle->dmd_set_reg(handle, demod_index, 0x55, 0x7A);
        handle->dmd_set_reg(handle, demod_index, 0x56, 0xD9);
        handle->dmd_set_reg(handle, demod_index, 0x57, 0xDF);
        handle->dmd_set_reg(handle, demod_index, 0x58, 0x39);
        handle->dmd_set_reg(handle, demod_index, 0x5A, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x5C, 0x71);
        handle->dmd_set_reg(handle, demod_index, 0x5D, 0x23);

        if (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) {
            handle->dmd_set_reg(handle, demod_index, 0x86, 0x40);
        }

        handle->dmd_set_reg(handle, demod_index, 0xF9, 0x08);
        handle->dmd_set_reg(handle, demod_index, 0x61, 0x40);
        handle->dmd_set_reg(handle, demod_index, 0x62, 0x0A);
        handle->dmd_set_reg(handle, demod_index, 0x90, 0x06);
        handle->dmd_set_reg(handle, demod_index, 0xDE, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0xA0, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0xDF, 0x81);
        handle->dmd_set_reg(handle, demod_index, 0xFA, 0x40);
        handle->dmd_set_reg(handle, demod_index, 0x37, 0x10);
        handle->dmd_set_reg(handle, demod_index, 0xF0, 0x40);
        handle->dmd_set_reg(handle, demod_index, 0xF2, 0x9C);
        handle->dmd_set_reg(handle, demod_index, 0xF3, 0x40);

        handle->dmd_get_reg(handle, demod_index, 0xE3, &RegE3H);
        handle->dmd_get_reg(handle, demod_index, 0xE4, &RegE4H);

        handle->dmd_set_reg(handle, demod_index, 0x30, 0xFF);

        handle->dmd_set_reg(handle, demod_index, 0x31, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x32, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x33, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x35, 0x32);

        if (((RegE3H & 0xC0) == 0x00) && ((RegE4H & 0xC0) == 0x00)) {
            handle->dmd_set_reg(handle, demod_index, 0x40, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x41, 0x10);
            handle->dmd_set_reg(handle, demod_index, 0xF1, 0x02);
            handle->dmd_set_reg(handle, demod_index, 0xF4, 0x04);
            handle->dmd_set_reg(handle, demod_index, 0xF5, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x42, 0x14);
            if (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) {
                handle->dmd_set_reg(handle, demod_index, 0xE1, 0x25);
            }
        } else if (((RegE3H & 0xC0) == 0x80) && ((RegE4H & 0xC0) == 0x40)) {
            handle->dmd_set_reg(handle, demod_index, 0x39, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x3A, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x40, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x41, 0x10);
            handle->dmd_set_reg(handle, demod_index, 0xF1, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF4, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF5, 0x40);
            handle->dmd_set_reg(handle, demod_index, 0x42, 0x14);
            if (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) {
                handle->dmd_set_reg(handle, demod_index, 0xE1, 0x25);
            }
        } else if (((RegE3H == 0x80) || (RegE3H == 0x81)) && ((RegE4H == 0x80) || (RegE4H == 0x81))) {
            handle->dmd_set_reg(handle, demod_index, 0x39, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x3A, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF1, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF4, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF5, 0x40);
            handle->dmd_set_reg(handle, demod_index, 0x42, 0x24);
            if (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) {
                handle->dmd_set_reg(handle, demod_index, 0xE1, 0x25);
            }

            handle->dmd_set_reg(handle, demod_index, 0x92, 0x7F);
            handle->dmd_set_reg(handle, demod_index, 0x93, 0x91);
            handle->dmd_set_reg(handle, demod_index, 0x95, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x2B, 0x33);
            handle->dmd_set_reg(handle, demod_index, 0x2A, 0x2A);
            handle->dmd_set_reg(handle, demod_index, 0x2E, 0x80);
            handle->dmd_set_reg(handle, demod_index, 0x25, 0x25);
            handle->dmd_set_reg(handle, demod_index, 0x2D, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0x26, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0x27, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x24, 0x25);
            handle->dmd_set_reg(handle, demod_index, 0xA4, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0xA3, 0x0D);
        } else {
            handle->dmd_set_reg(handle, demod_index, 0x39, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x3A, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF1, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF4, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0xF5, 0x40);
            handle->dmd_set_reg(handle, demod_index, 0x42, 0x24);
            if (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) {
                handle->dmd_set_reg(handle, demod_index, 0xE1, 0x27);
            }

            handle->dmd_set_reg(handle, demod_index, 0x92, 0x7F);
            handle->dmd_set_reg(handle, demod_index, 0x93, 0x91);
            handle->dmd_set_reg(handle, demod_index, 0x95, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x2B, 0x33);
            handle->dmd_set_reg(handle, demod_index, 0x2A, 0x2A);
            handle->dmd_set_reg(handle, demod_index, 0x2E, 0x80);
            handle->dmd_set_reg(handle, demod_index, 0x25, 0x25);
            handle->dmd_set_reg(handle, demod_index, 0x2D, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0x26, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0x27, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x24, 0x25);
            handle->dmd_set_reg(handle, demod_index, 0xA4, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0xA3, 0x10);
        }

        handle->dmd_set_reg(handle, demod_index, 0xF6, 0x4E);
        handle->dmd_set_reg(handle, demod_index, 0xF7, 0x20);
        handle->dmd_set_reg(handle, demod_index, 0x89, 0x02);
        handle->dmd_set_reg(handle, demod_index, 0x14, 0x08);
        handle->dmd_set_reg(handle, demod_index, 0x6F, 0x0D);
        handle->dmd_set_reg(handle, demod_index, 0x10, 0xFF);
        handle->dmd_set_reg(handle, demod_index, 0x11, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x12, 0x30);
        handle->dmd_set_reg(handle, demod_index, 0x13, 0x23);
        handle->dmd_set_reg(handle, demod_index, 0x60, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x69, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x6A, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0xE0, 0x75);

        if (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) {
            handle->dmd_set_reg(handle, demod_index, 0x8D, 0x29);
        }

        handle->dmd_set_reg(handle, demod_index, 0x4E, 0xD8);
        handle->dmd_set_reg(handle, demod_index, 0x88, 0x80);
        handle->dmd_set_reg(handle, demod_index, 0x52, 0x79);
        handle->dmd_set_reg(handle, demod_index, 0x53, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0x59, 0x30);
        handle->dmd_set_reg(handle, demod_index, 0x5E, 0x02);
        handle->dmd_set_reg(handle, demod_index, 0x5F, 0x0F);
        handle->dmd_set_reg(handle, demod_index, 0x71, 0x03);
        handle->dmd_set_reg(handle, demod_index, 0x72, 0x12);
        handle->dmd_set_reg(handle, demod_index, 0x73, 0x12);

    }

    return MtFeErr_Ok;
}

/***********************************************
   Set symbol rate
   sym:		symbol rate, unit: KBaud
   xtal:	unit, KHz
************************************************/
MT_FE_RET _mt_fe_dmd_tc6930_set_symbol_rate(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 sym, U32 xtal)
{
    MT_FE_RET ret = MtFeErr_Ok;

    U8	value;
    U8	reg6FH, reg12H;
    //DB	fValue;
    U32	fValue = 0;
    U32	dwValue;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    //fValue  = 4294967296.0 * (sym + 10) / xtal;
    fValue = ((((sym + 10) << 16) / xtal) << 16) + ((((sym + 10) << 16) % xtal) << 16) / xtal;
    dwValue = (U32)fValue;
    handle->dmd_set_reg(handle, demod_index, 0x58, (U8)((dwValue >> 24) & 0xff));
    handle->dmd_set_reg(handle, demod_index, 0x57, (U8)((dwValue >> 16) & 0xff));
    handle->dmd_set_reg(handle, demod_index, 0x56, (U8)((dwValue >>  8) & 0xff));
    handle->dmd_set_reg(handle, demod_index, 0x55, (U8)((dwValue >>  0) & 0xff));


    //fValue = 2048.0 * xtal / sym;
    fValue = 2048 * xtal / sym;
    dwValue = (U32)fValue;
    handle->dmd_set_reg(handle, demod_index, 0x5D, (U8)((dwValue >> 8) & 0xff));
    handle->dmd_set_reg(handle, demod_index, 0x5C, (U8)((dwValue >> 0) & 0xff));


    if ((handle->demod_device[demod_index].dev_index == 2) && (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
        handle->dmd_get_reg(handle, demod_index, 0x5A, &value);
        if (((dwValue >> 16) & 0x0001) == 0) {
            value &= 0x7F;
        } else {
            value |= 0x80;
        }
        handle->dmd_set_reg(handle, demod_index, 0x5A, value);

        handle->dmd_get_reg(handle, demod_index, 0x89, &value);
        if (sym <= 1800) {
            value |= 0x01;
        } else {
            value &= 0xFE;
        }
        handle->dmd_set_reg(handle, demod_index, 0x89, value);
    } else {
        if (((dwValue >> 16) & 0x0001) == 0) {
            value = 0x00;
        } else {
            value = 0x80;
        }
        handle->dmd_set_reg(handle, demod_index, 0x5A, value);


        if (sym <= 1800) {
            value = 0x03;
        } else {
            value = 0x02;
        }
        handle->dmd_set_reg(handle, demod_index, 0x89, value);
    }


    if (sym >= 6700) {
        reg6FH = 0x0D;
        reg12H = 0x30;
    } else if (sym >= 4000) {
        fValue = 22 * 4096 / sym;
        reg6FH = (U8)fValue;
        reg12H = 0x30;
    } else if (sym >= 2000) {
        fValue = 14 * 4096 / sym;
        reg6FH = (U8)fValue;
        reg12H = 0x20;
    } else {
        fValue = 7 * 4096 / sym;
        reg6FH = (U8)fValue;
        reg12H = 0x10;
    }

    handle->dmd_set_reg(handle, demod_index, 0x12, reg12H);


    if (sym < 3000) {
        handle->dmd_set_reg(handle, demod_index, 0x6E, 0x18);
    } else {
        handle->dmd_set_reg(handle, demod_index, 0x6E, 0x36);
    }

    return ret;
}


/***********************************************
   Set QAM mode
   Qam: QAM mode, 16, 32, 64, 128, 256
************************************************/
MT_FE_RET _mt_fe_dmd_tc6930_set_QAM(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U16 qam)
{
    MT_FE_RET ret = MtFeErr_Ok;

    U8 reg00H, reg4AH, reg4DH, reg8BH, reg8EH, regC2H, reg44H, reg4CH, reg74H;
    U8 value;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    handle->dmd_get_reg(handle, demod_index, 0xC2, &regC2H);
    regC2H &= 0xF8;

    switch (qam) {
    case 16:	// 16 QAM
        reg00H = 0x08;
        reg4AH = 0x0F;
        regC2H |= 0x02;
        reg44H = 0xAA;
        reg4CH = 0x0C;
        reg4DH = 0xF7;
        reg74H = 0x0E;
        reg8BH = 0x5A;
        reg8EH = 0xBD;
        handle->dmd_set_reg(handle, demod_index, 0x6E, 0x18);
        break;

    case 32:	// 32 QAM
        reg00H = 0x18;
        reg4AH = 0xFB;
        regC2H |= 0x02;
        reg44H = 0xAA;
        reg4CH = 0x0C;
        reg4DH = 0xF7;
        reg74H = 0x0E;
        reg8BH = 0x5A;
        reg8EH = 0xBD;
        handle->dmd_set_reg(handle, demod_index, 0x6E, 0x18);
        break;

    case 64:	// 64 QAM
        reg00H = 0x48;
        reg4AH = 0xCD;
        regC2H |= 0x02;
        reg44H = 0xAA;
        reg4CH = 0x0C;
        reg4DH = 0xF7;
        reg74H = 0x0E;
        reg8BH = 0x5A;
        reg8EH = 0xBD;
        break;

    case 128:	// 128 QAM
        reg00H = 0x28;
        reg4AH = 0xFF;
        regC2H |= 0x02;
        reg44H = 0xA9;
        reg4CH = 0x08;
        reg4DH = 0xF5;
        reg74H = 0x0E;
        reg8BH = 0x5B;
        reg8EH = 0x9D;
        break;

    case 256:	// 256 QAM
        if ((handle->demod_device[demod_index].dev_index == 2) &&
            (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) &&
            (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
            handle->dmd_set_reg(handle, demod_index, 0x90, 0x07);
            handle->dmd_set_reg(handle, demod_index, 0xA3, 0x0B);	//0x10
        }
        reg00H = 0x38;
        reg4AH = 0xCD;
        regC2H |= 0x02;
        reg44H = 0xA9;
        reg4CH = 0x08;
        reg4DH = 0xF5;
        reg74H = 0x0E;
        reg8BH = 0x5B;
        reg8EH = 0x9D;
        break;

    default:	// 64 QAM
        reg00H = 0x48;
        reg4AH = 0xCD;
        regC2H |= 0x02;
        reg44H = 0xAA;
        reg4CH = 0x0C;
        reg4DH = 0xF7;
        reg74H = 0x0E;
        reg8BH = 0x5A;
        reg8EH = 0xBD;
        break;
    }

    reg00H |= 0x80;
    handle->dmd_set_reg(handle, demod_index, 0x00, reg00H);

    handle->dmd_get_reg(handle, demod_index, 0x88, &value);
    value |= 0x08;
    handle->dmd_set_reg(handle, demod_index, 0x88, value);
    handle->dmd_set_reg(handle, demod_index, 0x4B, 0xFF);
    handle->dmd_set_reg(handle, demod_index, 0x4A, reg4AH);
    value &= 0xF7;
    handle->dmd_set_reg(handle, demod_index, 0x88, value);

    handle->dmd_set_reg(handle, demod_index, 0xC2, regC2H);
    handle->dmd_set_reg(handle, demod_index, 0x44, reg44H);
    handle->dmd_set_reg(handle, demod_index, 0x4C, reg4CH);
    handle->dmd_set_reg(handle, demod_index, 0x4D, reg4DH);
    handle->dmd_set_reg(handle, demod_index, 0x74, reg74H);

    if ((handle->demod_device[demod_index].dev_index == 2) &&
        (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) &&
        (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
        handle->dmd_set_reg(handle, demod_index, 0x8B, reg8BH);
        handle->dmd_set_reg(handle, demod_index, 0x8E, reg8EH);
    }

    return ret;
}


/***********************************************
   Set spectrum inversion and J83
   Inverted: 1, inverted; 0, not inverted
   J83: 0, J83A; 1, J83C
 ***********************************************/
MT_FE_RET _mt_fe_dmd_tc6930_set_tx_mode(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 inverted, U8 j83)
{
    MT_FE_RET ret = MtFeErr_Ok;

    U8 value = 0;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    if (inverted)	{
        value |= 0x08;    /*	spectrum inverted	*/
    }
    if (j83) {
        value |= 0x01;    /*	J83C				*/
    }

    ret = handle->dmd_set_reg(handle, demod_index, 0x83, value);

    return ret;
}


/***********************************************
   Set the type of MPEG/TS interface
   type: 1, serial format; 2, parallel format; 0, common interface
************************************************/
MT_FE_RET _mt_fe_dmd_tc6930_set_ts_output(MT_FE_TC6930_Device_Handle handle, U8 demod_index, MT_FE_TS_OUT_MODE ts_mode)
{
    MT_FE_RET ret = MtFeErr_Ok;

    U8 regC2H;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    if (demod_index > 2) {
        return MtFeErr_Param;
    }

    handle->demod_device[demod_index].ts_output_settings.output_mode = ts_mode;

    if (ts_mode == MtFeTsOutMode_Common) {
        if ((handle->demod_device[demod_index].dev_index == 2) &&
            (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) &&
            (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x6C);
        }
        handle->dmd_set_reg(handle, demod_index, 0xC0, 0x43);
        handle->dmd_set_reg(handle, demod_index, 0xE2, 0x06);
        handle->dmd_get_reg(handle, demod_index, 0xC2, &regC2H);
        regC2H &= 0xC7;
        regC2H |= 0x10;
        handle->dmd_set_reg(handle, demod_index, 0xC2, regC2H);
        handle->dmd_set_reg(handle, demod_index, 0xC1, 0x60);		//common interface
    } else if (ts_mode == MtFeTsOutMode_Serial) {
        if ((handle->demod_device[demod_index].dev_index == 2) &&
            (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) &&
            (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x6A);
        }
        handle->dmd_set_reg(handle, demod_index, 0xC0, 0x47);		//serial format
        handle->dmd_set_reg(handle, demod_index, 0xE2, 0x02);
        handle->dmd_get_reg(handle, demod_index, 0xC2, &regC2H);
        regC2H &= 0xC7;
        handle->dmd_set_reg(handle, demod_index, 0xC2, regC2H);

        if (demod_index == 0) {
            handle->dmd_set_reg(handle, demod_index, 0xC1, 0x00);	// 0x20

            //_mt_fe_tc6930_set_reg_32bit_bits((0x3f030 - 0x20000) >> 2, 0x01, 18, 16);
            //_mt_fe_tc6930_set_reg_32bit_bits((0x3f02c - 0x20000) >> 2, 0x01, 10,  8);
        } else if (demod_index == 1) {
            handle->dmd_set_reg(handle, demod_index, 0xC1, 0x00);

            //_mt_fe_tc6930_set_reg_32bit_bits((0x3f030 - 0x20000) >> 2, 0x01, 10,  8);
            //_mt_fe_tc6930_set_reg_32bit_bits((0x3f02c - 0x20000) >> 2, 0x01,  2,  0);
        } else {
            handle->dmd_set_reg(handle, demod_index, 0xC1, 0x00);
        }
    } else { //if (ts_mode == MtFeTsOutMode_Parallel)
        if ((handle->demod_device[demod_index].dev_index == 2) &&
            (handle->demod_device[demod_index].on_board_settings.chip_mode == 2) &&
            (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
            handle->dmd_set_reg(handle, demod_index, 0x84, 0x6C);
        }
        handle->dmd_set_reg(handle, demod_index, 0xC0, 0x43);		//parallel format
        handle->dmd_set_reg(handle, demod_index, 0xE2, 0x06);
        handle->dmd_get_reg(handle, demod_index, 0xC2, &regC2H);
        regC2H &= 0xC7;
        handle->dmd_set_reg(handle, demod_index, 0xC2, regC2H);
        handle->dmd_set_reg(handle, demod_index, 0xC1, 0x00);
    }

    return ret;
}

MT_FE_RET _mt_fe_dmd_tc6930_set_demod_appendix(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U16 qam)
{
    MT_FE_RET ret = MtFeErr_Ok;

    U8 tmp;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    if ((handle->demod_device[demod_index].dev_index < 2) || (handle->demod_device[demod_index].demod_type == MT_FE_DEMOD_CS8000_CAB)) {
        handle->dmd_set_reg(handle, demod_index, 0x01, 0x62);

        if ((qam == 256) || (qam == 128)) {
            handle->dmd_set_reg(handle, demod_index, 0x02, 0x10);
        } else {
            handle->dmd_set_reg(handle, demod_index, 0x02, 0x60);
        }

        handle->dmd_set_reg(handle, demod_index, 0x03, 0x38);

        handle->dmd_set_reg(handle, demod_index, 0x04, 0x88);

        handle->dmd_set_reg(handle, demod_index, 0x24, 0xFF);
        handle->dmd_set_reg(handle, demod_index, 0x25, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x26, 0xF8);
        handle->dmd_set_reg(handle, demod_index, 0x27, 0x80);


        if (qam == 16) {
            handle->dmd_set_reg(handle, demod_index, 0x62, 0x18);
        } else {
            handle->dmd_set_reg(handle, demod_index, 0x62, 0x28);
        }

        if ((qam == 128) || (qam == 32) || (qam == 16)) {
            handle->dmd_set_reg(handle, demod_index, 0x6C, 0x52);
        } else {
            handle->dmd_set_reg(handle, demod_index, 0x6C, 0x62);
        }

        handle->dmd_set_reg(handle, demod_index, 0x6D, 0xA3);
        handle->dmd_set_reg(handle, demod_index, 0x6F, 0x00);
        handle->dmd_set_reg(handle, demod_index, 0x75, 0x18);
        handle->dmd_set_reg(handle, demod_index, 0x7F, 0x71);

        tmp = 0x11;
        handle->dmd_set_reg(handle, demod_index, 0x93, tmp);

        handle->dmd_set_reg(handle, demod_index, 0xE5, 0xA6);
        handle->dmd_set_reg(handle, demod_index, 0xE8, 0x3B);
        handle->dmd_set_reg(handle, demod_index, 0xF0, 0x83);
        handle->dmd_set_reg(handle, demod_index, 0xF4, 0x0F);

        tmp = 0x0A;
        handle->dmd_set_reg(handle, demod_index, 0xF9, tmp);

        handle->dmd_set_reg(handle, demod_index, 0xFB, 0x44);

        handle->dmd_set_reg(handle, demod_index, 0xFC, 0xC1);
        handle->dmd_set_reg(handle, demod_index, 0xFD, 0x13);
        handle->dmd_set_reg(handle, demod_index, 0xFE, 0x50);

        tmp = 0x40;
        handle->dmd_set_reg(handle, demod_index, 0x95, tmp);

        handle->dmd_set_reg(handle, demod_index, 0x2A, 0x27);

        handle->dmd_get_reg(handle, demod_index, 0x86, &tmp);
        tmp |= 0x08;
        handle->dmd_set_reg(handle, demod_index, 0x86, tmp);

        handle->dmd_set_reg(handle, demod_index, 0xCE, 0x6D);
        handle->dmd_set_reg(handle, demod_index, 0xB8, 0x3C);
    } else {
        if (handle->demod_device[demod_index].on_board_settings.chip_mode == 0) {		// DC2800 mode
            handle->dmd_get_reg(handle, demod_index, 0x00, &tmp);
            tmp &= 0x7F;
            handle->dmd_set_reg(handle, demod_index, 0x00, tmp);

            handle->dmd_get_reg(handle, demod_index, 0x03, &tmp);
            tmp &= 0xDF;
            tmp |= 0x10;
            handle->dmd_set_reg(handle, demod_index, 0x03, tmp);

            handle->dmd_get_reg(handle, demod_index, 0x04, &tmp);
            tmp |= 0x02;
            handle->dmd_set_reg(handle, demod_index, 0x04, tmp);

            handle->dmd_get_reg(handle, demod_index, 0x4C, &tmp);
            tmp |= 0x40;
            handle->dmd_set_reg(handle, demod_index, 0x4C, tmp);

            handle->dmd_get_reg(handle, demod_index, 0x5F, &tmp);
            tmp &= 0x3F;
            handle->dmd_set_reg(handle, demod_index, 0x5F, tmp);

            handle->dmd_get_reg(handle, demod_index, 0x72, &tmp);
            tmp &= 0x7F;
            handle->dmd_set_reg(handle, demod_index, 0x72, tmp);

            handle->dmd_get_reg(handle, demod_index, 0x89, &tmp);
            tmp &= 0x0F;
            handle->dmd_set_reg(handle, demod_index, 0x89, tmp);

            handle->dmd_get_reg(handle, demod_index, 0xE5, &tmp);
            tmp &= 0x7F;
            handle->dmd_set_reg(handle, demod_index, 0xE5, tmp);

            handle->dmd_get_reg(handle, demod_index, 0xE8, &tmp);
            tmp &= 0xFE;
            handle->dmd_set_reg(handle, demod_index, 0xE8, tmp);

            if (qam == 256) {
                handle->dmd_set_reg(handle, demod_index, 0x7F, 0x71);
                handle->dmd_set_reg(handle, demod_index, 0xE8, 0x3B);
            }

            handle->dmd_get_reg(handle, demod_index, 0xF9, &tmp);
            tmp &= 0xFA;
            tmp |= 0x02;
            handle->dmd_set_reg(handle, demod_index, 0xF9, tmp);

            handle->dmd_get_reg(handle, demod_index, 0xFE, &tmp);
            tmp |= 0x08;
            handle->dmd_set_reg(handle, demod_index, 0xFE, tmp);


            handle->dmd_set_reg(handle, demod_index, 0x2A, 0x27);
        } else if (handle->demod_device[demod_index].on_board_settings.chip_mode == 1) {	// Jazz mode
            handle->dmd_get_reg(handle, demod_index, 0x00, &tmp);
            tmp |= 0x80;
            handle->dmd_set_reg(handle, demod_index, 0x00, tmp);

            handle->dmd_set_reg(handle, demod_index, 0x01, 0x42);
            handle->dmd_set_reg(handle, demod_index, 0x01, 0x62);

            if ((qam == 256) || (qam == 128)) {
                handle->dmd_set_reg(handle, demod_index, 0x02, 0x10);
            } else {
                handle->dmd_set_reg(handle, demod_index, 0x02, 0x60);
            }

            handle->dmd_get_reg(handle, demod_index, 0x03, &tmp);
            tmp |= 0x30;
            handle->dmd_set_reg(handle, demod_index, 0x03, tmp);

            handle->dmd_set_reg(handle, demod_index, 0x04, 0x88);

            handle->dmd_set_reg(handle, demod_index, 0x24, 0xFF);
            handle->dmd_set_reg(handle, demod_index, 0x25, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x26, 0xF9);
            handle->dmd_set_reg(handle, demod_index, 0x27, 0x80);

            handle->dmd_get_reg(handle, demod_index, 0x5F, &tmp);
            tmp &= 0x3F;
            handle->dmd_set_reg(handle, demod_index, 0x5F, tmp);


            if (qam == 16) {
                handle->dmd_set_reg(handle, demod_index, 0x62, 0x18);
            } else {
                handle->dmd_set_reg(handle, demod_index, 0x62, 0x28);
            }

            handle->dmd_get_reg(handle, demod_index, 0x00, &tmp);
            if (((tmp & 0x70) == 0x20) || ((tmp & 0x70) == 0x10) || ((tmp & 0x70) == 0x00)) {	// 128 QAM or 32QAM or 16QAM
                handle->dmd_set_reg(handle, demod_index, 0x6C, 0x52);
            } else {
                handle->dmd_set_reg(handle, demod_index, 0x6C, 0x62);
            }

            handle->dmd_set_reg(handle, demod_index, 0x6D, 0xA3);
            handle->dmd_set_reg(handle, demod_index, 0x6F, 0x00);
            handle->dmd_set_reg(handle, demod_index, 0x75, 0x18);
            handle->dmd_set_reg(handle, demod_index, 0x7F, 0x71);

            handle->dmd_get_reg(handle, demod_index, 0x93, &tmp);
            tmp &= 0x7F;
            handle->dmd_set_reg(handle, demod_index, 0x93, tmp);

            handle->dmd_set_reg(handle, demod_index, 0xE5, 0xA6);
            handle->dmd_set_reg(handle, demod_index, 0xE8, 0x3B);
            handle->dmd_set_reg(handle, demod_index, 0xF0, 0x83);
            handle->dmd_set_reg(handle, demod_index, 0xF4, 0x0F);
            handle->dmd_get_reg(handle, demod_index, 0xF9, &tmp);
            tmp &= 0xF8;
            tmp |= 0x02;
            handle->dmd_set_reg(handle, demod_index, 0xF9, tmp);

            handle->dmd_set_reg(handle, demod_index, 0xFB, 0x44);

            handle->dmd_set_reg(handle, demod_index, 0xFC, 0x91);
            handle->dmd_set_reg(handle, demod_index, 0xFD, 0x13);
            handle->dmd_set_reg(handle, demod_index, 0xFE, 0x50);

            handle->dmd_set_reg(handle, demod_index, 0x2A, 0x27);
        } else {
        }
    }

    return ret;
}


/***********************************************
 Soft reset M88DC2000
 Reset the internal status of each funtion block,
 not reset the registers.
************************************************/
MT_FE_RET mt_fe_dmd_tc6930_soft_reset(MT_FE_TC6930_Device_Handle handle, U8 demod_index)
{
    MT_FE_RET ret = MtFeErr_Ok;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    handle->dmd_set_reg(handle, demod_index, 0x80, 0x01);
    handle->dmd_set_reg(handle, demod_index, 0x82, 0x00);
    handle->mt_sleep(1);
    handle->dmd_set_reg(handle, demod_index, 0x80, 0x00);

    return ret;
}


MT_FE_RET mt_fe_dmd_tc6930_get_statistics(MT_FE_TC6930_Device_Handle handle, U8 demod_index,
        U32 *agc_lock,
        U32 *timing_lock,
        U32 *dagc_lock,
        U32 *carrier_lock,
        U32 *sync_lock,
        U32 *descrambler_lock,
        U32 *chip_lock
                                         )
{
    U8 tmp = 0;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    handle->dmd_get_reg(handle, demod_index, 0x43, &tmp);
    *agc_lock = ((tmp & 0x08) == 0x08) ? 1 : 0;

    handle->dmd_get_reg(handle, demod_index, 0x5E, &tmp);
    *timing_lock = ((tmp & 0x80) == 0x80) ? 1 : 0;

    handle->dmd_get_reg(handle, demod_index, 0x74, &tmp);
    *dagc_lock = ((tmp & 0x80) == 0x80) ? 1 : 0;

    handle->dmd_get_reg(handle, demod_index, 0xA3, &tmp);
    *carrier_lock = ((tmp & 0x80) == 0x80) ? 1 : 0;

    handle->dmd_get_reg(handle, demod_index, 0x85, &tmp);
    *sync_lock = ((tmp & 0x10) == 0x10) ? 1 : 0;

    *descrambler_lock = ((tmp & 0x01) == 0x01) ? 1 : 0;

    if ((handle->demod_device[demod_index].dev_index == 2) && (handle->demod_device[demod_index].demod_type != MT_FE_DEMOD_CS8000_CAB)) {
        if (handle->demod_device[demod_index].on_board_settings.chip_mode != 1) {	// 2: old DC2800	0: new DC2800
            *chip_lock = ((tmp & 0x08) == 0x08) ? 1 : 0;
        } else {			// 1: new Jazz
            *chip_lock = ((tmp & 0x11) == 0x11) ? 1 : 0;
        }
    } else {
        *chip_lock = ((tmp & 0x11) == 0x11) ? 1 : 0;
    }

    return MtFeErr_Ok;
}

/***********************************************
   Get lock status
   returned 1 when locked;0 when unlocked
************************************************/
MT_FE_RET mt_fe_dmd_tc6930_get_lock_state(MT_FE_TC6930_Device_Handle handle, U8 demod_index, MT_FE_LOCK_STATE *lock_status)
{
    MT_FE_RET ret = MtFeErr_Ok;
    S32 level = 0;
    U8 tmp = 0, val = 0;

    //U8 reg80H;

    *lock_status = MtFeLockState_Unlocked;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    if (handle->demod_device[demod_index].perf_optimized == 0) {	// not optimized yet
        handle->dmd_get_reg(handle, demod_index, 0xCE, &tmp);
        if ((tmp & 0x01) == 0x01) {
            level = mt_fe_tn_get_signal_strength_tc6930(handle, demod_index) + 107;

            if ((level < 65) && (level > 55)) {
#if 1	// 20220809
                if ((handle->demod_device[demod_index].channel_param.sym_KSs < 6000) &&
                    (handle->demod_device[demod_index].channel_param.qam_code == 64)) {
                    handle->dmd_get_reg(handle, demod_index, 0x95, &val);
                    val |= 0x80;
                    handle->dmd_set_reg(handle, demod_index, 0x95, val);
                }
#else
                _mt_fe_tn_set_reg_16bit_tc6930(0x4081, 0xb516);
#endif

                handle->demod_device[demod_index].perf_optimized = 1;	// optimized
            }
        }
    }

#if 0
    if (reg80H < 0x06) {
        U8 regDFH, reg91H, reg43H;

        handle->dmd_get_reg(handle, demod_index, 0xDF, &regDFH);
        handle->dmd_get_reg(handle, demod_index, 0x91, &reg91H);
        handle->dmd_get_reg(handle, demod_index, 0x43, &reg43H);

        if (((regDFH & 0x80) == 0x80) &&
            ((reg91H & 0x23) == 0x03) &&
            ((reg43H & 0x08) == 0x08)
           ) {
            *lock_status = MtFeLockState_Locked;
        } else {
            *lock_status = MtFeLockState_Unlocked;
        }
    } else
#endif
    {
        U8 reg85H;
        U8 i;

        for (i = 0; i < 10; i++) {
            handle->dmd_get_reg(handle, demod_index, 0x85, &reg85H);
            if ((reg85H & 0x11) != 0x11) {
                *lock_status = MtFeLockState_Unlocked;
                break;
            }

            *lock_status = MtFeLockState_Locked;
        }
    }

    return ret;
}


/***********************************************
   Get BER (bit error rate)
************************************************/
MT_FE_RET mt_fe_dmd_tc6930_get_ber(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 *error_bits, U32 *total_bits)
{
    MT_FE_RET ret = MtFeErr_Ok;

    U16	tmp;

    U8 regA0H, regA1H, regA2H, n_byte;


    MT_FE_LOCK_STATE lock_status = MtFeLockState_Unlocked;


    if (handle == NULL) {
        return MtFeErr_Uninit;
    }


    mt_fe_dmd_tc6930_get_lock_state(handle, demod_index, &lock_status);


    *error_bits = 0;
    *total_bits = 33554432;


    if (lock_status != MtFeLockState_Locked) {
        return MtFeErr_UnLock;
    }


    handle->dmd_get_reg(handle, demod_index, 0xA0, &regA0H);
    n_byte = regA0H & 0x07;

    if ((regA0H & 0x80) != 0x80) {
        handle->dmd_get_reg(handle, demod_index, 0xA1, &regA1H);
        handle->dmd_get_reg(handle, demod_index, 0xA2, &regA2H);

        tmp = (regA2H << 8) + regA1H;

        *error_bits = tmp;
        *total_bits = 1 << (n_byte * 2 + 15);

        handle->dmd_set_reg(handle, demod_index, 0xA0, n_byte);
        n_byte |= 0x80;
        handle->dmd_set_reg(handle, demod_index, 0xA0, n_byte);
    } else {
        ret = MtFeErr_Fail;
    }


    return ret;
}

/***********************************************
   Get SNR (signal noise ratio)
************************************************/
MT_FE_RET mt_fe_dmd_tc6930_get_snr(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 *signal_snr)
{
    MT_FE_RET ret = MtFeErr_Ok;

    const U32 mes_log[] = {
        0,	  3010,	  4771,	  6021,	  6990,	  7781,	  8451,	  9031,	  9542,	 10000,
        10414,	 10792,	 11139,	 11461,	 11761,	 12041,	 12304,	 12553,	 12788,	 13010,
        13222,	 13424,	 13617,	 13802,	 13979,	 14150,	 14314,	 14472,	 14624,	 14771,
        14914,	 15052,	 15185,	 15315,	 15441,	 15563,	 15682,	 15798,	 15911,	 16021,
        16128,	 16232,	 16335,	 16435,	 16532,	 16628,	 16721,	 16812,	 16902,	 16990,
        17076,	 17160,	 17243,	 17324,	 17404,	 17482,	 17559,	 17634,	 17709,	 17782,
        17853,	 17924,	 17993,	 18062,	 18129,	 18195,	 18261,	 18325,	 18388,	 18451,
        18513,	 18573,	 18633,	 18692,	 18751,	 18808,	 18865,	 18921,	 18976,	 19031
    };
    U32	snr;
    U8	i;
    U32	mse;


    U8 reg91H, reg08H, reg07H;

    U16 qam;


    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    qam = handle->demod_device[demod_index].channel_param.qam_code;

    *signal_snr = 0;

    handle->dmd_get_reg(handle, demod_index, 0x91, &reg91H);

    if ((reg91H & 0x23) != 0x03) {
        return MtFeErr_Fail;
    }



    mse = 0;
    for (i = 0; i < 30; i++) {
        handle->dmd_get_reg(handle, demod_index, 0x08, &reg08H);
        handle->dmd_get_reg(handle, demod_index, 0x07, &reg07H);

        mse += (reg08H << 8) + reg07H;
    }
    mse /= 30;
    if (mse > 80) {
        mse = 80;
    }



    switch (qam) {
    case 16:
        snr = 34080;
        break;	/*	16QAM	*/
    case 32:
        snr = 37600;
        break;	/*	32QAM	*/
    case 64:
        snr = 40310;
        break;	/*	64QAM	*/
    case 128:
        snr = 43720;
        break;	/*	128QAM	*/
    case 256:
        snr = 46390;
        break;	/*	256QAM	*/
    default:
        snr = 40310;
        break;
    }


    snr -= mes_log[mse - 1];					/*	C - 10*log10(MSE)	*/
    snr /= 1000;
    if (snr > 0xff) {
        snr = 0xff;
    }


    *signal_snr = (U8)snr;

    return ret;
}


MT_FE_RET mt_fe_dmd_tc6930_get_quality(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 *p_percent)
{
    const U32 mes_log[] = {
        0,	  3010,	  4771,	  6021,	  6990,	  7781,	  8451,	  9031,	  9542,	 10000,
        10414,	 10792,	 11139,	 11461,	 11761,	 12041,	 12304,	 12553,	 12788,	 13010,
        13222,	 13424,	 13617,	 13802,	 13979,	 14150,	 14314,	 14472,	 14624,	 14771,
        14914,	 15052,	 15185,	 15315,	 15441,	 15563,	 15682,	 15798,	 15911,	 16021,
        16128,	 16232,	 16335,	 16435,	 16532,	 16628,	 16721,	 16812,	 16902,	 16990,
        17076,	 17160,	 17243,	 17324,	 17404,	 17482,	 17559,	 17634,	 17709,	 17782,
        17853,	 17924,	 17993,	 18062,	 18129,	 18195,	 18261,	 18325,	 18388,	 18451,
        18513,	 18573,	 18633,	 18692,	 18751,	 18808,	 18865,	 18921,	 18976,	 19031
    };

    U8 i = 0;
    U32 snr = 0, snr_100, snr_int;
    U32 mse = 0, iRatio, iRatio_100;
    U8 reg91H = 0, reg08H = 0, reg07H = 0;
    MT_FE_LOCK_STATE q_state = MtFeLockState_Undef;
    U16 qam;

    qam = handle->demod_device[demod_index].channel_param.qam_code;


    mt_fe_dmd_tc6930_get_lock_state(handle, demod_index, &q_state);

    if (q_state == MtFeLockState_Locked) {
        handle->dmd_get_reg(handle, demod_index, 0x91, &reg91H);

        if ((reg91H & 0x23) != 0x03) {
            *p_percent = 0;
            return MtFeErr_Fail;
        }

        mse = 0;

        for (i = 0; i < 25; i ++) {
            handle->dmd_get_reg(handle, demod_index, 0x08, &reg08H);
            handle->dmd_get_reg(handle, demod_index, 0x07, &reg07H);

            mse += (reg08H << 8) + reg07H;

            handle->mt_sleep(1);
        }

        iRatio = mse / 25;
        iRatio_100 = (mse * 100 - iRatio * 100) / 2500;

        if (iRatio < 1) {
            iRatio = 1;
            iRatio_100 = 0;
        }

        if (iRatio >= 80) {
            iRatio = 80;
            iRatio_100 = 0;
        }

        snr_int = mes_log[iRatio - 1];

        if (iRatio == 80) {
            snr_100 = 0;
        } else {
            snr_100 = (mes_log[iRatio] - mes_log[iRatio - 1]) * iRatio_100 / 100;
        }

        snr_int += snr_100;

        switch (qam) {
        case 16:
            snr = 34080;
            break;	/*	16QAM	*/
        case 32:
            snr = 37600;
            break;	/*	32QAM	*/
        case 64:
            snr = 40310;
            break;	/*	64QAM	*/
        case 128:
            snr = 43720;
            break;	/*	128QAM	*/
        case 256:
            snr = 46390;
            break;	/*	256QAM	*/
        default:
            snr = 40310;
            break;
        }

        //snr -= mes_log[mse - 1];					/*	C - 10*log10(MSE)	*/
        snr -= snr_int;


        switch (qam) {
        case 16:
            if (snr <= 16000) {
                *p_percent = 0;
            } else if (snr <= 16200) {
                *p_percent =  0 + (snr - 16000) / 20;
            } else if (snr <= 16700) {
                *p_percent = 10 + (snr - 16200) / 50;
            } else if (snr <= 17500) {
                *p_percent = 20 + (snr - 16700) / 80;
            } else if (snr <= 18400) {
                *p_percent = 30 + (snr - 17500) / 60;
            } else if (snr <= 19300) {
                *p_percent = 45 + (snr - 18400) / 60;
            } else if (snr <= 20300) {
                *p_percent = 60 + (snr - 19300) / 50;
            } else if (snr <= 21300) {
                *p_percent = 80 + (snr - 20300) / 100;
            } else {
                *p_percent = 100;
            }
            break;

        case 32:
            if (snr <= 18900) {
                *p_percent = 0;
            } else if (snr <= 20300) {
                *p_percent =  0 + (snr - 18900) / 70;
            } else if (snr <= 21300) {
                *p_percent = 20 + (snr - 20300) / 100;
            } else if (snr <= 22200) {
                *p_percent = 30 + (snr - 21300) / 60;
            } else if (snr <= 23100) {
                *p_percent = 45 + (snr - 22200) / 60;
            } else if (snr <= 24100) {
                *p_percent = 60 + (snr - 23100) / 50;
            } else if (snr <= 25500) {
                *p_percent = 80 + (snr - 24100) / 70;
            } else {
                *p_percent = 100;
            }
            break;

        case 128:
            if (snr <= 24750) {
                *p_percent = 0;
            } else if (snr <= 25000) {
                *p_percent =  0 + (snr - 24750) / 25;
            } else if (snr <= 26000) {
                *p_percent = 10 + (snr - 25000) / 50;
            } else if (snr <= 26900) {
                *p_percent = 30 + (snr - 26000) / 60;
            } else if (snr <= 27800) {
                *p_percent = 45 + (snr - 26900) / 60;
            } else if (snr <= 28800) {
                *p_percent = 60 + (snr - 27800) / 100;
            } else if (snr <= 29600) {
                *p_percent = 80 + (snr - 28800) / 80;
            } else if (snr <= 30600) {
                *p_percent = 90 + (snr - 29600) / 100;
            } else {
                *p_percent = 100;
            }
            break;

        case 256:
            if (snr <= 27280) {
                *p_percent = 0;
            } else if (snr <= 27480) {
                *p_percent =  0 + (snr - 27280) / 10;
            } else if (snr <= 28080) {
                *p_percent = 20 + (snr - 27480) / 60;
            } else if (snr <= 28880) {
                *p_percent = 30 + (snr - 28080) / 80;
            } else if (snr <= 29880) {
                *p_percent = 40 + (snr - 28880) / 100;
            } else if (snr <= 30580) {
                *p_percent = 50 + (snr - 29880) / 70;
            } else if (snr <= 31480) {
                *p_percent = 60 + (snr - 30580) / 30;
            } else if (snr <= 32080) {
                *p_percent = 90 + (snr - 31480) / 60;
            } else {
                *p_percent = 100;
            }
            break;

        case 64:
        default:
            if (snr <= 21300) {
                *p_percent = 0;
            } else if (snr <= 21500) {
                *p_percent =  0 + (snr - 21300) / 10;
            } else if (snr <= 22500) {
                *p_percent = 10 + (snr - 21500) / 50;
            } else if (snr <= 23400) {
                *p_percent = 30 + (snr - 22500) / 60;
            } else if (snr <= 24300) {
                *p_percent = 45 + (snr - 23400) / 60;
            } else if (snr <= 25300) {
                *p_percent = 60 + (snr - 24300) / 100;
            } else if (snr <= 26300) {
                *p_percent = 70 + (snr - 25300) / 50;
            } else if (snr <= 27300) {
                *p_percent = 90 + (snr - 26300) / 100;
            } else {
                *p_percent = 100;
            }
            break;
        }
    } else {
        *p_percent = 0;
    }

    return MtFeErr_Ok;
}


/***********************************************
   Get signal strength
************************************************/
MT_FE_RET mt_fe_dmd_tc6930_get_strength(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 *signal_strength)
{
    MT_FE_RET ret = MtFeErr_Ok;

    //U8	SignalStrength = 0;
    //U8	reg43H;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

#if 0
    ret = handle->dmd_get_reg(handle, demod_index, 0x43, &reg43H);

    if ((reg43H & 0x08) == 0x08) {
        U8 reg3BH, reg3CH;

        ret = handle->dmd_get_reg(handle, demod_index, 0x3B, &reg3BH);
        ret = handle->dmd_get_reg(handle, demod_index, 0x3C, &reg3CH);

        SignalStrength = 255 - (reg3BH >> 1) - (reg3CH >> 1);
    }
#endif

    *signal_strength = (U8)(mt_fe_tn_get_signal_strength_tc6930(handle, demod_index) + 107);

//	*signal_strength = SignalStrength;

    return ret;
}


MT_FE_RET mt_fe_dmd_tc6930_get_offset(MT_FE_TC6930_Device_Handle handle, U8 demod_index, S32 *freq_offset_KHz, S32 *symbol_rate_offset_KSs)
{
    MT_FE_RET ret = MtFeErr_Ok;

    S32 freq_offset = 0;
    U8 regA6H = 0, regA7H = 0, reg51H = 0;
    S32 offset_val = 0;

    if (handle == NULL) {
        return MtFeErr_Uninit;
    }

    ret = handle->dmd_get_reg(handle, demod_index, 0xA6, &regA6H);
    ret = handle->dmd_get_reg(handle, demod_index, 0xA7, &regA7H);

    offset_val = (regA7H << 8);
    offset_val += regA6H;

    if (offset_val > 32768) {
        offset_val -= 65536;
    }

    freq_offset = (offset_val * (S32)(handle->demod_device[demod_index].on_board_settings.xtal_KHz)) >> 16;

    *freq_offset_KHz = freq_offset;


    handle->dmd_get_reg(handle, demod_index, 0x51, &reg51H);

    if ((reg51H & 0x80) == 0x80) {
        offset_val = reg51H - 256;
    } else {
        offset_val = reg51H;
    }


    //*symbol_rate_offset_KSs = (S32)(offset_val * (S32)(handle->demod_device[demod_index].channel_param.sym_KSs) * 0.5 / 1000);
    *symbol_rate_offset_KSs = (S32)(offset_val * (S32)(handle->demod_device[demod_index].channel_param.sym_KSs) / 2000);


    return ret;
}


MT_FE_RET mt_fe_dmd_tc6930_get_symbol_rate(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U16 *symbol_rate_KSs)
{
    U32 dwValue = 0;
    U8 tmp1, tmp2, tmp3, tmp4, tmp5;
    U16 sym = 0;
    S16 sym_offset = 0;
    S8 tmp6;
    U32 xtal = handle->demod_device[demod_index].on_board_settings.xtal_KHz;

    handle->dmd_get_reg(handle, demod_index, 0x55, &tmp1);
    handle->dmd_get_reg(handle, demod_index, 0x56, &tmp2);
    handle->dmd_get_reg(handle, demod_index, 0x57, &tmp3);
    handle->dmd_get_reg(handle, demod_index, 0x58, &tmp4);
    handle->dmd_get_reg(handle, demod_index, 0x51, &tmp5);

    dwValue = (tmp4 << 24) + (tmp3 << 16) + (tmp2 << 8) + tmp1;

    sym = (dwValue >> 24) * (xtal >> 8) + (((dwValue >> 24) * (xtal % 256)) >> 8) + (((dwValue % 16777216) * (xtal >> 8)) >> 24) - 10;

    if (tmp5 >= 128) {
        tmp6 = tmp5 - 256;
    } else {
        tmp6 = tmp5;
    }

    //sym_offset = tmp6 * sym * 5 / 10000;
    sym_offset = tmp6 * sym / 2000;

    sym -= sym_offset;

    *symbol_rate_KSs = sym;

    return MtFeErr_Ok;
}

/*****************************************************************
** Function: mt_fe_demod_tc6930_get_temperature
**
**
** Description:	Get temperature when chip is power on.
**
**
** Outputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	temp_degree		S16*		temperature degree
**
** Return:
**	Type:    MT_FE_RET
**  Value:   MtFeErr_Fail ----  Chip is power off, temperature data is invalid
**           MtFeErr_Ok   ----  Returned OK, Get temperature data is valid
**
*****************************************************************/
MT_FE_RET mt_fe_demod_tc6930_get_temperature(MT_FE_TC6930_Device_Handle handle, S16 *temp_degree)
{
    U8 index = 0, bPowerOn = 0;
    U16 uiVal = 0;
    //S16 degree = -20;

    for (index = 0; index < 3; index++) {
        if (handle->demod_device[index].tuner_settings.tuner_sleep_status == 1) {
            // Power on
            bPowerOn = 1;
            break;
        }
    }

    if (bPowerOn == 0) { // Power off, can't get the temperature
        return MtFeErr_Fail;
    }

    _mt_fe_tn_get_reg_16bit_tc6930(0x4008, &uiVal);
    uiVal &= ~(1 << 5);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4008, uiVal);

    _mt_fe_tn_get_reg_16bit_tc6930(0x400d, &uiVal);
    uiVal &= ~(1 << 5);
    _mt_fe_tn_set_reg_16bit_tc6930(0x400d, uiVal);

    _mt_fe_tn_get_reg_16bit_tc6930(0x4012, &uiVal);
    uiVal &= ~(1 << 5);
    _mt_fe_tn_set_reg_16bit_tc6930(0x4012, uiVal);

    _mt_fe_tn_get_reg_16bit_tc6930(0x42c4, &uiVal);
    uiVal >>= 4;
    uiVal &= 0x0F;

    *temp_degree = uiVal * 10; //-20 + uiVal * 10;

    return MtFeErr_Ok;
}

MT_FE_RET mt_fe_dmd_tc6930_get_tuner_version(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 *version_no, U32 *version_time)
{
    MT_FE_RET ret = MtFeErr_Ok;

    *version_no = handle->demod_device[demod_index].tuner_settings.tuner_version;
    *version_time = handle->demod_device[demod_index].tuner_settings.tuner_time;

    return ret;
}


MT_FE_RET mt_fe_dmd_tc6930_get_driver_version(U32 *version_no, U32 *version_time)
{
    *version_no		 = 404;
    *version_time	 = 22080910;

    return MtFeErr_Ok;
}


