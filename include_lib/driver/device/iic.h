#ifndef DEVICE_IIC_H
#define DEVICE_IIC_H

#include "typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"
#include "system/task.h"
#include "asm/iic.h" //硬件iic




#define IIC_IOCTL_TX_START_BIT 				_IOW('I', 0,  0)
#define IIC_IOCTL_TX_WITH_START_BIT 		_IOW('I', 1,  1)
#define IIC_IOCTL_TX_STOP_BIT 				_IOW('I', 2,  1)
#define IIC_IOCTL_TX 						_IOW('I', 3,  8)
#define IIC_IOCTL_TX_WITH_STOP_BIT 			_IOW('I', 4,  9)
#define IIC_IOCTL_RX 						_IOR('I', 5,  8)
#define IIC_IOCTL_RX_WITH_STOP_BIT 			_IOR('I', 6,  9)
#define IIC_IOCTL_RX_WITH_NOACK 			_IOR('I', 7,  9)
#define IIC_IOCTL_RX_WITH_ACK 				_IOR('I', 8,  9)
#define IIC_IOCTL_RX_WITH_START_BIT			_IOR('I', 9,  9)

#define IIC_IOCTL_START 					_IOW('I', 10,  0)
#define IIC_IOCTL_STOP 						_IOW('I', 11,  0)
#define IIC_IOCTL_SET_NORMAT_RATE 			_IOW('I', 12,  0)


struct iic_device;



enum iic_device_type {
    IIC_TYPE_HW, 			//hardware iic
    IIC_TYPE_SW, 			//software iic
};

struct software_iic {
    u8 clk_pin;
    u8 dat_pin;
    u32 sw_iic_delay;
};


struct iic_platform_data {
    enum iic_device_type type;
    struct iic_device *p_iic_device;
    u32 data[0];
};

struct sw_iic_platform_data {
    struct iic_platform_data head;
    struct  software_iic iic;
};

struct hw_iic_platform_data {
    struct iic_platform_data head;
    struct hardware_iic iic;
};


#define SW_IIC_PLATFORM_DATA_BEGIN(data) \
	extern int sw_iic_ops_link(void);module_initcall(sw_iic_ops_link);\
	static struct iic_device _iic_device_##data;\
	static const struct sw_iic_platform_data data = { \
		.head = { \
			.type = IIC_TYPE_SW, \
			.p_iic_device = &_iic_device_##data,\
		}, \
		.iic = {



#define SW_IIC_PLATFORM_DATA_END() \
		}, \
	};





struct iic_operations {
    enum iic_device_type type;
    int (*open)(struct iic_device *);
    int (*read)(struct iic_device *, void *buf, int len);
    int (*write)(struct iic_device *, void *buf, int len);
    int (*ioctl)(struct iic_device *, int cmd, int arg);
    int (*close)(struct iic_device *);
};


struct iic_device {
    u8 id;
    u8 type;
    u8 nrate;
    u8 open_status;
    struct list_head entry;
    const void *hw;
    struct device dev;
    const struct iic_operations *ops;
    OS_MUTEX mutex;
    struct device *curr_device;
};


#define REGISTER_IIC_DEVICE(name) \
	int name##_link(void){return 0;}\
	static const struct iic_operations name SEC_USED(.iic)

extern const struct iic_operations iic_device_begin[], iic_device_end[];

#define list_for_each_iic_device_ops(p) \
	for (p=iic_device_begin; p<iic_device_end; p++)



extern const struct device_operations iic_dev_ops;



#endif

