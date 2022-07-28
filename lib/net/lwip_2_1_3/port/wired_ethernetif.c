/* Includes ------------------------------------------------------------------*/

#include "lwip/err.h"
#include "etharp.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip.h"
#include "eth/RTL8201.h"
#include "eth/ethmac.h"
#include <string.h>
#include "printf.h"
#include "os/os_compat.h"
#include "os/os_api.h"
/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

extern void wired_ethernetif_input(struct netif *netif);
//6字节的目的MAC＋6字节的源MAC＋2字节的帧类型＋最大数据包(1500)
//unsigned char Rx_Data_Buf[1514];
//unsigned char Tx_Data_Buf[1514];
int __attribute__((weak)) ethmac_setup(struct eth_platform_data *__data)
{
    return 0;
}
u8 *__attribute__((weak)) oeth_get_txaddr(void)
{
    return NULL;
}

int __attribute__((weak)) oeth_get_rxpkt_addr_len(struct eth_platform_data *__data, struct oeth_data *_oeth_data)
{
    return -1;
}
void __attribute__((weak)) oeth_tx_packet(u16 length)
{

}
struct eth_platform_data *__attribute__((weak)) get_platform_data()
{
    return NULL;
}
static void eth_rx_task(void *p_arg)
{
    while (1) {
        /* Read a received packet from the Ethernet buffers and send it to the lwIP for handling */
        wired_ethernetif_input((struct netif *)p_arg);

    }
}

static int create_eth_rx_task(void *p_arg, u8 prio)
{

    return os_task_create(eth_rx_task, (void *)p_arg, prio, 0x2000, 0, "eth_rx_task_n");
}

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
//struct ethernetif {
//  struct eth_addr *ethaddr;
//  /* Add whatever per-interface state that is needed here. */
//};
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
wired_low_level_init(struct netif *netif)
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    struct eth_platform_data *__data = get_platform_data();
    if (__data == NULL) {
        printf("lwip config mac addr error %s %d\n", __func__, __LINE__);
        return ;
    }

    /* set MAC hardware address */
    memcpy(netif->hwaddr, __data->mac_addr, 6);
    printf("wire mac addr -> %x:%x:%x:%x:%x:%x\n\n"
           , netif->hwaddr[0]
           , netif->hwaddr[1]
           , netif->hwaddr[2]
           , netif->hwaddr[3]
           , netif->hwaddr[4]
           , netif->hwaddr[5]);

    /* maximum transfer unit */
    netif->mtu = NETIF_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;

    /* Do whatever else is needed to initialize interface. */

    /* Initialise the EMAC.  This routine contains code that polls status bits.
    If the Ethernet cable is not plugged in then this can take a considerable
    time.  To prevent this starving lower priority tasks of processing time we
    lower our priority prior to the call, then raise it back again once the
    initialisation is complete. */


    /* ethmac_setup(netif->hwaddr, RMII_MODE, 0, 0); */
    create_eth_rx_task(netif, 2);

}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t
wired_low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    unsigned int i;
    u8 *ptxpkt;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    /* Parameter not used. */

    //获取txbd指向的内存地址
    ptxpkt = oeth_get_txaddr();

    for (i = 0, q = p; q != NULL; q = q->next) {
        /* dma1_cpy(&ptxpkt[i], (u8_t*)q->payload, q->len); */
        memcpy(&ptxpkt[i], (u8_t *)q->payload, q->len);
        i = i + q->len;
    }
    /* printf("low_level_output len = %d \r\n",i); */
    /* put_buf(ptxpkt,i); */
    oeth_tx_packet(i);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
wired_low_level_input(struct netif *netif)
{
    struct pbuf *q, *p = NULL;
    u16 i;
    int ret = 0;
    struct oeth_data _oeth_data = {0};
    struct eth_platform_data *data = get_platform_data();
    /* Obtain the size of the packet and put it into the "len" variable. 并且调整下一个接收oeth_rxbd */
    ret = oeth_get_rxpkt_addr_len(data, &_oeth_data);

    if (ret < 0) {
        return NULL;
    }
    /* printf("low_level_input len = %d \r\n",len); */
    /* put_buf(prxpkt,len); */
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, _oeth_data.data_len, PBUF_POOL);

    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

        /* We iterate over the pbuf chain until we have read the entire packet into the pbuf. */
        for (i = 0, q = p; q != NULL; q = q->next) {
            /* Read enough bytes to fill this pbuf in the chain. The
             * available data in the pbuf is given by the q->len
             * variable.
             * This does not necessarily have to be a memcpy, you can also preallocate
             * pbufs for a DMA-enabled MAC and after receiving truncate it to the
             * actually received size. In this case, ensure the tot_len member of the
             * pbuf is the sum of the chained pbuf len members.
             */

//             dma0_cpy((void *)q->payload, (void *)&prxpkt[i], q->len);
            memcpy((void *)q->payload, (void *)&_oeth_data.data[i], q->len);
            i += q->len;
        }

        //释放当前oeth_rxbd
        oeth_clean_current_bd(data);
#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);
    } else {
        puts("pbuf_alloc fail!\n");
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

    return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void
wired_ethernetif_input(struct netif *netif)
{
    struct pbuf *p;
    int err;
    /* move received packet into a new pbuf */


    p = wired_low_level_input(netif);

    /* no packet could be read, silently ignore this */
    if (p == NULL) {
        return;
    }

    /* full packet send to tcpip_thread to process */
    err = netif->input(p, netif);

    if (err != ERR_OK) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
        p = NULL;
    }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
wired_ethernetif_init(struct netif *netif)
{
//  struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

//  ethernetif = malloc(sizeof(struct ethernetif));
//  if (ethernetif == NULL) {
//    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
//    return ERR_MEM;
//  }

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
#define LINK_SPEED_OF_YOUR_NETIF_IN_BPS (100*1000000)
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

//  netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = wired_low_level_output;

//  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    /* initialize the hardware */
    wired_low_level_init(netif);
    return ERR_OK;
}

