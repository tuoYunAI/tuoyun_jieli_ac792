/*****************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd                                    */
/* MONTAGE PROPRIETARY AND CONFIDENTIAL                                      */
/* Copyright (c) 2020 Montage Technology Group Limited. All Rights Reserved. */
/*****************************************************************************/
/*****************************************************************************
 * Filename:        mt_fe_def_tc6930.h
 *
 * Description:     Definitions of Jazz demodulator driver.
 *
 * Current Version:     0.04.01
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
 *  Modify          0.01.11     2019.02.14  YZ.Huang
 *  Modify          0.03.04     2020.09.16  YZ.Huang
 *  Modify          0.04.01     2021.04.07  YZ.Huang
 *****************************************************************************/
#ifndef __MT_FE_DEF_TC6930_H__
#define __MT_FE_DEF_TC6930_H__

#include "mt_fe_common_tc6930.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STBLT_IMP			0		/* 0: default ; 1:stability improvement*/

#define  MT_TC6930_CUSTOM_CONFIG	0		/* 0: General; 1: BB */
#define  TUNER_I2C_ADDR_TC6930		0xac

#define	MT_PLATFORM_SEL				0		/* 0: Symphony, 1: Others */

#define MT_ABS(a, b)	(((a) >= (b)) ? ((a) - (b)) : ((b) - (a)))



/*****************************************************************************/


typedef struct _MT_FE_TC6930_TS_OUTPUT_SETTING {
    int								active_edge_clk;		/*selects the active edge of pin M_CKOUT to update the MPEG-TS output.
																	0: active at falling edge		1: active at rising edge*/
    int								polar_sync;				/*define the output polarity of pin M_SYNC
																	0: pin M_SYNC is active high ;	1: pin M_SYNC is active low*/
    int								polar_val;				/*define the output polarity of pin M_VAL
																	0: pin M_VAL is active high ;	1: pin M_VAL is active low*/
    int								polar_err;				/*define the output polarity of pin M_ERR
																	0: pin M_ERR is active high ;	1: pin M_ERR is active low*/
    MT_FE_TS_OUT_MODE					output_mode;			/*parallel interface, serial interface, common  interface*/
    MT_FE_TS_OUT_MAX_CLOCK				output_clock;
    U8									custom_duty_cycle;		/* User defined ducy cycle of output clock. tmp1 = nibble[7:4], tmp2 = nibble[3:0] */
    /* If enable auto adjusting function, this variable defines the minimum divide ratio */
    int								serial_pin_select;		/*in the serial output mode only, select data output pin
																	0: select pin D0;	1: select pin D7*/
    int								output_high_Z;			/*TS output high-impedance status
																	0: disable high-Z, normal output;	1: enable high-Z, no output*/
} MT_FE_TC6930_TS_OUTPUT_SETTINGS;


typedef struct _MT_FE_DTV_PARAMETER_DVBC_TC6930 {
    U32		freq_KHz;
    U16		sym_KSs;
    U16		qam_code;
    int	inverted;
    U32		param_reserved;
} MT_FE_DTV_PARAMETER_DVBC_TC6930;

typedef struct _MT_FE_TC6930_ON_BOARD_SETTINGS {
    int	bSingleXTAL;
    int	bReadCali;
    U8		iVppSel;			// 0: 2Vpp, 1: 1Vpp
    U8		iGainStep;			// 0: default, 1: FY
    U8		chip_mode;			// 0: new DC2800        1: new Jazz        2: old DC2800
    U32		xtal_KHz;
    U32		chip_version;
    U32		on_board_reserved;
} MT_FE_TC6930_ON_BOARD_SETTINGS;

typedef struct _MT_FE_TN_Device_Settings_TC6930 {
    U8			tuner_index;
    U8			tuner_init_OK;			// Tuner initialize status
    U8			tuner_dev_addr;			// Tuner device address

    U8			tuner_application;		// 0: Cable, 1: DTMB, 2: Analog TV, 3: DVB-T2,4: DVB-T

    U16			tuner_qam;
    U16			tuner_mtt;
    U16			tuner_custom_cfg;
    U16			tuner_region_cfg;
    U8			tuner_fw_version;
    U32			tuner_version;
    U32			tuner_time;

    U8			tuner_crystal_cap;		// Crystal loading capacitor, from 0x00 to 0x1f,default 0x18;
    U8			tuner_clock_out;		// 0: Clock Out Off, 1: Clock Out On
    U8			tuner_mode;				// 0: Normal (LT Off), 1: Normal (LT On), 2: Sleep (LT On)
    U8			tuner_int_lt;
    U8			tuner_gpio_out;			// 0: GPIO out disable, 1: GPIO out enable
    U8			tuner_int_im;
    U16			tuner_mixer_type;
    U32			tuner_freq;				// RF frequency to be set, unit: KHz
    U32			tuner_crystal;			// Tuner crystal frequency, unit: KHz
    U32			tuner_int_ldiv;
    U32			tuner_int_fvco_tg;
    //double		tuner_Fvco_val;			// output
    U32			tuner_dac;				// Tuner DAC frequency, unit: KHz
    U8			tuner_dac_gain;			// Tuner DAC gain:1~6;
    U32			tuner_bandwidth;		// Bandwidth of the channel, unit: KHz, 1700/6000/7000/8000
    U8			tuner_sleep_status;		// 0: Sleep; 1: Wakeup
    U8			tuner_signal_delay_en;
    U32			tuner_signal_delay_ns;
    U8			tuner_harmonic_imp;
} MT_FE_TN_DEVICE_SETTINGS_TC6930, *MT_FE_TN_TC6930_Device_Handle;

typedef struct _MT_FE_TC6930_DEMOD_SETTINGS {
    int							bInitOK;
    U8								dev_index;					/*index*/
    U8								demod_dev_addr;				/*demodulator device i2c addres*/
    U8								perf_optimized;				/*performance optimized or not*/
    U32								chip_id;					/*chip id, version*/
    MT_FE_SUPPORTED_DEMOD			demod_type;					/*demod type*/
    MT_FE_TYPE						dtv_mode;					/*dtv mode*/
    MT_FE_DTV_PARAMETER_DVBC_TC6930	channel_param;
    MT_FE_TN_DEVICE_SETTINGS_TC6930	tuner_settings;
    MT_FE_TC6930_ON_BOARD_SETTINGS	on_board_settings;
    MT_FE_TC6930_TS_OUTPUT_SETTINGS	ts_output_settings;
} MT_FE_TC6930_DEMOD_SETTINGS;

typedef struct _MT_FE_TC6930_SETTINGS {
    MT_FE_TC6930_DEMOD_SETTINGS	demod_device[3];

    U8			cur_demod_index;

    U8			tuner_mcu_status;		// Tuner mcu status

    MT_FE_RET(*dmd_set_reg)(void *handle, U8 demod_index, U8 reg_index, U8 reg_value);
    MT_FE_RET(*dmd_get_reg)(void *handle, U8 demod_index, U8 reg_index, U8 *p_buf);
    void	(*mt_sleep)(U32 ticks_ms);
    MT_FE_RET(*Set32Bits)(U32 reg_addr, U32 reg_data);
    MT_FE_RET(*Get32Bits)(U32 reg_addr, U32 *p_data);
} MT_FE_TC6930_SETTINGS, *MT_FE_TC6930_Device_Handle;

/***************************************************************************
Summary:
front-end tuning status
***************************************************************************/
/* Demodulator APIs */
/*Internal*/
MT_FE_RET _mt_fe_dmd_tc6930_set_demod(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
MT_FE_RET _mt_fe_dmd_tc6930_set_demod_appendix(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U16 qam);
MT_FE_RET _mt_fe_dmd_tc6930_set_symbol_rate(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 sym, U32 xtal);
MT_FE_RET _mt_fe_dmd_tc6930_set_QAM(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U16 qam);
MT_FE_RET _mt_fe_dmd_tc6930_set_tx_mode(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 inverted, U8 j83);
MT_FE_RET _mt_fe_dmd_tc6930_set_ts_output(MT_FE_TC6930_Device_Handle handle, U8 demod_index, MT_FE_TS_OUT_MODE ts_mode);


/*Public*/
MT_FE_RET mt_fe_dmd_tc6930_match_tuners(MT_FE_TC6930_Device_Handle handle);
MT_FE_RET mt_fe_dmd_tc6930_config_default(MT_FE_TC6930_Device_Handle handle, U8 demod_index);

MT_FE_RET mt_fe_dmd_tc6930_reset_all(MT_FE_TC6930_Device_Handle handle);
MT_FE_SUPPORTED_DEMOD mt_fe_dmd_tc6930_check_chip_id(MT_FE_TC6930_Device_Handle handle);

MT_FE_RET mt_fe_dmd_tc6930_init(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
MT_FE_RET mt_fe_dmd_tc6930_connect(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 freq_KHz, U16 symbol_rate_KSs, U16 qam, U8 inverted);
MT_FE_RET mt_fe_dmd_tc6930_soft_reset(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
MT_FE_RET mt_fe_dmd_tc6930_get_lock_state(MT_FE_TC6930_Device_Handle handle, U8 demod_index, MT_FE_LOCK_STATE *lock_status);
MT_FE_RET mt_fe_dmd_tc6930_get_ber(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 *error_bits, U32 *total_bits);
MT_FE_RET mt_fe_dmd_tc6930_get_snr(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 *signal_snr);
MT_FE_RET mt_fe_dmd_tc6930_get_strength(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 *signal_strength);
MT_FE_RET mt_fe_dmd_tc6930_get_offset(MT_FE_TC6930_Device_Handle handle, U8 demod_index, S32 *freq_offset_KHz, S32 *symbol_rate_offset_KSs);
MT_FE_RET mt_fe_dmd_tc6930_get_symbol_rate(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U16 *symbol_rate_KSs);
MT_FE_RET mt_fe_dmd_tc6930_get_tuner_version(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 *version_no, U32 *version_time);
MT_FE_RET mt_fe_dmd_tc6930_get_driver_version(U32 *version_no, U32 *version_time);
MT_FE_RET mt_fe_dmd_tc6930_get_statistics(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 *agc_lock, U32 *timing_lock,
        U32 *dagc_lock, U32 *carrier_lock, U32 *sync_lock, U32 *descrambler_lock, U32 *chip_lock);

MT_FE_RET mt_fe_demod_tc6930_get_temperature(MT_FE_TC6930_Device_Handle handle, S16 *temp_degree);


/*Supported tuner APIs*/
//MT_FE_RET mt_fe_tc6930_tn_get_version_default(void *dev_handle, U32 *version_no, U32 *version_time);

/*Montage TC6930*/

void _mt_fe_tn_set_RF_front_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 Freq_KHz);
void _mt_fe_tn_set_Mixer_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 Freq_KHz, U8 mixer_sel);
void _mt_fe_tn_set_LO_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 Freq_KHz, U8 mixer_sel);
void _mt_fe_tn_set_PLL_freq_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 Delta_sign, U8 Num_step);
void _mt_fe_tn_freq_fine_tune_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U8 Delta_sign, U8 Num_step);
void _mt_fe_tn_set_BB_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
void _mt_fe_tn_set_BB_Cal_tc6930(MT_FE_TC6930_Device_Handle handle, U32 bandwidth);
void _mt_fe_tn_set_DAC_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 Freq_KHz);
void _mt_fe_tn_preset_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
void _mt_fe_tn_set_tune_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 Freq_KHz, U8 mixer_sel, U32 WaitTime);
void _mt_fe_tn_set_sysmon_tc6930(MT_FE_TC6930_Device_Handle handle);
void _mt_fe_tn_poweron_set_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
void _mt_fe_tn_set_appendix_tc6930(MT_FE_TC6930_Device_Handle handle);
void _mt_fe_tn_ADC_calibration_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);


void _mt_fe_tn_set_mch_cal_tc6930(MT_FE_TC6930_Device_Handle handle);
void _mt_fe_tn_set_mch_app_tc6930(MT_FE_TC6930_Device_Handle handle);


//APIs
void mt_fe_tn_wakeup_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
void mt_fe_tn_sleep_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
S32 mt_fe_tn_download_fw_tc6930(MT_FE_TC6930_Device_Handle handle);

void mt_fe_tn_init_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);
void mt_fe_tn_set_freq_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index, U32 Freq_KHz);

void mt_fe_tn_set_GPIOout_high_tc6930(MT_FE_TC6930_Device_Handle handle);
void mt_fe_tn_set_GPIOout_low_tc6930(MT_FE_TC6930_Device_Handle handle);

void mt_fe_tn_get_diagnose_info_tc6930(MT_FE_TC6930_Device_Handle handle, U32 *data1, U32 *data2);
S32	 mt_fe_tn_get_signal_strength_tc6930(MT_FE_TC6930_Device_Handle handle, U8 demod_index);

void mt_fe_tn_sleep_all_tc6930(MT_FE_TC6930_Device_Handle handle);
void mt_fe_tn_set_tuner_index_tc6930(MT_FE_TC6930_Device_Handle handle);


#ifdef __cplusplus
};
#endif


#endif /* __MT_FE_DEF_H__ */

