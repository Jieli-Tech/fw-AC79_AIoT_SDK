#ifndef ADAPTER_INTERRUPT_H
#define ADAPTER_INTERRUPT_H

#include "generic/typedef.h"
#include "asm/hwi.h"

#define IRQ_FUNCTION_PROTOTYPE(idx, hdl) \
    __attribute__((interrupt("")))

#define IRQ_REGISTER(idx, hdl, prio) \
    request_irq(idx, prio, hdl, 0)

#define IRQ_RELEASE(idx) \
    // irq_handler_unregister(idx)

bool irq_get_status(u32 index);

#endif

