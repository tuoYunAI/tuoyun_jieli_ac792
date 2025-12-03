#ifndef _DRC_API_DETECT_H_
#define _DRC_API_DETECT_H_

#include "AudioEffect_DataType.h"

#ifndef MEDIA_SUPPORT_MS_EXTENSIONS

#define AT_DRC(x)
#define AT_DRC_CODE
#define AT_DRC_CONST
#define AT_DRC_SPARSE_CODE
#define AT_DRC_SPARSE_CONST

#else

#define AT_DRC(x)           __attribute((section(#x)))
#define AT_DRC_CODE         AT_DRC(.drc.text.cache.L2.code)
#define AT_DRC_CONST        AT_DRC(.drc.text.cache.L2.const)
#define AT_DRC_SPARSE_CODE  AT_DRC(.drc.text)
#define AT_DRC_SPARSE_CONST AT_DRC(.drc.text.const)

#endif

enum {
    PEAK_DETECT = 0,
    RMS_DETECT
};

enum {
    PERPOINT_DETECT = 0,
    TWOPOINT_DETECT
};

typedef struct _DrcParam {
    int channel;
    int sampleRate;
    int attackTime;       //启动时间
    int releaseTime;	   //释放时间
    float *threshold;	   //阈值{x1，y1,x2,y2}  最多5组十个数据
    int ThresholdNum;	   //阈值的组数
    float rmsTime;		   // rms时间  algorithm为RMS有效
    float DetectGain;	   // 检测数据增益（db）
    float OutputGain;	   // 输出数据增益(db)
    unsigned char algorithm;		   //  PEAK或者RMS
    unsigned char mode;			   //  模式
    af_DataType pcm_info;
    unsigned char DetectdataBit;
    char DetectdataInc;
} DrcDetectParam;

int GetDrcDetectBuf(DrcDetectParam *param); // bufsize 与 algorithm，rmsTime channel、 sampleRate有关
int DrcDetectInit(void *WorkBuf, DrcDetectParam *param);
void DrcDetectUpdate(void *WorkBuf, DrcDetectParam *param);
int DrcDetectRun(void *WorkBuf, void *indata, void *detectdata, void *outdata, int per_channel_npoint);


#endif // !DRC_API_H

