#include "gSensor_manage.h"
#include "msa300.h"

#ifdef CONFIG_GSENSOR_ENABLE

u8 msa300_sensor_command(u8 register_address, u8 function_command)//往da380某寄存器写命令
{
    gravity_sensor_command(WRITE_COMMAND_FOR_MSA300, register_address, function_command);

    return 0;
}

u8 msa300_sensor_get_data(u8 register_address)//获取da380 sensor  数据
{
    return _gravity_sensor_get_data(WRITE_COMMAND_FOR_MSA300, READ_COMMAND_FOR_MSA300, register_address);
}


//映射active interrupt to INT pin
void msa300_int_map1(u8 map_int)
{
    switch (map_int) {
    case 0x04://active interrupt to INT
        msa300_sensor_command(MSA300_INT_MAP1, 0x04);
        break;
    }
}


#if 1
///获取当前三轴加速度值
void msa300_get_acceleration(void)
{
///三轴加速度
    volatile static s16 pacc_x, pacc_y, pacc_z;
    s16 acc_x, acc_y, acc_z;
    u8 acc_x_lsb, acc_x_msb;
    u8 acc_y_lsb, acc_y_msb;
    u8 acc_z_lsb, acc_z_msb;

    acc_x_lsb = msa300_sensor_get_data(MSA300_ACC_X_LSB);
    acc_x_msb = msa300_sensor_get_data(MSA300_ACC_X_MSB);

    acc_x = (acc_x_lsb >> 2) | ((acc_x_msb & 0x7F) << 6);
    if (acc_x_msb & BIT(7)) {
        acc_x = -((acc_x - 1) ^ 0x1fff);
    }


    acc_y_lsb = msa300_sensor_get_data(MSA300_ACC_Y_LSB);
    acc_y_msb = msa300_sensor_get_data(MSA300_ACC_Y_MSB);

    acc_y = (acc_y_lsb >> 2) | ((acc_y_msb & 0x7F) << 6);
    if (acc_y_msb & BIT(7)) {
        acc_y = -((acc_y - 1) ^ 0x1fff);
    }


    acc_z_lsb = msa300_sensor_get_data(MSA300_ACC_Y_LSB);
    acc_z_msb = msa300_sensor_get_data(MSA300_ACC_Y_MSB);


    acc_z = (acc_z_lsb >> 2) | ((acc_z_msb & 0x7F) << 6);
    if (acc_z_msb & BIT(7)) {
        acc_z = -((acc_z - 1) ^ 0x1fff);
    }
//    puts("\n ACC_Z : ");
//    put_u16hex(acc_z);


//printf("\n x:%d,   y:%d,   z:%d  \n",acc_x, acc_y,acc_z);
//printf("\n x:%d,   y:%d,   z:%d \n",abs(acc_x - pacc_x),abs(acc_y- pacc_y),abs(acc_z-pacc_z));

    pacc_x = acc_x;
    pacc_y = acc_y;
    pacc_z = acc_z;

}
#endif

void msa300_init(void)//config RESOLUTION_RANGE ,MODE_BW,INT_MAP1, ACTIVE_DUR, ACTIVE_THS
{
    msa300_resolution_range(G_SENSITY_LOW);
    msa300_sensor_command(MSA300_ODR_AXIS, 0x06);//enable X/Y/Z axis,62.5Hz
    msa300_work_mode(G_LOW_POWER_MODE);//低功耗模式
    msa300_sensor_command(MSA300_SWAP_POLARITY, 0x00); //remain the polarity of X/Y/Z-axis

    /*INT中断配置， 分别有三种使用方式使能z轴，使能z和y轴，或者使能z,y和x，这三种方式*/

    msa300_sensor_command(MSA300_INT_SET0,     0x27);//disable orient interrupt. enable the tap and active interrupt for the  z, y and x,axis
    msa300_sensor_command(MSA300_INT_SET1,     0x00);//disable the new data interrupt and the freefall interupt

    msa300_sensor_command(MSA300_INT_MAP0,     0x44);////mapping active interrupt to INT1
    msa300_sensor_command(MSA300_INT_MAP1,     0x00);//doesn't mappint new data interrupt to INT

    msa300_sensor_command(MSA300_INT_MAP2,     0x00);//doesn't mappint active 、tap\orient\freefall interrupt to INT2
    msa300_sensor_command(MSA300_INT_MAP3,     0x05);//int1 2 push-pull active high

    msa300_sensor_command(MSA300_INT_LTACH,    0x0E);// latch 100ms
    msa300_sensor_command(MSA300_FREEFALL_DUR, 0x09);//freefall duration time = (freefall_dur + 1)*2ms  default
    msa300_sensor_command(MSA300_FREEFALL_THS, 0x30);//default is 375mg
    msa300_sensor_command(MSA300_FREEFALL_HYST, 0x01);

    msa300_sensor_command(MSA300_ACTIVE_DUR,   0x11);//Active duration time = (active_dur + 1) ms
    msa300_sensor_command(MSA300_ACTIVE_THS,   0x8F);

    msa300_sensor_command(MSA300_TAP_DUR,      0x04);//
    msa300_sensor_command(MSA300_TAP_THS,      0x0a);
    msa300_sensor_command(MSA300_ORIENT_HYST,  0x18);
    msa300_sensor_command(MSA300_Z_BLOCK,      0x08);


    msa300_sensor_command(MSA300_CUSTOM_OFF_X, 0x00);//校正补偿
    msa300_sensor_command(MSA300_CUSTOM_OFF_Y, 0x00);
    msa300_sensor_command(MSA300_CUSTOM_OFF_Z, 0x00);
}


//工作分辨率选着
void msa300_resolution_range(u8 range)
{
    switch (range) {
    case G_SENSITY_HIGH:
        msa300_sensor_command(MSA300_RESOLUTION_RANGE, 0x00);//14bit +/-2g     对应分辨率4096 LSB/g   高
        break;
    case G_SENSITY_MEDIUM:
        msa300_sensor_command(MSA300_RESOLUTION_RANGE, 0x01);//14bit +/-4g     对应分辨率2048 LSB/g   中
        break;
    case G_SENSITY_LOW:
        msa300_sensor_command(MSA300_RESOLUTION_RANGE, 0x02);//14bit +/-8g    对应分辨率512 LSB/g
        break;
    }
}


//工作模式选着
void msa300_work_mode(u8 work_mode)
{
    switch (work_mode) {
    case G_NORMAL_MODE:
        msa300_sensor_command(MSA300_ODR_AXIS, 0x07);
        msa300_sensor_command(MSA300_MODE_BW, 0X0E);//normal mode 62.5HZ
        break;
    case G_LOW_POWER_MODE:
        msa300_sensor_command(MSA300_ODR_AXIS, 0x05);
        msa300_sensor_command(MSA300_MODE_BW, 0X4A);//low power mode 15.63hz
        break;
    case G_SUSPEND_MODE:
//         msa300_sensor_command(MSA300_INT_MAP3, 0x00);//int1 2 push-pull active high

//        msa300_sensor_command(MSA300_INT_LTACH, 0x80);// RESET latch
        msa300_sensor_command(MSA300_ODR_AXIS, 0xE2);
        msa300_sensor_command(MSA300_MODE_BW, 0XC0);//suspend mode  1.95HZ
        break;
    }
}

extern int  msa300_pin_int_interrupt();
/**提供给二级子菜单回调函数使用*/
int msa300_gravity_sensity(u8 gsid)
{

    if (gsid == G_SENSOR_SCAN) {
        if (msa300_pin_int_interrupt()) {
            return -EINVAL;
        }
        return 0;
    }

    if (gsid == G_SENSOR_HIGH) {
        puts("\n msa300 G_SENSOR_HIGH \n");
        msa300_work_mode(G_NORMAL_MODE);//normal mode
        msa300_resolution_range(G_SENSITY_HIGH);
        msa300_sensor_command(MSA300_ACTIVE_THS, 0x3F);//0x4F);
    }

    if (gsid == G_SENSOR_MEDIUM) {
        puts("\n msa300 G_SENSOR_HIGH \n");
        msa300_work_mode(G_NORMAL_MODE);//normal mode
        msa300_resolution_range(G_SENSITY_MEDIUM);
        msa300_sensor_command(MSA300_ACTIVE_THS, 0x5F);//0x8F);
    }

    if (gsid == G_SENSOR_LOW) {
        puts("\n msa300 G_SENSOR_LOW \n");
        msa300_work_mode(G_NORMAL_MODE);//normal mode
        msa300_resolution_range(G_SENSITY_LOW);
        msa300_sensor_command(MSA300_ACTIVE_THS, 0x6F);//0xFF);
    }

    if (gsid == G_SENSOR_CLOSE) {
        puts("\n msa300 G_SENSOR_CLOSE \n");
        msa300_work_mode(G_SUSPEND_MODE);//暂停
    }

    if (gsid == G_SENSOR_LOW_POWER_MODE) { //低功耗
        puts("\n msa300 G_SENSOR_LOW_POWER_MODE \n");
        msa300_work_mode(G_NORMAL_MODE);
        msa300_sensor_command(MSA300_INT_LTACH, 0x0F);//latched
        msa300_resolution_range(G_SENSITY_MEDIUM);
    }



    return 0;
}

/**
返回值 TRUE  重力传感器触发
       FALSE  重力传感器未触发
*/
u8 get_msa300_int_state()
{
    u8 date_tmp;

    date_tmp = msa300_sensor_get_data(MSA300_MOTION_FLAG);
    if (date_tmp & 0x24) { //  BIT0 FreeFall BIT2 Active BIT4 Double Tap BI5 Tap BIT6 Orient
        return TRUE;
    }

    return FALSE;
}
/**加速度超过与设定的阀值，产生中断。为当前视频文件上锁*/
int  msa300_pin_int_interrupt()
{
    if (get_msa300_int_state()) {
        return 0;
    }
    return -EINVAL;
}
/**返回0 id正确*/
s8 msa300_id_check(void)
{
    u8 chipid = 0x00;
    chipid = msa300_sensor_get_data(CHIPID_MSA300);
    log_d("CHIPID_msa300: %x\n", chipid);

    if (chipid != 0x13) {
        puts("not msa300\n");
        return -1;
    }

    return 0;
}


/**返回0 id正确*/
s8 msa300_check(void)
{

    if (0 != msa300_id_check()) {
        return -1;
    }

    return 0;
}

_G_SENSOR_INTERFACE msa300_ops = {
    .logo 						= "msa300",
    .gravity_sensor_check 		= msa300_check,
    .gravity_sensor_init 		= msa300_init,
    .gravity_sensor_sensity 	= msa300_gravity_sensity,
//	.gravity_sensor_interrupt	= msa300_pin_int_interrupt,
};

REGISTER_GRAVITY_SENSOR(msa300)
.gsensor_ops = &msa300_ops,

};



#endif // CONFIG_GSENSOR_ENABLE




