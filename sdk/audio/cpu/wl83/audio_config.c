#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_config.data.bss")
#pragma data_seg(".audio_config.data")
#pragma const_seg(".audio_config.text.const")
#pragma code_seg(".audio_config.text")
#endif
/*
 ******************************************************************************************
 *							Audio Config
 *
 * Discription: 音频模块与芯片系列相关配置
 *
 * Notes:
 ******************************************************************************************
 */
/* #include "cpu/includes.h" */
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
const int config_audio_dac_mix_enable = 1;

//***********************
//*		AAC Codec       *
//***********************
#ifdef AAC_DEC_IN_MASK
const int config_aac_dec_mp4a_latm_analysis = 0;
const int config_aac_dec_lib_support_24bit_output = 0;
#else
const int config_aac_dec_mp4a_latm_analysis = 1;
const int config_aac_dec_lib_support_24bit_output = 1;
#endif

#ifdef MP3_DEC_IN_MASK
const int config_mp3_dec_lib_support_24bit_output = 0;
#else
const int config_mp3_dec_lib_support_24bit_output = 1;
#endif

#ifdef WTS_DEC_IN_MASK
const int config_wts_dec_lib_support_24bit_output = 0;
#else
const int config_wts_dec_lib_support_24bit_output = 1;
#endif

//***********************
//*		Audio Params    *
//***********************
void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_platform_cfg *platform_cfg)
{
    mic_param->mic_mode             = platform_cfg->mic_mode;
    mic_param->mic_ain_sel          = platform_cfg->mic_ain_sel;
    mic_param->mic_bias_sel         = platform_cfg->mic_bias_sel;
    mic_param->mic_bias_rsel        = platform_cfg->mic_bias_rsel;
    mic_param->mic_dcc              = platform_cfg->mic_dcc;
    mic_param->inside_bias_resistor = platform_cfg->inside_bias_resistor;
    mic_param->dmic_enable          = platform_cfg->dmic_enable;
    mic_param->dmic_sclk_fre        = platform_cfg->dmic_sclk_fre;
    mic_param->dmic_io_sclk         = platform_cfg->dmic_io_sclk;
    mic_param->dmic_io_idat0        = platform_cfg->dmic_io_idat0;
    mic_param->dmic_io_idat1        = platform_cfg->dmic_io_idat1;
    mic_param->dmic_ch0_mode        = platform_cfg->dmic_ch0_mode;
    mic_param->dmic_ch1_mode        = platform_cfg->dmic_ch1_mode;
}

void audio_linein_param_fill(struct linein_open_param *linein_param, const struct adc_platform_cfg *platform_cfg)
{
    linein_param->linein_mode    = platform_cfg->mic_mode;
    linein_param->linein_ain_sel = platform_cfg->mic_ain_sel;
    linein_param->linein_dcc     = platform_cfg->mic_dcc;
}
