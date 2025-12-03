#include "lyrics/lyrics_api.h"
#include "fs/fs.h"
#include "app_config.h"
#include "network_download/net_download.h"
#include "system/init.h"

#if TCFG_LRC_LYRICS_ENABLE

#define LOG_TAG_CONST       LYRICS
#define LOG_TAG             "[LYRICS_API]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


static const LRC_FILE_IO lrc_file_io = {
    .seek = fseek,
    .read = fread,
};

static struct list_head head = LIST_HEAD_INIT(head);
static OS_MUTEX	mutex;

static int lrc_analysis_mutex_init(void)
{
    return os_mutex_create(&mutex);
}
late_initcall(lrc_analysis_mutex_init);

static int net_lrc_file_read(void *buf, u32 size, u32 count, FILE *file)
{
    return net_download_read((void *)file, buf, size);
}

static int net_lrc_file_seek(FILE *file, int offset, int orig)
{
    return net_download_seek((void *)file, offset, orig);
}

static const LRC_FILE_IO net_lrc_file_io = {
    .seek = net_lrc_file_seek,
    .read = net_lrc_file_read,
};

/*----------------------------------------------------------------------------*/
/**@brief 配置翻屏的速度
   @param lrc_len--当前歌词长度，time_gap-与下一条歌词的间隔，
   @param *roll_speed - 翻屏的速度 500ms unit
   @return
   @note
 */
/*----------------------------------------------------------------------------*/
static void lrc_roll_speed_ctrl(u8 lrc_len, u32 time_gap, u8 *roll_speed)
{
    ///翻页滚动速度控制
    ///log_info("speed = %d,%d",lrc_len,time_gap);
    ///DVcTxt1_11 显示长度为32 bytes
    if (lrc_len > (LRC_DISPLAY_TEXT_LEN * 2)) {
        *roll_speed = ((time_gap + 2) / 3) << 1;
    } else if (lrc_len > LRC_DISPLAY_TEXT_LEN) {
        *roll_speed = time_gap;
    } else {
        *roll_speed = 250; ///never load new page
    }
}

/*----------------------------------------------------------------------------*/
/**@brief 歌词模块初始化
   @param
   @return 0--成功，非0 失败
   @note
 */
/*----------------------------------------------------------------------------*/
void *lyric_init(void)
{
    LRC_CFG t_lrc_cfg = {0};
    t_lrc_cfg.once_read_len = ONCE_READ_LENGTH;
    t_lrc_cfg.once_disp_len = ONCE_DIS_LENGTH;
    t_lrc_cfg.label_temp_buf_len = LABEL_TEMP_BUF_LEN;
    t_lrc_cfg.roll_speed_ctrl_cb = lrc_roll_speed_ctrl;
    t_lrc_cfg.clr_lrc_disp_cb = NULL;
    /* t_lrc_cfg.lrc_text_id = LRC_DISPLAY_TEXT_ID; */
    t_lrc_cfg.read_next_lrc_flag = 0;
#if TCFG_LRC_ENABLE_SAVE_LABEL_TO_FLASH
    t_lrc_cfg.enable_save_lable_to_flash = 0;
#endif

    u32 need_buf_size = LRC_SIZEOF_ALIN(sizeof(LRC_INFO), 4)
                        + LRC_SIZEOF_ALIN(sizeof(LABEL_INFO), 4)
                        + LRC_SIZEOF_ALIN(sizeof(SORTING_INFO), 4)
                        + LRC_SIZEOF_ALIN(t_lrc_cfg.once_disp_len, 4)
                        + LRC_SIZEOF_ALIN(t_lrc_cfg.label_temp_buf_len, 4)
                        + LRC_SIZEOF_ALIN(t_lrc_cfg.once_read_len, 4);

    log_info("lrc need_buf_size=%d---", need_buf_size);
    /* ASSERT(sizeof(lrc_info) >= need_buf_size); */

    LRC_INFO *lrc_info = zalloc(need_buf_size);
    if (!lrc_info) {
        return NULL;
    }

    if (0 != lrc_param_init(&t_lrc_cfg, lrc_info)) {
        free(lrc_info);
        return NULL;
    }

    os_mutex_pend(&mutex, 0);
    list_add_tail(&lrc_info->entry, &head);
    os_mutex_post(&mutex);

    return lrc_info;
}

/*----------------------------------------------------------------------------*/
/**@brief 歌词模块退出
   @param
   @return
   @note

 */
/*----------------------------------------------------------------------------*/
void lyric_exit(void *lrc)
{
    LRC_INFO *lrc_info = (LRC_INFO *)lrc;

    if (!lrc_info) {
        return;
    }

    os_mutex_pend(&mutex, 0);
    list_del(&lrc_info->entry);
    /* lrc_destroy(lrc_info); */
    os_mutex_post(&mutex);

    free(lrc_info);
}

/*----------------------------------------------------------------------------*/
/**@brief 搜索歌词，解析
   @param
   @return 0--成功，非0 失败
   @note
 */
/*----------------------------------------------------------------------------*/
static bool lrc_find(FILE *file, FILE **newFile, void *ext_name)
{
    if (fget_file_byname_indir(file, newFile, ext_name)) {
        return FALSE;
    }

    return TRUE;
}

void lrc_set_analysis_flag(void *lrc, u8 flag)
{
    LRC_INFO *lrc_info = (LRC_INFO *)lrc;

    if (!lrc_info) {
        return;
    }

    os_mutex_pend(&mutex, 0);
    lrc_info->analysis_flag = flag;
    os_mutex_post(&mutex);
}

bool lrc_show_api(void *lrc, int text_id, u16 dbtime_s, u8 btime_100ms)
{
    LRC_INFO *lrc_info = (LRC_INFO *)lrc;
    bool ret = FALSE;

    os_mutex_pend(&mutex, 0);

    if (lrc_info) {
        if (lrc_info->analysis_flag) {
            if (TRUE == lrc_show(lrc_info, text_id, dbtime_s, btime_100ms)) {
                ret = TRUE;
            }
        }

        os_mutex_post(&mutex);

        return ret;
    }

    list_for_each_entry(lrc_info, &head, entry) {
        if (lrc_info->analysis_flag) {
            if (TRUE == lrc_show(lrc_info, text_id, dbtime_s, btime_100ms)) {
                ret = TRUE;
            }
        }
    }

    os_mutex_post(&mutex);

    return ret;
}

bool lrc_get_api(void *lrc, u16 dbtime_s, u8 btime_100ms)
{
    LRC_INFO *lrc_info = (LRC_INFO *)lrc;
    bool ret = FALSE;

    os_mutex_pend(&mutex, 0);

    if (lrc_info) {
        if (lrc_info->analysis_flag) {
            if (TRUE == lrc_get(lrc_info, dbtime_s, btime_100ms)) {
                ret = TRUE;
            }
        }

        os_mutex_post(&mutex);

        return ret;
    }

    list_for_each_entry(lrc_info, &head, entry) {
        if (lrc_info->analysis_flag) {
            if (TRUE == lrc_get(lrc_info, dbtime_s, btime_100ms)) {
                ret = TRUE;
            }
        }
    }

    os_mutex_post(&mutex);

    return ret;
}

bool net_lrc_analysis_api(void *lrc, void *net_lrc_file)
{
    LRC_INFO *lrc_info = (LRC_INFO *)lrc;
    if (!lrc_info) {
        return FALSE;
    }

    os_mutex_pend(&mutex, 0);

    if (TRUE == lrc_analysis(lrc_info, net_lrc_file, &net_lrc_file_io)) {
        lrc_set_analysis_flag(lrc_info, 1);
        os_mutex_post(&mutex);
        log_info("net lrc analazy succ");
        return FALSE;
    } else {
        lrc_set_analysis_flag(lrc_info, 0);
        os_mutex_post(&mutex);
        log_error("net lrc analazy err");
        return TRUE;
    }
}

bool lrc_analysis_api(void *lrc, FILE *file, FILE **newFile)
{
    LRC_INFO *lrc_info = (LRC_INFO *)lrc;
    if (!lrc_info) {
        return FALSE;
    }

    os_mutex_pend(&mutex, 0);

    if (*newFile) {
        fclose(*newFile);
        *newFile = NULL;
    }

    if (TRUE == lrc_find(file, newFile, (void *)"LRC")) {
        if (TRUE == lrc_analysis(lrc_info, *newFile, &lrc_file_io)) {
            lrc_set_analysis_flag(lrc_info, 1);
            os_mutex_post(&mutex);
            log_info("lrc analazy succ");
            return TRUE;
        } else {
            lrc_set_analysis_flag(lrc_info, 0);
            os_mutex_post(&mutex);
            log_error("lrc analazy err");
            return FALSE;
        }
    } else {
        lrc_set_analysis_flag(lrc_info, 0);
        os_mutex_post(&mutex);
        log_error("lrc_find err");
        return FALSE;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  歌词显示，默认显示两行
   @param  lrc_show_flag:滚动显示标志 lrc_update:显示下一条歌词内容标志
   @return true--成功
   @note   可以根据xi需要修改该函数的实现方法
 */
/*----------------------------------------------------------------------------*/
bool lrc_ui_show(int text_id, u8 encode_type, u8 *buf, int len, u8 lrc_show_flag, u8 lrc_update)
{
#if 0
#if(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX)
    static int disp_len = 0;
    static u8 lrc_showbytes = 0;
    static u8 offset = 0;
    if (lrc_update) {
        disp_len = len;
        lrc_showbytes = 0;
        offset = 0;
        ui_text_set_text_by_id(LRC_TEXT_ID_SEC, "", 16, FONT_DEFAULT);
    }
    if (lrc_show_flag == 1) {
        if (LRC_UTF16_S == encode_type) {
            lrc_showbytes =  ui_text_set_textw_by_id(LRC_TEXT_ID_FIR, (const char *)(buf + offset), disp_len, FONT_ENDIAN_SMALL, FONT_DEFAULT);
        } else if (LRC_UTF16_B == encode_type) {
            lrc_showbytes = ui_text_set_textw_by_id(LRC_TEXT_ID_FIR, (const char *)(buf + offset), disp_len, FONT_ENDIAN_BIG, FONT_DEFAULT);
        } else if (LRC_UTF8 == encode_type) {
            lrc_showbytes = ui_text_set_textw_by_id(LRC_TEXT_ID_FIR, (const char *)(buf + offset), disp_len, FONT_ENDIAN_SMALL, FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
        } else {
            lrc_showbytes = ui_text_set_text_by_id(LRC_TEXT_ID_FIR, (const char *)(buf + offset), disp_len, FONT_DEFAULT);
        }
        offset += lrc_showbytes;
        if (LRC_UTF16_S == encode_type) {
            lrc_showbytes = ui_text_set_textw_by_id(LRC_TEXT_ID_SEC, (const char *)(buf + offset), disp_len, FONT_ENDIAN_SMALL, FONT_DEFAULT);
        } else if (LRC_UTF16_B == encode_type) {
            lrc_showbytes = ui_text_set_textw_by_id(LRC_TEXT_ID_SEC, (const char *)(buf + offset), disp_len, FONT_ENDIAN_BIG, FONT_DEFAULT);
        } else if (LRC_UTF8 == encode_type) {
            lrc_showbytes = ui_text_set_textw_by_id(LRC_TEXT_ID_SEC, (const char *)(buf + offset), disp_len, FONT_ENDIAN_SMALL, FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
        } else {
            lrc_showbytes = ui_text_set_text_by_id(LRC_TEXT_ID_SEC, (const char *)(buf + offset), disp_len, FONT_DEFAULT);
        }
    }
#endif
    return true;
#else
    r_printf("====%s=>%s=====%d==NULL", __FILE__, __func__, __LINE__);
    return true;
#endif
}

#endif //TCFG_LRC_LYRICS_ENABLE

