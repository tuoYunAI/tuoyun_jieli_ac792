/********************************测试例程说明************************************
 *  功能说明：
 *      通过HTTP协议，先获取服务器上最新的固件版本信息（版本信息中一般包含有版本号、固件下载地址URL等信息），
 * 通过获取到的最新版本号和当前系统版本进行比较，如果已经是最新版本则退出升级，否则提取最新固件URL，获取最新固件进行升级。
 * 通过该方法可以实现设备开机版本自动检测、批量设备自检升级。
 */

#include "system/includes.h"
#include "os/os_api.h"
#include "fs/fs.h"
#include <stdlib.h>
#include "net_update.h"
#include "app_config.h"
#include "wifi/wifi_connect.h"
#include "http/http_cli.h"
#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "syscfg_id.h"
#include "app_event.h"
#include "app_ota.h"
#include "protocol.h"


#define LOG_TAG             "[OTA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"



#define DEFAULT_RECV_BUF_SIZE   (4*1024)   //数据缓存BUF大小
#define MAX_RECONNECT_CNT 10               //最大重连次数

struct download_hdl {
    char *url;                    //下载链接
    u32 file_size;                //需要下载的文件大小
    u8 *recv_buf;                 //数据缓存BUF
    u32 recv_buf_size;            //数据缓存BUF大小
    u32 download_len;             //已下载的数据长度
    u32 reconnect_cnt;             //重连次数
    httpcli_ctx ctx;              //http请求参数
    const struct net_download_ops *download_ops; //http操作集
};

struct download_parm {
    char *url;  //下载链接
};

enum {
    OTA_NO_ERROR = 0,
    OTA_GET_SAME_VERSION = -80,
    OTA_GET_VERSION_FAILURE = -81,
};

//设备版本描述信息长度
#define VERSION_INTRO_LEN 64

//设备版本号长度
#define VERSION_LEN 16

/** 保存固件升级的URL*/
#define OTA_URL_LEN 512
static char http_ota_url[OTA_URL_LEN] = {0};

/** 获取版本信息接收buf大小*/
#define HTTP_RECV_BUF_SIZE 1 * 1024

int http_create_download_task(void);

char* json_get_string(struct json_object *jobj, char* key)
{
    if (NULL == jobj || NULL == key) {
        return "";
    }

    json_object *obj = json_object_object_get(jobj, key);
    if(NULL == obj){
        return "";
    }
    return json_object_get_string(obj);
    
}


int register_device(char *mac, char* vendor_uid)
{
    int ret = 0;
    int err = -1;
    http_body_obj http_body_buf;
    json_object *parse = NULL;

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        log_info("calloc faile\n");
        return err;
    }
    
    memset(&http_body_buf, 0x0, sizeof(http_body_obj));

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = HTTP_RECV_BUF_SIZE;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *) malloc(http_body_buf.buf_len * http_body_buf.buf_count);
    if (!http_body_buf.p) {
        log_info("malloc faile\n");
        free(ctx);
        return err;
    }
    char reg_url[256] = {0};
    strcpy(reg_url, REG_OTA_URL);
    strcat(reg_url, vendor_uid);
    ctx->url = reg_url;
    ctx->data_format = "application/json";
    ctx->timeout_millsec = 5000;
    ctx->priv = &http_body_buf;
    ctx->connection = "close";

    ctx->post_data = (char *)malloc(1024);
    if (!ctx->post_data) {
        log_info("malloc post_data faile\n");
        free(http_body_buf.p);
        free(ctx);
        return err;
    }

    // 使用json-c库创建JSON对象
    json_object *root = json_object_new_object();
    json_object *app_obj = json_object_new_object();
    json_object *board_obj = json_object_new_object();
    if (!root || !app_obj || !board_obj) {
        log_info("fail to create json root object\n");
        if (root) json_object_put(root);
        if (app_obj) json_object_put(app_obj);
        if (board_obj) json_object_put(board_obj);
        return -1;
    }

    
    json_object_object_add(root, "version", json_object_new_int(OTA_PROTOCOL_VERSION));
    json_object_object_add(root, "type", json_object_new_string(PRODUCT_TYPE));
    json_object_object_add(root, "language", json_object_new_string(APP_LANGUAGE));
    json_object_object_add(root, "mac", json_object_new_string(mac));

    // 添加application对象字段
    json_object_object_add(app_obj, "name", json_object_new_string(FIRMWARE_NAME));
    json_object_object_add(app_obj, "type", json_object_new_string(FIRMWARE_TYPE));
    json_object_object_add(app_obj, "version", json_object_new_string(FIRMWARE_VERSION));
    json_object_object_add(app_obj, "compile_time", json_object_new_string(__DATE__ __TIME__));
    json_object_object_add(root, "application", app_obj);
    
    // 添加board对象字段
    json_object_object_add(board_obj, "type", json_object_new_string(BOARD_TYPE));
    json_object_object_add(board_obj, "name", json_object_new_string(BOARD_NAME));
    json_object_object_add(board_obj, "mac", json_object_new_string(mac));
    json_object_object_add(root, "board", board_obj);

    // 获取完整的JSON字符串
    const char *json_string = json_object_to_json_string(root);
    if (json_string) {
        memset(ctx->post_data, 0, 1024);
        strncpy(ctx->post_data, json_string, 1023);      
        ctx->data_len = strlen(ctx->post_data);
        log_info("total length of the requirement: %d", strlen(ctx->post_data));  
    }
    // 释放JSON对象
    json_object_put(root); 

    ret = httpcli_post(ctx);
    do{

        if (ret != HERROR_OK) {
            log_info("register_device : httpcli_post fail %d\n", ret);
            break;
        } 
        

        if (http_body_buf.recv_len > 0) {
            /** 对从服务器收到的json格式的版本信息进行解析，获取其版本号，版本描述信息和升级固件的URL信息*/
            parse = json_tokener_parse(http_body_buf.p);

            if (parse != NULL) {
                //log_info("register_device : json_tokener_parse : %s \n", json_object_to_json_string(parse));
                json_object *activation = json_object_object_get(parse, "activation");
                if (activation != NULL) {
                    log_info("required to activate");
                    struct app_event event =
                    {
                        .event = APP_EVENT_ACTIVATION,
                        .arg = NULL,
                    };
                    app_event_notify(APP_EVENT_FROM_USER, &event);
                    break;
                }

                json_object *firmware_info = json_object_object_get(parse, "firmware");

                if (firmware_info != NULL) {
                    /** 获取最新版本号*/
                    char ota_version[VERSION_LEN] = {0};
                    strcpy(ota_version, json_get_string(firmware_info, "version"));
                    strcpy(http_ota_url, json_get_string(firmware_info, "url"));

                    if(strcmp(ota_version, FIRMWARE_VERSION) > 0 && strlen(http_ota_url) > 0){
                        log_info("try to upgrade from %s to %s.\n", FIRMWARE_VERSION, ota_version);
                         http_create_download_task();
                        break;
                    }
                }
                
                json_object *mqtt_info = json_object_object_get(parse, "mqtt");
                if (mqtt_info != NULL) {
                    mqtt_connection_parameter_ptr mqtt_param = malloc(sizeof(mqtt_connection_parameter_t));
                    if (mqtt_param == NULL){
                        log_info("failed to alloc memo");
                        break;   
                    }
                    memset(mqtt_param, 0, sizeof(mqtt_connection_parameter_t));
                    strcpy(mqtt_param->end_point, json_get_string(mqtt_info, "endpoint"));
                    strcpy(mqtt_param->user_name, json_get_string(mqtt_info, "username"));
                    strcpy(mqtt_param->password, json_get_string(mqtt_info, "password"));
                    strcpy(mqtt_param->client_id, json_get_string(mqtt_info, "client_id"));
                    strcpy(mqtt_param->topic_pub, json_get_string(mqtt_info, "publish_topic"));
                    strcpy(mqtt_param->topic_sub, json_get_string(mqtt_info, "subscribe_topic"));

                    char *port_str = strchr(mqtt_param->end_point, ':');
                    

    
                    if (port_str != NULL) {
                        *port_str++ = '\0';
                        mqtt_param->port = atoi(port_str);
                    }else{
                        mqtt_param->port = 1883;
                    }   
                    struct app_event event =
                    {
                        .event = APP_EVENT_MQTT_CONNECTION_PARAM,
                        .arg = mqtt_param,
                    };
                    app_event_notify(APP_EVENT_FROM_USER, &event);
                }
                   
                
                
                json_object *server_time = json_object_object_get(parse, "server_time");
                if (server_time != NULL) {
                    //u64 time_stamp = json_object_get_int64(server_time);
                    //log_info("server_time:%u\n", time_stamp);
                    //sys_time_set(time_stamp);
                }
                
                /** 获取成功*/
                err = 0;
            }
        }

    }while (false);

    httpcli_close(ctx);

    if (http_body_buf.p) {
        free(http_body_buf.p);
        http_body_buf.p = NULL;
    }

    if (ctx->post_data) {
        free(ctx->post_data);
        ctx->post_data = NULL;
    }   

    if (ctx) {
        free(ctx);
        ctx = NULL;
    }

    if (parse) {
        json_object_put(parse);
        parse = NULL;
    }

    return err;
}


//http回调函数
static int http_user_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    struct download_hdl *hdl = (struct download_hdl *)priv;

    if (status == HTTPIN_HEADER) {
        //第一次连接时，记录文件总长度
        if (hdl->ctx.lowRange == 0) {
            hdl->file_size = hdl->ctx.content_length;
        }

        log_e("http_need_download_file=%d KB", hdl->file_size / 1024);
    }
    return 0;
}

static void download_task(void *priv)
{
    log_info("download_task\n");
    struct download_hdl *hdl = priv;
    void *update_fd = NULL;
    int ret = 0;
    int err = 0;
    char sock_err = 0;

    update_fd = net_fopen(CONFIG_UPGRADE_OTA_FILE_NAME, "w");
    if (!update_fd) {
        log_e("open update_fd error\n");
        goto _out_;
    }

_reconnect_:
    log_info("download from %s\n", hdl->url);

    //发起连接请求，建立socket连接
    ret = hdl->download_ops->init(&hdl->ctx);

    if (ret != HERROR_OK) {
        if (hdl->ctx.req_exit_flag == 0) {
            if (hdl->reconnect_cnt < MAX_RECONNECT_CNT) {
                hdl->reconnect_cnt++;
                goto _reconnect_;
            } else {
                log_e("download reconnect upto max count.\n");
                goto _out_;
            }
        }
    }

    while (hdl->ctx.req_exit_flag == 0) {
        ret = hdl->download_ops->read(&hdl->ctx, (char *)hdl->recv_buf, hdl->recv_buf_size);//最大接收为recv_buf_size
        if (ret <  0) {  //读取数据失败
            sock_err = 1;
            goto _out_;
        } else {
            hdl->download_len += ret;
            err = net_fwrite(update_fd, hdl->recv_buf, ret, 0);
            if (err != ret) {
                goto _out_;
            }
        }

        if (hdl->download_len >= hdl->file_size) {
            goto _finish_;
        }
    }

_finish_:
    log_info("download success.\n");

    net_fclose(update_fd, sock_err);
    system_soft_reset();

_out_:
    log_info("download err.\n");
    //关闭网络连接
    hdl->download_ops->close(&hdl->ctx);

    net_fclose(update_fd, sock_err);
    free(hdl->url);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);
}

int http_create_download_task(void)
{
    log_info("http_create_download_task\n");
    struct download_parm parm = {0};
    struct download_hdl *hdl = NULL;

    parm.url = http_ota_url;

    //申请堆内存
    hdl = (struct download_hdl *)calloc(1, sizeof(struct download_hdl));

    if (hdl == NULL) {
        return -ENOMEM;
    }

    hdl->download_ops = &http_ops;
    hdl->recv_buf_size = DEFAULT_RECV_BUF_SIZE;

    hdl->url = (char *)calloc(1, strlen(parm.url) + 1);

    if (hdl->url == NULL) {
        return -ENOMEM;
    }

    hdl->recv_buf = (u8 *)calloc(1, hdl->recv_buf_size);

    if (hdl->recv_buf == NULL) {
        return -ENOMEM;
    }

    strcpy(hdl->url, parm.url);

    //http请求参数赋值
    hdl->ctx.url = hdl->url;
    hdl->ctx.cb = http_user_cb;
    hdl->ctx.priv = hdl;
    hdl->ctx.connection = "close";

    //创建下载线程
    return thread_fork("download_task", 20,  2 * 1024, 0, NULL, download_task, hdl);
}


