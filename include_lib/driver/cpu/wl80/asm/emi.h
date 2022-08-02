#ifndef _EMI_H_
#define _EMI_H_

#include "device.h"
#include "generic/typedef.h"
#include "generic/list.h"
#include "device/gpio.h"
#include "generic/atomic.h"
#include "generic/ioctl.h"
#include "device/ioctl_cmds.h"
#include "os/os_api.h"


#define EMI_8BITS_MODE		0
#define EMI_16BITS_MODE		1
#define EMI_32BITS_MODE		2


#define EMI_RISING_COLT		0
#define EMI_FALLING_COLT	1


#define EMI_MAGIC                          'E'
#define EMI_SET_ISR_CB                   _IOW(EMI_MAGIC,1,u32)
#define EMI_USE_SEND_SEM                 _IO(EMI_MAGIC,2)
#define EMI_SET_WRITE_BLOCK              _IOW(EMI_MAGIC,3,u32)

enum EMI_BAUD {
    EMI_BAUD_DIV1 = 0,
    EMI_BAUD_DIV2 = 1,
    EMI_BAUD_DIV3 = 2,
    EMI_BAUD_DIV4 = 3,
    EMI_BAUD_DIV5 = 4,
    EMI_BAUD_DIV6 = 5,
    EMI_BAUD_DIV7 = 6,
    EMI_BAUD_DIV8 = 7,
    EMI_BAUD_DIV10 = 9,
    EMI_BAUD_DIV16 = 15,
    EMI_BAUD_DIV32 = 31,
    EMI_BAUD_DIV64 = 63,
    EMI_BAUD_DIV128 = 127,
    EMI_BAUD_DIV256 = 255,
};

struct emi_platform_data {
    u8 bits_mode;
    u8 baudrate;
    u8 colection;
};

struct emi_device {
    char *name;
    struct device dev;
    void (*emi_isr_cb)(void *priv);
    OS_SEM send_sem;
    char use_send_sem;
    char write_block;
    volatile char wait_send_flg;
    void *priv;
};

#define REGISTER_EMI_DEVICE(pdev) \
	static struct emi_device pdev = {\
		.name = "emi",\
	}

extern const struct device_operations emi_dev_ops;

#endif

