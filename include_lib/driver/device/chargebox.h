#ifndef __chargebox_H__
#define __chargebox_H__

#include "typedef.h"

#define DEVICE_EVENT_FROM_CHARGEBOX    (('C' << 24) | ('H' << 16) | ('B' << 8) | '\0')

enum {
    CMD_COMPLETE,
    CMD_RECVDATA,
    CMD_RECVBYTE,
};

enum {
    MODE_RECVDATA,
    MODE_SENDDATA,
};

enum {
    EAR_L,
    EAR_R,
};

struct chargebox_platform_data {
    u32 baudrate;
    u32 L_port;
    u32 R_port;
    void (*init)(const struct chargebox_platform_data *);
    void (*open)(u8 l_r, u8 mode);
    void (*close)(u8 l_r);
    u8(*write)(u8 l_r, u8 *data, u8 len);
    void (*set_baud)(u8 l_r, u32 baudrate);
};

/////handshake部分
enum {
    HS_CMD0,
    HS_CMD1,
    HS_CMD2,
    HS_CMD3,
};

enum {
    HS_DELAY_48M,
    HS_DELAY_60M,
    HS_DELAY_80M,
    HS_DELAY_96M,
    HS_DELAY_120M,
    HS_DELAY_160M,
    HS_DELAY_192M,
    HS_DELAY_240M,
};

enum {
    HS_DELAY_2US,
    HS_DELAY_3US,
    HS_DELAY_4US,
    HS_DELAY_7US,
    HS_DELAY_8US,
    HS_DELAY_14US,
    HS_DELAY_16US,
};

//自定义指令
enum {
    CMD_USER = 0xC0,
    /*可添加自定义指令*/
};

struct _hs_hdl {
    u32 port;
    void (*send_delay_us)(u8 us);
};
//handshake
extern void handshake_ctrl_init(struct _hs_hdl *hs);
extern void handshake_send_app(u8 cmd);

//app层使用的接口
extern bool chargebox_api_write_read(u8 l_r, u8 *buf, u8 len, u8 timeout);
extern void chargebox_api_init(const struct chargebox_platform_data *arg);
extern void chargebox_api_uninit(void);
extern void chargebox_api_set_baud(u8 l_r, u32 baudrate);
extern void chargebox_api_shutdown_port(u8 l_r);
extern void chargebox_api_close_port(u8 l_r);
extern void chargebox_api_open_port(u8 l_r);
extern void chargebox_api_reset(void);//左右耳掉线时调用

//协议层api
extern u8 chargebox_get_power(u8 lr);
extern u8 chargebox_send_power_close(u8 lr, u8 power, u8 is_charge, u8 other_power);
extern u8 chargebox_send_power_open(u8 lr, u8 power, u8 is_charge, u8 other_power);
extern u8 chargebox_send_shut_down(u8 lr);
extern u8 chargebox_send_restore_sys(u8 lr);
extern u8 chargebox_send_enter_dut(u8 lr);
extern u8 chargebox_send_close_cid(u8 lr, u8 data);
extern u8 chargebox_delete_tws_addr(u8 lr);
extern u8 chargebox_delete_phone_addr(u8 lr);
extern u8 chargebox_delete_all_addr(u8 lr);
extern u8 chargebox_send_L_or_R(u8 lr);
extern u8 chargebox_exchange_addr(void (*get_addr_cb)(u8 lr, u8 *inbuf), void (*exchange_succ_cb)(void));

#endif
