#ifndef _FILE_PLAYER_H_
#define _FILE_PLAYER_H_

#include "generic/typedef.h"
#include "jlstream.h"

#include "music/music_decrypt.h"
#include "music/music_id3.h"
#include "audio_decoder.h"
#include "fs/fs.h"
#include "effect/effects_default_param.h"
#include "sdk_config.h"

#define MAX_FILE_NUM 20

#if (defined(TCFG_DEC_APE_ENABLE) && (TCFG_DEC_APE_ENABLE))
#define BREAKPOINT_DATA_LEN   (2036 + 4)
#elif (defined(TCFG_DEC_FLAC_ENABLE) && (TCFG_DEC_FLAC_ENABLE))
#define BREAKPOINT_DATA_LEN	688
#elif (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
#define BREAKPOINT_DATA_LEN	536
#else
#define BREAKPOINT_DATA_LEN	32
#endif

#define AUDIO_ATTR_OPUS_OGG_TYPE        BIT(0)  /*!< ogg封装的opus数据 */
#define AUDIO_ATTR_OPUS_RAWDTF_TYPE     BIT(1)  /*!< opus为raw数据，带8字节packet头(4字节大端包长+4字节range校验值) */
#define AUDIO_ATTR_OPUS_CBR_PKTLEN_TYPE BIT(2)  /*!< opus为百度无头封装格式，需要设置cbr包长,数据为raw数据+CBR_OPUS包长 */

#define CONFIG_OPUS_DEC_FILE_TYPE       AUDIO_ATTR_OPUS_RAWDTF_TYPE
#define CONFIG_OPUS_DEC_PACKET_LEN      160     //16/32/64kbps - 40/80/160

#define CONFIG_SPEEX_DEC_FILE_QUALITY   5
#define CONFIG_SPEEX_DEC_FILE_SAMPLERATE 16000  //不带头部的解码数据需要设置采样率

#define CONFIG_PCM_DEC_FILE_SAMPLERATE  44100

typedef int (*music_player_cb_t)(void *, int parm, enum stream_event);

enum play_status : u8 {
    FILE_PLAYER_STOP = 0x0, //播放结束
    FILE_PLAYER_START, //播放中
    FILE_PLAYER_PAUSE//播放暂停
};

struct midi_player {
    struct jlstream *stream;
};

struct file_player {
    struct list_head entry;
    u8 ref;
    u8 index;
    u8 player_id;
    u8 read_err;

    u8 break_point_flag;    //是否有播放器申请的断点
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
    void *le_audio;     //广播音箱的句柄
    void *fmt;			//广播音箱的编码格式
    u32 pcm_addr;
};

// AB点复读模式
enum {
    AB_REPEAT_MODE_BP_A = 0x01,
    AB_REPEAT_MODE_BP_B,
    AB_REPEAT_MODE_CUR,
};

typedef enum {
    PLAY_SPEED_0_5 = 0x0, //0.5倍速
    PLAY_SPEED_0_75,      //0.75倍速
    PLAY_SPEED_1,         // 1倍速
    PLAY_SPEED_1_25,      //1.25倍速
    PLAY_SPEED_1_5,       //1.5倍速
    PLAY_SPEED_2,        // 2倍速
    PLAY_SPEED_3,        // 3倍速
    PLAY_SPEED_4,        // 4倍速
} speed_level_t;


/* ---------------------音乐播放API-----------------------------
 *
 * 音乐和其它音频之间默认采用叠加方式播放,可能由于资源限制而采用打断的方式播放
 * 音乐和音乐之间默认不抢占,采用排队方式播放
 *
 * ---------------------------------------------------------*/

/* 播放单个音乐文件 */
struct file_player *music_file_play(FILE *file, struct audio_dec_breakpoint *dbp);

/* 播放单个音乐文件,带有播放状态回调参数 */
struct file_player *music_file_play_callback(FILE *file, void *priv,
        music_player_cb_t callback,
        struct audio_dec_breakpoint *dbp);

/* 停止播放单个音乐 */
void music_file_player_stop(struct file_player *music_player);

/* 停止播放所有音乐 */
void music_file_player_stop_all(void);

/* 返回是否有音乐在播放 */
int music_player_runing(void);

/* 返回第一个打开的音乐播放器指针 */
struct file_player *get_music_file_player(void);

/* 暂停/继续播放音乐 */
int music_file_player_pp(struct file_player *music_player);

/* 音乐快进 */
int music_file_player_ff(u16 step_s, struct file_player *music_player);

/* 音乐快退  */
int music_file_player_fr(u16 step_s, struct file_player *music_player);

/* 音乐AB点切换  */
int music_file_ab_repeat_switch(struct file_player *music_player);

/* 音乐AB点关闭  */
int music_file_ab_repeat_close(struct file_player *music_player);

/* 音乐音调上升  */
int music_file_pitch_up(struct file_player *music_player);

/* 音乐音调下降  */
int music_file_pitch_down(struct file_player *music_player);

/* 音乐设置音调  */
int music_file_set_pitch(struct file_player *music_player, pitch_level_t pitch_mode);

/* 加速音乐播放,设置成功返回当前倍速播放的speed值,失败则返回-1 */
int music_file_speed_up(struct file_player *music_player);

/* 减速音乐播放,设置成功返回当前倍速播放的speed值,失败则返回-1 */
int music_file_speed_down(struct file_player *music_player);

/* 设置音乐播放速度值  */
int music_file_set_speed(struct file_player *music_player, speed_level_t speed_mode); //设置播放速度

/* 获取音乐断点  */
int music_file_get_breakpoints(struct audio_dec_breakpoint *bp, struct file_player *music_player);

/* 获取音乐播放器状态  */
int music_file_get_player_status(struct file_player *music_player);

/* 获取音乐当前播放时间  */
int music_file_get_cur_time(struct file_player *music_player);

/* 获取音乐播放总时长  */
int music_file_get_total_time(struct file_player *music_player);

/* 设置循环播放次数 */
int file_dec_repeat_set(struct file_player *music_player, u8 repeat_num);

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
int file_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct file_player *music_player);

/*----------------------------------------------------------------------------*/
/**@brief    跳到指定位置开始播放
   @param    start_time: 要跳转过去播放的起始时间，单位ms
   @return   0：成功
   @return   -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int file_dec_set_start_play(u32 start_time, u32 coding_type);

struct file_player *le_audio_music_file_play_callback(FILE *file, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp, void *le_audio, void *fmt);

#endif

