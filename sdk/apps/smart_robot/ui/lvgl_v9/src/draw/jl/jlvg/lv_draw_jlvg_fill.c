/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_jlvg.h"

#if LV_USE_DRAW_JLVG
#if (LV_USE_DRAW_JLVG_FILL_ENABLE == 1)

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

void lv_draw_jlvg_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords)
{
    //log_debug("%s()", __func__);
    lv_draw_jlvg_unit_t *u = (lv_draw_jlvg_unit_t *)draw_unit;
    lv_draw_task_t *t = u->task_act;
    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
        return;
    }

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_coords;
    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    //lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;   //这个区域无论是帧buff还是行buff都是相对的新的绘制区域
    if (!lv_area_intersect(&blend_area, coords, &rel_clip_area)) {
        return; /*Fully clipped, nothing to do*/
    }
    //printf("blend_area (x1,y1) = (%d,%d); (x2,y2) = (%d,%d).", blend_area.x1, blend_area.y1, blend_area.x2, blend_area.y2);

    //开始 JLVG 渲染动作
    // 1、初始化渲染空间
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_size;
    uint32_t dest_stride;
    uint8_t *dest_buf;
    lv_color_format_t dest_cf = draw_buf->header.cf;
    uint32_t bytes_per_pixel = lv_color_format_get_size(dest_cf);
    int dest_x = 0;
    int dest_y = 0;

#if (JLVG_DRAW_FULL_FB_ENABLE == 1)
    fb_width = layer->buf_area.x2 - layer->buf_area.x1 + 1;     // 显存区域的宽度
    fb_height = layer->buf_area.y2 - layer->buf_area.y1 + 1;    // 显存区域的高度
    fb_size = fb_width * fb_height * bytes_per_pixel;           // 显存区域的buff大小
    dest_stride = draw_buf->header.stride;
    dest_buf = draw_buf->data;  //最终会更新到屏幕上面的 buff 区域
#else
    dest_stride = draw_buf->header.stride;
    //dest_buf = draw_buf->data + (blend_area.x1 * bytes_per_pixel) + (blend_area.y1 * dest_stride);
    //计算目标地址偏移
    lv_area_intersect(&rel_coords, coords, &layer->buf_area);
    dest_x = rel_coords.x1 - layer->buf_area.x1;
    dest_y = rel_coords.y1 - layer->buf_area.y1;
    fb_width = lv_area_get_width(&rel_coords);
    fb_height = lv_area_get_height(&rel_coords);
    fb_size = fb_width * fb_height * bytes_per_pixel;   // 绘制区域的buff大小
    dest_buf = lv_draw_buf_goto_xy(draw_buf, dest_x, dest_y);
#endif
    //printf("x:%d y:%d fb_width = %d; fb_height = %d; dest_stride = %d.\n", blend_area.x1, blend_area.y1, fb_width, fb_height, dest_stride);


    jlvg_matrix_t user2surface; // 变换矩阵 user to surface
    jlvg_matrix_t surface2paint;// 变换矩阵 surface to paint

    jlvg_surface_t surface;     // 绘制的窗口
    jlvg_rect_t surface_area;   // 绘制的窗口区域:实际就是 layer->buf_area 所在的区域
    jlvg_rect_t draw_area;      // 在窗口内实际要绘制的区域:实际就是 blend_area

    //初始化单位变换矩阵
    jlvg_matrix_set_identity(&user2surface);
    jlvg_matrix_set_identity(&surface2paint);

    //窗口初始化
    jlvg_hw_imageformat_t jlvg_dest_cf = lv_jlvg_get_color_format(dest_cf, NULL);
    jlvg_surface_init(&surface, dest_buf, fb_size, fb_width, fb_height, dest_stride, jlvg_dest_cf, 0, 1, 1, 0);

    //初始化要绘制的区域
#if (JLVG_DRAW_FULL_FB_ENABLE == 1)
    draw_area.x = blend_area.x1;
    draw_area.y = blend_area.y1;
    draw_area.w = blend_area.x2 - blend_area.x1 + 1;
    draw_area.h = blend_area.y2 - blend_area.y1 + 1;
#else
    // path区域
    draw_area.x = (coords->x1 >= 0) ? 0 : coords->x1;
    draw_area.y = (coords->y1 >= 0) ? 0 : coords->y1;
    draw_area.w = lv_area_get_width(coords);
    draw_area.h = lv_area_get_height(coords);
#endif
    //printf("draw_area (x,y) = (%d,%d); w = %d, y = %d.", draw_area.x, draw_area.y, draw_area.w, draw_area.h);

    // 2、配置绘制路径
    jlvg_path_t rect_path;      // 绘制的矩形路径
    uint32_t nsegments_over = 0;    //绘制结束需要的路径步数
    uint8_t segments_over[16];  //绘制完整的路径动作
    float points_over[64];      //绘制结束需要的路径坐标

    if (dsc->radius != 0) { /* 有圆角属性 */
        /* printf("radius = %d", dsc->radius); */
        /* ASSERT(0, "There are still exceptions unresolved"); */
        //圆角矩形路径描述
        //确定四个圆弧的圆心位置以及四个起始点 + 四个终点，以左上角圆弧为起点： 1 + 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1   //13, p0 -> p3 顺时针
        int32_t coords_w = lv_area_get_width(coords);
        int32_t coords_h = lv_area_get_height(coords);
        int32_t short_side = LV_MIN(coords_w, coords_h);
        int32_t rout = LV_MIN(dsc->radius, short_side >> 1);    // 计算圆角的半径

        uint32_t nsegments = 0;
        int npoints = 0;
        uint8_t segments[1 + 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1];     //13
        float points[2 + 2 * 6 + 2 + 2 * 6 + 2 + 2 * 6 + 2 + 2 * 6 + 2];
#if 1
        jlvg_point2_t p0;
        p0.x = rout + draw_area.x;
        p0.y = rout + draw_area.y;
        jlvg_point2_t p01;
        p01.x = rout + draw_area.x;
        p01.y = draw_area.y;
        jlvg_point2_t p02;
        p02.x = draw_area.x;
        p02.y = rout + draw_area.y;

        jlvg_point2_t p1;
        p1.x = rout + draw_area.x;
        //p1.y = draw_area.y + draw_area.h - 1 - rout;
        p1.y = draw_area.y + draw_area.h - rout;    //修正
        jlvg_point2_t p11;
        p11.x = draw_area.x;
        //p11.y = draw_area.y + draw_area.h - 1 - rout;
        p11.y = draw_area.y + draw_area.h - rout;   //修正
        jlvg_point2_t p12;
        p12.x = rout + draw_area.x;
        //p12.y = draw_area.y + draw_area.h - 1;
        p12.y = draw_area.y + draw_area.h;  //修正

        jlvg_point2_t p2;
        //p2.x = draw_area.x + draw_area.w - 1 - rout;
        //p2.y = draw_area.y + draw_area.h - 1 - rout;
        p2.x = draw_area.x + draw_area.w - rout;    //修正
        p2.y = draw_area.y + draw_area.h - rout;    //修正
        jlvg_point2_t p21;
        //p21.x = draw_area.x + draw_area.w - 1 - rout;
        //p21.y = draw_area.y + draw_area.h - 1;
        p21.x = draw_area.x + draw_area.w - rout;   //修正
        p21.y = draw_area.y + draw_area.h;  //修正
        jlvg_point2_t p22;
        //p22.x = draw_area.x + draw_area.w - 1;
        //p22.y = draw_area.y + draw_area.h - 1 - rout;
        p22.x = draw_area.x + draw_area.w;  //修正
        p22.y = draw_area.y + draw_area.h - rout;   //修正

        jlvg_point2_t p3;
        //p3.x = draw_area.x + draw_area.w - 1 - rout;
        p3.x = draw_area.x + draw_area.w - rout;    //修正
        p3.y = rout + draw_area.y;
        jlvg_point2_t p31;
        //p31.x = draw_area.x + draw_area.w - 1;
        p31.x = draw_area.x + draw_area.w;  //修正
        p31.y = rout + draw_area.y;
        jlvg_point2_t p32;
        //p32.x = draw_area.x + draw_area.w - 1 - rout;
        p32.x = draw_area.x + draw_area.w - rout;   //修正
        p32.y = draw_area.y;

        int nseg_tmp = 0;
        int npoints_tmp = 0;

        //moveto
        segments[nsegments++] = VGHW_MOVE_TO;   //起点 and 终点
        points[npoints++] = p01.x;
        points[npoints++] = p01.y;

        //1st arc
        lv_jlvg_path_circle2bezier(&segments[nsegments], &nseg_tmp, &points[npoints], &npoints_tmp,
                                   p0.x, p0.y, rout, p01.x, p01.y, p02.x, p02.y, 0, 0);
        nsegments += nseg_tmp;
        npoints += npoints_tmp;

        //1st line to
        segments[nsegments++] = VGHW_LINE_TO;
        points[npoints++] = p11.x;
        points[npoints++] = p11.y;

        //2nd arc
        lv_jlvg_path_circle2bezier(&segments[nsegments], &nseg_tmp, &points[npoints], &npoints_tmp,
                                   p1.x, p1.y, rout, p11.x, p11.y, p12.x, p12.y, 0, 0);
        nsegments += nseg_tmp;
        npoints += npoints_tmp;

        //2nd line to
        segments[nsegments++] = VGHW_LINE_TO;
        points[npoints++] = p21.x;
        points[npoints++] = p21.y;

        //3rd arc
        lv_jlvg_path_circle2bezier(&segments[nsegments], &nseg_tmp, &points[npoints], &npoints_tmp,
                                   p2.x, p2.y, rout, p21.x, p21.y, p22.x, p22.y, 0, 0);
        nsegments += nseg_tmp;
        npoints += npoints_tmp;

        //3rd line to
        segments[nsegments++] = VGHW_LINE_TO;
        points[npoints++] = p31.x;
        points[npoints++] = p31.y;

        //4th arc
        lv_jlvg_path_circle2bezier(&segments[nsegments], &nseg_tmp, &points[npoints], &npoints_tmp,
                                   p3.x, p3.y, rout, p31.x, p31.y, p32.x, p32.y, 0, 0);
        nsegments += nseg_tmp;
        npoints += npoints_tmp;

        //4th line to
        segments[nsegments++] = VGHW_LINE_TO;
        points[npoints++] = p01.x;
        points[npoints++] = p01.y;
#else
        create_rounded_rect_jlvg_path(&draw_area, rout, &segments[0], &points[0], &nsegments, &npoints);
#endif
        nsegments_over = nsegments;
        memcpy(segments_over, segments, nsegments);
        memcpy(points_over, points, npoints * sizeof(float));
        jlvg_path_init(&rect_path, JLVG_PATH_DATATYPE_F32, nsegments_over, segments_over, npoints, points_over);


        //printf("\nX=[ ");
        //for (int i = 0; i < npoints; i+=2) {
        //    printf("%f,", points[i]);
        //}
        //printf("]\n");
        //printf("Y=[ ");
        //for (int i = 0; i < npoints; i += 2) {
        //    printf("%f,", points[i + 1]);
        //}
        //printf("]\n ");
    } else {
        //printf("no radius");
        //常规矩形路径描述
        //描述四个角的路径
        uint32_t nsegments = 1 + 1 + 1 + 1 + 1;//5
        uint8_t segments[1 + 1 + 1 + 1 + 1] = { VGHW_MOVE_TO, VGHW_HLINE_TO, VGHW_VLINE_TO, VGHW_HLINE_TO, VGHW_VLINE_TO };
        float points[2 + 2 + 2];
        points[0] = points[4] = draw_area.x;    // 原点(起点和终点)的 x
        points[1] = points[5] = draw_area.y;    // 原点(起点和终点)的 y
        //points[2] = draw_area.x + draw_area.w - 1;  // 矩形原点对角的 x : 实际的坐标，但是目前 VG 库绘图效果不对，需要修正
        //points[3] = draw_area.y + draw_area.h - 1;  // 矩形原点对角的 y
        points[2] = draw_area.x + draw_area.w;  // 矩形原点对角的 x : 修正
        points[3] = draw_area.y + draw_area.h;  // 矩形原点对角的 y : 修正

        nsegments_over = nsegments;
        memcpy(segments_over, segments, nsegments);
        memcpy(points_over, points, sizeof(points));
        jlvg_path_init(&rect_path, JLVG_PATH_DATATYPE_F32, nsegments_over, segments_over, sizeof(points) / sizeof(float), points_over);
    }
    jlvg_path_update_fill(&rect_path);  //更新路径信息

    //jlvg_boundbox_t bbox_out;
    //jlvg_path_transform_boundbox(&bbox_out, &rect_path.bbox, &user2surface);
    //jlvg_bbox2rect(&draw_area, &bbox_out);

    //3、配置颜色填充
    //渐变属性填充颜色
    jlvg_paint_linear_t paint_linear;   // 渐变渲染的颜色
    jlvg_point2_t g0;
    jlvg_point2_t g1;
    /* 注意 stops_count 和 LV_GRADIENT_MAX_STOPS 的取值 */
    uint32_t cnt = LV_MIN(dsc->grad.stops_count, LV_GRADIENT_MAX_STOPS);    //渐变阶梯是 2 的次幂关系：2 4 8 16 等
    jlvg_color_t rampstops[LV_GRADIENT_MAX_STOPS];

    //正常填充颜色
    jlvg_paint_color_t paint_color;     // 正常渲染的颜色
    jlvg_color_t fill_color;
    jlvg_color_t draw_color;
    if (dsc->grad.dir != (lv_grad_dir_t)LV_GRAD_DIR_NONE) { /* 有渐变属性 */
        printf("has grad");
        ASSERT(0, "There are still exceptions unresolved");
    } else {
        //填充颜色准备
        //printf("no grad");
        fill_color.a = dsc->opa;
        fill_color.r = dsc->color.red;
        fill_color.g = dsc->color.green;
        fill_color.b = dsc->color.blue;
        jlvg_color_premultiply(&fill_color);    // 填充像素预乘
        draw_color = fill_color;
        jlvg_paint_color_init(&paint_color, draw_color); //初始化填充的颜色内容
    }

    // 配置 paint
    jlvg_paint_t *draw_paint;
    if (dsc->grad.dir == (lv_grad_dir_t)LV_GRAD_DIR_NONE) {
        draw_paint = &paint_color;
    } else {
        draw_paint = &paint_linear;
    }

    // 配置 blend mode
    jlvg_hw_blendmode_t blend_mode;
    if (dsc->opa >= (lv_opa_t)LV_OPA_MAX) {
        // paint mode 下使用 VGHW_BLEND_SRC 混合模式和 clear mode 一个效果
        blend_mode = VGHW_BLEND_SRC;
    } else {
        blend_mode = VGHW_BLEND_SRC_OVER;
    }

    // 4、硬件绘制

    jlvg_start_frame(g_jlvg, &surface);
    //重新计算bbox区域,(转成相对于实际绘图区域的坐标)
    draw_area.x = LV_ABS(rel_coords.x1 - blend_area.x1);
    draw_area.y = LV_ABS(rel_coords.y1 - blend_area.y1);
    draw_area.w = lv_area_get_width(&blend_area);
    draw_area.h = lv_area_get_height(&blend_area);
    jlvg_fill_path(g_jlvg, &draw_area, &rect_path, draw_paint, &user2surface, &surface2paint, blend_mode, VGHW_NON_ZERO);

    jlvg_end_frame(g_jlvg);

    //4、清理现场
    jlvg_path_deinit(&rect_path);
    jlvg_paint_deinit((jlvg_paint_t *)&paint_color);
    jlvg_paint_deinit((jlvg_paint_t *)&paint_linear);
    jlvg_surface_deinit(&surface);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_JLVG_FILL_ENABLE*/
#endif /*LV_USE_DRAW_JLVG*/
