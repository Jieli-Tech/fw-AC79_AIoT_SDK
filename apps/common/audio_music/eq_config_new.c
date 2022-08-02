#include "system/includes.h"
#include "media/effects_adj.h"
#include "app_config.h"
#include "syscfg/config_interface.h"
#include "eq/eq_config.h"
#include "audio_eff_default_parm.h"

//eq_cfg_hw.bin中播歌eq曲线，当作用户自定义模式，参与效果切换.
#define EQ_FILE_CP_TO_CUSTOM  0

#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE

static const struct eq_seg_info eq_tab_normal[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, 0.7f},
};

static const struct eq_seg_info eq_tab_rock[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    -2, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    2, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   -2, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -2, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4, 0.7f},
};

static const struct eq_seg_info eq_tab_pop[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     3, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     1, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   -2, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   -4, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -4, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  -2, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   1, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2, 0.7f},
};

static const struct eq_seg_info eq_tab_classic[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     8, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    8, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   2, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2, 0.7f},
};

static const struct eq_seg_info eq_tab_country[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    -2, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    2, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    2, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4, 0.7f},
};

static const struct eq_seg_info eq_tab_jazz[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    4, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   4, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   2, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   3, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4, 0.7f},
};

static struct eq_seg_info eq_tab_custom[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, 0.7f},
};

// 默认系数表,用户可修改
const struct eq_seg_info *eq_type_tab[EQ_MODE_MAX] = {
    eq_tab_normal, eq_tab_rock, eq_tab_pop, eq_tab_classic, eq_tab_jazz, eq_tab_country, eq_tab_custom
};

// 默认系数表，每个表对应的总增益,用户可修改
static float global_gain_tab[EQ_MODE_MAX] = {0, 0, 0, 0, 0, 0, 0};

static const u8 tab_section_num[] = {
    ARRAY_SIZE(eq_tab_normal), ARRAY_SIZE(eq_tab_rock),
    ARRAY_SIZE(eq_tab_pop), ARRAY_SIZE(eq_tab_classic),
    ARRAY_SIZE(eq_tab_jazz), ARRAY_SIZE(eq_tab_country),
    ARRAY_SIZE(eq_tab_custom)
};

/*
 *mode:枚举型EQ_MODE
 *return 返回对应系数表的段数
 * */
u8 eq_get_table_nsection(EQ_MODE mode)
{
    if (mode >= ARRAY_SIZE(eq_type_tab)) {
        log_e("mode error %d\n", mode);
        return 0;
    }
    if (mode >= ARRAY_SIZE(tab_section_num)) {
        log_e("mode error %d\n", mode);
        return 0;
    }

    return tab_section_num[mode];
}

static u8 eq_mode = 0;
//eq效果表切换
int eq_mode_sw(void)
{
    eq_mode++;
    if (eq_mode >= ARRAY_SIZE(eq_type_tab)) {
        eq_mode = 0;
    }
    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[eq_mode];

    u8 nsection = ARRAY_SIZE(eq_tab_normal);
    if (nsection > mSECTION_MAX) {
        log_e("ERROR nsection:%d > mSECTION_MAX:%d ", nsection, mSECTION_MAX);
        return -1;
    }
    struct audio_eq *eq = get_cur_eq_hdl_by_name(AEID_MUSIC_EQ);
    if (!eq) {
        //eq未运行时，覆盖music eq的系数buf
        music_mode[nor_label].eq_parm.seg_num = nsection;
        music_mode[nor_label].eq_parm.global_gain = global_gain_tab[eq_mode];
        memcpy(music_mode[nor_label].eq_parm.seg, seg, sizeof(struct eq_seg_info)*nsection);
    }
    cur_eq_set_global_gain(AEID_MUSIC_EQ, global_gain_tab[eq_mode]);
    for (int i = 0; i < nsection; i++) {
        cur_eq_set_update(AEID_MUSIC_EQ, &seg[i], nsection, 0);
    }

    return 0;
}

//指定设置某个eq效果表
int eq_mode_set(EQ_MODE mode)
{
    if (mode >= ARRAY_SIZE(eq_type_tab)) {
        log_e("mode err %d\n", mode);
        return -1;
    }

    eq_mode = mode;

    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[eq_mode];

    u8 nsection = ARRAY_SIZE(eq_tab_normal);
    if (nsection > mSECTION_MAX) {
        log_e("ERROR nsection:%d > mSECTION_MAX:%d ", nsection, mSECTION_MAX);
        return -1;//
    }

    struct audio_eq *eq = get_cur_eq_hdl_by_name(AEID_MUSIC_EQ);
    if (!eq) {
        //eq未运行时，覆盖music eq的系数buf
        music_mode[nor_label].eq_parm.seg_num = nsection;
        music_mode[nor_label].eq_parm.global_gain = global_gain_tab[eq_mode];
        memcpy(music_mode[nor_label].eq_parm.seg, seg, sizeof(struct eq_seg_info)*nsection);
    }

    cur_eq_set_global_gain(AEID_MUSIC_EQ, global_gain_tab[eq_mode]);

    for (int i = 0; i < nsection; i++) {
        cur_eq_set_update(AEID_MUSIC_EQ, &seg[i], nsection, 0);
    }

    return 0;
}

//返回某个eq效果模式标号
EQ_MODE eq_mode_get_cur(void)
{
    return eq_mode;
}

/*----------------------------------------------------------------------------*/
/**@brief   设置custom系数表的某一段系数
  @param   seg->index：第几段(从0开始)
  @param   seg->iir_type:滤波器类型(EQ_IIR_TYPE_HIGH_PASS, EQ_IIR_TYPE_LOW_PASS, EQ_IIR_TYPE_BAND_PASS, EQ_IIR_TYPE_HIGH_SHELF,EQ_IIR_TYPE_LOW_SHELF)
  @param   seg->freq:中心截止频率(20~22kHz)
  @param   seg->gain:总增益(-18~18)
  @param   seg->q : q值（0.3~30）
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_seg(struct eq_seg_info *seg)
{
    struct eq_seg_info *tar_seg = eq_tab_custom;
    u8 index = seg->index;
    if (index > ARRAY_SIZE(eq_tab_custom)) {
        log_e("index %d > max_nsection %d", index, ARRAY_SIZE(eq_tab_custom));
        return -1;
    }
    memcpy(&tar_seg[index], seg, sizeof(struct eq_seg_info));
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    获取某个模式eq表内，某一段eq的信息
   @param
   @param
   @return  返回eq信息
   @note
*/
/*----------------------------------------------------------------------------*/
struct eq_seg_info *eq_mode_get_seg(EQ_MODE mode, u8 index)
{
    if (mode >= ARRAY_SIZE(eq_type_tab)) {
        log_e("mode error %d\n", mode);
        return NULL;
    }
    if (index >= eq_get_table_nsection(mode)) {
        log_e("index error %d\n", index);
        return NULL;
    }

    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[mode];
    return &seg[index];
}

/*----------------------------------------------------------------------------*/
/**@brief   获取custom系数表的增益、频率
  @param   index:哪一段
  @param   freq:中心截止频率
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_info(u16 index, int freq, float gain)
{
    struct eq_seg_info *seg = eq_mode_get_seg(EQ_MODE_CUSTOM, index);//获取某段eq系数
    if (!seg) {
        return -1;
    }
    seg->freq = freq;//修改freq gain
    seg->gain = gain;
    eq_mode_set_custom_seg(seg);//重设系数

    eq_mode_set(EQ_MODE_CUSTOM);//设置更新系数
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   设置用custom系数表一段eq的增益
  @param   index:哪一段
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_param(u16 index, int gain)
{
    struct eq_seg_info *seg = eq_mode_get_seg(EQ_MODE_CUSTOM, index);//获取某段eq系数
    if (!seg) {
        return -1;
    }
    seg->gain = gain;
    eq_mode_set_custom_seg(seg);//重设系数

    eq_mode_set(EQ_MODE_CUSTOM);//设置更新系数
    return 0;
}

void cp_eq_file_seg_to_custom_tab()
{
#if EQ_FILE_CP_TO_CUSTOM
    u8 nsection = music_mode[nor_label].eq_parm.seg_num;
    struct eq_seg_info *seg = eq_tab_custom;
    if (nsection > ARRAY_SIZE(eq_tab_custom)) {
        log_e("music nsection:%d > custom nsection:%d\n", nsection, ARRAY_SIZE(eq_tab_custom));
        return ;
    }
    global_gain_tab[EQ_MODE_CUSTOM] = music_mode[nor_label].eq_parm.global_gain;
    memcpy(seg, music_mode[nor_label].eq_parm.seg, sizeof(struct eq_seg_info)*nsection);
#endif
}

s8 eq_mode_get_gain(EQ_MODE mode, u16 index)
{
    struct eq_seg_info *seg = eq_mode_get_seg(mode, index);
    if (!seg) {
        return 0;
    }
    return seg->gain;
}

/*----------------------------------------------------------------------------*/
/**@brief   获取某eq系数表一段eq的中心截止频率
  @param   mode:EQ_MODE_NORMAL, EQ_MODE_ROCK,EQ_MODE_POP,EQ_MODE_CLASSIC,EQ_MODE_JAZZ,EQ_MODE_COUNTRY, EQ_MODE_CUSTOM
  @param   index:哪一段
  @return  中心截止频率
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_get_freq(EQ_MODE mode, u16 index)
{
    struct eq_seg_info *seg = eq_mode_get_seg(mode, index);
    if (!seg) {
        return 0;
    }
    return seg->freq;
}

/*----------------------------------------------------------------------------*/
/**@brief   设置用custom系数表一段eq的增益
  @param   index:哪一段
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
void set_global_gain(EQ_MODE mode, float global_gain)
{
    global_gain_tab[mode] = global_gain;
    cur_eq_set_global_gain(AEID_MUSIC_EQ, global_gain);
}

static int eq_init(void)
{
    int cpu_section = 0;
    //单声道或者立体声申请的cpu eq mem
    if (EQ_SECTION_MAX > 10) {
        u8 add_num = 0;
        if (hw_crossover_type0) {
            add_num = 4;//drc分频器使用eq硬件加速时、3段4阶使用最大eq段数24段(每段4个eq, 6声道)
        }
        cpu_section = EQ_SECTION_MAX - EQ_PRIV_SECTION_NUM + add_num;
    }

    audio_eq_init_new(cpu_section);

    return 0;
}
platform_initcall(eq_init);

#endif
