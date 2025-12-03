#include "key/touch_key.h"
#include "key/key_driver.h"
#include "asm/plcnt.h"
#include "app_config.h"


#if TCFG_TOUCH_KEY_ENABLE

/* =========== 触摸键使用说明 ============= */
//1. 使用plcnt模块作计数;
//2. 配置参数时, 在配置好时钟后, 需要调试no_touch_cnt和touch_cnt的值;

static const struct touch_key_platform_data *__this = NULL;

//按键驱动扫描参数列表
static struct key_driver_para touch_key_scan_param = {
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .filter_time  	  = 1,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 15),  	//按键判定HOLD数量
    .click_delay_time = 20,				//按键被抬起后等待连击延时数量
    .key_type		  = KEY_DRIVER_TYPE_TOUCH,
};

static u8 touch_key_get_value(void)
{
    u8 key = get_plcnt_value();

    if (key != NO_KEY) {
        return __this->port_list[key].key_value;
    }

    return NO_KEY;
}

__attribute__((weak))
const struct touch_key_platform_data *get_touch_key_platform_data(void)
{
    return NULL;
}

static int touch_key_init(void)
{
    __this = get_touch_key_platform_data();
    if (!__this) {
        return -EINVAL;
    }

    return plcnt_init((void *)__this);
}

REGISTER_KEY_DRIVER_OPS(touch_key) = {
    .param          = &touch_key_scan_param,
    .get_value      = touch_key_get_value,
    .key_init       = touch_key_init,
};

#endif  	/* #if TCFG_TOUCH_KEY_ENABLE */

