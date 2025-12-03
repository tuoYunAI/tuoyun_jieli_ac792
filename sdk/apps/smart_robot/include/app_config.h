#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define TELNET_LOG_OUTPUT

#define AUDIO_ENC_SAMPLE_SOURCE_MIC         0
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK0       1
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK1       2
#define AUDIO_ENC_SAMPLE_SOURCE_IIS0        3
#define AUDIO_ENC_SAMPLE_SOURCE_IIS1        4
#define AUDIO_ENC_SAMPLE_SOURCE_LINEIN      5

#define CONFIG_AUDIO_DEC_PLAY_SOURCE        "dac"

#include "sdk_config.h"

#include "board_config.h"
#define CONFIG_ASR_ALGORITHM_ENABLE
#ifdef CONFIG_ASR_ALGORITHM_ENABLE
#define AISP_ALGORITHM 1 //思必驰双mic唤醒,未授权版本只支持运行10分钟
#define ROOBO_ALGORITHM 2 //ROOBO 单/双MIC唤醒 ROOBO_DUAL_MIC_ALGORITHM ,,,测试版本只支持运行10分钟
#define WANSON_ALGORITHM 3 //华镇算法,测试版只能够正确识别1000次
#define JLKWS_ALGORITHM 4 //杰理算法,现阶段仅用于测试效果
#define CONFIG_ASR_ALGORITHM  AISP_ALGORITHM //本地打断唤醒算法选择
#endif



//编解码器使能
#define CONFIG_JLA_ENC_ENABLE
#define CONFIG_JLA_DEC_ENABLE
#define CONFIG_PCM_DEC_ENABLE
#define CONFIG_PCM_ENC_ENABLE
#define CONFIG_DTS_DEC_ENABLE
#define CONFIG_ADPCM_DEC_ENABLE
#define CONFIG_MP3_DEC_ENABLE
#define CONFIG_MP3_ENC_ENABLE
#define CONFIG_WMA_DEC_ENABLE
#define CONFIG_M4A_DEC_ENABLE
#define CONFIG_WAV_DEC_ENABLE
#define CONFIG_AMR_DEC_ENABLE
#define CONFIG_APE_DEC_ENABLE
#define CONFIG_FLAC_DEC_ENABLE
#define CONFIG_SPEEX_DEC_ENABLE
#define CONFIG_ADPCM_ENC_ENABLE
#define CONFIG_WAV_ENC_ENABLE
#define CONFIG_VAD_ENC_ENABLE
#define CONFIG_VIRTUAL_DEV_ENC_ENABLE
#define CONFIG_OPUS_ENC_ENABLE
#define CONFIG_OPUS_DEC_ENABLE
#define CONFIG_SPEEX_ENC_ENABLE
#define CONFIG_AMR_ENC_ENABLE
#define CONFIG_AEC_ENC_ENABLE
#define CONFIG_DNS_ENC_ENABLE
#define CONFIG_AAC_ENC_ENABLE
#define CONFIG_OGG_DEC_ENABLE

#define CONFIG_REVERB_MODE_ENABLE            //打开混响功能
#define CONFIG_AUDIO_MIX_ENABLE              //打开叠音功能
#define CONFIG_AUDIO_PS_ENABLE               //打开变调变速功能



//*********************************************************************************//
//                            AUDIO_ADC应用的通道配置                              //
//*********************************************************************************//
#define AUDIO_ENC_SAMPLE_SOURCE_MIC         0  //录音输入源：MIC
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK0       1  //录音输入源：数字麦PLNK0
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK1       2  //录音输入源：数字麦PLNK1
#define AUDIO_ENC_SAMPLE_SOURCE_IIS0        3  //录音输入源：IIS0
#define AUDIO_ENC_SAMPLE_SOURCE_IIS1        4  //录音输入源：IIS1
#define AUDIO_ENC_SAMPLE_SOURCE_LINEIN      5  //录音输入源：LINEIN

#define CONFIG_AUDIO_ENC_SAMPLE_SOURCE      AUDIO_ENC_SAMPLE_SOURCE_MIC    //录音输入源选择
#define CONFIG_AUDIO_DEC_PLAY_SOURCE        "dac"                          //播放输出源选择
#define CONFIG_AUDIO_RECORDER_SAMPLERATE    16000                          //录音采样率
#define CONFIG_AUDIO_RECORDER_CHANNEL       1                              //录音通道数
#define CONFIG_AUDIO_RECORDER_DURATION      (30 * 1000)                    //录音时长ms
#define CONFIG_AUDIO_RECORDER_SAVE_FORMAT   "ogg"                          //录音文件保存格式

#define CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE      //四路ADC硬件全开

#define CONFIG_AUDIO_ADC_CHANNEL_L          1        //左mic/aux通道
#define CONFIG_AUDIO_ADC_CHANNEL_R          3        //右mic/aux通道
#define CONFIG_REVERB_ADC_CHANNEL           1        //混响mic通道
#define CONFIG_UAC_MIC_ADC_CHANNEL          1        //UAC mic通道
#define CONFIG_AUDIO_ADC_GAIN               100      //mic/aux增益

#define CONFIG_AISP_MIC0_ADC_CHANNEL        1		//本地唤醒左mic通道
#define CONFIG_AISP_MIC1_ADC_CHANNEL        3		//本地唤醒右mic通道
#define CONFIG_AISP_LINEIN_ADC_CHANNEL      2		//本地唤醒LINEIN回采DAC通道
#define CONFIG_AISP_MIC_ADC_GAIN            80		//本地唤醒mic增益
#define CONFIG_AISP_LINEIN_ADC_GAIN         60		//本地唤醒LINEIN增益

//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
//EQ配置，使用在线EQ时，EQ文件和EQ模式无效。有EQ文件时，默认不用EQ模式切换功能
#define TCFG_EQ_ENABLE                      1     	//支持EQ功能
#define TCFG_EQ_ONLINE_ENABLE               0     	//支持在线EQ调试
#define TCFG_HW_SOFT_EQ_ENABLE              1     	//前3段使用软件运算
#define TCFG_LIMITER_ENABLE                 1     	//限幅器
#define TCFG_DRC_ENABLE                     TCFG_LIMITER_ENABLE
#define TCFG_EQ_FILE_ENABLE                 1     	//从bin文件读取eq配置数据
#define TCFG_EQ_MODE_CHOOSE                 0		//0:多通道共用1个eq, drc功能丰富; 1:多通道独立eq,独立drc, 请替换对应的bin文件

#if (TCFG_EQ_MODE_CHOOSE == 0)
#define TCFG_AUDIO_MDRC_ENABLE              2		//0:不使能低中DRC 1: 多带分频器使能 2: 多带分频后，再做一次全带处理使能
#define TCFG_LAST_WHOLE_DRC_ENABLE          1		//0:不使能最后的全带DRC; 1:使能
#elif (TCFG_EQ_MODE_CHOOSE == 1)
#define TCFG_EQ_DIVIDE_ENABLE               1		//0:前后通道共用EQ/DRC  1:支持EQ/DRC前后声道独立处理
#define TCFG_EQ_SPILT_ENABLE                1		//0:左右通道共用EQ  1:支持EQ左右声道独立处理
#define TCFG_DRC_SPILT_ENABLE               1		//0:左右通道共用DRC 1:支持DRC左右声道独立处理
#endif



//*********************************************************************************//
//                          预留区UI和AUDIO资源配置                                //
//*********************************************************************************//
//注意：ui和audio资源的起始地址和大小, 根据产品生命周期最大情况定义,根据实际需求进行配置
//FLASH后面4K用于一些配置存储，所以禁止覆盖
//具体说明和注意事项请阅读文档

//(1)ui和audio资源 ,如果存在ui资源则位于扩展预留区末尾,
//还存在audio则位于ui项前面，其他配置项则位于它们之前.
/*
#------------------------------|
#  (其他预留区配置项)          |
#------------------------------|<----CONFIG_UI_PACKRES_ADR - CONFIG_AUDIO_PACKRES_LEN = CONFIG_AUDIO_PACKRES_ADR
#  (CONFIG_AUDIO_PACKRES_LEN)  |
#------------------------------|<----__FLASH_SIZE__ - 0x1000 - CONFIG_UI_PACKRES_LEN = CONFIG_UI_PACKRES_ADR
#  (CONFIG_UI_PACKRES_LEN)     |
#------------------------------|<----__FLASH_SIZE__ - 0x1000
#  (4K Reserved)               |
#------------------------------+<----__FLASH_SIZE__
*/

//(2)只有audio资源,则将其位于扩展预留区末尾.
/*
#------------------------------|
#  (其他预留区配置项)          |
#------------------------------|<----__FLASH_SIZE__ - 0x1000 - CONFIG_AUDIO_PACKRES_LEN = CONFIG_AUDIO_PACKRES_ADR
#  (CONFIG_AUDIO_PACKRES_LEN)  |
#------------------------------|<----__FLASH_SIZE__ - 0x1000
#  (4K Reserved)               |
#------------------------------+<----__FLASH_SIZE__
*/

#if defined CONFIG_UI_ENABLE && !defined CONFIG_SDFILE_EXT_ENABLE
#define CONFIG_UI_FILE_SAVE_IN_RESERVED_EXPAND_ZONE //UI资源打包后放在扩展预留区
#endif

#if defined CONFIG_MEDIA_ENABLE && !defined CONFIG_SDFILE_EXT_ENABLE
//#define CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE //AUDIO资源打包后放在扩展预留区
#endif

#if defined CONFIG_UI_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
#define CONFIG_UI_PACKRES_LEN 0x200000
#define CONFIG_UI_PACKRES_ADR ((__FLASH_SIZE__) - (CONFIG_UI_PACKRES_LEN) - 0x1000)
#endif

#if defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
#if defined CONFIG_UI_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
#define CONFIG_AUDIO_PACKRES_LEN 0x180000
#define CONFIG_AUDIO_PACKRES_ADR (CONFIG_UI_PACKRES_ADR - CONFIG_AUDIO_PACKRES_LEN)
#else
#define CONFIG_AUDIO_PACKRES_LEN 0x180000
#define CONFIG_AUDIO_PACKRES_ADR ((__FLASH_SIZE__) - CONFIG_AUDIO_PACKRES_LEN - 0x1000)
#endif
#endif

#define CONFIG_DEBUG_ENABLE                     //打印开关
//#define CONFIG_USER_DEBUG_ENABLE
//#define CONFIG_SYS_DEBUG_DISABLE


//*********************************************************************************//
//                                  WIFI配置    start                              //
//*********************************************************************************//

#ifdef CONFIG_NET_ENABLE
// #define CONFIG_LTE_PHY_ENABLE                //usb网卡
#define CONFIG_WIFI_ENABLE                   //无线WIFI

#ifdef CONFIG_SFC_ENABLE
#ifdef CONFIG_NO_SDRAM_ENABLE
#define CONFIG_RF_TRIM_CODE_MOVABLE //把RF TRIM 的运行代码动态加载到ram运行(节省4K ram内存), 防止RF TRIM 期间500ms大电流访问flash造成flash挂掉持续大电流
#else
#define CONFIG_RF_TRIM_CODE_AT_RAM //把RF TRIM 的运行代码定死到ram运行(浪费4K ram内存,否则若动态加载到sdram需清cache), 防止RF TRIM 期间500ms大电流访问flash造成flash挂掉持续大电流
#endif
#endif

#endif

//*********************************************************************************//
//                                  WIFI配置    end                                 //
//*********************************************************************************//


//*********************************************************************************//
//                                  BT_BLE配置                                     //
//*********************************************************************************//
#ifdef CONFIG_BT_ENABLE

#define TCFG_BT_MODE                                BT_NORMAL   //BT_BQB
#define TCFG_DEF_BLE_NET_CFG
#define TCFG_USER_BLE_ENABLE                        1   //BLE功能使能
#define TCFG_USER_BT_CLASSIC_ENABLE                 0   //经典蓝牙功能

#if TCFG_USER_BLE_ENABLE

#define TCFG_BLE_HID_EN                             0   //从机 hid
#define TCFG_TRANS_DATA_EN                          1   //从机 传输数据
#define TCFG_BLE_MASTER_CENTRAL_EN                  0   //主机 client角色
#define TCFG_TRANS_MULTI_BLE_EN                     0   //多机通讯
#define TCFG_BLE_MESH_ENABLE                        0   //mesh测试demo
#define TCFG_NONCONN_24G_EN                         0   //2.4g加密通讯

#if (TCFG_TRANS_MULTI_BLE_EN + TCFG_BLE_HID_EN + TCFG_BLE_MASTER_CENTRAL_EN + TCFG_TRANS_DATA_EN + TCFG_BLE_MESH_ENABLE + TCFG_NONCONN_24G_EN > 1)
#error "they can not enable at the same time,just select one!!!"
#endif

#define TCFG_BLE_SECURITY_EN                        1   //配对加密使能

#if TCFG_TRANS_MULTI_BLE_EN
#define TCFG_TRANS_MULTI_BLE_SLAVE_NUMS             1
#define TCFG_TRANS_MULTI_BLE_MASTER_NUMS            2
#endif

#endif

#endif




//*********************************************************************************//
//                                MEDIA功能配置                                    //
//*********************************************************************************//
#ifdef CONFIG_MEDIA_ENABLE

#ifndef TCFG_ADC0_INSIDE_BIAS_RESISTOR_ENABLE
#define TCFG_ADC0_INSIDE_BIAS_RESISTOR_ENABLE 0 //MIC0 BIAS是否使用内置电阻
#endif

#ifndef TCFG_ADC1_INSIDE_BIAS_RESISTOR_ENABLE
#define TCFG_ADC1_INSIDE_BIAS_RESISTOR_ENABLE 0 //MIC1 BIAS是否使用内置电阻
#endif

#define TCFG_AUDIO_LINEIN_ENABLE                TCFG_APP_LINEIN_EN
#define SYS_VOL_TYPE                            VOL_TYPE_DIGITAL    /*目前仅支持软件数字音量模式*/

/*
 *第三方ASR（语音识别）配置
 *(1)用户自己开发算法
 *#define TCFG_AUDIO_ASR_DEVELOP                ASR_CFG_USER_DEFINED
 *(2)使用思必驰ASR算法
 *#define TCFG_AUDIO_ASR_DEVELOP                ASR_CFG_AIS
 *(2)使华镇ASR算法
 *#define TCFG_AUDIO_ASR_DEVELOP                ASR_CFG_WANSON
 */
#define ASR_CFG_USER_DEFINED                    1
#define ASR_CFG_AIS                             2
#define ASR_CFG_WANSON                          3
#define TCFG_AUDIO_ASR_DEVELOP                  ASR_CFG_AIS //0

//#define CONFIG_AUDIO_ENC_AEC_DATA_CHECK      //打开查看AEC操作数据(mic/dac/aec数据)

/*使能iis输出外部参考数据*/
#if (TCFG_IIS_NODE_ENABLE == 1) && (TCFG_DAC_NODE_ENABLE == 0)
#define TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE    1 //回声消除参考数据为iis数据
#else
#define TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE    0 //回声消除参考数据为iis数据
#endif

#if TCFG_AUDIO_CVP_SMS_ANS_MODE                 /*单MIC+ANS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              0
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_NORMAL
#elif (TCFG_AUDIO_CVP_SMS_DNS_MODE)             /*单MIC+DNS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              0
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_NORMAL
#elif (TCFG_AUDIO_CVP_DMS_ANS_MODE)             /*双MIC+ANS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              1
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_NORMAL
#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE)             /*双MIC+DNS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              1
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_NORMAL
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE)    /*话务双MIC+ANS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              1
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_FLEXIBLE
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)    /*话务双MIC+DNS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              1
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_FLEXIBLE
#elif (TCFG_AUDIO_CVP_3MIC_MODE)                /*3MIC通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            1
#define TCFG_AUDIO_DUAL_MIC_ENABLE              0
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_NORMAL
#else
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE            0
#define TCFG_AUDIO_DUAL_MIC_ENABLE              0
#define TCFG_AUDIO_CVP_NS_MODE                  CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL                      DMS_NORMAL
#endif/*TCFG_AUDIO_CVP_DMS_DNS_MODE*/

#if TCFG_ESCO_DL_CVSD_SR_USE_16K
#define TCFG_AUDIO_CVP_BAND_WIDTH_CFG           (CVP_WB_EN) //只保留16k参数
#else
#define TCFG_AUDIO_CVP_BAND_WIDTH_CFG           (CVP_NB_EN | CVP_WB_EN) //同时保留8k和16k的参数
#endif

#if TCFG_CFG_TOOL_ENABLE
#undef EQ_SECTION_MAX
#define EQ_SECTION_MAX                          (32+2)  //2多阶高低通滤波器
#endif

#ifndef EQ_SECTION_MAX
#if TCFG_EQ_ENABLE
#define EQ_SECTION_MAX                          (32+2)  //2多阶高低通滤波器
#endif
#endif

#endif


//*********************************************************************************//
//                                  路径配置                                       //
//*********************************************************************************//
#define CONFIG_JLFAT_ENABLE

#if defined CONFIG_UI_ENABLE
#define TCFG_JLFAT_SUPPORT_OVERSECTOR_RW_ENABLE 1
#endif

#if TCFG_SD0_ENABLE
#define CONFIG_STORAGE_PATH                     "storage/sd0"   //定义对应SD0的路径
#define SDX_DEV                                 "sd0"
#endif

#if TCFG_SD1_ENABLE && !TCFG_SD0_ENABLE
#define CONFIG_STORAGE_PATH                     "storage/sd1"   //定义对应SD1的路径
#define SDX_DEV                                 "sd1"
#endif

#ifndef CONFIG_STORAGE_PATH
#define CONFIG_STORAGE_PATH                     "storage/sdx"   //不使用SD定义对应别的路径，防止编译出错
#define SDX_DEV                                 "sdx"
#endif

#define CONFIG_UDISK_STORAGE_PATH               "storage/udisk0"
#define CONFIG_ROOT_PATH                        CONFIG_STORAGE_PATH"/C/"
#define CONFIG_UDISK_ROOT_PATH                  CONFIG_UDISK_STORAGE_PATH"/C/"

//*********************************************************************************//
//                                  打开双区备份模式                                  //
//*********************************************************************************//
#define CONFIG_DOUBLE_BANK_ENABLE




//*********************************************************************************//
//                                   AI配置                                        //
//*********************************************************************************//
#define RCSP_MODE_EN                            (1 << 0)
#define TRANS_DATA_EN                           (1 << 1)
#define LL_SYNC_EN                              (1 << 2)
#define ANCS_CLIENT_EN                          (1 << 4)
#define GFPS_EN                                 (1 << 5)
#define REALME_EN                               (1 << 6)
#define TME_EN                                  (1 << 7)
#define DMA_EN                                  (1 << 8)
#define GMA_EN                                  (1 << 9)
#define MMA_EN                                  (1 << 10)
#define FMNA_EN                                 (1 << 11)
#define SWIFT_PAIR_EN                           (1 << 12)
#define LE_AUDIO_CIS_RX_EN                      (1 << 13)
#define LE_AUDIO_CIS_TX_EN                      (1 << 14)
#define LE_AUDIO_BIS_RX_EN                      (1 << 15)
#define LE_AUDIO_BIS_TX_EN                      (1 << 16)
#define ONLINE_DEBUG_EN                         (1 << 18)


//*********************************************************************************//
//                                UI DEMO配置                                      //
//*********************************************************************************//
#if TCFG_LCD_ENABLE
#define USE_LVGL_V9_UI_DEMO

/*#define USE_LVGL_V8_UI_DEMO*/
#define LV_DISP_UI_FB_NUM 2
#define FB_LCD_BUF_NUM    0

// #define USE_AWTK_UI_DEMO

// #define USE_ARM2D_UI_DEMO
#endif



//*********************************************************************************//
//                          异常记录/离线log配置                                      //
//*********************************************************************************//
#if !TCFG_DEBUG_UART_ENABLE
#define TCFG_DEBUG_DLOG_ENABLE                  0    // 离线log功能
#define TCFG_DEBUG_DLOG_FLASH_SEL               0    // 选择log保存到内置flash还是外置flash; 0:内置flash; 1:外置flash
#define TCFG_DLOG_FLASH_START_ADDR              0    // 配置外置flash用于存储dlog和异常数据的区域起始地址
#define TCFG_DLOG_FLASH_REGION_SIZE             (512 * 1024)    // 配置外置flash用于存储dlog和异常数据的区域大小
#if (TCFG_DEBUG_DLOG_ENABLE && TCFG_DEBUG_DLOG_FLASH_SEL)
#if (!defined(TCFG_NORFLASH_DEV_ENABLE) || (TCFG_NORFLASH_DEV_ENABLE == 0))
#undef TCFG_NORFLASH_DEV_ENABLE
#define TCFG_NORFLASH_DEV_ENABLE                1    // 使能外置flash驱动
#define TCFG_NORFLASH_START_ADDR                0    // 配置外置flash起始地址
#define TCFG_NORFLASH_SIZE                      (512 * 1024)   // 配置外置flash大小
#endif
#endif
#define TCFG_DEBUG_DLOG_RESET_ERASE             0    // 开机擦除flash的log数据
#define TCFG_DEBUG_DLOG_AUTO_FLUSH_TIMEOUT     30    // 主动刷新的超时时间(当指定时间没有刷新过缓存数据到flash, 则主动刷新)(单位秒)
#define TCFG_DEBUG_DLOG_UART_TX_PIN            TCFG_DEBUG_PORT   // dlog串口打印的引脚
#endif



#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)


#endif ///< APP_CONFIG_H

