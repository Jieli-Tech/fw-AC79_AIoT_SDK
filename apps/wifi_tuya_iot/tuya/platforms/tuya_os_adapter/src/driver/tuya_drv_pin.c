/*============================================================================
*                                                                            *
* Copyright (C) by Tuya Inc                                                  *
* All rights reserved                                                        *
*                                                                            *
=============================================================================*/


/*============================ INCLUDES ======================================*/
#include "asm/gpio.h"
#include "system/includes.h"
#include "app_config.h"
#include "asm/port_waked_up.h"
#include "device/gpio.h"

#include "tuya_pin.h"
#include "tuya_os_adapter_error_code.h"
#include "tuya_os_adapt_output.h"
/*============================ MACROS ========================================*/

/*============================ TYPES =========================================*/

typedef struct {
    int jl_gpio;
} pin_dev_map_t;


typedef struct {

    PORT_EVENT_E event;

    int gpio;

    void *hdl;

} pin_event_map;

/*============================ PROTOTYPES ====================================*/
static int pin_dev_init(tuya_pin_name_t pin, tuya_pin_mode_t mode);
static int pin_dev_write(tuya_pin_name_t pin, tuya_pin_level_t level);
static int pin_dev_read(tuya_pin_name_t pin);
static int pin_dev_toggle(tuya_pin_name_t pin);
static int pin_dev_control(tuya_pin_name_t pin, uint8_t cmd, void *arg);
/*============================ LOCAL VARIABLES ===============================*/
const static pin_dev_map_t pinmap[] = {
    {IO_PORTA_00}, {IO_PORTA_01}, {IO_PORTA_02}, {IO_PORTA_03},
    {IO_PORTA_04}, {IO_PORTA_05}, {IO_PORTA_06}, {IO_PORTA_07},
    {IO_PORTA_08}, {IO_PORTA_09}, {IO_PORTA_10}, {IO_PORTB_00},
    {IO_PORTB_01}, {IO_PORTB_02}, {IO_PORTB_03}, {IO_PORTB_04},
    {IO_PORTB_05}, {IO_PORTB_06}, {IO_PORTB_07}, {IO_PORTB_08},
    {IO_PORTC_00}, {IO_PORTC_01}, {IO_PORTC_02}, {IO_PORTC_03},
    {IO_PORTC_04}, {IO_PORTC_05}, {IO_PORTC_06}, {IO_PORTC_07},
    {IO_PORTC_08}, {IO_PORTC_09}, {IO_PORTC_10}, {IO_PORTD_00},
    {IO_PORTD_01}, {IO_PORTD_02}, {IO_PORTD_03}, {IO_PORTD_04},
    {IO_PORTD_05}, {IO_PORTD_06}, {IO_PORTH_00}, {IO_PORTH_01},
    {IO_PORTH_02}, {IO_PORTH_03}, {IO_PORTH_04}, {IO_PORTH_05},
    {IO_PORTH_06}, {IO_PORTH_07}, {IO_PORTH_08}, {IO_PORTH_09},
};

static pin_event_map event_map[] = {
    {EVENT_IO_0, 0xffffffff, NULL}, {EVENT_IO_1, 0xffffffff, NULL}, {EVENT_PA0, IO_PORTA_00, NULL}, {EVENT_PA7, IO_PORTA_07, NULL},
    {EVENT_PA8, IO_PORTA_08, NULL}, {EVENT_PB1, IO_PORTB_01, NULL}, {EVENT_PB6, IO_PORTB_06, NULL}, {EVENT_PC0, IO_PORTC_00, NULL},
    {EVENT_PC1, IO_PORTC_01, NULL}, {EVENT_PH0, IO_PORTH_00, NULL}, {EVENT_PH7, IO_PORTH_07, NULL}
};


static const tuya_pin_ops_t s_pin_dev_ops = {
    .init     = pin_dev_init,
    .write    = pin_dev_write,
    .read     = pin_dev_read,
    .toggle   = pin_dev_toggle,
    .control  = pin_dev_control,
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/
int platform_pin_init(void)
{
    tuya_os_adapt_output_log("-----platform_pin_init \r\n");
    return tuya_pin_register((tuya_pin_ops_t *)&s_pin_dev_ops);
}

/**
 * @brief pin_dev_init
 *
 * @param[in] tuya_pin_name_t pin, tuya_pin_mode_t mode
 * @return int 0=成功，非0=失败
 */
static int pin_dev_init(tuya_pin_name_t pin, tuya_pin_mode_t mode)
{
    if (pin >= (sizeof(pinmap) / sizeof(pin_dev_map_t))) {
        /* code */
        tuya_os_adapt_output_log("pin error \r\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    //! set pin direction
    switch (mode & TUYA_PIN_INOUT_MASK)  {
    case TUYA_PIN_IN:
        /*设置成数字输入*/
        gpio_set_die(pinmap[pin].jl_gpio, 1);
        gpio_set_direction(pinmap[pin].jl_gpio, 1);
        break;
    case TUYA_PIN_OUT_PP:
        gpio_set_direction(pinmap[pin].jl_gpio, 0);
        break;
    case TUYA_PIN_OUT_OD:
        gpio_set_direction(pinmap[pin].jl_gpio, 0);
        break;
    default:
        return OPRT_OS_ADAPTER_NOT_SUPPORTED;
    }

    //! set pin mdoe
    switch (mode & TUYA_PIN_MODE_MASK) {
    case TUYA_PIN_PULL_UP:
        __gpio_set_pull_up(pinmap[pin].jl_gpio, 1);
        break;
    case TUYA_PIN_PULL_DOWN:
        __gpio_set_pull_down(pinmap[pin].jl_gpio, 1);
        break;
    case TUYA_PIN_PULL_NONE:
        break;
    }

    //! set pin init level
    switch (mode & TUYA_PIN_INIT_MASK) {
    case TUYA_PIN_INIT_LOW:
        __gpio_direction_output(pinmap[pin].jl_gpio, 0);
        break;

    case TUYA_PIN_INIT_HIGH:
        __gpio_direction_output(pinmap[pin].jl_gpio, 1);
        break;
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief pin_dev_write
 *
 * @param[in] tuya_pin_name_t pin, tuya_pin_level_t level
 * @return int 0=成功，非0=失败
 */
static int pin_dev_write(tuya_pin_name_t pin, tuya_pin_level_t level)
{
    if (pin >= (sizeof(pinmap) / sizeof(pin_dev_map_t))) {
        /* code */
        tuya_os_adapt_output_log("pin error \r\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (level == TUYA_PIN_HIGH) {
        __gpio_direction_output(pinmap[pin].jl_gpio, 1);
    } else {
        __gpio_direction_output(pinmap[pin].jl_gpio, 0);
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief pin_dev_read
 *
 * @param[in] tuya_pin_name_t pin
 * @return pin level
 */
static int pin_dev_read(tuya_pin_name_t pin)
{
    if (pin >= (sizeof(pinmap) / sizeof(pin_dev_map_t))) {
        /* code */
        tuya_os_adapt_output_log("pin error \r\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    return __gpio_read(pinmap[pin].jl_gpio);
}

/**
 * @brief pin_dev_toggle
 *
 * @param[in] tuya_pin_name_t pin
 * @return pin level
 */
static int pin_dev_toggle(tuya_pin_name_t pin)
{
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief pin_dev_irq_config
 *
 * @param[in] tuya_pin_name_t pin, tuya_pin_irq_t *irq
 * @return int 0=成功，非0=失败
 */
static int pin_dev_irq_config(tuya_pin_name_t pin, tuya_pin_irq_t *irq)
{
    int i = 0;

    PORT_EVENT_E event;
    int gpio;
    unsigned char find_it = 0;

    PORT_EDGE_E edge_type;

    if (TUYA_PIN_IN_IRQ != (irq->mode & TUYA_PIN_INOUT_MASK)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    for (i = 2; i < (sizeof(event_map) / sizeof(pin_event_map)); i++) {
        if (pinmap[pin].jl_gpio == event_map[i].gpio) {
            find_it = 1;
            event = event_map[i].event;
            gpio = pinmap[pin].jl_gpio;
            break;
        }
    }

    if (find_it == 0) {
        if (event_map[0].gpio == 0xffffffff) {
            i = 0;
            event_map[0].gpio =  pinmap[pin].jl_gpio;
            gpio = pinmap[pin].jl_gpio;
            event = event_map[0].event;
            find_it = 1;
        }

        if (event_map[1].gpio == 0xffffffff) {
            i = 1;
            event_map[1].gpio =  pinmap[pin].jl_gpio;
            gpio = pinmap[pin].jl_gpio;
            event = event_map[1].event;
            find_it = 1;
        }
    }

    if (find_it == 0) {
        return OPRT_OS_ADAPTER_NOT_SUPPORTED;
    }

    switch (irq->mode & TUYA_PIN_IRQ_MASK) {
    case TUYA_PIN_IRQ_RISE:
        edge_type = EDGE_POSITIVE;
        break;
    case TUYA_PIN_IRQ_FALL:
        edge_type = EDGE_NEGATIVE;
        break;
    case TUYA_PIN_IRQ_LOW:
        return OPRT_OS_ADAPTER_NOT_SUPPORTED;
        break;
    case TUYA_PIN_IRQ_HIGH:
        return OPRT_OS_ADAPTER_NOT_SUPPORTED;
        break;
    default:
        return OPRT_OS_ADAPTER_NOT_SUPPORTED;
    }

    event_map[i].hdl = port_wakeup_reg(event, gpio, edge_type, irq->cb);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief pin_dev_irq_deinit
 *
 * @param[in] tuya_pin_name_t pin
 * @return int 0=成功，非0=失败
 */
static int pin_dev_irq_deinit(tuya_pin_name_t pin)
{
    int i = 0;
    int ret = OPRT_OS_ADAPTER_NOT_SUPPORTED;

    for (i = 0; i < (sizeof(event_map) / sizeof(pin_event_map)); i++) {
        if (pinmap[pin].jl_gpio == event_map[i].gpio && event_map[i].hdl) {
            port_wakeup_unreg(event_map[i].hdl);
            event_map[i].hdl = NULL;
            ret = OPRT_OS_ADAPTER_OK;
            break;
        }
    }
    return ret;
}

/**
 * @brief pin_dev_control
 *
 * @param[in] tuya_pin_name_t pin, uint8_t cmd, void *arg
 * @return int 0=成功，非0=失败
 */
static int pin_dev_control(tuya_pin_name_t pin, uint8_t cmd, void *arg)
{
    int result = OPRT_OK;

    switch (cmd) {
    case TUYA_DRV_SET_ISR_CMD:
        result = pin_dev_irq_config(pin, (tuya_pin_irq_t *)arg);
        break;

    case TUYA_DRV_SET_INT_CMD:
        break;

    case TUYA_DRV_CLR_INT_CMD:
        result = pin_dev_irq_deinit(pin);
        break;
    }

    return result;
}


