#ifndef _DP83848_H_
#define _DP83848_H_

#include "generic/typedef.h"
#include "eth/phy_state.h"
#define DP8_PHYAD 0x01         //DP83848 PHY address (define by hardware)


/******************  function prototypes **********************/
extern void dp83848_enable_loopback(u8 phyad);
extern int dp83848_InitPhy(u8 phyad, u8 rii_rmii_mode, LINK_SPEED_MODE link_speed_mode);

int dp83848_IsPhyConnected(u8 phyad);
#endif  //_DP83848_H_


