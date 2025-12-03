#include "app_config.h"
#include "system/includes.h"
#include "media/includes.h"
#include "audio_config.h"
#include "media/audio_def.h"


/*
 *******************************************************************
 *						Audio Common Config
 *******************************************************************
 */
const int config_jlstream_bind_bt_name_enable = 0;
const int config_jlstream_multi_thread_enable = 1; //音频流多线程使能
const int config_multi_thread_self_adaption_enable = 0;
const int config_dac_cache_msec = TCFG_AUDIO_DAC_BUFFER_TIME_MS - 5;

const int config_media_24bit_enable = MEDIA_24BIT_ENABLE;

const int config_seamless_recorder_enable = 1;

#if TCFG_JLSTREAM_TURBO_ENABLE
const int config_jlstream_turbo_enable = 1;
#else
const int config_jlstream_turbo_enable = 0;
#endif

/* 最大录音数据缓存大小,
 * 如果打印"too_much_enc_data"表示设备写入速度不够,需要提高设备写入速度或增大缓存
 */
const int config_max_enc_data_cache_size = 16 * 1024;

//输出缓存低于该阈值触发一个中断对数据做淡出
const float const_out_dev_pns_time_ms = 1.0f;

/*
 *******************************************************************
 *						Audio Hardware Config
 *******************************************************************
 */
//***********************
//*     Audio ADC       *
//***********************
/*是否支持多个ADC 异步打开功能*/
const int config_adc_async_en = 1;

//***********************
//*     Audio DAC       *
//***********************
#if TCFG_AUDIO_DAC_NOISEGATE_ENABLE
const int config_audio_dac_noisefloor_optimize_enable = BIT(0) | BIT(2);
#else
const int config_audio_dac_noisefloor_optimize_enable = 0;//BIT(1);
#endif/*TCFG_MIC_EFFECT_ENABLE*/

#if TCFG_LOWPOWER_FUNCTION
const int config_audio_dac_delay_off_ms = 300;
#else
const int config_audio_dac_delay_off_ms = 0;
#endif

const int config_vcm_cap_poweron_delay_enable = 1;
const int config_audio_dac_vcm_delay_ms = 20;

//***********************
//*     FFT && CORDIC   *
//***********************
enum {
    PLATFORM_FREQSHIFT_CORDIC = 0,
    PLATFORM_FREQSHIFT_CORDICV2 = 1
};

#define PLATFORM_PARM_SEL  PLATFORM_FREQSHIFT_CORDICV2

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
//混音器声音突出功能使能
const int config_audio_mixer_ch_highlight_enable = 0;

//解码读文件缓存buf大小,如果遇到卡速较慢,播放高码率文件有卡顿情况，可增加此缓存大小
const int config_music_file_buf_cnt = 1;
const int config_music_file_buf_block = 188;
const int config_music_file_buf_size = config_music_file_buf_block * config_music_file_buf_cnt;

//***********************
//*	   common Codec     *
//***********************
const int config_dec_support_channels = 2;   	 //  支持的最大声道数
const int config_dec_support_samplerate = AUDIO_DEC_MAX_SAMPERATE; //  支持的最大采样率

//***********************
//*     MP3 Codec       *
//***********************
const int MP3_SEARCH_MAX = 20;      //本地解码设成200， 网络解码可以设成3
const int MP3_TGF_TWS_EN = 1;       //tws解码使能
const int MP3_TGF_POSPLAY_EN = 1;   //定点播放 获取ms级别时间 接口使能
const int MP3_TGF_AB_EN = 1;        //AB点复读使能
const int MP3_TGF_FASTMO = 0;       //快速解码使能(默认关闭，之前给一个sdk单独加的，配置是否解高频，解双声道等)
const int MP3_OUTPUT_LEN = 1;       //解码一次输出点数，1代表32对点，n就是n*32对点

#define FAST_FREQ_RESTRICT				0x01 //限制超过16k的频率不解(一般超出人耳听力范围，但是仪器会测出来)
#define FAST_FILTER_RESTRICT			0x02 //限制滤波器长度(子带滤波器旁瓣加大，边缘不够陡)
#define FAST_CHANNEL_RESTRICT			0x04 //混合左右声道，再解码(如果是左右声道独立性较强的歌曲，会牺牲空间感，特别耳机听会听出来的话)

const int config_mp3_dec_speed_mode = 0;//FAST_FREQ_RESTRICT | FAST_FILTER_RESTRICT | FAST_CHANNEL_RESTRICT; //3个开关都置上，是最快的解码模式
const int config_mp3_enc_use_layer_3 = TCFG_ENC_MP3_TYPE;
const int const_audio_mp3_dec16_fifo_precision = 16;    //24或者16

const int mp3encode_input_mode = 1; //0x01--short输入 0x02--float输入,使用mp3浮点数输入编码需要把config_mp3_enc_use_layer_3置1

//***********************
//*     M4A Codec       *
//***********************
const int const_audio_m4a_dec16_fifo_precision = 16;  //  24 或者 16

//***********************
//*     WAV Codec       *
//***********************
const int WAV_MAX_BITRATEV = (48 * 6 * 32); // wav最大支持比特率，单位kbps,//采样率*声道*数据位宽

const int WAV_DECODER_PCM_POINTS = 128;     // 解码一次输出点数,建议范围32到900,例如128代表128对点

//wav解码声道配置，
//0:不限制解码输出声道数，按照配置的支持最大解码声道(CONFIG_DEC_SUPPORT_CHANNELS)解码，超过就返回不支持。
//1:无论声道数是多少，限制解码输出前面2个声道,多声道的文件可能跑不过来;
const int WAV_DECODER_OUTPUT_CHANNEL_LIMIT = 0;

const int const_audio_codec_wav_checkdst_enable = 1;            //如果wav解码前面有经过dts解码库做过格式检查，就把这个设成0
const int const_audio_codec_wav_dec_support_aiff = 0;           //是否支持aif，因为get_aif_ops跟 wav解码 是 共用run函数（有分叉），所以需要通过const才能优化掉
const int const_audio_codec_wav_dec_support_24bit = MEDIA_24BIT_ENABLE; //24bit开关
const int const_audio_codec_wav_dec_support_AB_Repeat_en = 1;   //是否支持AB点跟循环
const int const_audio_codec_wav_dec_supoort_POS_play = 1;       //是否支持指定位置播放
const int const_audio_codec_wav_dec_bitDepth_set_en = 0;

//***********************
//*     ADPCM Codec     *
//***********************
const int config_sel_adpcm_type = TCFG_ENC_ADPCM_TYPE;//1：使用imaen_adpcm,  0:msen_adpcm


//***********************
//*     MTY Codec       *
//***********************
const int config_mty_repeat_enable = 1; //mty 支持循环播放

//***********************
//*     WMA Codec       *
//***********************
const int WMA_TWSDEC_EN = 0;    //wma tws 解码控制
const int WMA_ABpoint_EN = 1;   //是否支持AB点以及AB点循
const int WMA_OUTPUT_LEN = 1;   //解码一次输出点数，1代表32对点，n就是n*32对点

const int const_audio_codec_wma_dec_support_24bit = MEDIA_24BIT_ENABLE;
const int const_audio_wma_dec16_fifo_precision = 16;    //24或者16，控制16bit输出的时候fifo精度，影响16bit输出的申请的buf大小
const int const_audio_codec_wma_dec_supoort_POS_play = 1; //是否支持指定位置播放

//***********************
//*     OPUS Codec      *
//***********************
const int config_opus_srindex = 0; //选择opus解码文件的帧大小，0代表一帧40字节，1代表一帧80字节，2代表一帧160字节

//***********************
//*     SPEEX Codec     *
//***********************
const int speex_max_framelen = 106; //设置speex编码库最大读数大小

//***********************
//*     APE Codec       *
//***********************
const int APE_DEC_SUPPORT_LEVEL = 2;    //最高支持的层数  0:Fast   1:Normal    2:High

//***********************
//*     AMR Codec       *
//***********************
const int amr_support_16k_dec = TCFG_DEC_AMR_16K_ENABLE;
const int amr_support_16k_enc = TCFG_ENC_AMR_16K_ENABLE;

//***********************
//*     TONE Codec      *
//***********************
/* f2a解码常量设置 */
const int F2A_JUST_VOL = 0;     //帧长是否不超过512，置1的话，需要的buf会变小。默认置0，buf为11872.
const int WTGV2_STACK2BUF = 0;  //等于1时解码buf会加大760，栈会减小
const int silk_fsN_enable = 1;  //支持8-12k采样率
const int silk_fsW_enable = 1;  //支持16-24k采样率

//***********************
//*     LC3 Codec       *
//***********************
const int config_lc3_encode_i24bit_enable = MEDIA_24BIT_ENABLE; //控制LC3编码输入24bit是否使能
const int config_lc3_decode_o24bit_enable = MEDIA_24BIT_ENABLE; //控制LC3解码输出24bit是否使能

//控制库功能编译
const int LC3_PLC_EN = 1;            			//0_fade,1_时域,2_频域,3静音;
const int LC3_PLC_FADE_OUT_START_POINT = 480;   //丢包后维持音量的点数.
const int LC3_PLC_FADE_OUT_POINTS = 120 * 5;    //丢包维持指定点数后,淡出的速度,音量从满幅到0需要的点数.
const int LC3_PLC_FADE_IN_POINTS = 120 * 5;     //丢包后收到正确包淡入,淡入的速度,音量从0到满幅需要的点数.
const int LC3_HW_FFT = 1;
//LC3帧长使能配置
const char LC3_FRAME_LEN_SUPPORT_25_DMS = 1;    //2.5ms的帧长使能
const char LC3_FRAME_LEN_SUPPORT_50_DMS = 1;    //5ms的帧长使能
const char LC3_FRAME_LEN_SUPPORT_75_DMS = 1;    //7.5ms的帧长使能
const char LC3_FRAME_LEN_SUPPORT_100_DMS = 1;   //10ms的帧长使能
//LC3采样率使能配置
const char LC3_SAMPLE_RATE_SUPPORT_8K = 1;      //8K采样率使能
const char LC3_SAMPLE_RATE_SUPPORT_16K = 1;     //16K采样率使能
const char LC3_SAMPLE_RATE_SUPPORT_24K = 1;     //24K采样率使能
const char LC3_SAMPLE_RATE_SUPPORT_32K = 1;     //32K采样率使能
const char LC3_SAMPLE_RATE_SUPPORT_48K = 1;     //48K/44.1K采样率使能
//LC3 编解码24bit使能控制常量:
const int LC3_ENCODE_I24bit_ENABLE = config_lc3_encode_i24bit_enable;    //编码输入pcm数据位宽24比特,符号扩展到S32.   1使能，结合if_s24=1生效.
const int LC3_DECODE_O24bit_ENABLE = config_lc3_decode_o24bit_enable;    //解码输出pcm数据位宽24比特,符号扩展到S32.   1使能，结合if_s24=1生效.


//***********************
//*     JLA Codec      *
//***********************
#if (TCFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA)
const int config_jla_plc_en = 0;            //置1做plc，置0的效果类似补静音包
const int config_jla_plc_fade_in_ms = 30;   // config_jla_plc_en = 0时有效, 淡入时间设置ms;
const int config_jla_encode_i24bit_enable = MEDIA_24BIT_ENABLE; //控制JLA编码输入24bit是否使能
const int config_jla_decode_o24bit_enable = MEDIA_24BIT_ENABLE; //控制JLA解码输出24bit是否使能
const int config_jla_codec_hard_decision_enable = 0;
const int config_jla_codec_soft_decision_enable = 0;

//控制库功能编译
const int JLA_ENCODE_I24bit_ENABLE = config_jla_encode_i24bit_enable;
const int JLA_DECODE_O24bit_ENABLE = config_jla_decode_o24bit_enable;
const int JLA_HW_FFT = 1;
const int JLA_PLC_EN = 0;   //置1做plc，置0的效果类似补静音包
const int JLA_QUALTIY_CONFIG = 4;//【范围1到4， 1需要的速度最少，这个默认先配4】
#ifdef TCFG_LE_AUDIO_CODEC_FRAME_LEN
const int JLA_DMS_VAL = TCFG_LE_AUDIO_CODEC_FRAME_LEN;  //单位ms, 【只支持 25,50,100】
#else
const int JLA_DMS_VAL = 100;    //单位ms, 【只支持 25,50,100】
#endif
#ifndef TCFG_JLA_CODING_SAMPLERATE
const int JLA_DMS_FSINDEX = 4;
#else
#if (TCFG_JLA_CODING_SAMPLERATE <= 8000)
const int JLA_DMS_FSINDEX = 0;
#elif (TCFG_JLA_CODING_SAMPLERATE <= 16000)
const int JLA_DMS_FSINDEX = 1;
#elif (TCFG_JLA_CODING_SAMPLERATE <= 24000)
const int JLA_DMS_FSINDEX = 2;
#elif (TCFG_JLA_CODING_SAMPLERATE <= 32000)
const int JLA_DMS_FSINDEX = 3;
#elif (TCFG_JLA_CODING_SAMPLERATE <= 48000)
const int JLA_DMS_FSINDEX = 4;
#endif
#endif
const int JLA_PLC_FADE_OUT_START_POINT = 480;   //丢包后维持音量的点数.
const int JLA_PLC_FADE_OUT_POINTS = 120 * 5;    //丢包维持指定点数后,淡出的速度,音量从满幅到0需要的点数.
const int JLA_PLC_FADE_IN_POINTS = 120 * 5;     //丢包后收到正确包淡入,淡入的速度,音量从0到满幅需要的点数.
#endif



//***********************
//*    JLAV2 Codec      *
//***********************
//{32, 40, 48, 60, 64, 80, 96, 120, 128, 160, 240, 320, 480}; 0~12. 编码支持得输入点数

//0~12位: 编码支持得输入点数配置, 代码优化使用，可以禁用掉不用的点数,节省代码量
//最高位： 延时模式配置 	1: 延时=帧长点数.  0:延时1/4帧点.  注意： 160,240,320,480 固定延时1/4帧 不受配置影响.
const unsigned short JLA_V2_FRAMELEN_MASK = 0xffff;

//是否支持24bit编解码
const int JLA_V2_ENCODE_I24bit_ENABLE = MEDIA_24BIT_ENABLE;
const int JLA_V2_DECODE_O24bit_ENABLE = MEDIA_24BIT_ENABLE;

//HW_FFT配置  支持非2的指数次幂点的硬件FFT版本 可以设置为1调用硬件FFT加速运算
//不支持的点数    fr_idx = 0/2/6.  对应帧长32/48/96点.
const int JLA_V2_HW_FFT = 1;    //br27/br28置1，其他芯片置0

const int JLA_V2_PLC_EN = 2;    //pcl类型配置：0_fade,1_时域plc,2_频域plc,3补静音包;
const int JLA_V2_PLC_FADE_OUT_START_POINT = 480;   //plc维持音量的点数.
const int JLA_V2_PLC_FADE_OUT_POINTS = 120 * 5;    //plc维持指定点数后,淡出的速度,音量从满幅到0需要的点数.
const int JLA_V2_PLC_FADE_IN_POINTS = 120 * 5;     //plc后收到正确包淡入,淡入的速度,音量从0到满幅需要的点数.


//***********************
//*     JLA_LL Codec    *
//***********************
#if (TCFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LL)

#define JLA_LL_ORDER1  0 //jla_ll 一阶编码
#define JLA_LL_ORDER2  1 //jla_ll 二阶编码

#define JLA_LL_CODING_ORDER_TYPE    JLA_LL_ORDER1 //JLA_LL 编码阶数类型配置

#define JLA_LL_CODEC_INPUT_POINT  	(TCFG_LE_AUDIO_CODEC_SAMPLERATE * TCFG_LE_AUDIO_CODEC_FRAME_LEN  * TCFG_LE_AUDIO_CODEC_CHANNEL / 10 / 1000)
#if (JLA_LL_CODING_ORDER_TYPE == JLA_LL_ORDER1)
//编码压缩比配置。0 ~ JLA_LL_CODEC_INPUT_POINT,  0 :压缩率最高,
//长度需要减少N个byte ,则((JLA_LL_CODEC_INPUT_POINT - 8 * 2 * (N >> 1) - (N & 1) * 4) )
#define JLA_LL_CODEC_CR_CONFIG      ((JLA_LL_CODEC_INPUT_POINT - 8 * 2 * (0 >> 1) - (0 & 1) * 4) )
#else
//长度需要减少N个byte ,则(JLA_LL_CODEC_INPUT_POINT - 8 * N)
#define JLA_LL_CODEC_CR_CONFIG  	(JLA_LL_CODEC_INPUT_POINT - 8 * 0)
#endif//JLA_LL_CODING_ORDER_TYPE == JLA_LL_ORDER1

const int JLA_LL_CODING_TYPE = JLA_LL_CODING_ORDER_TYPE;
const int JLA_LL_CODEC_POINT = JLA_LL_CODEC_INPUT_POINT;
const int JLA_LL_CODEC_CR = JLA_LL_CODEC_CR_CONFIG;
const int JLA_LL_PLC_EN = 1;
const int JLA_LL_PLC_FSPEED = 120;//PLC连续丢包时的衰减系数，建议 70到124，不得超过127， 越小衰减越快
#endif

//***********************
//*     JLA_LW Codec    *
//***********************
#if (CTFG_LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LW)
const int JLA_LW_PLC_EN = 1;
const int JLA_LW_PLC_FADE_OUT_START_POINT = 120;
const int JLA_LW_PLC_FADE_OUT_POINTS = 120;
const int JLA_LW_PLC_FADE_IN_POINTS = 120;

//调参数影响对应频带位流分配,分别对应频率{0 ~ sample/2}
//+X 表示压制，-X 标志提升,建议±10以内.最高±100以内
//编解码必须要配置一致。
const int JLA_LW_BITSTREAM_WEIGHT_TAB[8] = { -2, 0, 0, 0, 4, 7, 9, 12 };
#endif


//***********************
//*     LE Audio        *
//***********************
#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN) || (TCFG_LEA_CIG_CENTRAL_EN || TCFG_LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
const int LE_AUDIO_TIME_ENABLE = 1;
#else
const int LE_AUDIO_TIME_ENABLE = 0;
#endif

//***********************
//*     MIDI Codec      *
//***********************
#ifdef CONFIG_MIDI_DEC_ADDR
const int MIDI_TONE_MODE = 0;   //音色访问方式  0为地址访问(仅支持在内置flash)，1为文件访问(内置、外挂flash,sd,u盘均可)
#else
const int MIDI_TONE_MODE = 1;
#endif
const int MAINTRACK_USE_CHN = 0; //主旋律选定方式0 为轨道  1为通道
const int MAX_DEC_PLAYER_CNT = 18; //MIDI解码最大同时播放的key数，立体声音色按下一个key 占播放两个key
const int MAX_CTR_PLAYER_CNT = 18; //MIDI琴最大同时播放的key数，立体声音色按下一个key 占播放两个key
const int NOTE_OFF_TRIGGER = 0;   //MIDI琴NOTE_OFF回调，设置为1 time 传0不回调
const int MIDI_TONE_CURVE = 1;    //音色文件访问时，如果需使用包络必需置为1
const int MIDI_SAVE_DIV_ENBALE = 1; //小节回退功能，如不使用该功能可置为0。减少midi解码运行buf
const int MIDI_DEC_SR = 44100;     //输出采样率配置

//***********************
//*     MSBC Codec      *
//***********************
const int config_msbc_decode_input_frame_replace = CONFIG_MSBC_INPUT_FRAME_REPLACE_DISABLE;
const int config_msbc_encode_used_software_enable = 1;  //使用msbc软件编码

////////////////////ID3信息使能/////////////////
const int config_flac_id3_enable = 0;
const int config_ape_id3_enable  = 0;
const int config_m4a_id3_enable  = 0;
const int config_wav_id3_enable  = 0;
const int config_wma_id3_enable  = 0;

/*
 *******************************************************************
 *						Audio Music Playback Config
 *******************************************************************
 */
//重复播放
#if FILE_DEC_REPEAT_EN
const int config_file_dec_repeat_en = 1;
#else
const int config_file_dec_repeat_en = 0;
#endif
//指定位置播放
#if FILE_DEC_DEST_PLAY
const int config_file_dec_dest_play = 1;
#else
const int config_file_dec_dest_play = 0;
#endif
//快进快退到文件end返回结束消息
const int config_decoder_ff_fr_end_return_event_end = 0;

/*
 *******************************************************************
 *						Audio Effects Config
 *******************************************************************
 */
//***********************
//* 	 EQ             *
//***********************
const int config_audio_eq_hp_enable = 1;		//High Pass
const int config_audio_eq_lp_enable = 1;		//Low Pass
const int config_audio_eq_bp_enable = 1;		//Band Pass(Peaking)
const int config_audio_eq_hs_enable = 1;		//High Shelf
const int config_audio_eq_ls_enable = 1;		//Low Shelf
const int config_audio_eq_hs_q_enable = 1;		//High Shelf Q
const int config_audio_eq_ls_q_enable = 1;		//Low Shelf Q
const int config_audio_eq_hp_adv_enable = 1;	//High Pass Advance
const int config_audio_eq_lp_adv_enable = 1;	//Low Pass Advance

const int AUDIO_EQ_MAX_SECTION = EQ_SECTION_MAX;

#if TCFG_EQ_ENABLE
const int config_audio_eq_en = EQ_EN | EQ_ASYNC_EN
#if TCFG_CROSSOVER_NODE_ENABLE
                               | EQ_HW_CROSSOVER_TYPE0_EN
#endif/*TCFG_CROSSOVER_NODE_ENABLE*/
                               /* | EQ_FADE_TACTICS_SEL    //相同节点不同的eq段数淡入切换使能(先总体淡出，再淡入到目标，耗时较长，默认关闭)*/
                               ;
#else
const int config_audio_eq_en = 0;
#endif/* TCFG_EQ_ENABLE */

const int const_eq_debug = 0;

//***********************
//* 	 PLC            *
//***********************
/*
 *播歌PLC仅淡入淡出配置,
 *0 - 补包运算(Ram 3660bytes, Code 1268bytes)
 *1 - 仅淡出淡入(Ram 688bytes, Code 500bytes)
 */
const int LPC_JUST_FADE = 0;

//影响plc申请的buf大小跟速度，这个值越大，申请的buf越多，速度也越快。
//增加的buf大小是  APLC_MOV_STAKLEN *类型(16bit是 sizeof(short), 32bit 是sizeof(int))
const int APLC_MOV_STAKLEN = 1024;
//是否使能24bit数据丢包时按照16bit修复，影响ram的使用
const int lfaudio_plc_mode24bit_16bit_en = 0;
/*
   不同配置的ram使用情况
-----------------------------------------------------------------------
  APLC_MOV_STAKLEN                |        0        |       1024      |
-----------------------------------------------------------------------
  lfaudio_plc_mode24bit_16bit_en  |   0    |   1    |    0    |   1   |
-----------------------------------------------------------------------
	ram(byte) ch=1                |  7580  |  5632  |  11676  |  7680 |
-----------------------------------------------------------------------
	ram(byte) ch=2                |  14828 |  9256  |  18924  | 11304 |
-----------------------------------------------------------------------
 */

/*
 * 通话PLC延时配置,支持 0【延时最大】，1，2【延时最小】配置
 * 16k:0:28.5ms, 1:17ms, 2:12.5ms
 *  8k:0:24.5ms, 1:22ms, 2:18ms
 */
const int ESCO_PLC_DELAY_CONTROL        = 0;
const int ESCO_PLC_SUPPORT_24BIT_EN     = MEDIA_24BIT_ENABLE;  //24bit开关
const int ESCO_PLC_FADE_OUT_START_POINT = 500;  //丢包后修复过程中，维持音量的点数.即修复这么多点后，开始淡出
const int ESCO_PLC_FADE_OUT_POINTS      = 2048; //丢包维持指定点数后,淡出的速度,音量从满幅到0需要的点数. 即淡出完需要的点数
const int ESCO_PLC_FADE_IN_POINTS       = 32;   //丢包后收到正确包淡入,淡入的速度,音量从0到满幅需要的点数.即淡入完需要的点数

//1:在配置的淡出点数结束之前，根据信号的特征如果认为已经修不好了，提前快速淡出，
//0:按照实际配置的淡出点数淡出
const int  ESCO_PLC_ADV_ENABLE          = 1;

//***********************
//*   Voice Changer     *
//***********************
const int vc_pitchshift_fastmode_flag = 1; //变声快速模式使能
const int vc_pitchshift_only = 0;
const int voicechange_mathfun_PLATFORM = PLATFORM_PARM_SEL;

//***********************
//*   Howling Suppress  *
//***********************
const int howling_freshift_PLATFORM = PLATFORM_PARM_SEL;
const int howling_freshift_highmode_flag = 0; //移频快速模式
const int howling_pitchshift_fastmode_flag = 1;//移频啸叫抑制快速模式使能
const int howling_freqshift_lowdelay = 0;//0:fir分支，跟以前效果一致, 1:使用iir分支来节省延时

#if TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE
const int config_audio_frequency_shift_howling_enable = 1; //啸叫抑制-移频使能
#else
const int config_audio_frequency_shift_howling_enable = 0;
#endif
#if TCFG_NOTCH_HOWLING_NODE_ENABLE
const int config_audio_notch_howling_enable = 1; //啸叫抑制-陷波使能
#else
const int config_audio_notch_howling_enable = 0;
#endif

//***********************
//*   	ECHO            *
//***********************
const int ECHO_INT_VAL_OUT = 0; //置1: echo的输出是int 后级需接DRC限幅 功能未实现
const int config_auido_echo_delay_ms = 200; //回声最大延时(单位ms),值越大RAM需求就越大
const int DOWN_S_FLAG = 0; //混响降采样处理使能

//***********************
//*   	REVERB          *
//***********************
const int PLATE_REVERB_ROOM_SIZE_Mutiplier = 2; //影响了plateReverb的nee_buf的大小( 约等于 33k * PLATE_REVERB_ROOM_SIZE_Mutiplier)，对应的是roomsize=100对应的是多大
#if TCFG_PLATE_REVERB_NODE_ENABLE
const int config_audio_effect_reverb_enable = 1;
#else
const int config_audio_effect_reverb_enable = 0;
#endif
#if TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE
const int config_audio_effect_reverb_adv_enable = 1;
#else
const int config_audio_effect_reverb_adv_enable = 0;
#endif

//***********************
//*   	NoiseGate       *
//***********************
#if TCFG_NOISEGATE_NODE_ENABLE
const int config_audio_effect_nsgate_enable = 1;
#else
const int config_audio_effect_nsgate_enable = 0;
#endif
#if TCFG_NOISEGATE_PRO_NODE_ENABLE
const int config_audio_effect_nsgate_pro_enable = 1;
#else
const int config_audio_effect_nsgate_pro_enable = 0;
#endif

//***********************
//*   	Others          *
//***********************
const int RS_FAST_MODE_QUALITY = 2;   //软件变采样 滤波阶数配置，范围2到8， 8代表16阶的变采样模式 ,速度跟它的大小呈正相关
const int config_tws_tone_player_reference_clock = 0; //0 - 默认使用经典蓝牙时钟，1 - 使用经典蓝牙网络转为本地参考时钟(避免时钟域的冲突)

/*
 *******************************************************************
 *						A2DP_PREEMPTED
 *******************************************************************
 */
#if TCFG_A2DP_PREEMPTED_ENABLE
const int config_a2dp_energy_calc_enable = 0;
#else
const int config_a2dp_energy_calc_enable = 1;
#endif

/*
 *******************************************************************
 *						DEBUG_ENABLE
 *******************************************************************
 */
#if TCFG_DEBUG_ENABLE
const int config_jlstream_log_output_enable = 1;
#else
const int config_jlstream_log_output_enable = 0;
#endif

/*
 *******************************************************************
 *						EQUALL_LOUDNESS_ENABLE
 *******************************************************************
 */
#if TCFG_EQUALL_LOUNDNESS_ENABLE
const int config_equall_loundness_en = 1;
#else
const int config_equall_loundness_en = 0;
#endif

/*
 *******************************************************************
 *						Audio Smart Voice Config
 *******************************************************************
 */
/*杰理KWS关键词识别*/
#if TCFG_SMART_VOICE_ENABLE
const int config_kws_wake_word_model_enable = 0;
const int config_kws_multi_cmd_model_enable = 1;
#if TCFG_CALL_KWS_SWITCH_ENABLE
const int config_kws_call_cmd_model_enable  = 1;
#else
const int config_kws_call_cmd_model_enable  = 0;
#endif/*TCFG_CALL_KWS_SWITCH_ENABLE*/
const int config_kws_online_model = 1;

#if (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_CH)
/*参数位置选择
 * 0 : 全部放在flash
 * 1 : 全部放在ram
 * 2 : 一部分放在flash，一部分放在ram
 * */
const int CONFIG_KWS_RAM_USE_ENABLE = 1;
const float config_kws_multi_confidence[] = {
    0.4, 0.4, 0.4, 0.4, //小杰小杰，小杰同学，播放音乐，停止播放
    0.4, 0.4, 0.4, 0.4, //暂停播放，增大音量，减小音量，上一首
    0.4, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};
const float config_kws_call_confidence[] = {
    0.4, 0.4, //接听电话，挂断电话
};
#elif (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_FAR_CH)
const int CONFIG_KWS_RAM_USE_ENABLE = 1;
const float config_kws_multi_confidence[] = {
    0.58, 0.5, 0.5, 0.5, //小杰小杰，小杰同学，播放音乐，停止播放
    0.47, 0.45, 0.47, 0.51, //暂停播放，增大音量，减小音量，上一首
    0.55, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};
const float config_kws_call_confidence[] = {
    0.4, 0.4, //接听电话，挂断电话
};
#else /*TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_INDIA_EN*/
const int CONFIG_KWS_RAM_USE_ENABLE = 2;
const float config_kws_multi_confidence[] = {
    0.4, 0.4, 0.3, 0.4, //小杰小杰，小杰同学，播放音乐，停止播放
    0.4, 0.4, 0.3, 0.4, //暂停播放，增大音量，减小音量，上一首
    0.3, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};
const float config_kws_call_confidence[] = {
    0.4, 0.3, //接听电话，挂断电话
};
#endif /*TCFG_AUDIO_KWS_LANGUAGE_SEL*/
#else
const int config_kws_wake_word_model_enable = 0;
const int config_kws_multi_cmd_model_enable = 0;
const int config_kws_call_cmd_model_enable  = 0;
const int config_kws_online_model = 0;
#endif/*TCFG_SMART_VOICE_ENABLE*/

const int config_audio_sync_resample_use_sram = 0;  //播放同步src使用内部ram
const int config_resample_pcm_frame_buffer = 256;
const int config_src_v2_select_mode = 0;
const int config_src_v2_taplen      = 96;
const int config_fast_aligned_pcm_silence = 1;  //音频同步快速对齐的数字音频默音配置 0 - 关闭, 1 - 打开
const float config_sync_into_silenc_threshold = 0.08f;  //音频同步调整幅度过大时，进入silence的阈值。 CONFIG_FAST_ALIGNED_PCM_SILENCE = 1 时生效
const int config_dev_sync_enable = 1;   //接到同个分流器下的多设备同步

/*
 *******************************************************************
 *						Audio Log Debug Config
 *
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 *******************************************************************
 */
const int config_stream_frame_debug     = 0;

const int config_jlstream_scene_debug = 0xff;

const char log_tag_const_v_EQ = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_EQ = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_EQ = CONFIG_DEBUG_LIB(const_eq_debug);
const char log_tag_const_w_EQ = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_EQ = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUD_ADC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUD_ADC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUD_ADC = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUD_ADC = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUD_ADC = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUD_DAC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUD_DAC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUD_DAC = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUD_DAC = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUD_DAC = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_APP_DAC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_APP_DAC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_APP_DAC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_APP_DAC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_APP_DAC = CONFIG_DEBUG_LIB(FALSE);

const char log_tag_const_v_AUD_PDM = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUD_PDM = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUD_PDM = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUD_PDM = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUD_PDM = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_IIS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AUDIO_IIS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUDIO_IIS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_AUDIO_IIS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUDIO_IIS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_IIS = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUD_AUX = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUD_AUX = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUD_AUX = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_AUD_AUX = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_AUD_AUX = CONFIG_DEBUG_LIB(FALSE);

const char log_tag_const_v_AUDIO_NODE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AUDIO_NODE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUDIO_NODE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUDIO_NODE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUDIO_NODE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_NODE = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_DECODER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AUDIO_DECODER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUDIO_DECODER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUDIO_DECODER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUDIO_DECODER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_DECODER = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_ENCODER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AUDIO_ENCODER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUDIO_ENCODER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUDIO_ENCODER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUDIO_ENCODER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_ENCODER = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_CVP = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AUDIO_CVP = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUDIO_CVP = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUDIO_CVP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AUDIO_CVP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_CVP = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_SYNCTS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_SYNCTS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_SYNCTS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_SYNCTS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_SYNCTS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_SYNCTS = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_EFFECTS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_EFFECTS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_EFFECTS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_EFFECTS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_EFFECTS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_EFFECTS = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_JLSTREAM = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_JLSTREAM = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_JLSTREAM = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_JLSTREAM = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_JLSTREAM = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_JLSTREAM = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AEC_USER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AEC_USER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AEC_USER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_AEC_USER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AEC_USER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AEC_USER = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_BT_SYNC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_BT_SYNC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_BT_SYNC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_BT_SYNC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_BT_SYNC = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_BT_SYNC = CONFIG_DEBUG_LIB(FALSE);

const char log_tag_const_v_VOL_NODE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_VOL_NODE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_VOL_NODE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_VOL_NODE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_VOL_NODE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_VOL_NODE = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_VOL_MIXER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_VOL_MIXER = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_VOL_MIXER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_VOL_MIXER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_VOL_MIXER = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_VOL_MIXER = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_SMART_VOICE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_SMART_VOICE = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_SMART_VOICE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_SMART_VOICE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_SMART_VOICE = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_SMART_VOICE = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_APP_CFG_TOOL = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_APP_CFG_TOOL = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_APP_CFG_TOOL = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_APP_CFG_TOOL = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_APP_CFG_TOOL = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_APP_CFG_TOOL = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_EDET = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_AUDIO_EDET = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_AUDIO_EDET = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AUDIO_EDET = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_AUDIO_EDET = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_AUDIO_EDET = CONFIG_DEBUG_LIB(FALSE);

const char log_tag_const_v_LYRICS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_LYRICS = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LYRICS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_LYRICS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LYRICS = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LYRICS = CONFIG_DEBUG_LIB(TRUE);
