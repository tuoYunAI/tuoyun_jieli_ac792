#ifndef __CUSTOM_MI_CONFIG_H__
#define __CUSTOM_MI_CONFIG_H__

#include "system/includes.h"

#define MIBLE_DEBUG_LOG
#define MI_LOG_ENABLED   1
#define DEVELOPER_VERSION 0001
#define PRODUCT_ID  156
#define MODEL_NAME  "xiaomi.dev.ble"
//#define PRODUCT_ID  4611//156
//#define MODEL_NAME  "xiaomiaoce.sensor_ht.t6"//"xiaomi.dev.ble"
#define MI_BLE_ENABLED
#define HAVE_MSC    0
#define HAVE_RESET_BUTTON   1
#define HAVE_CONFIRM_BUTTON 0
#define MI_SCHD_PROCESS_IN_MAIN_LOOP 1
#define MAX_ATT_MTU    247
#define DFU_NVM_START 0x4A000UL /**< DFU Buffer Start Address */
#define DFU_NVM_SIZE 10 * 1024 * 1024 /**< DFU Buffer Size in bytes */

#endif

