/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jlvg.h"

#if LV_USE_DRAW_JLVG

//#define LOG_TAG_CONST       UI
#define LOG_TAG             "[lv_gpu]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"

/*********************
 *      DEFINES
 *********************/
#define JLVG_PI 3.14159265358979323846f
// 计算贝塞尔曲线控制点偏移量的系数
#define KAPPA 0.5522847498f

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

jlvg_hw_imageformat_t lv_jlvg_get_color_format(lv_color_format_t lv_cf, bool *cf_flag)
{
    jlvg_hw_imageformat_t jlvg_cf = VGHW_FORMAT_ARGB8888;

    if (cf_flag != NULL) {
        *cf_flag = true;   // false : 不支持：true : 支持
    }

    //printf("lv_color_format_t  = 0x%02x.", lv_cf);
    switch (lv_cf) {
    case LV_COLOR_FORMAT_ARGB8888:
        jlvg_cf = VGHW_FORMAT_ARGB8888;
        break;
    case LV_COLOR_FORMAT_ARGB8565:
        jlvg_cf = VGHW_FORMAT_ARGB8565;
        break;
    case LV_COLOR_FORMAT_RGB888:
        jlvg_cf = VGHW_FORMAT_RGB888;
        break;
    case LV_COLOR_FORMAT_RGB565:
        jlvg_cf = VGHW_FORMAT_RGB565;
        break;
    case LV_COLOR_FORMAT_A8:
        jlvg_cf = VGHW_FORMAT_A8;
        break;
    case LV_COLOR_FORMAT_A4:
        jlvg_cf = VGHW_FORMAT_A4;
        break;
    case LV_COLOR_FORMAT_A2:
        jlvg_cf = VGHW_FORMAT_A2;
        break;
    case LV_COLOR_FORMAT_A1:
        jlvg_cf = VGHW_FORMAT_A1;
        break;
    case LV_COLOR_FORMAT_L8:
        jlvg_cf = VGHW_FORMAT_L8;
        break;
    case LV_COLOR_FORMAT_I4:
        jlvg_cf = VGHW_FORMAT_L4;
        break;
    case LV_COLOR_FORMAT_I2:
        jlvg_cf = VGHW_FORMAT_L2;
        break;
    case LV_COLOR_FORMAT_I1:
        jlvg_cf = VGHW_FORMAT_L1;
        break;
    default:
        if (cf_flag != NULL) {
            *cf_flag = false;   // false : 不支持：true : 支持
        }
        //printf("Unsupported color format.");
        break;
    }

    //printf("jlvg_cf  = 0x%02x.", jlvg_cf);
    return jlvg_cf;
}

uint8_t lv_jlvg_get_blend_mode(lv_blend_mode_t lv_blend_mode, bool *mode_flag)
{
    uint8_t jlvg_blend_mode = VGHW_BLEND_SRC;

    if (mode_flag != NULL) {
        *mode_flag = true;   // false : 不支持：true : 支持
    }

    //printf("lv_blend_mode 0x%02x.", lv_blend_mode);
    switch (lv_blend_mode) {
    case LV_BLEND_MODE_NORMAL:
        jlvg_blend_mode = VGHW_BLEND_SRC_OVER;
        break;
    case LV_BLEND_MODE_ADDITIVE:
        jlvg_blend_mode = VGHW_BLEND_ADDITIVE;
        break;
    case LV_BLEND_MODE_MULTIPLY:
        jlvg_blend_mode = VGHW_BLEND_MULTIPLY;
        break;
    default:
        if (mode_flag != NULL) {
            *mode_flag = false;   // false : 不支持：true : 支持
        }
        //printf("Unsupported blend mode.");
        break;
    }

    //printf("jlvg_blend_mode  = 0x%02x.", jlvg_blend_mode);
    return jlvg_blend_mode;
}

void lv_jlvg_flush_inv_dcache(uint8_t *buf, uint32_t w, uint32_t h, uint32_t stride, uint8_t color_depth)
{
    /* 区域刷新Cache */
    uint8_t *area_ptr = (uint8_t *)buf;
    uint32_t width_bytes = w * color_depth >> 3;
    uint32_t offset = stride * color_depth >> 3;
    for (int y = 0; y < h; y++) {
        DcuFlushinvRegion((u32 *)area_ptr, width_bytes);
        area_ptr += offset;
    }
}

// Helper function to calculate the angle between two vectors
static float __vecang(float ux, float uy, float vx, float vy)
{
    float r = (float)((ux * vx + uy * vy) / ((float)sqrtf(ux * ux + uy * uy) * (float)sqrtf(vx * vx + vy * vy)));
    if (r < -1.0f) {
        r = -1.0f;
    }
    if (r > 1.0f) {
        r = 1.0f;
    }
    return ((ux * vy < uy * vx) ? -1.0f : 1.0f) * acosf(r);
}

void lv_jlvg_path_circle2bezier(uint8_t *segments, uint32_t *nseg, float *data, uint32_t *ndata,
                                float cx, float cy, float r, float sx, float sy, float ex, float ey, uint8_t cw, uint8_t half)
{
    float x1, y1, x2, y2;
    float a1, da, hda, kappa;
    int ndivs;
    float x, y, tanx, tany, px = 0, py = 0, ptanx = 0, ptany = 0;

    *nseg = 0;
    *ndata = 0;

    // Start point
    x1 = sx;
    y1 = sy;
    x2 = ex;
    y2 = ey;

    if (half) {
        // Handle half circle case
        float ux = x1 - cx;
        float uy = y1 - cy;
        float vx = x2 - cx;
        float vy = y2 - cy;

        hda = JLVG_PI / 4.0f;
        hda = (1.0f - cosf(hda)) / sinf(hda);
        kappa = fabsf(4.0f / 3.0f * hda);

        px = x1;
        py = y1;
        if (cw) {
            x = cx + uy;
            y = cy - ux;
            ptanx = uy * kappa;
            ptany = -ux * kappa;
            tanx = -ux * kappa;
            tany = -uy * kappa;
        } else {
            x = cx - uy;
            y = cy + ux;
            ptanx = -uy * kappa;
            ptany = ux * kappa;
            tanx = -ux * kappa;
            tany = -uy * kappa;
        }

        segments[(*nseg)++] = VGHW_CUBIC_TO;
        data[(*ndata)++] = px + ptanx;
        data[(*ndata)++] = py + ptany;
        data[(*ndata)++] = x - tanx;
        data[(*ndata)++] = y - tany;
        data[(*ndata)++] = x;
        data[(*ndata)++] = y;

        float tanx_t = ptanx;
        float tany_t = ptany;

        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
        x = x2;
        y = y2;

        tanx = -tanx_t;
        tany = -tany_t;

        segments[(*nseg)++] = VGHW_CUBIC_TO;
        data[(*ndata)++] = px + ptanx;
        data[(*ndata)++] = py + ptany;
        data[(*ndata)++] = x - tanx;
        data[(*ndata)++] = y - tany;
        data[(*ndata)++] = x;
        data[(*ndata)++] = y;
    } else {
        // Calculate initial and delta angles
        float ux = x1 - cx;
        float uy = y1 - cy;
        float vx = x2 - cx;
        float vy = y2 - cy;

        a1 = __vecang(r, 0.0f, ux, uy);  // Initial angle

        if (!cw) {
            da = __vecang(ux, uy, vx, vy);  // Delta angle
        } else {
            da = -__vecang(vx, vy, ux, uy);  // Delta angle
        }

        // Split arc into max 90 degree segments
        ndivs = (int)ceilf(fabsf(da) / (JLVG_PI * 0.5f)) + 1;
        hda = (da / (float)ndivs) / 2.0f;

        // Avoid division by zero
        if (fabsf(hda) < 1e-6f) {
            hda = 1e-6f;
        }
        hda = (1.0f - cosf(hda)) / sinf(hda);
        kappa = fabsf(4.0f / 3.0f * hda);
        if (da < 0.0f) {
            kappa = -kappa;
        }

        for (int i = 0; i <= ndivs; i++) {
            float a = a1 + da * ((float)i / (float)ndivs);
            float dx = cosf(a);
            float dy = sinf(a);
            x = dx * r + cx;
            y = dy * r + cy;
            tanx = -dy * r * kappa;
            tany = dx * r * kappa;

            if (i == ndivs) {
                x = x2;
                y = y2;
            }

            if (i > 0) {
                segments[(*nseg)++] = VGHW_CUBIC_TO;
                data[(*ndata)++] = px + ptanx;
                data[(*ndata)++] = py + ptany;
                data[(*ndata)++] = x - tanx;
                data[(*ndata)++] = y - tany;
                data[(*ndata)++] = x;
                data[(*ndata)++] = y;
            }
            px = x;
            py = y;
            ptanx = tanx;
            ptany = tany;
        }
    }
}



/**
 * 创建圆角矩形的vg路径数据。
 * @param area 区域坐标
 * @param radius 圆角半径
 * @param op_buf 输出的操作符缓冲区
 * @param coord_buf 输出的坐标缓冲区
 * @param op_size 输出的路径操作数量
 * @param coord_size 输出的路径坐标数量
 */
void create_rounded_rect_jlvg_path(lv_area_t *coords, uint16_t final_radius,
                                   uint8_t *op_buf, float *coord_buf, int *op_size, int *coord_size)
{
    if (!(op_buf) || !(coord_buf)) {
        return;
    }

    /* Path data element index */
    uint8_t op_idx = 0;
    uint32_t coord_idx = 0;

    if (final_radius > 0) {
        /* Get the control point offset for rounded cases */
        int32_t cpoff = (int32_t)((float)final_radius * KAPPA);

        /* Rounded rectangle case */
        /* Starting point */
        op_buf[op_idx++] = VGHW_MOVE_TO;
        coord_buf[coord_idx++] = coords->x1 + final_radius;  // 起始点 x
        coord_buf[coord_idx++] = coords->y1;                // 起始点 y

        /* Top side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x2 - final_radius + 1;  // Extended for VGLite
        coord_buf[coord_idx++] = coords->y1;

        /* Top-right corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x2 - final_radius + cpoff;  // 控制点1 x
        coord_buf[coord_idx++] = coords->y1;                        // 控制点1 y
        coord_buf[coord_idx++] = coords->x2;                        // 控制点2 x
        coord_buf[coord_idx++] = coords->y1 + final_radius - cpoff; // 控制点2 y
        coord_buf[coord_idx++] = coords->x2;                        // 终点 x
        coord_buf[coord_idx++] = coords->y1 + final_radius;         // 终点 y

        /* Right side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x2 + 1;                     // Extended for VGLite
        coord_buf[coord_idx++] = coords->y2 - final_radius + 1;     // Extended for VGLite

        /* Bottom-right corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x2;                        // 控制点1 x
        coord_buf[coord_idx++] = coords->y2 - final_radius + cpoff; // 控制点1 y
        coord_buf[coord_idx++] = coords->x2 - final_radius + cpoff; // 控制点2 x
        coord_buf[coord_idx++] = coords->y2;                        // 控制点2 y
        coord_buf[coord_idx++] = coords->x2 - final_radius;         // 终点 x
        coord_buf[coord_idx++] = coords->y2;                        // 终点 y

        /* Bottom side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x1 + final_radius;         // 终点 x
        coord_buf[coord_idx++] = coords->y2 + 1;                    // Extended for VGLite

        /* Bottom-left corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x1 + final_radius - cpoff; // 控制点1 x
        coord_buf[coord_idx++] = coords->y2;                        // 控制点1 y
        coord_buf[coord_idx++] = coords->x1;                        // 控制点2 x
        coord_buf[coord_idx++] = coords->y2 - final_radius + cpoff; // 控制点2 y
        coord_buf[coord_idx++] = coords->x1;                        // 终点 x
        coord_buf[coord_idx++] = coords->y2 - final_radius;         // 终点 y

        /* Left side */
        op_buf[op_idx++] = VGHW_LINE_TO;
        coord_buf[coord_idx++] = coords->x1;                        // 终点 x
        coord_buf[coord_idx++] = coords->y1 + final_radius;         // 终点 y

        /* Top-left corner */
        op_buf[op_idx++] = VGHW_CUBIC_TO;  // 绝对坐标操作
        coord_buf[coord_idx++] = coords->x1;                        // 控制点1 x
        coord_buf[coord_idx++] = coords->y1 + final_radius - cpoff; // 控制点1 y
        coord_buf[coord_idx++] = coords->x1 + final_radius - cpoff; // 控制点2 x
        coord_buf[coord_idx++] = coords->y1;                        // 控制点2 y
        coord_buf[coord_idx++] = coords->x1 + final_radius;         // 终点 x
        coord_buf[coord_idx++] = coords->y1;                        // 终点 y
    }

    /* Resulting path size */
    *op_size = op_idx;
    *coord_size = coord_idx;

}



/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_JLVG*/
