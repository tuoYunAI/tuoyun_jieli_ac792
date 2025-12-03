#ifndef _BT_PROFILE_CFG_H_
#define _BT_PROFILE_CFG_H_

#include "app_config.h"

//ble demo的例子
#define DEF_BLE_DEMO_NULL                 0xff //ble 没有使能
#define DEF_BLE_DEMO_MULTI                1
#define DEF_BLE_DEMO_NONCONN_24G          2
#define DEF_BLE_DEMO_RCSP_DEMO            4
#define DEF_BLE_DEMO_ADV                  5
#define DEF_BLE_DEMO_TRANS_DATA           6
#define DEF_BLE_DEMO_NET_CFG              7
#define DEF_BLE_DEMO_MASTER_CENTRAL       8
#define DEF_BLE_DEMO_HOGP                 9
#define DEF_BLE_DEMO_MI                   10
#define DEF_BLE_DEMO_NET_CFG_DUI          11
#define DEF_BLE_DEMO_NET_CFG_TURING       12
#define DEF_BLE_DEMO_NET_CFG_TENCENT      13
#define DEF_BLE_DEMO_NET_CFG_DUEROS       14
#define DEF_BLE_DEMO_NET_CFG_XC           15
#define DEF_BLE_DEMO_NET_CFG_TY           16

//配置选择的demo
#if TCFG_USER_BLE_ENABLE

#if RCSP_MODE
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_RCSP_DEMO

#elif TCFG_TRANS_MULTI_BLE_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_MULTI

#elif TCFG_TRANS_DATA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_TRANS_DATA

#elif TCFG_BT_NET_CFG_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG

#elif TCFG_BT_NET_CFG_DUI_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_DUI

#elif TCFG_BT_NET_CFG_TURING_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_TURING

#elif TCFG_BT_NET_CFG_TENCENT_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_TENCENT

#elif TCFG_BT_NET_CFG_DUEROS_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_DUEROS

#elif TCFG_BLE_MASTER_CENTRAL_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_MASTER_CENTRAL

#elif TCFG_BT_NET_CFG_XC_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_XC

#elif TCFG_BT_NET_CFG_TY_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_TY

#elif TCFG_BLE_HID_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_HOGP

#elif TCFG_NONCONN_24G_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NONCONN_24G

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL//ble is closed

#endif

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL//ble is closed
#endif

#endif
