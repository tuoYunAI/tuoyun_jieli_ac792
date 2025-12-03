#ifndef _TTS_TONE_PLAYER_H_
#define _TTS_TONE_PLAYER_H_

#include "generic/typedef.h"
#include "jlstream.h"
#include "network_download/net_download.h"
#include "tone_player.h"

struct tts_player {
    struct list_head entry;

    u8 ref;
    u8 index;
    u8 index_max;
    u8 channel_mode;
    enum stream_coexist coexist;
    u16 wait;
    u32 player_id;

    u32 coding_type;
    u32 sample_rate;

    struct jlstream *stream;

    void *file;
    void *next_file;
    const char **url_file_list;

    void *priv;
    tone_player_cb_t callback;
    struct net_download_parm parm;
};

/* --------------------TTS提示音播放API------------------------
 *
 * 提示音和其它音频之间默认采用叠加方式播放,可能由于资源限制而采用打断的方式播放
 * 提示音和提示音之间默认不抢占,采用排队方式播放
 *
 * ---------------------------------------------------------*/

/*
 * 播放单个提示音文件
 */
int play_tts_file(const char *url);

/*
 * 播放单个提示音文件,带有播放状态回调参数
 */
int play_tts_file_callback(const char *url, void *priv,
                           tone_player_cb_t callback);

/*
 * 打断方式播放单个提示音文件
 */
int play_tts_file_alone(const char *url);

int play_tts_file_alone_callback(const char *url, void *priv,
                                 tone_player_cb_t callback);

/*
 * 播放文件列表,url_list[x]中的url要求持续有效
 */
int play_tts_files(const char *const url_list[], u8 file_num);

int play_tts_files_alone(const char *const url_list[], u8 file_num);

int play_tts_files_callback(const char *const url_list[], u8 file_num,
                            void *priv, tone_player_cb_t callback);

int play_tts_files_alone_callback(const char *const url_list[], u8 file_num,
                                  void *priv, tone_player_cb_t callback);

/*
 * 停止播放所有提示音
 */
void tts_player_stop(void);

/*
 * 返回是否有提示音在播放
 */
int tts_player_runing(void);

#endif

