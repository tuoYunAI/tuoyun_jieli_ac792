/*
 * file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Image library.
 *
 *
*/

#ifndef __IMLIB_H__
#define __IMLIB_H__



#include "typedef.h"
#include "os/os_api.h"

#define JIVE_TASK_MAX                           16

#define JIVE_IFMT_YUV420                        0x2
#define JIVE_IFMT_YUV422                        0x3

#define JIVE_FILTER_OFMT_B                      0x0 //8bits
#define JIVE_FILTER_OFMT_W                      0x1 //16bits

#define JIVE_CANNY_OFMT_AMP_AGL                 0x0 //
#define JIVE_CANNY_OFMT_AMP                     0x1

#define JIVE_SUBOP_COPY_THREE_CHANNEL           0x0
#define JIVE_SUBOP_COPY_SINGLE_CHANNEL          0x1

#define JIVE_SUBOP_CSC_YUV2RGB                  0x0
#define JIVE_SUBOP_CSC_RGB2YUV                  0x1
#define JIVE_SUBOP_CSC_RGB2HSV                  0x2

#define JIVE_SUBOP_AND                          0x0
#define JIVE_SUBOP_OR                           0x1
#define JIVE_SUBOP_SUB_ABS                      0x2
#define JIVE_SUBOP_SUB_SHIFT                    0x3

#define JIVE_SUBOP_INTERGRAL_2D                 0x0
#define JIVE_SUBOP_INTERGRAL_1D_HORIZONTAL      0x1
#define JIVE_SUBOP_INTERGRAL_1D_VERTICAL        0x2
#define JIVE_SUBOP_INTERGRAL_2D_SQR             0x3

#define JIVE_SUBOP_MORPH_DILATE                 0x0
#define JIVE_SUBOP_MORPH_ERODE                  0x1

#define JIVE_SUBOP_BINARY_NORMAL                0x0
#define JIVE_SUBOP_BINARY_TRUNC_HIGH            0x1
#define JIVE_SUBOP_BINARY_TRUNC_LOW             0x2

typedef enum {
    OPTYPE_COPY = 0,
    OPTYPE_FILTER,
    OPTYPE_CSC,
    OPTYPE_CANNY,
    OPTYPE_MORPH,
    OPTYPE_BINARY,
    OPTYPE_MATH,
    OPTYPE_INTERGRAL,
    OPTYPE_HIST,
    OPTYPE_GAMMA,
    OPTYPE_SCALE,
    OPTYPE_MOMENT,
} jive_optype_t;

struct integral_sq_coef {
    unsigned char min_val;
    unsigned char max_val;
    unsigned char offset;
    unsigned char shift;
};

typedef struct jive_task {
//gr0
    unsigned int   next_addr            ;
    unsigned short  in_fmt          : 2  ;
    unsigned short  acu_op          : 2  ;
    unsigned short  pma_op          : 1  ;
    unsigned short  filter0_out_fmt : 1  ;// 8 or 16 bit
    unsigned short  filter1_out_fmt : 1  ;// 8 or 16 bit
    unsigned short  filter2_out_fmt : 1  ;// 8 or 16 bit
    unsigned short  pma_out_fmt     : 1  ;// mag or ang
    unsigned short  copy            : 1  ;
    unsigned short  color_tturn     : 2  ;
    unsigned short  aor             : 1  ;
    unsigned short  integral        : 2  ;
    unsigned short  res0            : 1  ;
    unsigned short id                   ;
//gr1
    unsigned short dst1_stride          ;
    unsigned short src1_stride          ;
    unsigned short height1              ;
    unsigned short width1               ;
//gr2
    unsigned short dst2_stride          ;
    unsigned short src2_stride          ;
    unsigned short height2              ;
    unsigned short width2               ;
//gr3
    unsigned short dst3_stride          ;
    unsigned short src3_stride          ;
    unsigned short height3              ;
    unsigned short width3               ;
//gr4
    unsigned int   src1      ;
    unsigned int   dst1      ;
//gr5
    unsigned int   src2      ;
    unsigned int   dst2      ;
//gr6
    unsigned int   src3      ;
    unsigned int   dst3      ;
//gr7
    signed short mask0_0        ;
    signed short mask0_1        ;
    signed short mask0_2        ;
    signed short mask0_3        ;
//gr8
    signed short mask0_4        ;
    signed short mask0_5        ;
    signed short mask0_6        ;
    signed short mask0_7        ;
//gr9
    signed short mask1_0        ;
    signed short mask1_1        ;
    signed short mask1_2        ;
    signed short mask1_3        ;
//gr10
    signed short mask1_4        ;
    signed short mask1_5        ;
    signed short mask1_6        ;
    signed short mask1_7        ;
//gr11
    signed short mask2_0        ;
    signed short mask2_1        ;
    signed short mask2_2        ;
    signed short mask2_3        ;
//gr12
    signed short mask2_4        ;
    signed short mask2_5        ;
    signed short mask2_6        ;
    signed short mask2_7        ;
//gr13
    signed short filter0_mask8      ;
    unsigned       filter0_shift   : 8 ;
    unsigned       thc0_thd1      : 8 ;
    signed short filter1_mask8      ;
    unsigned       thc1_thd1      : 8 ;
    unsigned       filter1_shift  : 8 ;
//gr14
    signed short filter2_mask8        ;
    unsigned       filter2_shift  : 8    ;
    unsigned       thc_thd1      : 8    ;
    unsigned       dilate_mask8  : 8    ;
    unsigned       erode_mask8   : 8    ;
    unsigned       filter0_mux1  : 5    ;
    unsigned       filter1_mux2  : 5    ;
    unsigned       filter2_mux3  : 5    ;
    unsigned       res14         : 1    ;
//gr15
    unsigned      dilate_mask0   : 8    ;
    unsigned      dilate_mask1   : 8    ;
    unsigned      dilate_mask2   : 8    ;
    unsigned      dilate_mask3   : 8    ;
    unsigned      dilate_mask4   : 8    ;
    unsigned      dilate_mask5   : 8    ;
    unsigned      dilate_mask6   : 8    ;
    unsigned      dilate_mask7   : 8    ;
//gr16
    unsigned      erode_mask0   : 8    ;
    unsigned      erode_mask1   : 8    ;

    unsigned      erode_mask2   : 8    ;
    unsigned      erode_mask3   : 8    ;
    unsigned      erode_mask4   : 8    ;
    unsigned      erode_mask5   : 8    ;
    unsigned      erode_mask6   : 8    ;
    unsigned      erode_mask7   : 8    ;
//gr17
    unsigned long long     scale0_mux4   : 5    ;
    unsigned long long     scale1_mux5   : 5    ;
    unsigned long long     pma0_mux6     : 5    ;
    unsigned long long     pma1_mux7     : 5    ;
    unsigned long long     pec0_mux8     : 5    ;
    unsigned long long     pec1_mux9     : 5    ;
    unsigned long long     cellhist_mux10: 1    ;
    unsigned long long     thc0_mux11    : 5    ;
    unsigned long long     thc1_mux12    : 5    ;
    unsigned long long     dilate_mux13  : 5    ;
    unsigned long long     hist_mux14    : 5    ;
    unsigned long long     iimn_mux15    : 5    ;
    unsigned long long     erode_mux16   : 5    ;
    unsigned long long     res16         : 3    ;
//gr18
    unsigned long long     out0_mux17   : 5    ;
    unsigned long long     out1_mux18   : 5    ;
    unsigned long long     out2_mux19   : 5    ;
    unsigned long long     color_mux20  : 5    ;
    unsigned long long     aor0_mux21   : 5    ;
    unsigned long long     aor1_mux22   : 5    ;
    unsigned long long     gamma_mux23  : 5    ;
    unsigned long long     acu0_mux24   : 5    ;
    unsigned long long     acu1_mux25   : 5    ;
    unsigned long long     copy_mux26   : 5    ;
    unsigned long long     thc_mux27    : 5    ;
    unsigned long long     moment_mux28 : 5    ;
    unsigned long long     res18        : 4    ;
//gr19
    unsigned      iimn_shift   : 8    ;
    unsigned      iimn_min     : 8    ;
    unsigned      iimn_max     : 8    ;
    unsigned      iimn_offset  : 8    ;
    unsigned      thc0_thd0    : 8    ;
    unsigned      thc1_thd0    : 8    ;
    unsigned      thc_thd0      : 8    ;
    unsigned      thc0_mode    : 2    ;
    unsigned      thc1_mode    : 2    ;
    unsigned      thc_mode     : 2    ;
    unsigned      res19        : 2    ;
//gr20
    unsigned short scale0_hcoef    ;
    unsigned short scale0_vcoef    ;
    unsigned short scale0_height   ;
    unsigned short scale0_width    ;
//gr21
    unsigned short scale1_hcoef    ;
    unsigned short scale1_vcoef    ;
    unsigned short scale1_height   ;
    unsigned short scale1_width    ;
//gr22
    unsigned short out1_width     ;
    unsigned short out1_height    ;
    unsigned short out2_width     ;
    unsigned short out2_height    ;
//gr23
    unsigned short out3_width        ;
    unsigned short out3_height       ;
    unsigned       moment_shift  : 8 ;
    unsigned       iimn_score    : 3 ;
    unsigned       moment_score  : 3 ;
    unsigned       filter0_score : 3 ;
    unsigned       filter1_score : 3 ;
    unsigned       filter2_score : 3 ;
    unsigned       pec_score     : 3 ;
    unsigned       cellhist_score: 3 ;
    unsigned       dilate_score  : 3 ;
//gr24
    unsigned int   gamma_base_adr     ;
    unsigned       hist_score       : 3 ;
    unsigned       erode_score      : 3 ;
    unsigned       cellhist_shift   : 4 ;
    unsigned int   res24            : 22;


    unsigned  short     scale0_next_w;
    unsigned  short     scale0_next_h;
    unsigned  short     scale1_next_w;
    unsigned  short     scale1_next_h;

    unsigned int   *intergral_sq_count_ptr;

    unsigned int   pads[3];
} task_t __attribute__((aligned(64)));

struct jive_fh {
    struct jive_task *task_tbl;
    int max_task_num;
    u8  empty_task_idx;
    u16 task_id;

    u32 *integral_sq_count_ptr;
    OS_SEM sem;
    u8 state;
};


#endif

