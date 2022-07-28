#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include "uip.h"
#include "udp_demo.h"
#include "dhcpc.h"
#include "dhcps.h"

void udp_send_data(char *data)
{
    DEBUG_PRINTF("udp_send_data");//打印数据

#ifdef USE_UDP_SERVER
    memcpy(udp_server_databuf, data, strlen(data));
    udp_server_sta |= 1 << 5; //有数据需要发送
#endif

#ifdef USE_UDP_CLIENT
    memcpy(udp_client_databuf, data, strlen(data));
    udp_client_sta |= 1 << 5; //有数据需要发送
#endif

}
void udp_demo_appcall(void)
{
    DEBUG_PRINTF("udp demo appcall\r\n");

    switch (uip_udp_conn->lport) {

    /* case HTONS(67): */
    /* DEBUG_PRINTF("dhcpc_appcall 67\r\n"); */
    /* dhcps_appcall(); */
    /* break; */

    /* case HTONS(68): */
    /* DEBUG_PRINTF("dhcpc_appcall 68\r\n"); */
    /* dhcpc_appcall(); */
    /* break; */

    case HTONS(UDP_SERVER_PORT):
        DEBUG_PRINTF("udp_server_demo_appcall 1800\r\n");
        udp_server_demo_appcall();
        break;

    case HTONS(UDP_CLIENT_PORT):
        DEBUG_PRINTF("udp_client_demo_appcall 1900\r\n");
        udp_client_demo_appcall();
        break;

    default: {
        DEBUG_PRINTF("udp default appcall htons=%d\r\n", uip_udp_conn->lport);
    }
    break;
    }
}

