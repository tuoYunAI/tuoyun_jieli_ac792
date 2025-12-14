#ifndef __STORAGE_DEVICE_H__
#define __STORAGE_DEVICE_H__

int storage_device_ready(void);

int sdcard_storage_device_ready(const char *sd_name);

int sdcard_storage_device_format(const char *sd_name);

int udisk_storage_device_ready(int id);

int udisk_storage_device_format(int id);

#endif

