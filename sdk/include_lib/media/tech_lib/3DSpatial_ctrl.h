#ifndef _DSPATIAL_CTRL_H_
#define _DSPATIAL_CTRL_H_

#include "effects/AudioEffect_DataType.h"

#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#define text_code __attribute__((section(".TDSpatial.text")))
#define cache_code __attribute__((section(".TDSpatial.text.cache.L2")))
#else
#define text_code
#define cache_code
#endif

typedef struct SourceConfig {
    char CustomizedITD;//自定义延迟开关0 1
    char FarField;//距离模块使能 0 1
    int sampleRate;//采样率
} source_cfi;

typedef struct SourceAngle {
    int Azimuth_angle;//方位角
    int Elevation_angle;//俯仰角
    float radius;//半径,声源到人的距离，调节远近的效果
    int bias_angle;//偏角,调节声像，偏角为0，听上去就是点声源，比如30度就听感上是60度范围的声像
} source_ang;

typedef struct CORE {
    float ListenerHeadRadius;//人的头围半径，单位米
    short soundSpeed;//声速，一般都是固定的343
    char channel;// 通道数,固定2
} core;

typedef struct _Sound_3DSpatial {
    int (*need_buf)(int channel);
    int (*init)(void *ptr, core *cor, source_cfi *scfi, source_ang *sag, af_DataType *adt);
    void (*run)(void *ptr, void *tmpbuf, void *data, void *output, int len, int channel);//channel 0输出左声道，1输出右声道，2输出双声道数据
    int (*need_tmpbuf)(short len, af_DataType adt);
} Sound_3DSpatial;

Sound_3DSpatial *get_SpatialEffect_ops(void);
int Dspatial_VirtualBass_version_2_0_0(void);
void updata_position(void *ptr, core *cor, source_ang *sag);

#endif

