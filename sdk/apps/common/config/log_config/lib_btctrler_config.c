#include "app_config.h"
#include "btcontroller_config.h"

#ifdef CONFIG_BT_ENABLE

/**
 * @brief Bluetooth Module
 */
#if TCFG_USER_BT_CLASSIC_ENABLE && TCFG_USER_BLE_ENABLE
const int config_btctler_modules                    = (BT_MODULE_CLASSIC | BT_MODULE_LE);
#elif TCFG_USER_BT_CLASSIC_ENABLE
const int config_btctler_modules                    = (BT_MODULE_CLASSIC);
#elif TCFG_USER_BLE_ENABLE
const int config_btctler_modules                    = (BT_MODULE_LE);
#else
const int config_btctler_modules                    = 0;
#endif

#if TCFG_RF_FCC_TEST_ENABLE || TCFG_RF_PRODUCT_TEST_ENABLE
int CONFIG_DISTURB_SCAN_ENABLE                      = 0;
int config_btctler_mode                             = TCFG_BT_MODE;
#if TCFG_RF_FCC_TEST_ENABLE
int config_btctler_hci_standard                     = 1;
#else
int config_btctler_hci_standard                     = 0;
#endif
#else
const int CONFIG_DISTURB_SCAN_ENABLE                = 0;
const int config_btctler_mode                       = TCFG_BT_MODE;
#if (TCFG_BT_MODE != BT_NORMAL)
const int config_btctler_hci_standard               = 1;
#else
const int config_btctler_hci_standard               = 0;
#endif
#endif

const int CONFIG_BT_FUNCTION                        = 0;
const int CONFIG_DONGLE_SPEAK_ENABLE                = 0;
const int CONFIG_BTCTLER_JL_DONGLE_SOURCE_ENABLE    = 0;
const int CONFIG_WIFI_DETECT_ENABLE                 = 0;
const int CONFIG_ESCO_FORWARD_ENABLE                = 0;
const int CONFIG_UPDATE_BT_LMP_EN                   = 0;
const int CONFIG_AES_CCM_FOR_EDR_ENABLE             = 0;
const int CONFIG_MPR_CLOSE_WHEN_ESCO                = 0;
#if TCFG_BT_DUAL_CONN_ENABLE
const int CONFIG_LMP_SUPPORT_MULTI_CONN             = 1;
const int CONFIG_LMP_CONNECTION_NUM                 = 2;
const int CONFIG_LMP_CONNECTION_LIMIT_NUM           = 2;
#else
const int CONFIG_LMP_SUPPORT_MULTI_CONN             = 0;
const int CONFIG_LMP_CONNECTION_NUM                 = 1;
const int CONFIG_LMP_CONNECTION_LIMIT_NUM           = 1;
#endif
const int CONFIG_LMP_NAME_REQ_ENABLE                = 1;
const int CONFIG_LMP_PASSKEY_ENABLE                 = 0;
const int CONFIG_LMP_OOB_ENABLE                     = 0;
#if TCFG_BT_BQB_PROFILE_TEST_ENABLE
const int CONFIG_LMP_MASTER_ESCO_ENABLE             = 1;
#else
const int CONFIG_LMP_MASTER_ESCO_ENABLE             = 0;
#endif
const int CONFIG_BTCTLER_QOS_ENABLE                 = 0;
const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE        = 0;
#ifdef CONFIG_NO_SDRAM_ENABLE
const int CONFIG_BTCTLER_RAM_MALLOC_ENABLE          = 0;
#else
const int CONFIG_BTCTLER_RAM_MALLOC_ENABLE          = 1;
#endif
#if TCFG_USER_TWS_ENABLE
const int CONFIG_BTCTLER_TWS_ENABLE                 = 1;
const int CONFIG_BTCTLER_TWS_FUN                    = TWS_ESCO_FORWARD;
#else
const int CONFIG_BTCTLER_TWS_ENABLE                 = 0;
const int CONFIG_BTCTLER_TWS_FUN                    = 0;
#endif
const int CONFIG_EX_TWS_ADJUST                      = 1;
#if TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
const int CONFIG_TWS_AUTO_ROLE_SWITCH_ENABLE        = 1;
#else
const int CONFIG_TWS_AUTO_ROLE_SWITCH_ENABLE        = 0;
#endif
#if TCFG_TWS_POWER_BALANCE_ENABLE
const int CONFIG_TWS_POWER_BALANCE_ENABLE           = 1;
#else
const int CONFIG_TWS_POWER_BALANCE_ENABLE           = 0;
#endif
#if TCFG_LOCAL_TWS_ENABLE
const int CONFIG_TWS_DATA_TRANS_ENABLE              = 1;
#else
const int CONFIG_TWS_DATA_TRANS_ENABLE              = 0;
#endif
const int CONFIG_TWS_AFH_ENABLE                     = 0;
const int CONFIG_TWS_WORK_MODE                      = 2;
const int CONFIG_TWS_AUDIO_SHARE_ENABLE             = 0;
const int CONFIG_TWS_FORWARD_TIMES                  = 1;
const int CONFIG_TWS_RUN_SLOT_MAX                   = 48;
const int CONFIG_TWS_RUN_SLOT_AT_A2DP_FORWARD       = 8;
const int CONFIG_TWS_RUN_SLOT_AT_LOW_LATENCY        = 8;
const int CONFIG_TWS_RUN_SLOT_AT_LOCAL_MEDIA_TRANS  = 48;
const int CONFIG_TWS_SUPER_TIMEOUT                  = 2000;

const int CONFIG_PAGE_POWER                         = 9;
const int CONFIG_PAGE_SCAN_POWER                    = 9;
const int CONFIG_PAGE_SCAN_POWER_DUT                = 4;
const int CONFIG_INQUIRY_SCAN_POWER                 = 7;
const int CONFIG_INQUIRY_POWER                      = 7;
const int CONFIG_DUT_POWER                          = 10;

//固定使用正常发射功率的等级:0-使用不同模式的各自等级;1~10-固定发射功率等级
const int config_force_bt_pwr_tab_using_normal_level = 0;
/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth Classic setting
 */
unsigned char config_btctler_bredr_master           = 0;  ///bredr 强制 做 maseter

#if TCFG_SUPPORT_DUAL_A2DP_SOURCE
const int CONFIG_BTCTLER_DUAL_A2DP                  = 1;
#else
const int CONFIG_BTCTLER_DUAL_A2DP                  = 0;
#endif
const int CONFIG_TEST_FCC_CODE                      = 1;
const int CONFIG_TEST_DUT_ONLY_BOX_CODE             = 0;
const int CONFIG_BREDR_AFH_USER                     = 0;
#if TCFG_USER_EMITTER_ENABLE
const int CONFIG_MASTER_QOS_POLL                    = BT_QOS_EMITTER;
const int CONFIG_BREDR_INQUIRY                      = 1;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST         = 1;
#else
const int CONFIG_MASTER_QOS_POLL                    = 0; //BT_QOS_TWS
const int CONFIG_BREDR_INQUIRY                      = 0;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST         = 0;
#endif

const int CONFIG_A2DP_DATA_CACHE_LOW_AAC            = 100;
const int CONFIG_A2DP_DATA_CACHE_HI_AAC             = 250;
const int CONFIG_A2DP_DATA_CACHE_LOW_SBC            = 150;
const int CONFIG_A2DP_DATA_CACHE_HI_SBC             = 260;
#if defined TCFG_A2DP_DELAY_TIME_AAC
const int CONFIG_A2DP_DELAY_TIME_AAC                = TCFG_A2DP_DELAY_TIME_AAC;
#else
const int CONFIG_A2DP_DELAY_TIME_AAC                = 300;
#endif
#if defined TCFG_A2DP_DELAY_TIME_SBC
const int CONFIG_A2DP_DELAY_TIME_SBC                = TCFG_A2DP_DELAY_TIME_SBC;
#else
const int CONFIG_A2DP_DELAY_TIME_SBC                = 300;
#endif
#if defined TCFG_A2DP_DELAY_TIME_SBC_LO
const int CONFIG_A2DP_DELAY_TIME_SBC_LO             = TCFG_A2DP_DELAY_TIME_SBC_LO;
#else
const int CONFIG_A2DP_DELAY_TIME_SBC_LO             = 100;
#endif
#if defined TCFG_A2DP_DELAY_TIME_AAC_LO
const int CONFIG_A2DP_DELAY_TIME_AAC_LO             = TCFG_A2DP_DELAY_TIME_AAC_LO;
#else
const int CONFIG_A2DP_DELAY_TIME_AAC_LO             = 100;
#endif
#if defined TCFG_A2DP_ADAPTIVE_MAX_LATENCY
const int CONFIG_A2DP_ADAPTIVE_MAX_LATENCY          = TCFG_A2DP_ADAPTIVE_MAX_LATENCY;
#else
const int CONFIG_A2DP_ADAPTIVE_MAX_LATENCY          = 550;
#endif

#if (TCFG_BT_SUPPORT_LHDC_V5 || TCFG_BT_SUPPORT_LHDC || TCFG_BT_SUPPORT_LDAC)
//LHDC/LDAC使用较高码率时需要增大蓝牙buf
const int CONFIG_A2DP_MAX_BUF_SIZE                  = 50 * 1024;
const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT           = 40;
#else
const int CONFIG_A2DP_MAX_BUF_SIZE                  = 30 * 1024;
const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT           = 4;
#endif

const int config_btctler_eir_version_info_len       = 21;



/*-----------------------------------------------------------*/
/**
 * @brief Bluetooth LE setting
 */
const int CONFIG_BLE_SYNC_WORD_BIT                  = 30;
const int CONFIG_LE_HIGH_PRIORITY                   = 0;
const int config_tws_le_role_sw                     = 0;
const int config_rf_oob                             = 0;
const u32 config_vendor_le_bb                       = 0;
const int config_btctler_le_slave_conn_update_winden = 2500;//range:100 to 2500
const int config_btctler_single_carrier_en          = 0;
const int sniff_support_reset_anchor_point          = 0;
const int config_bt_security_vulnerability          = 0;
const int config_btctler_le_afh_en                  = 0;
const int config_btctler_le_tws                     = 0;

// Master multi-link
const int config_btctler_le_master_multilink        = 1;
// Slave multi-link
const int config_btctler_le_slave_multilink         = 1;

#ifndef TCFG_TRANS_MULTI_BLE_SLAVE_NUMS
#define TCFG_TRANS_MULTI_BLE_SLAVE_NUMS             1
#endif
#ifndef TCFG_TRANS_MULTI_BLE_MASTER_NUMS
#define TCFG_TRANS_MULTI_BLE_MASTER_NUMS            0
#endif

#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)
const int config_btctler_le_hw_nums                 = 6;
#elif (TCFG_LEA_CIG_CENTRAL_EN || TCFG_LEA_CIG_PERIPHERAL_EN)
const int config_btctler_le_hw_nums                 = 5;
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
const int config_btctler_le_hw_nums                 = 8;
#else
const int config_btctler_le_hw_nums                 = TCFG_TRANS_MULTI_BLE_SLAVE_NUMS + TCFG_TRANS_MULTI_BLE_MASTER_NUMS;
#endif

//le 配置,可以优化代码和RAM
const int config_le_sm_support_enable               = 1; //是否支持加密配对
const int config_le_hci_connection_num              = TCFG_TRANS_MULTI_BLE_SLAVE_NUMS + TCFG_TRANS_MULTI_BLE_MASTER_NUMS; //支持同时连接个数
const int config_le_gatt_server_num                 = TCFG_TRANS_MULTI_BLE_SLAVE_NUMS + TCFG_TRANS_MULTI_BLE_MASTER_NUMS; //支持server角色个数
const int config_le_gatt_client_num                 = TCFG_TRANS_MULTI_BLE_MASTER_NUMS; //支持client角色个数

/* const uint64_t config_btctler_le_features = (LE_ENCRYPTION | LE_FEATURES_ISO | LE_EXTENDED_ADVERTISING | LE_PERIODIC_ADVERTISING | LL_FEAT_ISO_SYNC | LL_FEAT_ISO_BROADCASTER | LL_FEAT_CIS_MASTER_ROLE | LL_FEAT_CIS_SLAVE_ROLE | LL_FEAT_LE_2M_PHY | LL_FEAT_LE_CODED_PHY); */

#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)

const int config_btctler_le_roles =
#if TCFG_LEA_BIG_CTRLER_TX_EN
    (LE_ADV | LE_SLAVE | LE_SCAN) |
#endif
#if TCFG_LEA_BIG_CTRLER_RX_EN
    (LE_SCAN | LE_MASTER | LE_INIT | LE_ADV | LE_SLAVE) |
#endif
    0;

#if TCFG_KBOX_1T3_MODE_EN
const uint64_t config_btctler_le_features           = LL_FEAT_ISO_SYNC | LE_2M_PHY | CHANNEL_SELECTION_ALGORITHM_2 | LL_FEAT_VENDOR_BIG_SYNC_TRANSFER;
//pkt_v3 optimized_en:BIT(18)
#if defined(TCFG_BB_PKT_V3_EN) && TCFG_BB_PKT_V3_EN
const int config_bb_optimized_ctrl                  = BIT(18) | BIT(19) | VENDOR_BB_ISO_DIRECT_PUSH; //BIT(7);//|BIT(8);
#else
const int config_bb_optimized_ctrl                  = VENDOR_BB_ISO_DIRECT_PUSH;//BIT(7);//|BIT(8);
#endif
#else
const int config_bb_optimized_ctrl                  = 0;//BIT(7);//|BIT(8);
const uint64_t config_btctler_le_features           = LL_FEAT_ISO_BROADCASTER | LL_FEAT_ISO_SYNC | LL_FEAT_ISO_HOST_SUPPORT | LE_2M_PHY | CHANNEL_SELECTION_ALGORITHM_2 | LE_EXTENDED_ADVERTISING | LE_PERIODIC_ADVERTISING;
#endif

// LE RAM Control
const int config_btctler_le_rx_nums                 = TCFG_LEA_BIG_CTRLER_RX_EN ? 20 : 5;
const int config_btctler_le_acl_packet_length       = TCFG_LEA_BIG_CTRLER_TX_EN ? 255 : 27;
const int config_btctler_le_acl_total_nums          = 10;

#elif (TCFG_LEA_CIG_CENTRAL_EN || TCFG_LEA_CIG_PERIPHERAL_EN)

const int config_btctler_le_roles =
#if TCFG_LEA_CIG_CENTRAL_EN
    (LE_SCAN | LE_INIT | LE_MASTER) | LE_ADV |    //添加LE_ADV是为了测试盒用
#endif
#if LEA_CIG_PERIPHERAL_EN
    (LE_ADV | LE_SLAVE) |
#endif
    0;

const uint64_t config_btctler_le_features           = LE_FEATURES_CIS | LE_2M_PHY | CHANNEL_SELECTION_ALGORITHM_2;

// LE RAM Control
const int config_btctler_le_rx_nums                 = TCFG_LEA_CIG_PERIPHERAL_EN ? 20 : 5;
const int config_btctler_le_acl_packet_length       = TCFG_LEA_CIG_CENTRAL_EN ? 255 : 27;
const int config_btctler_le_acl_total_nums          = 10;
const int config_bb_optimized_ctrl                  = BIT(13) | BIT(14) | BIT(20) | VENDOR_BB_ISO_DIRECT_PUSH | VENDOR_BB_DUAL_BD_SWITCH;

#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))

const int config_btctler_le_roles                   = (LE_MASTER | LE_SLAVE | LE_ADV | LE_SCAN | LE_INIT);
const uint64_t config_btctler_le_features           = LL_FEAT_ISO_BROADCASTER | LE_DATA_PACKET_LENGTH_EXTENSION | LL_FEAT_ISO_SYNC | LL_FEAT_ISO_HOST_SUPPORT | LE_2M_PHY | CHANNEL_SELECTION_ALGORITHM_2 | LE_EXTENDED_ADVERTISING | LE_PERIODIC_ADVERTISING | LE_ENCRYPTION;
const int config_btctler_le_rx_nums                 = 20;
const int config_btctler_le_acl_packet_length       = 255;
const int config_btctler_le_acl_total_nums          = 15;
const int config_bb_optimized_ctrl                  = VENDOR_BB_ISO_DIRECT_PUSH;//BIT(7);//|BIT(8);

#else /* TCFG_USER_BLE_ENABLE */

const uint64_t config_btctler_le_features           = (LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LL_FEAT_LE_EXT_ADV | LL_FEAT_LE_2M_PHY);
const int config_btctler_le_roles                   = (LE_ADV | LE_SCAN | LE_INIT | LE_SLAVE | LE_MASTER);
const int config_btctler_le_rx_nums                 = 5 * config_btctler_le_hw_nums;
const int config_btctler_le_acl_packet_length       = 255;
const int config_btctler_le_acl_total_nums          = 5 * config_btctler_le_hw_nums;
const int config_bb_optimized_ctrl                  = VENDOR_BB_ISO_DIRECT_PUSH;//BIT(7);//|BIT(8);

#endif//end TCFG_USER_BLE_ENABLE

#endif

/*-----------------------------------------------------------*/

/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
//RF part
const char log_tag_const_v_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);

const char log_tag_const_v_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);

const char log_tag_const_v_BDMGR AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_BDMGR AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_BDMGR AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_BDMGR AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_BDMGR AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

//Classic part
const char log_tag_const_v_HCI_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_HCI_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_HCI_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_HCI_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_HCI_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

//LE part
const char log_tag_const_v_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_ISO AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_ISO AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_ISO AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_LL_ISO AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_ISO AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_BIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_BIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_BIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_BIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_BIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_CIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_CIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_CIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_CIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_CIS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

//HCI part
const char log_tag_const_v_H4_USB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_H4_USB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_H4_USB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_H4_USB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_H4_USB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LINK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LINK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_LINK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_LINK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LINK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_TWS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_TWS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_TWS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_TWS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_TWS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_c_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_i_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);

