#ifndef CHRDEV_H
#define CHRDEV_H


#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/atomic.h"
#include "device/ioctl_cmds.h"

///  \cond DO_NOT_DOCUMENT
struct dev_node;
struct device;


struct device_operations {
    bool (*online)(const struct dev_node *node);
    int (*init)(const struct dev_node *node, void *);
    int (*open)(const char *name, struct device **device, void *arg);
    int (*read)(struct device *device, void *buf, u32 len, u32);
    int (*write)(struct device *device, void *buf, u32 len, u32);
    int (*seek)(struct device *device, u32 offset, int orig);
    int (*ioctl)(struct device *device, u32 cmd, u32 arg);
    int (*close)(struct device *device);
};

struct dev_node {
    const char *name;
    const struct device_operations *ops;
    void *priv_data;
};


struct device {
    atomic_t ref;
    void *private_data;
    const struct device_operations *ops;
    void *platform_data;
    void *driver_data;
};


#define REGISTER_DEVICE(node) \
    const struct dev_node node SEC_USED(.device)

#define REGISTER_DEVICES(node) \
    const struct dev_node node[] SEC_USED(.device)


int devices_init();

bool dev_online(const char *name);
/// \endcond

/**
 * @brief dev_open：用于打开一个设备
 *
 * @param name 设备名称
 * @param arg 控制参数，一般为NULL
 */
void *dev_open(const char *name, void *arg);

/**
 * @brief dev_read：用于设备数据的接收。
 *
 * @param device 设备句柄
 * @param buf 要读入的 buffer 缓冲区
 * @param len 要读入的长度
 */
int dev_read(void *device, void *buf, u32 len);

/**
 * @brief dev_write：用于设备数据的发送。
 *
 * @param device 设备句柄
 * @param buf 要写入的 buffer 缓冲区
 * @param len 要写入的长度
 */
int dev_write(void *device, void *buf, u32 len);

///  \cond DO_NOT_DOCUMENT
int dev_seek(void *device, u32 offset, int orig);
/// \endcond

/**
 * @brief dev_ioctl：用于对设备进行控制和参数的修改
 *
 * @param device 设备句柄
 * @param cmd 设备控制命令
 * @param arg 控制参数
 */
int dev_ioctl(void *device, int cmd, u32 arg);

/**
 * @brief dev_close：用于关闭一个设备
 *
 * @param device 设备句柄
 */
int dev_close(void *device);

///  \cond DO_NOT_DOCUMENT
int dev_bulk_read(void *_device, void *buf, u32 offset, u32 len);

int dev_bulk_write(void *_device, void *buf, u32 offset, u32 len);
/// \endcond

#endif


