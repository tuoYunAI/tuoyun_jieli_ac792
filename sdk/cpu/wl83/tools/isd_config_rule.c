#include "app_config.h"

// *INDENT-OFF*

[配置项预留起始项一_注意该位置不要定义子配置项]

[EXTRA_CFG_PARAM]
//###flash空间配置====================================================================
#if defined CONFIG_DOUBLE_BANK_ENABLE
BR22_TWS_DB=YES;
BR22_TWS_VERSION=0;
DB_UPDATE_DATA=YES;
#if defined (CONFIG_SUPPORT_DOUBLE_UBOOT)
UBOOT_AREA_SIZE=64K,64K;
#endif
#else
NEW_FLASH_FS=YES;                       [单备份模式]//单备份
#endif
#if __FLASH_SIZE__ == (1 * 1024 * 1024)
FLASH_SIZE=1M;                          [FLASH 总大小]
#elif __FLASH_SIZE__ == (2 * 1024 * 1024)
FLASH_SIZE=2M;                          [FLASH 总大小]
#elif __FLASH_SIZE__ == (4 * 1024 * 1024)
FLASH_SIZE=4M;                          [FLASH 总大小]
#elif __FLASH_SIZE__ == (8 * 1024 * 1024)
FLASH_SIZE=8M;                          [FLASH 总大小]
#elif __FLASH_SIZE__ == (16 * 1024 * 1024)
FLASH_SIZE=16M;                         [FLASH 总大小]
#elif __FLASH_SIZE__ == (32 * 1024 * 1024)
FLASH_SIZE=32M;                         [FLASH 总大小]
#elif __FLASH_SIZE__ == (64 * 1024 * 1024)
FLASH_SIZE=64M;                         [FLASH 总大小]
#endif

INTERNAL_DIR_ALIGN = 2; //资源文件对齐处理, 2^n
AMONG_FILE_ALIGN = 2;//目录内的资源文件对齐处理,2^n

//FORCE_VM_ALIGN=YES;
#if (defined CONFIG_SFC_ENABLE) || (defined CONFIG_NO_SDRAM_ENABLE)
ENTRY=0x8000120;                        [程序入口地址 SFC模式地址]
#else
ENTRY=0x18000120;                       [程序入口地址 SDRAM模式地址]
#endif
CHIP_NAME=AC792N;                       [芯片型号选择]
PID=AC792N_STORY;                       [长度16byte,示例：芯片封装_应用方向_方案名称]
VID=0.01;
RESERVED_OPT=0;

//###烧写配置====================================================================
FORCE_4K_ALIGN=YES;                     [用于烧写器烧写预留区文件, 需要定义该配置项]
SPECIAL_OPT=0;                          [用于烧写器烧写预留区文件, 需要定义该配置项]
NEED_RESERVED_4K=YES;                   [关闭4K保留写NO]
DOWNLOAD_MODEL=usb;                     [烧录模式usb|SERIAL]
SERIAL_DEVICE_NAME=JlVirtualJtagSerial;
SERIAL_BARD_RATE=1000000;
SERIAL_CMD_OPT=10;
SERIAL_CMD_RATE=100;                    [n*10000]
SERIAL_CMD_RES=0;
SERIAL_INIT_BAUD_RATE=9600;
LOADER_BAUD_RATE=1000000;
LOADER_ASK_BAUD_RATE=1000000;
SERIAL_SEND_KEY=YES;
BEFORE_LOADER_WAIT_TIME=150;
#ifdef CONFIG_RELEASE_ENABLE
CALC_RES_CFG=NO;                        [自动填充扩展预留区文件起始地址和大小，量产版本严禁打开]
#else
CALC_RES_CFG=YES;                       [自动填充扩展预留区文件起始地址和大小，量产版本严禁打开]
#endif
//#支持EFUSE 烧写到flash最后4K
//[FW_ADDITIONAL]
//FILE_LIST=(file=fw.cfg:type=0xEE)

/*#####################################################    UBOOT配置项，请勿随意调整顺序    ##################################################*/
[SYS_CFG_PARAM]
//###flash配置====================================================================
/*
#data_width[1 2 4]
#clkdiv [1-255] SPITZ_CLK=SYS_CLK/HSB_DIV/clkdiv
#mode:
#	  0 RD_OUTPUT,		 1 cmd 		 1 addr
#	  1 RD_I/O,   		 1 cmd 		 x addr
#	  2 RD_I/O_CONTINUE] no_send_cmd x add
*/
#if defined TCFG_SPITZ_WIDTH_CLKDIV_MODE_PORT
SPI=TCFG_SPITZ_WIDTH_CLKDIV_MODE_PORT;  [width_clkdiv_mode_port]
#else
SPI=2_2_0_0;                            [width_clkdiv_mode_port]
#endif
#if defined CONFIG_EXFLASH_ENABLE
EX_FLASH=PA04_1B_PA07;
#if TCFG_NORFLASH_SFC_DATA_WIDTH == 4
EX_FLASH_IO=4_PA09_PA10_PA05_PA06_PA08;
#else
EX_FLASH_IO=2_PA09_PA10_PA05_PA06_PA08;
#endif
#endif
SPI_IO_HD=1_1_1_0_0_1;[主控SPI口IO驱动档位,0-3档可配,clk_d0_d1_d2_d3_cs]
FLASH_QE_POS=0;[QE的位置:0-SR2的BIT1, 1-SR1的BIT6]

//###时钟配置====================================================================
#if defined TCFG_OSC_FREQUENCY
OSC_FREQ=TCFG_OSC_FREQUENCY;            [24M]
#else
OSC_FREQ=24MHz;                         [24M]
#endif
#if defined TCFG_SYS_CLK
SYS_CLK=TCFG_SYS_CLK;                   [240M,192M,160M,120M,96M,80M,64M,60M,48M,40M,32M,24M，当SYS_CLK为其他值时启用SYS_PLL]
#else
SYS_CLK=48MHz;                          [240M,192M,160M,120M,96M,80M,64M,60M,48M,40M,32M,24M，当SYS_CLK为其他值时启用SYS_PLL]
#endif
#if defined TCFG_LSB_CLK
LSB_CLK=TCFG_LSB_CLK;                   [96M,80M,60M,48M,40M,24M,20M,12M]
#else
LSB_CLK=48MHz;                          [96M,80M,60M,48M,40M,24M,20M,12M]
#endif
#if defined TCFG_HSB_CLK_DIV
HSB_DIV=TCFG_HSB_CLK_DIV;               [高速总线时钟分频系数=SYS_CLK/HSB_CLK]
#else
HSB_DIV=1;                              [高速总线时钟分频系数=SYS_CLK/HSB_CLK]
#endif
#if defined TCFG_SFCTZ_CLK
SFC_CLK=TCFG_SFCTZ_CLK;                 [160M,120M,96M,48M,24M,其他值时通过HSB_CLK分频出来]
#else
SFC_CLK=24MHz;                          [160M,120M,96M,48M,24M,其他值时通过HSB_CLK分频出来]
#endif
#if defined TCFG_UBOOT_DEBUG_PORT
UTTX=TCFG_UBOOT_DEBUG_PORT;             [配置UBOOT调试输出Pin]
UTBD=1000000;                           [配置UBOOT调试波特率]
#endif
//#############################################################################################################################################

//###SDRAM/DDR配置===================================================================
#if defined TCFG_FREE_DCACHE_WAY
FREE_DCACHE_WAY=TCFG_FREE_DCACHE_WAY;
#endif
#if defined CONFIG_LZ4_COMPRESS_APP_CODE_ENABLE
LZ4_ENABLE=1;
#else
LZ4_ENABLE=0;
#endif
#if defined CONFIG_NO_SDRAM_ENABLE
ENABLE_SDRAM=0;                         [使能SDRAM]
#else
ENABLE_SDRAM=1;                         [使能SDRAM]
#endif
#if defined TCFG_SDRAM_MODE
SDRAM_MODE=TCFG_SDRAM_MODE;             [0:sdram | 1:ddr1]
#else
SDRAM_MODE=0;                           [0:sdram | 1:ddr1]
#endif
#if defined TCFG_SDRAM_CLK
SDRAM_CLK=TCFG_SDRAM_CLK;               [SDRAM/DDR时钟]
#else
SDRAM_CLK=40000000;                     [SDRAM/DDR时钟]
#endif
#if __SDRAM_SIZE__ == (64 * 1024 * 1024)
SDRAM_SIZE=64M;                         [SDRAM存储容量]
#elif __SDRAM_SIZE__ == (32 * 1024 * 1024)
SDRAM_SIZE=32M;                         [SDRAM存储容量]
#elif __SDRAM_SIZE__ == (16 * 1024 * 1024)
SDRAM_SIZE=16M;                         [SDRAM存储容量]
#elif __SDRAM_SIZE__ == (8 * 1024 * 1024)
SDRAM_SIZE=8M;                          [SDRAM存储容量]
#elif __SDRAM_SIZE__ == (4 * 1024 * 1024)
SDRAM_SIZE=4M;                          [SDRAM存储容量]
#elif __SDRAM_SIZE__ == (2 * 1024 * 1024)
SDRAM_SIZE=2M;                          [SDRAM存储容量]
#else
SDRAM_SIZE=0M;                          [SDRAM存储容量]
#endif
SDRAM_REFRESH_TIME=32;                  [REFRESH_TIME_UNIT_IS_MS, 64(70度)|32(85度)|16(105度)]
#if __SDRAM_SIZE__ < (32 * 1024 * 1024)
SDRAM_REFRESH_CYCLES=4K;                [4K|8K]
#else
SDRAM_REFRESH_CYCLES=8K;                [4K|8K]
#endif
SDRAM_IO_HD=2;                          [SDRAM_IO_HIGH_DRIVE_LEVEL]
SDRAM_TRIM_ENABLE=1;                    [SDRAM_TRIM_ENABLE]
#if __SDRAM_SIZE__ >= (16 * 1024 * 1024)
SDRAM_COLUM=1;                          [THE_COLUMN_ADDRESS_ENABLE]
#else
SDRAM_COLUM=0;                          [THE_COLUMN_ADDRESS_ENABLE]
#endif
SDRAM_CL=3;
#if TCFG_SDRAM_MODE == 1
#if TCFG_SDRAM_CLK == 250000000
SDRAM_TRFC=68;                          [REFRESH_CYCLE_TIME_UNIT_IS_NS]
#else
SDRAM_TRFC=70;                          [REFRESH_CYCLE_TIME_UNIT_IS_NS]
#endif
SDRAM_TRP=16;                           [ROW_PRECHARGE_TIME_UNIT_IS_NS]
SDRAM_TRCD=16;                          [ROW_TO_COLUMN_DELAY_UNIT_IS_NS]
SDRAM_TRRD=8;                           [ACT_TO_ACT_DELAY_TIME_UNIT_IS_NS]
SDRAM_TWTR=2;
SDRAM_TRTW=4;
SDRAM_TWR=15;                           [WRITE_RECOVER_TIME_UNIT_IS_NS]
SDRAM_TRC=52;                           [AUTO_REFRESH_CMD_CYCLE_TIME_UNIT_IS_NS]
SDRAM_WLCNT=0;
#if TCFG_SDRAM_CLK >= 250000000
SDRAM_RLCNT=4;
SDRAM_PHASE=3;
SDRAM_DQS_DLY_TRM=5;
SDRAM_UDQS_DLY_TRM=0;
SDRAM_LDQS_DLY_TRM=0;
#elif TCFG_SDRAM_CLK >= 120000000
SDRAM_RLCNT=4;
SDRAM_PHASE=2;
SDRAM_DQS_DLY_TRM=5;
SDRAM_UDQS_DLY_TRM=0;
SDRAM_LDQS_DLY_TRM=0;
#else
SDRAM_RLCNT=3;
SDRAM_PHASE=3;
SDRAM_DQS_DLY_TRM=0;
SDRAM_UDQS_DLY_TRM=11;
SDRAM_LDQS_DLY_TRM=11;
#endif
SDRAM_D_DLY=1;
SDRAM_Q_DLY=1;
SDRAM_DQ_DLY_TRM=0;
SDRAM_DQ_DLY_INNER_TRM=0;
#else
SDRAM_TRFC=70;                          [REFRESH_CYCLE_TIME_UNIT_IS_NS]
SDRAM_TRP=21;                           [ROW_PRECHARGE_TIME_UNIT_IS_NS]
SDRAM_TRCD=21;                          [ROW_TO_COLUMN_DELAY_UNIT_IS_NS]
SDRAM_TRRD=14;                          [ACT_TO_ACT_DELAY_TIME_UNIT_IS_NS]
SDRAM_TWTR=2;
SDRAM_TRTW=6;
SDRAM_TWR=15;                           [WRITE_RECOVER_TIME_UNIT_IS_NS]
SDRAM_TRC=63;                           [AUTO_REFRESH_CMD_CYCLE_TIME_UNIT_IS_NS]
SDRAM_WLCNT=0;
SDRAM_RLCNT=4;
SDRAM_PHASE=3;
#endif

/*
[(sdx)_(port)_(width)_(detect mode)_(clk)]
sdx: (SD0:0 SD1:1)
port: (A:0 B:1 C:2 D:3)
width: (1:1线  4:4线)
detect mode: (0:时钟检测 1:命令检测)
clk: 时钟，单位为MHz
*/
#if defined CONFIG_SDFILE_EXT_ENABLE
#if TCFG_SD0_ENABLE
#if TCFG_SD_PORTS == 'A'
EX_SDCARD=SD_0_0_1_1_24;
#elif TCFG_SD_PORTS == 'B'
EX_SDCARD=SD_0_1_1_1_24;
#elif TCFG_SD_PORTS == 'C'
EX_SDCARD=SD_0_2_1_1_24;
#else
EX_SDCARD=SD_0_3_1_1_24;
#endif
#else
#if TCFG_SD_PORTS == 'A'
EX_SDCARD=SD_1_0_1_1_24;
#else
EX_SDCARD=SD_1_0_2_1_24;
#endif
#endif
#endif

/*
###########################################
#
# 支持当前SDK最低烧写器测试盒工具版本
#
*/
[TOOL_CONFIG]
1TO2_MIN_VER=2.24.5;
1TO8_MIN_VER=3.0.35;
BTBOX_MIN_VER=1.2.4.f;
//###########################################

#if defined(CONFIG_DOUBLE_BANK_ENABLE) && defined(CONFIG_UPDATE_COMPRESS)
[FW_ADDITIONAL]
FILE_LIST=(file=ota.bin:type=100);
#endif

/*
########flash空间使用配置区域###############################################
#PDCTNAME:    产品名，对应此代码，用于标识产品，升级时可以选择匹配产品名
#BOOT_FIRST:  1=代码更新后，提示APP是第一次启动；0=代码更新后，不提示
#UPVR_CTL：   0：不允许高版本升级低版本   1：允许高版本升级低版本
#XXXX_ADR:    区域起始地址	AUTO：由工具自动分配起始地址
#XXXX_LEN:    区域长度		CODE_LEN：代码长度
#XXXX_OPT:    区域操作属性
#
#
#
#操作符说明  OPT:
#	0:  下载代码时擦除指定区域
#	1:  下载代码时不操作指定区域
#	2:  下载代码时给指定区域加上保护
############################################################################
*/
[RESERVED_CONFIG]
//#升级之后需要保留VM数据，在生成升级文件时需要设置VM_OPT=1
VM_ADR=0; [设置VM]
VM_LEN=32K;
#if defined CONFIG_DOUBLE_BANK_ENABLE || TCFG_RF_FCC_TEST_ENABLE
VM_OPT=0;
#else
VM_OPT=1;//单备份升级VM在升级时候默认VM不需要擦除,选择擦除会在ota_loader第二阶段擦除比较长，而且可能会造成VM丢失
#endif

BTIF_ADR=AUTO; [设置资源]
BTIF_LEN=0x1000;
BTIF_OPT=1;

PRCT_ADR=0;
PRCT_LEN=CODE_LEN;
PRCT_OPT=2;

//#烧录后不可升级的资源
[RESERVED_EXPAND_CONFIG]
USER_ADR=AUTO; [固定预留给客户，避免客户量产后，想通过升级新增重要信息的保存却没有预先预留空间]
USER_LEN=0x1000;
USER_OPT=1;

#if defined CONFIG_SAVE_EXCEPTION_LOG_IN_FLASH
LOG_ADR=AUTO; [异常打印信息]
LOG_LEN=0x1000;
LOG_OPT=0;
#endif

// 离线log功能
#if (defined TCFG_DEBUG_DLOG_ENABLE  && TCFG_DEBUG_DLOG_FLASH_SEL == 0)
DLOG_ADR=AUTO; [离线log 内置flash区域配置]
DLOG_LEN=TCFG_DLOG_FLASH_REGION_SIZE
DLOG_OPT=0;
#endif

#if defined CONFIG_DUER_SDK_ENABLE
DUEROS_ADR=AUTO; [小度profile]
DUEROS_LEN=0x1000;
DUEROS_OPT=1;
#endif

#if defined CONFIG_TVS_SDK_ENABLE
TENCENT_ADR=AUTO; [腾讯云profile]
TENCENT_LEN=0x3000;
TENCENT_OPT=1;
#endif

#if defined CONFIG_TELECOM_SDK_ENABLE
TELECOM_ADR=AUTO; [电信云profile]
TELECOM_LEN=0x1000;
TELECOM_OPT=1;
#endif

#if defined CONFIG_TUYA_SDK_ENABLE
TUYA_ADR=AUTO; [涂鸦profile]
TUYA_LEN=0x19000;
TUYA_OPT=1;
#endif

//#第三方云平台profile
#if defined CONFIG_SERVER_ASSIGN_PROFILE
PROFILE_ADR=AUTO; [第三方云平台profile]
PROFILE_LEN=0x1000;
PROFILE_OPT=1;
#endif

//#思必驰打断唤醒profile
#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == AISP_ALGORITHM)
AISP_ADR=AUTO; [思必驰打断唤醒profile]
AISP_LEN=0x1000;
AISP_OPT=1;
#endif

#if TCFG_LRC_ENABLE_SAVE_LABEL_TO_FLASH
LRIF_ADR=AUTO;
LRIF_LEN=0x10000;
LRIF_OPT=1;
#endif

#if TCFG_EXTFLASH_ENABLE
//截取flash的一段空间作为extflash设备
EXTFLASH_ADR=AUTO;
EXTFLASH_LEN=0x400000;
EXTFLASH_OPT=1;
#endif

/***************资源文件打包 请阅读以下信息******************************/
//packers文件夹下会生成AUPACKRES,UIPACKRES这两个包是资源文件打包好的包需要
//根据实际大小填写AUPACKRES_ADR,AUPACKRES_LEN,
//工程中默认根据flash大小,在app_config.h中配置了资源的地址和长度，
//资源起始地址和大小, 根据产品生命周期最大情况定义,
//audio和ui资源需要放到最后, 新增的预留区配置请在前面添加
#if defined CONFIG_LOGO_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
LOGOPACKRES_FILE=packres/LOGOPACKRES;
LOGOPACKRES_ADR=CONFIG_LOGO_PACKRES_ADR;
LOGOPACKRES_LEN=CONFIG_LOGO_PACKRES_LEN;
LOGOPACKRES_OPT=1;
#endif

#if defined CONFIG_AUDIO_ENABLE && defined CONFIG_VOICE_PROMPT_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
AUPACKRES_FILE=packres/AUPACKRES;
AUPACKRES_ADR=CONFIG_AUDIO_PACKRES_ADR;
AUPACKRES_LEN=CONFIG_AUDIO_PACKRES_LEN;
AUPACKRES_OPT=1;
#endif

#if defined CONFIG_UI_ENABLE && defined CONFIG_UI_FILE_SAVE_IN_RESERVED_EXPAND_ZONE
UIPACKRES_FILE=packres/UIPACKRES;
UIPACKRES_ADR=CONFIG_UI_PACKRES_ADR;
UIPACKRES_LEN=CONFIG_UI_PACKRES_LEN;
UIPACKRES_OPT=1;
#endif

[BURNER_CONFIG]
SIZE=32;

#if defined(CONFIG_DOUBLE_BANK_ENABLE) && defined(CONFIG_UPDATE_COMPRESS)
[COMPRESS_BACKUP]
AREA_SIZE=0;
BLOCK_SIZE=65536;
#endif

