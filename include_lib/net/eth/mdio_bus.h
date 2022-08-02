#ifndef  __RMII_MII_IO_CONFIG_H__
#define  __RMII_MII_IO_CONFIG_H__

#include "device/gpio.h"

struct software_rmii {
    u8 phy_addr; //0-32
    u8 clk_pin;
    u8 dat_pin;
};


void eth_mii_write(struct software_rmii *bus, u16 regnum, u16 data);
u16 eth_mii_read(struct software_rmii *rmii_bus, u16 regnum);

#endif  /*RMII_MII_IO_CONFIG_H*/
