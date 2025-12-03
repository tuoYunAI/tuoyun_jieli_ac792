#ifndef __LIB_MEDIA_CONFIG_H__
#define __LIB_MEDIA_CONFIG_H__

extern const int AUDIO_EQ_MAX_SECTION;

extern const int config_stream_frame_debug;

extern const int config_jlstream_scene_debug;

/*
 *******************************************************************
 *						Effect Configs
 *******************************************************************
 */
extern const int config_audio_eq_hp_enable;		//High Pass
extern const int config_audio_eq_lp_enable;		//Low Pass
extern const int config_audio_eq_bp_enable;		//Band Pass(Peaking)
extern const int config_audio_eq_hs_enable;		//High Shelf
extern const int config_audio_eq_ls_enable;		//Low Shelf
extern const int config_audio_eq_hs_q_enable;	//High Shelf Q
extern const int config_audio_eq_ls_q_enable;	//Low Shelf Q
extern const int config_audio_eq_hp_adv_enable;	//High Pass Advance
extern const int config_audio_eq_lp_adv_enable;	//Low Pass Advance

extern const int config_audio_eq_xfade_enable;

extern const float config_audio_eq_xfade_time;//0：一帧fade完成 非0：连续多帧fade，过度更加平滑，fade过程算力会相应增加(fade时间 范围(0~1)单位:秒)

extern const int config_audio_limiter_xfade_enable;

extern const int config_audio_mblimiter_xfade_enable;

extern const int config_audio_gain_enable;

extern const int config_audio_split_gain_enable;

extern const int config_audio_stereomix_enable;

extern const int config_voicechanger_effect_v_config;

extern const int config_audio_crossover_3band_enable;

extern const int limiter_run_mode;

extern const int drc_advance_run_mode;

extern const int drc_run_mode;

extern const int stereo_phaser_run_mode;

extern const int stereo_flanger_run_mode;

extern const int stereo_chorus_run_mode;

extern const int dynamic_eq_run_mode;

extern const int drc_detect_run_mode;

extern const int virtual_bass_pro_soft_crossover;

extern const int virtual_bass_eq_hard_select;

extern const int virtualbass_noisegate_attack_time;

extern const int virtualbass_noisegate_release_time;

extern const int virtualbass_noisegate_hold_time;

extern const float virtualbass_noisegate_threshold;

extern const int plate_reverb_lite_run_mode;

extern const int config_equall_loundness_en;

extern const int config_audio_effect_reverb_enable;

extern const int config_audio_effect_reverb_adv_enable;

extern const int config_audio_frequency_shift_howling_enable; //啸叫抑制-移频使能

extern const int config_audio_notch_howling_enable;           //啸叫抑制-陷波使能

extern const int config_audio_vocal_remover_low_cut_enable;

extern const int config_audio_vocal_remover_high_cut_enable;

extern const int config_audio_vocal_remover_preset_mode;

extern const int config_audio_mixer_ch_highlight_enable;

extern const int config_fast_aligned_pcm_silence;

extern const float config_sync_into_silenc_threshold;

extern const int config_jlstream_log_output_enable;

extern const int config_media_tws_en;

extern const int config_jlstream_multi_thread_enable;

extern const int config_multi_thread_self_adaption_enable;

extern const int config_seamless_recorder_enable;

extern const int config_max_enc_data_cache_size;

extern const int config_dac_cache_msec;

extern const int config_audio_dac_mix_enable;

extern const int config_jlstream_turbo_enable;

extern const int config_a2dp_energy_calc_enable;

extern const int config_music_file_buf_size;

extern const int config_auido_echo_delay_ms;

extern const int audio_eq_core_sel;

extern const int config_audio_effect_nsgate_enable;

extern const int config_audio_effect_nsgate_pro_enable;

extern const int config_media_24bit_enable;

extern const int config_ch_adapter_32bit_enable;

extern const int config_mixer_32bit_enable;

extern const int config_peak_rms_32bit_enable;

extern const int config_audio_vocal_track_synthesis_32bit_enable;

extern const int config_jlstream_fade_32bit_enable;

extern const int config_dec_support_channels;

extern const int config_audio_cfg_online_enable;

extern const int config_audio_cfg_debug_online;

extern const int config_audio_dac_dma_buf_realloc_enable;

extern const int config_dec_support_samplerate;

extern const int config_decoder_ff_fr_end_return_event_end;

extern const int config_file_dec_repeat_en;

extern const int config_file_dec_dest_play; //定点播放使能变量

extern const int config_aac_dec_lib_support_24bit_output;

extern const int config_wts_dec_lib_support_24bit_output;

extern const int config_mp3_dec_lib_support_24bit_output;

extern const int config_aac_dec_mp4a_latm_analysis;

extern const int config_opus_srindex; //选择opus解码文件的帧大小，0代表一帧40字节，1代表一帧80字节，2代表一帧160字节

extern const int config_jla_plc_fade_in_ms;

extern const int config_jla_plc_en;

extern const int config_jla_codec_hard_decision_enable;

extern const int config_jla_codec_soft_decision_enable;

extern const int config_jla_decode_o24bit_enable;

extern const int config_jla_encode_i24bit_enable;

extern const int config_lc3_encode_i24bit_enable;

extern const int config_lc3_decode_o24bit_enable;

extern const int config_lc3_plc_fade_in_ms;

extern const int config_lc3_plc_en;

extern const int config_mp3_enc_use_layer_3;

extern const int config_mp3_dec_speed_mode;

extern const int config_sel_adpcm_type;

extern const int config_msbc_decode_input_frame_replace;

extern const int config_msbc_encode_used_software_enable;

extern const int config_flac_id3_enable;

extern const int config_ape_id3_enable;

extern const int config_m4a_id3_enable;

extern const int config_wav_id3_enable;

extern const int config_wma_id3_enable;

extern const float config_bandmerge_node_fade_step;

/*
 *******************************************************************
 *						DAC Configs
 *******************************************************************
 */
extern const int config_audio_dac_channel_left_enable;
extern const int config_audio_dac_channel_right_enable;
extern const int config_audio_dac_power_on_mode;
extern const int config_audio_dac_power_off_lite;
extern const int config_audio_dac_underrun_time_lea;
extern const int config_audio_dac_ng_debug;

void media_irq_disable(void);

void media_irq_enable(void);

#endif
