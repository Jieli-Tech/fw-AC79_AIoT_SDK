// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BT_COMMON_H
#define _BT_COMMON_H
#include <stdint.h>
#include "app_config.h"

#ifdef APP_PRIVATE_PROFILE_CFG
#include "lib_profile_cfg.h"
#else
#include "bt_profile_cfg.h"
#endif

enum {
    ST_BIT_INQUIRY = 0,
    ST_BIT_PAGE_SCAN,
    ST_BIT_BLE_ADV,
    ST_BIT_SPP_CONN,
    ST_BIT_BLE_CONN,
    ST_BIT_WEIXIN_CONN,
};

enum {
    BLE_PRIV_MSG_PAIR_CONFIRM = 0xF0,
    BLE_PRIV_PAIR_ENCRYPTION_CHANGE,
};//ble_state_e

enum {
    COMMON_EVENT_EDR_REMOTE_TYPE = 1,
    COMMON_EVENT_BLE_REMOTE_TYPE,
};

extern uint16_t little_endian_read_16(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_24(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_32(const uint8_t *buffer, int pos);
extern void swapX(const uint8_t *src, uint8_t *dst, int len);


//common api
extern void bt_ble_init(void);
extern void bt_ble_exit(void);
extern void bt_ble_adv_enable(u8 enable);
extern void ble_app_disconnect(void);
extern void bt_master_ble_init(void);
extern void bt_master_ble_exit(void);

#endif
