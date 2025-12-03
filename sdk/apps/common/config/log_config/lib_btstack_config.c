#include "app_config.h"
#include "avctp_user.h"

/**
 * @brief Bluetooth Stack Module
 */

#if !TCFG_BT_BQB_PROFILE_TEST_ENABLE
u8 hfp_more_feature_hf_indicators = 1;
#endif

const int CONFIG_BTSTACK_SUPPORT_FUN = HFP_UPDATE_BATTERY;

const int CONFIG_BTSTACK_TWS_AUDIO_SHARE_ENABLE = 0;

const int CONFIG_BTSTASK_AUTO_EXIT_SNIFF  = 1;

const int CONFIG_DELETE_LINK_KEY          = 1;	//配置是否连接失败返回PIN or Link Key Missing时删除linkKey

#if TCFG_BT_SUPPORT_AAC
const int CONFIG_BTSTACK_SUPPORT_AAC = 1;
#else
const int CONFIG_BTSTACK_SUPPORT_AAC = 0;
#endif

const int CONFIG_BTSTACK_SUPPORT_APTX = 0;

#if defined(TCFG_BT_SUPPORT_LDAC)
const int CONFIG_BTSTACK_SUPPORT_LDAC = TCFG_BT_SUPPORT_LDAC;
#else
const int CONFIG_BTSTACK_SUPPORT_LDAC = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LHDC)
const int CONFIG_BTSTACK_SUPPORT_LHDC = TCFG_BT_SUPPORT_LHDC;
#else
const int CONFIG_BTSTACK_SUPPORT_LHDC = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LHDC_V5)
const int CONFIG_BTSTACK_SUPPORT_LHDC_V5 = TCFG_BT_SUPPORT_LHDC_V5;
#else
const int CONFIG_BTSTACK_SUPPORT_LHDC_V5 = 0;
#endif

const int CONFIG_BTSTACK_SUPPORT_LC3 = 0;

#if RCSP_MODE
const int CONFIG_RCSP_STACK_ENABLE = 1;
#else
const int CONFIG_RCSP_STACK_ENABLE = 0;
#endif

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)))
const int CONFIG_BTSTACK_LE_AUDIO_ENABLE = 1;
#else
const int CONFIG_BTSTACK_LE_AUDIO_ENABLE = 0;
#endif

#if TCFG_BLE_BRIDGE_EDR_ENALBE || ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
const int config_le_sm_sub_sc_bridge_edr_enable = 1;
const int config_le_sm_sub_sc_enable = 1;
#else
const int config_le_sm_sub_sc_enable = 0;
const int config_le_sm_sub_sc_bridge_edr_enable = 0;
#endif

const int config_product_test_mode = 0;

const char config_product_test_rssi = 50;
const char config_product_adv_pair_rssi = 0;
const char config_product_con_pair_rssi = 0;

const char log_tag_const_v_AVCTP = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_AVCTP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_AVCTP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_AVCTP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AVCTP = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_SPP = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_SPP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_SPP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_SPP = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_SPP = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_LEA = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_LEA = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_LEA = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_LEA = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_LEA = CONFIG_DEBUG_LIB(TRUE);

