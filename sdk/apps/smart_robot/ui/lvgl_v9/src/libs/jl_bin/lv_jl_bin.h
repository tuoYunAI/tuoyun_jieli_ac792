/**
 * @file lv_bin.h
 *
 */


#ifndef LV_JL_BIN_H
#define LV_JL_BIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
/*******************JL************************/
typedef union _JLBinResourceHeaderNop {
    struct {
        uint32_t block : 2;
        uint32_t nop: 30;
    } image;
    uint32_t nop;
} JLBinResourceHeaderNop;
typedef struct {
    uint8_t res_type;
    uint8_t res_compress;
    uint16_t res_crc;
    uint32_t res_size;
    //uint32_t nop;
    JLBinResourceHeaderNop nop;
} JLBinResourceHeader;

typedef struct {
    char JL[2];
    uint16_t version;
    JLBinResourceHeader jl_header;
    void *resource;
} JLBinResource;
/*******************JL************************/

typedef struct {
    uint8_t compressed; /*lv_image_decoder.h的lv_jl_compress_t*/
    uint8_t blocks;     /*lv_image_decoder.h的lv_jl_raw_block_t*/
} jl_bin_user_data_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_res_t lv_get_img_dsc_bin(const char *src, lv_img_dsc_t *bin_dsc);
lv_res_t lv_get_JLBinResource(const char *src, JLBinResource *bin_res);
lv_image_src_t lv_get_img_src_type(const char *src);
uint8_t lv_get_compress_type(const void *src);
int lv_get_flash_src_addr(const char *src);


/**
 * Initialize the lvgl jl binary image decoder module
 */
void lv_jl_bin_decoder_init(void);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_JL_BIN_H*/


