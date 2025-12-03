/*
 *  NIST SP800-38D compliant GCM implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
 *
 * See also:
 * [MGV] http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-revised-spec.pdf
 *
 * We use the algorithm described as Shoup's method with 4-bit tables in
 * [MGV] 4.1, pp. 12-13, to enhance speed without using too much memory.
 */

#include "common.h"

#if defined(MBEDTLS_GCM_C)

#include "mbedtls/gcm.h"
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>

#if defined(MBEDTLS_AESNI_C)
#include "aesni.h"
#endif

#if defined(MBEDTLS_AESCE_C)
#include "aesce.h"
#endif

#if defined(MBEDTLS_GCM_ALT)

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            	\
	do {                                                    \
		(n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
			| ( (uint32_t) (b)[(i) + 1] << 16 )             \
			| ( (uint32_t) (b)[(i) + 2] <<  8 )             \
			| ( (uint32_t) (b)[(i) + 3]       );            \
	} while( 0 )
#endif

#ifndef GET_UINT32_LE
#define GET_UINT32_LE(n,b,i)                            	\
	do {                                                    \
		(n) = ( (uint32_t) (b)[(i)    ]       )             \
			| ( (uint32_t) (b)[(i) + 1] <<  8 )             \
			| ( (uint32_t) (b)[(i) + 2] << 16 )             \
			| ( (uint32_t) (b)[(i) + 3] << 24 );            \
	} while( 0 )
#endif


/* Parameter validation macros */
#define GCM_VALIDATE_RET( cond ) \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_GCM_BAD_INPUT )
#define GCM_VALIDATE( cond ) \
    MBEDTLS_INTERNAL_VALIDATE( cond )

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define IV_LENGTH        12U   /* implementations restrict support to 96 bits */

#if !defined(STM32_AAD_ANY_LENGTH_SUPPORT)
#define AAD_WORD_ALIGN   4U   /* implementations may restrict AAD support on  */
/* a buffer multiple of 32 bits                 */
#endif

/*
 * Initialize a context
 */
void mbedtls_gcm_init(mbedtls_gcm_context *ctx)
{
    GCM_VALIDATE(ctx != NULL);
    memset((void *)ctx, 0, sizeof(mbedtls_gcm_context));
}

int mbedtls_gcm_setkey(mbedtls_gcm_context *ctx,
                       mbedtls_cipher_id_t cipher,
                       const unsigned char *key,
                       unsigned int keybits)
{
    unsigned int i;
    int ret = 0;

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(key != NULL);

#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    switch (keybits) {
    case 128:
        ctx->hcryp_gcm.Init.KeySize = CRYP_KEYSIZE_128B;
        break;

    case 192:
        ctx->hcryp_gcm.Init.KeySize = CRYP_KEYSIZE_192B;
        break;

    case 256:
        ctx->hcryp_gcm.Init.KeySize = CRYP_KEYSIZE_256B;
        break;

    default :
        ret = MBEDTLS_ERR_GCM_BAD_INPUT;
        goto exit;
    }

    /* Format and fill AES key  */
    for (i = 0; i < (keybits / 32); i++) {
        GET_UINT32_BE(ctx->gcm_key[i], key, 4 * i);
    }

    ctx->hcryp_gcm.Init.Algorithm  = CRYP_AES_GCM;

    ctx->hcryp_gcm.Init.DataType = CRYP_DATATYPE_8B;
    ctx->hcryp_gcm.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;
    ctx->hcryp_gcm.Init.pKey = ctx->gcm_key;

    if (HAL_CRYP_Init(&ctx->hcryp_gcm) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }


exit :
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_unlock(&cryp_mutex) != 0) {
        ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
#endif /* MBEDTLS_THREADING_C */

    return (ret);
}


uint32_t __REV_DATA(uint32_t value)
{
    uint32_t result;

    result = ((value << 24) & 0xff000000) | ((value << 8) & 0x00ff0000) | ((value >> 24) & 0x000000ff) | ((value >> 8) & 0x0000ff00);

    return result;

}

int mbedtls_gcm_starts(mbedtls_gcm_context *ctx,
                       int mode,
                       const unsigned char *iv,
                       size_t iv_len)
{
    int ret = 0;
    unsigned int i;
    static uint32_t iv_32B[4] __attribute__((aligned(4)));

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(mode != MBEDTLS_GCM_ENCRYPT || mode != MBEDTLS_GCM_DECRYPT);
    GCM_VALIDATE_RET(iv != NULL);

    /* IV and AD are limited to 2^64 bits, so 2^61 bytes */
    /* IV is not allowed to be zero length */
    if (iv_len == 0 ||
        ((uint64_t) iv_len) >> 61 != 0) {
        return (MBEDTLS_ERR_GCM_BAD_INPUT);
    }

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    if (HAL_CRYP_Init(&ctx->hcryp_gcm) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

    ctx->mode = mode;
    ctx->len = 0;

    /* Set IV with invert endianness */
    if (iv_len == 12) {
        ctx->hcryp_gcm.Init.Gcm_Iv_Not96_En = 0;
        for (i = 0; i < 3; i++) {
            GET_UINT32_BE(iv_32B[i], iv, 4 * i);
        }

        /* According to NIST specification, the counter value is 0x2 when
        processing the first block of payload */
        iv_32B[3] = 0x00000002;

        ctx->hcryp_gcm.Init.pInitVect = iv_32B;
    } else {
        ctx->hcryp_gcm.Init.Gcm_Iv_Not96_En = 1;
        u32 iv_data_len;
        iv_data_len = ((iv_len / 16) + 2) * 4;

        ctx->hcryp_gcm.Init.gcm_mbed_iv = malloc(iv_data_len * 4);
        if (!ctx->hcryp_gcm.Init.gcm_mbed_iv) {
            printf("ctx->hcryp_gcm.Init.gcm_mbed_iv malloc err!");
        }

        for (i = 0; i < (iv_len / 4); i++) {
            GET_UINT32_LE(ctx->hcryp_gcm.Init.gcm_mbed_iv[i], iv, 4 * i);
        }
        for (i = (iv_len / 4); i < iv_data_len; i++) {
            ctx->hcryp_gcm.Init.gcm_mbed_iv[i] = 0;
        }
        ctx->hcryp_gcm.Init.gcm_mbed_iv[iv_data_len - 1] = __REV_DATA(iv_len * 8);

        ctx->hcryp_gcm.Init.Gcm_Ivlen = iv_data_len * 4;
        ctx->hcryp_gcm.Init.pInitVect = ctx->hcryp_gcm.Init.gcm_mbed_iv;
    }


    /* Do not Allow IV reconfiguration at every gcm update */
    ctx->hcryp_gcm.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ONCE;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_gcm, &ctx->hcryp_gcm.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
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

int mbedtls_gcm_update_ad(mbedtls_gcm_context *ctx,
                          const unsigned char *add, size_t add_len)
{
    int ret = 0;

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(add_len == 0 || add != NULL);

    /* IV and AD are limited to 2^64 bits, so 2^61 bytes */
    if (((uint64_t) add_len) >> 61 != 0) {
        return (MBEDTLS_ERR_GCM_BAD_INPUT);
    }
#if !defined(STM32_AAD_ANY_LENGTH_SUPPORT)
    /* implementation restrict support to a buffer multiple of 32 bits */
    if ((add_len % AAD_WORD_ALIGN) != 0U) {
        return (MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED);
    }
#endif

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    if (HAL_CRYP_Init(&ctx->hcryp_gcm) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

    if (add_len > 0) {
        ctx->hcryp_gcm.Init.Header = (uint32_t *)add;
#if defined(STM32_AAD_ANY_LENGTH_SUPPORT)
        /* header buffer in byte length */
        ctx->hcryp_gcm.Init.HeaderSize = (uint32_t)add_len;
#else
        /* header buffer in word length */
        ctx->hcryp_gcm.Init.HeaderSize = (uint32_t)(add_len / AAD_WORD_ALIGN);
#endif
    } else {
        ctx->hcryp_gcm.Init.Header = NULL;
        ctx->hcryp_gcm.Init.HeaderSize = 0;
    }

#if defined(STM32_AAD_ANY_LENGTH_SUPPORT)
    /* Additional Authentication Data in bytes unit */
    ctx->hcryp_gcm.Init.HeaderWidthUnit = CRYP_HEADERWIDTHUNIT_BYTE;
#endif

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_gcm, &ctx->hcryp_gcm.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
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

int mbedtls_gcm_update(mbedtls_gcm_context *ctx,
                       const unsigned char *input, size_t input_length,
                       unsigned char *output, size_t output_size,
                       size_t *output_length)
{
    int ret = 0;

    if (output_size < input_length) {
        return MBEDTLS_ERR_GCM_BUFFER_TOO_SMALL;
    }
    *output_length = input_length;

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(input_length == 0 || input != NULL);
    GCM_VALIDATE_RET(&output_length == 0 || output != NULL);

    if (output > input && (size_t)(output - input) < input_length) {
        return (MBEDTLS_ERR_GCM_BAD_INPUT);
    }

    /* Total length is restricted to 2^39 - 256 bits, ie 2^36 - 2^5 bytes
     * Also check for possible overflow */
    if (((ctx->len + input_length) < ctx->len) ||
        ((uint64_t)(ctx->len + input_length) > 0xFFFFFFFE0ull)) {
        return (MBEDTLS_ERR_GCM_BAD_INPUT);
    }

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    /* allow multi-context of CRYP use: restore context */
    /* ctx->hcryp_gcm.Instance->CR = ctx->ctx_save_cr; */

    ctx->len += input_length;

    if (ctx->mode == MBEDTLS_GCM_DECRYPT) {
#if 1 //cpu_mode
        if (HAL_CRYP_Decrypt(&ctx->hcryp_gcm,
                             (uint32_t *)input,
                             input_length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Decrypt_DMA(&ctx->hcryp_gcm,
                                 (uint32_t *)input,
                                 input_length,
                                 (uint32_t *)output) != HAL_OK) {

#endif
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }
    } else {

#if 1 //cpu_mode
        if (HAL_CRYP_Encrypt(&ctx->hcryp_gcm,
                             (uint32_t *)input,
                             input_length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_gcm,
                                 (uint32_t *)input,
                                 input_length,
                                 (uint32_t *)output) != HAL_OK) {

#endif
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
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


int mbedtls_gcm_finish(mbedtls_gcm_context *ctx,
                       unsigned char *output, size_t output_size,
                       size_t *output_length,
                       unsigned char *tag, size_t tag_len)
{
    int ret = 0;
    uint8_t mac[16] __attribute__((aligned(4))); /* temporary mac         */

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(tag != NULL);

    if (tag_len > 16 || tag_len < 4) {
        return (MBEDTLS_ERR_GCM_BAD_INPUT);
    }

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    /* Tag has a variable length */
    memset(mac, 0, sizeof(mac));

    /* Generate the authentication TAG */
    if (HAL_CRYPEx_AESGCM_GenerateAuthTAG(&ctx->hcryp_gcm,
                                          (uint32_t *)mac,
                                          0) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto exit;
    }

    memcpy(tag, mac, tag_len);

exit:
    /* Free context access */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_unlock(&cryp_mutex) != 0) {
        ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
#endif /* MBEDTLS_THREADING_C */

    return (ret);
}

int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *ctx,
                              int mode,
                              size_t length,
                              const unsigned char *iv,
                              size_t iv_len,
                              const unsigned char *add,
                              size_t add_len,
                              const unsigned char *input,
                              unsigned char *output,
                              size_t tag_len,
                              unsigned char *tag)
{
    int ret;

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(iv != NULL);
    GCM_VALIDATE_RET(add_len == 0 || add != NULL);
    GCM_VALIDATE_RET(length == 0 || input != NULL);
    GCM_VALIDATE_RET(length == 0 || output != NULL);
    GCM_VALIDATE_RET(tag != NULL);

    size_t olen;

    if ((ret = mbedtls_gcm_starts(ctx, mode, iv, iv_len)) != 0) {
        return ret;
    }

    if ((ret = mbedtls_gcm_update_ad(ctx, add, add_len)) != 0) {
        return ret;
    }

    if ((ret = mbedtls_gcm_update(ctx, input, length,
                                  output, length, &olen)) != 0) {
        return ret;
    }

    if ((ret = mbedtls_gcm_finish(ctx, NULL, 0, &olen, tag, tag_len)) != 0) {
        return ret;
    }

    return (0);
}

int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context *ctx,
                             size_t length,
                             const unsigned char *iv,
                             size_t iv_len,
                             const unsigned char *add,
                             size_t add_len,
                             const unsigned char *tag,
                             size_t tag_len,
                             const unsigned char *input,
                             unsigned char *output)
{
    int ret;
    unsigned char check_tag[16];
    size_t i;
    int diff;

    GCM_VALIDATE_RET(ctx != NULL);
    GCM_VALIDATE_RET(iv != NULL);
    GCM_VALIDATE_RET(add_len == 0 || add != NULL);
    GCM_VALIDATE_RET(tag != NULL);
    GCM_VALIDATE_RET(length == 0 || input != NULL);
    GCM_VALIDATE_RET(length == 0 || output != NULL);

    if ((ret = mbedtls_gcm_crypt_and_tag(ctx, MBEDTLS_GCM_DECRYPT, length,
                                         iv, iv_len, add, add_len,
                                         input, output, tag_len, check_tag)) != 0) {
        return (ret);
    }

    /* Check tag in "constant-time" */
    for (diff = 0, i = 0; i < tag_len; i++) {
        diff |= tag[i] ^ check_tag[i];
    }

    if (diff != 0) {
        mbedtls_platform_zeroize(output, length);
        return (MBEDTLS_ERR_GCM_AUTH_FAILED);
    }

    return (0);
}

void mbedtls_gcm_free(mbedtls_gcm_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->hcryp_gcm.Init.gcm_mbed_iv) {
        free(ctx->hcryp_gcm.Init.gcm_mbed_iv);
    }

    memset((void *)ctx, 0, sizeof(mbedtls_gcm_context));
}


#endif /* !MBEDTLS_GCM_ALT */


#endif /* MBEDTLS_GCM_C */
