

#ifndef _MSA300_H
#define _MSA300_H



#define WRITE_COMMAND_FOR_MSA300       0x4C
#define READ_COMMAND_FOR_MSA300       0x4D


//Read Only
#define RESET_MSA300         0x00
#define CHIPID_MSA300        0x01
#define MSA300_ACC_X_LSB           0x02
#define MSA300_ACC_X_MSB           0x03
#define MSA300_ACC_Y_LSB           0x04
#define MSA300_ACC_Y_MSB           0x05
#define MSA300_ACC_Z_LSB           0x06
#define MSA300_ACC_Z_MSB           0x07
#define MSA300_MOTION_FLAG         0x09
#define MSA300_NEWDATA_FLAG        0x0A

#define MSA300_TAP_ACTIVE_STATUS   0x0B//Read Only
#define MSA300_ORIENT_STATUS       0x0C

//R/W
#define MSA300_RESOLUTION_RANGE    0x0F//
#define MSA300_ODR_AXIS            0x10
#define MSA300_MODE_BW             0x11
#define MSA300_SWAP_POLARITY       0x12
#define MSA300_INT_SET0            0x16//R/W
#define MSA300_INT_SET1            0x17

#define MSA300_INT_MAP0            0x19
#define MSA300_INT_MAP1            0x1A
#define MSA300_INT_MAP2            0x1B
#define MSA300_INT_MAP3            0x20
//#define INT_CONFIG          0x20
#define MSA300_INT_LTACH           0x21
#define MSA300_FREEFALL_DUR        0x22
#define MSA300_FREEFALL_THS        0x23
#define MSA300_FREEFALL_HYST       0x24
#define MSA300_ACTIVE_DUR          0x27
#define MSA300_ACTIVE_THS          0x28
#define MSA300_TAP_DUR             0x2A//
#define MSA300_TAP_THS             0x2B//
#define MSA300_ORIENT_HYST         0x2C
#define MSA300_Z_BLOCK             0x2D


#define MSA300_CUSTOM_OFF_X        0x38
#define MSA300_CUSTOM_OFF_Y        0x39
#define MSA300_CUSTOM_OFF_Z        0x3A






extern u8 msa300_sensor_command(u8 register_address, u8 function_command);//向register_address 写入命令function_command
extern u8 msa300_sensor_get_data(u8 register_address);//读取register_address地址的数据

extern void msa300_work_mode(u8 work_mode);// normal mode , low-power mode and suspend mode,three different power modes
extern void msa300_motion_mode_interrupt(void);
extern void msa300_resolution_range(u8 range);




extern s8 msa300_check(void);
extern void msa300_init(void);
extern int msa300_gravity_sensity(u8 gsid);
extern int msa300_pin_int_interrupt();


#endif


