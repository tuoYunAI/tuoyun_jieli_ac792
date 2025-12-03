#include "asm/aes_hw.h"
#include "aes_test.h"
#include "asm/cpu.h"
#include "os/os_api.h"

#define TIMEOUT_VALUE 0

static void aes_test_check(unsigned int *output, unsigned int *res, int len)
{
    if (memcmp(output, res, len) != 0) {
        while (1) {
            printf("aes_acc_test_fail!!!!!!");
        }
    }
}


static uint32_t Encryptedtext[256] = {0};
static uint32_t Decryptedtext[256] = {0};
CRYP_HandleTypeDef hcryp;
CRYP_ConfigTypeDef Conf;

void test_aes_mode(int verbose)
{
//ecb
//1
    /* Set the CRYP parameters */
    hcryp.Init.DataType   	= CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    	= CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       	= AESKey128;
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;
    /* hcryp.Init.Aes_Int_EN 	= 0; */

    hcryp.Init.Algorithm  = CRYP_AES_ECB;

    /* Configure the CRYP  */
    HAL_CRYP_Init(&hcryp);

    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, Ciphertext, 64);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

//2
    /* Get the CRYP parameters */
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    /* Set the CRYP parameters */
    Conf.DataType = CRYP_DATATYPE_8B;

    /* Configure the CRYP  */
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, Ciphertext_8, 64);
    HAL_CRYP_Decrypt(&hcryp, Ciphertext_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_8, 64);

//3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, Ciphertext_16, 64);
    HAL_CRYP_Decrypt(&hcryp, Ciphertext_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

//4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType = CRYP_DATATYPE_1B   ;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, Ciphertext_1, 64);
    HAL_CRYP_Decrypt(&hcryp, Ciphertext_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = AESKey192;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESECB192, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESECB192, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ecb_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ecb_Ciphertext_192_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ecb_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ecb_Ciphertext_192_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ecb_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ecb_Ciphertext_192_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = AESKey256;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESECB256, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESECB256, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ecb_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ecb_Ciphertext_256_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ecb_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ecb_Ciphertext_256_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ecb_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ecb_Ciphertext_256_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_ecb_test_Pass");
    }

//cbc
//128
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType 	= CRYP_DATATYPE_32B;
    Conf.KeySize  	= CRYP_KEYSIZE_128B;
    Conf.pKey     	= AESKey128;
    Conf.Algorithm 	= CRYP_AES_CBC;
    Conf.pInitVect	= AESIV_CBC;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCBC128, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCBC128, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_128_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_128_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_128_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize  	= CRYP_KEYSIZE_192B;
    Conf.pKey     	= AESKey192;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCBC192, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCBC192, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_192_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_192_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_192_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize  	= CRYP_KEYSIZE_256B;
    Conf.pKey     	= AESKey256;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCBC256, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCBC256, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_256_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_256_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cbc_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt(&hcryp, cbc_Ciphertext_256_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_cbc_test_Pass");
    }
//ctr
//128
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = aes_test_ctr_padding_iv;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = aes_test_ctr_padding_key;
    Conf.Algorithm = CRYP_AES_CTR;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(aes_test_ctr_padding_pt1, 0, 36);
    memcpy(aes_test_ctr_padding_pt1, aes_test_ctr_padding_pt, 36);
    HAL_CRYP_Encrypt(&hcryp, aes_test_ctr_padding_pt1, 9, aes_test_ctr_padding_pt1, 0);
    aes_test_check(aes_test_ctr_padding_pt1, aes_test_ctr_padding_ct, 36);
    HAL_CRYP_Decrypt(&hcryp, aes_test_ctr_padding_ct, 9, Decryptedtext, 0);
    aes_test_check(Decryptedtext, aes_test_ctr_padding_pt, 36);

    //
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = AESIV_CTR;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = AESKey128;
    Conf.Algorithm = CRYP_AES_CTR;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCTR128, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCTR128, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_128_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_128_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_128_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_192B;
    Conf.pKey      = AESKey192;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCTR192, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCTR192, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_192_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_192_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_192_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_256B;
    Conf.pKey      = AESKey256;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCTR256, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCTR256, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_256_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_256_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ctr_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ctr_Ciphertext_256_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


    if (verbose != 0) {
        printf("=======aes_acc_ctr_test_Pass");
    }

//cfb
//128
    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = AESIV_CFB;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = AESKey128;
    Conf.Algorithm = CRYP_AES_CFB;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCFB128, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCFB128, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_128_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_128_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_128_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_192B;
    Conf.pKey      = AESKey192;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCFB192, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCFB192, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_192_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_192_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_192_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_256B;
    Conf.pKey      = AESKey256;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESCFB256, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESCFB256, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_256_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_256_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, cfb_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt(&hcryp, cfb_Ciphertext_256_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_cfb_test_Pass");
    }

//ofb
    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = AESIV_OFB;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = AESKey128;
    Conf.Algorithm = CRYP_AES_OFB;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESOFB128, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESOFB128, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_128_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_128_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_128_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_192B;
    Conf.pKey      = AESKey192;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESOFB192, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESOFB192, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_192_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_192_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_192_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_256B;
    Conf.pKey      = AESKey256;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, CiphertextAESOFB256, 64);
    HAL_CRYP_Decrypt(&hcryp, CiphertextAESOFB256, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_8, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_256_8, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_16, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_256_16, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, Plaintext_1, 16, Encryptedtext, 0);
    aes_test_check(Encryptedtext, ofb_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt(&hcryp, ofb_Ciphertext_256_1, 16, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


    if (verbose != 0) {
        printf("=======aes_acc_ofb_test_Pass");
    }
}

void test_aes_mode_dma(int verbose)
{
//ecb
//1
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = AESKey128;
    /* hcryp.Init.Aes_Int_EN   = 0; */

    hcryp.Init.Algorithm  = CRYP_AES_ECB;
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;

    HAL_CRYP_Init(&hcryp);

    memset(Plaintext1, 0, 64);
    memcpy(Plaintext1, Plaintext, 64); //for same in/out loop_test
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext1, 16, Plaintext1);
    aes_test_check(Plaintext1, Ciphertext, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

//2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_8, 64);

//3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

//4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = AESKey192;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESECB192, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESECB192, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ecb_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ecb_Ciphertext_192_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ecb_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ecb_Ciphertext_192_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ecb_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ecb_Ciphertext_192_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = AESKey256;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESECB256, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESECB256, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ecb_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ecb_Ciphertext_256_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ecb_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ecb_Ciphertext_256_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ecb_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ecb_Ciphertext_256_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_ecb_test_Pass_dma");
    }
//cbc
//128
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType 	= CRYP_DATATYPE_32B;
    Conf.KeySize  	= CRYP_KEYSIZE_128B;
    Conf.pKey     	= AESKey128;
    Conf.Algorithm 	= CRYP_AES_CBC;
    Conf.pInitVect	= AESIV_CBC;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(Plaintext2, 0, 64);
    memcpy(Plaintext2, Plaintext, 64); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext2, 16, Plaintext2);
    aes_test_check(Plaintext2, CiphertextAESCBC128, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCBC128, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_128_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_128_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_128_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize  	= CRYP_KEYSIZE_192B;
    Conf.pKey     	= AESKey192;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESCBC192, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCBC192, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_192_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_192_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_192_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize  	= CRYP_KEYSIZE_256B;
    Conf.pKey     	= AESKey256;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESCBC256, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCBC256, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_256_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_256_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cbc_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cbc_Ciphertext_256_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_cbc_test_Pass_dma");
    }

//ctr
//128

    HAL_CRYP_GetConfig(&hcryp, &Conf);

    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = aes_test_ctr_padding_iv;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = aes_test_ctr_padding_key;
    Conf.Algorithm = CRYP_AES_CTR;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(aes_test_ctr_padding_pt1, 0, 36);
    memcpy(aes_test_ctr_padding_pt1, aes_test_ctr_padding_pt, 36);
    HAL_CRYP_Encrypt_DMA(&hcryp, aes_test_ctr_padding_pt1, 9, aes_test_ctr_padding_pt1);
    aes_test_check(aes_test_ctr_padding_pt1, aes_test_ctr_padding_ct, 36);
    HAL_CRYP_Decrypt_DMA(&hcryp, aes_test_ctr_padding_ct, 9, Decryptedtext);
    aes_test_check(Decryptedtext, aes_test_ctr_padding_pt, 36);


    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = AESIV_CTR;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = AESKey128;
    Conf.Algorithm = CRYP_AES_CTR;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(Plaintext3, 0, 64);
    memcpy(Plaintext3, Plaintext, 64); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext3, 16, Plaintext3);
    aes_test_check(Plaintext3, CiphertextAESCTR128, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCTR128, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_128_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_128_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_128_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_192B;
    Conf.pKey      = AESKey192;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESCTR192, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCTR192, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_192_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_192_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_192_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_256B;
    Conf.pKey      = AESKey256;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESCTR256, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCTR256, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_256_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_256_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ctr_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ctr_Ciphertext_256_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_ctr_test_Pass_dma");
    }

//cfb
//128
    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = AESIV_CFB;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = AESKey128;
    Conf.Algorithm = CRYP_AES_CFB;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(Plaintext4, 0, 64);
    memcpy(Plaintext4, Plaintext, 64); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext4, 16, Plaintext4);
    aes_test_check(Plaintext4, CiphertextAESCFB128, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCFB128, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_128_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_128_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_128_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);



//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_192B;
    Conf.pKey      = AESKey192;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESCFB192, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCFB192, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_192_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_192_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_192_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_256B;
    Conf.pKey      = AESKey256;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESCFB256, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESCFB256, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_256_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_256_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, cfb_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, cfb_Ciphertext_256_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);

    if (verbose != 0) {
        printf("=======aes_acc_cfb_test_Pass_dma");
    }

//ofb
    Conf.DataType = CRYP_DATATYPE_32B;
    Conf.pInitVect = AESIV_OFB;
    Conf.KeySize   = CRYP_KEYSIZE_128B;
    Conf.pKey      = AESKey128;
    Conf.Algorithm = CRYP_AES_OFB;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(Plaintext5, 0, 64);
    memcpy(Plaintext5, Plaintext, 64); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext5, 16, Plaintext5);
    aes_test_check(Plaintext5, CiphertextAESOFB128, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESOFB128, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_128_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_128_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_128_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_128_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_128_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_128_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_192B;
    Conf.pKey      = AESKey192;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESOFB192, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESOFB192, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_192_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_192_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_192_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_192_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_192_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_192_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


//256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize   = CRYP_KEYSIZE_256B;
    Conf.pKey      = AESKey256;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext, 16, Encryptedtext);
    aes_test_check(Encryptedtext, CiphertextAESOFB256, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, CiphertextAESOFB256, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext, 64);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_8, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_256_8, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_256_8, 16, Decryptedtext);
    aes_test_check(Decryptedtext,  Plaintext_8, 64);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_16, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_256_16, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_256_16, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_16, 64);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_1, 16, Encryptedtext);
    aes_test_check(Encryptedtext, ofb_Ciphertext_256_1, 64);
    HAL_CRYP_Decrypt_DMA(&hcryp, ofb_Ciphertext_256_1, 16, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_1, 64);


    if (verbose != 0) {
        printf("=======aes_acc_ofb_test_Pass_dma");
    }
}

static void test_aes_gcm(int verbose)
{
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = AES128Key;
    hcryp.Init.Algorithm  = CRYP_AES_GCM;
    hcryp.Init.pInitVect  = InitVector;
    hcryp.Init.Header     = HeaderMessage;
    hcryp.Init.HeaderSize = 5;
    /* hcryp.Init.Aes_Int_EN = 0; */
    hcryp.Init.Gcm_Iv_Not96_En = 0;
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;
    HAL_CRYP_Init(&hcryp);

    memset(Plaintext_gcm1, 0, 60);
    memcpy(Plaintext_gcm1, Plaintext_gcm, 60); //for same in/out loop_test

    HAL_CRYP_Encrypt(&hcryp, Plaintext_gcm1, 15, Plaintext_gcm1, 0);
    aes_test_check(Plaintext_gcm1, Ciphertext_gcm, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, 0);
    aes_test_check(GCM_ENC_TAG, ExpectedTAG, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_gcm, 15, Decryptedtext, 0);
    aes_test_check(Decryptedtext, Plaintext_gcm, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, ExpectedTAG, 16);

//vct1
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.Gcm_Iv_Not96_En = 0;
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct1_key;
    Conf.pInitVect  = gcm_aes_vct1_iv;
    Conf.Header     = gcm_aes_vct1_aad;
    Conf.HeaderSize = 0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct1_t, 0, Encryptedtext, TIMEOUT_VALUE);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct1_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct1_ct, 0, Decryptedtext, TIMEOUT_VALUE);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct1_ctag, 16);

//vct2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct2_key;
    Conf.pInitVect  = gcm_aes_vct2_iv;
    Conf.Header     = gcm_aes_vct2_aad;
    Conf.HeaderSize = 0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct2_t, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct2_ct, 16);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct2_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct2_ct, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct2_t, 16);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct2_ctag, 16);

//vct3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct3_key;
    Conf.pInitVect  = gcm_aes_vct3_iv;
    Conf.Header     = gcm_aes_vct3_aad;
    Conf.HeaderSize = 0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct3_t, 16, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct3_ct, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct3_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct3_ct, 16, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct3_t, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct3_ctag, 16);

//vct4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct4_key;
    Conf.pInitVect  = gcm_aes_vct4_iv;
    Conf.Header     = gcm_aes_vct4_aad;
    Conf.HeaderSize = 5;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct4_t, 15, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct4_ct, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct4_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct3_ct, 15, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct4_t, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct4_ctag, 16);

//vct7
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct7_key;
    Conf.pInitVect  = gcm_aes_vct7_iv;
    Conf.Header     = gcm_aes_vct7_aad;
    Conf.HeaderSize = 4;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct7_t, 16, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct7_ct, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct7_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct7_ct, 16, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct7_t, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct7_ctag, 16);

//vct8
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct8_key;
    Conf.pInitVect  = gcm_aes_vct8_iv;
    Conf.Header     = gcm_aes_vct8_aad;
    Conf.HeaderSize = 8;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct8_t, 16, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct8_ct, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct8_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct8_ct, 16, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct8_t, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct8_ctag, 16);

//vct9
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct9_key;
    Conf.pInitVect  = gcm_aes_vct9_iv;
    Conf.Header     = gcm_aes_vct9_aad;
    Conf.HeaderSize = 8;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct9_t, 14, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct9_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct9_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct9_ct, 14, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct9_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct9_ctag, 16);

//vct10
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct10_key;
    Conf.pInitVect  = gcm_aes_vct10_iv;
    Conf.Header     = gcm_aes_vct10_aad;
    Conf.HeaderSize = 7;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t, 14, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct, 14, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_8, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_8, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_8, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_16, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_16, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_16, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_16, 16);


    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_1, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_1, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_1, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_1, 16);



//vct10_192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = gcm_aes_vct10_192_key;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t, 14, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_192, 14, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_8, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192_8, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_192_8, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192_8, 16);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_16, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192_16, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_192_16, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_1, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192_1, 16);


    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_192_1, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192_1, 16);

//vct10_256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = gcm_aes_vct10_256_key;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t, 14, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_256, 14, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_8, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256_8, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_256_8, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256_8, 16);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_16, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256_16, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_256_16, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct10_t_1, 14, Encryptedtext, 0);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256_1, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct10_ct_256_1, 14, Decryptedtext, 0);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256_1, 16);



//vct11
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = gcm_aes_vct11_key;
    Conf.pInitVect  = gcm_aes_vct11_iv;
    Conf.Header     = gcm_aes_vct11_aad;
    Conf.HeaderSize = 7;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct11_t, 14, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct11_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct11_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct11_ct, 14, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct11_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct11_ctag, 16);

//vct12
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = gcm_aes_vct12_key;
    Conf.pInitVect  = gcm_aes_vct12_iv;
    Conf.Header     = gcm_aes_vct12_aad;
    Conf.HeaderSize = 7;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct12_t, 14, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct12_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct12_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct12_ct, 14, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct12_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct12_ctag, 16);

//vct5
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct5_key;
    Conf.pInitVect  = gcm_aes_vct5_iv;
    Conf.Header     = gcm_aes_vct5_aad;
    Conf.HeaderSize = 5;
    Conf.Gcm_Iv_Not96_En = 1;
    Conf.Gcm_Ivlen = 32;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct5_t, 15, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct5_ct, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct5_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct5_ct, 15, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct5_t, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct5_ctag, 16);

//vct6
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct6_key;
    Conf.pInitVect  = gcm_aes_vct6_iv;
    Conf.Header     = gcm_aes_vct6_aad;
    Conf.HeaderSize = 5;
    Conf.Gcm_Iv_Not96_En = 1;
    Conf.Gcm_Ivlen = 80;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, gcm_aes_vct6_t, 15, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, gcm_aes_vct6_ct, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct6_ctag, 16);

    HAL_CRYP_Decrypt(&hcryp, gcm_aes_vct6_ct, 15, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, gcm_aes_vct6_t, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct6_ctag, 16);

    if (verbose != 0) {
        printf("=======aes_acc_gcm_test_Pass");
    }
}


static void test_aes_gcm_dma(int verbose)
{
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = AES128Key;
    hcryp.Init.Algorithm  = CRYP_AES_GCM;
    hcryp.Init.pInitVect  = InitVector;
    hcryp.Init.Header     = HeaderMessage;
    hcryp.Init.HeaderSize = 5;
    /* hcryp.Init.Aes_Int_EN = 0; */
    hcryp.Init.Gcm_Iv_Not96_En = 0;
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;
    HAL_CRYP_Init(&hcryp);

    memset(Plaintext_gcm1, 0, 60);
    memcpy(Plaintext_gcm1, Plaintext_gcm, 60); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_gcm1, 15, Plaintext_gcm1);
    aes_test_check(Plaintext_gcm1, Ciphertext_gcm, 60);

    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, ExpectedTAG, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_gcm, 15, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_gcm, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, ExpectedTAG, 16);

//vct1
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct1_key;
    Conf.pInitVect  = gcm_aes_vct1_iv;
    Conf.Header     = gcm_aes_vct1_aad;
    Conf.HeaderSize = 0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct1_t, 0, Encryptedtext);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct1_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct1_ct, 0, Decryptedtext);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct1_ctag, 16);

//vct2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct2_key;
    Conf.pInitVect  = gcm_aes_vct2_iv;
    Conf.Header     = gcm_aes_vct2_aad;
    Conf.HeaderSize = 0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct2_t, 4, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct2_ct, 16);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct2_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct2_ct, 4, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct2_t, 16);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct2_ctag, 16);

//vct3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct3_key;
    Conf.pInitVect  = gcm_aes_vct3_iv;
    Conf.Header     = gcm_aes_vct3_aad;
    Conf.HeaderSize = 0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct3_t, 16, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct3_ct, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct3_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct3_ct, 16, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct3_t, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct3_ctag, 16);

//vct4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct4_key;
    Conf.pInitVect  = gcm_aes_vct4_iv;
    Conf.Header     = gcm_aes_vct4_aad;
    Conf.HeaderSize = 5;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct4_t, 15, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct4_ct, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct4_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct3_ct, 15, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct4_t, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct4_ctag, 16);

//vct7
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct7_key;
    Conf.pInitVect  = gcm_aes_vct7_iv;
    Conf.Header     = gcm_aes_vct7_aad;
    Conf.HeaderSize = 4;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct7_t, 16, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct7_ct, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct7_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct7_ct, 16, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct7_t, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct7_ctag, 16);

//vct8
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct8_key;
    Conf.pInitVect  = gcm_aes_vct8_iv;
    Conf.Header     = gcm_aes_vct8_aad;
    Conf.HeaderSize = 8;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct8_t, 16, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct8_ct, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct8_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct8_ct, 16, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct8_t, 64);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct8_ctag, 16);

//vct9
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct9_key;
    Conf.pInitVect  = gcm_aes_vct9_iv;
    Conf.Header     = gcm_aes_vct9_aad;
    Conf.HeaderSize = 8;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct9_t, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct9_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct9_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct9_ct, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct9_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct9_ctag, 16);

//vct10
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct10_key;
    Conf.pInitVect  = gcm_aes_vct10_iv;
    Conf.Header     = gcm_aes_vct10_aad;
    Conf.HeaderSize = 7;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_8, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_8, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_8, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_16, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_16, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_16, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_16, 16);


    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_1, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_1, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_1, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_1, 16);



//vct10_192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = gcm_aes_vct10_192_key;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_192, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_8, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192_8, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_192_8, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192_8, 16);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_16, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192_16, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_192_16, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_1, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_192_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_192_1, 16);


    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_192_1, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_192_1, 16);

//vct10_256
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = gcm_aes_vct10_256_key;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_256, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_8, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256_8, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_256_8, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_8, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256_8, 16);


    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_16, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256_16, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_256_16, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_16, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;

    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct10_t_1, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct10_ct_256_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct10_ctag_256_1, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct10_ct_256_1, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct10_t_1, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct10_ctag_256_1, 16);


//vct11
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = gcm_aes_vct11_key;
    Conf.pInitVect  = gcm_aes_vct11_iv;
    Conf.Header     = gcm_aes_vct11_aad;
    Conf.HeaderSize = 7;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct11_t, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct11_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct11_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct11_ct, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct11_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct11_ctag, 16);

//vct12
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = gcm_aes_vct12_key;
    Conf.pInitVect  = gcm_aes_vct12_iv;
    Conf.Header     = gcm_aes_vct12_aad;
    Conf.HeaderSize = 7;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct12_t, 14, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct12_ct, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct12_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct12_ct, 14, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct12_t, 56);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct12_ctag, 16);

//vct5
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct5_key;
    Conf.pInitVect  = gcm_aes_vct5_iv;
    Conf.Header     = gcm_aes_vct5_aad;
    Conf.HeaderSize = 5;
    Conf.Gcm_Iv_Not96_En = 1;
    Conf.Gcm_Ivlen = 32;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct5_t, 15, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct5_ct, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct5_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct5_ct, 15, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct5_t, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct5_ctag, 16);

//vct6
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = gcm_aes_vct6_key;
    Conf.pInitVect  = gcm_aes_vct6_iv;
    Conf.Header     = gcm_aes_vct6_aad;
    Conf.HeaderSize = 5;
    Conf.Gcm_Iv_Not96_En = 1;
    Conf.Gcm_Ivlen = 80;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, gcm_aes_vct6_t, 15, Encryptedtext);
    aes_test_check(Encryptedtext, gcm_aes_vct6_ct, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(GCM_ENC_TAG, gcm_aes_vct6_ctag, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, gcm_aes_vct6_ct, 15, Decryptedtext);
    aes_test_check(Decryptedtext, gcm_aes_vct6_t, 60);
    HAL_CRYPEx_AESGCM_GenerateAuthTAG(&hcryp, GCM_DEC_TAG, 0);
    aes_test_check(GCM_DEC_TAG, gcm_aes_vct6_ctag, 16);

    if (verbose != 0) {
        printf("=======aes_acc_gcm_test_Pass_dma");
    }

}


static void test_aes_ccm_dma(int verbose)
{
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = AESCCM128Key;
    hcryp.Init.Algorithm  = CRYP_AES_CCM;
    hcryp.Init.Header     = BlockB1;
    hcryp.Init.HeaderSize = 4;
    hcryp.Init.B0         = BlockB0;
    /* hcryp.Init.Aes_Int_EN = 0; */
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;

    HAL_CRYP_Init(&hcryp);

    memset(Plaintext_ccm1, 0, 16);
    memcpy(Plaintext_ccm1, Plaintext_ccm, 16); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm1, 4, Plaintext_ccm1);
    aes_test_check(Plaintext_ccm1, Ciphertext_ccm_128, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_128, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_8, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128_8, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_128_8, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_16, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128_16, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_128_16, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_1, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128_1, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_128_1, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128_1, 16);

//192key
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = AESCCM192Key;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_192, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_8, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192_8, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_192_8, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_16, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192_16, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_192_16, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_1, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192_1, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_192_1, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192_1, 16);

//256key
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = AESCCM256Key;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_256, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_8, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256_8, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_256_8, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_16, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256_16, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_256_16, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, Plaintext_ccm_1, 4, Encryptedtext);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256_1, 16);

    HAL_CRYP_Decrypt_DMA(&hcryp, Ciphertext_ccm_256_1, 4, Decryptedtext);
    aes_test_check(Decryptedtext, Plaintext_ccm_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256_1, 16);

//ex1
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_ETEST_KEY128;
    Conf.Header     = CCM_ETEST1_B1;
    Conf.HeaderSize = 3;
    Conf.B0   = CCM_ETEST1_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(CCM_ETEST1_PT1, 0, 4);
    memcpy(CCM_ETEST1_PT1, CCM_ETEST1_PT, 4);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_ETEST1_PT1, 1, CCM_ETEST1_PT1);
    aes_test_check(CCM_ETEST1_PT1, CCM_ETEST1_CT, 4);

    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_ETEST1_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_ETEST1_CT, 1, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_ETEST1_PT, 4);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_ETEST1_TAG, 4);

//ex2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_ETEST_KEY128;
    Conf.Header     = CCM_ETEST2_B1;
    Conf.HeaderSize = 5;
    Conf.B0   = CCM_ETEST2_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_ETEST2_PT, 4, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_ETEST2_CT, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    /* aes_test_check(CCM_ENC_TAG, CCM_ETEST2_TAG, 8); */

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_ETEST2_CT, 4, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_ETEST2_PT, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    /* aes_test_check(CCM_DEC_TAG, CCM_ETEST2_TAG, 8);  */

//ex3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_ETEST_KEY128;
    Conf.Header     = CCM_ETEST3_B1;
    Conf.HeaderSize = 6;
    Conf.B0   = CCM_ETEST3_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_ETEST3_PT, 6, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_ETEST3_CT, 24);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_ETEST3_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_ETEST3_CT, 6, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_ETEST3_PT, 24);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_ETEST3_TAG, 4);

//smp1
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP1_KEY128;
    Conf.Header     = CCM_SMP1_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP1_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP1_PT, 0, Encryptedtext);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP1_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP1_CT, 0, Decryptedtext);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP1_TAG, 4);

//smp2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP2_KEY128;
    Conf.Header     = CCM_SMP2_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP2_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP2_PT, 2, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP2_CT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP2_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP2_CT, 2, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP2_PT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP2_TAG, 4);

//smp3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP3_KEY128;
    Conf.Header     = CCM_SMP3_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP3_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP3_PT, 2, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP3_CT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP3_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP3_CT, 2, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP3_PT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP3_TAG, 4);

//smp4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP4_KEY128;
    Conf.Header     = CCM_SMP4_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP4_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP4_PT, 5, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP4_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP4_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP4_CT, 5, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP4_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP4_TAG, 4);

//smp5
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP5_KEY128;
    Conf.Header     = CCM_SMP5_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP5_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP5_PT, 5, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP5_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP5_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP5_CT, 5, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP5_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP5_TAG, 4);

//smp6
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP6_KEY128;
    Conf.Header     = CCM_SMP6_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP6_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP6_PT, 5, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP6_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP6_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP6_CT, 5, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP6_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP6_TAG, 4);

//smp7
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP7_KEY128;
    Conf.Header     = CCM_SMP7_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP7_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP7_PT, 5, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP7_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP7_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP7_CT, 5, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP7_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP7_TAG, 4);

//smp8
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP8_KEY128;
    Conf.Header     = CCM_SMP8_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP8_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP8_PT, 7, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP8_CT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP8_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP8_CT, 7, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP8_PT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP8_TAG, 4);

//smp9
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP9_KEY128;
    Conf.Header     = CCM_SMP9_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP9_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP9_PT, 7, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP9_CT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP9_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP9_CT, 7, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP9_PT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP9_TAG, 4);

//smp10
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP10_KEY128;
    Conf.Header     = CCM_SMP10_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP10_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP10_PT, 92, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP10_CT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP10_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP10_CT, 92, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP10_PT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP10_TAG, 4);

//smp11
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP11_KEY128;
    Conf.Header     = CCM_SMP11_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP11_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP11_PT, 92, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP11_CT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP11_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP11_CT, 92, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP11_PT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP11_TAG, 4);

//smp12
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP12_KEY128;
    Conf.Header     = CCM_SMP12_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP12_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP12_PT, 256, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP12_CT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP12_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP12_CT, 256, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP12_PT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP12_TAG, 4);

//smp13
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP13_KEY128;
    Conf.Header     = CCM_SMP13_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP13_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, CCM_SMP13_PT, 256, Encryptedtext);
    aes_test_check(Encryptedtext, CCM_SMP13_CT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP13_TAG, 4);

    HAL_CRYP_Decrypt_DMA(&hcryp, CCM_SMP13_CT, 256, Decryptedtext);
    aes_test_check(Decryptedtext, CCM_SMP13_PT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP13_TAG, 4);

    if (verbose != 0) {
        printf("=======aes_acc_ccm_test_Pass_dma");
    }
}

static void test_aes_ccm(int verbose)
{
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = AESCCM128Key;
    hcryp.Init.Algorithm  = CRYP_AES_CCM;
    hcryp.Init.Header     = BlockB1;
    hcryp.Init.HeaderSize = 4;
    hcryp.Init.B0         = BlockB0;
    /* hcryp.Init.Aes_Int_EN = 0; */
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;

    HAL_CRYP_Init(&hcryp);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_128, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_8, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128_8, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_128_8, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_16, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128_16, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_128_16, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_1, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_128_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_128_1, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_128_1, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_128_1, 16);

//192key
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = AESCCM192Key;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_192, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_8, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192_8, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_192_8, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_16, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192_16, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_192_16, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_1, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_192_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_192_1, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_192_1, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_192_1, 16);


//256key
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = AESCCM256Key;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_256, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256, 16);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_8, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256_8, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_256_8, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_8, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256_8, 16);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_16, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256_16, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_256_16, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_16, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256_16, 16);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, Plaintext_ccm_1, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, Ciphertext_ccm_256_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, ExpectedTAG_ccm_256_1, 16);

    HAL_CRYP_Decrypt(&hcryp, Ciphertext_ccm_256_1, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, Plaintext_ccm_1, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, ExpectedTAG_ccm_256_1, 16);

//ex1
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_ETEST_KEY128;
    Conf.Header     = CCM_ETEST1_B1;
    Conf.HeaderSize = 3;
    Conf.B0   = CCM_ETEST1_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    memset(CCM_ETEST1_PT1, 0, 4);
    memcpy(CCM_ETEST1_PT1, CCM_ETEST1_PT, 4);

    HAL_CRYP_Encrypt(&hcryp, CCM_ETEST1_PT1, 1, CCM_ETEST1_PT1, 0);
    aes_test_check(CCM_ETEST1_PT1, CCM_ETEST1_CT, 4);

    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_ETEST1_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_ETEST1_CT, 1, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_ETEST1_PT, 4);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_ETEST1_TAG, 4);

    //ex2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_ETEST_KEY128;
    Conf.Header     = CCM_ETEST2_B1;
    Conf.HeaderSize = 5;
    Conf.B0   = CCM_ETEST2_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_ETEST2_PT, 4, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_ETEST2_CT, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    /* aes_test_check(CCM_ENC_TAG, CCM_ETEST2_TAG, 8); */

    HAL_CRYP_Decrypt(&hcryp, CCM_ETEST2_CT, 4, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_ETEST2_PT, 16);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    /* aes_test_check(CCM_DEC_TAG, CCM_ETEST2_TAG, 8);  */

    //ex3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_ETEST_KEY128;
    Conf.Header     = CCM_ETEST3_B1;
    Conf.HeaderSize = 6;
    Conf.B0   = CCM_ETEST3_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_ETEST3_PT, 6, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_ETEST3_CT, 24);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_ETEST3_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_ETEST3_CT, 6, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_ETEST3_PT, 24);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_ETEST3_TAG, 4);

    //smp1
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP1_KEY128;
    Conf.Header     = CCM_SMP1_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP1_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP1_PT, 0, Encryptedtext, TIMEOUT_VALUE);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP1_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP1_CT, 0, Decryptedtext, TIMEOUT_VALUE);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP1_TAG, 4);

    //smp2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP2_KEY128;
    Conf.Header     = CCM_SMP2_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP2_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP2_PT, 2, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP2_CT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP2_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP2_CT, 2, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP2_PT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP2_TAG, 4);

    //smp3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP3_KEY128;
    Conf.Header     = CCM_SMP3_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP3_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP3_PT, 2, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP3_CT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP3_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP3_CT, 2, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP3_PT, 8);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP3_TAG, 4);

    //smp4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP4_KEY128;
    Conf.Header     = CCM_SMP4_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP4_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP4_PT, 5, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP4_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP4_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP4_CT, 5, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP4_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP4_TAG, 4);

    //smp5
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP5_KEY128;
    Conf.Header     = CCM_SMP5_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP5_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP5_PT, 5, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP5_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP5_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP5_CT, 5, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP5_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP5_TAG, 4);

    //smp6
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP6_KEY128;
    Conf.Header     = CCM_SMP6_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP6_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP6_PT, 5, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP6_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP6_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP6_CT, 5, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP6_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP6_TAG, 4);

    //smp7
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP7_KEY128;
    Conf.Header     = CCM_SMP7_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP7_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP7_PT, 5, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP7_CT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP7_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP7_CT, 5, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP7_PT, 20);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP7_TAG, 4);

    //smp8
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP8_KEY128;
    Conf.Header     = CCM_SMP8_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP8_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP8_PT, 7, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP8_CT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP8_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP8_CT, 7, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP8_PT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP8_TAG, 4);

    //smp9
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP9_KEY128;
    Conf.Header     = CCM_SMP9_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP9_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP9_PT, 7, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP9_CT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP9_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP9_CT, 7, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP9_PT, 28);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP9_TAG, 4);

    //smp10
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP10_KEY128;
    Conf.Header     = CCM_SMP10_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP10_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP10_PT, 92, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP10_CT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP10_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP10_CT, 92, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP10_PT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP10_TAG, 4);

    //smp11
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP11_KEY128;
    Conf.Header     = CCM_SMP11_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP11_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP11_PT, 92, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP11_CT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP11_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP11_CT, 92, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP11_PT, 368);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP11_TAG, 4);

    //smp12
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP12_KEY128;
    Conf.Header     = CCM_SMP12_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP12_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP12_PT, 256, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP12_CT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP12_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP12_CT, 256, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP12_PT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP12_TAG, 4);

    //smp13
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = CCM_SMP13_KEY128;
    Conf.Header     = CCM_SMP13_B1;
    Conf.HeaderSize = 4;
    Conf.B0   = CCM_SMP13_B0;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, CCM_SMP13_PT, 256, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, CCM_SMP13_CT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_ENC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_ENC_TAG, CCM_SMP13_TAG, 4);

    HAL_CRYP_Decrypt(&hcryp, CCM_SMP13_CT, 256, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, CCM_SMP13_PT, 1024);
    HAL_CRYPEx_AESCCM_GenerateAuthTAG(&hcryp, CCM_DEC_TAG, TIMEOUT_VALUE);
    aes_test_check(CCM_DEC_TAG, CCM_SMP13_TAG, 4);


    if (verbose != 0) {
        printf("=======aes_acc_ccm_test_Pass");
    }


}

static void test_aes_xts(int verbose)
{
//vct1_128
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = xts_vct1_key1;
    hcryp.Init.Algorithm  = CRYP_AES_XTS;
    hcryp.Init.pKey2 	  = xts_vct1_key2;
    hcryp.Init.Tweak  	  = xts_vct1_twk;

    hcryp.Init.Xts_Ct_Steal_En = 0;
    /* hcryp.Init.Aes_Int_EN = 0; */
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;

    HAL_CRYP_Init(&hcryp);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

//vct1_128_xts_bp_en
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.Xts_Bp_En = 1;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_xts_bp, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_xts_bp, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.Xts_Bp_En = 0;
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_128_8, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_128_8, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_128_16, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_128_16, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_128_1, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_128_1, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

//vct1_192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = xts_vct1_key1;
    Conf.pKey2       = xts_vct1_key2;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_192, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_192, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_192_8, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_192_8, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_192_16, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_192_16, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_192_1, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_192_1, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);



//vct1_256key
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct1_256_key1;
    Conf.pKey2      = xts_vct1_256_key2;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_256, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_256, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_256_8, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_256_8, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_256_16, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_256_16, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct1_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct1_ct_256_1, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct1_ct_256_1, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

//vct2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct2_key1;
    Conf.pKey2      = xts_vct2_key2;
    Conf.Tweak  	= xts_vct2_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct2_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct2_ct, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct2_ct, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct2_pt, 32);

//vct3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct3_key1;
    Conf.pKey2      = xts_vct3_key2;
    Conf.Tweak  	= xts_vct3_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct3_pt, 8, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct3_ct, 32);

    HAL_CRYP_Decrypt(&hcryp, xts_vct3_ct, 8, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct3_pt, 32);

//vct4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct4_key1;
    Conf.pKey2      = xts_vct4_key2;
    Conf.Tweak  	= xts_vct4_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct4_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct4_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct4_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct4_pt, 512);

//vct5
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct5_key1;
    Conf.pKey2      = xts_vct5_key2;
    Conf.Tweak  	= xts_vct5_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct5_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct5_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct5_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct5_pt, 512);

//vct6
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct6_key1;
    Conf.pKey2      = xts_vct6_key2;
    Conf.Tweak  	= xts_vct6_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct6_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct6_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct6_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct6_pt, 512);

//vct7
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct7_key1;
    Conf.pKey2      = xts_vct7_key2;
    Conf.Tweak  	= xts_vct7_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct7_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct7_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct7_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct7_pt, 512);

//vct8
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct8_key1;
    Conf.pKey2      = xts_vct8_key2;
    Conf.Tweak  	= xts_vct8_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct8_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct8_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct8_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct8_pt, 512);

//vct9
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct9_key1;
    Conf.pKey2      = xts_vct9_key2;
    Conf.Tweak  	= xts_vct9_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct9_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct9_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct9_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct9_pt, 512);

//vct10
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct10_key1;
    Conf.pKey2      = xts_vct10_key2;
    Conf.Tweak  	= xts_vct10_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct10_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct10_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct10_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct10_pt, 512);

//vct11
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct11_key1;
    Conf.pKey2      = xts_vct11_key2;
    Conf.Tweak  	= xts_vct11_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct11_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct11_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct11_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct11_pt, 512);

//vct12
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct12_key1;
    Conf.pKey2      = xts_vct12_key2;
    Conf.Tweak  	= xts_vct12_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct12_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct12_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct12_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct12_pt, 512);

//vct13
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct13_key1;
    Conf.pKey2      = xts_vct13_key2;
    Conf.Tweak  	= xts_vct13_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct13_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct13_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct13_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct13_pt, 512);

//vct14
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct14_key1;
    Conf.pKey2      = xts_vct14_key2;
    Conf.Tweak  	= xts_vct14_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct14_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct14_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct14_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct14_pt, 512);

//vct19
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct19_key1;
    Conf.pKey2      = xts_vct19_key2;
    Conf.Tweak  	= xts_vct19_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct19_pt, 128, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct19_ct, 512);

    HAL_CRYP_Decrypt(&hcryp, xts_vct19_ct, 128, Decryptedtext, TIMEOUT_VALUE);
    aes_test_check(Decryptedtext, xts_vct19_pt, 512);

    if (verbose != 0) {
        printf("=======aes_acc_xts_test_Pass");
    }
}

static void test_aes_xts_dma(int verbose)
{
//vct1_128
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = xts_vct1_key1;
    hcryp.Init.Algorithm  = CRYP_AES_XTS;
    hcryp.Init.pKey2 	  = xts_vct1_key2;
    hcryp.Init.Tweak  	  = xts_vct1_twk;

    /* hcryp.Init.Aes_Int_EN = 0; */

    HAL_CRYP_Init(&hcryp);

    memset(xts_vct1_pt1, 0, 32);
    memcpy(xts_vct1_pt1, xts_vct1_pt, 32); //for same in/out loop_test

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt1, 8, xts_vct1_pt1);
    aes_test_check(xts_vct1_pt1, xts_vct1_ct, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_128_8, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_128_8, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_128_16, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_128_16, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_128_1, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_128_1, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

//vct1_192
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_192B;
    Conf.pKey       = xts_vct1_key1;
    Conf.pKey2       = xts_vct1_key2;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_192, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_192, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_192_8, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_192_8, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_192_16, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_192_16, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_192_1, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_192_1, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);


//vct1_256key
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct1_256_key1;
    Conf.pKey2      = xts_vct1_256_key2;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_256, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_256, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //byte_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_256_8, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_256_8, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //half_word_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_256_16, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_256_16, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

    //bit_swap
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_1B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct1_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct1_ct_256_1, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct1_ct_256_1, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct1_pt, 32);

//vct2
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.DataType   = CRYP_DATATYPE_32B;
    Conf.pKey       = xts_vct2_key1;
    Conf.pKey2      = xts_vct2_key2;
    Conf.Tweak  	= xts_vct2_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct2_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct2_ct, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct2_ct, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct2_pt, 32);

//vct3
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct3_key1;
    Conf.pKey2      = xts_vct3_key2;
    Conf.Tweak  	= xts_vct3_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct3_pt, 8, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct3_ct, 32);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct3_ct, 8, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct3_pt, 32);

//vct4
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct4_key1;
    Conf.pKey2      = xts_vct4_key2;
    Conf.Tweak  	= xts_vct4_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct4_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct4_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct4_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct4_pt, 512);

//vct5
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct5_key1;
    Conf.pKey2      = xts_vct5_key2;
    Conf.Tweak  	= xts_vct5_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct5_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct5_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct5_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct5_pt, 512);

//vct6
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct6_key1;
    Conf.pKey2      = xts_vct6_key2;
    Conf.Tweak  	= xts_vct6_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct6_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct6_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct6_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct6_pt, 512);

//vct7
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct7_key1;
    Conf.pKey2      = xts_vct7_key2;
    Conf.Tweak  	= xts_vct7_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct7_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct7_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct7_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct7_pt, 512);

//vct8
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct8_key1;
    Conf.pKey2      = xts_vct8_key2;
    Conf.Tweak  	= xts_vct8_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct8_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct8_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct8_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct8_pt, 512);

//vct9
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct9_key1;
    Conf.pKey2      = xts_vct9_key2;
    Conf.Tweak  	= xts_vct9_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct9_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct9_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct9_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct9_pt, 512);

//vct10
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct10_key1;
    Conf.pKey2      = xts_vct10_key2;
    Conf.Tweak  	= xts_vct10_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct10_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct10_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct10_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct10_pt, 512);

//vct11
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct11_key1;
    Conf.pKey2      = xts_vct11_key2;
    Conf.Tweak  	= xts_vct11_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct11_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct11_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct11_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct11_pt, 512);

//vct12
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct12_key1;
    Conf.pKey2      = xts_vct12_key2;
    Conf.Tweak  	= xts_vct12_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct12_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct12_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct12_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct12_pt, 512);

//vct13
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct13_key1;
    Conf.pKey2      = xts_vct13_key2;
    Conf.Tweak  	= xts_vct13_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct13_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct13_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct13_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct13_pt, 512);

//vct14
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_256B;
    Conf.pKey       = xts_vct14_key1;
    Conf.pKey2      = xts_vct14_key2;
    Conf.Tweak  	= xts_vct14_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct14_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct14_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct14_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct14_pt, 512);

//vct19
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.KeySize    = CRYP_KEYSIZE_128B;
    Conf.pKey       = xts_vct19_key1;
    Conf.pKey2      = xts_vct19_key2;
    Conf.Tweak  	= xts_vct19_twk;

    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt_DMA(&hcryp, xts_vct19_pt, 128, Encryptedtext);
    aes_test_check(Encryptedtext, xts_vct19_ct, 512);

    HAL_CRYP_Decrypt_DMA(&hcryp, xts_vct19_ct, 128, Decryptedtext);
    aes_test_check(Decryptedtext, xts_vct19_pt, 512);

    if (verbose != 0) {
        printf("=======aes_acc_xts_test_Pass_dma");
    }
}


static void test_aes_xts_ct_steal(int verbose)
{

//vct15
    hcryp.Init.DataType   = CRYP_DATATYPE_32B;
    hcryp.Init.KeySize    = CRYP_KEYSIZE_128B;
    hcryp.Init.pKey       = xts_vct15_key1;
    hcryp.Init.Algorithm  = CRYP_AES_XTS;
    hcryp.Init.pKey2 	  = xts_vct15_key2;
    hcryp.Init.Tweak  	  = xts_vct15_twk;
    hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;

    hcryp.Init.Xts_Ct_Steal_En = 1;
    /* hcryp.Init.Aes_Int_EN = 0; */

    HAL_CRYP_Init(&hcryp);

    /* HAL_CRYP_Encrypt(&hcryp, xts_vct15_pt, 17, Encryptedtext, TIMEOUT_VALUE); */
    /* aes_test_check(Encryptedtext, xts_vct15_ct, 16); */

    memset(xts_vct15_pt1, 0, 20);
    memcpy(xts_vct15_pt1, xts_vct15_pt, 20);

    HAL_CRYP_Encrypt(&hcryp, xts_vct15_pt1, 17, xts_vct15_pt1, TIMEOUT_VALUE);
    aes_test_check(xts_vct15_pt1, xts_vct15_ct, 16);
#if 0
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_8B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, xts_vct15_pt_8, 17, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct15_ct_8, 16);

    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_16B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);
    HAL_CRYP_Encrypt(&hcryp, xts_vct15_pt_16, 17, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct15_ct_16, 16);
#endif

//vct16
    HAL_CRYP_GetConfig(&hcryp, &Conf);
    Conf.DataType   = CRYP_DATATYPE_32B;
    HAL_CRYP_SetConfig(&hcryp, &Conf);

    HAL_CRYP_Encrypt(&hcryp, xts_vct16_pt, 18, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct16_ct, 16);
//vct17
    HAL_CRYP_Encrypt(&hcryp, xts_vct17_pt, 19, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct17_ct, 16);
//vct18
    HAL_CRYP_Encrypt(&hcryp, xts_vct18_pt, 20, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct18_ct, 20);
//vct20
    HAL_CRYP_Encrypt(&hcryp, xts_vct20_pt, 21, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct20_ct, 20);
//vct21
    HAL_CRYP_Encrypt(&hcryp, xts_vct21_pt, 22, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct21_ct, 20);
//vct22
    HAL_CRYP_Encrypt(&hcryp, xts_vct22_pt, 23, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct22_ct, 20);
//vct23
    HAL_CRYP_Encrypt(&hcryp, xts_vct23_pt, 24, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct23_ct, 24);
//vct24
    HAL_CRYP_Encrypt(&hcryp, xts_vct24_pt, 25, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct24_ct, 24);
//vct25
    HAL_CRYP_Encrypt(&hcryp, xts_vct25_pt, 26, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct25_ct, 24);
//vct26
    HAL_CRYP_Encrypt(&hcryp, xts_vct26_pt, 27, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct26_ct, 24);
//vct27
    HAL_CRYP_Encrypt(&hcryp, xts_vct27_pt, 28, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct27_ct, 28);
//vct28
    HAL_CRYP_Encrypt(&hcryp, xts_vct28_pt, 29, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct28_ct, 28);
//vct29
    HAL_CRYP_Encrypt(&hcryp, xts_vct29_pt, 30, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct29_ct, 28);
//vct30
    HAL_CRYP_Encrypt(&hcryp, xts_vct30_pt, 31, Encryptedtext, TIMEOUT_VALUE);
    aes_test_check(Encryptedtext, xts_vct30_ct, 28);


    if (verbose != 0) {
        printf("=======aes_acc_xts_ct_steal_test_Pass");
    }


    hcryp.Init.Xts_Ct_Steal_En = 0;
}


//aes_hal_test_total
void aes_hal_test(int verbose)
{
    memset(&hcryp, 0, sizeof(CRYP_HandleTypeDef));

    //key_lock_test
    static int aes_key_wr_lock_once = 1;
    if (aes_key_wr_lock_once) {
        round_key_wr_lock_test(0);
        aes_key_wr_lock_once = 0;
    }

    test_aes_mode(verbose);
    test_aes_mode_dma(verbose);

    test_aes_gcm(verbose);
    test_aes_gcm_dma(verbose);

    test_aes_ccm(verbose);
    test_aes_ccm_dma(verbose);

    test_aes_xts(verbose);
    test_aes_xts_dma(verbose);

    test_aes_xts_ct_steal(verbose);
}
