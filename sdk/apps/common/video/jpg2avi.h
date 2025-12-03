#ifndef __JPG2AVI_H__
#define __JPG2AVI_H__
#include "typedef.h"
#include "system/spinlock.h"
#include "fs/fs.h"

#define JL_RIFF		ntohl(0x52494646)
#define JL_AVI_		ntohl(0x41564920)
#define JL_LIST     ntohl(0x4c495354)
#define JL_hdrl		ntohl(0x6864726c)
#define JL_avih		ntohl(0x61766968)
#define JL_strl		ntohl(0x7374726c)
#define JL_strh		ntohl(0x73747268)
#define JL_strf		ntohl(0x73747266)
#define JL_JUNK		ntohl(0x4a554e4b)

#define JL_mjpg		ntohl(0x6d6a7067)
#define JL_MJPG		ntohl(0x4d4a5047)
#define JL_vids		ntohl(0x76696473)
#define JL_auds		ntohl(0x61756473)


#define JL_movi		ntohl(0x6d6f7669)
#define JL_idx1		ntohl(0x69647831)

#define JL_00dc		ntohl(0x30306463)
#define JL_01wb		ntohl(0x30317762)
#define JL_02JL		ntohl(0x30324a4c)


#define JL_JLKJ		ntohl(0x4a4c4b4a)
#define JL_ENDF		ntohl(0x56185719)

//avih dwFlags
#define AVIF_HASINDEX		0x00000010
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define TRUSTCKTYPE         0x00000800
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000


typedef struct avi_mainheader {
    FOURCC fcc;   // 必须为‘avih’
    u32  cb;    // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
    u32  dwMicroSecPerFrame;   // 视频帧间隔时间（以毫秒为单位）
    u32  dwMaxBytesPerSec;     // 这个AVI文件的最大数据率
    u32  dwPaddingGranularity; // 数据填充的粒度
    u32  dwFlags;         // AVI文件的全局标记，比如是否含有索引块等
    u32  dwTotalFrames;   // 总帧数
    u32  dwInitialFrames; // 为交互格式指定初始帧数（非交互格式应该指定为0）
    u32  dwStreams;       // 本文件包含的流的个数
    u32  dwSuggestedBufferSize; // 建议读取本文件的缓存大小（应能容纳最大的块）
    u32  dwWidth;         // 视频图像的宽（以像素为单位）
    u32  dwHeight;        // 视频图像的高（以像素为单位）
    u32  dwReserved[4];   // 保留
} AVI_MAINHEADER;

typedef struct avi_streamheader {
    FOURCC fcc;  // 必须为‘strh’
    u32  cb;   // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
    FOURCC fccType;    // 流的类型：‘auds’（音频流）、‘vids’（视频流）、‘mids’（MIDI流）、‘txts’（文字流）
    FOURCC fccHandler; // 指定流的处理者，对于音视频来说就是解码器
    u32  dwFlags;    // 标记：是否允许这个流输出？调色板是否变化？
    u16   wPriority;  // 流的优先级（当有多个相同类型的流时优先级最高的为默认流）
    u16   wLanguage;
    u32  dwInitialFrames; // 为交互格式指定初始帧数
    u32  dwScale;   // 这个流使用的时间尺度
    u32  dwRate;
    u32  dwStart;   // 流的开始时间
    u32  dwLength;  // 流的长度（单位与dwScale和dwRate的定义有关）
    u32  dwSuggestedBufferSize; // 读取这个流数据建议使用的缓存大小
    u32  dwQuality;    // 流数据的质量指标（0 ~ 10,000）
    u32  dwSampleSize; // Sample的大小
    struct {
        u16 left;
        u16 top;
        u16 right;
        u16 bottom;
    }  rcFrame;  // 指定这个流（视频流或文字流）在视频主窗口中的显示位置
    // 视频主窗口由AVIMAINHEADER结构中的dwWidth和dwHeight决定
} AVI_STREAMHEADER;

typedef struct _BIT_MAPINFOHEADER {
    FOURCC  fcc;				//必须为"strf"
    u32 strf_size;

    u32 biSize;					//本结构所占字节数
    u32 biWidth;				//位图宽度，以像素为单位
    u32 biHeight;				//位图高度，以像素为单位
    u16 biPlanes;				//目标设备级别，必须为1
    u16 biBitCount;				//每个像素的位数
    u32 biCompression;			//位图压缩类型,例如MJPG
    u32 biSizeImage;			//位图大小，以字节为单位
    u32 biXPelsPerMeter;		//位图水平分辨率
    u32 biYPelsPerMeter;		//位图垂直分辨率
    u32 biClrUsed;				//位图实际使用的调色板的颜色数
    u32 biClrImportant;			//位图显示过程中重要的颜色数
} BIT_MAPINFOHEADER;


#ifdef WIN32
#pragma pack(push,1)
#else
#pragma pack(1)
#endif


#define HEAD_JUNK_SIZE		(512 - ( (6 * 4) + sizeof(AVI_MAINHEADER) + ((3 * 4 + sizeof(AVI_STREAMHEADER)) * 2 + \
								sizeof(BIT_MAPINFOHEADER) + sizeof(WAVE_FORMATEX)) + (2 * 4) + 12) % 512 )


#define HEAD_JUNK_SIZE_NO_AUD	(512 - ( (6 * 4) + sizeof(AVI_MAINHEADER) + ((3 * 4 + sizeof(AVI_STREAMHEADER)) * 1 + \
								sizeof(BIT_MAPINFOHEADER)) + (2 * 4) + 12) % 512 )

#define HEAD_INFO_ISFT_SIZE (14)
#define HEAD_INFO_JUNK_SIZE (512 - (4*7) - HEAD_INFO_ISFT_SIZE)

typedef struct __JL_AVI_HEAD_NO_AUD {

    FOURCC  riff;  // 必须为‘RIFF’
    u32 file_size;
    u32 file_type; // "AVI "

    FOURCC  list_hdrl;  // 必须为"LIST"
    u32 hdrl_size;
    FOURCC  hdrl;   //必须为"hdrl"

    AVI_MAINHEADER avih;

    FOURCC  list_strl_vid;  // 必须为"LIST"
    u32 strl_vidsize;
    FOURCC  strl_vid;   //必须为"strl"
    AVI_STREAMHEADER vid_strh;
    BIT_MAPINFOHEADER vid_strf;

    FOURCC fcc_head_junk;  //必须为"JUNK"
    u32 head_junk_len;
    u8 head_junk[HEAD_JUNK_SIZE_NO_AUD];


    FOURCC list_info;
    u32 info_size;
    FOURCC head_info;
    FOURCC head_isft;
    u32 isft_size;
    u8 isft[HEAD_INFO_ISFT_SIZE];
    FOURCC info_head_junk;
    u32 info_junk_len;
    u8 info_junk[HEAD_INFO_JUNK_SIZE];


    u32 list;
    u32 len ;
    u32 movi ;

} JL_AVI_HEAD_NO_AUD;

int avi_dec_file_start(FILE *fp);
void avi_set_to_movi_tail(FILE *fp);
int avi_dec_file_stop(FILE *fp);
void jpg2avi_add_frame_to_movi(FILE *fp, void *data, unsigned int len);

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif


#endif /* end of include guard: __JPG2AVI_H__ */
