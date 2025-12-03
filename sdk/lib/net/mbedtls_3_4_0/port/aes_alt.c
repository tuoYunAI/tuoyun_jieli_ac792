/* #if !defined(MBEDTLS_CONFIG_FILE) */
#include "mbedtls/mbedtls_config.h"
/* #else */
/* #include MBEDTLS_CONFIG_FILE */
/* #endif */

#include "common.h"
#include "aes_alt.h"
#include "mbedtls/aes.h"
#include "mbedtls/error.h"

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            	\
	do {                                                    \
		(n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
			| ( (uint32_t) (b)[(i) + 1] << 16 )             \
			| ( (uint32_t) (b)[(i) + 2] <<  8 )             \
			| ( (uint32_t) (b)[(i) + 3]       );            \
	} while( 0 )
#endif

#if defined(MBEDTLS_AES_ALT)

#define AES_USE_CPU_MODE  1

/* Parameter validation macros based on platform_util.h */
#define AES_VALIDATE_RET( cond )    \
	MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_AES_BAD_INPUT_DATA )
#define AES_VALIDATE( cond )        \
	MBEDTLS_INTERNAL_VALIDATE( cond )

static int aes_set_key(mbedtls_aes_context *ctx,
                       const unsigned char *key,
                       unsigned int keybits)
{
    unsigned int i;
    int ret = 0;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    switch (keybits) {
    case 128:
        ctx->hcryp_aes.Init.KeySize = CRYP_KEYSIZE_128B;
        break;
    case 192:
        ctx->hcryp_aes.Init.KeySize = CRYP_KEYSIZE_192B;
        break;
    case 256:
        ctx->hcryp_aes.Init.KeySize = CRYP_KEYSIZE_256B;
        break;

    default :
        ret = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
        goto exit;
    }

    /* Format and fill AES key  */
    for (i = 0; i < (keybits / 32); i++) {
        GET_UINT32_BE(ctx->aes_key[i], key, 4 * i);
    }

    ctx->hcryp_aes.Init.DataType = CRYP_DATATYPE_8B;
    ctx->hcryp_aes.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;

    ctx->hcryp_aes.Init.pKey = ctx->aes_key;

    if (HAL_CRYP_Init(&ctx->hcryp_aes) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

exit :
    /* Free context access */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_unlock(&cryp_mutex) != 0) {
        ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
#endif /* MBEDTLS_THREADING_C */

    return (ret);
}

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
    AES_VALIDATE(ctx != NULL);

#if defined(MBEDTLS_THREADING_C)
    /* mutex cannot be initialized twice */
    if (!cryp_mutex_started) {
        mbedtls_mutex_init(&cryp_mutex);
        cryp_mutex_started = 1;
    }
#endif /* MBEDTLS_THREADING_C */

    memset((void *)ctx, 0, sizeof(mbedtls_aes_context));
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

#if defined(MBEDTLS_THREADING_C)
    if (cryp_mutex_started) {
        mbedtls_mutex_free(&cryp_mutex);
        cryp_mutex_started = 0;
    }
#endif /* MBEDTLS_THREADING_C */

    HAL_CRYP_DeInit(&ctx->hcryp_aes);

    memset((void *)ctx, 0, sizeof(mbedtls_aes_context));
}

/* XTS SW implementation inherited code from aes.c */
#if defined(MBEDTLS_CIPHER_MODE_XTS)
void mbedtls_aes_xts_init(mbedtls_aes_xts_context *ctx)
{
    AES_VALIDATE(ctx != NULL);

    mbedtls_aes_init(&ctx->crypt);
    mbedtls_aes_init(&ctx->tweak);
}

void mbedtls_aes_xts_free(mbedtls_aes_xts_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_aes_free(&ctx->crypt);
    mbedtls_aes_free(&ctx->tweak);
}
#endif /* MBEDTLS_CIPHER_MODE_XTS */

/*
 *  * AES key schedule (encryption)
 *   */
int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits)
{

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    return (aes_set_key(ctx, key, keybits));
}

/*
 *  * AES key schedule (decryption)
 *   */
int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits)
{
    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    return (aes_set_key(ctx, key, keybits));
}

#if defined(MBEDTLS_CIPHER_MODE_XTS)
static int mbedtls_aes_xts_decode_keys(const unsigned char *key,
                                       unsigned int keybits,
                                       const unsigned char **key1,
                                       unsigned int *key1bits,
                                       const unsigned char **key2,
                                       unsigned int *key2bits)
{
    const unsigned int half_keybits = keybits / 2;
    const unsigned int half_keybytes = half_keybits / 8;

    switch (keybits) {
    case 256:
        break;
    case 512:
        break;
    default :
        return (MBEDTLS_ERR_AES_INVALID_KEY_LENGTH);
    }

    *key1bits = half_keybits;
    *key2bits = half_keybits;
    *key1 = &key[0];
    *key2 = &key[half_keybytes];

    return (0);
}

int mbedtls_aes_xts_setkey_enc(mbedtls_aes_xts_context *ctx,
                               const unsigned char *key,
                               unsigned int keybits)
{
    int ret;
    const unsigned char *key1, *key2;
    unsigned int key1bits, key2bits;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    ret = mbedtls_aes_xts_decode_keys(key, keybits, &key1, &key1bits,
                                      &key2, &key2bits);
    if (ret != 0) {
        return (ret);
    }

    for (int i = 0; i < (keybits / 32); i++) {
        GET_UINT32_BE(ctx->crypt.aes_key2[i], key2, 4 * i);
    }
    ctx->crypt.hcryp_aes.Init.pKey2 = ctx->crypt.aes_key2;

    /* Set crypt key for encryption. */
    return mbedtls_aes_setkey_enc(&ctx->crypt, key1, key1bits);
}

int mbedtls_aes_xts_setkey_dec(mbedtls_aes_xts_context *ctx,
                               const unsigned char *key,
                               unsigned int keybits)
{
    int ret;
    const unsigned char *key1, *key2;
    unsigned int key1bits, key2bits;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(key != NULL);

    ret = mbedtls_aes_xts_decode_keys(key, keybits, &key1, &key1bits,
                                      &key2, &key2bits);
    if (ret != 0) {
        return (ret);
    }


    for (int i = 0; i < (keybits / 32); i++) {
        GET_UINT32_BE(ctx->crypt.aes_key2[i], key2, 4 * i);
    }
    ctx->crypt.hcryp_aes.Init.pKey2 = ctx->crypt.aes_key2;

    /* Set crypt key for decryption. */
    return mbedtls_aes_setkey_dec(&ctx->crypt, key1, key1bits);
}
#endif /* MBEDTLS_CIPHER_MODE_XTS */

/*
 *  * AES-ECB block encryption/decryption
 *   */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx,
                          int mode,
                          const unsigned char input[16],
                          unsigned char output[16])
{
    int ret = 0;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    ctx->hcryp_aes.Init.DataType = CRYP_DATATYPE_8B;
    ctx->hcryp_aes.Init.pKey = ctx->aes_key;

    /* Set the Algo if not configured till now */
    if (CRYP_AES_ECB != ctx->hcryp_aes.Init.Algorithm) {
        ctx->hcryp_aes.Init.Algorithm  = CRYP_AES_ECB;

        /* Configure the CRYP  */
        if (HAL_CRYP_SetConfig(&ctx->hcryp_aes,
                               &ctx->hcryp_aes.Init) != HAL_OK) {
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    }

    if (mode == MBEDTLS_AES_DECRYPT) {
        /* AES decryption */
        ret = mbedtls_internal_aes_decrypt(ctx, input, output);
        if (ret != 0) {
            goto exit;
        }
    } else {
        /* AES encryption */
        ret = mbedtls_internal_aes_encrypt(ctx, input, output);
        if (ret != 0) {
            goto exit;
        }
    }

exit:
    /* Free context access */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_unlock(&cryp_mutex) != 0) {
        ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
#endif /* MBEDTLS_THREADING_C */

    return (ret);
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 *  * AES-CBC buffer encryption/decryption
 *   */
int mbedtls_aes_crypt_cbc(mbedtls_aes_context *ctx,
                          int mode,
                          size_t length,
                          unsigned char iv[16],
                          const unsigned char *input,
                          unsigned char *output)
{
    unsigned int i;
    uint32_t iv_32B[4];
    int ret = 0;

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    if (length % 16) {
        return (MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH);
    }

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    /* Set the Algo if not configured till now */
    if (CRYP_AES_CBC != ctx->hcryp_aes.Init.Algorithm) {
        ctx->hcryp_aes.Init.Algorithm  = CRYP_AES_CBC;
        ctx->hcryp_aes.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
    }

    /* Set IV with invert endianness */
    for (i = 0; i < 4; i++) {
        GET_UINT32_BE(iv_32B[i], iv, 4 * i);
    }
    ctx->hcryp_aes.Init.pInitVect = iv_32B;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_aes,
                           &ctx->hcryp_aes.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

    if (mode == MBEDTLS_AES_DECRYPT) {
        /* current input is the IV vector for the next decrypt */
        memcpy(iv, input, 16);

#if AES_USE_CPU_MODE//use_cpu_mode
        if (HAL_CRYP_Decrypt(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Decrypt_DMA(&ctx->hcryp_aes,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {

#endif

            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    } else {

#if AES_USE_CPU_MODE//use_cpu_mode
        if (HAL_CRYP_Encrypt(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_aes,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {

#endif
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }

        /* current output is the IV vector for the next encrypt */
        memcpy(iv, output, 16);
    }


exit:
    /* Free context access */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_unlock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    return (ret);
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_XTS)

/* Endianness with 64 bits values */
#ifndef GET_UINT64_LE
#define GET_UINT64_LE(n,b,i)                            \
{                                                       \
	(n) = ( (uint64_t) (b)[(i) + 7] << 56 )             \
	| ( (uint64_t) (b)[(i) + 6] << 48 )             \
	| ( (uint64_t) (b)[(i) + 5] << 40 )             \
	| ( (uint64_t) (b)[(i) + 4] << 32 )             \
	| ( (uint64_t) (b)[(i) + 3] << 24 )             \
	| ( (uint64_t) (b)[(i) + 2] << 16 )             \
	| ( (uint64_t) (b)[(i) + 1] <<  8 )             \
	| ( (uint64_t) (b)[(i)    ]       );            \
}
#endif

#ifndef PUT_UINT64_LE
#define PUT_UINT64_LE(n,b,i)                            \
{                                                       \
	(b)[(i) + 7] = (unsigned char) ( (n) >> 56 );       \
	(b)[(i) + 6] = (unsigned char) ( (n) >> 48 );       \
	(b)[(i) + 5] = (unsigned char) ( (n) >> 40 );       \
	(b)[(i) + 4] = (unsigned char) ( (n) >> 32 );       \
	(b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
	(b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
	(b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
	(b)[(i)    ] = (unsigned char) ( (n)       );       \
}
#endif

/*
 *  * GF(2^128) multiplication function
 *   *
 *    * This function multiplies a field element by x in the polynomial field
 *     * representation. It uses 64-bit word operations to gain speed but compensates
 *      * for machine endianness and hence works correctly on both big and little
 *       * endian machines.
 *        */
static void mbedtls_gf128mul_x_ble(unsigned char r[16],
                                   const unsigned char x[16])
{
    uint64_t a, b, ra, rb;

    GET_UINT64_LE(a, x, 0);
    GET_UINT64_LE(b, x, 8);

    ra = (a << 1)  ^ 0x0087 >> (8 - ((b >> 63) << 3));
    rb = (a >> 63) | (b << 1);

    PUT_UINT64_LE(ra, r, 0);
    PUT_UINT64_LE(rb, r, 8);
}

/*
 *  * AES-XTS buffer encryption/decryption
 *   */
int mbedtls_aes_crypt_xts(mbedtls_aes_xts_context *ctx,
                          int mode,
                          size_t length,
                          const unsigned char data_unit[16],
                          const unsigned char *input,
                          unsigned char *output)
{
    int ret = 0;

    uint32_t tweak_32B[4];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(data_unit != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    /* Data units must be at least 16 bytes long. */
    if (length < 16) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    /* NIST SP 800-38E disallows data units larger than 2**20 blocks. */
    if (length > (1 << 20) * 16) {
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

    /* Set the Algo if not configured till now */
    if (CRYP_AES_XTS != ctx->crypt.hcryp_aes.Init.Algorithm) {
        ctx->crypt.hcryp_aes.Init.Algorithm  = CRYP_AES_XTS;
        ctx->crypt.hcryp_aes.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
    }

    for (int i = 0; i < 4; i++) {
        GET_UINT32_BE(tweak_32B[i], data_unit, 4 * i);
    }
    ctx->crypt.hcryp_aes.Init.Tweak = tweak_32B;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->crypt.hcryp_aes,
                           &ctx->crypt.hcryp_aes.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

    if (mode == MBEDTLS_AES_DECRYPT) {
#if AES_USE_CPU_MODE//use_cpu_mode
        if (HAL_CRYP_Decrypt(&ctx->crypt.hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Decrypt_DMA(&ctx->crypt.hcryp_aes,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {
#endif

            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    } else {
#if AES_USE_CPU_MODE//use_cpu_mode
        if (HAL_CRYP_Encrypt(&ctx->crypt.hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Encrypt_DMA(&ctx->crypt.hcryp_aes,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {
#endif

            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    }

exit:

    return ret;

}
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
/*
 *  * AES-CFB128 buffer encryption/decryption
 *   */
int mbedtls_aes_crypt_cfb128(mbedtls_aes_context *ctx,
                             int mode,
                             size_t length,
                             size_t *iv_off,
                             unsigned char iv[16],
                             const unsigned char *input,
                             unsigned char *output)
{
    unsigned int i;
    int ret = 0;

    uint32_t iv_32B[4];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(iv_off != NULL);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    /* Set the Algo if not configured till now */
    if (CRYP_AES_CFB != ctx->hcryp_aes.Init.Algorithm) {
        ctx->hcryp_aes.Init.Algorithm  = CRYP_AES_CFB;
        ctx->hcryp_aes.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
    }

    /* Set IV with invert endianness */
    for (i = 0; i < 4; i++) {
        GET_UINT32_BE(iv_32B[i], iv, 4 * i);
    }

    ctx->hcryp_aes.Init.pInitVect = iv_32B;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_aes,
                           &ctx->hcryp_aes.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

    if (mode == MBEDTLS_AES_DECRYPT) {
#if AES_USE_CPU_MODE//use_cpu_mode
        if (HAL_CRYP_Decrypt(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Decrypt_DMA(&ctx->hcryp_aes,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {
#endif

            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    } else {
#if AES_USE_CPU_MODE//use_cpu_mode

        if (HAL_CRYP_Encrypt(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {

#else //dma_mode

        if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_aes,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {
#endif

            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    }

exit:
    return ret;
}

/*
 *  * AES-CFB8 buffer encryption/decryption
 *   */
int mbedtls_aes_crypt_cfb8(mbedtls_aes_context *ctx,
                           int mode,
                           size_t length,
                           unsigned char iv[16],
                           const unsigned char *input,
                           unsigned char *output)
{
    int ret;
    unsigned char c;
    unsigned char ov[17];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(mode == MBEDTLS_AES_ENCRYPT ||
                     mode == MBEDTLS_AES_DECRYPT);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    while (length--) {
        memcpy(ov, iv, 16);
        ret = mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);
        if (ret != 0) {
            return (ret);
        }

        if (mode == MBEDTLS_AES_DECRYPT) {
            ov[16] = *input;
        }

        c = *output++ = (unsigned char)(iv[0] ^ *input++);

        if (mode == MBEDTLS_AES_ENCRYPT) {
            ov[16] = c;
        }

        memcpy(iv, ov + 1, 16);
    }

    return (0);
}

#endif /*MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_OFB)
/*
 *  * AES-OFB (Output Feedback Mode) buffer encryption/decryption
 *   */
int mbedtls_aes_crypt_ofb(mbedtls_aes_context *ctx,
                          size_t length,
                          size_t *iv_off,
                          unsigned char iv[16],
                          const unsigned char *input,
                          unsigned char *output)
{
    int ret = 0;
    unsigned int i;

    uint32_t iv_32B[4];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(iv_off != NULL);
    AES_VALIDATE_RET(iv != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    /* Set the Algo if not configured till now */
    if (CRYP_AES_OFB != ctx->hcryp_aes.Init.Algorithm) {
        ctx->hcryp_aes.Init.Algorithm  = CRYP_AES_OFB;
        ctx->hcryp_aes.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
    }

    /* Set IV with invert endianness */
    for (i = 0; i < 4; i++) {
        GET_UINT32_BE(iv_32B[i], iv, 4 * i);
    }

    ctx->hcryp_aes.Init.pInitVect = iv_32B;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_aes,
                           &ctx->hcryp_aes.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

#if AES_USE_CPU_MODE//use_cpu_mode
    if (HAL_CRYP_Encrypt(&ctx->hcryp_aes,
                         (uint32_t *)input,
                         length,
                         (uint32_t *)output,
                         0) != HAL_OK) {
#else //dma_mode

    if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output) != HAL_OK) {
#endif
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

exit:
    return (ret);
}
#endif /* MBEDTLS_CIPHER_MODE_OFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/*
 *  * AES-CTR buffer encryption/decryption
 *   */
int mbedtls_aes_crypt_ctr(mbedtls_aes_context *ctx,
                          size_t length,
                          size_t *nc_off,
                          unsigned char nonce_counter[16],
                          unsigned char stream_block[16],
                          const unsigned char *input,
                          unsigned char *output)
{
    unsigned int i;
    int ret = 0;
    uint32_t iv_32B[4];

    AES_VALIDATE_RET(ctx != NULL);
    AES_VALIDATE_RET(nc_off != NULL);
    AES_VALIDATE_RET(nonce_counter != NULL);
    AES_VALIDATE_RET(stream_block != NULL);
    AES_VALIDATE_RET(input != NULL);
    AES_VALIDATE_RET(output != NULL);

    /* Set the Algo if not configured till now */
    if (CRYP_AES_CTR != ctx->hcryp_aes.Init.Algorithm) {
        ctx->hcryp_aes.Init.Algorithm  = CRYP_AES_CTR;
        ctx->hcryp_aes.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
    }

    /* Set IV with invert endianness */
    for (i = 0; i < 4; i++) {
        GET_UINT32_BE(iv_32B[i], nonce_counter, 4 * i);
    }

    ctx->hcryp_aes.Init.pInitVect = iv_32B;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_aes,
                           &ctx->hcryp_aes.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

#if AES_USE_CPU_MODE//use_cpu_mode
    if (HAL_CRYP_Encrypt(&ctx->hcryp_aes,
                         (uint32_t *)input,
                         length,
                         (uint32_t *)output,
                         0) != HAL_OK) {
#else //dma_mode

    if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output) != HAL_OK) {


#endif
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

exit:
    return ret;

}
#endif /* MBEDTLS_CIPHER_MODE_CTR */

int mbedtls_internal_aes_encrypt(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16])
{

#if AES_USE_CPU_MODE//use_cpu_mode
    if (HAL_CRYP_Encrypt(&ctx->hcryp_aes,
                         (uint32_t *)input,
                         16,
                         (uint32_t *)output,
                         0) != HAL_OK) {
#else //dma_mode

    if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             16,
                             (uint32_t *)output) != HAL_OK) {
#endif

        return (MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED);
    }
    return (0);

}

int mbedtls_internal_aes_decrypt(mbedtls_aes_context *ctx,
                                 const unsigned char input[16],
                                 unsigned char output[16])
{
#if AES_USE_CPU_MODE//use_cpu_mode
    if (HAL_CRYP_Decrypt(&ctx->hcryp_aes,
                         (uint32_t *)input,
                         16,
                         (uint32_t *)output,
                         0) != HAL_OK) {
#else //dma_mode

    if (HAL_CRYP_Decrypt_DMA(&ctx->hcryp_aes,
                             (uint32_t *)input,
                             16,
                             (uint32_t *)output) != HAL_OK) {

#endif
        return (MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED);
    }
    return (0);
}

#if defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_aes_encrypt(mbedtls_aes_context *ctx,
                         const unsigned char input[16],
                         unsigned char output[16])
{
#error "mbedtls_aes_encrypt() is a deprecated function (not implemented)"
}

void mbedtls_aes_decrypt(mbedtls_aes_context *ctx,
                         const unsigned char input[16],
                         unsigned char output[16])
{
#error "mbedtls_aes_decrypt() is a deprecated function (not implemented)"
}
#endif /* MBEDTLS_DEPRECATED_REMOVED */


#endif //MBEDTLS_AES_ALT
