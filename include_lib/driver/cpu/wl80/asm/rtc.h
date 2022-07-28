#ifndef __CPU_RTC_H__
#define __CPU_RTC_H__

#include "typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"
#include "system/sys_time.h"


#define IOCTL_PORT_PR_IN        _IOR('R',  'T'+0,  0)
#define IOCTL_PORT_PR_OUT       _IOW('R',  'T'+1,  0)
#define IOCTL_PORT_PR_PU        _IOR('R',  'T'+2,  0)
#define IOCTL_PORT_PR_PD        _IOR('R',  'T'+3,  0)
#define IOCTL_PORT_PR_HD        _IOW('R',  'T'+4,  0)
#define IOCTL_PORT_PR_DIE       _IOW('R',  'T'+5,  0)
#define IOCTL_PORT_PR_READ      _IOR('R',  'T'+6,  0)

#define WKUP_IO_PR0     0x01
#define WKUP_IO_PR1     0x02
#define WKUP_IO_PR2     0x04
#define WKUP_IO_PR3     0x08
#define WKUP_ALARM      0x10
#define BAT_POWER_FIRST 0x20
#define ABNORMAL_RESET  0x40
#define WKUP_SHORT_KEY	0x80

struct _pr_wkup {
    u8 port;
    u8 edge;  //0 leading edge, 1 falling edge
    u8 port_en;
};

struct rtc_wkup_cfg {
    u8 wkup_en;
    struct _pr_wkup pr1;
    struct _pr_wkup pr2;
    struct _pr_wkup pr3;
};


extern const struct device_operations rtc_dev_ops;

void set_rtc_isr_callback(void (*cb)(void *priv), void *priv);

int rtc_port_pr_in(u8 port);

int rtc_port_pr_read(u8 port);

int rtc_port_pr_out(u8 port, bool on);

int rtc_port_pr_hd(u8 port, bool on);

int rtc_port_pr_pu(u8 port, bool on);

int rtc_port_pr_pd(u8 port, bool on);

int rtc_port_pr_die(u8 port, bool on);

int rtc_early_init();

int rtc_port_pr_wkup_clear_pnd(u8 port);

int rtc_port_pr_wkup_edge(u8 port, bool up_down);

int rtc_port_pr_wkup_en_port(u8 port, bool en);

void rtc_pin_reset_ctrl(u8 enable);

void rtc_wkup_ctrl(struct rtc_wkup_cfg *wkup_cfg);

void alarm_wkup_ctrl(int enable, u32 sec);

u32 rtc_wkup_reason();

void rtc_poweroff();

#endif

