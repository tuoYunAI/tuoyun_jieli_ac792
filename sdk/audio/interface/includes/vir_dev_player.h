#ifndef _VIRTUAL_PLAYER_H_
#define _VIRTUAL_PLAYER_H_

#include "generic/typedef.h"
#include "jlstream.h"
#include "music/music_decrypt.h"
#include "music/music_id3.h"
#include "audio_decoder.h"
#include "effect/effects_default_param.h"
#include "app_config.h"
#include "file_player.h"

struct vir_player {
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
    void *file;
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
};

/* ---------------------音乐播放API-----------------------------
 *
 * 音乐和其它音频之间默认采用叠加方式播放,可能由于资源限制而采用打断的方式播放
 * 音乐和音乐之间默认不抢占,采用排队方式播放
 *
 * ---------------------------------------------------------*/

/* 播放单个音乐文件 */
struct vir_player *virtual_dev_play(FILE *file, struct stream_file_ops *ops, struct audio_dec_breakpoint *dbp, u32 coding_type);

/* 播放单个音乐文件,带有播放状态回调参数 */
struct vir_player *virtual_dev_play_callback(FILE *file, struct stream_file_ops *ops, void *priv,
        music_player_cb_t callback,
        struct audio_dec_breakpoint *dbp, u32 coding_type);

/* 停止播放单个音乐 */
void virtual_dev_player_stop(struct vir_player *virtual_player);

/* 停止播放所有音乐 */
void virtual_dev_player_stop_all(void);

/* 返回是否有音乐在播放 */
int virtual_player_runing(void);

/* 返回第一个打开的音乐播放器指针 */
struct vir_player *get_virtual_dev_player(void);

/* 暂停/继续播放音乐 */
int virtual_dev_player_pp(struct vir_player *virtual_player);

/* 音乐快进 */
int virtual_dev_player_ff(u16 step_s, struct vir_player *virtual_player);

/* 音乐快退  */
int virtual_dev_player_fr(u16 step_s, struct vir_player *virtual_player);

/* 音乐AB点切换  */
int virtual_dev_ab_repeat_switch(struct vir_player *virtual_player);

/* 音乐AB点关闭  */
int virtual_dev_ab_repeat_close(struct vir_player *virtual_player);

/* 音乐音调上升  */
int virtual_dev_pitch_up(struct vir_player *virtual_player);

/* 音乐音调下降  */
int virtual_dev_pitch_down(struct vir_player *virtual_player);

/* 音乐设置音调  */
int virtual_dev_set_pitch(struct vir_player *virtual_player, pitch_level_t pitch_mode);

/* 加速音乐播放,设置成功返回当前倍速播放的speed值,失败则返回-1 */
int virtual_dev_speed_up(struct vir_player *virtual_player);

/* 减速音乐播放,设置成功返回当前倍速播放的speed值,失败则返回-1 */
int virtual_dev_speed_down(struct vir_player *virtual_player);

/* 设置音乐播放速度值  */
int virtual_dev_set_speed(struct vir_player *virtual_player, speed_level_t speed_mode); //设置播放速度

/* 获取音乐断点  */
int virtual_dev_get_breakpoints(struct audio_dec_breakpoint *bp, struct vir_player *virtual_player);

/* 获取音乐播放器状态  */
int virtual_dev_get_player_status(struct vir_player *virtual_player);

/* 获取音乐当前播放时间  */
int virtual_dev_get_cur_time(struct vir_player *virtual_player);

/* 获取音乐播放总时长  */
int virtual_dev_get_total_time(struct vir_player *virtual_player);

/* 设置循环播放次数 */
int vir_dev_dec_repeat_set(struct vir_player *virtual_player, u8 repeat_num);

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
int vir_dev_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct vir_player *virtual_player);

/*----------------------------------------------------------------------------*/
/**@brief    跳到指定位置开始播放
   @param    start_time: 要跳转过去播放的起始时间，单位ms
   @return   0：成功
   @return   -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int vir_dev_dec_set_start_play(u32 start_time, u32 coding_type);

#endif

