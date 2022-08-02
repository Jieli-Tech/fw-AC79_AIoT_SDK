#ifndef _GSENSOR_MANAGE_H
#define _GSENSOR_MANAGE_H
#include "printf.h"
#include "cpu.h"
#include "device/iic.h"
#include "asm/iic.h"
#include "timer.h"
#include "app_config.h"
#include "event.h"
#include "device_event.h"
#include "system/includes.h"

#define     GRA_SEN_OFF             0
#define     GRA_SEN_LO              1
#define     GRA_SEN_MD              2
#define     GRA_SEN_HI              3

extern unsigned char gravity_sensor_command(unsigned char w_chip_id, unsigned char register_address, unsigned char function_command);
extern unsigned char _gravity_sensor_get_data(unsigned char w_chip_id, unsigned char r_chip_id, unsigned char register_address);


typedef struct {
    unsigned char   logo[20];
    char (*gravity_sensor_check)(void);
    void (*gravity_sensor_init)(void);
    int (*gravity_sensor_sensity)(unsigned char gsid);
//    void (*gravity_sensor_interrupt)(void *pr);
//    int  (*gravity_sensor_read)(unsigned char reg);
//    int  (*gravity_sensor_wirte)(unsigned char reg, unsigned char reg_dat);

} _G_SENSOR_INTERFACE;


typedef struct gravity_sensor_device {

    struct device dev;
    _G_SENSOR_INTERFACE *gsensor_ops;

} G_SENSOR_INTERFACE;

struct gsensor_platform_data {
    const char *iic;
};


extern G_SENSOR_INTERFACE  gsensor_dev_begin[];
extern G_SENSOR_INTERFACE gsensor_dev_end[];
//G_SENSOR_INTERFACE gsensor_dev_begin[];
//G_SENSOR_INTERFACE gsensor_dev_end[];

#define REGISTER_GRAVITY_SENSOR(gSensor) \
	static G_SENSOR_INTERFACE gSensor SEC_USED(.gsensor_dev) = {


#define list_for_each_gsensor(c) \
	for (c=gsensor_dev_begin; c<gsensor_dev_end; c++)



//#define GSENSOR_PLATFORM_DATA_BEGIN(data) \
//	G_SENSOR_INTERFACE data = { \
//
//
//#define GSENSOR_PLATFORM_DATA_END() \
//	};




enum {
    //分辨率选择 高 中 低
    G_SENSITY_HIGH,
    G_SENSITY_MEDIUM,
    G_SENSITY_LOW,

};


enum {
    //工作模式选择 正常  低电  暂停
    G_NORMAL_MODE,
    G_LOW_POWER_MODE,
    G_SUSPEND_MODE,
};


enum {
    //gsensor菜单回调函数使用
    G_SENSOR_CLOSE = 0,
    G_SENSOR_LOW,
    G_SENSOR_MEDIUM,
    G_SENSOR_HIGH,
    G_SENSOR_SCAN,

    G_SENSOR_LOW_POWER_MODE,
};

extern const struct device_operations gsensor_dev_ops;

extern u8 get_gsen_active_flag();
extern void clear_gsen_active_flag();
extern void set_gse_sensity(u8 sensity);
extern void set_parking_guard_wkpu(u8 park_guad);//关机前配置重力感应唤醒

extern u8 get_park_guard_status(u8 park_guard_en);
extern void usb_online_clear_park_wkup();
extern void set_park_guard(u8 p_flag);//开机时 记录是否是停车守卫唤醒

#endif // _GSENSOR_MANAGE_H


