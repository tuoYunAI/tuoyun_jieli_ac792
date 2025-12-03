#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uartPcmSender.data.bss")
#pragma data_seg(".uartPcmSender.data")
#pragma const_seg(".uartPcmSender.text.const")
#pragma code_seg(".uartPcmSender.text")
#endif
#include "app_config.h"
#include "uartPcmSender.h"
#if ((defined TCFG_AUDIO_DATA_EXPORT_DEFINE) && (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART))
#include "system/includes.h"
#include "uart.h"
#include "asm/uart.h"
#include "device/uart.h"

struct uart_send_hdl_t {
    void *uart_hdl;
    u8 *dma_buf;
    int dma_buf_size;
};
struct uart_send_hdl_t *uart_send_hdl = NULL;

void uartSendData(void *buf, u16 len) 			//发送数据的接口。
{
    struct uart_send_hdl_t *hdl = uart_send_hdl;
    uartSendInit();
    if (hdl) {
        if (hdl->uart_hdl != NULL) {
            if (hdl->dma_buf == NULL) {
                printf("%s : %d", __func__, __LINE__);
                hdl->dma_buf_size = len;
                hdl->dma_buf = malloc(hdl->dma_buf_size);
                if (!hdl->dma_buf) {
                    printf("uartSendData malloc fail!");
                }

                // 设置发送数据为阻塞方式(非必须)
                dev_ioctl(hdl->uart_hdl, IOCTL_UART_SET_SEND_BLOCK, 1);

                // 使能串口,启动收发数据(必须)
                dev_ioctl(hdl->uart_hdl, IOCTL_UART_START, 0);

            }
            if (hdl->dma_buf_size != len) {
                printf("%s : %d", __func__, __LINE__);
                free(hdl->dma_buf);
                hdl->dma_buf_size = len;
                hdl->dma_buf = malloc(hdl->dma_buf_size);
                if (!hdl->dma_buf) {
                    printf("uartSendData remalloc fail!");
                }

                // 设置发送数据为阻塞方式(非必须)
                dev_ioctl(hdl->uart_hdl, IOCTL_UART_SET_SEND_BLOCK, 1);

                // 使能串口,启动收发数据(必须)
                dev_ioctl(hdl->uart_hdl, IOCTL_UART_START, 0);
            }

            memcpy(hdl->dma_buf, buf, len);
            int wlen = dev_write(hdl->uart_hdl, hdl->dma_buf, hdl->dma_buf_size);
            if (wlen != hdl->dma_buf_size) {
                putchar('f');
            }
        }
    }
    return;

}

void uartSendInit()
{
    if (uart_send_hdl) {
        return;
    }
    struct uart_send_hdl_t *hdl = zalloc(sizeof(*hdl));
    uart_send_hdl = hdl;

    hdl->uart_hdl = dev_open("uart1", 0);
    if (!hdl->uart_hdl) {
        printf("uartSendInit dev_open err!");
    }

    return;
}

void uartSendExit()
{
    struct uart_send_hdl_t *hdl = uart_send_hdl;
    if (hdl) {
        if (hdl->uart_hdl) {
            dev_close(hdl->uart_hdl);
            hdl->uart_hdl = NULL;
        }

        if (hdl->dma_buf) {
            free(hdl->dma_buf);
            hdl->dma_buf = NULL;
        }
        free(hdl);
        hdl = NULL;
        uart_send_hdl = NULL;
    }
}

#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/
