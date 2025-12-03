#ifndef __N5_DVP_H__
#define __N5_DVP_H__

#include "typedef.h"

//#define N5_DVP   1

#define N5_DVP_OUTPUT_W    1280
#define N5_DVP_OUTPUT_H    720

//#define INPUT_AHD1  0
//#define INPUT_AHD2  1

typedef enum _InputSourceIDType {
    INPUT_AHD1,
    INPUT_AHD2,
    MAX_VIDEO_CHANNEL,
    ALL_INPUT_SOURCE = 0XFF,
} InputSourceType;

extern u8 N5_CurChannel;

typedef enum _ConfigResolution {
    AHD_720P_25PFS 	= 0,
    AHD_720P_30PFS 	= 1,
    AHD_1080P_25PFS	= 2,
    AHD_1080P_30PFS	= 3,
    CVBS_PAL		= 4,
    CVBS_NTSC		= 5,
    TVI_720P_25FPS 	= 6,
    TVI_720P_30FPS 	= 7,
    CVI_720P_25FPS 	= 8,
    CVI_720P_30FPS 	= 9,
    SD_960H_EX_PAL 	= 10, //1920*576
    SD_960H_EX_NT	= 11,//1920*480

    NULL_INPUT		= 12,
} ConfigResolution;

struct rxchip_detect_info {
    u8 en_rxchip_check;

    u8 curch_check_have_signal_times;
    u8 curch_check_no_signal_times;
    u8 signalokcnt;
    u8 signalnookcnt;
    u8 signal_state;
    u8 signal_flag;
    u8 work_mode;
    u8 pre_work_mode;
    u8 nosignal_flag;
    u8 signalok_flag;

    u8 checktimecnt ;
    u8 signal_check_flag;

    u8 ch1_nosignal_status;
    u8 ch2_nosignal_status;

    u8 ch1_last_signal_status;
    u8 ch2_last_signal_status;

    u8 ch1_resolution;
    u8 ch2_resolution;

    u8 ch1_work_mode;
    u8 ch2_work_mode;
    u8 ch1_pre_work_mode;
    u8 ch2_pre_work_mode;

    u8 complete_check_times;//整个流程检测次数
    u8 complete_check_flag;//整个流程检测标志
    u8 first_check_flag;
};

s32 N5_DVP_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *frame_freq);
s32 N5_DVP_power_ctl(u8 isp_dev, u8 is_work);

s32 N5_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq);

void N5_DVP_sleep();
void N5_DVP_wakeup();
void N5_DVP_W_Reg(u8 isp_dev, u16 addr, u16 val);
u16 N5_DVP_R_Reg(u8 isp_dev, u16 addr);


#endif


