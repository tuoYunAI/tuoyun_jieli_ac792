#include "app_config.h"
#include "system/includes.h"
#include "asm/gpcrc.h"


#ifdef USE_GPCRC_TEST_DEMO

/* 常见的CRC算法配置，不需的算法可以注释掉，避免占用空间。也可以添加自定义算法*/
static const gpcrc_algorithm_cfg_t crc_algorithm[] = {
    /* crc_name             init        poly            rev_in          rev_out           pol_size   xorout */
    {"CRC-7-MMC",           0x00,       0x09,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL7,   0x00      },
    {"CRC-8",               0x00,       0x07,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL8,   0x00      },
    {"CRC-8-ITU",           0x00,       0x07,       GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL8,   0x55      },
    {"CRC-8-ROHC",          0xFF,       0x07,       GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL8,   0x00      },
    {"CRC-8-MAXIM",         0x00,       0x31,       GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL8,   0x00      },
    {"CRC-16-IBM",          0x0000,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    },
    {"CRC-16-MAXIM",        0x0000,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    },
    {"CRC-16-USB",          0xFFFF,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    },
    {"CRC-16-MODBUS",       0xFFFF,     0x8005,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    },
    {"CRC-16-CCITT",        0x0000,     0x1021,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    },
    {"CRC-16-CCITT-FALSE",  0xFFFF,     0x1021,     GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL16,  0x0000    },
    {"CRC-16-X25",          0xFFFF,     0x1021,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    },
    {"CRC-16-XMODEM",       0x0000,     0x1021,     GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL16,  0x0000    },
    {"CRC-16-XMODEM2",      0x0000,     0x8408,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0x0000    },
    {"CRC-16-DNP",          0x0000,     0x3D65,     GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL16,  0xFFFF    },
    {"CRC-32",              0xFFFFFFFF, 0x04C11DB7, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF},
    {"CRC-32-C",            0xFFFFFFFF, 0x1EDC6F41, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF},
    {"CRC-32-KOOPMAN",      0xFFFFFFFF, 0x741B8CD7, GPCRC_REV_IN,    GPCRC_REV_OUT,     GPCRC_POL32,  0xFFFFFFFF},
    {"CRC-32-MPEG-2",       0xFFFFFFFF, 0x04C11DB7, GPCRC_NO_REV_IN, GPCRC_NO_REV_OUT,  GPCRC_POL32,  0x00000000},
};


/**@brief    根据算法名获取已注册的算法配置
* @param algorithm_name 算法名
* @return    算法配置的地址
*/
static const gpcrc_algorithm_cfg_t *gpcrc_get_algorithm_cfg(char *algorithm_name)
{
    u8 crc_algorithm_cnt = sizeof(crc_algorithm) / sizeof(gpcrc_algorithm_cfg_t);
    for (u8 i = 0; i < crc_algorithm_cnt; i++) {
        if (strcmp(algorithm_name, crc_algorithm[i].crc_name) == 0) {
            return &crc_algorithm[i];
        }
    }
    printf("Can't find algorithm config!\n");
    return NULL;
}


static void gpcrc_test_task(void *arg)
{
    u32 crc_result;

    /* ALIGNE(4) u8  test_data_u8[] = {0x1A,0x2B,0x3C,0x4D,0x5E,0x6F,0x11,0x22,0x33,0x44,0x55}; */
    /* ALIGNE(4) u8  test_data_u8[] = {0x1A,0x2B,0x3C,0x4D,0x5E,0x6F,0x11,0x22,0x33,0x44}; */
    ALIGNE(4) u8  test_data_u8[] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
    /* ALIGNE(4) u8  test_data_u8[] = {0x1A,0x2B,0x3C,0x4D}; */
    u32 test_data_u8_len = sizeof(test_data_u8);


    ALIGNE(4) u16 test_data_u16[] = {0x8137, 0xDB1A, 0x584C, 0x617F};
    u32 test_data_u16_len = sizeof(test_data_u16) / 2;


    ALIGNE(4) u32 test_data_u32[] = {0xAABBCCDD, 0x55667788, 0x37374242};
    u32 test_data_u32_len = sizeof(test_data_u32) / 4;


    while (1) {
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");


        printf("data8 len:%u\n", test_data_u8_len);
        printf("input data8:\n");
        put_buf(test_data_u8, test_data_u8_len);

        printf("\ndata16 len:%u\n", test_data_u16_len);
        printf("input data16:\n");
        put_buf((u8 *)test_data_u16, test_data_u16_len * 2);

        printf("\ndata32 len:%u\n", test_data_u32_len);
        printf("input data32:\n");
        put_buf((u8 *)test_data_u32, test_data_u32_len * 4);

        printf("\n\n");

#if 1
        // 遍历所有已配置好的crc算法，计算指定u8数据格式的buf的crc值
        u8 crc_algorithm_cnt = sizeof(crc_algorithm) / sizeof(gpcrc_algorithm_cfg_t);
        /* const gpcrc_algorithm_cfg_t *algo_cfg = gpcrc_get_algorithm_cfg("CRC-16-MAXIM"); */
        for (u8 i = 0; i < crc_algorithm_cnt; i++) {
            const gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[i];
            printf("ALGORITHM NAME:%s\n", algo_cfg->crc_name);
            printf("INIT:0x%x\n", algo_cfg->init);
            printf("POL:0x%x\n", algo_cfg->pol);
            printf("XOROUT:0x%x\n", algo_cfg->xorout);
            crc_result = gpcrc_calc_u8_buf((gpcrc_algorithm_cfg_t *)algo_cfg, test_data_u8, test_data_u8_len);
            printf("CRC:%x\n\n", crc_result);
        }



#else
        // 遍历所有已配置好的crc算法，计算指定数据格式的buf的crc值
        printf("data8 len:%u\n", test_data_u8_len);
        printf("input data8:\n");
        put_buf(test_data_u8, test_data_u8_len);
        u8 crc_algorithm_cnt = sizeof(crc_algorithm) / sizeof(gpcrc_algorithm_cfg_t);
        for (u8 i = 0; i < crc_algorithm_cnt; i++) {
            gpcrc_algorithm_cfg_t *algo_cfg = &crc_algorithm[i];
            printf("~~~~~~~~~~~~~~~~~~%u.%s~~~~~~~~~~~~~~~~~~\n", i, algo_cfg->crc_name);
            printf("INIT:0x%x\n", algo_cfg->init);
            printf("POL:0x%x\n", algo_cfg->pol);
            printf("XOROUT:0x%x\n", algo_cfg->xorout);
            /* crc_result = gpcrc_calc_buf_crc_flex(algo_cfg, GPCRC_DAT8, test_data_u8, test_data_u8_len); */
            /* crc_result = gpcrc_calc_buf_crc_flex(algo_cfg, GPCRC_DAT16, test_data_u16, test_data_u16_len); */
            crc_result = gpcrc_calc_buf_crc_flex(algo_cfg, GPCRC_DAT32, test_data_u32, test_data_u32_len);
            printf("CRC:%x\n\n", crc_result);
        }

#endif


        printf("\n\n");
        os_time_dly(500);
    }
}

static int c_main_gpcrc(void)
{
    os_task_create(gpcrc_test_task, NULL, 10, 1000, 0, "gpcrc_test_task");
    return 0;
}

late_initcall(c_main_gpcrc);

#endif // USE_GPCRC_TEST_DEMO
