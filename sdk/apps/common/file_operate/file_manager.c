#include "file_manager.h"
#include "dev_manager.h"
#include "app_config.h"

#if (TCFG_DEV_UPDATE_IF_NOFILE_ENABLE)
#include "dev_update.h"
#endif

#define MODE_FIX 1

#define LOG_TAG         "[FILE_MANAGER]"
/* #define LOG_INFO_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ASSERT_ENABLE
#include "system/debug.h"


static int file_manager_mode_deal(struct vfscan *fs, int sel_mode, int *arg)
{
    int fnum = 0;
    int file_start = 1;
    int file_end = fs->file_number;

    if (file_end == 0) {
        return -ENOENT;
    }

    struct ffolder folder = {0};
    fget_folder(fs, &folder);
    /* if ((scan->cycle_mode == FCYCLE_FOLDER) && (scan->ff_api.fileTotalInDir) */
    /*     && ((scan->ff_api.fileTotalOutDir + scan->ff_api.fileTotalInDir) <= scan->ff_api.totalFileNumber) */
    /*    ) { */

    if ((fs->cycle_mode == FCYCLE_FOLDER) && (folder.fileTotal)
        && ((folder.fileStart + folder.fileTotal - 1) <= file_end)
       ) {
        file_start = folder.fileStart;
        file_end = folder.fileStart + folder.fileTotal - 1;
    }

    switch (sel_mode) {
    case FSEL_LAST_FILE:
        fnum = fs->file_number;
        break;
    case FSEL_FIRST_FILE:
        fnum = 1;
        break;
    case FSEL_AUTO_FILE:
        /* if (scan->ff_api.fileNumber == 0) { */
        /*     return -EINVAL; */
        /* } */
        if (fs->cycle_mode == FCYCLE_ONE) {
            /* fnum = scan->ff_api.fileNumber; */
            fnum = fs->file_counter;
            break;
        }
    case FSEL_NEXT_FILE:
        /* if (scan->ff_api.fileNumber == 0) { */
        /*     return -EINVAL; */
        /* } */
        if (fs->cycle_mode == FCYCLE_RANDOM) {
            fnum = rand32() % (file_end - file_start + 1) + file_start;
            if (fnum == fs->file_counter) {
                fnum = fs->file_counter + 1;
            }
        } else {
            fnum = fs->file_counter + 1;
        }
        /* if (fnum > scan->last_file_number) { */
        if (fnum > fs->file_number) {
            if (fs->cycle_mode == FCYCLE_LIST) {
                /* break; */
            } else if (fs->cycle_mode == FCYCLE_FOLDER) {
                fnum = file_start;
            } else {
                fnum = 1;
            }
        }
        if (fnum > file_end) {
            fnum = file_start;
        } else if (fnum < file_start) {
            fnum = file_end;
        }
        break;
    case FSEL_PREV_FILE:
        /* if (scan->ff_api.fileNumber == 0) { */
        /*     return -EINVAL; */
        /* } */
        if (fs->cycle_mode == FCYCLE_RANDOM) {
            fnum = rand32() % (file_end - file_start + 1) + file_start;
            if (fnum == fs->file_counter) {
                fnum = fs->file_counter + 1;
            }
        } else {
            fnum = fs->file_counter - 1;
        }
        /* if ((scan->ff_api.fileNumber | BIT(15)) != scan->cur_file_number) { */
        /*     fnum -= scan->last_file_number - scan->ff_api.totalFileNumber; */
        /* } */
        /* scan->last_file_number = scan->ff_api.totalFileNumber; */
        if (fs->cycle_mode == FCYCLE_LIST) {
            /* break; */
        }
        if (fnum > file_end) {
            fnum = file_start;
        } else if (fnum < file_start) {
            fnum = file_end;
        }
        break;

    case FSEL_NEXT_FOLDER_FILE:
        /* fnum = scan->ff_api.fileTotalOutDir + scan->ff_api.fileTotalInDir + 1; */
        fnum = folder.fileStart + folder.fileTotal;
        if (fnum > fs->file_number) {
            fnum = 1;
        }
        break;
    case FSEL_PREV_FOLDER_FILE:
        /* if ((scan->ff_api.fileTotalOutDir + 1) > 1) { */
        if (folder.fileStart > 1) {
            fnum = folder.fileStart - 1;
        } else {
            fnum = fs->file_number;
        }
        break;
    default:
        return -EINVAL;
    }

    if ((sel_mode != FSEL_NEXT_FOLDER_FILE) && (sel_mode != FSEL_PREV_FOLDER_FILE)) {
        if (fnum < file_start) {
            fnum = file_start;
        } else if (fnum > file_end) {
            fnum = file_end;
        }
    }
    *arg = fnum;
    return 0;
}

struct vfscan *file_manager_scan_disk(struct __dev *dev, const char *path, const char *parm, u8 cycle_mode, scan_callback_t *callback)
{
    if (dev_manager_check(dev) == NULL) {
        return NULL;
    }
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
#if (TCFG_DM_MULTIPLEX_WITH_SD_PORT == 0)
    //0:sd0  1:sd1 //dm 参与复用的sd配置
    if (!memcmp(dev->parm->logo, "sd0", strlen("sd0"))) {
#else
    if (!memcmp(dev->parm->logo, "sd1", strlen("sd1"))) {
#endif
        dev_usb_change_sd();
    }

    if (!memcmp(dev->parm->logo, "udisk", strlen("udisk"))) {
        dev_sd_change_usb();
    }

    if (dev_manager_online_check(dev, 1) == NULL) {
        return NULL;
    }
#endif

    const char *fsn_path = NULL;
    char *tmp_path = NULL;

    if (path) {
        if (*path == '/') {
            path++;
        }
        tmp_path = zalloc(strlen(dev->parm->root_path) + strlen(path) + 1);
        if (tmp_path == NULL) {
            return NULL;
        }
        sprintf(tmp_path, "%s%s", dev->parm->root_path, path);
        fsn_path = tmp_path;
    } else {
        fsn_path = dev->parm->root_path;
    }
    /* printf("fsn_path = %s, scan parm = %s\n", fsn_path, parm); */

    if (callback && callback->enter) {
        callback->enter(dev);//扫描前处理， 可以在注册的回调里提高系统时钟等处理
    }

    struct vfscan *fsn = fscan_interrupt(
                             fsn_path,
                             parm,
                             FILE_MANAGER_SCAN_DISK_MAX_DEEPTH,
                             ((callback) ? callback->scan_break : NULL));

    if (callback && callback->exit) {
        callback->exit(dev);//扫描后处理， 可以在注册的回调里还原到enter前的状态
    }

    if (fsn) {
        if (fsn->file_number == 0) {
            /* printf("dev nofile\n"); */
#if (TCFG_DEV_UPDATE_IF_NOFILE_ENABLE)
            ///没有文件找升级文件
            dev_update_check(dev->parm->logo);
#endif/*TCFG_DEV_UPDATE_IF_NOFILE_ENABLE*/
            ///没有文件,释放fsn， 减少外面流程的处理
            fscan_release(fsn);
            fsn = NULL;
        } else {
            fsn->cycle_mode = cycle_mode;
        }
    }

    if (tmp_path) {
        free(tmp_path);
    }

    return fsn;
}

FILE *file_manager_select(struct __dev *dev, struct vfscan *fs, int sel_mode, int arg, scan_callback_t *callback)
{
    if (callback && callback->enter) {
        callback->enter(dev);//扫描前处理， 可以在注册的回调里提高系统时钟等处理
    }
#if MODE_FIX
    if ((sel_mode != FSEL_BY_SCLUST) && (sel_mode != FSEL_BY_PATH) && (sel_mode != FSEL_BY_NUMBER)) {
        if (file_manager_mode_deal(fs, sel_mode, &arg)) {
            return NULL;
        }
        sel_mode = FSEL_BY_NUMBER;
    }
#endif
    FILE *_file = fselect(fs, sel_mode, arg);

    if (callback && callback->exit) {
        callback->exit(dev);//扫描后处理， 可以在注册的回调里还原到enter前的状态
    }

    return _file;
}

int file_manager_delete_deal(const char *path, const char *param, u8 dir_flag)
{
    u16 folder_total_file;
    int d_err;
    struct vfscan *fsn = NULL;
    FILE *d_f = NULL;
    int res = 0;

    fsn = fscan(path, param, 9);
    if (fsn == NULL) {
        res = -1;
        goto __exit;
    }

    folder_total_file = fsn->file_number;

    log_info(">>>[test]:total = %d", folder_total_file);

    for (int i = folder_total_file; i >= 1; i--) {
        if (!dir_flag) {
            d_f = fselect(fsn, FSEL_BY_NUMBER, folder_total_file - i + 1); //加速处理，不用找到最后一个文件。
        } else {
            d_f = fselect(fsn, FSEL_BY_NUMBER, i);
        }
        if (d_f == NULL) {
            log_error(">>>[test]:err!! select file err");
            res = -1;
            goto __exit;
        }
        /* putchar('D'); */
        d_err = fdelete(d_f);
        if (d_err) {
            log_error(">>>[test]:err!! delete file err");
            res = -1;
            goto __exit;
        }
        d_f = NULL;
    }
__exit:
    if (fsn) {
        fscan_release(fsn);
    }
    return res;
}

/*----------------------------------------------------------------------------*/
/** @brief:文件夹删除处理
    @param:dev_logo :设备logo  folder:文件夹路径(短名) folder_len:文件夹路径长度
    @return:
    @author:phewlee
    @note:
    @date: 2021-05-21,10:16
*/
/*----------------------------------------------------------------------------*/
int file_manager_delete_dir(const char *dev_logo, const char *folder, u16 folder_len)
{
    int err = 0;
    struct __dev *dev = NULL;
    char path[128] = {0};

    const char delete_file_param[] = "-t"
                                     "ALL"
                                     " -sn -r";

    const char delete_folder_param[] = "-t"
                                       "ALL"
                                       " -sn -d -r";

    dev = dev_manager_find_spec(dev_logo, 0);
    if (dev == NULL) {
        log_error(">>>[test]:errr!!!!!!!!! not find dev");
        return -1;
    }

    const char *root_path = dev_manager_get_root_path(dev);
    memcpy(path, root_path, strlen(root_path));
    memcpy(path + strlen(root_path), folder, folder_len);

    log_info(">>>[test]:path = %s", path);

    err = file_manager_delete_deal(path, delete_file_param, 0);
    if (err) {
        log_error(">>>[test]:errr!!!!!!!!! delete file deal fail");
        return -1;
    }

    err = file_manager_delete_deal(path, delete_folder_param, 1);
    if (err) {
        log_error(">>>[test]:errr!!!!!!!!! delete folder  deal fail");
        return -1;
    }

    FILE *folder_f = fopen(path, "r");
    if (folder_f == NULL) {
        log_error(">>>[test]:err open folder");
        return -1;
    }

    return fdelete(folder_f);
}

int file_manager_division_file(FILE *f, const char *dev_logo, const char *file_name, u32 name_len, u32 fptr, char *buf, u32 buf_len, const char *folder)
{
    struct __dev *dev;
    char path[128] = {0};
    char tmp_name[64] = {0};

    if ((dev_logo == NULL) || (file_name == NULL) || (buf == NULL)) {
        return -1;
    }

    dev = dev_manager_find_spec(dev_logo, 0);
    if (dev == NULL) {
        log_error(">>>[test]:errr!!!!!!!!! not find dev");
        return -1;
    }

    const char *root_path = dev_manager_get_root_path(dev);

    /****************分割文件*********************/
    if (folder) {
        memcpy(tmp_name, folder, strlen(folder));
    }
    strcat(tmp_name, "/");
    strcat(tmp_name, "jl_test.tmp");

    if (fdicvision_file(f, tmp_name, fptr)) {
        log_error(">>>[test]:file division fail");
        goto __exit;
    }

    /****************打开分割出去的文件*********************/
    memcpy(path, root_path, strlen(root_path) - 1);
    memcpy(path + strlen(root_path) - 1, tmp_name, strlen(tmp_name));

    log_info(">>>[test]:path1 = %s", path);

    FILE *i_f = fopen(path, "r");
    if (i_f == NULL) {
        log_error(">>>[test]:open i_f fail");
        return -1;
    }
    ///////////////////////////////////////////////////

    /****************创建新文件，写入头数据*********************/
    memset(path, 0, sizeof(path));
    memcpy(path, root_path, strlen(root_path));
    if (folder) {
        memcpy(path + strlen(path) - 1, folder, strlen(folder));
        strcat(path, "/");
    }
    memcpy(path + strlen(path), file_name, name_len);

    log_info(">>>[test]:path1 = %s", path);

    FILE *d_f = fopen(path, "w+");
    if (d_f == NULL) {
        log_error(">>>[test]:open d_f fail");
        fclose(i_f);
        i_f = NULL;
        goto __exit;
    }
    /* int wlen = fwrite(d_f, buf, buf_len); */

    int wlen = fwrite(buf, buf_len, 1, d_f);
    if (wlen != buf_len) {
        log_error(">>>[test]:err write!!!!!!!!wlen = %d, buf_len = %d", wlen, buf_len);
        fclose(i_f);
        i_f = NULL;
        fclose(d_f);
        d_f = NULL;
        goto __exit;
    }

    fclose(d_f);
    d_f = NULL;

    d_f = fopen(path, "r");
    if (d_f == NULL) {
        log_error(">>>[test]:open d_f fail");
        fclose(i_f);
        i_f = NULL;
        goto __exit;
    }
    //////////////////////////////////////////////////////////

    int fsize = flen(d_f);
    int err = finsert_file(d_f, i_f, fsize);
    fclose(i_f);
    i_f = NULL;
    fclose(d_f);
    d_f = NULL;
    return err;

__exit:
    memset(path, 0, sizeof(path));
    memcpy(path, root_path, strlen(root_path) - 1);
    memcpy(path + strlen(root_path) - 1, tmp_name, strlen(tmp_name));

    log_error(">>>[test]:error process: path1 = %s", path);

    FILE *f_tmp = fopen(path, "r");
    if (f_tmp) {
        fdelete(f_tmp);
        f_tmp = NULL;
    }

    return -1;
}

