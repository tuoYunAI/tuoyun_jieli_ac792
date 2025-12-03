/**
 * @file lv_font.h
 *
 */

#ifndef LV_FONT_H
#define LV_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"
#include "../misc/lv_types.h"

#include "lv_symbol_def.h"
#include "../draw/lv_draw_buf.h"
#include "../misc/lv_area.h"
#include "../misc/lv_ll.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*------------------
 * General types
 *-----------------*/

#if ((LV_USE_DRAW_JLVG == 1) && (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1))

typedef struct _lv_jlvg_path_segment_t {
    uint8_t type;
    lv_point_t to;
    lv_point_t control1;
    lv_point_t control2;
} lv_jlvg_path_segment_t;

typedef struct _lv_jlvg_glyph_ori_info_t {
    // freetype 解析得到的原始的路径信息
    lv_jlvg_path_segment_t *path_segments;   // 绘制完整的路径描述段
    uint32_t segments_number;   // 累加路径描述段的数量

    // unicode
    //uint32_t unicode_letter;
} lv_jlvg_glyph_ori_info_t;

typedef struct _lv_jlvg_glyph_info_t {
    // freetype 解析得到的原始的路径信息
    uint8_t *segments;      // 绘制完整的路径描述段
    float *points;            // 绘制结束需要的路径描述点
    uint32_t segments_ofs;  // 累加路径描述段的数量
    uint32_t points_ofs;    // 累加路径描述点的数量

    // freetype 解析得到的字形的度量信息
    //uint16_t contour_num;       // 字形的轮廓数, 比如字形 ‘i’ 就有两个闭合轮廓组成
    //uint16_t point_num;         // 字形的轮廓点数量
    //int hori_advance;           // 字形水平步进值
    //void *font_info;            // 指向字库解析信息本身
    void *label_info;           // 指向文本信息本身

    // unicode
    //uint32_t unicode_letter;
    lv_point_t pos;             // 字符相对于 label 在绘制显存区域的起点位置
} lv_jlvg_glyph_info_t;

typedef struct _lv_jlvg_label_info_t {
    // 文本信息
    lv_area_t effective_coords;   // 文本的有效区域

    // 解析的字库信息
    uint16_t font_size;
    uint16_t units_per_em;  // FT 中原像素的比例单位, 定义了每个EM方块（EM square）内的字体单位数量
    float original_scale;   // 原始缩放比例: font_size / units_per_em;
    short ascender;         // 字体的上行指的是从基线到小写字母顶部（如字母“b”、“d”或“h”的顶部）的距离
    short descender;        // 字体的下行指的是从基线到底部伸出部分（如字母“g”、“j”或“p”的尾部）的距离
    short height;           // 两个连续基线之间的垂直距离, 即全局字形的高度, 可以使用ascender - descender来计算

    // 解析的字形信息
    lv_ll_t glyph_info_list;

    // JLVG 的绘制信息
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf;
    lv_color_format_t dest_cf;
    uint32_t bytes_per_pixel;
} lv_jlvg_label_info_t;

typedef struct _lv_font_jlvg_privately_t {
    bool is_vector;

    /** Create the label drawing task callback */
    bool (*label_draw_task_create_cb)(lv_draw_unit_t *draw_unit, const lv_font_t *font_p, const lv_area_t *coords, lv_jlvg_label_info_t *label_info);
} lv_font_jlvg_privately_t;
#endif /*((LV_USE_DRAW_JLVG == 1) && (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1))*/

/** The font format.*/
typedef enum {
    LV_FONT_GLYPH_FORMAT_NONE   = 0, /**< Maybe not visible*/

    /**< Legacy simple formats with no byte padding at end of the lines*/
    LV_FONT_GLYPH_FORMAT_A1     = 0x01, /**< 1 bit per pixel*/
    LV_FONT_GLYPH_FORMAT_A2     = 0x02, /**< 2 bit per pixel*/
    LV_FONT_GLYPH_FORMAT_A3     = 0x03, /**< 3 bit per pixel*/
    LV_FONT_GLYPH_FORMAT_A4     = 0x04, /**< 4 bit per pixel*/
    LV_FONT_GLYPH_FORMAT_A8     = 0x08, /**< 8 bit per pixel*/

    LV_FONT_GLYPH_FORMAT_IMAGE  = 0x19, /**< Image format*/

    /**< Advanced formats*/
    LV_FONT_GLYPH_FORMAT_VECTOR = 0x1A, /**< Vectorial format*/
    LV_FONT_GLYPH_FORMAT_SVG    = 0x1B, /**< SVG format*/
    LV_FONT_GLYPH_FORMAT_CUSTOM = 0xFF, /**< Custom format*/
} lv_font_glyph_format_t;

/** Describes the properties of a glyph.*/
typedef struct {
    const lv_font_t *
    resolved_font;  /**< Pointer to a font where the glyph was actually found after handling fallbacks*/
    uint16_t adv_w; /**< The glyph needs this space. Draw the next glyph after this width.*/
    uint16_t box_w; /**< Width of the glyph's bounding box*/
    uint16_t box_h; /**< Height of the glyph's bounding box*/
    int16_t ofs_x;  /**< x offset of the bounding box*/
    int16_t ofs_y;  /**< y offset of the bounding box*/
    uint16_t stride;/**< Bytes in each line. If 0 than there is no padding at the end of the line. */
    lv_font_glyph_format_t format;  /**< Font format of the glyph see lv_font_glyph_format_t */
    uint8_t is_placeholder: 1;      /**< Glyph is missing. But placeholder will still be displayed*/

    /** 0: Get bitmap should return an A8 or ARGB8888 image.
      * 1: return the bitmap as it is (Maybe A1/2/4 or any proprietary formats). */
    uint8_t req_raw_bitmap: 1;

    int32_t outline_stroke_width;   /**< used with freetype vector fonts - width of the letter outline */

    union {
        uint32_t index;       /**< Unicode code point*/
        const void *src;      /**< Pointer to the source data used by image fonts*/
    } gid;                    /**< The index of the glyph in the font file. Used by the font cache*/
    lv_cache_entry_t *entry;  /**< The cache entry of the glyph draw data. Used by the font cache*/
#if ((LV_USE_DRAW_JLVG == 1) && (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1))
    lv_jlvg_label_info_t *label_info;   // 字形所在的标签的信息
    const lv_point_t *pos;
#endif
} lv_font_glyph_dsc_t;

/** The bitmaps might be upscaled by 3 to achieve subpixel rendering.*/
typedef enum {
    LV_FONT_SUBPX_NONE,
    LV_FONT_SUBPX_HOR,
    LV_FONT_SUBPX_VER,
    LV_FONT_SUBPX_BOTH,
} lv_font_subpx_t;

/** Adjust letter spacing for specific character pairs.*/
typedef enum {
    LV_FONT_KERNING_NORMAL,
    LV_FONT_KERNING_NONE,
} lv_font_kerning_t;

/** Describe the properties of a font*/
struct _lv_font_t {
    /** Get a glyph's descriptor from a font*/
    bool (*get_glyph_dsc)(const lv_font_t *, lv_font_glyph_dsc_t *, uint32_t letter, uint32_t letter_next);

    /** Get a glyph's bitmap from a font*/
    const void *(*get_glyph_bitmap)(lv_font_glyph_dsc_t *, lv_draw_buf_t *);

    /** Release a glyph*/
    void (*release_glyph)(const lv_font_t *, lv_font_glyph_dsc_t *);

    /*Pointer to the font in a font pack (must have the same line height)*/
    int32_t line_height;         /**< The real line height where any text fits*/
    int32_t base_line;           /**< Base line measured from the bottom of the line_height*/
    uint8_t subpx   : 2;            /**< An element of `lv_font_subpx_t`*/
    uint8_t kerning : 1;            /**< An element of `lv_font_kerning_t`*/
    uint8_t static_bitmap : 1;      /**< The font will be used as static bitmap */

    int8_t underline_position;      /**< Distance between the top of the underline and base line (< 0 means below the base line)*/
    int8_t underline_thickness;     /**< Thickness of the underline*/

    const void *dsc;                /**< Store implementation specific or run_time data or caching here*/
    const lv_font_t *fallback;    /**< Fallback font for missing glyph. Resolved recursively */
    void *user_data;                /**< Custom user data for font.*/
};

struct _lv_font_class_t {
    lv_font_t *(*create_cb)(const lv_font_info_t *info, const void *src);    /**< Font creation callback function*/
    void (*delete_cb)(lv_font_t *font);     /**< Font deletion callback function*/
    void *(*dup_src_cb)(const void *src);   /**< Font source duplication callback function*/
    void (*free_src_cb)(void *src);         /**< Font source free callback function*/
};

struct _lv_font_info_t {
    const char *name;                /**< Font name, used to distinguish different font resources*/
    const lv_font_class_t *class_p;  /**< Font backend implementation*/
    uint32_t size;                   /**< Font size in pixel*/
    uint32_t render_mode;            /**< Font rendering mode, see `lv_freetype_font_render_mode_t`*/
    uint32_t style;                  /**< Font style, see `lv_freetype_font_style_t`*/
    lv_font_kerning_t kerning;       /**< Font kerning, see `lv_font_kerning_t`*/
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#if ((LV_USE_DRAW_JLVG == 1) && (LV_USE_DRAW_JLVG_LABEL_ENABLE == 1))
/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_font_jlvg_draw_check_is_vector
 *
 * @Params user_data
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
bool lv_font_jlvg_draw_check_is_vector(const lv_font_t *font_p);


bool lv_jlvg_label_draw_task_create(lv_draw_unit_t *draw_unit, const lv_font_t *font_p, const lv_area_t *coords, lv_jlvg_label_info_t *label_info);
#endif
/**
 * Return with the bitmap of a font.
 * It always converts the normal fonts to A8 format in a draw_buf with
 * LV_DRAW_BUF_ALIGN and LV_DRAW_BUF_STRIDE_ALIGN
 * @note You must call lv_font_get_glyph_dsc() to get `g_dsc` (lv_font_glyph_dsc_t)
 *       before you can call this function.
 * @param g_dsc         the glyph descriptor including which font to use, which supply the glyph_index
 *                      and the format.
 * @param draw_buf      a draw buffer that can be used to store the bitmap of the glyph.
 * @return              pointer to the glyph's data.
 *                      It can be a draw buffer for bitmap fonts or an image source for imgfonts.
 */
const void *lv_font_get_glyph_bitmap(lv_font_glyph_dsc_t *g_dsc, lv_draw_buf_t *draw_buf);


/**
 * Return the bitmap as it is. It works only if the font stores the bitmap in
 * a non-volitile memory.
 * @param g_dsc         the glyph descriptor including which font to use, which supply the glyph_index
 *                      and the format.
 * @return              the bitmap as it is
 */
const void *lv_font_get_glyph_static_bitmap(lv_font_glyph_dsc_t *g_dsc);

/**
 * Get the descriptor of a glyph
 * @param font          pointer to font
 * @param dsc_out       store the result descriptor here
 * @param letter        a UNICODE letter code
 * @param letter_next   the next letter after `letter`. Used for kerning
 * @return true: descriptor is successfully loaded into `dsc_out`.
 *         false: the letter was not found, no data is loaded to `dsc_out`
 */
bool lv_font_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t letter,
                           uint32_t letter_next);

/**
 * Release the bitmap of a font.
 * @note You must call lv_font_get_glyph_dsc() to get `g_dsc` (lv_font_glyph_dsc_t) before you can call this function.
 * @param g_dsc         the glyph descriptor including which font to use, which supply the glyph_index and the format.
 */
void lv_font_glyph_release_draw_data(lv_font_glyph_dsc_t *g_dsc);

/**
 * Get the width of a glyph with kerning
 * @param font          pointer to a font
 * @param letter        a UNICODE letter
 * @param letter_next   the next letter after `letter`. Used for kerning
 * @return the width of the glyph
 */
uint16_t lv_font_get_glyph_width(const lv_font_t *font, uint32_t letter, uint32_t letter_next);

/**
 * Get the line height of a font. All characters fit into this height
 * @param font      pointer to a font
 * @return the height of a font
 */
int32_t lv_font_get_line_height(const lv_font_t *font);

/**
 * Configure the use of kerning information stored in a font
 * @param font    pointer to a font
 * @param kerning `LV_FONT_KERNING_NORMAL` (default) or `LV_FONT_KERNING_NONE`
 */
void lv_font_set_kerning(lv_font_t *font, lv_font_kerning_t kerning);

/**
 * Get the default font, defined by LV_FONT_DEFAULT
 * @return  return      pointer to the default font
 */
const lv_font_t *lv_font_get_default(void);

/**
 * Compare font information.
 * @param ft_info_1 font information 1.
 * @param ft_info_2 font information 2.
 * @return return true if the fonts are equal.
 */
bool lv_font_info_is_equal(const lv_font_info_t *ft_info_1, const lv_font_info_t *ft_info_2);

/**
 * Checks if a font has a static rendering bitmap.
 * @param font    pointer to a font
 * @return return true if the font has a bitmap generated for static rendering.
 */
bool lv_font_has_static_bitmap(const lv_font_t *font);


#if LV_USE_FONT_SCALE
void lv_set_scale_base_font(const lv_font_t *font_p);
#endif
/**********************
 *      MACROS
 **********************/

#define LV_FONT_DECLARE(font_name) LV_ATTRIBUTE_EXTERN_DATA extern const lv_font_t font_name;

#if LV_FONT_MONTSERRAT_8
LV_FONT_DECLARE(lv_font_montserrat_8)
#endif

#if LV_FONT_MONTSERRAT_10
LV_FONT_DECLARE(lv_font_montserrat_10)
#endif

#if LV_FONT_MONTSERRAT_12
LV_FONT_DECLARE(lv_font_montserrat_12)
#endif

#if LV_FONT_MONTSERRAT_14
LV_FONT_DECLARE(lv_font_montserrat_14)
#endif

#if LV_FONT_MONTSERRAT_16
LV_FONT_DECLARE(lv_font_montserrat_16)
#endif

#if LV_FONT_MONTSERRAT_18
LV_FONT_DECLARE(lv_font_montserrat_18)
#endif

#if LV_FONT_MONTSERRAT_20
LV_FONT_DECLARE(lv_font_montserrat_20)
#endif

#if LV_FONT_MONTSERRAT_22
LV_FONT_DECLARE(lv_font_montserrat_22)
#endif

#if LV_FONT_MONTSERRAT_24
LV_FONT_DECLARE(lv_font_montserrat_24)
#endif

#if LV_FONT_MONTSERRAT_26
LV_FONT_DECLARE(lv_font_montserrat_26)
#endif

#if LV_FONT_MONTSERRAT_28
LV_FONT_DECLARE(lv_font_montserrat_28)
#endif

#if LV_FONT_MONTSERRAT_30
LV_FONT_DECLARE(lv_font_montserrat_30)
#endif

#if LV_FONT_MONTSERRAT_32
LV_FONT_DECLARE(lv_font_montserrat_32)
#endif

#if LV_FONT_MONTSERRAT_34
LV_FONT_DECLARE(lv_font_montserrat_34)
#endif

#if LV_FONT_MONTSERRAT_36
LV_FONT_DECLARE(lv_font_montserrat_36)
#endif

#if LV_FONT_MONTSERRAT_38
LV_FONT_DECLARE(lv_font_montserrat_38)
#endif

#if LV_FONT_MONTSERRAT_40
LV_FONT_DECLARE(lv_font_montserrat_40)
#endif

#if LV_FONT_MONTSERRAT_42
LV_FONT_DECLARE(lv_font_montserrat_42)
#endif

#if LV_FONT_MONTSERRAT_44
LV_FONT_DECLARE(lv_font_montserrat_44)
#endif

#if LV_FONT_MONTSERRAT_46
LV_FONT_DECLARE(lv_font_montserrat_46)
#endif

#if LV_FONT_MONTSERRAT_48
LV_FONT_DECLARE(lv_font_montserrat_48)
#endif

#if LV_FONT_MONTSERRAT_28_COMPRESSED
LV_FONT_DECLARE(lv_font_montserrat_28_compressed)
#endif

#if LV_FONT_DEJAVU_16_PERSIAN_HEBREW
LV_FONT_DECLARE(lv_font_dejavu_16_persian_hebrew)
#endif

#if LV_FONT_SIMSUN_14_CJK
LV_FONT_DECLARE(lv_font_simsun_14_cjk)
#endif

#if LV_FONT_SIMSUN_16_CJK
LV_FONT_DECLARE(lv_font_simsun_16_cjk)
#endif

#if LV_FONT_SOURCE_HAN_SANS_SC_14_CJK
LV_FONT_DECLARE(lv_font_source_han_sans_sc_14_cjk)
#endif

#if LV_FONT_SOURCE_HAN_SANS_SC_16_CJK
LV_FONT_DECLARE(lv_font_source_han_sans_sc_16_cjk)
#endif

#if LV_FONT_UNSCII_8
LV_FONT_DECLARE(lv_font_unscii_8)
#endif

#if LV_FONT_UNSCII_16
LV_FONT_DECLARE(lv_font_unscii_16)
#endif

/*Declare the custom (user defined) fonts*/
#ifdef LV_FONT_CUSTOM_DECLARE
LV_FONT_CUSTOM_DECLARE
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*USE_FONT*/
