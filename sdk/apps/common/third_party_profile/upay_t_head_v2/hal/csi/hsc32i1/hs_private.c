
// #include "IIC_Master_Driver.h"
/* #include "asm/iic_hw.h" */
#include "app_config.h"
#include "system/includes.h"
/* #include "iic_api.h" */


const char log_tag_const_v_ALIPAY_HSP AT(.LOG_TAG_CONST) = 1;//LIB_DEBUG & FALSE;
const char log_tag_const_i_ALIPAY_HSP AT(.LOG_TAG_CONST) = 1;//LIB_DEBUG & TRUE;
const char log_tag_const_d_ALIPAY_HSP AT(.LOG_TAG_CONST) = 1;//LIB_DEBUG & TRUE;
const char log_tag_const_w_ALIPAY_HSP AT(.LOG_TAG_CONST) = 1;//LIB_DEBUG & TRUE;
const char log_tag_const_e_ALIPAY_HSP AT(.LOG_TAG_CONST) = 1;//LIB_DEBUG & TRUE;

#define LOG_TAG_CONST       ALIPAY_HSP
#define LOG_TAG             "[ALIPAY_HSP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "system/debug.h"
#include "gpio.h"

#if TCFG_PAY_ALIOS_ENABLE

#define _IIC_USE_HW

typedef struct {
    u8 init;
    char *iic_hdl;
} ft_param;

static ft_param module_param = {
    .init = 0,
    .iic_hdl = "iic3",
};
#define  SE_GPIO_RESET_PIN   IO_PORTA_02 // SE芯片的reset管脚配置,默认用IO_PORTA_02

#define __this   (&module_param)

#define HOST_ID        1
#define ALIPAY_SE_FW_V2_0  1


//unsigned char g_HSI2CBuf[2048];
unsigned char *g_HSI2CBuf;
#define HS_BUF_LEN 4096//2048
#if ALIPAY_SE_FW_V2_0

unsigned char gHSEncTransSign;	//通信是否加密，0 = 明文通信，1 = 加密通信
unsigned char g_DefTransKey[16] = {0x54, 0x8E, 0x8D, 0x7A, 0x26, 0x35, 0x85, 0x11, 0x25, 0x2E, 0xF8, 0xD6, 0xB7, 0x17, 0xA4, 0xE4};
unsigned char g_TransTmpKey[16];

static void delay_ms(int cnt)
{
    int delay = (cnt + 9) / 10;
    os_time_dly(delay);
}

void Delay_Ms(unsigned char byMilliSec)
{
    delay_ms(byMilliSec);
}
void Set_GPIO_RESET_DLE(void)
{
#if ALIPAY_SE_USE_RESET_PIN
    u32 gpio = SE_GPIO_RESET_PIN;//指定IO给加密芯片reste
    gpio_hw_set_pull_down(gpio / 16, BIT(gpio % 16), 0);
    gpio_hw_set_pull_up(gpio / 16, BIT(gpio % 16), 0);
    gpio_hw_direction_input(gpio / 16, BIT(gpio % 16));
    gpio_hw_set_die(gpio / 16, BIT(gpio % 16), 0);
#endif
}
void Set_GPIO_State(unsigned char byState)
{
#if ALIPAY_SE_USE_RESET_PIN
    u32 gpio = SE_GPIO_RESET_PIN;//指定IO给加密芯片reste
    gpio_hw_set_pull_down(gpio / 16, BIT(gpio % 16), 0);
    gpio_hw_set_pull_up(gpio / 16, BIT(gpio % 16), 0);
    gpio_hw_set_drive_strength(gpio / 16, BIT(gpio % 16), 0);//看需求是否需要开启强推,会导致芯片功耗大
    //gpio_hw_set_hd0(gpio / 16, BIT(gpio % 16), 0);
    gpio_hw_set_direction(gpio / 16, BIT(gpio % 16), 0);
    gpio_hw_set_output_value(gpio / 16, BIT(gpio % 16), byState); //1高0低*/

    return ;
#endif
}
void se_ic_reset(void)
{
    printf("func =%s", __func__);
    Set_GPIO_State(0);
    Delay_Ms(5);
    Set_GPIO_State(1);
    Delay_Ms(30);
    Set_GPIO_RESET_DLE();
    return ;
}
#endif
int system_iic_write_nbytes(char *iic,
                            unsigned char dev_7bit_addr, //7位设备地址
                            unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
                            unsigned char *write_buf, int write_len)//数据buf, 写入长度
{
    int ret;

    ret = i2c_master_write_nbytes_to_device_reg(iic, dev_7bit_addr << 1, reg_addr, reg_len, write_buf, write_len);

    return ret;
}
int system_iic_read_nbytes(char *iic,
                           unsigned char dev_7bit_addr, //7位设备地址
                           unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
                           unsigned char *read_buf, int read_len)//缓存buf，读取长度
{
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));

    int ret;

    ret = i2c_master_read_nbytes_from_device_reg(iic, dev_7bit_addr << 1, reg_addr, reg_len, read_buf, read_len);

    return ret;
}
//extern unsigned char * IIC_Master_Init(void);

//extern void IIC_Master_Send(unsigned char byAddr, unsigned char *pData, unsigned short wLen);

//extern void IIC_Master_Receive(unsigned char byAddr, unsigned char *pData, unsigned short wLen);

//CRC单字节计算——循环左移，先移最高bit
unsigned short LeftShift_CRC(unsigned char byValue, unsigned short *lpwCrc)
{
    byValue ^= (unsigned char)(*lpwCrc >> 8);
    byValue ^= (byValue >> 4);

    *lpwCrc = (*lpwCrc << 8) ^ (unsigned short)(byValue);
    *lpwCrc ^= (unsigned short)byValue << 12;
    *lpwCrc ^= (unsigned short)byValue << 5;

    return (*lpwCrc);
}

void ComputeCRC(unsigned char *Data, unsigned short Length, unsigned char *pbyOut)
{
    unsigned char chBlock;
    unsigned short wCrc;
    log_info("%s\n", __func__);

    wCrc = 0x0000; // ITU-V.41

    while (Length--) {
        chBlock = *Data++;
        LeftShift_CRC(chBlock, &wCrc);
    }

    pbyOut[0] = (unsigned char)((wCrc >> 8) & 0xFF);
    pbyOut[1] = (unsigned char)(wCrc & 0xFF);

    return;
}

void lmemset(unsigned char *pBuf, unsigned char byVal, unsigned short wLen)
{
    unsigned short i;
    log_info("%s\n", __func__);

    for (i = 0; i < wLen; ++i) {
        pBuf[i] = byVal;
    }
    return ;
}

void lmemcpy(unsigned char *pDst, unsigned char *pSrc, unsigned short wLen)
{
    unsigned short i;

    if ((pSrc == pDst) || (wLen == 0)) {
        return;
    }

    if (pSrc < pDst) {
        for (i = wLen - 1; i != 0; --i) {
            pDst[i] = pSrc[i];
        }
        pDst[0] = pSrc[0];
    } else {
        for (i = 0; i < wLen; ++i) {
            pDst[i] = pSrc[i];
        }
    }

    return ;
}

//相同返回0，不同返回1
unsigned char lmemcmp(unsigned char *pBuf1, unsigned char *pBuf2, unsigned short wLen)
{
    unsigned short i;
    log_info("%s\n", __func__);

    for (i = 0; i < wLen; ++i) {
        if (pBuf1[i] != pBuf2[i]) {
            return 1;
        }
    }

    return 0;
}

#define TEST_1		0

#define FT6336G_IIC_ADDR  0xC8
#define FT6336G_IIC_DELAY   1

#define HS_IIC_DELAY  120

#define SLAVE_ADDR      0x50

#define WRITE_ADDR      SLAVE_ADDR
#define READ_ADDR       (WRITE_ADDR|BIT(0))
static int hs_i2c_write(u8 *buf, int len)
{

#if TEST_1
    int ret = 0;
    log_info("%s[len:%d]", __func__, len);
    iic_start(__this->iic_hdl);
    ret = iic_tx_byte(__this->iic_hdl, WRITE_ADDR);
    if (0 == ret) {
        ret = -1;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    ret = iic_tx_byte(__this->iic_hdl, 0x55);
    if (0 == ret) {
        ret = -2;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, buf, len);
    if (0 == ret) {
        ret = -3;
        goto __end;
    }
__end:
    iic_stop(__this->iic_hdl);
    Delay_Ms(HS_IIC_DELAY);
    log_info("%s[ret:%d]", __func__, ret);
    return ret;
#else

    int ret = 0;
    log_info("%s[len:%d]", __func__, len);
    iic_start(__this->iic_hdl);
    ret = iic_tx_byte(__this->iic_hdl, FT6336G_IIC_ADDR);
    if (0 == ret) {
        ret = -1;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, buf, len);
    if (0 == ret) {
        ret = -3;
        goto __end;
    }
__end:
    iic_stop(__this->iic_hdl);
    Delay_Ms(HS_IIC_DELAY);
    log_info("%s[ret:%d]", __func__, ret);
    return ret;
#endif
}

u8 hs_i2c_read(u8 addr, u8 *buf, unsigned short len)
{
#if TEST_1
    int ret = 0;
    log_info("%s[len:%d]", __func__, len);
    iic_start(__this->iic_hdl);
    ret = iic_tx_byte(__this->iic_hdl, WRITE_ADDR);
    if (0 == ret) {
        ret = -1;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    ret = iic_tx_byte(__this->iic_hdl, 0xAA);
    if (0 == ret) {
        ret = -2;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    iic_start(__this->iic_hdl);
    ret = iic_tx_byte(__this->iic_hdl, READ_ADDR);
    if (0 == ret) {
        ret = -3;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    ret = iic_read_buf(__this->iic_hdl, buf, len);
    if (0 == ret) {
        ret = -4;
        goto __end;
    }
__end:
    iic_stop(__this->iic_hdl);
    Delay_Ms(HS_IIC_DELAY);
    log_info("%s[ret:0x%x]", __func__, ret);
    return ret;
#else
    int ret = 0;
    log_info("%s[len:%d]", __func__, len);

    iic_start(__this->iic_hdl);
    ret = iic_tx_byte(__this->iic_hdl, addr);
    if (0 == ret) {
        ret = -3;
        goto __end;
    }
    Delay_Ms(HS_IIC_DELAY);
    ret = iic_read_buf(__this->iic_hdl, buf, len);
    if (0 == ret) {
        ret = -4;
        goto __end;
    }
__end:
    iic_stop(__this->iic_hdl);
    Delay_Ms(HS_IIC_DELAY);
    put_buf(buf, len);
    log_info("%s[ret:0x%x]", __func__, ret);
    return ret;
#endif
}


static OS_MUTEX g_HSI2CBuf_mutex;

void pay_g_HSI2CBuf_lock()
{
    os_mutex_pend(&g_HSI2CBuf_mutex, 0);
}

void pay_g_HSI2CBuf_unlock()
{
    os_mutex_post(&g_HSI2CBuf_mutex);
}

void HS_IIC_Init(void)
{
#if ALIPAY_SE_FW_V2_0
    unsigned char ver[29];
    unsigned char i;

#if ALIPAY_SE_USE_RESET_PIN
    extern void se_ic_reset(void);
    se_ic_reset();
    os_time_dly(3);
#endif
    gHSEncTransSign = 0x00;
    lmemset(g_TransTmpKey, 0x00, 16);
#endif

    printf("%s[hd:%s]\n", __func__, __this->iic_hdl);

    if (!g_HSI2CBuf) {
        g_HSI2CBuf = malloc(HS_BUF_LEN);
    }
    printf("[msg]%s-%d>>>>>>>>>>>g_HSI2CBuf=%x", __FUNCTION__, __LINE__, g_HSI2CBuf);
    os_mutex_create(&g_HSI2CBuf_mutex);//按阿里要求加上互斥
    // 因tp与se共用一组iic，支付宝iic在亮屏时init，不在这里再次init
    //system_iic_init(__this->iic_hdl, get_iic_config(__this->iic_hdl));
    printf("delay:%d\n", HS_IIC_DELAY);

#if ALIPAY_SE_FW_V2_0
    g_HSI2CBuf[0] = 0x00;
    g_HSI2CBuf[1] = 0x05;
    g_HSI2CBuf[2] = 0x00;
    g_HSI2CBuf[3] = 0xCA;
    g_HSI2CBuf[4] = 0x00;
    g_HSI2CBuf[5] = 0x00;
    g_HSI2CBuf[6] = 0x00;
    ComputeCRC(g_HSI2CBuf, 7, g_HSI2CBuf + 7);
    unsigned char HS_IIC_Recv_Resp(unsigned char *pData, unsigned short wLen);
    void HS_IIC_Send_Cmd(unsigned char *pData, unsigned short wLen);
    HS_IIC_Send_Cmd(g_HSI2CBuf, 9);
    if (0 != HS_IIC_Recv_Resp(ver, 29)) { //如果获取初值密钥指令执行异常，就用16字节FF进行加密通信
        gHSEncTransSign = 0x01;
        lmemset(g_TransTmpKey, 0xFF, 16);
        return ;
    }
    printf("\n [ERROR] %s -[yuyu] %d\n", __FUNCTION__, __LINE__);

    if ((ver[0] == 0x80) && (ver[1] == 0x10) && (ver[18] == 0x81) && (ver[19] == 0x02)) { //简单的检查一下数据格式内容
        if ((ver[20] != 0x10) || (ver[21] != 0x03)) {
            gHSEncTransSign = 0x01;
            for (i = 0; i < 16; ++i) {
                g_TransTmpKey[i] = ver[2 + i] ^ g_DefTransKey[i];
            }
        }
    } else {
        gHSEncTransSign = 0x01;
        lmemset(g_TransTmpKey, 0xFF, 16);
    }

#endif  //ALIPAY_SE_FW_V2_0
}

void HS_IIC_Uninit(void)
{
    log_info("%s[hd:%d]\n", __func__, __this->iic_hdl);
    // 因tp与se共用一组iic，支付宝iic在灭屏时deinit，不在这里再次deinit
    //system_iic_deinit(__this->iic_hdl);
    printf("delay:%d\n", HS_IIC_DELAY);
    if (g_HSI2CBuf) {
        free(g_HSI2CBuf);
        g_HSI2CBuf = NULL;
    }
}

void HS_IIC_Send_Cmd(unsigned char *pData, unsigned short wLen)
{
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    log_info("%s[len:%d] rets=%x\n", __func__, wLen, rets_addr);
    put_buf(pData, wLen);
    int ret = system_iic_write_nbytes(__this->iic_hdl, FT6336G_IIC_ADDR >> 1, NULL, 0, pData, wLen);
    log_debug("%s[ret:%d]\n", __func__, ret);
}

//功能:	接收wLen长度的数据，到pData缓冲区中
//返回:
//			0 表示正确接收
//			1 表示接收数据CRC错误
//			2 表示实际接收数据长度域不是wLen
//			3 表示指令返回状态字不是9000
//			4 表示指令超时未响应
unsigned char HS_IIC_Recv_Resp(unsigned char *pData, unsigned short wLen)
{
    unsigned short wRspLen;
    unsigned char crc[2];
    unsigned int dwCnt;
    unsigned short i;
    log_info("%s[len:%d]\n", __func__, wLen);

    g_HSI2CBuf[0] = 0x00;
    dwCnt = 0;
    while (g_HSI2CBuf[0] != 0x55) {
        ++dwCnt;
        if (dwCnt > 0x100) {
            return 4;
        }
        system_iic_read_nbytes(__this->iic_hdl, (FT6336G_IIC_ADDR + 1) >> 1, NULL, 0, g_HSI2CBuf, 1);
        put_buf(g_HSI2CBuf, 1);

    }

    int rlen = wLen + 6;

    system_iic_read_nbytes(__this->iic_hdl, (FT6336G_IIC_ADDR + 1) >> 1, NULL, 0, g_HSI2CBuf, rlen);

    wRspLen = g_HSI2CBuf[0];
    wRspLen <<= 8;
    wRspLen |= g_HSI2CBuf[1];


    if (wRspLen != wLen + 2) {


        return 2;
    }

#if  ALIPAY_SE_FW_V2_0
    ComputeCRC(g_HSI2CBuf + 2, wLen + 2, crc);
    if ((g_HSI2CBuf[wLen + 4] != crc[0]) || (g_HSI2CBuf[wLen + 5] != crc[1])) {
        return 1;
    }

    if (gHSEncTransSign == 0x01) {
        for (i = 0; i < wLen + 2; ++i) {
            g_HSI2CBuf[2 + i] ^= g_TransTmpKey[i % 16];
        }
    }
#endif

    if ((g_HSI2CBuf[2 + wLen] != 0x90) || (g_HSI2CBuf[3 + wLen] != 0x00)) { //状态字


        pData[0] = g_HSI2CBuf[2 + wLen];
        pData[1] = g_HSI2CBuf[3 + wLen];

        return 3;
    }

#if  ALIPAY_SE_FW_V2_0
#else
    ComputeCRC(g_HSI2CBuf + 2, wLen + 2, crc);


    if ((g_HSI2CBuf[wLen + 4] != crc[0]) || (g_HSI2CBuf[wLen + 5] != crc[1])) {


        return 1;
    }
#endif

    lmemcpy(pData, g_HSI2CBuf + 2, wLen);


    return 0;

}

static u8 write[16];
static u8 read[16];
static u8 receive[16];
void iic_master_test(void)
{

    int ret = 0;
    int ret1 = 0;
    memset(read, 0x00, sizeof(read));
    write[0]++;
    for (u8 i = 0; i < sizeof(write); i++) {
        write[i] = i;
    }
    write[0] = 1;
    write[sizeof(write) - 1] = 0xff; //包结束符 //视情况修改
    HS_IIC_Send_Cmd(write, sizeof(write));
    ret1 = system_iic_read_nbytes(__this->iic_hdl, 0xAA, NULL, 0, read, sizeof(read));

    log_info("%s[send 0x%x  0x%x]", __func__, write[0], ret);
    put_buf(receive, sizeof(receive));
    log_info("%s[read 0x%x]", __func__, ret1);
    if (ret1 > 0) {
        put_buf(read, ret1);
    }
}

#endif
