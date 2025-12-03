#ifndef    __LE_COMMON_H_
#define    __LE_COMMON_H_

#include "typedef.h"
#include <stdint.h>
#include "btstack/bluetooth.h"
#include "app_config.h"
#include "btstack/le/le_common_define.h"

//--------------------------------------------
#define LE_DEBUG_PRINT_EN               0     // log switch
//--------------------------------------------

/*毫秒 转换到 0.625ms 单位*/
#define ADV_SCAN_MS(_ms)                ((_ms) * 8 / 5)

extern void bt_ble_init(void);
extern void bt_ble_exit(void);
extern void le_hogp_set_icon(u16 class_type);
extern void le_hogp_set_ReportMap(const u8 *map, u16 size);

#endif


