#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "event/net_event.h"
#include "event/bt_event.h"
#include "wifi/wifi_connect.h"
#include "net/config_network.h"
#include "key_event.h"
#include "app_msg.h"
#include "app_tone.h"
#include "app_event.h"
#include "app_protocol.h"
#include "app_wifi.h"
#include "app_ble.h"
#include "app_ota.h"
#include "audio_config.h"
#include "mic_effect.h"
#include "tone_player.h"
#include "app_audio.h"
#include "app_ui.h"

/*
#if 0
extern void printf_buf(u8 *buf, u32 len);
#define log_info(x, ...)       printf("[MAIN]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#else
//#define log_info(...)
//#define log_info_hexdump(...)
#endif
*/

#define LOG_TAG             "[MAIN]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"



/*中断列表 */
const struct irq_info irq_info_table[] = {
    //中断号   //优先级0-7   //注册的cpu(0或1)
#if CPU_CORE_NUM == 1
    { IRQ_SOFT5_IDX,      7,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      7,   1    }, //此中断强制注册到cpu1
    { -2,     			-2,   -2   },//如果加入了该行, 那么只有该行之前的中断注册到对应核, 其他所有中断强制注册到CPU0
#endif
    { -1,     -1,   -1    },
};


/*创建使用 os_task_create_static 或者task_create 接口的 静态任务堆栈*/
#define SYS_TIMER_STK_SIZE 512
#define SYS_TIMER_Q_SIZE 256
static u8 sys_timer_tcb_stk_q[sizeof(StaticTask_t) + SYS_TIMER_STK_SIZE * 4 + sizeof(struct task_queue) + SYS_TIMER_Q_SIZE] ALIGNE(4);

#define SYSTIMER_STK_SIZE 256
static u8 systimer_tcb_stk_q[sizeof(StaticTask_t) + SYSTIMER_STK_SIZE * 4] ALIGNE(4);

#define SYS_EVENT_STK_SIZE 512
static u8 sys_event_tcb_stk_q[sizeof(StaticTask_t) + SYS_EVENT_STK_SIZE * 4] ALIGNE(4);

#define APP_CORE_STK_SIZE 2048
#define APP_CORE_Q_SIZE 1024
static u8 app_core_tcb_stk_q[sizeof(StaticTask_t) + APP_CORE_STK_SIZE * 4 + sizeof(struct task_queue) + APP_CORE_Q_SIZE] ALIGNE(4);

/*创建使用  thread_fork 接口的 静态任务堆栈*/
#define WIFI_TASKLET_STK_SIZE 1400
static u8 wifi_tasklet_tcb_stk_q[sizeof(struct thread_parm) + WIFI_TASKLET_STK_SIZE * 4] ALIGNE(4);

#define WIFI_CMDQ_STK_SIZE 300
static u8 wifi_cmdq_tcb_stk_q[sizeof(struct thread_parm) + WIFI_CMDQ_STK_SIZE * 4] ALIGNE(4);

#define WIFI_MLME_STK_SIZE 900
static u8 wifi_mlme_tcb_stk_q[sizeof(struct thread_parm) + WIFI_MLME_STK_SIZE * 4] ALIGNE(4);

#define WIFI_RX_STK_SIZE 256
static u8 wifi_rx_tcb_stk_q[sizeof(struct thread_parm) + WIFI_RX_STK_SIZE * 4] ALIGNE(4);

#define PROTOCOL_SESSION_STK_SIZE 2048
static u8 protocol_session_tcb_stk_q[sizeof(struct thread_parm) + PROTOCOL_SESSION_STK_SIZE * 4] ALIGNE(4);

#define PROTOCOL_AUDIO_STK_SIZE 1024
static u8 protocol_audio_tcb_stk_q[sizeof(struct thread_parm) + PROTOCOL_AUDIO_STK_SIZE * 4] ALIGNE(4);
static u8 protocol_audio_demo_tcb_stk_q[sizeof(struct thread_parm) + PROTOCOL_AUDIO_STK_SIZE * 4] ALIGNE(4);

/*任务列表 */
const struct task_info task_info_table[] = {
    {"thread_fork_kill",    25,      256,   0     },
    {"app_core",            15,     APP_CORE_STK_SIZE,	  APP_CORE_Q_SIZE,		 app_core_tcb_stk_q },
    {"sys_event",           29,     SYS_EVENT_STK_SIZE,	   0, 					 sys_event_tcb_stk_q },
    {"systimer",            14,     SYSTIMER_STK_SIZE, 	   0,					 systimer_tcb_stk_q },
    {"sys_timer",            9,     SYS_TIMER_STK_SIZE,	  SYS_TIMER_Q_SIZE,		 sys_timer_tcb_stk_q },
    {"dlog",                 1,      256,   128   },


    /**
     * OTA START
     */
    {"update",              21,      512,   32    },
    {"dw_update",           21,      512,   32    },
    /**
     * OTA End
     */
    { "uac_play",            26,     512,     32 },
    { "uac_record",          26,     512,      0 },
    /**
     * AI SESSION START
     */
    {"protocol_mqtt_proc",      25,     PROTOCOL_SESSION_STK_SIZE,   0,		 protocol_session_tcb_stk_q	 },
    {"protocol_audio",          28,     PROTOCOL_AUDIO_STK_SIZE,     0,		 protocol_audio_tcb_stk_q	 },
    {"protocol_audio_demo",     16,     PROTOCOL_AUDIO_STK_SIZE,     0,		 protocol_audio_demo_tcb_stk_q	 },
    /**
     * AI SESSION END
     */
#ifdef CONFIG_MEDIA_ENABLE
#if TCFG_ENC_AMR_16K_ENABLE //16k amr enc
    { "jlstream_",           25,    3072,      0 },
#else
    { "jlstream_",           26,     768,      0 },
#endif
    { "jlstream",            26,     768,    128 },
    { "a2dp_dec",            24,     768,      0 },
    /* file dec任务不打断jlstream任务运行,故优先级低于jlstream */
#if TCFG_DEC_AMR_16K_ENABLE //16k amr dec
    { "file_dec",            24,    1536,      0 },
#else
    { "file_dec",            28,     768,      0 },
#endif
    { "file_cache",          26,     512,      0 },
    { "write_file",          25,     512,      0 },
    { "vir_data_tx",         25,     768,      0 },
    /* 麦克风音效任务优先级要高 */
    { "mic_effect",          26,     768,      0 },
    /* 为了防止dac buf太大，通话一开始一直解码，导致编码输入数据需要很大的缓存，这里提高编码的优先级 */
    { "audio_enc",           26,     768,    128 },
    { "aec",                 26,     768,    128 },
    { "aec_dbg",             13,     512,    128 },
    { "aud_capture",         24,     512,    256 },
    { "dac",                 26,     256,    128 },
    { "spec",                23,     512,      0 },
    { "spec_adv",            23,     512,      0 },
    { "kws",                  3,     256,     64 },
    { "smart_voice",         11,     512,    128 },
    { "audio_vad",           11,     768,      0 },
    { "key_tone",            25,     256,     32 },
    { "cvp_ref",             24,     256,    128 },
    { "CVP_RefTask",         26,     256,    128 },
    { "audio_transmitter",   20,     512,    128 },
#endif	 
#ifdef CONFIG_WIFI_ENABLE
    {"iperf_test",          15,     1024,   0     },
    {"tcpip_thread",        16,      800,   0     },
    {"tasklet",             10,     WIFI_TASKLET_STK_SIZE,   0,		 wifi_tasklet_tcb_stk_q	 },//通过调节任务优先级平衡WIFI收发占据总CPU的比重
    {"RtmpMlmeTask",        17,     WIFI_MLME_STK_SIZE,  	 0, 	 wifi_mlme_tcb_stk_q	 },
    {"RtmpCmdQTask",        17,     WIFI_CMDQ_STK_SIZE,   	 0,  	 wifi_cmdq_tcb_stk_q	 },
    {"wl_rx_irq_thread",    16,     WIFI_RX_STK_SIZE,    	 0,  	 wifi_rx_tcb_stk_q  	 },

#endif
#ifdef CONFIG_BT_ENABLE
    {"btencry",             14,      512,   128   },
    {"btctrler",            19,      512,   384   },
    {"btstack",             18,      768,   384   },
#endif

    {0, 0},
};

void enter_mode_network_interrupted(void){

    play_tone_file(get_tone_files()->net_disc);
    log_info("enter_idle_mode");
    ui_set_status_text("网络断开");
}
void enter_mode_activation(void){

    log_info("enter_mode_activation");
    play_tone_file(get_tone_files()->activating);
    ui_set_status_text("设备正在激活...");
}
void enter_mode_network_configuring(void){
    play_tone_file(get_tone_files()->net_cfg_enter);
    log_info("enter_mode_network_configuring");
    ui_set_status_text("进入配网模式");
}

void enter_mode_network_config_success(void){
    play_tone_file(get_tone_files()->net_cfg_succ);
    log_info("enter_mode_network_config_success");
    ui_set_status_text("网络配置成功，重启中...");
}

void enter_mode_regitering(void){

    play_tone_file(get_tone_files()->net_connected);
    ui_set_status_text("正在注册...");
}

void enter_mode_regiter_result(int result){
    if(result == 0){
        ui_set_status_text("注册失败"); 
        play_tone_file(get_tone_files()->register_fail);
    }else{
        ui_set_status_text("注册成功");
        play_tone_file(get_tone_files()->register_success);
        //jl_kws_speech_recognition_open();
        //os_time_dly(100);
    }
    
}

void enter_mode_conntecting_server(void){
    log_info("enter_mode_connteting_server");
    ui_set_status_text("正在连接服务");
}

void enter_mode_idle(void){
    log_info("enter_idle_mode");
    ui_set_status_text("待命");
    ui_set_emotion_text("");
    ui_set_content_text("");
    //jl_kws_speech_recognition_start();
    //log_info("@@@@@@@@@@@@: jl_kws_speech_recognition_start ---- start");
}
void enter_mode_ota(void){
    log_info("enter_mode_ota");
    ui_set_status_text("正在升级中...");
}

void enter_mode_ota_end(int result ){
    if(result == 0){
        log_info("enter_mode_ota_end: success");
        ui_set_status_text("升级成功，重启中..."); 
    }else{
        log_info("enter_mode_ota_end: failed"); 
        ui_set_status_text("升级失败");
    }
    
}

void enter_mode_renewal_overduep(void){
    play_tone_file(get_tone_files()->renewal_overdue);
    ui_set_status_text("您还未购买服务或服务已过期");
}

void enter_mode_dialog_initiating(void){
    ui_set_status_text("正在呼叫...");
    //jl_kws_speech_recognition_stop();

    //log_info("@@@@@@@@@@@@: jl_kws_speech_recognition_stop ---- stop");
}


void enter_mode_dialog_listening(void){
    ui_set_status_text("聆听中");
}

void enter_mode_dialog_speaking(void){
    ui_set_status_text("讲话中");
}


void start_network(void)
{
    
    thread_fork("wifi_init_task", 25, 4000, 0, 0, start_wifi_network, NULL);
    u16 id = sys_timer_add_to_task("sys_timer", NULL, wifi_status, 20 * 1000);
    return;
}

void proc_register_device(void)
{
    enter_mode_regitering();
    char mac[6] = {0};
    int ret = get_wifi_mac(mac);
    if (ret != 0) {
        log_info("get_wifi_mac err %s\n", mac);
        return;
    }
    char mac_str[18] = {0};
    // 将MAC地址转换为字符串格式
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0] & 0xff, mac[1] & 0xff, mac[2] & 0xff,
             mac[3] & 0xff, mac[4] & 0xff, mac[5] & 0xff);

    ret = register_device(mac_str, PRODUCT_VENDOR_UID);
    enter_mode_regiter_result(ret == 0);
}

void start_register_device()
{
    thread_fork("system_reg_device_task", 25, 8000, 0, 0, proc_register_device, NULL);
    return;
}



static int poweron_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) {
        //app_mode_change(APP_MODE_BT);
    }

    return 0;
}


/*
 * 应用程序主函数
 */
void app_main(void)
{
    log_info("\n\n\n ------------tuoyun smart robot run %s-------------\n\n\n", __TIME__);

    int ret = play_tone_file_callback(get_tone_files()->power_on, NULL, poweron_tone_play_end_callback);
    if (ret) {
        poweron_tone_play_end_callback(NULL, STREAM_EVENT_STOP);
    }
    

    start_network();


    //int lvgl_main_task_init(void);
    //lvgl_main_task_init();
    //int lvgl_main_task_init(void);
    //lvgl_main_task_init();
    return;
}


static int ble_inited = 0;
static int net_wifi_event_handler(void *evt)
{
    struct net_event *event = (struct net_event *)evt;

    switch (event->event) {
    case NET_EVENT_CONNECTED:
        log_info("net_wifi_event_handler: NET_EVENT_CONNECTED, note: %s", event->arg);
        start_register_device();
        void start_debug_server();
        start_debug_server();
        break;
    case NET_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        log_info("net_wifi_event_handler: NET_CONNECT_TIMEOUT_NOT_FOUND_SSID, note: %s", event->arg);
        
        if (!ble_inited) {
            ble_inited = 1;
            bt_ble_module_init();
            enter_config_network_state();
            enter_mode_network_configuring();
        }/*
        没有网络时, 会进入配网模式, 不需要提示网络断开
        else {
            enter_mode_network_interrupted();
        }*/
        break;
    default:
        break;
    }

    return FALSE;
}


void app_user_event_handler(struct app_event *event)
{
    switch (event->event) {
    case APP_EVENT_ACTIVATION:
        log_info("app_user_event_handler: APP_EVENT_ACTIVATION");
        
        ble_inited = 1;
        bt_ble_module_init();
        enter_config_network_state();
        enter_mode_activation();
        break;
    case APP_EVENT_OTA_START:
        log_info("app_user_event_handler: APP_EVENT_OTA_START");
        enter_mode_ota();
        break;
    case APP_EVENT_OTA_END:
        log_info("app_user_event_handler: APP_EVENT_OTA_END");
        enter_mode_ota_end((int)(event->arg));
        break;    
    case APP_EVENT_WIFI_CFG_FINISH:
        log_info("app_user_event_handler: APP_EVENT_WIFI_CFG_FINISH");
        enter_mode_network_config_success();
        break;
    case APP_EVENT_MQTT_CONNECTION_PARAM:
        log_info("app_user_event_handler: APP_EVENT_MQTT_CONNECTION_PARAM");
        enter_mode_conntecting_server();
        start_protocol(event->arg);
        break;
    case APP_EVENT_MQTT_CONNECTION_STATUS:
        if ((int)(event->arg) == MQTT_STATUS_ENSTABLISHED) {
            log_info("app_user_event_handler: APP_EVENT_MQTT_CONNECTION_STATUS: ESTABLISHED");
            enter_mode_idle();
        } else {
            log_info("app_user_event_handler: APP_EVENT_MQTT_CONNECTION_STATUS: INTERRUPTED");
            enter_mode_conntecting_server();
        }
        break;
    case APP_EVENT_DATA:
        log_info("app_user_event_handler: APP_EVENT_DATA");
        break;    
    case APP_EVENT_WAKEUP_WORD_DETECTED:
        log_info("app_user_event_handler: APP_EVENT_WAKEUP_WORD_DETECTED: %s", (char*)event->arg);
        init_call((char*)event->arg);
        break;      
    default:
        break;
    }
}





void app_protocol_event_handler(struct app_event *event)
{
    switch (event->event) {
    case APP_EVENT_SERVER_NOTIFY:
        message_notify_event_t* notify = event->arg;
        log_info("app_user_event_handler: APP_EVENT_SERVER_NOTIFY");
        if (notify) {
            log_info("notify event: %d, status: %s, message: %s, emotion: %s\n",
                     notify->event, notify->status, notify->message, notify->emotion);
            if (notify->event == CTRL_EVENT_EXPIRE) {
                enter_mode_renewal_overduep();   
            }     
        }
    
        break;  
    case APP_EVENT_CALL_ESTABLISHED:
        log_info("app_user_event_handler: APP_EVENT_CALL_ESTABLISHED");
        tuoyun_asr_recorder_close();
        dialog_audio_init((media_parameter_ptr)event->arg);
        enter_mode_dialog_initiating();
        break;
    case APP_EVENT_CALL_REJECTED:
        log_info("app_user_event_handler: APP_EVENT_CALL_REJECTED");
        enter_mode_idle();
        break;
    case APP_EVENT_CALL_NO_ANSWER:
        log_info("app_user_event_handler: APP_EVENT_CALL_NO_ANSWER");  
        enter_mode_idle();
        break;  
    case APP_EVENT_CALL_UPDATED:
        log_info("app_user_event_handler: APP_EVENT_CALL_UPDATED");
        message_session_event_t* session_event = event->arg;
        if (session_event) {
            log_info("session event: %d, status: %d, text: %s\n",
                     session_event->event, session_event->status, session_event->text?session_event->text:"");
        }
        switch (session_event->event)
        {
        case CTRL_EVENT_ALERT:
        
            /* code */
            break;
        case CTRL_EVENT_ACTIVATE:
            /* code */
            break;
        case CTRL_EVENT_EXPIRE:
            /* code */
            break;
        case CTRL_EVENT_SHOW_TEXT:
            log_info("Received text to speak: %s\n", session_event->text?session_event->text:"NULL");
            if(session_event->text) {
                ui_set_content_text(session_event->text);
            }
            /* code */
            break;
        case CTRL_EVENT_SPEAKER:
            if (session_event->status == WORKING_STATUS_TEXT) {
                log_info("Received text to speak: %s\n", session_event->text?session_event->text:"NULL");
                dialog_proc_speak_status(WORKING_STATUS_START);
                if(session_event->text) {
                    ui_set_content_text(session_event->text);
                }
                return;
            }
            dialog_proc_speak_status(session_event->status);
            if (session_event->status == WORKING_STATUS_START) {
                enter_mode_dialog_speaking();
            } else if (session_event->status == WORKING_STATUS_STOP) {
                enter_mode_dialog_listening();
            }
            
            if (strnlen(session_event->emotion, sizeof(session_event->emotion)) > 0) {
                ui_set_emotion_text(session_event->emotion);
            }
            /* code */
            break;
        case CTRL_EVENT_LISTEN:
            if (session_event->status == WORKING_STATUS_START) {
                enter_mode_dialog_listening();
            } else if (session_event->status == WORKING_STATUS_STOP) {
                enter_mode_dialog_speaking();
            }
            break;
        default:
            break;
        }
        
        break;
    case APP_EVENT_CALL_SERVER_TERMINATED:
        log_info("app_user_event_handler: APP_EVENT_CALL_SERVER_TERMINATED");
        dialog_audio_close();
        tuoyun_asr_recorder_open();
        enter_mode_idle();
        break;
    case APP_EVENT_CALL_TERMINATE_ACK:
        log_info("app_user_event_handler: APP_EVENT_CALL_TERMINATE_ACK");
        
        break;  
    case APP_EVENT_AUDIO_ENC_NOTIFY:
        log_info("app_user_event_handler: APP_EVENT_AUDIO_ENC_NOTIFY");
        char buf[120] = {0};
        audio_stream_packet_t packet = {
            .payload_len = 120,
            .payload = buf
        };
        
        memcpy(packet.payload, event->arg, 120);
        send_audio(&packet);
        break;       
    default:
        break;
    }

    //test_print_session_state();
    //test_print_traffic_state();
}

void app_audio_event_handler(struct app_event *event)
{
    switch (event->event) {
    case APP_EVENT_AUDIO_PLAY_END_NOTIFY:
        send_start_listening();
        break;  
    default:
        break;
    }
}

int vol = 30;
static void app_default_key_click(struct key_event *key)
{
    log_info("+++++++key click: %d\n", key->value);
    switch (key->value) {
    case KEY_OK:
        
        //init_call("你好小智");
        break;
    case KEY_VOLUME_DEC:
    case KEY_UP:
        int tuoyun_audio_player_get_volume(void);
         
        int v = tuoyun_audio_player_get_volume();
        log_info("current volume: %d\n", v);
        break;
    case KEY_PREV:
        
        break;
    case KEY_VOLUME_INC:
    case KEY_DOWN:  
        vol += 10;
        if (vol > 100) {
            vol = 100;
        }
        void tuoyun_audio_player_set_volume(s16 volume);
        tuoyun_audio_player_set_volume((s16)vol);
        log_info("set volume: %d\n", vol);
        break;
    case KEY_NEXT:
        
        break;
    case KEY_MODE:
        
        break;
    case KEY_MENU:
    case KEY_POWER:
        break;
    default:
        break;
    }
}


static void app_default_key_long(struct key_event *key)
{
    log_info("+++++++long key click: %d\n", key->value);
    switch (key->value) {
    case KEY_OK:

        break;
    case KEY_DOWN:
        /* app_mode_change(APP_MODE_BT); */
        /* config_network_start(); */
        break;

    case KEY_UP:
        /* app_mode_change(APP_MODE_BT); */
        /* config_network_start(); */
        break;



    default:
        break;
    }
}

static void app_default_key_event_handler(struct key_event *key)
{
    //log_info("key event: type=%d, action=%d, value=%d\n", key->type, key->action, key->value);
    switch (key->action) {
    case KEY_EVENT_CLICK:
        app_default_key_click(key);
        break;
    case KEY_EVENT_LONG:
        app_default_key_long(key);
        break;
    case KEY_EVENT_HOLD:
        break;
    default:
        break;
    }
}


/*
 * 默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 */
void app_default_event_handler(struct sys_event *event)
{
    //log_info("app_default_event_handler: type=%d, from=%d\n", event->type, event->from);
    const struct app_event_handler *handler;

    for_each_app_event_handler(handler) {
        if (event->type == handler->event && handler->from == event->from) {
            handler->handler(event->payload);
        }
    }

    switch (event->type) {
    case SYS_KEY_EVENT:
        app_default_key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        //device_event_handler(event);
        break;
    case SYS_NET_EVENT:
#ifdef CONFIG_NET_ENABLE
        //net_video_event_hander((void *)event->payload);
#endif
        break;
    case SYS_BT_EVENT:
        break;
    case SYS_APP_EVENT:
        break;    
    default:
        ASSERT(0, "unknow event type: %s\n", __func__);
        break;
    }
}


void app_ui_key_event_handler(struct app_event *event)
{
    struct key_event *key = (struct key_event *)event->arg;
    log_info("app_ui_key_event_handler: type=%d, action=%d, value=%d\n", key->type, key->action, key->value);
    app_default_key_event_handler(key);
}


REGISTER_APP_EVENT_HANDLER(network_wifi_event) = {
    .event      = SYS_NET_EVENT,
    .from       = NET_EVENT_FROM_USER,
    .handler    = net_wifi_event_handler,
};

REGISTER_APP_EVENT_HANDLER(bt_connction_status_event) = {
    .event      = SYS_BT_EVENT,
    .from       = BT_EVENT_FROM_CON,
    .handler    = bt_connction_status_event_handler,
};


REGISTER_APP_EVENT_HANDLER(app_key_event) = {
    .event      = SYS_APP_EVENT,
    .from       = APP_EVENT_FROM_UI,
    .handler    = app_ui_key_event_handler,
};


REGISTER_APP_EVENT_HANDLER(app_user_event) = {
    .event      = SYS_APP_EVENT,
    .from       = APP_EVENT_FROM_USER,
    .handler    = app_user_event_handler,
};

REGISTER_APP_EVENT_HANDLER(app_protocol_event) = {
    .event      = SYS_APP_EVENT,
    .from       = APP_EVENT_FROM_PROTOCOL,
    .handler    = app_protocol_event_handler,
};


REGISTER_APP_EVENT_HANDLER(app_audio_event) = {
    .event      = SYS_APP_EVENT,
    .from       = APP_EVENT_FROM_AUDIO,
    .handler    = app_audio_event_handler,
};
