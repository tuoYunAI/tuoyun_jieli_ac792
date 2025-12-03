#ifndef __UNICAST_SOURCE_API_H__
#define __UNICAST_SOURCE_API_H__

#include "typedef.h"

extern void le_audio_unicast_source_all_init(void);
extern void le_audio_unicast_source_all_uninit(void);

#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_8000_HZ      0x0001
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_11025_HZ     0x0002
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_16000_HZ     0x0004
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_22050_HZ     0x0008
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_24000_HZ     0x0010
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_32000_HZ     0x0020
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_44100_HZ     0x0040
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_48000_HZ     0x0080
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_88200_HZ     0x0100
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_96000_HZ     0x0200
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_176400_HZ    0x0400
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_192000_HZ    0x0800
#define UNICAST_PACS_CODEC_SAMPLING_FREQUENCY_MASK_384000_HZ    0x1000

#define UNICAST_PACS_CODEC_FRAME_DURATION_MASK_7500US               0x01
#define UNICAST_PACS_CODEC_FRAME_DURATION_MASK_10000US              0x02
#define UNICAST_PACS_CODEC_FRAME_DURATION_MASK_7500US_PREFERRED     0x10
#define UNICAST_PACS_CODEC_FRAME_DURATION_MASK_10000US_PREFERRED    0x20

typedef struct codec_param_set_t {
    uint16_t upload_sampling_frequency_mask;
    uint16_t upload_octets_per_frame_num;
    uint16_t download_sampling_frequency_mask;
    uint16_t download_octets_per_frame_num;
    uint8_t  upload_frame_durations_mask;
    uint8_t  download_frame_durations_mask;
} codec_param_set;

extern void le_audio_unicast_source_media_start(u16 conn_handle, codec_param_set *param);
extern void le_audio_unicast_source_media_stop(u16 conn_handle);
extern void le_audio_unicast_source_call_start(u16 conn_handle, codec_param_set *param);
extern void le_audio_unicast_source_call_stop(u16 conn_handle);
extern int  le_audio_unicast_source_scan_enable(u8 enable, u16 scan_int, u16 scan_win, u8 scanning_filter_policy);
extern int  le_audio_unicast_source_disconnect(u16 conn_handle);

#define EVENT_UNICAST_CLIENT_CONNECTED                          0x00000001
#define EVENT_UNICAST_CLIENT_DISCONNECTED                       0x00000002
#define EVENT_UNICAST_CIS_CREATED                               0x00000011
#define EVENT_UNICAST_CIS_TERMINATED                            0x00000012
#define EVENT_UNICAST_CIS_SEND_CALLBACK                         0x00000013
struct unicast_cis_recv_event_t {
    u8 *packet;
    u32 length;
    u16 packet_seqence_number;
    u8  num_channels;
};
#define EVENT_UNICAST_CIS_RECV_CALLBACK                         0x00000014
#define EVENT_UNICAST_CLIENT_ASCS_STATE_IDLE                    0x00000100
#define EVENT_UNICAST_CLIENT_ASCS_STATE_CODEC_CONFIGURED        0x00000101
#define EVENT_UNICAST_CLIENT_ASCS_STATE_QOS_CONFIGURED          0x00000102
#define EVENT_UNICAST_CLIENT_ASCS_STATE_ENABLING                0x00000103
#define EVENT_UNICAST_CLIENT_ASCS_STATE_STREAMING               0x00000104
#define EVENT_UNICAST_CLIENT_ASCS_STATE_DISABLING               0x00000105
#define EVENT_UNICAST_CLIENT_ASCS_STATE_RELEASING               0x00000106
#define EVENT_UNICAST_CLIENT_ASCS_STATE_RFU                     0x00000107
#define EVENT_UNICAST_CLIENT_VCS_STATE_READ                     0x00000200
#define EVENT_UNICAST_CLIENT_VCS_FLAGS_READ                     0x00000201
#define EVENT_UNICAST_CLIENT_VCS_VCP_WRITE                      0x00000202

#define CODEC_SAMPLING_FREQUENCY_MASK_8000_HZ      0x0001
#define CODEC_SAMPLING_FREQUENCY_MASK_11025_HZ     0x0002
#define CODEC_SAMPLING_FREQUENCY_MASK_16000_HZ     0x0004
#define CODEC_SAMPLING_FREQUENCY_MASK_22050_HZ     0x0008
#define CODEC_SAMPLING_FREQUENCY_MASK_24000_HZ     0x0010
#define CODEC_SAMPLING_FREQUENCY_MASK_32000_HZ     0x0020
#define CODEC_SAMPLING_FREQUENCY_MASK_44100_HZ     0x0040
#define CODEC_SAMPLING_FREQUENCY_MASK_48000_HZ     0x0080
#define CODEC_SAMPLING_FREQUENCY_MASK_88200_HZ     0x0100
#define CODEC_SAMPLING_FREQUENCY_MASK_96000_HZ     0x0200
#define CODEC_SAMPLING_FREQUENCY_MASK_176400_HZ    0x0400
#define CODEC_SAMPLING_FREQUENCY_MASK_192000_HZ    0x0800
#define CODEC_SAMPLING_FREQUENCY_MASK_384000_HZ    0x1000

#define CODEC_FRAME_DURATION_MASK_7500US               0x01
#define CODEC_FRAME_DURATION_MASK_10000US              0x02
#define CODEC_FRAME_DURATION_MASK_7500US_PREFERRED     0x10
#define CODEC_FRAME_DURATION_MASK_10000US_PREFERRED    0x20

struct pacs_codec_capability__t {
    uint8_t  codec_capability_mask;
    uint8_t  supported_frame_durations_mask;
    uint8_t  supported_audio_channel_counts_mask;
    uint8_t  frame_blocks_per_sdu_max_num;
    uint16_t octets_per_frame_min_num;          // 0-1 octet
    uint16_t octets_per_frame_max_num;          // 2-3 octet
    uint16_t sampling_frequency_mask;
};
struct pacs_codec_record_t {
    uint8_t                 role;  // 0:sink 1:source
    uint8_t                 record_num;
    struct pacs_codec_capability__t *record;
};

#define EVENT_UNICAST_CLIENT_PACS_RECORD_DONE                   0x00000300

typedef int (*le_audio_unicast_source_event_callback_t)(u32 event, void *packet, u32 len);
extern void le_audio_unicast_source_callback_register(le_audio_unicast_source_event_callback_t callback);

typedef enum {
    UNICAST_VCS_OPCODE_RELATIVE_VOLUME_DOWN = 0x00,
    UNICAST_VCS_OPCODE_RELATIVE_VOLUME_UP,
    UNICAST_VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN,
    UNICAST_VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_UP,
    UNICAST_VCS_OPCODE_SET_ABSOLUTE_VOLUME, // 0-255
    UNICAST_VCS_OPCODE_UNMUTE,
    UNICAST_VCS_OPCODE_MUTE,
    UNICAST_VCS_OPCODE_RFU
} unicast_vcs_opcode_t;
extern void le_audio_unicast_source_vcs_vcp_write(u16 conn_handle, u8 opcode, u8 value);
extern void le_audio_unicast_source_vcs_state_read(u16 conn_handle);
extern void le_audio_unicast_source_vcs_flags_read(u16 conn_handle);


#endif

