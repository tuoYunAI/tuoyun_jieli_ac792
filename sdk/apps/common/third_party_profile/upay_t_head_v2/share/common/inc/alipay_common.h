#ifndef __ALIPAY_COMMON_H__
#define __ALIPAY_COMMON_H__

// #include <string.h>
#include <stdint.h>
// #include <stdbool.h>

#define PARAM_IN
#define PARAM_OUT
#define PARAM_INOUT

#define INPUT_PARAM
#define OUTPUT_PARAM
#define INOUT_PARAM


#define __alipay_depricated__
#define __readonly
#define __readwrite

#ifdef __cplusplus
#define BEGIN_C_DECL extern "C" {
#else
#define BEGIN_C_DECL
#endif

#ifdef __cplusplus
#define END_C_DECL }
#else
#define END_C_DECL
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#if 0 // TODO precheck definition conflict
typedef unsigned short     uint16_t;
typedef unsigned char      uint8_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
#endif

typedef unsigned char   		bool;
// typedef int                int32_t;

// typedef int32_t retval_e;
#define ALIPAY_RV_OK 0 // 成功，其他值为失败

typedef enum {
    RV_OK = 0,
    RV_WRITE_ERROR,
    RV_READ_ERROR,
    RV_DEL_ERROR,
    RV_NOT_FOUND,
    RV_IO_ERROR,
    RV_NOT_INITIALISED,
    RV_NETWORK_ERROR,
    RV_ECC_GENERATE_ERROR,
    RV_ECC_SHARE_ERROR,
    RV_SE_INFO_ERROR,//10
    RV_SE_RESET_ERROR,
    RV_SE_GET_STATUS_ERROR,
    RV_SE_SAVE_ERROR,
    RV_SE_GENCODE_ERROR,
    RV_BUF_TOO_SHORT,
    RV_ENCRYPTION_ERRO,
    RV_DECRYPTION_ERRO,
    RV_WRONG_TIMESTAMP,
    RV_WRONG_PARAM,
    RV_PRODECT_MODEL_ERROR,//20
    RV_NOT_INPROCESSING,
    RV_SEMAPHORE_CREATE_ERROR,
    RV_SERVER_TIMEOUT,
    RV_BINDING_DATA_FORMAT_ERROR,
    RV_PROCESSING,//25
    RV_SERVER_FAIL_ERROR,
    RV_PB_PACK_ERROR,
    RV_ID2_ERROR,
    RV_JS_ERROR,
    RV_MEM_ERROR,//30
    RV_NO_QRCODE_DETECTED, //图片中未识别出二维码
    RV_NOT_SUPPORT_QRCODE, //不支持的码类型
    RV_BASE64_ERROR,
    RV_ECC_SIGN_ERROR,
    RV_RESP_TOO_LONG,             //服务端返回的数据量太大
    RV_SERVER_DATA_ERROR,         //服务端返回的数据格式有误
    RV_DATA_ITEM_EXCEED,          //数据项数量超出限制（公交卡个数太多，超出限制）
    RV_CARD_DATA_INVALID,         //卡数据无效或卡未支持
    RV_CARD_DATA_OVERDUE,         //卡数据过期
    RV_CARD_DATA_LIMITED,         //卡生码受限  40
    RV_CARD_DATA_NEED_TO_REFRESH, //卡数据即将过期
    RV_CODE_GEN_ERROR,            //生码异常
    RV_UNSUPPORTED_CARD,
    RV_JS_ENGINE_ERROR,
    RV_FILE_OPEN_ERROR,
    RV_FILE_WRITE_ERROR,
    RV_FILE_READ_ERROR,
    RV_FILE_CLOSE_ERROR,
    RV_COMMON_ERROR,
    RV_UNKNOWN = 99
} retval_e;



/**
 * 绑定状态相关
 */
typedef int32_t binding_status_e;
#define ALIPAY_STATUS_BINDING_FAIL  0    // 绑定失败
#define ALIPAY_STATUS_UNBINDED      1    // 解绑
#define ALIPAY_STATUS_START_BINDING 2    // 开始绑定
#define ALIPAY_STATUS_BINDING_OK    0xa5 // 绑定成功(已绑定)
#define ALIPAY_STATUS_UNKNOWN       0xff // 未知状态

// #if !defined(__cplusplus)
// #ifndef bool
// typedef unsigned char bool;
// #endif
// #ifndef false
// #define false ((bool)0)
// #define true  ((bool)1)
// #endif
// #endif

#define ALIPAY_WEAK_SYMBOL __attribute__((weak))

#endif
