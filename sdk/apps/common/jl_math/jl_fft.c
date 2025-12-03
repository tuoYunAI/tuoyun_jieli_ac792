#include "app_config.h"
#include "generic/typedef.h"
#include "system/spinlock.h"
#include "system/init.h"
#include "os/os_api.h"
#include "fftvec_math_drv.h"
#include "_kiss_fft_guts.h"


#if defined CONFIG_ASR_ALGORITHM || defined CONFIG_REVERB_MODE_ENABLE || defined CONFIG_AEC_ENC_ENABLE || defined CONFIG_DNS_ENC_ENABLE || defined CONFIG_M4A_DEC_ENABLE || defined CONFIG_NEW_M4A_DEC_ENABLE || defined CONFIG_AAC_DEC_ENABLE || defined CONFIG_WMA_DEC_ENABLE || defined CONFIG_SPECTRUM_FFT_EFFECT_ENABLE


#define  FFT_REAL       1                 //complex
#define  IFFT_DIV2      (3-FFT_REAL)
#define  FFT_ISR_IE     0

//软件fft算法
/* #define KISS_FFT */

#define FFT_THROUGH_CACHE   0
#define FFT_LOCK_USE_MUTEX  0

#define SHR(a,shift)    ((a) >> (shift))

#define PSHR(a,shift)   ((a)>=0?(SHR((a)+(1<<(shift-1)),shift)):\
                            (SHR((a)+(1<<(shift-1))-1,shift)))

#if defined CONFIG_WMA_DEC_ENABLE
#define MAX_FFT_SUPPORT_POINT_NUM 2048
#else
#define MAX_FFT_SUPPORT_POINT_NUM 1024
#endif

#if FFT_LOCK_USE_MUTEX
static OS_MUTEX fft_lock;
#define fft_lock(x)   os_mutex_pend(x, 0)
#define fft_unlock(x) os_mutex_post(x)
#else
static DEFINE_SPINLOCK(fft_lock);
#define fft_lock(x)   spin_lock(x)
#define fft_unlock(x) spin_unlock(x)
#endif

#if 1
sec(.fft_data) static int datain[MAX_FFT_SUPPORT_POINT_NUM];
sec(.fft_data) static int datain_y[MAX_FFT_SUPPORT_POINT_NUM];
sec(.fft_data) static int dataout[MAX_FFT_SUPPORT_POINT_NUM + 2];
#else
sec(.fft_data) static int datain[MAX_FFT_SUPPORT_POINT_NUM + 2];
sec(.fft_data) static int *dataout = datain;
sec(.fft_data) static int datain_y[MAX_FFT_SUPPORT_POINT_NUM];
#endif

typedef struct fft_config_dat {
    unsigned long fft0_con;
    unsigned long fft0_iadr;
    unsigned long fft0_oadr;
    unsigned long fft0_null;
} fft_config_struct;

sec(.fft_data) static fft_config_struct fft_config;
sec(.fft_data) static hwvec_ctx_t g_vector_core_set;

static const struct {
    u32 block;
    u32 config;
} complex_tab[] = {
    { 128, COMPLEX_FFT128_CONFIG  },
    { 128, COMPLEX_IFFT128_CONFIG },
    { 256, COMPLEX_FFT256_CONFIG  },
    { 256, COMPLEX_IFFT256_CONFIG },
    { 512, COMPLEX_FFT512_CONFIG  },
    { 512, COMPLEX_IFFT512_CONFIG },
    { 1024, COMPLEX_FFT1024_CONFIG },
    { 1024, COMPLEX_IFFT1024_CONFIG},
    { 2048, COMPLEX_FFT2048_CONFIG },
    { 2048, COMPLEX_IFFT2048_CONFIG},
    { 10, COMPLEX_FFT10_CONFIG   },
    { 10, COMPLEX_IFFT10_CONFIG  },
    { 15, COMPLEX_FFT15_CONFIG   },
    { 15, COMPLEX_IFFT15_CONFIG  },
    { 20, COMPLEX_FFT20_CONFIG   },
    { 20, COMPLEX_IFFT20_CONFIG  },
    { 30, COMPLEX_FFT30_CONFIG   },
    { 30, COMPLEX_IFFT30_CONFIG  },
    { 32, COMPLEX_FFT32_CONFIG   },
    { 32, COMPLEX_IFFT32_CONFIG  },
    { 40, COMPLEX_FFT40_CONFIG   },
    { 40, COMPLEX_IFFT40_CONFIG  },
    { 60, COMPLEX_FFT60_CONFIG   },
    { 60, COMPLEX_IFFT60_CONFIG  },
    { 64, COMPLEX_FFT64_CONFIG   },
    { 64, COMPLEX_IFFT64_CONFIG  },
    { 80, COMPLEX_FFT80_CONFIG   },
    { 80, COMPLEX_IFFT80_CONFIG  },
    { 120, COMPLEX_FFT120_CONFIG  },
    { 120, COMPLEX_IFFT120_CONFIG },
    { 160, COMPLEX_FFT160_CONFIG  },
    { 160, COMPLEX_IFFT160_CONFIG },
    { 240, COMPLEX_FFT240_CONFIG  },
    { 240, COMPLEX_IFFT240_CONFIG },
    { 320, COMPLEX_FFT320_CONFIG  },
    { 320, COMPLEX_IFFT320_CONFIG },
    { 480, COMPLEX_FFT480_CONFIG  },
    { 480, COMPLEX_IFFT480_CONFIG },
    { 960, COMPLEX_FFT960_CONFIG  },
    { 960, COMPLEX_IFFT960_CONFIG },
};

static const struct {
    u32 block;
    u32 config;
} real_tab[] = {
    { 128, REAL_FFT128_CONFIG  },
    { 128, REAL_IFFT128_CONFIG },
    { 256, REAL_FFT256_CONFIG  },
    { 256, REAL_IFFT256_CONFIG },
    { 512, REAL_FFT512_CONFIG  },
    { 512, REAL_IFFT512_CONFIG },
    { 1024, REAL_FFT1024_CONFIG },
    { 1024, REAL_IFFT1024_CONFIG},
    { 2048, REAL_FFT2048_CONFIG },
    { 2048, REAL_IFFT2048_CONFIG},
    { 20, REAL_FFT20_CONFIG   },
    { 20, REAL_IFFT20_CONFIG  },
    { 30, REAL_FFT30_CONFIG   },
    { 30, REAL_IFFT30_CONFIG  },
    { 40, REAL_FFT40_CONFIG   },
    { 40, REAL_IFFT40_CONFIG  },
    { 60, REAL_FFT60_CONFIG   },
    { 60, REAL_IFFT60_CONFIG  },
    { 64, REAL_FFT64_CONFIG   },
    { 64, REAL_IFFT64_CONFIG  },
    { 80, REAL_FFT80_CONFIG   },
    { 80, REAL_IFFT80_CONFIG  },
    { 120, REAL_FFT120_CONFIG  },
    { 120, REAL_IFFT120_CONFIG },
    { 160, REAL_FFT160_CONFIG  },
    { 160, REAL_IFFT160_CONFIG },
    { 240, REAL_FFT240_CONFIG  },
    { 240, REAL_IFFT240_CONFIG },
    { 320, REAL_FFT320_CONFIG  },
    { 320, REAL_IFFT320_CONFIG },
    { 480, REAL_FFT480_CONFIG  },
    { 480, REAL_IFFT480_CONFIG },
    { 960, REAL_FFT960_CONFIG  },
    { 960, REAL_IFFT960_CONFIG },
};


#if FFT_LOCK_USE_MUTEX
static int fft_mutex_init(void)
{
    return os_mutex_create(&fft_lock);
}
late_initcall(fft_mutex_init);
#endif

#ifdef KISS_FFT

//是否使用软硬混合fft
#define SOFT_FFT_ENABLE

typedef struct kiss_config {
    kiss_fft_cfg forward;
    kiss_fft_cfg backward;
    int N;
    float *fft_in;
    float *fft_out;
} kiss_config_t;

static kiss_config_t *kiss_512;
static kiss_config_t *kiss_1024;

static u8 fft_is_lock(void)
{
#if FFT_LOCK_USE_MUTEX
    return os_mutex_accept(&fft_lock);
#else
    return fft_lock.rwlock;
#endif
}

//input: only real
//output: interleave real and image, size is (N/2+1)*2
__attribute__((always_inline))
void kiss_cfg_fft(void *table, int *in, int *out)
{
    kiss_config_t *t = (kiss_config_t *)table;

    float scale = 1;// / t->N;

    for (int i = 0; i < t->N; i++) {
        t->fft_in[i * 2] = in[i] * scale;
        t->fft_in[i * 2 + 1] = 0;
    }

    kiss_fft(t->forward, t->fft_in, t->fft_out);

    for (int i = 0; i < t->N + 2; i++) {
        out[i] = t->fft_out[i];
    }
}

//output: only real
//input: interleave real and image, size is (N/2+1)*2
__attribute__((always_inline))
void kiss_cfg_ifft(void *table, int *in, int *out)
{
    kiss_config_t *t = (kiss_config_t *)table;

    float scale;
    int i;
    scale = 1.0f / t->N;

    for (int i = 0; i < t->N / 2 + 1; i++) {
        t->fft_in[i * 2] = in[i * 2];
        t->fft_in[i * 2 + 1] = in[i * 2 + 1];
    }
    for (int i = t->N / 2 + 1; i < t->N; i++) {
        t->fft_in[i * 2] = t->fft_in[(t->N - i) * 2];
        t->fft_in[i * 2 + 1] = -t->fft_in[(t->N - i) * 2 + 1];
    }

    kiss_fft(t->backward, t->fft_in, t->fft_out);

    for (int i = 0; i < t->N; i++) {
        out[i] = t->fft_out[i * 2];
    }

    for (i = 0; i < t->N; i++) {
        out[i] *= scale;
    }

}

void kiss_config_uninit(void)
{
    if (kiss_512->fft_in) {
        free(kiss_512->fft_in);
        kiss_512->fft_in = NULL;
    }
    if (kiss_512->fft_out) {
        free(kiss_512->fft_out);
        kiss_512->fft_out = NULL;
    }
    kiss_fft_free(kiss_512->forward);
    kiss_fft_free(kiss_512->backward);
    free(kiss_512);

    if (kiss_1024->fft_in) {
        free(kiss_1024->fft_in);
        kiss_1024->fft_in = NULL;
    }
    if (kiss_1024->fft_out) {
        free(kiss_1024->fft_out);
        kiss_1024->fft_out = NULL;
    }
    kiss_fft_free(kiss_1024->forward);
    kiss_fft_free(kiss_1024->backward);
    free(kiss_1024);
}

int kiss_config_init(void)
{
    kiss_512 = (kiss_config_t *)calloc(1, sizeof(kiss_config_t));
    if (!kiss_512) {
        goto __err;
    }
    kiss_1024 = (kiss_config_t *)calloc(1, sizeof(kiss_config_t));
    if (!kiss_1024) {
        goto __err;
    }

    kiss_512->N        	= 512;
    kiss_512->forward  	= kiss_fft_alloc(kiss_512->N, 0, NULL, NULL);
    kiss_512->backward 	= kiss_fft_alloc(kiss_512->N, 1, NULL, NULL);

    kiss_512->fft_in 	= (float *)calloc(1, sizeof(float) * kiss_512->N * 2);
    if (!kiss_512->fft_in) {
        goto __err;
    }
    kiss_512->fft_out 	= (float *)calloc(1, sizeof(float) * kiss_512->N * 2);
    if (!kiss_512->fft_out) {
        goto __err;
    }

    kiss_1024->N        = 1024;
    kiss_1024->forward  = kiss_fft_alloc(kiss_1024->N, 0, NULL, NULL);
    kiss_1024->backward = kiss_fft_alloc(kiss_1024->N, 1, NULL, NULL);

    kiss_1024->fft_in 	= (float *)calloc(1, sizeof(float) * kiss_1024->N * 2);
    if (!kiss_1024->fft_in) {
        goto __err;
    }
    kiss_1024->fft_out 	= (float *)calloc(1, sizeof(float) * kiss_1024->N * 2);
    if (!kiss_1024->fft_out) {
        goto __err;
    }

    return 0;

__err:
    kiss_config_uninit();

    return -1;
}
late_initcall(kiss_config_init);

#endif


__attribute__((always_inline))
static int vc_hw_fft(int *coef_in, int blockbit, int *coef_out, int real, int same)
{
    //init pi32v2 fft hardware
    int blocklen = 1 << blockbit;
    JL_FFT->CON = 0;

    int i;

    if (real) {
        for (i = 0; i < ARRAY_SIZE(real_tab); i += 2) {
            if (blocklen == real_tab[i].block) {
                fft_config.fft0_con = real_tab[i].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(real_tab)) {
            return -1;
        }
    } else {
        for (i = 0; i < ARRAY_SIZE(complex_tab); i += 2) {
            if (blocklen == complex_tab[i].block) {
                fft_config.fft0_con = complex_tab[i].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(complex_tab)) {
            return -1;
        }
    }
    fft_config.fft0_iadr = (unsigned int)coef_in;
    fft_config.fft0_oadr = (unsigned int)coef_out;

    JL_FFT->CADR = (unsigned int)&fft_config;
    JL_FFT->CON = (1 << 8) | (0 << 6) | (FFT_ISR_IE << 2) | (0 << 1) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    return 0;
}

__attribute__((always_inline))
static int vc_hw_ifft(int *coef_in, int blockbit, int *coef_out, int real, int same)
{
    //init pi32v2 fft hardware
    int blocklen = 1 << blockbit;
    JL_FFT->CON = 0;

    int i;

    if (real) {
        for (i = 0; i < ARRAY_SIZE(real_tab); i += 2) {
            if (blocklen == real_tab[i + 1].block) {
                fft_config.fft0_con = real_tab[i + 1].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(real_tab)) {
            return -1;
        }
    } else {
        for (i = 0; i < ARRAY_SIZE(complex_tab); i += 2) {
            if (blocklen == complex_tab[i + 1].block) {
                fft_config.fft0_con = complex_tab[i + 1].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(complex_tab)) {
            return -1;
        }
    }
    fft_config.fft0_iadr = (unsigned int)coef_in;
    fft_config.fft0_oadr = (unsigned int)coef_out;

    JL_FFT->CADR = (unsigned int)&fft_config;
    JL_FFT->CON = (1 << 8) | (0 << 6) | (FFT_ISR_IE << 2) | (0 << 1) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    return 0;
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_fft_1024(short *in, int *out)
{
    fft_lock(&fft_lock);
    for (int i = 0; i < 1024; i++) {
        datain[i] = in[i];
    }
    if (FFT_THROUGH_CACHE || (void *)out < (void *)__SFC0_ADDR_BEGIN__) {
        vc_hw_fft(datain, 10, out, 1, datain == out ? 1 : 0);
    } else {
        vc_hw_fft(datain, 10, dataout, 1, datain == dataout ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * 1026);
    }
    fft_unlock(&fft_lock);
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_fft_1024_ex(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (fft_is_lock()) {
        kiss_cfg_fft(kiss_1024, in, out);
    } else
#endif
    {
        fft_lock(&fft_lock);
        if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
            vc_hw_fft(in, 10, out, 1, in == out ? 1 : 0);
        } else {
            memcpy(datain, in, sizeof(int) * 1024);
            vc_hw_fft(datain, 10, dataout, 1, datain == dataout ? 1 : 0);
            memcpy(out, dataout, sizeof(int) * 1026);
        }
        fft_unlock(&fft_lock);
    }
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_fft_512(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (fft_is_lock()) {
        kiss_cfg_fft(kiss_512, in, out);
    } else
#endif
    {
        fft_lock(&fft_lock);
        if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
            vc_hw_fft(in, 9, out, 1, in == out ? 1 : 0);
        } else {
            memcpy(datain, in, sizeof(int) * 512);
            vc_hw_fft(datain, 9, dataout, 1, datain == dataout ? 1 : 0);
            memcpy(out, dataout, sizeof(int) * 514);
        }
        fft_unlock(&fft_lock);
    }
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
int jl_ifft_1024(int *in, short *out)
{
    int i;
    int tData_tmp = 0, tData_max = 0, shift = 0;

    fft_lock(&fft_lock);
    if (FFT_THROUGH_CACHE || (void *)in < (void *)__SFC0_ADDR_BEGIN__) {
        vc_hw_ifft(in, 10, dataout, 1, in == dataout ? 1 : 0);
    } else {
        memcpy(dataout, in, sizeof(int) * 1026);
        vc_hw_ifft(dataout, 10, datain, 1, dataout == datain ? 1 : 0);
    }
    for (i = 0; i < 1024; i++) {
        tData_tmp = datain[i] > 0 ? datain[i] : (-datain[i]);

        if (tData_max < tData_tmp) {
            tData_max = tData_tmp;
        }
    }
    while (tData_max > 32767) {
        tData_max >>= 1;
        shift += 1;
    }
    for (i = 0; i < 1024; i += 4) {
        out[i] = PSHR(datain[i], shift);
        out[i + 1] = PSHR(datain[i + 1], shift);
        out[i + 2] = PSHR(datain[i + 2], shift);
        out[i + 3] = PSHR(datain[i + 3], shift);
    }
    fft_unlock(&fft_lock);
    return shift;
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_ifft_1024_ex(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (fft_is_lock()) {
        kiss_cfg_ifft(kiss_1024, in, out);
    } else
#endif
    {
        fft_lock(&fft_lock);
        if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
            vc_hw_ifft(in, 10, out, 1, in == out ? 1 : 0);
        } else {
            memcpy(dataout, in, sizeof(int) * 1026);
            vc_hw_ifft(dataout, 10, datain, 1, dataout == datain ? 1 : 0);
            memcpy(out, datain, sizeof(int) * 1024);
        }
        fft_unlock(&fft_lock);
    }
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_ifft_512(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (fft_is_lock()) {
        kiss_cfg_ifft(kiss_512, in, out);
    } else
#endif
    {
        fft_lock(&fft_lock);
        if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
            vc_hw_ifft(in, 9, out, 1, in == out ? 1 : 0);
        } else {
            memcpy(datain, in, sizeof(int) * 514);
            vc_hw_ifft(datain, 9, dataout, 1, dataout == datain ? 1 : 0);
            memcpy(out, dataout, sizeof(int) * 512);
        }
        fft_unlock(&fft_lock);
    }
}

__attribute__((always_inline))
static u32 find_pow(u32 value)
{
    u32 i = 0;

    while (value) {
        value >>= 1;
        if (++i > 11) {
            ASSERT(NULL, "pow <= 10");
        }
    }

    return i - 1;
}

__attribute__((always_inline))
void jl_fft_N(int *in, int *out, int fft_N)
{
    fft_lock(&fft_lock);
    if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
        vc_hw_fft(in, find_pow(fft_N), out, 1, in == out ? 1 : 0);
    } else {
        memcpy(datain, in, sizeof(int) * fft_N);
        vc_hw_fft(datain, find_pow(fft_N), dataout, 1, dataout == datain ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * (fft_N + 2));
    }
    fft_unlock(&fft_lock);
}

__attribute__((always_inline))
void jl_ifft_N(int *in, int *out, int fft_N)
{
    fft_lock(&fft_lock);
    if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
        vc_hw_ifft(in, find_pow(fft_N), out, 1, in == out ? 1 : 0);
    } else {
        memcpy(dataout, in, sizeof(int) * (fft_N + 2));
        vc_hw_ifft(dataout, find_pow(fft_N), datain, 1, dataout == datain ? 1 : 0);
        memcpy(out, datain, sizeof(int) * fft_N);
    }
    fft_unlock(&fft_lock);
}

__attribute__((always_inline))
void jl_complex_fft_n(int *in, int *out, int bits)
{
    int len = 1 << bits;
    fft_lock(&fft_lock);
    if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
        vc_hw_fft(in, bits - 1, out, 0, in == out ? 1 : 0);
    } else {
        memcpy(datain, in, sizeof(int) * len);
        vc_hw_fft(datain, bits - 1, dataout, 0, datain == dataout ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * len);
    }
    fft_unlock(&fft_lock);
}

__attribute__((always_inline))
void jl_complex_ifft_n(int *in, int *out, int bits)
{
    int len = 1 << bits;
    fft_lock(&fft_lock);
    if (FFT_THROUGH_CACHE || ((void *)in < (void *)__SFC0_ADDR_BEGIN__ && (void *)out < (void *)__SFC0_ADDR_BEGIN__)) {
        vc_hw_ifft(in, bits - 1, out, 0, in == out ? 1 : 0);
    } else {
        memcpy(datain, in, sizeof(int) * len);
        vc_hw_ifft(datain, bits - 1, dataout, 0, datain == dataout ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * len);
    }
    fft_unlock(&fft_lock);
}

void hw_fft_run(int cfg, const int *coef_in, int *coef_out)
{
    int len;
    int *in = datain, *out = dataout;

    if (FFT_THROUGH_CACHE || (void *)coef_in < (void *)__SFC0_ADDR_BEGIN__) {
        in = (int *)coef_in;
    }
    if (FFT_THROUGH_CACHE || (void *)coef_out < (void *)__SFC0_ADDR_BEGIN__) {
        out = coef_out;
    }

    if (!coef_in || !coef_out) {
        return;
    }

    if (in == out) {
        cfg |= BIT(0);
    } else {
        cfg &= ~BIT(0);
    }

    if (cfg & BIT(1)) {
        len = (cfg & 0xFFF0000) >> 16;
        if (cfg & BIT(2)) {
            len += 2;
            if (in == datain) {
                in = dataout;
                if (out == dataout) {
                    out = datain;
                }
            }
        }
    } else {
        len = (cfg & 0xFFF0000) >> (16 - 1);
    }

    fft_lock(&fft_lock);
    if (in != coef_in) {
        memcpy(in, coef_in, sizeof(int) * len);
    }

    JL_FFT->CON = 0;
    fft_config.fft0_con = cfg;
    fft_config.fft0_iadr = (unsigned int)in;
    fft_config.fft0_oadr = (unsigned int)out;
    JL_FFT->CADR = (unsigned int)&fft_config;
    JL_FFT->CON = (1 << 8) | (0 << 6) | (FFT_ISR_IE << 2) | (0 << 1) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);

    if (cfg & BIT(1)) {
        if (cfg & BIT(2)) {
            len -= 2;
        } else {
            len += 2;
        }
    }

    if (out != coef_out) {
        memcpy(coef_out, out, sizeof(int) * len);
    }
    fft_unlock(&fft_lock);
}

void hw_fft_wrap(fft_config_struct *ctx)
{
    hw_fft_run(ctx->fft0_con, (const int *)ctx->fft0_iadr, (int *)ctx->fft0_oadr);
}

#if defined CONFIG_NEW_M4A_DEC_ENABLE
int hw_fft_core(int cfg, const int *coef_in, int *coef_out)
{
    hw_fft_run(cfg, coef_in, coef_out);

    return 0;
}
#endif

unsigned int Get_FFT_Base(void)
{
    return JL_FFT_BASE;
}

__attribute__((always_inline)) inline
void fast_i2f(const int *in, float *out, int N, float scale)
{
    int tmp32_1;
    asm volatile(
        " 1: \n\t"
        " rep %1 { \n\t"
        " 	%0 = [%2++=4] \n\t"
        " 	%0 = itof(%0) \n\t"
        " 	%0 = %0 * %4 (f) \n\t"
        " 	[%3++=4] = %0 \n\t"
        " } \n\t"
        " if(%1>0) goto 1b \n\t"
        : "=&r"(tmp32_1),
        "=&r"(N),
        "=&r"(in),
        "=&r"(out),
        "=&r"(scale)
        : "0"(tmp32_1),
        "1"(N),
        "2"(in),
        "3"(out),
        "4"(scale)
        :);
}

__attribute__((always_inline)) inline
void fast_f2i(const float *in, int *out, int N, float scale)
{
    int tmp32_1;
    asm volatile(
        " 1: \n\t"
        " rep %1 { \n\t"
        " 	%0 = [%2++=4] \n\t"
        " 	%0 = %0 * %4 (f) \n\t"
        " 	%0 = ftoi(%0)(even) \n\t"
        " 	[%3++=4] = %0 \n\t"
        " } \n\t"
        " if(%1>0) goto 1b \n\t"
        : "=&r"(tmp32_1),
        "=&r"(N),
        "=&r"(in),
        "=&r"(out),
        "=&r"(scale)
        : "0"(tmp32_1),
        "1"(N),
        "2"(in),
        "3"(out),
        "4"(scale)
        :);
}

//hwfft_ctx_t结构体和FFT输入输出的数据地址一定要放在内部ram

__attribute__((always_inline)) inline
void hwfft_exec_batch(hwfft_ctx_t *ctx, int nbatch)
{
    fft_lock(&fft_lock);
    JL_FFT->CADR = (unsigned long)(ctx);
    //CLR    //FFT     //ie     //en
    JL_FFT->CON = (nbatch << 8) | (1 << 6) | (0 << 3) | (FFT_ISR_IE << 2) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    fft_unlock(&fft_lock);
};

__attribute__((always_inline)) inline
void hwfft_exec_once(hwfft_ctx_t *ctx)
{
    fft_lock(&fft_lock);
    JL_FFT->CADR = (unsigned long)(ctx);
    //CLR    //FFT     //ie     //en
    JL_FFT->CON = (1 << 8) | (1 << 6) | (0 << 3) | (FFT_ISR_IE << 2) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    fft_unlock(&fft_lock);
};

__attribute__((always_inline)) inline
int hwfft_get_nfft(unsigned long fft_config)
{
    int nfft = (fft_config >> 16) & 0xfff;
    return nfft;
};

//0---forward  1---backward
__attribute__((always_inline)) inline
int hwfft_get_backward(unsigned long fft_config)
{
    return ((fft_config & (1 << 2)) != 0);
};

//0---complex  1---real
__attribute__((always_inline)) inline
int hwfft_get_real(unsigned long fft_config)
{
    return ((fft_config & (1 << 1)) != 0);
};

__attribute__((always_inline)) inline
int hwfft_check_fft_config(unsigned long fft_config, int nfft, int backward, int real)
{
    if (hwfft_get_nfft(fft_config) != nfft) {
        printf("fft nfft mismatch!\n");
        return -1;
    }

    if (hwfft_get_backward(fft_config) != backward) {
        printf("fft backward direction mismatch!\n");
        return -1;
    }

    if (hwfft_get_real(fft_config) != real) {
        printf("fft real mode mismatch!\n");
        return -1;
    }
    return 0;
};

__attribute__((always_inline)) inline
void hwfft_fill_fft_ctx(hwfft_ctx_t *ctx, unsigned long fft_config, long *iadr, long *oadr)
{
    unsigned long config;

    if (iadr == oadr) {
        config = fft_config | FFT_SAME_ADR;
    } else {
        config = fft_config & (~FFT_SAME_ADR);
    }
    ctx->fft_con = config;
    ctx->fft_iadr = (unsigned long)iadr;
    ctx->fft_oadr = (unsigned long)oadr;
    ctx->fft_null = 0;
//    printf("config1:%d %x\n",config,ctx->fft_iadr);
};

__attribute__((always_inline)) inline
void hwfft_ifft_scale(long *iadr, int nfft)
{
    int scale;
    int q;
    long config = VECX_S32 | VECY_S32 | VECZ_S32;
//    printf("nfft:%d\n",nfft);

    if (nfft % 15 == 0) {
        scale = 1118481;
        q = 21;
        vector_real_mul_const(iadr, iadr, nfft, q, scale, config);
    } else if (nfft % 10 == 0) {
        scale = 1677722;//1864135;
        q = 21;
        vector_real_mul_const(iadr, iadr, nfft, q, scale, config);
    }
}

__attribute__((always_inline)) inline
void hwfft_fft_i32(hwfft_ctx_t *ctx)
{
    hwfft_exec_once(ctx);
};

__attribute__((always_inline)) inline
void hwfft_ifft_i32(hwfft_ctx_t *ctx)
{
    int real = hwfft_get_real(ctx->fft_con);
    int nfft = hwfft_get_nfft(ctx->fft_con);
    hwfft_exec_once(ctx);

    hwfft_ifft_scale((long *)ctx->fft_oadr, nfft * (2 - real));
};

__attribute__((always_inline)) inline
void hwfft_fft_f32(hwfft_ctx_t *ctx, float scale)
{
    int nfft = hwfft_get_nfft(ctx->fft_con);
    int real = hwfft_get_real(ctx->fft_con);
    fast_f2i((float *)ctx->fft_iadr, (int *)ctx->fft_iadr, nfft * (2 - real), scale);
    hwfft_fft_i32(ctx);
    fast_i2f((int *)ctx->fft_oadr, (float *)ctx->fft_oadr, nfft * (2 - real) + 2 * real, 1.0f / scale); // com  0  real 1
};

__attribute__((always_inline)) inline
void hwfft_ifft_f32(hwfft_ctx_t *ctx, float scale)
{
    int nfft = hwfft_get_nfft(ctx->fft_con);
    int real = hwfft_get_real(ctx->fft_con);

    fast_f2i((float *)ctx->fft_iadr, (int *)ctx->fft_iadr, nfft * (2 - real) + 2 * real, scale);

    hwfft_ifft_i32(ctx);

    fast_i2f((int *)ctx->fft_oadr, (float *)ctx->fft_oadr, nfft * (2 - real), 1.0f / scale);
};

//////////////////////////////////////////////////////////////////////////
//VECTOR
__attribute__((always_inline)) inline
void hwvec_exec(
    void *xptr,
    void *yptr,
    void *zptr,
    short x_inc,
    short y_inc,
    short z_inc,
    short nlen,
    short nloop,
    char q,
    long config,
    long const_dat
)
{
    //硬件锁上锁
    fft_lock(&fft_lock);

    char vec_cpy_real = 0;
    char vec_zptr_cpy_dot = 0;

    //获取段内数组的地址, 硬件运算数据都需在段内进行
    void *_xptr = &datain[0];
    void *_yptr = &datain_y[0];
    void *_zptr = &dataout[0];

    //区分复制长度(nlen为数据个数)是实数还是复数, 复数需乘2
    if (config & VEC_CPY_REAL_OR_VEC) {
        vec_cpy_real = 1;
        config &= ~VEC_CPY_REAL_OR_VEC;
        if (FFT_THROUGH_CACHE || xptr < (void *)__SFC0_ADDR_BEGIN__) {
            _xptr = xptr;
        } else {
            memcpy(_xptr, xptr, nlen * 4);
        }
        //是否需要复制输入YPTR
        if (config & VEC_YPTR_NOCPY) {
            config &= ~VEC_YPTR_NOCPY;
        } else {
            if (FFT_THROUGH_CACHE || yptr < (void *)__SFC0_ADDR_BEGIN__) {
                _yptr = yptr;
            } else {
                memcpy(_yptr, yptr, nlen * 4);
            }
        }
    } else {
        if (FFT_THROUGH_CACHE || xptr < (void *)__SFC0_ADDR_BEGIN__) {
            _xptr = xptr;
        } else {
            memcpy(_xptr, xptr, nlen * 4 * 2);
        }
        //是否需要复制输入YPTR
        if (config & VEC_YPTR_NOCPY) {
            config &= ~VEC_YPTR_NOCPY;
        } else {
            if (FFT_THROUGH_CACHE || yptr < (void *)__SFC0_ADDR_BEGIN__) {
                _yptr = yptr;
            } else {
                memcpy(_yptr, yptr, nlen * 4 * 2);
            }
        }
    }

    //是否需要输入zptr数据
    if (config & VEC_ZPTR_CPY) {
        config &= ~VEC_ZPTR_CPY;
        if (vec_cpy_real) {
            if (FFT_THROUGH_CACHE || zptr < (void *)__SFC0_ADDR_BEGIN__) {
                _zptr = zptr;
            } else {
                memcpy(_zptr, zptr, nlen * 4);
            }
        } else {
            if (FFT_THROUGH_CACHE || zptr < (void *)__SFC0_ADDR_BEGIN__) {
                _zptr = zptr;
            } else {
                memcpy(_zptr, zptr, nlen * 4 * 2);
            }
        }
    } else if (zptr == xptr) {
        _zptr = _xptr;
    } else if (zptr == yptr) {
        _zptr = _yptr;
    }

    //区分ZPTR输出结果复制长度是一个点还是向量
    if (config & VEC_ZPTR_CPY_DOT) {
        vec_zptr_cpy_dot = 1;
        config &= ~VEC_ZPTR_CPY_DOT;
    }

    //VECTOR寄存器的配置
    g_vector_core_set.vector_con = config;
    g_vector_core_set.vector_config0 = (q << 24) | (nloop << 12) | (nlen);
    g_vector_core_set.vector_config1 = (z_inc << 20) | (y_inc << 10) | x_inc;
    //printf("nlen0:%d,nloop:%d,q:%d,config:%d,%d\n",nlen,nloop,q,config,const_dat);

    g_vector_core_set.vector_xadr = (unsigned long)_xptr;
    g_vector_core_set.vector_yadr = (unsigned long)_yptr;
    g_vector_core_set.vector_zadr = (unsigned long)_zptr;

    JL_FFT->CONST = const_dat;

    JL_FFT->CADR = (unsigned long)(&g_vector_core_set);
    /*printf("JL_FFT->CONST:%x\n",&g_vector_core_set);
    printf("JL_FFT->vector_xadr:%x\n",g_vector_core_set.vector_xadr);
    printf("JL_FFT->vector_yadr:%x\n",g_vector_core_set.vector_yadr);
    printf("JL_FFT->vector_zadr:%x\n",g_vector_core_set.vector_zadr);
    printf("JL_FFT->vector_config0:%x\n",g_vector_core_set.vector_config0);
    printf("JL_FFT->vector_config1:%x\n",g_vector_core_set.vector_config1);*/

    //开始运算
    //nu | clr | vector | ie | en
    JL_FFT->CON = (1 << 8) | (1 << 6) | (1 << 3) | (FFT_ISR_IE << 2) | (1 << 0);

    //等待运算完成中断并清中断
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);

    //获取输出数据
    if (vec_zptr_cpy_dot) {
        *(int *)zptr = *(int *)_zptr;
    } else {
        memcpy(zptr, _zptr, vec_cpy_real ? nlen * 4 : nlen * 4 * 2);
    }

    //硬件锁解锁
    fft_unlock(&fft_lock);
}


//=========================================================================
// vector_complex_add
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] + x[n] )  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_add(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, 0, config, 0);
}

//=========================================================================
// vector_complex_sub
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] - x[n] )  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_sub(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config | VEC_SUB;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, 0, config, 0);
}

//=========================================================================
// vector_complex_mul
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n]. * x[n]. / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_mul(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_CPX_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 0), 1, q, config, 0);
}

//=========================================================================
// vector_complex_mac32
// dw bit[1:0] : x_dw   2: s32
// dw bit[3:2] : y_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
// y_dw == z_dw == x_dw == 2
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32
// z[n] = saturated(z. + (y[n]. * x[n]. / 2^q))  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_mac32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    //config = VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_CPX_MUL_MODE;
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_CPX_MUL_MODE | VEC_ZPTR_CPY;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 0), 1, q, config, 0);
}

//=========================================================================
// vector_complex_msc32
// dw bit[1:0] : x_dw   2: s32
// dw bit[3:2] : y_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
// y_dw == z_dw == x_dw == 2
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32
// z[n] = saturated(z[n]. - (y[n]. * x[n]. / 2^q))  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_msc32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_SUB | VEC_CPX_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_complex_dot_product
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(sum(y[n]. * x[n]. / 2^q))  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_dot_product(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_CPX_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_complex_qdt
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated((x[n].real^2 + x[n].imag^2) / 2^q)  // 0 <= n < len/2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_qdt(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_CPX_QDT | VEC_QDT | VEC_QDT_MODE | VEC_YPTR_NOCPY;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_complex_qdtsum
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   3: s64  2: s32  1: s16  0:s8
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (x[0].real^2 + x[0].image^2 + ... + x[n].real^2+ x[n].image^2) / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_qdtsum(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_QDT | VEC_ACC | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_real_add
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] + x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_add(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, 0, config, 0);
}

//=========================================================================
// vector_real_add_dot_product
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z = sum( y[n] + x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_add_dot_product(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config | VEC_CPY_REAL_OR_VEC | VEC_ZPTR_CPY_DOT;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, 0, config, 0);
}
//=========================================================================
// vector_real_sub
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] - x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_sub(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config | VEC_SUB | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, 0, config, 0);
}

//=========================================================================
// vector_real_mul
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mul(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_mac
// dw bit[1:0] : x_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (z + y*x / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mac32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_msc
// dw bit[1:0] : x_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (z - y*x / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_msc32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_SUB | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_dot_product
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_dot_product(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC | VEC_ZPTR_CPY_DOT;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_sum
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// z_dw >= x_dw
//  -32 < Q < 32
// *zptr, *xptr:  s32/s16/s8
// z[n] = sum(x[n]) / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_sum(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_QDT_MODE | VEC_CPY_REAL_OR_VEC | VEC_YPTR_NOCPY;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_sum_dot_product
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// z_dw >= x_dw
//  -32 < Q < 32
// *zptr, *xptr:  s32/s16/s8
// z = sum(x[n]) / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_sum_dot_product(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_QDT_MODE | VEC_CPY_REAL_OR_VEC | VEC_YPTR_NOCPY | VEC_ZPTR_CPY_DOT;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_qdt
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// z_dw >= x_dw
//  -32 < Q < 32
// *zptr, *xptr:  s32/s16/s8
// z[n] = x[n]^2 / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_qdt(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_QDT | VEC_QDT_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_qdtsum
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(x[n]^2) / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_qdtsum(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_QDT | VEC_ACC | VEC_QDT_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_mac_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = x[n] + y[n] * const_dat / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mac_const(void *zptr, void *yptr, void *xptr, short len, char q, long const_dat, long config)
{
    config = config | VEC_CONST_MUL | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_msc_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] =  x[n] - y[n] * const_dat / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_msc_const(void *zptr, void *yptr, void *xptr, short len, char q, long const_dat, long config)
{
    config = config | VEC_CONST_MUL | VEC_SUB | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_mul_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (x[n] * const_dat/ 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mul_const(void *zptr, void *xptr, short len, char q, long const_dat, long config)
{
    config = config | VEC_CONST_MUL | VEC_QDT | VEC_QDT_MODE | VEC_YPTR_NOCPY | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_max
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
//  z = x*cosnt /2^q
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_max(void *zptr, void *xptr, short len, char q, long config, long const_dat)
{
    config = config | VEC_MAX | VEC_CONST_MUL | VEC_QDT | VEC_QDT_MODE | VEC_YPTR_NOCPY | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_min
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
//  z = x*cosnt /2^q
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_min(void *zptr, void *xptr, short len, char q, long config, long const_dat)
{
    config = config | VEC_MIN | VEC_CONST_MUL | VEC_QDT | VEC_QDT_MODE | VEC_YPTR_NOCPY | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

#endif

