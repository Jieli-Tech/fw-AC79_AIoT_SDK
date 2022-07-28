#include "btstack/bluetooth.h"
#include "device/vm.h"
#include "bt_common.h"
#include "model_api.h"
#include "api/sig_mesh_api.h"
#include "device/gpio.h"
#include "app_config.h"

#if CONFIG_BLE_MESH_ENABLE

#define LOG_TAG             "[Mesh-ModelApi]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define APPS_VM_START_INDEX                     238

typedef enum _INFO_SETTING_INDEX {

    MAC_ADDR_INDEX = 60,

    MESH_USER_INDEX = APPS_VM_START_INDEX,

    // ...more to add
} INFO_SETTING_INDEX;

u16_t primary_addr;
static u16_t primary_net_idx;

extern s32 vm_open(u16 index);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);
extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern char *bd_addr_to_str(u8 addr[6]);
extern const u8 led_use_port[2];

void prov_complete(u16_t net_idx, u16_t addr)
{
    log_info("provisioning complete for net_idx 0x%04x addr 0x%04x\n",
             net_idx, addr);
    primary_addr = addr;
    primary_net_idx = net_idx;
}

void prov_reset(void)
{
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static void relay_sent(u16 *delay, u16 *duration, void *cb_data)
{
    u16 delay_ms;

    pseudo_random_genrate((u8 *)&delay_ms, 2);

    delay_ms = btctler_get_rand_from_assign_range(delay_ms, 50, 200);

    *delay = delay_ms;

    log_info("Relay_delay = %u ms", delay_ms);
}

const struct bt_mesh_send_cb relay_sent_cb = {
    .user_intercept = relay_sent,
};

void gpio_pin_write(u8_t led_index, u8_t onoff)
{
    if (led_index >= ARRAY_SIZE(led_use_port)) {
        log_info("led_index over realy led set");
        return;
    }

    onoff ? gpio_direction_output(led_use_port[led_index], 1) : gpio_direction_output(led_use_port[led_index], 0);
}

static bool info_load(INFO_SETTING_INDEX index, void *buf, u16 len)
{
    int ret;

    vm_hdl hdl = vm_open(index);
    log_info("vm hdl=0x%x", hdl);

    ret = vm_read(hdl, (u8 *)buf, len);
    log_info("ret = %d\n", ret);
    if (ret != len) {
        log_info("vm_read err\n");
        log_info("ret = %d\n", ret);
        return 1;
    }

    return 0;
}

static void info_store(INFO_SETTING_INDEX index, void *buf, u16 len)
{
    int ret;

    /* vm_check_all(0); */

    vm_hdl hdl = vm_open(index);
    log_info("vm hdl=0x%x", hdl);

    ret = vm_write(hdl, (u8 *)buf, len);
    if (ret != len) {
        log_info("vm_read err\n");
        log_info("ret = %d\n", ret);
    }

    if (ret != len) {
        log_info("vm_write err\n");
        log_info("ret = %d\n", ret);
    }
}

static void generate_bt_address(u8 addr[6])
{
    u8 i;

    for (i = 0; i < 6;) {
        addr[i++] = JL_RAND->R64L;
        addr[i++] = JL_RAND->R64H;
    }
}

void bt_mac_addr_set(u8 *bt_addr)
{
    int err;

    if (!bt_addr) {
        u8 mac_addr[6];
        bt_addr = mac_addr;
        err = info_load(MAC_ADDR_INDEX, bt_addr, sizeof(bt_addr));
        if (err) {
            log_info(RedBoldBlink "first setup bt mac addr store" Reset);
            generate_bt_address(bt_addr);
            info_store(MAC_ADDR_INDEX, bt_addr, sizeof(bt_addr));
        } else {
            log_info(BlueBoldBlink "load exist bt mac addr" Reset);
        }
    }

    le_controller_set_mac(bt_addr);

    log_info(PurpleBold "CHIP bt MAC : %s" Reset, bd_addr_to_str(bt_addr));
}

void bt_ble_mesh_adv_enable(u8 enable)
{
    if (0 == enable) {
        ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, enable);
        ble_user_cmd_prepare(BLE_CMD_SCAN_ENABLE, 1, enable);
    }
}

void bt_ble_mesh_exit(void)
{
    bt_ble_adv_enable(0);
}

void ble_mesh_module_enable(u8 en)
{
    bt_ble_adv_enable(en);
}

__attribute__((weak)) void ble_profile_init(void)
{

}

#endif
