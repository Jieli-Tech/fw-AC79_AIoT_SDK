#include "mDNSUNP.h"
#include "lwip.h"

#define APPLICATION_LISTENING_PORT 0

struct ifi_info *get_ifi_info(int family, int doaliases)
{
    struct ifi_info *ifi;
    int index = 2;
    struct sockaddr_in sinptr = {0};
    char ipaddr[20];
    char netmask[4];
    struct lan_setting *lan = NULL;

    ifi = (struct ifi_info *)calloc(1, sizeof(struct ifi_info));
    if (!ifi) {
        return NULL;
    }

    Get_IPAddress(1, ipaddr);
    sinptr.sin_family = AF_INET;
    sinptr.sin_len = sizeof(struct sockaddr_in);
    sinptr.sin_addr.s_addr = inet_addr(ipaddr);
    sinptr.sin_port = htons(APPLICATION_LISTENING_PORT);

    ifi->ifi_addr = (struct sockaddr *)calloc(1, sizeof(struct sockaddr_in));
    if (ifi->ifi_addr == NULL) {
        goto gotError;
    }
    memcpy(ifi->ifi_addr, &sinptr, sizeof(struct sockaddr_in));

#if  1
    ifi->ifi_netmask = (struct sockaddr *)calloc(1, sizeof(struct sockaddr_in));
    if (ifi->ifi_netmask == NULL) {
        goto gotError;
    }

    lan = net_get_lan_info();
    netmask[0] = lan->WIRELESS_NETMASK0;
    netmask[1] = lan->WIRELESS_NETMASK1;
    netmask[2] = lan->WIRELESS_NETMASK2;
    netmask[3] = lan->WIRELESS_NETMASK3;
    sinptr.sin_addr.s_addr = inet_addr(netmask);
    memcpy(ifi->ifi_netmask, &sinptr, sizeof(struct sockaddr_in));
#endif

    ifi->ifi_index = index;

    memcpy(ifi->ifi_name, "AC52XX", IFI_NAME);
    ifi->ifi_name[IFI_NAME - 1] = '\0';

    return (ifi);

gotError:
    free_ifi_info(ifi);
    return NULL;
}

void free_ifi_info(struct ifi_info *ifi)
{
    if (!ifi) {
        return;
    }

    if (ifi->ifi_addr != NULL) {
        free(ifi->ifi_addr);
    }
    if (ifi->ifi_netmask != NULL) {
        free(ifi->ifi_netmask);
    }
    free(ifi);
}


