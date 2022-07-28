#ifndef __ISC_H__
#define __ISC_H__

#include "gpio.h"
#include "fs/fs.h"
#include "spinlock.h"
#include "asm/isp_dev.h"


/****** ISC sensor group_port ********/
#define ISC_GROUPA	IO_PORTA
#define ISC_GROUPC	IO_PORTC
#define ISC_GROUPH	IO_PORTH

/*************ie type ****************/
#define 	EXTERN_CONNECT_IE	(BIT(3)|BIT(4))//DMA_CON
#define 	LDONE_IE			BIT(3)//COM_CON
#define 	FDONE_IE			BIT(4)//COM_CON
#define 	BDONE_IE			BIT(5)//COM_CON
#define 	ISCERR_PROTEC_CLR	BIT(12)//COM_CON
#define 	ISCERR_PROTEC_PND	BIT(13)//COM_CON

#define 	BWERR_IE			BIT(6)//COM_CON
#define 	LINERR_IE			BIT(7)//COM_CON
#define 	OSDERR_IE			BIT(8)//COM_CON
#define 	SET_IE(reg,ie_type) 	(reg |= ie_type)
#define 	SET_IE(reg,ie_type) 	(reg |= ie_type)
#define 	GET_IE(reg,ie_type) 	(reg & ie_type)
#define 	GET_PND(reg,pnd_type) 	(reg & pnd_type)
#define 	UNSET_IE(reg,ie_type) 	(reg &=~ie_type)

/********** pending and clear type *************/
#define 	LDONE_PND			BIT(3)
#define 	FDONE_PND			BIT(4)
#define 	BDONE_PND			BIT(5)
#define 	BWERR_PND			BIT(6)
#define 	LINERR_PND			BIT(7)
#define 	OSDERR_PND			BIT(8)

/*************read pending*************/
#define 	IRQ_FSTART(reg, r)		((r & FSTART_PND) 	&& (reg & FSTART_IE))
#define 	IRQ_FDONE(reg, r)		((r & FDONE_PND) 	&& (reg & FDONE_IE))
#define 	IRQ_LDONE(reg, r)		((r & LDONE_PND) 	&& (reg & LDONE_IE))
#define 	IRQ_LINERR(reg, r)		((r & LINERR_PND) 	&& (reg & LINERR_IE))
#define 	IRQ_LOUTERR(reg, r)		((r & LOUTERR_PND)	&& (reg & LOUTERR_IE))
#define 	IRQ_BDONE(reg, r)		((r & BDONE_PND) 	&& (reg & BDONE_IE))
#define 	IRQ_BWERR(reg, r)		((r & BWERR_PND)	&& (reg & BWERR_IE))

#define 	IRQ_OSD_ERR(reg)		(reg & BIT(3))

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
    ISC_TIMINGMODE_VSYNC,
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
    u8 id;
    u8 bdone_ref;
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

extern void isc_sen_init(char id);
extern void isc_sen_reinit(void *isc);
extern void *isc_sen_open(char id);
extern void isc_sen_en(void *isc, u8 en);
extern void isc_sen_start(void *isc);
extern void isc_sen_stop(void *isc);
extern void isc_sen_close(void *isc);
extern void isc_sen_discon_from_jpeg(void *isc);
extern void isc_sen_set_polarity(void *isc, u8 pclk, u8 vsync, u8 hsync);
extern void isc_sen_set_datamode(void *isc, isc_datamode_t mode, u32 config, u32 start_io);
extern void isc_sen_set_output_datamode(void *isc, isc_output_datamode_t mode);
extern void isc_sen_set_timingmode(void *isc, isc_timingmode_t mode);
extern void isc_sen_set_vact_hact(void *isc, u16 hact, u16 vact);
extern void isc_sen_set_bufamode(void *isc, char bufmode, u8 connjpg_mode);
extern void isc_sen_set_complete(void *isc, char connjpg_mode);
extern void isc_sen_set_input_datamode(void *isc, sen_in_format_t mode);
extern void isc_sen_set_yuv_addr(void *isc, u8 *y, u32 yuvbuf_size);
extern int isc_sen_get_yuv_addr(void *isc, u32 *y);
extern int isc_sen_get_cfg_line(void *isc);
extern int isc_sen_set_cfg_line(void *isc, int line);
extern void isc_sen_set_out_fps(void *isc, u8 src_fps, u8 fps);
extern void isc_sen_register_irq(void *isc, int (*handler)(void *hdl, int code, int ft));
extern void *isc_get_handler(void *isc);
extern int isc_sen_set_hdl(void *isc, void *hdl);
extern void isc_yuv_mode_set_param(void *isc, u8 *fybuf, u8 *fubuf, u8 *fvbuf, u32 len, OS_SEM *sem);
extern void isc_yuv_mode_clear(void *isc);
extern void isc_yuv_mode_close(void *isc);
extern void *isc_dma_copy_wait(void *isc, u32 *size);
extern void isc_sen_yuv_addr_lock_reset(struct isc_cfg *isc_info, char dma_buf_id);
extern void isc_sen_info_init(void *isc);
extern int isc_sen_get_fps(void *isc);
extern void isc_sen_ppbuf_mode_set(void *isc, char set);
extern void isc_sen_block_done_cb_set(void *isc, int (*block_done_cb)(void *priv));
extern void isc_sen_frame_done_cb_set(void *isc, int (*frame_done_cb)(void *priv, int id));
extern void isc_sen_frame_done_priv_set(void *isc, void *priv);
extern void isc_sen_connet_to_jpeg(void *isc, char reset_isc);
extern void isc_vhsync_exchange(void *isc, char isc_id, int sync);



#endif

