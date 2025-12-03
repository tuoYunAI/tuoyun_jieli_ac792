#include "app_config.h"

#ifdef CONFIG_MEDIA_ENABLE

#include "audio_def.h"

source_node_adapter
decoder_node_adapter
resample_node_adapter
clk_sync_node_adapter

sine_dec_plug
pcm_dec_plug
music_file_plug
net_file_plug
tone_file_plug
ring_file_plug
key_tone_file_plug
tts_file_plug
#if TCFG_FILEPLAY_NODE_ENABLE
file_play_plug
#endif
#if TCFG_USER_TWS_ENABLE
tws_tone_file_plug
#endif
#if TCFG_LOCAL_TWS_ENABLE
local_tws_file_plug
#endif
#if TCFG_ZERO_ACTIVE_NODE_ENABLE
zero_file_plug
#endif
#if TCFG_USER_BT_CLASSIC_ENABLE
bt_audio_sync_node_adapter
#endif
#if TCFG_DEC_SBC_HW_ENABLE
sbc_hwaccel
sbc_decoder_hw_plug
#endif
#if TCFG_DEC_SBC_SW_ENABLE
sbc_decoder_sw_plug
#endif
#if TCFG_DEC_MSBC_HW_ENABLE
msbc_hw_decoder_plug
#endif
#if TCFG_DEC_MSBC_SW_ENABLE
msbc_sw_decoder_plug
#endif
#if TCFG_DEC_CVSD_ENABLE
cvsd_decoder_plug
#endif
#if TCFG_DEC_WTG_ENABLE
g729_dec_plug
#endif
#if TCFG_DEC_MTY_ENABLE
mty_dec_plug
#endif
#if TCFG_DEC_WAV_ENABLE
wav_dec_plug
#endif
#if TCFG_DEC_AIFF_ENABLE
aiff_dec_plug
#endif
#if TCFG_DEC_OGG_OPUS_ENABLE
opus_dec_plug
#endif
#if TCFG_DEC_OGG_VORBIS_ENABLE
ogg_dec_plug
#endif
#if TCFG_DEC_AMR_ENABLE
amr_dec_plug
#endif
#if TCFG_DEC_DTS_ENABLE
dts_dec_plug
#endif
#if TCFG_DEC_MP3_ENABLE
mp3_dec_plug
#endif
#if TCFG_DEC_F2A_ENABLE
f2a_dec_plug
#endif
#if TCFG_DEC_WMA_ENABLE
wma_dec_plug
#endif
#if TCFG_DEC_FLAC_ENABLE
flac_dec_plug
#endif
#if TCFG_DEC_M4A_ENABLE
m4a_dec_plug
#endif
#if TCFG_DEC_ALAC_ENABLE
alac_dec_plug
#endif
#if TCFG_DEC_APE_ENABLE
ape_dec_plug
#endif
#if TCFG_DEC_MIDI_FILE_ENABLE
midi_dec_plug
#endif
#if TCFG_DEC_MIDI_CTRL_ENABLE
midi_ctrl_dec_plug
#endif
#if (TCFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LW)
#if (TCFG_KBOX_1T3_MODE_EN == 0)
jla_lw_encoder_plug
#endif
jla_lw_dec_plug
#elif (TCFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LL)
#if (TCFG_KBOX_1T3_MODE_EN == 0)
jla_ll_encoder_plug
#endif
jla_ll_dec_plug
#elif (TCFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_V2)
jla_v2_dec_plug
jla_v2_enc_plug
#elif (TCFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA)
jla_dec_plug
jla_encoder_plug
#endif
#if defined TCFG_LE_AUDIO_APP_CONFIG && (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
lc3_dec_plug
lc3_encoder_plug
#endif
#if TCFG_ENC_OGG_ENABLE
ogg_encoder_plug
#endif
#if TCFG_ENC_OPUS_ENABLE
opus_encoder_plug
#endif
#if TCFG_ENC_AAC_ENABLE
aac_encoder_plug
#endif
#if TCFG_ENC_AMR_ENABLE
amr_encoder_plug
#endif
#if TCFG_ENC_SPEEX_ENABLE
speex_encoder_plug
#endif
#if TCFG_DEC_SPEEX_ENABLE
speex_dec_plug
#endif
#if TCFG_DEC_WTS_ENABLE
wts_dec_plug
#endif
#if TCFG_DEC_AAC_ENABLE
aac_dec_plug
#endif
#if TCFG_BT_SUPPORT_LHDC
lhdc_dec_plug
#endif
#if TCFG_BT_SUPPORT_LHDC_V5
lhdc_v5_dec_plug
#endif
#if TCFG_BT_SUPPORT_LDAC
ldac_dec_plug
#endif
#if TCFG_ENC_SBC_ENABLE
sbc_encoder_soft_plug
#endif
#if TCFG_ENC_MSBC_HW_ENABLE
msbc_encoder_hw_plug
#endif
#if TCFG_ENC_MSBC_SW_ENABLE
msbc_encoder_sw_plug
#endif
#if TCFG_ENC_CVSD_ENABLE
cvsd_encoder_plug
#endif
#if TCFG_ENC_MP3_ENABLE
mp3_encoder_plug
#endif
#if TCFG_ENC_ADPCM_ENABLE
wav_encoder_plug
wav_package
#endif
#if TCFG_ENC_PCM_ENABLE
pcm_encoder_plug
#endif
#if TCFG_MIXER_NODE_ENABLE
mixer_node_adapter
#endif
#if TCFG_DAC_NODE_ENABLE
dac_node_adapter
#endif
#if TCFG_ADC_NODE_ENABLE
adc_file_plug
#endif
#if TCFG_IIS_NODE_ENABLE
iis_node_adapter
iis1_node_adapter
iis_file_plug
iis1_file_plug
#endif
#if TCFG_PDM_NODE_ENABLE
pdm_mic_file_plug
#endif
#if TCFG_AUDIO_LINEIN_ENABLE
linein_file_plug
#endif
#if TCFG_AUDIO_FM_ENABLE
fm_file_plug
#endif
#if TCFG_VIRTUAL_DATA_RX_NODE_ENABLE
vir_data_rx_plug
#endif
#if TCFG_MULTI_CH_IIS_RX_NODE_ENABLE
mulit_ch_iis0_file_plug
mulit_ch_iis1_file_plug
#endif
#if TCFG_MULTI_CH_IIS_NODE_ENABLE
multi_ch_iis0_node_adapter
multi_ch_iis1_node_adapter
#endif
#if TCFG_ENERGY_DETECT_NODE_ENABLE
energy_detect_node_adapter
#endif
#if TCFG_CONVERT_NODE_ENABLE
convert_node_adapter
convert_data_round_node_adapter
#endif
#if TCFG_PITCH_SPEED_NODE_ENABLE
pitch_speed_node_adapter
#endif
#if TCFG_3BAND_MERGE_ENABLE
three_band_merge_node_adapter
#endif
#if TCFG_2BAND_MERGE_ENABLE
two_band_merge_node_adapter
#endif
#if TCFG_BASS_TREBLE_NODE_ENABLE
bass_treble_eq_node_adapter
#endif
#if TCFG_SOUND_SPLITTER_NODE_ENABLE
sound_splitter_node_adapter
#endif
#if TCFG_VOCAL_TRACK_SYNTHESIS_NODE_ENABLE
vocal_track_synthesis_node_adapter
#endif
#if TCFG_VOCAL_TRACK_SEPARATION_NODE_ENBALE
vocal_track_separation_node_adapter
#endif
#if TCFG_CROSSOVER_NODE_ENABLE
crossover_node_adapter
crossover_2band_node_adapter
#endif
#if TCFG_WDRC_NODE_ENABLE
wdrc_node_adapter
#endif
#if TCFG_STEROMIX_NODE_ENABLE
steromix_node_adapter
#endif
#if TCFG_DYNAMIC_EQ_NODE_ENABLE
dynamic_eq_node_adapter
#endif
#if TCFG_DYNAMIC_EQ_EXT_DETECTOR_NODE_ENABLE
dynamic_eq_ext_detector_node_adapter
#endif
#if TCFG_SURROUND_NODE_ENABLE
surround_node_adapter
#endif
#if TCFG_VOICE_CHANGER_NODE_ENABLE
voice_changer_node_adapter
#endif
#if TCFG_VOICE_CHANGER_ADV_NODE_ENABLE
voice_changer_adv_node_adapter
#endif
#if TCFG_NOISEGATE_NODE_ENABLE
noisegate_node_adapter
#endif
#if TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE
frequency_shift_node_adapter
#endif
#if TCFG_NOTCH_HOWLING_NODE_ENABLE
howling_suppress_node_adapter
#endif
#if TCFG_PLATE_REVERB_NODE_ENABLE
plate_reverb_node_adapter
#endif
#if TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE
plate_reverb_advance_node_adapter
#endif
#if TCFG_ECHO_NODE_ENABLE
echo_node_adapter
#endif
#if TCFG_SPECTRUM_NODE_ENABLE
spectrum_node_adapter
#endif
#if TCFG_SPECTRUM_ADVANCE_NODE_ENABLE
spectrum_advance_node_adapter
#endif
#if TCFG_AUTOTUNE_NODE_ENABLE
autotune_node_adapter
#endif
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
vocal_remover_node_adapter
#endif
#if TCFG_SPEAKER_EQ_NODE_ENABLE
spk_eq_node_adapter
#endif
#if TCFG_EQ_ENABLE
eq_node_adapter
#endif
#if TCFG_GAIN_NODE_ENABLE
gain_node_adapter
#endif
#if TCFG_VBASS_NODE_ENABLE
vbass_node_adapter
#endif
#if TCFG_NS_NODE_ENABLE
ns_node_adapter
#endif
#if TCFG_NS_NODE_LITE_ENABLE
ns_node_lite_adapter
#endif
#if TCFG_DNS_NODE_ENABLE
dns_node_adapter
#endif
#if TCFG_STEREO_WIDENER_NODE_ENABLE
stereo_widener_node_adapter
#endif
#if TCFG_HARMONIC_EXCITER_NODE_ENABLE
harmonic_exciter_node_adapter
#endif
#if TCFG_INDICATOR_NODE_ENABLE
indicator_node_adapter
#endif
#if TCFG_CHORUS_NODE_ENABLE
chorus_node_adapter
#endif
#if TCFG_CHANNEL_EXPANDER_NODE_ENABLE
channel_expander_node_adapter
#endif
#if TCFG_CHANNEL_MERGE_NODE_ENABLE
channel_merge_node_adapter
#endif
#if TCFG_WDRC_DETECTOR_NODE_ENABLE
wdrc_detector_node_adapter
#endif
#if TCFG_WDRC_ADVANCE_NODE_ENABLE
wdrc_advance_node_adapter
#endif
#if TCFG_PCM_DELAY_NODE_ENABLE
pcm_delay_node_adapter
#endif
#if TCFG_AUTODUCK_NODE_ENABLE
autoduck_trigger_node_adapter
autoduck_node_adapter
#endif
#if TCFG_EFFECT_DEV0_NODE_ENABLE
effect_dev0_node_adapter
#endif
#if TCFG_EFFECT_DEV1_NODE_ENABLE
effect_dev1_node_adapter
#endif
#if TCFG_EFFECT_DEV2_NODE_ENABLE
effect_dev2_node_adapter
#endif
#if TCFG_EFFECT_DEV3_NODE_ENABLE
effect_dev3_node_adapter
#endif
#if TCFG_EFFECT_DEV4_NODE_ENABLE
effect_dev4_node_adapter
#endif
#if TCFG_SOURCE_DEV0_NODE_ENABLE
source_dev0_file_plug
#endif
#if TCFG_SOURCE_DEV1_NODE_ENABLE
source_dev1_file_plug
#endif
#if TCFG_SOURCE_DEV2_NODE_ENABLE
source_dev2_file_plug
#endif
#if TCFG_SOURCE_DEV3_NODE_ENABLE
source_dev3_file_plug
#endif
#if TCFG_SOURCE_DEV4_NODE_ENABLE
source_dev4_file_plug
#endif
#if TCFG_SINK_DEV0_NODE_ENABLE
sink_dev0_adapter
#endif
#if TCFG_SINK_DEV1_NODE_ENABLE
sink_dev1_adapter
#endif
#if TCFG_SINK_DEV2_NODE_ENABLE
sink_dev2_adapter
#endif
#if TCFG_SINK_DEV3_NODE_ENABLE
sink_dev3_adapter
#endif
#if TCFG_SINK_DEV4_NODE_ENABLE
sink_dev4_adapter
#endif
#if TCFG_FILE_PACKAGE_NODE_ENABLE
packager_adapter
#endif
#if TCFG_WRITE_FILE_NODE_ENABLE
write_file_adapter
#endif
#if TCFG_DATA_SATURATION_NODE_ENABLE
data_saturation_node_adapter
#endif
#if TCFG_DYNAMIC_EQ_PRO_NODE_ENABLE
dynamic_eq_pro_node_adapter
#endif
#if TCFG_DYNAMIC_EQ_PRO_EXT_DETECTOR_NODE_ENABLE
dynamic_eq_pro_ext_detector_node_adapter
#endif
#if TCFG_AUTO_WAH_NODE_ENABLE
autowah_node_adapter
#endif
#if TCFG_PING_PONG_PCM_DELAY_NODE_ENABLE
pingpong_pcm_delay_node_adapter
#endif
#if TCFG_THREE_D_EFFECT_NODE_ENABLE
three_d_node_adapter
#endif
#if TCFG_LLNS_NODE_ENABLE
llns_node_adapter
#endif
#if TCFG_FADE_NODE_ENABLE
fade_node_adapter
#endif
#if TCFG_AGC_NODE_ENABLE
agc_node_adapter
#endif
#if TCFG_STEREO_MTAPS_ECHO_NODE_ENABLE
stereo_mtaps_echo_node_adapter
#endif
#if TCFG_HOWLING_GATE_ENABLE
howling_gate_node_adapter
#endif
#if TCFG_MULTI_BAND_DRC_NODE_ENABLE
multiband_drc_node_adapter
#endif
#if TCFG_DATA_EXPORT_NODE_ENABLE
data_export_node_adapter
#endif
#if TCFG_SWITCH_NODE_ENABLE
switch_node_adapter
#endif
#if TCFG_VAD_NODE_ENABLE
vad_node_adapter
#endif
#if TCFG_DISTORTION_NODE_ENABLE
distortion_node_adapter
#endif
#if TCFG_NOISEGATE_PRO_NODE_ENABLE
noisegate_pro_node_adapter
#endif
#if TCFG_SOFWARE_EQ_NODE_ENABLE
sof_eq_node_adapter
#endif
#if TCFG_MULTI_BAND_LIMITER_NODE_ENABLE
multiband_limiter_node_adapter
#endif
#if TCFG_LIMITER_NODE_ENABLE
limiter_node_adapter
#endif
#if TCFG_PLATE_REVERB_LITE_NODE_ENABLE
reverb_lite_node_adapter
#endif
#if TCFG_VIRTUAL_SURROUND_PRO_NODE_ENABLE
upmix_node_adapter
#endif
#if TCFG_SIGNAL_GENERATOR_NODE_ENABLE
signal_generator_file_plug
#endif
#if TCFG_REPLACE_NODE_ENABLE
replace_node_adapter
#endif
#if TCFG_REVERB_NODE_ENABLE
reverb_node_adapter
#endif
#if TCFG_SPLIT_GAIN_NODE_ENABLE
split_gain_node_adapter
#endif
#if TCFG_VIRTUAL_BASS_CLASSIC_NODE_ENABLE
virtual_bass_classic_node_adapter
#endif
#if TCFG_PHASER_NODE_ENABLE
phaser_node_adapter
#endif
#if TCFG_FLANGER_NODE_ENABLE
flanger_node_adapter
#endif
#if TCFG_CHORUS_ADVANCE_NODE_ENABLE
chorus_advance_node_adapter
#endif
#if TCFG_PINGPONG_ECHO_NODE_ENABLE
pingpong_echo_node_adapter
#endif
#if TCFG_STEREO_SPATIAL_WIDER_NODE_ENABLE
stereo_spatial_wider_node_adapter
#endif
#if TCFG_DISTORTION_CLIPPING_NODE_ENABLE
distortion_clipping_node_adapter
#endif
#if TCFG_FREQUENCY_COMPRESSOR_NODE_ENABLE
frequency_compressor_node_adapter
#endif

#endif


#ifdef CONFIG_VIDEO_ENABLE
isc0
isc1
csi0
mcv0
mcv1
mcv2
mcv3
isp
imc0
imc1
imc2
imc3
jpeg_enc0
jpeg_enc1
jpeg_enc2
jpeg_dec0
jpeg_dec1
jpeg_dec2
jpeg_dec3
file0
uvc0
uvc1
uvc_dev_ops
rep0
rep1
disp0
disp1
disp2
disp3
disp4
disp5
disp6
disp7
sink0
videosink0
videosink1
videosink2
imagesink0
gp0
virtual0
virtual1
virtual2
virtual3
sft0
sft1
gpu0
gpu1
gpu2
gpu3
dma2d0
dma2d1
yuv0
yuv1
yuv2
yuv3
composite0
composite1
video0
video1
video2
video3
video_dec
#endif

#if defined CONFIG_VIDEO_ENABLE || defined CONFIG_UI_ENABLE
fbvirtual_fbops
#endif

sdfile_vfs_ops
#ifdef CONFIG_SDFILE_EXT_ENABLE
sdfile_ext_vfs_ops
#endif
nor_sdfile_vfs_ops
#ifdef CONFIG_EXFLASH_ENABLE
fat_sdfile_fat_ops
#endif

#if TCFG_UDISK_ENABLE || TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
#ifdef CONFIG_JLFAT_ENABLE
jl_fat_vfs_ops
#else
fat_vfs_ops
#endif
#endif

#ifdef CONFIG_RAMFS_ENABLE
ramfs_vfs_ops
#endif

#ifdef CONFIG_DEVFS_ENABLE
devfs_vfs_ops
#endif

#ifdef CONFIG_VIDEO_ENABLE

video_server_info
video_engine_server_info
vpkg_server_info
vpkg_fsys_ops
avi_pkg_ops
camera_subdev
djpeg_subdev
video_dev_ops

#ifdef CONFIG_VIDEO_DEC_ENABLE
video_dec_server_info
vunpkg_server_info
#endif

#ifdef CONFIG_CTP_ENABLE
ctp_server_info
#endif

#endif


#ifdef CONFIG_AUDIO_ENABLE

audio_server_info

#ifdef CONFIG_JLA_DEC_ENABLE
jla_decoder
#endif
#ifdef CONFIG_JLA_ENC_ENABLE
subdev_jla_enc
#endif

#ifdef CONFIG_PCM_DEC_ENABLE
pcm_decoder_ops
#endif
#ifdef CONFIG_PCM_ENC_ENABLE
subdev_pcm_enc
pcm_package_ops
#endif

#ifdef CONFIG_ADPCM_DEC_ENABLE
adpcm_decoder_ops
#endif
#ifdef CONFIG_ADPCM_ENC_ENABLE
subdev_adpcm_enc
adpcm_package_ops
#endif

#ifdef CONFIG_WAV_DEC_ENABLE
wav_decoder
#endif
#ifdef CONFIG_WAV_ENC_ENABLE
wav_package_ops
#endif

#ifdef CONFIG_MSBC_DEC_ENABLE
msbc_decoder
#endif
#ifdef CONFIG_MSBC_ENC_ENABLE
subdev_msbc_enc
msbc_package_ops
#endif
#ifdef CONFIG_SBC_DEC_ENABLE
sbc_decoder
#endif
#ifdef CONFIG_SBC_ENC_ENABLE
subdev_sbc_enc
sbc_package_ops
#endif
#ifdef CONFIG_CVSD_DEC_ENABLE
cvsd_decoder
#endif
#ifdef CONFIG_CVSD_ENC_ENABLE
subdev_cvsd_enc
cvsd_package_ops
#endif

#ifdef CONFIG_MP3_DEC_ENABLE
mp3_decoder
#endif
#ifdef CONFIG_MP3_ENC_ENABLE
subdev_mp3_enc
mp3_package_ops
#endif
#ifdef CONFIG_MP2_ENC_ENABLE
subdev_mp2_enc
#endif

#ifdef CONFIG_WMA_DEC_ENABLE
wma_decoder
#endif

#ifdef CONFIG_AAC_ENC_ENABLE
subdev_aac_enc
aac_package_ops
#endif
#ifdef CONFIG_AAC_DEC_ENABLE
aac_decoder
aas_decoder
#endif

#ifdef CONFIG_M4A_DEC_ENABLE
m4a_decoder
#endif

#ifdef CONFIG_SPEEX_DEC_ENABLE
speex_decoder
#endif
#ifdef CONFIG_SPEEX_ENC_ENABLE
subdev_speex_enc
spx_package_ops
#endif

#ifdef CONFIG_AMR_DEC_ENABLE
amr_decoder
#endif
#ifdef CONFIG_AMR_ENC_ENABLE
subdev_amr_enc
amr_package_ops
#endif

#ifdef CONFIG_APE_DEC_ENABLE
ape_decoder
#endif

#ifdef CONFIG_FLAC_DEC_ENABLE
flac_decoder
#endif

#ifdef CONFIG_DTS_DEC_ENABLE
dts_decoder
#endif

#ifdef CONFIG_OGG_DEC_ENABLE
ogg_decoder
#endif
#ifdef CONFIG_OGG_ENC_ENABLE
subdev_ogg_enc
ogg_package_ops
#endif

#ifdef CONFIG_OPUS_DEC_ENABLE
opus_decoder
#endif
#ifdef CONFIG_OPUS_ENC_ENABLE
subdev_opus_enc
opus_package_ops
#endif

#ifdef CONFIG_VIRTUAL_DEV_ENC_ENABLE
vir_dev_sub
#endif

#ifdef CONFIG_VAD_ENC_ENABLE
subdev_vad_enc
subdev_jlvad_enc
#endif

#ifdef CONFIG_DNS_ENC_ENABLE
subdev_dns_enc
#endif

#ifdef CONFIG_AEC_ENC_ENABLE
subdev_aec_enc
#endif

#ifdef CONFIG_VIRTUAL_BASS_ENABLE
virtual_bass_effect
#endif

#ifdef CONFIG_SPECTRUM_FFT_EFFECT_ENABLE
spectrum_fft_effect
#endif

#ifdef CONFIG_DEC_DIGITAL_VOLUME_ENABLE
digital_vol_effect
#endif

#endif


#ifdef CONFIG_NET_ENABLE

#ifdef CONFIG_TURING_SDK_ENABLE
turing_sdk_api
wechat_sdk_api
#endif

#ifdef CONFIG_DEEPBRAIN_SDK_ENABLE
deepbrain_sdk_api
#endif

#ifdef CONFIG_ECHO_CLOUD_SDK_ENABLE
echo_cloud_sdk_api
#endif

#ifdef CONFIG_DLNA_SDK_ENABLE
dlna_sdk_api
#endif

#ifdef CONFIG_WT_SDK_ENABLE
wt_sdk_api
#endif

#ifdef CONFIG_DUER_SDK_ENABLE
duer_sdk_api
#endif

#ifdef CONFIG_DUI_SDK_ENABLE
dui_sdk_api
#endif

#ifdef CONFIG_ALI_SDK_ENABLE
ag_sdk_api
#endif

#ifdef CONFIG_TVS_SDK_ENABLE
tc_tvs_api
#endif

#ifdef CONFIG_TELECOM_SDK_ENABLE
#ifdef CONFIG_CTEI_DEVICE_ENABLE
psmarthome_ctei_device_api
#endif
#ifdef CONFIG_MC_DEVICE_ENABLE
CT_MC_api
#endif
#endif

#ifdef CONFIG_JL_CLOUD_SDK_ENABLE
jl_cloud_sdk_api
#endif

#endif
