#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#include "generic/typedef.h"
#include "fs/fs.h"
#include "dev_manager.h"

#define FILE_MANAGER_SCAN_DISK_MAX_DEEPTH   9

typedef struct {
    void (*enter)(struct __dev *dev);
    void (*exit)(struct __dev *dev);
    int (*scan_break)(void);
} scan_callback_t;


FILE *file_manager_select(struct __dev *dev, struct vfscan *fs, int sel_mode, int arg, scan_callback_t *callback);

//*----------------------------------------------------------------------------*/
/**@brief   设备文件扫描扫盘
   @param
   			dev：设备节点
   			path：指定扫描目录
   			parm：扫描参数，包括文件后缀等
   			cycle_mode：播放循环模式
			callback：扫描打断回调
   @return  成功返回扫描控制句柄，失败返回NULL
   @note
*/
/*----------------------------------------------------------------------------*/
struct vfscan *file_manager_scan_disk(struct __dev *dev, const char *path, const char *parm, u8 cycle_mode, scan_callback_t *callback);

/* --------------------------------------------------------------------------*/
/**
 * @brief    文件删除统一处理
 *
 * @param path 扫描路径名
 * @param param 配置参数
 * @param dir_flag 是否删除文件夹标志
 * @note 加速处理:(删除文件的时候使用)从前往后依次删除. 文件夹必须从后往前删。
 *
 * @return  0成功， 其他失败
 */
/* ----------------------------------------------------------------------------*/
int file_manager_delete_deal(const char *path, const char *param, u8 dir_flag);

/*----------------------------------------------------------------------------*/
/** @brief:文件夹删除处理
    @param:dev_logo :设备logo  folder:文件夹路径(短名) folder_len:文件夹路径长度
    @return:
    @author:phewlee
    @note:
    @date: 2021-05-21,10:16
*/
/*----------------------------------------------------------------------------*/
int file_manager_delete_dir(const char *dev_logo, const char *folder, u16 folder_len);

/*----------------------------------------------------------------------------*/
/** @brief:文件分割处理
    @param:f: 源文件句柄, dev_logo:设备logo , file_name : 文件名， name_len: 文件长度， fptr:偏移量, buf:头文件数据， buf_len:头文件数据长度, folder : 需要指定的路径，绝对路径，例如：/RECORD或者/1/2/3/4, 暂时只支持短名文件夹形式, folder 传NULL默认根目录。
    @return:
    @author:phewlee
    @note:
    @date: 2021-05-25,10:16
*/
/*----------------------------------------------------------------------------*/
int file_manager_division_file(FILE *f, const char *dev_logo, const char *file_name, u32 name_len, u32 fptr, char *buf, u32 buf_len, const char *folder);

#endif// __FILE_MANAGER_H__
