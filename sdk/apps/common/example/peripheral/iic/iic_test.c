#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "device/iic.h"


#ifdef USE_IIC_TEST_DEMO

#define  IIC_WRCMD  0x42  //IIC写地址命令
#define  IIC_RDCMD  0x43  //IIC读地址命令

static int iic_write_one_byte(void *iic, unsigned char addr, unsigned char data)//返回值为0成功，非0失败
{
    int ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);//IIC锁定
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, IIC_WRCMD)) {//起始信号 + 写地址命令
        ret = -1;
        printf("iic write err in IOCTL_IIC_TX_BYTE_WITH_START_BIT \n");
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE, addr)) {//写地址
        ret = 0 - 1;
        printf("iic write err in IOCTL_IIC_TX_BYTE\n");
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, data)) {//写数据 + 结束信号
        ret = -1;
        printf("iic write err in IOCTL_IIC_TX_BYTE_WITH_STOP_BIT\n");
        goto exit;
    }
    delay(1000);

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);//IIC解锁
    return ret;
}

static int iic_read_one_byte(void *iic, unsigned char addr, unsigned char *data)//返回值为0成功，非0失败
{
    int ret = 0;

    dev_ioctl(iic, IOCTL_IIC_START, 0);//IIC锁定
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, IIC_WRCMD)) {//起始信号 + 读地址命令
        ret = -1;
        printf("iic write err in IOCTL_IIC_TX_BYTE_WITH_START_BIT\n");
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_STOP_BIT, addr)) {//读地址 + 结束信号
        ret = -1;
        printf("iic write err IOCTL_IIC_TX_WITH_STOP_BIT\n");
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IOCTL_IIC_TX_BYTE_WITH_START_BIT, IIC_RDCMD)) {//起始信号 + 读数据命令
        ret = -1;
        printf("iic write err IOCTL_IIC_TX_BYTE_WITH_START_BIT\n");
        goto exit;
    }
    delay(1000);

#if 1
    //接收没有ACK：IOCTL_IIC_RX_BYTE_WITH_STOP_BIT，读数据 + 结束信号
    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_STOP_BIT, (u32)data);//读数据 + 结束信号
#else
    //接收需要ACK，则命令为 IOCTL_IIC_RX_BYTE_WITH_ACK，如下：
    dev_ioctl(iic, IOCTL_IIC_RX_BYTE_WITH_ACK, (u32)data);//读数据 + ACK
    dev_ioctl(iic, IOCTL_IIC_TX_STOP_BIT, 0);//结束信号
#endif

exit:
    dev_ioctl(iic, IOCTL_IIC_STOP, 0);//IIC解锁
    return ret;
}


static void iic_test_task(void *arg)
{
    //1.打开iic0的iic设备
    void *iic_hdl = dev_open("iic0", NULL);
    if (!iic_hdl) {
        printf("iic open err \n");
        goto exit;
    }

    u8 addr = 0x28;
    u8 data = 0x03;
    u8 id_addr = 0xf0;
    u8 id = 0;

    os_time_dly(2);//延时2个os tick

    //2.读取ID
    iic_read_one_byte(iic_hdl, id_addr, &id);
    if (id != 0x9d) { //ID为设备的ID好
        printf("err id no match \n");
        goto exit;
    }
    printf("dev id match = 0x%x \n", id);

    //3.对应地址写对应数据
    iic_write_one_byte(iic_hdl, addr, data);

    //4.对应地址写对应数据
    data = 0;
    iic_read_one_byte(iic_hdl, addr, &data);
    if (data != 0x03) { //写进去之后读取值进行比较
        printf("read no match \n");
        goto exit;
    }
    printf("write read test ok\n");

exit:
    if (iic_hdl) { //关闭iic设备
        dev_close(iic_hdl);
        printf("dev close \n");
    }

    while (1) { //等待其他任务杀此任务
        os_time_dly(100);
    }
}

static int c_main_iic(void)
{
    os_task_create(iic_test_task, NULL, 12, 1000, 0, "iic_test_task");
    return 0;
}

late_initcall(c_main_iic);

#endif // USE_IIC_TEST_DEMO

