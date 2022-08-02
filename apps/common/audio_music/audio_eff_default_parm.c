#include "audio_eff_default_parm.h"
#include "media/effects_adj.h"
#include "eq/eq_config.h"
#include "app_config.h"
#include "math.h"

#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
#define LOG_TAG     "[AUDIO_EFFECT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"


float powf(float x, float y);
extern struct mode_list *get_group_list(u16 module_name);
extern const struct eq_seg_info *eq_type_tab[EQ_MODE_MAX];

/*
 *混响音效默认eq系数
 * */
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
const struct eq_seg_info mic_eff_eq_tab[5] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 200,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 300,   0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 400,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 400,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
};
#endif

/*
 *通话下行eq系数表
 * */
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
const struct eq_seg_info phone_eq_tab_normal[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 200,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 300,   0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 400,   0, 0.7f},
};
#endif

/*
 *通话上行eq系数表
 * */
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AEC_UL_EQ_ENABLE
const struct eq_seg_info ul_eq_tab_normal[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 200,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 300,   0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 400,   0, 0.7f},
};
#endif

#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_DYNAMIC_EQ_ENABLE
const struct eq_seg_info music_eq2_tab[] = {
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
#endif

#if defined(TWO_POINT_X_SPECIAL_CONFIG) && TWO_POINT_X_SPECIAL_CONFIG
const struct eq_seg_info eq_tab_bass_treb[] = {
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
#endif

struct music_parm_tool_set music_mode[mode_add];

struct music_parm_tool_set *get_audio_eff_music_mode(void)
{
    return &music_mode[0];
}

//原来的是这个if
/* #if mpdeadd == 2 */
#if mode_add == 2
static u16 music_mode_seq[] = {music_mode_seq0, linein_mode_seq};//音乐模式的标号
#else
static u16 music_mode_seq[] = {music_mode_seq0};//音乐模式的标号
#endif
static const u16 eff_mode_seq[8] = {mic_mode_seq0, mic_mode_seq1, mic_mode_seq2, mic_mode_seq3, mic_mode_seq4, mic_mode_seq5, mic_mode_seq6, mic_mode_seq7}; //混响模式标号

void vbass_prev_gain_parm_default_init()
{
#if AUDIO_VBASS_CONFIG
    u8 calc = mode_add;
    float gain = 0;
    for (u8 tar = 0; tar < calc; tar++) {
        //gain
        vbass_prev_gain_parm[tar].is_bypass = 0;
        vbass_prev_gain_parm[tar].parm.gain[0] = powf(10, gain / 20.0f); //db转mag,工具传下来的值
        vbass_prev_gain_parm[tar].parm.gain[1] = powf(10, gain / 20.0f); //db转mag,工具传下来的值
    }
#endif
}

void vbass_prev_gain_file_analyze_init()
{
#if AUDIO_VBASS_CONFIG
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        void *tar_buf = &vbass_prev_gain_parm[tar];
        u16 tar_len = sizeof(Gain_Process_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], EFF_MUSIC_VBASS_PREV_GAIN, tar_buf, tar_len);
    }
#endif
}

void music_vbass_parm_default_init()
{
#if AUDIO_VBASS_CONFIG
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        vbass_parm[tar].is_bypass = 0;
        vbass_parm[tar].parm.ratio = 10;
        vbass_parm[tar].parm.boost = 1;
        vbass_parm[tar].parm.fc = 100;
    }
#endif
}

void music_vbass_file_analyze_init()
{
#if AUDIO_VBASS_CONFIG
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &vbass_parm[tar];
        u16 tar_len = sizeof(VirtualBass_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], EFF_MUSIC_VBASS, tar_buf, tar_len);
    }
#endif
}

void mix_gain_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        mic_eff->mix_gain.gain1 = 1;
        mic_eff->mix_gain.gain2 = 1;
        mic_eff->mix_gain.gain3 = 1;
    }
#endif
}

void mix_gain_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u16 group_id = EFF_MIC_MIX_GAIN;
        void *tar_buf = &mic_eff->mix_gain;
        u16 tar_len = sizeof(mic_eff->mix_gain);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
    }
#endif
}

void mic_voice_changer_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //voice_changer
#if defined(TCFG_MIC_VOICE_CHANGER_ENABLE) && TCFG_MIC_VOICE_CHANGER_ENABLE
        mic_eff->voicechanger_parm.is_bypass = 0;
        mic_eff->voicechanger_parm.parm.effect_v = 0;
        mic_eff->voicechanger_parm.parm.shiftv = 56;
        mic_eff->voicechanger_parm.parm.formant_shift = 90;
#endif
    }
#endif
}

void mic_voice_changer_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //voice_changer
#if defined(TCFG_MIC_VOICE_CHANGER_ENABLE) && TCFG_MIC_VOICE_CHANGER_ENABLE
        void *tar_buf = (void *)&mic_eff->voicechanger_parm;
        u16 tar_len = sizeof(mic_eff->voicechanger_parm);
        eff_file_analyze(eff_mode_seq[index], EFF_MIC_VOICE_CHANGER, tar_buf, tar_len);
#endif
    }
#endif
}

void high_bass_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    u8 calc = mode_add;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        high_bass_drc_parm[tar].is_bypass = 0;
        high_bass_drc_parm[tar].parm.attacktime = 10;
        high_bass_drc_parm[tar].parm.releasetime = 300;
        high_bass_drc_parm[tar].parm.inputgain = 0;
        high_bass_drc_parm[tar].parm.outputgain = 0;
        high_bass_drc_parm[tar].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(high_bass_drc_parm[tar].parm.threshold, group, sizeof(group));
        high_bass_drc_parm[tar].parm.rms_time = 25;
        high_bass_drc_parm[tar].parm.algorithm = 0;
        high_bass_drc_parm[tar].parm.mode = 1;
    }
#endif
}

void high_bass_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        u16 group_id = EFF_MUSIC_HIGH_BASS_DRC;
        void *tar_buf = &high_bass_drc_parm[tar];
        u16 tar_len = sizeof(wdrc_struct_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void aux_music_low_wdrc_parm_default_init()
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    /* u8 calc = mode_add; */
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(EFF_AUX_DRC, &index, aux_list_label);
    u8 i = index;
    u16 tar = aux_label;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    /* for (int tar = 0; tar < calc; tar++){ */
    music_mode[tar].drc_parm.wdrc_parm[i].is_bypass = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.attacktime = 10;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.releasetime = 300;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.inputgain = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.outputgain = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
    memcpy(music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold, group, sizeof(group));
    music_mode[tar].drc_parm.wdrc_parm[i].parm.rms_time = 25;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.algorithm = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.mode = 1;
    /* } */
#endif
#endif
}

void aux_music_low_wdrc_file_analyze_init()
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(EFF_AUX_DRC, &index, aux_list_label);
    u16 i = index;
    u8 tar = aux_label;
    /* for (int tar = 0; tar < calc; tar++){ */
    struct mode_list *list = get_group_list(drc_name);
    if (list) {
        u16 group_id = list->group_id[i];
        void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
        u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
    /* } */
#endif
#endif
}

void aux_music_eq_parm_default_init()
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 tar = aux_label;
    u8 seg_num =  eq_get_table_nsection(EQ_MODE_NORMAL);
    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[EQ_MODE_NORMAL];
    music_mode[tar].eq_parm.global_gain = 0;
    music_mode[tar].eq_parm.seg_num = seg_num;
    memcpy(music_mode[tar].eq_parm.seg, seg, sizeof(struct eq_seg_info)*seg_num);
#endif
#endif
}

void aux_music_eq_file_analyze_init()
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 tar = aux_label;
    u16 group_id = EFF_AUX_EQ;
    void *tar_buf = &music_mode[tar].eq_parm;
    u16 tar_len = sizeof(struct music_eq_tool);
    if (!eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len)) {
        int index = get_index_by_group_id(group_id, tar);
        if (index >= 0) {
            struct music_eq_tool *parm = (struct music_eq_tool *)tar_buf;
            set_list_nsection(tar, index, parm->seg_num);//根据文件段数，设定在线调试段数
        }
    }
    music_eq_printf(tar_buf);
#endif
#endif
}

void rl_eq_parm_default_init()
{
    //rl_wdrc
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        u8 seg_num =  eq_get_table_nsection(EQ_MODE_NORMAL);
        struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[EQ_MODE_NORMAL];
        rl_eq_parm[tar].global_gain = 0;
        rl_eq_parm[tar].seg_num = seg_num;
        memcpy(rl_eq_parm[tar].seg, seg, sizeof(struct eq_seg_info)*seg_num);
    }
#endif
#endif
}

void rl_eq_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //rl_eq
        u16 group_id = EFF_MUSIC_RL_EQ;
        void *tar_buf = &rl_eq_parm[tar];
        u16 tar_len = sizeof(struct music_eq_tool);
        if (!eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len)) {
            int index = get_index_by_group_id(group_id, tar);
            if (index >= 0) {
                struct music_eq_tool *parm = (struct music_eq_tool *)tar_buf;
                set_list_nsection(tar, index, parm->seg_num);//根据文件段数，设定在线调试段数
            }
        }
        music_eq_printf(tar_buf);
    }
#endif
#endif
}

/*
 *rr通道eq默认系数初始化
 * */
void rr_eq_parm_default_init()
{
    //rr_eq
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        u8 seg_num =  eq_get_table_nsection(EQ_MODE_NORMAL);
        struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[EQ_MODE_NORMAL];
        rr_eq_parm[tar].global_gain = 0;
        rr_eq_parm[tar].seg_num = seg_num;
        memcpy(rr_eq_parm[tar].seg, seg, sizeof(struct eq_seg_info)*seg_num);
    }
#endif
#endif
}

/*
 *rr通道eq文件系数解析
 * */
void rr_eq_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //rl_eq
        u16 group_id = EFF_MUSIC_RR_EQ;
        void *tar_buf = &rr_eq_parm[tar];
        u16 tar_len = sizeof(struct music_eq_tool);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        music_eq_printf(tar_buf);
    }
#endif
#endif
}

/*
 *fr通道eq默认系数初始化
 * */
void fr_eq_parm_default_init()
{
    //fr_eq
    /* #if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE */
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        u8 seg_num =  eq_get_table_nsection(EQ_MODE_NORMAL);
        struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[EQ_MODE_NORMAL];
        fr_eq_parm[tar].global_gain = 0;
        fr_eq_parm[tar].seg_num = seg_num;
        memcpy(fr_eq_parm[tar].seg, seg, sizeof(struct eq_seg_info)*seg_num);
    }
#endif
    /* #endif */
}

/*
 *fr通道eq文件系数解析
 * */
void fr_eq_file_analyze_init()
{
    /* #if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE */
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //fr_eq
        u16 group_id = EFF_MUSIC_FR_EQ;
        void *tar_buf = &fr_eq_parm[tar];
        u16 tar_len = sizeof(struct music_eq_tool);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        music_eq_printf(tar_buf);
    }
#endif
    /* #endif */
}

void low_pass_parm_default_init()
{
#if defined(SOUND_TRACK_2_P_X_CH_CONFIG) && SOUND_TRACK_2_P_X_CH_CONFIG && HIGH_GRADE_LOW_PASS_FILTER_EN
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //low pass
        low_pass_parm[tar].is_bypass = 0;
        low_pass_parm[tar].low_pass.fc = 100;
        low_pass_parm[tar].low_pass.order = 4;
        low_pass_parm[tar].low_pass.type = 1;
    }
#endif
}

void low_pass_file_analyze_init()
{
#if defined(SOUND_TRACK_2_P_X_CH_CONFIG) && SOUND_TRACK_2_P_X_CH_CONFIG && HIGH_GRADE_LOW_PASS_FILTER_EN
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //low pass
        group_id = EFF_MUSIC_RL_RR_LOW_PASS;
        tar_buf = &low_pass_parm[tar];
        tar_len = sizeof(LowPassParam_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
        log_info("low_pass_parm.is_bypass %d\n", low_pass_parm[tar].is_bypass);
        struct advance_iir *low_p = &low_pass_parm[tar].low_pass;
        log_info("low_p->fc %d, low_p->order %d, low_p->type %d\n", low_p->fc, low_p->order, low_p->type);
    }
#endif
#endif
}

void uplink_narrowband_eq_parm_default_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AEC_UL_EQ_ENABLE
    u8 index = ul_narrowband_label;
    phone_mode[index].eq_parm.global_gain = 0;
    phone_mode[index].eq_parm.seg_num = ARRAY_SIZE(ul_eq_tab_normal);
    memcpy(phone_mode[index].eq_parm.seg, ul_eq_tab_normal, sizeof(ul_eq_tab_normal));
#endif
}

void uplink_narrowband_eq_file_analyze_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AEC_UL_EQ_ENABLE
    u8 index = ul_narrowband_label;
    u16 mode_seq = aec_mode_seq;
    u16 group_id = EFF_AEC_NARROWBAND_EQ;
    void *tar_buf = &phone_mode[index].eq_parm;
    u16 tar_len = sizeof(phone_mode[index].eq_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
#endif
}

void uplink_wideband_eq_parm_default_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AEC_UL_EQ_ENABLE
    u8 index = ul_wideband_label;
    phone_mode[index].eq_parm.global_gain = 0;
    phone_mode[index].eq_parm.seg_num = ARRAY_SIZE(ul_eq_tab_normal);
    memcpy(phone_mode[index].eq_parm.seg, ul_eq_tab_normal, sizeof(ul_eq_tab_normal));
#endif
}

void uplink_wideband_eq_file_analyze_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AEC_UL_EQ_ENABLE
    u8 index = ul_wideband_label;
    u16 mode_seq = aec_mode_seq;
    u16 group_id = EFF_AEC_WIDEBAND_EQ;
    void *tar_buf = &phone_mode[index].eq_parm;
    u16 tar_len = sizeof(phone_mode[index].eq_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
#endif
}

void downlink_narrowband_eq_parm_default_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
    u8 index = dl_narrowband_label;
    phone_mode[index].eq_parm.global_gain = 0;
    phone_mode[index].eq_parm.seg_num = ARRAY_SIZE(phone_eq_tab_normal);
    memcpy(phone_mode[index].eq_parm.seg, phone_eq_tab_normal, sizeof(phone_eq_tab_normal));
#endif
}

void downlink_narrowband_eq_file_analyze_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
    u8 index = dl_narrowband_label;
    u16 mode_seq = phone_mode_seq;
    u16 group_id = EFF_PHONE_NARROWBAND_EQ;
    void *tar_buf = &phone_mode[index].eq_parm;
    u16 tar_len = sizeof(phone_mode[index].eq_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
#endif
}

void downlink_wideband_eq_parm_default_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
    u8 index = dl_wideband_label;
    phone_mode[index].eq_parm.global_gain = 0;
    phone_mode[index].eq_parm.seg_num = ARRAY_SIZE(phone_eq_tab_normal);
    memcpy(phone_mode[index].eq_parm.seg, phone_eq_tab_normal, sizeof(phone_eq_tab_normal));
#endif
}

void downlink_wideband_eq_file_analyze_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_PHONE_EQ_ENABLE
    u8 index = dl_wideband_label;
    u16 mode_seq = phone_mode_seq;
    u16 group_id = EFF_PHONE_WIDEBAND_EQ;
    void *tar_buf = &phone_mode[index].eq_parm;
    u16 tar_len = sizeof(phone_mode[index].eq_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
#endif
}

void mic_eq0_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0 ; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 0;
        mic_eff->eq_parm[i].global_gain = 0;
        mic_eff->eq_parm[i].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(mic_eff->eq_parm[i].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

void mic_eq0_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 0;
        u16 group_id = EFF_MIC_EQ0;
        void *tar_buf = (void *)&mic_eff->eq_parm[i];
        u16 tar_len = sizeof(mic_eff->eq_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        eq_printf(tar_buf);
    }
#endif
}

void mic_eq1_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 1;
        mic_eff->eq_parm[i].global_gain = 0;
        mic_eff->eq_parm[i].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(mic_eff->eq_parm[i].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

void mic_eq1_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 1;
        u16 group_id = EFF_MIC_EQ1;
        void *tar_buf = (void *)&mic_eff->eq_parm[i];
        u16 tar_len = sizeof(mic_eff->eq_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        eq_printf(tar_buf);
    }
#endif
}

void mic_eq2_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 2;
        mic_eff->eq_parm[i].global_gain = 0;
        mic_eff->eq_parm[i].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(mic_eff->eq_parm[i].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

void mic_eq2_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 2;
        u16 group_id = EFF_MIC_EQ2;
        void *tar_buf = (void *)&mic_eff->eq_parm[i];
        u16 tar_len = sizeof(mic_eff->eq_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        eq_printf(tar_buf);
    }
#endif
}

void mic_eq3_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 3;
        mic_eff->eq_parm[i].global_gain = 0;
        mic_eff->eq_parm[i].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(mic_eff->eq_parm[i].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

void mic_eq3_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 3;
        u16 group_id = EFF_MIC_EQ3;
        void *tar_buf = (void *)&mic_eff->eq_parm[i];
        u16 tar_len = sizeof(mic_eff->eq_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        eq_printf(tar_buf);
    }
#endif
}

void mic_eq4_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 4;
        mic_eff->eq_parm[i].global_gain = 0;
        mic_eff->eq_parm[i].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(mic_eff->eq_parm[i].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

void mic_eq4_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 4;
        u16 group_id = EFF_MIC_EQ4;
        void *tar_buf = (void *)&mic_eff->eq_parm[i];
        u16 tar_len = sizeof(mic_eff->eq_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        eq_printf(tar_buf);
    }
#endif
}

void high_bass_eq_parm_default_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //high_bass_eq
        high_bass_eq_parm[tar].global_gain = 0;
#if defined(TWO_POINT_X_SPECIAL_CONFIG)&&TWO_POINT_X_SPECIAL_CONFIG
        high_bass_eq_parm[tar].seg_num = ARRAY_SIZE(eq_tab_bass_treb);
        memcpy(high_bass_eq_parm[tar].seg, eq_tab_bass_treb, sizeof(eq_tab_bass_treb));
#else
        high_bass_eq_parm[tar].seg_num = 0;
#endif
    }
#endif
}

void high_bass_eq_file_analyze_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //eq
        u16 group_id = EFF_MUSIC_HIGH_BASS_EQ;
        void *tar_buf = &high_bass_eq_parm[tar];
        u16 tar_len = sizeof(struct music_eq_tool);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        music_eq_printf(tar_buf);
    }
#endif
}

void music_eq_parm_default_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 tar = nor_label;
    u8 seg_num = eq_get_table_nsection(EQ_MODE_NORMAL);
    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[EQ_MODE_NORMAL];
    music_mode[tar].eq_parm.global_gain = 0;
    music_mode[tar].eq_parm.seg_num = seg_num;
    memcpy(music_mode[tar].eq_parm.seg, seg, sizeof(struct eq_seg_info)*seg_num);
#endif
}

void music_eq_file_analyze_init()
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    u8 tar = nor_label;
    u16 group_id = EFF_MUSIC_EQ;
    void *tar_buf = &music_mode[tar].eq_parm;
    u16 tar_len = sizeof(struct music_eq_tool);
    if (!eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len)) {
        int index = get_index_by_group_id(group_id, tar);
        if (index >= 0) {
            struct music_eq_tool *parm = (struct music_eq_tool *)tar_buf;
            set_list_nsection(tar, index, parm->seg_num);//根据文件段数，设定在线调试段数
        }
    }
    music_eq_printf(tar_buf);
#endif
}

void music_eq2_parm_default_init()
{
#if TCFG_DYNAMIC_EQ_ENABLE
    //eq2
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        music_eq2_parm[tar].global_gain = 0;
        music_eq2_parm[tar].seg_num = ARRAY_SIZE(music_eq2_tab);
        memcpy(music_eq2_parm[tar].seg, music_eq2_tab, sizeof(music_eq2_tab));
    }
#endif
}

void music_eq2_file_analyze_init()
{
#if TCFG_DYNAMIC_EQ_ENABLE
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        u16 group_id = EFF_MUSIC_EQ2;
        void *tar_buf = &music_eq2_parm[tar];
        u16 tar_len = sizeof(struct music_eq2_tool);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        music_eq2_printf(tar_buf);
    }
#endif
}

void mic_wdrc0_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 0;
        //drc
        struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
        mic_eff->drc_parm[i].is_bypass = 0;
        mic_eff->drc_parm[i].parm.attacktime = 10;
        mic_eff->drc_parm[i].parm.releasetime = 300;
        mic_eff->drc_parm[i].parm.inputgain = 0;
        mic_eff->drc_parm[i].parm.outputgain = 0;
        mic_eff->drc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(mic_eff->drc_parm[i].parm.threshold, group, sizeof(group));
        mic_eff->drc_parm[i].parm.rms_time = 25;
        mic_eff->drc_parm[i].parm.algorithm = 0;
        mic_eff->drc_parm[i].parm.mode = 1;
    }
#endif
}

void mic_wdrc0_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        int i = 0;
        u16 group_id = EFF_MIC_DRC0;
        void *tar_buf = (void *)&mic_eff->drc_parm[i];
        u16 tar_len = sizeof(mic_eff->drc_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void mic_wdrc1_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 1;
        //drc
        struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
        mic_eff->drc_parm[i].is_bypass = 0;
        mic_eff->drc_parm[i].parm.attacktime = 10;
        mic_eff->drc_parm[i].parm.releasetime = 300;
        mic_eff->drc_parm[i].parm.inputgain = 0;
        mic_eff->drc_parm[i].parm.outputgain = 0;
        mic_eff->drc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(mic_eff->drc_parm[i].parm.threshold, group, sizeof(group));
        mic_eff->drc_parm[i].parm.rms_time = 25;
        mic_eff->drc_parm[i].parm.algorithm = 0;
        mic_eff->drc_parm[i].parm.mode = 1;
    }
#endif
}

void mic_wdrc1_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm  *mic_eff = &eff_mode[index];
        int i = 1;
        u16 group_id = EFF_MIC_DRC1;
        void *tar_buf = (void *)&mic_eff->drc_parm[i];
        u16 tar_len = sizeof(mic_eff->drc_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void mic_wdrc2_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 2;
        //drc
        struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
        mic_eff->drc_parm[i].is_bypass = 0;
        mic_eff->drc_parm[i].parm.attacktime = 10;
        mic_eff->drc_parm[i].parm.releasetime = 300;
        mic_eff->drc_parm[i].parm.inputgain = 0;
        mic_eff->drc_parm[i].parm.outputgain = 0;
        mic_eff->drc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(mic_eff->drc_parm[i].parm.threshold, group, sizeof(group));
        mic_eff->drc_parm[i].parm.rms_time = 25;
        mic_eff->drc_parm[i].parm.algorithm = 0;
        mic_eff->drc_parm[i].parm.mode = 1;
    }
#endif
}

void mic_wdrc2_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        int i = 2;
        u16 group_id = EFF_MIC_DRC2;
        void *tar_buf = (void *)&mic_eff->drc_parm[i];
        u16 tar_len = sizeof(mic_eff->drc_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void mic_wdrc3_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 3;
        //drc
        struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
        mic_eff->drc_parm[i].is_bypass = 0;
        mic_eff->drc_parm[i].parm.attacktime = 10;
        mic_eff->drc_parm[i].parm.releasetime = 300;
        mic_eff->drc_parm[i].parm.inputgain = 0;
        mic_eff->drc_parm[i].parm.outputgain = 0;
        mic_eff->drc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(mic_eff->drc_parm[i].parm.threshold, group, sizeof(group));
        mic_eff->drc_parm[i].parm.rms_time = 25;
        mic_eff->drc_parm[i].parm.algorithm = 0;
        mic_eff->drc_parm[i].parm.mode = 1;
    }
#endif
}

void mic_wdrc3_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        int i = 3;
        u16 group_id = EFF_MIC_DRC3;
        void *tar_buf = (void *)&mic_eff->drc_parm[i];
        u16 tar_len = sizeof(mic_eff->drc_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void mic_wdrc4_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        u8 i = 4;
        //drc
        struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
        mic_eff->drc_parm[i].is_bypass = 0;
        mic_eff->drc_parm[i].parm.attacktime = 10;
        mic_eff->drc_parm[i].parm.releasetime = 300;
        mic_eff->drc_parm[i].parm.inputgain = 0;
        mic_eff->drc_parm[i].parm.outputgain = 0;
        mic_eff->drc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(mic_eff->drc_parm[i].parm.threshold, group, sizeof(group));
        mic_eff->drc_parm[i].parm.rms_time = 25;
        mic_eff->drc_parm[i].parm.algorithm = 0;
        mic_eff->drc_parm[i].parm.mode = 1;
    }
#endif
}

void mic_wdrc4_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        int i = 4;
        u16 group_id = EFF_MIC_DRC4;
        void *tar_buf = (void *)&mic_eff->drc_parm[i];
        u16 tar_len = sizeof(mic_eff->drc_parm[i]);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rl_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    /* for (int i = 0; i < 4; i++) { */
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RL_LOW_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rl_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rl_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rl_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rl_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rl_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rl_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rl_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    //rl_wdrc
    u16 group_id = EFF_MUSIC_RL_LOW_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rl_mid_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RL_MID_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rl_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rl_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rl_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rl_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rl_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rl_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rl_mid_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    u16 group_id = EFF_MUSIC_RL_MID_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rl_high_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RL_HIGH_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rl_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rl_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rl_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rl_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rl_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rl_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rl_high_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    u16 group_id = EFF_MUSIC_RL_HIGH_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rl_whole_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RL_WHOLE_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rl_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rl_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rl_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rl_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rl_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rl_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rl_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rl_whole_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    u16 group_id = EFF_MUSIC_RL_WHOLE_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}
void music_rl_crossover_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rl_drc_parm[tar].crossover.way_num = 2;
        rl_drc_parm[tar].crossover.N = 2;
        rl_drc_parm[tar].crossover.low_freq = 200;
    }
#endif
#endif
#endif
}

void music_rl_crossover_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    u16 drc_name[] = {AEID_MUSIC_RL_DRC, AEID_AUX_RL_DRC};
    for (int tar = 0; tar < calc; tar++) {
        struct mode_list *list = get_group_list(drc_name[tar]);
        if (list) {
            u16 group_id = list->group_id[4];
            void *tar_buf = &rl_drc_parm[tar].crossover;
            u16 tar_len = sizeof(rl_drc_parm[tar].crossover);
            eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
            CrossOverParam_TOOL_SET *parm = tar_buf;
            log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
        }
    }
#endif
#endif
#endif
}

void music_crossover_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        music_mode[tar].drc_parm.crossover.way_num = 2;
        music_mode[tar].drc_parm.crossover.N = 2;
        music_mode[tar].drc_parm.crossover.low_freq = 200;
    }
#endif
#endif
}

void music_crossover_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    u16 drc_name[] = {AEID_MUSIC_DRC, AEID_AUX_DRC};
    for (int tar = 0; tar < calc; tar++) {
        struct mode_list *list = get_group_list(drc_name[tar]);
        if (list) {
            u16 group_id = list->group_id[4];
            void *tar_buf = &music_mode[tar].drc_parm.crossover;
            u16 tar_len = sizeof(music_mode[tar].drc_parm.crossover);
            eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
            CrossOverParam_TOOL_SET *parm = tar_buf;
            log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
        }
    }
#endif
#endif
}

void music_low_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(EFF_MUSIC_LOW_DRC, &index, nor_list_label);
    u16 i = index;
    u8 tar = nor_label;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    /* for (int tar = 0; tar < calc; tar++){ */
    music_mode[tar].drc_parm.wdrc_parm[i].is_bypass = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.attacktime = 10;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.releasetime = 300;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.inputgain = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.outputgain = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
    memcpy(music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold, group, sizeof(group));
    music_mode[tar].drc_parm.wdrc_parm[i].parm.rms_time = 25;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.algorithm = 0;
    music_mode[tar].drc_parm.wdrc_parm[i].parm.mode = 1;
    /* } */
#endif
}

void music_low_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    u16 index = 0;
    u16 group_id = EFF_MUSIC_LOW_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 tar = nor_label;
    void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
    u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
    eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
    wdrc_printf(tar_buf);
#endif
}

void music_mid_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_MID_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        music_mode[tar].drc_parm.wdrc_parm[i].is_bypass = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.attacktime = 10;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.releasetime = 300;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.inputgain = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.outputgain = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold, group, sizeof(group));
        music_mode[tar].drc_parm.wdrc_parm[i].parm.rms_time = 25;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.algorithm = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.mode = 1;
    }
#endif
#endif
}

void music_mid_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_MID_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
        u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
#endif
}

void music_high_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_HIGH_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        music_mode[tar].drc_parm.wdrc_parm[i].is_bypass = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.attacktime = 10;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.releasetime = 300;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.inputgain = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.outputgain = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold, group, sizeof(group));
        music_mode[tar].drc_parm.wdrc_parm[i].parm.rms_time = 25;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.algorithm = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.mode = 1;
    }
#endif
#endif
}

void music_high_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_HIGH_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
        u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
#endif
}

void music_whole_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC && (TCFG_AUDIO_MDRC_ENABLE == 2)
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_WHOLE_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        music_mode[tar].drc_parm.wdrc_parm[i].is_bypass = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.attacktime = 10;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.releasetime = 300;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.inputgain = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.outputgain = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold, group, sizeof(group));
        music_mode[tar].drc_parm.wdrc_parm[i].parm.rms_time = 25;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.algorithm = 0;
        music_mode[tar].drc_parm.wdrc_parm[i].parm.mode = 1;
    }
#endif
#endif
}

void music_whole_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC && (TCFG_AUDIO_MDRC_ENABLE == 2)
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_WHOLE_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
        u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
#endif
}

void downlink_wideband_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    //通话下行drc
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    u8 tar = dl_wideband_label;
    /* for (u8 tar = 0; tar < 2; tar++) { */
    phone_mode[tar].drc_parm.is_bypass = 0;
    phone_mode[tar].drc_parm.parm.attacktime = 10;
    phone_mode[tar].drc_parm.parm.releasetime = 300;
    phone_mode[tar].drc_parm.parm.inputgain = 0;
    phone_mode[tar].drc_parm.parm.outputgain = 0;
    phone_mode[tar].drc_parm.parm.threshold_num = ARRAY_SIZE(group);
    memcpy(phone_mode[tar].drc_parm.parm.threshold, group, sizeof(group));
    phone_mode[tar].drc_parm.parm.rms_time = 25;
    phone_mode[tar].drc_parm.parm.algorithm = 0;
    phone_mode[tar].drc_parm.parm.mode = 1;
    /* } */
#endif
}

void downlink_wideband_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    u16 mode_seq = phone_mode_seq;
    u16 group_id = EFF_PHONE_WIDEBAND_DRC;
    u16 index = group_id - EFF_PHONE_WIDEBAND_DRC;
    void *tar_buf = &phone_mode[index].drc_parm;
    u16 tar_len = sizeof(phone_mode[index].drc_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
    wdrc_printf(tar_buf);
#endif
}

void downlink_narrowband_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
//通话下行drc
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    u8 index = dl_narrowband_label;
    phone_mode[index].drc_parm.is_bypass = 0;
    phone_mode[index].drc_parm.parm.attacktime = 10;
    phone_mode[index].drc_parm.parm.releasetime = 300;
    phone_mode[index].drc_parm.parm.inputgain = 0;
    phone_mode[index].drc_parm.parm.outputgain = 0;
    phone_mode[index].drc_parm.parm.threshold_num = ARRAY_SIZE(group);
    memcpy(phone_mode[index].drc_parm.parm.threshold, group, sizeof(group));
    phone_mode[index].drc_parm.parm.rms_time = 25;
    phone_mode[index].drc_parm.parm.algorithm = 0;
    phone_mode[index].drc_parm.parm.mode = 1;
#endif
}

void downlink_narrowband_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    u16 mode_seq = phone_mode_seq;
    u16 group_id = EFF_PHONE_NARROWBAND_DRC;
    u16 index = dl_narrowband_label;
    void *tar_buf = &phone_mode[index].drc_parm;
    u16 tar_len = sizeof(phone_mode[index].drc_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
    wdrc_printf(tar_buf);
#endif
}

void uplink_wideband_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    //通话上行行drc
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u8 index = ul_wideband_label;
    phone_mode[index].drc_parm.is_bypass = 0;
    phone_mode[index].drc_parm.parm.attacktime = 10;
    phone_mode[index].drc_parm.parm.releasetime = 300;
    phone_mode[index].drc_parm.parm.inputgain = 0;
    phone_mode[index].drc_parm.parm.outputgain = 0;
    phone_mode[index].drc_parm.parm.threshold_num = ARRAY_SIZE(group2);
    memcpy(phone_mode[index].drc_parm.parm.threshold, group2, sizeof(group2));
    phone_mode[index].drc_parm.parm.rms_time = 25;
    phone_mode[index].drc_parm.parm.algorithm = 0;
    phone_mode[index].drc_parm.parm.mode = 1;
#endif
}

void uplink_wideband_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    u16 mode_seq = aec_mode_seq;
    u16 group_id = EFF_AEC_WIDEBAND_DRC;
    u16 index = ul_wideband_label;
    void *tar_buf = &phone_mode[index].drc_parm;
    u16 tar_len = sizeof(phone_mode[index].drc_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
    wdrc_printf(tar_buf);
#endif
}

void uplink_narrowband_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    //通话上行行drc
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u8 index = ul_narrowband_label;
    phone_mode[index].drc_parm.is_bypass = 0;
    phone_mode[index].drc_parm.parm.attacktime = 10;
    phone_mode[index].drc_parm.parm.releasetime = 300;
    phone_mode[index].drc_parm.parm.inputgain = 0;
    phone_mode[index].drc_parm.parm.outputgain = 0;
    phone_mode[index].drc_parm.parm.threshold_num = ARRAY_SIZE(group2);
    memcpy(phone_mode[index].drc_parm.parm.threshold, group2, sizeof(group2));
    phone_mode[index].drc_parm.parm.rms_time = 25;
    phone_mode[index].drc_parm.parm.algorithm = 0;
    phone_mode[index].drc_parm.parm.mode = 1;
#endif
}

void uplink_narrowband_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_PHONE_DRC_ENABLE
    u16 mode_seq = aec_mode_seq;
    u16 group_id = EFF_AEC_NARROWBAND_DRC;
    u16 index = ul_narrowband_label;
    void *tar_buf = &phone_mode[index].drc_parm;
    u16 tar_len = sizeof(phone_mode[index].drc_parm);
    eff_file_analyze(mode_seq, group_id, tar_buf, tar_len);
    wdrc_printf(tar_buf);
#endif
}

void noisegate_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //noisegate
        mic_eff->noise_gate_parm.is_bypass = 0;
        mic_eff->noise_gate_parm.parm.attackTime = 300;
        mic_eff->noise_gate_parm.parm.releaseTime = 5;
        mic_eff->noise_gate_parm.parm.threshold = -90300;//mdb -90.3dB
        mic_eff->noise_gate_parm.parm.low_th_gain = 0;
    }
#endif
}

void noisegate_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //noisegate
        void *tar_buf = (void *)&mic_eff->noise_gate_parm;
        u16 tar_len = sizeof(mic_eff->noise_gate_parm);
        eff_file_analyze(eff_mode_seq[index], EFF_MIC_NOISEGATE, tar_buf, tar_len);
    }
#endif
}

void howling_ps_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
//howlingps_parm
        mic_eff->howlingps_parm.is_bypass = 0;
        mic_eff->howlingps_parm.parm.effect_v = EFFECT_HOWLING_FS;
        mic_eff->howlingps_parm.parm.ps_parm = -50;
        mic_eff->howlingps_parm.parm.fe_parm = 4;
    }
#endif
}

void howling_ps_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
//howlingps_parm
        void *tar_buf = (void *)&mic_eff->howlingps_parm;
        u16 tar_len = sizeof(mic_eff->howlingps_parm);
        eff_file_analyze(eff_mode_seq[index], EFF_MIC_HOWLINE_PS, tar_buf, tar_len);
    }
#endif
}

void notchhowling_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
//notchowling_parm
        mic_eff->notchhowling_parm.is_bypass = 0;
        mic_eff->notchhowling_parm.parm.Q = 2.0f;
        mic_eff->notchhowling_parm.parm.gain = -20;
        mic_eff->notchhowling_parm.parm.fade_n = 10;
        mic_eff->notchhowling_parm.parm.threshold = 25;
    }
#endif
}

void notchhowling_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
//notchowling_parm
        void *tar_buf = (void *)&mic_eff->notchhowling_parm;
        u16 tar_len = sizeof(mic_eff->notchhowling_parm);
        eff_file_analyze(eff_mode_seq[index], EFF_MIC_NOTCH_HOWLING, tar_buf, tar_len);
    }
#endif
}

void plate_reverb_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //palte_reverb
        mic_eff->plate_reverb_parm.is_bypass = 0;
        mic_eff->plate_reverb_parm.parm.pre_delay = 0;
        mic_eff->plate_reverb_parm.parm.highcutoff = 12200;
        mic_eff->plate_reverb_parm.parm.diffusion = 43;
        mic_eff->plate_reverb_parm.parm.decayfactor = 70;
        mic_eff->plate_reverb_parm.parm.highfrequencydamping = 26;
        mic_eff->plate_reverb_parm.parm.dry = 80;
        mic_eff->plate_reverb_parm.parm.wet = 40;
        mic_eff->plate_reverb_parm.parm.modulate = 1;
        mic_eff->plate_reverb_parm.parm.roomsize = 100;
    }
#endif
}

void plate_reverb_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
//palte_reverb
        void *tar_buf = (void *)&mic_eff->plate_reverb_parm;
        u16 tar_len = sizeof(mic_eff->plate_reverb_parm);
        eff_file_analyze(eff_mode_seq[index], EFF_MIC_PLATE_REVERB, tar_buf, tar_len);
        /* Plate_reverb_TOOL_SET *parm = tar_buf; */
    }
#endif
}

void echo_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //echo
        mic_eff->echo_parm.is_bypass = 0;
        mic_eff->echo_parm.parm.decayval = 60;
        mic_eff->echo_parm.parm.delay = 400;
        mic_eff->echo_parm.parm.filt_enable = 1;
        mic_eff->echo_parm.parm.lpf_cutoff = 5000;
        mic_eff->echo_parm.parm.drygain = 60;
        mic_eff->echo_parm.parm.wetgain = 50;
    }
#endif
}

void echo_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //echo
        void *tar_buf = (void *)&mic_eff->echo_parm;
        u16 tar_len = sizeof(mic_eff->echo_parm);
        eff_file_analyze(eff_mode_seq[index], EFF_MIC_ECHO, tar_buf, tar_len);
    }
#endif
}

void dynamic_eq_parm_default_init()
{
#if TCFG_DYNAMIC_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //dynamic eq
        dynamic_eq[tar].is_bypass = 0;
        dynamic_eq[tar].nSection = 1;
        dynamic_eq[tar].detect_mode = 1;
        for (u8 i = 0; i < dynamic_eq[tar].nSection; i++) {
            dynamic_eq[tar].effect_param[i].fc = 1000;
            dynamic_eq[tar].effect_param[i].Q = 0.7f;
            dynamic_eq[tar].effect_param[i].gain = 0.0f;
            dynamic_eq[tar].effect_param[i].type = 0x2;
            dynamic_eq[tar].effect_param[i].attackTime = 5;
            dynamic_eq[tar].effect_param[i].releaseTime = 300;
            dynamic_eq[tar].effect_param[i].rmsTime = 25;
            dynamic_eq[tar].effect_param[i].threshold = 0.0f;
            dynamic_eq[tar].effect_param[i].ratio = 1.0f;
            dynamic_eq[tar].effect_param[i].noisegate_threshold = -90.3f;
            dynamic_eq[tar].effect_param[i].fixGain = 0.0f;
            dynamic_eq[tar].effect_param[i].algorithm = 1;
        }
    }
#endif
}

void dynamic_eq_file_analyze_init()
{
#if TCFG_DYNAMIC_EQ_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //dynamic eq
        u16 group_id = EFF_MUSIC_DYNAMIC_EQ;
        void *tar_buf = &dynamic_eq[tar];
        u16 tar_len = sizeof(DynamicEQParam_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        dynamic_eq_printf(tar_buf);
    }
#endif
}

void rl_music_gain_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE) && TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE
    float gain = 0;
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //rl_gain
        rl_gain_parm[tar].is_bypass = 0;
        rl_gain_parm[tar].parm.gain[0] = 1.0f;
        rl_gain_parm[tar].parm.gain[1] = 0.0f;
        rl_gain_parm[tar].parm.gain[2] = 0.0f;
        rl_gain_parm[tar].parm.gain[3] = 1.0f;
    }
#endif
#endif
}

void rl_music_gain_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE) && TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //rl_rr_gain
        u16 group_id = EFF_MUSIC_RL_GAIN;
        void *tar_buf = &rl_gain_parm[tar];
        u16 tar_len = sizeof(Gain_Process_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
        log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[0]);
        log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[1]);
        log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[2]);
        log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[3]);
#endif
    }
#endif
#endif
}

void music_gain_parm_default_init()
{
#if defined(TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE) && TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE
    u8 calc = mode_add;
    float gain = 0;
    for (u8 tar = 0; tar < calc; tar++) {
        //gain
        gain_parm[tar].is_bypass = 0;
        gain_parm[tar].parm.gain[0] = 1.0f;
        gain_parm[tar].parm.gain[1] = 0.0f;
        gain_parm[tar].parm.gain[2] = 0.0f;
        gain_parm[tar].parm.gain[3] = 1.0f;
    }
#endif
}

void music_gain_file_analyze_init()
{
#if defined(TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE) && TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        //gain
        u16 gain_id[] = {EFF_MUSIC_GAIN, EFF_AUX_GAIN};
        u16 group_id = gain_id[tar];
        void *tar_buf = &gain_parm[tar];
        u16 tar_len = sizeof(Gain_Process_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
        log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[0]);
        log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[1]);
        log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[2]);
        log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[3]);
#endif
    }
#endif
}

void mic_gain_parm_default_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
#if 0//GAIN_PROCESS_EN
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        float gain = 0;
        mic_eff->gain_parm.parm.gain[0] = powf(10, gain / 20.0f); //db转mag,工具传下来的值
        mic_eff->gain_parm.parm.gain[1] = powf(10, gain / 20.0f); //db转mag,工具传下来的值
        mic_eff->gain_parm.is_bypass = 0;
    }
#endif
#endif
}

void mic_gain_file_analyze_init()
{
#if defined(TCFG_MIC_EFFECT_ENABLE) && TCFG_MIC_EFFECT_ENABLE
#if 0//GAIN_PROCESS_EN
    for (int index = 0; index < EFFECT_REVERB_PARM_MAX; index++) {
        struct eff_parm *mic_eff = &eff_mode[index];
        //gain
        u16 group_id = EFF_MIC_GAIN;
        void *tar_buf = &mic_eff->gain_parm;
        u16 tar_len = sizeof(mic_eff->gain_parm);
        eff_file_analyze(eff_mode_seq[index], group_id, tar_buf, tar_len);
#if PARM_DEBUG
        log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm.is_bypass, *(int *)&gain_parm.parm.gain[0]);
        log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm.is_bypass, *(int *)&gain_parm.parm.gain[1]);
#endif
    }
#endif
#endif
}

void music_noise_gate_parm_default_init()
{
#if AUDIO_VBASS_CONFIG && MUSIC_NOISE_GATE_EN
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        music_noisegate_parm[tar].is_bypass = 0;
        music_noisegate_parm[tar].parm.attackTime = 300;
        music_noisegate_parm[tar].parm.releaseTime = 5;
        music_noisegate_parm[tar].parm.threshold = -90300;//mdb -90.3dB
        music_noisegate_parm[tar].parm.low_th_gain = 0;
    }
#endif
}

void music_noise_gate_file_analyze_init()
{
#if AUDIO_VBASS_CONFIG && MUSIC_NOISE_GATE_EN
    u8 calc = mode_add;
    for (int index = 0 ; index < calc; index++) {
        //noisegate
        u16 group_id = EFF_MUSIC_NOISEGATE;
        void *tar_buf = (void *)&music_noisegate_parm[index];
        u16 tar_len = sizeof(NoiseGateParam_TOOL_SET);
        eff_file_analyze(music_mode_seq[index], group_id, tar_buf, tar_len);
    }
#endif
}

/*
 *环绕声默认初始参数
 * */
void music_surround_effect_parm_default_init()
{
#if AUDIO_SURROUND_CONFIG
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        //3d全景
        sur_parm[tar].is_bypass = 0;
        sur_parm[tar].parm.surround_type = EFFECT_3D_TYPE1;
        sur_parm[tar].parm.rotatestep    = 2;
        sur_parm[tar].parm.damping       = 120;
        sur_parm[tar].parm.feedback      = 110;
        sur_parm[tar].parm.roomsize      = 128;
    }
#endif
}

/*
 *环绕声效果文件解析
 * */
void music_surround_effect_file_analyze_init()
{
#if AUDIO_SURROUND_CONFIG
    u8 calc = mode_add;
    for (int index = 0; index < calc; index++) {
        //noisegate
        u16 group_id = EFF_MUSIC_SURRROUND_EFF;
        void *tar_buf = (void *)&sur_parm[index];
        u16 tar_len = sizeof(SurroundEffect_TOOL_SET);
        eff_file_analyze(music_mode_seq[index], group_id, tar_buf, tar_len);

#if PARM_DEBUG
        SurroundEffect_TOOL_SET *parmt = tar_buf;
        surround_update_parm *parm = &parmt->parm;
        log_debug("music surround effect  surround_type %d,rotatestep %d, damping %d, feedback %d, roomsize %d\n",
                  parm->surround_type, parm->rotatestep, parm->damping, parm->feedback, parm->roomsize);
#endif
    }
#endif
}

/*
 *drc之后额外的eq默认值初始化
 * */
void music_ext_eq_parm_default_init()
{
#if defined(MUSIC_EXT_EQ_AFTER_DRC) && MUSIC_EXT_EQ_AFTER_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        muisc_ext_eq[tar].global_gain = 0;
        muisc_ext_eq[tar].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(muisc_ext_eq[tar].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

/*
 *drc之后额外的eq效果文件解析
 * */
void music_ext_eq_file_analyze_init()
{
#if defined(MUSIC_EXT_EQ_AFTER_DRC) && MUSIC_EXT_EQ_AFTER_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        u16 group_id = EFF_MUSIC_EXT_EQ;
        void *tar_buf = &muisc_ext_eq[tar];
        u16 tar_len = sizeof(struct eq_tool);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        music_eq2_printf(tar_buf);
    }
#endif
}

/*
 *drc之后额外的eq默认值初始化
 * */
void music_fr_ext_eq_parm_default_init()
{
#if defined(MUSIC_EXT_EQ_AFTER_DRC) && MUSIC_EXT_EQ_AFTER_DRC
#if defined(MUSIC_EXT_EQ_SPILT) && MUSIC_EXT_EQ_SPILT
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        muisc_fr_ext_eq[tar].global_gain = 0;
        muisc_fr_ext_eq[tar].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(muisc_fr_ext_eq[tar].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
#endif
}
/*
 *drc之后额外的eq效果文件解析
 * */
void music_fr_ext_eq_file_analyze_init()
{
#if defined(MUSIC_EXT_EQ_AFTER_DRC) && MUSIC_EXT_EQ_AFTER_DRC
#if defined(MUSIC_EXT_EQ_SPILT) && MUSIC_EXT_EQ_SPILT
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        u16 group_id = EFF_MUSIC_FR_EXT_EQ;
        void *tar_buf = &muisc_fr_ext_eq[tar];
        u16 tar_len = sizeof(struct eq_tool);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        music_eq2_printf(tar_buf);
    }
#endif
#endif
}

/*
 *低音通路drc之后额外的eq默认值初始化
 * */
void music_ext_eq2_parm_default_init()
{
#if defined(MUSIC_EXT_EQ2_AFTER_DRC) && MUSIC_EXT_EQ2_AFTER_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        muisc_ext_eq2[tar].global_gain = 0;
        muisc_ext_eq2[tar].seg_num = ARRAY_SIZE(mic_eff_eq_tab);
        memcpy(muisc_ext_eq2[tar].seg, mic_eff_eq_tab, sizeof(mic_eff_eq_tab));
    }
#endif
}

/*
 *低音通路drc之后额外的eq效果文件解析
 * */
void music_ext_eq2_file_analyze_init()
{
#if defined(MUSIC_EXT_EQ2_AFTER_DRC) && MUSIC_EXT_EQ2_AFTER_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        u16 group_id = EFF_MUSIC_EXT_EQ2;
        void *tar_buf = &muisc_ext_eq2[tar];
        u16 tar_len = sizeof(struct eq_tool);
        if (!eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len)) {
            int index = get_index_by_group_id(group_id, tar);
            if (index >= 0) {
                struct eq_tool *parm = (struct eq_tool *)tar_buf;
                set_list_nsection(tar, index, parm->seg_num);//根据文件段数，设定在线调试段数
            }
        }
        music_eq2_printf(tar_buf);
    }
#endif
}

/*
 *声道互换接口参数初始化
 * */
void music_ch_swap_parm_default_init()
{
#if defined(CH_SWAP_OLD) && CH_SWAP_OLD
    u8 calc = mode_add;
    float gain = 0;
    for (u8 tar = 0; tar < calc; tar++) {
        //gain
        music_ch_swap[tar].is_bypass = 1;//bypass
    }
#endif
}

/*
 *声道互换参数效果文件解析
 * */
void music_ch_swap_file_analyze_init()
{
#if defined(CH_SWAP_OLD) && CH_SWAP_OLD
    u8 calc = mode_add;
    for (u8 tar = 0; tar < calc; tar++) {
        void *tar_buf = &music_ch_swap[tar];
        u16 tar_len = sizeof(ChannelSwap_TOOL_SET);
        eff_file_analyze(music_mode_seq[tar], EFF_MUSIC_CH_SWAP, tar_buf, tar_len);
    }
#endif
}

/*
 *dac pga默认值
 * */
void dac_pga_parm_default_init()
{
    dac_pga.again_fl = 30;
    dac_pga.again_fr = 30;
    dac_pga.again_rl = 30;
    dac_pga.again_rr = 30;
}

/*
 *dac pga效果文件解析
 * */
void dac_pga_file_analyze_init()
{
    u8 tar = 0;
    void *tar_buf = &dac_pga;
    u16 tar_len = sizeof(dac_pga_TOOL_SET);
    eff_file_analyze(music_mode_seq[tar], EFF_DAC_PGA, tar_buf, tar_len);
    log_debug("max : gain_fl %d, gain_fr %d, gain_rl %d, gain_rr %d\n",
              dac_pga.again_fl, dac_pga.again_fr, dac_pga.again_rl, dac_pga.again_rr);
}

void adc_pga_parm_default_init()
{
    adc_pga.gain_line = 3;
    adc_pga.gain_mic = 3;
    adc_pga.MicBoostFlag = 1;
}

/*
 *adc pga效果文件解析
 * */
void adc_pga_file_analyze_init()
{
    u8 tar = 0;
    void *tar_buf = &adc_pga;
    u16 tar_len = sizeof(dac_pga_TOOL_SET);
    eff_file_analyze(music_mode_seq[tar], EFF_DAC_PGA, tar_buf, tar_len);
    log_debug("gain_mic %d, gain_linein %d, MicBoostFlag %d\n",
              adc_pga.gain_mic, adc_pga.gain_line, adc_pga.MicBoostFlag);
}

#if (defined(TCFG_DRC_SPILT_ENABLE) && TCFG_DRC_SPILT_ENABLE)
void music_fr_low_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    u16 index = 0;
    u16 group_id = EFF_MUSIC_FR_LOW_DRC;
    u32 drc_name = get_module_name_and_index(EFF_MUSIC_FR_LOW_DRC, &index, nor_list_label);
    u16 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        fr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        fr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        fr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(fr_drc_parm[tar].wdrc_parm[i].parm.threshold, group, sizeof(group));
        fr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        fr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_fr_low_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    u16 index = 0;
    u16 group_id = EFF_MUSIC_FR_LOW_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_fr_mid_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_FR_MID_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        fr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        fr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        fr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(fr_drc_parm[tar].wdrc_parm[i].parm.threshold, group, sizeof(group));
        fr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        fr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
#endif
}

void music_fr_mid_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_FR_MID_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
#endif
}

void music_fr_high_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_FR_HIGH_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        fr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        fr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        fr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(fr_drc_parm[tar].wdrc_parm[i].parm.threshold, group, sizeof(group));
        fr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        fr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
#endif
}

void music_fr_high_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_FR_HIGH_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
#endif
}

void music_fr_whole_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC && (TCFG_AUDIO_MDRC_ENABLE == 2)
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_FR_WHOLE_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    for (int tar = 0; tar < calc; tar++) {
        fr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        fr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        fr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group);
        memcpy(music_mode[tar].drc_parm.wdrc_parm[i].parm.threshold, group, sizeof(group));
        fr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        fr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        fr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
#endif
}

void music_fr_whole_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC && (TCFG_AUDIO_MDRC_ENABLE == 2)
    u16 index = 0;
    u8 calc = mode_add;
    u16 group_id = EFF_MUSIC_FR_WHOLE_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
#endif
}

void music_fr_crossover_wdrc_parm_default_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        fr_drc_parm[tar].crossover.way_num = 2;
        fr_drc_parm[tar].crossover.N = 2;
        fr_drc_parm[tar].crossover.low_freq = 200;
    }
#endif
#endif
}

void music_fr_crossover_wdrc_file_analyze_init()
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    u16 drc_name[] = {AEID_MUSIC_FR_DRC, AEID_AUX_FR_DRC};
    for (int tar = 0; tar < calc; tar++) {
        struct mode_list *list = get_group_list(drc_name[tar]);
        if (list) {
            u16 group_id = list->group_id[4];
            void *tar_buf = &fr_drc_parm[tar].crossover;
            u16 tar_len = sizeof(fr_drc_parm[tar].crossover);
            eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
            CrossOverParam_TOOL_SET *parm = tar_buf;
            log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
        }
    }
#endif
#endif
}

void music_rr_low_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RR_LOW_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rr_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rr_low_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    //rl_wdrc
    u16 group_id = EFF_MUSIC_RR_LOW_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rr_mid_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RR_MID_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rr_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rr_mid_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    //rl_wdrc
    u16 group_id = EFF_MUSIC_RR_MID_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rr_high_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RR_HIGH_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rr_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rr_high_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    //rl_wdrc
    u16 group_id = EFF_MUSIC_RR_HIGH_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rr_whole_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    struct threshold_group group2[] = {{0, 0}, {50, 50}, {90, 90}};
    u16 index = 0;
    u16 group_id = EFF_MUSIC_RR_WHOLE_DRC;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u8 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rr_drc_parm[tar].wdrc_parm[i].is_bypass = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.attacktime = 10;
        rr_drc_parm[tar].wdrc_parm[i].parm.releasetime = 300;
        rr_drc_parm[tar].wdrc_parm[i].parm.inputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.outputgain = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.threshold_num = ARRAY_SIZE(group2);
        memcpy(rr_drc_parm[tar].wdrc_parm[i].parm.threshold, group2, sizeof(group2));
        rr_drc_parm[tar].wdrc_parm[i].parm.rms_time = 25;
        rr_drc_parm[tar].wdrc_parm[i].parm.algorithm = 0;
        rr_drc_parm[tar].wdrc_parm[i].parm.mode = 1;
    }
#endif
}

void music_rr_whole_wdrc_file_analyze_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    //rl_wdrc
    u16 group_id = EFF_MUSIC_RR_WHOLE_DRC;
    u16 index = 0;
    u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
    u16 i = index;
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
        u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
        eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

void music_rr_crossover_wdrc_parm_default_init()
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    for (int tar = 0; tar < calc; tar++) {
        rr_drc_parm[tar].crossover.way_num = 2;
        rr_drc_parm[tar].crossover.N = 2;
        rr_drc_parm[tar].crossover.low_freq = 200;
    }
#endif
#endif
#endif
}

void music_rr_crossover_wdrc_file_analyze_init()
{
    /* #if (defined(TCFG_DRC_SPILT_ENABLE) && TCFG_DRC_SPILT_ENABLE) */
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    u8 calc = mode_add;
    u16 drc_name[] = {AEID_MUSIC_RR_DRC, AEID_AUX_RR_DRC};
    for (int tar = 0; tar < calc; tar++) {
        struct mode_list *list = get_group_list(drc_name[tar]);
        if (list) {
            u16 group_id = list->group_id[4];
            void *tar_buf = &rr_drc_parm[tar].crossover;
            u16 tar_len = sizeof(rr_drc_parm[tar].crossover);
            eff_file_analyze(music_mode_seq[tar], group_id, tar_buf, tar_len);
#if PARM_DEBUG
            CrossOverParam_TOOL_SET *parm = tar_buf;
            log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
        }
    }
#endif
#endif
#endif
}

#endif/*TCFG_DRC_SPILT_ENABLE*/


#if defined(AUDIO_EFF_SNAP_PARM_SWITCH_ENABLE) && AUDIO_EFF_SNAP_PARM_SWITCH_ENABLE
/********************************************************************************************/
/*音效模块的快照信息解析 */
/********************************************************************************************/

/*
 *aux eq快照信息解析
 * */
void aux_eq_snap_parm_switch(u8 tar_snap)
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (!tar_snap) {
        aux_music_eq_file_analyze_init();
    } else {
        u8 tar = aux_label;
        u16 group_id = EFF_AUX_EQ;
        void *tar_buf = &music_mode[tar].eq_parm;
        u16 tar_len = sizeof(struct music_eq_tool);
        ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
        music_eq_printf(tar_buf);
    }
#endif
#endif
}

/*
 *音乐eq快照信息解析
 * */
void music_eq_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (!tar_snap) {
        music_eq_file_analyze_init();
    } else {
        u8 tar = nor_label;
        u16 group_id = EFF_MUSIC_EQ;
        void *tar_buf = &music_mode[tar].eq_parm;
        u16 tar_len = sizeof(struct music_eq_tool);
        ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
        music_eq_printf(tar_buf);
    }
#endif
}

/*
 *低频带drc快照信息切换
 * */
void music_low_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    if (!tar_snap) {
        music_low_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u16 group_id = EFF_MUSIC_LOW_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        u8 tar = nor_label;
        void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
        u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
        ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
        wdrc_printf(tar_buf);
    }
#endif
}

/*
 *中频带drc快照信息切换
 * */
void music_mid_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_mid_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u8 calc = mode_add;
        u16 group_id = EFF_MUSIC_MID_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
            u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *rl_eq快照信息解析
 * */
void rl_eq_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (!tar_snap) {
        rl_eq_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            //rl_eq
            u16 group_id = EFF_MUSIC_RL_EQ;
            void *tar_buf = &rl_eq_parm[tar];
            u16 tar_len = sizeof(struct music_eq_tool);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *高频带drc快照信息解析
 * */
void music_high_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_high_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u8 calc = mode_add;
        u16 group_id = EFF_MUSIC_HIGH_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
            u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *多带drc后的全带drc快照信息解析
 * */
void music_whole_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC && (TCFG_AUDIO_MDRC_ENABLE == 2)
    if (!tar_snap) {
        music_whole_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u8 calc = mode_add;
        u16 group_id = EFF_MUSIC_WHOLE_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
            u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *fl通道分频器快照信息解析
 * */
void music_crossover_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_crossover_wdrc_file_analyze_init();
    } else {
        u8 calc = mode_add;
        u16 drc_name[] = {AEID_MUSIC_DRC, AEID_AUX_DRC};
        for (int tar = 0; tar < calc; tar++) {
            struct mode_list *list = get_group_list(drc_name[tar]);
            if (list) {
                u16 group_id = list->group_id[4];
                void *tar_buf = &music_mode[tar].drc_parm.crossover;
                u16 tar_len = sizeof(music_mode[tar].drc_parm.crossover);
                ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
                CrossOverParam_TOOL_SET *parm = tar_buf;
                log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
            }
        }
    }
#endif
#endif
}

/*
 *rr_eq快照信息解析
 * */
void rr_eq_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (!tar_snap) {
        rr_eq_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            //rl_eq
            u16 group_id = EFF_MUSIC_RR_EQ;
            void *tar_buf = &rr_eq_parm[tar];
            u16 tar_len = sizeof(struct music_eq_tool);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *fr快照信息解析
 * */
void fr_eq_snap_parm_switch(u8 tar_snap)
{
    /* #if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE */
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (!tar_snap) {
        fr_eq_file_analyze_init();
    } else {

        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            //fr_eq
            u16 group_id = EFF_MUSIC_FR_EQ;
            void *tar_buf = &fr_eq_parm[tar];
            u16 tar_len = sizeof(struct music_eq_tool);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq_printf(tar_buf);
        }
    }
#endif
    /* #endif */
}

/*
 *aux 低频带drc快照信息解析
 * */
void aux_music_low_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    if (!tar_snap) {
        aux_music_low_wdrc_file_analyze_init();
    } else {

        u16 index = 0;
        u32 drc_name = get_module_name_and_index(EFF_AUX_DRC, &index, aux_list_label);
        u16 i = index;
        u8 tar = aux_label;
        /* for (int tar = 0; tar < calc; tar++){ */
        struct mode_list *list = get_group_list(drc_name);
        if (list) {
            u16 group_id = list->group_id[i];
            void *tar_buf = &music_mode[tar].drc_parm.wdrc_parm[i];
            u16 tar_len = sizeof(music_mode[tar].drc_parm.wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
        /* } */
    }
#endif
#endif
}

/*
 *环绕声快照信息解析
 * */
void music_surround_snap_parm_switch(u8 tar_snap)
{
#if AUDIO_SURROUND_CONFIG
    if (!tar_snap) {
        music_surround_effect_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (int index = 0 ; index < calc; index++) {
            //noisegate
            u16 group_id = EFF_MUSIC_SURRROUND_EFF;
            void *tar_buf = (void *)&sur_parm[index];
            u16 tar_len = sizeof(SurroundEffect_TOOL_SET);
            ext_eff_file_analyze(music_mode_seq[index], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
            SurroundEffect_TOOL_SET *parmt = tar_buf;
            surround_update_parm *parm = &parmt->parm;
            log_debug("music surround effect  surround_type %d,rotatestep %d, damping %d, feedback %d, roomsize %d\n",
                      parm->surround_type, parm->rotatestep, parm->damping, parm->feedback, parm->roomsize);
#endif
        }
    }
#endif
}

/*
 *虚拟低音前的gain快照信息解析
 * */
void vbass_prev_gain_snap_parm_switch(u8 tar_snap)
{
#if AUDIO_VBASS_CONFIG
    if (!tar_snap) {
        vbass_prev_gain_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            void *tar_buf = &vbass_prev_gain_parm[tar];
            u16 tar_len = sizeof(Gain_Process_TOOL_SET);
            ext_eff_file_analyze(music_mode_seq[tar], EFF_MUSIC_VBASS_PREV_GAIN, tar_snap - 1, tar_buf, tar_len);

#if PARM_DEBUG
            log_debug("vbass_prev_gain_parm[%d].is_bypass %d, gain 0x%x", tar, vbass_prev_gain_parm[tar].is_bypass, *(int *)&vbass_prev_gain_parm[tar].parm.gain[0]);
            log_debug("vbass_prev_gain_parm[%d].is_bypass %d, gain 0x%x", tar, vbass_prev_gain_parm[tar].is_bypass, *(int *)&vbass_prev_gain_parm[tar].parm.gain[1]);
#endif
        }
    }
#endif
}

/*
 *虚拟低音快照信息解析
 * */
void music_vbass_snap_parm_switch(u8 tar_snap)
{
#if AUDIO_VBASS_CONFIG
    if (!tar_snap) {
        music_vbass_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &vbass_parm[tar];
            u16 tar_len = sizeof(VirtualBass_TOOL_SET);
            ext_eff_file_analyze(music_mode_seq[tar], EFF_MUSIC_VBASS, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
            log_debug("is_bypass %d, ratio %d, boost %d, fc %d\n", vbass_parm[tar].is_bypass, vbass_parm[tar].parm.ratio, vbass_parm[tar].parm.boost, vbass_parm[tar].parm.fc);
#endif
        }
    }
#endif
}

/*
 *动态eq前的eq快照信息解析
 * */
void music_eq2_snap_parm_switch(u8 tar_snap)
{
#if TCFG_DYNAMIC_EQ_ENABLE
    if (!tar_snap) {
        music_eq2_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            u16 group_id = EFF_MUSIC_EQ2;
            void *tar_buf = &music_eq2_parm[tar];
            u16 tar_len = sizeof(struct music_eq2_tool);
            music_eq2_printf(tar_buf);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq2_printf(tar_buf);
        }
    }
#endif
}

/*
 *动态eq快照信息解析
 * */
void dynamic_eq_snap_parm_switch(u8 tar_snap)
{
#if TCFG_DYNAMIC_EQ_ENABLE
    if (!tar_snap) {
        dynamic_eq_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            //dynamic eq
            u16 group_id = EFF_MUSIC_DYNAMIC_EQ;
            void *tar_buf = &dynamic_eq[tar];
            u16 tar_len = sizeof(DynamicEQParam_TOOL_SET);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            dynamic_eq_printf(tar_buf);
        }
    }
#endif
}

/*
 *drc之后的扩展eq快照信息解析
 * */
void music_ext_eq_snap_parm_switch(u8 tar_snap)
{
#if defined(MUSIC_EXT_EQ_AFTER_DRC) && MUSIC_EXT_EQ_AFTER_DRC
    if (!tar_snap) {
        music_ext_eq_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            u16 group_id = EFF_MUSIC_EXT_EQ;
            void *tar_buf = &muisc_ext_eq[tar];
            u16 tar_len = sizeof(struct eq_tool);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq2_printf(tar_buf);
        }
    }
#endif
}

/*
 *drc之后的扩展eq快照信息解析
 * */
void music_fr_ext_eq_snap_parm_switch(u8 tar_snap)
{
#if defined(MUSIC_EXT_EQ_AFTER_DRC) && MUSIC_EXT_EQ_AFTER_DRC
    if (!tar_snap) {
        music_fr_ext_eq_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            u16 group_id = EFF_MUSIC_FR_EXT_EQ;
            void *tar_buf = &muisc_fr_ext_eq[tar];
            u16 tar_len = sizeof(struct eq_tool);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq2_printf(tar_buf);
        }
    }
#endif
}

/*
 *低音通道drc之后的eq快照信息处理
 * */
void music_ext_eq2_snap_parm_switch(u8 tar_snap)
{
#if defined(MUSIC_EXT_EQ2_AFTER_DRC) && MUSIC_EXT_EQ2_AFTER_DRC
    if (!tar_snap) {
        music_ext_eq2_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            u16 group_id = EFF_MUSIC_EXT_EQ2;
            void *tar_buf = &muisc_ext_eq2[tar];
            u16 tar_len = sizeof(struct eq_tool);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            music_eq2_printf(tar_buf);
        }
    }
#endif
}

/*
 *music eq前相位、声道互换模块快照信息解析
 * */
void music_gain_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE) && TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE
    if (!tar_snap) {
        music_gain_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            //gain
            u16 gain_id[] = {EFF_MUSIC_GAIN, EFF_AUX_GAIN};
            u16 group_id = gain_id[tar];
            void *tar_buf = &gain_parm[tar];
            u16 tar_len = sizeof(Gain_Process_TOOL_SET);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
            log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[0]);
            log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[1]);
            log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[2]);
            log_debug("gain_parm.is_bypass %d, gain 0x%x", gain_parm[tar].is_bypass, *(int *)&gain_parm[tar].parm.gain[3]);
#endif
        }
    }
#endif
}

/*
 *bass eq前相位、声道互换模块快照信息解析
 * */
void rl_music_gain_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE) && TCFG_PHASER_GAIN_AND_CH_SWAP_ENABLE
    if (!tar_snap) {
        rl_music_gain_file_analyze_init();
    } else {
        u8 calc = mode_add;
        for (u8 tar = 0; tar < calc; tar++) {
            //rl_rr_gain
            u16 group_id = EFF_MUSIC_RL_GAIN;
            void *tar_buf = &rl_gain_parm[tar];
            u16 tar_len = sizeof(Gain_Process_TOOL_SET);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
            log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[0]);
            log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[1]);
            log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[2]);
            log_debug("rl_gain_parm.is_bypass %d, gain 0x%x", rl_gain_parm[tar].is_bypass, *(int *)&rl_gain_parm[tar].parm.gain[3]);
#endif
        }
    }
#endif
#endif
}


#if (defined(TCFG_DRC_SPILT_ENABLE) && TCFG_DRC_SPILT_ENABLE)
/*
 *fr通道低频drc快照信息解析
 * */
void music_fr_low_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    if (!tar_snap) {
        music_fr_low_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u16 group_id = EFF_MUSIC_FR_LOW_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *fr通道中频drc快照信息解析
 * */
void music_fr_mid_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_fr_mid_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u8 calc = mode_add;
        u16 group_id = EFF_MUSIC_FR_MID_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *fr通道高频drc快照信息解析
 * */
void music_fr_high_wdrc_snap_parm_switch(u8 tar_snap)
{
    if (!tar_snap) {
        music_fr_high_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u8 calc = mode_add;
        u16 group_id = EFF_MUSIC_FR_HIGH_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
}

/*
 *fr通道多带drc之后做的全带drc快照信息解析
 * */
void music_fr_whole_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC && (TCFG_AUDIO_MDRC_ENABLE == 2)
    if (!tar_snap) {
        music_fr_whole_wdrc_file_analyze_init();
    } else {
        u16 index = 0;
        u8 calc = mode_add;
        u16 group_id = EFF_MUSIC_FR_WHOLE_DRC;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &fr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(fr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
#endif
}

/*
 *fr通道多带分频器快照信息解析
 * */
void music_fr_crossover_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_fr_crossover_wdrc_file_analyze_init();
    } else {
        u8 calc = mode_add;
        u16 drc_name[] = {AEID_MUSIC_FR_DRC, AEID_AUX_FR_DRC};
        for (int tar = 0; tar < calc; tar++) {
            struct mode_list *list = get_group_list(drc_name[tar]);
            if (list) {
                u16 group_id = list->group_id[4];
                void *tar_buf = &fr_drc_parm[tar].crossover;
                u16 tar_len = sizeof(fr_drc_parm[tar].crossover);
                ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
                CrossOverParam_TOOL_SET *parm = tar_buf;
                log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
            }
        }
    }
#endif
#endif
}

/*
 *rr通道低频drc快照信息解析
 * */
void music_rr_low_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rr_low_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RR_LOW_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rr通道中频drc快照信息解析
 * */
void music_rr_mid_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rr_mid_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RR_MID_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rr通道高频drc快照信息解析
 * */
void music_rr_high_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rr_high_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RR_HIGH_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rr通道多带drc之后做的全带drc快照信息解析
 * */
void music_rr_whole_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rr_whole_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RR_WHOLE_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rr_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rr_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rr通道多带分频器快照信息解析
 * */
void music_rr_crossover_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_rr_crossover_wdrc_file_analyze_init();
    } else {
        u8 calc = mode_add;
        u16 drc_name[] = {AEID_MUSIC_RR_DRC, AEID_AUX_RR_DRC};
        for (int tar = 0; tar < calc; tar++) {
            struct mode_list *list = get_group_list(drc_name[tar]);
            if (list) {
                u16 group_id = list->group_id[4];
                void *tar_buf = &rr_drc_parm[tar].crossover;
                u16 tar_len = sizeof(rr_drc_parm[tar].crossover);
                ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
                CrossOverParam_TOOL_SET *parm = tar_buf;
                log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
            }
        }

    }
#endif
#endif
#endif
}

#endif/*TCFG_DRC_SPILT_ENABLE*/


/*
 *rl通道低频/全带drc快照信息解析
 * */
void music_rl_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rl_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RL_LOW_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rl通道中频drc快照信息解析
 * */
void music_rl_mid_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rl_mid_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RL_MID_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rl通道高频drc快照信息解析
 * */
void music_rl_high_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rl_high_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RL_HIGH_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u8 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rl通道多带drc之后的全带drc快照信息解析
 * */
void music_rl_whole_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    if (!tar_snap) {
        music_rl_whole_wdrc_file_analyze_init();
    } else {
        //rl_wdrc
        u16 group_id = EFF_MUSIC_RL_WHOLE_DRC;
        u16 index = 0;
        u32 drc_name = get_module_name_and_index(group_id, &index, nor_list_label);
        u16 i = index;
        u8 calc = mode_add;
        for (int tar = 0; tar < calc; tar++) {
            void *tar_buf = &rl_drc_parm[tar].wdrc_parm[i];
            u16 tar_len = sizeof(rl_drc_parm[tar].wdrc_parm[i]);
            ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
            wdrc_printf(tar_buf);
        }
    }
#endif
}

/*
 *rl通道分频器快照信息解析
 * */
void music_rl_crossover_wdrc_snap_parm_switch(u8 tar_snap)
{
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
#if defined(MULTI_BAND_DRC) && MULTI_BAND_DRC
    if (!tar_snap) {
        music_rl_crossover_wdrc_file_analyze_init();
    } else {
        u8 calc = mode_add;
        u16 drc_name[] = {AEID_MUSIC_RL_DRC, AEID_AUX_RL_DRC};
        for (int tar = 0; tar < calc; tar++) {
            struct mode_list *list = get_group_list(drc_name[tar]);
            if (list) {
                u16 group_id = list->group_id[4];
                void *tar_buf = &rl_drc_parm[tar].crossover;
                u16 tar_len = sizeof(rl_drc_parm[tar].crossover);
                ext_eff_file_analyze(music_mode_seq[tar], group_id, tar_snap - 1, tar_buf, tar_len);
#if PARM_DEBUG
                CrossOverParam_TOOL_SET *parm = tar_buf;
                log_debug("way_num %d, N %d, low_freq %d, high_freq %d\n", parm->way_num, parm->N, parm->low_freq, parm->high_freq);
#endif
            }
        }
    }
#endif
#endif
#endif

}

#endif/*AUDIO_EFF_SNAP_PARM_SWITCH_ENABLE*/

#endif/*TCFG_EQ_ENABLE*/
