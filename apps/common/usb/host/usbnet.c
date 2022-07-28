#include "app_config.h"
#include "usb_host.h"
#include "usbnet.h"
#include "cdc_defs.h"
#include "device_drive.h"
#include "host/usb_ctrl_transfer.h"

#if TCFG_HOST_WIRELESS_ENABLE

/* #define __LOG_LEVEL  __LOG_DEBUG */
#define netif_dbg(fmt,arg...)

#define log_info(x, ...)     printf("[USBNET] " x " ", ## __VA_ARGS__)

#define	EL3RST		(40)	/* Level 3 reset */
#define ETH_ALEN    (6)
/* #define	ETIMEDOUT	(238)	[> Connection timed out <] */

static u32 usb_recv_len = 0;
static u8 *usb_recv_buf = NULL;
static USBNET_RX_COMPLETE_CB usbnet_rx_complete_cb = NULL;

static void keepalive_period_task(void *priv);

static int set_power(struct usb_host_device *host_dev, u32 value)
{
    return DEV_ERR_NONE;
}

static int get_power(struct usb_host_device *host_dev, u32 value)
{
    return DEV_ERR_NONE;
}

static struct wireless_device_t wireless_dev = {0};

static const struct interface_ctrl wireless_ctrl = {
    .interface_class = USB_CLASS_WIRELESS_CONTROLLER,
    .set_power = set_power,
    .get_power = get_power,
    .ioctl = NULL,
};

static const struct usb_interface_info wireless_inf = {
    .ctrl = (struct interface_ctrl *) &wireless_ctrl,
    .dev.wireless = &wireless_dev,
};


void __attribute__((weak)) copy_usbnet_mac_addr(u8 *mac)
{

}


void __attribute__((weak)) copy_usbnet_endpoint(struct usb_endpoint_descriptor *endpoint, u8 type, u32 i, u32 len)
{
    return;
}


int is_wireless_rndis(struct usb_interface_descriptor *desc)
{
    return (desc->bInterfaceClass == USB_CLASS_WIRELESS_CONTROLLER &&
            desc->bInterfaceSubClass == 1 &&
            desc->bInterfaceProtocol == 3);
}


u32 usbnet_host_bulk_only_send(u8 *buf, u32 len)
{
    u32 ret = usb_h_bulk_write(wireless_dev.usb_id, wireless_dev.host_epout, wireless_dev.txmaxp, wireless_dev.epout, buf, len);
    /* log_info(">>>%s, ret = %d\n", __FUNCTION__, ret); */
    /* put_buf(buf, len); */
    return ret;
}


void usbnet_set_rx_complete_cb(USBNET_RX_COMPLETE_CB cb)
{
    usbnet_rx_complete_cb = cb;
}


void usbnet_host_bulk_only_receive_int(void *buf, u32 len)
{
    usb_recv_buf = buf;
    usb_recv_len = len;
}


static void usbnet_rx_isr(struct usb_interface_info *usb_if, u32 ep)
{
    u32 rx_len = 0;
    struct usb_host_device *host_dev = usb_if->dev.wireless->parent;
    usb_dev usb_id = host_device2id(host_dev);
    u32 target_ep = usb_if->dev.wireless->epin;

    if (!usb_recv_buf || !usb_recv_len) {
        return;
    }

    rx_len = usb_h_ep_read_async(usb_id, ep, target_ep, usb_recv_buf, usb_recv_len, USB_ENDPOINT_XFER_INT, 0);
    if (rx_len && usbnet_rx_complete_cb) {
        /* printf("------------------111----------------\n"); */
        /* printf("rx_len = %d, usb_recv_len = %d", rx_len, usb_recv_len); */
        /* put_buf(usb_recv_buf, rx_len); */
        /* printf("------------------222----------------\n"); */
        usbnet_rx_complete_cb(rx_len);
    }

    usb_h_ep_read_async(usb_id, ep, target_ep, usb_recv_buf, usb_recv_len, USB_ENDPOINT_XFER_INT, 1);
}


s32 usbnet_generic_cdc_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    int len = 0;
    u8 loop = 1;
    u8 rx_interval = 0;
    const usb_dev usb_id = host_device2id(host_dev);
    host_dev->interface_info[interface_num] = &wireless_inf;
    struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
    wireless_inf.dev.wireless->parent = host_dev;
    struct usb_endpoint_descriptor *end_desc = NULL;

    pBuf += sizeof(struct usb_interface_descriptor);
    const u8 *buf = pBuf;
    wireless_dev.usb_id = usb_id;

    puts("usb_cdc_parser \n");

    while (loop) {
        if (buf[1] != USB_DT_CS_INTERFACE) {
            if (buf[1] == USB_DT_INTERFACE_ASSOCIATION) { //下一个设备
                loop = 0;
                break;
            }
            goto next_desc;
        }
        /* use bDescriptorSubType to identify the CDC descriptors.
         * We expect devices with CDC header and union descriptors.
         * For CDC Ethernet we need the ethernet descriptor.
         * For RNDIS, ignore two (pointless) CDC modem descriptors
         * in favor of a complicated OID-based RPC scheme doing what
         * CDC Ethernet achieves with a simple descriptor.
         */
        switch (buf[2]) {
        case USB_CDC_HEADER_TYPE:
            break;

        case USB_CDC_ACM_TYPE:
            /* paranoia:  disambiguate a "real" vendor-specific
             * modem interface from an RNDIS non-modem.
             */
            break;

        case USB_CDC_UNION_TYPE:
            len += buf [0];	/* bLength */
            buf += buf [0];
            if (buf[1] == USB_DT_ENDPOINT) {
                len += USB_DT_ENDPOINT_SIZE;
                len += USB_DT_INTERFACE_SIZE;

                for (char endnum = 0; endnum < 2; endnum++) {
                    end_desc = (struct usb_endpoint_descriptor *)(pBuf + len);

                    if (end_desc->bDescriptorType != USB_DT_ENDPOINT ||
                        end_desc->bLength < USB_DT_ENDPOINT_SIZE) {
                        return -USB_DT_ENDPOINT;
                    }
                    len += USB_DT_ENDPOINT_SIZE;

                    if ((end_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
                        if (end_desc->bEndpointAddress & USB_DIR_IN) {
                            wireless_dev.host_epin = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_BULK);
                            wireless_dev.epin   = end_desc->bEndpointAddress & 0x0f;
                            wireless_dev.rxmaxp = end_desc->wMaxPacketSize;
                            rx_interval = end_desc->bInterval;
                            printf("^^^IN : end_desc->wMaxPacketSize = %d, end_desc->bInterval = %d\n", end_desc->wMaxPacketSize, end_desc->bInterval);
                        } else {
                            wireless_dev.host_epout = usb_get_ep_num(usb_id, USB_DIR_OUT, USB_ENDPOINT_XFER_BULK);
                            wireless_dev.epout  = end_desc->bEndpointAddress & 0x0f;
                            wireless_dev.txmaxp = end_desc->wMaxPacketSize;
                            printf("^^^OUT : end_desc->wMaxPacketSize = %d\n", end_desc->wMaxPacketSize);
                        }
                    }
                }

                usb_h_set_ep_isr(host_dev, wireless_dev.host_epin | USB_DIR_IN, usbnet_rx_isr, &wireless_inf);
                if (!wireless_dev.epin_buf) {
                    wireless_dev.epin_buf = usb_h_alloc_ep_buffer(usb_id, wireless_dev.host_epin | USB_DIR_IN, 64 * 2);
                }
                usb_h_ep_config(usb_id, wireless_dev.host_epin | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 1, rx_interval, wireless_dev.epin_buf, 64);

                if (!wireless_dev.epout_buf) {
                    wireless_dev.epout_buf = usb_h_alloc_ep_buffer(usb_id, wireless_dev.host_epout | USB_DIR_OUT, 64);
                }
                usb_h_ep_config(usb_id, wireless_dev.host_epout | USB_DIR_OUT, USB_ENDPOINT_XFER_BULK, 0, 0, wireless_dev.epout_buf, 64);

                usb_h_ep_read_async(usb_id, wireless_dev.host_epin, wireless_dev.epin, NULL, 0, USB_ENDPOINT_XFER_BULK, 1);
            }
            return len;
            break;

        case USB_CDC_ETHERNET_TYPE:
            break;

        case USB_CDC_MDLM_TYPE:
            break;

        case USB_CDC_MDLM_DETAIL_TYPE:
            break;
        }
next_desc:
        len += buf[0];	/* bLength */
        buf += buf[0];
    }
    return len;
}


/*
 * RPC done RNDIS-style.  Caller guarantees:
 * -rndis_command message is properly byteswapped
 * - there's no other request pending
 * - buf can hold up to 1KB response (required by RNDIS spec)
 * On return, the first few entries are already byteswapped.
 *
 * Call context is likely probe(), before interface name is known,
 * which is why we won't try to use it in the diagnostics.
 */
int rndis_command(struct usb_host_device *device, struct rndis_msg_hdr *buf, int buflen)
{

    int			master_ifnum;
    int			retval;
    int			partial;
    unsigned		count;
    __le32			rsp;
    static u32			xid = 0;
    u32			 msg_len, request_id;

    /* REVISIT when this gets called from contexts other than probe() or
     * disconnect(): either serialize, or dispatch responses on xid
     */

    /* Issue the request; xid is unique, don't bother byteswapping it */
    if (likely(buf->msg_type != RNDIS_MSG_HALT &&
               buf->msg_type != RNDIS_MSG_RESET)) {
        xid++;
        if (!xid) {
            xid++;
        }
        buf->request_id = (__le32) xid;
    }
    master_ifnum = 0;//info->control->cur_altsetting->desc.bInterfaceNumber;//控制接口描述符
    retval = usb_control_msg(device,
                             USB_CDC_SEND_ENCAPSULATED_COMMAND,//0x00
                             USB_TYPE_CLASS | USB_RECIP_INTERFACE,//0x21
                             0, master_ifnum,
                             buf, le32_to_cpu(buf->msg_len)
                            );
    if (unlikely(retval < 0 || xid == 0)) {
        return retval;
    }
    /* Poll the control channel; the request probably completed immediately */
    rsp = buf->msg_type | RNDIS_MSG_COMPLETION;
    for (count = 0; count < 10; count++) {
        memset(buf, 0, CONTROL_BUFFER_SIZE);
        retval = usb_control_msg(device,
                                 USB_CDC_GET_ENCAPSULATED_RESPONSE,
                                 USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                                 0, master_ifnum,
                                 buf, buflen);


        if (likely(retval == 0)) {
            msg_len = le32_to_cpu(buf->msg_len);
            request_id = (u32) buf->request_id;
            if (likely(buf->msg_type == rsp)) {
                if (likely(request_id == xid)) {
                    /* log_info( */
                    /*     "rndis reply status %08x\n", */
                    /*     le32_to_cpu(buf->status)); */
                    if (unlikely(rsp == RNDIS_MSG_RESET_C)) {
                        return 0;
                    } else if (unlikely(rsp == RNDIS_MSG_KEEPALIVE_C)) {
                        log_info("RNDIS_MSG_KEEPALIVE_C\n");
                        return 0;
                    }

                    if (likely(RNDIS_STATUS_SUCCESS == buf->status)) {
                        return 0;
                    }

                    return -EL3RST;
                }
                log_info(
                    "rndis reply id %d expected %d\n",
                    request_id, xid);
                /* then likely retry */
            } else switch (buf->msg_type) {
                case RNDIS_MSG_INDICATE:	/* fault/event */
                    //rndis_msg_indicate(dev, (void *)buf, buflen);
                    log_info("RNDIS_MSG_INDICATE\n");
                    /* put_buf(buf, buflen); */
                    break;
                case RNDIS_MSG_KEEPALIVE: {	/* ping */
                    struct rndis_keepalive_c *msg = (void *)buf;

                    msg->msg_type = RNDIS_MSG_KEEPALIVE_C;
                    msg->msg_len = cpu_to_le32(sizeof * msg);
                    msg->status = RNDIS_STATUS_SUCCESS;
                    retval = usb_control_msg(device,
                                             USB_CDC_SEND_ENCAPSULATED_COMMAND,
                                             USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                                             0, master_ifnum,
                                             msg, sizeof * msg
                                            );
                    if (unlikely(retval < 0))
                        log_info(
                            "rndis keepalive err %d\n",
                            retval);
                }
                break;
                default:
                    log_info(
                        "unexpected rndis msg %08x len %d\n",
                        le32_to_cpu(buf->msg_type), msg_len);
                }
        } else {
            /* device probably issued a protocol stall; ignore */
            log_info(
                "rndis response error, code %d\n", retval);
        }
        msleep(20);
    }
    log_info("rndis response timeout\n");
    return -ETIMEDOUT;
}


/*
 * rndis_query:
 *
 * Performs a query for @oid along with 0 or more bytes of payload as
 * specified by @in_len. If @reply_len is not set to -1 then the reply
 * length is checked against this value, resulting in an error if it
 * doesn't match.
 *
 * NOTE: Adding a payload exactly or greater than the size of the expected
 * response payload is an evident requirement MSFT added for ActiveSync.
 *
 * The only exception is for OIDs that return a variably sized response,
 * in which case no payload should be added.  This undocumented (and
 * nonsensical!) issue was found by sniffing protocol requests from the
 * ActiveSync 4.1 Windows driver.
 */
static int rndis_query(struct usb_host_device *device,
                       void *buf, __le32 oid, u32 in_len,
                       void **reply, int *reply_len)
{
    int retval;
    union {
        void			*buf;
        struct rndis_msg_hdr	*header;
        struct rndis_query	*get;
        struct rndis_query_c	*get_c;
    } u;
    u32 off, len;

    u.buf = buf;

    memset(u.get, 0, sizeof * u.get + in_len);
    u.get->msg_type = RNDIS_MSG_QUERY;
    u.get->msg_len = cpu_to_le32(sizeof * u.get + in_len);
    u.get->oid = oid;
    u.get->len = cpu_to_le32(in_len);
    u.get->offset = cpu_to_le32(20);

    /* put_buf((u8*)u.header,u.header->msg_len); */


    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    if (unlikely(retval < 0)) {
        log_info("RNDIS_MSG_QUERY(0x%08x) failed, %d\n",
                 oid, retval);
        return retval;
    }

    off = le32_to_cpu(u.get_c->offset);
    len = le32_to_cpu(u.get_c->len);

    if (unlikely((8 + off + len) > CONTROL_BUFFER_SIZE)) {
        goto response_error;
    }

    if (*reply_len != -1 && len != *reply_len) {
        goto response_error;
    }

    *reply = (unsigned char *) &u.get_c->request_id + off;
    *reply_len = len;

    return retval;

response_error:
    log_info("RNDIS_MSG_QUERY(0x%08x) "
             "invalid response - off %d len %d\n",
             oid, off, len);
    return -EDOM;
}


int generic_rndis_bind(struct usb_host_device *device, int flags)
{
    int			retval;

    union {
        void			           *buf;
        struct rndis_msg_hdr	   *header;
        struct rndis_init	       *init;
        struct rndis_init_c	       *init_c;
        struct rndis_query	       *get;
        struct rndis_query_c       *get_c;
        struct rndis_set	       *set;
        struct rndis_set_c	       *set_c;
        struct rndis_halt	       *halt;
        struct rndis_keepalive     *alive;
        struct rndis_keepalive_c   *alive_c;
    } u;
    u32	tmp;
    __le32	phym_unspec, *phym, *conn_status = NULL;
    int	reply_len;
    unsigned char *bp;

    /* we can't rely on i/o from stack working, or stack allocation */
    u.buf = malloc(CONTROL_BUFFER_SIZE);
    if (!u.buf) {
        return -ENOMEM;
    }

    //retval = usbnet_generic_cdc_bind(dev, intf);



    u.init->msg_type = RNDIS_MSG_INIT;
    u.init->msg_len = cpu_to_le32(sizeof * u.init);
    u.init->major_version = cpu_to_le32(1);
    u.init->minor_version = cpu_to_le32(0);
    u.init->max_transfer_size = 0x4000;

    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);

    if (unlikely(retval < 0)) {
        /* it might not even be an RNDIS device!! */
        log_info("RNDIS init failed, %d\n", retval);
        goto fail_and_release;
    }
    log_info("RNDIS_MSG_INIT_C:\r\n \
              status = %d\r\n \
              major_version = %d\r\n \
              minor_version = %d\r\n \
              device_flags = %d\r\n \
              medium = %d\r\n \
              max_packets_per_message = %d\r\n \
              max_transfer_size = %d\r\n \
              packet_alignment = %d\r\n \
              af_list_offset = %d\r\n \
              af_list_size = %d\n", \
             u.init_c->status, u.init_c->major_version, u.init_c->minor_version, u.init_c->device_flags, u.init_c->medium, \
             u.init_c->max_packets_per_message, u.init_c->max_transfer_size, u.init_c->packet_alignment, \
             u.init_c->af_list_offset, u.init_c->af_list_size);

    tmp = le32_to_cpu(u.init_c->max_transfer_size);
#if 0
    if (tmp < dev->hard_mtu) {
        if (tmp <= net->hard_header_len) {
            log_info(
                "dev can't take %u byte packets (max %u)\n",
                dev->hard_mtu, tmp);
            retval = -EINVAL;
            goto halt_fail_and_release;
        }
        dev_warn(
            "dev can't take %u byte packets (max %u), "
            "adjusting MTU to %u\n",
            dev->hard_mtu, tmp, tmp - net->hard_header_len);
        dev->hard_mtu = tmp;
        net->mtu = dev->hard_mtu - net->hard_header_len;
    }

    /* REVISIT:  peripheral "alignment" request is ignored ... */
    log_info(
        "hard mtu %u (%u from dev), rx buflen %Zu, align %d\n",
        dev->hard_mtu, tmp, dev->rx_urb_size,
        1 << le32_to_cpu(u.init_c->packet_alignment));



    /* module has some device initialization code needs to be done right
     * after RNDIS_INIT */
    if (dev->driver_info->early_init &&
        dev->driver_info->early_init(dev) != 0) {
        goto halt_fail_and_release;
    }

    /* Check physical medium */

#endif
    phym = NULL;
    reply_len = sizeof * phym;
    retval = rndis_query(device, u.buf, OID_GEN_PHYSICAL_MEDIUM,
                         0, (void **) &phym, &reply_len);

    log_info("QUERY->OID_GEN_PHYSICAL_MEDIUM, retval = %d, phym = %x\n", retval, *phym);

    if (retval != 0 || !phym) {
        /* OID is optional so don't fail here. */
        phym_unspec = RNDIS_PHYSICAL_MEDIUM_UNSPECIFIED;
        phym = &phym_unspec;
    }
    if ((flags & FLAG_RNDIS_PHYM_WIRELESS) &&
        *phym != RNDIS_PHYSICAL_MEDIUM_WIRELESS_LAN) {
        netif_dbg(dev, probe, dev->net,
                  "driver requires wireless physical medium, but device is not\n");
        log_info("driver requires wireless physical medium, but device is not\n");
        retval = -ENODEV;
        goto halt_fail_and_release;
    }
    if ((flags & FLAG_RNDIS_PHYM_NOT_WIRELESS) &&
        *phym == RNDIS_PHYSICAL_MEDIUM_WIRELESS_LAN) {
        netif_dbg(dev, probe, dev->net,
                  "driver requires non-wireless physical medium, but device is wireless.\n");
        log_info("driver requires non-wireless physical medium, but device is wireless\n");
        retval = -ENODEV;
        goto halt_fail_and_release;
    }

////////////////////////////////////////////////

    reply_len = sizeof * phym;
    retval = rndis_query(device, u.buf, OID_GEN_MEDIA_CONNECT_STATUS,
                         0, (void **)&conn_status, &reply_len);
    log_info("QUERY->OID_GEN_MEDIA_CONNECT_STATUS, retval = %d, conn_status = %x\n", retval, *conn_status);


    reply_len = ETH_ALEN;
    retval = rndis_query(device, u.buf, OID_802_3_CURRENT_ADDRESS,
                         48, (void **) &bp, &reply_len);

    log_info("QUERY->OID_802_3_CURRENT_ADDRESS, retval = %d: \
              addr = [%X:%X:%X:%X:%X:%X]\r\n", retval, bp[0], bp[1], bp[2], bp[3], bp[4], bp[5]);

////////////////////////////////////////////////


    /* Get designated host ethernet address */
    reply_len = ETH_ALEN;
    retval = rndis_query(device, u.buf, OID_802_3_PERMANENT_ADDRESS,
                         48, (void **) &bp, &reply_len);

    if (unlikely(retval < 0)) {
        log_info("rndis get ethaddr, %d\n", retval);
        goto halt_fail_and_release;
    }
    log_info("QUERY->OID_802_3_PERMANENT_ADDRESS: \
              addr = [%X:%X:%X:%X:%X:%X]\r\n", bp[0], bp[1], bp[2], bp[3], bp[4], bp[5]);
    copy_usbnet_mac_addr(bp);

#if 0
    memcpy(net->dev_addr, bp, ETH_ALEN);
    memcpy(net->perm_addr, bp, ETH_ALEN);
#endif

    /* set a nonzero filter to enable data transfers */
#if 0
    memset(u.set, 0, sizeof * u.set);
    u.set->msg_type = RNDIS_MSG_SET;
    u.set->msg_len = cpu_to_le32(4 + sizeof * u.set);
    u.set->oid = OID_GEN_CURRENT_PACKET_FILTER;
    u.set->len = cpu_to_le32(4);
    u.set->offset = cpu_to_le32((sizeof * u.set) - 8);
    *(__le32 *)(u.buf + sizeof * u.set) = RNDIS_DEFAULT_FILTER;


    /* put_buf((u8*)u.header,u.header->msg_len); */

    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    log_info("SET->RNDIS_DEFAULT_FILTER:\r\n \
              status = %d\n", u.set_c->status);

    if (unlikely(retval < 0)) {
        log_info("rndis set packet filter, %d\n", retval);
        goto halt_fail_and_release;
    }

#else
    ////////////////////////////////////////////////////////
    memset(u.set, 0, sizeof * u.set);
    u.set->msg_type = RNDIS_MSG_SET;
    u.set->msg_len = cpu_to_le32(4 + sizeof * u.set);
    u.set->oid = OID_GEN_CURRENT_PACKET_FILTER;
    u.set->len = cpu_to_le32(4);
    u.set->offset = cpu_to_le32((sizeof * u.set) - 8);
    *(__le32 *)(u.buf + sizeof * u.set) = cpu_to_le32(0x03);
    /* put_buf(u.set, 4 + sizeof * u.set); */
    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    log_info("SET->RNDIS_DEFAULT_FILTER, 0x03:  retval = %d\n", retval);


    u8 val[6] = {0x33, 0x33, 0x00, 0x00, 0x00, 0x01};
    memset(u.set, 0, sizeof * u.set);
    u.set->msg_type = RNDIS_MSG_SET;
    u.set->msg_len = cpu_to_le32(6 + sizeof * u.set);
    u.set->oid = OID_802_3_MULTICAST_LIST;
    u.set->len = cpu_to_le32(6);
    u.set->offset = cpu_to_le32((sizeof * u.set) - 8);
    memcpy((__le32 *)(u.buf + sizeof * u.set), val, 6);
    /* put_buf(u.set, 6 + sizeof * u.set); */
    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    log_info("SET->OID_802_3_MULTICAST_LIST:  retval = %d\n", retval);


    memset(u.set, 0, sizeof * u.set);
    u.set->msg_type = RNDIS_MSG_SET;
    u.set->msg_len = cpu_to_le32(4 + sizeof * u.set);
    u.set->oid = OID_GEN_CURRENT_PACKET_FILTER;
    u.set->len = cpu_to_le32(4);
    u.set->offset = cpu_to_le32((sizeof * u.set) - 8);
    *(__le32 *)(u.buf + sizeof * u.set) = cpu_to_le32(0x07);
    /* put_buf(u.set, 4 + sizeof * u.set); */
    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    log_info("SET->RNDIS_DEFAULT_FILTER, 0x07:  retval = %d\n", retval);


    memset(u.set, 0, sizeof * u.set);
    u.set->msg_type = RNDIS_MSG_SET;
    u.set->msg_len = cpu_to_le32(4 + sizeof * u.set);
    u.set->oid = OID_GEN_CURRENT_PACKET_FILTER;
    u.set->len = cpu_to_le32(4);
    u.set->offset = cpu_to_le32((sizeof * u.set) - 8);
    *(__le32 *)(u.buf + sizeof * u.set) = cpu_to_le32(0x0f);
    /* put_buf(u.set, 4 + sizeof * u.set); */
    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    log_info("SET->RNDIS_DEFAULT_FILTER, 0x0f:  retval = %d\n", retval);


#endif
    memset(u.alive, 0, sizeof * u.alive);
    u.alive->msg_type = RNDIS_MSG_KEEPALIVE;
    u.alive->msg_len = cpu_to_le32(sizeof * u.alive);
    retval = rndis_command(device, u.header, CONTROL_BUFFER_SIZE);
    log_info("SET->RNDIS_MSG_KEEPALIVE, retval = %d\n", retval);

    /* thread_fork("keepalive_period_task", 6, 512, 0, NULL, keepalive_period_task, NULL); */
    ////////////////////////////////////////////////////////

    retval = 0;

    free(u.buf);
    return retval;

halt_fail_and_release:
    memset(u.halt, 0, sizeof * u.halt);
    u.halt->msg_type = RNDIS_MSG_HALT;
    u.halt->msg_len = cpu_to_le32(sizeof * u.halt);

    (void) rndis_command(device, (void *)u.halt, CONTROL_BUFFER_SIZE);
fail_and_release:
    /* usb_set_intfdata(info->data, NULL); */
    /* usb_driver_release_interface(driver_of(intf), info->data); */
    /* info->data = NULL; */
fail:
    free(u.buf);
    return retval;
}


static void keepalive_period_task(void *priv)
{
    int retval = 0;
    struct usb_host_device *host_dev = host_id2device(0);

    union {
        void			           *buf;
        struct rndis_msg_hdr	   *header;
        struct rndis_keepalive     *alive;
        struct rndis_keepalive_c   *alive_c;
    } u;

    u.buf = malloc(CONTROL_BUFFER_SIZE);

    for (;;) {
        memset(u.alive, 0, sizeof * u.alive);
        u.alive->msg_type = RNDIS_MSG_KEEPALIVE;
        u.alive->msg_len = cpu_to_le32(sizeof * u.alive);
        retval = rndis_command(host_dev, u.header, CONTROL_BUFFER_SIZE);
        log_info("%s, retval = %d\n", __FUNCTION__, retval);
        os_time_dly(100);
    }
}

#endif
