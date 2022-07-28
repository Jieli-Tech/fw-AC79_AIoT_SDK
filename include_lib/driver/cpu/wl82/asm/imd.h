#ifndef __IMD_H__
#define __IMD_H__

#include "typedef.h"
#include "video/fb.h"
#include "asm/imr.h"
#include "device/device.h"
#include "asm/port_waked_up.h"
#include "os/os_api.h"

#define     FBIOGET_IMR_DOUBLE_BUFFER0  _IOR('A', 0, sizeof(int))
#define     FBIOGET_IMR_DOUBLE_BUFFER1  _IOR('A', 1, sizeof(int))
#define     FBIOSET_SLIDER_WINDOW		_IOR('A', 2, sizeof(int))
#define     FBIOGET_IMR_ROTATE_BUF		_IOR('A', 3, sizeof(int))
#define     FBIOSET_IMR_MODE            _IOW('A', 4, sizeof(int))

struct layer_attr {
    u8 imr_index; /* imr选择 */
    u8 buf_num;
};

struct fb_fh {
    struct fb_info fb_info;
    struct fb_map_user map;
    s8 ref_cnt;
    OS_SEM sem;
    u8 buf_num;
    u8 buf_state;
    u8 *buf_inused;
    u8 *buf_pending;
    u32 buf_addr[3];
    u32 buf_size;
    void *imr;
    struct imr_info imr_info_t;
    u8 *imr_buf0;
    u8 *imr_buf1;
};

enum {
    IMD_LAYER_OUTPUT_CAPTURE,
    IMD_LAYER_OUTPUT_DISPLAY,
};

extern const struct device_operations imd_dev_ops;
extern const struct device_operations mipi_dev_ops;

#define layer_for_fb_info(fb_info) \
	container_of(fb_info, struct imd_layer, fb)


#define IMD_SET_COLOR_CFG 			_IOW('F', 0, sizeof(int))
#define IMD_GET_COLOR_CFG 			_IOR('F', 0, sizeof(int))
#define IMD_SET_COLOR_VALID 		_IOW('F', 1, sizeof(int))
#define IMD_GET_COLOR_CFG_SIZE 		_IOR('F', 1, sizeof(int))
#define IMD_SET_DEFAULT_COLOR_CFG 	_IOW('F', 2, sizeof(int))
#define IMD_GET_FRAMERATE		 	_IOR('F', 2, sizeof(int))
#define IMD_SET_REFRESH_STATUS   	_IOW('F', 3, sizeof(int))
#define IMD_GET_SCREEN_INFO			_IOR('F', 3, sizeof(int))


#define IMD_DMM_CLK_EN 		(imd_lc_con |= BIT(0))
#define IMD_DMM_CLK_DIS 	(imd_lc_con &=~BIT(0))
#define IMD_DMM_RST_RELEASE (imd_lc_con |= BIT(1))
#define IMD_DMM_RST         (imd_lc_con &=~BIT(1))
#define IMD_AXI_LEN_256     (imd_lc_con |= BIT(2))
#define IMD_AXI_LEN_512		(imd_lc_con &=~BIT(2))
#define IMD_INTERLACE_EN    (imd_lc_con |= BIT(3))
#define IMD_INTERLACE_DIS   (imd_lc_con &=~BIT(3))
#define IMD_INTERLACE_ODD_STARTLINE1	(imd_lc_con |= BIT(4))
#define IMD_INTERLACE_ODD_STARTLINE0    (imd_lc_con &=~BIT(4))


#define IMD_LCD_EN 			(imd_dpi_con |= BIT(0))
#define IMD_LCD_DIS			(imd_dpi_con &=~BIT(0))
#define IMD_LCD_KICKSTART   (imd_dpi_con |= BIT(1))
#define IMD_SINGLE_FRAME    (imd_dpi_con |= BIT(2))
#define IMD_CONTINUE_FRAME  (imd_dpi_con &=~BIT(2))
#define IMD_LCD_IE_EN       (imd_dpi_con |= BIT(3))
#define IMD_LCD_IE_DIS  	(imd_dpi_con &=~BIT(3))
#define IMD_LCD_CLR_PND     (imd_dpi_con |= BIT(4))
#define IMD_LCD_PND			(imd_dpi_con & BIT(5))


#define DPI_MULTICYC_ONE		(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x03<<0)))
#define DPI_MULTICYC_TWO		(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x03<<0))|0x01)
#define DPI_MULTICYC_THREE      (imd_dpi_fmt |= (imd_dpi_fmt & ~(0x03<<0))|0x02)
#define DPI_MULTICYC_FOUR       (imd_dpi_fmt |= (imd_dpi_fmt & ~(0x03<<0))|0x03)
#define DPI_FMT_RGB333  		(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x3f<<2))|BIT(2))
#define DPI_FMT_RGB666			(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x3f<<2))|BIT(3))
#define DPI_FMT_RGB888			(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x3f<<2))|BIT(4))
#define DPI_FMT_RGB565			(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x3f<<2))|BIT(5))
#define DPI_FMT_YUV888			(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x3f<<2))|BIT(6))
#define DPI_FMT_YUV422			(imd_dpi_fmt |= (imd_dpi_fmt & ~(0x3f<<2))|BIT(7))
#define ITU656_EN				(imd_dpi_fmt |= BIT(8))
#define ITU656_DIS				(imd_dpi_fmt &=~BIT(8))
#define INTERLACE_TIMING_EN 	(imd_dpi_fmt |= BIT(9))
#define INTERLACE_TIMING_DIS 	(imd_dpi_fmt &=~BIT(9))
#define INTERLACE_DATA_EN		(imd_dpi_fmt |= BIT(10))
#define INTERLACE_DATA_DIS		(imd_dpi_fmt &=~BIT(10))
#define INTERLACE_TOP_MODE		(imd_dpi_fmt &=~BIT(11))
#define INTERLACE_BOTTOM_MODE	(imd_dpi_fmt |= BIT(11))
#define INTERLACE_FILED_EN		(imd_dpi_fmt |= BIT(12))
#define INTERLACE_FILED_DIS		(imd_dpi_fmt &=~BIT(12))
#define RAW_DLT					(imd_dpi_fmt |= BIT(13))//R->G->B
#define RAW_STR					(imd_dpi_fmt &=~BIT(13))//B->R->G
#define DUMMY_EN				(imd_dpi_fmt |= BIT(18))//R->G->B->dummy
#define DUMMY_DIS				(imd_dpi_fmt &=~BIT(18))
#define SIGNAL_DCLK_INV			(imd_dpi_fmt |= BIT(19))
#define SIGNAL_DCLK_NOR			(imd_dpi_fmt &=~BIT(19))
#define SIGNAL_SYNC0_INV		(imd_dpi_fmt |= BIT(20))
#define SIGNAL_SYNC0_NOR		(imd_dpi_fmt &=~BIT(20))
#define SIGNAL_SYNC1_INV		(imd_dpi_fmt |= BIT(21))
#define SIGNAL_SYNC1_NOR		(imd_dpi_fmt &=~BIT(21))
#define SIGNAL_SYNC2_INV		(imd_dpi_fmt |= BIT(22))
#define SIGNAL_SYNC2_NOR		(imd_dpi_fmt &=~BIT(22))
#define SIGNAL_SYNC3_INV		(imd_dpi_fmt |= BIT(23))
#define SIGNAL_SYNC3_NOR		(imd_dpi_fmt &=~BIT(23))

#define LCD_IO_CLK_EN			(imd_dpi_io_con |= BIT(0))
#define LCD_IO_CLK_DIS			(imd_dpi_io_con &=~BIT(0))
#define LCD_IO_SYNC0_EN			(imd_dpi_io_con |= BIT(1))
#define LCD_IO_SYNC0_DIS		(imd_dpi_io_con &=~BIT(1))
#define LCD_IO_SYNC1_EN			(imd_dpi_io_con |= BIT(2))
#define LCD_IO_SYNC1_DIS		(imd_dpi_io_con &=~BIT(2))
#define LCD_IO_SYNC2_EN			(imd_dpi_io_con |= BIT(3))
#define LCD_IO_SYNC2_DIS		(imd_dpi_io_con &=~BIT(3))

#define DPI_CLK_EN				(imd_dpi_clk_con |= BIT(0))
#define DPI_CLK_DIS				(imd_dpi_clk_con &=~BIT(0))
#define DPI_RESET_RELEASE		(imd_dpi_clk_con |= BIT(1))
#define DPI_RESET				(imd_dpi_clk_con &=~BIT(1))
#define DPI_PATTERN_EN			(imd_dpi_clk_con |= BIT(10))
#define DPI_PATTERN_DIS			(imd_dpi_clk_con &=~BIT(10))

#define DPI_EMI_EN				(imd_dpi_emi |= BIT(12))
#define DPI_EMI_DIS				(imd_dpi_emi &=~BIT(12))

#define IMD_MIX_PND_IE					(imd_mc_con |= BIT(2))
#define IMD_MIX_PND_DIS					(imd_mc_con &=~BIT(2))
#define IMD_MIX_BANDWIDTH_ERR_IE 		(imd_mc_con |= BIT(3))
#define IMD_MIX_BANDWIDTH_ERR_DIS 		(imd_mc_con &=~BIT(3))
#define IMD_CLR_MIX_PND					(imd_mc_con |= BIT(4))
#define IMD_CLR_MIX_BANDWIDTH_ERR_PND 	(imd_mc_con |= BIT(5))
#define IMD_MIX_PND						(imd_mc_con &  BIT(6))
#define IMD_MIX_BANDWIDTH_ERR_PND 		(imd_mc_con &  BIT(7))
#define IMD_LAYER3_LUT_BUSY				(imd_mc_con &  BIT(8))
#define IMD_LAYER2_LUT_BUSY				(imd_mc_con &  BIT(9))
#define IMD_GAMMA_B_LUT_BUSY			(imd_mc_con &  BIT(10))
#define IMD_GAMMA_G_LUT_BUSY			(imd_mc_con &  BIT(11))
#define IMD_GAMMA_R_LUT_BUSY			(imd_mc_con &  BIT(12))
#define IMD_RGB_MODE					(imd_mc_con &=~BIT(13))
#define IMD_YUV_MODE					(imd_mc_con |= BIT(13))
#define IMD_SEL_BT601					(imd_mc_con &=~BIT(14))
#define IMD_SEL_BT709					(imd_mc_con |= BIT(14))
#define IMD_COLOR_COMPRESS_DIS			(imd_mc_con &=~BIT(15))
#define IMD_COLOR_COMPRESS_EN			(imd_mc_con |= BIT(15))
#define IMD_MIX_BUSY					(imd_mc_con &  BIT(16))

#define IMD_SCA_BANDWIDTH_ERR_IE	(imd_sca_con |= BIT(0))
#define IMD_SCA_BANDWIDTH_ERR_DIS	(imd_sca_con &=~BIT(0))
#define IMD_CLR_BANDWIDTH_ERR_PND	(imd_sca_con |= BIT(1))
#define IMD_BANDWIDTH_ERR_PND		(imd_sca_con &  BIT(2))
#define IMD_TEST_EN					(imd_sca_con |= BIT(8))
#define IMD_TEST_DIS				(imd_sca_con &=~BIT(8))

//<时钟选择以及分频>
#define IMD_INTERNAL    	0x8000
#define IMD_EXTERNAL    	0x0000 //[默认,可省略] 外部PLL时钟目标频率为480MHz,需要通过分频得到所需的频率
//<晶振选择:IMD_INTERNAL模式有效>
#define XOSC0_12M				0x4000
#define XOSC1_12M				0x2000
#define OSC_32K				0x0000 //[默认,可省略]
#define OSC_12M				0x8000

/*
 * 注: IMD_EXTERNAL,OSC_32K可省略
 * IMD_EXTERNAL|PLL2_CLK|DIVA_1|DIVB_2|DIVC_3         等同于 PLL2_CLK|DIVA_1|DIVB_2|DIVC_3
 * IMD_INTERNAL|OSC_32K|PLL2_CLK|DIVA_1|DIVB_2|DIVC_3 等同于 IMD_INTERNAL|PLL2_CLK|DIVA_1|DIVB_2|DIVC_3
 */

#define PLL_SEL   			2
#define PLL_DIV1  			6
#define PLL_DIV2  			8

/*
 * @brief 时钟源选择
 */
#define PLL0_CLK        	(0<<PLL_SEL)
#define PLL1_CLK        	(1<<PLL_SEL)
#define PLL2_CLK        	(2<<PLL_SEL)
#define PLL3_CLK        	(3<<PLL_SEL)


/**
 * @brief  时钟分频系统
 */
#define DIVA_1          	(0<<PLL_DIV1)
#define DIVA_3         		(1<<PLL_DIV1)
#define DIVA_5          	(2<<PLL_DIV1)
#define DIVA_7          	(3<<PLL_DIV1)

#define DIVB_1          	(0<<PLL_DIV2)
#define DIVB_2          	(1<<PLL_DIV2)
#define DIVB_4          	(2<<PLL_DIV2)
#define DIVB_8          	(3<<PLL_DIV2)


//<时钟数>
enum NCYCLE {
    CYCLE_ONE,      //RGB-24BIT、MCU-16BIT
    CYCLE_TWO,      //MCU-8BIT
    CYCLE_THREE,    //RGB-8BIT
    CYCLE_FOUR,     //RGB-8BIT+dummy
};

//<输出数据格式>
enum OUT_FORMAT {
    FORMAT_RGB333,
    FORMAT_RGB666,
    FORMAT_RGB888,
    FORMAT_RGB565,
    FORMAT_YUV888,
    FORMAT_YUV422,
    FORMAT_ITU656, //YUV422同时使能
};


//<扫描方式>
enum INTERLACED {
    INTERLACED_NONE,		//非隔行
    INTERLACED_TIMING,		//时序隔行
    INTERLACED_DATA,		//数据隔行
    INTERLACED_ALL,			//(INTERLACED_DATA|INTERLACED_TIMING)时序数据隔行
};


//<位宽>
enum LCD_PORT {
    PORT_1BIT   = 0x00,
    PORT_3BITS  = 0x01,
    PORT_6BITS  = 0x03,
    PORT_8BITS  = 0x07,
    PORT_9BITS  = 0x0F,
    PORT_16BITS = 0x1F,
    PORT_18BITS = 0x3F,
    PORT_24BITS = 0x7F,
};


//<奇/偶行相位>
enum PHASE {
    PHASE_R,
    PHASE_G,
    PHASE_B,
};

//<RAW模式>
enum RAW_MODE {
    RAW_STR_MODE,//B->R->G(default)
    RAW_DLT_MODE,//R->G->B
};

//<模式>
enum LCD_MODE {
    MODE_RGB_SYNC,			// 无显存 连续帧 接HSYNC VSYNC信号
    MODE_RGB_DE_SYNC,		// 无显存 连续帧 按DE HSYNC VSYNC信号
    MODE_RGB_DE,			// 无显存 连续帧 接DE信号
    MODE_MCU,				// 有显存 单帧 SmartPanel 接DE信号
    MODE_AVOUT,				// AV OUT
    MODE_CCIR656,			// 嵌入同步信号(8BITS+DCLK、不需要HSYNC、VSYNC信号,00 FF为同步信号,传输数据不能为00 FF)
};

//<sync0/sync1/sync2信号使能/极性/类型>
#define CLK_DIS				0
#define CLK_POSITIVE        0
#define CLK_EN          	BIT(0)
#define CLK_NEGATIVE    	BIT(1)
#define SIGNAL_TYPE_DEN		BIT(2)
#define SIGNAL_TYPE_HSYNC	BIT(3)
#define SIGNAL_TYPE_VSYNC	BIT(4)
#define SIGNAL_TYPE_FIELD	BIT(5)

struct imd_layer_reg {
    volatile u32 con;
    volatile u32 hs;
    volatile u32 he;
    volatile u32 vs;
    volatile u32 ve;
    volatile u32 haw;
    volatile u32 htw;
    volatile u32 aph;
    volatile u32 baddr0a;
    volatile u32 baddr1a;
    volatile u32 baddr2a;
    volatile u32 baddr0b;
    volatile u32 baddr1b;
    volatile u32 baddr2b;
};

struct imd_layer {
    u8 enable;
    u8 index;
    u8 status;
    u8 alpha;
    u8 data_fmt;
    u8 rotate;
    u8 hori_mirror;
    u8 vert_mirror;
    u8 slide_window_en;
    //[图层窗口大小]
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    //[图层显存大小]
    u16 buf_x;
    u16 buf_y;
    u16 buf_width;
    u16 buf_height;
    u32 doublebuf0;
    u32 doublebuf1;
    u32 baddr;
    u32 baddr0;
    u32 baddr1;
    u32 baddr2;
    struct fb_info fb;
    struct fb_map_user map;
    // u32 reserved;
    void *imr;
    struct imr_info imr_info_t;
    struct imd_layer_reg *reg;
    u8 reg_index;
};

/**
 * @brief  颜色校正
 */

//二级命令										参数说明
#define ADJUST_EN       0x31//使能调试          不带参数
#define ADJUST_DIS      0x32//禁止调试			不带参数
#define GET_ALL         0x33//					struct color_correct
#define SET_ALL         0x34//					struct color_correct

#define SET_Y_GAIN      0x01//					u8
#define SET_U_GAIN      0x02//					u8
#define SET_V_GAIN      0x03//					u8
#define SET_Y_OFFS      0x04//					s8
#define SET_U_OFFS      0x05//					s8
#define SET_V_OFFS      0x06//					s8
#define SET_R_GAIN      0x07//					u8
#define SET_G_GAIN      0x08//					u8
#define SET_B_GAIN      0x09//					u8
#define SET_R_OFFS      0x0a//					s8
#define SET_G_OFFS      0x0b//					s8
#define SET_B_OFFS      0x0c//					s8
#define SET_R_COE0      0x0d//					u8
#define SET_R_COE1      0x0e//					s8
#define SET_R_COE2      0x0f//					s8
#define SET_G_COE0      0x10//					s8
#define SET_G_COE1      0x11//					u8
#define SET_G_COE2      0x12//					s8
#define SET_B_COE0      0x13//					s8
#define SET_B_COE1      0x14//					s8
#define SET_B_COE2      0x15//					u8
#define SET_R_GMA       0x16//					u16
#define SET_G_GMA       0x17//					u16
#define SET_B_GMA       0x18//					u16
#define SET_R_GMA_TBL   0x19//					256 bytes
#define SET_G_GMA_TBL   0x1a//					256 bytes
#define SET_B_GMA_TBL   0x1b//					256 bytes
#define SET_ISP_SCENE   0x1c//					s8
#define SET_CAMERA   	0x1d//					u8


#define FORE_CAMERA		0x01 /*前视摄像头*/
#define BACK_CAMERA 	0x02 /*后视摄像头*/
#define BOTH_CAMERA		0x03 /*前后视摄像头*/


struct color_correct {
    u16 y_gain;
    u16 u_gain;
    u16 v_gain;
    s16 y_offs;
    s16 u_offs;
    s16 v_offs;

    u16 r_gain;
    u16 g_gain;
    u16 b_gain;
    s16 r_offs;
    s16 g_offs;
    s16 b_offs;

    u16 r_coe0;
    s16 r_coe1;
    s16 r_coe2;

    s16 g_coe0;
    u16 g_coe1;
    s16 g_coe2;

    s16 b_coe0;
    s16 b_coe1;
    u16 b_coe2;

    // u8 rev;			//结构体对齐

    u16 r_gma;
    u16 g_gma;
    u16 b_gma;

    u8 r_gamma_tab[256];
    u8 g_gamma_tab[256];
    u8 b_gamma_tab[256];
};

struct color_effect_cfg {
    const char *fname;
    struct color_correct *adj;
};


enum LEN_CFG {
    LEN_256,
    LEN_512,
};

enum ODD_EVEN_FILED {
    SAME_FILED,
    ODD_FILED,
    EVEN_FILED,
};

enum ROTATE {
    ROTATE_0,	/*不旋转*/
    ROTATE_90,	/*旋转90度*/
    ROTATE_270, /*旋转270度*/
};

struct imd_dmm_info {
    u16 xres;					// 显存水平分辨率
    u16 yres;					// 显存垂直分辨率
    u16 target_xres;			// 放大后的水平分辨率
    u16 target_yres;			// 放大后的垂直分辨率
    u8  test_mode;				// 测试模式(纯色)使能
    u8  layer_buf_num;          // 图层显存数量
    u32 test_mode_color;		// 测试模式颜色设置
    u32 background_color;		// 图层背景层颜色
    enum OUT_FORMAT format;     // 输出数据格式
    enum INTERLACED interlaced_mode;         	// 隔行模式
    enum ODD_EVEN_FILED interlaced_1st_filed;  	// 首场是奇场or偶场,INTERLACED_DATA 时有效
    struct color_correct adjust;// 颜色校正参数
    enum LEN_CFG len;			// 访问瞬时长度
    enum ROTATE rotate;			// 屏幕旋转
};

/*
 * 硬件时序
 */
struct lcd_timing {
    u16 hori_total; 			/*水平时钟总数(Horizontal Line/HSD period)*/
    u16 hori_sync;				/*水平同步时钟(HSYNC pulse width/HSD pulse width)*/
    u16 hori_back_porth;		/*水平起始时钟(HSYNC blanking/HSD back porth)*/
    u16 hori_pixel;				/*水平像素(Horizontal display area/HSD display period)*/

    u16 vert_total;				/*垂直同步总数(Vertical display area/VSD period time)*/
    u16 vert_sync;				/*垂直同步时钟(VSYNC pulse width)*/
    u16 vert_back_porth_odd;	/*垂直起始时钟(VSYNC Blanking Odd field/VSD back porch Odd field)*/
    u16 vert_back_porth_even;	/*垂直起始时钟(隔行模式)(VSYNC Blanking Even field/VSD back porch Even field)*/
    u16 vert_pixel;				/*垂直像素(Vertical display area)*/
};

//<lcd port select>
enum LCD_GROUP {
    PORT_GROUP_A,
    PORT_GROUP_B,
};

#define IMD_LAYER_NUM 6

/*
 *  图层分配说明
 *  0:背景
 *  1:fb1(0/1)
 *  2:fb1(0/1)
 *  3:fb0(2/3)
 *  4:fb0(2/3)
 *  5:fb2(0)
 * */

struct te_mode_ctrl {
    u8 te_mode_en;
    PORT_EVENT_E event;
    PORT_EDGE_E edge;
    unsigned int gpio;
};

struct imd_dev {
    struct imd_dmm_info info;
    enum LCD_MODE drive_mode;   // 驱动模式
    enum NCYCLE ncycle;         // 每像素时钟数
    enum PHASE raw_odd_phase;	// 奇行相位
    enum PHASE raw_even_phase;  // 偶行相位
    enum RAW_MODE raw_mode;     // RAW模式选择
    enum LCD_PORT data_width;   // 数据位宽
    u8 avout_mode;				// AVOUT制式(PAL/NTSC/TESTMODE)
    u8 dclk_cfg;                // PH2 dclk使能以及极性配置
    u8 sync0_cfg;               // PH3 (DE/HSYNC/VSYNC)
    u8 sync1_cfg;               // PH4 (DE/HSYNC/VSYNC)
    u8 sync2_cfg;               // PH5 (DE/HSYNC/VSYNC)
    u8 sync3_cfg;               // Reversed
    u32 clk_cfg;                // clk时钟分频
    u16 pll0_nf;				// 选IMD_INTERNAL时配置
    u16 pll0_nr;				// 选IMD_INTERNAL时配置
    struct lcd_timing timing;   // 时序参数
    struct te_mode_ctrl te_mode;
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                LCD设备驱动相关                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
 * 屏接口类型
 */
enum LCD_IF {
    LCD_MIPI,
    LCD_DVP_MCU,
    LCD_DVP_RGB,
};

enum {
    BL_CTRL_BACKLIGHT_AND_CONTROLER,
    BL_CTRL_BACKLIGHT,
};

struct lcd_dev_drive {
    const char *logo;
    u8 enable;
    enum LCD_IF type;
    int (*init)(void *_data);
    int (*uninit)(void *_data);
    void (*bl_ctrl)(void *_data, u8 onoff);
    u8 bl_ctrl_flags;
    void (*setxy)(int x0, int x1, int y0, int y1);
    void *dev;
    u8 bl_sta;//背光状态
    u8 te_mode_dbug;
};

#define REGISTER_IMD_DEVICE(dev) \
	static struct imd_dev dev SEC_USED(.lcd_device)

#define REGISTER_MIPI_DEVICE(dev) \
	static struct mipi_dev dev SEC_USED(.lcd_device)

#define REGISTER_LCD_DEVICE_DRIVE(dev) \
	static const struct lcd_dev_drive dev##_drive SEC_USED(.lcd_device_drive)

extern struct lcd_dev_drive lcd_device_drive_begin[];
extern struct lcd_dev_drive lcd_device_drive_end[];

#define list_for_each_lcd_device_drive(p) \
	for (p=lcd_device_drive_begin; p < lcd_device_drive_end; p++)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                  MIPI屏相关                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/**
 * @Synopsis  lane重映射
 */
struct dsi_lane_mapping {
    u8 x0_lane;
    u8 x1_lane;
    u8 x2_lane;
    u8 x3_lane;
    u8 x4_lane;
};
/*
 * 视频/命令模式
 */
enum VIDEO_STREAM_MODE {
    VIDEO_STREAM_COMMAND,   //命令模式
    VIDEO_STREAM_VIDEO,		//视频模式
};
/*
 * 同步事件
 */
enum SYNC_MODE {
    SYNC_PULSE_MODE,
    SYNC_EVENT_MODE,
    BURST_MODE,
};

enum COLOR_FORMAT {
    COLOR_FORMAT_RGB565,
    COLOR_FORMAT_RGB666,
    COLOR_FORMAT_RGB666_LOOSELY,
    COLOR_FORMAT_RGB888,
};
/*
 *  数据包类型
 */
enum PIXEL_TYPE {
    PIXEL_RGB565_COMMAND,
    PIXEL_RGB565_VIDEO,
    PIXEL_RGB666,
    PIXEL_RGB666_LOOSELY,
    PIXEL_RGB888,
};
/**
 * @Synopsis  时序
 */
struct dsi_video_timing {
    u8 video_mode;			//command/video mode
    u8 sync_mode;
    u8 color_mode;
    u8 virtual_ch;		    //virtual channel(video mode)
    u8 hs_eotp_en;			//enable EoT packet
    u8 pixel_type;

    u16 dsi_vdo_vsa_v;		//vertical sync pluse
    u16 dsi_vdo_vbp_v;		//vertical back porch
    u16 dsi_vdo_vact_v; 	//vertical active line
    u16 dsi_vdo_vfp_v;		//vertical front porch

    u16 dsi_vdo_hsa_v;		//horizontal sync pulse
    u16 dsi_vdo_hbp_v;		//horizontal back porch
    u16 dsi_vdo_hact_v;		//horizontal active pixel
    u16 dsi_vdo_hfp_v;		//horizontal front porch

    u16 dsi_vdo_bllp0_v;
    u16 dsi_vdo_bllp1_v;
};
/**
 * @Synopsis  dsi时序设置
 */
struct dsi_timing {
    u16 freq;			// mipi pll总线频率
    u16 tval_lpx;		// LowPower state period 传输时间
    u32 tval_wkup;      // 总线从ulps模式唤醒的时间
    u16 tval_c_pre;		// clk lane HS transition prepare time
    u16 tval_c_sot;     // clk lane HS transition sot time:LP-00
    u16 tval_c_eot;     // clk lane HS transition eot time
    u16 tval_c_brk;		// clk lane总线离开回到LP-11的时间
    u16 tval_d_pre;		// data lane HS transition prepare time
    u16 tval_d_sot;		// data lane HS transition sot time:LP-00
    u16 tval_d_eot;		// data lane HS transition eot time
    u16 tval_d_brk;		// data lane总线离开回到LP-11的时间
    u16 tval_c_rdy;
};
/*
 * MIPI PLL时钟分频
 */
enum MIPI_PLL_DIV {
    MIPI_PLL_DIV1,
    MIPI_PLL_DIV2,
    MIPI_PLL_DIV4,
    MIPI_PLL_DIV8,
};

/*
 * 包类型
 */
enum PACKET_TYPE {
    PACKET_DCS,		 			/* DCS包 */
    PACKET_DCS_WITHOUT_BTA,		/* DCS包不带响应 */
    PACKET_GENERIC,  			/* 通用包 */
    PACKET_GENERIC_WITHOUT_BTA, /* 通用包不带响应 */
};
struct mipi_dev {
    struct imd_dmm_info info;
    struct dsi_lane_mapping lane_mapping;
    struct dsi_video_timing video_timing;
    struct dsi_timing timing;
    struct te_mode_ctrl te_mode;

    unsigned int reset_gpio;
    void (*lcd_reset)(unsigned int reset_gpio);
    void (*lcd_reset_release)(unsigned int reset_gpio);

    const u8 *cmd_list;
    u16 cmd_list_item;

    u8 debug_mode;								//调试使能
    /* 配置PLL频率的最佳范围为600MHz~1.2GHz,少于600MHz的频率通过二分频获得 */
    u16 pll_freq;							    //目标频率(MHz)
    enum MIPI_PLL_DIV  pll_division;			//分频
};

void imd_start();
int imd_dev_init();
int _imd_dmm_init();
int imd_get_rotate();

#endif

