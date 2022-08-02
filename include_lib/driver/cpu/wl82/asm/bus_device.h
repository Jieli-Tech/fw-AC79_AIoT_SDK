#ifndef CPU_BUS_DEVICE_H
#define CPU_BUS_DEVICE_H

#include "typedef.h"


struct cpu_bus_device {
    const char *name;
    int (*bandwidth)();
};

#define REGISTER_CPU_BUS_DEVICE(dev) \
    const struct cpu_bus_device dev SEC_USED(.bus_device)



















#endif

