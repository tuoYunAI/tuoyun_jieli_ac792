/******************************************************************************
* File:             jpg2avi.c
*
* Author:           jiabum
* Created:          05/11/23
* Description:      本文件实现jpg数据帧插入进AVI文件的功能
*                   暂不支持带音频数据的AVI文件
*****************************************************************************/

#include "jpg2avi.h"
#include "list.h"
#include "app_config.h"

static JL_AVI_HEAD_NO_AUD jl_avi_head_no_aud_tab = {
    .riff = JL_RIFF,
    .file_size = 0,			//文件总大小
    .file_type = JL_AVI_,
    .list_hdrl = JL_LIST,
    .hdrl_size = (sizeof(JL_AVI_HEAD_NO_AUD) - 20 - 12),
    .hdrl = JL_hdrl,

    //avih
    {
        .fcc = JL_avih,
        .cb = (sizeof(AVI_MAINHEADER) - 8),
        .dwMicroSecPerFrame = 0,// 视频帧间隔时间（以微妙为单位）
        .dwMaxBytesPerSec = 0,	// 这个AVI文件的最大数据率
        .dwPaddingGranularity = 0,
        .dwFlags = (AVIF_HASINDEX | TRUSTCKTYPE),//
        .dwTotalFrames = 0,		//总帧数
        .dwInitialFrames = 0,
        .dwStreams = 0x00000001,
        .dwSuggestedBufferSize = 0x000000,
        .dwWidth = 0,			//图像宽度
        .dwHeight = 0,			//图像高度
        .dwReserved[0] = 0,
        .dwReserved[1] = 0,
        .dwReserved[2] = 0,
        .dwReserved[3] = 0,
    },

    .list_strl_vid = JL_LIST,
    .strl_vidsize = (4 + sizeof(AVI_STREAMHEADER) + sizeof(BIT_MAPINFOHEADER)),
    .strl_vid = JL_strl,
    //vedio strh
    {
        .fcc = JL_strh,
        .cb = (sizeof(AVI_STREAMHEADER) - 8),
        .fccType = JL_vids,
        .fccHandler = JL_MJPG,//
        .dwFlags = 0,
        .wPriority = 0x0000,
        .wLanguage = 0x0000,
        .dwInitialFrames = 0,
        .dwScale = 0x00000001,
        .dwRate = 0,		//帧率
        .dwStart = 0,
        .dwLength = 0,		//总帧数
        .dwSuggestedBufferSize = 0x000000,//
        .dwQuality = 0xffffffff,//
        .dwSampleSize = 0,
        {
            .left = 0,
            .top = 0,
            .right = 0,		//图像宽度(16bit)
            .bottom = 0,	//图像高度(16bit)
        },
    },

    //vedio strf
    {
        .fcc = JL_strf,
        .strf_size = (sizeof(BIT_MAPINFOHEADER) - 8),
        .biSize = (sizeof(BIT_MAPINFOHEADER) - 8),
        .biWidth = 0,	//图像宽度
        .biHeight = 0,	//图像高度
        .biPlanes = 0x0001,//
        .biBitCount = 0x0018,
        .biCompression = JL_MJPG,
        .biSizeImage = 0,  //位图大小 = w*h*3
        .biXPelsPerMeter = 0,
        .biYPelsPerMeter = 0,
        .biClrUsed = 0,
        .biClrImportant = 0,
    },

    .fcc_head_junk = JL_JUNK,
    .head_junk_len = HEAD_JUNK_SIZE_NO_AUD,
    .list = JL_LIST,
    .len = 0,
    .movi = JL_movi,

};

struct ListNode {
    int offset;
    int value;
    struct list_head head;
};

/*******************************************************************/
static u32 movi_addr = 0;
static u32 idx1_addr = 0;
static u32 idx1_len = 0;
static u32 insert_frames = 0;
static u32 insert_frames_size = 0;
static int fill_junk_size = 0;
static JL_AVI_HEAD_NO_AUD *head_tab = NULL;
static u8 *idx1_buf = NULL;
static struct list_head idx1_list; //索引块链表
int avi_dec_file_start(FILE *fp)
{
    u8 *tmp;
    int rlen = 0;
    if (!fp) {
        printf("avi dec file start err fp null !\n");
        return -1;
    }
    if (!head_tab) {
        head_tab = (JL_AVI_HEAD_NO_AUD *)malloc(sizeof(JL_AVI_HEAD_NO_AUD) + 512);
    }
    fseek(fp, 0, SEEK_SET);
    rlen = fread(head_tab, 1, sizeof(JL_AVI_HEAD_NO_AUD) + 512, fp);
    if (rlen != sizeof(JL_AVI_HEAD_NO_AUD) + 512) {
        printf("read err rlen=%d\n", rlen);
        return 0;
    }
    /* rlen = sizeof(JL_AVI_HEAD_NO_AUD); */

    /* 定位movi块的位置 */
    tmp = (u8 *)head_tab;
    while (rlen) {
        if (tmp[rlen - 1] == 0x69 && tmp[rlen - 2] == 0x76 && tmp[rlen - 3] == 0x6F && tmp[rlen - 4] == 0x6D) {
            rlen -= 4;
            break;
        }
        rlen -= 4;
    }

    u32 movi_len = 0;
    movi_addr = rlen;
    memcpy(&movi_len, &tmp[movi_addr - 4], 4);
    idx1_addr = movi_addr + movi_len/*head_tab->len*/;
    printf("movi len=%d\n", movi_len/*head_tab->len*/);
    printf("movi addr=%x\n", movi_addr);
    printf("idx1 addr=%x\n", idx1_addr);

    /* 定位idx1块的位置 */
    fseek(fp, idx1_addr + 4, SEEK_SET);

    /* 读出idx1块的大小 */
    fread(&idx1_len, 1, 4, fp);
    printf("idx1 len=%d\n", idx1_len);
    idx1_buf = malloc(idx1_len);

    /* 读出idx1块的数据保存 */
    fseek(fp, idx1_addr + 8, SEEK_SET);
    fread(idx1_buf, 1, idx1_len, fp);

    {
        //找到最后一个数据帧结尾
        u8 last_idx1_junk[16];
        u32 last_idx1_offset = 0;
        u32 last_idx1_len = 0;
        memcpy(last_idx1_junk, &idx1_buf[idx1_len - 16], sizeof(last_idx1_junk));
        memcpy(&last_idx1_offset, &last_idx1_junk[8], sizeof(u32));
        memcpy(&last_idx1_len, &last_idx1_junk[12], sizeof(u32));
        fill_junk_size = idx1_addr - (movi_addr + last_idx1_offset + last_idx1_len + 8);
        idx1_addr = movi_addr + last_idx1_offset + last_idx1_len + 8;
        printf("fill_junk_size=0x%x\n", fill_junk_size);
        printf("last_idx1_offset=0x%x\n", last_idx1_offset);
        printf("last_idx1_len=0x%x\n", last_idx1_len);
        printf("idx1_addrr=0x%x\n", idx1_addr);
    }

    INIT_LIST_HEAD(&idx1_list);

    return 0;
}
void avi_set_to_movi_tail(FILE *fp)
{
    if (!fp) {
        printf("fp null !1\n");
        return ;
    }
    /* 定位到movi块末尾 */
    fseek(fp, idx1_addr, SEEK_SET);
}
void jpg2avi_add_frame_to_movi(FILE *fp, void *data, unsigned int len)
{
    unsigned char tmp[4] = {'0', '0', 'd', 'c'};  //00dc = 压缩的视频数据
    u32 real_offset = 0;
    if (!fp) {
        printf("fp null !\n");
        return;
    }
    struct ListNode *node = (struct ListNode *)malloc(sizeof(struct ListNode));

    fwrite(tmp, 1, 4, fp);    //写入是否是压缩的视频数据信息
    fwrite(&len, 1, 4, fp);   //写入4字节对齐后的JPEG图像大小
    real_offset = ftell(fp);
    fwrite(data, 1, len, fp); //写入真正的JPEG数据

    insert_frames += 1;
    insert_frames_size += len;

    if (node != NULL) {
        node->offset = real_offset - movi_addr - 8; //转出movi相对偏移
        node->value = len;
        list_add_tail(&node->head, &idx1_list);
    }
}


static void flush_index_chunk(FILE *fp)
{
    unsigned char index[4] = {'i', 'd', 'x', '1'};  //索引块ID
    u32 old_idx1_len = idx1_len;
    idx1_len += 16 * insert_frames;   //索引块大小
    struct list_head *p = NULL;
    struct list_head *n = NULL;
    if (!fp) {
        return ;
    }

    fwrite(index, 1, 4, fp);
    fwrite(&idx1_len, 1, 4, fp);


    fwrite(idx1_buf, 1, old_idx1_len, fp);
    list_for_each_safe(p, n, &idx1_list) {
        unsigned char tmp[4] = {'0', '0', 'd', 'c'};  //00dc = 压缩的视频数据
        unsigned int keyframe = 0x10;                 //0x10表示当前帧为关键帧
        struct ListNode *node = list_entry(p, struct ListNode, head);

        fwrite(tmp, 1, 4, fp);
        fwrite(&keyframe, 1, 4, fp);
        fwrite(&node->offset, 1, 4, fp);
        fwrite(&node->value, 1, 4, fp);

        list_del(p);
        free(node);
    }
}
static void back_fill_head_data(FILE *fp)
{
    JL_AVI_HEAD_NO_AUD *avi_head_no_aud;

    if (!fp) {
        return ;
    }

    int cur_addr = ftell(fp);

    avi_head_no_aud = head_tab;

    if (!avi_head_no_aud) {
        return ;
    }

    /* avi_head_no_aud->file_size = 4 + sizeof(AVI_HDRL_LIST) + sizeof(AVI_LIST_HEAD) + nframes * 8 + totalsize + index_chunk_size + 8; */

    avi_head_no_aud->file_size = cur_addr - 8;			//文件总大小
    avi_head_no_aud->avih.dwTotalFrames += insert_frames;	//总帧数

    avi_head_no_aud->vid_strh.dwLength += insert_frames;		//总帧数

    avi_head_no_aud->len += insert_frames * 8 + insert_frames_size - fill_junk_size;

    //定位到文件头，回填avi头部
    fseek(fp, 0, SEEK_SET);
    fwrite(avi_head_no_aud, 1, sizeof(JL_AVI_HEAD_NO_AUD), fp);

}
int avi_dec_file_stop(FILE *fp)
{

    flush_index_chunk(fp);

    back_fill_head_data(fp);

    if (head_tab) {
        free(head_tab);
        head_tab = NULL;
    }
    if (idx1_buf) {
        free(idx1_buf);
        idx1_buf = NULL;
    }
    insert_frames = 0;
    insert_frames_size = 0;
    movi_addr = 0;
    idx1_addr = 0;
    idx1_len = 0;

    return 0;
}

/* Demo */
int jpg_frames_insert_to_avi_demo(void)
{
    FILE *fp_jpg;
    FILE *fp_avi;
    char filename[20];
    int filesize;

    u8 *jpg_data = NULL;

    fp_avi = fopen(CONFIG_REC_PATH_0"input.avi", "r+"); /* 打开待修改的AVI */

    if (!fp_avi) {
        printf("fopen input avi err!\n");
        return -1;
    }

    avi_dec_file_start(fp_avi); /* 开始解析avi文件信息 */

    avi_set_to_movi_tail(fp_avi); /* 偏移到movi地址，准备追加jpg帧 */

    /* 测试读取卡里面的5张jpg 追加到AVI文件 */
    for (int i = 0; i < 4; i++) {

        memset(filename, 0, 10);

        sprintf(filename, CONFIG_REC_PATH_0"data/%d.jpg", i + 1);
        fp_jpg = fopen(filename, "r");

        if (fp_jpg != NULL) {

            filesize = flen(fp_jpg); /*获取JPEG数据大小*/

            jpg_data = malloc(filesize);

            fread(jpg_data, 1, filesize, fp_jpg); /*将JPEG数据读到缓冲区*/

            jpg2avi_add_frame_to_movi(fp_avi, jpg_data, filesize); /*将JPEG数据追加写入AVI文件*/

            fclose(fp_jpg);

            if (jpg_data) {
                free(jpg_data);
                jpg_data = NULL;
            }
        }

    }
    if (fp_avi) {
        avi_dec_file_stop(fp_avi); /* avi文件追加结束 */
        fclose(fp_avi);
    }
    puts("insert jpg end!!!\n");

    return 0;
}
