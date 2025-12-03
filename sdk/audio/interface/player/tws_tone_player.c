#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tws_tone_player.data.bss")
#pragma data_seg(".tws_tone_player.data")
#pragma const_seg(".tws_tone_player.text.const")
#pragma code_seg(".tws_tone_player.text")
#endif
#include "tws_tone_player.h"
#include "os/os_api.h"
#include "system/timer.h"
#include "fs/resfile.h"
#include "sync/audio_syncts.h"
#include "media/bt_audio_timestamp.h"
#include "reference_time.h"
#include "classic/tws_api.h"
#include "app_config.h"

#if TCFG_USER_TWS_ENABLE

#define LOG_TAG             "[TWS_TONE_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"


#define MAX_FNAME_LEN 32

struct tws_tone_message {
    enum stream_scene scene;
    enum stream_coexist coexist;
    u8 preemption;
    u8 file_num;
    u8 network;
    u8 reference_clk;
    u8 net_addr[6];
    u32 timestamp;
    u32 tws_timestamp;
    u32 func_uuid;
    char name[MAX_FILE_NUM][MAX_FNAME_LEN];
};

struct tws_tone_file {
    enum stream_scene scene;
    enum stream_coexist coexist;
    u16 delay_time;
    u32 func_uuid;
    char name[MAX_FNAME_LEN];
};

struct tws_tone_player {
    struct tone_player player;
    u8 reference;
    u8 network;
    u8 net_addr[6];
    u32 timestamp;
    u32 tws_timestamp;
    u32 func_uuid;
    char file_list[MAX_FILE_NUM][MAX_FNAME_LEN];
};

extern const int config_tws_tone_player_reference_clock;

static OS_MUTEX g_tws_tone_mutex;
static struct tone_player *g_tws_tone_player;
static u8 g_tws_tone_adding = 0;

static void tws_tone_reference_clock_close(u8 id);
static void tws_play_tone_try_timeout(void *arg);
extern int ring_player_start(struct tone_player *player);
extern int tws_conn_system_clock_init(u8 factor);
extern u32 tws_conn_master_to_local_time(u32 usec);

static int __tws_tone_player_init(void)
{
    return os_mutex_create(&g_tws_tone_mutex);
}
early_initcall(__tws_tone_player_init);

static bool tws_tone_player_runing(void)
{
    return g_tws_tone_player ? TRUE : FALSE;
}

static int tws_player_callback(int fname_uuid, u32 func_uuid, enum stream_event event)
{
    const struct tws_tone_callback *p;

    for (p = tws_tone_cb_begin; p < tws_tone_cb_end; p++) {
        if (p->func_uuid == func_uuid) {
            if (p->callback) {
                p->callback(fname_uuid, event);
            }
            break;
        }
    }
    return 0;
}

static int tws_files_callback(void *priv, enum stream_event event)
{
    return tws_player_callback(0, (u32)priv, event);
}

static void play_timestamp_init(struct tws_tone_player *player)
{
    if (config_tws_tone_player_reference_clock == 1) {
        if (!player->tws_timestamp) {
            return;
        }
        tws_conn_system_clock_init(TIMESTAMP_US_DENOMINATOR);
        player->timestamp = tws_conn_master_to_local_time((player->tws_timestamp * 625 * TIMESTAMP_US_DENOMINATOR));
        jlstream_node_ioctl(player->player.stream, NODE_UUID_DECODER,
                            NODE_IOC_SET_TIME_STAMP, player->timestamp);
        return;
    }

    if (!player->timestamp) {
        return;
    }

    if (!player->reference) {
        player->reference = audio_reference_clock_select(player->net_addr,
                            player->network);
        if (!player->reference) {
            return;
        }
    }

    if (!audio_reference_network_exist(player->reference) && player->network == 0) {
        audio_reference_clock_exit(player->reference);
        player->network = 1;
        player->reference = audio_reference_clock_select(NULL, player->network);
        player->timestamp = player->tws_timestamp;
    }

    u8 current_network = audio_reference_clock_network(NULL);
    if (current_network != (u8) - 1 && player->network != current_network) {
        audio_reference_clock_exit(player->reference);
        player->timestamp = audio_reference_clock_remapping(player->network,
                            current_network, player->timestamp);
        current_network = audio_reference_clock_network(player->net_addr);
        player->reference = audio_reference_clock_select(current_network == 0 ?
                            player->net_addr : NULL, current_network);
    }

    if (player->timestamp && player->timestamp != (u32) - 1) {
        jlstream_node_ioctl(player->player.stream, NODE_UUID_DECODER,
                            NODE_IOC_SET_TIME_STAMP, (player->timestamp * 625 * TIMESTAMP_US_DENOMINATOR));
    }
}

static int tws_tone_player_callback(void *_player, enum stream_event event)
{
    int err = 0;
    struct tws_tone_player *player = (struct tws_tone_player *)_player;

    switch (event) {
    case STREAM_EVENT_INIT:
        play_timestamp_init(player);
        break;
    case STREAM_EVENT_START:
        if (player->func_uuid) {
            tws_player_callback(player->player.fname_uuid, player->func_uuid, event);
        }
        break;
    case STREAM_EVENT_STOP:
        if (player->reference) {
            tws_tone_reference_clock_close(player->reference);
        }
        int func_uuid = player->func_uuid;
        int fname_uuid = player->player.fname_uuid;
        free(player);
        if (func_uuid) {
            tws_player_callback(fname_uuid, func_uuid, event);
        }
        break;
    case STREAM_EVENT_END:
        break;
    default:
        break;
    }
    return 0;
}

static void *open_next_file(struct tone_player *player)
{
    if (player->next_file) {
        void *file = player->next_file;
        player->next_file = NULL;
        return file;
    }

    int index = ++player->index;

    do {
        if (player->file_name_list[index]) {
            char file_path[48];

            strcpy(file_path, FLASH_RES_PATH);
            strcpy(file_path + strlen(FLASH_RES_PATH), player->file_name_list[index]);
            void *file = resfile_open(file_path);

            if (file) {
                player->index = index;
                return file;
            }
            index++;
        } else {
            player->index = 0;
            break;
        }
    } while (index != player->index);

    return NULL;
}

static void tone_player_callback(void *_player_id, int event)
{
    void *file = NULL;
    struct tone_player *player;

    switch (event) {
    case STREAM_EVENT_START:
        os_mutex_pend(&g_tws_tone_mutex, 0);
        player = g_tws_tone_player;
        if (player && player->player_id == (u32)_player_id) {
            if (player->callback) {
                player->callback(player->priv, STREAM_EVENT_START);
            }
        }
        os_mutex_post(&g_tws_tone_mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        os_mutex_pend(&g_tws_tone_mutex, 0);
        player = g_tws_tone_player;
        if (!player || player->player_id != (u32)_player_id) {
            os_mutex_post(&g_tws_tone_mutex);
            break;
        }

        if (event == STREAM_EVENT_STOP) {
            /*
             * 打开文件列表的下一个文件,重新启动解码
             */
            while (1) {
                file = open_next_file(player);
                if (!file) {
                    break;
                }
                resfile_close(player->file);

                player->file = file;

                jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_FILE_LEN, resfile_get_len(player->file));

                int err = jlstream_start(player->stream);
                if (err == 0) {
                    break;
                }
            }
        }

        if (!file) {
            jlstream_release(player->stream);
            g_tws_tone_player = NULL;

            if (event == STREAM_EVENT_STOP && player->callback) {
                player->callback(player->priv, STREAM_EVENT_END);
            }

            tone_player_free(player);
        }

        os_mutex_post(&g_tws_tone_mutex);
        break;
    }
}

static int tws_tone_player_start(struct tone_player *player)
{
    int err = -EINVAL;

    os_mutex_pend(&g_tws_tone_mutex, 0);

    if (g_tws_tone_player) {
        goto __exit;
    }

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"tws_tone");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_TWS_TONE);
    if (!player->stream) {
        goto __exit;
    }

    u32 player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, tone_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    if (player->file) {
        jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_FILE_LEN, resfile_get_len(player->file));
    }

    if (player->callback) {
        err = player->callback(player->priv, STREAM_EVENT_INIT);
        if (err) {
            goto __exit;
        }
    }
    jlstream_set_dec_file(player->stream, player, &tone_file_ops);

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit;
    }

    g_tws_tone_player = player;

    os_mutex_post(&g_tws_tone_mutex);

    return 0;

__exit:
    os_mutex_post(&g_tws_tone_mutex);

    if (player->stream) {
        jlstream_release(player->stream);
    }

    tone_player_free(player);

    return err;
}

static void tws_tone_player_add_timeout(void *arg)
{
    struct tws_tone_player *player = (struct tws_tone_player *)arg;

    int offset = 30 * 1000 / 625;
    player->timestamp += offset;
    player->tws_timestamp += offset;

    if (!tws_tone_player_runing() && !tone_player_runing()) {
        tws_tone_player_start(&player->player);
        g_tws_tone_adding = 0;
    } else {
        sys_timeout_add((void *)player, tws_tone_player_add_timeout, 30);
    }
}

static void tws_tone_play_in_task(struct tws_tone_message *msg)
{
    struct tws_tone_player *player;
    const char *file_name = msg->name[0];

    if (msg->preemption) {
        tone_player_stop();
    }

    log_info("tws_tone_play: %x, %s", msg->timestamp, file_name);

    player = zalloc(sizeof(*player));
    if (!player) {
        goto __exit0;
    }

    for (int i = 0; i < msg->file_num; i++) {
        strcpy(player->file_list[i], msg->name[i]);
        player->player.file_name_list[i] = player->file_list[i];
    }

    int err = tone_player_init(&player->player, player->file_list[0]);
    if (err) {
        free(player);
        goto __exit0;
    }

    player->player.ref      = 2;
    player->player.priv     = player;
    player->player.callback = tws_tone_player_callback;
    player->player.scene    = msg->scene;
    player->player.coexist  = msg->coexist;
    player->player.play_by_tws  = 1;

    player->func_uuid       = msg->func_uuid;
    player->timestamp       = msg->timestamp;
    player->tws_timestamp   = msg->tws_timestamp;
    player->network         = msg->network;
    player->reference       = msg->reference_clk;
    memcpy(player->net_addr, msg->net_addr, 6);

    if (msg->scene == STREAM_SCENE_RING) {
        ring_player_start(&player->player);
    } else {
        player->player.scene = STREAM_SCENE_TWS_TONE;
        if ((!tws_tone_player_runing() && !tone_player_runing()) || player->player.coexist == STREAM_COEXIST_DISABLE) {
            tws_tone_player_start(&player->player);
            g_tws_tone_adding = 0;
        } else {
            sys_timeout_add((void *)player, tws_tone_player_add_timeout, 30);
        }
    }
    goto __exit1;

__exit0:
    g_tws_tone_adding = 0;
    tws_tone_reference_clock_close(msg->reference_clk);
    tws_files_callback((void *)(msg->func_uuid), STREAM_EVENT_STOP);
__exit1:
    free(msg);
}

static void tws_tone_file_data_in_irq(void *data, u16 len, bool rx)
{
    int msg[4];

    u8 *buf = malloc(len);
    if (!buf) {
        goto __err;
    }
    memcpy(buf, data, len);

    msg[0] = (int)tws_tone_play_in_task;
    msg[1] = 1;
    msg[2] = (int)buf;

    if (rx) {
        ((struct tws_tone_message *)buf)->reference_clk = 0;
    }

    int err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
    if (err == OS_ERR_NONE) {
        return;
    }
    if (buf) {
        free(buf);
        buf = NULL;
    }
    if (rx) {
        return;
    }

__err:
    g_tws_tone_adding = 0;
    struct tws_tone_message *tmsg = (struct tws_tone_message *)data;
    tws_tone_reference_clock_close(tmsg->reference_clk);
}

REGISTER_TWS_FUNC_STUB(tws_tone_player_stub) = {
    .func_id = 0xE70A68F2,
    .func = tws_tone_file_data_in_irq,
};

static void tws_tone_reference_clock_setup(struct tws_tone_message *msg)
{
    if (config_tws_tone_player_reference_clock == 1) {
        return;
    }
    msg->reference_clk = audio_reference_clock_select(NULL, 1);
    msg->network = audio_reference_clock_network(msg->net_addr);
}

static void tws_tone_reference_clock_close(u8 id)
{
    audio_reference_clock_exit(id);
}

static int __play_tws_tone_file_with_calllback(const char *file_name, enum stream_coexist coexist, void *priv, tone_player_cb_t callback)
{
    struct tone_player *player;

    if (g_tws_tone_player) {
        return -EBUSY;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    int err = tone_player_init(player, file_name);
    if (err) {
        free(player);
        return err;
    }

    player->priv = priv;
    player->callback = callback;
    player->scene = STREAM_SCENE_TWS_TONE;
    player->coexist = coexist;

    return tws_tone_player_start(player);
}

static int __play_tws_tone_files_with_calllback(const char *const file_name[], u8 file_num, enum stream_coexist coexist, void *priv, tone_player_cb_t callback)
{
    struct tone_player *player;

    if (g_tws_tone_player) {
        return -EBUSY;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    int err = tone_player_init(player, file_name[0]);
    if (err) {
        free(player);
        return err;
    }

    for (int i = 0; i < file_num; i++) {
        player->file_name_list[i] = file_name[i];
    }

    player->priv = priv;
    player->callback = callback;
    player->scene = STREAM_SCENE_TWS_TONE;
    player->coexist = coexist;

    return tws_tone_player_start(player);
}

static int play_tws_tone_files_with_callback(const char *const file_name[], u8 file_num, void *priv, tone_player_cb_t callback)
{
    return __play_tws_tone_files_with_calllback(file_name, file_num, STREAM_COEXIST_AUTO, priv, callback);
}

static int play_tws_tone_file_alone_with_callback(const char *file_name, void *priv, tone_player_cb_t callback)
{
    return __play_tws_tone_file_with_calllback(file_name, STREAM_COEXIST_DISABLE, priv, callback);
}

static int __tws_play_tone_file(const char *const file_list[], u8 file_num,
                                int delay_msec, enum stream_scene scene,
                                enum stream_coexist coexist,
                                u32 func_uuid, u8 preemption)
{
    struct tws_tone_message msg;

    if (tws_api_is_sniff_state()) {
        tws_api_tx_unsniff_req();
        delay_msec += 1000;
    }

    if (scene == STREAM_SCENE_TWS_TONE &&
        (tws_tone_player_runing() || tone_player_runing() || g_tws_tone_adding)) {
        struct tws_tone_file *file = zalloc(sizeof(*file));
        if (file) {
            strcpy(file->name, file_list[0]);
            file->delay_time    = delay_msec;
            file->scene         = scene;
            file->coexist       = coexist;
            file->func_uuid     = func_uuid;
            sys_timeout_add((void *)file, tws_play_tone_try_timeout, 200);
        }
        return 0;
    }

    int slot = (delay_msec + 1) * 1000 / 625;

    tws_tone_reference_clock_setup(&msg);
    msg.timestamp       = audio_reference_clock_time() + slot;
    msg.tws_timestamp   = audio_reference_network_clock_time(1) + slot;
    msg.scene           = scene;
    msg.coexist         = coexist;
    msg.func_uuid       = func_uuid;
    msg.file_num        = file_num;
    msg.preemption      = preemption;

    for (int i = 0; i < file_num; i++) {
        strcpy(msg.name[i], file_list[i]);
    }
    int data_len = offsetof(struct tws_tone_message, name) + file_num * MAX_FNAME_LEN;

    log_info("tws_play_tone_file: %x, %s, %d", msg.network, file_list[0], data_len);

    int err = tws_api_send_data_to_sibling(&msg, data_len, 0xE70A68F2);

    if (scene == STREAM_SCENE_TWS_TONE) {
        g_tws_tone_adding = err == 0 ? 1 : 0;
    }

    if (err) {
        if (config_tws_tone_player_reference_clock == 0) {
            tws_tone_reference_clock_close(msg.reference_clk);
        }

        if (scene == STREAM_SCENE_RING) {
            if (coexist == STREAM_COEXIST_AUTO) {
                err = play_ring_file(file_list[0]);
            } else {
                err = play_ring_file_alone(file_list[0]);
            }
        } else {
            if (coexist == STREAM_COEXIST_AUTO) {
                err = play_tws_tone_files_with_callback(file_list, file_num, (void *)func_uuid,
                                                        tws_files_callback);
            } else {
                if (msg.preemption) {//tws提示音,发送失败，检查抢占标志，关闭提示音
                    tone_player_stop();
                }

                if (file_num == 1) {
                    err = play_tws_tone_file_alone_with_callback(file_list[0], (void *)func_uuid,
                            tws_files_callback);
                } else {
                    err = play_tws_tone_files_with_callback(file_list, file_num, (void *)func_uuid,
                                                            tws_files_callback);
                }
            }
        }
    }

    return err;
}

static void tws_play_tone_try_timeout(void *arg)
{
    struct tws_tone_file *file = (struct tws_tone_file *)arg;
    if (file) {
        const char *fname = file->name;
        __tws_play_tone_file(&fname, 1, file->delay_time,
                             file->scene, file->coexist, file->func_uuid, 0);
        free(file);
    }
}

int tws_play_tone_file(const char *file_name, int delay_msec)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_AUTO, 0, 0);
}

int tws_play_tone_file_callback(const char *file_name, int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_AUTO, func_uuid, 0);
}

int tws_play_ring_file(const char *file_name, int delay_msec)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_RING,
                                STREAM_COEXIST_AUTO, 0, 0);
}

int tws_play_ring_file_alone(const char *file_name, int delay_msec)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_RING,
                                STREAM_COEXIST_DISABLE, 0, 0);
}

int tws_play_ring_file_callback(const char *file_name, int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_RING,
                                STREAM_COEXIST_AUTO, func_uuid, 0);
}

int tws_play_ring_file_alone_callback(const char *file_name, int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_RING,
                                STREAM_COEXIST_DISABLE, func_uuid, 0);
}

int tws_play_tone_files_callback(const char *const file_name[], u8 file_num,
                                 int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(file_name, file_num, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_AUTO, func_uuid, 0);
}

int tws_play_tone_files_alone_callback(const char *const file_name[], u8 file_num,
                                       int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(file_name, file_num, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_DISABLE, func_uuid, 0);
}

int tws_play_tone_file_alone(const char *file_name, int delay_msec)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_DISABLE, 0, 0);
}

int tws_play_tone_file_alone_callback(const char *file_name, int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_DISABLE, func_uuid, 0);
}
//抢占播放:打断其他播放器,且会先关闭当前提示音，再再播下一个
int tws_play_tone_file_preemption_callback(const char *file_name, int delay_msec, u32 func_uuid)
{
    return __tws_play_tone_file(&file_name, 1, delay_msec, STREAM_SCENE_TWS_TONE,
                                STREAM_COEXIST_DISABLE, func_uuid, 1);
}

#endif
