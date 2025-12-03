#ifndef _LIB_MEDIA_ANALYSIS_H_
#define _LIB_MEDIA_ANALYSIS_H_

#include "generic/typedef.h"
#include "media/media_config.h"

float rms_calc(const short *mono_pcm, int npoint);
float rms_calc_32bit(const int *mono_pcm, int npoint);
int peak_calc(short *mono_pcm, int npoint);
int peak_calc_32bit(int *mono_pcm, int npoint);

#endif
