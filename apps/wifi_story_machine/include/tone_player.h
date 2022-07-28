#ifndef _TONE_PLAYER_
#define _TONE_PLAYER_

#include "app_config.h"

struct sin_param {
    int idx_increment;
    int points;
    int decay;
};

#define TONE_STOP       0
#define TONE_START      1

#define DEVICE_EVENT_FROM_TONE		(('T' << 24) | ('N' << 16) | ('E' << 8) | '\0')

#define TONE_DEFAULT_VOL   SYS_MAX_VOL

enum {
    IDEX_TONE_NUM_0,
    IDEX_TONE_NUM_1,
    IDEX_TONE_NUM_2,
    IDEX_TONE_NUM_3,
    IDEX_TONE_NUM_4,
    IDEX_TONE_NUM_5,
    IDEX_TONE_NUM_6,
    IDEX_TONE_NUM_7,
    IDEX_TONE_NUM_8,
    IDEX_TONE_NUM_9,
    IDEX_TONE_BT_MODE,
    IDEX_TONE_BT_CONN,
    IDEX_TONE_BT_DISCONN,
    IDEX_TONE_TWS_CONN,
    IDEX_TONE_TWS_DISCONN,
    IDEX_TONE_LOW_POWER,
    IDEX_TONE_POWER_OFF,
    IDEX_TONE_POWER_ON,
    IDEX_TONE_RING,
    IDEX_TONE_MAX_VOL,
    IDEX_TONE_NONE = 0xFF,
};

#define TONE_NUM_0      		CONFIG_ROOT_PATH"tone/0.*"
#define TONE_NUM_1      		CONFIG_ROOT_PATH"tone/1.*"
#define TONE_NUM_2				CONFIG_ROOT_PATH"tone/2.*"
#define TONE_NUM_3				CONFIG_ROOT_PATH"tone/3.*"
#define TONE_NUM_4				CONFIG_ROOT_PATH"tone/4.*"
#define TONE_NUM_5				CONFIG_ROOT_PATH"tone/5.*"
#define TONE_NUM_6				CONFIG_ROOT_PATH"tone/6.*"
#define TONE_NUM_7				CONFIG_ROOT_PATH"tone/7.*"
#define TONE_NUM_8				CONFIG_ROOT_PATH"tone/8.*"
#define TONE_NUM_9				CONFIG_ROOT_PATH"tone/9.*"
#define TONE_BT_MODE			CONFIG_ROOT_PATH"tone/bt.*"
#define TONE_BT_CONN       		CONFIG_ROOT_PATH"tone/bt_conn.*"
#define TONE_BT_DISCONN    		CONFIG_ROOT_PATH"tone/bt_dconn.*"
#define TONE_TWS_CONN			CONFIG_ROOT_PATH"tone/tws_conn.*"
#define TONE_TWS_DISCONN		CONFIG_ROOT_PATH"tone/tws_dconn.*"
#define TONE_LOW_POWER			CONFIG_ROOT_PATH"tone/low_power.*"
#define TONE_POWER_OFF			CONFIG_ROOT_PATH"tone/power_off.*"
#define TONE_POWER_ON			CONFIG_ROOT_PATH"tone/power_on.*"
#define TONE_RING				CONFIG_ROOT_PATH"tone/ring.*"
#define TONE_MAX_VOL			CONFIG_ROOT_PATH"tone/vol_max.*"
#define TONE_MUSIC				CONFIG_ROOT_PATH"tone/music.*"
#define TONE_LINEIN				CONFIG_ROOT_PATH"tone/linein.*"

#define TONE_REPEAT_BEGIN(a)  (char *)((0x1 << 30) | (a & 0xffff))
#define TONE_REPEAT_END()     (char *)(0x2 << 30)

#define IS_REPEAT_BEGIN(a)    ((((u32)a >> 30) & 0x3) == 0x1 ? 1 : 0)
#define IS_REPEAT_END(a)      ((((u32)a >> 30) & 0x3) == 0x2 ? 1 : 0)
#define TONE_REPEAT_COUNT(a)  (((u32)a) & 0xffff)


void tone_stop();
int tone_play(const char *name);
int tone_play_index(u8 index);
int tone_sin_play(int time_ms, u8 wait);
int tone_get_status();

int tone_file_list_play(const char **list);
int tone_file_list_stop(void);
#endif
