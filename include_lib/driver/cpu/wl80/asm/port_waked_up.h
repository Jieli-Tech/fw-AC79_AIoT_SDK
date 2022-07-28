#ifndef  __PORT_WAKED_UP_H__
#define  __PORT_WAKED_UP_H__

#include "typedef.h"



typedef enum {
    EVENT_IO_0 = 0,
    EVENT_IO_1,
    EVENT_PA0,
    EVENT_PA7,
    EVENT_PA8,
    EVENT_UT0_RX,
    EVENT_UT1_RX,
    EVENT_UT2_RX,
    EVENT_PB1,
    EVENT_PB6,
    EVENT_PC0,
    EVENT_PC1,
    EVENT_PH0,
    EVENT_PH7,
    EVENT_SDC0_DAT1,
    EVENT_SDC1_DAT1,
} PORT_EVENT_E;

typedef enum {
    EDGE_POSITIVE = 0,
    EDGE_NEGATIVE,
} PORT_EDGE_E;


void *port_wakeup_reg(PORT_EVENT_E event, unsigned int gpio, PORT_EDGE_E edge, void (*handler)(void));
void port_wakeup_unreg(void *hdl);


#endif  /*PORT_WAKED_UP_H*/
