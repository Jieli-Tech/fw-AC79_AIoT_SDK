#ifndef _RTL8201_H_
#define _RTL8201_H_

#include "generic/typedef.h"
#include "eth/eth_phy.h"

#define RTL8201_PHYAD 0x01         //RTL8201 PHY address (define by hardware

/******************  function prototypes **********************/
void rtl8201_enable_loopback(struct eth_platform_data *);
int rtl8201_InitPhy(struct eth_platform_data *);
void rtl8201_GetLinkSpeed(struct eth_platform_data *);
int rtl8201_IsPhyConnected(struct eth_platform_data *);

/******************  function prototypes **********************/
#endif //_RTL8201_H_

