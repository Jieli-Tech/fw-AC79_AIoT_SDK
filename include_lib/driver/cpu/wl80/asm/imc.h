#ifndef __IMC_H__
#define __IMC_H__




#define IMC_CMD_BASE                0x00100000
#define IMC_OPEN_OSD                (IMC_CMD_BASE + 1)
#define IMC_CLOSE_OSD               (IMC_CMD_BASE + 2)
#define IMC_SET_TLP_TIME            (IMC_CMD_BASE + 3)
#define IMC_SET_DISP_PRESCA         (IMC_CMD_BASE + 4)
#define IMC_SET_CROP        		(IMC_CMD_BASE + 5)
#define IMC_CROP_TRIG				(IMC_CMD_BASE + 6)
#define IMC_ICAP_OPEN_OSD           (IMC_CMD_BASE + 7)
#define IMC_SET_OSD_STR             (IMC_CMD_BASE + 8)
#define IMC_ENC_REP_CTRL            (IMC_CMD_BASE + 9)
#define IMC_KEEP_ENCODE_OSD         (IMC_CMD_BASE + 10)
#define IMC_RESET_TLP_TIME          (IMC_CMD_BASE + 11)


struct osd_icon_config {
    u32 color[3];
    u16 width;
    u16 height;
    u8  *buf;
    int buf_size;
};

struct imc_osd_info {
    u8 mode; //0 -- 1bit, 1 -- 2bit
    u16 x ;//起始地址
    u16 y ;//结束地址
    u32 osd_yuv;//osd颜色

    struct osd_icon_config icon;//
//注意：下面的字符串地址必须是全局的,然后年是yyyy，月是nn，日是dd，时是hh，分是mm，秒是ss,其他字符是英文字母&&符号&&汉字
    char *osd_str; //用户自定义格式，例如 "yyyy-nn-dd\hh:mm:ss" 或者 "hh:mm:ss"
    char *osd_matrix_str; //用户自定义字模字符串,例如“abcd....0123..”
    u8 *osd_matrix_base; //用户自定义字模的起始地址
    u32 osd_matrix_len;//用户自定义字模数组的长度,no str len!!!
    u8 osd_w;//用户自定义字体大小,8的倍数
    u8 osd_h;//8的倍数
    struct imc_osd_info *next;

} ;

struct imc_icap_osd_info {
    u16 width;
    u16 height;
    struct imc_osd_info *osd;
};

struct imc_presca_ctl {
    u8 presca_en;
    u32 gs_parma;
    u32 gs_parmb;
    u32 gs_parmc;
    u32 gs_parmd;
};

struct drop_fps {
    u32 fps_a;
    u32 fps_b;
};






#endif


