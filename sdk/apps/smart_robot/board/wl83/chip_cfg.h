#ifndef CONFIG_CHIP_CFG_H
#define CONFIG_CHIP_CFG_H

#ifdef CONFIG_BOARD_SMART_ROBOT_DEMO
#define __FLASH_SIZE__    (8 * 1024 * 1024)
#define __SDRAM_SIZE__    (8 * 1024 * 1024)
#endif

#ifdef CONFIG_BOARD_SMART_ROBOT_DEVELOP
#define __FLASH_SIZE__    (8 * 1024 * 1024)
#define __SDRAM_SIZE__    (16 * 1024 * 1024)
#endif



#endif
