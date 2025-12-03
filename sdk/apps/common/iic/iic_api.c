#include "device/iic.h"

#define LOG_TAG_CONST       IIC_API
#define LOG_TAG             "[IIC_API]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "system/debug.h"

int i2c_master_read_nbytes_from_device(const char *dev_name,
                                       unsigned char dev_addr, //设备地址，无设备寄存器地址
                                       unsigned char *read_buf, unsigned int read_len)//缓存buf, 读取长度
{
    int res = 0;

    void *iic = dev_open(dev_name, NULL);
    if (!iic) {
        return IIC_ERROR_INIT_FAIL;
    }

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    res = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, dev_addr & ~0x01);
    if (res != IIC_OK) {
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR;
        goto __exit;
    }

    dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);

    res = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, dev_addr | 0x01);
    if (res != IIC_OK) {
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR;
        goto __exit;
    }

    while (--read_len > 0) {
        dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_ACK, (u32)&read_buf[res++]);
    }

    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_NACK, (u32)&read_buf[res++]);

__exit:
    dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);

    dev_close(iic);

    return res;
}

int i2c_master_read_nbytes_from_device_reg(const char *dev_name,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len,//设备寄存器地址，长度
        unsigned char *read_buf, unsigned int read_len)//缓存buf，读取长度
{
    int res = 0;

    void *iic = dev_open(dev_name, NULL);
    if (!iic) {
        return IIC_ERROR_INIT_FAIL;
    }

    dev_ioctl(iic, IOCTL_IIC_START, 0);
    if (reg_addr != NULL) {

        res = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, dev_addr & ~0x01);
        if (res != IIC_OK) {
            res = IIC_ERROR_DEV_ADDR_ACK_ERROR;
            goto __exit;
        }

        for (res = 0; res < reg_len; ++res) {
            if (IIC_OK != dev_ioctl(iic, IOCTL_IIC_TX_BYTE, reg_addr[res])) {
                res = IIC_ERROR_REG_ADDR_ACK_ERROR;
                log_error("reg addr no ack!");
                goto __exit;
            }
        }

        dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);
    }
    res = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, dev_addr | 0x01);
    if (res != IIC_OK) {
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR;
        goto __exit;
    }

    while (--read_len > 0) {
        dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_ACK, (u32)&read_buf[res++]);
    }

    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_NACK, (u32)&read_buf[res++]);

__exit:
    dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);

    dev_close(iic);

    return res;
}

int i2c_master_write_nbytes_to_device(const char *dev_name,
                                      unsigned char dev_addr, //设备地址，无设备寄存器地址
                                      unsigned char *write_buf, unsigned int write_len) //数据buf, 写入长度
{
    int res = 0;

    void *iic = dev_open(dev_name, NULL);
    if (!iic) {
        return IIC_ERROR_INIT_FAIL;
    }

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    res = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, dev_addr);
    if (res != IIC_OK) {
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR;
        goto __exit;
    }

    for (res = 0; res < write_len; ++res) {
        if (IIC_OK != dev_ioctl(iic, IOCTL_IIC_TX_BYTE, write_buf[res])) {
            log_error("write data no ack!");
            goto __exit;
        }
    }

__exit:
    dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);

    dev_close(iic);

    return res;
}

int i2c_master_write_nbytes_to_device_reg(const char *dev_name,
        unsigned char dev_addr, //设备地址
        unsigned char *reg_addr, unsigned char reg_len, //设备寄存器地址，长度
        unsigned char *write_buf, unsigned int write_len) //数据buf, 写入长度
{
    int res = 0;

    void *iic = dev_open(dev_name, NULL);
    if (!iic) {
        return IIC_ERROR_INIT_FAIL;
    }

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    res = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, dev_addr);
    if (res != IIC_OK) {
        res = IIC_ERROR_DEV_ADDR_ACK_ERROR;
        goto __exit;
    }

    for (res = 0; res < reg_len; ++res) {
        if (IIC_OK != dev_ioctl(iic, IOCTL_IIC_TX_BYTE, reg_addr[res])) {
            res = IIC_ERROR_REG_ADDR_ACK_ERROR;
            log_error("reg addr no ack!");
            goto __exit;
        }
    }

    for (res = 0; res < write_len; ++res) {
        if (IIC_OK != dev_ioctl(iic, IOCTL_IIC_TX_BYTE, write_buf[res])) {
            log_error("write data no ack!");
            goto __exit;
        }
    }

__exit:
    dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);

    dev_close(iic);

    return res;
}

//rx协议:start,addr write,data0,data1,,,,,,stop
int hw_iic_slave_polling_rx(void *iic, u8 *rx_buf)
{
    int rx_cnt = 0, rx_state;

    if (!iic) {
        return 0;
    }

    log_info("--iic slave polling rx --");

    dev_ioctl(iic, IOCTL_IIC_START, 0);

    rx_state = dev_ioctl(iic, IOCTL_IIC_SET_SLAVE_RX_PREPARE, 600000);
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx

    } else if (rx_state == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        log_info("iic slave wait addr timeout!");
        goto __exit;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        log_info("iic slave wait end!");
        goto __exit;
    }

    rx_state = dev_ioctl(iic, IOCTL_IIC_RX_BYTE, (u32)&rx_buf[0]);
    if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx

    } else if (rx_state == IIC_SLAVE_RX_ADDR_NO_MATCH) { //error
        log_error("iic slave rx addr error!");
        goto __exit;
    } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx

    }

    rx_state = dev_ioctl(iic, IOCTL_IIC_SET_SLAVE_RX_PREPARE, 100000);
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx

    } else if (rx_state  == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        log_error("iic slave wait reg timeout!");
        goto __exit;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        log_info("iic slave wait end!");
        goto __exit;
    }

    rx_cnt = dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)&rx_buf[1]);
    ++rx_cnt;

    log_info_hexdump(rx_buf, rx_cnt);

__exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);

    log_info("~~~~~iic rx polling end~~~~~");

    return rx_cnt;
}

//tx协议:start,addr read,data0,data1,,,,,nack,stop
int hw_iic_slave_polling_tx(void *iic, u8 *tx_buf)
{
    int rx_cnt = 0, tx_cnt = 0, rx_state;
    u8 slave_rx_data[3] = {0, 0, 0};

    if (!iic) {
        return 0;
    }

    log_info("--iic slave polling tx --");

    dev_ioctl(iic, IOCTL_IIC_START, 0);

#if SLAVE_NO_STRETCH_AUTO_TASK
    dev_ioctl(iic, IOCTL_IIC_TX_BYTE, tx_buf[tx_cnt]);
#endif

    rx_state = dev_ioctl(iic, IOCTL_IIC_SET_SLAVE_RX_PREPARE, 600000);
    if (rx_state == IIC_SLAVE_RX_PREPARE_OK) { //rx

    } else if (rx_state == IIC_SLAVE_RX_PREPARE_TIMEOUT) { //error
        log_info("iic slave wait addr timeout!");
        goto __exit;
    } else if (rx_state == IIC_SLAVE_RX_PREPARE_END_OK) { //end
        log_info("iic slave wait end!");
        goto __exit;
    }

    rx_state = dev_ioctl(iic, IOCTL_IIC_RX_BYTE, (u32)&slave_rx_data[rx_cnt++]);
    if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx

    } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
#if SLAVE_NO_STRETCH_AUTO_TASK
        tx_cnt = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, (u32)&tx_buf[++tx_cnt]);
        ++tx_cnt;	//上面预先填充了一个字节
#else
        tx_cnt = dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, (u32)&tx_buf[tx_cnt]);
#endif
    } else {
        log_error("iic slave rx addr error!");
        goto __exit;
    }

    log_info_hexdump(slave_rx_data, rx_cnt);

__exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);

    log_info("~~~~~iic tx polling end~~~~~");

    return tx_cnt;
}

static void *iic;
static u8 slave_rx_buf_test[256];
static u8 slave_tx_buf_test[256];
static u16 slave_rx_cnt;
static u16 slave_tx_cnt;

#define SLAVE_NO_STRETCH_AUTO_TASK 0

//rx协议:start,addr write,data0,data1,,,,,,stop
//tx协议:start,addr read,data0,data1,,,,,nack,stop
//IIC中断响应一定要快，否则会丢失数据
static void hw_iic_isr_callback(void)//slave test
{
    u8 ack = 0;
    int rx_cnt;
    iic_plus_pnd_typedef_t pnd;
    static int rx_state = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);

#if SLAVE_NO_STRETCH_AUTO_TASK
    pnd = IIC_PND_TXTASK_LOAD;
    dev_ioctl(iic, IOCTL_IIC_GET_PNDING, (u32)&pnd);
    if (pnd == TRUE) {
        dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)slave_tx_buf_test[++slave_tx_cnt]);
    }

    pnd = IIC_PND_RXDATA_DONE;
    dev_ioctl(iic, IOCTL_IIC_GET_PNDING, (u32)&pnd);
    if (pnd == TRUE) {
        rx_cnt = dev_ioctl(iic, IOCTL_IIC_RX_BYTE, (u32)&slave_rx_buf_test[slave_rx_cnt]); //addr
        if (rx_cnt == IIC_SLAVE_RX_DATA) {
            slave_rx_cnt++;
        }
    }

    pnd = IIC_PND_STOP;
    dev_ioctl(iic, IOCTL_IIC_GET_PNDING, (u32)&pnd);
    if (pnd == TRUE) {
        pnd = 0x1f << 20;
        dev_ioctl(iic, IOCTL_IIC_CLR_PNDING, (u32)pnd);
        dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)slave_tx_buf_test[slave_tx_cnt]);
    }
#else
    if (rx_state == 0) {
        rx_state = dev_ioctl(iic, IOCTL_IIC_RX_BYTE, (u32)&slave_rx_buf_test[slave_rx_cnt]); //addr
        if (rx_state == IIC_SLAVE_RX_ADDR_RX) { //rx
            ++slave_rx_cnt;
            /* rx_cnt = dev_ioctl(iic, IOCTL_IIC_SET_SLAVE_RX_PREPARE, 0); */
        } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
            dev_ioctl(iic, IOCTL_IIC_TX_BYTE, slave_tx_buf_test[slave_tx_cnt++]);
        } else { //error
            log_info("iic slave rx addr error!");
            rx_state = 0;
        }
    }

    pnd = IIC_PND_STOP;
    dev_ioctl(iic, IOCTL_IIC_GET_PNDING, (u32)&pnd);
    if (pnd == TRUE) {
        pnd = 0x1f << 20;
        dev_ioctl(iic, IOCTL_IIC_CLR_PNDING, (u32)pnd);
        if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
            log_info_hexdump(slave_rx_buf_test, slave_rx_cnt);
        }
        rx_state = 0;
    }

    pnd = IIC_PND_TASK_DONE;
    dev_ioctl(iic, IOCTL_IIC_GET_PNDING, (u32)&pnd);
    if (pnd == TRUE) {
        if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
            dev_ioctl(iic, IOCTL_IIC_GET_SLAVE_TX_ACK, (u32)&ack);
            if (ack) {
                dev_ioctl(iic, IOCTL_IIC_TX_BYTE, slave_tx_buf_test[slave_tx_cnt++]);
            }
        } else if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
            rx_cnt = dev_ioctl(iic, IOCTL_IIC_RX_BYTE, (u32)&slave_rx_buf_test[slave_rx_cnt]);
            if (rx_cnt == IIC_SLAVE_RX_DATA) {
                ++slave_rx_cnt;
            } else if (rx_cnt == IIC_SLAVE_RX_PREPARE_TIMEOUT) {
                pnd = IIC_PND_TASK_DONE;
                dev_ioctl(iic, IOCTL_IIC_CLR_PNDING, (u32)pnd);
            }
            /* dev_ioctl(iic, IOCTL_IIC_SET_SLAVE_RX_PREPARE, 0); */
        } else {
            pnd = 0x1f << 20;
            dev_ioctl(iic, IOCTL_IIC_CLR_PNDING, (u32)pnd);
        }
    }
#endif

    dev_ioctl(iic, IOCTL_IIC_STOP, 0);
}

void hw_iic_slave_test(const char *dev_name)
{
    iic = dev_open(dev_name, NULL);
    if (!iic) {
        return;
    }

    for (int i = 0; i < sizeof(slave_tx_buf_test); ++i) {
        slave_tx_buf_test[i] = i;
    }

#if 1//ie test
#if SLAVE_NO_STRETCH_AUTO_TASK
    dev_ioctl(iic, IOCTL_IIC_TX_BYTE, (u32)slave_tx_buf_test[slave_tx_cnt]);
#endif
    dev_ioctl(iic, IOCTL_IIC_SET_IRQ_CALLBACK, (u32)hw_iic_isr_callback);

    while (1) {
        os_time_dly(10);
        if (slave_tx_cnt > 0) {
            printf("slave tx cnt : %d\n", slave_tx_cnt);
        }
        if (slave_rx_cnt > 0) {
            put_buf(slave_rx_buf_test, slave_rx_cnt);
        }
    }

    return;
#endif

    //polling test
#if 1
    int rx_cnt = 0;
    //rx test
    while (1) {
        rx_cnt = hw_iic_slave_polling_rx(iic, slave_rx_buf_test + slave_rx_cnt);
        slave_rx_cnt += rx_cnt;
        if (rx_cnt == 0 && slave_rx_cnt > 0) {
            put_buf(slave_rx_buf_test, slave_rx_cnt);
        }
        /* wdt_clear(); */
    }
#else
    int tx_cnt = 0;
    //tx test
    while (1) {
        tx_cnt = hw_iic_slave_polling_tx(iic, &slave_tx_buf_test[slave_tx_cnt]);
        if (tx_cnt > 0) {
            slave_tx_cnt += tx_cnt;
            log_info("iic slave tx ok");
        } else {
            if (slave_tx_cnt > 0) {
                printf("slave tx cnt : %d\n", slave_tx_cnt);
            }
            log_info("iic slave tx fail");
        }
        /* wdt_clear(); */
    }
#endif

    dev_close(iic);

    iic = NULL;
}
