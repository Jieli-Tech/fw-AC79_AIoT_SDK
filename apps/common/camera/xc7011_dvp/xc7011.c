#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gpio.h"
#include "xc7011.h"
#include "system/timer.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"

#define FLIP_XC7011 1//1:翻转 0:不翻转
//#define MIRROR_XC7011 1//1:镜像 0:不翻转
#define CONFIG_INPUT_FPS 20

#if (CONFIG_VIDEO_IMAGE_W > 1280)
#define XC7011_DEVP_INPUT_W	1280
#define XC7011_DEVP_INPUT_H	720
#else
#define XC7011_DEVP_INPUT_W CONFIG_VIDEO_IMAGE_W
#define XC7011_DEVP_INPUT_H	CONFIG_VIDEO_IMAGE_H
#endif

#define  XC7011_WRCMD  0x36
#define  XC7011_RDCMD  0x37
#define  H42_WRCMD  0x60
#define  H42_RDCMD  0x61
#define  WRCMD  0x60
#define  RDCMD  0x61

#define DELAY_TIME	10
static u8 reset_gpio[2] = {-1, -1};
static u8 pwdn_gpios[2] = {-1, -1};
static void *iic = NULL;
typedef struct {
    u16 addr;
    u8 value;
} Sensor_reg_ini;

typedef struct {
    u8 addr;
    u8 value;
} Sensor_reg_inix;
/*
这个驱动针对XC7100 + XC7011，可出图，客户需要针对优化：
1、联系原厂，提高PCLK高电平或者波峰需要超过2v(是否可以可对摄像头1.8v电源提高到2.5v？)
2、联系原厂，对输出行周期进行缩短，对输出行结束到帧同步结束的时间进行缩短（行结束之后的4-10行周期就有帧同步结束信号）
3、联系原厂，绘本功能的15帧帧率即可
 */
unsigned char wr_XC7011_Reg(u16 regID, u8 regDat)
{
    u8 ret = 1;

    dev_ioctl(iic, IIC_IOCTL_START, 0);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, WRCMD)) {
        ret = 0;
        goto __wend;
    }

    delay(DELAY_TIME);

    if (dev_ioctl(iic, IIC_IOCTL_TX, regID >> 8)) {
        ret = 0;
        goto __wend;
    }

    delay(DELAY_TIME);

    if (dev_ioctl(iic, IIC_IOCTL_TX, regID & 0xff)) {
        ret = 0;
        goto __wend;
    }

    delay(DELAY_TIME);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regDat)) {
        ret = 0;
        goto __wend;
    }

__wend:

    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;

}

unsigned char rd_XC7011_Reg(u16 regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, WRCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(DELAY_TIME);

    if (dev_ioctl(iic, IIC_IOCTL_TX, regID >> 8)) {
        ret = 0;
        goto __rend;
    }

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regID & 0xff)) {
        ret = 0;
        goto __rend;
    }

    delay(DELAY_TIME);

    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, RDCMD)) {
        ret = 0;
        goto __rend;
    }

    delay(DELAY_TIME);

    dev_ioctl(iic, IIC_IOCTL_RX_WITH_STOP_BIT, (u32)regDat);

__rend:

    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}

void XC7011_dev_wr_reg(u16 regID, u16 regDat)
{
    wr_XC7011_Reg(regID, (u8)regDat);
}
u16 XC7011_dev_rd_reg(u16 regID)
{
    u8 reg;
    rd_XC7011_Reg(regID, &reg);
    return reg;
}

void XC7011_Init(void)
{
//printf("***1****XC7011_Init\n");
    wr_XC7011_Reg(0xfffd, 0x80);
//printf("***2****XC7011_Init\n");
    wr_XC7011_Reg(0xfffd, 0x80);

    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x001c, 0xff);
    wr_XC7011_Reg(0x001d, 0xff);
    wr_XC7011_Reg(0x001e, 0xff);
    wr_XC7011_Reg(0x001f, 0xef); //clk en
    wr_XC7011_Reg(0x0018, 0x00);
    wr_XC7011_Reg(0x0019, 0x00);
    wr_XC7011_Reg(0x001a, 0x00);
    wr_XC7011_Reg(0x001b, 0x00);  // reset
    wr_XC7011_Reg(0x00bc, 0x11);
    wr_XC7011_Reg(0x00bd, 0x00);
    wr_XC7011_Reg(0x00be, 0x00);
    wr_XC7011_Reg(0x00bf, 0x00);

    wr_XC7011_Reg(0x0030, 0x5);
    wr_XC7011_Reg(0x0031, 0x4);
    wr_XC7011_Reg(0x0032, 0x10); //0x0b
    wr_XC7011_Reg(0x0020, 0x1);
    wr_XC7011_Reg(0x0021, 0xE);
    wr_XC7011_Reg(0x0023, 0x2);
    wr_XC7011_Reg(0x0024, 0x6);
    wr_XC7011_Reg(0x0025, 0x6);
    wr_XC7011_Reg(0x0026, 0x1);
    wr_XC7011_Reg(0x0027, 0x6);
    wr_XC7011_Reg(0x0028, 0x8);
    wr_XC7011_Reg(0x0029, 0x6);
    wr_XC7011_Reg(0x0029, 0x00);
    wr_XC7011_Reg(0x00aa, 0x40);

    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x00bc, 0x11);
    wr_XC7011_Reg(0x001b, 0x0);
    wr_XC7011_Reg(0x0090, 0x29);

    wr_XC7011_Reg(0xfffe, 0x20);
    wr_XC7011_Reg(0x0000, 0x00);
    wr_XC7011_Reg(0x0004, 0x5);
    wr_XC7011_Reg(0x0005, 0x0);
    wr_XC7011_Reg(0x0006, 0x2);
    wr_XC7011_Reg(0x0007, 0xD0);

    wr_XC7011_Reg(0xfffe, 0x26);
    wr_XC7011_Reg(0x4000, 0xF9);
    wr_XC7011_Reg(0x6001, 0x14);
    wr_XC7011_Reg(0x6005, 0xc4);
    wr_XC7011_Reg(0x6006, 0xA);
    wr_XC7011_Reg(0x6007, 0x8C);
    wr_XC7011_Reg(0x6008, 0x9);
    wr_XC7011_Reg(0x6009, 0xFC);

    wr_XC7011_Reg(0x8000, 0x3f);
    wr_XC7011_Reg(0x8001, 0x0);
    wr_XC7011_Reg(0x8002, 0x5);
    wr_XC7011_Reg(0x8003, 0xD0);
    wr_XC7011_Reg(0x8004, 0x2);
    wr_XC7011_Reg(0x8005, 0x3);
    wr_XC7011_Reg(0x8006, 0x5);
    wr_XC7011_Reg(0x8007, 0x99);
    wr_XC7011_Reg(0x8008, 0x14);

    wr_XC7011_Reg(0x8010, 0x4);
    wr_XC7011_Reg(0x8012, 0x0);
    wr_XC7011_Reg(0x8013, 0x5);
    wr_XC7011_Reg(0x8014, 0xD0);
    wr_XC7011_Reg(0x8015, 0x2);
    wr_XC7011_Reg(0x8016, 0x0);
    wr_XC7011_Reg(0x8017, 0x0);
    wr_XC7011_Reg(0x8018, 0x0);
    wr_XC7011_Reg(0x8019, 0x0);

    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0001, 0x1);
    wr_XC7011_Reg(0x0004, 0x18);
    wr_XC7011_Reg(0x0006, 0x5);
    wr_XC7011_Reg(0x0007, 0x0);
    wr_XC7011_Reg(0x0008, 0x2);
    wr_XC7011_Reg(0x0009, 0xD0);
    wr_XC7011_Reg(0x000a, 0x5);
    wr_XC7011_Reg(0x000b, 0x0);
    wr_XC7011_Reg(0x000c, 0x2);
    wr_XC7011_Reg(0x000d, 0xD0);
    wr_XC7011_Reg(0x0027, 0xF1);
    wr_XC7011_Reg(0x005e, 0x4);
    wr_XC7011_Reg(0x005f, 0xFF);
    wr_XC7011_Reg(0x0060, 0x2);
    wr_XC7011_Reg(0x0061, 0xCF);
    wr_XC7011_Reg(0x1908, 0x0);
    wr_XC7011_Reg(0x1900, 0x0);
    wr_XC7011_Reg(0x1901, 0x0);
    wr_XC7011_Reg(0x1902, 0x0);
    wr_XC7011_Reg(0x1903, 0x0);
    wr_XC7011_Reg(0x1904, 0x5);
    wr_XC7011_Reg(0x1905, 0x0);
    wr_XC7011_Reg(0x1906, 0x2);
    wr_XC7011_Reg(0x1907, 0xD0);

    wr_XC7011_Reg(0xfffe, 0x25);
    wr_XC7011_Reg(0x0002, 0xf0);

//patch start
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x000e, 0x54);

    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x0006, 0x00);
    wr_XC7011_Reg(0x0007, 0x1e);
    wr_XC7011_Reg(0x0014, 0x00);
    wr_XC7011_Reg(0x0015, 0x14);
    wr_XC7011_Reg(0x0016, 0x06);
    wr_XC7011_Reg(0x0017, 0x58);
    wr_XC7011_Reg(0x03d4, 0x9c);
    wr_XC7011_Reg(0x03d5, 0x21);
    wr_XC7011_Reg(0x03d6, 0xff);
    wr_XC7011_Reg(0x03d7, 0xe8);
    wr_XC7011_Reg(0x03d8, 0xd4);
    wr_XC7011_Reg(0x03d9, 0x01);
    wr_XC7011_Reg(0x03da, 0x48);
    wr_XC7011_Reg(0x03dc, 0xd4);
    wr_XC7011_Reg(0x03dd, 0x01);
    wr_XC7011_Reg(0x03de, 0x50);
    wr_XC7011_Reg(0x03df, 0x04);
    wr_XC7011_Reg(0x03e0, 0xd4);
    wr_XC7011_Reg(0x03e1, 0x01);
    wr_XC7011_Reg(0x03e2, 0x60);
    wr_XC7011_Reg(0x03e3, 0x08);
    wr_XC7011_Reg(0x03e4, 0xd4);
    wr_XC7011_Reg(0x03e5, 0x01);
    wr_XC7011_Reg(0x03e6, 0x70);
    wr_XC7011_Reg(0x03e7, 0x0c);
    wr_XC7011_Reg(0x03e8, 0xd4);
    wr_XC7011_Reg(0x03e9, 0x01);
    wr_XC7011_Reg(0x03ea, 0x80);
    wr_XC7011_Reg(0x03eb, 0x10);
    wr_XC7011_Reg(0x03ec, 0xd4);
    wr_XC7011_Reg(0x03ed, 0x01);
    wr_XC7011_Reg(0x03ee, 0x90);
    wr_XC7011_Reg(0x03ef, 0x14);
    wr_XC7011_Reg(0x03f0, 0x1a);
    wr_XC7011_Reg(0x03f3, 0x14);
    wr_XC7011_Reg(0x03f4, 0xaa);
    wr_XC7011_Reg(0x03f5, 0x10);
    wr_XC7011_Reg(0x03f6, 0x02);
    wr_XC7011_Reg(0x03f7, 0xb0);
    wr_XC7011_Reg(0x03f8, 0x84);
    wr_XC7011_Reg(0x03f9, 0x70);
    wr_XC7011_Reg(0x03fc, 0x84);
    wr_XC7011_Reg(0x03fd, 0x63);
    wr_XC7011_Reg(0x03ff, 0xd0);
    wr_XC7011_Reg(0x0400, 0xb9);
    wr_XC7011_Reg(0x0401, 0x43);
    wr_XC7011_Reg(0x0403, 0x41);
    wr_XC7011_Reg(0x0404, 0xbc);
    wr_XC7011_Reg(0x0405, 0x4a);
    wr_XC7011_Reg(0x0407, 0x1f);
    wr_XC7011_Reg(0x0408, 0x10);
    wr_XC7011_Reg(0x040b, 0x38);
    wr_XC7011_Reg(0x040c, 0xbc);
    wr_XC7011_Reg(0x040d, 0x4a);
    wr_XC7011_Reg(0x040f, 0x3e);
    wr_XC7011_Reg(0x0410, 0x9d);
    wr_XC7011_Reg(0x0411, 0xc0);
    wr_XC7011_Reg(0x0414, 0xa5);
    wr_XC7011_Reg(0x0415, 0x8a);
    wr_XC7011_Reg(0x0416, 0xff);
    wr_XC7011_Reg(0x0417, 0xff);
    wr_XC7011_Reg(0x0418, 0xaa);
    wr_XC7011_Reg(0x0419, 0x4e);
    wr_XC7011_Reg(0x041c, 0x84);
    wr_XC7011_Reg(0x041d, 0x70);
    wr_XC7011_Reg(0x0420, 0xb8);
    wr_XC7011_Reg(0x0421, 0x92);
    wr_XC7011_Reg(0x0423, 0x43);
    wr_XC7011_Reg(0x0424, 0x8c);
    wr_XC7011_Reg(0x0425, 0xa3);
    wr_XC7011_Reg(0x0427, 0x8b);
    wr_XC7011_Reg(0x0428, 0xa4);
    wr_XC7011_Reg(0x0429, 0x84);
    wr_XC7011_Reg(0x042b, 0x01);
    wr_XC7011_Reg(0x042c, 0x07);
    wr_XC7011_Reg(0x042d, 0xfb);
    wr_XC7011_Reg(0x042e, 0x1a);
    wr_XC7011_Reg(0x042f, 0x0a);
    wr_XC7011_Reg(0x0430, 0x9c);
    wr_XC7011_Reg(0x0431, 0x60);
    wr_XC7011_Reg(0x0432, 0x3e);
    wr_XC7011_Reg(0x0433, 0x07);
    wr_XC7011_Reg(0x0434, 0xb8);
    wr_XC7011_Reg(0x0435, 0x6e);
    wr_XC7011_Reg(0x0437, 0x02);
    wr_XC7011_Reg(0x0438, 0xb8);
    wr_XC7011_Reg(0x0439, 0x92);
    wr_XC7011_Reg(0x043b, 0x05);
    wr_XC7011_Reg(0x043c, 0x84);
    wr_XC7011_Reg(0x043d, 0xb0);
    wr_XC7011_Reg(0x0440, 0xa4);
    wr_XC7011_Reg(0x0441, 0x63);
    wr_XC7011_Reg(0x0443, 0x1c);
    wr_XC7011_Reg(0x0444, 0xa4);
    wr_XC7011_Reg(0x0445, 0x84);
    wr_XC7011_Reg(0x0447, 0xe0);
    wr_XC7011_Reg(0x0448, 0xa8);
    wr_XC7011_Reg(0x0449, 0x63);
    wr_XC7011_Reg(0x044b, 0x03);
    wr_XC7011_Reg(0x044c, 0x8c);
    wr_XC7011_Reg(0x044d, 0xa5);
    wr_XC7011_Reg(0x044f, 0x8b);
    wr_XC7011_Reg(0x0450, 0xe0);
    wr_XC7011_Reg(0x0451, 0x84);
    wr_XC7011_Reg(0x0452, 0x18);
    wr_XC7011_Reg(0x0454, 0x07);
    wr_XC7011_Reg(0x0455, 0xfb);
    wr_XC7011_Reg(0x0456, 0x1a);
    wr_XC7011_Reg(0x0458, 0x9c);
    wr_XC7011_Reg(0x0459, 0x60);
    wr_XC7011_Reg(0x045a, 0x3e);
    wr_XC7011_Reg(0x045b, 0x08);
    wr_XC7011_Reg(0x045c, 0x84);
    wr_XC7011_Reg(0x045d, 0x70);
    wr_XC7011_Reg(0x0460, 0xa8);
    wr_XC7011_Reg(0x0461, 0x8c);
    wr_XC7011_Reg(0x0464, 0x8c);
    wr_XC7011_Reg(0x0465, 0xa3);
    wr_XC7011_Reg(0x0467, 0x8b);
    wr_XC7011_Reg(0x0468, 0x07);
    wr_XC7011_Reg(0x0469, 0xfb);
    wr_XC7011_Reg(0x046a, 0x19);
    wr_XC7011_Reg(0x046b, 0xfb);
    wr_XC7011_Reg(0x046c, 0x9c);
    wr_XC7011_Reg(0x046d, 0x60);
    wr_XC7011_Reg(0x046e, 0x3e);
    wr_XC7011_Reg(0x046f, 0x09);
    wr_XC7011_Reg(0x0470, 0x9c);
    wr_XC7011_Reg(0x0471, 0x60);
    wr_XC7011_Reg(0x0472, 0x39);
    wr_XC7011_Reg(0x0473, 0x03);
    wr_XC7011_Reg(0x0474, 0x9c);
    wr_XC7011_Reg(0x0475, 0x80);
    wr_XC7011_Reg(0x0477, 0x84);
    wr_XC7011_Reg(0x0478, 0x07);
    wr_XC7011_Reg(0x0479, 0xfb);
    wr_XC7011_Reg(0x047a, 0x19);
    wr_XC7011_Reg(0x047b, 0xf7);
    wr_XC7011_Reg(0x047c, 0x9c);
    wr_XC7011_Reg(0x047d, 0xa0);
    wr_XC7011_Reg(0x047f, 0x01);
    wr_XC7011_Reg(0x0480, 0x9c);
    wr_XC7011_Reg(0x0481, 0x60);
    wr_XC7011_Reg(0x0482, 0x39);
    wr_XC7011_Reg(0x0483, 0x03);
    wr_XC7011_Reg(0x0484, 0x9c);
    wr_XC7011_Reg(0x0485, 0x80);
    wr_XC7011_Reg(0x0487, 0x04);
    wr_XC7011_Reg(0x0488, 0x07);
    wr_XC7011_Reg(0x0489, 0xfb);
    wr_XC7011_Reg(0x048a, 0x19);
    wr_XC7011_Reg(0x048b, 0xf3);
    wr_XC7011_Reg(0x048c, 0x9c);
    wr_XC7011_Reg(0x048d, 0xa0);
    wr_XC7011_Reg(0x048f, 0x01);
    wr_XC7011_Reg(0x0490, 0x9c);
    wr_XC7011_Reg(0x0491, 0x60);
    wr_XC7011_Reg(0x0492, 0x38);
    wr_XC7011_Reg(0x0493, 0x12);
    wr_XC7011_Reg(0x0494, 0x9c);
    wr_XC7011_Reg(0x0495, 0x80);
    wr_XC7011_Reg(0x0498, 0x07);
    wr_XC7011_Reg(0x0499, 0xfb);
    wr_XC7011_Reg(0x049a, 0x19);
    wr_XC7011_Reg(0x049b, 0xef);
    wr_XC7011_Reg(0x049c, 0x9c);
    wr_XC7011_Reg(0x049d, 0xa0);
    wr_XC7011_Reg(0x049f, 0x01);
    wr_XC7011_Reg(0x04a0, 0xbc);
    wr_XC7011_Reg(0x04a1, 0x4a);
    wr_XC7011_Reg(0x04a3, 0x1f);
    wr_XC7011_Reg(0x04a4, 0x0c);
    wr_XC7011_Reg(0x04a7, 0x25);
    wr_XC7011_Reg(0x04a8, 0xbc);
    wr_XC7011_Reg(0x04a9, 0x4a);
    wr_XC7011_Reg(0x04ab, 0x3f);
    wr_XC7011_Reg(0x04ac, 0x0c);
    wr_XC7011_Reg(0x04af, 0x20);
    wr_XC7011_Reg(0x04b0, 0xbc);
    wr_XC7011_Reg(0x04b1, 0x4a);
    wr_XC7011_Reg(0x04b3, 0x7f);
    wr_XC7011_Reg(0x04b4, 0x10);
    wr_XC7011_Reg(0x04b7, 0x2c);
    wr_XC7011_Reg(0x04b8, 0x15);
    wr_XC7011_Reg(0x04bc, 0x9c);
    wr_XC7011_Reg(0x04bd, 0x60);
    wr_XC7011_Reg(0x04be, 0x33);
    wr_XC7011_Reg(0x04bf, 0x01);
    wr_XC7011_Reg(0x04c0, 0x9c);
    wr_XC7011_Reg(0x04c1, 0x80);
    wr_XC7011_Reg(0x04c3, 0x09);
    wr_XC7011_Reg(0x04c4, 0x07);
    wr_XC7011_Reg(0x04c5, 0xfb);
    wr_XC7011_Reg(0x04c6, 0x19);
    wr_XC7011_Reg(0x04c7, 0xe4);
    wr_XC7011_Reg(0x04c8, 0x9c);
    wr_XC7011_Reg(0x04c9, 0xa0);
    wr_XC7011_Reg(0x04cb, 0x01);
    wr_XC7011_Reg(0x04cc, 0x9c);
    wr_XC7011_Reg(0x04cd, 0x60);
    wr_XC7011_Reg(0x04ce, 0x36);
    wr_XC7011_Reg(0x04cf, 0x31);
    wr_XC7011_Reg(0x04d0, 0x9c);
    wr_XC7011_Reg(0x04d1, 0x80);
    wr_XC7011_Reg(0x04d3, 0x86);
    wr_XC7011_Reg(0x04d4, 0x07);
    wr_XC7011_Reg(0x04d5, 0xfb);
    wr_XC7011_Reg(0x04d6, 0x19);
    wr_XC7011_Reg(0x04d7, 0xe0);
    wr_XC7011_Reg(0x04d8, 0x9c);
    wr_XC7011_Reg(0x04d9, 0xa0);
    wr_XC7011_Reg(0x04db, 0x01);
    wr_XC7011_Reg(0x04dc, 0x9c);
    wr_XC7011_Reg(0x04dd, 0x60);
    wr_XC7011_Reg(0x04de, 0x36);
    wr_XC7011_Reg(0x04df, 0x20);
    wr_XC7011_Reg(0x04e3, 0x46);
    wr_XC7011_Reg(0x04e4, 0x9c);
    wr_XC7011_Reg(0x04e5, 0x80);
    wr_XC7011_Reg(0x04e7, 0x08);
    wr_XC7011_Reg(0x04e8, 0x10);
    wr_XC7011_Reg(0x04eb, 0x07);
    wr_XC7011_Reg(0x04ec, 0xbc);
    wr_XC7011_Reg(0x04ed, 0x4a);
    wr_XC7011_Reg(0x04ef, 0x7c);
    wr_XC7011_Reg(0x04f0, 0xb8);
    wr_XC7011_Reg(0x04f1, 0x83);
    wr_XC7011_Reg(0x04f3, 0x42);
    wr_XC7011_Reg(0x04f4, 0x9e);
    wr_XC7011_Reg(0x04f5, 0x40);
    wr_XC7011_Reg(0x04f8, 0x9d);
    wr_XC7011_Reg(0x04f9, 0xc0);
    wr_XC7011_Reg(0x04fb, 0x01);
    wr_XC7011_Reg(0x04fc, 0x03);
    wr_XC7011_Reg(0x04fd, 0xff);
    wr_XC7011_Reg(0x04fe, 0xff);
    wr_XC7011_Reg(0x04ff, 0xc8);
    wr_XC7011_Reg(0x0500, 0xa5);
    wr_XC7011_Reg(0x0501, 0x84);
    wr_XC7011_Reg(0x0502, 0xff);
    wr_XC7011_Reg(0x0503, 0xff);
    wr_XC7011_Reg(0x0504, 0x0c);
    wr_XC7011_Reg(0x0507, 0x1b);
    wr_XC7011_Reg(0x0508, 0xb8);
    wr_XC7011_Reg(0x0509, 0x83);
    wr_XC7011_Reg(0x050b, 0x43);
    wr_XC7011_Reg(0x050c, 0xbc);
    wr_XC7011_Reg(0x050d, 0x4a);
    wr_XC7011_Reg(0x050f, 0xf8);
    wr_XC7011_Reg(0x0510, 0x10);
    wr_XC7011_Reg(0x0513, 0x1c);
    wr_XC7011_Reg(0x0514, 0xbc);
    wr_XC7011_Reg(0x0515, 0x4a);
    wr_XC7011_Reg(0x0516, 0x01);
    wr_XC7011_Reg(0x0517, 0xf0);
    wr_XC7011_Reg(0x0518, 0xb8);
    wr_XC7011_Reg(0x0519, 0x83);
    wr_XC7011_Reg(0x051b, 0x44);
    wr_XC7011_Reg(0x051c, 0x9e);
    wr_XC7011_Reg(0x051d, 0x40);
    wr_XC7011_Reg(0x0520, 0x9d);
    wr_XC7011_Reg(0x0521, 0xc0);
    wr_XC7011_Reg(0x0523, 0x07);
    wr_XC7011_Reg(0x0524, 0x03);
    wr_XC7011_Reg(0x0525, 0xff);
    wr_XC7011_Reg(0x0526, 0xff);
    wr_XC7011_Reg(0x0527, 0xbe);
    wr_XC7011_Reg(0x0528, 0xa5);
    wr_XC7011_Reg(0x0529, 0x84);
    wr_XC7011_Reg(0x052a, 0xff);
    wr_XC7011_Reg(0x052b, 0xff);
    wr_XC7011_Reg(0x052c, 0x9c);
    wr_XC7011_Reg(0x052d, 0x60);
    wr_XC7011_Reg(0x052e, 0x33);
    wr_XC7011_Reg(0x052f, 0x01);
    wr_XC7011_Reg(0x0530, 0x03);
    wr_XC7011_Reg(0x0531, 0xff);
    wr_XC7011_Reg(0x0532, 0xff);
    wr_XC7011_Reg(0x0533, 0xe5);
    wr_XC7011_Reg(0x0534, 0x9c);
    wr_XC7011_Reg(0x0535, 0x80);
    wr_XC7011_Reg(0x0537, 0x08);
    wr_XC7011_Reg(0x0538, 0x9c);
    wr_XC7011_Reg(0x0539, 0x60);
    wr_XC7011_Reg(0x053a, 0x33);
    wr_XC7011_Reg(0x053b, 0x01);
    wr_XC7011_Reg(0x053c, 0x9c);
    wr_XC7011_Reg(0x053d, 0x80);
    wr_XC7011_Reg(0x053f, 0x04);
    wr_XC7011_Reg(0x0540, 0x07);
    wr_XC7011_Reg(0x0541, 0xfb);
    wr_XC7011_Reg(0x0542, 0x19);
    wr_XC7011_Reg(0x0543, 0xc5);
    wr_XC7011_Reg(0x0544, 0x9c);
    wr_XC7011_Reg(0x0545, 0xa0);
    wr_XC7011_Reg(0x0547, 0x01);
    wr_XC7011_Reg(0x0548, 0x9c);
    wr_XC7011_Reg(0x0549, 0x60);
    wr_XC7011_Reg(0x054a, 0x36);
    wr_XC7011_Reg(0x054b, 0x31);
    wr_XC7011_Reg(0x054c, 0x9c);
    wr_XC7011_Reg(0x054d, 0x80);
    wr_XC7011_Reg(0x054f, 0x84);
    wr_XC7011_Reg(0x0550, 0x07);
    wr_XC7011_Reg(0x0551, 0xfb);
    wr_XC7011_Reg(0x0552, 0x19);
    wr_XC7011_Reg(0x0553, 0xc1);
    wr_XC7011_Reg(0x0554, 0x9c);
    wr_XC7011_Reg(0x0555, 0xa0);
    wr_XC7011_Reg(0x0557, 0x01);
    wr_XC7011_Reg(0x0558, 0x9c);
    wr_XC7011_Reg(0x0559, 0x60);
    wr_XC7011_Reg(0x055a, 0x36);
    wr_XC7011_Reg(0x055b, 0x20);
    wr_XC7011_Reg(0x055f, 0x27);
    wr_XC7011_Reg(0x0560, 0x9c);
    wr_XC7011_Reg(0x0561, 0x80);
    wr_XC7011_Reg(0x0563, 0x28);
    wr_XC7011_Reg(0x0564, 0x9c);
    wr_XC7011_Reg(0x0565, 0x60);
    wr_XC7011_Reg(0x0566, 0x33);
    wr_XC7011_Reg(0x0567, 0x01);
    wr_XC7011_Reg(0x0568, 0x03);
    wr_XC7011_Reg(0x0569, 0xff);
    wr_XC7011_Reg(0x056a, 0xff);
    wr_XC7011_Reg(0x056b, 0xd7);
    wr_XC7011_Reg(0x056c, 0x9c);
    wr_XC7011_Reg(0x056d, 0x80);
    wr_XC7011_Reg(0x056f, 0x20);
    wr_XC7011_Reg(0x0570, 0x9e);
    wr_XC7011_Reg(0x0571, 0x40);
    wr_XC7011_Reg(0x0574, 0x9d);
    wr_XC7011_Reg(0x0575, 0xc0);
    wr_XC7011_Reg(0x0577, 0x03);
    wr_XC7011_Reg(0x0578, 0x03);
    wr_XC7011_Reg(0x0579, 0xff);
    wr_XC7011_Reg(0x057a, 0xff);
    wr_XC7011_Reg(0x057b, 0xa9);
    wr_XC7011_Reg(0x057c, 0xa5);
    wr_XC7011_Reg(0x057d, 0x84);
    wr_XC7011_Reg(0x057e, 0xff);
    wr_XC7011_Reg(0x057f, 0xff);
    wr_XC7011_Reg(0x0580, 0x0c);
    wr_XC7011_Reg(0x0583, 0x09);
    wr_XC7011_Reg(0x0584, 0xb8);
    wr_XC7011_Reg(0x0585, 0x83);
    wr_XC7011_Reg(0x0587, 0x45);
    wr_XC7011_Reg(0x0588, 0xbc);
    wr_XC7011_Reg(0x0589, 0x4a);
    wr_XC7011_Reg(0x058a, 0x03);
    wr_XC7011_Reg(0x058b, 0xe0);
    wr_XC7011_Reg(0x058c, 0x10);
    wr_XC7011_Reg(0x058f, 0x09);
    wr_XC7011_Reg(0x0590, 0xbc);
    wr_XC7011_Reg(0x0591, 0x4a);
    wr_XC7011_Reg(0x0592, 0x07);
    wr_XC7011_Reg(0x0593, 0xc0);
    wr_XC7011_Reg(0x0594, 0xb8);
    wr_XC7011_Reg(0x0595, 0x83);
    wr_XC7011_Reg(0x0597, 0x46);
    wr_XC7011_Reg(0x0598, 0x9e);
    wr_XC7011_Reg(0x0599, 0x40);
    wr_XC7011_Reg(0x059b, 0x03);
    wr_XC7011_Reg(0x059c, 0x03);
    wr_XC7011_Reg(0x059d, 0xff);
    wr_XC7011_Reg(0x059e, 0xff);
    wr_XC7011_Reg(0x059f, 0xe2);
    wr_XC7011_Reg(0x05a0, 0x9d);
    wr_XC7011_Reg(0x05a1, 0xc0);
    wr_XC7011_Reg(0x05a3, 0x07);
    wr_XC7011_Reg(0x05a4, 0x9e);
    wr_XC7011_Reg(0x05a5, 0x40);
    wr_XC7011_Reg(0x05a7, 0x01);
    wr_XC7011_Reg(0x05a8, 0x03);
    wr_XC7011_Reg(0x05a9, 0xff);
    wr_XC7011_Reg(0x05aa, 0xff);
    wr_XC7011_Reg(0x05ab, 0xdf);
    wr_XC7011_Reg(0x05ac, 0x9d);
    wr_XC7011_Reg(0x05ad, 0xc0);
    wr_XC7011_Reg(0x05af, 0x07);
    wr_XC7011_Reg(0x05b0, 0x10);
    wr_XC7011_Reg(0x05b3, 0x06);
    wr_XC7011_Reg(0x05b4, 0xbc);
    wr_XC7011_Reg(0x05b5, 0x4a);
    wr_XC7011_Reg(0x05b6, 0x0f);
    wr_XC7011_Reg(0x05b7, 0x80);
    wr_XC7011_Reg(0x05b8, 0xb8);
    wr_XC7011_Reg(0x05b9, 0x83);
    wr_XC7011_Reg(0x05bb, 0x47);
    wr_XC7011_Reg(0x05bc, 0x9d);
    wr_XC7011_Reg(0x05bd, 0xc0);
    wr_XC7011_Reg(0x05bf, 0x07);
    wr_XC7011_Reg(0x05c0, 0x03);
    wr_XC7011_Reg(0x05c1, 0xff);
    wr_XC7011_Reg(0x05c2, 0xff);
    wr_XC7011_Reg(0x05c3, 0x96);
    wr_XC7011_Reg(0x05c4, 0xa5);
    wr_XC7011_Reg(0x05c5, 0x84);
    wr_XC7011_Reg(0x05c6, 0xff);
    wr_XC7011_Reg(0x05c7, 0xff);
    wr_XC7011_Reg(0x05c8, 0x10);
    wr_XC7011_Reg(0x05cb, 0x06);
    wr_XC7011_Reg(0x05cc, 0xbc);
    wr_XC7011_Reg(0x05cd, 0xaa);
    wr_XC7011_Reg(0x05ce, 0x0f);
    wr_XC7011_Reg(0x05cf, 0xc0);
    wr_XC7011_Reg(0x05d0, 0xb8);
    wr_XC7011_Reg(0x05d1, 0x83);
    wr_XC7011_Reg(0x05d3, 0x48);
    wr_XC7011_Reg(0x05d4, 0x9e);
    wr_XC7011_Reg(0x05d5, 0x40);
    wr_XC7011_Reg(0x05d7, 0x0f);
    wr_XC7011_Reg(0x05d8, 0x03);
    wr_XC7011_Reg(0x05d9, 0xff);
    wr_XC7011_Reg(0x05da, 0xff);
    wr_XC7011_Reg(0x05db, 0xd3);
    wr_XC7011_Reg(0x05dc, 0x9d);
    wr_XC7011_Reg(0x05dd, 0xc0);
    wr_XC7011_Reg(0x05df, 0x07);
    wr_XC7011_Reg(0x05e0, 0x13);
    wr_XC7011_Reg(0x05e1, 0xff);
    wr_XC7011_Reg(0x05e2, 0xff);
    wr_XC7011_Reg(0x05e3, 0x8f);
    wr_XC7011_Reg(0x05e4, 0x15);
    wr_XC7011_Reg(0x05e8, 0x9e);
    wr_XC7011_Reg(0x05e9, 0x40);
    wr_XC7011_Reg(0x05eb, 0x0f);
    wr_XC7011_Reg(0x05ec, 0x9d);
    wr_XC7011_Reg(0x05ed, 0xc0);
    wr_XC7011_Reg(0x05ef, 0x07);
    wr_XC7011_Reg(0x05f0, 0x03);
    wr_XC7011_Reg(0x05f1, 0xff);
    wr_XC7011_Reg(0x05f2, 0xff);
    wr_XC7011_Reg(0x05f3, 0x8b);
    wr_XC7011_Reg(0x05f4, 0x9d);
    wr_XC7011_Reg(0x05f5, 0x80);
    wr_XC7011_Reg(0x05f7, 0x1f);
    wr_XC7011_Reg(0x05f8, 0x07);
    wr_XC7011_Reg(0x05f9, 0xfb);
    wr_XC7011_Reg(0x05fa, 0x19);
    wr_XC7011_Reg(0x05fb, 0x97);
    wr_XC7011_Reg(0x05fc, 0x9c);
    wr_XC7011_Reg(0x05fd, 0xa0);
    wr_XC7011_Reg(0x05ff, 0x01);
    wr_XC7011_Reg(0x0600, 0x9c);
    wr_XC7011_Reg(0x0601, 0x60);
    wr_XC7011_Reg(0x0602, 0x38);
    wr_XC7011_Reg(0x0603, 0x12);
    wr_XC7011_Reg(0x0604, 0x9c);
    wr_XC7011_Reg(0x0605, 0x80);
    wr_XC7011_Reg(0x0607, 0x30);
    wr_XC7011_Reg(0x0608, 0x07);
    wr_XC7011_Reg(0x0609, 0xfb);
    wr_XC7011_Reg(0x060a, 0x19);
    wr_XC7011_Reg(0x060b, 0x93);
    wr_XC7011_Reg(0x060c, 0x9c);
    wr_XC7011_Reg(0x060d, 0xa0);
    wr_XC7011_Reg(0x060f, 0x01);
    wr_XC7011_Reg(0x0610, 0x85);
    wr_XC7011_Reg(0x0611, 0x21);
    wr_XC7011_Reg(0x0614, 0x85);
    wr_XC7011_Reg(0x0615, 0x41);
    wr_XC7011_Reg(0x0617, 0x04);
    wr_XC7011_Reg(0x0618, 0x85);
    wr_XC7011_Reg(0x0619, 0x81);
    wr_XC7011_Reg(0x061b, 0x08);
    wr_XC7011_Reg(0x061c, 0x85);
    wr_XC7011_Reg(0x061d, 0xc1);
    wr_XC7011_Reg(0x061f, 0x0c);
    wr_XC7011_Reg(0x0620, 0x86);
    wr_XC7011_Reg(0x0621, 0x01);
    wr_XC7011_Reg(0x0623, 0x10);
    wr_XC7011_Reg(0x0624, 0x86);
    wr_XC7011_Reg(0x0625, 0x41);
    wr_XC7011_Reg(0x0627, 0x14);
    wr_XC7011_Reg(0x0628, 0x44);
    wr_XC7011_Reg(0x062a, 0x48);
    wr_XC7011_Reg(0x062c, 0x9c);
    wr_XC7011_Reg(0x062d, 0x21);
    wr_XC7011_Reg(0x062f, 0x18);
    wr_XC7011_Reg(0x0630, 0x9c);
    wr_XC7011_Reg(0x0631, 0x21);
    wr_XC7011_Reg(0x0632, 0xff);
    wr_XC7011_Reg(0x0633, 0xfc);
    wr_XC7011_Reg(0x0634, 0xd4);
    wr_XC7011_Reg(0x0635, 0x01);
    wr_XC7011_Reg(0x0636, 0x48);
    wr_XC7011_Reg(0x0638, 0xbc);
    wr_XC7011_Reg(0x0639, 0x23);
    wr_XC7011_Reg(0x063a, 0x04);
    wr_XC7011_Reg(0x063b, 0x0c);
    wr_XC7011_Reg(0x063c, 0x10);
    wr_XC7011_Reg(0x063f, 0x04);
    wr_XC7011_Reg(0x0640, 0x15);
    wr_XC7011_Reg(0x0644, 0x07);
    wr_XC7011_Reg(0x0645, 0xff);
    wr_XC7011_Reg(0x0646, 0xff);
    wr_XC7011_Reg(0x0647, 0x64);
    wr_XC7011_Reg(0x0648, 0x15);
    wr_XC7011_Reg(0x064c, 0x85);
    wr_XC7011_Reg(0x064d, 0x21);
    wr_XC7011_Reg(0x0650, 0x44);
    wr_XC7011_Reg(0x0652, 0x48);
    wr_XC7011_Reg(0x0654, 0x9c);
    wr_XC7011_Reg(0x0655, 0x21);
    wr_XC7011_Reg(0x0657, 0x04);
    wr_XC7011_Reg(0x0658, 0x9c);
    wr_XC7011_Reg(0x0659, 0x21);
    wr_XC7011_Reg(0x065a, 0xff);
    wr_XC7011_Reg(0x065b, 0xfc);
    wr_XC7011_Reg(0x065c, 0xd4);
    wr_XC7011_Reg(0x065d, 0x01);
    wr_XC7011_Reg(0x065e, 0x48);
    wr_XC7011_Reg(0x0660, 0x07);
    wr_XC7011_Reg(0x0661, 0xff);
    wr_XC7011_Reg(0x0662, 0xff);
    wr_XC7011_Reg(0x0663, 0xf4);
    wr_XC7011_Reg(0x0664, 0x15);
    wr_XC7011_Reg(0x0668, 0x18);
    wr_XC7011_Reg(0x0669, 0x60);
    wr_XC7011_Reg(0x066b, 0x14);
    wr_XC7011_Reg(0x066c, 0xa8);
    wr_XC7011_Reg(0x066d, 0x63);
    wr_XC7011_Reg(0x066e, 0x03);
    wr_XC7011_Reg(0x066f, 0x18);
    wr_XC7011_Reg(0x0670, 0x9d);
    wr_XC7011_Reg(0x0671, 0x60);
    wr_XC7011_Reg(0x0674, 0x84);
    wr_XC7011_Reg(0x0675, 0x63);
    wr_XC7011_Reg(0x0678, 0xd4);
    wr_XC7011_Reg(0x0679, 0x03);
    wr_XC7011_Reg(0x067a, 0x59);
    wr_XC7011_Reg(0x067b, 0xcc);
    wr_XC7011_Reg(0x067c, 0xd4);
    wr_XC7011_Reg(0x067d, 0x03);
    wr_XC7011_Reg(0x067e, 0x59);
    wr_XC7011_Reg(0x067f, 0xc0);
    wr_XC7011_Reg(0x0680, 0xd4);
    wr_XC7011_Reg(0x0681, 0x03);
    wr_XC7011_Reg(0x0682, 0x59);
    wr_XC7011_Reg(0x0683, 0xc4);
    wr_XC7011_Reg(0x0684, 0xd4);
    wr_XC7011_Reg(0x0685, 0x03);
    wr_XC7011_Reg(0x0686, 0x59);
    wr_XC7011_Reg(0x0687, 0xc8);
    wr_XC7011_Reg(0x0688, 0x85);
    wr_XC7011_Reg(0x0689, 0x21);
    wr_XC7011_Reg(0x068c, 0x44);
    wr_XC7011_Reg(0x068e, 0x48);
    wr_XC7011_Reg(0x0690, 0x9c);
    wr_XC7011_Reg(0x0691, 0x21);
    wr_XC7011_Reg(0x0693, 0x04);


    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0137, 0x99);

//patch end


    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x1f00, 0x00);
    wr_XC7011_Reg(0x1f01, 0x00);
    wr_XC7011_Reg(0x1f02, 0x00);
    wr_XC7011_Reg(0x1f03, 0x00);
    wr_XC7011_Reg(0x1f04, 0x05);
    wr_XC7011_Reg(0x1f05, 0x00);
    wr_XC7011_Reg(0x1f06, 0x02);
    wr_XC7011_Reg(0x1f07, 0xd0);
    wr_XC7011_Reg(0x1f08, 0x03); //r_avg_ctrl); bit[1]:avg_opt; bit[0]:avg_man

    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x003f, 0x10);
    wr_XC7011_Reg(0x0040, 0x10);
    wr_XC7011_Reg(0x0041, 0x10);
    wr_XC7011_Reg(0x0042, 0x10);
    wr_XC7011_Reg(0x0043, 0x10);

    wr_XC7011_Reg(0x0044, 0x10);
    wr_XC7011_Reg(0x0045, 0x10);
    wr_XC7011_Reg(0x0046, 0x10);
    wr_XC7011_Reg(0x0047, 0x10);
    wr_XC7011_Reg(0x0048, 0x10);

    wr_XC7011_Reg(0x0049, 0x10);
    wr_XC7011_Reg(0x004a, 0x10);
    wr_XC7011_Reg(0x004b, 0x10);
    wr_XC7011_Reg(0x004c, 0x10);
    wr_XC7011_Reg(0x004d, 0x10);

    wr_XC7011_Reg(0x004e, 0x10);
    wr_XC7011_Reg(0x004f, 0x10);
    wr_XC7011_Reg(0x0050, 0x10);
    wr_XC7011_Reg(0x0051, 0x10);
    wr_XC7011_Reg(0x0052, 0x10);

    wr_XC7011_Reg(0x0053, 0x10);
    wr_XC7011_Reg(0x0054, 0x10);
    wr_XC7011_Reg(0x0055, 0x10);
    wr_XC7011_Reg(0x0056, 0x10);
    wr_XC7011_Reg(0x0057, 0x10);

    wr_XC7011_Reg(0x003a, 0x01);

    wr_XC7011_Reg(0x0051, 0x03);

    //ae
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x004d, 0x00);
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x0026, 0x01);  //AEC enable

    wr_XC7011_Reg(0x00be, 0x60);  	//camera i2c id
    wr_XC7011_Reg(0x00bf, 0x01);  	//camera i2c bits
    wr_XC7011_Reg(0x00c0, 0x05); //5 	//sensor type gain
    wr_XC7011_Reg(0x00c1, 0x00);  	//sensor type exposure

    //{wr_XC7011_Reg(0x00c3,0x01);    //sensor mode select

    //exposure
    wr_XC7011_Reg(0x00c4, 0x3e); //write camera exposure ); ariable [15:12]
    wr_XC7011_Reg(0x00c5, 0x02);
    wr_XC7011_Reg(0x00c6, 0x3E); //write camera exposure ); ariable [11:4]
    wr_XC7011_Reg(0x00c7, 0x01);
    wr_XC7011_Reg(0x00c8, 0x00); //write camera exposure ); ariable [3:0]
    wr_XC7011_Reg(0x00c9, 0x00);

    wr_XC7011_Reg(0x00cc, 0x00); //camera exposure addr mask 1
    wr_XC7011_Reg(0x00cd, 0xff);
    wr_XC7011_Reg(0x00ce, 0x00); //camera exposure addr mask 2
    wr_XC7011_Reg(0x00cf, 0xff);
    wr_XC7011_Reg(0x00d0, 0x00); //camera exposure addr mask 3
    wr_XC7011_Reg(0x00d1, 0x00);
    wr_XC7011_Reg(0x00d2, 0x00); //camera exposure addr mask 4
    wr_XC7011_Reg(0x00d3, 0x00);

    //gain
    wr_XC7011_Reg(0x00e4, 0x3E); //camera gain addr
    wr_XC7011_Reg(0x00e5, 0x08);
    wr_XC7011_Reg(0x00e6, 0x3E); //camera gain addr
    wr_XC7011_Reg(0x00e7, 0x09);

    wr_XC7011_Reg(0x00ec, 0x00); //camera gain addr mask 1
    wr_XC7011_Reg(0x00ed, 0xff);
    wr_XC7011_Reg(0x00ee, 0x00); //camera gain addr mask 2
    wr_XC7011_Reg(0x00ef, 0xff);
    wr_XC7011_Reg(0x00f0, 0x00); //camera gain addr mask 3
    wr_XC7011_Reg(0x00f1, 0x00);
    wr_XC7011_Reg(0x00f2, 0x00); //camera gain addr mask 4
    wr_XC7011_Reg(0x00f3, 0x00);


    //banding
    wr_XC7011_Reg(0x00b4, 0x02);   // banding mode
    wr_XC7011_Reg(0x00b8, 0x0b);   // banding  exposure lines
    wr_XC7011_Reg(0x00b9, 0xb0);
    wr_XC7011_Reg(0x0035, 0x01);

    // AE CFG
// AE CFG
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x00aa, 0x01);
    wr_XC7011_Reg(0x00ab, 0xc0);  //max gain
    wr_XC7011_Reg(0x00ba, 0x00);   //low light mode
    wr_XC7011_Reg(0x006a, 0x01);
    wr_XC7011_Reg(0x006b, 0xc0);  //night target
    wr_XC7011_Reg(0x006c, 0x00);
    wr_XC7011_Reg(0x006d, 0xa0);  //day target
    wr_XC7011_Reg(0x0073, 0x18);  //AttAvgReviseDiff


//AE SPEED
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x0039, 0x00);   //DETECT enable
    wr_XC7011_Reg(0x0060, 0x00);   //smart mode 03

    wr_XC7011_Reg(0x005d, 0x00);
    wr_XC7011_Reg(0x005e, 0x30);
    wr_XC7011_Reg(0x005f, 0x00);
    wr_XC7011_Reg(0x003a, 0x01);
    wr_XC7011_Reg(0x0071, 0xc0);   //  target offset
    wr_XC7011_Reg(0x0076, 0x02);    // delay frame
//wr_XC7011_Reg(0x0079,0x01);
    wr_XC7011_Reg(0x007a, 0x00);
    wr_XC7011_Reg(0x007b, 0x30);		//1. threshold low
    wr_XC7011_Reg(0x007c, 0x00);
    wr_XC7011_Reg(0x007d, 0xb0);  //2. threshold high
    wr_XC7011_Reg(0x0077, 0x20);   //3. finally thr
    wr_XC7011_Reg(0x0078, 0x01);   //total speed
    wr_XC7011_Reg(0x007f, 0x60); // jump threshold
    wr_XC7011_Reg(0x0087, 0x60); // jump ratio
    wr_XC7011_Reg(0x0088, 0x02); // jump times
    wr_XC7011_Reg(0x008c, 0x03);
    wr_XC7011_Reg(0x008d, 0x80);

// AE SMART

    wr_XC7011_Reg(0x0112, 0x03);  //Att Detect Speed
    wr_XC7011_Reg(0x0113, 0x03);
    wr_XC7011_Reg(0x0114, 0x03);		// below or above cnt
    wr_XC7011_Reg(0x011a, 0x00);
    wr_XC7011_Reg(0x011b, 0x00);  //day min attention  avg thr
    wr_XC7011_Reg(0x011c, 0x02);
    wr_XC7011_Reg(0x011d, 0x20);  //day max attention  avg thr
    wr_XC7011_Reg(0x011e, 0x00);
    wr_XC7011_Reg(0x011f, 0x00);  //day min avg thr
    wr_XC7011_Reg(0x0120, 0x02);
    wr_XC7011_Reg(0x0121, 0x80);  //day max avg thr
    wr_XC7011_Reg(0x0062, 0xff);
    wr_XC7011_Reg(0x0063, 0xf0);   // D-N
    wr_XC7011_Reg(0x0064, 0xff);
    wr_XC7011_Reg(0x0065, 0xf0);   // N-D
    wr_XC7011_Reg(0x0066, 0x00);
    wr_XC7011_Reg(0x0067, 0x00);   //dark thr
    wr_XC7011_Reg(0x0074, 0x00);   //diff ratio low
    wr_XC7011_Reg(0x0075, 0x00);   //diff ratio high
    wr_XC7011_Reg(0x006e, 0x00);    //magnituede offset
    wr_XC7011_Reg(0x006f, 0x00);    //magnituede MIN
    wr_XC7011_Reg(0x0070, 0xff);
    wr_XC7011_Reg(0x012a, 0x00);  //variance thr promote luma
    wr_XC7011_Reg(0x012b, 0x00);

    wr_XC7011_Reg(0x003b, 0x40);
    wr_XC7011_Reg(0x003c, 0x40);
    wr_XC7011_Reg(0x003d, 0x90);
    wr_XC7011_Reg(0x003e, 0x10);
    wr_XC7011_Reg(0x0039, 0x00);
//0927 by wenzhe
    wr_XC7011_Reg(0x0112, 0x08); //target speed

    wr_XC7011_Reg(0x012e, 0x01); // patch add
    wr_XC7011_Reg(0x012f, 0xc0);
    wr_XC7011_Reg(0x0131, 0x10);

    wr_XC7011_Reg(0x010f, 0x00); // gain and exp delay
    wr_XC7011_Reg(0x0110, 0x00);

//IQ


    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x008a, 0x00);		//Low
    wr_XC7011_Reg(0x008b, 0x30);
    wr_XC7011_Reg(0x008c, 0x05);		//Hight
    wr_XC7011_Reg(0x008d, 0x00);

//AE SENSOR  END

//lenc
    wr_XC7011_Reg(0xfffe, 0x30); //lenc_88

    wr_XC7011_Reg(0x0200, 0x18);
    wr_XC7011_Reg(0x0201, 0x10);
    wr_XC7011_Reg(0x0202, 0xf);
    wr_XC7011_Reg(0x0203, 0xf);
    wr_XC7011_Reg(0x0204, 0x12);
    wr_XC7011_Reg(0x0205, 0x1b);
    wr_XC7011_Reg(0x0206, 0x7);
    wr_XC7011_Reg(0x0207, 0x5);
    wr_XC7011_Reg(0x0208, 0x4);
    wr_XC7011_Reg(0x0209, 0x4);
    wr_XC7011_Reg(0x020a, 0x5);
    wr_XC7011_Reg(0x020b, 0x9);
    wr_XC7011_Reg(0x020c, 0x3);
    wr_XC7011_Reg(0x020d, 0x1);
    wr_XC7011_Reg(0x020e, 0x0);
    wr_XC7011_Reg(0x020f, 0x0);
    wr_XC7011_Reg(0x0210, 0x1);
    wr_XC7011_Reg(0x0211, 0x4);
    wr_XC7011_Reg(0x0212, 0x3);
    wr_XC7011_Reg(0x0213, 0x0);
    wr_XC7011_Reg(0x0214, 0x0);
    wr_XC7011_Reg(0x0215, 0x0);
    wr_XC7011_Reg(0x0216, 0x0);
    wr_XC7011_Reg(0x0217, 0x3);
    wr_XC7011_Reg(0x0218, 0x6);
    wr_XC7011_Reg(0x0219, 0x3);
    wr_XC7011_Reg(0x021a, 0x2);
    wr_XC7011_Reg(0x021b, 0x2);
    wr_XC7011_Reg(0x021c, 0x4);
    wr_XC7011_Reg(0x021d, 0x7);
    wr_XC7011_Reg(0x021e, 0x10);
    wr_XC7011_Reg(0x021f, 0xc);
    wr_XC7011_Reg(0x0220, 0xb);
    wr_XC7011_Reg(0x0221, 0xb);
    wr_XC7011_Reg(0x0222, 0xd);
    wr_XC7011_Reg(0x0223, 0x12);
    wr_XC7011_Reg(0x0224, 0x2);
    wr_XC7011_Reg(0x0225, 0x22);
    wr_XC7011_Reg(0x0226, 0x12);
    wr_XC7011_Reg(0x0227, 0x2);
    wr_XC7011_Reg(0x0228, 0x13);
    wr_XC7011_Reg(0x022a, 0x22);
    wr_XC7011_Reg(0x022b, 0x22);
    wr_XC7011_Reg(0x022c, 0x32);
    wr_XC7011_Reg(0x022d, 0x22);
    wr_XC7011_Reg(0x022e, 0x12);
    wr_XC7011_Reg(0x0230, 0x22);
    wr_XC7011_Reg(0x0231, 0x21);
    wr_XC7011_Reg(0x0232, 0x21);
    wr_XC7011_Reg(0x0233, 0x21);
    wr_XC7011_Reg(0x0234, 0x22);
    wr_XC7011_Reg(0x0236, 0x22);
    wr_XC7011_Reg(0x0237, 0x22);
    wr_XC7011_Reg(0x0238, 0x21);
    wr_XC7011_Reg(0x0239, 0x22);
    wr_XC7011_Reg(0x023a, 0x12);
    wr_XC7011_Reg(0x023c, 0x3);
    wr_XC7011_Reg(0x023d, 0x13);
    wr_XC7011_Reg(0x023e, 0x13);
    wr_XC7011_Reg(0x023f, 0x13);
    wr_XC7011_Reg(0x0240, 0x3);
    wr_XC7011_Reg(0x0248, 0xef);

    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x024d, 0x1);
    wr_XC7011_Reg(0x024e, 0x33);
    wr_XC7011_Reg(0x024f, 0x2);
    wr_XC7011_Reg(0x250, 0x22);
    wr_XC7011_Reg(0x251, 0x1);
    wr_XC7011_Reg(0x252, 0x99);
    wr_XC7011_Reg(0x253, 0x1);
    wr_XC7011_Reg(0x254, 0x6C);
    wr_XC7011_Reg(0x0013, 0x0d);	  	//LENC END 0D


    wr_XC7011_Reg(0xfffd, 0x80);	//AWB
    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0xcf);

// txt部分
    wr_XC7011_Reg(0x0730, 0x5d); // win1 startx
    wr_XC7011_Reg(0x0731, 0x89); // win1 endx
    wr_XC7011_Reg(0x0732, 0x36); // win1 starty
    wr_XC7011_Reg(0x0733, 0x4d); // win1 endy
    wr_XC7011_Reg(0x0734, 0x5a); // win2 startx
    wr_XC7011_Reg(0x0735, 0x80); // win2 endx
    wr_XC7011_Reg(0x0736, 0x4f); // win2 starty
    wr_XC7011_Reg(0x0737, 0x6e); // win2 endy
    wr_XC7011_Reg(0x0738, 0x3a); // win3 startx
    wr_XC7011_Reg(0x0739, 0x57); // win3 endx
    wr_XC7011_Reg(0x073a, 0x53); // win3 starty
    wr_XC7011_Reg(0x073b, 0x84); // win3 endy
    wr_XC7011_Reg(0x073c, 0x56); // win4 startx
    wr_XC7011_Reg(0x073d, 0x65); // win4 endx
    wr_XC7011_Reg(0x073e, 0x5f); // win4 starty
    wr_XC7011_Reg(0x073f, 0x87); // win4 endy
    wr_XC7011_Reg(0x0740, 0x53); // win5 startx
    wr_XC7011_Reg(0x0741, 0x76); // win5 endx
    wr_XC7011_Reg(0x0742, 0x42); // win5 starty
    wr_XC7011_Reg(0x0743, 0x54); // win5 endy
    wr_XC7011_Reg(0x0744, 0x45); // win6 startx
    wr_XC7011_Reg(0x0745, 0x59); // win6 endx
    wr_XC7011_Reg(0x0746, 0x83); // win6 starty
    wr_XC7011_Reg(0x0747, 0x91); // win6 endy
    wr_XC7011_Reg(0x0748, 0x00); // win7 startx
    wr_XC7011_Reg(0x0749, 0x00); // win7 endx
    wr_XC7011_Reg(0x074a, 0x00); // win7 starty
    wr_XC7011_Reg(0x074b, 0x00); // win7 endy
    wr_XC7011_Reg(0x074c, 0x00); // win8 startx
    wr_XC7011_Reg(0x074d, 0x00); // win8 endx
    wr_XC7011_Reg(0x074e, 0x00); // win8 starty
    wr_XC7011_Reg(0x074f, 0x00); // win8 endy
    wr_XC7011_Reg(0x0750, 0x00); // win9 startx
    wr_XC7011_Reg(0x0751, 0x00); // win9 endx
    wr_XC7011_Reg(0x0752, 0x00); // win9 starty
    wr_XC7011_Reg(0x0753, 0x00); // win9 endy
    wr_XC7011_Reg(0x0754, 0x00); // win10 startx
    wr_XC7011_Reg(0x0755, 0x00); // win10 endx
    wr_XC7011_Reg(0x0756, 0x00); // win10 starty
    wr_XC7011_Reg(0x0757, 0x00); // win10 endy
    wr_XC7011_Reg(0x0758, 0x00); // win11 startx
    wr_XC7011_Reg(0x0759, 0x00); // win11 endx
    wr_XC7011_Reg(0x075a, 0x00); // win11 starty
    wr_XC7011_Reg(0x075b, 0x00); // win11 endy
    wr_XC7011_Reg(0x075c, 0x00); // win12 startx
    wr_XC7011_Reg(0x075d, 0x00); // win12 endx
    wr_XC7011_Reg(0x075e, 0x00); // win12 starty
    wr_XC7011_Reg(0x075f, 0x00); // win12 endy
    wr_XC7011_Reg(0x0760, 0x00); // win13 startx
    wr_XC7011_Reg(0x0761, 0x00); // win13 endx
    wr_XC7011_Reg(0x0762, 0x00); // win13 starty
    wr_XC7011_Reg(0x0763, 0x00); // win13 endy
    wr_XC7011_Reg(0x0764, 0x00); // win14 startx
    wr_XC7011_Reg(0x0765, 0x00); // win14 endx
    wr_XC7011_Reg(0x0766, 0x00); // win14 starty
    wr_XC7011_Reg(0x0767, 0x00); // win14 endy
    wr_XC7011_Reg(0x0768, 0x00); // win15 startx
    wr_XC7011_Reg(0x0769, 0x00); // win15 endx
    wr_XC7011_Reg(0x076a, 0x00); // win15 starty
    wr_XC7011_Reg(0x076b, 0x00); // win15 endy
    wr_XC7011_Reg(0x076c, 0x00); // win16 startx
    wr_XC7011_Reg(0x076d, 0x00); // win16 endx
    wr_XC7011_Reg(0x076e, 0x00); // win16 starty
    wr_XC7011_Reg(0x076f, 0x00); // win16 endy
    wr_XC7011_Reg(0x0770, 0x22); // wt1 wt2
    wr_XC7011_Reg(0x0771, 0x21); // wt3 wt4
    wr_XC7011_Reg(0x0772, 0x21); // wt5 wt6
    wr_XC7011_Reg(0x0773, 0x00); // wt7 wt8
    wr_XC7011_Reg(0x0774, 0x00); // wt9 wt10
    wr_XC7011_Reg(0x0775, 0x00); // wt11 wt12
    wr_XC7011_Reg(0x0776, 0x00); // wt13 wt14
    wr_XC7011_Reg(0x0777, 0x00); // wt15 wt16

//txtend

    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x013c, 0x02);
    wr_XC7011_Reg(0x0176, 0x06);
    wr_XC7011_Reg(0x0177, 0x00);
    wr_XC7011_Reg(0x017a, 0x04);
    wr_XC7011_Reg(0x017b, 0x00);
    wr_XC7011_Reg(0x017e, 0x04);
    wr_XC7011_Reg(0x017f, 0x00);
    wr_XC7011_Reg(0x0182, 0x04);
    wr_XC7011_Reg(0x0183, 0x04);
    wr_XC7011_Reg(0x01aa, 0x05);
    wr_XC7011_Reg(0x01ab, 0x00);
    wr_XC7011_Reg(0x01ae, 0x04);
    wr_XC7011_Reg(0x01af, 0x00);
    wr_XC7011_Reg(0x01b2, 0x05);
    wr_XC7011_Reg(0x01b3, 0xa0);


    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x0027, 0x01);
    wr_XC7011_Reg(0x013c, 0x01);
    wr_XC7011_Reg(0x013d, 0x01);
    wr_XC7011_Reg(0x013e, 0x00);
    wr_XC7011_Reg(0x0170, 0x0d);
    wr_XC7011_Reg(0x0171, 0xff);
    wr_XC7011_Reg(0x016e, 0x08);


    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0708, 0x03);
    wr_XC7011_Reg(0x0709, 0xf0);
    wr_XC7011_Reg(0x070a, 0x00);
    wr_XC7011_Reg(0x070b, 0x0c);
    wr_XC7011_Reg(0x0001, 0x83);
    wr_XC7011_Reg(0x0003, 0xe5);
    wr_XC7011_Reg(0x0096, 0x83);
    wr_XC7011_Reg(0x019c, 0x0a);
    wr_XC7011_Reg(0x0019, 0x48);
    wr_XC7011_Reg(0x071c, 0x0a);  //简单白平衡；1a/0a

    wr_XC7011_Reg(0xfffd, 0x80); //Nathan5
    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x1400, 0x00);
    wr_XC7011_Reg(0x1401, 0x05);
    wr_XC7011_Reg(0x1402, 0x0a);
    wr_XC7011_Reg(0x1403, 0x0f);
    wr_XC7011_Reg(0x1404, 0x15);
    wr_XC7011_Reg(0x1405, 0x1a);
    wr_XC7011_Reg(0x1406, 0x1f);
    wr_XC7011_Reg(0x1407, 0x24);
    wr_XC7011_Reg(0x1408, 0x29);
    wr_XC7011_Reg(0x1409, 0x2e);
    wr_XC7011_Reg(0x140a, 0x32);
    wr_XC7011_Reg(0x140b, 0x37);
    wr_XC7011_Reg(0x140c, 0x3c);
    wr_XC7011_Reg(0x140d, 0x40);
    wr_XC7011_Reg(0x140e, 0x45);
    wr_XC7011_Reg(0x140f, 0x49);
    wr_XC7011_Reg(0x1410, 0x4e);
    wr_XC7011_Reg(0x1411, 0x52);
    wr_XC7011_Reg(0x1412, 0x56);
    wr_XC7011_Reg(0x1413, 0x5a);
    wr_XC7011_Reg(0x1414, 0x5e);
    wr_XC7011_Reg(0x1415, 0x62);
    wr_XC7011_Reg(0x1416, 0x65);
    wr_XC7011_Reg(0x1417, 0x69);
    wr_XC7011_Reg(0x1418, 0x6c);
    wr_XC7011_Reg(0x1419, 0x6f);
    wr_XC7011_Reg(0x141a, 0x73);
    wr_XC7011_Reg(0x141b, 0x76);
    wr_XC7011_Reg(0x141c, 0x79);
    wr_XC7011_Reg(0x141d, 0x7c);
    wr_XC7011_Reg(0x141e, 0x7e);
    wr_XC7011_Reg(0x141f, 0x81);
    wr_XC7011_Reg(0x1420, 0x84);
    wr_XC7011_Reg(0x1421, 0x89);
    wr_XC7011_Reg(0x1422, 0x8d);
    wr_XC7011_Reg(0x1423, 0x92);
    wr_XC7011_Reg(0x1424, 0x96);
    wr_XC7011_Reg(0x1425, 0x9a);
    wr_XC7011_Reg(0x1426, 0x9d);
    wr_XC7011_Reg(0x1427, 0xa1);
    wr_XC7011_Reg(0x1428, 0xa4);
    wr_XC7011_Reg(0x1429, 0xa7);
    wr_XC7011_Reg(0x142a, 0xaa);
    wr_XC7011_Reg(0x142b, 0xad);
    wr_XC7011_Reg(0x142c, 0xaf);
    wr_XC7011_Reg(0x142d, 0xb2);
    wr_XC7011_Reg(0x142e, 0xb4);
    wr_XC7011_Reg(0x142f, 0xb6);
    wr_XC7011_Reg(0x1430, 0xb8);
    wr_XC7011_Reg(0x1431, 0xbc);
    wr_XC7011_Reg(0x1432, 0xc0);
    wr_XC7011_Reg(0x1433, 0xc4);
    wr_XC7011_Reg(0x1434, 0xc8);
    wr_XC7011_Reg(0x1435, 0xcc);
    wr_XC7011_Reg(0x1436, 0xd0);
    wr_XC7011_Reg(0x1437, 0xd4);
    wr_XC7011_Reg(0x1438, 0xd8);
    wr_XC7011_Reg(0x1439, 0xdd);
    wr_XC7011_Reg(0x143a, 0xe1);
    wr_XC7011_Reg(0x143b, 0xe6);
    wr_XC7011_Reg(0x143c, 0xeb);
    wr_XC7011_Reg(0x143d, 0xf0);
    wr_XC7011_Reg(0x143e, 0xf5);
    wr_XC7011_Reg(0x143f, 0xfa);
    wr_XC7011_Reg(0x1440, 0xff);		//Gamma end

    wr_XC7011_Reg(0x1200, 0x3);			//CMX
    wr_XC7011_Reg(0x1201, 0xD6);
    wr_XC7011_Reg(0x1202, 0x0);
    wr_XC7011_Reg(0x1203, 0x9);
    wr_XC7011_Reg(0x1204, 0x3);
    wr_XC7011_Reg(0x1205, 0x40);
    wr_XC7011_Reg(0x1206, 0x1);
    wr_XC7011_Reg(0x1207, 0x57);
    wr_XC7011_Reg(0x1208, 0x0);
    wr_XC7011_Reg(0x1209, 0xC3);
    wr_XC7011_Reg(0x120A, 0x1);
    wr_XC7011_Reg(0x120B, 0x1D);
    wr_XC7011_Reg(0x120C, 0x7);
    wr_XC7011_Reg(0x120D, 0x9F);
    wr_XC7011_Reg(0x120E, 0x1);
    wr_XC7011_Reg(0x120F, 0x60);
    wr_XC7011_Reg(0x1210, 0x4);
    wr_XC7011_Reg(0x1211, 0x33);
    wr_XC7011_Reg(0x1212, 0x2);
    wr_XC7011_Reg(0x1213, 0xBA);
    wr_XC7011_Reg(0x1214, 0x3);
    wr_XC7011_Reg(0x1215, 0x7D);
    wr_XC7011_Reg(0x1216, 0x3);
    wr_XC7011_Reg(0x1217, 0xDE);
    wr_XC7011_Reg(0x1218, 0x5);
    wr_XC7011_Reg(0x1219, 0x96);
    wr_XC7011_Reg(0x121A, 0x0);
    wr_XC7011_Reg(0x121B, 0x66);
    wr_XC7011_Reg(0x121C, 0x1);
    wr_XC7011_Reg(0x121D, 0xCC);
    wr_XC7011_Reg(0x121E, 0x2);
    wr_XC7011_Reg(0x121F, 0x36);
    wr_XC7011_Reg(0x1220, 0x0);
    wr_XC7011_Reg(0x1221, 0xB7);
    wr_XC7011_Reg(0x1222, 0x2);
    wr_XC7011_Reg(0x1223, 0xEF);
    wr_XC7011_Reg(0x122e, 0x10);
    wr_XC7011_Reg(0x122F, 0x1);
    wr_XC7011_Reg(0x1230, 0x2);
    wr_XC7011_Reg(0x1228, 0x0);
    wr_XC7011_Reg(0x1229, 0x56);
    wr_XC7011_Reg(0x122A, 0x0);
    wr_XC7011_Reg(0x122B, 0x99);
    wr_XC7011_Reg(0x122C, 0x0);
    wr_XC7011_Reg(0x122D, 0xED);		//CMX end


    wr_XC7011_Reg(0xfffe, 0x30);  //YEE
    wr_XC7011_Reg(0x2000, 0x3f); //3f  //bit[5]Yedge en; bit[3]UV_dns_en; bit[2]pre_dns_en; bit[1]ydns_man; bit[0]uvdns_man
    wr_XC7011_Reg(0x2001, 0x00); //edge_ratio
    wr_XC7011_Reg(0x2002, 0x07); //Y_dns_level_man
    wr_XC7011_Reg(0x2003, 0x02); //UV_dns_level_man
    wr_XC7011_Reg(0x2004, 0x01);   //gain list0
    wr_XC7011_Reg(0x2005, 0x01);
    wr_XC7011_Reg(0x2006, 0x02);
    wr_XC7011_Reg(0x2007, 0x03);
    wr_XC7011_Reg(0x2008, 0x04);
    wr_XC7011_Reg(0x2009, 0x05);
    wr_XC7011_Reg(0x200a, 0x06);
    wr_XC7011_Reg(0x200b, 0x1f);  //gain list7

    wr_XC7011_Reg(0x1907, 0xd2); //y_win+2
    wr_XC7011_Reg(0x1908, 0x01); //win_man_en

    wr_XC7011_Reg(0xfffe, 0x30);  //RAW_DNS
    wr_XC7011_Reg(0x0e00, 0x34); //34 //bit[5] Para_man_en  bit[4]G_en
    wr_XC7011_Reg(0x0e01, 0x08); //06 //noise list0
    wr_XC7011_Reg(0x0e02, 0x0f); //0c
    wr_XC7011_Reg(0x0e03, 0x14); //10
    wr_XC7011_Reg(0x0e04, 0x24); //20
    wr_XC7011_Reg(0x0e05, 0x34); //30
    wr_XC7011_Reg(0x0e06, 0x38);
    wr_XC7011_Reg(0x0e07, 0x38);
    wr_XC7011_Reg(0x0e08, 0x38);   //noise list7
    wr_XC7011_Reg(0x0e09, 0x3f);   //max_edge_thre
    wr_XC7011_Reg(0x0e0a, 0x09); //noise_man
    wr_XC7011_Reg(0x0e0b, 0x20); //edgethre_man

    wr_XC7011_Reg(0xfffe, 0x30);  //RGB_DNS
    wr_XC7011_Reg(0x130e, 0x02); //bit[3] Para_man_en  bit[2:0]Y_noise_man
    wr_XC7011_Reg(0x130f, 0x08); // bit[5:0]UV_noise_man

    wr_XC7011_Reg(0xfffe, 0x30);  //cip
    wr_XC7011_Reg(0x0f00, 0xf0);  //bit[7:4] noise_y_slp  bit[3:0] Lsharp
    wr_XC7011_Reg(0x0f01, 0x00);
    wr_XC7011_Reg(0x0f02, 0x00); //noise list0
    wr_XC7011_Reg(0x0f03, 0x00);
    wr_XC7011_Reg(0x0f04, 0x00);
    wr_XC7011_Reg(0x0f05, 0x00);
    wr_XC7011_Reg(0x0f06, 0x00);
    wr_XC7011_Reg(0x0f07, 0x00);
    wr_XC7011_Reg(0x0f08, 0x00);
    wr_XC7011_Reg(0x0f09, 0x00);		//noise list7
    wr_XC7011_Reg(0x0f0a, 0x00);		//min_shp
    wr_XC7011_Reg(0x0f0b, 0x00);		//max_shp
    wr_XC7011_Reg(0x0f0c, 0x00); 		//min_detail
    wr_XC7011_Reg(0x0f0d, 0x00);		//max_detail
    wr_XC7011_Reg(0x0f0e, 0x00);  //min_shp_gain
    wr_XC7011_Reg(0x0f0f, 0x00);  //max_shp_gain

    wr_XC7011_Reg(0xfffe, 0x14);		//saturation adjust
    wr_XC7011_Reg(0x0160, 0x00);		//bAutoSat_en
    wr_XC7011_Reg(0x0162, 0x40);		//sat_u[0]
    wr_XC7011_Reg(0x0163, 0x38);		//sat_u[1]
    wr_XC7011_Reg(0x0164, 0x30);		//sat_u[2]
    wr_XC7011_Reg(0x0165, 0x2c);		//sat_u[3]
    wr_XC7011_Reg(0x0166, 0x28);		//sat_u[4]
    wr_XC7011_Reg(0x0167, 0x24);		//sat_u[5]
    wr_XC7011_Reg(0x0168, 0x44);		//sat_v[0]
    wr_XC7011_Reg(0x0169, 0x3c);		//sat_v[1]
    wr_XC7011_Reg(0x016a, 0x34);		//sat_v[2]
    wr_XC7011_Reg(0x016b, 0x30);		//sat_v[3]
    wr_XC7011_Reg(0x016c, 0x2c);		//sat_v[4]
    wr_XC7011_Reg(0x016d, 0x28);		//sat_v[5]

//TOP
    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0xcf);
    wr_XC7011_Reg(0x0001, 0xb3); //a3
    wr_XC7011_Reg(0x0003, 0xe5);
    wr_XC7011_Reg(0x071c, 0x0a);
    wr_XC7011_Reg(0x1700, 0x09);
    wr_XC7011_Reg(0x1701, 0x40);
    wr_XC7011_Reg(0x1702, 0x40);
    wr_XC7011_Reg(0x1704, 0x26);

}
void xc7011_day_int()
{

    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x00aa, 0x01);
    wr_XC7011_Reg(0x00ab, 0xc0);  //max gain
    wr_XC7011_Reg(0x0060, 0x00);
    wr_XC7011_Reg(0x006c, 0x01);
    wr_XC7011_Reg(0x006d, 0x90);
    wr_XC7011_Reg(0x0071, 0xc0);
    wr_XC7011_Reg(0x0092, 0x45);  //max exposure   //2d
    wr_XC7011_Reg(0x0093, 0x00);


    //AE window
    wr_XC7011_Reg(0x003f, 0x18);
    wr_XC7011_Reg(0x0040, 0x18);
    wr_XC7011_Reg(0x0041, 0x18);
    wr_XC7011_Reg(0x0042, 0x18);
    wr_XC7011_Reg(0x0043, 0x18);

    wr_XC7011_Reg(0x0044, 0x18);
    wr_XC7011_Reg(0x0045, 0x18);
    wr_XC7011_Reg(0x0046, 0x18);
    wr_XC7011_Reg(0x0047, 0x18);
    wr_XC7011_Reg(0x0048, 0x18);

    wr_XC7011_Reg(0x0049, 0x18);
    wr_XC7011_Reg(0x004a, 0x60);
    wr_XC7011_Reg(0x004b, 0x60);
    wr_XC7011_Reg(0x004c, 0x60);
    wr_XC7011_Reg(0x004d, 0x30);

    wr_XC7011_Reg(0x004e, 0x10);
    wr_XC7011_Reg(0x004f, 0x60);
    wr_XC7011_Reg(0x0050, 0x60);
    wr_XC7011_Reg(0x0051, 0x60);
    wr_XC7011_Reg(0x0052, 0x10);

    wr_XC7011_Reg(0x0053, 0x20);
    wr_XC7011_Reg(0x0054, 0x20);
    wr_XC7011_Reg(0x0055, 0x20);
    wr_XC7011_Reg(0x0056, 0x20);
    wr_XC7011_Reg(0x0057, 0x20);

    wr_XC7011_Reg(0x005c, 0x00);
    wr_XC7011_Reg(0x005d, 0x00);
    wr_XC7011_Reg(0x005e, 0x00);			//attention block cfg
    wr_XC7011_Reg(0x005f, 0x00);
    wr_XC7011_Reg(0x003a, 0x01);

    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0x4f); //4f
    wr_XC7011_Reg(0x0001, 0xa1);
    wr_XC7011_Reg(0x0013, 0x20); //20
    wr_XC7011_Reg(0x130e, 0x00);
    wr_XC7011_Reg(0x130f, 0x00);
    wr_XC7011_Reg(0x0e00, 0x34);
    wr_XC7011_Reg(0x0e0a, 0x00);
    wr_XC7011_Reg(0x0e0b, 0x78);
    wr_XC7011_Reg(0x2000, 0x3f);
    wr_XC7011_Reg(0x2001, 0x00); //0x13
    wr_XC7011_Reg(0x2002, 0x04); //Y_dns_level_man
    wr_XC7011_Reg(0x2003, 0x02); //UV_dns_level_man

    wr_XC7011_Reg(0x0f00, 0x00);
    wr_XC7011_Reg(0x0f0a, 0x00); //min_shp     //0x20
    wr_XC7011_Reg(0x0f0b, 0x00); //max_shp     //0x30
    wr_XC7011_Reg(0x0f0c, 0x00); //min_detail
    wr_XC7011_Reg(0x0f0d, 0x00); //max_detail
    wr_XC7011_Reg(0x0f0e, 0x01);  //min_shp_gain
    wr_XC7011_Reg(0x0f0f, 0x0f);  //max_shp_gain

    wr_XC7011_Reg(0x1701, 0xa8);	//x070
    wr_XC7011_Reg(0x1702, 0xa0);	//{wr_XC7011_Reg(0x70
    wr_XC7011_Reg(0x1704, 0x1e); //21


    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0004, 0x00);
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x1300, 0x03); //写入寄存的个数
    wr_XC7011_Reg(0x1301, 0x32); //第一个寄存器地址H
    wr_XC7011_Reg(0x1302, 0x0e); //第一个寄存器地址L
    wr_XC7011_Reg(0x1303, 0x02); //2 //第一个寄存器的值
    wr_XC7011_Reg(0x1304, 0x32); //第二个寄存器地址H
    wr_XC7011_Reg(0x1305, 0x0f); //第二个寄存器地址L
    wr_XC7011_Reg(0x1306, 0xee); //ee //第二个寄存器的值
    wr_XC7011_Reg(0x1307, 0x39); //第二个寄存器地址
    wr_XC7011_Reg(0x1308, 0x08); //第二个寄存器地址L
    wr_XC7011_Reg(0x1309, 0x13); //第二个寄存器的值blc

    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0007, 0x60); //sensor I2C地址
    wr_XC7011_Reg(0x000d, 0x31); //[7:4]速度[3:0]模式
    wr_XC7011_Reg(0x0009, 0x00);
    wr_XC7011_Reg(0x00c4, 0x10);
    wr_XC7011_Reg(0x00c0, 0x01);

}

void xc7011_night_int()
{

    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x00aa, 0x01);
    wr_XC7011_Reg(0x00ab, 0xc0);  //max gain
    wr_XC7011_Reg(0x0060, 0x00);
    wr_XC7011_Reg(0x006c, 0x01);	//0x00	//gain
    wr_XC7011_Reg(0x006d, 0x40);	//0xf0
    wr_XC7011_Reg(0x0071, 0xc0);	//0xf0
    wr_XC7011_Reg(0x0092, 0x45); //maxexposure//2d
    wr_XC7011_Reg(0x0093, 0x00);


    wr_XC7011_Reg(0x003f, 0x10);
    wr_XC7011_Reg(0x0040, 0x10);
    wr_XC7011_Reg(0x0041, 0x10);
    wr_XC7011_Reg(0x0042, 0x10);
    wr_XC7011_Reg(0x0043, 0x10);

    wr_XC7011_Reg(0x0044, 0x10);
    wr_XC7011_Reg(0x0045, 0x10);
    wr_XC7011_Reg(0x0046, 0x10);
    wr_XC7011_Reg(0x0047, 0x10);
    wr_XC7011_Reg(0x0048, 0x10);

    wr_XC7011_Reg(0x0049, 0x10);
    wr_XC7011_Reg(0x004a, 0x60);
    wr_XC7011_Reg(0x004b, 0x80);
    wr_XC7011_Reg(0x004c, 0x60);
    wr_XC7011_Reg(0x004d, 0x10);

    wr_XC7011_Reg(0x004e, 0x10);
    wr_XC7011_Reg(0x004f, 0xf0);
    wr_XC7011_Reg(0x0050, 0xf0);
    wr_XC7011_Reg(0x0051, 0xf0);
    wr_XC7011_Reg(0x0052, 0x18);

    wr_XC7011_Reg(0x0053, 0x10);
    wr_XC7011_Reg(0x0054, 0x10);
    wr_XC7011_Reg(0x0055, 0x10);
    wr_XC7011_Reg(0x0056, 0x10);
    wr_XC7011_Reg(0x0057, 0x10);

    wr_XC7011_Reg(0x005c, 0x00);
    wr_XC7011_Reg(0x005d, 0x07);
    wr_XC7011_Reg(0x005e, 0x38);			//attentionblockcfg
    wr_XC7011_Reg(0x005f, 0x00);
    wr_XC7011_Reg(0x003a, 0x01); //单次有效，设置之后权重和关注区域生效

    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0x4f); //4f
    wr_XC7011_Reg(0x0001, 0xa1);
    wr_XC7011_Reg(0x0013, 0x20); //20
    wr_XC7011_Reg(0x130e, 0x00);
    wr_XC7011_Reg(0x130f, 0x00);
    wr_XC7011_Reg(0x0e00, 0x34);
    wr_XC7011_Reg(0x0e0a, 0x00);
    wr_XC7011_Reg(0x0e0b, 0x58);
    wr_XC7011_Reg(0x2000, 0x3f);
    wr_XC7011_Reg(0x2001, 0x00);
    wr_XC7011_Reg(0x2002, 0x04); //Y_dns_level_man
    wr_XC7011_Reg(0x2003, 0x02); //UV_dns_level_man

    wr_XC7011_Reg(0x0f00, 0x00);
    wr_XC7011_Reg(0x0f02, 0x01);
    wr_XC7011_Reg(0x0f0a, 0x01); //min_shp
    wr_XC7011_Reg(0x0f0b, 0x03); //max_shp
    wr_XC7011_Reg(0x0f0c, 0x00); //min_detail
    wr_XC7011_Reg(0x0f0d, 0x00); //max_detail
    wr_XC7011_Reg(0x0f0e, 0x01); //min_shp_gain
    wr_XC7011_Reg(0x0f0f, 0x0f); //max_shp_gain

    wr_XC7011_Reg(0x1701, 0xa0);	//x070
    wr_XC7011_Reg(0x1702, 0xa0);	//{wr_XC7011_Reg(0x70
    wr_XC7011_Reg(0x1704, 0x24); //24



    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0004, 0x00);
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x1300, 0x03); //写入寄存的个数
    wr_XC7011_Reg(0x1301, 0x32); //第一个寄存器地址H
    wr_XC7011_Reg(0x1302, 0x0e); //第一个寄存器地址L
    wr_XC7011_Reg(0x1303, 0x02); //02//第一个寄存器的值
    wr_XC7011_Reg(0x1304, 0x32); //第二个寄存器地址H
    wr_XC7011_Reg(0x1305, 0x0f); //第二个寄存器地址L
    wr_XC7011_Reg(0x1306, 0xee); //ee//第二个寄存器的值
    wr_XC7011_Reg(0x1307, 0x39); //第二个寄存器地址
    wr_XC7011_Reg(0x1308, 0x08); //第二个寄存器地址L
    wr_XC7011_Reg(0x1309, 0x13); //第二个寄存器的值blc

    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0007, 0x60); //sensorI2C地址
    wr_XC7011_Reg(0x000d, 0x31); //[7:4]速度[3:0]模式
    wr_XC7011_Reg(0x0009, 0x00);
    wr_XC7011_Reg(0x00c4, 0x10);
    wr_XC7011_Reg(0x00c0, 0x01);

}

void xc7011_lata_night_int()
{

    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x0060, 0x00);
    wr_XC7011_Reg(0x006c, 0x01);	//0x00	//gain
    wr_XC7011_Reg(0x006d, 0xf0);	//0xf0
    wr_XC7011_Reg(0x0071, 0xc0);	//0xf0
    wr_XC7011_Reg(0x0092, 0x4f); //maxexposure//2d
    wr_XC7011_Reg(0x0093, 0x00);


    wr_XC7011_Reg(0x003f, 0x10);
    wr_XC7011_Reg(0x0040, 0x10);
    wr_XC7011_Reg(0x0041, 0x10);
    wr_XC7011_Reg(0x0042, 0x10);
    wr_XC7011_Reg(0x0043, 0x10);

    wr_XC7011_Reg(0x0044, 0x10);
    wr_XC7011_Reg(0x0045, 0x10);
    wr_XC7011_Reg(0x0046, 0x20);
    wr_XC7011_Reg(0x0047, 0x10);
    wr_XC7011_Reg(0x0048, 0x10);

    wr_XC7011_Reg(0x0049, 0x10);
    wr_XC7011_Reg(0x004a, 0x20);
    wr_XC7011_Reg(0x004b, 0x40);
    wr_XC7011_Reg(0x004c, 0x20);
    wr_XC7011_Reg(0x004d, 0x10);

    wr_XC7011_Reg(0x004e, 0x10);
    wr_XC7011_Reg(0x004f, 0x40);
    wr_XC7011_Reg(0x0050, 0x60);
    wr_XC7011_Reg(0x0051, 0x40);
    wr_XC7011_Reg(0x0052, 0x18);

    wr_XC7011_Reg(0x0053, 0x18);
    wr_XC7011_Reg(0x0054, 0x30);
    wr_XC7011_Reg(0x0055, 0x40);
    wr_XC7011_Reg(0x0056, 0x30);
    wr_XC7011_Reg(0x0057, 0x18);

    wr_XC7011_Reg(0x005c, 0x00);
    wr_XC7011_Reg(0x005d, 0x07);
    wr_XC7011_Reg(0x005e, 0x38);			//attentionblockcfg
    wr_XC7011_Reg(0x005f, 0x00);
    wr_XC7011_Reg(0x003a, 0x01); //单次有效，设置之后权重和关注区域生效

    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0x4f); //4f
    wr_XC7011_Reg(0x0001, 0xa1);
    wr_XC7011_Reg(0x0013, 0x20); //20
    wr_XC7011_Reg(0x130e, 0x00);
    wr_XC7011_Reg(0x130f, 0x00);
    wr_XC7011_Reg(0x0e00, 0x34);
    wr_XC7011_Reg(0x0e0a, 0x00);
    wr_XC7011_Reg(0x0e0b, 0x58);
    wr_XC7011_Reg(0x2000, 0x3f);
    wr_XC7011_Reg(0x2001, 0x00);
    wr_XC7011_Reg(0x2002, 0x04); //Y_dns_level_man
    wr_XC7011_Reg(0x2003, 0x02); //UV_dns_level_man

    wr_XC7011_Reg(0x0f00, 0x00);
    wr_XC7011_Reg(0x0f02, 0x01);
    wr_XC7011_Reg(0x0f0a, 0x01); //min_shp
    wr_XC7011_Reg(0x0f0b, 0x03); //max_shp
    wr_XC7011_Reg(0x0f0c, 0x00); //min_detail
    wr_XC7011_Reg(0x0f0d, 0x00); //max_detail
    wr_XC7011_Reg(0x0f0e, 0x01); //min_shp_gain
    wr_XC7011_Reg(0x0f0f, 0x0f); //max_shp_gain

    wr_XC7011_Reg(0x1701, 0xa0);	//x070
    wr_XC7011_Reg(0x1702, 0xa0);	//{wr_XC7011_Reg(0x70
    wr_XC7011_Reg(0x1704, 0x24); //24



    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0004, 0x00);
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x1300, 0x03); //写入寄存的个数
    wr_XC7011_Reg(0x1301, 0x32); //第一个寄存器地址H
    wr_XC7011_Reg(0x1302, 0x0e); //第一个寄存器地址L
    wr_XC7011_Reg(0x1303, 0x04); //02//第一个寄存器的值
    wr_XC7011_Reg(0x1304, 0x32); //第二个寄存器地址H
    wr_XC7011_Reg(0x1305, 0x0f); //第二个寄存器地址L
    wr_XC7011_Reg(0x1306, 0xee); //ee//第二个寄存器的值
    wr_XC7011_Reg(0x1307, 0x39); //第二个寄存器地址
    wr_XC7011_Reg(0x1308, 0x08); //第二个寄存器地址L
    wr_XC7011_Reg(0x1309, 0x13); //第二个寄存器的值blc

    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x0007, 0x60); //sensorI2C地址
    wr_XC7011_Reg(0x000d, 0x31); //[7:4]速度[3:0]模式
    wr_XC7011_Reg(0x0009, 0x00);
    wr_XC7011_Reg(0x00c4, 0x10);
    wr_XC7011_Reg(0x00c0, 0x01);

}



void XC7011_bypass_on()
{
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x004d, 0x01);
}

void XC7011_bypass_off()
{
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x50);
    wr_XC7011_Reg(0x004d, 0x00);
};


void XC7011_DVP_INT()
{
    wr_XC7011_Reg(0x0103, 0x01);
    wr_XC7011_Reg(0x0100, 0x00);

    wr_XC7011_Reg(0x4500, 0x51);

    wr_XC7011_Reg(0x320e, 0x02); //30fps  25:900 30:750
    wr_XC7011_Reg(0x320f, 0xee);

    wr_XC7011_Reg(0x3e01, 0x38);
    wr_XC7011_Reg(0x3635, 0xa0);

    wr_XC7011_Reg(0x3620, 0x28);
    wr_XC7011_Reg(0x3309, 0xa0);
    wr_XC7011_Reg(0x331f, 0x9d);
    wr_XC7011_Reg(0x3321, 0x9f);
    wr_XC7011_Reg(0x3306, 0x65);
    wr_XC7011_Reg(0x330b, 0x66);

    wr_XC7011_Reg(0x3e08, 0x03);
    wr_XC7011_Reg(0x3e07, 0x00);
    wr_XC7011_Reg(0x3e09, 0x10);

    wr_XC7011_Reg(0x3367, 0x18); //pchtx min

    wr_XC7011_Reg(0x335e, 0x01);  //ana dithering
    wr_XC7011_Reg(0x335f, 0x03);
    wr_XC7011_Reg(0x337c, 0x04);
    wr_XC7011_Reg(0x337d, 0x06);
    wr_XC7011_Reg(0x33a0, 0x05);
    wr_XC7011_Reg(0x3301, 0x05);
    wr_XC7011_Reg(0x3302, 0x7d);
    wr_XC7011_Reg(0x3633, 0x43);

    wr_XC7011_Reg(0x3908, 0x11);


    wr_XC7011_Reg(0x3621, 0x28);
    wr_XC7011_Reg(0x3622, 0x02);

//0714
    wr_XC7011_Reg(0x3637, 0x1c);
    wr_XC7011_Reg(0x3308, 0x1c);
    wr_XC7011_Reg(0x3367, 0x10);

    wr_XC7011_Reg(0x337f, 0x03); //new auto precharge  330e in 3372   [7:6] 11: close div_rst 00:open div_rst
    wr_XC7011_Reg(0x3368, 0x04);//auto precharge
    wr_XC7011_Reg(0x3369, 0x00);
    wr_XC7011_Reg(0x336a, 0x00);
    wr_XC7011_Reg(0x336b, 0x00);
    wr_XC7011_Reg(0x3367, 0x08);
    wr_XC7011_Reg(0x330e, 0x30);

    wr_XC7011_Reg(0x3366, 0x7c); // div_rst gap


//0805
    wr_XC7011_Reg(0x3635, 0xc4); //fpn
    wr_XC7011_Reg(0x3621, 0x20); //col_fpn
    wr_XC7011_Reg(0x3334, 0x40);
    wr_XC7011_Reg(0x3333, 0x10);
    wr_XC7011_Reg(0x3302, 0x3d);

//0807
    wr_XC7011_Reg(0x335e, 0x01);  //ana dithering
    wr_XC7011_Reg(0x335f, 0x02);
    wr_XC7011_Reg(0x337c, 0x03);
    wr_XC7011_Reg(0x337d, 0x05);
    wr_XC7011_Reg(0x33a0, 0x04);
    wr_XC7011_Reg(0x3301, 0x03);
    wr_XC7011_Reg(0x3633, 0x48);

    wr_XC7011_Reg(0x3670, 0x0a);  //[1]:3630 logic ctrl  real value in 3681
    wr_XC7011_Reg(0x367c, 0x07);  //gain0
    wr_XC7011_Reg(0x367d, 0x0f);  //gain1
    wr_XC7011_Reg(0x3674, 0x20);  //<gain0
    wr_XC7011_Reg(0x3675, 0x2f);  //gain0 - gain1
    wr_XC7011_Reg(0x3676, 0x2f);  //>gain1


    wr_XC7011_Reg(0x3631, 0x84);
    wr_XC7011_Reg(0x3622, 0x06);
    wr_XC7011_Reg(0x3630, 0x20);

    wr_XC7011_Reg(0x3620, 0x08); //col_fpn


//wr_XC7011_Reg(0x330b, 0x66);

    wr_XC7011_Reg(0x3638, 0x1f); //ramp by clk
    wr_XC7011_Reg(0x3625, 0x02);
    wr_XC7011_Reg(0x3637, 0x1e);
    wr_XC7011_Reg(0x3636, 0x25);

    wr_XC7011_Reg(0x3306, 0x1c);  //5.0k
    wr_XC7011_Reg(0x3637, 0x1a);
    wr_XC7011_Reg(0x331e, 0x0c);
    wr_XC7011_Reg(0x331f, 0x9c);
    wr_XC7011_Reg(0x3320, 0x0c);
    wr_XC7011_Reg(0x3321, 0x9c);

    wr_XC7011_Reg(0x366e, 0x08);  // ofs auto en [3]
    wr_XC7011_Reg(0x366f, 0x2a);  // ofs+finegain  real ofs in wr_XC7011_Reg(0x3687[4:0]

    wr_XC7011_Reg(0x3622, 0x16); //sig clamp
    wr_XC7011_Reg(0x363b, 0x0c); //hvdd

    wr_XC7011_Reg(0x3639, 0x0a);
    wr_XC7011_Reg(0x3632, 0x28);
    wr_XC7011_Reg(0x3038, 0x84);

//0814
    wr_XC7011_Reg(0x3635, 0xc0);

//0819

//digital ctrl

    wr_XC7011_Reg(0x3f00, 0x07);  // bit[2] = 1
    wr_XC7011_Reg(0x3f04, 0x02);
    wr_XC7011_Reg(0x3f05, 0xfc);  // hts / 2 - wr_XC7011_Reg(0x24

    wr_XC7011_Reg(0x3802, 0x01);
    wr_XC7011_Reg(0x3235, 0x07);
    wr_XC7011_Reg(0x3236, 0x06); // vts x 2 - 2

//0831
    wr_XC7011_Reg(0x3639, 0x09);
    wr_XC7011_Reg(0x3670, 0x02);

//0904
    wr_XC7011_Reg(0x3635, 0x80);

//0906
//wr_XC7011_Reg(0x320c, 0x05);
//wr_XC7011_Reg(0x320d, 0xa0);
//wr_XC7011_Reg(0x320e, 0x03);
//wr_XC7011_Reg(0x320f, 0xe8);
    wr_XC7011_Reg(0x320e, 0x02);
    wr_XC7011_Reg(0x320f, 0xee);
    wr_XC7011_Reg(0x3235, 0x05);
    wr_XC7011_Reg(0x3236, 0xda);

    wr_XC7011_Reg(0x3039, 0x51);
    wr_XC7011_Reg(0x303a, 0xba);
    wr_XC7011_Reg(0x3034, 0x06);
    wr_XC7011_Reg(0x3035, 0xe2);

//for AE control per frame
    wr_XC7011_Reg(0x3633, 0x53);
    wr_XC7011_Reg(0x3802, 0x00);

    wr_XC7011_Reg(0x3e03, 0x0b);//gain mode --->0908

    wr_XC7011_Reg(0x3640, 0x03);
    wr_XC7011_Reg(0x0100, 0x01);

#if ((FLIP_XC7011==1)/*&&(MIRROR_XC7011==1)*/)
    wr_XC7011_Reg(0x3221, 0x66);
    /*
    #elif ((FLIP_XC7011==1)&&(MIRROR_XC7011==0))
    wr_XC7011_Reg(0x3221, 0x06);
    #elif ((FLIP_XC7011==0)&&(MIRROR_XC7011==1))
    wr_XC7011_Reg(0x3221, 0x60);
    */
#else
    wr_XC7011_Reg(0x3221, 0x00);
#endif

};


void XD7011_Black(void)
{
    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0xcf);
    wr_XC7011_Reg(0x0001, 0xb3); //a3
    wr_XC7011_Reg(0x0003, 0xe5);
    wr_XC7011_Reg(0x071c, 0x0a);
    wr_XC7011_Reg(0x1700, 0x81); //81黑白  09彩色
    wr_XC7011_Reg(0x1701, 0x00);
    wr_XC7011_Reg(0x1702, 0x00);
    wr_XC7011_Reg(0x1704, 0x20);
    wr_XC7011_Reg(0x1707, 0x10);
    wr_XC7011_Reg(0xfffe, 0x14);
    wr_XC7011_Reg(0x00aa, 0x07);
    wr_XC7011_Reg(0x00ab, 0xc0);  //max gain
    wr_XC7011_Reg(0xfffe, 0x30);  //cip
    wr_XC7011_Reg(0x0f00, 0xf0);  //bit[7:4] noise_y_slp  bit[3:0] Lsharp
    wr_XC7011_Reg(0x0f01, 0x04);
    wr_XC7011_Reg(0x0f02, 0x18); //noise list0
    wr_XC7011_Reg(0x0f03, 0x1f);
    wr_XC7011_Reg(0x0f04, 0x28);
    wr_XC7011_Reg(0x0f05, 0x2f);
    wr_XC7011_Reg(0x0f06, 0x38);
    wr_XC7011_Reg(0x0f07, 0x3f);
    wr_XC7011_Reg(0x0f08, 0x48);
    wr_XC7011_Reg(0x0f09, 0x4f);		//noise list7

}

void XD7011_Color(void)
{
    wr_XC7011_Reg(0xfffe, 0x30);
    wr_XC7011_Reg(0x0000, 0xcf);
    wr_XC7011_Reg(0x0001, 0xb3); //a3
    wr_XC7011_Reg(0x0003, 0xe5);
    wr_XC7011_Reg(0x071c, 0x0a);
    wr_XC7011_Reg(0x1700, 0x09);
    wr_XC7011_Reg(0x1701, 0x40);
    wr_XC7011_Reg(0x1702, 0x40);
    wr_XC7011_Reg(0x1704, 0x20);
    wr_XC7011_Reg(0xfffe, 0x30);  //cip
    wr_XC7011_Reg(0x0f00, 0xf0);  //bit[7:4] noise_y_slp  bit[3:0] Lsharp
    wr_XC7011_Reg(0x0f01, 0x00);
    wr_XC7011_Reg(0x0f02, 0x00); //noise list0
    wr_XC7011_Reg(0x0f03, 0x00);
    wr_XC7011_Reg(0x0f04, 0x00);
    wr_XC7011_Reg(0x0f05, 0x00);
    wr_XC7011_Reg(0x0f06, 0x00);
    wr_XC7011_Reg(0x0f07, 0x00);
    wr_XC7011_Reg(0x0f08, 0x00);
    wr_XC7011_Reg(0x0f09, 0x00);		//noise list7

}
u8 xc7011_color = 1;
void XC7011_SCENE_SWITCH(void)
{
    static int g_cur_mode = -1;
    static u16 data_com = 0;
    u32 temp;
    u8 pid = 0x00;
    u8 ver = 0x00;
    wr_XC7011_Reg(0xfffd, 0x80);
    wr_XC7011_Reg(0xfffe, 0x14);
    rd_XC7011_Reg(0x0106, &pid);
    rd_XC7011_Reg(0x0107, &ver);
    data_com = (((0x00ff & pid) << 8) | (0x00ff & ver));
//    printf("g_cur_mode==== %d\n",g_cur_mode);
//    printf("data_com==== %x\n",data_com);
    temp = 1;//db_select("tvm");
    if (temp == 2) {
        if (data_com >= 0x1c0) {
            XD7011_Black();
            xc7011_color = 1;
        } else if (data_com <= 56) {
            XD7011_Color();
            xc7011_color = 2;
        }
    } else if (temp == 1) {
        if (xc7011_color != 3) {
            XD7011_Color();
            xc7011_color = 3;
        }
    } else if (temp == 0) {
        if (xc7011_color != 0) {
            XD7011_Color();
            xc7011_color = 0;
        }
    }
    if (g_cur_mode == -1)  {
        g_cur_mode = 0;
        //	xc7011_day_int();
    }
    if (data_com >= 0x0020 && data_com <= 0x0090) {
        if (g_cur_mode != 0) {
            g_cur_mode = 0;
            //   xc7011_day_int();
            // puts("\nSC1233_SCENE_SWITCH0000000000000000000000\n");
        }
    } else if (data_com >= 0x120 && data_com <= 0x0500) {
        if (g_cur_mode != 1) {
            g_cur_mode = 1;
            //   xc7011_night_int();
            // puts("\nSC1233_SCENE_SWITCH11111111111111111111\n");
        }
    } else if (data_com >= 0x600 && data_com <= 0x0800) {
        if (g_cur_mode != 2) {
            g_cur_mode = 2;
            // xc7011_lata_night_int();
            //  puts("\nSC1233_SCENE_SWITCH2222222222222222222222222\n");
        }
    }

}



void xc7011_reset(u8 isp_dev)
{
    u8 gpio;
    u8 pwdn_gpio;
    if (isp_dev == ISP_DEV_0) {
        gpio = reset_gpio[0];
        pwdn_gpio = pwdn_gpios[0];
    } else {
        gpio = reset_gpio[1];
        pwdn_gpio = pwdn_gpios[1];
    }


    if (pwdn_gpio != (u8) - 1) {
        gpio_direction_output(pwdn_gpio, 0);
    }
    if (gpio != (u8) - 1) {
        gpio_direction_output(gpio, 1);
        gpio_direction_output(gpio, 0);
        os_time_dly(1);
        gpio_direction_output(gpio, 1);
    }
}
s32 _xc7011_id_check(void)
{
    u8 id_hi;
    u8 id_lo;
    u32 i ;

    for (i = 0; i < 5; i++) { //

        rd_XC7011_Reg(0xfffb, &id_hi);
        rd_XC7011_Reg(0xfffc, &id_lo);

        if (id_hi == 0x70 && id_lo == 0x11) {
            puts("Sensor PID \n");
            put_u32hex(id_hi);
            put_u32hex(id_lo);
            puts("\n----hello XC7011-----\n");
            return 0;
        }
    }
    puts("\n----not XC7011-----\n");
    return -1;
}


static u8 cur_sensor_type = 0xff;
s32 xc7011_id_check(u8 isp_dev, u32 _reset_gpio, u32 _pwdn_gpio)
{
    puts("\n xc7011 id check\n");
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

    reset_gpio[isp_dev] = (u8)_reset_gpio;
    pwdn_gpios[isp_dev] = (u8)_pwdn_gpio;
    xc7011_reset(isp_dev);

    if (0 != _xc7011_id_check()) {
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    cur_sensor_type = isp_dev;

    return 0;

}
void XC7011_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    u32 i;
    u32 frame_exp_rows;
    u8 tmp;
//    for (i = 0; i < sizeof(XC7011_default_regs) / sizeof(Sensor_reg_ini); i++) {
//        wrXC7011Reg(XC7011_default_regs[i].addr, XC7011_default_regs[i].value);
//    }

//    for (i = 0; i < sizeof(bypass_on) / sizeof(Sensor_reg_ini); i++) {
//        wrXC7011Reg(bypass_on[i].addr, bypass_on[i].value);
//    }
//
//    for (i = 0; i < sizeof(H42_regs) / sizeof(Sensor_reg_inix); i++) {
//        wrH42Reg(H42_regs[i].addr, H42_regs[i].value);
//    }
//
//    for (i = 0; i < sizeof(bypass_off) / sizeof(Sensor_reg_ini); i++) {
//        wrXC7011Reg(bypass_off[i].addr, bypass_off[i].value);
//    }


    return;
}



s32 XC7011_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    //XC7011_config_SENSOR(width, height, format, frame_freq);
    //  XC7011_reset();
    xc7011_reset(isp_dev);
    XC7011_Init();
    XC7011_bypass_on();

    puts("\n\n XC7011_init \n\n");
    XC7011_DVP_INT();
    XC7011_bypass_off();
    /*sys_timer_add(NULL, XC7011_SCENE_SWITCH, 1000);*/
    return 0;
}

s32 xc7011_set_output_size(u16 *width, u16 *height, u8 *freq)
{

    return 0;
}


s32 xc7011_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}
u8 xc7011_fps()
{
    return 0;//30fps
}
u8 xc7011_valid_signal(void *p)
{
    return 1;//valid
}

static u8 xc7011_mode_det(void *p)
{
    return 0;
}

// *INDENT-OFF*
REGISTER_CAMERA(XC7011) = {
    .logo 				= 	"XC7011",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_YUYV,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    						SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_VSYNC_ACTIVE_LOW,
#if CONFIG_CAMERA_H_V_EXCHANGE
    .sync_config		=   SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,//WL82/AC791才可以H-V SYNC互换，请留意
#endif
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,
    .sen_size 			= 	{XC7011_DEVP_INPUT_W, XC7011_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{XC7011_DEVP_INPUT_W, XC7011_DEVP_INPUT_H},


    .ops                    = {
        .avin_fps           =   xc7011_fps,
        .avin_valid_signal  =   xc7011_valid_signal,
        .avin_mode_det      =   xc7011_mode_det,
        .sensor_check 		= 	xc7011_id_check,
        .init 		        = 	XC7011_init,
        .set_size_fps 		=	xc7011_set_output_size,
        .power_ctrl         =   xc7011_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	XC7011_dev_wr_reg,
        .read_reg 		    =	XC7011_dev_rd_reg,
        .set_sensor_reverse =   NULL,
    }
};









