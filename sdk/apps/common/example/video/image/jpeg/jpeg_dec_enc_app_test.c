
#include "asm/jpeg_codec.h"
#include "asm/hwi.h"
#include "asm/debug.h"
#include "system/includes.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "video_ioctl.h"
#include "video.h"
#include "yuv_to_rgb.h"
#include "app_config.h"

#if 1 //新的文件系统
#define ffread(b,s,l,f) 	fread(b,s,l,f)
#define ffwrite(b,s,l,f) 	fwrite(b,s,l,f)
#else //旧的文件系统
#define ffread(b,s,l,f) 	fread(f,b,l)
#define ffwrite(b,s,l,f) 	fwrite(f,b,l)
#endif
/*********************************************本文件使用与FPGA和回片验证***********************************************/

#define JPEG_DEC_MUC_TEST		1 //1分行MCU测试
#define JPEG_DEC_DRI_FILE_ALSO	1 //1同时也解码DRI图片
#define JPEG_DEC_PLANAR         1  //行分布
#define JPEG_DEC_PACKED_LINEBASE       0 //
#define JPEG_DEC_PACKED_TILEBASE       0

#define JPEG_DEC_DRI_FILE_ONLY	0 //1仅仅解码DRI图片
#define JPEG_DEC_DEV_LIMIT		0 //1使用debug模块框柱YUV区域
#define JPEG_DEC_YUV_UNCACH		0 //1YUV解码地址为不经过cach(0x8000000起始)
#define JPEG_DEC_INTERRUPT		1 //0解码查询法，1解码使用中断法

#define JPEG_DEC_BITBUF_SRAM	0 //1解码的位流放在sram
#define JPEG_DEC_START_NUM 		0 //起始分辨率:0-128 1-160 2-320 3-480 4-640 5-960 6-1024 7-1920
#define JPEG_DEC_END_NUM 		7 //结束分辨率:0-128 1-160 2-320 3-480 4-640 5-960 6-1024 7-1920

#define JPEG_YUV_BIT_ADDR_ONE	0 //yuv和位流地址连续在一个buf

#define CYC_FILE_DECODE			0 //1单一分辨率循环解码
#define CYC_FILE_NUM			0 //单一分辨率:0-128 1-160 2-320 3-480 4-640 5-960 6-1024 7-1920
#define CYC_FILE_BUF_SRAM_EN	0 //1单分辨率解码地址sdram/sram控制

#define FILE_DECODE_NUM			0xFFFFFFFF //解码循环次数
#define FILE_CHECK_SUM			0 //1使用文件自带和校验的jpg图片
#define SAVE_FILE_YUVCRC32_ERR  1 //1保存解码CRC32校验失败的YUV成文件

#if SAVE_FILE_YUVCRC32_ERR
#define SAVE_ONE_FILE_YUV_ERR   0 //1保存同分辨率下的失败的YUV成1个文件
#endif

#define SAVE_FILE_YUV_OR_RGB    0 //1保存YUV或者RGB查看文件
#define SAVE_FILE_CRC32			0 //1保存CRC32到文件

#if SAVE_FILE_CRC32
#undef FILE_DECODE_NUM
#define FILE_DECODE_NUM 1
#endif

#if SAVE_FILE_YUVCRC32_ERR
#define SAVE_FILE_YUV		    1 //保存查看文件类型：0 RGB ， 1 YUV
#endif

#define JPEG_DEC_OK_SAVE_FEILE	//"640_29.jpg"
#define CHECK_MAG				0x0

#define FILE_ENCODE_NUM			0xFFFFFFFF //编码循环次数
#define FILE_ENCODE_ONLY_YUV640 0 //1:yuv-vga编码

/**********************************************所有分辨率解码测试*******************************************************/
int thread_fork(const char *thread_name, int prio, int stk_size, u32 q_size, int *pid, void (*func)(void *), void *parm);
static const u8 jpg_dri_file[][32] = {
    "320_240_sum/320_16.jpg",
    "320_240_sum/320_34.jpg",
    "320_240_sum/320_96.jpg",
    "END320",
    "480_320_sum/480_15.jpg",
    "480_320_sum/480_17.jpg",
    "480_320_sum/480_55.jpg",
    "480_320_sum/480_87.jpg",
    "480_320_sum/480_106.jpg",
    "END480",
    "640_480_sum/640_15.jpg",
    "640_480_sum/640_61.jpg",
    "END640",
    "960_640_sum/960_24.jpg",
    "960_640_sum/960_44.jpg",
    "960_640_sum/960_68.jpg",
    "960_640_sum/960_76.jpg",
    "960_640_sum/960_98.jpg",
    "END960",
    "1024_720_sum/1024_66.jpg",
    0,
};

#if JPEG_DEC_PLANAR
static const u32 jpg_128file_crc32[] = {
    0x00000000, 0xe3f8faa0, 0x881515d7, 0x881515d7, 0xa7b22b1c, 0xa91998ef, 0x63f83fcf, 0x623d420b, 0x9db6ea33, 0x336cedca,
    0xd73bace6, 0x178e85f4, 0xa66cdcf1, 0x07515d8a, 0x62e0e315, 0x4437429a, 0x31ef55de, 0x8006e265, 0x1da9997e, 0xed26e133,
    0xf6df2e73, 0x4b404167, 0x7d42bfae, 0xf5c49936, 0xbf8e8f1e, 0xc4700556, 0xbbd31b31, 0x130747d0, 0x0f3f3a6e, 0xf63dc21b,
    0x11847068, 0x4198d6d4, 0x872d9ebf, 0xb4a90712, 0xa4c19405, 0x6af85356, 0xb588596a, 0x7425fd30, 0x7db561fc, 0x4cc6b91e,
    0x3888bce8, 0xe390d74c, 0xdc387eec, 0x4f05432c, 0x9b6ab434, 0x8030665f, 0xcbc22de4, 0x70a44e08, 0x9dd059a5, 0xf4fbba56,
    0xf9539221, 0xefc9d480, 0x5136edb7, 0x45e11406, 0xd8ac678a, 0x8253986f, 0x3d8145d0, 0xaad53aba, 0xaa4bf5f2, 0xf461e2ae,
    0x2823eca5, 0x6cf87e97, 0xc8bad000, 0x54652674, 0x7708b8e7, 0x375bfe09, 0x4367500c, 0x89eae229, 0x5a5fc7c9, 0x4fa6e01b,
    0x13d48fc0, 0xf8ebf875, 0xfa7ad599, 0x3c707b80, 0x7a9603b4, 0x799b4122, 0xf30f4618, 0x88a69104, 0xfe7d8d03, 0x8b4efd35,
    0xccbbd744, 0x71b99d24, 0x122b6161, 0x89548ed4, 0x19a8c5a9, 0x2e8d2efd, 0xa3c4ad36, 0xedb673eb, 0x9ad3adb6, 0x9ec088f6,
    0x22f7e123, 0x731d1a36, 0x92490cbd, 0x8b9711d3, 0x727816d4, 0xfcf9593c, 0x1a5d7268, 0xc4b6ab21, 0x710619a8, 0x6722e468,
    0x655119c6, 0xa62a5932, 0xf1803bb2, 0xc4af5526, 0xa4498062, 0x2ee245af, 0xbaa2c3f6, 0xe628e417, 0xb2f76fe8, 0xb869d4c1,
    0x43f58062, 0x2c17e14e, 0x65adbeb6, 0x1da87acf, 0x48e02305, 0x709d9706, 0xca12e146, 0x107e20e7, 0x85e10a9b, 0x78095c33,
    0xdbbe0832, 0x0410746c, 0xcb518043, 0xce1f3bde, 0xe591f8f7, 0xe33fe921, 0x78e7b3f7, 0x0edee60a, 0xa6c33690, 0x4961655f,
    0x369abf0e, 0xc5aed778, 0xe2fe0253, 0xbe02acfb, 0x629c3068, 0x4fdc24dc, 0xf129ce19, 0x65a351b5, 0xde63195e, 0xbe73ad2d,
    0x1b1b4fdb, 0x94c76519, 0x07f99859, 0xde37b2a5, 0xfffdd154, 0x761c5789, 0x733d77e1, 0x4f271e33, 0x4f271e33, 0x7b4201c1,
    0x2f9f137e, 0xb55312ad, 0x9da0436b, 0x60edac32, 0xddeb4027, 0x10abdc9b, 0x10abdc9b, 0x10abdc9b, 0x0b96e742, 0x0b96e742,
    0x05a8a9f0, 0x05a8a9f0, 0x93f7216e, 0x93f7216e, 0xe87b0f59, 0x84f6819b, 0x885d1cbe, 0x885d1cbe, 0x885d1cbe, 0x885d1cbe,
    0x885d1cbe, 0x885d1cbe, 0x885d1cbe, 0x885d1cbe, 0xb31b7de6, 0xc9aa7ace, 0x832b1c87, 0x1dfb4bb4, 0xdb1e3f5f, 0xf817ab8c,
    0xe3ad7b78, 0x23def50a, 0x32729635, 0x5521bb94, 0xe86e73ad, 0x7a2813e7, 0x466b91d8, 0xb0d9a9d9, 0xd1be20ad, 0x07bdc420,
    0x3603234d, 0x70844054, 0xc8cdd491, 0x4e4cdaef, 0xc5516fc2, 0x61134b0f, 0x8b64f7d2, 0x00cf9720, 0x2df666a3, 0x8aed2b09,
    0xc4eb428e,

    0x00000000,

};
static const u32 jpg_160file_crc32[] = {
    0x00000000, 0x10f08c99, 0xc6275f4f, 0x8a7f9a26, 0x5df94cb8, 0x72ec1b8a, 0x4a0785cb, 0x310546d7, 0xfa8c2526, 0xdceef07d,
    0x19bf6c0e, 0x1e1f3f11, 0x52aed1d8, 0x1e902ffe, 0x2a8096f6, 0x97f65e3d, 0xc1f44879, 0x6d50992c, 0xacc748eb, 0x7ded95b1,
    0xbf6e1601, 0x46bfc1ae, 0xbc5e5dfa, 0x1b7ad07e, 0x5e76aa47,
    0x00000000,
};
static const u32 jpg_320file_crc32[] = {
    0x00000000, 0x00000000, 0xda258aac, 0x4f5c3034, 0xf21d0240, 0x63f0eff0, 0x43a62e2a, 0xbbb7c5e6, 0x643e0368, 0x51db285c,
    0xcf88d86e, 0x9d7b6ae4, 0x09725353, 0x2435534b, 0x55335304, 0x24076a21, 0x15a8c399, 0xdb3e00c6, 0x04c72ad7, 0xcaf48689,
    0x84247b56, 0xaac0420b, 0xb20e346f, 0xa07b7458, 0x668a71f1, 0x08a4d301, 0xc3c9262c, 0x2d47444a, 0x157933f8, 0xef34179a,
    0x91bdf1fc, 0xa5ddc184, 0xe8b2a67f, 0x89e3edc8, 0xd06ca495, 0x059edb37, 0x65373112, 0xf88da2a9, 0x5a250a90, 0xb4c1029c,
    0x3c8ee7a6, 0xce396fa8, 0xf7fd4350, 0x50c01382, 0x84e01cde, 0x00000000, 0x2b9ab9bb, 0x617dbc18, 0x5f99177d, 0x5b3c050c,
    0xf92d0c70, 0x6f129a02, 0xce07d1fe, 0x00000000, 0x5d431224, 0x88f1f5ac, 0x1a5b694e, 0x03f25dd8, 0xfc9f98b4, 0x8196833b,
    0x24aada2f, 0x0724db48, 0x081acf1c, 0xd953285a, 0x477f6699, 0xf91b4151, 0x86f82640, 0x403db92b, 0x7e2855eb, 0x482639b2,
    0x1223288d, 0x1433a745, 0x036c93ac, 0x6718c9f9, 0x540d2594, 0xb531bc6a, 0x55e9fba7, 0x115c65f7, 0x85726a95, 0x0bd7a915,
    0x60246fad, 0x3a679c52, 0x658ae7ab, 0x41e29287, 0x6cfcd17e, 0x64fa496e, 0x0ba31912, 0xd5134ccc, 0xb78f89ab, 0x0772d43e,
    0xdabc3b66, 0x2f6ebd7b, 0x338213a0, 0x3906bed6, 0x8a36dc17, 0x95f886b9, 0xa62a2bd8, 0x952379f2, 0x7214e34e, 0x0053827a,
    0x4bee2b4f, 0xa940e944, 0xb9584efa, 0x2c181f20,


    0x00000000,

};
static const u32 jpg_480file_crc32[] = {
    0x00000000, 0x95e3922b, 0xa7c9aff0, 0xdfd4d79b, 0x6834af76, 0x2364db55, 0x2747b625, 0x85915817, 0xca7411f7, 0x83771cc9,
    0x113163fe, 0x67241221, 0x043beb90, 0xeec459bd, 0x00000000, 0xf4d72164, 0x52472871, 0x5acdf858, 0xa8458564, 0xac783b6c,
    0xd1c9c5e4, 0x9d2cc3f3, 0xcb759411, 0x96dba8f5, 0x14261329, 0x39c088a2, 0xd16908ad, 0x101ae130, 0x00000000, 0x79ccb90e,
    0xbfa7a42c, 0x33a30473, 0x837bb539, 0x4c0e7085, 0x899b886c, 0xc094d62b, 0x5ac0640f, 0xc4119573, 0x458aed59, 0x56abdfe3,
    0x00000000, 0xcb4086c6, 0x5f25e550, 0xe5c9b142, 0x25b4daac, 0x27722dcc, 0xc8dc9737, 0x04928419, 0xfc8fcdc7, 0xf8eaad23,
    0xae084ff3, 0x642c57e9, 0x3cdd73d1, 0xe2b5a7a1, 0xfd80dbba, 0xd1d2f81d, 0x4b8ef4f2, 0x4bf8f5d5, 0x92e85f46, 0x3f4df089,
    0x3cc7d2d2, 0xe2dc75d2, 0x9009b0d4, 0x4d07419d, 0x5b9fbd82, 0x4c6181de, 0x359f7c1a, 0xed6ea81e, 0xff6041d4, 0xd7a04b35,
    0xb5353249, 0x1e357a88, 0xf5bb7e88, 0x86345073, 0x41494732, 0x93ac74ad, 0x2bc95279, 0x19a5c8c0, 0x3c80296f, 0xdc7e70c8,
    0xa556edb1, 0x710df1db, 0xe5c4a7ae, 0xe5c21e71, 0x22c5a82d, 0x69663d5c, 0x60cbbeaa, 0xcbebedc2, 0x34159a13, 0xc2efbb65,
    0x1218c6e4, 0xba90e96a, 0xb94e75d2, 0x90f116db, 0xd1f0afa3, 0xa13f7836, 0xc094d62b, 0x7b3f8d81, 0x64bbb345, 0x9b0b513c,
    0x5747f46c, 0xabfea354, 0x00000000, 0xd67cb2b8, 0x0e94ac72, 0x8c889a84, 0x0a186f67, 0x2f28c585, 0x00000000, 0x00000000,
    0x56c0d512, 0x50927504, 0x00000000, 0x00000000, 0xd815aa69, 0x00317d94, 0x6abefa4b,
    0x00000000,

};
static const u32 jpg_640file_crc32[] = {
    0x00000000, 0x84a3d393, 0xdff12ca9, 0xee5b22f6, 0xe42d59b0, 0x32bcdff9, 0xcb3cc452, 0x7d1b0b88, 0xf8e42812, 0x764cfd15,
    0x2ded3970, 0xe3f92c89, 0x4ea50b59, 0xfeb03af0, 0xfc17c935, 0x0b6e6287, 0x6cadb28e, 0x99e8f55b, 0xa288276f, 0x71c6cd05,
    0xe224a102, 0x3c8062f3, 0x33ac9a8d, 0x1d4b777e, 0x030053ec, 0xd968de5f, 0xf21b7809, 0xba03a621, 0x541a7270, 0xa6c49f90,
    0x7af40b85, 0x8f7d2774, 0x6d45ecea, 0x7f89491b, 0x20b87dad, 0x1196c3e0, 0x729a9b30, 0x74d53cb1, 0x28664c0e, 0xca394a93,
    0x469a85ca, 0x9d481f05, 0x6bd13396, 0x552584a4, 0x041aaa8a, 0x80052f2c, 0x6c9a73e3, 0xcc82b2c2, 0x5ec40818, 0xd6286a87,
    0x1944cc60, 0x0030ebe2, 0x6ea0c13b, 0xc1b32c10, 0x00000000, 0x58c8e674, 0xf567fa41, 0x25a0ac3d, 0xa288276f, 0x71410011,
    0x8f7d2774, 0x6b100570, 0x5be78a1e, 0x3699941d, 0x6b40e1e0, 0xa331957a, 0x87d3e422, 0x490eb3ec, 0x881928e7, 0x3873b25f,
    0xf9c48151, 0x8670bbb4, 0x0a89e764, 0x9b036160, 0x22b6951c, 0x3a369203, 0x6c2b9f35, 0x2ae36ee4, 0xb6a64679, 0x180c1c63,
    0x98b538ab, 0x75162e3a, 0x7e9ff9a4, 0x20ed2feb, 0x997ce5e1, 0xbc02da9a, 0x57e3a619, 0xbb3eaad5, 0x2505b25f, 0x05240c26,
    0xac455f41, 0x7743eaca, 0x1258c084, 0x51c77240, 0xef2748f4, 0xdee902bb, 0xa680796d, 0x303cf908, 0xf4857c54, 0x9d1ebc31,
    0x00000000, 0x76534349, 0x423ffcd7, 0x4ea50b59, 0x908bb336, 0x2afa8cb0, 0xbf09e4a6, 0xc569fdb7, 0xdc348c44, 0xc2a9080e,
    0x00000000, 0x7b051dad, 0x2b22bd60, 0x299a5d4c, 0x4a0d6598,

    0x00000000,
};
static const u32 jpg_960file_crc32[] = {

    0x00000000, 0x48c550b0, 0x967feff2, 0xbf1c3abe, 0x521ecb66, 0x32460b62, 0x00000000, 0xada2d49a, 0x87483d36, 0x666c5b2a,
    0x65af0677, 0x9f691823, 0x8e3f8ca7, 0xb971ffa2, 0x00000000, 0x23d6b654, 0x00000000, 0x5bdf9a90, 0xd64da343, 0x9d576850,
    0x4ba6ef1d, 0x13e242fe, 0x191b3a14, 0x1f61f486, 0x8979a62f, 0x71c51e5f, 0x2d2403b4, 0xbb52f385, 0xca429d9f, 0x622c23ba,
    0x5a814c9c, 0x6cbed5c7, 0xc5a4126e, 0x00000000, 0x0da9b61b, 0xe32a8b70, 0xbc9e6a9e, 0x8bf0c9fa, 0x7c176b4f, 0xe3bf1d50,
    0x32a9463d, 0xa99c2545, 0x6c8077ef, 0x00000000, 0x34080aab, 0x2d73819e, 0xd96debe7, 0x00000000, 0xcad4501d, 0x2b49bb1b,
    0xf3eeb138, 0x00000000, 0x2b169aa2, 0xace333b2, 0x0dff1cc8, 0x2f2e1fd5, 0x25f47b0d, 0xa637008a, 0xbeb2a27b, 0x01feb2bf,
    0x818a9d23, 0x89d895b0, 0x9089630c, 0x570c921d, 0x60de6bef, 0xeaa9f4dc, 0x3595856a, 0x8bb646ab, 0x427b3db0, 0xdb05844f,
    0xd91c5d54, 0xd52c83d8, 0xf1c9e693, 0x230abb92, 0x5bd95efd, 0xa8d83a4a, 0xaf5b7be0, 0x35b85384, 0x88a4c7c4, 0x08082085,
    0x23eb5b13, 0xcc3ad175, 0xe3345f02, 0x47ee67f9, 0x72d15aed, 0x1cb5b18b, 0x657bdaa8, 0x7820bfa1, 0x4037c558, 0x6b667bef,
    0x8604833a, 0xa88bc97d, 0x76aa9ad4, 0xbfb0161c, 0x5939bc3d, 0x390e2afd, 0x7178d4ee, 0x6364d42c, 0x7b4a1e25, 0x2185ed5e,
    0x32eab3ae, 0xd4540b3f, 0x751973c0, 0x82e9a52a, 0x3d081883, 0x927dca0e, 0xf82ce599, 0xa1d9d169,

    0x00000000,
};
static const u32 jpg_1024file_crc32[] = {
    0x00000000, 0x564c2013, 0xce1dff06, 0xac483b03, 0x5af84472, 0x8388530c, 0x592f4fec, 0x5088d0a0, 0x70d3464b, 0x2d155138,
    0x93c39683, 0xc07bd747, 0x02c1b6f2, 0x0db5ed3c, 0x20049b52, 0xa0cb5f5d, 0x6401199a, 0xf3d43fec, 0xb2c280ce, 0x5aba9d4f,
    0xa71799f2, 0x7bc7baeb, 0xc9f9cfe7, 0xdb710a9d, 0xa857945b, 0xb1e293d7, 0x9ab0da56, 0x041e4ac9, 0x1deed718, 0xdec1ca39,
    0xacbacbd0, 0x5ae06863, 0x44e926bb, 0x9252284a, 0xb3b164f9, 0x41f5a410, 0xd040220e, 0x10f08e25, 0x47a4f162, 0xe9216040,
    0x00000000, 0xb9b0abb3, 0x0ec86a2f, 0x5aba9d4f, 0xdb710a9d, 0x21f9fb56, 0x25db7adb, 0xbe1628cb, 0xea99f9b4, 0xea5f8132,
    0x130e453e, 0xea5864b0, 0x31dbfcf2, 0x0aaf6cb1, 0x297f9e02, 0x25a9cc64, 0xeb39ab76, 0x69490f70, 0x74926b5a, 0xa7337d9e,
    0x140521db, 0x1af05502, 0x646229ed, 0x3ca16a27, 0x0eabd27f, 0x00000000, 0xf0aaa8da, 0x8a034974, 0x6e8a056f, 0xb5f87ebc,
    0x2ae13781, 0x561443d9, 0xfd343975, 0x70dfeb63, 0x17b6b660, 0xfc1683d7, 0xfd597997, 0x1209db63, 0x00000000, 0x50d0eb84,
    0xc2fc300d, 0x00000000, 0xe1584448, 0xd256547b, 0x16b1375d, 0x19f127ac, 0xad909ac5, 0x0f6e3d58, 0x43b417d5, 0x570c0f98,
    0x96475a48, 0x9b1db418, 0xfae71172,

    0x00000000,
};
static const u32 jpg_1920file_crc32[] = {
    0x00000000, 0x6b4077f2, 0x7ba8a00d, 0xa65361ba,
    0x00000000,
};
#endif


#if JPEG_DEC_PACKED_LINEBASE
static const u32 jpg_128file_crc32[] = {
    0x00000000, 0x9ad3a8b5, 0xde9c5173, 0xde9c5173, 0x04b7177c, 0x20fdf7fd, 0xd2011b2e, 0x10b58bbc, 0x4d867503, 0xa1335d55,
    0x1a38c2f4, 0xdaaada99, 0x2c9358f5, 0x39ac4a16, 0xcbd0dcaa, 0xfc855482, 0x040d5bae, 0x6be8bea6, 0xdaf820a1, 0xe2f37733,
    0x806f22da, 0x7afe3d23, 0xdf8877cd, 0xacb2f50c, 0x02fbdf97, 0x3b10f8c4, 0x01ff703a, 0xddbf897e, 0x5a21724f, 0x4b2027c1,
    0x2256d55b, 0x3fff1932, 0xd8460937, 0x209c9032, 0x571fd9d4, 0x6a915a7c, 0x87f56b14, 0x2b27c72f, 0x00b07851, 0xe84a0211,
    0xafbb5a3f, 0x312895cb, 0x84383d9d, 0xcad4d80a, 0x19adc67b, 0x9552ca1e, 0x746ba677, 0x52b08a3b, 0x722e8857, 0x92818684,
    0xc1901a69, 0xe48783b5, 0xe87d21e8, 0xe8c0fdaa, 0x31ebcc99, 0x74f5bcb9, 0x813b4f3a, 0xb30405a0, 0x444e95cc, 0xb6702f01,
    0xd1170e46, 0x81f70793, 0xb1b1f1c3, 0xd1f6ed56, 0x0afbdac9, 0x62fae350, 0x1ac59970, 0x577046d7, 0xd5394f67, 0xffeeefcd,
    0xb93d322a, 0x67b650d8, 0x7b02b58b, 0x482c131c, 0x22da977a, 0x1a31102a, 0x70a57563, 0xa3cb9984, 0x67cf0a2b, 0xc4258cdc,
    0xf96c38ba, 0x59448133, 0x83bbc4e5, 0xd88d137f, 0x3bb71776, 0xc3830918, 0x6044cdb2, 0x32d573e2, 0x4d35dab5, 0x24184788,
    0x3f8f4871, 0xdada35ad, 0x13b57c37, 0x6a092f76, 0x3b20aa58, 0x2cf7f151, 0xa7f30fd2, 0xfd983c2c, 0x993f9e75, 0x2437de5b,
    0x5f2686c5, 0x74f2a97b, 0x9f752b83, 0xa0ec80f0, 0x641197a0, 0x7231d272, 0x93182d6f, 0xc6b766c6, 0x45a75ce0, 0x2daaaad2,
    0x566e9ad8, 0x9500be45, 0x4437728a, 0x55a54501, 0x86809d6a, 0x6719877a, 0xf477a892, 0x70ee1d19, 0xe563c3ef, 0x5fefdec4,
    0x286f2c7a, 0x7c5b1b68, 0x85769a5b, 0x5d01564e, 0x50c93e33, 0x6ae10a09, 0x71f28c52, 0xefc51091, 0x8821ae59, 0x1efe5708,
    0x46014308, 0xc9eec351, 0xdaa5e412, 0xdc10621a, 0x17269afd, 0x187a5415, 0x13c743f6, 0x60006681, 0xaaa04c67, 0x736030da,
    0xe5d0dd2c, 0xee37e20e, 0x7b73048b, 0x65e01b68, 0xe90eb20f, 0x2ca9963c, 0xf2838f0e, 0xb087d2d5, 0xb087d2d5, 0x2a0e31fb,
    0x350b4460, 0xcd1356ac, 0x256bdbc4, 0x5d52671a, 0xd8a1467f, 0xf847fa41, 0xf847fa41, 0xf847fa41, 0x17d213d1, 0x17d213d1,
    0xb161bbef, 0xb161bbef, 0x0ec80165, 0x0ec80165, 0x1ee23beb, 0xb06a66a1, 0x2ae175f1, 0x2ae175f1, 0x2ae175f1, 0x2ae175f1,
    0x2ae175f1, 0x2ae175f1, 0x2ae175f1, 0x2ae175f1, 0x36a61b73, 0x5c718bc4, 0x524b2167, 0x57039451, 0xd9a0ebae, 0x733373c7,
    0x5ff4c37d, 0x3895f2e1, 0x1bdbccd5, 0x0428c308, 0xcd1f7f93, 0x48e7896c, 0x82bfe7ac, 0xf5f3eeff, 0x70554443, 0xaf5c0df5,
    0x5c7f3935, 0xd8128aed, 0x5a75bf44, 0x79a71014, 0x4a78cc15, 0x0d001bf3, 0x42247213, 0xc17e12e4, 0x1c1a15df, 0x093ba316,
    0x333a61b8,


    0x00000000,

};
static const u32 jpg_160file_crc32[] = {

    0x00000000, 0x6ea8b930, 0xd75bb0e2, 0x93618026, 0x0c730a17, 0xdce5d14b, 0x00000000, 0x012dea58, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x370eb560, 0xdff8e56f, 0xd398760b, 0x9ae88eb1, 0x957697f3, 0x843a0e69,
    0x5153a1b5, 0x25ed47ce, 0xf7ac53b7, 0x5774ccf2, 0xe8b9ebb7,

    0x00000000,
};
static const u32 jpg_320file_crc32[] = {

    0x00000000, 0x00000000, 0x56a2235f, 0x57122875, 0x9e4af921, 0x2d8a1dc2, 0x00000000, 0xaa8b0600, 0xca65188d, 0x7ccda441,
    0xfff6a3ba, 0x29c668ab, 0xb64bd9ae, 0x4dc072d9, 0x7278013d, 0x9a2554d9, 0x00000000, 0x634fed4d, 0x52cdabeb, 0xdaacb079,
    0x4d922519, 0xd0302d5d, 0x72ef4213, 0x00000000, 0x00000000, 0x7df4191b, 0x493a0018, 0x48e796fc, 0x955490c1, 0xc0f231f8,
    0x3270b1ca, 0xd2e26c44, 0x47c6bbbc, 0x806c7f73, 0x00000000, 0x9c386d75, 0x96528450, 0x743ff063, 0x107a174a, 0xb3e1269a,
    0x33596e53, 0x992481f0, 0x6a14ec70, 0xdbf83251, 0xd525d8b8, 0x00000000, 0xf2d7bc5d, 0xb11da01c, 0x411d8944, 0xc362b609,
    0x00000000, 0xe82139e3, 0x70b719ab, 0x00000000, 0x87a4da2c, 0xa3304506, 0xc4b6852b, 0x00000000, 0xd006819b, 0x077a3342,
    0xc7a655ce, 0xc27da6f9, 0x00000000, 0x00000000, 0xab48d754, 0x54bc2d99, 0x45816cb1, 0xeaa3c1d4, 0x083e9daa, 0x6b87b71b,
    0xa43f77ee, 0x815b09e7, 0x6247193d, 0xe0696853, 0x6875fdfb, 0xb6d8c218, 0x5cab7ede, 0xf927b418, 0x160ad3b8, 0x05440014,
    0x0a75de68, 0x00000000, 0xa2d08e95, 0xd010fa56, 0xeb68d87a, 0x2b8cd055, 0xcb306d79, 0x82a87e9c, 0x05595d37, 0x00000000,
    0x00000000, 0x197d7203, 0x8b88e5ed, 0x5f1fe0cf, 0x79ab7459, 0xbb13d936, 0x00000000, 0xa22b530b, 0x6e55613d, 0x262612fe,
    0x927630e1, 0x8d3ebe10, 0x7672a4e1, 0x28d2a248,

    0x00000000,

};
static const u32 jpg_480file_crc32[] = {

    0x00000000, 0xc99c0481, 0x31c26c99, 0x687604d9, 0xd04720c5, 0x0452f199, 0x35d70fdc, 0xcf35bc25, 0x3091d800, 0xd8dc3122,
    0xa0cb94d6, 0x3205ef1b, 0xb6ad3b5b, 0x6a4ce10f, 0x00000000, 0x00000000, 0x5a5f42b6, 0x00000000, 0xd1bd6e80, 0x781eb8a1,
    0x9219bbb7, 0xb017f10c, 0xd5a7abb8, 0x21b782d3, 0xa98dcf12, 0x0814b5b3, 0x7fdbaefc, 0x10e05411, 0x00000000, 0x2d63a93a,
    0x15debb64, 0x25472844, 0x149a3e6e, 0x3d69bd6e, 0xecf7a92a, 0xcc85bd82, 0xa7ec6076, 0x510b6d19, 0x66d52275, 0x9bf3b5cb,
    0x00000000, 0x78e74e2b, 0x00000000, 0x35b464e1, 0x66377887, 0x3cb3d9b8, 0x53eb8e52, 0x20187be1, 0x3d58da4d, 0x4ac51b6d,
    0x5ffcc624, 0x2b376e8b, 0xa3f0d290, 0xdff531d6, 0x8f3dea81, 0x00000000, 0x814713ff, 0xa1faed40, 0x7f38d281, 0xb15847ef,
    0xf077f23f, 0x74bc1cfa, 0xfe171b76, 0xe58e8cc9, 0x6ef2bbc1, 0xc3402836, 0x240ed074, 0x909e1f25, 0xc67ee176, 0x7cd824ce,
    0x8b16c681, 0xf85c5230, 0x30a13836, 0x27cea435, 0xa8127501, 0xb295dccc, 0x02706af2, 0xb9d51c76, 0xce5a72b1, 0xe6c85212,
    0xc9da6789, 0xc6a42cb2, 0x126ac498, 0xad212a1b, 0xf490a7e4, 0xba2c8499, 0xcd232729, 0x00000000, 0xcbfdeaa4, 0x9d6efd18,
    0xc3c7f742, 0x4c03f8dd, 0x2e0f66c3, 0x0707af0c, 0xdfa7552b, 0xc98c8553, 0xcc85bd82, 0x297807a8, 0xe378023c, 0xfb4205c6,
    0x1cdb4ad6, 0x1bf2f8f0, 0x00000000, 0x098105d5, 0xbbe508ef, 0xd4e7daf2, 0x00000000, 0xb0e51693, 0x00000000, 0x00000000,
    0xe92a7781, 0x8a7b2224, 0x00000000, 0x00000000, 0x96e66305, 0x865cbe43, 0x4d4f55b7,


    0x00000000,

};
static const u32 jpg_640file_crc32[] = {

    0x00000000, 0xc4c6ff88, 0xb280d059, 0xf3bcdc11, 0x86d0077b, 0xe2a0f4a3, 0xa4dddf93, 0x029e0eae, 0x13b826e1, 0x00000000,
    0x598c822b, 0xeba70b8c, 0x2af68ff9, 0x1784b18c, 0x7d4a17d8, 0x00000000, 0x42f611c0, 0x461fb12a, 0x00000000, 0x00000000,
    0x096f6882, 0x00000000, 0x00000000, 0xdec03585, 0x00000000, 0xaf490a0a, 0x3517c9d3, 0x00000000, 0x00000000, 0x6d3f0487,
    0x00000000, 0x82839244, 0x497bd871, 0x0635e8c8, 0xba2042c1, 0x1e4d4828, 0xe54e9b16, 0x338f67a1, 0x00000000, 0xce838217,
    0x00000000, 0xd0e2f68f, 0x00000000, 0xfb37eef7, 0x00000000, 0xc1575e90, 0x8ebe314a, 0x34575112, 0x8d90b5ab, 0x62d41ab0,
    0x00000000, 0xccd10b21, 0xfce46d2d, 0x4c03fa29, 0x00000000, 0x61a79697, 0x272b1eb0, 0x1719e343, 0x00000000, 0x60b944f9,
    0x82839244, 0x00000000, 0x486f47d9, 0x0c595650, 0x00000000, 0xfd049664, 0x5f8d9d94, 0xf078128b, 0x00000000, 0x3fe2e24f,
    0x24534714, 0x7890b5ef, 0x95dd13d5, 0x40f16906, 0xbcd84fc8, 0x00000000, 0x00000000, 0x1f6d16b3, 0x00000000, 0x210040c8,
    0xa4687d5a, 0x1f0ecc23, 0xd3467d4e, 0x00000000, 0x0a4dab6b, 0x07c052e2, 0xf0cc5117, 0x674e8aab, 0xa2f11893, 0x00000000,
    0x65a0b8da, 0x00000000, 0x1258c084, 0x00000000, 0xa85192c1, 0x00000000, 0x529efe49, 0x00000000, 0xe89930f3, 0x7e1aae0e,
    0x00000000, 0x00000000, 0x00000000, 0x2af68ff9, 0x00000000, 0x00000000, 0x00000000, 0xfb9eca1c, 0x40ad8453, 0x5eb74f87,
    0x00000000, 0x045b2d69, 0x27797e37, 0x5e30c494,

    0x00000000,
};
static const u32 jpg_960file_crc32[] = {

    0x00000000, 0x5e382358, 0xd62f2b7c, 0x00000000, 0x9b371c56, 0x6235fa64, 0x00000000, 0x8e70ac2b, 0x4be59826, 0x00000000,
    0xc011b52a, 0x00000000, 0x0808f69e, 0xb2bf5e82, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5a08fb08,
    0x00000000, 0xf281a723, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xc564de5a, 0x00000000, 0x00000000, 0xed83cacf,
    0xcef1cb69, 0x00000000, 0x2444a00c, 0x00000000, 0x7d083c2e, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xd0624945,
    0x00000000, 0x20be226e, 0x64cb595f, 0x00000000, 0x00000000, 0x29bf41c5, 0x6e8050fc, 0x00000000, 0x960c2649, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x8c011e43, 0xb9d75200, 0x6a425eb1, 0x00000000, 0x2474e2fa, 0xcc460431, 0xe6bf0529,
    0xe09ba205, 0x00000000, 0xafe7bb90, 0x426be080, 0xcf696189, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0xda71f9b3, 0x00000000, 0x105ed779, 0x89be2a18, 0x00000000, 0x00000000, 0x00000000, 0xd85c8764, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xaed37b89, 0x00000000, 0x00000000, 0x4ea23ad7, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xe306753a, 0x00000000, 0x53e44e11, 0xe3323dc4, 0x00000000, 0xbc26d175, 0x00000000, 0x00000000,
    0x00000000, 0x358ace2f, 0x065f776f, 0x7321b59b, 0x00000000, 0xc7c47439, 0x6a8cf4d9,


    0x00000000,
};
static const u32 jpg_1024file_crc32[] = {

    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x29b308a7, 0x00000000, 0x00000000, 0x00000000, 0x815f0239,
    0x1c4ddea8, 0x00000000, 0x57d579dd, 0x23e23dc5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x99653126,
    0x00000000, 0x00000000, 0xf20cb3e5, 0x00000000, 0x00000000, 0x69297b8f, 0xc75e0ca9, 0x00000000, 0x00000000, 0xc2e3ecec,
    0x00000000, 0x756afdc1, 0x00e0365c, 0x00000000, 0x1f4657ba, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xf7978363,
    0x00000000, 0x00000000, 0x3623b033, 0x99653126, 0x00000000, 0xa0a388c6, 0x00000000, 0x00000000, 0x5bafa3db, 0x00000000,
    0x4237ad25, 0x7d05dde8, 0x00000000, 0x00000000, 0x9a2439e6, 0x00000000, 0xbbbe994c, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x4a7cbc65, 0xdd370b6d, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8048c570,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xf9167096, 0x056214e8, 0xdf40553c, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x3538fcde, 0x79902bcb, 0x645726b6, 0x00000000, 0x00000000, 0x1de09e40, 0xeba40d9f, 0xdc2d65dc,
    0x5b1b8e6f, 0x02cea750,


    0x00000000,
};
static const u32 jpg_1920file_crc32[] = {
    0x00000000, 0x6b4077f2, 0x7ba8a00d, 0xa65361ba,
    0x00000000,
};
#endif


#if JPEG_DEC_PACKED_TILEBASE




static const u32 jpg_128file_crc32[] = {
    0x00000000, 0x9ad3a8b5, 0xde9c5173, 0xde9c5173, 0x04b7177c, 0x20fdf7fd, 0xd2011b2e, 0x10b58bbc, 0x4d867503, 0xa1335d55,
    0x1a38c2f4, 0xdaaada99, 0x2c9358f5, 0x39ac4a16, 0xcbd0dcaa, 0xfc855482, 0x040d5bae, 0x6be8bea6, 0xdaf820a1, 0xe2f37733,
    0x806f22da, 0x7afe3d23, 0xdf8877cd, 0xacb2f50c, 0x02fbdf97, 0x3b10f8c4, 0x01ff703a, 0xddbf897e, 0x5a21724f, 0x4b2027c1,
    0x2256d55b, 0x3fff1932, 0xd8460937, 0x209c9032, 0x571fd9d4, 0x6a915a7c, 0x87f56b14, 0x2b27c72f, 0x00b07851, 0xe84a0211,
    0xafbb5a3f, 0x312895cb, 0x84383d9d, 0xcad4d80a, 0x19adc67b, 0x9552ca1e, 0x746ba677, 0x52b08a3b, 0x722e8857, 0x92818684,
    0xc1901a69, 0xe48783b5, 0xe87d21e8, 0xe8c0fdaa, 0x31ebcc99, 0x74f5bcb9, 0x813b4f3a, 0xb30405a0, 0x444e95cc, 0xb6702f01,
    0xd1170e46, 0x81f70793, 0xb1b1f1c3, 0xd1f6ed56, 0x0afbdac9, 0x62fae350, 0x1ac59970, 0x577046d7, 0xd5394f67, 0xffeeefcd,
    0xb93d322a, 0x67b650d8, 0x7b02b58b, 0x482c131c, 0x22da977a, 0x1a31102a, 0x70a57563, 0xa3cb9984, 0x67cf0a2b, 0xc4258cdc,
    0xf96c38ba, 0x59448133, 0x83bbc4e5, 0xd88d137f, 0x3bb71776, 0xc3830918, 0x6044cdb2, 0x32d573e2, 0x4d35dab5, 0x24184788,
    0x3f8f4871, 0xdada35ad, 0x13b57c37, 0x6a092f76, 0x3b20aa58, 0x2cf7f151, 0xa7f30fd2, 0xfd983c2c, 0x993f9e75, 0x2437de5b,
    0x5f2686c5, 0x74f2a97b, 0x9f752b83, 0xa0ec80f0, 0x641197a0, 0x7231d272, 0x93182d6f, 0xc6b766c6, 0x45a75ce0, 0x2daaaad2,
    0x566e9ad8, 0x9500be45, 0x4437728a, 0x55a54501, 0x86809d6a, 0x6719877a, 0xf477a892, 0x70ee1d19, 0xe563c3ef, 0x5fefdec4,
    0x286f2c7a, 0x7c5b1b68, 0x85769a5b, 0x5d01564e, 0x50c93e33, 0x6ae10a09, 0x71f28c52, 0xefc51091, 0x8821ae59, 0x1efe5708,
    0x46014308, 0xc9eec351, 0xdaa5e412, 0xdc10621a, 0x17269afd, 0x187a5415, 0x13c743f6, 0x60006681, 0xaaa04c67, 0x736030da,
    0xe5d0dd2c, 0xee37e20e, 0x7b73048b, 0x65e01b68, 0xe90eb20f, 0x2ca9963c, 0xf2838f0e, 0xb087d2d5, 0xb087d2d5, 0x2a0e31fb,
    0x350b4460, 0xcd1356ac, 0x256bdbc4, 0x5d52671a, 0xd8a1467f, 0xf847fa41, 0xf847fa41, 0xf847fa41, 0x17d213d1, 0x17d213d1,
    0xb161bbef, 0xb161bbef, 0x0ec80165, 0x0ec80165, 0x1ee23beb, 0xb06a66a1, 0x2ae175f1, 0x2ae175f1, 0x2ae175f1, 0x2ae175f1,
    0x2ae175f1, 0x2ae175f1, 0x2ae175f1, 0x2ae175f1, 0x36a61b73, 0x5c718bc4, 0x524b2167, 0x57039451, 0xd9a0ebae, 0x733373c7,
    0x5ff4c37d, 0x3895f2e1, 0x1bdbccd5, 0x0428c308, 0xcd1f7f93, 0x48e7896c, 0x82bfe7ac, 0xf5f3eeff, 0x70554443, 0xaf5c0df5,
    0x5c7f3935, 0xd8128aed, 0x5a75bf44, 0x79a71014, 0x4a78cc15, 0x0d001bf3, 0x42247213, 0xc17e12e4, 0x1c1a15df, 0x093ba316,
    0x333a61b8,


    0x00000000,

};
static const u32 jpg_160file_crc32[] = {

    0x00000000, 0x6ea8b930, 0xd75bb0e2, 0x93618026, 0x0c730a17, 0xdce5d14b, 0x00000000, 0x012dea58, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x370eb560, 0xdff8e56f, 0xd398760b, 0x9ae88eb1, 0x957697f3, 0x843a0e69,
    0x5153a1b5, 0x25ed47ce, 0xf7ac53b7, 0x5774ccf2, 0xe8b9ebb7,

    0x00000000,
};
static const u32 jpg_320file_crc32[] = {

    0x00000000, 0x00000000, 0x56a2235f, 0x57122875, 0x9e4af921, 0x2d8a1dc2, 0x00000000, 0xaa8b0600, 0xca65188d, 0x7ccda441,
    0xfff6a3ba, 0x29c668ab, 0xb64bd9ae, 0x4dc072d9, 0x7278013d, 0x9a2554d9, 0x00000000, 0x634fed4d, 0x52cdabeb, 0xdaacb079,
    0x4d922519, 0xd0302d5d, 0x72ef4213, 0x00000000, 0x00000000, 0x7df4191b, 0x493a0018, 0x48e796fc, 0x955490c1, 0xc0f231f8,
    0x3270b1ca, 0xd2e26c44, 0x47c6bbbc, 0x806c7f73, 0x00000000, 0x9c386d75, 0x96528450, 0x743ff063, 0x107a174a, 0xb3e1269a,
    0x33596e53, 0x992481f0, 0x6a14ec70, 0xdbf83251, 0xd525d8b8, 0x00000000, 0xf2d7bc5d, 0xb11da01c, 0x411d8944, 0xc362b609,
    0x00000000, 0xe82139e3, 0x70b719ab, 0x00000000, 0x87a4da2c, 0xa3304506, 0xc4b6852b, 0x00000000, 0xd006819b, 0x077a3342,
    0xc7a655ce, 0xc27da6f9, 0x00000000, 0x00000000, 0xab48d754, 0x54bc2d99, 0x45816cb1, 0xeaa3c1d4, 0x083e9daa, 0x6b87b71b,
    0xa43f77ee, 0x815b09e7, 0x6247193d, 0xe0696853, 0x6875fdfb, 0xb6d8c218, 0x5cab7ede, 0xf927b418, 0x160ad3b8, 0x05440014,
    0x0a75de68, 0x00000000, 0xa2d08e95, 0xd010fa56, 0xeb68d87a, 0x2b8cd055, 0xcb306d79, 0x82a87e9c, 0x05595d37, 0x00000000,
    0x00000000, 0x197d7203, 0x8b88e5ed, 0x5f1fe0cf, 0x79ab7459, 0xbb13d936, 0x00000000, 0xa22b530b, 0x6e55613d, 0x262612fe,
    0x927630e1, 0x8d3ebe10, 0x7672a4e1, 0x28d2a248,

    0x00000000,

};
static const u32 jpg_480file_crc32[] = {

    0x00000000, 0xc99c0481, 0x31c26c99, 0x687604d9, 0xd04720c5, 0x0452f199, 0x35d70fdc, 0xcf35bc25, 0x3091d800, 0xd8dc3122,
    0xa0cb94d6, 0x3205ef1b, 0xb6ad3b5b, 0x6a4ce10f, 0x00000000, 0x00000000, 0x5a5f42b6, 0x00000000, 0xd1bd6e80, 0x781eb8a1,
    0x9219bbb7, 0xb017f10c, 0xd5a7abb8, 0x21b782d3, 0xa98dcf12, 0x0814b5b3, 0x7fdbaefc, 0x10e05411, 0x00000000, 0x2d63a93a,
    0x15debb64, 0x25472844, 0x149a3e6e, 0x3d69bd6e, 0xecf7a92a, 0xcc85bd82, 0xa7ec6076, 0x510b6d19, 0x66d52275, 0x9bf3b5cb,
    0x00000000, 0x78e74e2b, 0x00000000, 0x35b464e1, 0x66377887, 0x3cb3d9b8, 0x53eb8e52, 0x20187be1, 0x3d58da4d, 0x4ac51b6d,
    0x5ffcc624, 0x2b376e8b, 0xa3f0d290, 0xdff531d6, 0x8f3dea81, 0x00000000, 0x814713ff, 0xa1faed40, 0x7f38d281, 0xb15847ef,
    0xf077f23f, 0x74bc1cfa, 0xfe171b76, 0xe58e8cc9, 0x6ef2bbc1, 0xc3402836, 0x240ed074, 0x909e1f25, 0xc67ee176, 0x7cd824ce,
    0x8b16c681, 0xf85c5230, 0x30a13836, 0x27cea435, 0xa8127501, 0xb295dccc, 0x02706af2, 0xb9d51c76, 0xce5a72b1, 0xe6c85212,
    0xc9da6789, 0xc6a42cb2, 0x126ac498, 0xad212a1b, 0xf490a7e4, 0xba2c8499, 0xcd232729, 0x00000000, 0xcbfdeaa4, 0x9d6efd18,
    0xc3c7f742, 0x4c03f8dd, 0x2e0f66c3, 0x0707af0c, 0xdfa7552b, 0xc98c8553, 0xcc85bd82, 0x297807a8, 0xe378023c, 0xfb4205c6,
    0x1cdb4ad6, 0x1bf2f8f0, 0x00000000, 0x098105d5, 0xbbe508ef, 0xd4e7daf2, 0x00000000, 0xb0e51693, 0x00000000, 0x00000000,
    0xe92a7781, 0x8a7b2224, 0x00000000, 0x00000000, 0x96e66305, 0x865cbe43, 0x4d4f55b7,


    0x00000000,

};
static const u32 jpg_640file_crc32[] = {

    0x00000000, 0xc4c6ff88, 0xb280d059, 0xf3bcdc11, 0x86d0077b, 0xe2a0f4a3, 0xa4dddf93, 0x029e0eae, 0x13b826e1, 0x00000000,
    0x598c822b, 0xeba70b8c, 0x2af68ff9, 0x1784b18c, 0x7d4a17d8, 0x00000000, 0x42f611c0, 0x461fb12a, 0x00000000, 0x00000000,
    0x096f6882, 0x00000000, 0x00000000, 0xdec03585, 0x00000000, 0xaf490a0a, 0x3517c9d3, 0x00000000, 0x00000000, 0x6d3f0487,
    0x00000000, 0x82839244, 0x497bd871, 0x0635e8c8, 0xba2042c1, 0x1e4d4828, 0xe54e9b16, 0x338f67a1, 0x00000000, 0xce838217,
    0x00000000, 0xd0e2f68f, 0x00000000, 0xfb37eef7, 0x00000000, 0xc1575e90, 0x8ebe314a, 0x34575112, 0x8d90b5ab, 0x62d41ab0,
    0x00000000, 0xccd10b21, 0xfce46d2d, 0x4c03fa29, 0x00000000, 0x61a79697, 0x272b1eb0, 0x1719e343, 0x00000000, 0x60b944f9,
    0x82839244, 0x00000000, 0x486f47d9, 0x0c595650, 0x00000000, 0xfd049664, 0x5f8d9d94, 0xf078128b, 0x00000000, 0x3fe2e24f,
    0x24534714, 0x7890b5ef, 0x95dd13d5, 0x40f16906, 0xbcd84fc8, 0x00000000, 0x00000000, 0x1f6d16b3, 0x00000000, 0x210040c8,
    0xa4687d5a, 0x1f0ecc23, 0xd3467d4e, 0x00000000, 0x0a4dab6b, 0x07c052e2, 0xf0cc5117, 0x674e8aab, 0xa2f11893, 0x00000000,
    0x65a0b8da, 0x00000000, 0xc47d8eb4, 0x00000000, 0xa85192c1, 0x00000000, 0x529efe49, 0x00000000, 0xe89930f3, 0x7e1aae0e,
    0x00000000, 0x00000000, 0x00000000, 0x2af68ff9, 0x00000000, 0x00000000, 0x00000000, 0xfb9eca1c, 0x40ad8453, 0x5eb74f87,
    0x00000000, 0x045b2d69, 0x27797e37, 0x5e30c494,

    0x00000000,
};
static const u32 jpg_960file_crc32[] = {

    0x00000000, 0x5e382358, 0xd62f2b7c, 0x00000000, 0x9b371c56, 0x6235fa64, 0x00000000, 0x8e70ac2b, 0x4be59826, 0x00000000,
    0xc011b52a, 0x00000000, 0x0808f69e, 0xb2bf5e82, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5a08fb08,
    0x00000000, 0xf281a723, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xc564de5a, 0x00000000, 0x00000000, 0xed83cacf,
    0xcef1cb69, 0x00000000, 0x2444a00c, 0x00000000, 0x7d083c2e, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xd0624945,
    0x00000000, 0x20be226e, 0x64cb595f, 0x00000000, 0x00000000, 0x29bf41c5, 0x6e8050fc, 0x00000000, 0x960c2649, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x8c011e43, 0xb9d75200, 0x6a425eb1, 0x00000000, 0x2474e2fa, 0xcc460431, 0xe6bf0529,
    0xe09ba205, 0x00000000, 0xafe7bb90, 0x426be080, 0xcf696189, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0xda71f9b3, 0x00000000, 0x105ed779, 0x89be2a18, 0x00000000, 0x00000000, 0x00000000, 0xd85c8764, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xaed37b89, 0x00000000, 0x00000000, 0x4ea23ad7, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xe306753a, 0x00000000, 0x53e44e11, 0xe3323dc4, 0x00000000, 0xbc26d175, 0x00000000, 0x00000000,
    0x00000000, 0x358ace2f, 0x065f776f, 0x7321b59b, 0x00000000, 0xc7c47439, 0x6a8cf4d9,


    0x00000000,
};
static const u32 jpg_1024file_crc32[] = {

    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x29b308a7, 0x00000000, 0x00000000, 0x00000000, 0x815f0239,
    0x1c4ddea8, 0x00000000, 0x57d579dd, 0x23e23dc5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x99653126,
    0x00000000, 0x00000000, 0xf20cb3e5, 0x00000000, 0x00000000, 0x69297b8f, 0xc75e0ca9, 0x00000000, 0x00000000, 0xc2e3ecec,
    0x00000000, 0x756afdc1, 0x00e0365c, 0x00000000, 0x1f4657ba, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xf7978363,
    0x00000000, 0x00000000, 0x3623b033, 0x99653126, 0x00000000, 0xa0a388c6, 0x00000000, 0x00000000, 0x5bafa3db, 0x00000000,
    0x4237ad25, 0x7d05dde8, 0x00000000, 0x00000000, 0x9a2439e6, 0x00000000, 0xbbbe994c, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x4a7cbc65, 0xdd370b6d, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8048c570,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xf9167096, 0x056214e8, 0xdf40553c, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x3538fcde, 0x79902bcb, 0x645726b6, 0x00000000, 0x00000000, 0x1de09e40, 0xeba40d9f, 0xdc2d65dc,
    0x5b1b8e6f, 0x02cea750,


    0x00000000,
};
static const u32 jpg_1920file_crc32[] = {
    0x00000000, 0x6b4077f2, 0x7ba8a00d, 0xa65361ba,
    0x00000000,
};


#endif






int YUV444pToYUV420p(unsigned char *yuv444p, unsigned char *yuv420p, int width, int height);
int YUV422pToYUV420p(unsigned char *yuv422p, unsigned char *yuv420p, int width, int height);
void all_dev_range_limit(void *addr, int size);
void all_dev_range_unlimit(void *addr);


#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
#if CYC_FILE_NUM == 0
#define JPEG_WIDTH		128 //用户根据实际应用更改
#define JPEG_HEIGH		160 //用户根据实际应用更改
#elif CYC_FILE_NUM == 1
#define JPEG_WIDTH		160 //用户根据实际应用更改
#define JPEG_HEIGH		128 //用户根据实际应用更改
#elif CYC_FILE_NUM == 2
#define JPEG_WIDTH		320 //用户根据实际应用更改
#define JPEG_HEIGH		240 //用户根据实际应用更改
#elif CYC_FILE_NUM == 3
#define JPEG_WIDTH		480 //用户根据实际应用更改
#define JPEG_HEIGH		320 //用户根据实际应用更改
#elif CYC_FILE_NUM == 4
#define JPEG_WIDTH		640 //用户根据实际应用更改
#define JPEG_HEIGH		480 //用户根据实际应用更改
#elif CYC_FILE_NUM == 5
#define JPEG_WIDTH		960 //用户根据实际应用更改
#define JPEG_HEIGH		640 //用户根据实际应用更改
#elif CYC_FILE_NUM == 6
#define JPEG_WIDTH		1024 //用户根据实际应用更改
#define JPEG_HEIGH		720 //用户根据实际应用更改
#else
#define JPEG_WIDTH		1920 //用户根据实际应用更改
#define JPEG_HEIGH		1200 //用户根据实际应用更改
#endif

/*static u8 yuv_addr[320*240*3 + 4*1024] sec(.sram) ALIGNED(32);*/
/*static u8 bit_addr[120*1024] sec(.sram)  ALIGNED(32);*/
/*static u8 bit_addr[120*1024]  ALIGNED(32);*/

/*static u8 dbg_addr[120*1024 + 320*240*3 + 4*1024] ALIGNED(32);*/
/*static u8 *yuv_addr = &dbg_addr[120*1024];*/
/*static u8 *bit_addr = &dbg_addr[0];*/

/*static u8 yuv_addr[320 * 240 * 3 + 4 * 1024] sec(.sram) ALIGNED(32);*/
/*static u8 bit_addr[120 * 1024] ALIGNED(32);*/

/*static u8 yuv_addr[320 * 240 * 3 + 4 * 1024] ALIGNED(32);*/
static u8 yuv_addr[JPEG_WIDTH * JPEG_HEIGH * 3 + 4 * 1024] ALIGNED(32);
#if JPEG_DEC_BITBUF_SRAM
static u8 bit_addr[400 * 1024] sec(.sram) ALIGNED(32);
#else
static u8 bit_addr[400 * 1024] ALIGNED(32);
#endif

#define JPEG_LINE		16  //用户根据实际应用更改:yuv420/422->16 ,yuv444->8, 默认使用16行最大值
#define JPEG_YUV_TTPE	4	//444:1, 422:2, 420:4 ,默认使用444最大值
#define JPEG_YUV_SIZE	((JPEG_WIDTH * JPEG_LINE + JPEG_WIDTH * JPEG_LINE / JPEG_YUV_TTPE * 2 + 64) * 2)
static u8 yuv_buf[JPEG_YUV_SIZE] sec(.sram) ALIGNE(4);//内部使用
#else
#if JPEG_DEC_BITBUF_SRAM
static u8 bit_addr[400 * 1024] sec(.sram) ALIGNED(32);
#endif
#endif

//下列几个变量用于异常中断打印信息
char *yuv_dec_name = NULL;
int yuv_saddr = 0;
int yuv_eaddr = 0;
char jpeg_start = 0;
int check_addr = 0;
char *jpeg_start_name;

#if SAVE_FILE_CRC32
static u32 yuvcrc_addr[4 * 1024] = {0};
static u32 yuvcrc_addr_cnt = 0;
#endif
#if 0
void tilebase2linebase(char *src, char *dst)
{
    printf("src:%x  dst:%x\n", src, dst);
    int i, j;
    int k, l;
    int o = 0;
    for (i = 0; i < 480 / 8; i++) {
        for (j = 0; j < 320 / 8; j++) {
            char *m = (char *)&src[8 * i][8 * j];
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    dst[o++] = m[i][j];
                }

            }
        }
    }



    put_buf(src, 64);

    put_buf(dst, 64);
    printf("======================end\n");
}
#endif

void convertYUV422TileToLine(unsigned char *tile_base, unsigned char *line_base, int width, int height)
{
    // Assuming each block has 2 pixels
    int blockWidth = 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += blockWidth) {
            // Copy Y component
            line_base[y * width + x] = tile_base[y * width + x];
            line_base[y * width + x + 1] = tile_base[y * width + x + 1];

            // Copy U and V components
            line_base[width * height + y * (width / 2) + x / 2] = tile_base[width * height + y * (width / 2) + x / 2];
            line_base[width * height + (height / 2) * (width / 2) + y * (width / 2) + x / 2] = tile_base[width * height + (height / 2) * (width / 2) + y * (width / 2) + x / 2];
        }
    }
    put_buf(tile_base, 64);
    put_buf(line_base, 64);
}


typedef struct {
    char y0;
    char u0;
    char y1;
    char v0;
    char y2;
    char u1;
    char y3;
    char v1;

    char y4;
    char u2;
    char y5;
    char v2;
    char y6;
    char u3;
    char y7;
    char v3;
} pix;



void tilebase2linebase(pix src[], pix dst[], int width, int height)
{
    /* printf("src:%x  dst:%x\n",src,dst); */
    /* printf("src[0]:%d\n",sizeof(src[0])); */
    int i = 0, j = 0, k = 0;
    for (i = 0; i < height / 8; i++) {
        for (j = 0; j < width / 8; j++) {
            for (k = 0; k < 8; k++) {
                dst[i * width + k * width / 8 + j] = src[i * width + j * 8 + k];
            }
        }
    }

}







int jpeg_decoder_more(void)//FPGA test
{
    FILE *svfd = NULL;
    FILE *fd = NULL;
    FILE *yuv_fd = NULL;
    FILE *src_fd = NULL;
    FILE *rgb_fd = NULL;
    int cyc_cnt = 0;
    u8 flag = 0;
    u32 ok_cnt = 0;
    u32 lenth = 0, l_len, i, j, k, m;
    u8 *cy = NULL, *cb = NULL, *cr = NULL, *yuv = NULL, *rgb16 = NULL, *yuv2 = NULL;
    u8 *buf = NULL, *pbuf = NULL;
    u32 pix = 0;
    u16 width, height;
    u16 jpeg_num = 100;
    u8 cnt = 0;
    u8 ytype = 0;
    u32 addr_shown = 0;
    u32 len;
    u32 blen = 0;
    u8 *baddr;
    u32 jpg_sum = 0;
    u32 jpg_sum_check = 0;
    struct jpeg_decode_req req = {0};
    char name[128];
    u32 crc32_check;
    char crc32_check_err;
    u32 *yuvcrc32_addr = NULL;
    u32 yuvcrc32[10] = {
        (u32)jpg_128file_crc32,
        (u32)jpg_160file_crc32,
        (u32)jpg_320file_crc32,
        (u32)jpg_480file_crc32,
        (u32)jpg_640file_crc32,
        (u32)jpg_960file_crc32,
        (u32)jpg_1024file_crc32,
        (u32)jpg_1920file_crc32,
        NULL,
    };

    u8 name_type[10][32] = {
        "128_160_sum/160_",
        "160_128_sum/160_",
        "320_240_sum/320_",
        "480_320_sum/480_",
        "640_480_sum/640_",
        "960_640_sum/960_",
        "1024_720_sum/1024_",
        "1920_1200_sum/1920_",
        0,
    };
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }
    os_time_dly(2);
    printf("---> jpg decoder start\n");
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
    printf("---> yuv_addr = 0x%x , bit_addr = 0x%x \n", yuv_addr, bit_addr);
#elif JPEG_DEC_BITBUF_SRAM
    printf("---> bit_addr = 0x%x \n", bit_addr);
#endif

#if CYC_FILE_DECODE
    for (j = CYC_FILE_NUM; cyc_cnt < FILE_DECODE_NUM; cyc_cnt++) {
#else
    cyc_cnt = FILE_DECODE_NUM;
    while (cyc_cnt--)
        for (j = JPEG_DEC_START_NUM; j <= JPEG_DEC_END_NUM && name_type[j][0] != 0; j++) {
#endif
        cnt = 0;
        printf("file name : %s\n", &name_type[j]);
        for (i = 1; i <= 200; i++) {
            sprintf(name, "%s%s%d.jpg", CONFIG_ROOT_PATH, &name_type[j], i);
#if (!JPEG_DEC_DRI_FILE_ALSO || JPEG_DEC_DRI_FILE_ONLY)
            char find_dri = 0;
            for (int m = 0; jpg_dri_file[m][0] != 0; m++) {
                if (strstr(name, &jpg_dri_file[m])) {
                    find_dri = 1;
                    break;
                }
            }
#if JPEG_DEC_DRI_FILE_ONLY
            if (!find_dri) {
#else
            if (find_dri) {
#endif
                /*printf("no dec %s\n",name);*/
                continue;
            }
#endif
            char output[128];
            long_file_name_encode(name, output, sizeof(output));
            fd = fopen(output, "r");
            if (fd == NULL) {
                /*printf("no file name : %s\n", name);*/
                cnt++;
                /*if (cnt > 3) {*/
                /*break;*/
                /*}*/
                continue;
            }
            cnt = 0;
            crc32_check_err = 0;
            /*printf("file name : %s\n", name);*/
            lenth = flen(fd);
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN) || JPEG_DEC_BITBUF_SRAM
            buf = bit_addr;
            if (lenth > sizeof(bit_addr)) {
                printf("err in buf no enough : %s\n", name);
                fclose(fd);
                goto free_buf;
            }
#else
            buf = malloc(lenth);//bit_addr;
#endif
            if (!buf) {
                printf("err no mem \r\n");
                fclose(fd);
                goto free_buf;
            }
            if (ffread(buf, 1, lenth, fd) != lenth) {
                fclose(fd);
                printf("read file lenth err ...\n");
                goto free_buf;
            }
            fclose(fd);
            pbuf = buf;

#if FILE_CHECK_SUM
            //SD卡JPG和校验
            jpg_sum = 0;
            jpg_sum_check = 0;
            for (k = 0; k < (lenth - 4); k++) {
                jpg_sum += pbuf[k];
            }
            jpg_sum_check |= (u8)pbuf[k + 0] << 24;
            jpg_sum_check |= (u8)pbuf[k + 1] << 16;
            jpg_sum_check |= (u8)pbuf[k + 2] << 8;
            jpg_sum_check |= (u8)pbuf[k + 3] << 0;
            if (jpg_sum != jpg_sum_check) {
                printf("read file check sum err : %s\n", name);
                goto free_buf;
            }
            lenth -= 4;
#endif

            struct jpeg_image_info info = {0};
            info.input.data.buf = pbuf;
            info.input.data.len = lenth;
            if (jpeg_decode_image_info(&info)) {
                printf("jpeg_decode_image_info err %s\n", name);
                goto free_buf;
            }
            width = info.width;
            height = info.height;
            pix = width * height;
            switch (info.sample_fmt) {
            case JPG_SAMP_FMT_YUV444:
                printf("JPG_SAMP_FMT_YUV444\n");
                ytype = 1;
                break;//444
            case JPG_SAMP_FMT_YUV420:
                printf("JPG_SAMP_FMT_YUV420\n");
                ytype = 4;
                break;//420
            default:
                printf("JPG_SAMP_FMT_YUV422\n");
                ytype = 2;
                break;//422
            }

#if JPEG_DEC_PACKED_LINEBASE || JPEG_DEC_PACKED_TILEBASE
            if (ytype == 1) {
                printf("packed not support yuv444\n");
                goto free_buf;
            }
            ytype = 2;
#endif
            len = pix + pix / ytype * 2;
            if (!yuv) {
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
                yuv = yuv_addr;
                if (len > sizeof(yuv_addr)) {
                    printf("err in yuv not enough \n\n");
                    goto free_buf;
                }
#else
#if JPEG_YUV_BIT_ADDR_ONE
                blen = ADDR_ALIGNE4(lenth);
                yuv = malloc(len + blen + 4 * 1024);
                baddr = yuv + len + 4 * 1024;
#else
                yuv = malloc(len + 4 * 1024);
#endif
#endif
                if (!yuv) {
                    printf("yuv malloc err len : %d , width : %d , height : %d \n", width, height, len);
                    goto free_buf;
                }
                yuv2 = malloc(len + 4 * 1024);
                printf("malloc yuv:%x  yuv2:%x\n", yuv, yuv2);


            }
#if SAVE_FILE_YUV_OR_RGB
#if SAVE_FILE_YUV
            if (j < 7) { //大于1024，空间不足
                if (!rgb16) {
                    rgb16 = malloc(pix * 3 / 2);
                }
#else
            if (j < 7) { //大于720P，RGB空间不足
                if (!rgb16) {
                    rgb16 = malloc(pix * 2);
                }
#endif
            } else {
                if (!rgb16) {
                    rgb16 = malloc(32);
                }
            }

            if (!rgb16) {
                printf("rgb16 malloc err !!!!\n");
                goto free_buf;
            }
#endif
            /* memset(yuv,0,len); */

            cy = yuv;
            cb = cy + pix;
            cr = cb + pix / ytype;
#if JPEG_YUV_BIT_ADDR_ONE
            memcpy(baddr, pbuf, lenth);
            pbuf = baddr;
#endif

            /* DcuFlushinvRegion(yuv,len); */
            req.input_type = JPEG_INPUT_TYPE_DATA;
            req.input.data.buf = pbuf;
            req.input.data.len = lenth;
            /* printf("lenth:%d\n",lenth); */
            /* put_buf(pbuf,lenth); */
            req.buf_y = cy;
            req.buf_u = cb;
            req.buf_v = cr;
            req.buf_width = width;
            req.buf_height = height;
            req.out_width = width;
            req.out_height = height;
            req.output_type = JPEG_DECODE_TYPE_DEFAULT; //buf mode
#if JPEG_DEC_PLANAR
#elif JPEG_DEC_PACKED_LINEBASE
            req.output_format = JPEG_DECODE_OUTPUT_PACKED_LINEBASE_UYVY; //output_format
#elif JPEG_DEC_PACKED_TILEBASE
            req.output_format = JPEG_DECODE_OUTPUT_PACKED_TILEBASE_UYVY; //output_format
#endif
            printf("data:%x    y:%x\n", pbuf, cy);
#if !JPEG_DEC_INTERRUPT
            req.dec_query_mode = TRUE;
#endif
#if JPEG_DEC_YUV_UNCACH
            req.bits_mode = BITS_MODE_UNCACHE;
#else
            req.bits_mode = BITS_MODE_CACHE;
#endif

            unsigned char *check_buf = (unsigned char *)(yuv + len);
            memset(check_buf, CHECK_MAG, 4 * 1024);
            /* DcuFlushinvRegion((void *)((u32)check_buf / 32 * 32), 4 * 1024); */

            yuv_dec_name = name;
            yuv_saddr = (int)yuv;
            yuv_eaddr = (int)yuv + len;

            if (addr_shown++ == 0) {
                printf("---> yuv_addr = 0x%x , bit_addr = 0x%x \n", yuv, buf);
            }
            jpeg_start_name = name;

#if JPEG_DEC_DEV_LIMIT
            if (req.bits_mode == BITS_MODE_CACHE) { //经过cach才行
                check_addr = check_buf;
                all_dev_range_limit((void *)(check_buf), 4 * 1024);
            }
#endif
            jpeg_start = 1;

            flag = jpeg_decode_one_image(&req, JPEG_DEV_ID_0);

#if JPEG_DEC_DEV_LIMIT
            if (req.bits_mode == BITS_MODE_CACHE) { //经过cach才行
                all_dev_range_unlimit((void *)(check_buf));
            }
#endif
            if (flag) {
                printf("---> err in, yuv = 0x%x %d, jpeg_decode_one_image = %s \n\n\n", yuv, len, name);
                /*while (1) {*/
                os_time_dly(10);
                /*}*/
                /*goto free_buf;*/
            }
            ok_cnt++;
            if (ok_cnt % 100 == 0) {
                putchar('@');
            }
            for (k = 0; k < 4 * 1024; k++) {
                if (check_buf[k] != CHECK_MAG) {
                    printf("---> err in decode image,check_buf[%d]=0x%x, yuv=0x%x,check_buf=0x%x\n", k, check_buf[k], yuv, check_buf);
                    put_buf(&check_buf[k], 32);
                    break;
                }
            }
            pix = req.out_width * req.out_height;
            /* printf("==================%s   %d\n",__func__,__LINE__); */
            /* put_buf(yuv,32); */
#if JPEG_DEC_PACKED_TILEBASE
            tilebase2linebase(yuv, yuv2, width, height);
            crc32_check = CRC32(yuv2, len);
#else
            crc32_check = CRC32(yuv, len);
#endif
            jpeg_start = 0;

#if SAVE_FILE_CRC32
            yuvcrc_addr_cnt = i;
            yuvcrc_addr[yuvcrc_addr_cnt] = crc32_check;
#endif

            //CRC32数字校验YUV
            yuvcrc32_addr = (u32 *)((u32)yuvcrc32[j] + i * sizeof(u32));
            /*printf("---> crc=0x%x ,  org_crc=0x%x \n",crc32_check,*yuvcrc32_addr);*/
            if (yuvcrc32_addr && *yuvcrc32_addr != 0) {
                if (crc32_check != *yuvcrc32_addr) {
                    crc32_check_err = 1;
                    printf("---> err in crc = 0x%x , org_crc = 0x%x , %s\n", crc32_check, (u32)*yuvcrc32_addr, name);
#if SAVE_FILE_YUVCRC32_ERR
                    sprintf(name, "%sYUV/%s%d_org_err.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                    if (!svfd) {
                        char output[128];
                        long_file_name_encode(name, output, sizeof(output));

                        svfd = fopen(output, "w+");
                    }
                    if (svfd) {
#if JPEG_DEC_PACKED_TILEBASE
                        ffwrite(yuv2, 1, len, svfd);
#else
                        ffwrite(yuv, 1, len, svfd);
#endif

#if !SAVE_ONE_FILE_YUV_ERR
                        fclose(svfd);
                        svfd = NULL;
                        printf("file close ok : %s\n", name);
#endif
                    } else {
                        printf("file open err : %s\n", name);
                    }
#endif
                } else {
#ifdef JPEG_DEC_OK_SAVE_FEILE
                    /* if (strstr(name, JPEG_DEC_OK_SAVE_FEILE)) { */
                    sprintf(name, "%sYUV/%s%d_OK.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                    if (!svfd) {
                        char output[128];
                        long_file_name_encode(name, output, sizeof(output));


                        svfd = fopen(output, "w+");
                    }
                    if (svfd) {
#if JPEG_DEC_PACKED_TILEBASE

                        ffwrite(yuv2, 1, len, svfd);
#else

                        ffwrite(yuv, 1, len, svfd);
#endif
                        fclose(svfd);
                        svfd = NULL;
                        printf("file close ok : %s\n", name);
                    }
                    /* } */
#endif
                }
            } else {
                printf("---> err in crc, yuvcrc32_addr = 0x%x file = %s \n", (u32)*yuvcrc32_addr, name);
            }

#if SAVE_FILE_YUV_OR_RGB
#if SAVE_FILE_YUV
            if (j < 7) { //大于1024，空间不足
                switch (ytype) {
                case 1:
                    YUV444pToYUV420p(yuv, rgb16, width, height);
                    break;//444
                case 4:
                    memcpy(rgb16, yuv, pix * 3 / 2);
                    break;//420
                default:
                    YUV422pToYUV420p(yuv, rgb16, width, height);
                    break;//422
                }
                //小于等于1024统一用YUV420文件格式
                if (!rgb_fd) {

                    sprintf(name, "%sYUV/%s%d.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                    char output[128];
                    long_file_name_encode(name, output, sizeof(output));


                    rgb_fd = fopen(output, "w+");
                }
                if (rgb_fd) {
                    ffwrite(rgb16, 1, pix * 3 / 2, rgb_fd);
                    printf("decode file name : %s, size = %d\n", name, pix * 3 / 2);
                }
#if SAVE_FILE_YUVCRC32_ERR
                if (crc32_check_err) {
                    sprintf(name, "%sYUV/%s%d_yuv420_err.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                    if (!svfd) {
                        char output[128];
                        long_file_name_encode(name, output, sizeof(output));


                        svfd = fopen(output, "w+");
                    }
                    if (svfd) {
                        ffwrite(rgb16, 1, pix * 3 / 2, svfd);
                        fclose(svfd);
                        svfd = NULL;
                        printf("file close ok : %s\n", name);
                    } else {
                        printf("file open err : %s\n", name);
                    }
                }
#endif
#else
            if (j < 7) { //大于720P，RGB空间不足
                switch (ytype) {
                case 1:
                    yuv444p_quto_rgb565(yuv, rgb16, width, height);
                    break;//444
                case 4:
                    yuv420p_quto_rgb565(yuv, rgb16, width, height);
                    break;//420
                default:
                    yuv422p_quto_rgb565(yuv, rgb16, width, height);
                    break;//422
                }
                //小于等于1024统一用RGB16文件格式
                if (!rgb_fd) {
                    sprintf(name, "%sYUV/%s%d.rgb", CONFIG_ROOT_PATH, &name_type[j], i);
                    char output[128];
                    long_file_name_encode(name, output, sizeof(output));


                    rgb_fd = fopen(output, "w+");
                }
                if (rgb_fd) {
                    ffwrite(rgb16, 1, pix * 2, rgb_fd);
                    printf("decode file name : %s, size = %d\n", name, pix * 2);
                }
#if SAVE_FILE_YUVCRC32_ERR
                if (crc32_check_err) {
                    sprintf(name, "%sYUV/%s%d_rgb_err.rgb", CONFIG_ROOT_PATH, &name_type[j], i);
                    if (!svfd) {
                        char output[128];
                        long_file_name_encode(name, output, sizeof(output));


                        svfd = fopen(output, "w+");
                    }
                    if (svfd) {
                        ffwrite(rgb16, 1, pix * 2, svfd);
#if !SAVE_ONE_FILE_YUV_ERR
                        fclose(svfd);
                        svfd = NULL;
#endif
                        printf("file close ok : %s\n", name);
                    } else {
                        printf("file open err : %s\n", name);
                    }
                }
#endif
#endif
            } else { //大于1024统一单YUV文件格式
                printf("decode file name : %s, size = %d\n", name, len);
                sprintf(name, "%sYUV/%s%d.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                char output[128];
                long_file_name_encode(name, output, sizeof(output));


                fd = fopen(output, "w+");
                if (fd) {
                    ffwrite(yuv, 1, len, fd);
                    fclose(fd);
                }
            }
#endif

free_buf:
            ;
#if !(CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
#if (!JPEG_DEC_BITBUF_SRAM && !JPEG_YUV_BIT_ADDR_ONE)
            if (buf) {
                free(buf);
                buf = NULL;
            }
#endif
            if (yuv) {
                free(yuv);
                yuv = NULL;
            }
            if (yuv2) {
                free(yuv2);
                yuv2 = NULL;
            }
#endif
        }
#if SAVE_FILE_CRC32
        sprintf(name, "%sYUV/%scrc32.txt", CONFIG_ROOT_PATH, &name_type[j]);
        char output[128];
        long_file_name_encode(name, output, sizeof(output));


        fd = fopen(output, "w+");
        if (fd) {
            k = 0;
            int ret;
            printf("yuvcrc_addr_cnt = %d \n", yuvcrc_addr_cnt);
            while (k <= yuvcrc_addr_cnt) {
                ret = sprintf(name, "0x%08x, ", yuvcrc_addr[k]);
                ffwrite(name, 1, ret, fd);
                if (++k % 10 == 0) {
                    ret = sprintf(name, "\r\n");
                    ffwrite(name, 1, ret, fd);
                }
            }
            fclose(fd);
        }
        yuvcrc_addr_cnt = 0;
        memset(yuvcrc_addr, 0, sizeof(yuvcrc_addr));
#endif
        if (svfd) {
            fclose(svfd);
            printf("file close ok : %s\n", name);
            svfd = NULL;
            os_time_dly(300);
        }
        if (rgb_fd) {
            fclose(rgb_fd);
            rgb_fd = NULL;
            printf("jpeg_dec save ok \n\n");
        }
        if (rgb16) {
            free(rgb16);
            rgb16 = NULL;
        }
    }
exit:
    ;
#if !(CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
    if (yuv) {
        free(yuv);
    }
#endif

    if (rgb16) {
        free(rgb16);
    }
    if (rgb_fd) {
        fclose(rgb_fd);
    }
    return  pix;//return value : width and height
}
#if 1
struct yuv_recv {
    volatile unsigned char *y;
    volatile unsigned char *u;
    volatile unsigned char *v;
    volatile int y_size;
    volatile int u_size;
    volatile int v_size;
    volatile int recv_size;
    volatile int size;
    volatile char complit;
    volatile int yuv_cb_cnt;
};
static int yuv_out_cb(void *priv, struct YUV_frame_data * p) {
    struct yuv_recv *rec = (struct yuv_recv *)priv;
    u8 type = (p->pixformat == VIDEO_PIX_FMT_YUV444) ? 1 : ((p->pixformat == VIDEO_PIX_FMT_YUV422) ? 2 : 4);
    ++rec->yuv_cb_cnt;
    if (!rec->complit) {
        /* printf("rec->y:%x   p->y:%x\n",rec->y+rec->y_size,p->y); */
        /* put_buf(p->y,32); */
        memcpy(rec->y + rec->y_size, p->y, p->width * p->data_height);
        memcpy(rec->u + rec->u_size, p->u, p->width * p->data_height / type);
        memcpy(rec->v + rec->v_size, p->v, p->width * p->data_height / type);
        rec->y_size += p->width * p->data_height;
        rec->u_size += p->width * p->data_height / type;
        rec->v_size += p->width * p->data_height / type;
        rec->recv_size +=  p->width * p->data_height + (p->width * p->data_height / type) * 2;
        if (rec->recv_size >= rec->size) {
            rec->complit = 1;
        }
    } else {
        printf("err in complit  = %d , type = %s \n", rec->complit, (type == 1) ? "YUV444" : (type == 2) ? "yuv422" : "YUV420");
    }
    return 0;
}
static void jpg_frame_dec_test(void) {
    extern void *jpg_dec_open(struct video_format * f);
    extern int jpg_dec_input_data(void *_fh, void *data, u32 len);
    extern int jpg_dec_set_output_handler(void *_fh, void *priv, int (*handler)(void *, struct YUV_frame_data *));
    extern int jpg_dec_get_s_attr(void *_fh, struct jpg_dec_s_attr * attr);
    extern int jpg_dec_set_yuv(void *_fh, void *yuv, u32 len);
    extern int jpg_dec_close(void *_fh);
    char name[128];
    u32 ok_cnt = 0;
    u32 crc32_check;
    char crc32_check_err;
    u32 *yuvcrc32_addr = NULL;
    u32 yuvcrc32[10] = {
        (u32)jpg_128file_crc32,
        (u32)jpg_160file_crc32,
        (u32)jpg_320file_crc32,
        (u32)jpg_480file_crc32,
        (u32)jpg_640file_crc32,
        (u32)jpg_960file_crc32,
        (u32)jpg_1024file_crc32,
        (u32)jpg_1920file_crc32,
        NULL,
    };
    u8 name_type[10][32] = {
        "128_160_sum/160_",
        "160_128_sum/160_",
        "320_240_sum/320_",
        "480_320_sum/480_",
        "640_480_sum/640_",
        "960_640_sum/960_",
        "1024_720_sum/1024_",
        "1920_1200_sum/1920_",
        0,
    };
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }
    int ret, i, j;
    FILE *svfd = NULL;
    FILE *fd = NULL;
    FILE *fdyuv = NULL;
    int cyc_cnt = 0;
    int cnt, lenth, pix, width, height, ytype, len;
    unsigned char *buf = NULL, *yuv = NULL;
    struct yuv_recv yuv_rec_data = {0};
    void *fh = jpg_dec_open(NULL);
    if (!fh) {
        printf("err in jpg_dec_open \n\n");
        return;
    }
    struct jpg_dec_s_attr jpg_attr;
    jpg_dec_get_s_attr(fh, &jpg_attr);
    jpg_attr.max_o_width  = 1920;
    jpg_attr.max_o_height = 1200;
    jpg_dec_set_s_attr(fh, &jpg_attr);
    jpg_dec_set_output_handler(fh, (void *)&yuv_rec_data, yuv_out_cb);
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
    jpg_dec_set_yuv(fh, (void *)yuv_buf, JPEG_YUV_SIZE);
#endif

    printf("---> jpg decoder start\n");
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
    printf("---> yuv_addr = 0x%x , bit_addr = 0x%x \n", yuv_addr, bit_addr);
#elif JPEG_DEC_BITBUF_SRAM
    printf("---> bit_addr = 0x%x \n", bit_addr);
#endif

#if CYC_FILE_DECODE
    for (j = CYC_FILE_NUM; cyc_cnt < FILE_DECODE_NUM; cyc_cnt++) {
#else
    cyc_cnt = FILE_DECODE_NUM;
    while (cyc_cnt--)
        for (j = JPEG_DEC_START_NUM; j <= JPEG_DEC_END_NUM && name_type[j][0] != 0; j++) {
#endif
        cnt = 0;
        printf("MCU file name : %s\n", &name_type[j]);
        for (i = 1; i <= 200; i++) {
            sprintf(name, "%s%s%d.jpg", CONFIG_ROOT_PATH, &name_type[j], i);
#if (!JPEG_DEC_DRI_FILE_ALSO || JPEG_DEC_DRI_FILE_ONLY)
            char find_dri = 0;
            for (int m = 0; jpg_dri_file[m][0] != 0; m++) {
                if (strstr(name, &jpg_dri_file[m])) {
                    find_dri = 1;
                    break;
                }
            }
#if JPEG_DEC_DRI_FILE_ONLY
            if (!find_dri) {
#else
            if (find_dri) {
#endif
                printf("no dec %s\n", name);
                continue;
            }
#endif
            fd = fopen(name, "r");
            if (fd == NULL) {
                /*printf("no file name : %s\n", name);*/
                cnt++;
                if (cnt > 3) {
                    break;
                }
                continue;
            }
            cnt = 0;
            crc32_check_err = 0;
            lenth = flen(fd);
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN) || JPEG_DEC_BITBUF_SRAM
            buf = bit_addr;
            if (lenth > sizeof(bit_addr)) {
                printf("err in buf no enough : %s\n", name);
                fclose(fd);
                goto free_buf;
            }
#else
            buf = malloc(lenth);
#endif
            if (!buf) {
                fclose(fd);
                printf("buf malloc err ...\n");
                goto free_buf;
            }
            if (ffread(buf, 1, lenth, fd) != lenth) {
                fclose(fd);
                printf("read file lenth err ...\n");
                goto free_buf;
            }
            fclose(fd);
            struct jpeg_image_info info = {0};
            info.input.data.buf = buf;
            info.input.data.len = lenth;
            if (jpeg_decode_image_info(&info)) {
                printf("jpeg_decode_image_info err %s\n", name);
                goto free_buf;
            }
            width = info.width;
            height = info.height;
            pix = width * height;
            switch (info.sample_fmt) {
            case JPG_SAMP_FMT_YUV444:
                puts("JPG_SAMP_FMT_YUV444\n");
                ytype = 1;
                break;//444
            case JPG_SAMP_FMT_YUV420:
                puts("JPG_SAMP_FMT_YUV420\n");
                ytype = 4;
                break;//420
            default:
                ytype = 2;
                puts("JPG_SAMP_FMT_YUV422\n");
                break;//422
            }
            len = pix + pix / ytype * 2;
            if (!yuv) {
#if (CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
                yuv = yuv_addr;
                if (len > sizeof(yuv_addr)) {
                    printf("err in yuv not enough \n\n");
                    goto free_buf;
                }
#else
                yuv = malloc(len + 4 * 1024);
#endif
                if (!yuv) {
                    printf("err no mem in yuv \n\n");
                    goto free_buf;
                }
            }
            yuv_rec_data.y = yuv;
            yuv_rec_data.u = yuv_rec_data.y + pix;
            yuv_rec_data.v = yuv_rec_data.u + pix / ytype;
            yuv_rec_data.size = len;
            yuv_rec_data.recv_size = 0;
            yuv_rec_data.y_size = 0;
            yuv_rec_data.u_size = 0;
            yuv_rec_data.v_size = 0;
            yuv_rec_data.complit = 0;
            yuv_rec_data.yuv_cb_cnt = 0;

            unsigned char *check_buf = (unsigned char *)(yuv + len);
            memset(check_buf, CHECK_MAG, 4 * 1024);
            /* DcuFlushinvRegion((void *)((u32)check_buf / 32 * 32), 4 * 1024); */

            /*putchar('@');*/
            ret = jpg_dec_input_data(fh, buf, lenth);
            if (ret) {
                printf("jpg_dec_input_data waite err\n");
                printf("err decode cnt = %d, file name : %s\n", yuv_rec_data.yuv_cb_cnt, name);
                printf("err yuv_rec_data.complit , size=%d, recv_size=%d\n\n", yuv_rec_data.size, yuv_rec_data.recv_size);
                goto free_buf;
            }
            //printf("yuv_cb_cnt=%d\n", yuv_rec_data.yuv_cb_cnt);
            if (!yuv_rec_data.complit) {
                printf("err decode cnt = %d, file name : %s\n", yuv_rec_data.yuv_cb_cnt, name);
                printf("err yuv_rec_data.complit , size=%d, recv_size=%d\n\n", yuv_rec_data.size, yuv_rec_data.recv_size);
                goto free_buf;
            }

            //CRC32数字校验YUV
            crc32_check = CRC32(yuv, len);
            yuvcrc32_addr = (u32 *)((u32)yuvcrc32[j] + i * sizeof(u32));
            printf("---> crc=0x%x ,  org_crc=0x%x \n", crc32_check, *yuvcrc32_addr);
            if (yuvcrc32_addr && *yuvcrc32_addr != 0) {
                if (crc32_check != *yuvcrc32_addr) {
                    crc32_check_err = 1;
                    printf("---> err in crc = 0x%x , org_crc = 0x%x , %s\n", crc32_check, (u32)*yuvcrc32_addr, name);
#if SAVE_FILE_YUVCRC32_ERR
                    sprintf(name, "%sYUV/%s%d_org_err.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                    char output[256] = {0};
                    printf("name : %s\n", name);
                    long_file_name_encode(name, output, sizeof(output));

                    if (!svfd) {
                        svfd = fopen(output, "w+");
                    }
                    if (svfd) {
                        ffwrite(yuv, 1, len, svfd);
                        fclose(svfd);
                        svfd = NULL;
                        printf("file close ok : %s\n", name);
                    } else {
                        printf("file open err : %s\n", name);
                    }
#endif
                }
            } else {
                printf("---> err in crc, yuvcrc32_addr = 0x%x file = %s \n", (u32)*yuvcrc32_addr, name);
            }

            for (int k = 0; k < 4 * 1024; k++) {
                if (check_buf[k] != CHECK_MAG) {
                    printf("---> err in decode image frame ,check_buf[%d]=0x%x, yuv=0x%x,check_buf=0x%x\n", k, check_buf[k], yuv, check_buf);
                    put_buf(&check_buf[k], 32);
                    break;
                }
            }
            if (yuv_rec_data.recv_size != yuv_rec_data.size) {
                printf("err yuv_rec_data.recv_size \n\n");
                goto free_buf;
            }
            /*printf("name=%s\n",name);*/
            ok_cnt++;
            if (ok_cnt % 100 == 0) {
                putchar('&');
            }
#if SAVE_FILE_YUV_OR_RGB
            switch (ytype) {
            case 1:
                yuv_rec_data.recv_size = YUV444pToYUV420p(yuv, yuv, width, height);
                break;//444
            case 2:
                yuv_rec_data.recv_size = YUV422pToYUV420p(yuv, yuv, width, height);
                break;//422
            }
            if (!fdyuv) {
                sprintf(name, "%sYUV/%s%d.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                fdyuv = fopen(name, "wb+");
            }
            if (fdyuv) {
                ffwrite(yuv, 1, yuv_rec_data.recv_size, fdyuv);
                /*printf("decode file name : %s, size = %d\n", name, yuv_rec_data.recv_size);*/
            }
#if SAVE_FILE_YUVCRC32_ERR
            if (crc32_check_err) {
                sprintf(name, "%sYUV/%s%d_yu420_err.yuv", CONFIG_ROOT_PATH, &name_type[j], i);
                if (!svfd) {
                    svfd = fopen(name, "wb+");
                }
                if (svfd) {
                    ffwrite(yuv, 1, yuv_rec_data.recv_size, svfd);
                    fclose(svfd);
                    svfd = NULL;
                    printf("file close ok : %s\n", name);
                } else {
                    printf("file open err : %s\n", name);
                }
            }
#endif
#endif
free_buf:
            ;
#if !(CYC_FILE_DECODE && CYC_FILE_BUF_SRAM_EN)
#if !JPEG_DEC_BITBUF_SRAM
            if (buf) {
                free(buf);
                buf = NULL;
            }
#endif
            if (yuv) {
                free(yuv);
                yuv = NULL;
            }
#endif
        }
        if (fdyuv) {
            fclose(fdyuv);
            fdyuv = NULL;
            printf("jpeg_dec save ok \n\n");
        }
    }
    if (fd) {
        fclose(fd);
    }
    if (fdyuv) {
        fclose(fdyuv);
    }
    if (fh) {
        jpg_dec_close(fh);
    }
}
#endif
void jpeg_decode_test(void) {
    jpeg_codec_init();
#if JPEG_DEC_MUC_TEST
    thread_fork("jpg_frame_dec_test", 29, 1024 * 4, 64, 0, jpg_frame_dec_test, NULL);
#else
    thread_fork("jpeg_decoder_more", 29, 1024 * 4, 64, 0, jpeg_decoder_more, NULL);
#endif
}

/**********************************************所有分辨率编码测试,q=12*******************************************************/

static const u32 jpeg_128_crc32[] = {
    0,
};
static const u32 jpeg_160_crc32[] = {
    0x0a36a641, 0x69e6e28c, 0x1e208ede, 0x98ef1df8, 0x49a713fa, 0x0f131524, 0x19de83dd, 0x959bc8d4, 0x8ce2a195, 0x34f21cad,
    0xf5b0638e, 0xb7ca9fa7, 0x2849eabf, 0x885fbb36, 0x4890e6d5, 0x388982f2, 0x6c11d4cd, 0xe0a9abf6, 0xe94204bb, 0xa0bec036,
    0x9e48282a, 0x9bbcedb6, 0xdd2006f8, 0xaad8f65e,

    0,
};
static const u32 jpeg_320_crc32[] = {
    0xc0f40f0b, 0x042eb529, 0xb1ae261f, 0x465165b4, 0x58bd6fb4, 0x9ace9585, 0xc7370aab, 0xd04f7994, 0x6586aa2a, 0x7819d080,
    0x169dd34a, 0xe7809cb8, 0x6769e6ff, 0xe699509c, 0x04b7950f, 0x2cc403c9, 0x21cbfbb8, 0xa271ef53, 0x94025fb7, 0x1916872d,
    0x111fa94d, 0x96650942, 0x4440be69, 0xa83335a1, 0xa1987ec5, 0x909724b2, 0x6bbdf543, 0x9aeb9993, 0x493c6011, 0x92a04eba,
    0x76bea74a, 0xff4ba16b, 0x53130f11, 0xba2b0a65, 0x1c30d4fe, 0x8b8cf9e3, 0x8bb2c664, 0x07bafbb5, 0xfb021c5b, 0x16be8d6a,
    0xff1ec43b, 0x3bd3ce7a, 0x5a4e5b05, 0x01a9b454, 0x89e9c3a8, 0xa7dc225f, 0xea71b669, 0x610ced29, 0xe6d4f514, 0x9b1b572a,
    0xa759c180, 0xa92e1720, 0xb94d1b79, 0xf0e7d67a, 0x2e1bcf28, 0x18b8b098, 0x342d7608, 0xc63e8168, 0x39d05ff7, 0xeb13ff9a,
    0x8a143de8, 0x5ab6b3c2, 0xc98895c0, 0xba898e7f, 0x8db8975f, 0xaf85dbff, 0x0c4d94e3, 0xa1200858, 0x922f3634, 0x517eeb58,
    0x742da06e, 0x7946d653, 0x96fa4ae3, 0x39b4f6a1, 0xeda68b64, 0x1efedee1, 0x6ca1a4f7, 0xaf72f2ee, 0x21990ea6, 0x4ffebff7,
    0x47c71e55, 0xf33b729e, 0xb0800e82, 0xa53eb691, 0x78ab5e79, 0x9bcf4924, 0xf94de95e, 0x84a7830b, 0x11c2bba8, 0x48f00d67,
    0x2c4c55b3, 0xbe05d5c0, 0x07ae45fe, 0xdb164f29, 0x12bc985f, 0xc6db2d2b, 0x4fdf3295, 0xad7466f9, 0xf657ae81, 0xff69e9bb,

    0,
};
static const u32 jpeg_480_crc32[] = {
    0xea374c98, 0xe2dc8a7a, 0x64d755db, 0x963ff631, 0xe55f4737, 0xcacc80ac, 0x6d64e55a, 0xaa80bfa0, 0x8232f902, 0x7fe3a53c,
    0x7b81dc64, 0x2d039c8e, 0x14fa34b2, 0x78e6c314, 0x2d1a2587, 0xf7898933, 0xe60d304b, 0x26b6efec, 0x15c43577, 0xb8cdc246,
    0xbd47c12b, 0xec0ca9f3, 0x2693d19a, 0x3a865111, 0x1a746fbd, 0xc5daadfb, 0x3e075e8f, 0x4d8bda05, 0x78c23aaf, 0xfd899fac,
    0x6ef74da5, 0x5c918466, 0x82e513e5, 0xd644227f, 0x048e7dbb, 0x7247adf9, 0xeae18b5b, 0x0d6098e1, 0x11e23a24, 0x11a0e7ea,
    0x36ca0885, 0x6938c577, 0xc10c21eb, 0x80e27144, 0x1a0a10a9, 0x0099973d, 0xc0135568, 0x67dd4ac1, 0xcf5578d3, 0xf9d106bb,
    0x9fa77d6b, 0x6630ec7a, 0xb8907c29, 0x84469ed7, 0x3acda20c, 0x4ade8ea0, 0xace7721f, 0x2ba008dd, 0x78ddc64c, 0xb3265a8d,
    0x7fdda305, 0x00cd45f9, 0x519bda3d, 0xc08e812c, 0xb743bf49, 0xc2d8a33a, 0xb208a381, 0x809f8fb1, 0x81e03eda, 0xede5fb54,
    0x4221e334, 0x5c50e73f, 0x71ad6d00, 0x7ad0e1b3, 0x717a9ffb, 0xe80550a2, 0x4bad27de, 0x56a2bc5c, 0xe0905e74, 0xdc682647,
    0x4ea20a4e, 0x4012cc84, 0x517e495a, 0xda53ac4c, 0x70db0b35, 0xee3dd5d4, 0x8e5e80de, 0xa054a92b, 0x21c9d36d, 0x2efbc155,
    0xcd4ae050, 0x2f2995a1, 0x82e513e5, 0x691008ef, 0xaf427acb, 0x05fd333d, 0x14999388, 0x07905d93, 0x027d5b3b, 0xc19bbc57,
    0xa884e1ff, 0x5ae13655, 0x0ea0a7de, 0xee181fe0, 0x446b1044, 0xe77b7cbe, 0x2e25c483, 0xe8d630fa,

    0,
};
static const u32 jpeg_640_crc32[] = {

    0xadf22d3f, 0x7a60cfa7, 0x51a79b78, 0x43b7a570, 0xf661c881, 0x53b0deb1, 0xcc624cca, 0x3ed4875a, 0xc64171d2, 0x463da956,
    0x506e279a, 0x3f7d789a, 0x0313434a, 0x95871ea9, 0x5d7834f0, 0x0142982f, 0x43b059d1, 0xfd15c4e0, 0x7c5b4260, 0x4006c169,
    0x9b890aa4, 0xd5042317, 0x44c5edbc, 0x7496e986, 0x3d338200, 0x9f182516, 0x23f31ea0, 0x3276c81e, 0xb32fd550, 0x4afa3c6b,
    0x4deb5f4f, 0xad70ae07, 0xe560472d, 0x2aa2c1f0, 0xf0089e55, 0xcec7c09a, 0x1f44a97b, 0xbb67b595, 0x22f9c59b, 0x51d3466f,
    0xe1bf7311, 0xe1abefce, 0x01a3da0b, 0x13c3a24b, 0x3499dce0, 0x15619281, 0xaa8132ed, 0x5d450594, 0x372a37a8, 0x8fd18a01,
    0x486445fb, 0x2ec1ed0f, 0xd4573dfc, 0x1b6a7520, 0xeed11b15, 0x364197de, 0xfd15c4e0, 0x43b64960, 0x4deb5f4f, 0x9121ccce,
    0x6bb8c77d, 0x97712d32, 0x6d95169b, 0x38fb37bc, 0xc6fc01a5, 0x97261e06, 0xd7b2e6e7, 0xe50f0509, 0x0978f337, 0x02465818,
    0x489b3941, 0x1e2428ef, 0x0e2a2aa3, 0x05836cbc, 0x51629eec, 0x07fc210a, 0x16c9e7e0, 0xf126815b, 0xc26ab6ae, 0x20cdea12,
    0x110c9fce, 0xdcac7aff, 0x50429770, 0x9a131f67, 0x603ea01c, 0x4e91a7ef, 0x766612e3, 0x0355c0bb, 0xc2b57585, 0x3c323c06,
    0xbb33ff49, 0xb4c6214d, 0xce8b96f0, 0x7270a6f5, 0x4a9a545a, 0x0142b8e6, 0x149c2200, 0x0ccd907e, 0x950caf73, 0x09d5ad67,
    0x3f7d789a, 0x344840f4, 0x62c4003a, 0xabab023d, 0xd1766149, 0x86856066, 0xa399709b, 0xe0e46935, 0x72bdc181, 0xb5cf0c2f,
    0xfe4e1aea,

    0,
};
static const u32 jpeg_960_crc32[] = {


    0xa8d552f1, 0xfb0a366f, 0xb4746024, 0x18c92f19, 0x65ae7cc7, 0xeba5bc8f, 0x0c931cab, 0xd9d3d1ae, 0x3844f26e, 0xc9bb9449,
    0x4047c3a2, 0x96608aa7, 0x8e40112e, 0x48b9e6e2, 0x3ec0815b, 0x7d6572fa, 0xa70a8295, 0x031744d3, 0x3cf0a1be, 0xceeb32f3,
    0xd3345d4b, 0xe0c46584, 0x4f197843, 0xdaefd253, 0x2cd43b3c, 0x229fbf28, 0x22b72b36, 0x5cad04a7, 0xb14c3558, 0x3a8d4eb0,
    0x2b4a5eb3, 0x7dd5161e, 0x1a22e9ef, 0x5dc30830, 0xb6579eed, 0xf55e7629, 0x49f1cdb6, 0x735f3da5, 0x1a8647d7, 0x9fcd2494,
    0xd54d47bc, 0x39ac933b, 0x09395687, 0xdc49a07f, 0x86e289f1, 0x57bbd449, 0xd1d64189, 0xd1cbfef6, 0x2c0eba9a, 0xa0950549,
    0xbd1ada93, 0x0bb5a51c, 0x3b84c431, 0x250503a6, 0x5d950077, 0xdff7f1d2, 0x22080f75, 0x82ee06cd, 0xf2bdf076, 0x514f753f,
    0x136415e2, 0x5e556075, 0x1c8dea51, 0x3b628599, 0xa7e14b6b, 0xf805025c, 0x6cabad17, 0x8151a893, 0x08080c03, 0x10389881,
    0x11b08347, 0x7aa3ed6f, 0x9293b37c, 0x2491cdfc, 0xfa714104, 0xfca75e17, 0x87583318, 0xb7ea28c8, 0xc3985006, 0x8f65faf3,
    0xb9679efb, 0xe7d74775, 0x438e509d, 0x64afb865, 0xaa3524a2, 0x66802f9c, 0x0c514d3f, 0xb4fbbf6a, 0x9a33ea28, 0x04db9937,
    0x7737f309, 0x6f25b574, 0xad242a51, 0xec2b803e, 0x6c927524, 0x69a7ec22, 0xf802b769, 0x639c796e, 0x0d69a939, 0x72db347c,

    0,
};
static const u32 jpeg_1024_crc32[] = {

    0x23878fc2, 0xef10924e, 0x28534d4a, 0x61975d5d, 0x5748c6ed, 0x190c007b, 0x05d6ab31, 0x1f8a58ee, 0xab21f39a, 0xeb632d65,
    0x54215552, 0x774f84bb, 0x44d5c9a2, 0xf03576d4, 0xd5f41f9e, 0x6c89ccac, 0xe4dcb1a4, 0xa25b5c18, 0xfb73ca21, 0x3232119a,
    0x281adb06, 0x45b35c4b, 0xc46d44d1, 0xb904b006, 0xa79a2bf3, 0x79358bbe, 0x2d5c62ae, 0x60430623, 0xaf7f39dc, 0xc6e5a678,
    0x2bcc5c80, 0x220ea020, 0xac85efb1, 0x9b33df56, 0xa727958a, 0xb408bf26, 0x34d210d7, 0x89e5f9d9, 0x1d0dec37, 0x96eea848,
    0x0bd9e704, 0xfb73ca21, 0xc46d44d1, 0xaf372ebb, 0x2545d97e, 0x3ce61715, 0x19dcd6fc, 0xa7691843, 0x4e2500ef, 0x7cf4c7c2,
    0x9bd58623, 0x1066291d, 0x4cd00ea1, 0x7278af61, 0xc9a2a66c, 0x2c3b9417, 0x818df73f, 0x4c5c33c1, 0x832facd9, 0x25627c51,
    0xa69b373a, 0x8fce8072, 0xaebf77d2, 0x79d49ccc, 0x10d114c3, 0xe853d0b5, 0x81e3d71c, 0xe2e50182, 0x7b00f9f2, 0xfe0e2f41,
    0xe2081e20, 0xa418c959, 0x37720353, 0x6e027600, 0x9bc9c0d6, 0x3180b3d8, 0xa7b7ca6e, 0xcb279903, 0x11f321f0, 0xb910a2c7,
    0xad96ca02, 0x3b088e48, 0x34da2f9a, 0xeb9d85e1, 0x1ff8d9e6, 0x79e9434d, 0x1d5a2667, 0xd3f105c0,

    0,
};





static int jpeg_encode_task(void) {
    int err;
    int ret;
    int i, j, k, m;
    char name[128];
    char pbuf[128];
    int width = 640;
    int height = 480;
    int pix = width * height;
    int yuv_size = pix * 3 / 2;
    int jpg_size = 300 * 1024;
    FILE *fd_org = NULL;
    FILE *fd_crc = NULL;
    FILE *fd = NULL;
    u32 crc32_check;
    u32 cyc_cnt = 0;
    int retry = 0;
    u32 cnt = 0;
    u32 fcnt = 0;
    int fflen;
    int read_len;
    u32 err_cnt = 0;
    char *yuvbuf;
    char *jpg_yuvbuf;
    char ql = 0;
    u32 *yuvcrc32_addr = NULL;
    u16 pix_type[10][2] = {
        128, 160,
        160, 128,
        320, 240,
        480, 320,
        640, 480,
        960, 640,
        1024, 720,
        1920, 1200,
        0, 0,
    };
    u32 yuvcrc32[10] = {
        (u32)jpeg_128_crc32,
        (u32)jpeg_160_crc32,
        (u32)jpeg_320_crc32,
        (u32)jpeg_480_crc32,
        (u32)jpeg_640_crc32,
        (u32)jpeg_960_crc32,
        (u32)jpeg_1024_crc32,
        NULL,
    };
    u8 name_type[10][32] = {
        "128_160_sum/160_",
        "160_128_sum/160_",
        "320_240_sum/320_",
        "480_320_sum/480_",
        "640_480_sum/640_",
        "960_640_sum/960_",
        "1024_720_sum/1024_",
        "1920_1200_sum/1920_",
        0,
    };
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }
    char *jpg_img = malloc(jpg_size);
    if (!jpg_img) {
        printf("jpg img malloc err!!!\n");
        return 0;
    }
#if FILE_ENCODE_NUM
    cyc_cnt = FILE_DECODE_NUM;
#endif
    while (cyc_cnt--) {
        for (i = 0; i < 10; i++) {
            if (name_type[i] == 0 || pix_type[i][0] == 0) {
                i = 0;
                if (!cyc_cnt) {
                    goto exit;
                }
            }
            width = pix_type[i][0];
            height = pix_type[i][1];
            pix = width * height;
            yuv_size = pix * 3 / 2;
            yuvbuf = malloc(yuv_size);
            if (!yuvbuf) {
                i = 0;
                printf("yuvbuf malloc err in i=%d, %d x %d!!!!\n", i, width, height);
                if (!cyc_cnt) {
                    goto exit;
                }
                continue;
            }
            sprintf(name, "%s%sYUV420.YUV", CONFIG_ROOT_PATH, &name_type[i]);
            /* int long_file_name_encode(const char *input, u8 *output, u32 output_buf_len); */
            char output[128];
            long_file_name_encode(name, output, sizeof(output));
            printf("name : %s\n", output);
            retry = 0;
            if (!fd_org) {
                fd_org = fopen(output, "r");
                if (!fd_org) {
                    free(yuvbuf);
                    yuvbuf = NULL;
                    continue;
                }
            }
            fflen = flen(fd_org);
            read_len = 0;
            fcnt = 0;
#if SAVE_FILE_CRC32
            sprintf(name, "%sJPEG_TEST/%d_%d/crc32.txt", CONFIG_ROOT_PATH, width, height);
            fd_crc = fopen(name, "w+");
#endif
            while (read_len < fflen) {
                if (ffread(yuvbuf, 1, yuv_size, fd_org) != yuv_size) {
                    printf("err in read from file, need read %d \n", yuv_size);
                    goto free_data;
                }
                read_len += yuv_size;
                fcnt++;
                for (j = 12; j < 13; j++) {
                    memset(jpg_img, 0, jpg_size);
                    /*memset(yuvbuf + pix, 0x80, yuv_size - pix);//只编码Y*/
                    struct jpeg_encode_req req = {0};
                    req.format = JPG_SAMP_FMT_YUV420;
                    req.data.buf = jpg_img;
                    req.data.len = jpg_size;
                    req.width = width;
                    req.height = height;
                    req.y = yuvbuf;
                    req.u = req.y + pix;
                    req.v = req.u + pix / 4;
                    /*req.q = ql++;*/
                    req.q = j;

                    ql = ql > 12 ? 0 : ql;

                    crc32_check = 0;
                    err = jpeg_encode_one_image(&req);
                    if (!err) {
                        printf("jpeg_encode_one_image ok \n\n");
                        crc32_check = CRC32(jpg_img, req.data.len);
                        yuvcrc32_addr = (u32 *)((u32)yuvcrc32[i] + (fcnt - 1) * sizeof(u32));
                        printf("crc32_check = 0x%x, 0x%x\n", crc32_check, *yuvcrc32_addr);
                        if (yuvcrc32_addr && *yuvcrc32_addr != 0) {
                            if (crc32_check != *yuvcrc32_addr) {
                                printf("err in crc32_check = 0x%x, 0x%x\n", crc32_check, *yuvcrc32_addr);
#if SAVE_FILE_YUVCRC32_ERR
                                sprintf(name, "%sJPEG_TEST/%d_%d/jpeg_%d_crc32err%d.jpg", CONFIG_ROOT_PATH, width, height, fcnt, err_cnt++);
                                char output[128];
                                long_file_name_encode(name, output, sizeof(output));
                                fd = fopen(output, "w+");
                                if (fd) {
                                    ffwrite(req.data.buf, 1, req.data.len, fd);
                                    fclose(fd);
                                    printf("file close ok : %s\n", name);
                                } else {
                                    printf("file open err : %s\n", name);
                                }
#endif
                            }
                        }
                        cnt++;
                        if (cnt % 100 == 0) {
                            putchar('K');
                        }
#if SAVE_FILE_YUV_OR_RGB
                        /*sprintf(name, "%sJPEG_TEST/%d_%d/f%d/jpeg_%d.jpg", CONFIG_ROOT_PATH,width,height, fcnt,req.q);*/
                        sprintf(name, "%sJPEG_TEST/%d_%d/jpeg_%d.jpg", CONFIG_ROOT_PATH, width, height, fcnt);
                        fd = fopen(name, "w+");
                        if (fd) {
                            printf("save name : %s\n", name);
                            ffwrite(req.data.buf, 1, req.data.len, fd);
                            ffwrite(&crc32_check, 1, 4, fd);
                            fclose(fd);
                        }
#endif
                    } else {
                        printf("encode req.q = %d err \n\n", req.q);
                    }
                }
#if SAVE_FILE_CRC32
                if (fd_crc) {
                    memset(pbuf, 0, sizeof(pbuf));
                    sprintf(pbuf, "0x%08x, ", crc32_check);
                    ffwrite(pbuf, 1, strlen(pbuf), fd_crc);
                    if (fcnt % 10 == 0) {
                        sprintf(pbuf, "\r\n");
                        ffwrite(pbuf, 1, strlen(pbuf), fd_crc);
                    }
                }
#endif
            }
free_data:
            if (fd_org) {
                fclose(fd_org);
                fd_org = NULL;
            }
            if (yuvbuf) {
                free(yuvbuf);
                yuvbuf = NULL;
            }
#if SAVE_FILE_CRC32
            if (fd_crc) {
                fclose(fd_crc);
                fd_crc = NULL;
            }
#endif
        }
    }
exit:
    if (jpg_img) {
        free(jpg_img);
        jpg_img = NULL;
    }
    return 0;
}
static const u32 yuv640_testcrc32[][2] = {
    0, 0x4c9d9222,
    1, 0x285d48bc,
    2, 0x79a13e4d,
    3, 0x52096587,
    4, 0x3b11697d,
    5, 0xba320f43,
    6, 0x49443e8a,
    7, 0x8145305e,
    8, 0xdc222639,
    9, 0xd8a79158,
    10, 0x79fc81f7,
    11, 0x58836699,
    12, 0xd4f3916d,
};



static int jpeg_encode_yuv640_task(void) {
    int err;
    char name[128];
    int width = 640;
    int height = 480;
    int pix = width * height;
    int yuv_size = pix * 3 / 2;
    int jpg_size = 100 * 1024;
    FILE *fd = NULL;
    u32 cnt = 0;
    u32 err_cnt = 0;
    char *yuvbuf;
    char *jpg_yuvbuf;
    char ql = 0;
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }
    char *jpg_img = malloc(jpg_size);
    if (!jpg_img) {
        printf("jpg img malloc err!!!\n");
        return 0;
    }
    yuvbuf = malloc(yuv_size);
    if (!yuvbuf) {
        printf("yuvbuf malloc err !!!!\n");
        return 0;
    }
    sprintf(name, "%sYUV-640.YUV", CONFIG_ROOT_PATH);
    printf("name : %s\n", name);
    if (!fd) {
        fd = fopen(name, "rb");
    }
    if (fd) {
        ffread(yuvbuf, 1, yuv_size, fd);
        fclose(fd);
    } else {
        printf("file open err !!!\n\n");
        return 0;
    }
    for (;;) {
        memset(jpg_img, 0, jpg_size);
        /*memset(yuvbuf + pix, 0x80, yuv_size - pix);//只编码Y*/
        struct jpeg_encode_req req = {0};
        req.format = JPG_SAMP_FMT_YUV420;
        req.data.buf = jpg_img;
        req.data.len = jpg_size;
        req.width = width;
        req.height = height;
        req.y = yuvbuf;
        req.u = req.y + pix;
        req.v = req.u + pix / 4;
        req.q = ql++;

        ql = ql > 12 ? 0 : ql;

        err = jpeg_encode_one_image(&req);
        if (!err) {
            printf("jpeg_encode_one_image ok \n\n");
            /* put_buf(jpg_img,32); */
            u32 crc32_check = CRC32(jpg_img, req.data.len);
            if (crc32_check != yuv640_testcrc32[req.q][1]) {
                printf("err in crc32_check = 0x%x, 0x = 0x%x, req.q=%d \n", crc32_check, yuv640_testcrc32[req.q][1], req.q);
                sprintf(name, "%sJPEG_TEST/%d_%d.jpg", CONFIG_ROOT_PATH, req.q, err_cnt++);
                printf("fname:%s\n", name);
                fd = fopen(name, "w+");
                if (fd) {
                    printf("save name : %s\n", name);
                    ffwrite(req.data.buf, 1, req.data.len, fd);
                    fclose(fd);
                }
            }
            /*printf("crc32_check = 0x%x, q = %d \n",crc32_check,req.q);*/
            cnt++;
            if (cnt % 100 == 0) {
                putchar('K');
                /*break;*/
            }
#if 0
            sprintf(name, "%sJPEG_TEST/jpeg_%d.jpg", CONFIG_ROOT_PATH, cnt);
            fd = fopen(name, "w+");
            if (fd) {
                printf("save name : %s\n", name);
                ffwrite(req.data.buf, 1, req.data.len, fd);
                fclose(fd);
            }
            if (cnt > 14) {
                break;
            }
#endif
        } else {
            printf("encode err \n\n");
        }
    }
    return err;
}

int jpeg_encode_test(void) {

    jpeg_codec_init();
#if FILE_ENCODE_ONLY_YUV640
    return thread_fork("jpeg_encode_yuv640_task", 29, 1024 * 4, 64, 0, jpeg_encode_yuv640_task, NULL);
#else
    return thread_fork("jpeg_encode_task", 29, 1024 * 4, 64, 0, jpeg_encode_task, NULL);
#endif
}
