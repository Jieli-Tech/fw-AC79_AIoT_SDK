#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include <time.h>
#include <sys/time.h>

#ifdef CONFIG_WIFI_ENABLE
#include "wifi/wifi_connect.h"
#endif

#ifdef USE_RTC_TEST_DEMO
//当应用层不定义该函数时，系统默认时间为SDK发布时间，当RTC设置时间小于SDK发布时间则设置无效
static void *rtc_hdl;

static void time_print(void)//网络时间获取方法
{
    char time_str[64];
    struct tm timeinfo;
    time_t timestamp;

    timestamp = time(NULL) + 28800;
    localtime_r(&timestamp, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    printf("RTC TIME [%s]\n", time_str);

    printf("system boot run time [%d sec] [%d msec]\n", timer_get_sec(), timer_get_ms());
    printf("jiffies run time [%d sec]\n", jiffies * 10);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("gettimeofday [%d sec+%d usec]\n", tv.tv_sec, tv.tv_usec);

    static u32 time_lapse_hdl = 0; //需要初始化为0,句柄在time_lapse使用过程中不可消亡
    u32 to = time_lapse(&time_lapse_hdl, 5000); //指定时间后返回真,并且返回比上一次执行的时间经过了多少毫秒

    if (to) {
        printf("time_lapse timeout %d msec\r\n", to);
    }
}

/* 闹钟响了进行这个函数 */
static void alarm_rings(void *priv)
{
    printf("alarm clock is rings!!!!!!!!!!!!!!!");
    /* 测试闹钟关机唤醒  使能唤醒 1 设置时间10秒  system will off set 1*/
#if 1
    alarm_wkup_ctrl(1, 10, 1); //打开闹钟闹钟唤醒功能 设置10s 一次
    power_set_soft_poweroff();
#else
    alarm_wkup_ctrl(1, 10, 0); //打开闹钟闹钟唤醒功能 设置10s 一次
#endif
}

static void time_rtc_test_task(void *p)
{
    static struct sys_time test_rtc_time;
    rtc_hdl = dev_open("rtc", NULL);

    if (!rtc_hdl) {
        printf("err in rtc_hdl_open rtc\n");
        return;
    }
#if 1
    /* 注册闹钟响铃回调函数 */
    set_rtc_isr_callback(alarm_rings, NULL);

    /* 打开RTC设备 */
    rtc_hdl = dev_open("rtc", NULL);
    dev_ioctl(rtc_hdl, IOCTL_GET_ALARM, (u32)&test_rtc_time);
    printf("get_alarm_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);
    /* 获取时间信息 */
    dev_ioctl(rtc_hdl, IOCTL_GET_SYS_TIME, (u32)&test_rtc_time);
    /* 打印时间信息 */
    printf("frist_time_get_sys_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);
    /* 赋值时间信息 */
    test_rtc_time.year = 2023;
    test_rtc_time.month = 8;
    test_rtc_time.day = 24;
    test_rtc_time.hour = 14;
    test_rtc_time.min = 23;
    test_rtc_time.sec = 0;
    /* 设置时间信息  */
    dev_ioctl(rtc_hdl, IOCTL_SET_SYS_TIME, (u32)&test_rtc_time);
    printf("set_sys_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);
    /* 获取时间星期 */
    u8 weekday = dev_ioctl(rtc_hdl, IOCTL_GET_WEEKDAY, (u32)&test_rtc_time);
    /* 打开闹钟开关 */
    extern void set_alarm_ctrl(u8 set_alarm);
    set_alarm_ctrl(1);
    /* 赋值闹钟时间信息 */
    test_rtc_time.year = 2023;
    test_rtc_time.month = 8;
    test_rtc_time.day = 24;
    test_rtc_time.hour = 14;
    test_rtc_time.min = 23;
    test_rtc_time.sec = 18;
    /* 设置闹钟 */
    dev_ioctl(rtc_hdl, IOCTL_SET_ALARM, (u32)&test_rtc_time);

    while (1) {
        os_time_dly(100);//just test this dly
        dev_ioctl(rtc_hdl, IOCTL_GET_SYS_TIME, (u32)&test_rtc_time);
        printf("get_sys_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);
        dev_ioctl(rtc_hdl, IOCTL_GET_ALARM, (u32)&test_rtc_time);
        printf("get_alarm_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);
    }

#else
//def CONFIG_WIFI_ENABLE //网络时间,当不需要网络时间则不需以下代码操作
    while (wifi_get_sta_connect_state() != WIFI_STA_NETWORK_STACK_DHCP_SUCC) {
        printf("Waitting STA Connected...\r\n");
        //当网络连接成功前, 获取的是同步网络时间前的RTC时间
        //当网络连接成功后并且连接远端NTP服务商成功, 执行time函数会获取网络时间同步本地RTC时间
        time_print();
        os_time_dly(100);
    }

    //联网成功后，系统自动把网络时间同步到系统RTC实际
    while (1) {
        time_print();  //当网络连接成功前, 获取的是同步网络时间前的RTC时间
        os_time_dly(300);
    }
#endif
}

static int c_main(void)
{
    os_task_create(time_rtc_test_task, NULL, 10, 1000, 0, "time_rtc_test_task");
    return 0;
}
late_initcall(c_main);
#endif
