#include "gSensor_manage.h"
#include "stk8312.h"

#ifdef CONFIG_GSENSOR_ENABLE

#define WRITE_COMMAND_FOR_STK8312   (0x3D << 1)
#define READ_COMMAND_FOR_STK8312    ((0x3D << 1) | BIT(0))

int stk8312_sensor_command(u8 register_address, u8 function_command)//往stk8312某寄存器写命令
{
    gravity_sensor_command(WRITE_COMMAND_FOR_STK8312, register_address, function_command);
    return 0;
}

int stk8312_sensor_get_data(u8 register_address)//获取stk8312 sensor  数据
{
    return _gravity_sensor_get_data(WRITE_COMMAND_FOR_STK8312, READ_COMMAND_FOR_STK8312, register_address);
}


void stk8312_set_enable(int en_able)
{
    u8 RegAddr, RegWriteValue;
    u8 readvalue = 0;

    if (en_able) {
        //---------------
        //Set to Active mode
        //---------------
        //0x07: Mode Register (Read/Write)
        //D7        	D6      	D5      		D4      		D3      		D2          	D1      		D0
        //IAH           IPP         Reserved        Reserved        Reserved        Reserved    	Reserved      	Mode
        //
        RegAddr       = 0x07;   //set Register Address
        RegWriteValue = 0xC1;   //INT# active-high, push-pull, Active-Mode
//        RegWriteValue = 0x81;	//Interrupt active high, open-drain, active-mode
        stk8312_sensor_command(RegAddr, RegWriteValue);
//		os_time_dly(5);

        stk8312_sensor_command(0x24, 0xdc);
//#if 0
//        os_time_dly(5);
//        //---------------
//        //Engineering Setting
//        //---------------
//
//        stk8312_sensor_command(0x3D, 0x70);
//        stk8312_sensor_command(0x3D, 0x70);
//        stk8312_sensor_command(0x3F, 0x02);
//
//        os_time_dly(5);
//
//        while((readvalue>>7)!=1 && count!=0)
//        {
//            os_time_dly(5);
//            readvalue = stk8312_sensor_get_data(0x3F);// Read Engineering Register 0x3F
//            --count;
//        }
//
//        readvalue = stk8312_sensor_get_data(0x3E);
//
//        if(readvalue != 0x00)
//            stk8312_sensor_command(0x24, readvalue);
//
//        os_time_dly(100);	//Waiting for data ready
//#endif
    } else {
        //---------------------
        //Set to standby mode
        //---------------------
        //0x07—MODE/Features: 11000000
        //D7            D6          D5          	D4          	D3         		D2          	D1          	D0
        //IAH           IPP         Reserved        Reserved        Reserved        Reserved    	Reserved      	Mode
        //
        RegAddr       = 0x07;
        RegWriteValue = 0xC0;	//Interrupt active high, push-pull, standby-mode
        stk8312_sensor_command(RegAddr, RegWriteValue);
    }

}


void stk8312_init(void)//config RESOLUTION_RANGE ,MODE_BW,INT_MAP1, ACTIVE_DUR, ACTIVE_THS
{
    u8 RegAddr, RegWriteValue;

//    init_i2c_io();
    //---------------------
    //Software Reset
    //---------------------
    RegAddr       = 0x20;
    RegWriteValue = 0x00;
    stk8312_sensor_command(RegAddr, RegWriteValue);
//    os_time_dly(20);

    //---------------------
    //Set to standby mode
    //---------------------
    //0x07—MODE/Features: 11000000
    //D7            D6          D5          	D4          	D3         		D2          	D1          	D0
    //IAH           IPP         Reserved        Reserved        Reserved        Reserved    	Reserved      	Mode
    //
    RegAddr       = 0x07;
    RegWriteValue = 0xC0;	//Interrupt active high, push-pull, standby-mode
//    RegWriteValue = 0xC1;	//Interrupt active high, push-pull, active-mode
//    RegWriteValue = 0x81;	//Interrupt active high, open-drain, active-mode
    stk8312_sensor_command(RegAddr, RegWriteValue);

    //RegAddr       = 0x03;
    //  RegWriteValue = 0x80;
//	stk8312_sensor_command(RegAddr, RegWriteValue);
    //---------------
    //Output Data Rate Settings :
    //---------------
    //0x08: Auto-Wake and Active Mode Portrait/Landscape Samples per Seconds Register (Read/Write)
    //D7        D6      D5      D4      D3      D2      D1      D0
    //FILT[2]   FILT[1] FILT[0] AWSR[1] AWSR[0] AMSR[2] AMSR[1] AMSR[0]
    RegAddr       = 0x08;
    RegWriteValue = 0xe3;	//ODR=50Hz
//    RegWriteValue = 0x03;	//ODR=50Hz
//    RegWriteValue = 0xe1;	//ODR=200Hz
//    RegWriteValue = 0x02;	//ODR=100Hz
//    RegWriteValue = 0xe4;	//ODR=25Hz
//    RegWriteValue = 0xe5;	//ODR=12.5Hz
    stk8312_sensor_command(RegAddr, RegWriteValue);

    //---------------
    //Set Mode as interrupt and measurement operation mode
    //---------------

    //0x06: Interrupt Setup Register (Read/Write)
    //D7        D6      D5      D4      D3      D2      D1      D0
    //SHINTX    SHINTY  SHINTZ  GINT    ASINT   TINT    PLINT   FBINT
    RegAddr       = 0x06;   //set Register Address
//    RegWriteValue = 0x10;   //New Data Interrupt enabled
    RegWriteValue = 0xe0;   //New Data Interrupt enabled add by lt
//    RegWriteValue = 0xF0;   //New Data Interrupt enabled add by lt
    stk8312_sensor_command(RegAddr, RegWriteValue);

///////////////////////////////////////////
#if 0
    RegAddr       = 0x09;//tap/pulse detection register
//    RegWriteValue = 0x00;
    RegWriteValue = 0xe0;//62.5mg   tap detection threshold
//    RegWriteValue = 0xe1;//125mg
//    RegWriteValue = 0xe2;//187.5mg
//    RegWriteValue = 0xe3;//250mg
//    RegWriteValue = 0xe4;//312.5mg
//    RegWriteValue = 0xe5;//375mg
//    RegWriteValue = 0xe6;//437.5mg
//    RegWriteValue = 0xe7;//500mg
    stk8312_sensor_command(RegAddr, RegWriteValue);



    RegAddr       = 0x0f;//tap latency regitser
//    RegWriteValue = 0x3c;//300ms
    RegWriteValue = 0x00;//
    stk8312_sensor_command(RegAddr, RegWriteValue);


    RegAddr       = 0x10;//tap window regitser
//    RegWriteValue = 0x3c;//300ms
    RegWriteValue = 0x00;////disable the double-tap
    stk8312_sensor_command(RegAddr, RegWriteValue);
#endif

    /*   RegAddr       = 0x11;//free fall threshold register
       RegWriteValue = 0x06;//
       stk8312_sensor_command(RegAddr, RegWriteValue);

       RegAddr       = 0x12;//free fall time register
       RegWriteValue = 0x28;//
       stk8312_sensor_command(RegAddr, RegWriteValue);
    */
#if 1
    //---------------
    //Set Measure Range, Resolution
    //---------------
    //0x13: Dynamic Range Setup and Shake Threshold Register (Read/Write)
    //D7            D6          D5          D4      D3          D2          D1          D0
    //RNG[1]        RNG[0]      -           -       -           STH[2]      STH[1]      STH[0]
    // *** RNG[1:0]     MEASUREMENT RANGE   Resolution ***
    //     00           ±1.5 g                6
    //     01           ±6   g                8
    //     10           ±16  g                8
    //     11           RESERVED
    RegAddr       = 0x13;
    RegWriteValue = 0x42;	//6g, 8bit      shake threshold 1.375g
//    RegWriteValue = 0x46;	//6g, 8bit     shake threshold 1.875g
//    RegWriteValue = 0x02;	//1.5g, 6bit    shake threshold 1.375g
//    RegWriteValue = 0x86;	//16g, 8bit     shake threshold 1.875g
    stk8312_sensor_command(RegAddr, RegWriteValue);
#endif

    //  RegAddr       = 0x14;
//   RegWriteValue = 0x02;
//    RegWriteValue = 0x0b;
//    RegWriteValue = 0x0a;
//    RegWriteValue = 0x08;
    stk8312_sensor_command(RegAddr, RegWriteValue);

    stk8312_set_enable(1);

    puts("\n stk8312 init success\n");

}



void Polling_xyz_data(void *parm)	//X = data_out[0], Y = data_out[1], Z = data_out[2]
{
#if 0
    puts("\n XXXXXX : ");
//    if (stk8312_sensor_get_data(0x00) & BIT(6) == 0)
    {
        put_u16hex(stk8312_sensor_get_data(0x00));
    }
    puts("\n YYYYYY : ");
    put_u16hex(stk8312_sensor_get_data(0X01));

    puts("\n ZZZZZZ : ");
    put_u16hex(stk8312_sensor_get_data(0X02));

    puts("\n CCCCCC : ");
    put_u16hex(stk8312_sensor_get_data(0X03));
#endif
}


//工作分辨率选着
void stk8312_resolution_range(u8 range)
{
    u8 RegAddr, RegWriteValue;
    switch (range) {
    case G_SENSITY_HIGH:
        RegAddr       = 0x13;
//        RegWriteValue = 0x00;	//1.5g, 6bit  shake threshold 1.125g
        RegWriteValue = 0x02;	//1.5g, 6bit  shake threshold 1.375g
        stk8312_sensor_command(RegAddr, RegWriteValue);


        break;
    case G_SENSITY_MEDIUM:
        RegAddr       = 0x13;
//        RegWriteValue = 0x40;	//6g, 8bit  shake threshold 1.125g
//        RegWriteValue = 0x42;	//6g, 8bit  shake threshold 1.375g
//        RegWriteValue = 0x44;	//6g, 8bit  shake threshold 1.625g
        RegWriteValue = 0x46;	//6g, 8bit  shake threshold 1.875g
//        RegWriteValue = 0x47;	//6g, 8bit  shake threshold 2g
        stk8312_sensor_command(RegAddr, RegWriteValue);

        break;
    case G_SENSITY_LOW:
        RegAddr       = 0x13;
//        RegWriteValue = 0x80;	//16g, 8bit  shake threshold 1.12g
//        RegWriteValue = 0x82;	//16g, 8bit  shake threshold 1.375g
//        RegWriteValue = 0x84;	//16g, 8bit  shake threshold 1.625g
//        RegWriteValue = 0x86;	//16g, 8bit  shake threshold 1.875g
        RegWriteValue = 0x87;	//16g, 8bit  shake threshold 2g
        stk8312_sensor_command(RegAddr, RegWriteValue);


        break;
    }
}


//工作模式选着
void stk8312_work_mode(u8 work_mode)
{
    switch (work_mode) {
    case G_NORMAL_MODE:
        stk8312_set_enable(1);
        break;
    case G_LOW_POWER_MODE:
        stk8312_set_enable(1);
        break;
    case G_SUSPEND_MODE:
        stk8312_set_enable(0);
        break;
    }
}

extern int stk8312_pin_int_interrupt();
/**提供给二级子菜单回调函数使用*/
int stk8312_gravity_sensity(u8 gsid)
{

    if (gsid == G_SENSOR_SCAN) {
        if (stk8312_pin_int_interrupt()) {
            return -EINVAL;
        }
        return 0;
    }

    stk8312_set_enable(0);

    if (gsid == G_SENSOR_HIGH) {
        stk8312_resolution_range(G_SENSITY_HIGH);
        stk8312_work_mode(G_NORMAL_MODE);//normal mode
    }

    if (gsid == G_SENSOR_MEDIUM) {
        stk8312_resolution_range(G_SENSITY_MEDIUM);
        stk8312_work_mode(G_NORMAL_MODE);//normal mode
    }

    if (gsid == G_SENSOR_LOW) {
        stk8312_resolution_range(G_SENSITY_LOW);
        stk8312_work_mode(G_NORMAL_MODE);//normal mode
    }

//    if ((gsid != G_SENSOR_LOW_POWER_MODE)
//        && (gsid != G_SENSOR_CLOSE))
    {
        stk8312_set_enable(1) ;
    }

    if (gsid == G_SENSOR_LOW_POWER_MODE) { //低功耗
//        stk8312_resolution_range(get_gravity_sensor());
        stk8312_resolution_range(G_SENSITY_MEDIUM);
        stk8312_work_mode(G_LOW_POWER_MODE);

        os_time_dly(200);
        stk8312_sensor_get_data(0x00);//clear int
        stk8312_sensor_get_data(0x01);//clear int
        stk8312_sensor_get_data(0x02);//clear int
        stk8312_sensor_get_data(0x03);//clear int

    }


    if (gsid == G_SENSOR_CLOSE) {
        stk8312_work_mode(G_SUSPEND_MODE);//暂停
    }

    return 0;

}

/**
返回值 TRUE  重力传感器触发
       FALSE  重力传感器未触发
*/
int get_stk8312_int_state()
{
    u8 date_tmp;

//    if (get_menu_statu() == 1)
//    {
//        return FALSE;
//    }

    date_tmp = stk8312_sensor_get_data(0x07);
    if (date_tmp == 0xC0) {
        //standby mode 不做检测
        return -EINVAL;
    }

    date_tmp = stk8312_sensor_get_data(0x03);
    if ((date_tmp & BIT(6)) == 0) {
        if (((date_tmp & BIT(7)) >> 7)//shake
//            || ((date_tmp & BIT(5)) >> 5)//tap
           ) {
//            puts("\n jjjj 0\n");
//            puts("\n get_stk8312_int_state : ");
//            put_u8hex(date_tmp);
            return 0;
        }

    }

    return -EINVAL;
}

/**加速度超过与设定的阀值，产生中断。为当前视频文件上锁*/
int stk8312_pin_int_interrupt()
{
    if (get_stk8312_int_state()) {
        return  -EINVAL;
    }
    stk8312_sensor_get_data(0x03);//clear int
    return 0;
}

/**返回0 id正确*/
s8 stk8312_id_check(void)
{
    u8 chipid = 0x00;
    chipid = stk8312_sensor_get_data(0x12);
    log_d("stk8312 id = %x\n", chipid);
    if (chipid != 0x28) {
        puts("not stk8312\n");
        return -1;
    }

    return 0;
}


/**返回0 id正确*/
s8 stk8312_check(void)
{
    u8 RegAddr, RegWriteValue;
//    init_i2c_io();

//    stk8312_iic_set(1);

    //---------------------
    //Software Reset
    //---------------------
    RegAddr       = 0x20;
    RegWriteValue = 0x00;
    stk8312_sensor_command(RegAddr, RegWriteValue);
//    os_time_dly(20);

    if (0 != stk8312_id_check()) {
        return -1;
    }

    return 0;
}

_G_SENSOR_INTERFACE stk8312_ops = {
    .logo 				    = 	"stk8312",
    .gravity_sensor_check   =   stk8312_check,
    .gravity_sensor_init    =   stk8312_init,
    .gravity_sensor_sensity =   stk8312_gravity_sensity,
//    .gravity_sensor_interrupt = stk8312_pin_int_interrupt,
//    .gravity_sensor_read    = stk8312_sensor_get_data,
//    .gravity_sensor_write   = stk8312_sensor_command,
};

REGISTER_GRAVITY_SENSOR(stk8312)
.gsensor_ops = &stk8312_ops,
};

#endif // CONFIG_GSENSOR_ENABLE
