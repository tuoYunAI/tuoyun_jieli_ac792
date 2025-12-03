#include "alipay.h"
#include "alipay_bind.h"
#include "alipay_common.h"
#include "device/gpio.h"
#include "app_config.h"
#include "system/sys_time.h"

#if TCFG_PAY_ALIOS_ENABLE

#define LOG_TAG_CONST       ALIPAY
#define LOG_TAG     		"[ALIPAY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

#define QR_CODE_MESS_SIZE   			(180 * sizeof(u8))
static char *qr_code_mess = NULL;

#define alipay_QRcode_mess				qr_code_mess
#define ALIPAY_QRCODE_BUF_LEN			QR_CODE_MESS_SIZE

#define ALIPAY_CHECK_TIME_MS				200
#define ALIPAY_CHECK_TIME_OUT_CNT			(30*1000/ALIPAY_CHECK_TIME_MS)	// 绑定超时
#define ALIPAY_WARN_TIME_OUT_CNT			(5*1000/ALIPAY_CHECK_TIME_MS)	// 提示时间
#define ALIPAY_REFRESH_TIME_OUT_CNT			(60*1000/ALIPAY_CHECK_TIME_MS)	// 超时刷新支付码

#define ALIPAY_QRCODE_NAME_LEN			(ALIPAY_QRCODE_BUF_LEN/2)
#define ALIPAY_QRCODE_ID_LEN			(ALIPAY_QRCODE_BUF_LEN - ALIPAY_QRCODE_NAME_LEN)

#define ALIPAY_QRCODE_NAME_OFFSET		(0)
#define ALIPAY_QRCODE_ID_OFFSET			(ALIPAY_QRCODE_NAME_OFFSET + ALIPAY_QRCODE_NAME_LEN)

#define BINDING_STU_E					binding_status_e
#define GET_BINDING_STU()				alipay_get_binding_status()
#define GET_PAYCODE(x,y)				alipay_get_paycode(x,y)
#define GET_NICK_NAME(x,y)				alipay_get_nick_name(x,y)
#define GET_LOGON_ID(x,y)				alipay_get_logon_ID(x,y)

#define BINDING_STU_OK					ALIPAY_STATUS_BINDING_OK
#define BINDING_STU_FAIL				ALIPAY_STATUS_BINDING_FAIL
#define BINDING_STU_GETTING_PROFILE		ALIPAY_STATUS_START_BINDING
#define BINDING_STU_FINISH_OK			ALIPAY_STATUS_BINDING_OK
#define BINDING_STU_UNKNOWN             ALIPAY_STATUS_UNKNOWN

#define GET_BINDING_STRING(x,y)			alipay_get_binding_code(x,y)//       alipay_get_binding_string(x,y)//alipay_get_aid_code(x,y)
#define GET_VERSION()					printf("SE2.0,SDK Hard alipay V201")
#define ENV_INIT()				        alipay_pre_init()//	alipay_bind_env_init()//	alipay_pre_init()
#define UNBINDING()						alipay_unbinding()

#define ALIOS_CACHE_INIT()				alipay_cache_buf_init()
#define ALIOS_CACHE_UNINIT()			alipay_cache_buf_uninit()
#define POWER_ON()                      alipay_power_on()
#define POWER_OFF()                     alipay_power_off()

#define IO_IS_SET_DRIVE_STRENGTH           0

u8 alipay_check_open_status = 0;

void alipay_power_on(void)
{
#if ALIPAY_SE_USE_RESET_PIN
    HS_IIC_Init();
    log_info("%s,%d", __func__, __LINE__);
    os_time_dly(5);
    if (alipay_check_open_status == 0) {
        csi_exit_lpm();
    }
    alipay_check_open_status = 1;
#else
    log_info("%s,%d", __func__, __LINE__);
    alipay_check_open_status = 1;
    printf("alipay_check_open_status =%d", alipay_check_open_status);
#if 0  //SE 电源配置管脚
    gpio_set_pull_up(SE_POWER_GPIO, GPIO_PULL_UP_DISABLE);
    gpio_set_pull_down(SE_POWER_GPIO, GPIO_PULL_DOWN_DISABLE);
#if IO_IS_SET_DRIVE_STRENGTH
    spin_lock(&locks[2]);
    JL_PORTC->HD0 |= BIT(3);
    JL_PORTC->HD1 |= BIT(3);
    spin_unlock(&locks[2]);
#endif
    gpio_direction_output(SE_POWER_GPIO, 1);
    os_time_dly(5);
#endif
    extern void HS_IIC_Init(void);
    HS_IIC_Init();
#endif
    ALIOS_CACHE_INIT();
    u8 ret = ENV_INIT();    //环境初始化，此时状态应该为STATUS_START_BINDING
    /* qr_code_mess = zalloc(ALIPAY_QRCODE_BUF_LEN); */
}

void alipay_power_off(void)
{
#if  ALIPAY_SE_USE_RESET_PIN
    log_info("%s,%d", __func__, __LINE__);
    csi_enter_lpm();
    alipay_check_open_status = 0;
    os_time_dly(2);
    extern void HS_IIC_Uninit(void);
    HS_IIC_Uninit();
    Set_GPIO_RESET_DLE();//设为高阻态

#else
    log_info("%s,%d", __func__, __LINE__);
    alipay_check_open_status = 0;
    printf("alipay_check_open_status =%d", alipay_check_open_status);
    extern void HS_IIC_Uninit(void);
    HS_IIC_Uninit();
    os_time_dly(1);
    gpio_direction_output(SE_POWER_GPIO, 0);
#endif

    /* free(qr_code_mess); */
    ALIOS_CACHE_UNINIT();
}
void upay_recv_data_handle(const uint8_t *data, u16 len)
{
    extern int upay2ali_ibuf_to_cbuf(u8 * buf, u32 len);
    upay2ali_ibuf_to_cbuf((u8 *)data, len);
    extern void upay2ali_send_event(void);
    upay2ali_send_event();
}
void alipay_upay_init()
{
    extern void upay_ble_regiest_recv_handle(void (*handle)(const uint8_t *data, u16 len));
    upay_ble_regiest_recv_handle(upay_recv_data_handle);
}
void alipay_get_bind_code(char *data)
{
    upay_ble_mode_enable(1);
    printf("After init ,status is %02X,  \n", GET_BINDING_STU());
    u32 ret = 0;
    u32 msg_len = ALIPAY_QRCODE_BUF_LEN;
    memset(data, 0, msg_len);
    ret = GET_BINDING_STRING(data, (int *)&msg_len);   //获取绑定码
    printf("upay_get_binding_string, len:%d, ret:%d \n", msg_len, ret);
    printf("%s\n", data);
}
void alipay_get_pay_code(char *data)
{
    printf("After init ,status is %02X,  \n", GET_BINDING_STU());
    u32 msg_len = ALIPAY_QRCODE_BUF_LEN;
    memset(data, 0, ALIPAY_QRCODE_BUF_LEN);
    GET_PAYCODE((uint8_t *)data, (uint32_t *) &msg_len); //获取支付码
    printf("%s\n", data);
}
void alipay_get_account(char *account_name, char *account_id)
{
    u32 account_name_len = 128;
    u32 account_id_len = 128;
    memset(account_name, 0, 128);
    memset(account_id, 0, 128);
    alipay_get_nick_name((u8 *)account_name, &account_name_len);
    alipay_get_logon_ID((u8 *)account_id, &account_id_len);
    printf("[msg]%s-%d>>>>>>>>>>>account_name=%s", __FUNCTION__, __LINE__, account_name);
    printf("[msg]%s-%d>>>>>>>>>>>account_id=%s", __FUNCTION__, __LINE__, account_id);
}
void alipay_unpair()
{
    UNBINDING();
}
void bt_addr2string(u8 *addr, u8 *buf)
{
    u8 len = 0;
    for (s8 i = 5; i >= 0; i--) {
        if ((addr[i] / 16) >= 10) {
            buf[len] = 'A' + addr[i] / 16 - 10;
        } else {
            buf[len] = '0' + addr[i] / 16;
        }
        if ((addr[i] % 16) >= 10) {
            buf[len + 1] = 'A' + addr[i] % 16 - 10;
        } else {
            buf[len + 1] = '0' + addr[i] % 16;
        }
        len += 2;
        buf[len] = ':';
        len += 1;
    }
    buf[len - 1] = '\0';
    log_info("%s", buf);
}

#endif
