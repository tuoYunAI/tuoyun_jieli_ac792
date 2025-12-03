/*
 *  NIST SP800-38C compliant CCM implementation
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
 * Definition of CCM:
 * http://csrc.nist.gov/publications/nistpubs/800-38C/SP800-38C_updated-July20_2007.pdf
 * RFC 3610 "Counter with CBC-MAC (CCM)"
 *
 * Related:
 * RFC 5116 "An Interface and Algorithms for Authenticated Encryption"
 */

#include "common.h"

#if defined(MBEDTLS_CCM_C)

#include "mbedtls/ccm.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#if defined(MBEDTLS_SELF_TEST) && defined(MBEDTLS_AES_C)
#include <stdio.h>
#define mbedtls_printf printf
#endif /* MBEDTLS_SELF_TEST && MBEDTLS_AES_C */
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_CCM_ALT)

int mbedtls_ccm_starts(mbedtls_ccm_context *ctx,
                       int mode,
                       const unsigned char *iv,
                       size_t iv_len)
{
    return 0;
}

int mbedtls_ccm_set_lengths(mbedtls_ccm_context *ctx,
                            size_t total_ad_len,
                            size_t plaintext_len,
                            size_t tag_len)
{
    return 0;
}

int mbedtls_ccm_update_ad(mbedtls_ccm_context *ctx,
                          const unsigned char *add,
                          size_t add_len)
{
    return 0;
}

int mbedtls_ccm_update(mbedtls_ccm_context *ctx,
                       const unsigned char *input, size_t input_len,
                       unsigned char *output, size_t output_size,
                       size_t *output_len)
{
    return 0;
}

int mbedtls_ccm_finish(mbedtls_ccm_context *ctx,
                       unsigned char *tag, size_t tag_len)
{
    return 0;
}

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
			| ( (uint32_t) (b)[(i) + 1] << 8  )             \
			| ( (uint32_t) (b)[(i) + 2] << 16 )             \
			| ( (uint32_t) (b)[(i) + 3] << 24 );            \
	} while( 0 )
#endif



/* Parameter validation macros */
#define CCM_VALIDATE_RET( cond ) \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_CCM_BAD_INPUT )
#define CCM_VALIDATE( cond ) \
    MBEDTLS_INTERNAL_VALIDATE( cond )


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define CCM_ENCRYPT 0
#define CCM_DECRYPT 1

#define H_LENGTH 2 /* Formatting of the Associated Data */
/* If 0 < a < 2e16-2e8, */
/* then a is encoded as [a]16, i.e., two octets */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*
 * Initialize context
 */
void mbedtls_ccm_init(mbedtls_ccm_context *ctx)
{
    CCM_VALIDATE(ctx != NULL);

    memset((void *)ctx, 0, sizeof(mbedtls_ccm_context));
}

int mbedtls_ccm_setkey(mbedtls_ccm_context *ctx,
                       mbedtls_cipher_id_t cipher,
                       const unsigned char *key,
                       unsigned int keybits)
{
    unsigned int i;
    int ret = 0;

    CCM_VALIDATE_RET(ctx != NULL);
    CCM_VALIDATE_RET(key != NULL);

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    switch (keybits) {
    case 128:
        ctx->hcryp_ccm.Init.KeySize = CRYP_KEYSIZE_128B;;
        break;

    case 192:
        ctx->hcryp_ccm.Init.KeySize = CRYP_KEYSIZE_192B;
        break;

    case 256:
        ctx->hcryp_ccm.Init.KeySize = CRYP_KEYSIZE_256B;
        break;

    default :
        ret = MBEDTLS_ERR_CCM_BAD_INPUT;
        goto exit;
    }

    /* Format and fill AES key  */
    for (i = 0; i < (keybits / 32) ; i++) {
        GET_UINT32_BE(ctx->ccm_key[i], key, 4 * i);
    }

    ctx->hcryp_ccm.Init.DataType = CRYP_DATATYPE_8B;
    ctx->hcryp_ccm.Init.pKey = ctx->ccm_key;
    ctx->hcryp_ccm.Init.pInitVect  = NULL;
    ctx->hcryp_ccm.Init.Algorithm  = CRYP_AES_CCM;
    ctx->hcryp_ccm.Init.Header     = NULL;
    ctx->hcryp_ccm.Init.HeaderSize = 0;
    ctx->hcryp_ccm.Init.B0 = NULL;
    ctx->hcryp_ccm.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;

    if (HAL_CRYP_Init(&ctx->hcryp_ccm) != HAL_OK) {
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

/*
 * Free context
 */
void mbedtls_ccm_free(mbedtls_ccm_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset((void *)ctx, 0, sizeof(mbedtls_ccm_context));
}







/*
 * Authenticated encryption or decryption
 */
static int ccm_auth_crypt(mbedtls_ccm_context *ctx, int mode, size_t length,
                          const unsigned char *iv, size_t iv_len,
                          const unsigned char *add, size_t add_len,
                          const unsigned char *input, unsigned char *output,
                          unsigned char *tag, size_t tag_len)
{
    int ret = 0;
    unsigned char i;
    unsigned char q;
    size_t len_left;
    unsigned int j;

    unsigned char b0[16] __attribute__((aligned(4)));  /* Formatting of B0   */
    uint32_t b0_32B[4]  __attribute__((aligned(4)));  /* B0 data swapping   */
    unsigned char *b1_padded_addr = NULL;            /* Formatting of B1   */
    unsigned char *b1_aligned_addr = NULL;
    size_t b1_length;                                /* B1 with padding    */
    uint8_t b1_padding;                              /* B1 word alignment */

    uint8_t mac[16] __attribute__((aligned(4)));  /* temporary mac      */


    CCM_VALIDATE_RET(mode != CCM_ENCRYPT || mode != CCM_DECRYPT);

    /*
     * Check length requirements: SP800-38C A.1
     * Additional requirement: a < 2^16 - 2^8 to simplify the code.
     * 'length' checked later (when writing it to the first block)
     *
     * Also, loosen the requirements to enable support for CCM* (IEEE 802.15.4).
     */

    /* tag_len, aka t, is an element of {4, 6, 8, 10, 12, 14, 16} */
    if (tag_len < 4 || tag_len > 16 || tag_len % 2 != 0) {
        return (MBEDTLS_ERR_CCM_BAD_INPUT);
    }

    /* Also implies q is within bounds */
    /* iv_len, aka n, is an element of {7, 8, 9, 10, 11, 12, 13} */
    if (iv_len < 7 || iv_len > 13) {
        return (MBEDTLS_ERR_CCM_BAD_INPUT);
    }

    /* add_len, aka a, a < 2^16 - 2^8 */
    if (add_len > 0xFF00) {
        return (MBEDTLS_ERR_CCM_BAD_INPUT);
    }

    /* The octet length of Q, denoted q */
    q = 15 - (unsigned char) iv_len;

    /*
     * First block B_0:
     * 0        .. 0        flags
     * 1        .. iv_len   nonce (aka iv)
     * iv_len+1 .. 15       length
     *
     * With flags as (bits):
     * 7        0
     * 6        add present?
     * 5 .. 3   (t - 2) / 2
     * 2 .. 0   q - 1
     */
    memset(b0, 0, 16);
    if (add_len > 0) {
        b0[0] |= 0x40;
    }
    b0[0] |= ((tag_len - 2) / 2) << 3;
    b0[0] |= q - 1;

    /* Nonce concatenation */
    memcpy(b0 + 1, iv, iv_len);

    /* Data length concatenation */
    for (i = 0, len_left = length; i < q; i++, len_left >>= 8) {
        b0[15 - i] = (unsigned char)(len_left & 0xFF);
    }

    if (len_left > 0) {
        return (MBEDTLS_ERR_CCM_BAD_INPUT);
    }

    /* Protect context access                                  */
    /* (it may occur at a same time in a threaded environment) */
#if defined(MBEDTLS_THREADING_C)
    if (mbedtls_mutex_lock(&cryp_mutex) != 0) {
        return (MBEDTLS_ERR_THREADING_MUTEX_ERROR);
    }
#endif /* MBEDTLS_THREADING_C */

    /*
    * If there is additional data, update with
    * add_len, add, 0 (padding to a block boundary)
    */
    if (add_len > 0) {
        /* Extra bytes to deal with data padding such that         */
        /* the resulting string can be partitioned into words      */
        b1_padding = ((add_len + H_LENGTH) % 4);
        b1_length = add_len + H_LENGTH + b1_padding;

        /* reserve extra bytes to deal with 4-bytes memory alignment */
        b1_padded_addr =
            mbedtls_calloc(1, b1_length + 3);

        if (b1_padded_addr == NULL) {
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto exit;
        }

        /* move up to a 4-bytes aligned address in the reserved memory chuck */
        b1_aligned_addr = (unsigned char *)((uint32_t)(b1_padded_addr + 3) & 0xFFFFFFFC);

        /* Header length */
        b1_aligned_addr[0] = (unsigned char)((add_len >> 8) & 0xFF);
        b1_aligned_addr[1] = (unsigned char)((add_len) & 0xFF);

        /* data concatenation */
        memcpy(b1_aligned_addr + H_LENGTH, add, add_len);

        /* blocks (B) associated to the Associated Data (A) */
        ctx->hcryp_ccm.Init.Header     = (uint32_t *)b1_aligned_addr;

        ctx->hcryp_ccm.Init.HeaderSize = b1_length / 4;
    } else {
        ctx->hcryp_ccm.Init.Header = NULL;
        ctx->hcryp_ccm.Init.HeaderSize = 0;
    }

    /* first authentication block */
    for (j = 0; j < 4; j++) {
        GET_UINT32_BE(b0_32B[j], b0, 4 * j);
    }

    ctx->hcryp_ccm.Init.B0         = b0_32B;

    /* reconfigure the CRYP */
    if (HAL_CRYP_SetConfig(&ctx->hcryp_ccm, &ctx->hcryp_ccm.Init) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto free_block;
    }

    /* blocks (B) associated to the plaintext message (P) */
    if (mode == CCM_DECRYPT) {
#if 1//cpu_mode
        if (HAL_CRYP_Decrypt(&ctx->hcryp_ccm,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {
#else //dma_mode

        if (HAL_CRYP_Decrypt_DMA(&ctx->hcryp_ccm,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {

#endif
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto free_block;
        }
    } else {

#if 1 //cpu_mode

        if (HAL_CRYP_Encrypt(&ctx->hcryp_ccm,
                             (uint32_t *)input,
                             length,
                             (uint32_t *)output,
                             0) != HAL_OK) {

#else //dma_mode

        if (HAL_CRYP_Encrypt_DMA(&ctx->hcryp_ccm,
                                 (uint32_t *)input,
                                 length,
                                 (uint32_t *)output) != HAL_OK) {
#endif
            ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
            goto free_block;
        }
    }

    /* Tag has a variable length */
    memset(mac, 0, sizeof(mac));

    /* Generate the authentication TAG */
    if (HAL_CRYPEx_AESCCM_GenerateAuthTAG(&ctx->hcryp_ccm,
                                          (uint32_t *)mac,
                                          0) != HAL_OK) {
        ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        goto free_block;
    }

    memcpy(tag, mac, tag_len);


free_block:
    if (add_len > 0) {
        mbedtls_free(b1_padded_addr);
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

/*
 * Authenticated encryption
 */
int mbedtls_ccm_star_encrypt_and_tag(mbedtls_ccm_context *ctx, size_t length,
                                     const unsigned char *iv, size_t iv_len,
                                     const unsigned char *add, size_t add_len,
                                     const unsigned char *input, unsigned char *output,
                                     unsigned char *tag, size_t tag_len)
{
    CCM_VALIDATE_RET(ctx != NULL);
    CCM_VALIDATE_RET(iv != NULL);
    CCM_VALIDATE_RET(add_len == 0 || add != NULL);
    CCM_VALIDATE_RET(length == 0 || input != NULL);
    CCM_VALIDATE_RET(length == 0 || output != NULL);
    CCM_VALIDATE_RET(tag_len == 0 || tag != NULL);
    return (ccm_auth_crypt(ctx, CCM_ENCRYPT, length, iv, iv_len,
                           add, add_len, input, output, tag, tag_len));
}

int mbedtls_ccm_encrypt_and_tag(mbedtls_ccm_context *ctx, size_t length,
                                const unsigned char *iv, size_t iv_len,
                                const unsigned char *add, size_t add_len,
                                const unsigned char *input, unsigned char *output,
                                unsigned char *tag, size_t tag_len)
{
    CCM_VALIDATE_RET(ctx != NULL);
    CCM_VALIDATE_RET(iv != NULL);
    CCM_VALIDATE_RET(add_len == 0 || add != NULL);
    CCM_VALIDATE_RET(length == 0 || input != NULL);
    CCM_VALIDATE_RET(length == 0 || output != NULL);
    CCM_VALIDATE_RET(tag_len == 0 || tag != NULL);
    if (tag_len == 0) {
        return (MBEDTLS_ERR_CCM_BAD_INPUT);
    }

    return (mbedtls_ccm_star_encrypt_and_tag(ctx, length, iv, iv_len, add,
            add_len, input, output, tag, tag_len));
}

/*
 * Authenticated decryption
 */
int mbedtls_ccm_star_auth_decrypt(mbedtls_ccm_context *ctx, size_t length,
                                  const unsigned char *iv, size_t iv_len,
                                  const unsigned char *add, size_t add_len,
                                  const unsigned char *input, unsigned char *output,
                                  const unsigned char *tag, size_t tag_len)
{
    int ret;
    unsigned char check_tag[16];
    unsigned char i;
    int diff;

    CCM_VALIDATE_RET(ctx != NULL);
    CCM_VALIDATE_RET(iv != NULL);
    CCM_VALIDATE_RET(add_len == 0 || add != NULL);
    CCM_VALIDATE_RET(length == 0 || input != NULL);
    CCM_VALIDATE_RET(length == 0 || output != NULL);
    CCM_VALIDATE_RET(tag_len == 0 || tag != NULL);

    if ((ret = ccm_auth_crypt(ctx, CCM_DECRYPT, length,
                              iv, iv_len, add, add_len,
                              input, output, check_tag, tag_len)) != 0) {
        return (ret);
    }

    /* Check tag in "constant-time" */
    for (diff = 0, i = 0; i < tag_len; i++) {
        diff |= tag[i] ^ check_tag[i];
    }

    if (diff != 0) {
        mbedtls_platform_zeroize(output, length);
        return (MBEDTLS_ERR_CCM_AUTH_FAILED);
    }

    return (0);
}

int mbedtls_ccm_auth_decrypt(mbedtls_ccm_context *ctx, size_t length,
                             const unsigned char *iv, size_t iv_len,
                             const unsigned char *add, size_t add_len,
                             const unsigned char *input, unsigned char *output,
                             const unsigned char *tag, size_t tag_len)
{
    CCM_VALIDATE_RET(ctx != NULL);
    CCM_VALIDATE_RET(iv != NULL);
    CCM_VALIDATE_RET(add_len == 0 || add != NULL);
    CCM_VALIDATE_RET(length == 0 || input != NULL);
    CCM_VALIDATE_RET(length == 0 || output != NULL);
    CCM_VALIDATE_RET(tag_len == 0 || tag != NULL);

    if (tag_len == 0) {
        return (MBEDTLS_ERR_CCM_BAD_INPUT);
    }

    return (mbedtls_ccm_star_auth_decrypt(ctx, length, iv, iv_len, add,
                                          add_len, input, output, tag, tag_len));
}

#endif /*MBEDTLS_CCM_ALT*/
#endif /*MBEDTLS_CCM_C*/

