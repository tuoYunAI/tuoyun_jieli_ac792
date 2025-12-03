/**
* 注意点：
* 0.此文件变化，在工具端会自动同步修改到工具配置中
* 1.功能块通过【---------xxx------】和 【#endif // xxx 】，是工具识别的关键位置，请勿随意改动
* 2.目前工具暂不支持非文件已有的C语言语法，此文件应使用文件内已有的语法增加业务所需的代码，避免产生不必要的bug
* 3.修改该文件出现工具同步异常或者导出异常时，请先检查文件内语法是否正常
**/

#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#include "jlstream_node_cfg.h"

// ------------板级配置.json------------
#define TCFG_DEBUG_PORT                     IO_PORTD_01
#define TCFG_UBOOT_DEBUG_PORT               PD01

#define TCFG_UART0_ENABLE                   1              //UART0默认用来打印口
#define TCFG_UART0_TX_IO                    TCFG_DEBUG_PORT
#define TCFG_UART0_RX_IO                    IO_PORTE_11    //-1
#define TCFG_UART0_BAUDRATE                 1000000
#define TCFG_UART0_PARITY                   UART_PARITY_DISABLE

#define TCFG_UART1_ENABLE                   1
#define TCFG_UART1_TX_IO                    IO_PORTA_00
#define TCFG_UART1_RX_IO                    IO_PORTA_01
#define TCFG_UART1_BAUDRATE                 1000000
#define TCFG_UART1_FLOW_CTL_ENABLE          0
#define TCFG_UART1_RTS_HW_ENABLE            0
#define TCFG_UART1_RTS_RX_THRESH            80
#define TCFG_UART1_MAX_CONTINUE_RECV_CNT    1024
#define TCFG_UART1_RTS_IO                   -1
#define TCFG_UART1_CTS_IO                   -1
#define TCFG_UART1_RTS_IDLE_LEVEL           0
#define TCFG_UART1_CTS_IDLE_LEVEL           0
#define TCFG_UART1_PARITY                   UART_PARITY_DISABLE

#define TCFG_RTOS_STACK_CHECK_ENABLE 1 // 定时检查任务栈
#define TCFG_SAVE_EXCEPTION_LOG_IN_FLASH 0 // 保存异常打印信息至FLASH
#define TCFG_MEM_LEAK_CHECK_ENABLE 0 // 内存泄漏检测

#define TCFG_CFG_TOOL_ENABLE 0 // FW编辑、在线调音
#if TCFG_CFG_TOOL_ENABLE
#define TCFG_ONLINE_TX_PORT IO_PORT_USB_DPA // 串口引脚TX
#define TCFG_ONLINE_RX_PORT IO_PORT_USB_DMA // 串口引脚RX
#define TCFG_USB_CDC_BACKGROUND_RUN 1 // USB后台模式使能
#define TCFG_COMM_TYPE TCFG_USB_COMM // 通信方式
#endif // TCFG_CFG_TOOL_ENABLE


#define TCFG_AUDIO_DAC_PA_MUTE_EN 0 // DAC功放AUTO MUTE 使能
#define TCFG_AUDIO_DAC_PA_MUTE_PORT IO_PORTE_15 // DAC MUTE IO
#define TCFG_AUDIO_DAC_PA_MUTE_LEVEL 0 // MUTE电平
#define TCFG_AUDIO_DAC_PA_MUTE_DELAY_MS 2000 // MUTE 延迟
#define TCFG_ALINK0_IIS_SET 0 // IIS模块选择
#define TCFG_ALINK0_ROLE 0 // 主从配置
#define TCFG_ALINK0_MCLK_IO 0xdba6 // MCLK_IO
#define TCFG_ALINK0_SCLK_IO 0x4f66 // SCLK_IO
#define TCFG_ALINK0_LRCLK_IO 0x4f67 // LRCLK_IO
#define TCFG_ALINK0_CHANNLE0_ENABLE 1 // 使能
#define TCFG_ALINK0_CHANNEL0_MODE 1 // 输入输出
#define TCFG_ALINK0_DATA0_IO 0xdba3 // DATA0_IO
#define TCFG_ALINK0_CHANNLE1_ENABLE 1 // 使能
#define TCFG_ALINK0_CHANNEL1_MODE 1 // 输入输出
#define TCFG_ALINK0_DATA1_IO 0xdba4 // DATA1_IO
#define TCFG_ALINK0_CHANNLE2_ENABLE 1 // 使能
#define TCFG_ALINK0_CHANNEL2_MODE 0 // 输入输出
#define TCFG_ALINK0_DATA2_IO 0xdba5 // DATA2_IO
#define TCFG_ALINK0_CHANNLE3_ENABLE 1 // 使能
#define TCFG_ALINK0_CHANNEL3_MODE 0 // 输入输出
#define TCFG_ALINK0_DATA3_IO 0xdba7 // DATA3_IO
// ------------板级配置.json------------

// ------------功能配置.json------------
#define TCFG_APP_BT_EN 1 // 蓝牙模式
#define TCFG_APP_MUSIC_EN 1 // 音乐模式
#define TCFG_APP_LINEIN_EN 1 // LINEIN模式
#define TCFG_APP_NET_MUSIC_EN 1 // 网络播放模式
#define TCFG_APP_PC_EN 1 // PC模式
#define TCFG_APP_RECORD_EN 1 // 录音模式
#define TCFG_APP_USB_HOST_EN 0 // USB_HOST_AUDIO模式
#define TCFG_MIC_EFFECT_ENABLE 1 // 混响使能
#define TCFG_MIX_RECORD_ENABLE 0 // 混合录音使能
#define TCFG_RECORD_AUDIO_REPLAY_EN 0 // 录音回播使能
#define TCFG_REC_FOLDER_NAME "RECORDER" // 录音文件夹名称
#define TCFG_REC_FILE_NAME "rec_****" // 录音文件名称
#define TCFG_DEC_ID3_V2_ENABLE 0 // ID3_V2
#define TCFG_DEC_ID3_V1_ENABLE 0 // ID3_V1
#define FILE_DEC_REPEAT_EN 0 // 无缝循环播放
#define FILE_DEC_DEST_PLAY 0 // 指定时间播放
#define FILE_DEC_AB_REPEAT_EN 0 // AB点复读
#define TCFG_DEC_DECRYPT_ENABLE 0 // 加密文件播
#define TCFG_DEC_DECRYPT_KEY 0x12345678 // 加密KEY
#define TCFG_LRC_LYRICS_ENABLE 0 // 歌词解析
#define TCFG_LRC_ENABLE_SAVE_LABEL_TO_FLASH 0 // 保存歌词时间标签到flash
#define MUSIC_PLAYER_CYCLE_ALL_DEV_EN 1 // 循环播放模式是否循环所有设备
#define MUSIC_PLAYER_PLAY_FOLDER_PREV_FIRST_FILE_EN 0 // 切换文件夹播放时从第一首歌开始
// ------------功能配置.json------------


// ------------公共配置.json------------
#define TCFG_LE_AUDIO_STREAM_ENABLE 1 // 公共配置
#if TCFG_LE_AUDIO_STREAM_ENABLE
#define TCFG_LE_AUDIO_APP_CONFIG (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN) // le_audio 应用选择
#define TCFG_LE_AUDIO_CODEC_TYPE 4194304 // 编解码格式
#define TCFG_LE_AUDIO_CODEC_CHANNEL 2 // 编解码声道数
#define TCFG_LE_AUDIO_CODEC_FRAME_LEN 100 // 帧持续时间
#define TCFG_LE_AUDIO_CODEC_SAMPLERATE 48000 // 采样率
#define TCFG_LEA_TX_DEC_OUTPUT_CHANNEL 37 // 发送端解码输出
#define TCFG_LEA_RX_DEC_OUTPUT_CHANNEL 37 // 接收端解码输出
#define TCFG_LE_AUDIO_PLAY_LATENCY 30000 // le_audio延时（us）
#endif // TCFG_LE_AUDIO_STREAM_ENABLE

#define TCFG_KBOX_1T3_MODE_EN 0 // 1T3使能
// ------------公共配置.json------------

// ------------BIS配置.json------------
#define TCFG_LEA_BIG_CTRLER_TX_EN 0 // 发送使能
#define TCFG_LEA_BIG_CTRLER_RX_EN 0 // 接收使能
#define TCFG_LEA_BIG_CUSTOM_DATA_EN 1 // 自定义数据同步
#define TCFG_LEA_BIG_VOL_SYNC_EN 1 // 音量同步
#define TCFG_LEA_BIG_RX_CLOSE_EDR_EN 0 // 接收端关EDR
#define TCFG_LEA_LOCAL_SYNC_PLAY_EN 1 // 本地同步播放
#define TCFG_LEA_BIG_FIX_ROLE 0 // 广播角色
// ------------BIS配置.json------------

// ------------CIS配置.json------------
#define TCFG_LEA_CIG_CENTRAL_EN 0 // 主机使能
#define TCFG_LEA_CIG_PERIPHERAL_EN 0 // 从机使能
#define TCFG_LEA_CIG_CENTRAL_CLOSE_EDR_CONN 0 // 主机关闭EDR
#define TCFG_LEA_CIG_PERIPHERAL_CLOSE_EDR_CONN 1 // 从机关闭EDR
#define TCFG_LEA_CIG_KEY_EVENT_SYNC 0 // 按键同步
#define TCFG_LEA_CIG_FIX_ROLE 1 // 连接角色
#define TCFG_LEA_CIG_CONNECT_MODE 2 // 连接方式
#define TCFG_LEA_CIG_TRANS_MODE 1 // 音频传输方式
// ------------CIS配置.json------------

// ------------网络配置.json------------
#define TCFG_IPERF_ENABLE 1 // iperf测试
#define TCFG_SERVER_ASSIGN_PROFILE 1 // 第三方平台profile由杰理服务器分配
#define TCFG_PROFILE_UPDATE 0 // 第三方平台profile更新
#define TCFG_AI_SERVER TCFG_AI_SERVER_DISABLE // 第三方平台选择
#define TCFG_HTTP_SERVER_ENABLE 0 // HTTP服务器
#define TCFG_FTP_SERVER_ENABLE 0 // FTP服务器
#define TCFG_DLNA_SDK_ENABLE 1 // DLNA音乐播放
#define TCFG_STATIC_IPADDR_ENABLE 0 // 静态IP地址使能
#define TCFG_ASSIGN_MACADDR_ENABLE 1 // 初始MAC地址分配
// ------------网络配置.json------------

// ------------音频配置.json------------
#define TCFG_AUDIO_DAC_CONNECT_MODE DAC_OUTPUT_MONO_L // 声道配置
#define TCFG_AUDIO_DAC_MODE DAC_MODE_DIFF // 输出模式
#define TCFG_DAC_PERFORMANCE_MODE DAC_MODE_HIGH_PERFORMANCE // 性能模式
#define TCFG_AUDIO_DAC_HP_PA_ISEL0 6 // PA_ISEL0
#define TCFG_AUDIO_DAC_HP_PA_ISEL1 6 // PA_ISEL1
#define TCFG_AUDIO_DAC_HP_PA_ISEL2 3 // PA_ISEL2
#define TCFG_AUDIO_DAC_LP_PA_ISEL0 4 // PA_ISEL0
#define TCFG_AUDIO_DAC_LP_PA_ISEL1 3 // PA_ISEL1
#define TCFG_AUDIO_DAC_LP_PA_ISEL2 3 // PA_ISEL2
#define TCFG_DAC_POWER_LEVEL 0x3 // 音频供电档位
#define TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE 0x0 // 轻量关闭
#define TCFG_AUDIO_VCM_CAP_EN 0x1 // VCM电容
#define TCFG_AUDIO_DAC_BUFFER_TIME_MS 50 // DMA缓存大小
#define TCFG_AUDIO_DAC_IO_ENABLE 0 // DAC IO
#define TCFG_AUDIO_DIGITAL_GAIN 0 // 硬件数字音量

#define TCFG_AUDIO_ADC_ENABLE 1 // ADC配置
#if TCFG_AUDIO_ADC_ENABLE
#define TCFG_ADC0_ENABLE 1 // ADC0使能
#define TCFG_ADC0_MODE 1 // 模式
#define TCFG_ADC0_AIN_SEL 1 // 输入端口
#define TCFG_ADC0_BIAS_SEL 1 // 供电端口
#define TCFG_ADC0_INSIDE_BIAS_RESISTOR_ENABLE 0 // MIC BIAS是否使用内置电阻
#define TCFG_ADC0_BIAS_RSEL 3 // MIC BIAS内置上拉电阻挡位
#define TCFG_ADC0_DCC_LEVEL 14 // DCC 档位
#define TCFG_ADC0_POWER_IO 0 // IO供电选择
#define TCFG_ADC1_ENABLE 1 // ADC1使能
#define TCFG_ADC1_MODE 1 // 模式
#define TCFG_ADC1_AIN_SEL 1 // 输入端口
#define TCFG_ADC1_BIAS_SEL 2 // 供电端口
#define TCFG_ADC1_INSIDE_BIAS_RESISTOR_ENABLE 0 // MIC BIAS是否使用内置电阻
#define TCFG_ADC1_BIAS_RSEL 3 // MIC BIAS内置上拉电阻挡位
#define TCFG_ADC1_DCC_LEVEL 14 // DCC 档位
#define TCFG_ADC1_POWER_IO 0 // IO供电选择
#define TCFG_DMIC_ENABLE 0 // 数字麦使能
#define TCFG_DMIC_IO_SCLK -1 // 数字麦SCLK_IO
#define TCFG_DMIC_SCLK_FREQUENCY 2000000 // 数字麦SCLK频率
#define TCFG_DMIC_IO_IDAT0 -1 // 数字麦IDAT0_IO
#define TCFG_DMIC_IO_IDAT1 -1 // 数字麦IDAT1_IO
#define TCFG_DMIC_CH0_MODE 4 // 数字麦通道0模式
#define TCFG_DMIC_CH1_MODE 5 // 数字麦通道1模式
#endif // TCFG_AUDIO_ADC_ENABLE

#define TCFG_AUDIO_GLOBAL_SAMPLE_RATE 48000 // 全局采样率
#define TCFG_AEC_TOOL_ONLINE_ENABLE 0 // 手机APP在线调试
#define TCFG_AUDIO_CVP_SYNC 0 // 通话上行同步
#define TCFG_AUDIO_DMS_DUT_ENABLE 1 // 通话产测
#define TCFG_ESCO_DL_CVSD_SR_USE_16K 0 // 通话下行固定16K
#define TCFG_TWS_ESCO_MODE TWS_ESCO_MASTER_AND_SLAVE // 通话模式
#define TCFG_AUDIO_SMS_SEL SMS_TDE // 1mic算法选择
#define TCFG_AUDIO_SMS_DNS_VERSION SMS_DNS_V100 // 1micDNS算法选择
#define TCFG_KWS_VOICE_RECOGNITION_ENABLE 0 // 关键词检测KWS
#define TCFG_KWS_MIC_CHA_SELECT AUDIO_ADC_MIC_0 // 麦克风选择
#define TCFG_SMART_VOICE_ENABLE 0 // 离线语音识别
#define TCFG_SMART_VOICE_USE_AEC 0 // 回音消除使能
#define TCFG_SMART_VOICE_MIC_CH_SEL AUDIO_ADC_MIC_1 // 麦克风选择
#define TCFG_AUDIO_KWS_LANGUAGE_SEL KWS_FAR_CH // 模式选择
#define TCFG_DEC_WAV_ENABLE 0 // WAV
#define TCFG_DEC_MP3_ENABLE 1 // MP3
#define TCFG_DEC_M4A_ENABLE 0 // M4A
#define TCFG_DEC_WMA_ENABLE 0 // WMA
#define TCFG_DEC_FLAC_ENABLE 0 // FLAC
#define TCFG_DEC_APE_ENABLE 0 // APE
#define TCFG_DEC_AAC_ENABLE 0 // AAC
#define TCFG_DEC_AMR_ENABLE 0 // AMR
#define TCFG_DEC_AMR_16K_ENABLE 0 // AMR 16k
#define TCFG_DEC_DTS_ENABLE 0 // DTS
#define TCFG_DEC_AIFF_ENABLE 0 // AIFF
#define TCFG_DEC_ALAC_ENABLE 0 // ALAC
#define TCFG_DEC_OGG_OPUS_ENABLE 1 // OGG_OPUS
#define TCFG_DEC_OGG_VORBIS_ENABLE 0 // OGG_VORBIS
#define TCFG_DEC_CVSD_ENABLE 0 // CVSD
#define TCFG_DEC_SBC_SW_ENABLE 0 // SBC
#define TCFG_DEC_MSBC_SW_ENABLE 0 // MSBC
#define TCFG_DEC_JLA_ENABLE 0 // JLA
#define TCFG_DEC_SPEEX_ENABLE 0 // SPEEX
#define TCFG_DEC_F2A_ENABLE 0 // F2A
#define TCFG_DEC_WTG_ENABLE 0 // WTG
#define TCFG_DEC_MTY_ENABLE 0 // MTY
#define TCFG_DEC_WTS_ENABLE 0 // WTS
#define TCFG_ENC_MSBC_SW_ENABLE 0 // MSBC
#define TCFG_ENC_CVSD_ENABLE 0 // CVSD
#define TCFG_ENC_OGG_ENABLE 1 // OGG
#define TCFG_ENC_AAC_ENABLE 0 // AAC
#define TCFG_ENC_SBC_ENABLE 0 // SBC
#define TCFG_ENC_JLA_ENABLE 0 // JLA
#define TCFG_ENC_PCM_ENABLE 1 // PCM
#define TCFG_ENC_AMR_ENABLE 0 // AMR
#define TCFG_ENC_AMR_16K_ENABLE 0 // AMR 16k
#define TCFG_ENC_OPUS_ENABLE 1 // OPUS
#define TCFG_ENC_SPEEX_ENABLE 0 // SPEEX
#define TCFG_ENC_MP3_ENABLE 0 // MP3
#define TCFG_ENC_MP3_TYPE 0 // MP3格式选择
#define TCFG_ENC_ADPCM_ENABLE 0 // ADPCM
#define TCFG_ENC_ADPCM_TYPE 0 // ADPCM格式选择
#define TCFG_DEC_AMR_16K_ENABLE 0 // AMR_16K
#define TCFG_DEC_LC3_ENABLE 0 // LC3
#define TCFG_ENC_LC3_ENABLE 0 // LC3
#define TCFG_ENC_AMR_16K_ENABLE 0 // AMR_16K
// ------------音频配置.json------------
#endif

