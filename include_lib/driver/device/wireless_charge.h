#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#include "asm/includes.h"

enum {
    IO_LOW,      //低
    IO_HIGH,     //高
    IO_OVERTURN, //翻转
    IO_OPEN,     //打开
    IO_DIR_IN,//高阻
    IO_INIT,    //初始化
};

struct _wireless_info {
    volatile u8 busy: 1;
        volatile u8 open: 1;
        volatile u8 flag: 1;
        volatile u8 dcdc_en: 1;
        volatile u8 bit_totle: 4;
        volatile u8 bit_num: 4;
        volatile u8 byte_num: 4;
        volatile u8 data_number: 4;
        volatile u8 error_count: 4;
        volatile u8 bit_count;
        volatile u8 nex_packet;
    };

    typedef struct _wireless_hdl_ {
    u16(*get_wl_power)(void);
    void (*dcdc_en_set)(u8 en);
    void (*wpc_set)(u8 en);
    u16   *send_buff;
    u16  voltage_max;
    u16  voltage_min;
    u8   err_packet_fast_cnt;
    struct _wireless_info info;
} _wireless_hdl;


//库C函数声明
void wireless_250us_run(void);
void wireless_open(u16 min, u16 max);
void wireless_close(void);
void get_signal_value(void);
void get_configuration(void);
void get_identification(void);
void wireless_100ms_run(void);
void wireless_lib_init(_wireless_hdl *wldata, u16 min, u16 max);


#endif/*__WIRELESS_H__*/

