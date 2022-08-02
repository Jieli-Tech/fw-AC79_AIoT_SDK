#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "pv4109.h"
#include "asm/isp_alg.h"
#include "device/iic.h"

#define PCLK  74.25
#define LINE_LENGTH_CLK     2200
#define FRAME_LENGTH30        1124
#define FRAME_LENGTH25        1348

static void *iic = NULL;

static u8 WRCMD = 0x64;
static u8 RDCMD = 0x65;

static u8 reset_gpios[2] = {-1, -1};
static u8 pwdn_gpios[2] = {-1, -1};
static u32 frameLen ;

static u32 cur_again = -1;
static u32 cur_dgain = -1;
static u32 cur_expline = -1;
extern void *PV4109_DVP_get_ae_params();
extern void *PV4109_DVP_get_awb_params();
extern void *PV4109_DVP_get_iq_params();
extern void  PV4109_DVP_ae_ev_init(u32 fps);


typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_ini;
const Sensor_reg_ini PV4109_DVP_INI_REG[] = {
///[PV4109K(Rev.0)]

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// Don't change setting value start //////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////! PV4109K_DIGITAL_211206 (PV4109K_Digital_DVP_YUV_1280x720_25fps_211206.ccf)

//[UNKNO{0xN]



//// Digital I/F //////////////////////////////////////////
    {0x03, 0x00},
    {0x35, 0xF0}, // i2c_control_1
    {0x57, 0x63}, // pll_n
    {0x58, 0x05}, // pll_r //04
    {0x59, 0xB8}, // pll_r
    {0x65, 0x01}, // pad_control7
    {0x60, 0x00}, // pad_control2
    {0x61, 0x00}, // pad_control3
    {0x62, 0x00}, // pad_control4
    {0x06, 0x07}, // frame{0xidth_h // 25fps
    {0x07, 0xBB}, // frame{0xidth_l
    {0x14, 0x00}, // vsyncstartro{0x_f0_h
    {0x15, 0x0F}, // vsyncstartro{0x_f0_l
    {0x16, 0x02}, // vsyncstopro{0x_f0_h
    {0x17, 0xDF}, // vsyncstopro{0x_f0_l
    {0x1D, 0x0A},
    {0x35, 0x50}, // i2c_control_1
    {0x55, 0x52}, // pll_control2

    {0x03, 0x00},
    {0x05, 0x03},	//mirror
    {0x3D, 0xB8},	//analog_control_01
    {0x3E, 0x08},	//analog_control_02
    {0x3F, 0x04},	//analog_control_03
    {0x40, 0x46},	//analog_control_04
    {0x41, 0x54},	//analog_control_05
    {0x42, 0xA0},	//analog_control_06
    {0x45, 0x13},	//analog_control_09

    {0x5C, 0x00},	//clkdiv4
    {0x96, 0xA4},	//vdac_ctrl1
    {0xBC, 0x00},	//close osd
    {0x03, 0x01},
    {0x05, 0xCF},	//bayer_control_02
    {0x06, 0xB0},	//bayer_control_03
    {0x0A, 0xA4},	//bayer_control_07
    {0x2D, 0x03},	//tghstart_h
    {0x2E, 0x20},	//tghstart_l
    {0x2F, 0x05},	//tghstop_h
    {0x30, 0xA2},	//tghstop_l
    {0x35, 0x03},	//blhstart_h
    {0x36, 0x20},	//blhstart_l
    {0x37, 0x05},	//blhstop_h
    {0x38, 0xA2},	//blhstop_l
    {0x39, 0x07},	//coffset_h
    {0x3A, 0xA2},	//coffset_l
    {0x63, 0x5F},	//globalgain_max
    {0xA3, 0x00},	//fb_ref0
    {0xA4, 0x02},	//fb_ref1
    {0xA5, 0x03},	//fb_ref2
    {0xA6, 0x04},	//fb_ref3
    {0xA7, 0x05},	//fb_ref4
    {0xA8, 0x08},	//fb_ref5
    {0xA9, 0x2A},	//fb_ref6
    {0xAA, 0x10},	//fb_ref7
    {0xCF, 0x00},	//ext_rampc
    {0xD0, 0x00},	//blc_top_th_h
    {0xD1, 0xD0},	//blc_top_th_l
    {0xD2, 0x00},	//blc_bot_th_h
    {0xD3, 0x70},	//blc_bot_th_l
    {0xD5, 0x00},	//rampc_x0
    {0xD6, 0x20},	//rampc_x1
    {0xD7, 0x40},	//rampc_x2
    {0xD8, 0x80},	//rampc_x3
    {0xD9, 0xF0},	//rampc_x4
    {0xDA, 0x00},	//rampc_y0
    {0xDB, 0x20},	//rampc_y1
    {0xDC, 0x46},	//rampc_y2
    {0xDD, 0x86},	//rampc_y3
    {0xDE, 0xFF},	//rampc_y4

    {0x03, 0x02},
    {0x04, 0x00},	//ro{0x_ls_en_start_h
    {0x05, 0x0B},	//ro{0x_ls_en_start_l
    {0x06, 0x07},	//ro{0x_ls_en_stop_h
    {0x07, 0xBC},	//ro{0x_ls_en_stop_l
    {0x08, 0x00},	//ro{0x_ls_rst_start_h
    {0x09, 0x0F},	//ro{0x_ls_rst_start_l
    {0x0A, 0x07},	//ro{0x_ls_rst_stop_h
    {0x0B, 0xB2},	//ro{0x_ls_rst_stop_l
    {0x0C, 0x00},	//ro{0x_ls_tx_start_h
    {0x0D, 0xE6},	//ro{0x_ls_tx_start_l
    {0x0E, 0x01},	//ro{0x_ls_tx_stop_h
    {0x0F, 0x13},	//ro{0x_ls_tx_stop_l
    {0x10, 0x07},	//ro{0x_flush_start_start_h
    {0x11, 0xB4},	//ro{0x_flush_start_start_l
    {0x12, 0x07},	//ro{0x_flush_start_stop_h
    {0x13, 0xB6},	//ro{0x_flush_start_stop_l
    {0x14, 0x07},	//ro{0x_latch_rst_start_h
    {0x15, 0xB8},	//ro{0x_latch_rst_start_l
    {0x16, 0x07},	//ro{0x_latch_rst_stop_h
    {0x17, 0xBA},	//ro{0x_latch_rst_stop_l
    {0x1F, 0x00},	//cds_load_start1_h
    {0x20, 0x79},	//cds_load_start1_l
    {0x21, 0x00},	//cds_load_stop1_h
    {0x22, 0xC9},	//cds_load_stop1_l
    {0x23, 0x01},	//cds_load_start2_h
    {0x24, 0x78},	//cds_load_start2_l
    {0x25, 0x03},	//cds_load_stop2_h
    {0x26, 0x1B},	//cds_load_stop2_l
    {0x27, 0x00},	//cds_latch_start1_h
    {0x28, 0x7A},	//cds_latch_start1_l
    {0x29, 0x00},	//cds_latch_stop1_h
    {0x2A, 0xC8},	//cds_latch_stop1_l
    {0x2B, 0x01},	//cds_latch_start2_h
    {0x2C, 0x79},	//cds_latch_start2_l
    {0x2D, 0x03},	//cds_latch_stop2_h
    {0x2E, 0x1A},	//cds_latch_stop2_l
    {0x2F, 0x00},	//cds_store1_start_h
    {0x30, 0x1E},	//cds_store1_start_l
    {0x31, 0x00},	//cds_store1_stop_h
    {0x32, 0x5F},	//cds_store1_stop_l
    {0x33, 0x00},	//cds_store2_stop1_h
    {0x34, 0x64},	//cds_store2_stop1_l
    {0x35, 0x02},	//cds_store2_start2_h
    {0x36, 0xFE},	//cds_store2_start2_l
    {0x37, 0x03},	//cds_store2_stop2_h
    {0x38, 0x17},	//cds_store2_stop2_l
    {0x39, 0x00},	//cds_bls_en_start_h
    {0x3A, 0xE5},	//cds_bls_en_start_l
    {0x3B, 0x00},	//cds_bias_track_start_h
    {0x3C, 0x00},	//cds_bias_track_start_l
    {0x3D, 0x00},	//cds_bias_track_stop_h
    {0x3E, 0x28},	//cds_bias_track_stop_l
    {0x4F, 0x07},	//ramp_atten_rst_start_h
    {0x50, 0x26},	//ramp_atten_rst_start_l
    {0x51, 0x07},	//ramp_atten_rst_stop_h
    {0x52, 0xBC},	//ramp_atten_rst_stop_l
    {0x54, 0x00},	//rst_offset_stop_h
    {0x55, 0x69},	//rst_offset_stop_l
    {0x56, 0x00},	//rst_ramp_start_h
    {0x57, 0x7E},	//rst_ramp_start_l
    {0x58, 0x00},	//rst_ramp_stop_h
    {0x59, 0xE1},	//rst_ramp_stop_l
    {0x5C, 0x01},	//sig_ramp_start_l
    {0x5D, 0x7D},	//sig_ramp_stop_h
    {0x5E, 0x03},	//sig_ramp_stop_l
    {0x5F, 0x1B},	//sig_rampcnt
    {0x67, 0x02},	//ramp_bls_en_stop_h
    {0x68, 0xFE},	//ramp_bls_en_stop_l
    {0x69, 0x03},	//ramp_bls_x_ref
    {0x6A, 0x17},	//reserved
    {0xA8, 0x00},	//lim_fit0
    {0xA9, 0x09},	//lim_fit1
    {0xAA, 0x0D},	//lim_fit2
    {0xBC, 0x00},	//adc_clk_start1_h
    {0xBD, 0x7E},	//adc_clk_start1_l
    {0xBE, 0x00},	//adc_clk_stop1_h
    {0xBF, 0xC6},	//adc_clk_stop1_l
    {0xC0, 0x01},	//adc_clk_start2_h
    {0xC1, 0x7D},	//adc_clk_start2_l
    {0xC2, 0x02},	//adc_clk_stop2_h
    {0xC3, 0xFD},	//adc_clk_stop2_l
    {0xC4, 0x07},	//cntr_rstb_start_h
    {0xC5, 0xB8},	//cntr_rstb_start_l
    {0xC6, 0x07},	//cntr_rstb_stop_h
    {0xC7, 0xBA},	//cntr_rstb_stop_l
    {0xC8, 0x03},	//cntr_bls_latch_start_h
    {0xC9, 0x11},	//cntr_bls_latch_start_l
    {0xCA, 0x03},	//cntr_bls_latch_stop_h
    {0xCB, 0x15},	//cntr_bls_latch_stop_l

    {0x03, 0x03},
    {0x2B, 0x07},	//a{0xb_ctrl
    {0x2C, 0x10},	//a{0xb_size
    {0x2D, 0xF0},	//a{0xb_maxc
    {0x2E, 0x02},	//a{0xb_minc
    {0x2F, 0x02},	//a{0xb_redp
    {0x30, 0x40},	//a{0xb_gth
    {0x31, 0x01},	//a{0xb_nth_h
    {0x32, 0xF4},	//a{0xb_nth_l
    {0x33, 0x02},	//a{0xb_ath
    {0x34, 0x6E},	//a{0xb_ired
    {0x35, 0x27},	//a{0xb_iblu
    {0x36, 0x2D},	//a{0xb_rmin1
    {0x37, 0x47},	//a{0xb_rmax1
    {0x38, 0x8D},	//a{0xb_bmin1
    {0x39, 0x01},	//a{0xb_bmax1_h
    {0x3A, 0x00},	//a{0xb_bmax1_l
    {0x3B, 0x41},	//a{0xb_dbup1
    {0x3C, 0x34},	//a{0xb_dbl{0x1
    {0x3D, 0x54},	//a{0xb_rmin2
    {0x3E, 0x80},	//a{0xb_rmax2
    {0x3F, 0x48},	//a{0xb_bmin2
    {0x40, 0xA7},	//a{0xb_bmax2
    {0x41, 0x36},	//a{0xb_dbup2
    {0x42, 0x34},	//a{0xb_dbl{0x2
    {0x55, 0x04},	//flicker_control1
    {0x5A, 0x00},	//fd_period_b_h
    {0x5B, 0xE1},	//fd_period_b_m
    {0x5C, 0x00},	//fd_period_b_l
    {0x67, 0xC8},	//isp_func_1
    {0x68, 0x21},	//isp_func_2
    {0x6B, 0x40},	//isp_func_5
    {0x6D, 0x8A},	//isp_func_7
    {0x86, 0x03},	//lens_gainr
    {0x87, 0x19},	//lens_gaing1
    {0x88, 0x19},	//lens_gaing2
    {0x89, 0x03},  //lens_gainb
    {0x8B, 0x14},	//dpc_offset
    {0xB1, 0x40},	//reserved
    {0xB2, 0xC0},	//reserved
    {0xD0, 0x5E},	//egm_y0
    {0xD1, 0x55},	//egm_y1
    {0xD2, 0x4B},	//egm_y2
    {0xD3, 0x45},	//egm_y3
    {0xD4, 0x49},	//egm_y4
    {0xD5, 0x51},	//egm_y5
    {0xD6, 0x56},	//egm_y6

    {0x03, 0x04},
    {0x04, 0x00},	//ygm1_y0
    {0x05, 0x00},	//ygm1_y1
    {0x06, 0x00},	//ygm1_y2
    {0x07, 0x01},	//ygm1_y3
    {0x08, 0x02},	//ygm1_y4
    {0x09, 0x04},	//ygm1_y5
    {0x0A, 0x06},	//ygm1_y6
    {0x0B, 0x08},	//ygm1_y7
    {0x0C, 0x0B},	//ygm1_y8
    {0x0D, 0x11},	//ygm1_y9
    {0x0E, 0x18},	//ygm1_y10
    {0x0F, 0x1F},	//ygm1_y11
    {0x10, 0x27},	//ygm1_y12
    {0x11, 0x36},	//ygm1_y13
    {0x12, 0x44},	//ygm1_y14
    {0x13, 0x50},	//ygm1_y15
    {0x14, 0x5C},	//ygm1_y16
    {0x15, 0x83},	//ygm1_y17
    {0x16, 0xA2},	//ygm1_y18
    {0x17, 0xBD},	//ygm1_y19
    {0x18, 0xD5},	//ygm1_y20
    {0x19, 0xEB},	//ygm1_y21
    {0x1A, 0xFF},	//ygm1_y22
    {0x7A, 0x1F},	//user_cs

    {0x03, 0x07},
    {0x51, 0x0F},	//nr2d2_lut00
    {0x52, 0x19},	//nr2d2_lut01
    {0x53, 0x26},	//nr2d2_lut02
    {0x54, 0x3A},	//nr2d2_lut03
    {0x55, 0x4C},	//nr2d2_lut04
    {0x56, 0x56},	//nr2d2_lut05
    {0x57, 0x60},	//nr2d2_lut06
    {0x58, 0x6A},	//nr2d2_lut07
    {0x59, 0x74},	//nr2d2_lut08
    {0x5A, 0x7E},	//nr2d2_lut09
    {0x5B, 0x88},	//nr2d2_lut10
    {0x5C, 0x92},	//nr2d2_lut11
    {0x5D, 0x9C},	//nr2d2_lut12
    {0x5E, 0xA6},	//nr2d2_lut13
    {0x5F, 0xB0},	//nr2d2_lut14
    {0x60, 0xBA},	//nr2d2_lut15
    {0x61, 0xC4},	//nr2d2_lut16
    {0x62, 0xCE},	//nr2d2_lut17
    {0x63, 0xD8},	//nr2d2_lut18
    {0x64, 0xDE},	//nr2d2_lut19
    {0x65, 0xE0},	//nr2d2_lut20
    {0x66, 0xE2},	//nr2d2_lut21
    {0x67, 0xE4},	//nr2d2_lut22
    {0x68, 0xE6},	//nr2d2_lut23
    {0x69, 0xE8},	//nr2d2_lut24
    {0x6A, 0xEB},	//nr2d2_lut25
    {0x6B, 0xED},	//nr2d2_lut26
    {0x6C, 0xEE},	//nr2d2_lut27
    {0x6D, 0xF0},	//nr2d2_lut28
    {0x6E, 0xF2},	//nr2d2_lut29
    {0x6F, 0xF5},	//nr2d2_lut30
    {0x70, 0xFA},	//nr2d2_lut31
    {0x71, 0xFF},	//nr2d2_lut32

    {0x03, 0x08},
    {0x79, 0x90},	//DAC_OUT_SEL

    {0x03, 0x09},
    {0x1C, 0x20},	//dark_dpc_p0
    {0x1D, 0x20},	//dark_dpc_p1
    {0x1E, 0x20},	//dark_dpc_p2
    {0x1F, 0x5A},	//dark_dpc_p3
    {0x20, 0x5A},	//dark_dpc_p4
    {0x21, 0x5A},	//dark_dpc_p5
    {0x22, 0x20},	//dark_dpc_n0
    {0x23, 0x20},	//dark_dpc_n1
    {0x24, 0x20},	//dark_dpc_n2
    {0x25, 0x5A},	//dark_dpc_n3
    {0x26, 0x5A},	//dark_dpc_n4
    {0x27, 0x5A},	//dark_dpc_n5
    {0x28, 0x5D},	//reserved
    {0x29, 0xFF},	//reserved
    {0x2A, 0xFF},	//reserved
    {0x2B, 0x5F},	//reserved
    {0x2C, 0x5F},	//reserved
    {0x2D, 0x00},	//reserved
    {0x2E, 0x00},	//reserved
    {0x2F, 0x00},	//reserved
    {0x30, 0x26},	//reserved
    {0x31, 0x26},	//reserved
    {0x32, 0x40},	//reserved
    {0x33, 0x40},	//reserved
    {0x34, 0x40},	//reserved
    {0x35, 0x40},	//reserved
    {0x36, 0x40},	//reserved
    {0x37, 0x03},	//nr2d_dark0
    {0x38, 0x04},	//nr2d_dark1
    {0x39, 0x06},	//nr2d_dark2
    {0x3A, 0x08},	//nr2d_dark3
    {0x3B, 0x0B},	//nr2d_dark4
    {0x3C, 0x1F},	//adg2_maxb0
    {0x3D, 0x1F},	//adg2_maxb1
    {0x3E, 0x1F},	//adg2_maxb2
    {0x3F, 0x1F},	//adg2_maxb3
    {0x40, 0x1F},	//adg2_maxb4
    {0x41, 0x00},	//nr_xref0
    {0x42, 0x10},	//nr_xref1
    {0x43, 0x30},	//nr_xref2
    {0x44, 0x40},	//nr_xref3
    {0x45, 0x5F},	//nr_xref4
    {0x46, 0x3F},	//adg2_gamma0
    {0x47, 0x34},	//adg2_gamma1
    {0x48, 0x33},	//adg2_gamma2
    {0x49, 0x3B},	//adg2_gamma3
    {0x4A, 0x2A},	//adg2_gamma4
    {0x4B, 0x2A},	//adg2_gamma5
    {0x4C, 0x04},	//fcmscl_ref0
    {0x4D, 0x04},	//fcmscl_ref1
    {0x4E, 0x04},	//fcmscl_ref2
    {0x4F, 0x04},	//fcmscl_ref3
    {0x50, 0x04},	//fcmscl_ref4
    {0x51, 0x00},	//fchscl_ref0
    {0x52, 0x00},	//fchscl_ref1
    {0x53, 0x00},	//fchscl_ref2
    {0x54, 0x00},	//fchscl_ref3
    {0x55, 0x00},	//fchscl_ref4
    {0x56, 0x00},	//edgeth_ref0
    {0x57, 0x00},	//edgeth_ref1
    {0x58, 0x00},	//edgeth_ref2
    {0x59, 0x00},	//edgeth_ref3
    {0x5A, 0x00},	//edgeth_ref4
    {0x5B, 0x00},	//ymcor_ref0
    {0x5C, 0x00},	//ymcor_ref1
    {0x5D, 0x00},	//ymcor_ref2
    {0x5E, 0x00},	//ymcor_ref3
    {0x5F, 0x00},	//ymcor_ref4
    {0x60, 0x00},	//yhcor_ref0
    {0x61, 0x00},	//yhcor_ref1
    {0x62, 0x00},	//yhcor_ref2
    {0x63, 0x00},	//yhcor_ref3
    {0x64, 0x00},	//yhcor_ref4
    {0x65, 0x14},	//ymgain_ref0
    {0x66, 0x17},	//ymgain_ref1
    {0x67, 0x18},	//ymgain_ref2
    {0x68, 0x1D},	//ymgain_ref3
    {0x69, 0x00},	//ymgain_ref4
    {0x6A, 0x08},	//yhgain_ref0
    {0x6B, 0x08},	//yhgain_ref1
    {0x6C, 0x06},	//yhgain_ref2
    {0x6D, 0x06},	//yhgain_ref3
    {0x6E, 0x00},	//yhgain_ref4
    {0x75, 0x0A},	//dark_e_gm0
    {0x76, 0x0A},	//dark_e_gm1
    {0x77, 0x00},	//dark_e_gm2
    {0x78, 0x00},	//dark_e_gm3
    {0x79, 0x00},	//dark_e_gm4
    {0x7A, 0x00},	//dark_e_gm5
    {0x7B, 0x2B},	//edge_pgain0
    {0x7C, 0x20},	//edge_pgain1
    {0x7D, 0x15},	//edge_pgain2
    {0x7E, 0x0B},	//edge_pgain3
    {0x7F, 0x03},	//edge_pgain4
    {0x80, 0x03},	//edge_pgain5
    {0x81, 0x38},	//edge_mgain0  //0x50 锐度
    {0x82, 0x40},	//edge_mgain1
    {0x83, 0x1C},	//edge_mgain2
    {0x84, 0x0E},	//edge_mgain3
    {0x85, 0x04},	//edge_mgain4
    {0x86, 0x00},	//edge_mgain5
    {0x8C, 0x80},	//dark_ec_pmax0
    {0x8D, 0x80},	//dark_ec_pmax1
    {0x8E, 0x80},	//dark_ec_pmax2
    {0x8F, 0x80},	//dark_ec_mmax0
    {0x90, 0x80},	//dark_ec_mmax1
    {0x91, 0x80},	//dark_ec_mmax2
    {0x92, 0x04},	//dark_ec_pth0
    {0x93, 0x04},	//dark_ec_pth1
    {0x94, 0x04},	//dark_ec_pth2
    {0x95, 0x04},	//dark_ec_pth3
    {0x96, 0x04},	//dark_ec_pth4
    {0x97, 0x04},	//dark_ec_mth0
    {0x98, 0x04},	//dark_ec_mth1
    {0x99, 0x04},	//dark_ec_mth2
    {0x9A, 0x04},	//dark_ec_mth3
    {0x9B, 0x04},	//dark_ec_mth4
    {0xA1, 0x01},	//dark_y_gm0
    {0xA2, 0x03},	//dark_y_gm1
    {0xA3, 0x05},	//dark_y_gm2
    {0xA4, 0x06},	//dark_y_gm3
    {0xA5, 0x20},	//dark_y_gm4
    {0xA6, 0x20},	//dark_y_gm5
    {0xA7, 0x00},	//ybrightness_ref0
    {0xA8, 0x00},	//ybrightness_ref1
    {0xA9, 0x00},	//ybrightness_ref2
    {0xAA, 0x00},	//ybrightness_ref3
    {0xAB, 0x08},	//ybrightness_ref4
    {0xAC, 0x08},	//ybrightness_ref5
    {0xAD, 0x28},	//y_cont_bx0
    {0xAE, 0x28},	//y_cont_bx1
    {0xAF, 0x2E},	//y_cont_bx2
    {0xB0, 0x30},	//y_cont_bx3
    {0xB1, 0x34},	//y_cont_bx4
    {0xB3, 0xC2},	//y_cont_by0
    {0xB4, 0xC2},	//y_cont_by1
    {0xB5, 0xC2},	//y_cont_by2
    {0xB6, 0xC2},	//y_cont_by3
    {0xB7, 0xC2},	//y_cont_by4
    {0xB9, 0x00},	//dark_lens_ref0
    {0xBA, 0x00},	//dark_lens_ref1
    {0xBB, 0x00},	//dark_lens_ref2
    {0xBC, 0x00},	//dark_lens_ref3
    {0xBD, 0x00},	//dark_lens_ref4
    {0xBE, 0x00},	//dark_lens_ref5
    {0xBF, 0x3C},	//ccr_m11_a
    {0xC0, 0x3C},	//ccr_m11_b
    {0xC1, 0x3C},	//ccr_m11_c
    {0xC2, 0x9B},	//ccr_m12_a
    {0xC3, 0x9B},	//ccr_m12_b
    {0xC4, 0x9B},	//ccr_m12_c
    {0xC5, 0x80},	//ccr_m13_a
    {0xC6, 0x80},	//ccr_m13_b
    {0xC7, 0x80},	//ccr_m13_c
    {0xC8, 0x8C},	//ccr_m21_a
    {0xC9, 0x8C},	//ccr_m21_b
    {0xCA, 0x8C},	//ccr_m21_c
    {0xCB, 0x3A},	//ccr_m22_a
    {0xCC, 0x3A},	//ccr_m22_b
    {0xCD, 0x3A},	//ccr_m22_c
    {0xCE, 0x8E},	//ccr_m23_a
    {0xCF, 0x8E},	//ccr_m23_b
    {0xD0, 0x8E},	//ccr_m23_c
    {0xD1, 0x82},	//ccr_m31_a
    {0xD2, 0x82},	//ccr_m31_b
    {0xD3, 0x82},	//ccr_m31_c
    {0xD4, 0x96},	//ccr_m32_a
    {0xD5, 0x96},	//ccr_m32_b
    {0xD6, 0x96},	//ccr_m32_c
    {0xD7, 0x38},	//ccr_m33_a
    {0xD8, 0x38},	//ccr_m33_b
    {0xD9, 0x38},	//ccr_m33_c
    {0xDA, 0x02},	//dark_ccr0
    {0xDB, 0x02},	//dark_ccr1
    {0xDC, 0x02},	//dark_ccr2
    {0xDD, 0x02},	//dark_ccr3
    {0xDE, 0x0B},	//dark_ccr4
    {0xDF, 0x16},	//dark_ccr5
    {0xE0, 0x02},	//dark_rgb_gm0
    {0xE1, 0x05},	//dark_rgb_gm1
    {0xE2, 0x08},	//dark_rgb_gm2
    {0xE3, 0x16},	//dark_rgb_gm3
    {0xE4, 0x20},	//dark_rgb_gm4
    {0xE5, 0x20},	//dark_rgb_gm5
    {0xE6, 0x40},	//#y_{0xeight_ref0
    {0xE7, 0x40},	//#y_{0xeight_ref1
    {0xE8, 0x40},	//#y_{0xeight_ref2
    {0xE9, 0x40},	//#y_{0xeight_ref3
    {0xEA, 0x40},	//#y_{0xeight_ref4
    {0xEB, 0x40},	//#y_{0xeight_ref5
    {0xEC, 0x04},	//dark_dc0
    {0xED, 0x04},	//dark_dc1
    {0xEE, 0x04},	//dark_dc2
    {0xEF, 0x04},	//dark_dc3
    {0xF0, 0x06},	//dark_dc4
    {0xF1, 0x08},	//dark_dc5
    {0xF2, 0x00},	//dark_dc_y10
    {0xF3, 0x00},	//dark_dc_y11
    {0xF4, 0x02},	//dark_dc_y12
    {0xF5, 0x7E},	//dark_dc_y13
    {0xF6, 0x17},	//dark_dc_y14
    {0xF7, 0x1C},	//dark_dc_y15
    {0xF8, 0xFF},	//dark_dc_y20
    {0xF9, 0xFF},	//dark_dc_y21
    {0xFA, 0xFF},	//dark_dc_y22
    {0xFB, 0xFF},	//dark_dc_y23
    {0xFC, 0xFF},	//dark_dc_y24
    {0xFD, 0xFF},	//dark_dc_y25

    {0x03, 0x0D},
    {0x04, 0x34},	//axis_a
    {0x05, 0x5A},	//axis_b
    {0x06, 0x60},	//axis_c
    {0x07, 0x22},	//cs11_a
    {0x08, 0x25},	//cs11_b
    {0x09, 0x1F},	//cs11_c
    {0x0A, 0x84},	//cs12_a
    {0x0B, 0x00},	//cs12_b
    {0x0C, 0x82},	//cs12_c
    {0x0D, 0x03},	//cs21_a
    {0x0E, 0x00},	//cs21_b
    {0x0F, 0x89},	//cs21_c
    {0x10, 0x1F},	//cs22_a
    {0x11, 0x25},	//cs22_b
    {0x12, 0x20},	//cs22_c
    {0x37, 0x18},	//post_pegain0
    {0x38, 0x18},	//post_pegain1
    {0x39, 0x18},	//post_pegain2
    {0x3A, 0x0B},	//post_pegain3
    {0x3B, 0x00},	//post_pegain4
    {0x3C, 0x00},	//post_pegain5
    {0x3D, 0x1C},	//post_megain0
    {0x3E, 0x1C},	//post_megain1
    {0x3F, 0x1C},	//post_megain2
    {0x40, 0x05},	//post_megain3
    {0x41, 0x00},	//post_megain4
    {0x42, 0x00},	//post_megain5
    {0x44, 0x00},	//blc_xref0
    {0x45, 0x00},	//blc_xref1
    {0x46, 0x00},	//blc_xref2
    {0x47, 0x00},	//blc_xref3
    {0x48, 0x00},	//blc_xref4
    {0x49, 0x20},	//dark_ymref1
    {0x4A, 0x10},	//dark_ymref2

    {0x03, 0x0E},
    {0x05, 0x64},	//auto_control_2
    {0x06, 0xA0},	//auto_control_3
    {0x07, 0x20},	//auto_control_4
    {0x09, 0x00},	//{0xb_mode
    {0x0A, 0x02},	//expfrmh_h
    {0x0B, 0xE7},	//expfrmh_l
    {0x0C, 0x02},	//midfrmheight_h
    {0x0D, 0xE7},	//midfrmheight_l
    {0x0E, 0x02},	//maxfrmheight_h
    {0x0F, 0xEA},	//maxfrmheight_l
    {0x10, 0x00}, 	//minexp_h
    {0x11, 0x00}, 	//minexp_m
    {0x12, 0x0C}, 	//minexp_l
    {0x13, 0x00},	//midexp_t
    {0x14, 0x2E},	//midexp_h
    {0x15, 0xA0},	//midexp_m
    {0x16, 0x00},	//maxexp_t
    {0x17, 0x2E},	//maxexp_h
    {0x18, 0xA0},	//maxexp_m
    {0x38, 0x80},	//max_yt1    //0x80亮度
    {0x39, 0x78},	//#max_yt2
    {0x3A, 0x68},	//max_yt3
    {0x3B, 0x78},	//min_yt1
    {0x3C, 0x68},	//min_yt2
    {0x3D, 0x68},	//min_yt3
    {0x3E, 0x00},	//yt_xref1_t
    {0x3F, 0x01},	//yt_xref1_h
    {0x40, 0x72},	//yt_xref1_m
    {0x41, 0x00},	//yt_xref2_t
    {0x42, 0x02},	//yt_xref2_h
    {0x43, 0xE7},	//yt_xref2_m
    {0x44, 0x00},	//yt_xref3_t
    {0x45, 0x0B},	//yt_xref3_h
    {0x46, 0x9C},	//yt_xref3_m
    {0x47, 0x00},	//yt_xref4_t
    {0x48, 0x17},	//yt_xref4_h
    {0x49, 0x38},	//yt_xref4_m
    {0x50, 0x80},	//user_{0xyt
    {0x51, 0x08},	//ae_up_speed
    {0x52, 0x08},	//ae_do{0xn_speed
    {0x53, 0x10},	//ae_lock
    {0x57, 0x82},	//rg_ratio_a
    {0x58, 0x7C},	//bg_ratio_a
    {0x59, 0x78},	//#rg_ratio_b
    {0x5A, 0x79},	//#bg_ratio_b
    {0x5B, 0x81},	//#rg_ratio_c
    {0x5C, 0x7D},	//#bg_ratio_c
    {0x5D, 0x4A},	//ratio_axis_a
    {0x5E, 0x78},	//ratio_axis_b
    {0x5F, 0x87},	//ratio_axis_c
    {0x60, 0x80},	//a{0xb_rgratio
    {0x61, 0x80},	//a{0xb_bgratio
    {0x62, 0x08},	//a{0xb_lock
    {0x63, 0x08},	//a{0xb_speed
    {0x6A, 0x01},	//dark_xref0_h
    {0x6B, 0x73},	//dark_xref0_m
    {0x6C, 0x05},	//dark_xref1_h
    {0x6D, 0xCE},	//dark_xref1_m
    {0x6E, 0x17},	//dark_xref2_h
    {0x6F, 0x38},	//dark_xref2_m
    {0x70, 0x00},	//dark_xref3_t
    {0x71, 0x2E},	//dark_xref3_h
    {0x72, 0x70},	//dark_xref3_m
    {0x73, 0x00},	//dark_xref4_t
    {0x74, 0xA7},	//dark_xref4_h
    {0x75, 0x6F},	//dark_xref4_m
    {0x81, 0x90},	//a{0xb_full_th1
    {0x82, 0x80},	//a{0xb_full_th2

/////,0x//},/// TO_END_START ////////////////////////////////////


    {0x03, 0x00},
    {0x55, 0x42},	//pll_control2
    {0x60, 0xC0}, // pad_control2
    {0x61, 0x90}, // pad_control3
    {0x62, 0xFF}, // pad_control4

////////// TO_END_STOP ////////////////////////////////////


};



unsigned char wrPV4109_DVPReg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IIC_IOCTL_START, 0);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, WRCMD)) {
        ret = 0;
        puts("\n iic wr err 0\n");
        goto __wend;
    }
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID)) {
        ret = 0;
        puts("\n iic wr err 1\n");
        goto __wend;
    }
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regDat)) {
        ret = 0;
        puts("\n iic wr err 2\n");
        goto __wend;
    }
__wend:

    dev_ioctl(iic, IIC_IOCTL_STOP, 0);

    return ret;

}

unsigned char rdPV4109_DVPReg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, WRCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regID)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, RDCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(10);

    if (dev_ioctl(iic, IIC_IOCTL_RX_WITH_STOP_BIT, (u32)regDat)) {
        ret = 0;
        goto __rend;
    }
__rend:

    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;

}

unsigned char wrPV4109_DVPRegArray(u8 *array, u32 size)
{

    dev_ioctl(iic, IIC_IOCTL_START, 0);
    dev_ioctl(iic, IIC_IOCTL_SET_NORMAT_RATE, 2);
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    dev_write(iic, array, size);
    return 1;

}


/*************************************************************************************************
    sensor api
*************************************************************************************************/



static void pv4109_test_mode(void)
{
    int prodct_jpeg_test_get(void);
    if (prodct_jpeg_test_get()) {
        wrPV4109_DVPReg(0x03, 0x03);
        wrPV4109_DVPReg(0x70, 0x1e);
        wrPV4109_DVPReg(0x71, 0xff);
        wrPV4109_DVPReg(0x72, 0xff);
        wrPV4109_DVPReg(0x73, 0xff);
        wrPV4109_DVPReg(0x74, 0xff);
        wrPV4109_DVPReg(0x75, 0xff);
    }
}
void PV4109_DVP_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    u8 val;

    for (i = 0; i < sizeof(PV4109_DVP_INI_REG) / sizeof(Sensor_reg_ini); i++) {
        if (PV4109_DVP_INI_REG[i].addr == 0xff) {
            os_time_dly(20);    //PV4109_DVP_INI_REG[i].value/2);
        } else {
            wrPV4109_DVPReg(PV4109_DVP_INI_REG[i].addr, PV4109_DVP_INI_REG[i].value);
        }
    }
#ifdef CONFIG_AUTO_PRODUCTION_ENABLE
    pv4109_test_mode();
#endif
    for (i = 0; i < 25; i++) {
        delay(40000);
    }

    /*
    	wrPV4109_DVPReg(0x03,0x05);
    	wrPV4109_DVPReg(0x04,0x50);
        if (*frame_freq == 25)
        {
            frameLen = FRAME_LENGTH25;
            wrPV4109_DVPReg(0x03,0x00);
            wrPV4109_DVPReg(0x08,0x05);
            wrPV4109_DVPReg(0x09,0x44);
            printf("PV4109 25fps\n");
        }
        else
        {
            frameLen = FRAME_LENGTH30;
            printf("PV4109 30fps\n");
        }
    */
    u8 ver, r60 = 0; //,r55=0;
    //rdPV4109_DVPReg(0x55,&r55);
    rdPV4109_DVPReg(0x60, &r60);
    rdPV4109_DVPReg(0x01, &ver);
    printf("\nver:%02x,r60:%02x\n", ver, r60); //,r55);


    return;
}


s32 PV4109_DVP_set_output_size(u16 *width, u16 *height, u8 *frame_freq)
{
    return 0;
}


s32 PV4109_DVP_power_ctl(u8 isp_dev, u8 is_work)
{

    return 0;
}

s32 PV4109_DVP_ID_check(void)
{
    u8 pid = 0x00;
    u8 ver = 0x00;
    u8 i ;

    WRCMD = 0x64;
    RDCMD = 0x65;
    for (i = 0; i < 30; i++) {
        //
        rdPV4109_DVPReg(0x00, &pid);
        rdPV4109_DVPReg(0x01, &ver);
        printf("pv4109 id:0x%x,0x%x\n", pid, ver);
        put_u8hex(pid);
        put_u8hex(ver);
        puts("\n");



        if (pid == 0x41 && ver == 0x09) {
            puts("\n----hello PV4109_DVP-----\n");
            return 0;
        }
    }
    puts("\n----not PV4109_DVP-----\n");
    return -1;


}

static u8 cur_sensor_type = 0xff;
void PV4109_DVP_reset(u8 isp_dev)
{
    u8 reset_gpio;
    u8 pwdn_gpio;

    if (isp_dev == ISP_DEV_0) {
        reset_gpio = reset_gpios[0];
        pwdn_gpio = pwdn_gpios[0];
    } else {
        reset_gpio = reset_gpios[1];
        pwdn_gpio = pwdn_gpios[1];
    }
    gpio_direction_output(reset_gpio, 0);
    gpio_direction_output(pwdn_gpio, 0);

    delay(40000);
    gpio_direction_output(pwdn_gpio, 1);
    delay(40000);

    printf("pwdn_gpio=%d\n", pwdn_gpio);
    gpio_direction_output(reset_gpio, 1);
    delay(40000);
}
void PV4109_iic_close()
{
    dev_close(iic);
    iic = NULL;

}
s32 PV4109_DVP_check(u8 isp_dev, u32 reset_gpio, u32 pwdn_gpio)
{
    puts("\n\n PV4109_DVP_check \n\n");
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        if (!iic) {
            return -1;
        }
    } else {
        if (cur_sensor_type != isp_dev) {
            return -1;
        }
    }
    printf("\n\n isp_dev =%d\n\n", isp_dev);

    reset_gpios[isp_dev] = reset_gpio;
    pwdn_gpios[isp_dev] = pwdn_gpio;
    PV4109_DVP_reset(isp_dev);
    puts("PV4109_DVP_id_check\n");
    if (0 != PV4109_DVP_ID_check()) {
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;
}

s32 PV4109_DVP_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    printf("\n\n PV4109_DVP_init width:%d,height:%d,format:%d, frame_freq:%d\n\n", *width, *height, *format, *frame_freq);
    PV4109_DVP_config_SENSOR(width, height, format, frame_freq);
    return 0;
}

s32 PV4109_DVP_get_DVP_clk(u32 *DVP_clk,  u32 *tval_hstt, u32 *tval_stto, u16 width, u16 height, u8 frame_freq)
{
    *DVP_clk = 186;
    *tval_hstt = 0;
    *tval_stto = 30;
    return 0;
}


u32 PV4109_DVP_calc_shutter(isp_ae_shutter_t *shutter, u32 exp_time_us, u32 gain)
{
    u32 texp;
    u32 texp_align;
    u32 ratio;

    texp = exp_time_us * PCLK / LINE_LENGTH_CLK;


    if (texp > frameLen - 5) {
        texp = frameLen - 5;
    }
    texp_align = (texp) * LINE_LENGTH_CLK / (PCLK);

    if (texp_align < exp_time_us) {
        ratio = (exp_time_us) * (1 << 10) / texp_align;
        //printf("ratio = %d\n",ratio);
    } else {
        ratio = (1 << 10);
    }

    shutter->ae_exp_line =  texp;
    shutter->ae_gain = (gain * ratio) >> 10;
    shutter->ae_exp_clk = 0;

//    printf("exp_time_us=%d, texp=%d, gain=%d->%d\n", exp_time_us, texp, gain,shutter->ae_gain);
//    return 0;

    return 0;

}
#define MAX_AGAIN (64*1024)
#define MAX_DGAIN (8*1024)

static void calc_gain(u32 gain, u32 *_again, u32 *_dgain)
{
    int i;
    u32 Decimal;
    u32 reg0, reg1, reg2;
    u32 dgain, again, dcggain = 0;

    if (gain < 1024) {
        gain = 1024;
    }
    if (gain > 127 * 1024) {
        gain = 127 * 1024;
    }

    if (gain > MAX_AGAIN) {
        again = MAX_AGAIN;
        dgain = gain * 1024 / MAX_AGAIN;
        if (dgain > MAX_DGAIN) {
            dgain = MAX_DGAIN;
        }
    } else {
        again = gain;
        dgain = 1024;
    }

    reg0 = 0;
    while (again >= 2048) {
        again >>= 1;
        reg0 += 0x10;
    }
    again -= 1024;
    reg0 += (again / 64);

    *_again = reg0;
    reg0 = dgain >> 10;
    reg1 = (dgain & 0x3ff) >> 6;

    *_dgain = (reg0 << 4) + (reg1 & 0xf);

    //printf(" ag = 0x%x; dg= 0x%x; \n ",*_again,*_dgain);
}


static void set_shutter(u32 texp)
{
    u8 wval, rval;
    if (cur_expline == texp) {
        return;
    }
    cur_expline  = texp;

    wrPV4109_DVPReg(0x03, 01);
    wval = (u8)(texp >> 8);
    wrPV4109_DVPReg(0x6e, wval);
    wval = (u8)(texp >> 0);
    wrPV4109_DVPReg(0x6f, wval);
//    wval = (u8)texp;
//    wrPV4109_DVPReg(0x70, wval);
}
static void set_again(u32 again)
{
    if (cur_again == again) {
        return;
    }
    cur_again  = again;
    wrPV4109_DVPReg(0x71, (again >> 0) & 0xff);
}

static void set_dgain(u32 dgain)
{
    if (cur_dgain == dgain) {
        return;
    }
    cur_dgain  = dgain;

    wrPV4109_DVPReg(0x72, (dgain >> 0) & 0xff);
}
u32 PV4109_DVP_set_shutter(isp_ae_shutter_t *shutter)
{
    u32 gain;
    u32 again, dgain;

    gain = (shutter->ae_gain);
    calc_gain(gain, &again, &dgain);
    set_shutter(shutter->ae_exp_line);
    set_again(again);
    set_dgain(dgain);
    wrPV4109_DVPReg(0x7a, 0x01);
    return 0;
}

void PV4109_DVP_sleep()
{
    //wrPV4109_DVPReg(0x3e, 0x90);//close DVP
}

void PV4109_DVP_wakeup()
{

    //wrPV4109_DVPReg(0x3e, 0x91);// open DVP
}

void PV4109_DVP_W_Reg(u16 addr, u16 val)
{
    printf("update reg%x with %x\n", addr, val);
    wrPV4109_DVPReg((u8)addr, (u8)val);
}

u16 PV4109_DVP_R_Reg(u16 addr)
{
    u8 val;
    rdPV4109_DVPReg((u8)addr, &val);
    return val;
}
void PV4109_DVP_set_direction(u8 value)
{
    if (iic) {
        static u8 time = 0;
        if (!value) {
            wrPV4109_DVPReg(0x03, 0x00);
            wrPV4109_DVPReg(0x05, 0x00);
        } else {
            time = !time;
            if (time) {
                wrPV4109_DVPReg(0x03, 0x00);
                wrPV4109_DVPReg(0x05, 0x03);
            } else {
                wrPV4109_DVPReg(0x03, 0x00);
                wrPV4109_DVPReg(0x05, 0x00);
            }
        }
    }
}

REGISTER_CAMERA(PV4109_DVP) = {
    .logo 				= 	"PV4109",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_VYUY,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    SEN_MBUS_PCLK_SAMPLE_RISING | SEN_MBUS_VSYNC_ACTIVE_HIGH,
    .fps         		= 	25,

    .sen_size 			= 	{PV4109_DVP_OUTPUT_W, PV4109_DVP_OUTPUT_H},
    //.isp_size 			= 	{PV4109_DVP_OUTPUT_W, PV4109_DVP_OUTPUT_H},

    .cap_fps         		= 	25,
    .sen_cap_size 			= 	{PV4109_DVP_OUTPUT_W, PV4109_DVP_OUTPUT_H},
    //.isp_cap_size 			= 	{PV4109_DVP_OUTPUT_W, PV4109_DVP_OUTPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	PV4109_DVP_check,
        .init 		        = 	PV4109_DVP_init,
        .set_size_fps 		=	PV4109_DVP_set_output_size,
        //.get_DVP_clk       =   PV4109_DVP_get_DVP_clk,
        .power_ctrl         =   PV4109_DVP_power_ctl,

        //.get_ae_params  	=	PV4109_DVP_get_ae_params,
        //.get_awb_params 	=	PV4109_DVP_get_awb_params,
        //.get_iq_params 	    =	PV4109_DVP_get_iq_params,

        .sleep 		        = NULL, //	PV4109_DVP_sleep,
        .wakeup 		    = NULL, //	PV4109_DVP_wakeup,
        .write_reg 		    =	PV4109_DVP_W_Reg,
        .read_reg 		    =	PV4109_DVP_R_Reg,

    }
};



