#ifndef __ISC_H__
#define __ISC_H__

#include "gpio.h"
#include "fs/fs.h"
#include "spinlock.h"
#include "asm/isp_dev.h"


/****** ISC sensor group_port ********/
#define ISC_GROUPA	IO_PORTA
#define ISC_GROUPC	IO_PORTC

/*************ie type ****************/
#define 	EXTERN_CONNECT_IE	BIT(9)
#define 	FSTART_IE			BIT(10)
#define 	FDONE_IE			BIT(13)
#define 	BDONE_IE			BIT(12)
#define 	LDONE_IE			BIT(14)
#define 	LINERR_IE			BIT(11)
#define 	LOUTERR_IE			BIT(15)
#define 	BWERR_IE			BIT(16)
#define 	SET_IE(ie_type) 	(JL_ISC->CON |= ie_type)
#define 	GET_IE(ie_type) 	(JL_ISC->CON & ie_type)
#define 	UNSET_IE(ie_type) 	(JL_ISC->CON &=~ie_type)

/********** pending type *************/
#define 	FSTART_PND			BIT(17)
#define 	FDONE_PND			BIT(23)
#define 	LINERR_PND			BIT(19)
#define 	LOUTERR_PND			BIT(27)
#define 	LDONE_PND			BIT(25)
#define 	BDONE_PND			BIT(21)
#define 	BWERR_PND			BIT(29)

/*************clear type**************/
#define 	FSTART_CLR			BIT(18)
#define 	FDONE_CLR			BIT(24)
#define 	LINERR_CLR			BIT(20)
#define 	LOUTERR_CLR			BIT(28)
#define 	LDONE_CLR			BIT(26)
#define 	BDONE_CLR			BIT(22)
#define 	BWERR_CLR			BIT(30)
#define 	CLEAR_PND(clr_type) (JL_ISC->CON |= clr_type)
#define 	CLEAR_OSD_PND() 	(JL_ISC->OSD_CON |= BIT(2))

/*************read pending*************/
#define 	IRQ_FSTART(r)		((r & FSTART_PND) 	&& (r & FSTART_IE))
#define 	IRQ_FDONE(r)		((r & FDONE_PND) 	&& (r & FDONE_IE))
#define 	IRQ_LDONE(r)		((r & LDONE_PND) 	&& (r & LDONE_IE))
#define 	IRQ_LINERR(r)		((r & LINERR_PND) 	&& (r & LINERR_IE))
#define 	IRQ_LOUTERR(r)		((r & LOUTERR_PND)	&& (r & LOUTERR_IE))
#define 	IRQ_BDONE(r)		((r & BDONE_PND) 	&& (r & BDONE_IE))
#define 	IRQ_BWERR(r)		((r & BWERR_PND)	&& (r & BWERR_IE))

#define 	IRQ_OSD_ERR()		(JL_ISC->OSD_CON & BIT(3))

/****************buf mode**************/
#define 	FRAME_BUF_MODE		0x0
#define 	CYCLE_BUF_MODE		0x1
#define 	REGISTER_DEBUG		0x2

typedef enum {
    ISC_DATAMODE_8B,
    ISC_DATAMODE_8B_REVERSE,
} isc_datamode_t;

typedef enum {
    ISC_INPUT_DATAMODE_YUYV = 0,
    ISC_INPUT_DATAMODE_UYVY,
} isc_input_datamode_t;

typedef enum {
    ISC_OUTPUT_DATAMODE_YUV422 = 2,
    ISC_OUTPUT_DATAMODE_YUV420 = 4,
} isc_output_datamode_t;

typedef enum {
    ISC_TIMINGMODE_DVP = 0,
    ISC_TIMINGMODE_ITU656,
    ISC_TIMINGMODE_ITU601,
    ISC_TIMINGMODE_BT656,
} isc_timingmode_t;

typedef enum {
    ISC_SCANMODE_PROGRESSIVE = 0,
    ISC_SCANMODE_ODD,
    ISC_SCANMODE_EVEN,
} isc_scanmode_t;

typedef enum {
    ISC_DATAMUX_SENSOR0 = 0,
    ISC_DATAMUX_SENSOR1,
    ISC_DATAMUX_MIPI,
} isc_datamux_t;

typedef enum {
    ISC_IRQ_ERR_NONE = 0,
    ISC_IRQ_BANDWITH_ERR = 0x12000,
    ISC_IRQ_LINE_IN_ERR,
    ISC_IRQ_LINE_OUT_ERR,
} ISC_IRQ_ENC_ERR;

struct yuv_block_info {
    volatile u8 *y;
    volatile u8 *u;
    volatile u8 *v;
    volatile u32 ylen;
    volatile u32 ulen;
    volatile u32 vlen;
    volatile u16 width;
    volatile u16 height;
    volatile u16 line;
    volatile u8 start;
};

struct isc_cfg {
    u8 *y;
    u32 y_len;
    u32 u_len;
    u32 v_len;
    u32 yuv_len;
    u32 yuv_buf_size;
    u16 width;
    u16 height;
    JL_ISC_TypeDef *reg;
    u16 line;//循环BUF模式行数
    void *hdl;
    int (*handler)(void *hdl, int code, int ft);
    u8 *fybuf;
    u8 *fubuf;
    u8 *fvbuf;
    volatile u8 *next_yaddr;
    volatile u8 *prev_yaddr;
    volatile u32 yoffset;
    volatile u32 uoffset;
    volatile u32 voffset;
    u32 flen;
    u32 next_dma_addr;
    u32 read_dma_addr;
    OS_SEM *sem;
    u8 dma_copy_id;

    u32 ppbuf_cnt;
    u32 frame_cnt;

    volatile u8 fstart;
    volatile u8 buf_mode;
    volatile u8 fps;
    volatile u8 output_mode;
    volatile u8 ppbuf_mode;
    volatile u8 yuv_used;
    volatile u8 yuv_start;
    volatile u8 yuv_use_dam;
    volatile u8 in_irq;
    volatile u8 connjpg;

    struct yuv_block_info block_info;
    int (*block_done_cb)(void *info);
    void *frame_done_priv;
    int (*frame_done_cb)(void *priv, int id, void *blk_info);
};

extern void isc_sen_init(void);
extern void isc_sen_en(u8 en);
extern void isc_sen_start(void);
extern void isc_sen_stop(void);
extern void isc_sen_close(void);
extern void isc_sen_discon_from_jpeg(void);
extern void isc_sen_set_polarity(u8 pclk, u8 vsync, u8 hsync);
extern void isc_sen_set_datamode(isc_datamode_t mode);
extern void isc_sen_set_output_datamode(isc_output_datamode_t mode);
extern void isc_sen_set_timingmode(isc_timingmode_t mode);
extern void isc_sen_set_vact_hact(u16 hact, u16 vact);
extern void isc_sen_set_bufamode(char bufmode, u8 connjpg_mode);
extern void isc_sen_set_complete(char connjpg_mode);
extern void isc_sen_set_input_datamode(sen_in_format_t mode);
extern void isc_sen_set_yuv_addr(u8 *y, u32 yuvbuf_size);
extern int isc_sen_get_yuv_addr(u32 *y);
extern int isc_sen_get_cfg_line(void);
extern int isc_sen_set_cfg_line(int line);
extern void isc_sen_set_out_fps(u8 src_fps, u8 fps);
extern void isc_sen_register_irq(int (*handler)(void *hdl, int code, int ft));
extern void *isc_get_handler(void);
extern int isc_sen_set_hdl(void *hdl);
extern void isc_yuv_mode_set_param(u8 *fybuf, u8 *fubuf, u8 *fvbuf, u32 len, OS_SEM *sem);
extern void isc_yuv_mode_clear(void);
extern void isc_yuv_mode_close(void);
extern void *isc_dma_copy_wait(u32 *size);
extern void isc_sen_yuv_addr_lock_reset(u32 reg);
extern void isc_sen_info_init(void);
extern int isc_sen_get_fps(void);
extern void isc_sen_ppbuf_mode_set(char set);
extern void isc_sen_block_done_cb_set(int (*block_done_cb)(void *priv));
extern void isc_sen_frame_done_cb_set(int (*frame_done_cb)(void *priv, int id));
extern void isc_sen_frame_done_priv_set(void *priv);
extern void isc_sen_connet_to_jpeg(char reset_isc);



#endif

