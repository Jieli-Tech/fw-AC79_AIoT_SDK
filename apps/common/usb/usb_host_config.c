#include "usb_config.h"
#include "usb/scsi.h"
#include "irq.h"
#include "system/spinlock.h"
#include "app_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define     SET_INTERRUPT   ___interrupt

#if TCFG_UDISK_ENABLE || TCFG_HID_HOST_ENABLE || TCFG_AOA_ENABLE || TCFG_ADB_ENABLE || TCFG_HOST_AUDIO_ENABLE || \
	TCFG_HOST_UVC_ENABLE || TCFG_HOST_WIRELESS_ENABLE

#if TCFG_HOST_UVC_ENABLE
#define     MAX_HOST_EP_RX  5
#define     MAX_HOST_EP_TX  5
#elif TCFG_HID_HOST_ENABLE || TCFG_HOST_AUDIO_ENABLE
#define     MAX_HOST_EP_RX  4
#define     MAX_HOST_EP_TX  4
#else
#define     MAX_HOST_EP_RX  2
#define     MAX_HOST_EP_TX  2   //ep0 & ep1(msd)
#endif

struct host_var_t {
    struct usb_ep_addr_t host_ep_addr;
    usb_h_interrupt usb_h_interrupt_rx[MAX_HOST_EP_RX];
    usb_h_interrupt usb_h_interrupt_tx[MAX_HOST_EP_TX];
    struct usb_host_device *dev_at_ep[MAX_HOST_EP_RX];
};
static struct host_var_t *host_var[USB_MAX_HW_NUM];// SEC(.usb_h_bss);
static spinlock_t lock[USB_MAX_HW_NUM];

static void usb_h_isr(const usb_dev usb_id)
{
    u32 intr_usb, intr_usbe;
    u32 intr_tx, intr_txe;
    u32 intr_rx, intr_rxe;

    __asm__ volatile("ssync");

    usb_read_intr(usb_id, &intr_usb, &intr_tx, &intr_rx);
    usb_read_intre(usb_id, &intr_usbe, &intr_txe, &intr_rxe);
    struct usb_host_device *host_dev = NULL;

    /* r_printf("usb_h_isr %x %x %x %x",host_dev,intr_usb,intr_tx,intr_rx); */
    intr_usb &= intr_usbe;
    intr_tx &= intr_txe;
    intr_rx &= intr_rxe;

    if (intr_usb & INTRUSB_SUSPEND) {
        log_error("usb suspend");
    }
    if (intr_usb & INTRUSB_RESET_BABBLE) {
        log_error("usb reset");
    }
    if (intr_usb & INTRUSB_RESUME) {
        log_error("usb resume");
    }

    if (intr_tx & BIT(0)) {
        spin_lock(&lock[usb_id]);
        if (host_var[usb_id] && host_var[usb_id]->usb_h_interrupt_tx[0]) {
            host_dev = host_var[usb_id]->dev_at_ep[0];
            host_var[usb_id]->usb_h_interrupt_tx[0](host_dev, 0);
        }
        spin_unlock(&lock[usb_id]);
    }

    for (int i = 1; i < MAX_HOST_EP_TX; i++) {
        if (intr_tx & BIT(i)) {
            spin_lock(&lock[usb_id]);
            if (host_var[usb_id] && host_var[usb_id]->usb_h_interrupt_tx[i]) {
                host_dev = host_var[usb_id]->dev_at_ep[i];
                host_var[usb_id]->usb_h_interrupt_tx[i](host_dev, i);
            }
            spin_unlock(&lock[usb_id]);
        }
    }

    for (int i = 1; i < MAX_HOST_EP_RX; i++) {
        if (intr_rx & BIT(i)) {
            spin_lock(&lock[usb_id]);
            if (host_var[usb_id] && host_var[usb_id]->usb_h_interrupt_rx[i]) {
                host_dev = host_var[usb_id]->dev_at_ep[i];
                host_var[usb_id]->usb_h_interrupt_rx[i](host_dev, i);
            }
            spin_unlock(&lock[usb_id]);
        }
    }

    __asm__ volatile("csync");
}

SET_INTERRUPT
static void usb0_h_isr(void)
{
    usb_h_isr(0);
}

SET_INTERRUPT
static void usb1_h_isr(void)
{
    usb_h_isr(1);
}

__attribute__((always_inline_when_const_args))
u32 usb_h_set_intr_hander(const usb_dev usb_id, u32 ep, usb_h_interrupt hander)
{
    spin_lock(&lock[usb_id]);
    if (ep & USB_DIR_IN) {
        if (host_var[usb_id]) {
            host_var[usb_id]->usb_h_interrupt_rx[ep & 0xf] = hander;
        }
    } else {
        if (host_var[usb_id]) {
            host_var[usb_id]->usb_h_interrupt_tx[ep] = hander;
        }
    }
    spin_unlock(&lock[usb_id]);
    return 0;
}

void usb_h_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id)
{
    if (usb_id == 0) {
        request_irq(IRQ_USB_CTRL_IDX, priority, usb0_h_isr, 0);
#if USB_MAX_HW_NUM > 1
    } else if (usb_id == 1) {
        request_irq(IRQ_USB1_CTRL_IDX, priority, usb1_h_isr, 1);
#endif
    }
}

void usb_h_isr_unreg(const usb_dev usb_id, u8 cpu_id)
{
    if (usb_id == 0) {
        unrequest_irq(IRQ_USB_CTRL_IDX, 0);
#if USB_MAX_HW_NUM > 1
    } else if (usb_id == 1) {
        unrequest_irq(IRQ_USB1_CTRL_IDX, 1);
#endif
    }
}

#if (USB_H_MALLOC_ENABLE == 0)
#if TCFG_ADB_ENABLE && TCFG_AOA_ENABLE && TCFG_HID_HOST_ENABLE
static u8 ep0_dma[USB_MAX_HW_NUM][64 + 4]  __attribute__((aligned(4)));
static u8 ep1_dma[USB_MAX_HW_NUM][64 * 2 + 4]  __attribute__((aligned(4)));
static u8 ep2_dma[USB_MAX_HW_NUM][64 * 2 + 4]  __attribute__((aligned(4)));
static u8 ep3_dma[USB_MAX_HW_NUM][64 * 2 + 4]  __attribute__((aligned(4)));
static u8 ep4_dma[USB_MAX_HW_NUM][64 * 2 + 4]  __attribute__((aligned(4)));
#else
static u8 msd_h_dma_buffer[2][64 + 4]  __attribute__((aligned(4))) SEC(.usb_h_dma);
#endif

__attribute__((always_inline_when_const_args))
void *usb_h_get_ep_buffer(const usb_dev usb_id, u32 ep)
{
#if TCFG_ADB_ENABLE && TCFG_AOA_ENABLE && TCFG_HID_HOST_ENABLE
    u8 dir = !!(ep & USB_DIR_IN);
    u8 *p = NULL;
    switch (ep & 0xf) {
    case 0:
        p = &ep0_dma[usb_id][0];
        break;
    case 1:
        p = &ep1_dma[usb_id][dir * 64];
        break;
    case 2:
        p = &ep2_dma[usb_id][dir * 64];
        break;
    case 3:
        p = &ep3_dma[usb_id][dir * 64];
        break;
    case 4:
        p = &ep4_dma[usb_id][dir * 64];
        break;
    }
    return p;
#else
    return msd_h_dma_buffer;
#endif
}
#endif //(USB_H_MALLOC_ENABLE == 0)

extern void *usb_epbuf_alloc(const usb_dev usb_id, u32 size);
extern void usb_epbuf_free(const usb_dev usb_id, void *buf);

void *usb_h_alloc_ep_buffer(const usb_dev usb_id, u32 ep, u32 dma_size)
{
    u8 *ep_buffer = NULL;
#if USB_H_MALLOC_ENABLE
    ep_buffer = usb_epbuf_alloc(usb_id, dma_size);
#else
    ep_buffer = usb_h_get_ep_buffer(usb_id, ep);
#endif
    ASSERT(ep_buffer, "%s() ep_buffer = NULL!!!, usb_id = %d, ep = %x, dma_size = %d\n", __func__, usb_id, ep, dma_size);
    log_info("usb%d host, ep = %x, dma_size = %d, ep_buffer = %x\n", usb_id, ep, dma_size, ep_buffer);
    return ep_buffer;
}

void usb_h_free_ep_buffer(const usb_dev usb_id, void *buf)
{
#if USB_H_MALLOC_ENABLE
    usb_epbuf_free(usb_id, buf);
#endif
    log_info("usb%d host free ep buffer %x\n", usb_id, (u32)buf);
}

void usb_h_set_ep_isr(struct usb_host_device *host_dev, u32 ep, usb_h_interrupt hander, void *p)
{
    if (host_dev) {
        usb_dev usb_id = host_device2id(host_dev);
        spin_lock(&lock[usb_id]);
        if (host_var[usb_id]) {
            host_var[usb_id]->dev_at_ep[ep & 0xf] = p;
        }
        spin_unlock(&lock[usb_id]);
        usb_h_set_intr_hander(usb_id, ep, hander);
    }
}

void usb_host_config(usb_dev usb_id)
{
    if (!host_var[usb_id]) {
        host_var[usb_id] = zalloc(sizeof(struct host_var_t));
    }

    ASSERT(host_var[usb_id], "host_var_t");

    /* g_printf("%s() %x %x", __func__, host_var[usb_id], &(host_var[usb_id]->host_ep_addr)); */
    usb_var_init(usb_id, &(host_var[usb_id]->host_ep_addr));
}

void usb_host_free(usb_dev usb_id)
{
    struct host_var_t *__host_var = NULL;

    g_printf("%s() %x", __func__, host_var[usb_id]);

    usb_h_isr_unreg(usb_id, 0);

    spin_lock(&lock[usb_id]);
    if (host_var[usb_id]) {
        memset(host_var[usb_id]->usb_h_interrupt_rx, 0, sizeof(usb_h_interrupt) * MAX_HOST_EP_RX);
        memset(host_var[usb_id]->usb_h_interrupt_tx, 0, sizeof(usb_h_interrupt) * MAX_HOST_EP_TX);
        __host_var = host_var[usb_id];
        /* usb_var_release(usb_id); */	//释放可能有异步问题
        /* host_var[usb_id] = NULL; */	//释放可能有异步问题
    }
    spin_unlock(&lock[usb_id]);

    /* free(__host_var); */	//释放可能有异步问题
}
#endif
