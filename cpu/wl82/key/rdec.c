#include "key/key_driver.h"
#include "device/gpio.h"
#include "asm/rdec.h"
#include "app_config.h"

#if TCFG_RDEC_KEY_ENABLE

/* #define RDEC_DEBUG_ENABLE */
#ifdef RDEC_DEBUG_ENABLE
#define rdec_debug(fmt, ...) 	printf("[RDEC] "fmt, ##__VA_ARGS__)
#else
#define rdec_debug(...)
#endif

#define rdec_error(fmt, ...) 	printf("[RDEC] ERR: "fmt, ##__VA_ARGS__)

#define RDEC_MODULE_NUM 		1

struct rdec {
    const struct rdec_platform_data *user_data;
    u8 init;
};

static struct rdec _rdec;

#define __this 			(&_rdec)

static const u8 rdec_hw_sin0[RDEC_MODULE_NUM] = {IO_PORTC_09};
static const u8 rdec_hw_sin1[RDEC_MODULE_NUM] = {IO_PORTC_10};
static const u8 rdec_int_index[RDEC_MODULE_NUM] = {IRQ_RDEC_IDX};

extern int gpio_rdec_inputchannel(u32 gpio, u8 sel, u8 mode);

typedef volatile struct {
    u32 con;
    int dat;
} RDEC_REG;

static RDEC_REG *rdec_get_reg(u8 index)
{
    RDEC_REG *reg = NULL;

    switch (index) {
    case RDEC0:
        reg = (RDEC_REG *)JL_RDEC;
        break;
    default:
        break;
    }

    return reg;
}

static void __rdec_port_init(u8 port)
{
    gpio_set_pull_down(port, 0);
    gpio_set_pull_up(port, 1);
    gpio_set_die(port, 1);
    gpio_set_direction(port, 1);
}

static void rdec_port_init(const struct rdec_device *rdec)
{
    switch (rdec->index) {
    case RDEC0:
        if (rdec->sin_port0 == rdec_hw_sin0[0]) {
            JL_IOMAP->CON1 &= ~BIT(12);
        } else {
            gpio_rdec_inputchannel(rdec->sin_port0, 0, 1);
        }

        if (rdec->sin_port1 == rdec_hw_sin1[0]) {
            JL_IOMAP->CON1 &= ~BIT(13);
        } else {
            gpio_rdec_inputchannel(rdec->sin_port1, 1, 1);
        }
        break;
    default:
        return;
    }

    __rdec_port_init(rdec->sin_port0);
    __rdec_port_init(rdec->sin_port1);
}

static u8 rdec_get_key_value(u8 index, u8 key_num)
{
    for (u8 i = 0; i < __this->user_data->num; i++) {
        if (index == __this->user_data->rdec[i].index) {
            if (key_num == 0) {
                return __this->user_data->rdec[i].key_value0;
            } else if (key_num == 1) {
                return __this->user_data->rdec[i].key_value1;
            }
        }
    }
    return 0xFF;
}

___interrupt
static void rdec_isr(void)
{
    u8 i;
    s8 rdat = 0;
    u8 key_value;
    RDEC_REG *reg = NULL;

    for (i = 0; i < RDEC_MODULE_NUM; i++) {
        reg = rdec_get_reg(i);
        if (reg->con & BIT(7)) {
            rdat = reg->dat;
            reg->con |= BIT(6); //clr pending, DAT reg will reset as well
            if (rdat < 0) {
                key_value = rdec_get_key_value(i, 0);
            } else if (rdat > 0) {
                key_value = rdec_get_key_value(i, 1);
            } else {
                key_value = 0xFF;
            }
            if (key_value != 0xFF) {
                //TODO: notify rdec event
                rdec_debug("RDEC%d isr, key_value: %d, dat: %d", i, key_value, rdat);
            }
        }
    }
}

s8 get_rdec_rdat(i)
{
    RDEC_REG *reg = NULL;
    s8 _rdat = 0;
    reg = rdec_get_reg(i);
    if (reg->con & BIT(7)) {
        _rdat = reg->dat;
        reg->con |= BIT(6);
    }
    if (_rdat != 0) {
        rdec_debug("RDEC: %d , dat: %d", i, _rdat);
    }
    return _rdat;
}

static void log_rdec_info(void)
{
    RDEC_REG *reg = NULL;

    for (u8 i = 0; i < RDEC_MODULE_NUM; i++) {
        reg = rdec_get_reg(i);
        rdec_debug("RDEC%d CON = 0x%x", i, reg->con);
    }
}

static int rdec_param_cfg_check(const struct rdec_platform_data *user_data)
{
    const struct rdec_device *rdec;

    u8 input_ch6_sta = 0;
    u8 input_ch7_sta = 0;

    if (user_data->enable == 0) {
        rdec_error("rdec not enable");
    }
    //port check
    for (u8 i = 0; i < user_data->num; i++) {
        rdec = &(user_data->rdec[i]);
        ASSERT(rdec->index < RDEC_MODULE_NUM, "RDEC cfg index err");

        if (rdec->sin_port0 != rdec_hw_sin0[rdec->index]) {
            input_ch6_sta++;
        }
        if (rdec->sin_port1 != rdec_hw_sin1[rdec->index]) {
            input_ch7_sta++;
        }
    }

    if ((input_ch6_sta >= 2) || (input_ch7_sta >= 2)) {
        rdec_error("rdec input channel not enough");
        return -1;
    }

    return 0;
}

//for test
int rdec_init(const struct rdec_platform_data *user_data)
{
    RDEC_REG *reg = NULL;
    const struct rdec_device *rdec;

    __this->init = 0;

    if (user_data == NULL) {
        return -1;
    }

    if (rdec_param_cfg_check(user_data)) {
        return -1;
    }

    for (u8 i = 0; i < user_data->num; i++) {
        rdec = &(user_data->rdec[i]);

        rdec_port_init(rdec);
        //module init
        reg = rdec_get_reg(rdec->index);
        reg->con = 0;
        reg->con |= (0xf << 2); //2^15, lsb = 53MHz, 2^15 / 53us = 618us = 0.618ms
        reg->con &= ~BIT(1); //pol = 0, io should pull up
        //reg->con |= BIT(1); //pol = 1, io should pull down
        reg->con |= BIT(6); //clear pending
        reg->con |= BIT(0); //RDECx EN

        /* request_irq(rdec_int_index[rdec->index], 1, rdec_isr, 0); */
    }

    __this->init = 1;
    __this->user_data = user_data;

    log_rdec_info();

    return 0;
}

#endif
