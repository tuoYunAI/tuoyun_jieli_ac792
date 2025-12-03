#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".a2dp_tx_node.data.bss")
#pragma data_seg(".a2dp_tx_node.data")
#pragma const_seg(".a2dp_tx_node.text.const")
#pragma code_seg(".a2dp_tx_node.text")
#endif

#include "jlstream.h"
#include "btstack/avctp_user.h"
#include "classic/hci_lmp.h"
#include "media/audio_base.h"
#include "effects/effects_adj.h"
#include "sync/audio_syncts.h"
#include "codec/sbc_enc.h"
#include "app_config.h"
#include "system/spinlock.h"
#include "asm/rf_coexistence_config.h"


#if TCFG_A2DP_TX_NODE_ENABLE

#define LOG_TAG     		"[A2DP_TX_NODE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"

#define BT_PROTOCOL_A2DP    0
#define BT_PROTOCOL_BIS     1
#define BT_PROTOCOL_CIS     2

#define AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME     1   // 使用系统时钟来做传输时间戳参考
#define TIMESTAMP_USE_AUDIO_JIFFIES                 0   // 是否用杰理时间戳
#if TCFG_WIFI_ENABLE && TCFG_USER_EMITTER_ENABLE
#define A2DP_SEND_ONCE_PACKET_NUM                   4
#else
#define A2DP_SEND_ONCE_PACKET_NUM                   0
#endif


struct bt_tx_param {
    u8 protocol;
    u8 jl_timestamp;
    u8 frame_num;
} __attribute__((packed));

struct a2dp_tx_sync_node {
    u8 trigger;
    void *syncts;
    struct list_head entry;
};

struct a2dp_tx_packet {
    u8 *packet;
    int len;
    int frame_sum;
    u32 ts;
};

#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
struct audio_bt_emitter_timestamp {
    u16 id;
    u16 run_once;
    u32 run_points;
};
#endif

struct a2dp_tx_hdl {
    spinlock_t lock;
    u8 start;
    u8 num;
    u8 reference_network;
    u8 bt_addr[6];
    u16 packet_size;
    u8 first_timestamp;
    u8 channel;
#if A2DP_SEND_ONCE_PACKET_NUM > 1
    u8 temp_packet_num;
    struct a2dp_tx_packet temp_packet[A2DP_SEND_ONCE_PACKET_NUM - 1];
#endif
    u8 *packet;
    u32 timestamp;
    u32 start_timestamp;
    int offset;
    u32 sample_rate;
    u32 sample_offset;
    u32 coding_type;
    u32 packet_sn;
    struct stream_node *node;
    struct bt_tx_param tx_param;
    u32 local_latch_time;
    struct list_head sync_list;
    struct stream_frame *frame;
#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
    u32 sbc_input_len;
    struct audio_bt_emitter_timestamp *ts;
#endif
};

extern void bt_edr_conn_system_clock_init(void *addr, u8 factor);
extern u32 bt_edr_conn_local_to_master_time(void *addr, u32 usec);
extern void *a2dp_sbc_encoder_get_param(u8 *addr);
extern int bt_source_a2dp_send_media_packet(void *priv, u8 *packet, int len, int frame_sum, u32 TS);
extern int bt_get_source_send_a2dp_buf_size();
extern u8 *get_cur_connect_emitter_mac_addr(void);
extern void wifi_psm_run_notify(int power_save);
extern u8 get_a2dp_source_open_flag(void);
extern bool a2dp_sbc_encoder_status_check_ready(u8 *addr);


#if TCFG_WIFI_ENABLE && TCFG_USER_EMITTER_ENABLE
static u8 tx_ok_num;

void bredr_tx_prepare_callback(void *conn, void *bulk, int err)
{
    if (!get_a2dp_source_open_flag()) {
        return;
    }

    if (get_rf_coexistence_config_index() != 10) {
        switch_rf_coexistence_config_table(10);
    }
}

void bredr_tx_result_callback(void *conn, void *bulk, int err)
{
    if (!get_a2dp_source_open_flag()) {
        return;
    }

    if (++tx_ok_num == A2DP_SEND_ONCE_PACKET_NUM) {
        if (get_rf_coexistence_config_index() != 4) {
            switch_rf_coexistence_config_table(4);
        }
        tx_ok_num = 0;
        wifi_psm_run_notify(0);
    }
}
#endif

#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
static void audio_bt_emitter_time_func(void *priv)
{
    struct a2dp_tx_hdl *hdl = (struct a2dp_tx_hdl *)priv;
    spin_lock(&hdl->lock);
    if (hdl->ts) {
        u32 run_once = hdl->ts->run_once;
        hdl->ts->run_points += run_once;
    }
    spin_unlock(&hdl->lock);
}
#endif

#if TIMESTAMP_USE_AUDIO_JIFFIES
static int a2dp_tx_timestamp_init(struct a2dp_tx_hdl *hdl)
{
    if (!hdl->tx_param.jl_timestamp) {
        return 0;
    }

    hdl->local_latch_time = audio_jiffies_usec();
    hdl->start_timestamp = hdl->local_latch_time * TIMESTAMP_US_DENOMINATOR;
    /*r_printf("a2dp tx timestamp init : %u, %u\n", hdl->local_latch_time, hdl->start_timestamp);*/
    hdl->first_timestamp = 1;
    return 0;
}

static void a2dp_tx_timestamp_handler(struct a2dp_tx_hdl *hdl)
{
    int tx_pcm_frames = 0;
    struct a2dp_tx_sync_node *node;

    if (!hdl->tx_param.jl_timestamp) {
        return;
    }

    if (hdl->coding_type == AUDIO_CODING_SBC) {
        tx_pcm_frames = hdl->num * 128;
    } else if (hdl->coding_type == AUDIO_CODING_AAC) {
        tx_pcm_frames = 1024;
    }

    hdl->sample_offset += tx_pcm_frames;

    if (hdl->sample_offset >= hdl->sample_rate) {
        hdl->sample_offset -= hdl->sample_rate;
        hdl->start_timestamp += PCM_SAMPLE_ONE_SECOND;
        hdl->local_latch_time += 1000000L;
    }

    u32 time = hdl->local_latch_time + ((u64)hdl->sample_offset * 1000000) / hdl->sample_rate;

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->trigger) {
            node->trigger = 1;
            sound_pcm_syncts_latch_trigger(node->syncts);
        }
        sound_pcm_update_frame_num(node->syncts, tx_pcm_frames);
        if (audio_syncts_latch_enable(node->syncts)) {
            sound_pcm_update_frame_num_and_time(node->syncts, 0, time, 0);
        }
    }
}
#endif

static void a2dp_tx_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct a2dp_tx_hdl *hdl = (struct a2dp_tx_hdl *)iport->node->private_data;

    while (1) {
        if (hdl->frame == NULL) {
            hdl->frame = jlstream_pull_frame(iport, note);
        }
        if (!hdl->frame) {
            break;
        }

#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
        if (hdl->ts->run_points < hdl->sbc_input_len) {
            break;
        }
#endif

        if (hdl->offset + hdl->frame->len > hdl->packet_size) {
            /* g_printf("send : %d, %d\n", hdl->num, hdl->offset); */
            hdl->packet_sn++;
            hdl->timestamp = hdl->packet_sn * (128 * hdl->num);
            bt_source_a2dp_send_media_packet(hdl, hdl->packet, hdl->offset, hdl->num, hdl->timestamp);
#if TIMESTAMP_USE_AUDIO_JIFFIES
            a2dp_tx_timestamp_handler(hdl);
#endif
            hdl->offset = 0;
            hdl->num = 0;
        }

        if (hdl->offset == 0) {
            if (hdl->tx_param.jl_timestamp == 1) {
#if TIMESTAMP_USE_AUDIO_JIFFIES
                if (hdl->first_timestamp) {
                    int time_diff = hdl->frame->timestamp - hdl->start_timestamp;
                    time_diff /= TIMESTAMP_US_DENOMINATOR;
                    hdl->local_latch_time += time_diff;

                    hdl->start_timestamp = bt_edr_conn_local_to_master_time(hdl->bt_addr, hdl->frame->timestamp);
                    hdl->first_timestamp = 0;
                    hdl->sample_offset = 0;
                    /*r_printf("start : %u, %u, %d, %u\n", frame->timestamp, hdl->start_timestamp, time_diff, hdl->local_latch_time);*/
                } else {
                    hdl->timestamp = hdl->start_timestamp + PCM_SAMPLE_TO_TIMESTAMP(hdl->sample_offset, hdl->sample_rate);
                }
#endif
            } else {
                hdl->timestamp = hdl->frame->timestamp;
            }
        }
        memcpy(hdl->packet + hdl->offset, hdl->frame->data, hdl->frame->len);
        hdl->num++;
        hdl->offset += hdl->frame->len;
        if (hdl->num >= hdl->tx_param.frame_num) {
            /* g_printf("send2 : %d, %d\n", hdl->num, hdl->offset); */
            hdl->packet_sn++;
            hdl->timestamp = hdl->packet_sn * (128 * hdl->num);
#if A2DP_SEND_ONCE_PACKET_NUM > 1
            if (hdl->temp_packet_num == A2DP_SEND_ONCE_PACKET_NUM - 1) {
#if TCFG_WIFI_ENABLE && TCFG_USER_EMITTER_ENABLE
                tx_ok_num = 0;
#endif
                for (int i = 0; i < hdl->temp_packet_num; ++i) {
                    bt_source_a2dp_send_media_packet(hdl, hdl->temp_packet[i].packet, hdl->temp_packet[i].len, hdl->temp_packet[i].frame_sum, hdl->temp_packet[i].ts);
                }
                bt_source_a2dp_send_media_packet(hdl, hdl->packet, hdl->offset, hdl->num, hdl->timestamp);
                hdl->temp_packet_num = 0;
            } else {
                memcpy(hdl->temp_packet[hdl->temp_packet_num].packet, hdl->packet, hdl->offset);
                hdl->temp_packet[hdl->temp_packet_num].len = hdl->offset;
                hdl->temp_packet[hdl->temp_packet_num].frame_sum = hdl->num;
                hdl->temp_packet[hdl->temp_packet_num].ts = hdl->timestamp;
                ++hdl->temp_packet_num;
#if TCFG_WIFI_ENABLE && TCFG_USER_EMITTER_ENABLE
                if (hdl->temp_packet_num == A2DP_SEND_ONCE_PACKET_NUM - 1) {
                    sys_hi_timeout_add((void *)1, (void (*)(void *))wifi_psm_run_notify, hdl->sbc_input_len * 1000 * hdl->tx_param.frame_num / hdl->sample_rate / 2 / hdl->channel - 5);
                }
#endif
            }
#else
            bt_source_a2dp_send_media_packet(hdl, hdl->packet, hdl->offset, hdl->num, hdl->timestamp);
#endif
#if TIMESTAMP_USE_AUDIO_JIFFIES
            a2dp_tx_timestamp_handler(hdl);
#endif
            hdl->offset = 0;
            hdl->num = 0;
        }

#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
        spin_lock(&hdl->lock);
        if (hdl->ts->run_points >= hdl->sbc_input_len) {
            hdl->ts->run_points -= hdl->sbc_input_len;
        } else {
            ASSERT((hdl->ts->run_points >= hdl->sbc_input_len));
        }
        spin_unlock(&hdl->lock);
#endif
        jlstream_free_frame(hdl->frame);
        hdl->frame = NULL;
    }
}

static int a2dp_tx_bind(struct stream_node *node, u16 uuid)
{
    struct a2dp_tx_hdl *hdl = (struct a2dp_tx_hdl *)node->private_data ;
    hdl->reference_network = 0xff;

    INIT_LIST_HEAD(&hdl->sync_list);

    return 0;
}

static void a2dp_tx_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = a2dp_tx_handle_frame;
}

static void a2dp_tx_set_bt_addr(struct a2dp_tx_hdl *hdl, void *bt_addr)
{
    memcpy(hdl->bt_addr, bt_addr, 6);

#if TIMESTAMP_USE_AUDIO_JIFFIES
    if (hdl->tx_param.jl_timestamp == 1) {
        bt_edr_conn_system_clock_init(hdl->bt_addr, TIMESTAMP_US_DENOMINATOR);
    }
#endif
}

static u32 get_a2dp_tx_sr(u8 sbc_freqency)
{
    /* #define SBC_FREQ_16000		0x00 */
    /* #define SBC_FREQ_32000		0x01 */
    /* #define SBC_FREQ_44100		0x02 */
    /* #define SBC_FREQ_48000		0x03 */

    u32 sr[] = {16000, 32000, 44100, 48000};
    if (sbc_freqency >= ARRAY_SIZE(sr)) {
        log_error("a2dp sbc get sample_rate error %d", sbc_freqency);
    }
    return sr[sbc_freqency];
}

static u8 get_a2dp_channel_mode(u8 mode)
{
    /* [> channel mode <] */
    /* #define SBC_MODE_MONO		0x00 */
    /* #define SBC_MODE_DUAL_CHANNEL	0x01 */
    /* #define SBC_MODE_STEREO		0x02 */
    /* #define SBC_MODE_JOINT_STEREO	0x03 */

    if (mode == SBC_MODE_MONO) {
        return AUDIO_CH_MIX;
    } else {
        return AUDIO_CH_LR;
    }
}

static int a2dp_tx_ioc_fmt_nego(struct stream_iport *iport)
{
    void *bt_addr = get_cur_connect_emitter_mac_addr();
    if (!bt_addr) {
        return 0;     // 若无蓝牙发射连接，直接返回
    }

    if (!a2dp_sbc_encoder_status_check_ready(bt_addr)) {
        return 0;
    }

    struct a2dp_tx_hdl *hdl = (struct a2dp_tx_hdl *)iport->node->private_data;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    int ret = NEGO_STA_ACCPTED;

    hdl->coding_type = in_fmt->coding_type;

    if (hdl->coding_type == AUDIO_CODING_AAC) {
        /*TDDO : AAC格式发送*/
    }

    sbc_t param = {0};
    sbc_t *sbc_param = a2dp_sbc_encoder_get_param(hdl->bt_addr);
    if (!sbc_param) {
        log_info("no sbc param, use initlized param");
        sbc_param = &param;
        sbc_param->frequency = SBC_FREQ_44100;    // 给sbc_param初值,防止未连接emitter的时候nego不过
        sbc_param->blocks = SBC_BLK_16;
        sbc_param->subbands = SBC_SB_8;
        sbc_param->mode = SBC_MODE_STEREO;
        sbc_param->allocation = 0;
        sbc_param->endian = SBC_LE;
        sbc_param->bitpool = 38;
    }

    u32 sample_rate = get_a2dp_tx_sr(sbc_param->frequency);
    if (in_fmt->sample_rate != sample_rate) {
        in_fmt->sample_rate = sample_rate;
        ret = NEGO_STA_CONTINUE;
    }

    u8 channel_mode = get_a2dp_channel_mode(sbc_param->mode);
    if (in_fmt->channel_mode != channel_mode) {
        in_fmt->channel_mode = channel_mode;
        ret = NEGO_STA_CONTINUE;
    }

    u32 bit_rate = ((sbc_param->bitpool) | (sbc_param->mode << 8) | (sbc_param->blocks << 16) | (sbc_param->subbands << 20) | (sbc_param->allocation << 24) | (sbc_param->endian << 28));
    if (in_fmt->bit_rate != bit_rate) {//sbc编码使用
        log_info("bit_rate 0x%x, 0x%x", in_fmt->bit_rate, bit_rate);
        //TODO
        /* in_fmt->bit_rate = bit_rate; */
        /* ret = NEGO_STA_CONTINUE; */
    }

    hdl->sample_rate = in_fmt->sample_rate;
    /* printf("====bit_rate %x\n", bit_rate); */
    /* printf("a2dp_tx frequency %d, sr %d, bitpool %d, mode %d, channle_mode %x,allocation %d, blocks %d, subbands %d, endian %d\n", sbc_param->frequency, sample_rate, sbc_param->bitpool, sbc_param->mode, channel_mode, sbc_param->allocation, sbc_param->blocks, sbc_param->subbands, sbc_param->endian); */

    // 调整sbc编码参数与耳机sbc参数一致
    stream_node_ioctl(iport->prev->node, NODE_UUID_ENCODER, NODE_IOC_SET_PARAM, (int)sbc_param);

    return ret;
}

static int a2dp_tx_get_fmt(struct a2dp_tx_hdl *hdl, struct stream_fmt *fmt)
{
    if (!hdl) {
        return -1;
    }

    sbc_t *sbc_param = a2dp_sbc_encoder_get_param(hdl->bt_addr);
    if (!sbc_param) {
        return -1;
    }

    fmt->sample_rate = get_a2dp_tx_sr(sbc_param->frequency);

    return 0;
}

static int bt_a2dp_get_packet_frame_num(void)
{
    return 7;
}

static int a2dp_tx_start(struct a2dp_tx_hdl *hdl)
{
#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
    sbc_t *sbc_param = a2dp_sbc_encoder_get_param(hdl->bt_addr);
    u8 subbands = 0;
    u8 blocks = 0;
    u8 channels = 0;
    if (sbc_param) {
        channels = sbc_param->mode == SBC_MODE_MONO ? 1 : 2;
        subbands = sbc_param->subbands ? 8 : 4;
        blocks = ((sbc_param->blocks) + 1) * 4;
    }
    hdl->sbc_input_len = subbands * blocks * channels * 2;

    if (!hdl->ts) {
        hdl->ts = zalloc(sizeof(struct audio_bt_emitter_timestamp));
        if (!hdl->ts) {
            return -1;
        }
    }
    hdl->channel = channels;
    hdl->ts->run_points = 0;
    hdl->ts->run_once = (hdl->sample_rate * 20 / 1000) * channels * 2;
    hdl->ts->id = sys_s_hi_timer_add((void *)hdl, audio_bt_emitter_time_func, 20);
#endif

    hdl->start = 1;

    hdl->packet_size = bt_get_source_send_a2dp_buf_size();

    //根据协商设置组帧数
    if (bt_a2dp_get_packet_frame_num()) {
        hdl->tx_param.frame_num = bt_a2dp_get_packet_frame_num();
    }

    ASSERT(hdl->packet_size != 0);
    hdl->packet = malloc(hdl->packet_size);
    if (!hdl->packet) {
        log_error("a2dp tx packet buffer error.");
    }
    log_info("a2dp tx packet size : %d", hdl->packet_size);
#if A2DP_SEND_ONCE_PACKET_NUM > 1
    hdl->temp_packet_num = 0;

    for (int i = 0; i < A2DP_SEND_ONCE_PACKET_NUM - 1; ++i) {
        hdl->temp_packet[i].packet = malloc(hdl->packet_size);
        if (!hdl->temp_packet[i].packet) {
            break;
        }
    }
#endif
#if TCFG_WIFI_ENABLE && TCFG_USER_EMITTER_ENABLE
    rf_coexistence_scene_enter(RF_COEXISTENCE_SCENE_A2DP_SOURCE, -1);
#endif
#if TIMESTAMP_USE_AUDIO_JIFFIES
    a2dp_tx_timestamp_init(hdl);
#endif
    hdl->packet_sn = 0;
    hdl->offset = 0;
    hdl->num = 0;
    return 0;
}

static int a2dp_tx_stop(struct a2dp_tx_hdl *hdl)
{
    hdl->start = 0;

#if A2DP_SEND_ONCE_PACKET_NUM > 1
    if (hdl->temp_packet_num > 0) {
        for (int i = 0; i < hdl->temp_packet_num; ++i) {
            bt_source_a2dp_send_media_packet(hdl, hdl->temp_packet[i].packet, hdl->temp_packet[i].len, hdl->temp_packet[i].frame_sum, hdl->temp_packet[i].ts);
        }
        hdl->temp_packet_num = 0;
    }
    for (int i = 0; i < A2DP_SEND_ONCE_PACKET_NUM - 1; ++i) {
        if (hdl->temp_packet[i].packet) {
            free(hdl->temp_packet[i].packet);
            hdl->temp_packet[i].packet = NULL;
        }
    }
#endif

    if (hdl->packet) {
        free(hdl->packet);
        hdl->packet = NULL;
    }

#if AUDIO_BT_EMITTER_TIMESTAMP_USE_SYS_TIME
    spin_lock(&hdl->lock);
    if (hdl->ts) {
        if (hdl->ts->id) {
            sys_s_hi_timer_del(hdl->ts->id);
        }
        free(hdl->ts);
        hdl->ts = NULL;
    }
    spin_unlock(&hdl->lock);
#endif

    if (hdl->frame) {
        jlstream_free_frame(hdl->frame);
        hdl->frame = NULL;
    }

#if TCFG_WIFI_ENABLE && TCFG_USER_EMITTER_ENABLE
    rf_coexistence_scene_exit(RF_COEXISTENCE_SCENE_A2DP_SOURCE);
    wifi_psm_run_notify(0);
#endif

    return 0;
}

static int a2dp_tx_mount_syncts(struct a2dp_tx_hdl *hdl, void *syncts, u32 timestamp, u8 network)
{
    struct a2dp_tx_sync_node *node = NULL;

#if TIMESTAMP_USE_AUDIO_JIFFIES
    if (!hdl->tx_param.jl_timestamp) {
        return 0;
    }
#endif

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->syncts == (u32)syncts) {
            return 0;
        }
    }

    node = (struct a2dp_tx_sync_node *)zalloc(sizeof(struct a2dp_tx_sync_node));
    if (!node) {
        return -1;
    }
    node->syncts = syncts;

    /*g_printf("a2dp tx mount syncts : 0x%x, %u\n", (u32)syncts, timestamp);*/
    if (hdl->reference_network == 0xff) {
        hdl->reference_network = network;
    }
    list_add(&node->entry, &hdl->sync_list);

    return 0;
}

static void a2dp_tx_unmount_syncts(struct a2dp_tx_hdl *hdl, void *syncts)
{
    struct a2dp_tx_sync_node *node;

#if TIMESTAMP_USE_AUDIO_JIFFIES
    if (!hdl->tx_param.jl_timestamp) {
        return;
    }
#endif

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->syncts == syncts) {
            goto unmount;
        }
    }

    return;

unmount:
    /*g_printf("a2dp tx unmount syncts : 0x%x\n", syncts);*/
    list_del(&node->entry);
    free(node);
}

static int a2dp_tx_syncts_handler(struct a2dp_tx_hdl *hdl, struct audio_syncts_ioc_params *params)
{
    if (!params) {
        return 0;
    }

    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        a2dp_tx_mount_syncts(hdl, (void *)params->data[0], params->data[1], params->data[2]);
        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        a2dp_tx_unmount_syncts(hdl, (void *)params->data[0]);
        break;
    }

    return 0;
}

static int a2dp_tx_buffer_latency(struct a2dp_tx_hdl *hdl)
{
    return 0;
}

static int a2dp_tx_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct a2dp_tx_hdl *hdl = (struct a2dp_tx_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        a2dp_tx_open_iport(iport);
        break;
    case NODE_IOC_SET_BTADDR:
        a2dp_tx_set_bt_addr(hdl, (void *)arg);
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_START:
        a2dp_tx_start(hdl);
        break;
    case NODE_IOC_GET_FMT:
        a2dp_tx_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        a2dp_tx_stop(hdl);
        break;
    case NODE_IOC_SYNCTS:
        a2dp_tx_syncts_handler(hdl, (struct audio_syncts_ioc_params *)arg);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= a2dp_tx_ioc_fmt_nego(iport);
        break;
    case NODE_IOC_GET_DELAY:
        return a2dp_tx_buffer_latency(hdl);;
    }

    return 0;
}

static void a2dp_tx_release(struct stream_node *node)
{
    struct a2dp_tx_hdl *hdl = (struct a2dp_tx_hdl *)node->private_data;

    if (hdl) {
        free(hdl);
    }
}

REGISTER_STREAM_NODE_ADAPTER(a2dp_tx_adapter) = {
    .name       = "a2dp_tx",
    .uuid       = NODE_UUID_A2DP_TX,
    .bind       = a2dp_tx_bind,
    .ioctl      = a2dp_tx_ioctl,
    .release    = a2dp_tx_release,
    .hdl_size   = sizeof(struct a2dp_tx_hdl),
};

#endif
