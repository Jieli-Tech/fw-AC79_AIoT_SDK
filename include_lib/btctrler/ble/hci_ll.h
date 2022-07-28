/*********************************************************************************************
    *   Filename        : hci_ll.h

    *   Description     : 提供Vendor Host 直接调用Controller API LL Part

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2018-12-04 11:58

    *   Copyright:(c)JIELI  2011-2017  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _HCI_LL_H_
#define _HCI_LL_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "typedef.h"

enum {
    LL_EVENT_SUPERVISION_TIMEOUT,
    LL_EVENT_RX,
    LL_EVENT_ACL_TX_POST,
};

typedef struct {
    u8 Own_Address_Type: 2;
    u8 Adv_Filter_Policy: 2;
    u8 Scan_Filter_Policy: 2;
    u8 initiator_filter_policy: 2;
} hci_ll_param_t;

//Adjust Host part API
void ll_hci_init(void);

void ll_hci_reset(void);

void ll_hci_destory(void);

void ll_hci_set_event_mask(const u8 *mask);

void ll_hci_set_name(const char *name);

void ll_hci_adv_set_params(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
                           uint8_t direct_address_type, uint8_t *direct_address,
                           uint8_t channel_map, uint8_t filter_policy);

void ll_hci_adv_set_data(uint8_t advertising_data_length, uint8_t *advertising_data);

void ll_hci_adv_scan_response_set_data(uint8_t scan_response_data_length, uint8_t *scan_response_data);

int ll_hci_adv_enable(bool enable);

void ll_hci_scan_set_params(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window);

int ll_hci_scan_enable(bool enable, u8 filter_duplicates);

int ll_hci_create_conn(u8 *conn_param, u8 *addr_param);
int ll_hci_create_conn_ext(void *param);

int ll_hci_create_conn_cancel(void);

int ll_hci_vendor_send_key_num(u16 con_handle, u8 num);

int ll_vendor_latency_hold_cnt(u16 conn_handle, u16 hold_cnt);

int ll_hci_encryption(u8 *key, u8 *plaintext_data);

int ll_hci_get_le_rand(void);


int ll_hci_start_encryption(u16 handle, u32 rand_low, u32 rand_high, u16 peer_ediv, u8 *ltk);

int ll_hci_long_term_key_request_reply(u16 handle, u8 *ltk);

int ll_hci_long_term_key_request_nagative_reply(u16 handle);

int ll_hci_connection_update(u16 handle, u16 conn_interval_min, u16 conn_interval_max,
                             u16 conn_latency, u16 supervision_timeout,
                             u16 minimum_ce_length, u16 maximum_ce_length);

u16 ll_hci_get_acl_data_len(void);

u16 ll_hci_get_acl_total_num(void);

void ll_hci_set_random_address(u8 *addr);

int ll_hci_disconnect(u16 handle, u8 reason);

int ll_hci_read_local_p256_pb_key(void);

int ll_hci_generate_dhkey(const u8 *data, u32 size);

//Adjust Controller part API
void ll_hci_cmd_handler(int *cmd);

void ll_event_handler(int *msg);

void ll_hci_private_free_dma_rx(u8 *rx_head);

void ll_hci_set_data_length(u16 conn_handle, u16 tx_octets, u16 tx_time);

hci_ll_param_t *ll_hci_param_config_get(void);
void hci_ll_get_device_address(uint8_t *addr_type, u8 *addr);
void ll_hci_set_host_channel_classification(u8 *channel_map);

// ble5
void ll_hci_set_ext_adv_params(u8 *data, u32 size);
void ll_hci_set_ext_adv_data(u8 *data, u32 size);
void ll_hci_set_ext_adv_enable(u8 *data, u32 size);
void ll_hci_set_phy(u16 conn_handle, u8 all_phys, u8 tx_phy, u8 rx_phy, u16 phy_options);
void ll_hci_set_ext_scan_params(u8 *data, u32 size);
void ll_hci_set_ext_scan_enable(u8 *data, u32 size);
void ll_hci_ext_create_conn(u8 *data, u32 size);
void ll_hci_set_periodic_adv_params(u8 *data, u32 size);
void ll_hci_set_periodic_adv_data(u8 *data, u32 size);
void ll_hci_set_periodic_adv_enable(u8 *data, u32 size);
void ll_hci_periodic_adv_creat_sync(u8 *data, u32 size);
void ll_hci_periodic_adv_terminate_sync(u8 *data, u32 size);
void ll_hci_periodic_adv_create_sync_cancel(void);

int le_controller_set_mac(void *addr);

#endif
