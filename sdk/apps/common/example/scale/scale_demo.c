#include "asm/jl_scale_driver.h"
#include "fs/fs.h"

#ifdef USE_SCALE_DEMO

#define JLSCALER_FRACTION_WIDTH     (14)
#define CONFIG_STORAGE_PATH  "storage/sd0"

//static uint8_t	src_img_adr[32*128] sec(.volatile_ram);
//static uint8_t	dst_img_adr[128*128] sec(.volatile_ram);

int test_scale_demo(int test_num, int test_mode) //1/3/4通道测试例子
{

    /*printf("\n[lyc][this file] %s -[this function] %s -[this lnie] %d\n", __FILE__, __FUNCTION__, __LINE__);*/

    int src_w = 640;
    int src_h = 480;

    uint32_t dst_w = 1280;//1280
    uint32_t dst_h = 720;//720

    uint32_t x_step = 0.5f * (1 << JLSCALER_FRACTION_WIDTH);//缩放比
    uint32_t y_step = 0.667f * (1 << JLSCALER_FRACTION_WIDTH);

    /*printf("x_step == %d\n",x_step);*/
    /*printf("y_step == %d\n",y_step);*/

    int32_t x_offset = 0;
    int32_t y_offset = 0;

    x_step = ((float)(src_w - x_offset) / (float)dst_w) * (1 << JLSCALER_FRACTION_WIDTH);
    y_step = ((float)(src_h - y_offset) / (float)dst_h) * (1 << JLSCALER_FRACTION_WIDTH);

    /*printf("x_step == %d\n",x_step);*/
    /*printf("y_step == %d\n",y_step);*/

    x_offset = ((float)x_offset) * (1 << JLSCALER_FRACTION_WIDTH);
    y_offset = ((float)y_offset) * (1 << JLSCALER_FRACTION_WIDTH);


    if (test_mode == 1) { //放大到720p
        src_w = 640;
        src_h = 480;

        dst_w = 1280;//
        dst_h = 720;//

        x_offset = 0;
        y_offset = 0;

        x_step = ((float)(src_w - x_offset) / (float)dst_w) * (1 << JLSCALER_FRACTION_WIDTH);
        y_step = ((float)(src_h - y_offset) / (float)dst_h) * (1 << JLSCALER_FRACTION_WIDTH);

        x_offset = ((float)x_offset) * (1 << JLSCALER_FRACTION_WIDTH);
        y_offset = ((float)y_offset) * (1 << JLSCALER_FRACTION_WIDTH);
    }

    else if (test_mode == 0) { //放大到2k
        src_w = 1280;
        src_h = 720;

        dst_w = 2560;//2k  通道4:14.7m
        dst_h = 1440;//

        x_offset = 0;
        y_offset = 0;

        x_step = ((float)(src_w - x_offset) / (float)dst_w) * (1 << JLSCALER_FRACTION_WIDTH);
        y_step = ((float)(src_h - y_offset) / (float)dst_h) * (1 << JLSCALER_FRACTION_WIDTH);

        x_offset = ((float)x_offset) * (1 << JLSCALER_FRACTION_WIDTH);
        y_offset = ((float)y_offset) * (1 << JLSCALER_FRACTION_WIDTH);
    } else if (test_mode == 2) { //缩小10倍
        src_w = 640;
        src_h = 480;

        dst_w = 64;//
        dst_h = 48;//

        x_offset = 0;
        y_offset = 0;

        x_step = ((float)(src_w - x_offset) / (float)dst_w) * (1 << JLSCALER_FRACTION_WIDTH);
        y_step = ((float)(src_h - y_offset) / (float)dst_h) * (1 << JLSCALER_FRACTION_WIDTH);

        x_offset = ((float)x_offset) * (1 << JLSCALER_FRACTION_WIDTH);
        y_offset = ((float)y_offset) * (1 << JLSCALER_FRACTION_WIDTH);

    } else if (test_mode == 3) { //放大小像素点
        src_w = 640;
        src_h = 480;

        dst_w = 641;//
        dst_h = 483;//

        x_offset = 0;
        y_offset = 0;

        x_step = ((float)(src_w - x_offset) / (float)dst_w) * (1 << JLSCALER_FRACTION_WIDTH);
        y_step = ((float)(src_h - y_offset) / (float)dst_h) * (1 << JLSCALER_FRACTION_WIDTH);

        x_offset = ((float)x_offset) * (1 << JLSCALER_FRACTION_WIDTH);
        y_offset = ((float)y_offset) * (1 << JLSCALER_FRACTION_WIDTH);

    } else if (test_mode == 4) { //偏移一半
        src_w = 640;
        src_h = 480;

        dst_w = 1280;//
        dst_h = 720;//

        x_offset = 320;
        y_offset = 240;

        x_step = ((float)(src_w - x_offset) / (float)dst_w) * (1 << JLSCALER_FRACTION_WIDTH);
        y_step = ((float)(src_h - y_offset) / (float)dst_h) * (1 << JLSCALER_FRACTION_WIDTH);

        x_offset = ((float)x_offset) * (1 << JLSCALER_FRACTION_WIDTH);
        y_offset = ((float)y_offset) * (1 << JLSCALER_FRACTION_WIDTH);
    } else if (test_mode == 5) { //自由设置
        src_w = 640;
        src_h = 480;

        dst_w = 1280;//
        dst_h = 720;//

        x_step = 0.3f * (1 << JLSCALER_FRACTION_WIDTH);
        y_step = 0.3f * (1 << JLSCALER_FRACTION_WIDTH);

        x_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);
        y_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);

    }

    int channel = test_num;// 1/3/4

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };

    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = JLSCALER_INTERP_BILINEAR;//插值算法切换

    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel);
    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel);
    if (!src_img_adr) {
        printf("Failed to malloc src_img_adr.\n");
        return 1;
    }

    if (!dst_img_adr) {
        printf("Failed to malloc dst_img_adr.\n");
        return 1;
    }

    memset(dst_img_adr, 0, dst_w * dst_h * channel);

    FILE *fp = NULL;

#if 1  //读卡
    if (test_num == 1)
        /*fp = fopen(CONFIG_STORAGE_PATH"/C/src0123.bin", "rb");*/
    {
        fp = fopen(CONFIG_STORAGE_PATH"/C/2_4801.bin", "rb");
    } else if (test_num == 3) {
        fp = fopen(CONFIG_STORAGE_PATH"/C/2_4803.bin", "rb");
    } else if (test_num == 4) {
        fp = fopen(CONFIG_STORAGE_PATH"/C/2_4804.bin", "rb");
    }

    if (fp == NULL) {
        printf("Failed to load src file.\n");
        return 1;
    }
    fread(src_img_adr, 1, src_w * src_h * channel, fp);
    fclose(fp);
#else  //直接写数据

    memset(src_img_adr, 255, src_w * src_h * channel);
    //memset(src_img_adr,0,src_w*src_h*channel);
#endif
    scale_init();//寄存器清零
    printf("scale_init");


#if 1 //按帧处理

    /*printf("src_adr=%x,dst_adr=%x",src_img_adr,dst_img_adr);*/
    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                    dst_img_adr, dst_w, dst_h, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);//填入算法参数
    printf("set_param finish");

    DcuFlushRegion(src_img_adr, src_w * src_h * channel);
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel); //

    u32 start_time = timer_get_ms(); //获取起始滴答秒

    scale_run();//配置寄存器 kick start 即开始工作

    printf("scale_running\n");

    scale_wait_done();//等待pending响应,并且clean掉中断标志

    u32 end_time = timer_get_ms();   //获取结束滴答秒
    printf("\n--------------------------run_time = %d ms\n", end_time - start_time); //打印获取的代码运行时间(精确到ms)

    printf("scale wait done finish\n");//处理完成


#else  //按行/列处理


#if 1  //按行处理
    int32_t y_offset_line = 0;
    int offset = 0;
    int h_num = 16;
    int count = dst_h / h_num;
    for (int k = 0; k < count; k++) {
        offset = k * h_num * dst_w * channel;

        y_offset_line = k * h_num * y_step + y_offset;

        set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                        dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);

        DcuFlushRegion(src_img_adr, src_w * src_h * channel);
        DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel); //

        scale_run();
        scale_wait_done();
    }
#else//按列处理

    int offset = 0;
    int w_num = 16;
    int count = dst_w / w_num;
    for (int k = 0; k < count; k++) {
        offset = k * w_num * channel;
        x_offset = k * w_num * x_step;
        set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                        dst_img_adr + offset, w_num, dst_h, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);

        DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel);
        scale_run();
        scale_wait_done();
    }
#endif

#endif

    /*printf("[line-%d] printf_the_reg:%x- %d\n",__LINE__,dst_img_adr,dst_w * dst_h * channel);*/

#if 1
    FILE *out_fp = NULL;


    if (test_mode == 0) {
        if (test_num == 1) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out01.bin", "wb");
        } else if (test_num == 3) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out03.bin", "wb");
        } else if (test_num == 4) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out04.bin", "wb");
        }
    } else if (test_mode == 1) {
        if (test_num == 1) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out11.bin", "wb");
        } else if (test_num == 3) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out13.bin", "wb");
        } else if (test_num == 4) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out14.bin", "wb");
        }
    } else if (test_mode == 2) {
        if (test_num == 1) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out21.bin", "wb");
        } else if (test_num == 3) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out23.bin", "wb");
        } else if (test_num == 4) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out24.bin", "wb");
        }
    } else if (test_mode == 3) {
        if (test_num == 1) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out31.bin", "wb");
        } else if (test_num == 3) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out33.bin", "wb");
        } else if (test_num == 4) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out34.bin", "wb");
        }
    } else if (test_mode == 4) {
        if (test_num == 1) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out41.bin", "wb");
        } else if (test_num == 3) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out43.bin", "wb");
        } else if (test_num == 4) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out44.bin", "wb");
        }
    } else if (test_mode == 5) {
        if (test_num == 1) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out51.bin", "wb");
        } else if (test_num == 3) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out53.bin", "wb");
        } else if (test_num == 4) {
            out_fp = fopen(CONFIG_STORAGE_PATH"/C/out54.bin", "wb");
        }
    }
    if (out_fp == NULL) {
        printf("out file is not exist\n");
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel, out_fp);//(首地址,被写入每个字节大小,元素个数，流)

    fclose(out_fp);

#endif
    printf("save done");
    free(src_img_adr);
    free(dst_img_adr);

    scale_free();
    printf("free done");
}


int yuv_test_code_by_line_420p(int mode)//yuv420p数据测试例子
{
    printf("\n[this function] %s -[this lnie] %d\n", __FUNCTION__, __LINE__);

    int extra_rows = 0;
    int src_w = 640;//
    int src_h = 480;
    uint32_t dst_w = 2560;//
    uint32_t dst_h = 1080;//
    uint32_t x_step = 0.25f * (1 << JLSCALER_FRACTION_WIDTH);//设置缩放比
    uint32_t y_step = 0.444f * (1 << JLSCALER_FRACTION_WIDTH);
    int32_t x_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);//设置偏移量
    int32_t y_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);
    int channel = 1;//使用通道1

    if (mode == 0) {
        dst_w = 2560;//
        dst_h = 1440;//
    } else if (mode == 1) {
        dst_w = 4096;//
        dst_h = 2048;//
    } else if (mode == 2) {
        dst_w = 320;//
        dst_h = 240;//
    }



    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel * 3 / 2); //一帧yuv420pbuf
    if (!src_img_adr) {
        printf("Failed to load src image.\n");
        return -1;
    }
    FILE *fp = fopen(CONFIG_STORAGE_PATH"/C/yuv420.bin", "rb");//格式yuv420p:640*320 大小:640*320*3/2
    if (fp == NULL) {
        printf("file is not exist\n");
        printf("\n[lyc][this file] %s -[this function] %s -[this lnie] %d\n", __FILE__, __FUNCTION__, __LINE__);
    }
    fread(src_img_adr, 1, src_w * src_h * channel * 3 / 2, fp);
    fclose(fp);

    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel * 3 / 2);
    if (!dst_img_adr) {
        return -1;
    }

    memset(dst_img_adr, 0, dst_w * dst_h * channel * 3 / 2);//buf清0


    scale_init();//寄存器清零
    printf("scale_init");

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };
    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = JLSCALER_INTERP_BILINEAR;//插值算法切换

    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;

    int offset = 0;
    int h_num = 16;
    int count = 0;
    int32_t y_offset_line = 0;

    DcuFlushRegion(src_img_adr, src_w * src_h * channel * 3 / 2);
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel * 3 / 2);


    u32 start_time = timer_get_ms(); //获取起始滴答秒


    for (int k = 0; k < 3; k++) {

        switch (k) {
        case 0:
            //处理y
            offset = 0;
            y_offset_line = 0;

            count = dst_h / h_num;
            extra_rows = dst_h % h_num;
            if (extra_rows != 0) {
                count += 1;
            }
            printf("????--------resdiue == %d-------\n", extra_rows);

            for (int k = 0; k < count; k++) {

                if (k == count - 1 && extra_rows) { //尾行处理
                    printf("count == %d\n", k);
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, extra_rows, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                    /*printf("-------------the dst_img_adr == %x---------\n",dst_img_adr + offset);	*/
                } else {

                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);

                    scale_run();
                    scale_wait_done();

                    /*printf("count == %d\n",k);*/
                }

            }

            printf("\n--------process y done ---------\n");

            break;
        case 1:
            //处理u
            src_offset = src_h * src_w * channel;
            dst_offset = dst_h * dst_w * channel;

            offset = 0;
            count = (dst_h / 2) / h_num;
            extra_rows = (dst_h / 2) % h_num;
            if (extra_rows != 0) {
                count += 1;
            }

            printf("????--------resdiue == %d-------\n", extra_rows);

            for (int k = 0; k < count; k++) {


                if (k == count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }

                else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }

            printf("\n--------process u done ---------\n");
            break;
        case 2:
            //处理v
            src_offset = src_h * src_w * 5 / 4 * channel;
            dst_offset = dst_h * dst_w * 5 / 4 * channel;

            offset = 0;
            count = (dst_h / 2) / h_num;
            if (extra_rows != 0) {
                count += 1;
            }

            printf("????--------resdiue == %d-------\n", extra_rows);

            for (int k = 0; k < count; k++) {

                if (k == count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();

                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);

                    scale_run();
                    scale_wait_done();
                }
            }

            printf("\n--------process v done ---------\n");
            break;
        default:
            break;
        }
    }

    u32 end_time = timer_get_ms();   //获取结束滴答秒
    printf("\n--------------------------run_time = %d ms\n", end_time - start_time); //打印获取的代码运行时间(精确到ms)


    printf("cache done");
    FILE *out_fp = NULL;

    if (mode == 0) {

        out_fp = fopen(CONFIG_STORAGE_PATH"/C/out4200.bin", "wb");
    } else if (mode == 1) {

        out_fp = fopen(CONFIG_STORAGE_PATH"/C/out4201.bin", "wb");
    } else if (mode == 2) {

        out_fp = fopen(CONFIG_STORAGE_PATH"/C/out4202.bin", "wb");
    }


    if (out_fp == NULL) {
        printf("out file is not exist\n");
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel * 3 / 2, out_fp); //(首地址,被写入每个字节大小,元素个数，流)

    fclose(out_fp);

    printf("save done");
    free(src_img_adr);
    free(dst_img_adr);

    scale_free();
    printf("free done");

}


int yuv_test_code_by_line_422p(int mode)//yuv422p数据测试例子
{
    printf("running the yuv422p by line\n");

    int extra_rows = 0;
    int src_w = 640;//
    int src_h = 480;
    int channel = 1;//使用通道1
    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel * 2); //一帧yuv420pbuf
    if (!src_img_adr) {
        printf("Failed to load src image.\n");
        return -1;
    }
    //读文件yuv420p,写入src_img_adr buf
    FILE *fp = fopen(CONFIG_STORAGE_PATH"/C/yuv422.bin", "rb");//格式yuv420p:640*320 大小:640*320*3/2
    if (fp == NULL) {
        printf("file is not exist\n");
        return -1;
    }
    fread(src_img_adr, 1, src_w * src_h * channel * 2, fp);
    fclose(fp);

    uint32_t dst_w = 1280;//
    uint32_t dst_h = 720;//

    if (mode == 0) {
        dst_w = 2560;//
        dst_h = 1080;//
    } else if (mode == 1) {
        dst_w = 4096;//
        dst_h = 2048;//
    } else if (mode == 2) {
        dst_w = 320;//
        dst_h = 240;//
    }

    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel * 2);
    if (!dst_img_adr) {
        printf("malloc dst_img_adr error\n");
        return -1;
    }

    memset(dst_img_adr, 0, dst_w * dst_h * channel *  2);//buf清0

    uint32_t x_step = 0.25f * (1 << JLSCALER_FRACTION_WIDTH);//设置缩放比
    uint32_t y_step = 0.333f * (1 << JLSCALER_FRACTION_WIDTH);

    int32_t x_offset = 320.0f * (1 << JLSCALER_FRACTION_WIDTH);//设置偏移量
    int32_t y_offset = 240.0f * (1 << JLSCALER_FRACTION_WIDTH);


    scale_init();
    printf("scale_init");

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };

    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = JLSCALER_INTERP_BILINEAR;//插值算法切换


    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;

    int offset = 0;
    int h_num = 16;
    int count = 0;
    int32_t y_offset_line = 0;

    DcuFlushRegion(src_img_adr, src_w * src_h * channel * 2);
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel * 2);


    u32 start_time = timer_get_ms(); //获取起始滴答秒

    for (int k = 0; k < 3; k++) {

        switch (k) {
        case 0:
            //处理y
            offset = 0;
            y_offset_line = 0;
            count = dst_h / h_num;
            extra_rows = dst_h % h_num;
            if (extra_rows) {
                count += 1;
            }

            for (int k = 0; k < count; k++) {

                if (k != count - 1 && extra_rows != 0) {
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, extra_rows, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }

            printf("\n--------process y done ---------\n");

            break;
        case 1:
            //处理u
            src_offset = src_h * src_w * channel;
            dst_offset = dst_h * dst_w * channel;

            offset = 0;
            /*extra_rows = dst_h % h_num;*/
            /*if (extra_rows) { count += 1;}*/

            for (int k = 0; k < count; k++) {
                if (k != count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }


            printf("\n--------process u done ---------\n");
            break;
        case 2:
            //处理v
            src_offset = src_h * src_w * 3 / 2 * channel;
            dst_offset = dst_h * dst_w * 3 / 2 * channel;

            offset = 0;
            /*extra_rows = dst_h % h_num;*/
            /*if (extra_rows) { count += 1;}*/

            for (int k = 0; k < count; k++) {
                if (k != count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }

            printf("\n--------process v done ---------\n");
            break;
        default:
            break;
        }
    }

    u32 end_time = timer_get_ms();   //获取结束滴答秒
    printf("\n--------------------------run_time = %d ms\n", end_time - start_time); //打印获取的代码运行时间(精确到ms)


    printf("cache done");
    FILE *out_fp = NULL;


    if (mode == 0) {

        out_fp = fopen(CONFIG_STORAGE_PATH"/C/out4220.bin", "wb");
    } else if (mode == 1) {

        out_fp = fopen(CONFIG_STORAGE_PATH"/C/out4221.bin", "wb");
    } else if (mode == 2) {

        out_fp = fopen(CONFIG_STORAGE_PATH"/C/out4222.bin", "wb");
    }

    if (out_fp == NULL) {
        printf("out file is not exist\n");
        return -1;
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel * 2, out_fp); //(首地址,被写入每个字节大小,元素个数，流)

    fclose(out_fp);

    printf("save done");

    free(src_img_adr);
    free(dst_img_adr);
    scale_free();
    printf("free done");

}


int yuv420p_scale_demo()//yuv420p处理例子
{
    printf("\n yuv420p scale bengin \n");

    int extra_rows = 0;
    int src_w = 640;
    int src_h = 480;
    uint32_t dst_w = 1280;
    uint32_t dst_h = 720;
    uint32_t x_step = 0.5f * (1 << JLSCALER_FRACTION_WIDTH);//设置缩放比
    uint32_t y_step = 0.667f * (1 << JLSCALER_FRACTION_WIDTH);
    int32_t x_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);//设置偏移量
    int32_t y_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);
    int channel = 1;//使用通道1

    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel * 3 / 2); //一帧yuv420pbuf
    if (!src_img_adr) {
        printf("Failed to load src image.\n");
        return 1;
    }
    FILE *fp = fopen(CONFIG_STORAGE_PATH"/C/yuv420.bin", "rb");//格式yuv420p:640*320 大小:640*320*3/2
    if (fp == NULL) {
        printf("file is not exist\n");
        return 1;
    }
    fread(src_img_adr, 1, src_w * src_h * channel * 3 / 2, fp);
    fclose(fp);

    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel * 3 / 2);
    if (!dst_img_adr) {
        return -1;
    }

    memset(dst_img_adr, 0, dst_w * dst_h * channel * 3 / 2);//buf清0


    scale_init();
    printf("scale_init");

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };
    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = JLSCALER_INTERP_BILINEAR;//插值算法切换

    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;

    int offset = 0;
    int h_num = 16;
    int count = 0;
    int32_t y_offset_line = 0;

    DcuFlushRegion(src_img_adr, src_w * src_h * channel * 3 / 2);
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel * 3 / 2);

    u32 start_time = timer_get_ms(); //获取起始滴答秒
    for (int k = 0; k < 3; k++) {

        switch (k) {
        case 0:
            //处理y
            offset = 0;
            y_offset_line = 0;
            count = dst_h / h_num;
            extra_rows = dst_h % h_num;
            if (extra_rows != 0) {
                count += 1;
            }

            for (int k = 0; k < count; k++) {
                if (k == count - 1 && extra_rows) {
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, extra_rows, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }

            }
            printf("\n--------process y done ---------\n");

            break;
        case 1:
            //处理u
            src_offset = src_h * src_w * channel;
            dst_offset = dst_h * dst_w * channel;

            offset = 0;
            count = (dst_h / 2) / h_num;
            extra_rows = (dst_h / 2) % h_num;
            if (extra_rows != 0) {
                count += 1;
            }

            for (int k = 0; k < count; k++) {
                if (k == count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }
            printf("\n--------process u done ---------\n");
            break;
        case 2:
            //处理v
            src_offset = src_h * src_w * 5 / 4 * channel;
            dst_offset = dst_h * dst_w * 5 / 4 * channel;
            offset = 0;
            count = (dst_h / 2) / h_num;
            if (extra_rows != 0) {
                count += 1;
            }
            for (int k = 0; k < count; k++) {
                if (k == count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset / 2;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h / 2, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);

                    scale_run();
                    scale_wait_done();
                }
            }
            printf("\n--------process v done ---------\n");
            break;
        default:
            break;
        }
    }

    u32 end_time = timer_get_ms();
    printf("\n--------------------------run_time = %d ms\n", end_time - start_time); //打印获取的代码运行时间(精确到ms)

    FILE *out_fp = NULL;
    out_fp = fopen(CONFIG_STORAGE_PATH"/C/outyuv0.bin", "wb");

    if (out_fp == NULL) {
        printf("out file is not exist\n");
        return 1;
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel * 3 / 2, out_fp); //(首地址,被写入每个字节大小,元素个数，流)
    fclose(out_fp);
    printf("save done");

    free(src_img_adr);
    free(dst_img_adr);
    scale_free();
    scale_doublebuf_free();

    printf("free done");
}


int yuv422p_scale_demo()//yuv422p处理例子
{
    printf("\n yuv422p scale bengin \n");

    int extra_rows = 0;
    int src_w = 640;
    int src_h = 480;
    uint32_t dst_w = 1280;
    uint32_t dst_h = 720;
    uint32_t x_step = 0.25f * (1 << JLSCALER_FRACTION_WIDTH);//设置缩放比
    uint32_t y_step = 0.333f * (1 << JLSCALER_FRACTION_WIDTH);
    int32_t x_offset = 320.0f * (1 << JLSCALER_FRACTION_WIDTH);//设置偏移量
    int32_t y_offset = 240.0f * (1 << JLSCALER_FRACTION_WIDTH);
    int channel = 1;//使用通道1

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };
    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = JLSCALER_INTERP_BILINEAR;//插值算法切换

    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel * 2); //一帧yuv422pbuf
    if (!src_img_adr) {
        printf("Failed to load src image.\n");
        return 1;
    }
    FILE *fp = fopen(CONFIG_STORAGE_PATH"/C/yuv422.bin", "rb");//格式yuv422p:640*320 大小:640*320*2
    if (fp == NULL) {
        printf("file is not exist\n");
        return 1;
    }
    fread(src_img_adr, 1, src_w * src_h * channel * 2, fp);
    fclose(fp);

    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel * 2);
    if (!dst_img_adr) {
        printf("dst_img_adr malloc error\n");
        return 1;
    }

    memset(dst_img_adr, 0, dst_w * dst_h * channel * 2);//buf清0

    scale_init();//寄存器清零
    printf("scale_init");

    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;
    int offset = 0;
    int h_num = 16;
    int count = 0;
    int32_t y_offset_line = 0;

    DcuFlushRegion(src_img_adr, src_w * src_h * channel * 2);
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel * 2);

    u32 start_time = timer_get_ms(); //获取起始滴答秒
    for (int k = 0; k < 3; k++) {

        switch (k) {
        case 0:
            //处理y
            offset = 0;
            y_offset_line = 0;
            count = dst_h / h_num;
            extra_rows = dst_h % h_num;
            if (extra_rows) {
                count += 1;
            }

            for (int k = 0; k < count; k++) {
                if (k != count - 1 && extra_rows != 0) {
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, extra_rows, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * dst_w * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                                    dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }
            printf("\n--------process y done ---------\n");
            break;
        case 1:
            //处理u
            src_offset = src_h * src_w * channel;
            dst_offset = dst_h * dst_w * channel;
            offset = 0;

            for (int k = 0; k < count; k++) {
                if (k != count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }

            printf("\n--------process u done ---------\n");
            break;
        case 2:
            //处理v
            src_offset = src_h * src_w * 3 / 2 * channel;
            dst_offset = dst_h * dst_w * 3 / 2 * channel;
            offset = 0;

            for (int k = 0; k < count; k++) {
                if (k != count - 1 && extra_rows != 0) {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, extra_rows, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                } else {
                    offset = k * h_num * (dst_w / 2) * channel;
                    y_offset_line = k * h_num * y_step + y_offset;
                    set_scale_param(src_img_adr + src_offset, src_w / 2, src_h, (src_w / 2) * channel,
                                    dst_img_adr + dst_offset + offset, dst_w / 2, h_num, (dst_w / 2) * channel, hw_channel, x_step, y_step, x_offset / 2, y_offset_line, interp);
                    scale_run();
                    scale_wait_done();
                }
            }
            printf("\n--------process v done ---------\n");
            break;
        default:
            break;
        }
    }
    u32 end_time = timer_get_ms();   //获取结束滴答秒
    printf("\n--------------------------run_time = %d ms\n", end_time - start_time); //打印获取的代码运行时间(精确到ms)

    FILE *out_fp = NULL;
    out_fp = fopen(CONFIG_STORAGE_PATH"/C/outyuv2.bin", "wb");
    if (out_fp == NULL) {
        printf("out file is not exist\n");
        return 1;
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel * 2, out_fp); //(首地址,被写入每个字节大小,元素个数，流)
    fclose(out_fp);
    printf("save done");

    free(src_img_adr);
    free(dst_img_adr);
    scale_free();
    printf("free done");
}


int RGB_scale_demo()//RGB24,RGB32格式scalel例子
{
    enum SCARE_DATA_TYPE rgb_format = RGB;//ARGB

    int channel = 3;
    printf("\n RGB scale bengin \n");
    int src_w = 640;
    int src_h = 480;

    uint32_t dst_w = 1280;//1280
    uint32_t dst_h = 720;//720

    uint32_t x_step = 0.5f * (1 << JLSCALER_FRACTION_WIDTH);//缩放比
    uint32_t y_step = 0.667f * (1 << JLSCALER_FRACTION_WIDTH);

    int32_t x_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);//偏移量
    int32_t y_offset = 0.0f * (1 << JLSCALER_FRACTION_WIDTH);

    if (rgb_format == RGB) {
        channel = 3;
    } else {
        channel = 4;
    }

    static uint8_t hw_channel_map[5] = {
        0, 0, 0, 1, 2
    };

    uint32_t hw_channel = hw_channel_map[channel];
    uint32_t interp = JLSCALER_INTERP_BILINEAR;//插值算法切换

    uint8_t *src_img_adr = (uint8_t *)malloc(src_w * src_h * channel);
    uint8_t *dst_img_adr = (uint8_t *)malloc(dst_w * dst_h * channel);
    if (!src_img_adr) {
        printf("Failed to malloc src_img_adr.\n");
        return 1;
    }


    if (!dst_img_adr) {
        printf("Failed to malloc dst_img_adr.\n");
        return 1;
    }

    memset(dst_img_adr, 0, dst_w * dst_h * channel);

    FILE *fp = NULL;

#if 1  //读卡

    if (rgb_format == RGB) {
        fp = fopen(CONFIG_STORAGE_PATH"/C/2_4803.bin", "rb");//RGB格式文件
    } else {
        fp = fopen(CONFIG_STORAGE_PATH"/C/2_4804.bin", "rb");//ARGB格式文件
    }

    if (fp == NULL) {
        printf("file is not exist\n");
        return 1;
    }
    fread(src_img_adr, 1, src_w * src_h * channel, fp);
    fclose(fp);
#else  //直接写数据

    memset(src_img_adr, 255, src_w * src_h * channel);
    //memset(src_img_adr,0,src_w*src_h*channel);
#endif

    scale_init();//scale初始化
    printf("scale_init");

    DcuFlushRegion(src_img_adr, src_w * src_h * channel);
#if 1 //按帧处理

    printf("src_adr=%x,dst_adr=%x", src_img_adr, dst_img_adr);
    set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                    dst_img_adr, dst_w, dst_h, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);//填入算法参数
    printf("set_param finish");

    DcuFlushRegion(src_img_adr, src_w * src_h * channel);
    DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel);

    scale_run();//配置寄存器 kick start 即开始工作
    printf("scale_running\n");

    scale_wait_done();//等待pending响应,并且clean掉中断标志
    printf("scale wait done finish\n");//处理完成

#else  //按行或列处理

#if 1  //按行处理
    int32_t y_offset_line = 0;
    int offset = 0;
    int h_num = 18;
    int count = dst_h / h_num;
    for (int k = 0; k < count; k++) {
        offset = k * h_num * dst_w * channel;

        y_offset_line = k * h_num * y_step + y_offset;

        set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                        dst_img_adr + offset, dst_w, h_num, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset_line, interp);

        DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel); //

        scale_run();
        scale_wait_done();
    }
#else//按列处理

    int offset = 0;
    int w_num = 16;
    int count = dst_w / w_num;
    for (int k = 0; k < count; k++) {
        offset = k * w_num * channel;
        x_offset = k * w_num * x_step;
        set_scale_param(src_img_adr, src_w, src_h, src_w * channel,
                        dst_img_adr + offset, w_num, dst_h, dst_w * channel, hw_channel, x_step, y_step, x_offset, y_offset, interp);

        DcuFlushinvRegion(dst_img_adr, dst_w * dst_h * channel);
        scale_run();
        scale_wait_done();
    }
#endif

#endif

    FILE *out_fp = NULL;
    out_fp = fopen(CONFIG_STORAGE_PATH"/C/out.bin", "wb");
    if (out_fp == NULL) {
        printf("out file is not exist\n");
        return 1;
    }
    fwrite(dst_img_adr, 1, dst_w * dst_h * channel, out_fp);
    fclose(out_fp);
    printf("save done");

    free(src_img_adr);
    free(dst_img_adr);
    scale_free();
    printf("free done");
}

#endif

