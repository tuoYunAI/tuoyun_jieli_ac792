#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tts_player.data.bss")
#pragma data_seg(".tts_player.data")
#pragma const_seg(".tts_player.text.const")
#pragma code_seg(".tts_player.text")
#endif
#include "tts_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/wait.h"
#include "system/spinlock.h"
#include "jldemuxer.h"
#include "app_config.h"

#define LOG_TAG             "[TTS_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

#ifdef CONFIG_NET_ENABLE

static const struct stream_file_ops tts_file_ops;

static u32 g_player_id;
static spinlock_t lock;
static OS_MUTEX g_tts_mutex;
static struct list_head g_head = LIST_HEAD_INIT(g_head);

char *strdup(const char *s);
static int __tts_net_completion_callback(void *priv, int timeout);
static int tts_player_start(struct tts_player *, int);
static int tts_file_close(void *file);
static int tts_file_flen(void *file);

static void *open_next_file(struct tts_player *player)
{
    void *file = NULL;

    if (player->next_file) {
        file = player->next_file;
        player->next_file = NULL;
        return file;
    }

    int index = ++player->index;

    do {
        if (index < player->index_max) {
            if (player->url_file_list[index]) {
                player->parm.url = player->url_file_list[index];
                int err = net_download_open(&file, &player->parm);
                if (err) {
                    log_error("tts_player_faild: open err = %d", err);
                    return NULL;
                }
                if (file) {
                    player->index = index;
                    return file;
                }

                index++;
            }
        } else {
            player->index = 0;
            break;
        }
    } while (index != player->index);

    return NULL;
}

static void tts_player_free(struct tts_player *player, int notify)
{
    void *cb_priv = player->priv;
    tone_player_cb_t callback = player->callback;

    if (--player->ref == 0) {
        tts_file_close(player);
        if (player->url_file_list) {
            for (int file_num = 0; file_num < player->index_max; file_num++) {
                if (player->url_file_list[file_num]) {
                    free((void *)player->url_file_list[file_num]);
                }
            }
            free(player->url_file_list);
        }
        free(player);
    }

    if (notify && callback) {
        callback(cb_priv,  STREAM_EVENT_STOP);
    }
}

static void tts_player_callback(void *priv, int event)
{
    u32 player_id = (u32)priv;
    void *file = NULL;
    struct tts_player *player;

    log_info("tts_callback: 0x%x, %d", event, player_id);

    switch (event) {
    case STREAM_EVENT_START:
        os_mutex_pend(&g_tts_mutex, 0);
        if (list_empty(&g_head)) {          //先判断是否为空防止触发异常
            os_mutex_post(&g_tts_mutex);
            break;
        }
        player = list_first_entry(&g_head, struct tts_player, entry);
        if (player->player_id == player_id) {
            if (player->callback) {
                player->callback(player->priv,  STREAM_EVENT_START);
            }
        }
        os_mutex_post(&g_tts_mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
    case STREAM_EVENT_NEGOTIATE_FAILD:
    /*
     * 提示音被抢占或者参数协商失败,直接结束播放
     */
    case STREAM_EVENT_STOP:
        os_mutex_pend(&g_tts_mutex, 0);
        if (list_empty(&g_head)) {          //先判断是否为空防止触发异常
            os_mutex_post(&g_tts_mutex);
            break;
        }
        player = list_first_entry(&g_head, struct tts_player, entry);
        if (player->player_id != player_id) {
            log_warn("player_id_not_match: %d %d", player->player_id, player_id);
            os_mutex_post(&g_tts_mutex);
            break;
        }

        if (event == STREAM_EVENT_STOP) {
            player->wait = 0;
            /*
             * 打开文件列表的下一个文件,重新启动解码
             */
            while (1) {
                file = open_next_file(player);
                if (!file) {
                    break;
                }
                net_download_close(player->file);

                player->file = file;

                int err = net_download_check_ready(player->file);
                if (err > 0) {
                    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                                        NODE_IOC_SET_FILE_LEN, tts_file_flen(player));

                    err = jlstream_start(player->stream);
                    if (!err) {
                        break;
                    }
                } else if (err == 0) {
                    player->wait = wait_completion_timeout_add_to_task("app_core", net_download_check_ready, __tts_net_completion_callback, (void *)player->player_id, player->file, 10000);
                    if (player->wait) {
                        break;
                    }
                }
            }
        }

        if (!file) {
            jlstream_release(player->stream);

            list_del(&player->entry);

            if (event == STREAM_EVENT_STOP && player->callback) {
                player->callback(player->priv, STREAM_EVENT_END);
            }

__try_play_next_tts:
            if (!list_empty(&g_head)) {
                struct tts_player *p = list_first_entry(&g_head, struct tts_player, entry);
                int ret = tts_player_start(p, 1);
                if (ret) { //提示音播放失败，继续查询下一个
                    log_info("try_play_next_tts");
                    goto __try_play_next_tts;
                }
            }

            tts_player_free(player, 1);
        }

        os_mutex_post(&g_tts_mutex);
        break;
    }
}

static int tts_file_read(void *p_file, u8 *buf, int len)
{
    int offset = 0;
    struct tts_player *player = (struct tts_player *)p_file;

    while (len) {
        if (!player->file) {
            break;
        }
        int rlen = net_download_read(player->file, buf + offset, len);
        if (rlen < 0) {
            break;
        }
        offset += rlen;
        if ((len -= rlen) == 0) {
            break;
        }
        if (player->next_file) {
            break;
        }

        /*
         * 打开文件列表中的下一个文件,检查format是否和当前文件相同
         * 如果相同就继读下一个文件, 可以保证TWS同步播放文件列表情况下的音频同步
         */
        int index = player->index;
        void *file = open_next_file(player);
        if (!file) {
            player->index = index;
            break;
        }

        char name[16] = {"tts_list."};
        const char *music_type = net_download_get_media_type(player->file);
        if (music_type && music_type[0] != '\0') {
            strcpy(name + strlen(name), music_type);
        } else {
            strcpy(name + strlen(name), "mp3");
        }

        struct stream_file_ops file_ops = {
            .read       = (int (*)(void *, u8 *, int))net_download_read,
            .seek       = (int (*)(void *, int, int))net_download_seek,
        };
        struct stream_file_info file_info = {
            .file   = file,
            .fname  = name,
            .ops    = &file_ops,
        };
        struct stream_fmt fmt;
        int err = jldemuxer_get_tone_file_fmt(&file_info, &fmt);
        if (err == 0) {
            if (player->coding_type == fmt.coding_type &&
                player->sample_rate == fmt.sample_rate &&
                player->channel_mode == fmt.channel_mode) {
                net_download_close(player->file);
                player->file = file;
                continue;
            }
        }
        player->next_file = file;
        break;
    }

    return offset;
}

static int tts_file_seek(void *file, int offset, int fromwhere)
{
    struct tts_player *player = (struct tts_player *)file;

    return net_download_seek(player->file, offset, fromwhere);
}

static int tts_file_flen(void *file)
{
    struct tts_player *player = (struct tts_player *)file;
    u32 len = 0;
    if (player->file) {
        len = net_download_get_file_len(player->file);
    }
    return len;
}

static int tts_file_close(void *file)
{
    struct tts_player *player = (struct tts_player *)file;

    if (player->wait) {
        wait_completion_del(player->wait);
        player->wait = 0;
    }
    if (player->file) {
        net_download_close(player->file);
        player->file = NULL;
    }
    if (player->next_file) {
        net_download_close(player->next_file);
        player->next_file = NULL;
    }
    return 0;
}

static int tts_file_get_fmt(void *file, struct stream_fmt *fmt)
{
    struct tts_player *player = (struct tts_player *)file;

    char name[16] = {"tts."};
    const char *music_type = net_download_get_media_type(player->file);
    if (music_type && music_type[0] != '\0') {
        strcpy(name + strlen(name), music_type);
    } else {
        strcpy(name + strlen(name), "mp3");
    }

    struct stream_file_info file_info = {
        .file   = player,
        .fname  = name,
        .ops    = &tts_file_ops,
    };
    int err = jldemuxer_get_tone_file_fmt(&file_info, fmt);
    if (err) {
        player->coding_type = AUDIO_CODING_UNKNOW;
        return err;
    }

    player->coding_type = fmt->coding_type;
    player->sample_rate = fmt->sample_rate;
    player->channel_mode = fmt->channel_mode;

    return 0;
}

static const struct stream_file_ops tts_file_ops = {
    .read       = tts_file_read,
    .seek       = tts_file_seek,
    .close      = tts_file_close,
    .get_fmt    = tts_file_get_fmt,
};

int tts_player_get_cur_time(struct tts_player *player)
{
    os_mutex_pend(&g_tts_mutex, 0);

    if (list_empty(&(g_head))) {          //先判断是否为空
        os_mutex_post(&g_tts_mutex);
        return -1;
    }
    if (!player) {
        player = list_first_entry(&g_head, struct tts_player, entry);
    }
    if (player && player->stream) {
        int ret = jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_GET_CUR_TIME, 0);
        os_mutex_post(&g_tts_mutex);
        return ret;
    }
    os_mutex_post(&g_tts_mutex);

    return -1;
}

int tts_player_get_total_time(struct tts_player *player)
{
    os_mutex_pend(&g_tts_mutex, 0);

    if (list_empty(&(g_head))) {          //先判断是否为空
        os_mutex_post(&g_tts_mutex);
        return -1;
    }
    if (!player) {
        player = list_first_entry(&g_head, struct tts_player, entry);
    }
    if (player && player->stream) {
        int ret = jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_GET_TOTAL_TIME, 0);
        os_mutex_post(&g_tts_mutex);
        return ret;
    }
    os_mutex_post(&g_tts_mutex);

    return -1;
}

static int __tts_net_completion_callback(void *priv, int timeout)
{
    u32 player_id = (u32)priv;

    if (timeout) {
        log_info("tts player start timeout: %d", player_id);
        tts_player_callback(priv, STREAM_EVENT_NEGOTIATE_FAILD);
        return 0;
    }

    os_mutex_pend(&g_tts_mutex, 0);
    if (list_empty(&(g_head))) {
        os_mutex_post(&g_tts_mutex);
        return -1;
    }

    struct tts_player *player = list_first_entry(&g_head, struct tts_player, entry);
    if (player->player_id != player_id) {
        log_warn("tts wait player id not match: %d %d", player->player_id, player_id);
        os_mutex_post(&g_tts_mutex);
        return -1;
    }

    player->wait = 0;

    if (net_download_check_ready(player->file) < 0) {
        os_mutex_post(&g_tts_mutex);
        tts_player_callback(priv, STREAM_EVENT_NEGOTIATE_FAILD);
        return 0;
    }

    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, tts_file_flen(player));

    int err = jlstream_start(player->stream);
    if (err) {
        goto __exit;
    }

    os_mutex_post(&g_tts_mutex);

    return 0;

__exit:
    jlstream_release(player->stream);
    list_del(&player->entry);
    tts_player_free(player, 1);
    os_mutex_post(&g_tts_mutex);

    return err;
}

static int tts_player_start(struct tts_player *player, int notify)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"tts");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_TTS);
    if (!player->stream) {
        goto __exit0;
    }
    u32 player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, tts_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_TTS);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_set_dec_file(player->stream, player, &tts_file_ops);

    if (player->callback) {
        err = player->callback(player->priv, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }

    err = net_download_check_ready(player->file);
    if (err < 0) {
        goto __exit1;
    } else if (err == 0) {
        player->wait = wait_completion_timeout_add_to_task("app_core", net_download_check_ready, __tts_net_completion_callback, (void *)player_id, player->file, 10000);
        if (!player->wait) {
            err = -1;
            goto __exit1;
        }
    } else {
        jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                            NODE_IOC_SET_FILE_LEN, tts_file_flen(player));

        err = jlstream_start(player->stream);
        if (err) {
            goto __exit1;
        }
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    tts_player_free(player, notify);

    return err;
}

static int tts_player_init(struct tts_player *player, const char *url)
{
    void *file = NULL;

    player->ref = 1;

    player->parm.url = url;
    //网络缓冲buf大小
    player->parm.cbuf_size = 100 * 1024;
    //设置网络下载超时
    player->parm.timeout_millsec = 10000;
    player->parm.start_play_threshold = 4 * 1024;

    int err = net_download_open(&file, &player->parm);
    if (err) {
        log_error("tts_player_faild: open err = %d", err);
        return -EINVAL;
    }
    if (!file) {
        log_error("tts_player_faild: %s", url);
        return -EINVAL;
    }
    /* log_info("tts_player: %s_file", net_download_get_media_type(file)); */

    player->index_max   = 1;//非列表播放默认输入1；
    player->file        = file;
    spin_lock(&lock);
    player->player_id   = g_player_id++;
    spin_unlock(&lock);
    player->coexist     = STREAM_COEXIST_AUTO;
    INIT_LIST_HEAD(&player->entry);

    return 0;
}

static struct tts_player *tts_player_create(const char *url)
{
    struct tts_player *player;

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    int err = tts_player_init(player, url);
    if (err) {
        free(player);
        return NULL;
    }

    return player;
}

static int tts_player_add(struct tts_player *player)
{
    os_mutex_pend(&g_tts_mutex, 0);

    if (list_empty(&g_head)) {
        int err = tts_player_start(player, 0);
        if (err) {
            os_mutex_post(&g_tts_mutex);
            return err;
        }
    }
    list_add_tail(&player->entry, &g_head);

    os_mutex_post(&g_tts_mutex);

    return 0;
}

int play_tts_file(const char *url)
{
    struct tts_player *player;

    player = tts_player_create(url);
    if (!player) {
        return -ENOMEM;
    }

    return tts_player_add(player);
}

int play_tts_file_callback(const char *url, void *priv, tone_player_cb_t callback)
{
    struct tts_player *player;

    player = tts_player_create(url);
    if (!player) {
        return -ENOMEM;
    }
    player->priv        = priv;
    player->callback    = callback;

    return tts_player_add(player);
}

static struct tts_player *tts_files_create(const char *const url_list[], u8 file_num)
{
    struct tts_player *player;

    player = tts_player_create(url_list[0]);
    if (!player) {
        return NULL;
    }

    player->index_max = file_num;//列表模式下根据实际传参

    player->url_file_list = (const char **)zalloc(file_num * sizeof(char *));
    if (!player->url_file_list) {
        log_error("url_list malloc fail!!");
        tts_player_free(player, 0);
        return NULL;
    }

    for (int i = 0; i < file_num; i++) {
        player->url_file_list[i] = strdup(url_list[i]);
        if (!player->url_file_list[i]) {
            log_error("url_list strdup fail!!");
            tts_player_free(player, 0);
            return NULL;
        }
    }

    return player;
}

int play_tts_files(const char *const url_list[], u8 file_num)
{
    struct tts_player *player;

    player = tts_files_create(url_list, file_num);
    if (!player) {
        return -EFAULT;
    }

    return tts_player_add(player);
}

int play_tts_files_alone(const char *const url_list[], u8 file_num)
{
    struct tts_player *player;

    player = tts_files_create(url_list, file_num);
    if (!player) {
        return -EFAULT;
    }
    player->coexist = STREAM_COEXIST_DISABLE;

    return tts_player_add(player);
}

int play_tts_files_callback(const char *const url_list[], u8 file_num,
                            void *priv, tone_player_cb_t callback)
{
    struct tts_player *player;

    player = tts_files_create(url_list, file_num);
    if (!player) {
        return -EFAULT;
    }
    player->priv = priv;
    player->callback = callback;

    return tts_player_add(player);
}

int play_tts_files_alone_callback(const char *const url_list[], u8 file_num,
                                  void *priv, tone_player_cb_t callback)
{
    struct tts_player *player;

    player = tts_files_create(url_list, file_num);
    if (!player) {
        return -EFAULT;
    }
    player->priv = priv;
    player->callback = callback;
    player->coexist = STREAM_COEXIST_DISABLE;

    return tts_player_add(player);
}

int play_tts_file_alone(const char *url)
{
    struct tts_player *player;

    player = tts_player_create(url);
    if (!player) {
        return -ENOMEM;
    }
    player->coexist = STREAM_COEXIST_DISABLE;

    return tts_player_add(player);
}

int play_tts_file_alone_callback(const char *url, void *priv,
                                 tone_player_cb_t callback)
{
    struct tts_player *player;

    player = tts_player_create(url);
    if (!player) {
        return -ENOMEM;
    }
    player->coexist     = STREAM_COEXIST_DISABLE;
    player->priv        = priv;
    player->callback    = callback;

    return tts_player_add(player);
}

int tts_player_runing(void)
{
    return !list_empty(&g_head);
}

void tts_player_stop(void)
{
    struct tts_player *player, *n;

    os_mutex_pend(&g_tts_mutex, 0);

    list_for_each_entry_safe(player, n, &g_head, entry) {
        __list_del_entry(&player->entry);
        if (player->stream) {
            if (player->file) {
                net_download_buf_inactive(player->file);
            }
            jlstream_stop(player->stream, 50);
            jlstream_release(player->stream);
        }
        tts_player_free(player, 1);
    }

    os_mutex_post(&g_tts_mutex);
}

static int __tts_player_init(void)
{
    spin_lock_init(&lock);
    os_mutex_create(&g_tts_mutex);
    return 0;
}
__initcall(__tts_player_init);

#endif
