#include "asm/plcnt.h"
#include "device/gpio.h"
#include "key/key_driver.h"
#include "key/touch_key.h"
#include "system/timer.h"
#include "app_config.h"

#if TCFG_TOUCH_KEY_ENABLE

#define PLCNT_CLOCK_SEL(x)				SFR(JL_PCNT->CON, 2, 2, x)
#define INPUT_CHANNLE2_SRC_SEL(x)		SFR(JL_IOMAP->CON2, 16, 6, x)

static u32 Touchkey_value_old;
static u8 bCapState;        //电容充放电状态
static u8 bCap_ch;          //触摸通道

static const struct touch_key_platform_data *__this = NULL;
static sPLCNT_KEY_VAR plcnt_key_value;

static u8 plcnt_channel2port(u8 chan)
{
    return __this->port_list[chan].port;
}

static void set_port_out_H(u8 chan)
{
    gpio_direction_output(plcnt_channel2port(chan), 1);
}

static void set_port_pd(u8 chan)
{
    gpio_set_pull_down(plcnt_channel2port(chan), 1);
}

static void set_port_die(u8 chan)
{
    gpio_set_die(plcnt_channel2port(chan), 1);
}

static void set_port_in(u8 chan)
{
    gpio_direction_input(plcnt_channel2port(chan)); //输入
}

static void set_touch_io(u8 chan)
{
    u8 port = plcnt_channel2port(chan);

    if (port >= IO_PORTA_00 && port <= IO_PORTA_10) {
        port = port - IO_PORTA_00;
    } else if (port >= IO_PORTB_00 && port <= IO_PORTB_08) {
        port = port - IO_PORTB_00 + 11;
    } else if (port >= IO_PORTC_00 && port <= IO_PORTC_10) {
        port = port - IO_PORTC_00 + 20;
    } else if (port >= IO_PORTE_00 && port <= IO_PORTE_09) {
        port = port - IO_PORTE_00 + 31;
    } else if (port >= IO_PORTH_00 && port <= IO_PORTH_09) {
        port = port - IO_PORTH_00 + 41;
    } else if (port >= IO_PORTG_08 && port <= IO_PORTG_15) {
        port = port - IO_PORTG_08 + 51;
    } else if (port >= IO_PORT_USB_DPA && port <= IO_PORT_USB_DMB) {
        port = port - IO_PORT_USB_DPA + 60;
    }

    INPUT_CHANNLE2_SRC_SEL(port);
    set_port_in(chan);  //设置为输入
}

static void plcnt_irq(sPLCNT_KEY_VAR *plcnt_key_var, u32 plcnt_res, u8 ch)
{
    u16 temp_u16_0, temp_u16_1;
    s16 temp_s16_0, temp_s16_1;
    s32 temp_s32_0;
//..............................................................................................
//取计数值/通道判断
//..............................................................................................

    if (plcnt_key_var->touch_init_cnt[ch]) {
        plcnt_key_var->touch_init_cnt[ch]--;
//		touch_cnt_buf[ch] = rvalue << FLT0CFG;
//		touch_release_buf[ch] = (long)(rvalue) << FLT1CFG0;
        plcnt_key_var->touch_cnt_buf[ch] = plcnt_res << plcnt_key_var->FLT0CFG;
        plcnt_key_var->touch_release_buf[ch] = plcnt_res << plcnt_key_var->FLT1CFG0;
    }

//..............................................................................................
//当前计数值去抖动滤波器
//..............................................................................................
    temp_u16_0 = plcnt_key_var->touch_cnt_buf[ch];
    temp_u16_1 = temp_u16_0;
    temp_u16_1 -= (temp_u16_1 >> plcnt_key_var->FLT0CFG);
    temp_u16_1 += plcnt_res;//temp_u16_1 += rvalue;
    plcnt_key_var->touch_cnt_buf[ch] = temp_u16_1;
    temp_u16_0 += temp_u16_1;
    temp_u16_0 >>= (plcnt_key_var->FLT0CFG + 1);

//..............................................................................................
//各通道按键释放计数值滤波器
//..............................................................................................
    temp_s32_0 = plcnt_key_var->touch_release_buf[ch];
    temp_u16_1 = temp_s32_0 >> plcnt_key_var->FLT1CFG0;	//获得滤波器之后的按键释放值
    temp_s16_0 = temp_u16_0 - temp_u16_1;	//获得和本次检测值的差值，按下按键为负值，释放按键为正值
    temp_s16_1 = temp_s16_0;

//	if(ch == 1)
//	{
//		printf("ch%d: %d  %d", (short)ch, temp_u16_0, temp_s16_1);
//	}

    if (!(plcnt_key_var->touch_key_state & BIT(ch))) {	//如果本通道按键目前是处于释放状态
        if (temp_s16_1 >= 0) {	//当前计数值大于低通值，放大后参与运算
            if (temp_s16_1 < (plcnt_key_var->FLT1CFG2 >> 3)) {
                temp_s16_1 <<= 3;	//放大后参与运算
            } else {
                temp_s16_1 = plcnt_key_var->FLT1CFG2;	//饱和，防止某些较大的正偏差导致错判
            }
        } else if (temp_s16_1 >= plcnt_key_var->FLT1CFG1) {	//当前计数值小于低通值不多，正常参与运算

        } else {			//当前计数值小于低通值很多，缩小后参与运算 (有符号数右移自动扩展符号位???)
            temp_s16_1 >>= 3;
        }
    } else {		//如果本通道按键目前是处于按下状态, 缓慢降低释放计数值
        if (temp_s16_1 <= plcnt_key_var->RELEASECFG1) {
            temp_s16_1 >>= 3;		//缩小后参与运算
        } else {
            temp_s16_1 = 0;
        }
    }

    temp_s32_0 += (s32)temp_s16_1;
    plcnt_key_var->touch_release_buf[ch] = temp_s32_0;

//..............................................................................................
//按键按下与释放检测
//..............................................................................................
    if (temp_s16_0 <= plcnt_key_var->PRESSCFG) {			//按键按下
        plcnt_key_var->touch_key_state |= BIT(ch);
    } else if (temp_s16_0 >= plcnt_key_var->RELEASECFG0) {	//按键释放
        plcnt_key_var->touch_key_state &= ~BIT(ch);
    }
}

static void scan_capkey(void *priv)
{
    u32 temp;
    u32 Touchkey_value_delta = 0;
    u32 Touchkey_value_new = 0;

    if (bCapState == 0) {
        bCapState = 1;
        Touchkey_value_new = JL_PCNT->VAL;   ///获取计数值

        set_port_out_H(bCap_ch);//set output H
        ///////***wait IO steady for pulse counter

        if (Touchkey_value_old > Touchkey_value_new) {
            /* Touchkey_value_new += 0x10000; */
            Touchkey_value_delta = Touchkey_value_new + ~Touchkey_value_old + 1;
        } else {
            Touchkey_value_delta = Touchkey_value_new - Touchkey_value_old;
        }

        /* printf("old: %x, new: %x, delta : %d\n", Touchkey_value_old, Touchkey_value_new, Touchkey_value_delta); */
        Touchkey_value_old = Touchkey_value_new;	///记录旧值
        temp = 6800L - Touchkey_value_delta * __this->change_gain;    ///1变化增大倍数

        /* if(bCap_ch == 0){ */
        /* printf("value: %x\n", Touchkey_value_delta); */
        /* } */

        //printf("delta: %d, ch: %d", Touchkey_value_delta, bCap_ch);
        /* if(bCap_ch == 0) */
        /* { */
        /* printf("temp: %d\n", Touchkey_value_delta); */
        /* } */

        /*调用滤波算法*/
        plcnt_irq(&plcnt_key_value, temp, bCap_ch);

        /*清除前一个通道状态，锁定PLL CNT*/
        /*切换通道，开始充电，PLL CNT 输入Mux 切换*/
        bCap_ch++;
        bCap_ch %= __this->num;
        //bCap_ch = (bCap_ch >= __this->num) ? 0 : bCap_ch;

        ////////***make sure IO is steady (IO output high voltage) for pulse counter
        set_port_out_H(bCap_ch);
    } else {
        bCapState = 0;
        set_touch_io(bCap_ch); //设为输入开始计数
    }
}

int plcnt_init(void *_data)
{
    const struct touch_key_platform_data *user_data = (const struct touch_key_platform_data *)_data;
    if (!user_data) {
        return -EINVAL;
    }

    if (user_data->num > TOUCH_KEY_CH_MAX) {
        return -EINVAL;
    }

    __this = user_data;
    bCap_ch = 0;

    memset(&plcnt_key_value, 0x0, sizeof(sPLCNT_KEY_VAR));

    /*触摸按键参数配置*/
    plcnt_key_value.FLT0CFG = 0;
    plcnt_key_value.FLT1CFG0 = 7;
    plcnt_key_value.FLT1CFG1 = -80;
    plcnt_key_value.FLT1CFG2 = 10 << 7; //1280

    ///调节灵敏度的主要参数
    /* plcnt_key_value.PRESSCFG = -10; */
    /* plcnt_key_value.RELEASECFG0 = -50; */
    /* plcnt_key_value.RELEASECFG1 = -80;//-81; */

    plcnt_key_value.PRESSCFG = user_data->press_cfg;
    plcnt_key_value.RELEASECFG0 = user_data->release_cfg0;
    plcnt_key_value.RELEASECFG1 = user_data->release_cfg1;

    memset(plcnt_key_value.touch_init_cnt, 0x10, TOUCH_KEY_CH_MAX);

    /* PLCNT_CLOCK_SEL(TOUCH_KEY_PLL_192M_CLK); */
    PLCNT_CLOCK_SEL(TOUCH_KEY_PLL_240M_CLK);

    JL_PCNT->CON |= BIT(1);	//使能计数器

    Touchkey_value_old = JL_PCNT->VAL;   ///获取计数值

    set_port_out_H(bCap_ch);

#if 1       //如果外部有下拉电阻，可不是用芯片内部下拉
    for (u8 i = 0; i < user_data->num; i++) {
        set_port_pd(i);
        set_port_die(i);
    }
#endif

    sys_s_hi_timer_add(NULL, scan_capkey, 2); //2ms

    return 0;
}

u8 get_plcnt_value(void)
{
    u8 key = NO_KEY;

    for (u8 i = 0; i < __this->num; i++) {
        if (plcnt_key_value.touch_key_state & BIT(i)) {
            key = i;
            break;
        }
    }

    return key;
}

#endif

