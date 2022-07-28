#include "app_config.h"
#include "system/includes.h"
#include "asm/pwm.h"
#include "device/device.h"

/*********************************PWM设备例子****************************************************
  注意：初始化之后，只支持单通道选择
  PWM通过dev_write写，返回值为成功0，失败非0；
  PWM通过dev_read读，返回值为通道的占空比值(0-100%)；
  PWM通过dev_ioctl控制，形参arg为PWM通道；
************************************************************************************************/

#ifdef USE_PWM_TEST_DEMO
static void pwm_test(void)
{
    int ret;
    u32 duty;
    u32 channel;
    void *pwm_dev_handl = NULL;
    struct pwm_platform_data pwm = {0};

    /*1.open ，open之后的PWM通道最多支持同一个通道的H和L设置，不能进行多通道的设置*/
    pwm_dev_handl = dev_open("pwm1", &pwm);//打开PWM0设备，传参可以获取board.c配置的参数
    if (!pwm_dev_handl) {
        printf("open pwm err !!!\n\n");
        return ;
    }
    printf("pwm : ch=0x%x,duty=%2f%%,pbit=%d,freq=%dHz\n", pwm.pwm_ch, pwm.duty, pwm.point_bit, pwm.freq);
    os_time_dly(200);

    /*open PWM设备之后就会初始化PWM，PWM相关参数为board.c配置，在不需要更改参数时，只需要open就行，不需要进行以下操作*/

#if 1
    /*2.write and read 配置占空比*/
    pwm.pwm_ch = PWMCH0_H | PWMCH0_L;//该通道需在board.c中有定义，若没有则需先PWM_ADD_CHANNEL,下面有例子。
    pwm.duty = 80;
    dev_write(pwm_dev_handl, (void *)&pwm, 0);
    ret = dev_read(pwm_dev_handl, (void *)&pwm, 0);//dev_read函数返回值为占空比（不带小数点）0-100
    printf("pwm0 read duty : %d \n", ret);
    os_time_dly(200);

    /*3.ioctl控制PWM暂停、运行、正反向,调用1次ioctl只支持1组通道PWMCH_H/L控制*/
    printf("----pwm0 ioctl-------\n\n");
    dev_ioctl(pwm_dev_handl, PWM_STOP, (u32)&pwm);//PWM停止
    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, PWM_RUN, (u32)&pwm);//PWM运行
    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, PWM_REVDIRC, (u32)&pwm);//PWM正向 PWMCHx_H/L才能支持
    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, PWM_FORDIRC, (u32)&pwm);//PWM反向 PWMCHx_H/L才能支持

    pwm.deathtime = 6;//最大值31 死区时间为系统时钟的(deathtime+1)倍,使用PWMCHx_H/L有效
    dev_ioctl(pwm_dev_handl, PWM_SET_DEATH_TIME, (u32)&pwm);//PWM死区时间设置

    os_time_dly(200);
    printf("----pwm0 set freq-------\n\n");
    /*4.ioctl配置频率和占空比*/
    pwm.pwm_ch = PWMCH0_H | PWMCH0_L;
    pwm.freq = 2000;
    pwm.duty = 20;
    dev_ioctl(pwm_dev_handl, PWM_SET_FREQ, (u32)&pwm);//设置频率
    os_time_dly(200);

    pwm.duty = 50;
    dev_ioctl(pwm_dev_handl, PWM_SET_DUTY, (u32)&pwm);//设置占空比
    os_time_dly(200);

    pwm.duty = 80;
    dev_ioctl(pwm_dev_handl, PWM_SET_DUTY, (u32)&pwm);//设置占空比
    os_time_dly(200);

    /*5.中途可以添加TIMER2 PWM 任意IO,添加通道后关闭前必须删除 */
    printf("----timer add channel-------\n\n");
    pwm.pwm_ch = PWM_TIMER2_OPCH2;
    pwm.port = IO_PORTA_07;
    pwm.duty = 10;
    pwm.freq = 1500;
    dev_ioctl(pwm_dev_handl, PWM_ADD_CHANNEL, (u32)&pwm);//中途添加通道，可以是PWMCHx_H/L和PWM_TIMER2_OPCH2或PWM_TIMER3_OPCH3

    pwm.duty = 80;
    dev_write(pwm_dev_handl, (void *)&pwm, 0);//dev_write也可以设置占空比
    ret = dev_read(pwm_dev_handl, (void *)&pwm, 0);//读取占空比
    printf("pwm0 read duty : %d \n", ret);

    os_time_dly(200);


    printf("----timer ioctl pwm.pwm_ch = 0x%x-------\n\n", pwm.pwm_ch);
    /*6.ioctl控制PWM暂停、运行、正反向,调用1次ioctl只支持1组通道PWMCH_H/L控制*/
    os_time_dly(300);
    dev_ioctl(pwm_dev_handl, PWM_STOP, (u32)&pwm);//PWM停止

    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, PWM_RUN, (u32)&pwm);//PWM运行
    os_time_dly(200);

    pwm.freq = 2000;
    pwm.duty = 20;
    printf("----timer set freq-------\n\n");
    dev_ioctl(pwm_dev_handl, PWM_SET_FREQ, (u32)&pwm);//设置频率
    os_time_dly(200);
    pwm.duty = 50;
    dev_ioctl(pwm_dev_handl, PWM_SET_DUTY, (u32)&pwm);//设置占空比
    os_time_dly(200);
    pwm.duty = 80;
    dev_ioctl(pwm_dev_handl, PWM_SET_DUTY, (u32)&pwm);//设置占空比
    os_time_dly(200);

    /*7.关闭前把添加通道删除,*/
    dev_ioctl(pwm_dev_handl, PWM_REMOV_CHANNEL, (u32)&pwm);
#endif
    dev_close(pwm_dev_handl);
    printf("pwm test end\n\n");

    while (1) {
        os_time_dly(2);
    }
}
static int c_main(void)
{
    os_task_create(pwm_test, NULL, 12, 1000, 0, "pwm_test");
    return 0;
}
late_initcall(c_main);

#endif
