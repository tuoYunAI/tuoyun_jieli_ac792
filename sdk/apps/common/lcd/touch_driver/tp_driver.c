#include "app_config.h"
#include "lcd_config.h"
#include "tp_driver.h"
#include "stdlib.h"

#define LOG_TAG_CONST       TP
#define LOG_TAG             "[TP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

#if TCFG_TP_DRIVER_ENABLE

#define TP_USE_TASK     1                      ///< TP是否使用任务

#define TP_RESOLUTION_SCALE_ENABLE   0         ///< TP与显示屏分辨率不一致时，可使能TP坐标缩放，有一定误差，仅调试用
#if TP_RESOLUTION_SCALE_ENABLE
#define TP_REAL_X                    4080      ///< TP驱动芯片返回的实际最大X坐标值，单位pixel
#define TP_REAL_Y                    4080      ///< TP驱动芯片返回的实际最大Y坐标值，单位pixel
#endif

#define TP_EVENT_INTERVAL            100       ///< TP发送触摸事件到系统的最小间隔，单位ms
#define TP_REPOST_DATA_TO_UI_TIME    2         ///< UI接收不过来数据，TP重新发送数据给UI的时间间隔，单位system ticks
#define TP_FILTER_SAME_COORDINATE    0         ///< 过滤相同坐标。如果UI没有长按事件的需求，可打开减少消息发送消耗


#if TP_USE_TASK
#define TP_TASK_NAME                 "tp_task" ///< 触摸面板任务名
static OS_SEM tp_drdy_sem;                     ///< 表示TP芯片数据ready的信号
#endif

static const tp_platform_data_t *tp_pd_data;   ///< 指向板级里注册的驱动配置
static const tp_driver_t *tp_driver;           ///< 指向注册的TP芯片驱动接口
#define __this tp_driver

static tp_info_t tp_latest_info;               ///< 最新的提供给ui的坐标&状态数据
#if TP_FILTER_SAME_COORDINATE
static tp_info_t tp_last_info;                 ///< 存放上次发送给UI的坐标&状态数据
#endif

/************************************坐标算法相关的变量 start************************************/
#define VK_Y_MOVE       30    ///< Y方向识别为移动的阈值
#define VK_X_MOVE       30    ///< X方向识别为移动的阈值
#define VK_X_Y_DIFF     27    ///< X与Y各自的移动距离的差值超过该值才能识别为移动
#define VK_HOLD_COUNT   30    ///< 触摸按下多少次被识别为HOLD事件的阈值
static u8 tp_last_status = ELM_EVENT_TOUCH_UP;
static touch_action_t tp_last_event = TOUCH_EVENT_NULL;
static volatile u8 tp_post_to_ui_enable = 1;
static volatile u8 tp_post_enable = 1;

static u8 y_mirror;
static u8 x_mirror;
static u8 swap_x_y;
static u16 vk_y = LCD_H;
static u16 vk_x = LCD_W;
/************************************坐标算法相关的变量 end************************************/

static touch_elm_t tp_move(int x, int y, int last_x, int last_y)
{
    static u8 move_flag = 0;
    touch_elm_t elm;
    int x_move = abs(x - last_x);
    int y_move = abs(y - last_y);

    if (!move_flag && (x_move >= VK_X_MOVE || y_move >= VK_Y_MOVE) && (abs(x_move - y_move) >= VK_X_Y_DIFF)) {
        if (x_move > y_move) {
            if (x > last_x) {
                elm = ELM_EVENT_TOUCH_R_MOVE; ///< 快速右移
                log_debug("----tp_rigth_move----x=%d, y=%d", x, y);
            } else {
                elm = ELM_EVENT_TOUCH_L_MOVE; ///< 快速左移
                log_debug("----tp_left_move----x=%d, y=%d", x, y);
            }
        } else {
            if (y > last_y) {
                elm = ELM_EVENT_TOUCH_D_MOVE; ///< 快速下移
                log_debug("----tp_down_move----x=%d, y=%d", x, y);
            } else {
                elm = ELM_EVENT_TOUCH_U_MOVE; ///< 快速上移
                log_debug("----tp_up_move----x=%d, y=%d", x, y);
            }
        }
    } else {
        elm = ELM_EVENT_TOUCH_MOVE; ///< 慢速移动
        log_debug("----tp_move----x=%d, y=%d", x, y);
    }

    return elm;
}

static touch_action_t tp_down(int x, int y)
{
    static int last_x = 0;
    static int last_y = 0;
    static int tp_down_cnt = 0;

    // 第一次按下
    if (tp_last_status == ELM_EVENT_TOUCH_UP) {
        tp_last_event = TOUCH_EVENT_DOWN;
        tp_last_status = ELM_EVENT_TOUCH_DOWN;
        tp_down_cnt = 0;
        last_x = x;
        last_y = y;
        log_debug("----tp_down----x=%d, y=%d", x, y);
        return tp_last_event;
    }

    // 除了第一次按下事件，其他事件只要有移动，则进入移动事件解析
    if ((x != last_x) || (y != last_y)) {
        /* tp_last_status = tp_move(x, y, last_x, last_y); */
        /* tp_last_event = ?; ///< 后续有需要可新增EVENT_MOVE事件 */
        /* log_debug("tp_last_status: %d", tp_last_status); */
        last_x = x;
        last_y = y;
        return tp_last_event;
    }

    // 按下事件处理，并且没有出现移动，解析是否为HOLD事件
    if (tp_last_status == ELM_EVENT_TOUCH_DOWN) {
        tp_down_cnt++;
        if (tp_down_cnt < VK_HOLD_COUNT) {
            tp_last_event = TOUCH_EVENT_DOWN;
        } else {
            tp_last_status = ELM_EVENT_TOUCH_HOLD;
            tp_last_event = TOUCH_EVENT_HOLD;
            tp_down_cnt = 0;
            log_debug("----tp_hold----x=%d, y=%d", x, y);
        }
    }

    // 剩下的事件，都直接返回上次的状态
    return tp_last_event;
}

static touch_action_t tp_up(int x, int y)
{
    tp_last_status = ELM_EVENT_TOUCH_UP;
    tp_last_event = TOUCH_EVENT_UP;
    log_debug("----tp_up----x=%d, y=%d", x, y);
    return tp_last_event;
}

static void tp_xy_calc(tp_info_t *data_in, tp_info_t *data_out)
{
    tp_info_t temp;

    temp.status = data_in->status;

    if (swap_x_y) {
        temp.x = data_in->y;
        temp.y = data_in->x;
    } else {
        temp.x = data_in->x;
        temp.y = data_in->y;
    }

    if (temp.x < 0) {
        temp.x = 0;
    }
    if (temp.y < 0) {
        temp.y = 0;
    }

    if (y_mirror) {
        temp.x = vk_x - temp.x - 1;
    }

    if (x_mirror) {
        temp.y = vk_y - temp.y - 1;
    }

#if TP_RESOLUTION_SCALE_ENABLE
    temp.x = (temp.x - 1) * LCD_W / TP_REAL_X;
    temp.y = (temp.y - 1) * LCD_H / TP_REAL_Y;
#endif

    if (temp.x > (vk_x - 1)) {
        temp.x = vk_x - 1;
    }
    if (temp.y > (vk_y - 1)) {
        temp.y = vk_y - 1;
    }

    data_out->act = temp.status ? tp_down(temp.x, temp.y) : tp_up(temp.x, temp.y);
    data_out->x = temp.x;
    data_out->y = temp.y;
    data_out->status = temp.status;
}


void get_touch_x_y_status(uint16_t *x, uint16_t *y, uint8_t *status)
{
    if (touch_event_is_disable()) {
        *x = 0;
        *y = 0;
        *status = TP_STATUS_RELEASE;
        return;
    }

    *x = tp_latest_info.x;
    *y = tp_latest_info.y;
    *status = tp_latest_info.status;
}


// 计算触摸面板中断触发间隔。(测试时长按触摸面板)
static void tp_calc_interval_of_trig(void)
{
    static u32 last_touch_trig_time;
    u32 current_touch_trig_time = timer_get_ms();
    log_debug("touch_interupt period = %dms", current_touch_trig_time - last_touch_trig_time);
    last_touch_trig_time = current_touch_trig_time;
}


//发送触摸事件到系统
static int tp_event_notify_sys(u16 x, u16 y, touch_action_t act)
{
    static u32 tp_last_send_time;
    u32 now_time = get_system_ms();
    struct touch_event event;
    event.x = x;
    event.y = y;
    event.action = act;

    if ((now_time - tp_last_send_time) < TP_EVENT_INTERVAL) {
        return 0; ///< 低于最低发送间隔，取消发送事件给系统
    }

    log_debug("tp_event_notify_sys: %d\n", act);
    if (touch_event_notify(TOUCH_EVENT_FROM_TOUCH, &event)) {
        return -1;
    }


    if (touch_event_check_consume()) {
        return -1;
    }

    tp_last_send_time = now_time;

    return 0;
}


int __attribute__((weak)) lcd_touch_interrupt_event(const char *tp_task_name, u16 x, u16 y, u8 status)
{
    log_info("Please implement this function to post coordinate data to UI!");
    return 0;
}



#if TP_USE_TASK

static void tp_task(void *arg)
{
    int sem_timeout = 0;
    int os_pend_status;
    tp_info_t tp_data;

    while (1) {
        os_pend_status = os_sem_pend(&tp_drdy_sem, sem_timeout);

        if (os_pend_status == OS_TIMEOUT) {
            log_debug("os pend timeout, tp repost data to UI.");
            goto __tp_post_xy_to_ui;
        }

        /* if (lcd_get_rotate()) { */
        /* lcd_touch_width_height_rotate(&vk_x, &vk_y); */
        /* } */

        __this->get_xy_and_status(&tp_data);

        if (tp_data.status == TP_STATUS_PRESS || tp_data.status == TP_STATUS_RELEASE) {
            tp_xy_calc(&tp_data, &tp_latest_info);
            /* tp_calc_interval_of_trig(); */
        } else {
            continue;
        }

        /* if (lcd_get_rotate()) { */
        /* lcd_touch_xy_coord_rotate(&tp_latest_info.x, &tp_latest_info.y, tp_latest_info.status); */
        /* } */

        if (!tp_post_enable) {
            /* log_debug("no send data"); */
            // tp disable时，抬起的消息还是需要发给ui
            if (tp_latest_info.status == TP_STATUS_RELEASE) {
                goto __tp_post_xy_to_ui;
            }
            continue;
        }

        // 发送触摸事件，根据返回值决定是否将坐标发送给UI
        if (tp_event_notify_sys(tp_latest_info.x, tp_latest_info.y, tp_latest_info.act) < 0) {
            log_debug("touch position x:%d y:%d", tp_latest_info.x, tp_latest_info.y);
            continue;
        }

        // tp post to ui disable时，只拦截发给ui的按下消息
        if (!tp_post_to_ui_enable && (tp_latest_info.status == TP_STATUS_PRESS)) {
            /* log_debug("no send data to ui"); */
            continue;
        }

#if TP_FILTER_SAME_COORDINATE
        if (tp_last_info.x == tp_latest_info.x && \
            tp_last_info.y == tp_latest_info.y && \
            tp_last_info.status == tp_latest_info.status) {
            log_debug("filter same tp coordinates %d, %d, %d", tp_latest_info.x, tp_latest_info.y, tp_latest_info.status);
            continue;
        }
#endif

__tp_post_xy_to_ui:
        log_debug("ui_x:%d, ui_y: %d, ui_status:%d", tp_latest_info.x, tp_latest_info.y, tp_latest_info.status);
        if (lcd_touch_interrupt_event(TP_TASK_NAME, tp_latest_info.x, tp_latest_info.y, tp_latest_info.status)) {
            /*
             * UI线程跑得慢，触摸事件处理不过来，按压坐标可丢失，抬起事件不可丢失:
             * UI触摸事件处理不过来时，continue可以让抬起事件隔20m后再次推送。
             */
            sem_timeout = TP_REPOST_DATA_TO_UI_TIME;
            log_debug("post xy data to UI error!!!");
            continue;
        } else {
#if TP_FILTER_SAME_COORDINATE
            tp_last_info.x = tp_latest_info.x;
            tp_last_info.y = tp_latest_info.y;
            tp_last_info.status = tp_latest_info.status;
            tp_last_info.act = tp_latest_info.act;
#endif
            sem_timeout = 0;
        }
    }
}

#else

static void tp_interrupt_cb(void)
{
    tp_info_t tp_data;
    __this->get_xy_and_status(&tp_data);

    if (tp_data.status == TP_STATUS_PRESS || tp_data.status == TP_STATUS_RELEASE) {
        tp_xy_calc(&tp_data, &tp_latest_info);
    } else {
        return;
    }

    if (!tp_post_enable) {
        if (tp_latest_info.status == TP_STATUS_RELEASE) {
            goto __tp_post_xy_to_ui;
        }
        return;
    }

    // 发送触摸事件，根据返回值决定是否将坐标发送给UI
    if (tp_event_notify_sys(tp_latest_info.x, tp_latest_info.y, tp_latest_info.act) < 0) {
        log_debug("touch position x:%d y:%d", tp_latest_info.x, tp_latest_info.y);
        return;
    }

    // tp post to ui disable时，只拦截发给ui的按下消息
    if (!tp_post_to_ui_enable && (tp_latest_info.status == TP_STATUS_PRESS)) {
        return;
    }

#if TP_FILTER_SAME_COORDINATE
    if (tp_last_info.x == tp_latest_info.x && \
        tp_last_info.y == tp_latest_info.y && \
        tp_last_info.status == tp_latest_info.status) {
        log_debug("filter same tp coordinates %d, %d, %d", tp_latest_info.x, tp_latest_info.y, tp_latest_info.status);
        return;
    }
#endif

__tp_post_xy_to_ui:
    log_debug("ui_x:%d, ui_y: %d, ui_status:%d", tp_latest_info.x, tp_latest_info.y, tp_latest_info.status);
    if (lcd_touch_interrupt_event(NULL, tp_latest_info.x, tp_latest_info.y, tp_latest_info.status)) {
        log_debug("post xy data to UI error!!!");
        return;
    } else {
#if TP_FILTER_SAME_COORDINATE
        tp_last_info.x = tp_latest_info.x;
        tp_last_info.y = tp_latest_info.y;
        tp_last_info.status = tp_latest_info.status;
        tp_last_info.act = tp_latest_info.act;
#endif
    }

}

#endif

static void tp_post_drdy_sem(void)
{
#if TP_USE_TASK
    os_sem_post(&tp_drdy_sem);
#else
    tp_interrupt_cb();
#endif
}

__attribute__((weak)) const tp_platform_data_t *tp_get_platform_data(void)
{
    log_info("Please implement this function to return the configured Platform data!");
    return NULL;
}


int tp_init(void)
{

    log_info("init start");
    tp_pd_data = tp_get_platform_data();
    if (!tp_pd_data) {
        log_error("can't get the platform data!!");
        return -ENODEV;
    }

#if TP_USE_TASK
    os_sem_create(&tp_drdy_sem, 0);
#endif

    const tp_driver_t *p;
    list_for_each_tp_driver(p) {
        log_info(">>>>>>>>>>>%s init<<<<<<<<<<<", p->ic_name);
        __this = p;
        if (p->init && !p->init(tp_pd_data, tp_post_drdy_sem)) {
            break;
        } else {
            __this = NULL;
            log_error("%s init fail!!!!", p->ic_name);
        }
    }

    if (!__this) {

#if TP_USE_TASK
        os_sem_del(&tp_drdy_sem, 0);
#endif

        log_error("Can't find any model of tp driver!!");
        return -ENODEV;
    }

    y_mirror = tp_pd_data->y_mirror;
    x_mirror = tp_pd_data->x_mirror;
    swap_x_y  = tp_pd_data->swap_x_y;

#if TP_USE_TASK
    os_task_create(tp_task, NULL, 29, 256, 32, TP_TASK_NAME);
#endif

    return 0;
}

int tp_deinit(void)
{
    log_info(">>>>deinit");

    if (__this && __this->deinit) {
        __this->deinit(tp_pd_data);
    } else {
        log_warn("This model of tp driver no support deinit function!");
        return -ESRCH;
    }

#if TP_USE_TASK
    os_task_del(TP_TASK_NAME);
    os_sem_del(&tp_drdy_sem, 0);
#endif
    return 0;
}

int tp_check_online(void)
{
    if (__this && __this->check_online) {
        return __this->check_online();
    } else {
        log_warn("This model of tp driver no support check online function!");
        return -ESRCH;
    }
}

int tp_reset(void)
{
    if (__this && __this->reset) {
        __this->reset(tp_pd_data);
        return 0;
    } else {
        log_warn("This model of tp driver no support reset tp function!");
        return -ESRCH;
    }
}

int tp_sleep(void)
{
    if (__this && __this->sleep) {
        return __this->sleep(tp_pd_data);
    } else {
        log_warn("This model of tp driver no support sleep function!");
        return -ESRCH;
    }
}

int tp_wakeup(void)
{
    if (__this && __this->wakeup) {
        return __this->wakeup(tp_pd_data);
    } else {
        log_warn("This model of tp driver no support wakeup function!");
        return -ESRCH;
    }
}

void tp_enable(void)
{
    tp_post_enable = 1;
}

void tp_disable(void)
{
    tp_post_enable = 0;
}

void tp_post_xy_ui_enable(void)
{
    tp_post_to_ui_enable = 1;
}

void tp_post_xy_ui_disable(void)
{
    tp_post_to_ui_enable = 0;
}

static void tp_driver_test(void *arg)
{
#define TEST_TIMES 5
    u8 cnt = 0;

    os_time_dly(500);

    while (1) {
        if (tp_check_online()) {
            // if (__this->check_online()) {
            log_debug("offline");
        } else {
            log_debug("online");
        }
        os_time_dly(100);

        if (++cnt > TEST_TIMES - 1) {
            cnt = 0;
            break;
        }
    }

    while (1) {
        tp_reset();
        // __this->reset(tp_pd_data);
        os_time_dly(500);

        tp_deinit();
        // __this->deinit(tp_pd_data);
        os_time_dly(500);

        tp_init();
        // __this->init(tp_pd_data, &tp_drdy_sem);
        os_time_dly(500);

        if (++cnt > TEST_TIMES - 1) {
            cnt = 0;
            break;
        }
    }

    while (1) {
        tp_sleep();
        // __this->sleep(tp_pd_data);
        os_time_dly(500);

        tp_wakeup();
        // __this->wakeup(tp_pd_data);
        os_time_dly(1000);

        if (++cnt > TEST_TIMES - 1) {
            cnt = 0;
            break;
        }
    }

    u8 flag = 0;
    while (1) {
        if (flag) {
            /* tp_post_xy_ui_enable(); */
            tp_enable();
            printf("tp enable!!\n");
        } else {
            /* tp_post_xy_ui_disable(); */
            tp_disable();
            printf("tp disable!!\n");
        }
        flag = ~flag;
        os_time_dly(500);

        if (++cnt > TEST_TIMES - 1) {
            cnt = 0;
            break;
        }
    }

    if (cnt) {};
}

static int tp_driver_api_test(void)
{
    thread_fork("tp_driver_test", 15, 256, 0, NULL, tp_driver_test, NULL);
    return 0;
}
// late_initcall(tp_driver_api_test);

late_initcall(tp_init);

#endif // TCFG_TP_DRIVER_ENABLE
