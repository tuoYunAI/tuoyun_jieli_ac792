/**
 * @file ota_impl.h
 * @brief OTA 双备份接口封装
 *
 * @copyright Copyright(C),2010-2024, 杰理科技 https://www.zh-jieli.com/
 *
 */

#ifndef _OTA_IMPL_H
#define _OTA_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "error_code.h"

#define CONFIG_UPGRADE_OTA_FILE_NAME        "update-ota.ufw"

typedef void (*update_process_notify)(void *priv);

typedef struct _update_file_info {
    char FileName[16];
    u32 file_size;
    u32 finish_len;
} update_file_info;

enum {
    CLEAR_APP_RUNNING_BANK = 0,
    CLEAR_APP_UPDATE_BANK,
};

/**
 * @brief set_update_process_notify_cb用于设置升级进度回调
 *
 * @param[in] update_process_notify
 */
void set_update_process_notify_cb(update_process_notify callback);

/**
 * @brief dual_bank_passive_update_init用于初始化升级所需资源
 *
 * @param[in] fw_crc  固件crc, 设置为0由内部计算
 * @param[in] fw_size 固件大小, 设置为0由内部计算
 * @param[in] max_pkt_len 升级申请的最大缓存大小，不能小于512，建议为4K
 * @param[in] priv     保留未使用
 * @return update_err_t UPDATE_ERR_NONE=成功，非0=失败
 */
update_err_t dual_bank_passive_update_init(u32 fw_crc, u32 fw_size, u16 max_pkt_len, void *priv);

struct BootInfo {
    u32 codeLength;  //code总长度
    u32 baseAddress; //code基地址
    u16 version;     //当前版本
};

/* @brief:get current startup information
 * @param info: get information
   @returns: if 0, succeed, other value means to fail.
 */
int get_current_boot_info(struct BootInfo *info);


u32 get_dual_bank_passive_update_max_buf(void);
update_err_t dual_bank_passive_update_init(u32 fw_crc, u32 fw_size, u16 max_pkt_len, void *priv);
update_err_t dual_bank_passive_update_exit(void *priv);
update_err_t dual_bank_update_allow_check(u32 fw_size);
update_err_t dual_bank_update_write(void *data, u16 len, int (*write_complete_cb)(void *priv));
update_err_t dual_bank_update_burn_boot_info(int (*burn_boot_info_result_hdl)(int err));
update_err_t flash_update_clr_boot_info(u8 type);
update_err_t dual_bank_update_verify(void (*crc_init_hdl)(void), u32(*crc_calc_hdl)(u32 init_crc, const void *data, u32 len), int (*verify_result_hdl)(int calc_crc));


void *net_fopen(char *path, char *mode);//net_fopen支持写flash固件升级和写到SD卡，当名字字符有CCONFIG_UPGRADE_OTA_FILE_NAME时是固件升级
int net_fwrite(void *file, unsigned char *buf, int len, int end);
int net_fread(void *file, char *buf, int len);
int net_flen(void *file);
int net_fclose(void *file, char is_socket_err);
int net_update_request(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*_OTA_IMPL_H*/
