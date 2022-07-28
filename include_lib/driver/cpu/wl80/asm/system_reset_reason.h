#ifndef _SYS_RESET_RS_
#define _SYS_RESET_RS_

#ifndef BIT
#define BIT(x)	(1L << (x))
#endif

enum sys_reset_type {
    SYS_RST_NONE = 0,
    SYS_RST_12V = BIT(1),
    SYS_RST_WDT = BIT(2),
    SYS_RST_VCM = BIT(3),
    SYS_RST_SOFT = BIT(4),
    SYS_RST_ALM_WKUP = BIT(5),
    SYS_RST_PORT_WKUP = BIT(6),
    SYS_RST_LONG_PRESS = BIT(7),
    SYS_RST_VDDIO_PWR_ON = BIT(8),
    SYS_RST_VDDIO_LOW_PWR = BIT(9),
};

#define SYS_ALM_WAKUP	"ALM_WAKUP"
#define SYS_PORT_WAKUP	"PORT WKUP"
#define SYS_SOFT		"SOFT"
#define SYS_POWER_ON	"POWER ON"
#define SYS_LOW_POWER	"LOW POWER"
#define SYS_WDT			"WDT"
#define SYS_VCM			"VCM"
#define SYS_LONG_PRESS	"LONG PRESS"
#define SYS_SYS_12V		"SYS_1.2V"

int system_reset_reason_get(void);

#endif


