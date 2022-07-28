#include "dhcpc.h"
#include "dhcps.h"
#include "dhcp_appcall.h"

void dhcp_appcall(void)
{
    switch (uip_udp_conn->lport) {
    case HTONS(67):
        dhcps_appcall();
        break;

    case HTONS(68):
        dhcpc_appcall();
        break;

    default:
        break;
    }
}

