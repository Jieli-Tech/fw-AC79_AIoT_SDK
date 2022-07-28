#include "system/includes.h"
#include "app_config.h"
#include "gSensor_manage.h"

#ifdef USE_GSENSOR_TEST_DEMO

//添加gsensor灵敏度设置函数
static int gra_set_function(u32 parm)
{
//#ifdef CONFIG_GSENSOR_ENABLE
    switch (parm) {

    case GRA_SEN_OFF:
        parm = G_SENSOR_CLOSE;
        break;
    case GRA_SEN_LO:
        parm = G_SENSOR_LOW;
        break;
    case GRA_SEN_MD:
        parm = G_SENSOR_MEDIUM;
        break;
    case GRA_SEN_HI:
        parm = G_SENSOR_HIGH;
        break;
    }
    set_gse_sensity(parm);
//#endif
    return 0;
}

static int c_main(void)
{
    gra_set_function(GRA_SEN_MD);
    return 0;
}

late_initcall(c_main);

#endif

