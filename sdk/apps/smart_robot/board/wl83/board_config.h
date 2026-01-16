#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/**
 * ***注意*** 
 * 请选择所使用的板子配置, 否设备可能无法启动!!!
 */
#error "请选择所使用的板子配置 Please select the board config"
//#define CONFIG_BOARD_SMART_ROBOT_DEVELOP
#define CONFIG_BOARD_SMART_ROBOT_DEMO

//芯片型号sdram和flash配置文件
#include "chip_cfg.h"



//不同板子外设配置文件，如有新的板子，在这里同理添加
#ifdef CONFIG_BOARD_SMART_ROBOT_DEVELOP
#include "board_smart_robot_develop.h"
#endif


#ifdef CONFIG_BOARD_SMART_ROBOT_DEMO
#include "board_smart_robot_demo.h"
#endif

#ifdef CONFIG_NO_SDRAM_ENABLE
#undef  __SDRAM_SIZE__
#define __SDRAM_SIZE__    0
#else
#if (__SDRAM_SIZE__ == 0)
#ifndef CONFIG_NO_SDRAM_ENABLE
#define CONFIG_NO_SDRAM_ENABLE
#endif
#endif
#endif

#endif
