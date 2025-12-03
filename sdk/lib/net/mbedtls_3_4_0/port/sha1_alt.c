
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/mbedtls_config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "common.h"
#include "sha1_alt.h"

void jl_sha1_process(int *is_start, unsigned int state[5], const unsigned char data[64]);

int mbedtls_internal_sha1_process(mbedtls_sha1_context *ctx, const unsigned char data[64])
{
    int sha1_start = 1;
    jl_sha1_process(&sha1_start, ctx->state, data);
    return 0;
}


