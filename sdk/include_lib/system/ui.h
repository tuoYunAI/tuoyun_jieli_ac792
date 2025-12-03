#ifndef __UI_H__
#define __UI_H__

#include "event/key_event.h"
/* -------------------------------------------------------------------------------
 * @brief                     给控件模型ID发送全局指针消息
 * @application               适用于给模型发送全局(结构体等)指针等情况
 * @param[in] msg_id:         控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in] ptr:            模型数据指针,根据控件ID实际数据类型使用
 * @param[in] len:            发送消息的长度
 * @param[in] refr_now:       是否立刻刷新UI界面,还是说等UI刷新周期自动更新,建议填写0
 * -------------------------------------------------------------------------------*/
int lvgl_module_msg_send_global_ptr(unsigned int msg_id, const void *ptr, unsigned int len, char refr_now);

/*-------------------------------------------------------------------------------
 * @brief                     给控件模型ID发送string消息
 * @application               适用于发送字符串类型的消息
 * @param[in] msg_id:         控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in] string:         模型字符串指针,根据控件ID实际数据类型使用
 * @param[in] refr_now:       是否立刻刷新UI界面,还是说等UI刷新周期自动更新,建议填写0
 * ------------------------------------------------------------------------------*/
int lvgl_module_msg_send_string(unsigned int msg_id, const char *string, char refr_now);

/* -------------------------------------------------------------------------------
 * @brief                     给控件模型ID发送value消息
 * @application               适用于发送整数类型的消息
 * @param[in] msg_id:         控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in] value:          模型数据指针,根据控件ID实际数据类型使用
 * @param[in] refr_now:       是否立刻刷新UI界面,还是说等UI刷新周期自动更新,建议填写0
 *-------------------------------------------------------------------------------*/
int lvgl_module_msg_send_value(unsigned int msg_id, unsigned int value, char refr_now);

/*-------------------------------------------------------------------------------
 * @brief                     通过模型ID，获取一个与该ID绑定的指针，填充该指针后，
                              使用lvgl_module_msg_send_ptr发送给模型，发送后无需考虑指针、内存释放的问题
 * @application               适用于发送长度大于4的字符串或者申请一个（结构体变量等）指针的情况，填充后发送
 * @param[in]     msg_id:     控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in]     len:        申请指针的大小,根据控件ID实际数据类型使用
 * @param[out/in] ptr:        申请的指针/发送时需要作为参数输入
 * @param[in]     refr_now:   是否立刻刷新UI界面,还是说等UI刷新周期自动更新,建议填写0
 *-------------------------------------------------------------------------------*/
void *lvgl_module_msg_get_ptr(unsigned int msg_id, unsigned int len);
int lvgl_module_msg_send_ptr(void *ptr, char refr_now);


/*-------------------------------------------------------------------------------
 * @brief                      创建一个模型组
 * @application                适用于用于需要同时刷新多个控件模型ID的情况
 * @param[in]  msg_group_id:   模型组的ID，可以自行定义，只需确保不同模型使用不同的ID即可
 * @param[out] group:          返回模型组指针
 *-------------------------------------------------------------------------------*/
void *lvgl_module_group_create(unsigned int msg_group_id);

/*-------------------------------------------------------------------------------
 * @brief                     给组中的控件模型ID添加全局指针消息
 * @application               适用于给模型发送全局(结构体等)指针等情况
 * @param[in] group:          模型组指针,需要将该消息添加到模型组中，同一个消息可以多次添加到不同的模型组中
 * @param[in] msg_id:         控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in] ptr:            模型数据指针,根据控件ID实际数据类型使用
 * @param[in] len:            发送消息的长度
 *-------------------------------------------------------------------------------*/
int lvgl_module_msg_group_add_global_ptr(void *group, unsigned int msg_id, const void *ptr, unsigned int len);

/*-------------------------------------------------------------------------------
 * @brief                     给组中的控件模型ID添加string消息
 * @application               适用于控件ID使用的数据类型为字符串类型
 * @param[in] group:          模型组指针,需要将该消息添加到模型组中，同一个消息可以多次添加到不同的模型组中
 * @param[in] msg_id:         控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in] string:         模型字符串指针,根据控件ID实际数据类型使用
 *-------------------------------------------------------------------------------*/
int lvgl_module_msg_group_add_string(void *group, unsigned int msg_id, const char *string);

/*-------------------------------------------------------------------------------
 * @brief                     给组中的控件模型ID添加value消息
 * @application               适用于控件ID使用的数据类型为整数类型
 * @param[in] group:          模型组指针,需要将该消息添加到模型组中，同一个消息可以多次添加到不同的模型组中
 * @param[in] msg_id:         控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in] value:          模型数据指针,根据控件ID实际数据类型使用
 *-------------------------------------------------------------------------------*/
int lvgl_module_msg_group_add_value(void *group, unsigned int msg_id, unsigned int value);

/*-------------------------------------------------------------------------------
 * @brief                     给组中的控件模型ID添加指针消息, 同样无需考虑后续指针、内存释放的问题
 * @application               适用于发送长度大于4的字符串或者申请一个（结构体变量等）指针的情况，填充后发送
 * @param[in]     group:      模型组指针,需要将该消息添加到模型组中，同一个消息可以多次添加到不同的模型组中
 * @param[in]     msg_id:     控件模型ID,gui_model_msg_id_t,由UI界面工具生成
 * @param[in]     len:        申请指针的大小,根据控件ID实际数据类型使用
 * @param[out/in] ptr:        申请的指针/发送时需要作为参数输入
 *-------------------------------------------------------------------------------*/
void *lvgl_module_msg_group_add_ptr(void *group, unsigned int msg_id, unsigned int len);

/*-------------------------------------------------------------------------------
 * @brief                     一次性刷新组内添加的模型消息
 * @param[in] group:          需要刷新的模型组指针
 * @param[in] refr_now:       是否立刻刷新UI界面,还是说等UI刷新周期自动更新,建议填写0
 *-------------------------------------------------------------------------------*/
int lvgl_module_msg_group_send(void *group, char refr_now);


/*-------------------------------------------------------------------------------
* @brief                     通过一个变量的变化来发送控件值/状态,注：变量要配合DEFINE_UI_VARIABLE_REG()使用
* @param[in] msg_id:         需要刷新的模型id
* @param[in] refr_now:       是否立刻刷新UI界面,还是说等UI刷新周期自动更新,建议填写0
*-------------------------------------------------------------------------------*/
int lvgl_module_varable_reg_send(unsigned int msg_id, char refr_now);
/*-------------------------------------------------------------------------------
* @brief                     通过模型组添加并同时发送多个变量来刷新控件值/状态,注：每个变量都要配合DEFINE_UI_VARIABLE_REG()使用
* @param[in] group:          需要刷新的模型组指针
* @param[in] msg_id:         添加到模型组的模型id
*-------------------------------------------------------------------------------*/
int lvgl_module_varable_reg_group_add(void *group, unsigned int msg_id);
int lvgl_module_varable_reg_group_send(void *group, char refr_now);

/*-------------------------------------------------------------------------------
* @brief                                    非LVGL线程通过rpc解耦函数来调用LVGL线程接口
* @param[in] void:(*rpc_func)(...)          LVGL内部接口
* @param[in] argc                           LVGL内部接口所需要的参数数量
* @param[in] ...                            LVGL内部接口所需要的参数
*-------------------------------------------------------------------------------*/
int lvgl_rpc_post_func(void (*rpc_func)(void *, ...), int argc, ...);



struct ui_module_event_handler {
    int msg_id;
    int (*onchange)(int change_event, void *data);
};


extern const struct ui_module_event_handler ui_module_event_handler_begin[], ui_module_event_handler_end[];


#define REGISTER_UI_MODULE_EVENT_HANDLER(_msg_id) \
        static const struct ui_module_event_handler ui_module_event_handler_##_msg_id \
            sec(.ui_module_event_handler) __attribute__((used)) = { \
                                .msg_id = _msg_id,





//UI页面切换注册函数,页面注册函数通常是进行app切换操作，需要在UI工具上针对指定页面开启“加载/卸载函数”
struct ui_screen_action_handler {
    int page_id;
    int (*onchange)(int action);
};

struct ui_key_event_handler {
    int page_id;
    int (*key_onchange)(struct key_event *key);
};

extern const struct ui_screen_action_handler ui_screen_action_handler_begin[], ui_screen_action_handler_end[];
extern const struct ui_key_event_handler ui_key_event_handler_begin[], ui_key_event_handler_end[];


#define REGISTER_UI_SCREEN_ACTION_HANDLER(_page_id) \
        static const struct ui_screen_action_handler ui_screen_action_handler_##_page_id \
            sec(.ui_screen_action_handler) __attribute__((used)) = { \
                                .page_id = _page_id,


#define REGISTER_UI_KEY_EVENT_HANDLER(_page_id) \
        static const struct ui_key_event_handler ui_key_event_handler_##_page_id \
            sec(.ui_key_event_handler) __attribute__((used)) = { \
                                .page_id = _page_id,



struct ui_varable_reg_t {
    void *ptr;
    unsigned int len;
    void (*get_set_value_cb)(char access);
    unsigned short msg_id;
};

extern const struct ui_varable_reg_t ui_varable_reg_tab_begin[], ui_varable_reg_tab_end[];

/*-------------------------------------------------------------------------------------------------
* @brief                              varable变量模型注册接口
* @param[in] MSG_ID                   绑定的模型ID
* @param[in] VARIABLE_TYPE            varable变量类型，只允许发送u8,u16,u32类型的变量
* @param[in] VARIABLE_NAME            varable变量名称，使用时，需要在外部用extern声明该变量,再修改它,最后调用lvgl_module_varable_reg_send接口发送
* @param[in] GET_SET_VALUE_CALLBACK   需要指定GET_SET_VALUE_CALLBACK回调,lvgl现成发送并刷新完varable变量消息后，会进到该回调，
                                      可以在该回调中做一些通知外部线程的操作，若不需要通知，则直接填NULL
*---------------------------------------------------------------------------------------------------*/
#define DEFINE_UI_VARIABLE_REG(MSG_ID,VARIABLE_TYPE,VARIABLE_NAME,GET_SET_VALUE_CALLBACK) \
        VARIABLE_TYPE VARIABLE_NAME; \
        static const struct ui_varable_reg_t varable_reg_##MSG_ID \
            sec(.ui_varable_reg_tab) __attribute__((used)) = { \
                                .msg_id = MSG_ID,\
                                .ptr =&VARIABLE_NAME,\
                                .len=sizeof(VARIABLE_TYPE),\
                                .get_set_value_cb=GET_SET_VALUE_CALLBACK,\
                                }

#endif


