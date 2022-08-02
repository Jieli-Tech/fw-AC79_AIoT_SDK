
#include "app_config.h"
#include "lcd_drive.h"
#include "init.h"
#include "ui_api.h"
#include "yuv_soft_scalling.h"
#include "system/includes.h"
#include "asm/gpio.h"

#ifdef CONFIG_UI_ENABLE

OS_SEM send_ok_sem;
OS_SEM te_ready_sem;
static u32 te_send_size;
static u8 *te_send_buf;

static void (*cb_TE_send_lcd_data)(u8 *buf, u32 buf_size);
static u8 te_fps = 0;

void get_te_fps(char *fps)
{
    *fps = te_fps;
}

static void Calculation_frame(void)
{
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;

        if (tdiff >= 1000) {
            printf("\n [te_MSG] fps_count = %d\n", fps_cnt *  1000 / tdiff);
            te_fps = fps_cnt *  1000 / tdiff;
            tstart = 0;
            fps_cnt = 0;
        }
    }
}
void init_TE(void (*cb)(u8 *buf, u32 buf_size))
{
    cb_TE_send_lcd_data = cb;
}

#if USE_LCD_TE
/*===============TE发送数据应用===============*/
/*该文件主要用于解决屏幕条纹发送一帧数据到屏幕*/
/*屏幕驱动需要对接(init_TE)cb_TE_send_lcd_data(屏幕发送一帧数据)*/
/*参考st7789s屏幕驱动*/

static u8 data_flog = 0;

void te_send_data(u8 *data_buf, u32 data_size)
{
    te_send_buf = data_buf;
    te_send_size = data_size;
    data_flog = 1 ;
}

extern void *port_wakeup_reg(PORT_EVENT_E event, unsigned int gpio, PORT_EDGE_E edge, void (*handler)(void));

static void TE_interrupt(void)//TE中断函数 中断发送消息去TE线程发送数据
{
    if (data_flog) {
        data_flog = 0;
        os_sem_post(&te_ready_sem); //告知线程TE到了并且数据也准备好了可以发送数据
    }
}

static void TE_wakeup_send_data_task(void *priv)//TE发送数据线程
{
    static u8 time = 0;
    while (1) {
        Calculation_frame();
        os_sem_pend(&te_ready_sem, 0);
        cb_TE_send_lcd_data(te_send_buf, te_send_size);
        os_sem_post(&send_ok_sem);
        if (!time) {
            time = 1;
            lcd_bl_pinstate(BL_ON);
        }
    }
}

static int lcd_TE_wakeup_send_data_init(void)//TE初始化
{
    extern const struct ui_devices_cfg ui_cfg_data;
    static const struct ui_lcd_platform_data *pdata;
    pdata = (struct ui_lcd_platform_data *)ui_cfg_data.private_data;

    os_sem_create(&te_ready_sem, 0);
    os_sem_create(&send_ok_sem, 0);

    int *ret = NULL;
    /*=====TE中断配置=====*/
    ret = port_wakeup_reg(EVENT_IO_1, pdata->te_pin, EDGE_POSITIVE, TE_interrupt);
    if (ret) {
        printf("port_wakeup_reg success.\n");
    } else {
        printf("port_wakeup_reg fail.\n");
    }

    return thread_fork("TE_wakeup_send_data_task", 9, 512, 32, NULL, TE_wakeup_send_data_task, NULL);
}
late_initcall(lcd_TE_wakeup_send_data_init);//开机就创建TE数据发送线程

#else  //SPI 等不带TE的 数据发送
/*===============不带TE发送数据应用===============*/
void te_send_data(u8 *data_buf, u32 data_size)
{
    te_send_buf = data_buf;
    te_send_size = data_size;
    os_sem_post(&te_ready_sem); //告知线程TE到了并且数据也准备好了可以发送数据
}
static void no_te_send_data_task(void *priv)//发送数据线程
{
    static u8 time = 0;
    while (1) {
        Calculation_frame();
        os_sem_pend(&te_ready_sem, 0);
        cb_TE_send_lcd_data(te_send_buf, te_send_size);
        os_sem_post(&send_ok_sem);
        if (!time) {
            time = 1;
            lcd_bl_pinstate(BL_ON);
        }
    }
}

static int send_data_init(void)//数据发送初始化
{
    os_sem_create(&te_ready_sem, 0);
    os_sem_create(&send_ok_sem, 0);

    return thread_fork("no_te_send_data_task", 9, 512, 32, NULL, no_te_send_data_task, NULL);
}
late_initcall(send_data_init);//开机就创建数据发送线程
#endif
#endif

