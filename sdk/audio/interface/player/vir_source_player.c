#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".vir_source_player.data.bss")
#pragma data_seg(".vir_source_player.data")
#pragma const_seg(".vir_source_player.text.const")
#pragma code_seg(".vir_source_player.text")
#endif

#include "system/init.h"
#include "vir_source_player.h"

#define LOG_TAG             "[VIR_SOURCE_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#undef LOG_TAG_CONST
#include "system/debug.h"

struct virtual_source_player_hdl {
    OS_MUTEX mutex;
    struct list_head head;
};

static struct virtual_source_player_hdl g_vir_source_player;

static void vir_source_player_callback(void *private_data, int event)
{
    struct vir_source_player *player = (struct vir_source_player *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int vir_source_player_pp(void *player_)
{
    log_debug("player pp toggle!");
    if (!player_) {
        log_error("palyer ptr NULL !");
        return -1;
    }
    struct vir_source_player *player = player_;

    if (player && player->stream) {
        jlstream_pp_toggle(player->stream, 50);
        player->playing = !player->playing;
        return 0;
    } else {
        log_error("palyer ptr NULL !");
        return -1;
    }
}

int vir_source_player_pause(void *player_)
{
    log_debug("player pause !");
    if (!player_) {
        log_error("palyer ptr NULL !");
        return -1;
    }
    struct vir_source_player *player = player_;

    if (!player->playing) {
        return 0;
    }

    return vir_source_player_pp(player);
}

int vir_source_player_resume(void *player_)
{
    log_debug("player resume !");
    if (!player_) {
        log_error("palyer ptr NULL !");
        return -1;
    }
    struct vir_source_player *player = player_;

    if (player->playing) {
        return 0;
    }

    return vir_source_player_pp(player);
}

void *vir_source_player_open(void *file, struct stream_file_ops *ops)
{
    log_debug("player open !");
    int err;
    struct vir_source_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"vir_source");
    if (uuid == 0) {
        log_error("get uuid err");
        return NULL;
    }

    player = zalloc(sizeof(struct vir_source_player));
    if (!player) {
        log_error("zalloc fail!");
        return NULL;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SOURCE_DEV4);
    if (!player->stream) {
        log_error("pipeline parse err");
        err = -ENOMEM;
        goto __exit0;
    }

    jlstream_set_callback(player->stream, player, vir_source_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_DEV_FLOW);
    jlstream_set_dec_file(player->stream, file, ops);

    os_mutex_pend(&g_vir_source_player.mutex, 0);
    list_add_tail(&player->entry, &(g_vir_source_player.head));
    os_mutex_post(&g_vir_source_player.mutex);

    return (void *)player;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return NULL;
}

int vir_source_player_start(void *player_)
{
    if (!player_) {
        log_error("palyer ptr NULL !");
        return -1;
    }
    struct vir_source_player *player = player_;
    log_debug("player start !");
    int err = 0;

    err = jlstream_start(player->stream);
    if (err) {
        log_error("player start fail");
    }
    player->playing = 1;

    return err;
}

void vir_source_player_close(void *player_)
{
    log_debug("player close !");
    if (!player_) {
        log_error("palyer ptr NULL !");
        return;
    }
    struct vir_source_player *player = player_;
    struct vir_source_player *p, *n;

    os_mutex_pend(&g_vir_source_player.mutex, 0);
    list_for_each_entry_safe(p, n, &g_vir_source_player.head, entry) {
        if (p == player) {
            __list_del_entry(&player->entry);
            os_mutex_post(&g_vir_source_player.mutex);
            goto __stop;
        }
    }
    os_mutex_post(&g_vir_source_player.mutex);

    log_error("not found player close !");
    return;

__stop:
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);
    free(player);
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"vir_source");
}

static int vir_source_player_init(void)
{
    log_debug("player init !");
    INIT_LIST_HEAD(&g_vir_source_player.head);
    os_mutex_create(&g_vir_source_player.mutex);
    return 0;
}
__initcall(vir_source_player_init);

