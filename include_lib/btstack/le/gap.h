/*********************************************************************************************
    *   Filename        : gap.h

    *   Description     :

    *   Author          : Minxian

    *   Email           : liminxian@zh-jieli.com

    *   Last modifiled  : 2020-07-01 16:37

    *   Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _BT_GAP_H_
#define _BT_GAP_H_

#include "typedef.h"
#include "btstack/btstack_typedef.h"

int gap_request_connection_parameter_update(hci_con_handle_t con_handle, uint16_t conn_interval_min,
        uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout);


void gap_advertisements_enable(int enabled);

void gap_advertisements_set_data(uint8_t advertising_data_length, uint8_t *advertising_data);

void gap_scan_response_set_data(uint8_t scan_response_data_length, uint8_t *scan_response_data);

void gap_advertisements_set_params(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
                                   uint8_t direct_address_typ, uint8_t *direct_address, uint8_t channel_map, uint8_t filter_policy);


#endif
