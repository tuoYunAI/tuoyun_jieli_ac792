

/**
* @file upgrade_adapter.h
* @brief Differential upgrade adaptation
* @version 0.1
* @date 2025-4-10
*
* @copyright Copyright 2020-2025 JL Inc. All Rights Reserved.
*
*/

#ifndef _UPGRADE_ADAPTER_H
#define _UPGRADE_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *(*source_open)(void);
    int (*source_close)(void *fd);
    int (*source_read)(unsigned char *buf, unsigned int len, void *fd);
    int (*local_write)(unsigned char *buf, unsigned int len, void *fd);
    int (*local_read)(void *buf, unsigned int offset, unsigned int len);
    int (*process_callback)(unsigned char event, void *parm);
} upgrade_opt_t;

typedef struct {
    unsigned int total_len;
    unsigned int current_len;
} upgrade_progress_t;

/**
 * @brief error code of ota
 */
#define INCREMENTAL_UPGRADE_ERR_NONE                      0

#define INCREMENTAL_UPGRADE_ERRCODE                              1000
#define INCREMENTAL_UPGRADE_ERR_MALLOC                           -(INCREMENTAL_UPGRADE_ERRCODE + 1)
#define INCREMENTAL_UPGRADE_DATA_READ_FAILED                     -(INCREMENTAL_UPGRADE_ERRCODE + 2)
#define INCREMENTAL_UPGRADE_ERR_INVALID_PARM                     -(INCREMENTAL_UPGRADE_ERRCODE + 3)
#define INCREMENTAL_UPGRADE_HEAD_INFO_VERIFY_FAILED              -(INCREMENTAL_UPGRADE_ERRCODE + 4)
#define INCREMENTAL_UPGRADE_OPEN_FAILED                          -(INCREMENTAL_UPGRADE_ERRCODE + 5)
#define INCREMENTAL_UPGRADE_ERR_DECOMPRESS_INIT                  -(INCREMENTAL_UPGRADE_ERRCODE + 6)
#define INCREMENTAL_UPGRADE_DECOMPRESS_STREAM_READ_FAILED        -(INCREMENTAL_UPGRADE_ERRCODE + 7)
#define INCREMENTAL_UPGRADE_ERR_DSPATCH_INIT                     -(INCREMENTAL_UPGRADE_ERRCODE + 8)
#define INCREMENTAL_UPGRADE_ERR_GET_BACKUP_INFO                  -(INCREMENTAL_UPGRADE_ERRCODE + 9)
#define INCREMENTAL_UPGRADE_ERR_PATCH_DATA_CRC                   -(INCREMENTAL_UPGRADE_ERRCODE + 10)
#define INCREMENTAL_UPGRADE_ERR_GET_CURRENT_INFO                 -(INCREMENTAL_UPGRADE_ERRCODE + 11)
#define INCREMENTAL_UPGRADE_ERR_LOCAL_CODE_CRC_UNMATCH           -(INCREMENTAL_UPGRADE_ERRCODE + 12)
#define INCREMENTAL_UPGRADE_ERR_LATEST_CODE_CRC_UNMATCH          -(INCREMENTAL_UPGRADE_ERRCODE + 13)
#define INCREMENTAL_UPGRADE_ERR_BOOT_INFO_UPDATE_FAILED          -(INCREMENTAL_UPGRADE_ERRCODE + 14)
#define INCREMENTAL_UPGRADE_ERR_DB_INIT_FAILED                   -(INCREMENTAL_UPGRADE_ERRCODE + 15)
#define INCREMENTAL_UPGRADE_ERR_PACKGE_SIZE_NOT_MATCH            -(INCREMENTAL_UPGRADE_ERRCODE + 16)
#define INCREMENTAL_UPGRADE_DATA_WRITE_FAILED                    -(INCREMENTAL_UPGRADE_ERRCODE + 2)

/**
 * @brief event code of ota
 */
#define INCREMENTAL_UPGRADE_PROCESS_FAILED_EVENT             1
#define INCREMENTAL_UPGRADE_LOAD_EVENT                       2
#define INCREMENTAL_UPGRADE_LOCAL_UPDATE_EVENT               3
#define INCREMENTAL_UPGRADE_LOCAL_UPGRADE_COMPLETED_EVENT    4


#ifdef __cplusplus
}
#endif

#endif /* _UPGRADE_ADAPTER_H */

