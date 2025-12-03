#include "rf_fcc_main.h"

#ifdef TCFG_EXT_RF_FCC_TEST_ENABLE

static void *dev_hdl;
static u8 uart_circlebuf[1024];


static void comm_dev_init(void);
static s32 comm_dev_read(u8 *buf, u32 len);
static s32 comm_dev_write(u8 *buf, u32 len);


static fcc_comm_type comm_ops = {
    .init  = comm_dev_init,
    .read  = comm_dev_read,
    .write = comm_dev_write,
};


static void comm_dev_init(void)
{
    dev_hdl = dev_open(fcc_get_uart(), NULL);
    ASSERT(dev_hdl != NULL, "comm_dev_init err");

    dev_ioctl(dev_hdl, IOCTL_UART_SET_CIRCULAR_BUFF_ADDR, (u32)uart_circlebuf);
    dev_ioctl(dev_hdl, IOCTL_UART_SET_CIRCULAR_BUFF_LENTH, (u32)sizeof(uart_circlebuf));
    dev_ioctl(dev_hdl, IOCTL_UART_SET_RECV_BLOCK, 1);
    dev_ioctl(dev_hdl, IOCTL_UART_SET_RECV_TIMEOUT, 0);
    dev_ioctl(dev_hdl, IOCTL_UART_START, 0);
}


static s32 comm_dev_read(u8 *buf, u32 len)
{
    return dev_read(dev_hdl, buf, len);
}


static s32 comm_dev_write(u8 *buf, u32 len)
{
    return dev_write(dev_hdl, buf, len);
}


fcc_comm_type *fcc_comm_ops(void)
{
    return &comm_ops;
}


#endif


