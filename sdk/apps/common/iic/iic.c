#include "device/iic.h"

static struct list_head head = LIST_HEAD_INIT(head);

static int iic_open(const char *name, struct device **device, void *arg)
{
    iic_device_t *p;
    int id = name[3] - '0';

    list_for_each_entry(p, &head, entry) {
        if (p->id == id) {
            *device = &p->dev;
            (*device)->private_data = p;
            if (!p->open_status) {
                if (p->ops->open) {
                    p->ops->open(p);
                }
                os_mutex_create(&p->mutex);
                p->open_status = 1;
            }
            return 0;
        }
    }

    return -EINVAL;
}

static int iic_read(struct device *device, void *buf, unsigned int len, u32 addr)
{
    iic_device_t *iic = (iic_device_t *)device->private_data;
    int ret;

    os_mutex_pend(&iic->mutex, 0);
    ret = iic->ops->read(iic, buf, len);
    os_mutex_post(&iic->mutex);

    return ret;
}

static int iic_write(struct device *device, void *buf, unsigned int len, u32 addr)
{
    iic_device_t *iic = (iic_device_t *)device->private_data;
    int ret;

    os_mutex_pend(&iic->mutex, 0);
    ret = iic->ops->write(iic, buf, len);
    os_mutex_post(&iic->mutex);

    return ret;
}

static int iic_ioctl(struct device *device, unsigned int cmd, unsigned int arg)
{
    int ret = 0;
    iic_device_t *iic = (iic_device_t *)device->private_data;

    if (iic->ops->ioctl) {
        if (cmd == IOCTL_IIC_START) {
            if (!(cpu_in_irq() || cpu_irq_disabled())) {
                os_mutex_pend(&iic->mutex, 0);
            }
        }
        ret = iic->ops->ioctl(iic, cmd, arg);
        if (cmd == IOCTL_IIC_STOP) {
            if (!(cpu_in_irq() || cpu_irq_disabled())) {
                os_mutex_post(&iic->mutex);
            }
        }
        return ret;
    }

    return 0;
}

static int iic_init(const struct dev_node *node, void *_data)
{
    int id;
    iic_device_t *iic;
    const struct iic_operations *p;
    const struct iic_platform_data *data = (const struct iic_platform_data *)_data;

    id = node->name[3] - '0';

    list_for_each_iic_device_ops(p) {
        if (p->type == data->type) {
            iic = data->p_iic_device;//(iic_device_t *)malloc(sizeof(*iic));
            if (!iic) {
                return -ENOMEM;
            }
            iic->id = id;
            iic->hw = &data->data;
            iic->ops = p;
            iic->open_status = 0;
            list_add(&iic->entry, &head);
            return 0;
        }
    }

    return -EINVAL;
}

static int iic_close(struct device *device)
{
    iic_device_t *iic = (iic_device_t *)device->private_data;
    int ret = 0;

    os_mutex_pend(&iic->mutex, 0);
    if (iic->ops->close) {
        ret = iic->ops->close(iic);
    }
    os_mutex_del(&iic->mutex, OS_DEL_ALWAYS);
    iic->open_status = 0;

    return ret;
}

const struct device_operations iic_dev_ops = {
    .init 	= iic_init,
    .open 	= iic_open,
    .read 	= iic_read,
    .write	= iic_write,
    .ioctl 	= iic_ioctl,
    .close 	= iic_close,
};

