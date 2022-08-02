#include "asm/includes.h"
#include "device/gpio.h"
#include "device/iic.h"
#include "system/includes.h"


static u8 hw_iic_busy;


static const struct iic_outport *get_hw_iic_outport(const struct hardware_iic *iic)
{
    int i;

    for (i = 0; i < IIC_OUTPORT_NUM; i++) {
        if (iic->clk_pin == iic->outport_map[i].clk_pin &&
            iic->dat_pin == iic->outport_map[i].dat_pin) {
            return &iic->outport_map[i];
        }
    }

    return NULL;
}

static void iic_set_baudrate(const struct hardware_iic *iic, int baudrate)
{
    iic->reg->BAUD = baudrate;
}

static void iic_start(const struct hardware_iic *iic)
{
    iic_set_output(iic);
    iic_add_start_bit(iic);
}

static void iic_stop(const struct hardware_iic *iic)
{
    iic_add_end_bit(iic);
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
#if 0
    u32 time = jiffies + msecs_to_jiffies(200);
    while (time_before(jiffies, time)) {
        if (iic_epnd(iic)) {
            iic_clr_epnd(iic);
            return;
        }
    }
    log_e("iic_wait_stop_timeout\r\n");
#else
    while (!iic_epnd(iic));
#endif
    iic_clr_epnd(iic);
}

static int iic_wait_end(const struct hardware_iic *iic)
{
    u32 time = jiffies + msecs_to_jiffies(200);

    while (time_before(jiffies, time)) {
        /* while (1) { */
        if (iic_pnd(iic)) {
            iic_clr_pnd(iic);
            return 0;
        }
    }
    log_e("iic_wait_end_timeout\r\n");
    return -EFAULT;
}

static int iic_tx(const struct hardware_iic *iic, u8 dat)
{
    iic_set_output(iic);
    iic->reg->BUF = dat;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    return iic_wait_end(iic);
}

static int iic_tx_with_start_bit(const struct hardware_iic *iic, u8 dat)
{
    iic_set_output(iic);
    iic_add_start_bit(iic);
    iic->reg->BUF = dat;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    return iic_wait_end(iic);
}

static int iic_tx_with_stop_bit(const struct hardware_iic *iic, u8 dat)
{
    int err;

    iic_set_output(iic);
    iic->reg->BUF = dat;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    err = iic_wait_end(iic);
    iic_stop(iic);

    return err;
}

static u8 iic_rx(const struct hardware_iic *iic, int *err)
{
    u8 res;
    iic_set_input(iic);
    iic_set_ack(iic);
    iic->reg->BUF = 0xff;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    *err = iic_wait_end(iic);
    res = iic->reg->BUF;
    return res;
}

static u8 iic_rx_with_start_bit(const struct hardware_iic *iic, int *err)
{
    u8 res;
    iic_set_input(iic);
    iic_add_start_bit(iic);
    iic_set_ack(iic);
    iic->reg->BUF = 0xff;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    *err = iic_wait_end(iic);
    return iic->reg->BUF;
}

static u8 iic_rx_with_stop_bit(const struct hardware_iic *iic, int *err)
{
    u8 res;
    iic_set_input(iic);
    iic_set_unack(iic);
    iic->reg->BUF = 0xff;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    *err = iic_wait_end(iic);
    res = iic->reg->BUF;
    iic_stop(iic);
    return res;
}

static u8 iic_rx_with_no_ack(const struct hardware_iic *iic, int *err)
{
    iic_set_input(iic);
    iic_set_unack(iic);
    iic->reg->BUF = 0xff;
    __asm_csync();
    asm("nop");
    iic_cfg_done(iic);
    *err = iic_wait_end(iic);
    return iic->reg->BUF;
}

static int iic_open(struct iic_device *iic)
{
    int value;
    const struct iic_outport *port;
    const struct hardware_iic *hw = (const struct hardware_iic *)iic->hw;

    port = get_hw_iic_outport(hw);
    if (!port) {
        return -EINVAL;
    }

    value = readl(hw->occupy_reg);
    value &= hw->occupy_io_mask;
    value |= port->value;
    writel(hw->occupy_reg, value);

    gpio_direction_output(port->clk_pin, 1);
    gpio_direction_output(port->dat_pin, 1);
    gpio_set_pull_up(port->clk_pin, 1);
    gpio_set_pull_up(port->dat_pin, 1);
    gpio_set_pull_down(port->clk_pin, 0);
    gpio_set_pull_down(port->dat_pin, 0);
    gpio_set_hd(port->clk_pin, 0);
    gpio_set_hd(port->dat_pin, 0);
    gpio_set_die(port->clk_pin, 1);
    gpio_set_die(port->dat_pin, 1);

    hw->reg->BAUD  	= hw->baudrate;
    hw->reg->CON0 	= BIT(0) | BIT(9);
    hw->reg->CON0   |= BIT(14) | BIT(12);

    return 0;
}

static int iic_read(struct iic_device *iic, void *buf, int len)
{
    int err = 0;
    u8 *p = (u8 *)buf;
    u32 rlen = 0;

    if (len == 0) {
        return 0;
    }
    p[rlen++] = iic_rx_with_start_bit((const struct hardware_iic *)iic->hw, &err);

    while (rlen < (len - 1)) {
        p[rlen++] = iic_rx((const struct hardware_iic *)iic->hw, &err);
    }
    if (len - rlen == 1) {
        p[rlen++] = iic_rx_with_stop_bit((const struct hardware_iic *)iic->hw, &err);
    } else {
        iic_stop((const struct hardware_iic *)iic->hw);
    }

    return rlen;
}

static int iic_write(struct iic_device *iic, void *buf, int len)
{
    u8 *p = (u8 *)buf;
    u32 wlen = 0;

    if (len == 0) {
        return 0;
    }
    iic_tx_with_start_bit((const struct hardware_iic *)iic->hw, p[wlen++]);

    while (wlen < (len - 1)) {
        iic_tx((const struct hardware_iic *)iic->hw, p[wlen++]);
    }
    if (len - wlen == 1) {
        iic_tx_with_stop_bit((const struct hardware_iic *)iic->hw, p[wlen++]);
    } else {
        iic_stop((const struct hardware_iic *)iic->hw);
    }

    return wlen;
}

static int iic_ioctl(struct iic_device *iic, int cmd, int arg)
{
    int err = 0;
    struct hardware_iic *hw_iic;
    u8 tmp = 0;

    switch (cmd) {
    case IOCTL_SET_SPEED:
        iic_set_baudrate(iic->hw, arg);
        break;
    case IIC_IOCTL_TX:
        err = iic_tx(iic->hw, arg);
        break;
    case IIC_IOCTL_TX_WITH_START_BIT:
        err = iic_tx_with_start_bit(iic->hw, arg);
        break;
    case IIC_IOCTL_TX_WITH_STOP_BIT:
        err = iic_tx_with_stop_bit(iic->hw, arg);
        break;
    case IIC_IOCTL_TX_STOP_BIT:
        iic_stop(iic->hw);
        break;
    case IIC_IOCTL_TX_START_BIT:
        iic_start(iic->hw);
        break;
    case IIC_IOCTL_RX:
        tmp = iic_rx(iic->hw, &err);
        if (arg) {
            *((u8 *)arg) = tmp;
        }
        break;
    case IIC_IOCTL_RX_WITH_START_BIT:
        tmp = iic_rx_with_start_bit(iic->hw, &err);
        if (arg) {
            *((u8 *)arg) = tmp;
        }
        break;
    case IIC_IOCTL_RX_WITH_STOP_BIT:
        tmp = iic_rx_with_stop_bit(iic->hw, &err);
        if (arg) {
            *((u8 *)arg) = tmp;
        }
        break;
    case IIC_IOCTL_RX_WITH_NOACK:
        tmp = iic_rx_with_no_ack(iic->hw, &err);
        if (arg) {
            *((u8 *)arg) = tmp;
        }
        break;
    case IIC_IOCTL_RX_WITH_ACK:
        tmp = iic_rx(iic->hw, &err);
        if (arg) {
            *((u8 *)arg) = tmp;
        }
        break;
    case IIC_IOCTL_START:
        hw_iic_busy = 1;
        break;
    case IIC_IOCTL_STOP:
        hw_iic_busy = 0;
        break;
    default:
        return -EINVAL;
    }

    return err;
}

static int iic_close(struct iic_device *iic)
{
    const struct hardware_iic *hw = (const struct hardware_iic *)iic->hw;
    const struct iic_outport *port = get_hw_iic_outport(hw);

    hw->reg->CON0 = 0;

    gpio_direction_input(port->clk_pin);
    gpio_set_pull_down(port->clk_pin, 0);
    gpio_set_pull_up(port->clk_pin, 0);
    gpio_set_die(port->clk_pin, 0);

    gpio_direction_input(port->dat_pin);
    gpio_set_pull_down(port->dat_pin, 0);
    gpio_set_pull_up(port->dat_pin, 0);
    gpio_set_die(port->dat_pin, 0);

    return 0;
}

REGISTER_IIC_DEVICE(hw_iic_ops) = {
    .type   = IIC_TYPE_HW,
    .open 	= iic_open,
    .read 	= iic_read,
    .write 	= iic_write,
    .ioctl 	= iic_ioctl,
    .close 	= iic_close,
};

static u8 iic_idle_query(void)
{
    return !hw_iic_busy;
}

REGISTER_LP_TARGET(iic_lp_target) = {
    .name       = "iic",
    .is_idle    = iic_idle_query,
};

