/*************************************************************************
	> File Name: video_engine.h
	> Author:
	> Mail:
	> Created Time: Tue 17 Jan 2017 12:03:57 PM HKT
 ************************************************************************/

#ifndef _VIDEO_ENGINE_H
#define _VIDEO_ENGINE_H
#include "typedef.h"
#include "device/device.h"

#define    JIVE_OP_ATTR_NEW_TASK (1 << 31)
//#define    JIVE_OP_ATTR_APPEND   (1<<31)
//#define    JIVE_OP_ATTR_BATCH    (1<<30)
#define    JIVE_OP_ATTR_RESET    (1 << 30)
#define    JIVE_OP_ATTR_END     (1<<29)
#define    JIVE_OP_ATTR_MASK     (0xf<<28)  //???

#define    JIVE_MD_OBJS_MAX       512

#if 0
typedef enum {
    SOURCE_A = 1,
    SOURCE_B,
    SOURCE_C,
    FILTER0,
    FILTER1,
    FILTER2,
    SCALE0,
    SCALE1,
    PMA,
    PEC,
    CELLHIST,
    THC0,
    THC1,
    DILATE,
    ERODE,
    IIMN,
    HIST,
    COLOR,
    AOR,
    GAMMA,
    ACU,
    COPY,
    THC,
    MOMENT,
} jive_id_t;
#endif

typedef enum {
    JIVE_OP_MD = 0,
    JIVE_OP_MD1,
    JIVE_OP_INPUT0,
    JIVE_OP_INPUT1,
    JIVE_OP_INPUT2,
    JIVE_OP_OUTPUT0,
    JIVE_OP_OUTPUT1,
    JIVE_OP_OUTPUT2,
    JIVE_OP_SCALE0,
    JIVE_OP_SCALE1,
    JIVE_OP_FILTER0,
    JIVE_OP_FILTER1,
    JIVE_OP_FILTER2,
    JIVE_OP_PMA,
    JIVE_OP_PEC,
    JIVE_OP_CELLHIST,
    JIVE_OP_THC,
    JIVE_OP_THC0,
    JIVE_OP_THC1,
    JIVE_OP_DILATE,
    JIVE_OP_ERODE,
    JIVE_OP_HIST,
    JIVE_OP_IIMN,
    JIVE_OP_COLOR,
    JIVE_OP_AOR,
    JIVE_OP_GAMMA,
    JIVE_OP_ACU,
    JIVE_OP_COPY,
    JIVE_OP_MOMENT,

    //JIVE_OP_RESIZE,

    JIVE_OP_JIVE_LOCK,
    JIVE_OP_JIVE_UNLOCK,
    JIVE_OP_JLMD_LOCK,
    JIVE_OP_JLMD_UNLOCK,

    JIVE_OP_MAX,
} jive_operate_t;


typedef enum {
    JIVE_FORMAT_NULL,
    JIVE_FORMAT_GRAY,
    JIVE_FORMAT_RGB,
    JIVE_FORMAT_RGB_P, //planar
    JIVE_FORMAT_YUV420P, //planar
    JIVE_FORMAT_YUV422P, //planar
    JIVE_FORMAT_YUV444P, //planar
    JIVE_FORMAT_HSV,
    JIVE_FORMAT_HSV_P, //planar
    JIVE_FORMAT_DATA_S8,
    JIVE_FORMAT_DATA_U16,
    JIVE_FORMAT_DATA_S16,
    JIVE_FORMAT_DATA_U32,
    JIVE_FORMAT_DATA_S32,
} jive_format_t;

typedef enum {
    JIVE_INTERGRAL_HORIZONTAL,
    JIVE_INTERGRAL_VERTICAL,
} jive_integral_mode_t;

typedef enum {
    JIVE_BINARY_NORMAL,
    JIVE_BINARY_TRUNC_HIGH,
    JIVE_BINARY_TRUNC_LOW,
} jive_binary_mode_t;

typedef enum {
    JIVE_MD_MORPH_KERNEL_RECT,
    JIVE_MD_MORPH_KERNEL_CROSS,
} jive_md_morph_kernel_t;

typedef enum {
    JIVE_ERR_NONE = 0,
    JIVE_ERR_TIMEOUT = -1,
    JIVE_ERR_PARAM = -2,
    JIVE_ERR_MD_FULL = -3,
} jive_errno_t;


typedef struct imagesize {
    u16 w;
    u16 h;
    u16 stride;
    jive_format_t format;

    union {
        u8 *pixels;
        u8 *data;
        u8 *data_s8;
        u16 *data_u16;
        s16 *data_s16;
        u32 *data_u32;
    };
} jive_image_t;



typedef struct {
    jive_image_t *src;
    jive_image_t *dst;
} jive_param_generic_t;



//---jive独立模块

//input端口信息
typedef struct {
    //s16 source;
    u16 width;
    u16 height;
    u16 src_stride;
    //u16 dst_stride;
    u32 src_addr;
    //u32 dst_addr;
} jive_param_input_t;


//output端口信息
typedef struct {
    u8 output_io;
    u16 width;
    u16 height;
    u16 dst_stride;
    u32 dst_addr;
} jive_param_output_t;


//scale
typedef struct {
    u8 input_io;
    u16 scale_width;
    u16 scale_height;
    u16 scale_vcoef;
    u16 scale_hcoef;

    u16 scale_next_width;
    u16 scale_next_height;
} jive_param_scale_t;


//filter
typedef struct {
    u8 input_io;
    u8 coef_index;
    s8 coef8[9];
    s16 coef16[9];
    u8 shift_n;
    u16 score;
} jive_param_filter_t;


//pma
typedef struct {
    u8 input_io0;
    u8 input_io1;
    u8 mode;//0:canny;   1:hog
    u8 out_fmt;//0:angle+magnitude
} jive_param_pma_t;


//pec
typedef struct {
    u8 input_io0;
    u8 input_io1;
    u16 score;
} jive_param_pec_t;


//cellhist
typedef struct {
    u8 input_io;
    u8 score;
    u8 shift_n;
} jive_param_cellhist_t;


//thc
typedef struct {
    u8 input_io;
    u8 t0;
    u8 t1;
    u8 mode; //0 -- (value > t0 -> 255, value < t0 -> 0), 1 -- (value > t0 -> 255, value < t0 -> same), 2 -- (value > t0 -> same, value < t0 -> 0), 3 -- (value >= t1 -> 255,value <= t0 -> 0, t0 < value < t1 -> same)
} jive_param_thc_t;


//dilate
typedef struct {
    u8 input_io;
    s8 coef[9];
    u8 score;
} jive_param_dilate_t;


//erode
typedef struct {
    u8 input_io;
    s8 coef[9];
    u8 score;
} jive_param_erode_t;


//hist
typedef struct {
    u8 input_io;
    u8 score;
} jive_param_hist_t;


//iimn
typedef struct {
    u8 input_io;
    u8 min;
    u8 max;
    u8 shift_n;
    u8 offset;
    u8 mode;  //0 -- 2d iimn, 1 -- horizontal iimn, 2 -- vertical iimn, 3 -- sq iimn
    u8 score;
    u32 *count;
} jive_param_iimn_t;


//color
typedef struct {
    u8 input_io; //置1即可
    u8 mode;   //0 -- yuv -> rgb, 1 -- rgb -> yuv, 2 -- rgb -> hsv
    u8 format; //0 -- yuv420
} jive_param_color_t;


//aor
typedef struct {
    u8 input_io0;
    u8 input_io1;
    u8 mode;  //0 -- and, 1 -- or
} jive_param_aor_t;


//gamma
typedef struct {
    u8 input_io;
    u32 addr;
} jive_param_gamma_t;


//acu
typedef struct {
    u8 input_io0;
    u8 input_io1;
    u8 mode;   //0 -- add, 1 -- mul, 2 -- sub_abs, 3 -- sub_shift
} jive_param_acu_t;


//copy
typedef struct {
    u8 input_io;
    u8 mode;  //0 -- copy gray,  1 -- copy rgb
} jive_param_copy_t;


//moment
typedef struct {
    u8 input_io;
    u8 shift_n;
    u8 score;
} jive_param_moment_t;


typedef struct {
    int x;
    int y;
    int win_n;
    int score;
} jive_param_win_t;


typedef struct {
    u16 src_w;
    u16 src_h;
    u16 win_num_h;
    u16 win_num_v;
    u16 per_win_blocks_h;
    u16 per_win_blocks_v;
    int svm_bias_value;
    u16 svm_threshold;
    u8  norm_shift_n;
    u8  svm_shift_n;
    u16 *num;


    u32 cellhist_addr;
    u32 svm_addr;
    u32 hog_dst_addr;

} jive_param_hog_t;








typedef struct {
    u32 count;
    u8 y2;
    u8 y1;
    u8 x2;
    u8 x1;
} jive_md_obj_info_t;

typedef struct {
    jive_image_t *src;
    jive_image_t *bg;
    jive_image_t *ref;
    jive_md_obj_info_t *objs;

    u16 weight;
    u16 th;
    u8  open_times;
    u8  close_times;
    jive_md_morph_kernel_t  dilate_kernel;
    jive_md_morph_kernel_t  erode_kernel;
    u16 obj_num;    //output
    u32 fg_total;   //output
    u32 frame_num;  //output
    s32 errno;      //output

} jive_md_context_t;


struct engine_ops {
    int (*init)(void);
    void *(*open)(void *param);
    int (*ioctl)(void *handle, u32 cmd, void *param);
    int (*close)(void *handle);
};

struct engine_device {
    const char *name;
    const struct engine_ops *ops;
    void *priv;
};

extern const struct device_operations video_engine_ops; //device_operations在device.h中定义，在video_engine.c中初始化

extern struct engine_device video_engine_jive_dev; //在jive.c中初始化
extern struct engine_device video_engine_jlmd_dev;
extern struct engine_device video_engine_jlhog_dev;
#endif
