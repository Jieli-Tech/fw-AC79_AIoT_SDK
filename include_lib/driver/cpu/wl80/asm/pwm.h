#ifndef _TIME_PWM_H
#define _TIME_PWM_H

#include "includes.h"
#include "device/device.h"

#define PWM_MAX_NUM		8	//0-7，最多,8个通道

/***PWMCHx***/
#define PWMCH0_H  		BIT(0)
#define PWMCH1_H  		BIT(1)
#define PWMCH2_H  		BIT(2)
#define PWMCH3_H  		BIT(3)
#define PWMCH4_H  		BIT(4)
#define PWMCH5_H  		BIT(5)
#define PWMCH6_H  		BIT(6)
#define PWMCH7_H  		BIT(7)

#define PWM_CHL_OFFSET	10
#define PWMCH0_L  		BIT(PWM_CHL_OFFSET + 0)
#define PWMCH1_L  		BIT(PWM_CHL_OFFSET + 1)
#define PWMCH2_L  		BIT(PWM_CHL_OFFSET + 2)
#define PWMCH3_L  		BIT(PWM_CHL_OFFSET + 3)
#define PWMCH4_L  		BIT(PWM_CHL_OFFSET + 4)
#define PWMCH5_L  		BIT(PWM_CHL_OFFSET + 5)
#define PWMCH6_L  		BIT(PWM_CHL_OFFSET + 6)
#define PWMCH7_L  		BIT(PWM_CHL_OFFSET + 7)

#define TIME_PWM_MAX	2
#define TIME_PWM_OFFSET	20
#define PWM_TIMER2_OPCH2		BIT(TIME_PWM_OFFSET + 0)
#define PWM_TIMER3_OPCH3		BIT(TIME_PWM_OFFSET + 1)

/***ioctrl cmd***/
#define PWM_STOP				_IOW('P',0,u32)
#define PWM_RUN					_IOW('P',1,u32)
#define PWM_FORDIRC				_IOW('P',2,u32) //正向
#define PWM_REVDIRC				_IOW('P',3,u32) //反向
#define PWM_ADD_CHANNEL			_IOW('P',4,u32)
#define PWM_SET_DEATH_TIME		_IOW('P',5,u32)//死区时间为系统频率周期时间的整数倍，最大值31
#define PWM_SET_FREQ			_IOW('P',6,u32)
#define PWM_SET_DUTY			_IOW('P',7,u32)
#define PWM_GET_DUTY			_IOR('P',7,u32)
#define PWM_REMOV_CHANNEL		_IOW('P',8,u32)

typedef struct pwm_timer {
    u32	con;
    u32	cnt;
    u32 prd;
} PWM_TIMER;

typedef struct pwm_chcon {
    u32	con0;
    u32	con1;
    u32	cmph;
    u32	cmpl;
} PWM_CHCON;

struct pwm_platform_data {
    PWM_TIMER *treg;
    PWM_CHCON *creg;
    float duty;//用于带2位小数点占空比的PWM，0.00%-100.00%
    u8  deathtime;//最大值31
    u8  point_bit;//小数点精度:0-2
    u32 port;
    u32  pwm_ch;
    u32 freq;
};

struct pwm_operations {
    int (*init)(void);
    int (*open)(struct pwm_platform_data *pwm_data);
    int (*close)(struct pwm_platform_data *pwm_data);
    int (*write)(struct pwm_platform_data *pwm_data);
    int (*read)(struct pwm_platform_data *pwm_data);
    int (*ioctl)(struct pwm_platform_data *pwm_data, u32 cmd, u32 arg);
};

struct pwm_device {
    const char *name;
    const struct pwm_operations *ops;
    struct device dev;
    void *priv;
};

#define PWM_PLATFORM_DATA_BEGIN(data) \
		static const struct pwm_platform_data data={

#define PWM_PLATFORM_DATA_END() \
		.treg=NULL, \
		.creg=NULL, \
	};

extern const struct device_operations pwm_dev_ops;

#endif




