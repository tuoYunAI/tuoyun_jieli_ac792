#include "event/key_event.h"
#include "os/os_api.h"
#include "timer.h"
#include "event/key_event.h"
#include "event/touch_event.h"
#include "lvgl.h"
extern uint32_t timer_get_ms(void);

static char lvgl_suspend_flag = 1;
volatile static char lvgl_timerout_msg_remain_flag = 2;
static char lvgl_key_msg_remain_cnt;
static char lvgl_touch_timer_status; //0:轮询定时器关闭  1:轮询定时器打开
static int lvgl_touch_timeout_id;
static int lvgl_task_pid;
static u16 lvgl_timeout_id = 0;

enum {
    UI_MSG_TOUCH = 1,
    UI_MSG_ENCODER,
    UI_MSG_KEY,
    UI_MSG_TIMER_TIMEOUT,
    UI_MSG_SUSPEND,
    UI_MSG_RESUME,
    UI_MSG_RPC_FUNC,
};



enum LVGL_Q_USER_TYPE {
    LVGL_Q_USER_TYPE_TOUCH = 0x10000,//从这个立即数开始是为了删消息池使用,不可更改
    LVGL_Q_USER_TYPE_KEY = 0x20000,
};
void lv_port_get_touch_x_y_status(struct touch_event *e, u16 *x, u16 *y, u8 *status)
{
    *x = e->x;
    *y = e->y;
    *status = e->action;
}

static struct key_event s_drop_key_event;
static u8 s_last_key_value;
static u8 s_last_key_action;
static u16 key_event_drop_timeout_id;

static void key_event_drop_timeout_handler(void *p)
{
    int msg[2 + sizeof(struct key_event) / 4];
    msg[0] = UI_MSG_KEY;

    memcpy(&msg[1], &s_drop_key_event, sizeof(struct key_event));

    if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg)) {
        puts("lvgl_key_event_handler post failed 0");
        key_event_drop_timeout_id = sys_timeout_add_to_task(LVGL_TASK_NAME, NULL, key_event_drop_timeout_handler, 30);
    } else {
        ++lvgl_key_msg_remain_cnt;
        key_event_drop_timeout_id = 0;
    }
}


int lvgl_key_event_handler(struct sys_event *event)
{
    if (lvgl_suspend_flag) {
        return 0;
    }

    struct key_event *key = (struct key_event *)event->payload;
    if (key->key_intercept) {
        return 0;
    }
    if ((key->action == KEY_EVENT_UP) && (key->value != s_last_key_value)) {
        puts("lvgl_key_event_handler post failed 1");
        return -EINVAL;
    }

    if (key_event_drop_timeout_id) {
        puts("lvgl_key_event_handler post failed 2");
        return -EINVAL;
    }

    if (lvgl_key_msg_remain_cnt > 2) {
        puts("lvgl_key_event_handler post failed 3");
        goto  _retry;
    }
    /*printf("lvgl key event->type:%d  key->action:%d  key->value:%d", event->type, key->action, key->value);*/
    int msg[2 + sizeof(struct key_event) / 4];
    msg[0] = UI_MSG_KEY;
    memcpy(&msg[1], key, sizeof(struct key_event));
    if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg)) {
        puts("lvgl_key_event_handler post failed 4");
        goto  _retry;
    } else {
        s_last_key_value = key->value;
        ++lvgl_key_msg_remain_cnt;
    }

    return -EINVAL;

_retry:
    //由于LVGL不可丢失按键抬起消息, 判断丢失的消息是按键抬起消息才做重发动作
    if (key->action == KEY_EVENT_UP && key->value == s_last_key_value) {
        memcpy(&s_drop_key_event, key, sizeof(struct key_event));
        key_event_drop_timeout_id = sys_timeout_add_to_task(LVGL_TASK_NAME, NULL, key_event_drop_timeout_handler, 30);
    }

    return -EINVAL;
}

/*
 * down事件不发给UI,等到按键抬起后up事件拆分为2次事件发给UI
 * 这样做的好处是能区分出短按和长按,例如按键短按和长按分别有不同UI功能时
 * */
int lvgl_key_event_handler_2(struct sys_event *event)
{
    if (lvgl_suspend_flag) {
        return 0;
    }

    struct key_event *key = (struct key_event *)event->payload;
    if (key->key_intercept) {
        return 0;
    }

    int msg[2 + sizeof(struct key_event) / 4];
    //编码器旋钮
    if (key->action == KEY_EVENT_RDEC_UP || KEY_EVENT_RDEC_DOWN) {
        msg[0] = UI_MSG_ENCODER;
        memcpy(&msg[1], key, sizeof(struct key_event));
        if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg)) {
            puts("lvgl_key_event_handler_2 post failed 5");
            goto  _retry;
        } else {
            s_last_key_value = key->value;
            ++lvgl_key_msg_remain_cnt;
        }
        return -EINVAL;
    }


    if ((key->action == KEY_EVENT_UP) && (key->value != s_last_key_value)) {
        puts("lvgl_key_event_handler_2 post failed 1");
        return -EINVAL;
    }
    if (key->action == KEY_EVENT_DOWN) {
        //按下事件不发给UI
        s_last_key_value = key->value;
        s_last_key_action = key->action;
        return -EINVAL;
    }

    if (key_event_drop_timeout_id) {
        puts("lvgl_key_event_handler_2 post failed 2");
        return -EINVAL;
    }

    if (lvgl_key_msg_remain_cnt > 2) {
        puts("lvgl_key_event_handler_2 post failed 3");
        goto  _retry;
    }

    if (key->action == KEY_EVENT_UP) {
        u8 event_split = 2;
        if (s_last_key_action == KEY_EVENT_HOLD || s_last_key_action == KEY_EVENT_LONG) {
            //长按后的抬起UI只响应一次up
            event_split = 1;
        }
        //按键抬起后才开始分发2次key事件,一次down 一次up
        s_last_key_action = key->action;
        for (int i = 0; i < event_split; i++) {
            msg[0] = UI_MSG_KEY;
            if (event_split == 2) {
                key->action = (i == 0) ? KEY_EVENT_DOWN : KEY_EVENT_UP;
            }
            memcpy(&msg[1], key, sizeof(struct key_event));
            /* printf("lv key event->type:%d  key->action:%d  key->value:%d", event->type, key->action, key->value); */
            if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg)) {
                puts("lvgl_key_event_handler_2 post failed 4");
                goto  _retry;
            } else {
                s_last_key_value = key->value;
                ++lvgl_key_msg_remain_cnt;
            }
        }
    } else if (key->action == KEY_EVENT_LONG || key->action == KEY_EVENT_HOLD || key->action == KEY_EVENT_CLICK) {
        msg[0] = UI_MSG_KEY;
        memcpy(&msg[1], key, sizeof(struct key_event));
        s_last_key_action = key->action;
        /* printf("lvgl key event->type:%d  key->action:%d  key->value:%d", event->type, key->action, key->value); */
        if (os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg)) {
            puts("lvgl_key_event_handler_2 post failed 4");
            goto  _retry;
        } else {
            s_last_key_value = key->value;
            ++lvgl_key_msg_remain_cnt;
        }
    }

    return -EINVAL;

_retry:
    //由于LVGL不可丢失按键抬起消息, 判断丢失的消息是按键抬起消息才做重发动作
    if (key->action == KEY_EVENT_UP && key->value == s_last_key_value) {
        memcpy(&s_drop_key_event, key, sizeof(struct key_event));
        key_event_drop_timeout_id = sys_timeout_add_to_task(LVGL_TASK_NAME, NULL, key_event_drop_timeout_handler, 30);
    }

    return -EINVAL;
}

void lv_set_touch_timer_status(u8 status)
{
    lvgl_touch_timer_status = status;
}

static void lv_set_timer_read_touch_mode(u16 x, u16 y, u8 status)
{
    struct touch_event event;
    event.x = x;
    event.y = y;
    event.action = status;
    lv_indev_timer_read_touch(&event);
    lv_indev_set_touch_timer_en(1);
}

int lcd_touch_interrupt_event(const char *tp_task_name, u16 x, u16 y, u8 status)
{
    int err;
    if (lvgl_suspend_flag) {
        return 0;
    }

    if (status) {
        int msg[6];
        //检测到触摸,创建定时器启用轮询
        if (!lvgl_touch_timer_status) {//这个标志是用来防止多次发消息
            msg[0] = UI_MSG_TOUCH;
            msg[1] = (int)x;
            msg[2] = (int)y;
            msg[3] = (int)status;

            err = os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg);
            if (err) {
                printf("lvgl touch msg drop2 err = %d\n", err);
                return err;
            }
            lvgl_touch_timer_status = 1;
        } else {
            //确保定时器创建成功
            if (lv_indev_set_touch_timer_check() == false) {
                msg[0] = UI_MSG_TOUCH;
                msg[1] = (int)x;
                msg[2] = (int)y;
                msg[3] = (int)status;
                err = os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg);
                if (err) {
                    printf("lvgl touch msg drop3 err = %d\n", err);
                    return err;
                }
            }
        }

        return 0;
    }

    return 0;
}

static void lvgl_timer_event_timeout(void *p)
{
    int err;
    int msg[2];
    msg[0] = UI_MSG_TIMER_TIMEOUT;
    lvgl_timeout_id = 0;
    err =  task_queue_post_event(LVGL_TASK_NAME,  msg, ARRAY_SIZE(msg));
    if (err) {
        printf("lvgl_timer_event_timeout post_ui_msg err=%d\n", err);
    } else {
        /*printf("lvgl post timeout msg ok.\r\n");*/
        lvgl_timerout_msg_remain_flag = 1;
    }
}


static void lvgl_rpc_post_func_deal(int msg[])
{

    if (msg[1] == 0) {
        ((void (*)(void))msg[2])();
    } else if (msg[1] == 1) {
        ((void (*)(int))msg[2])(msg[3]);
    } else if (msg[1] == 2) {
        ((void (*)(int, int))msg[2])(msg[3], msg[4]);
    } else if (msg[1] == 3) {
        ((void (*)(int, int, int))msg[2])(msg[3], msg[4], msg[5]);
    } else if (msg[1] == 4) {
        ((void (*)(int, int, int, int))msg[2])(msg[3], msg[4], msg[5], msg[6]);
    } else if (msg[1] == 5) {
        ((void (*)(int, int, int, int, int))msg[2])(msg[3], msg[4], msg[5], msg[6], msg[7]);
    }

}

int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...)
{
    int err;

    if (lvgl_suspend_flag) {
        printf("lvgl_rpc_post suspended \n");
        return -1;
    }

    int msg[8];

    msg[0] = UI_MSG_RPC_FUNC;
    msg[1] = argc;
    msg[2] = (int)rpc_func;

    ASSERT(argc <= ARRAY_SIZE(msg) - 3, "%s argc too long!\n", __FUNCTION__);

    va_list argptr;
    va_start(argptr, argc);
    for (int i = 0; i < argc; i++) {
        msg[i + 3] = va_arg(argptr, int);
    }
    va_end(argptr);


_RETRY:

    err = os_taskq_post_type(LVGL_TASK_NAME, Q_USER, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_rpc_post_func err=%d\n", err);
        msleep(20);
        goto _RETRY;
    }
    return 0;
}

/**
 * @brief    lvgl 暂停函数,内部使用，非外部api接口
 * @param:   state: 1:开 0:关
 * @param:   cb: 执行成功的回调函数
 * @return:  none
 **/
void _lvgl_ui_task_suspend_resume(uint8_t state, void *cb)
{
    OS_SEM sem_wait;
    int err;
    int msg[4];
    u8 is_need_wait = 1;
    msg[0] = state ? UI_MSG_RESUME : UI_MSG_SUSPEND;
    msg[1] = (int)cb;
    msg[2] = state;
    msg[3] = (int)&sem_wait;

    if (!strcmp(os_current_task(), LVGL_TASK_NAME)) {
        //当前如果不在lvgl线程则需要等待
        is_need_wait = 0;
        msg[3] = 0;
    }


    os_sem_create(&sem_wait, 0);
    err =  os_taskq_post_type(LVGL_TASK_NAME, Q_MSG, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("lvgl_ui_start_stop post err=%d\n", err);
    } else {
        if (is_need_wait) {
            err = os_sem_pend(&sem_wait, 300);
            ASSERT(err != OS_TIMEOUT, "%s %s no response\n", __FUNCTION__, state ? "RESUME" : "SUSPEND");
        }
    }
    os_sem_del(&sem_wait, OS_DEL_ALWAYS);
}

static void _lv_ui_suspend_resume_cb(uint8_t state)
{
    if (state == 0) {
        printf("lvgl ui pause!\n");
    } else {
        printf("lvgl ui resume!\n");
        //设置全屏脏矩形
        int err;
        int msg[2];
        lv_disp_t *disp = lv_disp_get_default();
        lv_area_t a;
        lv_area_set(&a, 0, 0, lv_disp_get_hor_res(disp) - 1, lv_disp_get_ver_res(disp) - 1);
        _lv_inv_area(disp, &a);
    }
}
void lvgl_ui_suspend(void)
{
    _lvgl_ui_task_suspend_resume(0, _lv_ui_suspend_resume_cb);
}
void lvgl_ui_resume(void)
{
    _lvgl_ui_task_suspend_resume(1, _lv_ui_suspend_resume_cb);
}
u8 lvgl_ui_is_suspended(void)
{
    return lvgl_suspend_flag;
}


_WEAK_ void lvgl_v9_main_task_hook(void)
{

}

_WEAK_ void lvgl_v9_gui_init(void)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_fs_init();
    lv_tick_set_cb(&timer_get_ms);

#if LV_USE_MONKEY != 0
    /*Create pointer monkey test*/
    lv_monkey_config_t monkey_pointer_config;
    monkey_pointer_config.type = LV_INDEV_TYPE_POINTER;
    monkey_pointer_config.period_range.min = 20;
    monkey_pointer_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_pointer_config), true);

    /*Create keypad monkey test*/
    lv_monkey_config_t monkey_keypad_config;
    monkey_keypad_config.type = LV_INDEV_TYPE_KEYPAD;
    monkey_keypad_config.period_range.min = 20;
    monkey_keypad_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_keypad_config), true);

    /*Create encoder monkey test*/
    lv_monkey_config_t monkey_encode_config;
    monkey_encode_config.type = LV_INDEV_TYPE_ENCODER;
    monkey_encode_config.period_range.min = 20;
    monkey_encode_config.period_range.max = 20;
    monkey_encode_config.input_range.min = -5;
    monkey_encode_config.input_range.max = 5;
    lv_monkey_set_enable(lv_monkey_create(&monkey_encode_config), true);
#endif

    lv_demo_benchmark();
    /*lv_demo_widgets();*/
    /*lv_demo_keypad_encoder();*/
}

void lvgl_main_task(void *priv)
{
    lvgl_suspend_flag = 0;
    int msg[8] = {0};
    int ret;
    lvgl_touch_timer_status = 0;
    char lvgl_task_pend_timeout_refr_flag = 0;
    u32 time_till_next;


    lvgl_v9_gui_init();

    //刷新第一帧不需要事件触发
    time_till_next = lv_timer_handler();
    if (time_till_next != 0xFFFFFFFF) {
        sys_hi_timeout_add(NULL, lvgl_timer_event_timeout, time_till_next);
    }

    while (1) {

        ret = os_taskq_pend_timeout(msg, ARRAY_SIZE(msg), portMAX_DELAY);
        if (ret != OS_TASKQ) {
            printf("lvgl_main_task os_taskq_pend err=%d %d\n", ret, msg[0]);
            continue;
        }

        lvgl_v9_main_task_hook();

        /* if (lvgl_suspend_flag) { */
        /* printf("ui task suspended...,%d\r\n", msg[1]); */
        /* } */

        if (ret == OS_TIMEOUT) {
            if (lvgl_task_pend_timeout_refr_flag == 0) {
                /*puts("ui task pend timeout, nothing happend\r\n");*/
                continue; //如果没有模型改变消息，不需要刷新屏幕
            } else {
                puts("ui task pend timeout, module change happend\r\n");
            }
            lvgl_task_pend_timeout_refr_flag = 0;
        } else if (msg[1] == UI_MSG_TOUCH) {
            lv_set_timer_read_touch_mode((u16)msg[2], (u16)msg[3], (u8)msg[4]);
        } else if (msg[1] == UI_MSG_KEY) {
            --lvgl_key_msg_remain_cnt;
            /*printf("lvgl_key_msg_remain_cnt = %d\r\n", lvgl_key_msg_remain_cnt);*/
            struct key_event *e = (struct key_event *)&msg[2];
            lv_indev_timer_read_key(e);
        } else if (msg[1] == UI_MSG_ENCODER) {
            --lvgl_key_msg_remain_cnt;
            struct key_event *e = (struct key_event *)&msg[2];
            lv_indev_timer_read_encoder(e);
        } else if (msg[1] == UI_MSG_TIMER_TIMEOUT) {
            lvgl_timerout_msg_remain_flag = 2; //标记LVGL定时器超时消息已经消耗掉
        } else if (msg[1] == UI_MSG_SUSPEND) {
            lvgl_suspend_flag = 1;


            ((void (*)(int))msg[2])(msg[3]);
            if (msg[4]) {
                os_sem_post((OS_SEM *)msg[4]);
            }

            continue;
        } else if (msg[1] == UI_MSG_RESUME) {
            lvgl_suspend_flag = 0;

            ((void (*)(int))msg[2])(msg[3]);
            if (msg[4]) {
                os_sem_post((OS_SEM *)msg[4]);
            }
        } else if (msg[1] == UI_MSG_RPC_FUNC) {
            lvgl_rpc_post_func_deal(&msg[1]);
        }


        if (lvgl_suspend_flag) {
            time_till_next = 0xFFFFFFFF;
        } else {

            time_till_next = lv_timer_handler();
        }

        if (time_till_next == 0) {//没有时间释放CPU了
            /*printf("lvgl time_till_next == 0\r\n");*/
        }

        if (lvgl_key_msg_remain_cnt <= 0) { //如果按键事件还没消耗完，利用按键事件来调度运行，不需要超时事件来调度, LVGL任务会持续运转
            /*printf("lvgl time_till_next %d ms, lvgl_timerout_msg_remain_flag=%d\r\n", time_till_next,lvgl_timerout_msg_remain_flag); */
            if (time_till_next != 0xFFFFFFFF) {//如果LVGL内部有注册超时定时器
                if (lvgl_timerout_msg_remain_flag == 2) { //定时器超时消息已经被消耗掉
                    lvgl_timerout_msg_remain_flag = 0;
                    /* printf("lvgl add timeout msg %d\r\n",time_till_next); */
                    lvgl_timeout_id = sys_timeout_add(NULL, lvgl_timer_event_timeout, time_till_next < 3 ? 3 : time_till_next); //FIX_ME:系统未支持1ms时间超时释放cpu
                } else if (lvgl_timerout_msg_remain_flag == 1) { //超时函数已跑起，并且post消息成功，但是消息还在队列内未消耗
                    lvgl_timerout_msg_remain_flag = 0;
                    if (os_taskq_del_type(LVGL_TASK_NAME, Q_EVENT)) {
                        //puts("lvgl_timeout_event_del err\r\n"); //有可能由于超时定时器和lvgl任务不同gpu导致lvgl_timerout_msg_remain_flag执行乱序,不过不会造成不良影响
                    }
                    /* printf("lvgl add timeout msg %d\r\n", time_till_next); */
                    lvgl_timeout_id = sys_timeout_add(NULL, lvgl_timer_event_timeout, time_till_next < 3 ? 3 : time_till_next); //FIX_ME:系统未支持1ms时间超时释放cpu
                } else { //超时函数未跑起
                    if (lvgl_timeout_id) {
                        sys_timer_modify(lvgl_timeout_id, time_till_next < 3 ? 3 : time_till_next); //有可能由于超时函数已经跑起修改不成功,但是也没所谓,  主要担心修改到其他ID的注册定时器
                    }
                }
            }
        }

    }//while(1)
}

int lvgl_v9_main_task_init(void)
{
    puts("lvgl_v9_main_task_init \n\n");

    return thread_fork(LVGL_TASK_NAME, 18, 8 * 1024, 256, &lvgl_task_pid, lvgl_main_task, NULL);
}

int *lvgl_v9_get_task_pid(void)
{
    return &lvgl_task_pid;
}



