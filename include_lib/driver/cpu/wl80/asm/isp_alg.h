

#ifndef __ISP_ALG_H__
#define __ISP_ALG_H__

#include "typedef.h"


enum {
    ISP_ISO_100 = 0,
    ISP_ISO_200,
    ISP_ISO_400,
    ISP_ISO_800,
    ISP_ISO_1600,
    ISP_ISO_3200,
    ISP_ISO_6400,
    ISP_ISO_AUTO = 0xff,
};
#define ISP_ISO_MAX   (ISP_ISO_6400+1)


//AE
#define AE_CURVE_END                ((unsigned int)(-1))
#define AE_CURVE_INFO_MAX                 4


enum {
    AE_INTERP_EXP = 1,
    AE_INTERP_GAIN = 2,
};

enum {
    AE_CURVE_50HZ = 0,
    AE_CURVE_60HZ,
};

typedef enum {
    AE_WIN_WEIGHT_AVG = 0,  //  平均测光
    AE_WIN_WEIGHT_CENTRE,   //  中央区域测光
    AE_WIN_WEIGHT_SPOT,     //  点测光
    AE_WIN_WEIGHT_MULTI,    //  多区域测光
    AE_WIN_WEIGHT_CARDV,    //  行车记录仪专用
    AE_WIN_WEIGHT_CUST,
} isp_ae_ww_type_t;

#define AE_WIN_WEIGHT_MAX  AE_WIN_WEIGHT_CUST


typedef struct {
    u32 ae_exp_line;
    u32 ae_exp_clk;
    u32 ae_gain;
} isp_ae_shutter_t;

typedef struct {
    u32(*calc_shutter)(isp_ae_shutter_t *shutter, u32 exp_time, u32 gain);
    u32(*set_shutter)(isp_ae_shutter_t *shutter);
    void   *(*get_ae_curve)(u32 type,  u32 fps, int *min_ev, int *max_ev);
} isp_ae_ops_t;

typedef struct ae_params {

    u32 ae_max_ev;
    u32 ae_min_ev;
    u32 ae_init_ev;

    u32 ae_curve_type;

    u32 ae_target;

    u32 ae_phase_comp;

    u32 ae_luma_smooth_num;

    u32 ae_conver_h;
    u32 ae_conver_l;
    u32 ae_diver_h;
    u32 ae_diver_l;

    u32 ae_ratio_max_h;
    u32 ae_ratio_max_l;

    u32 ae_ratio_max2_h;
    u32 ae_ratio_max2_l;

    u32 ae_ratio_slope;

    u32 ae_expline_th;

    u32 *ae_user_ev_value_table;
    u32 ae_user_ev_value_table_size;
    s32 ae_user_ev_value; // centered to zero

    u8 ae_init_weight_type;
    u8 *ae_win_cust_weights;

    isp_ae_ops_t ae_ops;

    u32 ae_step;

    u32 ae_hl_comp_en;
    u32 ae_hl_count_th0;
    u32 ae_hl_count_th1;
    u32 ae_hl_lv_th;
    u32 ae_hl_comp_max;

    u32 reserved[11];

} isp_ae_params_t;


//AWB
typedef enum {
    AWB_DAYLIGHT,  // 晴天5600K
    AWB_CLOUNDY,       //  多云/阴天 D65 6500K
    AWB_TUNGSTEN,      // 钨丝灯/白炽灯 TL83/U30 3000K
    AWB_FLUORESCENT1,    // 荧光灯1 D65 6500K
    AWB_FLUORESCENT2,    // 荧光灯2 TL84 4000K
    AWB_AUTO,
} isp_wb_type_t;

#define AWB_SCENE_MAX   AWB_AUTO

enum {
    AWB_ALG_GW1 = 0,
    AWB_ALG_GW2,
    AWB_ALG_CT1,
    AWB_ALG_CT2,
};

typedef enum {
    AWB_WIN_WEIGHT_AVG = 0,  //  平均测光
    AWB_WIN_WEIGHT_CENTRE,   //  中央区域测光
    AWB_WIN_WEIGHT_SPOT,     //  点测光
    AWB_WIN_WEIGHT_MULTI,    //  多区域测光
    AWB_WIN_WEIGHT_CARDV,    //  行车记录仪专用
    AWB_WIN_WEIGHT_CUST,
} isp_awb_ww_type_t;

#define AWB_WIN_WEIGHT_MAX  AWB_WIN_WEIGHT_CUST

#define ISP_AWB_ONE      (1<<10)
typedef struct {
    u16 r_gain;
    u16 g_gain;
    u16 b_gain;
} isp_wb_gain_t;

typedef struct {
    s16 wp_th;
    u8 r_th;
    u8 g_th;
    u8 b_th;

    u16 wp_ratio_th_numerator;
    u16 wp_ratio_th_denominator;

    u16 rgain_min;
    u16 rgain_max;
    u16 bgain_min;
    u16 bgain_max;
    u32 ev_th;

    u32 ultralow_th;
    u16 ultralow_rgain;
    u16 ultralow_bgain;
} isp_awb_gw1_params_t;

typedef struct {
    u32 prev_w;
    u32 new_w;

    u16 rgain_min;
    u16 rgain_max;
    u16 bgain_min;
    u16 bgain_max;
} isp_awb_gw2_params_t;

typedef enum {
    ISP_AWB_ZONE_UNKNOWN = -1,
    ISP_AWB_ZONE_UHCT,
    ISP_AWB_ZONE_HCT,
    ISP_AWB_ZONE_MCT,
    ISP_AWB_ZONE_LCT,
    ISP_AWB_ZONE_ULCT,
    ISP_AWB_ZONE_GREEN,
    ISP_AWB_ZONE_SKIN,
    ISP_AWB_ZONE_MAX,
} isp_awb_zone_type_t;

typedef struct {
    isp_awb_zone_type_t type;

    u16 rg_min; //q8
    u16 bg_min; //q8

    u16 rg_max; //q8
    u16 bg_max; //q8

    u16 rg_center; //q8
    u16 bg_center; //q8

    //8x8 array
    const u8 *desc;
} isp_awb_zone_t;

#define ISP_AWB_CT1_WP_WEIGHT_SIZE    16

typedef struct {
    isp_awb_zone_t *zones[ISP_AWB_ZONE_MAX];

    //16x1 array
    u8 *wp_weights;

    u8  y_min;
    u8  y_max;
    u32 hct_ev_th;
    u32 lct_ev_th;

    u32 prev_w;
    u32 new_w;

    u32 ev_th;
} isp_awb_ct1_params_t;


typedef struct {
    void (*awb_post_process)(u32 prob, u32 ev, u32 zone_type, u16 *r_gain, u16 *b_gain, void *data);

    isp_awb_zone_t *zones[ISP_AWB_ZONE_MAX];

    u32 prev_w;
    u32 new_w;

    u8  y_min;
    u8  y_max;

    u32 refine_radius;

    //base 1000
    u32 subwin_weight_ratio_th;
    u32 zone_weight_ratio_th;
    u32 green_weight_ratio_th;
    u32 gw_weight_ratio_th;

    u32 daylight_ev_th;
    u32 outdoor_ev_th;
    u32 lowlight_ev_th;

    u8  fallback_gw;
    u8  inference;
    u8  inference_g;
    u8  inference_d;
    u8  refine;
    u32 skylight_high_ev_th;
    u32 skylight_low_ev_th;

} isp_awb_ct2_params_t;

typedef struct {
    u8 awb_alg_type;
    u8 awb_scene_type;
    u8 awb_win_type;

    isp_wb_gain_t  awb_init_gain;
    isp_wb_gain_t  awb_scene_gains[AWB_SCENE_MAX];

    u8 *awb_win_cust_weights;
    u8 *awb_ct_weights;

    isp_awb_gw1_params_t *awb_gw1_params;
    isp_awb_gw2_params_t *awb_gw2_params;
    isp_awb_ct1_params_t *awb_ct1_params;
    isp_awb_ct2_params_t *awb_ct2_params;

} isp_awb_params_t;

typedef struct {
    s16     r;
    s16     gr;
    s16     gb;
    s16     b;
} isp_blc_t;

typedef struct {
    u16     dth;
    u16     sth;
    u16     vth;
} isp_dpc_t;

typedef struct {
    u16     cx;
    u16     cy;
    u16     *lr;
    u16     *lg;
    u16     *lb;
    u16     dp0;
    u16     dp1;
    u16     dth;
} isp_lsc_t;

typedef struct {
    u8      en3d;
    u8      str;
    u16     mth;
    u16     lth;
    u16     hth;
    u8      wm0;
    u8      wm1;
    u8      c;
    u16     s[8];
} __attribute__((packed)) isp_tnr_t;

typedef s16(isp_ccm_t)[12];

typedef u8  *isp_gamma_t;

typedef struct {
    u32 enable;
    u8  *r_gamma;
    u8  *g_gamma;
    u8  *b_gamma;
} isp_three_gamma_t;

typedef struct {
    u8      s0/*mul 10*/;
    u8      s1;
    u16     th;

} isp_nr_t;

typedef struct {
    u8      lth0;
    u8      lth1;
    u8      o0;
    u8      o1;
    u16     a0;
    u16     a1;
    u8      g0;
    u8      g1;
    u16     th00;
    u16     th01;
    u16     th02;
    u16     th10;
    u16     th11;
} isp_shp_t;

typedef struct {

    u8      men;
    u8      th;
    u8      s/*mul 10*/;
} isp_cnr_t;


typedef struct {
    u16    *curve;
    u32     g;
} isp_wdr_t;

typedef struct {
    u16     gc0;
    u16     gc1;
    u16     gc2;
    s16     oc0;
    s16     oc1;
    s16     oc2;
} isp_adj_t;

typedef struct {
    isp_blc_t           blc;
    isp_lsc_t           lsc;
    isp_wdr_t           wdr;

    isp_adj_t           adj;
    isp_gamma_t         gamma;

    isp_ccm_t           ccm;

    isp_dpc_t           dpc;
    isp_tnr_t           tnr;
    isp_nr_t            nr;
    isp_shp_t           shp;
    isp_cnr_t           cnr;

    u32                 md_wms[5];
    u32                 md_level;
    isp_three_gamma_t       three_gamma;
} isp_iq_params_t;


void  isp_ae_curve_interp(u32(*ae_curve)[AE_CURVE_INFO_MAX], u32 ev, u32 *time, u32 *gain/*q10*/);

#endif
