#include "system/includes.h"
#include "asm/port_waked_up.h"
#include "typedef.h"
#include "os/os_api.h"
#include "asm/iic.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"
#include "system/includes.h"
#include "gpio.h"
#include "ui_api.h"
#include "touch_event.h"
#include "sys_common.h"

#if TCFG_TOUCH_GT911_ENABLE
#define TEST 0



#if 0
#define log_info(x, ...)    printf("\n[touch]>" x " \n", ## __VA_ARGS__)
#else
#define log_info(...)
#endif


extern int ui_touch_msg_post(struct touch_event *event);

//I2C读写命令
#define GT911_WRCMD 			0X28     	//写命令
#define GT911_RDCMD 			0X29		//读命令

//GT911 部分寄存器定义
#define GT_CTRL_REG 			0X8040   	//GT911控制寄存器
#define GT_CFGS_REG 			0X8047   	//GT911配置起始地址寄存器
#define GT_CHECK_REG		 	0X80FF   	//GT911校验和寄存器

#define GT_PID_REG0				0X8140   	//GT911产品ID寄存器
#define GT_PID_REG1				0X8141   	//GT911产品ID寄存器
#define GT_PID_REG2				0X8142   	//GT911产品ID寄存器

#define GT_GSTID_REG 	        0X814E   	//GT911当前检测到的触摸情况
#define GT_TP1_REG 		        0X8150  	//第一个触摸点数据地址 //每个点4四个地址控制GT_TP1_X_L_REG
#define GT_TP2_REG 				0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 				0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 				0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 				0X8170		//第五个触摸点数据地址

static void *iic = NULL;

#define Y_MIRROR  0
#define X_MIRROR  0
#define VK_Y      	    320
#define VK_X      	    240
#define VK_Y_MOVE       30
#define VK_X_MOVE       30
#define VK_X_Y_DIFF     27


static unsigned char wrGT911Reg(unsigned int regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GT911_WRCMD)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID >> 8)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID & 0xff)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(1000);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regDat)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(1000);

exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}

static unsigned char rdGT911Reg(u16 regID, u8 *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GT911_WRCMD)) {
        ret = 0;
        log_info("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(100);
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID >> 8)) {
        ret = 0;;
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GT911_RDCMD)) {
        ret = 0;
        goto exit;
    }
    delay(100);
    dev_ioctl(iic, IIC_IOCTL_RX_WITH_STOP_BIT, (u32)regDat);
exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}

int get_GT911_pid(void)
{
    u8 temp = 0;
    rdGT911Reg(GT_PID_REG0, &temp);
    if (temp != 57) { // 9
        return 1;
    }
    rdGT911Reg(GT_PID_REG1, &temp);
    if (temp != 49) { // 1
        return 1;
    }
    rdGT911Reg(GT_PID_REG2, &temp);
    if (temp != 49) { // 1
        return 1;
    }
    log_info(">>>>>>>>>>>hell touch GT911<<<<<<<<<<<");
    return 0;
}

static void get_GT911_xy(u16 addr, u16 *x, u16 *y)
{
    u8 buf[4];
    for (u8 i = 0; i < 4; i++) {
        rdGT911Reg((addr + i), &buf[i]);	//读取XY坐标值
    }
    *x = buf[0] + buf[1] * 255;
    *y = buf[2] + buf[3] * 255;
}

static void GT911_test(void)
{
    u8 status = 0;
    u16 x = 0;
    u16 y = 0;
    rdGT911Reg(GT_GSTID_REG, &status);	//读取触摸点的状态   // BIT7表示有数据 ,bit0-3 表示触摸点个数
    if (status & 0x80) { //有触摸值
        if (status > 127) { //有一个触摸点按下
            get_GT911_xy(GT_TP1_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 1]x = %d, y = %d", x, y);
        }
        if (status > 128) { //有两个触摸点按下
            get_GT911_xy(GT_TP2_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 2]x = %d, y = %d", x, y);
        }
        if (status > 129) { //有三个触摸点按下
            get_GT911_xy(GT_TP3_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 3]x = %d, y = %d", x, y);
        }
        if (status > 130) { //有四个触摸点按下
            get_GT911_xy(GT_TP4_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 4]x = %d, y = %d", x, y);
        }
        if (status > 131) { //有五个触摸点按下
            get_GT911_xy(GT_TP5_REG, &x, &y);
            log_info(">>>>>>>>>>>[pint 5]x = %d, y = %d", x, y);
        }
        status = 0;
        wrGT911Reg(GT_GSTID_REG, status); //清标志
    }
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
    t.y = y;
    log_info("----tpd_up----x=%d, y=%d", x, y);
    ui_touch_msg_post(&t);
}


static void GT911_interrupt(void)
{
    u8 status = 0;
    static u16 touch_x = 0;
    static u16 touch_y = 0;
    static u8 touch_status = 0;


    rdGT911Reg(GT_GSTID_REG, &status);	//读取触摸点的状态   // BIT7表示有数据 ,bit0-3 表示触摸点个数

    if (status != 0x80) { //有触摸值
        if (status > 127) { //有一个触摸点按下
            get_GT911_xy(GT_TP1_REG, &touch_x, &touch_y);
            tpd_down(touch_x, touch_y);//做触摸运算
            touch_status = 1;//标记触摸按下
        }
    } else {
        if (touch_status) { //这样做的目的是使得发消息做处理只有一次
            tpd_up(touch_x, touch_y);//做触摸运算
        }
        touch_status = 0;//标记触摸抬起
    }
    status = 0;
    wrGT911Reg(GT_GSTID_REG, status); //清标志
}

static int GT911_init(void)
{
    iic = dev_open("iic0", NULL);

    extern const struct ui_devices_cfg ui_cfg_data;
    static const struct ui_lcd_platform_data *pdata;
    pdata = (struct ui_lcd_platform_data *)ui_cfg_data.private_data;

    gpio_direction_output(pdata->touch_reset_pin, 0);
    os_time_dly(1);
    gpio_direction_output(pdata->touch_int_pin, 1);
    os_time_dly(1);
    gpio_direction_output(pdata->touch_reset_pin, 1);
    os_time_dly(1);
    gpio_direction_output(pdata->touch_int_pin, 0);
    os_time_dly(10);

    //注册中断注意触摸用的事件0 屏幕TE用的事件1
    port_wakeup_reg(EVENT_IO_0, pdata->touch_int_pin, EDGE_NEGATIVE, GT911_interrupt);

    if (get_GT911_pid()) {
        log_info("[err]>>>>>GT911 err!!!");
        return 1;
    }

    return 0;
}

static void my_touch_test_task(void *priv)
{
    /*=====TE中断配置=====*/
    GT911_init();
}

static int GT911_task_init(void)
{
    return thread_fork("my_touch_test_task", 10, 1024, 0, NULL, my_touch_test_task, NULL);
}
late_initcall(GT911_task_init);
#endif








