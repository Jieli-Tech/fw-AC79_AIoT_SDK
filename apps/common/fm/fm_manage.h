#ifndef _FM_MANAGE_H
#define _FM_MANAGE_H

#include "app_config.h"
#include "os/os_api.h"


#define FREQ_STEP           (100)//100 步进

#if 0
#define REAL_FREQ_MIN       (875*10)
#define REAL_FREQ_MAX       (1080*10)
#define VIRTUAL_FREQ_STEP   (FREQ_STEP/10)
#define REAL_FREQ(x)        ((REAL_FREQ_MIN-VIRTUAL_FREQ_STEP) + (x)*VIRTUAL_FREQ_STEP)
#define VIRTUAL_FREQ(x)     ((x - (REAL_FREQ_MIN-VIRTUAL_FREQ_STEP))/VIRTUAL_FREQ_STEP)
#define MAX_CHANNEL         ((REAL_FREQ_MAX - REAL_FREQ_MIN)/VIRTUAL_FREQ_STEP + 1)
#else
#define REAL_FREQ_MIN       (875)
#define REAL_FREQ_MAX       (1080)
#define REAL_FREQ(x)        ((REAL_FREQ_MIN-1) + x)
#define VIRTUAL_FREQ(x)     (x - (REAL_FREQ_MIN-1))
#define MAX_CHANNEL         (REAL_FREQ_MAX - REAL_FREQ_MIN + 1)
#endif


#define FM_MUSIC_PP       (0x01)
#define FM_SCAN_ALL_UP    (0x02)
#define FM_SCAN_ALL_DOWN  (0x03)
#define FM_PREV_STATION   (0x04)
#define FM_NEXT_STATION   (0x05)
#define FM_PREV_FREQ      (0x06)
#define FM_NEXT_FREQ      (0x07)
#define FM_VOLUME_UP      (0x08)
#define FM_VOLUME_DOWN    (0x09)
#define FM_DEC_ON         (0x0a)
#define FM_DEC_OFF        (0x0b)
#define FM_MSG_EXIT       (0xff)


typedef const struct {
    int (*init)(void *priv);
    int (*close)(void *priv);
    u8(*set_fre)(void *priv, u16 fre);
    void(*mute)(void *priv, u8 flag);
    u8(*read_id)(void *priv);
    const char *logo;
} FM_INTERFACE;

extern FM_INTERFACE fm_dev_begin[];
extern FM_INTERFACE fm_dev_end[];

#define REGISTER_FM(fm) \
	static FM_INTERFACE fm SEC_USED(.fm_dev)

#define list_for_each_fm(c) \
	for (c=fm_dev_begin; c<fm_dev_end; c++)


void fm_IIC_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len);
u8 fm_IIC_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len);
int fm_server_msg_post(int msg);
int fm_server_init(void);
void fm_sever_kill(void);

#endif
