/*********************************************************************************************
    *   Filename        : audio_interface.h

    *   Description     :

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2018-05-05 09:40

    *   Copyright:(c)JIELI  2011-2017  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef	_ADAPTER_AUDIO_INTERFACE_H_
#define _ADAPTER_AUDIO_INTERFACE_H_

#include "typedef.h"

typedef enum {
    AUDIO_INPUT = 0,
    AUDIO_OUTPUT,
} AUDIO_TYPE;


void audio_init(void);

void audio_open_stream(AUDIO_TYPE type, u32 samplerate, void (*callback)(void *buffer, int size));

void audio_start_stream(void);

void audio_stop_stream(void);

#endif
