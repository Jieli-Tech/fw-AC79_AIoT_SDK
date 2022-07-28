#include "system/includes.h"
#include "asm/port_waked_up.h"
#include "typedef.h"
#include "os/os_api.h"
#include "asm/iic.h"
#include "device/iic.h"
#include "device/device.h"
#include "lcd_config.h"
#include "app_config.h"
#include "system/includes.h"
#include "gpio.h"
#include "ui_api.h"
#include "touch_event.h"
#include "sys_common.h"

#if 0
#define TEST 0



#if 1
#define log_info(x, ...)    printf("\n[touch]>" x " \n", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

static OS_SEM touch_sem;

extern int ui_touch_msg_post(struct touch_event *event);

//I2C读写命令
#define FT6236_WRCMD 			0X70     	//写命令
#define FT6236_RDCMD 			0X71		//读命令

#define FTS_REG_INT_CNT                     0x8F
#define FTS_REG_FLOW_WORK_CNT               0x91
#define FTS_REG_WORKMODE                    0x00
#define FTS_REG_WORKMODE_FACTORY_VALUE      0x40
#define FTS_REG_WORKMODE_WORK_VALUE         0x00
#define FTS_REG_ESDCHECK_DISABLE            0x8D
#define FTS_REG_CHIP_ID                     0xA3
#define FTS_REG_CHIP_ID2                    0x9F
#define FTS_REG_POWER_MODE                  0xA5
#define FTS_REG_POWER_MODE_SLEEP_VALUE      0x03
#define FTS_REG_FW_VER                      0xA6
#define FTS_REG_VENDOR_ID                   0xA8
#define FTS_REG_LCD_BUSY_NUM                0xAB
#define FTS_REG_FACE_DEC_MODE_EN            0xB0
#define FTS_REG_FACE_DEC_MODE_STATUS        0x01
#define FTS_REG_IDE_PARA_VER_ID             0xB5
#define FTS_REG_IDE_PARA_STATUS             0xB6
#define FTS_REG_GLOVE_MODE_EN               0xC0
#define FTS_REG_COVER_MODE_EN               0xC1
#define FTS_REG_CHARGER_MODE_EN             0x8B
#define FTS_REG_GESTURE_EN                  0xD0
#define FTS_REG_GESTURE_OUTPUT_ADDRESS      0xD3
#define FTS_REG_MODULE_ID                   0xE3
#define FTS_REG_LIC_VER                     0xE4
#define FTS_REG_ESD_SATURATE                0xED

static void *iic = NULL;

//I2C读写命令
#define FT_CMD_WR 				0X70    	//写命令
#define FT_CMD_RD 				0X71		//读命令
//FT6236 部分寄存器定义
#define FT_DEVIDE_MODE 			0x00   		//FT6236模式控制寄存器
#define FT_REG_NUM_FINGER       0x02		//触摸状态寄存器

#define FT_TP1_REG 				0X03	  	//第一个触摸点数据地址


#define	FT_ID_G_LIB_VERSION		0xA1		//版本
#define FT_ID_G_MODE 			0xA4   		//FT6236中断模式控制寄存器
#define FT_ID_G_THGROUP			0x80   		//触摸有效值设置寄存器
#define FT_ID_G_PERIODACTIVE	0x88   		//激活状态周期设置寄存器
#define Chip_Vendor_ID          0xA3        //芯片ID(0x36)
#define ID_G_FT6236ID			0xA8		//0x11

#define Y_MIRROR  0
#define X_MIRROR  0
/*#define VK_Y      	    LCD_W*/
/*#define VK_X      	    LCD_H*/
#define VK_Y      	    480
#define VK_X      	    320
#define VK_Y_MOVE       50
#define VK_X_MOVE       50
#define VK_X_Y_DIFF     30


static unsigned char wrFT6236Reg(u8 regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, FT6236_WRCMD)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(100);
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID & 0xff)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(100);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regDat)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(100);

exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}

static unsigned char rdFT6236Reg(u8 regID, u8 *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, FT6236_WRCMD)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(100);
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID & 0xff)) {
        ret = 0;;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(100);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, FT6236_RDCMD)) {
        ret = 0;
        goto exit;
    }
    delay(100);
    dev_ioctl(iic, IIC_IOCTL_RX_WITH_STOP_BIT, (u32)regDat);
exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}

int get_FT6236_pid(void)
{
    u8 temp = 0;
    rdFT6236Reg(FTS_REG_CHIP_ID, &temp);
    if (temp != 100) {
        return 1;
    }
    log_info(">>>>>>>>>>>hell touch FT6236<<<<<<<<<<<");
    return 0;
}

static void get_FT6236_xy(u16 addr, u16 *x, u16 *y)
{
    u8 buf[4];
    for (u8 i = 0; i < 4; i++) {
        rdFT6236Reg((addr + i), &buf[i]);	//读取XY坐标值
    }
    *y = (((u16)(buf[2] & 0X0F) << 8) + buf[3]);
    *x = ((u16)(buf[0] & 0X0F) << 8) + buf[1];
}


static u8 tp_last_staus = ELM_EVENT_TOUCH_UP;
static int tp_down_cnt = 0;

static void tpd_down(int x, int y)
{
    struct touch_event t;
    static u8 move_flag = 0;
    static int first_x = 0;
    static int first_y = 0;

    if (x < 0) {
        x = 0;
    }
    if (x > (VK_X - 1)) {
        x = VK_X - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > (VK_Y - 1)) {
        y = VK_Y - 1;
    }
#if Y_MIRROR
    x = VK_X - x - 1;
#endif

#if X_MIRROR
    y = VK_Y - y - 1;
#endif

    if ((tp_last_staus == ELM_EVENT_TOUCH_DOWN) && (x == first_x) && (y == first_y)) {
        tp_down_cnt++;
        if (tp_down_cnt < 30) {
            return;
        }
        tp_last_staus = ELM_EVENT_TOUCH_HOLD;
        tp_down_cnt = 0;

        t.action = tp_last_staus;
        t.x = x;
        t.y = y;
        log_info("----tpd_hold----x=%d, y=%d", x, y);
        ui_touch_msg_post(&t);
        return;
    }

    if (tp_last_staus != ELM_EVENT_TOUCH_UP) {
        int x_move = abs(x - first_x);
        int y_move = abs(y - first_y);

        if (!move_flag && (x_move >= VK_X_MOVE || y_move >= VK_Y_MOVE) && (abs(x_move - y_move) >= VK_X_Y_DIFF)) {
            if (x_move > y_move) {
                if (x > first_x) {
                    tp_last_staus = ELM_EVENT_TOUCH_R_MOVE;
                    log_info("----tpd_rigth_move----x=%d, y=%d", x, y);
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_L_MOVE;
                    log_info("----tpd_left_move----x=%d, y=%d", x, y);
                }

            } else {
                if (y > first_y) {
                    tp_last_staus = ELM_EVENT_TOUCH_D_MOVE;
                    log_info("----tpd_down_move----x=%d, y=%d", x, y);
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_U_MOVE;
                    log_info("----tpd_up_move----x=%d, y=%d", x, y);
                }
            }
            move_flag = 1;
        } else {
            if ((x == first_x) && (y == first_y)) {
                return;
            }
            tp_last_staus = ELM_EVENT_TOUCH_MOVE;
            log_info("----tpd_move----x=%d, y=%d", x, y);
        }
    } else {
        tp_last_staus = ELM_EVENT_TOUCH_DOWN;
        move_flag = 0;
        log_info("----tpd_down----x=%d, y=%d", x, y);
    }
    first_x = x;
    first_y = y;
    t.action = tp_last_staus;
    t.x = x;
    t.y = y;
    ui_touch_msg_post(&t);
}

static void tpd_up(int x, int y)
{
    struct touch_event t;

    if (x < 0) {
        x = 0;
    }
    if (x > (VK_X - 1)) {
        x = VK_X - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > (VK_Y - 1)) {
        y = VK_Y - 1;
    }

#if Y_MIRROR
    x = VK_X - x - 1;
#endif

#if X_MIRROR
    y = VK_Y - y - 1;
#endif

    /* log_info("U[%4d %4d %4d]\n", x, y, 0); */
    tp_last_staus = ELM_EVENT_TOUCH_UP;
    tp_down_cnt = 0;
    t.action = tp_last_staus;
    t.x = x;
    t.y =  y;
    log_info("----tpd_up----x=%d, y=%d", x, y);
    ui_touch_msg_post(&t);
}

static void FT6236_interrupt(void)
{
    os_sem_post(&touch_sem);
}

void FT6236_init(void)
{
    static u8 status = 0;
    static int x, y;
    static u16 touch_x = 0;
    static u16 touch_y = 0;
    static u8 touch_status = 0;

    os_sem_create(&touch_sem, 0);

    extern const struct ui_devices_cfg ui_cfg_data;
    static const struct ui_lcd_platform_data *pdata;
    pdata = (struct ui_lcd_platform_data *)ui_cfg_data.private_data;

    gpio_direction_output(pdata->touch_reset_pin, 0);
    os_time_dly(100);
    gpio_direction_output(pdata->touch_reset_pin, 1);
    os_time_dly(100);
    iic = dev_open("iic0", NULL);

    //注册中断注意触摸用的事件0 屏幕TE用的事件1
    port_wakeup_reg(EVENT_IO_0, pdata->touch_int_pin, EDGE_NEGATIVE, FT6236_interrupt);

    /*wrFT6236Reg(FT_DEVIDE_MODE, 0);*/
    /*wrFT6236Reg(FT_ID_G_THGROUP, 25);*/
    /*wrFT6236Reg(FT_ID_G_PERIODACTIVE, 20);*/
    if (get_FT6236_pid()) {
        log_info("[err]>>>>>FT6236 err!!!");
    }

    while (1) {
        os_sem_pend(&touch_sem, 0);

        rdFT6236Reg(FT_REG_NUM_FINGER, &status);	//读取触摸点的状态   // BIT7表示有数据 ,bit0-3 表示触摸点个数
        if (status) { //有触摸值
            get_FT6236_xy(FT_TP1_REG, &touch_x, &touch_y);
            tpd_down(touch_x, touch_y);//做触摸运算
            touch_status = 1;//标记触摸按下
        } else {
            if (touch_status) { //这样做的目的是使得发消息做处理只有一次
                tpd_up(touch_x, touch_y);//做触摸运算
            }
            touch_status = 0;//标记触摸抬起
        }
        status = 0;
    }
}

static void my_touch_test_task(void *priv)
{
    FT6236_init();
}

static int FT6236_task_init(void)
{
    return thread_fork("my_touch_test_task", 10, 1024, 0, NULL, my_touch_test_task, NULL);
}
late_initcall(FT6236_task_init);
#endif








