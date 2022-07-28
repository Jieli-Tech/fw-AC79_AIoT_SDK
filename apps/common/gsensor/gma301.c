
#include "gSensor_manage.h"
#include "gma301.h"

#ifdef CONFIG_GSENSOR_ENABLE

u8 gma301_sensor_command(u8 register_address, u8 function_command)
{
    gravity_sensor_command(WRITE_COMMAND_FOR_GMA301, register_address, function_command);
    return 0;
}

u8 gma301_sensor_get_data(u8 register_address)
{
    return _gravity_sensor_get_data(WRITE_COMMAND_FOR_GMA301, READ_COMMAND_FOR_GMA301, register_address);
}


void gma301_init(void)
{
//    puts("\n gma301_init\n");
    gma301_sensor_command(0x21, 0x52); //RESET


    gma301_sensor_command(0x00, 0x02); //This will download OTP data into internal parameter registers and
    gma301_sensor_command(0x00, 0x12); //enable internal clock then clear the data path.
    gma301_sensor_command(0x00, 0x02); //
    gma301_sensor_command(0x00, 0x82); //
    gma301_sensor_command(0x00, 0x02); //
    gma301_sensor_command(0x1F, 0x28); //Select the calibrated data as output
    //gma301_sensor_command(0x1F,0x08);//Select the decimation filter as output 2014-12-29 alter by zhuo
    gma301_sensor_command(0x0F, 0x00); // set tap number register 2014-12-29 add by zhuo
    gma301_sensor_command(0x11, 0x07); //
    gma301_sensor_command(0x0C, 0x8f); // 0x8f to enable enz eny enx ent  / 0x8d  to enable z y ent / 0x89 to enable z ent
    gma301_sensor_command(0x00, 0x06); //enable sensor  and output


//interrupt init
//    gma301_sensor_command(0x11,0x07);//IIC 0X07 for no pull up //0x06 High active  0x04 low active
#if 0
//    gma301_sensor_command(0x38,0X53);//用来配置加速度值的大小//10 1g 20 2g 30 3g 40 4g 50 5g 60 6g 最大60 ，在gma301_resolution_range中配置
//    gma301_sensor_command(0x39,0X60);//10 1g 20 2g 30 3g 40 4g 50 5g 60 6g 最大60

    delay_2ms(1);

    gma301_sensor_command(0x0E, 0x00); //0x1C//0x00 // 0x00:disable interrupt

    delay_2ms(1); //2014_0819 added 1ms delay for micro motion setup itself.

    gma301_sensor_command(0x0E, 0x1C); //To enable interrupt.
    gma301_sensor_get_data(0x1C);//clear INT status
    gma301_sensor_get_data(0x1D);
#endif
    puts("\n gma301_init end\n");

}



s8 gma301_id_check(void)
{
    int chipid;

    chipid = gma301_sensor_get_data(0x12);//chip id

    log_d("CHIPID_GMA301: %x", chipid);

    if (chipid != 0x55) {
        puts("not gma301");
        return -1;
    }

    return 0;
}


s8 gma301_check(void)
{

//    init_i2c_io();

//    gma301_iic_set(1);

//    gma301_sensor_command(0x21,0x52);//RESET

    if (0 != gma301_id_check()) {
        return -1;
    }

    return 0;
}


//工作模式选着
void gma301_work_mode(u8 work_mode)
{
    switch (work_mode) {
    case G_NORMAL_MODE:
        gma301_sensor_command(0x00, 0X06);//normal mode
        break;
    case G_SUSPEND_MODE:
        gma301_sensor_command(0x00, 0X00);
        break;
    case G_LOW_POWER_MODE:
//            gma301_sensor_command(0x11, 0X07);//INT  active H
        gma301_sensor_command(0x00, 0X06);//low_power mode
        gma301_sensor_command(0x0C, 0x8F);
        gma301_sensor_command(0x0D, 0X50);//感度

        break;
    }
}


//工作分辨率选着
void gma301_resolution_range(u8 range)
{
    //gma301_sensor_command(0x00, 0X02);
    gma301_sensor_command(0x11, 0x07);
    gma301_sensor_command(0x0F, 0X00);
//        gma301_sensor_command(0x0D, 0X50);
    gma301_sensor_command(0x0E, 0X00);
    gma301_sensor_command(0x1F, 0X28);
    switch (range) {
    case G_SENSITY_HIGH:
//        gma301_sensor_command(0x38, 0x3F);// 高灵敏度 2g
        puts("\n G_SENSITY_HIGH \n");
        gma301_sensor_command(0x0D, 0X70);//感度 70 60 50 44 34 24 14
        gma301_sensor_command(0x39, 0x18);// 高灵敏度 1.5g
        break;
    case G_SENSITY_MEDIUM:
//        gma301_sensor_command(0x38, 0x7F);// 中  4g
        puts("\n G_SENSITY_MEDIUM \n");
        gma301_sensor_command(0x0D, 0X50);//感度 34
        gma301_sensor_command(0x39, 0x38);// 中  3.5g
        break;
    case G_SENSITY_LOW:
        puts("\n G_SENSITY_LOW \n");
//        gma301_sensor_command(0x38, 0xFF);// 低 8g
        gma301_sensor_command(0x0D, 0X44);//感度 34
        gma301_sensor_command(0x39, 0x48);// 低 5.5g
        break;
    }
//        puts("\n gma301_sensor_get_data 1 \n");
    gma301_sensor_get_data(0x12);//
    gma301_sensor_get_data(0x13);
    gma301_sensor_get_data(0x14);//X
    gma301_sensor_get_data(0x15);
    gma301_sensor_get_data(0x16);//Y
    gma301_sensor_get_data(0x17);
    gma301_sensor_get_data(0x18);//Z
    gma301_sensor_get_data(0x19);
//        puts("\n gma301_sensor_get_data 2 \n");

//        delay_2ms(2); //2014_0819 added 1ms delay for micro motion setup itself.
    delay(1000);
//        puts("\n gma301_sensor_get_data 3 \n");
    gma301_sensor_command(0x1F, 0x38);
//        gma301_sensor_command(0x11,0x07);
//        puts("\n gma301_sensor_get_data 4 \n");
//        delay_2ms(1);
//        delay(1000);
//        puts("\n gma301_sensor_get_data 5 \n");
    gma301_sensor_get_data(0x1C);//clear INT status
    gma301_sensor_get_data(0x1D);
    gma301_sensor_command(0x0E, 0x1C);
    //gma301_sensor_command(0x00, 0X06);
//        puts("\n gma301_resolution_range \n");

}

extern int gma301_pin_int_interrupt();
int gma301_gravity_sensity(u8 gsid)
{

    if (gsid == G_SENSOR_SCAN) {
        if (gma301_pin_int_interrupt()) {
            return -EINVAL;
        }
        return 0;
    }

    //高中低
    if (gsid == G_SENSOR_HIGH) {
        gma301_work_mode(G_NORMAL_MODE);
        gma301_resolution_range(G_SENSITY_HIGH);
    }

    if (gsid == G_SENSOR_MEDIUM) {
        gma301_work_mode(G_NORMAL_MODE);
        gma301_resolution_range(G_SENSITY_MEDIUM);
    }

    if (gsid == G_SENSOR_LOW) {
        gma301_work_mode(G_NORMAL_MODE);
        gma301_resolution_range(G_SENSITY_LOW);
    }

    if (gsid == G_SENSOR_CLOSE) {
        gma301_sensor_get_data(0x1c);
        gma301_sensor_get_data(0x1D);
        os_time_dly(50);

        gma301_sensor_command(0x38, 0XFF); //关闭
        gma301_work_mode(G_SUSPEND_MODE);//暂停

    }

    if (gsid == G_SENSOR_LOW_POWER_MODE) { //低功耗

        gma301_sensor_get_data(0x1c);
        gma301_sensor_get_data(0x1D);
        os_time_dly(50);

        gma301_work_mode(G_LOW_POWER_MODE);
        gma301_resolution_range(G_SENSITY_MEDIUM);
    }



    return 0;

}

//u8 lock_current_file;


int gma301_pin_int_interrupt()
{
    u8 temp;
    temp = gma301_sensor_get_data(0x12);
    temp = gma301_sensor_get_data(0x13);
    if (temp & BIT(5)) { //INT
//        if(!gma301_sensor_get_data(0x1c))
//        {
//            gma301_init();
//            return;
//        }
//        if(!gma301_sensor_get_data(0x1D))
//        {
//            gma301_init();
//            return;
//        }
        gma301_sensor_get_data(0x1c);
        gma301_sensor_get_data(0x1D);

        return 0;
    }
    return -EINVAL;
}

void gma301_test(void)
{
//    temp = gma301_sensor_get_data(0x12);
//    put_u32hex(temp);
//    printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>gma301  0x12:%8x\n",temp);
//
//    temp = gma301_sensor_get_data(0x13);
//
//
//    if (temp & BIT(5))
//    {
//        put_u32hex(temp);
//        gma301_sensor_get_data(0x1c);
//        gma301_sensor_get_data(0x1D);
//        putbyte('\n');
//    }
//    gui_number(DVcNumber2_3,temp, -1, -1, ERASE_BACKGROUND);
//    gui_number(DVcNumber2_3,temp, -1, -1, USE_ORIG_COLOR);


//    printf("\n>gma301 0x13:%8x\n",temp);

//    temp = gma301_sensor_get_data(0x1c);
//    put_u32hex(temp);
//
//    temp = gma301_sensor_get_data(0x14);//X
//    put_u32hex(temp);
//    temp = gma301_sensor_get_data(0x16);//Y
//    put_u32hex(temp);
//    temp = gma301_sensor_get_data(0x18);//Z
//    put_u32hex(temp);
//
//    printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>gma301 0x1c:%8x\n",temp);
}

_G_SENSOR_INTERFACE gma301_ops = {
    .logo 				    = 	"gma301",
    .gravity_sensor_check   =   gma301_check,
    .gravity_sensor_init    =   gma301_init,
    .gravity_sensor_sensity =   gma301_gravity_sensity,
//    .gravity_sensor_interrupt = gma301_pin_int_interrupt,
//    .gravity_sensor_read    = gma301_sensor_get_data,
//    .gravity_sensor_write   = gma301_sensor_command,
};
REGISTER_GRAVITY_SENSOR(gma301)
.gsensor_ops = &gma301_ops,
};

#endif // CONFIG_GSENSOR_ENABLE
