#include "generic/typedef.h"
#include "app_config.h"

#ifdef CONFIG_UI_ENABLE

/**
 * 0:flash资源使用sfc访问 1:flash资源使用内存拷贝
 * 说明: 硬件只有1个flash,没有sdnand,并且代码加载到ddr运行的情况下使用
 */
const uint8_t flash_src_use_malloc = 0;

//临时用法，写1时 使用可视化音频接口播放AVI音频
const uint8_t jlffmpeg_use_media_interface = 1;
const uint8_t lv_jlffmpeg_debug_time = 0;    //读卡时间debug
const uint8_t lv_jlffmpeg_use_frame_ctl = 1; //使用avi实际帧率
const uint8_t frame_control_sens = 0;        //灵敏度设置:单位ms,作用是减小帧率波动极值

const char log_tag_const_v_JLFFMPEG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_JLFFMPEG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_JLFFMPEG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_JLFFMPEG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_JLFFMPEG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

#endif


