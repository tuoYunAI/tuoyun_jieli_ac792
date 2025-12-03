#include "event/key_event.h"
#include "system/init.h"
#include "system/timer.h"
#include "key/key_driver.h"
#include "key/iokey.h"
#include "key/adkey.h"
#include "key/slidekey.h"
#include "key/irkey.h"
#include "key/touch_key.h"
#include "key/rdec_key.h"
#include "key/ctmu_key.h"
#include "key/tent600_key.h"
#include "app_config.h"
#include "asm/power_interface.h"

#define LOG_TAG_CONST       KEY
#define LOG_TAG             "[KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
//#define LOG_DUMP_ENABLE
//#define LOG_CLI_ENABLE
#include "system/debug.h"

#define KEY_EVENT_CLICK_ONLY_SUPPORT    1   //是否支持某些按键只响应单击事件

static volatile u8 is_key_active = 0;

#if TCFG_RDEC_KEY_ENABLE
static u8 key_value0; 	//键值1
static u8 key_value1; 	//键值2
#endif
//=======================================================//
// 按键值重新映射函数:
// 用户可以实现该函数把一些按键值重新映射, 可用于组合键的键值重新映射
//=======================================================//
int __attribute__((weak)) key_event_remap(struct key_event *e)
{
    return true;
}

//=======================================================//
// 按键扫描函数: 扫描所有注册的按键驱动
//=======================================================//
static void key_driver_scan(void *key)
{
    struct key_driver_ops *key_handler = (struct key_driver_ops *)key;
    struct key_driver_para *scan_para = (struct key_driver_para *)key_handler->param;

    u8 key_event = 0;
    u8 cur_key_value = NO_KEY;
    u8 key_value = 0;
    struct key_event e = {0};

    cur_key_value = key_handler->get_value();


#if TCFG_RDEC_KEY_ENABLE
    if (scan_para->key_type == KEY_DRIVER_TYPE_RDEC) {
        if (cur_key_value != 0 && cur_key_value != 255) {
            if (cur_key_value == key_value0) {
                key_event = KEY_EVENT_RDEC_UP;
            } else if (cur_key_value == key_value1) {
                key_event = KEY_EVENT_RDEC_DOWN;
            } else {
                return;
            }
            key_value = cur_key_value;
            goto __notify;
        } else {
            return;
        }
    }
#endif


    if (cur_key_value != NO_KEY) {
        is_key_active = 35;      //35*10Ms
    } else if (is_key_active) {
        is_key_active --;
    }
//===== 按键消抖处理
    if (cur_key_value != scan_para->filter_value && scan_para->filter_time) {	//当前按键值与上一次按键值如果不相等, 重新消抖处理, 注意filter_time != 0;
        scan_para->filter_cnt = 0; 		//消抖次数清0, 重新开始消抖
        scan_para->filter_value = cur_key_value;	//记录上一次的按键值
        return; 		//第一次检测, 返回不做处理
    } 		//当前按键值与上一次按键值相等, filter_cnt开始累加;
    if (scan_para->filter_cnt < scan_para->filter_time) {
        scan_para->filter_cnt++;
        return;
    }
    //为了滤掉adkey与mic连在一起时电容充放电导致的按键误判,一般用于type-c耳机
    /* if ((cur_key_value != scan_para->last_key) && (scan_para->last_key != NO_KEY) && (cur_key_value != NO_KEY)) { */
    /*     return; */
    /* } */
//===== 按键消抖结束, 开始判断按键类型(单击, 双击, 长按, 多击, HOLD, (长按/HOLD)抬起)
    if (cur_key_value != scan_para->last_key) {
        if (cur_key_value == NO_KEY) {  //cur_key = NO_KEY; last_key = valid_key -> 按键被抬起
#if MOUSE_KEY_SCAN_MODE
            /* if (scan_para->press_cnt >= scan_para->long_time) {  //长按/HOLD状态之后被按键抬起; */
            key_event = KEY_EVENT_UP;
            key_value = scan_para->last_key;
            goto __notify;  	//发送抬起消息
            /* } */
#else
            if (scan_para->press_cnt >= scan_para->long_time) {  //长按/HOLD状态之后被按键抬起;
                key_event = KEY_EVENT_UP;
                key_value = scan_para->last_key;
                goto __notify;  	//发送抬起消息
            }
#endif

            scan_para->click_delay_cnt = 1;  //按键等待下次连击延时开始
#if defined CONFIG_UI_ENABLE && (defined USE_LVGL_V8_UI_DEMO || defined USE_LVGL_V9_UI_DEMO)
            e.type = scan_para->key_type;//区分按键类型
            e.action = KEY_EVENT_UP;
            e.value = scan_para->last_key;

            if (key_event_remap(&e)) {
                key_event_notify(KEY_EVENT_FROM_KEY, &e);
            }
#endif
        } else {  //cur_key = valid_key, last_key = NO_KEY -> 按键被按下
            scan_para->press_cnt = 1;  //用于判断long和hold事件的计数器重新开始计时;
            if (cur_key_value != scan_para->notify_value) {  //第一次单击/连击时按下的是不同按键, 单击次数重新开始计数
                scan_para->click_cnt = 1;
                scan_para->notify_value = cur_key_value;
            } else {
                scan_para->click_cnt++;  //单击次数累加
            }
#if defined CONFIG_UI_ENABLE && (defined USE_LVGL_V8_UI_DEMO || defined USE_LVGL_V9_UI_DEMO)
            e.type = scan_para->key_type;//区分按键类型
            e.action = KEY_EVENT_DOWN;
            e.value = cur_key_value;

            if (key_event_remap(&e)) {
                key_event_notify(KEY_EVENT_FROM_KEY, &e);
            }
#endif
        }
        goto __scan_end;  //返回, 等待延时时间到
    } else { 	//cur_key = last_key -> 没有按键按下/按键长按(HOLD)
        if (cur_key_value == NO_KEY) {  //last_key = NO_KEY; cur_key = NO_KEY -> 没有按键按下
            if (scan_para->click_cnt > 0) {  //有按键需要消息需要处理
#if KEY_EVENT_CLICK_ONLY_SUPPORT 	//是否支持某些按键只响应单击事件
                if (scan_para->notify_value & BIT(7)) {  //BIT(7)按键特殊处理标志, 只发送单击事件, 也可以用于其它扩展
                    key_event = KEY_EVENT_CLICK;  //单击
                    key_value = scan_para->notify_value;
                    goto __notify;
                }
#endif
                if (scan_para->click_delay_cnt > scan_para->click_delay_time) { //按键被抬起后延时到
                    //TODO: 在此可以添加任意多击事件
                    if (scan_para->click_cnt >= 5) {
                        key_event = KEY_EVENT_FIRTH_CLICK;  //五击
                    } else if (scan_para->click_cnt >= 4) {
                        key_event = KEY_EVENT_FOURTH_CLICK;  //4击
                    } else if (scan_para->click_cnt >= 3) {
                        key_event = KEY_EVENT_TRIPLE_CLICK;  //三击
                    } else if (scan_para->click_cnt >= 2) {
                        key_event = KEY_EVENT_DOUBLE_CLICK;  //双击
                    } else {
                        key_event = KEY_EVENT_CLICK;  //单击
                    }
                    key_value = scan_para->notify_value;
                    goto __notify;
                } else {	//按键抬起后等待下次延时时间未到
                    scan_para->click_delay_cnt++;
                    goto __scan_end; //按键抬起后延时时间未到, 返回
                }
            } else {
                goto __scan_end;  //没有按键需要处理
            }
        } else {  //last_key = valid_key; cur_key = valid_key, press_cnt累加用于判断long和hold
            scan_para->press_cnt++;
            if (scan_para->press_cnt == scan_para->long_time) {
                key_event = KEY_EVENT_LONG;
            } else if (scan_para->press_cnt == scan_para->hold_time) {
                key_event = KEY_EVENT_HOLD;
                scan_para->press_cnt = scan_para->long_time;
            } else {
                goto __scan_end;  //press_cnt没到长按和HOLD次数, 返回
            }
            //press_cnt没到长按和HOLD次数, 发消息
            key_value = cur_key_value;
            goto __notify;
        }
    }

__notify:
    key_value &= ~BIT(7);  //BIT(7) 用作按键特殊处理的标志

    e.type = scan_para->key_type;//区分按键类型
    e.action = key_event;
    e.value = key_value;

    scan_para->click_cnt = 0;  //单击次数清0
    scan_para->notify_value = NO_KEY;

    /* printf("key_value: 0x%x, event: %d\n", key_value, key_event); */
    if (key_event_remap(&e)) {
        key_event_notify(KEY_EVENT_FROM_KEY, &e);
    }

__scan_end:
    scan_para->last_key = cur_key_value;
}

//wakeup callback
void key_active_set(u8 port)
{
    is_key_active = 35;      //35*10Ms
}

//=======================================================//
// 按键初始化函数: 初始化所有注册的按键驱动
//=======================================================//
int key_driver_init(void)
{
    const struct key_driver_ops *key;

    list_for_each_key_driver(key) {
        if (!key->key_init) {
            continue;
        }
        if ((!key->key_init()) && key->get_value) {
            sys_s_hi_timer_add((void *)key, key_driver_scan, key->param->scan_time); //注册按键扫描定时器
        }
    }

#if TCFG_RDEC_KEY_ENABLE
    struct rdec_platform_data *platform_data = get_rdec_key_platform_data();
    if (platform_data) {
        key_value0 = platform_data->rdec[0].key_value0;
        key_value1 = platform_data->rdec[0].key_value1;
    }
#endif

    return 0;
}

static u8 key_idle_query(void)
{
    return !is_key_active;
}

#if !TCFG_LP_TOUCH_KEY_ENABLE
REGISTER_LP_TARGET(key_lp_target) = {
    .name = "key",
    .is_idle = key_idle_query,
};
#endif /* #if !TCFG_LP_TOUCH_KEY_ENABLE */

