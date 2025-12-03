#ifndef _DEV_REG_H_
#define _DEV_REG_H_

#include "generic/typedef.h"

///设备参数控制句柄
struct __dev_reg {
    const char *logo;//设备选择使用的逻辑盘符
    const char *name;//设备名称，底层匹配设备节点使用
    const char *storage_path;//设备路径，文件系统mount时使用
    const char *root_path;//设备文件系统根目录
    const char *fs_type;//文件系统类型,如：fat, sdfile
};

#endif//__DEV_REG_H__


