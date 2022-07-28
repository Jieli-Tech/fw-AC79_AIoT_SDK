

#ifndef GMA301_H
#define GMA301_H

#include "gSensor_manage.h"

extern unsigned char gma301_sensor_command(u8 register_address, u8 function_command);//向register_address 写入命令function_command
extern unsigned char gma301_sensor_get_data(u8 register_address);//读取register_address地址的数据


extern void gma301_init(void);
extern void gma301_test(void);

extern char gma301_check(void);
extern int gma301_gravity_sensity(unsigned char gsid);
extern int gma301_pin_int_interrupt();
extern void delay_2ms(int cnt);

#define WRITE_COMMAND_FOR_GMA301       0x30
#define READ_COMMAND_FOR_GMA301        0x31

#endif
