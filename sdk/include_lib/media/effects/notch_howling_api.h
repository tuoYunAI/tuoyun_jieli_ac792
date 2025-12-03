#ifndef _NOTCH_HOWLING_API_H_
#define _NOTCH_HOWLING_API_H_

#include "effects/AudioEffect_DataType.h"

#ifndef MEDIA_SUPPORT_MS_EXTENSIONS

#define AT_NOTCHHOWLING(x)
#define AT_NOTCHHOWLING_CODE
#define AT_NOTCHHOWLING_CONST
#define AT_NOTCHHOWLING_SPARSE_CODE
#define AT_NOTCHHOWLING_SPARSE_CONST

#else

#define AT_NOTCHHOWLING(x)           __attribute__((section(#x)))
#define AT_NOTCHHOWLING_CODE         AT_NOTCHHOWLING(.notchhowling.text.cache.L2.code)
#define AT_NOTCHHOWLING_CONST        AT_NOTCHHOWLING(.notchhowling.text.cache.L2.const)
#define AT_NOTCHHOWLING_SPARSE_CODE  AT_NOTCHHOWLING(.notchhowling.text)
#define AT_NOTCHHOWLING_SPARSE_CONST AT_NOTCHHOWLING(.notchhowling.text.const)

#endif

typedef struct _NotchHowlingParam {
    int fade_time;
    float notch_gain;
    float notch_Q;
    float threshold;
    int SampleRate;
    af_DataType pcm_info;
} NotchHowlingParam;

int get_howling_buf(NotchHowlingParam *param);
int howling_init(void *workbuf, NotchHowlingParam *param);
int howling_run(void *workbuf, short *in, short *out, int len);
void SetHowlingDetection(void *workbuf, int OnlyDetection);
float *getHowlingFreq(void *workbuf, int *num);

#endif // !NOTCH_HOWLING_API_H

