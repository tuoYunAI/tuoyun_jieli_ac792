#include "key/key_driver.h"
#include "key/adkey.h"
#include "device/gpio.h"
#include "asm/adc_api.h"
#include "app_config.h"

#if TCFG_ADKEY_ENABLE

static const struct adkey_platform_data *__this = NULL;

//按键驱动扫描参数列表
static struct key_driver_para adkey_scan_param = {
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .filter_time  	  = 2,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 15),  	//按键判定HOLD数量
    .click_delay_time = 20,				//按键被抬起后等待连击延时数量
    .key_type		  = KEY_DRIVER_TYPE_AD,
};

static u8 ad_get_key_value(void)
{
    u16 ad_data = adc_get_value(__this->ad_channel);
    /* printf("ad_value = %d \n", ad_data); */
    for (u8 i = 0; i < ADKEY_MAX_NUM; i++) {
        if ((ad_data <= __this->ad_value[i]) && (__this->ad_value[i] < 0x3ffL)) {
            return __this->key_value[i];
        }
    }
    return NO_KEY;
}

__attribute__((weak))
const struct adkey_platform_data *get_adkey_platform_data(void)
{
    return NULL;
}

static int adkey_init(void)
{
    __this = get_adkey_platform_data();
    if (!__this) {
        return -EINVAL;
    }

    if (__this->ad_channel == ADC_IO_CH_DP || __this->ad_channel == ADC_IO_CH_DM) {
        usb_iomode(0, 1);
    }
    if (__this->extern_up_en) {
        gpio_set_mode(__this->adkey_pin, GPIO_DEINIT);
    } else {
        gpio_set_mode(__this->adkey_pin, GPIO_INPUT_PULL_UP_10K);
        gpio_set_die(__this->adkey_pin, 0);
    }

    adc_add_sample_ch(__this->ad_channel);          //注意：初始化AD_KEY之前，先初始化ADC

    return 0;
}

REGISTER_KEY_DRIVER_OPS(adkey) = {
    .param          = &adkey_scan_param,
    .get_value      = ad_get_key_value,
    .key_init       = adkey_init,
};

#endif  /* #if TCFG_ADKEY_ENABLE */

