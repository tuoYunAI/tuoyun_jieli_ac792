#include "app_config.h"
#if TCFG_LCD_ENABLE
#include "asm/dmm.h"
#include "fs/fs.h"
#include "asm/gpcrc.h"

#define MAX_SCENES 4

#define scene1 0
#define scene2 1
#define scene3 2
#define scene4 3

/*添加新的环境参数路径*/
static char scenes_bin_path[MAX_SCENES][64] = {
    "mnt/sdfile/res/cfg/lcd_cfg_0.bin",
    "mnt/sdfile/res/cfg/lcd_cfg_1.bin",
    "mnt/sdfile/res/cfg/lcd_cfg_2.bin",
    "mnt/sdfile/res/cfg/lcd_cfg_3.bin",
};
typedef struct coordinates {
    int x[3];
    int y[3];
} coordinates;
#pragma pack(1)
typedef struct {//ccm+gamma+rbs+blend
    dmm_gamma_param_t rgb_gamma_param; // rgb gamma参数
    dmm_ccmf_param_t ccmf_param;  	   // ccm矩阵
    float rbs; 				       // in_swp:r分量和b分量需要对调
    float r_gain; 		//r分量增益 (0.00-2.00, 步进0.01)
    float g_gain; 		//g分量增益 (0.00-2.00, 步进0.01)
    float b_gain; 		//b分量增益 (0.00-2.00, 步进0.01)
    float bright_gain;	//亮度 (0.00-4.00, 步进0.01)
    float contrast_gain;//对比度 (0.00-4.00, 步进0.01)
    float mean;			//对比度均值 (0.0-255.00, 步进1)
    float saturation_gain;//饱和度 (0.00-4.00, 步进0.01)
    float angle;		//色相 (0.00-360.00, 步进1)
    int line_type;      //曲线类型(0:standard 1:3 points  2:Bezier 3:Multi 4:Custom)
    coordinates coordinates_; //伽马曲线坐标
} _dmm_param;
#pragma pack()

static _dmm_param *scenes[MAX_SCENES];
static u8 lcd_scenes_init_flag = 0;

static const gpcrc_algorithm_cfg_t crc_algorithm[] = {
    /* crc_name             init        poly            rev_in          rev_out           pol_size   xorout */
    /* {"CRC-7-MMC",           0x00,       0x09,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL7,   0x00      }, */
    /* {"CRC-8",               0x00,       0x07,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL8,   0x00      }, */
    /* {"CRC-8-ITU",           0x00,       0x07,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL8,   0x55      }, */
    /* {"CRC-8-ROHC",          0xFF,       0x07,       GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL8,   0x00      }, */
    /* {"CRC-8-MAXIM",         0x00,       0x31,       GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL8,   0x00      }, */
    /* {"CRC-16-IBM",          0x0000,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-MAXIM",        0x0000,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    /* {"CRC-16-USB",          0xFFFF,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    {"CRC-16-MODBUS",       0xFFFF,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    },
    /* {"CRC-16-CCITT",        0x0000,     0x1021,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-CCITT-FALSE",  0xFFFF,     0x1021,     GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-X25",          0xFFFF,     0x1021,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    /* {"CRC-16-XMODEM",       0x0000,     0x1021,     GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-XMODEM2",      0x0000,     0x8408,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    }, */
    /* {"CRC-16-DNP",          0x0000,     0x3D65,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    }, */
    /* {"CRC-32",              0xFFFFFFFF, 0x04C11DB7, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF}, */
    /* {"CRC-32-C",            0xFFFFFFFF, 0x1EDC6F41, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF}, */
    /* {"CRC-32-KOOPMAN",      0xFFFFFFFF, 0x741B8CD7, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF}, */
    /* {"CRC-32-MPEG-2",       0xFFFFFFFF, 0x04C11DB7, GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL32,  0x00000000}, */
};

void lcd_scenes_init(void)
{
    /* const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[8]; */
    const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[0];
    if (lcd_scenes_init_flag) {
        return;
    }
    for (int i = 0; i < MAX_SCENES; i++) {
        void *fd;
        fd = fopen(scenes_bin_path[i], "r");
        if (fd) {
            int len = flen(fd);
            if (len != (sizeof(_dmm_param) + sizeof(u32))) {
                printf("lcd scenes file is not intact!");
                fclose(fd);
                continue;
            }
            char *buf = malloc(len);
            fread(buf, len, 1, fd);

            if (buf[0] != 'J' || buf[1] != 'L') {
                printf("---- lcd scenes bin file frame header error ----");
                fclose(fd);
                free(buf);
                buf = NULL;
                fd = NULL;
                continue;
            }

            u16 crc_result = gpcrc_calc_u8_buf((gpcrc_algorithm_cfg_t *)algo_cfg, buf, len - 2);
            u16 value;
            memcpy(&value, buf + len - 2, sizeof(u16));
            if (crc_result != value) {
                printf("---- crc is not equality----");
                fclose(fd);
                free(buf);
                buf = NULL;
                fd = NULL;
                continue;
            }
            scenes[i] = (_dmm_param *)malloc(sizeof(_dmm_param));
            if (!scenes[i]) {
                printf("----malloc error-----");
                continue;
            }
            memcpy(scenes[i], buf + 2, sizeof(_dmm_param));
            fclose(fd);
            free(buf);
            buf = NULL;
            fd = NULL;
        } else {
            printf("%s file is not exist!\n", scenes_bin_path[i]);
        }
    }
}

void lcd_scenes_switch(int model)
{
    if (lcd_scenes_init_flag == 0) {
        lcd_scenes_init();
        lcd_scenes_init_flag = 1;
    }

    if (model > MAX_SCENES) {
        printf("This Scene param is not set!");
        return;
    }
    if (scenes[model] == NULL) {
        printf("----error! This scene param is NULL !-------");
        return;
    }
    switch (model) {
    case scene1:
    case scene2:
    case scene3:
    case scene4:
        if (scenes[model]->rbs == 1) {
            set_final_ccm_param(scenes[model]->b_gain, scenes[model]->g_gain, scenes[model]->r_gain, scenes[model]->bright_gain, scenes[model]->contrast_gain, \
                                scenes[model]->saturation_gain, scenes[model]->mean, scenes[model]->angle, 0, &scenes[model]->ccmf_param);
            set_dmm_rgb_gamma_param(&scenes[model]->rgb_gamma_param);
        } else {
            set_final_ccm_param(scenes[model]->r_gain, scenes[model]->g_gain, scenes[model]->b_gain, scenes[model]->bright_gain, scenes[model]->contrast_gain, \
                                scenes[model]->saturation_gain, scenes[model]->mean, scenes[model]->angle, 0, &scenes[model]->ccmf_param);
            set_dmm_rgb_gamma_param(&scenes[model]->rgb_gamma_param);
        }
        break;

    default:
        printf("没有该场景的配置");
        break;

    }
}
#else
void lcd_scenes_init(void)
{

}
void lcd_scenes_switch(int model)
{
}
#endif

