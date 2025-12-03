#ifndef _SDFILE_H_
#define _SDFILE_H_

#include "generic/typedef.h"

#define SDFILE_PAGE_SIZE 	256
#define SDFILE_SECTOR_SIZE 	4096

/********* sdfile 文件头 **********/
#define SDFILE_NAME_LEN 			16

typedef struct sdfile_file_head {
    u16 head_crc;
    u16 data_crc;
    u32 addr;
    u32 len;
    u8 attr;
    u8 res;
    u16 index;
    char name[SDFILE_NAME_LEN];
} SDFILE_FILE_HEAD;

#if 0
////////////////////////////sdfile_file_head成员详细说明///////////////////////////////////////
typedef struct SDFILEJL_FILE_HEAD {
    u16 u16Crc;     // 结构体自身校验码
    u16 u16DataCrc; // 文件数据校验码
    u32 u32Address; // 数据存储地址
    u32 u32Length;  // 数据长度
    u8 u8Attribute; // 属性
    u8 u8Res;       // 保留数据
    u16 u16Index;   // 文件编号，从大到小排列，编号为0时，代表后面没有文件
    char szFileName[16];// 文件名
} JL_SDFILE_HEAD;

////////////////////////////flash_head成员详细说明///////////////////////////////////////
typedef struct SDFILEJL_FLASH_HEAD_V2 {
    u16 u16Crc;             // 结构体自身校验码
    u16 u16SizeForBurner;   // 为烧写器保留的空间大小
    char vid[4];    		// 存放VID信息，长度是4个byte
    u32 u32FlashSize;       // FLASH大小,由isd_download计算得出
    u8 u8FsVersion;         // flash类型：BR18(0)/BR22(1)
    u8 u8BlockAlingnModulus;// 对齐系数，对齐的值=对齐系数*256
    u8 u8Res;          		// 保留
    u8 u8SpecialOptFlag;    // 用于标记是否需要生成2种flash格式的标记位，目前只用1位
    char pid[16];    		// 存放PID信息，长度是16个byte
} SDFILEJL_FLASH_HEAD_V2;

#endif

struct sdfile_dir {
    u16 file_num;
    struct sdfile_file_head head;
};

enum sdfile_err_table {
    SDFILE_DIR_NOT_EXIST = -0xFF,
    SDFILE_FILE_NOT_EXIST,
    SDFILE_MALLOC_ERR,
    SDFILE_VM_NOT_FIND,
    SDFILE_DATA_CRC_ERR,
    SDFILE_WRITE_AREA_NEED_ERASE_ERR,
    SDFILE_SUSS = 0,
    SDFILE_END,
};

#define SDFILE_MAX_DEEPTH 		2

struct sdfile_folder {
    u32 saddr;
    u32 addr;
    u32 len;
};

struct sdfile_scn {
    u8 subpath;
    u8 cycle_mode;
    u8 attr;
    u8 deepth;
    u8 part_index; //标记分区，0为res 1：app 2:ext_reserve
    u8 scan_dir_flag; //按目录扫描标志
    u16 dirCounter;
    u16 fileCounter;
    u16	fileNumber;			// 当前文件序号
    u16	totalFileNumber;
    u16	last_file_number;
    u16 fileTotalInDir;     // 当前目录的根下有效文件的个数
    u16 fileTotalOutDir;	// 当前目录前的文件总数，目录循环模式下，需要用它来计算文件序号
    u32 sclust_id;
    u32 root_addr; //分区根地址
    const char *ftypes;
    struct sdfile_file_head head;
    struct sdfile_folder folder[SDFILE_MAX_DEEPTH];
};

typedef struct sdfile_file_t {
    u32 fptr;
    struct sdfile_file_head head;
    struct sdfile_scn *pscn;
} SDFILE;

#ifndef SDFILE_MOUNT_PATH
#define SDFILE_MOUNT_PATH     	"mnt/sdfile"
#endif /* #ifndef SDFILE_MOUNT_PATH */

#ifndef SDFILE_APP_ROOT_PATH
#define SDFILE_APP_ROOT_PATH       	SDFILE_MOUNT_PATH"/app/"  //app分区
#endif /* #ifndef SDFILE_APP_ROOT_PATH */

#ifndef SDFILE_RES_ROOT_PATH
#define SDFILE_RES_ROOT_PATH       	SDFILE_MOUNT_PATH"/res/"  //资源文件分区
#endif /* #ifndef SDFILE_RES_ROOT_PATH */

#ifndef SDFILE_EXT_RESERVED_ROOT_PATH
#define SDFILE_EXT_RESERVED_ROOT_PATH       SDFILE_MOUNT_PATH"/EXT_RESERVED/"  //资源文件分区
#endif /* #ifndef SDFILE_RES_ROOT_PATH */

//获取flash物理大小, 单位: Byte
u32 sdfile_get_disk_capacity(void);
u32 sdfile_ext_get_disk_capacity(void);
//flash addr  -> cpu addr
u32 sdfile_flash_addr2cpu_addr(u32 offset);
u32 sdfile_ext_flash_addr2cpu_addr(u32 offset);
//cpu addr  -> flash addr
u32 sdfile_cpu_addr2flash_addr(u32 offset);
u32 sdfile_ext_cpu_addr2flash_addr(u32 offset);

int set_res_startaddr(int offset);
void set_vm_addr_info(u32 addr, u32 size);

int sdfile_ext_read_origin(void *buf, u32 addr, u32 len);
int sdfile_ext_read(void *buf, u32 addr, u32 len);
int sdfile_ext_write(void *buf, u32 addr, u32 len);
int sdfile_ext_erase(u32 addr, u32 len);

int sdile_get_ext_reserve_zone_addr(const char *zone_name);
int sdile_get_ext_reserve_zone_size(const char *zone_name);
int sdfile_reserve_zone_read(void *buf, u32 addr, u32 len, int mode);
int sdfile_reserve_zone_write(void *buf, u32 addr, u32 len, int mode);
int sdfile_reserve_zone_erase(u32 addr, u32 len, int mode);

extern const int config_sdfile_ext_enable;

#endif /* _SDFILE_H_ */

