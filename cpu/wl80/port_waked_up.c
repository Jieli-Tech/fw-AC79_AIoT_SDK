#include "asm/includes.h"
#include "device/gpio.h"
#include "system/includes.h"


struct port_wakeup_work {
    u8 event;
    /* u8 wkup_edge; */
    void (*handler)(void);
    /* void *priv; */
    /* unsigned int gpio; */
} port_wakeup_work;

#define CLEAR_PORT_PEND()    JL_WAKEUP->CON2 = 0xffff
#define READ_PORT_PEND()     JL_WAKEUP->CON3
#define WKUP_ENABLE(x)       JL_WAKEUP->CON0 |= BIT(x)
#define WKUP_DISABLE(x)      JL_WAKEUP->CON0 &= (~BIT(x))
#define WKUP_EDGE(x,edg)     JL_WAKEUP->CON1 = ((JL_WAKEUP->CON1 & (~BIT(x))) | (edg? BIT(x): 0))
#define WKUP_CLEAR_PEND(x)   (JL_WAKEUP->CON2 |= BIT(x))

#define PORT_WAKED_ID_BASE		0X2A
#define PORT_WAKED_ID_GIVE(i)   (PORT_WAKED_ID_BASE + i)
#define PORT_WAKED_ID_GET(hdl)	(hdl - PORT_WAKED_ID_BASE)
#define PORT_WAKED_ID_NUM		(3) //最多16个事件

static struct port_wakeup_work g_wakeup_work[PORT_WAKED_ID_NUM];
static DEFINE_SPINLOCK(lock);


static const s32 port_table[64] = {
    IO_PORTA_00, IO_PORTA_01, IO_PORTA_02, IO_PORTA_03, IO_PORTA_04, IO_PORTA_05, IO_PORTA_06, IO_PORTA_07, IO_PORTA_08, IO_PORTA_09, IO_PORTA_10,
    IO_PORTB_00, IO_PORTB_01, IO_PORTB_02, IO_PORTB_03, IO_PORTB_04, IO_PORTB_05, IO_PORTB_06, IO_PORTB_07, IO_PORTB_08,
    IO_PORTC_00, IO_PORTC_01, IO_PORTC_02, IO_PORTC_03, IO_PORTC_04, IO_PORTC_05, IO_PORTC_06, IO_PORTC_07, IO_PORTC_08, IO_PORTC_09, IO_PORTC_10,
    IO_PORTE_00, IO_PORTE_01, IO_PORTE_02, IO_PORTE_03, IO_PORTE_04, IO_PORTE_05, IO_PORTE_06, IO_PORTE_07, IO_PORTE_08, IO_PORTE_09,
    IO_PORTH_00, IO_PORTH_01, IO_PORTH_02, IO_PORTH_03, IO_PORTH_04, IO_PORTH_05, IO_PORTH_06, IO_PORTH_07, IO_PORTH_08, IO_PORTH_09,
    IO_PORTG_08, IO_PORTG_09, IO_PORTG_10, IO_PORTG_11, IO_PORTG_12, IO_PORTG_13, IO_PORTG_14, IO_PORTG_15,
    -1, /* TMR5_PWM_OUT,\ */
    IO_PORT_USB_DPA, IO_PORT_USB_DMA,
    IO_PORT_USB_DPB, IO_PORT_USB_DMB
};


___interrupt
static void port_wakeup_isr(void)
{
    int i;
    u16 wkup_pend;

    //spin_lock(&lock);
    wkup_pend = READ_PORT_PEND();
    for (i = 0; i < sizeof(g_wakeup_work) / sizeof(struct port_wakeup_work); i++) {
        if (wkup_pend & BIT(g_wakeup_work[i].event)) {
            if (g_wakeup_work[i].handler) {
                g_wakeup_work[i].handler();
            }
            WKUP_CLEAR_PEND(g_wakeup_work[i].event);
        }
    }
    //spin_unlock(&lock);
}

static s32 get_remap_gpio(u32 gpio)
{
    for (u32 idx = 0; idx < ARRAY_SIZE(port_table); idx++) {
        if (port_table[idx] == gpio) {
            return idx;
        }
    }

    return -1;
}

static u8 gpio_2_port_bit(unsigned int gpio)
{
    u8 port_bit;
    u8 port_h;
    u8 port_l;
    port_h = gpio / IO_GROUP_NUM;
    port_l = gpio % IO_GROUP_NUM;
    if (port_h == 4) {
        //PORTE
        port_h = 6;
    } else if (port_h == 6) {
        //PORTG
        port_h = 4;
    } else if (port_h == 7) {
        //PORTH
        port_h = 5;
    }
    port_bit = (port_h << 4) | port_l;
    return port_bit;
}

//event: event参数为EVENT_IO_0或EVENT_IO_1时，gpio为任意IO都可以唤醒
void *port_wakeup_reg(PORT_EVENT_E event, unsigned int gpio, PORT_EDGE_E edge, void (*handler)(void))
{
    int i = 0;
    s32 idx = -1;

    if (!handler) {
        return NULL;
    }

    if (event > EVENT_PH7) {
        return NULL;
    }

    if (gpio > IO_PORT_USB_DMB) {
        return NULL;
    }

    static char port_wakeup_init_flag;
    if (!port_wakeup_init_flag) {
        port_wakeup_init_flag = 1;
        JL_WAKEUP->CON0 = 0;
        JL_WAKEUP->CON2 = 0xffffffff;
        request_irq(IRQ_PORT_IDX, 3, port_wakeup_isr, 0x1);
    }

    spin_lock(&lock);

    while (i < sizeof(g_wakeup_work) / sizeof(struct port_wakeup_work)) {
        if (g_wakeup_work[i].handler == NULL) {
            break;
        }
        ++i;
    }

    if (i == sizeof(g_wakeup_work) / sizeof(struct port_wakeup_work)) {
        spin_unlock(&lock);
        return NULL;
    }

    /* u8 port_io = gpio_2_port_bit(gpio); */

    /* if (event == EVENT_IO_0) { */
    /*     JL_IOMAP->CON2 &= ~(0xff); */
    /*     JL_IOMAP->CON2 |= gpio; */
    /* } else if (event == EVENT_IO_1) { */
    /*     JL_IOMAP->CON2 &= ~(0xff << 8); */
    /*     JL_IOMAP->CON2 |= (gpio << 8); */
    /* } */

    if ((event == EVENT_IO_0) || (event == EVENT_IO_1)) {
        idx = get_remap_gpio(gpio);
        if (idx != -1) {
            if (event == EVENT_IO_0) {
                JL_IOMAP->CON2 &= ~(0xff);
                JL_IOMAP->CON2 |= idx;
            } else if (event == EVENT_IO_1) {
                JL_IOMAP->CON2 &= ~(0xff << 8);
                JL_IOMAP->CON2 |= (idx << 8);
            }
        } else {
            spin_unlock(&lock);
            return NULL;
        }
    }

    if (event < EVENT_SDC0_DAT1 && (event < EVENT_UT0_RX || event > EVENT_UT2_RX)) {
        gpio_direction_input(gpio);
        gpio_set_die(gpio, 1);
        if (edge == EDGE_POSITIVE) {
            gpio_set_pull_down(gpio, 1);
            gpio_set_pull_up(gpio, 0);
        } else if (edge == EDGE_NEGATIVE) {
            gpio_set_pull_down(gpio, 0);
            gpio_set_pull_up(gpio, 1);
        }
    }

    g_wakeup_work[i].event = event;
    /* g_wakeup_work[i].wkup_edge = edge; */
    /* g_wakeup_work[i].gpio = gpio; */
    g_wakeup_work[i].handler = handler;
    /* g_wakeup_work[i].priv = priv; */

    WKUP_EDGE(event, edge);
    WKUP_CLEAR_PEND(event);
    WKUP_ENABLE(event);

    spin_unlock(&lock);

    log_i("ENABLE REG = 0x%x.\n", JL_WAKEUP->CON0);
    log_i("EDGE REG = 0x%x.\n", JL_WAKEUP->CON1);
    log_i("PEND REG = 0x%x.\n", JL_WAKEUP->CON3);
    log_i("JL_IOMAP->CON2=0x%x\n", JL_IOMAP->CON2);

    return (void *)PORT_WAKED_ID_GIVE(i);
}

void port_wakeup_unreg(void *hdl)
{
    if (!hdl || (int)hdl < PORT_WAKED_ID_BASE || (int)hdl >= (PORT_WAKED_ID_BASE + PORT_WAKED_ID_NUM)) {
        log_i("hdl err = 0x%x \r\n", hdl);
        return;
    }
    int i = PORT_WAKED_ID_GET((int)hdl);
    spin_lock(&lock);
    WKUP_DISABLE(g_wakeup_work[i].event);
    WKUP_CLEAR_PEND(g_wakeup_work[i].event);
    memset(&g_wakeup_work[i], 0, sizeof(struct port_wakeup_work));
    spin_unlock(&lock);
}
