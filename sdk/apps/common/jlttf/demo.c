#include "app_config.h"
#include "system/includes.h"
#include "storage_device.h"
#include "lcd_config.h"
#include "fs/fs.h"

// 说明： 该 demo 的测试环境：JLVG 绘制 + 文件位于 FLASH + TTF 文件必须使用 cmap 表 format4 + 不支持复合字符

/*********************
 *      DEFINES
 *********************/
#include "asm/gpu/jlvg.h"
#include "asm/jlgpu_driver.h"

static jlvg_t *jlvg;
static uint32_t begin_time;
static uint32_t end_time;
static uint32_t hw_cfgtime_begin;
static uint32_t hw_cfgtime;
static uint32_t sw_blendtime;
static uint32_t cfg_path_time;

// 需要生成字库文件并烧写到 FLASH
#define CONFIG_FONT_TTF_SUB0 "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub0.ttf"       // 风急天高猿啸哀渚清沙白鸟飞回无边落木萧萧下不尽长江滚滚来万里悲秋常作客百年多病独登台艰难苦恨繁霜鬓潦倒新停浊酒杯
#define CONFIG_FONT_TTF_SUB1 "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub1.ttf"       // 龘
#define CONFIG_FONT_TTF_SUB2 "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub2.ttf"       // 841 个汉字
#define CONFIG_FONT_TTF_TEST "mnt/sdfile/EXT_RESERVED/uipackres/ui/test_zh.ttf"    // 杰理科技股份有限公司

#define VG_DRAW_FULL_DISP_ENBLE 0   // 调试用，开全屏窗口性能会明显下降

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define   BLACK     0x0000       //   黑色
#define   NAVY      0x000F       //    深蓝色
#define   DGREEN    0x03E0       //  深绿色
#define   DCYAN     0x03EF       //   深青色
#define   MAROON    0x7800       //   深红色
#define   PURPLE    0x780F       //   紫色
#define   OLIVE     0x7BE0       //   橄榄绿
#define   LGRAY     0xC618       //  灰白色
#define   DGRAY     0x7BEF       //  深灰色
#define   BLUE      0x001F       //  蓝色
#define   GREEN     0x07E0       //  绿色
#define   CYAN      0x07FF       //  青色
#define   RED       0xF800       //  红色
#define   MAGENTA   0xF81F       //  品红
#define   YELLOW    0xFFE0       //  黄色
#define   WHITE     0xFFFF       //  白色

static u16 char_color[16] = {
    BLACK,
    NAVY,
    DGREEN,
    DCYAN,
    MAROON,
    PURPLE,
    OLIVE,
    LGRAY,
    DGRAY,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE,
};

static u16 back_color[16] = {
    BLACK,
    NAVY,
    DGREEN,
    DCYAN,
    MAROON,
    PURPLE,
    OLIVE,
    LGRAY,
    DGRAY,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE,
};

typedef struct _stbttf_table_t {
    uint32_t table_ofs;
    uint32_t table_len; // 单位：字节
} stbttf_table_t;

typedef struct _stbttf_table_list_t {
    stbttf_table_t head;
    stbttf_table_t maxp;
    stbttf_table_t hhea;
    stbttf_table_t hmtx;
    stbttf_table_t cmap;
    stbttf_table_t loca;
    stbttf_table_t glyf;
} stbttf_table_list_t;

typedef struct _cmap_format4_info_t {
    uint16_t cmap_format4_segcont;  // camp format4 的段数
    uint16_t *end_code;
    uint16_t *start_code;
    uint16_t *id_delta;
    uint16_t *id_range_offset;
    //uint16_t *glyph_index_array;
} cmap_format4_info_t;

typedef struct _stbttf_info_t {
    uint16_t units_per_em;          // 字库的量度单位
    //int16_t x_min;                  // 字库中所有字形的最小 x 坐标
    //int16_t y_min;                  // 字库中所有字形的最小 y 坐标
    //int16_t x_max;                  // 字库中所有字形的最大 x 坐标
    //int16_t y_max;                  // 字库中所有字形的最大 y 坐标
    int16_t ascent;                     // 从基线到最高上升部分的距离
    int16_t descent;                    // 从基线到最低下降部分的距离
    int16_t line_gap;                   // 行间距，用于控制文本行之间的垂直间距
    uint16_t num_of_long_hor_metrics;   // 水平度量表 (hmtx) 中的前进宽度条目数量
    uint8_t *hor_metrics1;              // 水平度量表1，每个成员包括前进宽度以及左侧边距
    uint16_t last_advance_width;        // 水平度量表1的最后一个前进宽度
    uint8_t *hor_metrics2;              // 水平度量表2, 表2只有左侧边距数组，前进宽度数组是表1的最后一个前进宽度
    int16_t index2loca_format;      // loca 表的数据位宽：0 - 16bit；1 - 32bit；
    uint16_t glyphs_num;            // ttf 字库包含的字形数量；
    cmap_format4_info_t format4_info;
} stbttf_info_t;

typedef struct _ttf_stream_t {
    const void *data;   // TTF 文件在 FLASH 中的地址
    size_t size;
    uint8_t flag;   // 0:flash; 1:sd; 2:static C array
} ttf_stream_t;

typedef struct _ttf_font_desc_t {
    ttf_stream_t stream;
    stbttf_info_t info;
    stbttf_table_list_t list;
} ttf_font_desc_t;

typedef struct _jlvg_path_info_t {
    int16_t x_min;      // 当前字形的最小 x 坐标
    int16_t y_min;      // 当前字形的最小 y 坐标
    int16_t x_max;      // 当前字形的最大 x 坐标
    int16_t y_max;      // 当前字形的最大 y 坐标
    uint8_t *segments;  // 绘制完整的路径动作
    float *points;      // 绘制结束需要的路径坐标
    uint32_t segments_ofs;
    uint32_t points_ofs;
    uint16_t advance_width;    // 水平前进宽度
    int16_t left_side_bearing;      // 水平左侧边距
} jlvg_path_info_t;

static void jl_ttf_set_size(ttf_font_desc_t *font_des, uint16_t font_size)
{
    if (font_size <= 0) {
        printf("invalid font size: %d.", font_size);
        return;
    }
}

static bool stbttf_get_tab(stbttf_table_list_t *list, ttf_stream_t *stream)
{
    uint8_t *curr_data = stream->data;
    uint16_t table_num;
    // 取出表的数量
    curr_data += 4;   // 文件头版本号偏移

    table_num = (curr_data[0] << 8) | curr_data[1];
    curr_data += 2;  // 表数量偏移
    curr_data += 2;  // 二分法查找参数1偏移
    curr_data += 2;  // 二分法查找参数2偏移
    curr_data += 2;  // 二分法查找参数3偏移

    uint8_t mast_tab_cont = 0;   // 有效表计数
    // 取出表的偏移地址和长度
    for (int i = 0; i < table_num; i++) {
        char table_name[5];
        memcpy(table_name, curr_data, 4);
        table_name[4] = '\0';
        curr_data += 4;  // 表名字偏移
        curr_data += 4;  // 表校验和偏移
        if (strcmp(table_name, "head") == 0) {
            list->head.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->head.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else if (strcmp(table_name, "maxp") == 0) {
            list->maxp.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->maxp.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else if (strcmp(table_name, "hhea") == 0) {
            list->hhea.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->hhea.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else if (strcmp(table_name, "hmtx") == 0) {
            list->hmtx.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->hmtx.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else if (strcmp(table_name, "cmap") == 0) {
            list->cmap.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->cmap.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else if (strcmp(table_name, "loca") == 0) {
            list->loca.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->loca.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else if (strcmp(table_name, "glyf") == 0) {
            list->glyf.table_ofs = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表地址偏移
            list->glyf.table_len = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
            curr_data += 4;  // 表长度偏移
            mast_tab_cont++;
        } else {
            // nothing
            curr_data += 4;  // 表地址偏移
            curr_data += 4;  // 表长度偏移
        }
    }

#if 0
    // dump table list
    printf("head:");
    printf("table ofs = 0x%08x.", list->head.table_ofs);
    printf("table len = %d.", list->head.table_len);
    printf("\n");

    printf("maxp:");
    printf("table ofs = 0x%08x.", list->maxp.table_ofs);
    printf("table len = %d.", list->maxp.table_len);
    printf("\n");

    printf("hhea:");
    printf("table ofs = 0x%08x.", list->hhea.table_ofs);
    printf("table len = %d.", list->hhea.table_len);
    printf("\n");

    printf("hmtx:");
    printf("table ofs = 0x%08x.", list->hmtx.table_ofs);
    printf("table len = %d.", list->hmtx.table_len);
    printf("\n");

    printf("cmap:");
    printf("table ofs = 0x%08x.", list->cmap.table_ofs);
    printf("table len = %d.", list->cmap.table_len);
    printf("\n");

    printf("loca:");
    printf("table ofs = 0x%08x.", list->loca.table_ofs);
    printf("table len = %d.", list->loca.table_len);
    printf("\n");

    printf("glyf:");
    printf("table ofs = 0x%08x.", list->glyf.table_ofs);
    printf("table len = %d.", list->glyf.table_len);
    printf("\n");
#endif

    if (mast_tab_cont < 6) {    // 必须有这六个表,否则字库文件时损坏的或者不支持的
        return false;
    } else {
        return true;
    }
}

static bool stbttf_info_init(stbttf_info_t *info, stbttf_table_list_t *list, ttf_stream_t *stream)
{
    uint8_t *curr_data;
    // 解析 head 表
    curr_data = stream->data + list->head.table_ofs;
    //curr_data += 50;    // 偏移50个字节

    curr_data += 18;    // 4 + 4 + 4 + 4 + 2 : version - fontRevision - checkSumAdjustment - magicNumber - flags
    // 量度单位的获取
    info->units_per_em = (curr_data[0] << 8) | curr_data[1];  // 64 - 16384   : 2 size
    printf("units_per_em = %d.", info->units_per_em);

    curr_data += 16;    // 2 + 8 + 8 : unitsPerEm - created - modified

    // 所有字形的最大最小边界
    //info->x_min = (curr_data[0] << 8) | curr_data[1];
    curr_data += 2;
    //info->y_min = (curr_data[0] << 8) | curr_data[1];
    curr_data += 2;
    //info->x_max = (curr_data[0] << 8) | curr_data[1];
    curr_data += 2;
    //info->y_max = (curr_data[0] << 8) | curr_data[1];
    curr_data += 2;
    //printf("(x_min, y_min) = (%d, %d); (x_max, y_max) = (%d, %d).", info->x_min, info->y_min, info->x_max, info->y_max);

    curr_data += 6;     // 2 + 2 + 2 : macStyle - lowestRecPPEM - fontDirectionHint

    // loca 表格式的获取
    info->index2loca_format = (curr_data[0] << 8) | curr_data[1];
    //printf("index2loca_format = %d.\n", info->index2loca_format);

    // 解析 maxp 表
    curr_data = stream->data + list->maxp.table_ofs;
    curr_data += 4;    // 偏移4个字节(version)
    info->glyphs_num = (curr_data[0] << 8) | curr_data[1];
    //printf("glyphs_num = %d.\n", info->glyphs_num);

    // 解析 hhea 表
    curr_data = stream->data + list->hhea.table_ofs;
    curr_data += 4;    // 偏移4个字节(version)
    info->ascent = (curr_data[0] << 8) | curr_data[1];  // 最高上升部分的距离，即从基线到最高上升部分的距离。
    curr_data += 2;
    info->descent = (curr_data[0] << 8) | curr_data[1]; // 最低下降部分的距离，即从基线到最低下降部分的距离。
    curr_data += 2;
    info->line_gap = (curr_data[0] << 8) | curr_data[1];    // 行间距，用于控制文本行之间的垂直间距。
    curr_data += 2;

    curr_data += 8;    // 2 + 2 + 2 + 2 : advanceWidthMax - minLeftSideBearing - minRightSideBearing - xMaxExtent
    curr_data += 6;     // 2 + 2 + 2 : caretSlopeRise - caretSlopeRun - caretOffset
    curr_data += 8;     // 2 + 2 + 2 + 2 : reserved - reserved - reserved - reserved
    curr_data += 2;     // 2 : metricDataFormat
    info->num_of_long_hor_metrics = (curr_data[0] << 8) | curr_data[1]; // 水平度量表 (hmtx) 中的前进宽度条目数量。
    curr_data += 2;
    printf("ascent = %d; descent = %d; line_gap = %d; num_of_long_hor_metrics = %d.\n", info->ascent, info->descent, info->line_gap, info->num_of_long_hor_metrics);

    // 解析 hmtx 表
    curr_data = stream->data + list->hmtx.table_ofs;

    // 水平量度表1
    info->hor_metrics1 = curr_data;
    curr_data += 4 * (info->num_of_long_hor_metrics - 1);   // num_of_long_hor_metrics - 1 个水平度量表
    info->last_advance_width = (curr_data[0] << 8) | curr_data[1];
    curr_data += 4; // 2 + 2: last longHorMetric {advanceWidth + leftSideBearing}
    // 水平量度表2
    info->hor_metrics2 = curr_data;
    printf("last_advance_width = %d.\n", info->last_advance_width);

    // 解析 cmap 表，根据字符编码，计算得到 loca 表的字形索引
    curr_data = stream->data + list->cmap.table_ofs;
    curr_data += 2;    // 偏移2个字节(version)
    uint16_t cmap_subtab_number = (curr_data[0] << 8) | curr_data[1];
    curr_data += 2;    // 偏移2个字节(numberSubtables)
    //printf("cmap_subtab_number = %d.\n", cmap_subtab_number);

    uint16_t cmap_platformID[cmap_subtab_number];
    uint16_t cmap_platformSpecificID[cmap_subtab_number];
    uint32_t cmap_offset[cmap_subtab_number];
    uint16_t cmap_format_id[cmap_subtab_number];
    uint16_t cmap_format_len[cmap_subtab_number];
    uint16_t cmap_format_language[cmap_subtab_number];
    int subtab_id;
    uint16_t segcountX2;
    for (subtab_id = 0; subtab_id < cmap_subtab_number; subtab_id ++) {
        cmap_platformID[subtab_id] = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2; // 偏移两个字节(platformID)
        cmap_platformSpecificID[subtab_id] = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2; // 偏移两个字节(platformSpecificID)
        cmap_offset[subtab_id] = curr_data[0] << 24 | curr_data[1] << 16 | curr_data[2] << 8 | curr_data[3];
        curr_data += 4; // 偏移四个字节(offset)
        //printf("cmap [%d]: cmap_platformID = %d; cmap_platformSpecificID = %d; cmap_offset = 0x%08X.\n", subtab_id, cmap_platformID[subtab_id], cmap_platformSpecificID[subtab_id], cmap_offset[subtab_id]);
        uint8_t *format_addr = stream->data + list->cmap.table_ofs + cmap_offset[subtab_id];
        cmap_format_id[subtab_id] = (format_addr[0] << 8) | format_addr[1];
        format_addr += 2;   // 偏移两个字节(format)
        cmap_format_len[subtab_id] = (format_addr[0] << 8) | format_addr[1];
        format_addr += 2;   // 偏移两个字节(length)
        cmap_format_language[subtab_id] = (format_addr[0] << 8) | format_addr[1];
        format_addr += 2;   // 偏移两个字节(language)
        //printf("format [%d]: format_id = %d; format_len = %d; format_language = 0x%04X.\n", subtab_id, cmap_format_id[subtab_id], cmap_format_len[subtab_id], cmap_format_language[subtab_id]);

        if (cmap_format_id[subtab_id] == 4) {   // 默认只支持 format4 的格式
            segcountX2 = (format_addr[0] << 8) | format_addr[1];
            goto cmap_cfg_seg;
        }
    }

    printf("Format is currently not supported, only supported format4!!!");
    return false;

cmap_cfg_seg:

    uint8_t *format4_seg_data = stream->data + list->cmap.table_ofs + cmap_offset[subtab_id] + 6 /* format + length + language */ + 8 \
                                /* segCountX2 + searchRange + entrySelector + rangeShift */;
    info->format4_info.cmap_format4_segcont = segcountX2 / 2;
    //printf("cmap_format4_segcont = %d.\n", info->format4_info.cmap_format4_segcont);

    info->format4_info.end_code = malloc(segcountX2);
    if (info->format4_info.end_code == NULL) {
        printf("format4_info end_code malloc fail!!!");
        return false;
    }

    info->format4_info.start_code = malloc(segcountX2);
    if (info->format4_info.start_code == NULL) {
        printf("format4_info start_code malloc fail!!!");
        return false;
    }

    info->format4_info.id_delta = malloc(segcountX2);
    if (info->format4_info.id_delta == NULL) {
        printf("format4_info id_delta malloc fail!!!");
        return false;
    }

    info->format4_info.id_range_offset = malloc(segcountX2);
    if (info->format4_info.id_range_offset == NULL) {
        printf("format4_info id_range_offset malloc fail!!!");
        return false;
    }

    for (int j = 0; j < (info->format4_info.cmap_format4_segcont); j++) {
        info->format4_info.end_code[j] = (format4_seg_data[0] << 8) | format4_seg_data[1];
        format4_seg_data += 2;
        //printf("format [%d]: endcode[%d] = 0x%04X.\n", subtab_id, j, info->format4_info.end_code[j]);
    }

    format4_seg_data += 2;   // 偏移两个字节(reservedPad)

    for (int j = 0; j < (info->format4_info.cmap_format4_segcont); j++) {
        info->format4_info.start_code[j] = (format4_seg_data[0] << 8) | format4_seg_data[1];
        format4_seg_data += 2;
        //printf("format [%d]: startCode[%d] = 0x%04X.\n", subtab_id, j, info->format4_info.start_code[j]);
    }

    for (int j = 0; j < (info->format4_info.cmap_format4_segcont); j++) {
        info->format4_info.id_delta[j] = (format4_seg_data[0] << 8) | format4_seg_data[1];
        format4_seg_data += 2;
        //printf("format [%d]: idDelta[%d] = 0x%04X.\n", subtab_id, j, info->format4_info.id_delta[j]);
    }

    for (int j = 0; j < (info->format4_info.cmap_format4_segcont); j++) {
        info->format4_info.id_range_offset[j] = (format4_seg_data[0] << 8) | format4_seg_data[1];
        format4_seg_data += 2;
        if (info->format4_info.id_range_offset[j] != 0) {
            printf("id_range_offset = 0x%04x, is Unsupported!!!", info->format4_info.id_range_offset[j]);
            return false;
        }
        //printf("format [%d]: idRangeOffset[%d] = 0x%04X.\n", subtab_id, j, info->format4_info.id_range_offset[j]);
    }

    return true;
}

static ttf_font_desc_t *jl_ttf_create(const char *path, const void *data, size_t data_size, uint16_t font_size)
{
    if ((path == NULL && data == NULL) || 0 >= font_size) {
        printf("tiny_ttf: invalid argument\n");
        return NULL;
    }
    ttf_font_desc_t *dsc = (ttf_font_desc_t *)malloc(sizeof(ttf_font_desc_t));
    if (dsc == NULL) {
        printf("tiny_ttf: out of memory\n");
        return NULL;
    }

    if (path != NULL) {
        if (path[0] == 'm') {   // 使用 flash 路径
            u32 addr;
            u32 data_len;
            FILE *profile_fp = fopen(path, "r");
            if (profile_fp == NULL) {
                printf("tiny_ttf: unable to open %s\n", path);
                goto err_after_dsc;
            }
            struct vfs_attr file_attr;
            fget_attrs(profile_fp, &file_attr);
            addr = file_attr.sclust;
            data_len = flen(profile_fp);
            fclose(profile_fp);

            dsc->stream.data = (const uint8_t *)addr;
            dsc->stream.size = data_len;
            dsc->stream.flag = 0;
        } else {    // 其他路径：如 SD 卡，直接读取到 ram 中
            FILE *profile_fp = fopen(path, "r");
            if (profile_fp == NULL) {
                printf("tiny_ttf: unable to open %s\n", path);
                goto err_after_dsc;
            }
            uint8_t *addr;
            uint32_t data_len;
            data_len = flen(profile_fp);
            if (data_len > 0) {
                addr = malloc(data_len);
                if (addr == NULL) {
                    printf("path: %s len = %d is too long, alloc fail\n", path, data_len);
                    return NULL;
                }
            } else {
                printf("path: %s len = %d is error!!!\n", path, data_len);
                return NULL;
            }

            fread(addr, 1, data_len, profile_fp);
            dsc->stream.data = addr;
            dsc->stream.size = data_len;
            dsc->stream.flag = 1;
            fclose(profile_fp);
        }
    } else {    // 使用静态 C 数组字库
        dsc->stream.data = (const uint8_t *)data;
        dsc->stream.size = data_size;
        dsc->stream.flag = 2;
    }

    if (0 == stbttf_get_tab(&(dsc->list), &(dsc->stream))) {
        printf("jlttf: get table failed\n");
        goto err_after_dsc;
    }

    if (0 == stbttf_info_init(&(dsc->info), &(dsc->list), &(dsc->stream))) {
        printf("jlttf: info init failed\n");
        goto err_after_dsc;
    }

    jl_ttf_set_size(dsc, font_size);
    return dsc;

err_after_dsc:
    free(dsc);
    return NULL;
}

static void jl_ttf_get_points_x(uint8_t flag, uint8_t *curr_data, int16_t *x, uint32_t *x_ofs)
{
    if (flag & 0x02) {  // x-Short Vector 位为 1
        uint8_t dx = curr_data[(*x_ofs)];
        if (flag & 0x10) {  // This x is same 位为 1
            (*x) += dx;
        } else {    // This x is same 位为 0
            (*x) -= dx;
        }
        (*x_ofs)++;
        //printf("x is bit8:");
    } else {    // // x-Short Vector 位为 0
        if (flag & 0x10) {   // This x is same 位为 1
            //(*x) = (*x);    // 和上一个 x 一样；
        } else {    // This x is same 位为 0
            (*x) = (*x) + (int16_t)(curr_data[(*x_ofs)] << 8 | curr_data[(*x_ofs) + 1]);
            (*x_ofs) += 2;
        }
        //printf("x is bit16:");
    }
    //printf("x = %d.", (*x));
}

static void jl_ttf_get_points_y(uint8_t flag, uint8_t *curr_data, int16_t *y, uint32_t *y_ofs)
{
    if (flag & 0x04) {  // y-Short Vector 位为 1
        uint8_t dy = curr_data[(*y_ofs)];
        if (flag & 0x20) {  // This y is same 位为 1
            (*y) += dy;
        } else {    // This y is same 位为 0
            (*y) -= dy;
        }
        (*y_ofs)++;
        //printf("y is bit8:");
    } else {    // // y-Short Vector 位为 0
        if (flag & 0x20) {   // This y is same 位为 1
            //(*y) = (*y);    // 和上一个 y 一样；
        } else {    // This y is same 位为 0
            (*y) = (*y) + (curr_data[(*y_ofs)] << 8 | curr_data[(*y_ofs) + 1]);
            (*y_ofs) += 2;
        }
        //printf("y is bit16:");
    }
    //printf("y = %d.", (*y));
}

static void jl_ttf_get_hor_metrics(ttf_font_desc_t *font_des, jlvg_path_info_t *path_info, uint16_t glyph_index)
{
    uint8_t *curr_data;
    if (glyph_index < font_des->info.num_of_long_hor_metrics) {
        curr_data = font_des->info.hor_metrics1 + glyph_index * 4;
        path_info->advance_width = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2;
        path_info->left_side_bearing = (curr_data[0] << 8) | curr_data[1];
    } else {
        curr_data = font_des->info.hor_metrics2 + ((glyph_index - font_des->info.num_of_long_hor_metrics) * 2);
        path_info->advance_width = font_des->info.last_advance_width;
        path_info->left_side_bearing = (curr_data[0] << 8) | curr_data[1];
    }
    //printf("[%d]: advance_width = %d; left_side_bearing = %d.", glyph_index, path_info->advance_width, path_info->left_side_bearing);
}

static bool jl_ttf_get_glayh_path(ttf_font_desc_t *font_des, jlvg_path_info_t *path_info, uint32_t letter)
{
    if (font_des != NULL) {
        // 解析 cmap 表，根据字符编码，计算得到 loca 表的字形索引
        int32_t seg_id = -1;
        for (int j = 0; j < font_des->info.format4_info.cmap_format4_segcont; j++) {
            //printf("seg [%d]: endcode = 0x%04X; startCode = 0x%04X.\n", j, font_des->info.format4_info.end_code[j], font_des->info.format4_info.start_code[j]);
            if ((letter <= font_des->info.format4_info.end_code[j]) && (letter >= font_des->info.format4_info.start_code[j])) {
                seg_id = j;
                //printf("seg_id = %d.", seg_id);
                break;
            }
        }

        uint16_t glyph_index; // 字形索引: 计算索引的时候存在超过 16bit 长度的高位截断
        uint8_t *curr_data;
        uint32_t glyph_ofs;
        uint32_t glyph_ofs_next;
        uint32_t glyph_len;
        glyph_index = font_des->info.format4_info.id_delta[seg_id] + letter;
        if (glyph_index >= font_des->info.glyphs_num) {
            printf("glyphIndex = %d is error!!! Max id = %d.", glyph_index, font_des->info.glyphs_num);
            return false;
        }
        //printf("glyphIndex = 0x%04X.", glyph_index);
        // 解析 hmtx 表
        jl_ttf_get_hor_metrics(font_des, path_info, glyph_index);

        // 解析 loca 表，根据字形索引，计算得到 glyf 表的路径信息偏移地址和长度
        curr_data = font_des->stream.data + font_des->list.loca.table_ofs;

        if (font_des->info.index2loca_format == 0) {    // loca 偏移长度为 16bit
            glyph_ofs = (curr_data[(glyph_index << 1)] << 8 | curr_data[((glyph_index << 1) + 1)]) << 1;
            glyph_ofs_next = (curr_data[((glyph_index + 1) << 1)] << 8 | curr_data[(((glyph_index + 1) << 1) + 1)]) << 1;
        } else {    // index2loca_format == 1; loca 偏移长度为 32bit
            glyph_ofs = (curr_data[(glyph_index << 2)] << 24 | curr_data[((glyph_index << 2) + 1)] << 16 | \
                         curr_data[((glyph_index << 2) + 2)] << 8 | curr_data[((glyph_index << 2) + 3)]);
            glyph_ofs_next = (curr_data[((glyph_index + 1) << 2)] << 24 | curr_data[(((glyph_index + 1) << 2) + 1)] << 16 | \
                              curr_data[((glyph_index + 1) << 2) + 2] << 8 | curr_data[((glyph_index + 1) << 2) + 3]);
        }
        glyph_len = glyph_ofs_next - glyph_ofs;

        //printf("glyph_ofs = %d.", glyph_ofs);
        //printf("glyph_len = %d.\n", glyph_len);

        // 解析 glyf 表，根据路径信息配置路径
        curr_data = font_des->stream.data + font_des->list.glyf.table_ofs;
        curr_data += glyph_ofs;

        int16_t number_of_contours = curr_data[0] << 8 | curr_data[1];  // 字形轮廓数量
        curr_data += 2; // 偏移两个字节(numberOfContours)
        //printf("numberOfContours = %d.\n", number_of_contours);
        if (number_of_contours < 0) {   //  暂时不支持复合字形
            printf("ERROR!!! number_of_contours < 0 is Unsupported.");
            return false;
        }

        // 当前字形的最大最小边界
        path_info->x_min = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2;
        path_info->y_min = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2;
        path_info->x_max = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2;
        path_info->y_max = (curr_data[0] << 8) | curr_data[1];
        curr_data += 2;
        //printf("(x_min, y_min) = (%d, %d); (x_max, y_max) = (%d, %d).", path_info->x_min, path_info->y_min, path_info->x_max, path_info->y_max);

        // 解析简单字形
        uint16_t end_pts_of_contours[number_of_contours];   // 每个轮廓的结束点索引
        for (int j = 0; j < number_of_contours; j++) {
            end_pts_of_contours[j] = curr_data[0] << 8 | curr_data[1];
            curr_data += 2; // 偏移两个字节(endPtsOfContours)
            //printf("endPtsOfContours = %d.", end_pts_of_contours[j] + 1);
        }

        uint32_t outline_len = (end_pts_of_contours[number_of_contours - 1] + 1);   //  轮廓点数量
        //printf("outline_len = %d.", outline_len);

        uint16_t instruction_length = curr_data[0] << 8 | curr_data[1]; // 指令长度
        curr_data += 2;

        //printf("instruction_length = %d.", instruction_length);
#if 0   // nothing : 只进行偏移
        uint8_t instruction[instruction_length];
        for (int j = 0; j < instruction_length; j++) {
            instruction[j] = curr_data[j];
            //printf("instruction[%d] = 0x%02x.", j, instruction[j]);
        }
#endif
        curr_data += instruction_length;    // 偏移 instruction_length 个字节(instructions)

        // 开始配置 JLVG 的路径：path_info
        uint8_t flags[outline_len];
        uint8_t reflag_count = 0;   // flags 重复计数
        uint8_t flag_val;
        uint32_t flags_ofs = 0; // 字库连续地址中 flag 的偏移量
        uint32_t x_ofs = 0; // 字库连续地址中 x 坐标的偏移量
        uint32_t y_ofs = 0; // 字库连续地址中 y 坐标的偏移量
        path_info->points_ofs = 0;
        path_info->segments_ofs = 0;

        uint32_t segments_max = outline_len << 1;
        path_info->points = malloc((sizeof(float) * (segments_max << 1)));
        path_info->segments = malloc((sizeof(uint8_t) * segments_max));
        uint16_t next_move = 0; // 下一个轮廓的起点的索引
        for (int j = 0; j < outline_len; j++) {
            if (reflag_count == 0) {
                flag_val = curr_data[flags_ofs++];
                if (flag_val & 0x08) {  // 确认 flag 的 Repeat 位
                    reflag_count = curr_data[flags_ofs++];
                }
            } else {
                reflag_count--;
            }

            flags[j] = flag_val;
            //printf("flags[%d] = 0x%02x.", j, flags[j]);
        }
        curr_data += flags_ofs;       // 偏移 flags_ofs 个字节(flags); 但最终会得到 outline_len 个 flag

        uint16_t contours_cnt = 0;
        bool origin_off;// true: 起点为曲线外的点； false：起点为曲线上的点
        bool last_off;  // true: 上一个点为曲线外的点；false：上一个点为曲线上的点
        bool curr_off;  // true: 当前点为曲线外的点；false：当前点为曲线上的点
        int16_t x = 0;
        int16_t y = 0;      // 当前操作的轮廓点
        int16_t nx = 0;
        int16_t ny = 0;    // 下一个待操作的轮廓点
        int16_t cx = 0;
        int16_t cy = 0;    // 当前操作的曲线上轮廓点对应的控制点
        int16_t sx = 0;
        int16_t sy = 0;     // 第一个曲线上的点，但不一点是轮廓的第一个点
        int16_t scx = 0;
        int16_t scy = 0;    // 一定是轮廓上的第一个点，并且这个点不在曲线上

        // 获取 x 坐标的点，得到 x_ofs 后再计算得到 y 坐标的点
        for (int j = 0; j < outline_len; j++) {
            curr_off = (!(flags[j] & 0x01)) ? true : false;
            if (j == 0) {    // 第一个轮廓点
                jl_ttf_get_points_x(flags[j], curr_data, &x, &x_ofs);       // 记录第一个点
                nx = x; // 第一个轮廓点和第二个轮廓点起始都为 0， 因此计算是需要提前追加
                jl_ttf_get_points_x(flags[j + 1], curr_data, &nx, &x_ofs);  // 记录下一个点
            } else if (j == (outline_len - 1)) {    // 最后一个轮廓点
                x = nx; // 记录最后一个点
            } else {
                x = nx; // 记录上一次取出来的下一个点为当前点
                jl_ttf_get_points_x(flags[j + 1], curr_data, &nx, &x_ofs);  // 记录下一个点
            }

            if (next_move == j) {   // 新的起点
                if (j != 0) {   // j != 0 的起始点处都需要对上一次的路径进行闭合
                    if (origin_off) {   // 轮廓起点不在曲线上, 起点是 scx
                        if (last_off) { // 轮廓最后一个点也不在曲线上, 以轮廓起始点和轮廓终点的中点作为一个曲线上的点绘制二次贝塞尔曲线
                            // 设置路径 outline_conic_to 的 x 坐标
                            path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                            path_info->segments_ofs++;
                            path_info->points[path_info->points_ofs] = cx;
                            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                            path_info->points[path_info->points_ofs] = (scx + cx) >> 1;
                            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                        }

                        // 以轮廓起始点作为控制点构建二次贝塞尔曲线
                        // 设置路径 outline_conic_to 的 x 坐标
                        path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                        path_info->segments_ofs++;
                        path_info->points[path_info->points_ofs] = scx;
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                        path_info->points[path_info->points_ofs] = sx;
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                    } else {    // 轮廓起点在曲线上，起点是 sx
                        if (last_off) { // 轮廓的最后一个点不在曲线上，是一个控制点，则以起点为终点画二次贝塞尔曲线
                            // 设置路径 outline_conic_to 的 x 坐标
                            path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                            path_info->segments_ofs++;
                            path_info->points[path_info->points_ofs] = cx;
                            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                            path_info->points[path_info->points_ofs] = sx;
                            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                        } else {    // 轮廓的最后一个点在曲线上，则直接画直线
                            // 设置路径 outline_line_to 的 x 坐标
                            path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_LINE_TO;
                            path_info->segments_ofs++;
                            path_info->points[path_info->points_ofs] = sx;
                            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                        }
                    }
                }

                // 确定有效的起始点
                origin_off = curr_off;
                if (origin_off) {   //  起始点不在曲线上
                    // 起始点不在曲线上，记录起始点和起始点控制点
                    scx = x;
                    if (!(flags[j + 1] & 0x01)) {    // 如果起始点的下一个点也不在曲线上，则取两个点的中点作为起始点，并且这个中点作为曲线上的点！！！！
                        sx = (x + nx) >> 1;
                        // 没有消耗掉下一个点，不需要 j++
                    } else {    // 如果起始点的下一个点在曲线上，则取这个点作为起始点
                        sx = nx;

                        //提前取出下一次的点
                        j++;    // 消耗掉了下一个点，偏移加1
                        x = nx;
                        jl_ttf_get_points_x(flags[j + 1], curr_data, &nx, &x_ofs);  // 记录下一个点
                    }
                } else {    // 起始点在曲线上
                    sx = x;
                }

                // 设置路径 outline_move_to 的 x 坐标
                path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_MOVE_TO;
                path_info->segments_ofs++;
                path_info->points[path_info->points_ofs] = sx;
                path_info->points_ofs += 2;   // 需要同时偏移 y 坐标

                last_off = false;   // 标志当前在曲线上
                next_move = 1 + end_pts_of_contours[contours_cnt];  // 下一个轮廓的起点的索引
                contours_cnt++;
            } else {
                if (curr_off) { // if it's a curve
                    if (last_off) { // 起点紧接着的下一个点如果是非曲线上的点, 不会走这里，因为起点本身必然是曲线上的点
                        // 当前点和上一次的点都是非曲线上的点
                        // 设置路径 outline_conic_to 的 x 坐标
                        path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                        path_info->segments_ofs++;
                        path_info->points[path_info->points_ofs] = cx;
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                        path_info->points[path_info->points_ofs] = (x + cx) >> 1;   // 两个连续的非曲线点的中点作为一个曲线上的点来构建二次贝塞尔曲线
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                    }

                    cx = x; // 当前的这个非曲线上的点可以作为下一个点的控制点
                    last_off = true;    // 标志当前不在曲线上
                } else {
                    if (last_off) {
                        // 设置路径 outline_conic_to 的 x 坐标
                        path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                        path_info->segments_ofs++;
                        path_info->points[path_info->points_ofs] = cx;  // 上一个不是曲线上的点，则作为控制点构建二次贝塞尔曲线
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                        path_info->points[path_info->points_ofs] = x;
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                    } else {
                        // 设置路径 outline_line_to 的 x 坐标
                        path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_LINE_TO;
                        path_info->segments_ofs++;
                        path_info->points[path_info->points_ofs] = x;
                        path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                    }
                    last_off = false;   // 标志当前在曲线上
                }
            }
        }
        // 闭合最后一个终点
        if (origin_off) {   // 轮廓起点不在曲线上, 起点是 scx
            if (last_off) { // 轮廓最后一个点也不在曲线上, 以轮廓起始点和轮廓终点的中点作为一个曲线上的点绘制二次贝塞尔曲线
                // 设置路径 outline_conic_to 的 x 坐标
                path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                path_info->segments_ofs++;
                path_info->points[path_info->points_ofs] = cx;
                path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                path_info->points[path_info->points_ofs] = (scx + cx) >> 1;
                path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
            }

            // 以轮廓起始点作为控制点构建二次贝塞尔曲线
            // 设置路径 outline_conic_to 的 x 坐标
            path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
            path_info->segments_ofs++;
            path_info->points[path_info->points_ofs] = scx;
            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
            path_info->points[path_info->points_ofs] = sx;
            path_info->points_ofs += 2;   // 需要同时偏移 y 坐标，这里是最后一个坐标
        } else {    // 轮廓起点在曲线上，起点是 sx
            if (last_off) { // 轮廓的最后一个点不在曲线上，是一个控制点，则以起点为终点画二次贝塞尔曲线
                // 设置路径 outline_conic_to 的 x 坐标
                path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_QUAD_TO;
                path_info->segments_ofs++;
                path_info->points[path_info->points_ofs] = cx;
                path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
                path_info->points[path_info->points_ofs] = sx;
                path_info->points_ofs += 2;   // 需要同时偏移 y 坐标
            } else {    // 轮廓的最后一个点在曲线上，则直接画直线
                // 设置路径 outline_line_to 的 x 坐标
                path_info->segments[path_info->segments_ofs] = (uint8_t)VGHW_LINE_TO;
                path_info->segments_ofs++;
                path_info->points[path_info->points_ofs] = sx;
                path_info->points_ofs += 2;   // 需要同时偏移 y 坐标，这里是最后一个坐标
            }
        }
        //printf("x_ofs = %d; segments_ofs = %d; points_ofs = %d.", x_ofs, path_info->segments_ofs, path_info->points_ofs);

        // 重新调整计数初始值，不再重新配置 segments，只配置 path_info 的 y 坐标
        path_info->points_ofs = 1;  // 这样做是为了给 y 坐标的赋值事先进行一次偏移
        curr_data += x_ofs;       // 偏移 x_ofs 个字节(flags); 但最终会得到 outline_len 个 x 坐标
        origin_off = 0;
        last_off = 0;
        curr_off = 0;
        next_move = 0;
        contours_cnt = 0;

        for (int j = 0; j < outline_len; j++) {
            curr_off = (!(flags[j] & 0x01)) ? true : false;

            if (j == 0) {    // 第一个轮廓点
                jl_ttf_get_points_y(flags[j], curr_data, &y, &y_ofs);       // 记录第一个点
                ny = y; // 第一个轮廓点和第二个轮廓点起始都为 0， 因此计算是需要提前追加
                jl_ttf_get_points_y(flags[j + 1], curr_data, &ny, &y_ofs);  // 记录下一个点
            } else if (j == (outline_len - 1)) {    // 最后一个轮廓点
                y = ny; // 记录最后一个点
            } else {
                y = ny; // 记录上一次取出来的下一个点为当前点
                jl_ttf_get_points_y(flags[j + 1], curr_data, &ny, &y_ofs);  // 记录下一个点
            }

            if (next_move == j) {   // 新的起点
                if (j != 0) {   // j != 0 的起始点处都需要对上一次的路径进行闭合
                    if (origin_off) {   // 轮廓起点不在曲线上, 起点是 scy
                        if (last_off) { // 轮廓最后一个点也不在曲线上, 以轮廓起始点和轮廓终点的中点作为一个曲线上的点绘制二次贝塞尔曲线
                            // 设置路径 outline_conic_to 的 y 坐标
                            path_info->points[path_info->points_ofs] = cy;
                            path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                            path_info->points[path_info->points_ofs] = (scy + cy) >> 1;
                            path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                        }

                        // 以轮廓起始点作为控制点构建二次贝塞尔曲线
                        // 设置路径 outline_conic_to 的 y 坐标
                        path_info->points[path_info->points_ofs] = scy;
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                        path_info->points[path_info->points_ofs] = sy;
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                    } else {    // 轮廓起点在曲线上，起点是 sy
                        if (last_off) { // 轮廓的最后一个点不在曲线上，是一个控制点，则以起点为终点画二次贝塞尔曲线
                            // 设置路径 outline_conic_to 的 y 坐标
                            path_info->points[path_info->points_ofs] = cy;
                            path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                            path_info->points[path_info->points_ofs] = sy;
                            path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                        } else {    // 轮廓的最后一个点在曲线上，则直接画直线
                            // 设置路径 outline_line_to 的 y 坐标
                            path_info->points[path_info->points_ofs] = sy;
                            path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                        }
                    }
                }

                // 确定有效的起始点
                origin_off = curr_off;
                if (origin_off) {   //  起始点不在曲线上
                    // 起始点不在曲线上，记录起始点和起始点控制点
                    scy = y;
                    if (!(flags[j + 1] & 0x01)) {    // 如果起始点的下一个点也不在曲线上，则取两个点的中点作为起始点，并且这个中点作为曲线上的点！！！！
                        sy = (y + ny) >> 1;
                        // 没有消耗掉下一个点，不需要 j++
                    } else {    // 如果起始点的下一个点在曲线上，则取这个点作为起始点
                        sy = ny;

                        //提前取出下一次的点
                        j++;    // 消耗掉了下一个点，偏移加1
                        y = ny;
                        jl_ttf_get_points_y(flags[j + 1], curr_data, &ny, &y_ofs);  // 记录下一个点
                    }
                } else {    // 起始点在曲线上
                    sy = y;
                }

                // 设置路径 outline_move_to 的 y 坐标
                path_info->points[path_info->points_ofs] = sy;
                path_info->points_ofs += 2;   // 需要同时偏移 x 坐标

                last_off = false;   // 标志当前在曲线上
                next_move = 1 + end_pts_of_contours[contours_cnt];  // 下一个轮廓的起点的索引
                contours_cnt++;
            } else {
                if (curr_off) { // if it's a curve
                    if (last_off) { // 起点紧接着的下一个点如果是非曲线上的点, 不会走这里，因为起点本身必然是曲线上的点
                        // 当前点和上一次的点都是非曲线上的点
                        // 设置路径 outline_conic_to 的 y 坐标
                        path_info->points[path_info->points_ofs] = cy;
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                        path_info->points[path_info->points_ofs] = (y + cy) >> 1;   // 两个连续的非曲线点的中点作为一个曲线上的点来构建二次贝塞尔曲线
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                    }

                    cy = y; // 当前的这个非曲线上的点可以作为下一个点的控制点
                    last_off = true;    // 标志当前不在曲线上
                } else {
                    if (last_off) {
                        // 设置路径 outline_conic_to 的 y 坐标
                        path_info->points[path_info->points_ofs] = cy;  // 上一个不是曲线上的点，则作为控制点构建二次贝塞尔曲线
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                        path_info->points[path_info->points_ofs] = y;
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                    } else {
                        // 设置路径 outline_line_to 的 y 坐标
                        path_info->points[path_info->points_ofs] = y;
                        path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                    }
                    last_off = false;   // 标志当前在曲线上
                }
            }
        }
        // 闭合最后一个终点
        if (origin_off) {   // 轮廓起点不在曲线上, 起点是 scy
            if (last_off) { // 轮廓最后一个点也不在曲线上, 以轮廓起始点和轮廓终点的中点作为一个曲线上的点绘制二次贝塞尔曲线
                // 设置路径 outline_conic_to 的 y 坐标
                path_info->points[path_info->points_ofs] = cy;
                path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                path_info->points[path_info->points_ofs] = (scy + cy) >> 1;
                path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
            }

            // 以轮廓起始点作为控制点构建二次贝塞尔曲线
            // 设置路径 outline_conic_to 的 y 坐标
            path_info->points[path_info->points_ofs] = scy;
            path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
            path_info->points[path_info->points_ofs] = sy;
            path_info->points_ofs++;   // 只偏移 y 坐标，这里是最后一个坐标
        } else {    // 轮廓起点在曲线上，起点是 sy
            if (last_off) { // 轮廓的最后一个点不在曲线上，是一个控制点，则以起点为终点画二次贝塞尔曲线
                // 设置路径 outline_conic_to 的 y 坐标
                path_info->points[path_info->points_ofs] = cy;
                path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
                path_info->points[path_info->points_ofs] = sy;
                path_info->points_ofs += 2;   // 需要同时偏移 x 坐标
            } else {    // 轮廓的最后一个点在曲线上，则直接画直线
                // 设置路径 outline_line_to 的 y 坐标
                path_info->points[path_info->points_ofs] = sy;
                path_info->points_ofs++;   // 只偏移 y 坐标，这里是最后一个坐标
            }
        }

        //printf("y_ofs = %d; segments_ofs = %d; points_ofs = %d.", y_ofs, path_info->segments_ofs, path_info->points_ofs);
        curr_data += y_ofs;       // 偏移 y_ofs 个字节(flags); 但最终会得到 outline_len 个 y 坐标

#if 0 // JLVG PATH dump
        int seg_i = 0;
        int poi_i = 0;
        for (seg_i = 0; seg_i < path_info->segments_ofs; seg_i++) {
            if (path_info->segments[seg_i] == VGHW_MOVE_TO) {
                printf("move to : (x, y) = (%f, %f).", path_info->points[poi_i], path_info->points[poi_i + 1]);
                poi_i += 2;
            } else if (path_info->segments[seg_i] == VGHW_LINE_TO) {
                printf("line to : (x, y) = (%f, %f).", path_info->points[poi_i], path_info->points[poi_i + 1]);
                poi_i += 2;
            } else if (path_info->segments[seg_i] == VGHW_QUAD_TO) {
                printf("quad to : (cx, cy) = (%f, %f); (x, y) = (%f, %f).", path_info->points[poi_i], path_info->points[poi_i + 1], \
                       path_info->points[poi_i + 2], path_info->points[poi_i + 3]);
                poi_i += 4;
            } else {
                printf("segments = 0x%02x is Unsupported.", path_info->segments[seg_i]);
                //return false;
            }
        }
#endif

        return true;
    } else {
        printf("ERROR!!! font_des is NULL.");
        return false;
    }
}

static void jl_ttf_destroy(ttf_font_desc_t *font_des)
{
    if (font_des != NULL) {
        if (font_des->stream.flag == 1) {
            free(font_des->stream.data);
            free(font_des->info.format4_info.end_code);
            free(font_des->info.format4_info.start_code);
            free(font_des->info.format4_info.id_delta);
            free(font_des->info.format4_info.id_range_offset);
        }

        free(font_des);
    }
}

static void JLVG_JLTTF_lcd_show_char(ttf_font_desc_t *font_des, u8 char_size, int16_t sx, int16_t sy, wchar_t *show_char)
{
    int16_t pen_x = sx;
    int16_t pen_y = sy;
    jlvg_path_info_t path_info;
    uint16_t color;
    static uint8_t run_time = 0;
    float scale_x;
    float scale_y;
    scale_x = (float)char_size / (float)font_des->info.units_per_em;
    scale_y = scale_x;
    //printf("scale_x = %f; scale_y = %f.", scale_x, scale_y);

    for (u8 i = 0; i < wcslen(show_char); i++) {
        //printf("pen_x = %d; pen_y = %d.", pen_x, pen_y);
        hw_cfgtime_begin = get_system_us();
        color = char_color[run_time];

        //font_glyph_dsc_t g;
        uint32_t letter = show_char[i];
        jl_ttf_get_glayh_path(font_des, &path_info, letter);
        uint32_t get_path_end = get_system_us();
        cfg_path_time += (get_path_end - hw_cfgtime_begin);

        // JLVG 配置
        jlvg_surface_t surface;
        jlvg_color_t fill_color = { 0xff, 0xff, 0xff, 0xff };

        u16 color_tmp;
        color_tmp = (color << 3);
        fill_color.r = color_tmp;
        color_tmp = (color >> 5) << 2;
        fill_color.g = color_tmp;
        color_tmp = (color >> 11) << 3;
        fill_color.b = color_tmp;

        jlvg_rect_t draw_area;

        jlvg_path_t rect_path;
        jlvg_paint_color_t paint;

        jlvg_matrix_t user2surface;

        u8 *showbuf = lcd_get_draw_buf();

#if (VG_DRAW_FULL_DISP_ENBLE == 1) // vg 开窗大小和屏幕大小一致，耗时大
        int width = LCD_W;
        int height = LCD_H;
        int Bpp = 2;    //RGB565
        int fb_size = width * height * Bpp;
        uint32_t dest_stride;
        u8 *dest_buf;

        draw_area.x = 0;
        draw_area.y = 0;
        draw_area.w = width;
        draw_area.h = height;
        dest_stride = width * Bpp;
        dest_buf = showbuf;
#else
        int width = char_size;
        int height = char_size;
        int Bpp = 2;    //RGB565
        int fb_size = width * height * Bpp;
        uint32_t dest_stride;
        u8 *dest_buf;

        draw_area.x = 0;
        draw_area.y = 0;
        draw_area.w = width;
        draw_area.h = height;
        dest_stride = LCD_W * Bpp;
        dest_buf = showbuf + (pen_x * Bpp) + (pen_y * dest_stride);
#endif

        jlvg_surface_init(&surface, dest_buf, fb_size, width, height, dest_stride, VGHW_FORMAT_RGB565, 0, 1, 1, 0);

        jlvg_start_frame(jlvg, &surface);

        jlvg_matrix_set_identity(&user2surface);

        jlvg_paint_color_init(&paint, fill_color);

        jlvg_path_init(&rect_path, JLVG_PATH_DATATYPE_F32, path_info.segments_ofs, path_info.segments, path_info.points_ofs, path_info.points);
        jlvg_path_update_fill(&rect_path);

#if (VG_DRAW_FULL_DISP_ENBLE == 1) // vg 开窗大小和屏幕大小一致，耗时大
        // 配置矩阵
        jlvg_matrix_translate(&user2surface, -(path_info.x_min), -(path_info.y_min));

        jlvg_matrix_scale(&user2surface, scale_x, -scale_y);

        jlvg_matrix_translate(&user2surface, (path_info.x_min + path_info.left_side_bearing) * scale_x + pen_x, (font_des->info.ascent + font_des->info.descent - path_info.y_min) * scale_y + pen_y);
#else
        // 配置矩阵
        jlvg_matrix_translate(&user2surface, -(path_info.x_min), -(path_info.y_min));

        jlvg_matrix_scale(&user2surface, scale_x, -scale_y);

        jlvg_matrix_translate(&user2surface, (path_info.x_min + path_info.left_side_bearing) * scale_x, (font_des->info.ascent + font_des->info.descent - path_info.y_min) * scale_y);
#endif

        jlvg_fill_path(jlvg, &draw_area, &rect_path, (jlvg_paint_t *)&paint, &user2surface, NULL, VGHW_BLEND_SRC_OVER, VGHW_NON_ZERO);

        hw_cfgtime = hw_cfgtime + (get_system_us() - hw_cfgtime_begin);
        // JLVG 启动
        jlvg_end_frame(jlvg);

        jlvg_path_deinit(&rect_path);
        jlvg_paint_deinit((jlvg_paint_t *)&paint);
        jlvg_surface_deinit(&surface);

        free(path_info.points);
        free(path_info.segments);

        pen_x += (scale_x * path_info.advance_width);
        //pen_x += char_size;
        //pen_y += char_size;
        run_time++;
        if (run_time == 15) {
            run_time = 0 ;
        }
    }
}

static void jlttf64_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相";
    wchar_t *wstr2  = L"广袤春夏秋冬昼";
    wchar_t *wstr3  = L"梅兰竹菊松柏常";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄";
    wchar_t *wstr5  = L"习进步勤奋努力";
    wchar_t *wstr6  = L"人湖光山色美不";
    wchar_t *wstr7  = L"起潮落人生如梦";

    const u8 char_size = 64;

    ttf_font_desc_t *font_des = jl_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0, L"Ab)-|我");
#if 1
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
#endif

        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    jl_ttf_destroy(font_des);
}

static void jlttf48_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜";
    wchar_t *wstr5  = L"习进步勤奋努力自强";
    wchar_t *wstr6  = L"人湖光山色美不胜收";
    wchar_t *wstr7  = L"起潮落人生如梦岁月";
    wchar_t *wstr8  = L"印永结同古往今来历";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰";

    const u8 char_size = 48;

    ttf_font_desc_t *font_des = jl_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0, L"我一");
#if 1
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
#endif

        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    jl_ttf_destroy(font_des);
}

static void jlttf32_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日月星辰风雨";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交替岁月如梭";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂馥兰金银铜";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜吉祥如意心";
    wchar_t *wstr5  = L"习进步勤奋努力自强不息厚德载";
    wchar_t *wstr6  = L"人湖光山色美不胜收人间仙境世";
    wchar_t *wstr7  = L"起潮落人生如梦岁月匆高山流水";
    wchar_t *wstr8  = L"印永结同古往今来历史长河英雄";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰宝艺术瑰宝";
    wchar_t *wstr10 = L"生机勃勃绿意盎然花团锦簇五彩";
    wchar_t *wstr11 = L"河山万里长城巍峨壮家国情怀赤";
    wchar_t *wstr12 = L"舍生取尊师重道礼仪之邦和为贵";
    wchar_t *wstr13 = L"智信五常之道诚信为本以德服人";
    wchar_t *wstr14 = L"程万里前程似锦未来可期梦想成";

    const u8 char_size = 32;

    ttf_font_desc_t *font_des = jl_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0, L"我一");
#if 1
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
#endif

        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    jl_ttf_destroy(font_des);
}

static void jlttf24_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日月星辰风雨雷电山川草";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交替岁月如梭时光荏书声";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂馥兰金银铜铁玉石珠宝";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜吉祥如意心想事成万事";
    wchar_t *wstr5  = L"习进步勤奋努力自强不息厚德载物博学多才";
    wchar_t *wstr6  = L"人湖光山色美不胜收人间仙境世外桃清风明";
    wchar_t *wstr7  = L"起潮落人生如梦岁月匆高山流水知音难觅情";
    wchar_t *wstr8  = L"印永结同古往今来历史长河英雄辈出才子佳";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰宝艺术瑰宝文明之光照";
    wchar_t *wstr10 = L"生机勃勃绿意盎然花团锦簇五彩缤江山如画";
    wchar_t *wstr11 = L"河山万里长城巍峨壮家国情怀赤子之心精忠";
    wchar_t *wstr12 = L"舍生取尊师重道礼仪之邦和为贵和谐共处天";
    wchar_t *wstr13 = L"智信五常之道诚信为本以德服人天下为公大";
    wchar_t *wstr14 = L"程万里前程似锦未来可期梦想成学海无涯书";
    wchar_t *wstr15 = L"而不舍金石可镂水虫苒香门民安略源匆心海";
    wchar_t *wstr16 = L"龙腾虎跃马到成功步步高升日新月异蒸蒸日";
    wchar_t *wstr17 = L"好河山锦绣江山壮丽辉煌青山绿水碧波荡漾";
    wchar_t *wstr18 = L"仙境历史悠久源远流长博大精深璀璨文化文";
    wchar_t *wstr19 = L"你我得会里后自能着用们在下出就分说地主";

    const u8 char_size = 24;

    ttf_font_desc_t *font_des = jl_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0, L"我一");
#if 1
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 14 * char_size, wstr15);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 15 * char_size, wstr16);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 16 * char_size, wstr17);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 17 * char_size, wstr18);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 18 * char_size, wstr19);
#endif

        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    jl_ttf_destroy(font_des);
}

static void jlttf16_test(const char *path)
{
    wchar_t *wstr1  = L"天地人和心手相应日月星辰风雨雷电山川草木花鸟鱼江河湖海森林";
    wchar_t *wstr2  = L"广袤春夏秋冬昼夜交替岁月如梭时光荏书声琅琅琴棋书画诗酒花茶";
    wchar_t *wstr3  = L"梅兰竹菊松柏常青桂馥兰金银铜铁玉石珠宝锦绣文章龙飞凤舞麒麟";
    wchar_t *wstr4  = L"献瑞瑞气盈福禄寿喜吉祥如意心想事成万事如意家和万事兴国泰学";
    wchar_t *wstr5  = L"习进步勤奋努力自强不息厚德载物博学多才文韬武山川壮丽景色宜";
    wchar_t *wstr6  = L"人湖光山色美不胜收人间仙境世外桃清风明月云卷云舒花开花落潮";
    wchar_t *wstr7  = L"起潮落人生如梦岁月匆高山流水知音难觅情深意长海誓山盟心心相";
    wchar_t *wstr8  = L"印永结同古往今来历史长河英雄辈出才子佳人红颜薄命情深似诗词";
    wchar_t *wstr9  = L"歌赋传承千年文化瑰宝艺术瑰宝文明之光照耀四春夏秋冬季节更迭";
    wchar_t *wstr10 = L"生机勃勃绿意盎然花团锦簇五彩缤江山如画美景如画锦绣江山大好";
    wchar_t *wstr11 = L"河山万里长城巍峨壮家国情怀赤子之心精忠报国忠肝义胆英勇无畏";
    wchar_t *wstr12 = L"舍生取尊师重道礼仪之邦和为贵和谐共处天下大同世界一家仁义礼";
    wchar_t *wstr13 = L"智信五常之道诚信为本以德服人天下为公大道壮志凌云雄心勃勃鹏";
    wchar_t *wstr14 = L"程万里前程似锦未来可期梦想成学海无涯书山有路勤为径苦作舟锲";
    wchar_t *wstr15 = L"而不舍金石可镂水虫苒香门民安略源匆心海方纷观义之行真滴石穿";
    wchar_t *wstr16 = L"龙腾虎跃马到成功步步高升日新月异蒸蒸日上万里长城巍峨壮观大";
    wchar_t *wstr17 = L"好河山锦绣江山壮丽辉煌青山绿水碧波荡漾湖光山色美不胜收人间";
    wchar_t *wstr18 = L"仙境历史悠久源远流长博大精深璀璨文化文明之光书声琅琅欢声笑";
    wchar_t *wstr19 = L"你我得会里后自能着用们在下出就分说地主么去好我们看听想知做";
    wchar_t *wstr20 = L"学用玩吃睡说爱笑走跑跳飞高远近大小多少前后左右东西南北上下";
    wchar_t *wstr21 = L"男女老少师友亲朋同学同事工友客敌友敌我他你我它金木水火土日";
    wchar_t *wstr22 = L"月星云风雨雷电雪霜露冰春秋夏秋冬花鸟鱼虫马牛羊狗猪鸡鸭鹅猫";
    wchar_t *wstr23 = L"兔鼠龙虎蛇猴狮象熊狼狐兔牛马羊鸡鱼鸟虫红黄蓝绿黑白灰紫橙棕";
    wchar_t *wstr24 = L"青粉绿青紫蓝灰棕黑橙黄红绿青蓝紫红橙黄绿青灰白黑紫粉绿声香";
    wchar_t *wstr25 = L"色味触感思听看想读写算说画唱跳笑哭打闹玩跑走游飞游泳游唱跳";
    wchar_t *wstr26 = L"跑跳游唱跳画龙点睛一鸣惊人狐假虎威杯弓蛇影对牛弹琴狗尾续貂";
    wchar_t *wstr27 = L"画蛇添足井底之蛙刻舟求剑滥竽充数掩耳盗铃亡羊补牢守株待兔画";
    wchar_t *wstr28 = L"饼充饥破釜沉舟破镜重圆盲人摸象杯水车薪水中捞月纸上谈兵指鹿";
    wchar_t *wstr29 = L"腰缠万贯为马闭月羞花沉鱼落雁惊弓之鸟草木皆兵瓮中捉鳖望梅止";

    const u8 char_size = 16;

    ttf_font_desc_t *font_des = jl_ttf_create(path, NULL, 0, char_size);

    do {
        printf("begin :");
        hw_cfgtime = 0;
        sw_blendtime = 0;
        cfg_path_time = 0;
        begin_time = get_system_us();

        //JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0, L"我一");
#if 1
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 0  * char_size, wstr1);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 1  * char_size, wstr2);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 2  * char_size, wstr3);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 3  * char_size, wstr4);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 4  * char_size, wstr5);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 5  * char_size, wstr6);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 6  * char_size, wstr7);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 7  * char_size, wstr8);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 8  * char_size, wstr9);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 9  * char_size, wstr10);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 10 * char_size, wstr11);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 11 * char_size, wstr12);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 12 * char_size, wstr13);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 13 * char_size, wstr14);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 14 * char_size, wstr15);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 15 * char_size, wstr16);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 16 * char_size, wstr17);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 17 * char_size, wstr18);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 18 * char_size, wstr19);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 19 * char_size, wstr20);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 20 * char_size, wstr21);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 21 * char_size, wstr22);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 22 * char_size, wstr23);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 23 * char_size, wstr24);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 24 * char_size, wstr25);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 25 * char_size, wstr26);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 26 * char_size, wstr27);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 27 * char_size, wstr28);
        JLVG_JLTTF_lcd_show_char(font_des, char_size, 0, 28 * char_size, wstr29);
#endif

        end_time = get_system_us();
        //printf("begin_time = %d; end_time = %d.", begin_time, end_time);
        printf("### all diff time = %d; sw blend time = %d; hw cfg time = %d; get path time = %d.", (end_time - begin_time), sw_blendtime, hw_cfgtime, cfg_path_time);
        lcd_draw_finish();
    } while (1);

    jl_ttf_destroy(font_des);
}

static void jlttf_demo_task(void *priv) //测试显示不同大小文字
{
    printf("LCD_W = %d; LCD_H = %d.", LCD_W, LCD_H);

    const char *filename = CONFIG_FONT_TTF_SUB2;
    jlvg = jlvg_create();

    jlttf16_test(filename);
    //jlttf24_test(filename);
    //jlttf32_test(filename);
    //jlttf48_test(filename);
    //jlttf64_test(filename);

    jlvg_free(jlvg);
    while (1) {
        os_time_dly(1);
    }
}

static int jlttf_demo_task_init(void)
{
    puts("jlttf_demo_task_init \n\n");
    thread_fork("jlttf_demo_task", 30, 1024 * 10, 64, 0, jlttf_demo_task, NULL);
    while (1) {
        os_time_dly(1111);
    }
    return 0;
}
module_initcall(jlttf_demo_task_init);
