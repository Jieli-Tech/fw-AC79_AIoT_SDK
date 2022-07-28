#include "gSensor_manage.h"
#include "da380.h"


#ifdef CONFIG_GSENSOR_ENABLE

unsigned char da380_sensor_command(unsigned char register_address, unsigned char function_command)//往da380某寄存器写命令
{
    gravity_sensor_command(WRITE_COMMAND_FOR_DA380, register_address, function_command);
    return 0;
}

unsigned char da380_sensor_get_data(unsigned char register_address)//获取da380 sensor  数据
{
    return _gravity_sensor_get_data(WRITE_COMMAND_FOR_DA380, READ_COMMAND_FOR_DA380, register_address);
}



//映射active interrupt to INT pin
void da380_int_map1(unsigned char map_int)
{
    switch (map_int) {
    case 0x04://active interrupt to INT
        da380_sensor_command(INT_MAP1, 0x04);
        break;
    }
}


#if 0
///获取当前三轴加速度值
void da380_get_acceleration(void)
{
///三轴加速度

    u16 acc_x, acc_y, acc_z;
    u8 acc_x_lsb, acc_x_msb;
    u8 acc_y_lsb, acc_y_msb;
    u8 acc_z_lsb, acc_z_msb;

    acc_x_lsb = da380_sensor_get_data(ACC_X_LSB);
    puts("\nACC_X_LSB : ");
    put_u16hex(acc_x_lsb);
    acc_x_msb = da380_sensor_get_data(ACC_X_MSB);
    puts("\nACC_X_MSB : ");
    put_u16hex(acc_x_msb);

    acc_x = (acc_x_lsb | (acc_x_msb << 8));

    acc_y_lsb = da380_sensor_get_data(ACC_Y_LSB);
    puts("\nACC_Y_LSB : ");
    put_u16hex(acc_y_lsb);
    acc_y_msb = da380_sensor_get_data(ACC_Y_MSB);
    puts("\nACC_Y_MSB : ");
    put_u16hex(acc_y_msb);

    acc_y = (acc_y_lsb | (acc_y_msb << 8));

    acc_z_lsb = da380_sensor_get_data(ACC_Y_LSB);
    puts("\nACC_Z_LSB : ");
    put_u16hex(acc_z_lsb);
    acc_z_msb = da380_sensor_get_data(ACC_Y_MSB);
    puts("\nACC_Z_MSB : ");
    put_u16hex(acc_z_msb);
    acc_z = (acc_z_lsb | (acc_z_msb << 8));
}
#endif

void da380_init(void)//config RESOLUTION_RANGE ,MODE_BW,INT_MAP1, ACTIVE_DUR, ACTIVE_THS
{


    printf("da380_init\r\n");
    /* da380_sensor_command(RESET_DA380, 0x20); */
//    init_i2c_io();

    /* os_time_dly(10); */

    da380_resolution_range(G_SENSITY_LOW);
    da380_sensor_command(ODR_AXIS,     0x06);//enable X/Y/Z axis,1000Hz
    da380_work_mode(G_LOW_POWER_MODE);//normal mode, 500Hz
    da380_sensor_command(SWAP_POLARITY, 0x00); //remain the polarity of X/Y/Z-axis

    /*INT中断配置， 分别有三种使用方式使能z轴，使能z和y轴，或者使能z,y和x，这三种方式*/

//    da380_sensor_command(INT_SET1,     0x04);//disable orient interrupt.enable the active interrupt for the  z,axis
//    da380_sensor_command(INT_SET1,     0x07);//disable orient interrupt.enable the active interrupt for the  z, y and x,axis
    da380_sensor_command(INT_SET1,     0x27);//disable orient interrupt.enable the active interrupt for the  z, y and x,axis
    da380_sensor_command(INT_SET2,     0x00);//disable the new data interrupt and the freefall interupt
    da380_int_map1(0x04);             //mapping active interrupt to INT
    da380_sensor_command(INT_MAP2,     0x00);//doesn't mappint new data interrupt to INT
    da380_sensor_command(INT_CONFIG,   0x01);//push-pull output for INT ,selects active level high for pin INT
    da380_sensor_command(INT_LTACH,    0x0E);///Burgess_151210
    da380_sensor_command(FREEFALL_DUR, 0x09);//freefall duration time = (freefall_dur + 1)*2ms
    da380_sensor_command(FREEFALL_THS, 0x30);//default is 375mg
    da380_sensor_command(FREEFALL_HYST, 0x01);
    da380_sensor_command(ACTIVE_DUR,   0x11);//Active duration time = (active_dur + 1) ms
    da380_sensor_command(ACTIVE_THS,   0x8F);
    da380_sensor_command(TAP_DUR,      0x04);//
    da380_sensor_command(TAP_THS,      0x0a);
    da380_sensor_command(ORIENT_HYST,  0x18);
    da380_sensor_command(Z_BLOCK,      0x08);
    da380_sensor_command(SELF_TEST,    0x00);//close self_test
    da380_sensor_command(CUSTOM_OFF_X, 0x00);
    da380_sensor_command(CUSTOM_OFF_Y, 0x00);
    da380_sensor_command(CUSTOM_OFF_Z, 0x00);
    da380_sensor_command(CUSTOM_FLAG,  0x00);
    da380_sensor_command(CUSTOM_CODE,  0X00);
    da380_sensor_command(Z_ROT_HODE_TM, 0x09);
    da380_sensor_command(Z_ROT_DUR,    0xFF);
    da380_sensor_command(ROT_TH_H,     0x45);
    da380_sensor_command(ROT_TH_L,     0x35);
    /*puts(" dac380 init inti xxxxxxxxxxx\n");*/
}


//工作分辨率选着
void da380_resolution_range(unsigned char range)
{
    switch (range) {
    case G_SENSITY_HIGH:
        da380_sensor_command(RESOLUTION_RANGE, 0x0C);//14bit +/-2g     对应分辨率4096 LSB/g   高
        break;
    case G_SENSITY_MEDIUM:
        da380_sensor_command(RESOLUTION_RANGE, 0x0D);//14bit +/-4g     对应分辨率2048 LSB/g   中
        break;
    case G_SENSITY_LOW:
        /* da380_sensor_command(RESOLUTION_RANGE, 0x0F);//14bit +/-16g    对应分辨率512 LSB/g */
        da380_sensor_command(RESOLUTION_RANGE, 0x0E);//14bit +/-16g    对应分辨率512 LSB/g
        break;
    default :
        da380_sensor_command(RESOLUTION_RANGE, 0x0C);//14bit +/-2g     对应分辨率4096 LSB/g   高
        break;
    }
}


//工作模式选着
void da380_work_mode(unsigned char work_mode)
{
    switch (work_mode) {
    case G_NORMAL_MODE:
        da380_sensor_command(MODE_BW, 0X1E);//normal mode
        break;
    case G_LOW_POWER_MODE:
        da380_sensor_command(MODE_BW, 0X5E);//low power mode
        break;
    case G_SUSPEND_MODE:
        da380_sensor_command(MODE_BW, 0X9E);//suspend mode
        break;
    }
}

extern int da380_pin_int_interrupt();
/**提供给二级子菜单回调函数使用*/
int da380_gravity_sensity(unsigned char gsid)
{

    if (gsid == G_SENSOR_SCAN) {
        if (da380_pin_int_interrupt()) {
            return -EINVAL;
        }
        return 0;
    }

    if (gsid == G_SENSOR_HIGH) {
        da380_work_mode(G_NORMAL_MODE);//normal mode
        da380_resolution_range(G_SENSITY_HIGH);
        da380_sensor_command(INT_CONFIG,   0x01);//push-pull output for INT ,selects active level high for pin INT
        da380_sensor_command(ACTIVE_THS, 0x3F);//0x4F);
    }

    if (gsid == G_SENSOR_MEDIUM) {
        da380_work_mode(G_NORMAL_MODE);//normal mode
        da380_resolution_range(G_SENSITY_MEDIUM);
        da380_sensor_command(INT_CONFIG,   0x01);//push-pull output for INT ,selects active level high for pin INT
        da380_sensor_command(ACTIVE_THS, 0x5F);//0x8F);
    }

    if (gsid == G_SENSOR_LOW) {
        da380_work_mode(G_NORMAL_MODE);//normal mode
        da380_resolution_range(G_SENSITY_LOW);
        /* da380_resolution_range(G_SENSITY_MEDIUM); */
        da380_sensor_command(INT_CONFIG,   0x01);//push-pull output for INT ,selects active level high for pin INT
        da380_sensor_command(ACTIVE_THS, 0xaF);//0xFF);
    }

    if (gsid == G_SENSOR_CLOSE) {
        /* da380_sensor_command(RESET_DA380, 0x20); */
        da380_init();
        da380_resolution_range(G_SENSITY_LOW);
        da380_sensor_command(ACTIVE_THS, 0x00);//0xFF);
        da380_sensor_command(INT_CONFIG,   0x00);//no output for INT
        da380_work_mode(G_SUSPEND_MODE);//暂停
    }

    if (gsid == G_SENSOR_LOW_POWER_MODE) { //低功耗
        /* da380_sensor_command(RESET_DA380, 0x20); */
        /* delay(5000); */
        /* da380_init(); */
        da380_work_mode(G_LOW_POWER_MODE);
        da380_resolution_range(G_SENSITY_MEDIUM);
        da380_sensor_command(INT_CONFIG,   0x01);//push-pull output for INT ,selects active level high for pin INT
        /* da380_resolution_range(G_SENSITY_HIGH); */
        da380_sensor_command(ACTIVE_THS, 0xAF);//0xFF);
        da380_sensor_command(INT_LTACH, 0x0F);//latched
    }
    return 0;
}

/**
返回值 TRUE  重力传感器触发
       FALSE  重力传感器未触发
*/
unsigned char get_da380_int_state()
{
    unsigned char date_tmp;
    date_tmp = da380_sensor_get_data(MOTION_FLAG);
    if (date_tmp & 0x24) {
        return TRUE;
    }

    return false;
}
/**加速度超过与设定的阀值，产生中断。为当前视频文件上锁*/
int da380_pin_int_interrupt()
{

    if (get_da380_int_state()) {
        return 0;
    }
    return -EINVAL;
}
/**返回0 id正确*/
char da380_id_check(void)
{
    unsigned char chipid = 0x00;

    chipid = da380_sensor_get_data(CHIPID_DA380);

    printf("CHIPID_da379: %x\n", chipid);
    if (chipid != 0x13) {
        puts("not da380 \n");
        return -1;
    }

    return 0;
}


/**返回0 id正确*/
s8 da380_check(void)
{
    u32 i;
    da380_sensor_command(RESET_DA380, 0x20);
    for (i = 0; i < 5; i++) {
        delay(10000);
        if (!da380_id_check()) {
            return 0;
        }
    }
    return -1;
}

_G_SENSOR_INTERFACE da380_ops = {
    .logo 				    = 	"da380",
    .gravity_sensor_check   =   da380_check,
    .gravity_sensor_init    =   da380_init,
    .gravity_sensor_sensity =   da380_gravity_sensity,
//    .gravity_sensor_interrupt = da380_pin_int_interrupt,
//    .gravity_sensor_read    = da380_sensor_get_data,
//    .gravity_sensor_write   = da380_sensor_command,
};

REGISTER_GRAVITY_SENSOR(da380)

.gsensor_ops = &da380_ops,

};

#endif // CONFIG_GSENSOR_ENABLE


