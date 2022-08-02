/*============================================================================
*                                                                            *
* Copyright (C) by Tuya Inc                                                  *
* All rights reserved                                                        *
*                                                                            *
*                                                                            *
=============================================================================*/

/*============================ INCLUDES ======================================*/
#include "tuya_i2c.h"
#include "tuya_os_adapter_error_code.h"
/*============================ MACROS ========================================*/
#define I2C_DEV_NUM            1

/*============================ TYPES =========================================*/
typedef struct {
    tuya_i2c_t                  dev;
} i2c_dev_t;

/*============================ PROTOTYPES ====================================*/
static int i2c_dev_init(tuya_i2c_t *uart, tuya_i2c_cfg_t *cfg);
static int i2c_dev_control(tuya_i2c_t *uart, uint8_t cmd, void *arg);
static int i2c_dev_deinit(tuya_i2c_t *uart);
static int i2c_dev_xfer(tuya_i2c_t *i2c, tuya_i2c_msg_t *msgs, uint8_t num);

/*============================ LOCAL VARIABLES ===============================*/
static i2c_dev_t s_i2c_dev[I2C_DEV_NUM];

static const tuya_i2c_ops_t  i2c_dev_ops = {
    .init       = i2c_dev_init,
    .xfer       = i2c_dev_xfer,
    .control    = i2c_dev_control,
    .deinit     = i2c_dev_deinit,
};
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/
int platform_i2c_init(void)
{
    s_i2c_dev[TUYA_I2C0].dev.ops = (tuya_i2c_ops_t *)&i2c_dev_ops;
    return tuya_driver_register(&s_i2c_dev[TUYA_I2C0].dev.node, TUYA_DRV_I2C, TUYA_I2C0);
}

int i2c_dev_init(tuya_i2c_t *i2c, tuya_i2c_cfg_t *cfg)
{

    return OPRT_OS_ADAPTER_OK;
}

static int i2c_dev_xfer(tuya_i2c_t *i2c, tuya_i2c_msg_t *msgs, uint8_t num)
{
    return OPRT_OS_ADAPTER_OK;
}

int i2c_dev_control(tuya_i2c_t *i2c, uint8_t cmd, void *arg)
{
    return OPRT_OS_ADAPTER_OK;
}

int i2c_dev_deinit(tuya_i2c_t *i2c)
{
    return OPRT_OS_ADAPTER_OK;
}


