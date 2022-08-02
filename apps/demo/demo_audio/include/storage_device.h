#ifndef STORAGE_DEVICE_H
#define STORAGE_DEVICE_H

extern int storage_device_ready(void);

extern int sdcard_storage_device_ready(const char *sd_name);

extern int sdcard_storage_device_format(const char *sd_name);

extern int udisk_storage_device_ready(int id);

extern int udisk_storage_device_format(int id);

#endif

