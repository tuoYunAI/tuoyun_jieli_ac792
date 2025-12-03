#ifndef _NET_PLAYER_H_
#define _NET_PLAYER_H_

#include "generic/typedef.h"
#include "jlstream.h"
#include "music/music_decrypt.h"
#include "music/music_id3.h"
#include "audio_decoder.h"
#include "effect/effects_default_param.h"
#include "app_config.h"
#include "file_player.h"

struct net_file_player {
    struct list_head entry;
    u8 ref;
    u8 index;
    u8 read_err;
    u8 break_point_flag;    //是否有播放器申请的断点

    u32 player_id;
    enum play_status status;    //播放状态
    enum stream_scene scene;
    enum stream_coexist coexist;

    u32 coding_type;
    struct jlstream *stream;
    void *net_lrc_file;
    void *net_file;
    void *le_audio;//广播音箱句柄
    void *le_audio_fmt;//广播音箱的编码格式
    const char *file_name_list[MAX_FILE_NUM + 1];

    void *priv;
    music_player_cb_t callback;

    CIPHER mply_cipher;		// 解密播放

#if TCFG_DEC_ID3_V1_ENABLE
    MP3_ID3_OBJ *p_mp3_id3_v1;	// id3_v1信息
#endif
#if TCFG_DEC_ID3_V2_ENABLE
    MP3_ID3_OBJ *p_mp3_id3_v2;	// id3_v2信息
#endif

    struct audio_dec_breakpoint *break_point;
    u8 ab_repeat_status;
    s8 music_speed_mode; //播放倍速
    s8 music_pitch_mode; //变调模式
#if FILE_DEC_REPEAT_EN
    u8 repeat_num;			// 无缝循环次数
    struct fixphase_repair_obj repair_buf;	// 无缝循环句柄
#endif

    u16 wait;
};


/* ---------------------音乐播放API-----------------------------
 *
 * 音乐和其它音频之间默认采用叠加方式播放,可能由于资源限制而采用打断的方式播放
 * 音乐和音乐之间默认不抢占,采用排队方式播放
 *
 * ---------------------------------------------------------*/

/* 播放单个音乐文件 */
struct net_file_player *net_file_play(const char *url, struct audio_dec_breakpoint *dbp);

/* 播放单个音乐文件,带有播放状态回调参数 */
struct net_file_player *net_file_play_callback(const char *url, void *priv,
        music_player_cb_t callback,
        struct audio_dec_breakpoint *dbp);

/* 停止播放所有音乐 */
void net_file_player_stop_all(void);

/* 停止播放所有音乐 */
void net_file_player_stop(struct net_file_player *net_player);

/* 返回是否有音乐在播放 */
int net_player_runing(void);

/* 返回第一个打开的音乐播放器指针 */
struct net_file_player *get_net_file_player(void);

/* 网络播放音频lrc歌词文件开始解析*/
int net_music_lrc_analy_start(const char *url, struct net_file_player *net_player);

/* 暂停/继续播放音乐 */
int net_file_player_pp(struct net_file_player *net_player);

/* 音乐快进 */
int net_file_player_ff(u16 step_s, struct net_file_player *net_player);

/* 音乐快退  */
int net_file_player_fr(u16 step_s, struct net_file_player *net_player);

/* 音乐AB点切换  */
int net_file_ab_repeat_switch(struct net_file_player *net_player);

/* 音乐AB点关闭  */
int net_file_ab_repeat_close(struct net_file_player *net_player);

/* 音乐音调上升  */
int net_file_pitch_up(struct net_file_player *net_player);

/* 音乐音调下降  */
int net_file_pitch_down(struct net_file_player *net_player);

/* 音乐设置音调  */
int net_file_set_pitch(struct net_file_player *net_player, pitch_level_t pitch_mode);

/* 加速音乐播放,设置成功返回当前倍速播放的speed值,失败则返回-1 */
int net_file_speed_up(struct net_file_player *net_player);

/* 减速音乐播放,设置成功返回当前倍速播放的speed值,失败则返回-1 */
int net_file_speed_down(struct net_file_player *net_player);

/* 设置音乐播放速度值  */
int net_file_set_speed(struct net_file_player *net_player, speed_level_t speed_mode); //设置播放速度

/* 获取音乐断点  */
int net_file_get_breakpoints(struct audio_dec_breakpoint *bp, struct net_file_player *net_player);

/* 获取音乐播放器状态  */
int net_file_get_player_status(struct net_file_player *net_player);

/* 获取音乐当前播放时间  */
int net_file_get_cur_time(struct net_file_player *net_player);

/* 获取音乐播放总时长  */
int net_file_get_total_time(struct net_file_player *net_player);

/* 设置循环播放次数 */
int net_file_dec_repeat_set(struct net_file_player *net_player, u8 repeat_num);

/*----------------------------------------------------------------------------*/
/**@brief    跳到指定位置开始播放，播放到目标时间后回调
   @param    start_time: 要跳转过去播放的起始时间，单位ms
   @param    dest_time: 要跳转过去播放的目标时间，单位ms
   @param    *cb: 到达目标时间后回调
   @param    *cb_priv: 回调参数
   @return   0：成功
   @return   -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int net_file_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct net_file_player *net_player);

/*----------------------------------------------------------------------------*/
/**@brief    跳到指定位置开始播放
   @param    start_time: 要跳转过去播放的起始时间，单位ms
   @return   0：成功
   @return   -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int net_file_dec_set_start_play(u32 start_time, u32 coding_type);

/*le_audio 网络音频广播转发启动回调接口*/
struct net_file_player *le_audio_net_file_play_callback(const char *url, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp, void *le_audio, void *fmt);

#endif

