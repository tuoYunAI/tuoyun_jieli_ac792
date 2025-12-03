#ifndef _MUSIC_PLAYER_H_
#define _MUSIC_PLAYER_H_

#include "media/audio_decoder.h"
#include "dev_manager/dev_manager.h"
#include "file_operate/file_manager.h"
#include "jlstream.h"


//解码错误码表
enum {
    MUSIC_PLAYER_ERR_NULL = 0x0,		//没有错误， 不需要做任何
    MUSIC_PLAYER_SUCC	  = 0x1,		//播放成功， 可以做相关显示、断点记忆等处理
    MUSIC_PLAYER_ERR_PARM,	    		//参数错误
    MUSIC_PLAYER_ERR_POINT,	    		//参数指针错误
    MUSIC_PLAYER_ERR_NO_RAM,			//没有ram空间错误
    MUSIC_PLAYER_ERR_DECODE_FAIL,		//解码器启动失败错误
    MUSIC_PLAYER_ERR_DEV_NOFOUND,		//没有找到指定设备
    MUSIC_PLAYER_ERR_DEV_OFFLINE,		//设备不在线错误
    MUSIC_PLAYER_ERR_DEV_READ,			//设备读错误
    MUSIC_PLAYER_ERR_FSCAN,				//设备扫描失败
    MUSIC_PLAYER_ERR_FILE_NOFOUND,		//没有找到文件
    MUSIC_PLAYER_ERR_RECORD_DEV,		//录音文件夹操作错误
    MUSIC_PLAYER_ERR_FILE_READ,			//文件读错误
};

//断点信息结构体
typedef struct {
    //文件信息
    u32 fsize;
    u32 sclust;
    //解码信息
    struct audio_dec_breakpoint dbp;
} breakpoint_t;

//回调接口结构体
typedef struct {
    //解码成功回调
    void (*start)(void *priv, int parm);
    //解码结束回调
    void (*end)(void *priv, int parm);
    //解码错误回调
    void (*err)(void *priv, int parm);
} player_callback_t;

//参数设置结构体
struct __player_parm {
    const player_callback_t *cb;
    //其他扩展
    const scan_callback_t *scan_cb;//扫盘打断回调
};

//music player总控制句柄
struct music_player {
    struct __dev 			*dev;//当前播放设备节点
    struct vfscan			*fsn;//设备扫描句柄
    FILE		 			*file;//当前播放文件句柄
    FILE		 			*lrc_file;//当前播放歌词文件句柄
    void		 			*lrc_info;//播放信息句柄
    void 					*priv;//music回调函数，私有参数
    struct __player_parm 	parm;//回调及参数配置
    void                    *le_audio;//广播音箱的句柄
    struct stream_enc_fmt fmt;//广播音箱的编码格式
};


/*----------------------------------------------------------------------------*/
/**@brief    music_player初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
struct music_player *music_player_creat(void);

/*----------------------------------------------------------------------------*/
/**@brief    music_playe注册扫盘回调
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_reg_scandisk_callback(struct music_player *player_hd, const scan_callback_t *scan_cb);

/*----------------------------------------------------------------------------*/
/**@brief    music_playe注册解码状态回调
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_reg_dec_callback(struct music_player *player_hd, const player_callback_t *cb);

///*----------------------------------------------------------------------------*/
/**@brief    music_player释放接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_destroy(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player解码器启动接口
   @param
			 file：文件句柄
			 dbp：断点信息
   @return   music_player 错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_decode_start(struct music_player *player_hd, FILE *file, struct audio_dec_breakpoint *dbp);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备断点信息
   @param
			 bp：断点缓存，外部调用提供
			 flag： 1-需要获取歌曲断点信息及文件信息，0-只获取文件信息
   @return   成功与否
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_get_playing_breakpoint(struct music_player *player_hd, breakpoint_t *bp, u8 flag);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放事件回调
   @param
   @return
   @note	非api层接口
*/
/*----------------------------------------------------------------------------*/
int music_player_decode_event_callback(void *priv, int parm, enum stream_event event);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放结束处理
   @param	 parm：结束参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_end_deal(struct music_player *player_hd, int parm);

/*----------------------------------------------------------------------------*/
/**@brief    music_player解码错误处理
   @param	 event：err事件
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_decode_err_deal(struct music_player *player_hd, int event);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放对应的music设备
   @param
   @return   设备盘符
   @note	 播放录音区分时，可以通过该接口判断当前播放的音乐设备是什么以便做录音区分判断
*/
/*----------------------------------------------------------------------------*/
const char *music_player_get_cur_music_dev(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备下一个设备
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
const char *music_player_get_dev_next(struct music_player *player_hd, u8 auto_next);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备上一个设备
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
const char *music_player_get_dev_prev(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取文件句柄
   @param
   @return   文件句柄
   @note	 需要注意文件句柄的生命周期
*/
/*----------------------------------------------------------------------------*/
FILE *music_player_get_file_hdl(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player移除文件句柄
   @param
   @note	 需要注意文件句柄的生命周期
*/
/*----------------------------------------------------------------------------*/
void music_player_remove_file_hdl(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放循环模式
   @param
   @return   当前播放循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
u8 music_player_get_repeat_mode(void);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前录音区分播放状态
   @param
   @return   true：录音文件夹播放, false：非录音文件夹播放
   @note	 播放录音区分时，可以通过该接口判断当前播放的是录音文件夹还是非录音文件夹
*/
/*----------------------------------------------------------------------------*/
bool music_player_get_record_play_status(struct music_player *player_hd);

///*----------------------------------------------------------------------------*/
/**@brief    music_player从设备列表里面往前或往后找设备，并且过滤掉指定字符串的设备
   @param
            flit:过滤字符串， 查找设备时发现设备logo包含这个字符串的会被过滤
            direct：查找方向， 1:往后， 0：往前
   @return   查找到符合条件的设备逻辑盘符， 找不到返回NULL
   @note
*/
/*----------------------------------------------------------------------------*/
const char *music_player_get_dev_flit(struct music_player *player_hd, const char *flit, u8 direct);

/*----------------------------------------------------------------------------*/
/**@brief    music_player设置播放循环模式
   @param	 mode：循环模式
				FCYCLE_ALL
				FCYCLE_ONE
				FCYCLE_FOLDER
				FCYCLE_RANDOM
   @return  循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_set_repeat_mode(struct music_player *player_hd, u8 mode);

/*----------------------------------------------------------------------------*/
/**@brief    music_player切换循环模式
   @param
   @return   循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_change_repeat_mode(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player解码停止
   @param
			 fsn_release：
				1：释放扫盘句柄
				0：不释放扫盘句柄
   @return
   @note	 如果释放了扫盘句柄，需要重新扫盘，否则播放失败
*/
/*----------------------------------------------------------------------------*/
void music_player_stop(struct music_player *player_hd, u8 fsn_release);

/*----------------------------------------------------------------------------*/
/**@brief    music_player删除当前播放文件,并播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_delete_playing_file(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_prev(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_next(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放第一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_first_file(struct music_player *player_hd, const char *logo);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放最后一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_last_file(struct music_player *player_hd, const char *logo);

/*----------------------------------------------------------------------------*/
/**@brief    music_player自动播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_auto_next(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放上一个文件夹
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_folder_prev(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放下一个文件夹
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_folder_next(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player上一个设备
   @param	 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_devcie_prev(struct music_player *player_hd, breakpoint_t *bp);

/*----------------------------------------------------------------------------*/
/**@brief    music_player下一个设备
   @param	 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_devcie_next(struct music_player *player_hd, breakpoint_t *bp);

/*----------------------------------------------------------------------------*/
/**@brief    music_player断点播放指定设备
   @param
            logo：逻辑盘符，如：sd0/sd1/udisk0
            bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_breakpoint(struct music_player *player_hd, const char *logo, breakpoint_t *pb);

/*----------------------------------------------------------------------------*/
/**@brief    music_player序号播放指定设备
   @param
            logo：逻辑盘符，如：sd0/sd1/udisk0
            number：指定播放序号
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_number(struct music_player *player_hd, const char *logo, u32 number);

/*----------------------------------------------------------------------------*/
/**@brief    music_player簇号播放指定设备
   @param
            logo：逻辑盘符，如：sd0/sd1/udisk0
            sclust：指定播放簇号
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_sclust(struct music_player *player_hd, const char *logo, u32 sclust);

/*----------------------------------------------------------------------------*/
/**@brief    music_player路径播放指定设备
   @param
            logo：逻辑盘符，如：sd0/sd1/udisk0, 设置为NULL，为默认当前播放设备
            path：指定播放路径
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_path(struct music_player *player_hd, const char *logo, const char *path);

/*----------------------------------------------------------------------------*/
/**@brief    music_player录音区分切换播放
   @param
            logo：逻辑盘符，如：sd0/sd1/udisk0, 设置为NULL，为默认当前播放设备
            bp：断点信息
   @return   播放错误码
   @note	 通过指定设备盘符，接口内部通过解析盘符是否"_rec"来确定是切换到录音播放设备还是非录音播放设备
*/
/*----------------------------------------------------------------------------*/
int music_player_play_record_folder(struct music_player *player_hd, const char *logo, breakpoint_t *bp);

/*----------------------------------------------------------------------------*/
/**@brief    music_player扫盘
   @param
   @return   播放错误码
   @note 	 音乐模式下盘符曲目发生变化时
			 调用接口重新扫盘当前设备
*/
/*----------------------------------------------------------------------------*/
int music_player_scan_disk(struct music_player *player_hd);

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取获取当前设备的物理设备
   @param
   @return   设备盘符
*/
/*----------------------------------------------------------------------------*/
const char *music_player_get_phy_dev(struct music_player *player_hd, int *len);

/*----------------------------------------------------------------------------*/
/**@brief    music_player歌词分析
   @param
   @return   0 成功
*/
/*----------------------------------------------------------------------------*/
int music_player_lrc_analy_start(struct music_player *player_hd);

#endif//__MUSIC_PLAYER_H__

