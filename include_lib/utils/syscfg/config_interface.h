/*********************************************************************************************
    *   Filename        : config_interface.h

    *   Description     :

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2019-01-10 09:55

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _CONFIG_INTERFACE_H_
#define _CONFIG_INTERFACE_H_

#include "syscfg/config_transport.h"
#include "syscfg/config_target.h"

//配置工具 0:旧工具 1:新工具
#define NEW_CONFIG_TOOL     1       //

void config_layer_init(const ci_transport_t *transport, void *config);

void ci_send_packet(u32 id, u8 *packet, int size);


#endif
