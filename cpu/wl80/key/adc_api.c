#include "generic/typedef.h"
#include "asm/clock.h"
#include "asm/adc_api.h"
#include "system/timer.h"
#include "asm/efuse.h"
#include "asm/p33.h"
#include "asm/power_interface.h"

static u32 adc_sample(u32 ch);
static volatile u16 _adc_res;
static volatile u16 cur_ch_value;
static u8 cur_ch = 0;

struct adc_info_t {
    u32 ch;
    u16 value;
};

#define ENABLE_OCCUPY_MODE 1

static struct adc_info_t adc_queue[ADC_MAX_CH + ENABLE_OCCUPY_MODE];

static u16 vbg_adc_value;
/* static u16 vbat_adc_value; */

u32 adc_add_sample_ch(u32 ch)
{
    u32 i = 0;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            break;
        } else if (adc_queue[i].ch == -1) {
            adc_queue[i].ch = ch;
            adc_queue[i].value = 1;
            printf("add sample ch %x\n", ch);
            break;
        }
    }
    return i;
}

u32 adc_remove_sample_ch(u32 ch)
{
    u32 i = 0;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            adc_queue[i].ch = -1;
            break;
        }
    }
    return i;
}

static u32 adc_get_next_ch(u32 cur_ch)
{
    for (int i = cur_ch + 1; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch != -1) {
            return i;
        }
    }
    return 0;
}

#define vbat_value_array_size   20
static u16 vbat_value_array[vbat_value_array_size];

static void vbat_value_push(u16 vbat_value)
{
    static u32 pos = 0;
    vbat_value_array[pos] = vbat_value;
    if (++pos == vbat_value_array_size) {
        pos = 0;
    }
}
static u16 vbat_value_avg(void)
{
    u32 i, sum = 0;
    for (i = 0; i < vbat_value_array_size; i++) {
        sum += vbat_value_array[i];
    }
    return sum / vbat_value_array_size;
}
u32 adc_get_value(u32 ch)
{
    if (ch == AD_CH_VBAT) {
        return vbat_value_avg();
    }

    if (ch == AD_CH_LDOREF) {
        return vbg_adc_value;
    }

    for (int i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            return adc_queue[i].value;
        }
    }
    return 0;
}

#define     CENTER0 800
#define     CENTER1 720
#define 	TRIM_MV	3
u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_ch_val)
{
    u32 adc_trim = get_vbg_trim();
    u32 tmp, tmp1, center;

    tmp1 = adc_trim & 0x1f;
    center = (adc_trim & BIT(6)) ? CENTER1 : CENTER0;
    tmp = (adc_trim & BIT(5)) ? center - tmp1 * TRIM_MV : center + tmp1 * TRIM_MV;
    return adc_ch_val * tmp / adc_vbg;
}

u32 adc_get_voltage(u32 ch)
{
    u32 adc_vbg = adc_get_value(AD_CH_LDOREF);
    u32 adc_res = adc_get_value(ch);
    u32 adc_trim = get_vbg_trim();
    u32 tmp, tmp1, center;

    tmp1 = adc_trim & 0x1f;
    center = (adc_trim & BIT(6)) ? CENTER1 : CENTER0;
    tmp = (adc_trim & BIT(5)) ? center - tmp1 * TRIM_MV : center + tmp1 * TRIM_MV;
    return adc_res * tmp / adc_vbg;
}

u32 adc_check_vbat_lowpower()
{
    return 0;
    /* u32 vbat = adc_get_value(AD_CH_VBAT); */
    /* return __builtin_abs(vbat - 255) < 5; */
}

void adc_audio_ch_select(u32 ch)
{
    SFR(JL_ANA->DAA_CON0, 12, 4, ch);
}

void adc_off()
{
    JL_ADC->CON = 0;
    JL_ADC->CON = 0;
    JL_ADC->CON = 0;
}
void adc_suspend()
{
    JL_ADC->CON &= ~BIT(4);
}
void adc_resume()
{
    JL_ADC->CON |= BIT(4);
}

void adc_enter_occupy_mode(u32 ch)
{
    if (JL_ADC->CON & BIT(4)) {
        return;
    }
    adc_queue[ADC_MAX_CH].ch = ch;
    cur_ch_value = adc_sample(ch);
}

void adc_exit_occupy_mode()
{
    adc_queue[ADC_MAX_CH].ch = -1;
}

u32 adc_occupy_run()
{
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        while (1) {
            asm volatile("idle");//wait isr
            if (_adc_res != (u16) - 1) {
                break;
            }
        }
        if (_adc_res == 0) {
            _adc_res ++;
        }
        adc_queue[ADC_MAX_CH].value = _adc_res;
        _adc_res = cur_ch_value;
        return adc_queue[ADC_MAX_CH].value;
    }
    return 0;
}

u32 adc_get_occupy_value()
{
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        return adc_queue[ADC_MAX_CH].value;
    }
    return 0;
}

u8 __attribute__((weak)) adc_io_reuse_enter(u32 ch)
{
    return 0;
}

u8 __attribute__((weak)) adc_io_reuse_exit(u32 ch)
{
    return 0;
}

___interrupt
static void adc_isr()
{
    _adc_res = JL_ADC->RES;

    u32 ch;
    ch = (JL_ADC->CON & 0xf00) >> 8;
    adc_io_reuse_exit(ch);

    adc_pmu_detect_en(AD_CH_WVDD >> 16);
    JL_ADC->CON = BIT(6);
    JL_ADC->CON = 0;
}

static u32 adc_sample(u32 ch)
{
    const u32 tmp_adc_res = _adc_res;
    _adc_res = (u16) - 1;

    if (adc_io_reuse_enter(ch)) {
        _adc_res = adc_get_value(ch);
        return tmp_adc_res;
    }

    u32 adc_con = 0;
    SFR(adc_con, 0, 3, 0b110);//div 96

    adc_con |= (0xf << 12); //启动延时控制，实际启动延时为此数值*8个ADC时钟
    adc_con |= BIT(3);
    adc_con |= BIT(6);
    adc_con |= BIT(5);//ie

    SFR(adc_con, 8, 4, ch & 0xf);

    if ((ch & 0xffff) == AD_CH_PMU) {
        adc_pmu_detect_en(ch >> 16);
    } else if ((ch & 0xffff) == AD_CH_AUDIO) {
        adc_audio_ch_select(ch >> 16);
    }

    JL_ADC->CON = adc_con;
    JL_ADC->CON |= BIT(4);//en
    JL_ADC->CON |= BIT(6);//kistart

    return tmp_adc_res;
}

#define VBG_VBAT_SCAN_CNT    100
static void adc_scan(void *priv)
{
    static u16 vbg_vbat_cnt = VBG_VBAT_SCAN_CNT;
    static u8 vbg_vbat_step = 0;
    static u16 old_adc_res;
    static u16 tmp_vbg_adc_value;

    if (adc_queue[ADC_MAX_CH].ch != -1) {//occupy mode
        return;
    }

    if (JL_ADC->CON & BIT(4)) {
        return;
    }

    /* if (!(JL_ADC->CON & BIT(4))) { //adc disable */
    /*     return;                                  */
    /* }                                            */

    if (++vbg_vbat_cnt > VBG_VBAT_SCAN_CNT) {
        if (vbg_vbat_step == 0) {
            vbg_vbat_step = 1;
            old_adc_res = _adc_res;
            adc_sample(AD_CH_LDOREF);
            return;
        } else if (vbg_vbat_step == 1) {
            vbg_vbat_step = 2;
            tmp_vbg_adc_value = adc_sample(AD_CH_VBAT);
            //printf("vbg = %d\n", tmp_vbg_adc_value);
            return;
        } else if (vbg_vbat_step == 2) {
            vbg_vbat_step = 0;
            vbat_value_push(_adc_res);
            vbg_adc_value = tmp_vbg_adc_value;
            //printf("vbg = %d  vbat = %d\n", vbg_adc_value, vbat_adc_value);
            _adc_res = old_adc_res;
            vbg_vbat_cnt = 0;
        }
    }

    u8 next_ch;

    next_ch = adc_get_next_ch(cur_ch);

    adc_queue[cur_ch].value = adc_sample(adc_queue[next_ch].ch);

    cur_ch = next_ch;
}

//获取当前采集ad的通道总数
u8 get_cur_total_ad_ch(void)
{
    u8 total_ch = 0;
    u8 i = 0;
    while (i < ADC_MAX_CH) {
        if (adc_queue[i].ch != -1) {
            total_ch++;
        }
        /* printf("i:%d,ch:%x\n",i,adc_queue[i].ch); */
        i++;
    }
    /* printf("total_ch:%d\n",total_ch); */
    return total_ch;
}

static void __adc_init(void)
{
    memset(adc_queue, 0xff, sizeof(adc_queue));

    JL_ADC->CON = 0;

    adc_sample(AD_CH_VBAT);

    for (u8 i = 0; i < vbat_value_array_size; i++) {
        while (!(JL_ADC->CON & BIT(7)));
        vbat_value_array[i] = JL_ADC->RES;
        JL_ADC->CON |= BIT(6);
    }
    printf("vbat_adc_value = %d\n", vbat_value_avg());

    vbg_adc_value = 0;
    adc_sample(AD_CH_LDOREF);
    for (u8 i = 0; i < 10; i++) {
        while (!(JL_ADC->CON & BIT(7)));
        vbg_adc_value += JL_ADC->RES;
        JL_ADC->CON |= BIT(6);
    }
    vbg_adc_value /= 10;
    printf("vbg_adc_value = %d\n", vbg_adc_value);

    _adc_res = 1;

    request_irq(IRQ_SARADC_IDX, 0, adc_isr, 0);

    sys_s_hi_timer_add(NULL, adc_scan, 2); //2ms
}

static u32 get_wvdd_voltage()
{
    u32 vbg_value = 0;
    u32 wvdd_value = 0;

    adc_pmu_detect_en(0);
    adc_sample(AD_CH_LDOREF);

    for (int i = 0; i < 10; i++) {
        while (!(JL_ADC->CON & BIT(7)));
        vbg_value += JL_ADC->RES;
        JL_ADC->CON |= BIT(6);
    }

    adc_sample(AD_CH_WVDD);
    for (int i = 0; i < 10; i++) {
        while (!(JL_ADC->CON & BIT(7)));
        wvdd_value += JL_ADC->RES;
        JL_ADC->CON |= BIT(6);
    }

    u32 adc_vbg = vbg_value / 10;
    u32 adc_res = wvdd_value / 10;
    u32 adc_trim = get_vbg_trim();
    u32 tmp, tmp1, center;

    tmp1 = adc_trim & 0x1f;
    center = (adc_trim & BIT(6)) ? CENTER1 : CENTER0;
    tmp = (adc_trim & BIT(5)) ? center - tmp1 * TRIM_MV : center + tmp1 * TRIM_MV;
    adc_res = adc_res * tmp / adc_vbg;
    /* printf("adc_res %d mv vbg:%d wvdd:%d %x\n", adc_res, vbg_value / 10, wvdd_value / 10,adc_trim); */
    return adc_res;
}

static void wvdd_trim()
{
    static u8 wvdd_lev = 0;

    P33_CON_SET(P3_WLDO06_AUTO, 0, 3, wvdd_lev);
    WLDO06_EN(1);
    delay(2000);//1ms

    do {
        P33_CON_SET(P3_WLDO06_AUTO, 0, 3, wvdd_lev);
        delay(2000);//1ms * n
        if (get_wvdd_voltage() > 700) {
            break;
        }
        wvdd_lev ++;
    } while (wvdd_lev < 8);

    WLDO06_EN(0);

    printf("wvdd_lev: %d\n", wvdd_lev);

    /* power_set_wvdd(wvdd_lev); */
}

static void vddiom_trim(void)
{
    u32 vbg_value = 0;

    adc_pmu_detect_en(0);
    adc_sample(AD_CH_LDOREF);

    for (int i = 0; i < 10; i++) {
        while (!(JL_ADC->CON & BIT(7)));
        vbg_value += JL_ADC->RES;
        JL_ADC->CON |= BIT(6);
    }

    vbg_value /= 10;

    u32 vbg_trim = get_vbg_trim();
    u32 tmp1 = vbg_trim & 0x1f;
    u32 center = (vbg_trim & BIT(6)) ? CENTER1 : CENTER0;
    u32 vbg_vol = (vbg_trim & BIT(5)) ? center - tmp1 * TRIM_MV : center + tmp1 * TRIM_MV;
    u32 vddio_vol = vbg_vol * 1023 / vbg_value;
    printf("vddio_vol %d (mv) %d %x\n", vddio_vol, vbg_value, vbg_trim);
    if (vddio_vol < 3400) {
        VDDIOM_VOL_SEL(VDDIOM_VOL_36V);
    }
}

void adc_init()
{
    if (GET_VDDIOM_VOL() == VDDIOM_VOL_34V) {
        vddiom_trim();
    }

    //trim wvdd
    /* wvdd_trim(); */
    __adc_init();
}

void adc_vbg_init()
{

}
