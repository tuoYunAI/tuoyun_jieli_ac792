#ifdef CONFIG_BOARD_SMART_ROBOT_DEMO

//*********************************************************************************//
//                                功能模块配置                                     //
//*********************************************************************************//
#define CONFIG_SFC_ENABLE
// #define CONFIG_NO_SDRAM_ENABLE                             //关闭sdram
// #define CONFIG_EXFLASH_ENABLE                              //外挂资源flash
// #define CONFIG_SDFILE_EXT_ENABLE                           //外挂隐藏sdfile区的支持
// #define CONFIG_DMSDX_ENABLE                                //msd多分区显示支持

#define CONFIG_NET_ENABLE
#define CONFIG_BT_ENABLE
//#define CONFIG_AUDIO_ENABLE
#define CONFIG_MEDIA_ENABLE
//#define CONFIG_UI_ENABLE
//#define CONFIG_VIDEO_ENABLE

//*********************************************************************************//
//                                   时钟配置                                      //
//*********************************************************************************//
#define TCFG_OSC_FREQUENCY                  24000000
#define TCFG_SYS_CLK                        192000000         //240M,192M,160M,120M,96M,80M,64M,60M,48M,40M,32M,24M，其他值时启用SYS_PLL
#define TCFG_LSB_CLK                        48000000          //96M,80M,60M,48M,40M,24M,20M,12M
#define TCFG_HSB_CLK_DIV                    2                 //HSB_CLK = SYS_CLK / HSB_CLK_DIV
#define TCFG_SFCTZ_CLK                      48000000          //SFC时钟，和加载代码的速度有关
#define TCFG_SDRAM_CLK                      200000000         //SDRAM/DDR时钟
#define TCFG_VIDEO_CLK                      TCFG_SYS_CLK      //EVA/PUB时钟，240M,192M,160M,120M,96M,80M,64M,60M,48M，其他值时启用SYS_PLL或者DDR_PLL
#define TCFG_GPU_CLK                        TCFG_SYS_CLK      //GPU时钟，240M,192M,160M,120M,96M,80M,64M,60M,48M，TCFG_SYS_CLK(和cpu同频)

//*********************************************************************************//
//                                  flash配置                                      //
//*********************************************************************************//
/*
#data_width[1 2 3 4] 3的时候uboot自动识别2或者4线
#clkdiv [1-255] SPITZ_CLK=SYS_CLK/HSB_CLK_DIV/clkdiv
#mode:
#	  0 RD_OUTPUT,		 1 cmd 		 1 addr
#	  1 RD_I/O,   		 1 cmd 		 x addr
#	  2 RD_I/O_CONTINUE] no_send_cmd x add
#port:固定0
*/
#define TCFG_SPITZ_WIDTH_CLKDIV_MODE_PORT   4_3_0_0


//*********************************************************************************//
//                                  sdram配置                                      //
//*********************************************************************************//
#ifdef CONFIG_NO_SDRAM_ENABLE
#define TCFG_FREE_DCACHE_WAY                7
#else
#define TCFG_FREE_DCACHE_WAY                0
#endif
#define TCFG_SDRAM_MODE                     1              //0-sdram    1-ddr1



//*********************************************************************************//
//                              电源低功耗配置                                     //
//*********************************************************************************//
#define TCFG_POWER_MODE                     PWR_DCDC15
#define TCFG_POWER_INTERNAL_VDDIO_ENABLE    1
#define TCFG_POWER_AVDD18_ENABLE            1
#define TCFG_POWER_AVDD28_ENABLE            1

#if TCFG_POWER_INTERNAL_VDDIO_ENABLE
#define TCFG_LOWPOWER_VDDIOM_LEVEL          VDDIOM_VOL_330V//强VDDIO电压档位
#else
#define TCFG_LOWPOWER_VDDIOM_LEVEL          VDDIOM_VOL_320V//强VDDIO电压档位，不要高于外部DCDC的电压
#endif
#define TCFG_LOWPOWER_VDDIOW_LEVEL          VDDIOW_VOL_200V //弱VDDIO电压档位
#define TCFG_LOWPOWER_VDC14_LEVEL           DCVDD_VOL_140V
#define TCFG_LOWPOWER_FUNCTION              LOWPOWER_CLOSE
#define TCFG_LOWPOWER_OSC_TYPE              OSC_TYPE_LRC

#define TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE   0
#define TCFG_LOWPOWER_WAKEUP_PORT0_IO       IO_PORTA_01
#define TCFG_LOWPOWER_WAKEUP_PORT0_EDGE     FALLING_EDGE
#define TCFG_LOWPOWER_WAKEUP_PORT0_FILTER   PORT_FLT_DISABLE



//*********************************************************************************//
//                                AUDIO DAC配置                                    //
//*********************************************************************************//
#define TCFG_AUDIO_DAC_ENABLE               1
#define TCFG_AUDIO_DAC_PA_AUTO_MUTE_ENABLE  1
#define TCFG_AUDIO_DAC_PA_MUTE_PORT         IO_PORTE_15
#define TCFG_AUDIO_DAC_PA_MUTE_LEVEL        0
#define TCFG_AUDIO_DAC_DIFFER_OUTPUT_ENABLE 0
#define TCFG_AUDIO_DAC_HW_CHANNEL           (AUDIO_DAC_CH_L | AUDIO_DAC_CH_R)
#define TCFG_AUDIO_DAC_VCM_CAP_ENABLE       1
#if TCFG_AUDIO_DAC_VCM_CAP_ENABLE
#define TCFG_AUDIO_DAC_VCM_INIT_DELAY_MS    1000
#else
#define TCFG_AUDIO_DAC_VCM_INIT_DELAY_MS    0
#endif
#define TCFG_AUDIO_DAC_PA_MUTE_DELAY_MS     500


//*********************************************************************************//
//                                AUDIO ADC配置                                    //
//*********************************************************************************//
//--- AIN_AP0: PC7 AIN_AN0: PC6 AIN_BP0: PC11 AIN_BN0: PC12
//--- AIN_AP1: PC3 AIN_AN1: PC2 AIN_BP1: PC4  AIN_BN1: PC5
#define TCFG_AUDIO_ADC_ENABLE               1
#define TCFG_MIC_IO_PORT                    {-1/*MIC0P*/, -1/*MIC0N*/, IO_PORTC_11/*MIC1P*/, IO_PORTC_12/*MIC1N*/}
#define TCFG_MIC_CHANNEL_NUM                1
#define TCFG_LINEIN_IO_PORT                 {IO_PORTC_07/*AUX0P*/, IO_PORTC_06/*AUX0N*/, -1/*AUX1P*/, -1/*AUX1N*/}
#define TCFG_LINEIN_CHANNEL_NUM             1
#define TCFG_AUDIO_ADC_ALL_CHANNEL_OPEN     1
#define TCFG_DMIC_ENABLE                    0
#define TCFG_DMIC_SCLK_FREQUENCY            2000000
#define TCFG_DMIC_IO_SCLK                   IO_PORTC_11
#define TCFG_DMIC_IO_IDAT0                  IO_PORTA_11
#define TCFG_DMIC_IO_IDAT1                  IO_PORTA_11


//*********************************************************************************//
//                                   PLNK配置                                      //
//*********************************************************************************//
#define TCFG_PLNK_ENABLE                    1
#define TCFG_PLNK_HW_CHANNEL                PLNK_CH_MIC_DAT0
#define TCFG_PLNK_INPUT_DAT_SHARE_IO_ENABLE 0
#define TCFG_PLNK_SCLK_IO                   IO_PORTC_11
#define TCFG_PLNK_INPUT_DAT0_IO             IO_PORTA_11
#define TCFG_PLNK_INPUT_DAT1_IO             IO_PORTA_11
#define TCFG_PLNK_SCLK_FREQUENCY            2000000
#define TCFG_PLNK_CH0_SAMPLE_MODE           DATA0_SCLK_RISING_EDGE_SAMPLE
#define TCFG_PLNK_CH1_SAMPLE_MODE           DATA0_SCLK_FALLING_EDGE_SAMPLE
#define TCFG_PLNK_CIC_ORDER                 3
#define TCFG_PLNK_CIC_DFDLY_M               2


//*********************************************************************************//
//                                   ALNK配置                                      //
//*********************************************************************************//
#define TCFG_ALNK_ENABLE                    1
#define TCFG_ALNK_PORTS                     {\
                                                IO_PORTC_03/*MCLK*/, IO_PORTB_14/*LRCLK*/, IO_PORTB_13/*SCLK*/, \
                                                IO_PORTC_00/*DAT0*/, IO_PORTB_12/*DAT1*/, IO_PORTB_08/*DAT2*/, IO_PORTB_11/*DAT3*/, \
                                            }
#define TCFG_ALNK_CHANNLE_IN_MAP            ALNK_INPUT_CHANNEL1
#define TCFG_ALNK_CHANNLE_OUT_MAP           ALNK_OUTPUT_CHANNEL0
#define TCFG_ALNK_DATA_WIDTH                0
#define TCFG_ALNK_MODE                      ALNK_BASIC_IIS_MODE
#define TCFG_ALNK_DMA_MODE                  ALNK_DMA_PINGPANG_BUFFER
#define TCFG_ALNK_SLAVE_MODE_ENABLE         0
#define TCFG_ALNK_MCLK_SRC                  ALNK_MCLK_FROM_PLL_ALNK_CLOCK
#define TCFG_ALNK_UPDATE_EDGE               0


//*********************************************************************************//
//                                    FM配置                                       //
//*********************************************************************************//
#define TCFG_FM_DEV_ENABLE                  0
#define TCFG_FM_QN8035_ENABLE               1
#define TCFG_FM_BK1080_ENABLE               0
#define TCFG_FM_RDA5807_ENABLE              0


//*********************************************************************************//
//                            AUDIO_ADC应用的通道配置                              //
//*********************************************************************************//
#define CONFIG_AUDIO_ENC_SAMPLE_SOURCE      AUDIO_ENC_SAMPLE_SOURCE_MIC
#define CONFIG_ASR_CLOUD_ADC_CHANNEL        1              //云端识别mic通道
#define CONFIG_VOICE_NET_CFG_ADC_CHANNEL    1              //声波配网mic通道
#define CONFIG_AISP_MIC0_ADC_CHANNEL        1              //本地唤醒左mic通道
#define CONFIG_AISP_MIC1_ADC_CHANNEL        0              //本地唤醒右mic通道
#define CONFIG_REVERB_ADC_CHANNEL           1              //混响mic通道
#define CONFIG_PHONE_CALL_ADC_CHANNEL       1              //通话mic通道
#define CONFIG_UAC_MIC_ADC_CHANNEL          1              //UAC mic通道
#define CONFIG_AISP_LINEIN_ADC_CHANNEL      0              //本地唤醒LINEIN回采通道
#define CONFIG_FM_LINEIN_ADC_CHANNEL        0              //FM音频流LINEIN回采通道
#define CONFIG_AISP_MIC_ADC_GAIN            80             //本地唤醒mic增益
#define CONFIG_AISP_LINEIN_ADC_GAIN         10             //本地唤醒LINEIN增益
#define CONFIG_FM_LINEIN_ADC_GAIN           100            //FM音频流LINEIN增益
#define CONFIG_AUDIO_LINEIN_CHANNEL         1              //LIENIN通道数
#define CONFIG_AUDIO_LINEIN_ADC_GAIN        10             //LIENIN的模拟增益
#define CONFIG_AUDIO_LINEIN_SAMPLERATE      16000          //LINEIN默认采样率
#define CONFIG_AUDIO_LINEIN_CHANNEL_MAP     0


//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
//EQ配置，使用在线EQ时，EQ文件和EQ模式无效。有EQ文件时，默认不用EQ模式切换功能
#define TCFG_EQ_ENABLE                      1              //支持EQ功能
#define TCFG_EQ_ONLINE_ENABLE               0              //支持在线EQ调试
#define TCFG_LIMITER_ENABLE                 1              //限幅器
#define TCFG_EQ_FILE_ENABLE                 1              //从bin文件读取eq配置数据
#define TCFG_DRC_ENABLE                     TCFG_LIMITER_ENABLE
#define TCFG_EQ_MODE_CHOOSE                 0              //0:多通道共用1个eq, drc功能丰富; 1:多通道独立eq,独立drc, 请替换对应的bin文件
#if (TCFG_EQ_MODE_CHOOSE == 0)
#define TCFG_AUDIO_MDRC_ENABLE              2              //0:不使能低中DRC 1: 多带分频器使能 2: 多带分频后，再做一次全带处理使能
#define TCFG_LAST_WHOLE_DRC_ENABLE          1              //0:不使能最后的全带DRC; 1:使能
#elif (TCFG_EQ_MODE_CHOOSE == 1)
#define TCFG_EQ_DIVIDE_ENABLE               1              //0:前后通道共用EQ/DRC  1:支持EQ/DRC前后声道独立处理
#define TCFG_EQ_SPILT_ENABLE                1              //0:左右通道共用EQ  1:支持EQ左右声道独立处理
#define TCFG_DRC_SPILT_ENABLE               1              //0:左右通道共用DRC 1:支持DRC左右声道独立处理
#endif
#define TCFG_NULL_COMM                      0              //不支持通信
#define TCFG_USB_COMM                       1              //USB通信
#define TCFG_COMM_TYPE                      TCFG_USB_COMM  //EQ在线调试通信类型




//*********************************************************************************//
//                                 WIFI配置                                        //
//*********************************************************************************//
#ifdef CONFIG_NET_ENABLE
#define TCFG_WIFI_ENABLE                    1
#endif




//*********************************************************************************//
//                                   LCD配置                                       //
//*********************************************************************************//
#ifdef CONFIG_UI_ENABLE
#define TCFG_LCD_ENABLE                     1
#define TCFG_LCD_INPUT_FORMAT               LCD_IN_RGB565
#define TCFG_LCD_SUPPORT_MULTI_DRIVER_EN    0 ///< 多屏驱支持(目前仅支持具有相同分辨率的屏, 适用于一个case有多款屏混用的情况，比如因为缺货)

// 屏驱配置使能
#define TCFG_LCD_MIPI_ST7701S_480x800             1
#define TCFG_LCD_MIPI_ST7701S_480x800_ROTATE_90   0 ///< mipi ST7701S_480x800竖屏横显的配置
#include "lcd_board_cfg_template.h"                 ///< WL83开发板标配屏以外的屏驱配置放这里

#if TCFG_LCD_MIPI_ST7701S_480x800
#define TCFG_LCD_DEVICE_NAME                "MIPI_480x800_ST7701S"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_01
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif

#if TCFG_LCD_MIPI_ST7701S_480x800_ROTATE_90
#define TCFG_LCD_DEVICE_NAME                "MIPI_480x800_ST7701S_ROTATE90"
#define TCFG_LCD_BL_VALUE                   1
#define TCFG_LCD_RESET_IO                   IO_PORTB_00
#define TCFG_LCD_BL_IO                      IO_PORTB_01
#define TCFG_LCD_RS_IO                      -1
#define TCFG_LCD_CS_IO                      -1
#define TCFG_LCD_TE_ENABLE                  0
#define TCFG_LCD_TE_IO                      -1
#define TCFG_LCD_SPI_INTERFACE              NULL
#endif


//*********************************************************************************//
//                               触摸面板配置                                      //
//*********************************************************************************//
#define TCFG_TP_DRIVER_ENABLE               1
#define TCFG_TP_GT1151_ENABLE               0
#define TCFG_TP_FT6236_ENABLE               0
#define TCFG_TP_GT911_ENABLE                0
#define TCFG_TP_CST3240_ENABLE              1
#define TCFG_TP_GT9271_ENABLE               0

/*
 * TP旋转坐标常用的配置参数
 * ---------------------------------------------------
 *    配置   |  逆时针90° |  旋转180° |  逆时针270° |
 * ---------------------------------------------------
 *  SWAP_X_Y |     1      |     0     |      1      |
 * ---------------------------------------------------
 *  X_MIRROR |     0      |     1     |      1      |
 * ---------------------------------------------------
 *  Y_MIRROR |     1      |     1     |      0      |
 * ---------------------------------------------------
*/
#define TCFG_TP_SWAP_X_Y                    0///< 是否交换TP的X和Y坐标
#define TCFG_TP_X_MIRROR                    0///< 是否以TP的X中心轴镜像y坐标。
#define TCFG_TP_Y_MIRROR                    0///< 是否以TP的Y中心轴镜像x坐标。
#define TCFG_TP_RST_PIN                     IO_PORTE_06
#define TCFG_TP_INT_PIN                     IO_PORTA_03
#define TCFG_TP_COMMUNICATE_IF              "iic0"

#endif



//*********************************************************************************//
//                                  AD按键配置                                     //
//*********************************************************************************//
#define TCFG_ADKEY_ENABLE                   1              //AD按键
#define TCFG_PRESS_LONG_KEY_POWERON_ENABLE  TCFG_ADKEY_ENABLE       //长按开关机功能
#define TCFG_ADKEY_INPUT_IO                 IO_PORTD_00
#define TCFG_ADKEY_INPUT_CHANNEL            ADC_IO_CH_PD00

#define ADKEY_UPLOAD_R                      22             //上拉电阻值
#define TCFG_ADC_LEVEL_09                   0x3FF
#define TCFG_ADC_LEVEL_08                   0x3FF
#define TCFG_ADC_LEVEL_07                   0x3FF
#define TCFG_ADC_LEVEL_06                   0x3FF
#define TCFG_ADC_LEVEL_05                   0x3FF
#define TCFG_ADC_LEVEL_04                   (ADC_VDDIO * 100 / (100 + ADKEY_UPLOAD_R))
#define TCFG_ADC_LEVEL_03                   (ADC_VDDIO * 33  / (33  + ADKEY_UPLOAD_R))
#define TCFG_ADC_LEVEL_02                   (ADC_VDDIO * 15  / (15  + ADKEY_UPLOAD_R))
#define TCFG_ADC_LEVEL_01                   (ADC_VDDIO * 51  / (51  + ADKEY_UPLOAD_R * 10))
#define TCFG_ADC_LEVEL_00                   (0)

#define TCFG_ADKEY_VALUE_0                  KEY_POWER
#define TCFG_ADKEY_VALUE_1                  KEY_MENU
#define TCFG_ADKEY_VALUE_2                  KEY_UP
#define TCFG_ADKEY_VALUE_3                  KEY_DOWN
#define TCFG_ADKEY_VALUE_4                  KEY_OK
#define TCFG_ADKEY_VALUE_5                  KEY_CANCLE
#define TCFG_ADKEY_VALUE_6                  KEY_ENC
#define TCFG_ADKEY_VALUE_7                  KEY_PHOTO
#define TCFG_ADKEY_VALUE_8                  KEY_F1
#define TCFG_ADKEY_VALUE_9                  NO_KEY




//*********************************************************************************//
//                                  产品信息配置                                     //
//*********************************************************************************//
#define OTA_PROTOCOL_VERSION  2                               //ota升级协议版本
#define APP_LANGUAGE          "zh-CN"                         //中文语言包
#define REG_OTA_URL           "https://ota.lovaiot.com/ota/v2/"  //ota升级服务器地址
#define PRODUCT_VENDOR_UID    "5137786831399878656"           //设备厂商的UID, 由平台提供

#define PRODUCT_TYPE          "JIELI7926E_PRODUCT_DEMO"       //产品的类型, 需要在平台上注册*
#define PRODUCT_NAME          "TUOYUN_ROBOT"                  //产品名称

#define BOARD_TYPE            "JIELI7926E_DEMO"               //控制板的类型, 由平台提供*
#define BOARD_NAME            "jieli-ai-dev"                  //控制板的名称, 由平台提供


#define FIRMWARE_TYPE         "TY_JIELI7926E_DEMO"            //固件的类型, 由平台提供*
#define FIRMWARE_NAME         "TUOYUN_ROBOT_RUN"              //固件名称
#define FIRMWARE_VERSION      "1.0.2"                         //固件版本号

#endif
