#ifndef POWER_INTERFACE_H
#define POWER_INTERFACE_H

// #include "asm/hwi.h"
//
#include "generic/typedef.h"
#define NEW_BASEBAND_COMPENSATION       0

#define AT_VOLATILE_RAM             //AT(.volatile_ram)
#define AT_VOLATILE_RAM_CODE        AT(.volatile_ram_code)
#define AT_NON_VOLATILE_RAM         //AT(.non_volatile_ram)
#define AT_NON_VOLATILE_RAM_CODE    //AT(.non_volatile_ram_code)

extern u32 nvbss_begin;
extern u32 nvbss_length;
extern u32 nvdata_begin;
extern u32 nvdata_size;
extern u32 nvdata_addr;

#define NV_RAM_START                &nvbss_begin
#define NV_RAM_SIZE                 &nvbss_length
#define NV_RAM_END                  (NV_RAM_START + NV_RAM_SIZE)

enum {
    MAGIC_ADDR = 2,
    ENTRY_ADDR = 3,
};
#define RAM1_MAGIC_ADDR (NV_RAM_END - MAGIC_ADDR*4)
#define RAM1_ENTRY_ADDR (NV_RAM_END - ENTRY_ADDR*4)

#define SYS_SLEEP_EN                        BIT(1)
#define SYS_SLEEP_BY_IDLE                   BIT(2)
#define RF_SLEEP_EN                         BIT(3)
#define RF_FORCE_SYS_SLEEP_EN               BIT(4)
#define SLEEP_SAVE_TIME_US                  1L

#define DSLEEP_SAVE_BEFORE_ENTER_MS         1
#define DSLEEP_RECOVER_AFTER_EXIT_MS        10
#define DEEP_SLEEP_TIMEOUT_MIN_US           (60*625L)  //间隔至少要60slot以上才进入power off

#define SLEEP_TICKS_UNIT                    (10*1000L) //
#define DEEP_SLEEP_TICKS_UNIT               (20*1000L) //


enum {
    BT_OSC = 0,
    RTC_OSCL,
    LRC_32K,
    LRC_16M,
};

enum {
    OSC_TYPE_BT_OSC = 0,
    OSC_TYPE_RTC,
    OSC_TYPE_LRC,
};

enum {
    PWR_NO_CHANGE = 0,
    PWR_LDO33,
    PWR_LDO15,
    PWR_DCDC15,
};

enum {
    LONG_4S_RESET = 0,
    LONG_8S_RESET,
};

//Macro for VDDIOM_VOL_SEL
enum {
    VDDIOM_VOL_28V = 0,
    VDDIOM_VOL_29V,
    VDDIOM_VOL_30V,
    VDDIOM_VOL_31V,
    VDDIOM_VOL_32V,
    VDDIOM_VOL_33V,
    VDDIOM_VOL_34V,
    VDDIOM_VOL_35V,
};
//Macro for VDDIOW_VOL_SEL
enum {
    VDDIOW_VOL_21V = 0,
    VDDIOW_VOL_24V,
    VDDIOW_VOL_28V,
    VDDIOW_VOL_32V,
};

struct low_power_param {
    u8 osc_type;
    u32 osc_hz;
    u16 delay_us;
    u8  config;
    u8  btosc_disable;

    u8 vddiom_lev;
    u8 vddiow_lev;
    u8 vdc14_lev;
    u8 vdc14_dcdc;
    u8 sysvdd_lev;
    u8 pd_wdvdd_lev;
    u8 vlvd_value;
    u8 vlvd_enable;

    u32 sys_sleep_internal_us;
};

#define BLUETOOTH_RESUME    BIT(1)
#define POWER_SLEEP_WAKEUP 		BIT(2)
#define POWER_OFF_WAKEUP 		BIT(3)

#define RISING_EDGE         0
#define FALLING_EDGE        1

#define POWER_KEEP_DACVDD	BIT(0)
#define POWER_KEEP_RTC		BIT(1)
#define POWER_KEEP_RESET	BIT(2)
#define POWER_KEEP_PWM_LED	BIT(3)

struct port_wakeup {
    u8 edge;        //[0]: Rising / [1]: Falling
    u8 attribute;   //Relate operation bitmap OS_RESUME | BLUETOOTH_RESUME
    u8 iomap;       //Port Group-Port Index
    u8 low_power;
};
struct long_press {
    u8 enable;
    u8 use_sec4;    //1:sec 4 , 0:sec8
    u8 edge;		//rising --> hight reset , falling --> low reset
    u8 iomap;       //Port Group-Port Index
};
struct charge_wakeup {
    u8 attribute;   //Relate operation bitmap OS_RESUME | BLUETOOTH_RESUME
};

struct alarm_wakeup {
    u8 attribute;   //Relate operation bitmap OS_RESUME | BLUETOOTH_RESUME
};

struct lvd_wakeup {
    u8 attribute;   //Relate operation bitmap OS_RESUME | BLUETOOTH_RESUME
};

struct sub_wakeup {
    u8 attribute;   //Relate operation bitmap OS_RESUME | BLUETOOTH_RESUME
};

//<Max hardware wakeup port
#define MAX_WAKEUP_PORT     8

struct wakeup_param {
    const struct port_wakeup *port[MAX_WAKEUP_PORT];
    const struct charge_wakeup *charge;
    const struct alarm_wakeup *alram;
    const struct lvd_wakeup *lvd;
    const struct sub_wakeup *sub;
    const struct long_press *lpres;;
};

struct reset_param {
    u8 en;
    u8 mode;
    u8 level;
    u8 iomap;   //Port Group, Port Index
};

struct low_power_operation {

    const char *name;

    u32(*get_timeout)(void *priv);

    void (*set_timeout)(void *priv, u32 usec);

    void (*suspend_probe)(void *priv);

    void (*suspend_post)(void *priv, u32 usec);

    void (*resume)(void *priv, u32 usec);

    void (*resume_post)(void *priv, u32 usec);

    void (*off_probe)(void *priv);

    void (*off_post)(void *priv, u32 usec);

    void (*on)(void *priv);
};

u32 __tus_carry(u32 x);

u8 __power_is_poweroff(void);

void poweroff_recover(void);

void power_init(const struct low_power_param *param);

u8 power_is_low_power_probe(void);

u8 power_is_low_power_post(void);

void power_set_soft_poweroff(void);

void power_set_mode(u8 mode);

void power_keep_state(u8 data);

u8 get_power_keep_state(void);

void power_set_callback(u8 mode, void (*powerdown_enter)(u8 step), void (*powerdown_exit)(u32), void (*soft_poweroff_enter)(void));

u8 power_is_poweroff_post(void);
// #define  power_is_poweroff_post()   0

void power_set_proweroff(void);

void power_reset_source_dump(void);
/*-----------------------------------------------------------*/

void low_power_on(void);

void low_power_request(char *name);

void low_power_exit_request(void);

void low_power_lock(void);

void low_power_unlock(void);

void *low_power_get(void *priv, const struct low_power_operation *ops);

void low_power_put(void *priv);

void low_power_sys_request(void *priv);

void *low_power_sys_get(void *priv, const struct low_power_operation *ops);

void low_power_sys_put(void *priv);

u8 low_power_sys_is_idle(void);

void low_power_set_audio_run(u8 is_run);//音频播放、唤醒API，参数为真，则禁止系统休眠

s32 low_power_trace_drift(u32 usec);

void low_power_reset_osc_type(u8 type);

u8 low_power_get_default_osc_type(void);

u8 low_power_get_osc_type(void);
void low_power_hw_unsleep_lock(void);
void low_power_hw_unsleep_unlock(void);
/*-----------------------------------------------------------*/

void power_wakeup_init(const struct wakeup_param *param);

void power_wakeup_index_enable(u8 index);

void power_wakeup_index_disable(u8 index);

void power_wakeup_init_test();

u8 get_wakeup_source(void);

u8 is_ldo5v_wakeup(void);
// void power_wakeup_callback(JL_SignalEvent_t cb_event);

void p33_soft_reset(void);
/*-----------------------------------------------------------*/


void lrc_debug(u8 a, u8 b);

void sdpg_config(int enable);


/*-----------------------------------------------------------*/

typedef u8(*idle_handler_t)(void);

struct lp_target {
    char *name;
    idle_handler_t is_idle;
};

#define REGISTER_LP_TARGET(target) \
        const struct lp_target target SEC_USED(.lp_target)


extern const struct lp_target lp_target_begin[];
extern const struct lp_target lp_target_end[];

#define list_for_each_lp_target(p) \
    for (p = lp_target_begin; p < lp_target_end; p++)
/*-----------------------------------------------------------*/

struct deepsleep_target {
    char *name;
    void (*enter)(void);
    void (*exit)(void);
};

#define DEEPSLEEP_TARGET_REGISTER(target) \
        const struct deepsleep_target target SEC_USED(.deepsleep_target)


extern const struct deepsleep_target deepsleep_target_begin[];
extern const struct deepsleep_target deepsleep_target_end[];

#define list_for_each_deepsleep_target(p) \
		    for (p = deepsleep_target_begin; p < deepsleep_target_end; p++)
/*-----------------------------------------------------------*/

#endif
