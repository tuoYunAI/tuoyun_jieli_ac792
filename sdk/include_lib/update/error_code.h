/**
 * @file  error_code.h
 * @brief 升级错误码
 *
 * @copyright Copyright(C),2010-2024, 杰理科技 https://www.zh-jieli.com/
 *
 */
#ifndef _ERROR_CODE_H
#define _ERROR_CODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int update_err_t;

/**
 * @brief common error code
 */
#define UPDATE_ERR_NONE                                             0
#define UPDATE_ERR_INVALID_PARM                                     -1
#define UPDATE_ERR_MALLOC_FAILED                                    -2
#define UPDATE_ERR_UNEXPECTED_ERROR                                 -3

/**
 * @brief common update error code
 */
#define UPDATE_COMMON_ERR_BASE                                      100
#define UPDATE_ERR_PEER_FILE_VERIFY_FAILED                         -(UPDATE_COMMON_ERR_BASE + 1)
#define UPDATE_ERR_HEAD_CRC_ERROR                                  -(UPDATE_COMMON_ERR_BASE + 2)
#define UPDATE_ERR_NOT_ENOUGH_SPACE                                -(UPDATE_COMMON_ERR_BASE + 3)
#define UPDATE_ERR_BOOT_INFO_NOT_MATCH                             -(UPDATE_COMMON_ERR_BASE + 4)
#define UPDATE_ERR_BOOT_AREA_VERIFY_FAILED                         -(UPDATE_COMMON_ERR_BASE + 5)
#define UPDATE_ERR_FILE_DATA_CRC_ERROR                             -(UPDATE_COMMON_ERR_BASE + 6)
#define UPDATE_ERR_BOOT_INFO_UPDATE_ERROR                          -(UPDATE_COMMON_ERR_BASE + 7)

/**
 * @brief ota error code
 */
#define OTA_UPDATE_ERR_BASE                                         110
#define UPDATE_ERR_INIT_FAILED                                      -(OTA_UPDATE_ERR_BASE + 1)
#define UPDATE_ERR_TASK_START_FAILED                                -(OTA_UPDATE_ERR_BASE + 2)

/**
 * @brief sd update  error code
 */
#define SD_UPDATE_ERR_BASE                                          120
#define UPDATE_RESULT_ERR_NONE                                      UPDATE_ERR_NONE
#define UPDATE_RESULT_RESOURCE_LIMIT                               	-(SD_UPDATE_ERR_BASE + 1)
#define UPDATE_RESULT_READ_REMOTE_FILE_ERR                          -(SD_UPDATE_ERR_BASE + 2)
#define UPDATE_RESULT_UFW_FLASH_HEAD_CRC_ERR                        -(SD_UPDATE_ERR_BASE + 3)
#define UPDATE_RESULT_UFW_CODE_HEAD_CRC_ERR                         -(SD_UPDATE_ERR_BASE + 4)
#define UPDATE_RESULT_UFW_ALGIN_OF_OFFSET_MATCH_ERR                 -(SD_UPDATE_ERR_BASE + 5)
#define UPDATE_RESULT_REMOTE_FILE_HEAD_ERR                          -(SD_UPDATE_ERR_BASE + 6)
#define UPDATE_RESULT_NOT_FIND_TARGET_FILE_ERR                      -(SD_UPDATE_ERR_BASE + 7)
#define UPDATE_RESULT_LOADER_HEAD_CRC_ERR                           -(SD_UPDATE_ERR_BASE + 8)
#define UPDATE_RESULT_NOT_FIND_TARGET_LOADER                        -(SD_UPDATE_ERR_BASE + 9)
#define UPDATE_RESULT_LOADER_VERIFY_ERR                             -(SD_UPDATE_ERR_BASE + 10)
#define UPDATE_RESULT_LOADER_WRITE_ERR                              -(SD_UPDATE_ERR_BASE + 11)
#define UPDATE_RESULT_LOADER_INFO_VERIFY_ERR                        -(SD_UPDATE_ERR_BASE + 12)
#define UPDATE_RESULT_LOADER_INFO_WRITE_ERR                         -(SD_UPDATE_ERR_BASE + 13)
#define UPDATE_RESULT_PRODUCT_ID_NOT_MATCH                          -(SD_UPDATE_ERR_BASE + 14)
#define UPDATE_RESULT_UFW_CANNOT_FIND_VM_AREA                       -(SD_UPDATE_ERR_BASE + 15)
#define UPDATE_RESULT_KEY_ERR                                       -(SD_UPDATE_ERR_BASE + 16)
#define UPDATE_RESULT_FILE_OPERATION_ERR                            -(SD_UPDATE_ERR_BASE + 17)
#define UPDATE_RESULT_LOADER_SIZE_ERR                               -(SD_UPDATE_ERR_BASE + 18)
#define UPDATE_RESULT_REMOTE_FLASH_HEAD_NOT_MATCH                   -(SD_UPDATE_ERR_BASE + 19)
#define UPDATE_RESULT_REMOTE_FILE_INFO_NOT_MATCH                    -(SD_UPDATE_ERR_BASE + 20)
#define UPDATE_RESULT_EX_DSP_UPDATE_ERR                             -(SD_UPDATE_ERR_BASE + 21)
#define UPDATE_RESULT_DUALBANK_GET_UFW_APP_HEAD_ERR                 -(SD_UPDATE_ERR_BASE + 22)
#define UPDATE_RESULT_DUALBANK_GET_LOCAL_APP_HEAD_ERR               -(SD_UPDATE_ERR_BASE + 23)
#define UPDATE_RESULT_PRODUCT_INFO_NOT_MATCH                        -(SD_UPDATE_ERR_BASE + 24)
#define UPDATE_RESULT_FILE_SIZE_ERR                                 -(SD_UPDATE_ERR_BASE + 25)
#define UPDATE_RESULT_DUALBANK_APP_HEAD_NOT_MATCH                   -(SD_UPDATE_ERR_BASE + 26)
#define UPDATE_RESULT_CFG_UPDATE_ERR                                -(SD_UPDATE_ERR_BASE + 27)
#define UPDATE_RESULT_UPDATE_FILE_SAME                              -(SD_UPDATE_ERR_BASE + 28)

/**
 * @brief reserved update error code
 */
#define RESERVED_ZONE_ERR_BASE                                      150
#define RESERVED_ZONE_ERR_NONE                                      UPDATE_ERR_NONE
#define RESERVED_ZONE_ERR_REMOTE_LOCAL_ADDR_NOT_EQUAL               -(RESERVED_ZONE_ERR_BASE + 1)
#define RESERVED_ZONE_ERR_REMOTE_LOCAL_ZONE_LEN_LESS                -(RESERVED_ZONE_ERR_BASE + 2)
#define RESERVED_ZONE_ERR_REMOTE_LOCAL_FILE_LEN_LESS                -(RESERVED_ZONE_ERR_BASE + 3)
#define RESERVED_ZONE_ERR_APP_LOCAL_BAK_LEN_LESS                    -(RESERVED_ZONE_ERR_BASE + 4)
#define RESERVED_ZONE_ERR_DEVICE_OPS                                -(RESERVED_ZONE_ERR_BASE + 5)
#define RESERVED_ZONE_ERR_DATA_REMOTE_READ                          -(RESERVED_ZONE_ERR_BASE + 6)
#define RESERVED_ZONE_ERR_DATA_LOCAL_READ                           -(RESERVED_ZONE_ERR_BASE + 7)
#define RESERVED_ZONE_ERR_DATA_LOCAL_WRITE                          -(RESERVED_ZONE_ERR_BASE + 8)
#define RESERVED_ZONE_ERR_FILE_HEAD_VERIFY                          -(RESERVED_ZONE_ERR_BASE + 9)
#define RESERVED_ZONE_ERR_FILE_DATA_VERIFY                          -(RESERVED_ZONE_ERR_BASE + 10)
#define RESERVED_ZONE_ERR_FILE_ADDR                                 -(RESERVED_ZONE_ERR_BASE + 12)
#define RESERVED_ZONE_ERR_FILE_ATTRIBUTE                            -(RESERVED_ZONE_ERR_BASE + 13)
#define RESERVED_ZONE_ERR_RECORD_INFO                               -(RESERVED_ZONE_ERR_BASE + 14)
#define RESERVED_ZONE_ERR_LOCAL_NONE_RESERVED_FILE                  -(RESERVED_ZONE_ERR_BASE + 15)
#define RESERVED_ZONE_ERR_LOCAL_RESERVED_FILE_NO_MATCH              -(RESERVED_ZONE_ERR_BASE + 16)
#define RESERVED_ZONE_WARNING_FORCED_UPDATE                         -(RESERVED_ZONE_ERR_BASE + 17) //预留区强制升级

/**
 * @brief bt update error code
 */
#define UPDATE_TWS_ERR_BASE                                         170
#define UPDATE_ERR_WAIT_TWS_RESPONSE_TIMEOUT                        -(UPDATE_TWS_ERR_BASE + 1)
#define UPDATE_RESULT_OTA_APP_EXIT                                  -(UPDATE_TWS_ERR_BASE + 2)
#ifdef __cplusplus
}
#endif

#endif
