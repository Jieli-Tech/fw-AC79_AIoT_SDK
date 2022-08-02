#ifndef _IMD_H_
#define _IMD_H_

#include "typedef.h"
#include "asm/cpu.h"
#include "os/os_api.h"
#include "device/device.h"

#define IOCTL_IMD_SET_CB_FUNC   		_IOW('M', 1, 4)//注册帧回调函数
#define IOCTL_IMD_SET_CB_PRIV   		_IOW('M', 2, 4)//注册帧回调函数参数
#define IOCTL_IMD_GET_FRAME_BUFF		_IOW('M', 3, 4)//获取IMD的可用缓存帧buff
#define IOCTL_IMD_GET_FRAME_BUFF_SIZE	_IOW('M', 4, 4)//获取IMD的可用缓存帧buff大小
#define IOCTL_IMD_UPDATE_BUFF			_IOW('M', 5, 4)//更新缓冲区数据


#define IMD_LD_PND 			BIT(31)
#define IMD_LD_PND_CLR 		BIT(30)
#define IMD_LD_PND_IE 		BIT(29)
#define IMD_MCH_PND 		BIT(28)
#define IMD_MCH_PND_CLR 	BIT(27)
#define IMD_MCH_PND_IE 		BIT(26)
#define IMD_EMPY_PND 		BIT(25)
#define IMD_EMPY_PND_CLR 	BIT(24)
#define IMD_EMPY_IE 		BIT(23)
#define IMD_RESET_PND 		BIT(22)
#define IMD_RESET_PND_CLR 	BIT(21)
#define IMD_RESET_IE 		BIT(20)
#define IMD_RGB_PND 		BIT(31)
#define IMD_RGB_PND_CLR 	BIT(30)
#define IMD_RGB_PND_IE 		BIT(29)

#define IMD_LD_KSTART 		BIT(1)
#define IMD_RGB_KSTART 		BIT(1)

enum imd_port {
    IMD_IO_GROUPA = 0,
    IMD_IO_GROUPC,
};

//IMD_PLL_DIV28 一帧数据28ms
//IMD_PLL_DIV24 一帧数据24ms
//IMD_PLL_DIV20 一帧数据20ms
//依次类推 DIV后面数值就是一帧数据的时间 帧数为 1000ms/28ms = 35帧
enum imd_pll_clk_div {
    IMD_PLL_DIV1 = 0,
    IMD_PLL_DIV2,
    IMD_PLL_DIV3,
    IMD_PLL_DIV4,
    IMD_PLL_DIV5,
    IMD_PLL_DIV6,
    IMD_PLL_DIV7,
    IMD_PLL_DIV8,
    IMD_PLL_DIV10,
    IMD_PLL_DIV12,
    IMD_PLL_DIV14,
    IMD_PLL_DIV20,
    IMD_PLL_DIV24,
    IMD_PLL_DIV28,
    IMD_PLL_DIV40,
    IMD_PLL_DIV56,
};
enum imd_rgb_format {
    IMD_RGB_888 = 0,
    IMD_RGB_666,
    IMD_RGB_565,
};
enum imd_sync {
    IMD_SYNC_DIS = 0,//关闭sync输出
    IMD_SYNC_EN  = BIT(2),//使能SYN输出
    IMD_HSYN_EN  = BIT(2),//使能HSYN输出
    IMD_VSYN_EN  = BIT(2) | BIT(0), //使能VSYN输出
    IMD_DE_EN  = BIT(2) | BIT(1), //使能DE输出
    IMD_SYN_EDGE_H  = BIT(3),//SYNC(HSYNC/VSYNC/DE)有效电平
    IMD_SYN_EDGE_L  = BIT(4),//SYNC(HSYNC/VSYNC/DE)有效电平
};
enum imd_data_out_mode {//输出输出大小端
    IMD_DATA_MSB = 0,
    IMD_DATA_LSB,
};
enum imd_clk_edge {
    IMD_CLK_UPDATE_L = 0,
    IMD_CLK_UPDATE_H = 1,
};
/*
 1、vsync_forward(vf),vsync_behind(vb),vsync_time(vt)帧同步场时间配置(单位:行时间个数):
例如下面时序:vsync_forward = 2,vsync_behind = 3,vsync_time = 2;
帧VSYNC:
               ________________________________________           ____________________________________
         _____|                                        |_________|                                    |_____

行HSYNC:
           _   _   _   _   _   _          _   _   _   _   _   _   _   _   _   _          _   _   _   _   _
         _| |_| |_| |_| |_| |_| |_......_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_......_| |_| |_| |_| |_| |_

行有效DE:
                           _   _          _   _                               _          _   _
         _________________| |_| |_......_| |_| |_____________________________| |_......_| |_| |______________

                          |                    |       |         |           |
                          |-->    height    <--|-> vf<-|-> vt  <-|->  vb   <-|
                          |                    |       |         |           |


 2、hsync_forward(hf),hsync_behind(hb),hsync_time(ht)行同步场时间配置(单位:时钟个数):
例如下面时序:hsync_forward = 2,hsync_behind = 3,hsync_time = 2;
像素点时钟PCLK:
           _   _   _   _   _   _          _   _   _   _   _   _   _   _   _   _          _   _   _   _   _
         _| |_| |_| |_| |_| |_| |_......_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_......_| |_| |_| |_| |_| |_

行HSYNC:
               ________________________________________           ____________________________________
         _____|                                        |_________|                                    |_____

行有效DE:
                           ____________________                               _________________
         _________________|                    |_____________________________|                 |____________

                          |                    |       |         |           |
                          |--> width*3byte  <--|-> hf<-|-> ht  <-|->  hb   <-|
                          |                    |       |         |           |


*/
struct imd_rgb_platform_data {
    u8 test_mode;//IMD打开显示test_color颜色_
    u8 group_port;//IMD_IO_GROUPA/IMD_IO_GROUPC
    u8 irq_en;//true/false
    u8 req_buf_waite;//底层buff更新是否需要等待在中断更新：小分辨率等待，大分辨率不等带
    u8 data_out_mode;//enum imd_data_out_mode 输出输出大小端
    u8 data_format;//enum imd_rgb_format
    u8 pll_clk_div;//enum imd_pll_clk_div
    u8 clk_edge;//IMD_CLK_UPDATE_H/IMD_CLK_UPDATE_L
    u8 de_edge;//IMD_DE_H/IMD_DE_L
    u8 hsync_edge;//IMD_HSYN_H/IMD_HSYN_L
    u8 vsync_edge;//IMD_VSYN_H/IMD_VSYN_L
    u8 data_shift_en;//RGB666数据右移位2位(先右移在按照MSB/LSB输出)
    u8 double_buffer;//底层使用双buffer切换
    u16 hsync_forward;//行前沿宽度周期个数
    u16 hsync_behind;//行后沿宽度周期个数
    u16 hsync_time;//行同步脉冲宽度周期
    u16 vsync_forward;//帧前沿宽度周期个数
    u16 vsync_behind;//帧后沿宽度周期个数
    u16 vsync_time;//帧同步脉冲宽度周期
    u16 width;
    u16 height;
    u16 load_height;
    u32 test_color;//初始化颜色
    u32 sync0;//enum imd_sync
    u32 sync1;//enum imd_sync
    u32 sync2;//enum imd_sync
};
typedef struct imd_rgb {
    struct imd_rgb_platform_data info;
    struct device dev;
    u8 init;
    u8 in_irq;
    u8 start;
    u8 data_shift_en;//rgb666移位
    u8 soft_kst;
    u8 double_buffer;//底层使用双buffer切换
    u8 req_buf_waite;//是否需要等待imd中断更新数据，小分辨率需要等待，大分辨率不需要
    volatile u8 req_upate;
    volatile u8 upating;
    OS_SEM sem;
    OS_MUTEX mtx;
    u16 load_height;//IMD每次load数据行个数
    u8 *frame_addr;
    u8 *frame_buf;
    u32 fps_cnt;
    u32 frame_size;
    u32 one_line_bytes;
    void *irq_cb_priv;
    void (*irq_cb)(void *priv);
} IMD_RGB;

#define IMD_RGB_PLATFORM_DATA_BEGIN(rgb) \
	static const struct imd_rgb_platform_data rgb = {


#define IMD_RGB_PLATFORM_DATA_END() \
	};

extern const struct device_operations imd_dev_ops;

#endif
