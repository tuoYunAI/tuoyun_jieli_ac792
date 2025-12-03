
#ifndef __RLE_HEADER_H__
#define __RLE_HEADER_H__

#include <stdint.h>

struct rle_info {
    uint32_t width : 12;
    uint32_t height : 12;
    uint32_t format : 5;      // 0 : ARGB8888,
    // 1 : ARGB8565
    // 2 : ARGB1555
    // 3 : ARGB4444
    // 4 : RGB888
    // 5 : RGB565
    // 6 : BT601
    // 7 : BT709
    // 8 : AL88
    // 16 : AL44
    // 17 : AL22
    // 18 : L8
    // 19 : L4
    // 20 : L2
    // 21 : L1
    // 22 : A8
    // 23 : A4
    // 24 : A2
    // 25 : A1

    uint32_t block : 2;       // 0 : block_64x1
    // 1 : block_8x8
    // 2 : block_256x1
    // 3 : block_16x16

    uint32_t clut_format : 2; // 0 : ARGB8888
    // 1 : ARGB8565
    // 2 : RGB888
    // 3 : RGB565

    uint32_t yuv_format : 1;  // 0 : YUYV
    // 1 : UYVY
};

struct image_info {
    uint16_t width;			//ͼ�����
    uint16_t height;			//ͼ��߶�
    uint16_t stride;			//ͼ��ÿ�������ֽ�
    uint8_t format;			//ͼ���ʽ
    uint8_t lut_format;		//��ɫ����ʽ
    uint8_t yuv_format;		//0:YUYV���и�ʽ    1:UYVY���и�ʽ
    uint8_t rle_format;		//0: ��RLE��ʽ  	1:RLE��ʽ
    uint8_t rsv0;
    uint8_t rsv1;
    uint32_t rsv2;
    uint32_t lut_addr;		//��ɫ����ַ
    uint32_t lut_len;		//��ɫ����С
    uint32_t addr;			//ѹ�����߷�ѹ�����ݵ�ַ
    uint32_t len;			//ѹ�����߷�ѹ�����ݳ���
};

#endif

