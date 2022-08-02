#ifndef _OSD_H_
#define _OSD_H_

#include "video/video.h"
#include "system/sys_time.h"

#define OSD_OSD_1BIT_MODE	0x1
#define OSD_OSD_2BIT_MODE	0x2
#define OSD_OSD_16BIT_MODE	0x3

struct osd_cfg {
    struct video_osd_attr attr;
    struct video_text_osd text_osd;
    void *rtc_fd;
    char *osd_buf;
    u32 osd_size;
    u8 update;
    u8 enable;
    u16 src_width;
    u16 src_height;
    u8 format_str[64];
};

extern char *osd_buf_addr;
extern int sys_time_format(char *str, int str_len, struct sys_time *time);
extern void osd_close(void);
extern int osd_buff_update(void);
extern void osd_init(void);
extern void osd_en(u8 enable);
extern int osd_attr_set(struct video_osd_attr *osd_attr);
extern int osd_attr_get(struct video_osd_attr *osd_attr);
extern int osd_set_src_width_height(u16 w, u16 h);

#define OSD_DEFAULT_WIDTH	16	//与net_video_rec请求的OSD宽相同
#define OSD_DEFAULT_HEIGHT	32	//与net_video_rec请求的OSD高相同

extern const unsigned char osd_text_str[];
extern const unsigned char osd_text_tab16[66][64] ALIGNED(64);

#endif
