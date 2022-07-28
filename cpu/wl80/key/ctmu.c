#include "device/gpio.h"
#include "key/key_driver.h"
#include "key/ctmu_key.h"
#include "key/touch_key.h"
#include "asm/ctmu.h"
#include "app_config.h"

#if TCFG_CTMU_TOUCH_KEY_ENABLE

#define CTMU_CON0 		JL_CTM->CON0
#define CTMU_CON1 		JL_CTM->CON1
#define CTMU_RES 		JL_CTM->RES

static sCTMU_KEY_VAR ctm_key_value;
static const struct touch_key_platform_data *__this;
static u8 port_index_mapping_talbe[CTMU_KEY_CH_MAX];

//=================================================================================//
/*
ctmu原理:

  1.三个时钟源:
     1)闸门时钟源(可选), 分频值可以选
     2)放电时钟源(固定是lsb), 分频值可以选
     3)充电时钟源(不可选), 分频值不可选

  2.闸门时间内
            _________________________________
         __|                                 |__
      		    __    __    __    __    __
 放电时计数  __|  |__|  |__|  |__|  |__|  |__

  3. 过程: 放电 --> 充电(计数) --> 放电[该过程可选]

  4.分析计数值
 */
//=================================================================================//

//时钟闸门时钟源选择, 可分频
enum {
    GATE_SOURCE_LSB = 0,
    GATE_SOURCE_OSC,
    GATE_SOURCE_RTOSC,
    GATE_SOURCE_PLL12M,
};

enum {
    GATE_SOURCE_PRE_DIV1 = 0,
    GATE_SOURCE_PRE_DIV2,
    GATE_SOURCE_PRE_DIV4,
    GATE_SOURCE_PRE_DIV8,
    GATE_SOURCE_PRE_DIV16,
    GATE_SOURCE_PRE_DIV32,
    GATE_SOURCE_PRE_DIV64,
    GATE_SOURCE_PRE_DIV128,
    GATE_SOURCE_PRE_DIV256,
    GATE_SOURCE_PRE_DIV512,
    GATE_SOURCE_PRE_DIV1024,
    GATE_SOURCE_PRE_DIV2048,
    GATE_SOURCE_PRE_DIV4096,
    GATE_SOURCE_PRE_DIV8192,
    GATE_SOURCE_PRE_DIV16384,
    GATE_SOURCE_PRE_DIV32768,
};

//放电时钟源硬件上默认是lsb, 不可分频
enum {
    DISCHARGE_SOURCE_PRE_DIV1 = 0,
    DISCHARGE_SOURCE_PRE_DIV2,
    DISCHARGE_SOURCE_PRE_DIV4,
    DISCHARGE_SOURCE_PRE_DIV8,
    DISCHARGE_SOURCE_PRE_DIV16,
    DISCHARGE_SOURCE_PRE_DIV32,
    DISCHARGE_SOURCE_PRE_DIV64,
    DISCHARGE_SOURCE_PRE_DIV128,
};

//充电时钟源不可选择, 不可分频
enum {
    CHARGE_SOURCE_OSC = 0,
    CHARGE_SOURCE_LRC,
    CHARGE_SOURCE_PLL240M,
    CHARGE_SOURCE_PLL480M,
};

static const u8 ctmu_ch_table[] = {
    IO_PORTC_00, IO_PORTC_01, IO_PORTC_02, IO_PORTC_03,
    IO_PORTC_04, IO_PORTC_05, IO_PORTC_06, IO_PORTC_07,
    IO_PORTC_08, IO_PORTC_09, IO_PORTC_10, IO_PORTH_00,
    IO_PORTH_01, IO_PORTH_02, IO_PORTH_03,
};

static void ctm_irq(sCTMU_KEY_VAR *ctm_key_var, u32 ctm_res, u8 ch)
{
    u16 temp_u16_0, temp_u16_1;
    s16 temp_s16_0, temp_s16_1;
    s32 temp_s32_0;

    for (u32 i = 0; i < CTMU_KEY_CH_MAX; ++i) {
        if (port_index_mapping_talbe[i] == ch) {
            /* printf("res : 0x%x, i : %d, ch : %d\n", ctm_res, i, ch); */
            ch = i;
            break;
        }
    }

//..............................................................................................
//取计数值/通道判断
//..............................................................................................
    if (ctm_key_var->touch_init_cnt[ch]) {
        ctm_key_var->touch_init_cnt[ch]--;
        /* ctm_key_var->touch_cnt_buf[ch] = ctm_res << (ctm_key_var->FLT0CFG + 1); */
        /* ctm_key_var->touch_release_buf[ch] = ctm_res << (ctm_key_var->FLT1CFG0 + 1); */
        ctm_key_var->touch_cnt_buf[ch] = ctm_res << ctm_key_var->FLT0CFG;
        ctm_key_var->touch_release_buf[ch] = ctm_res << ctm_key_var->FLT1CFG0;
    }
//..............................................................................................
//当前计数值去抖动滤波器
//..............................................................................................
    temp_u16_0 = ctm_key_var->touch_cnt_buf[ch];
    temp_u16_1 = temp_u16_0;
    temp_u16_1 -= (temp_u16_1 >> ctm_key_var->FLT0CFG);
    temp_u16_1 += ctm_res;
    ctm_key_var->touch_cnt_buf[ch] = temp_u16_1;
    temp_u16_0 += temp_u16_1;
    temp_u16_0 >>= (ctm_key_var->FLT0CFG + 1);
//..............................................................................................
//各通道按键释放计数值滤波器
//..............................................................................................
    temp_s32_0 = ctm_key_var->touch_release_buf[ch];
    temp_u16_1 = temp_s32_0 >> ctm_key_var->FLT1CFG0;	//获得滤波器之后的按键释放值
    temp_s16_0 = temp_u16_0 - temp_u16_1;	//获得和本次检测值的差值，按下按键为负值，释放按键为正值
    temp_s16_1 = temp_s16_0;

//	if(ch == 1)
//	{
    /* printf("ch%d: %d  %d", (short)ch, temp_u16_0, temp_s16_1); */
//	}

    if (!(ctm_key_var->touch_key_state & BIT(ch))) {	//如果本通道按键目前是处于释放状态
        if (temp_s16_1 >= 0) {	//当前计数值小于低通值，放大后参与运算
            if (temp_s16_1 < (ctm_key_var->FLT1CFG2 >> 3)) {
                temp_s16_1 = temp_s16_1 * 8; //temp_s16_1 <<= 3;	//放大后参与运算
            } else {
                temp_s16_1 = ctm_key_var->FLT1CFG2;	//饱和，防止某些较大的正偏差导致错判
            }
        } else if (temp_s16_1 >= ctm_key_var->FLT1CFG1) {	//当前计数值小于低通值不多，正常参与运算

        } else {			//当前计数值小于低通值很多，缩小后参与运算
            temp_s16_1 = temp_s16_1 / 8;  //temp_s16_1 >>= 3;(有符号数右移自动扩展符号位???)
        }
    } else {		//如果本通道按键目前是处于按下状态, 缓慢降低释放计数值
        if (temp_s16_1 <= ctm_key_var->RELEASECFG1) {
            temp_s16_1 >>= 3;		//缩小后参与运算
        } else {
            temp_s16_1 = 0;
        }
    }

    temp_s32_0 += (s32)temp_s16_1;
    ctm_key_var->touch_release_buf[ch] = temp_s32_0;

//..............................................................................................
//按键按下与释放检测
//..............................................................................................
    if (temp_s16_0 <= ctm_key_var->PRESSCFG) {			//按键按下
        ctm_key_var->touch_key_state |= BIT(ch);
    } else if (temp_s16_0 >= ctm_key_var->RELEASECFG0) {	//按键释放
        ctm_key_var->touch_key_state &= ~BIT(ch);
    }
}

___interrupt
static void ctmu_isr_handle(void)
{
    CTMU_CON1 |= BIT(6);
    ctm_irq(&ctm_key_value, CTMU_RES, CTMU_CON1 & 0xf);
}

static void ctmu_port_init(const struct touch_key_port *port_list, u8 port_num)
{
    u8 i, j;

    for (i = 0; i < port_num; i++) {
        gpio_set_pull_down(port_list[i].port, 0);
        gpio_set_pull_up(port_list[i].port, 0);
        gpio_set_die(port_list[i].port, 0);
        gpio_set_direction(port_list[i].port, 1);

        for (j = 0; j < ARRAY_SIZE(ctmu_ch_table); j++) {
            if (ctmu_ch_table[j] == port_list[i].port) {
                CTMU_CON1 |= (BIT(j) << 16);
                port_index_mapping_talbe[i] = j;
            }
        }
    }
}

static void touch_ctmu_init(const struct touch_key_port *port, u8 num)
{
    CTMU_CON0 = 0;
    CTMU_CON1 = 0;

    //闸门时钟选择
    CTMU_CON0 |= (GATE_SOURCE_OSC << 2); //24 MHz
    CTMU_CON0 |= (GATE_SOURCE_PRE_DIV32768 << 5); //1.36ms
    /* CTMU_CON0 |= (1 << 8); //PRD */

    //充电电流
    CTMU_CON1 |= 3 << 8;  //0 ~ 7   2ua  //8ua
    //LDO enable
    CTMU_CON1 |= BIT(11);
    CTMU_CON1 |= BIT(13); //2.7V

    CTMU_CON1 |= BIT(6);  // Clear  Pending

    ctmu_port_init(port, num);
}

static void touch_ctmu_enable(u8 en)
{
    if (en) {
        CTMU_CON0 |= BIT(1); //Int Enable
        CTMU_CON0 |= BIT(0); //Moudle Enable
    } else {
        CTMU_CON0 &= ~BIT(1); //Int Disable
        CTMU_CON0 &= ~BIT(0); //Moudle Disable
    }
}

int ctmu_init(void *_data)
{
    const struct touch_key_platform_data *user_data = (const struct touch_key_platform_data *)_data;
    if (!user_data) {
        return -EINVAL;
    }

    __this = user_data;
    memset(&ctm_key_value, 0x0, sizeof(sCTMU_KEY_VAR));

    /*触摸按键参数配置*/
    ctm_key_value.FLT0CFG = 0;
    ctm_key_value.FLT1CFG0 = 7;
    ctm_key_value.FLT1CFG1 = -80;
    ctm_key_value.FLT1CFG2 = 10 * 128; //128 = 2^7

    ///调节灵敏度的主要参数
    ctm_key_value.PRESSCFG = user_data->press_cfg;
    ctm_key_value.RELEASECFG0 = user_data->release_cfg0;
    ctm_key_value.RELEASECFG1 = user_data->release_cfg1;

    memset(&ctm_key_value.touch_init_cnt, 0x10, CTMU_KEY_CH_MAX);

    if (user_data->num > CTMU_KEY_CH_MAX) {
        return -EINVAL;
    }

    touch_ctmu_init(user_data->port_list, user_data->num);

    request_irq(IRQ_CTM_IDX, 5, ctmu_isr_handle, 0);

    touch_ctmu_enable(1);

    return 0;
}

u8 get_ctmu_value(void)
{
    u8 key = NO_KEY;

    for (u8 i = 0; i < __this->num; i++) {
        if (ctm_key_value.touch_key_state & (u16)(BIT(i))) {
            key = i;
            break;
        }
    }

    return key;
}

#endif

