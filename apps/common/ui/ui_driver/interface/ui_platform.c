
#include "ui/includes.h"
#include "timer.h"
#include "asm/crc16.h"
#include "lcd_drive.h"
#include "ascii.h"
#include "font/font_textout.h"
#include "res/rle.h"
#include "res/resfile.h"
#include "res/mem_var.h"
#include "res_config.h"
#include "app_config.h"
#include "fs/fs.h"

#ifdef CONFIG_UI_ENABLE

#define USE_PI32V2_MATH  0
#define PI32V2_MATH_DSP

void pi32v2_fill_q15(
    u16 value,
    u16 *pDst,
    u32 blockSize)
{
    u32 blkCnt;                               /* Loop counter */

#if defined (PI32V2_MATH_DSP)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = blockSize >> 1U;

    __asm__ volatile(
        " r8 = pack(%0.l,%0.l)\n\t"
        "1: \n\t " \
        " rep %2 { \n\t" \
        " [%1++=4] = r8\n\t" \
        " }\n\t" \
        " if(%2!=0) goto 1b \n\t" \
        : "=&r"(value), "=&r"(pDst), "=&r"(blkCnt) \
        : "0"(value), "1"(pDst), "2"(blkCnt) \
        : "r8", "memory");

    /* Loop unrolling: Compute remaining outputs */
    blkCnt = blockSize & 0x01U;


#else

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;

#endif /* #if defined (PI32V2_MATH_LOOPUNROLL) */

    while (blkCnt > 0U) {
        /* C = value */

        /* Fill value in destination buffer */
        *pDst++ = value;

        /* Decrement loop counter */
        blkCnt--;
    }
}
void pi32v2_fill_q31(
    u32 value,
    u32 *pDst,
    u32 blockSize)
{
    u32 blkCnt;                               /* Loop counter */

#if defined (PI32V2_MATH_DSP)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = blockSize;

    __asm__ volatile(
        "1: \n\t " \
        " rep %2 { \n\t" \
        " [%1++=4] = %0\n\t" \
        " }\n\t" \
        " if(%2!=0) goto 1b \n\t" \
        : "=&r"(value), "=&r"(pDst), "=&r"(blockSize) \
        : "0"(value), "1"(pDst), "2"(blockSize) \
        : "memory");

    /* Loop unrolling: Compute remaining outputs */
    blkCnt = 0;


#else

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;

#endif /* #if defined (PI32V2_MATH_LOOPUNROLL) */

    while (blkCnt > 0U) {
        /* C = value */

        /* Fill value in destination buffer */
        *pDst++ = value;

        /* Decrement loop counter */
        blkCnt--;
    }
}
void pi32v2_copy_q7(
    const u8 *pSrc,
    u8 *pDst,
    u32 blockSize)
{
    u32 blkCnt;                               /* Loop counter */

#if defined (PI32V2_MATH_DSP)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    __asm__ volatile(
        "1: \n\t " \
        " rep %2 { \n\t" \
        " r7 = [%0++=4]\n\t" \
        " [%1++=4] = r7\n\t" \
        " }\n\t" \
        " if(%2!=0) goto 1b \n\t" \
        : "=&r"(pSrc), "=&r"(pDst), "=&r"(blkCnt) \
        : "0"(pSrc), "1"(pDst), "2"(blkCnt) \
        : "r7", "memory");

    /* Loop unrolling: Compute remaining outputs */
    blkCnt = blockSize & 0x03U;


#else

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;

#endif /* #if defined (PI32V2_MATH_LOOPUNROLL) */

    while (blkCnt > 0U) {
        /* C = A */

        /* Copy and store result in destination buffer */
        *pDst++ = *pSrc++;

        /* Decrement loop counter */
        blkCnt--;
    }
}
void pi32v2_copy_q31(
    const u32 *pSrc,
    u32 *pDst,
    u32 blockSize)
{
    u32 blkCnt;                               /* Loop counter */

#if defined (PI32V2_MATH_DSP)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = blockSize;

    __asm__ volatile(
        "1: \n\t " \
        " rep %2 { \n\t" \
        " r7 = [%0++=4]\n\t" \
        " [%1++=4] = r7\n\t" \
        " }\n\t" \
        " if(%2!=0) goto 1b \n\t" \
        : "=&r"(pSrc), "=&r"(pDst), "=&r"(blockSize) \
        : "0"(pSrc), "1"(pDst), "2"(blockSize) \
        : "r7", "memory");

    /* Loop unrolling: Compute remaining outputs */
    blkCnt = 0;

#else

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;

#endif /* #if defined (PI32V2_MATH_LOOPUNROLL) */

    while (blkCnt > 0U) {
        /* C = A */

        /* Copy and store result in destination buffer */
        *pDst++ = *pSrc++;

        /* Decrement loop counter */
        blkCnt--;
    }
}

/*#if TCFG_SPI_LCD_ENABLE*/


#define UI_DEBUG 0
/* #define UI_BUF_CALC */

#if (UI_DEBUG == 1)

#define UI_PUTS puts
#define UI_PRINTF printf

#else

#define UI_PUTS(...)
#define UI_PRINTF(...)

#endif

#define _RGB565(r,g,b)  (u16)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))
#define UI_RGB565(c)  \
        _RGB565((c>>16)&0xff,(c>>8)&0xff,c&0xff)

#define TEXT_MONO_CLR 0x555aaa
#define TEXT_MONO_INV 0xaaa555
#define RECT_MONO_CLR 0x555aaa
#define BGC_MONO_SET  0x555aaa


struct fb_map_user {
    u16 xoffset;
    u16 yoffset;
    u16 width;
    u16 height;
    u8  *baddr;
    u8  *yaddr;
    u8  *uaddr;
    u8  *vaddr;
    u8 transp;
    u8 format;
};

struct fb_var_screeninfo {
    u16 s_xoffset;            //显示区域x坐标
    u16 s_yoffset;            //显示区域y坐标
    u16 s_xres;               //显示区域宽度
    u16 s_yres;               //显示区域高度
    u16 v_xoffset;      //屏幕的虚拟x坐标
    u16 v_yoffset;      //屏幕的虚拟y坐标
    u16 v_xres;         //屏幕的虚拟宽度
    u16 v_yres;         //屏幕的虚拟高度
    u16 rotate;
};

struct window_head {
    u32 offset;
    u32 len;
    u32 ptr_table_offset;
    u16 ptr_table_len;
    u16 crc_data;
    u16 crc_table;
    u16 crc_head;
};

struct ui_file_head {
    u8  res[16];
    u8 type;
    u8 window_num;
    u16 prop_len;
    u8 rotate;
    u8 rev[3];
};


struct ui_load_info ui_load_info_table[] = {
    {1, NULL, NULL},
    {2, RES_PATH"prj2.sty", NULL},//by yyj
    {-1, NULL, NULL},
};



static u32 ui_rotate = false;
static u32 ui_hori_mirror = false;
static u32 ui_vert_mirror = false;
static int malloc_cnt = 0;
static FILE *ui_file = NULL;
static FILE *ui_file1 = NULL;
static FILE *ui_file2 = NULL;
static int ui_file_len = 0;

static int open_resource_file();
void select_strfile(u8 index);
void select_resfile(u8 index);

static const struct ui_platform_api br23_platform_api;

struct ui_priv {
    struct ui_platform_api *api;
    struct lcd_interface *lcd;
    int window_offset;
    struct lcd_info info;
};
static struct ui_priv priv ALIGNED(4);
#define __this (&priv)

#ifdef UI_BUF_CALC
struct buffer {
    struct list_head list;
    u8 *buf;
    int size;
};
struct buffer buffer_used = {0};
#endif


void *br23_malloc(int size)
{
    void *buf;
    malloc_cnt++;
    buf = (void *)malloc(size);

    /* printf("platform_malloc : 0x%x, %d\n", buf, size); */
#ifdef UI_BUF_CALC
    struct buffer *new = (struct buffer *)malloc(sizeof(struct buffer));
    new->buf = buf;
    new->size = size;
    list_add_tail(new, &buffer_used);
    printf("platform_malloc : 0x%x, %d\n", buf, size);

    struct buffer *p;
    int buffer_used_total = 0;
    list_for_each_entry(p, &buffer_used.list, list) {
        buffer_used_total += p->size;
    }
    printf("used buffer size:%d\n\n", buffer_used_total);
#endif

    return buf;
}

void br23_free(void *buf)
{

    /* printf("platform_free : 0x%x\n",buf); */
    free(buf);
    malloc_cnt--;

#ifdef UI_BUF_CALC
    struct buffer *p, *n;
    list_for_each_entry_safe(p, n, &buffer_used.list, list) {
        if (p->buf == buf) {
            printf("platform_free : 0x%x, %d\n", p->buf, p->size);
            __list_del_entry(p);
            free(p);
        }
    }

    int buffer_used_total = 0;
    list_for_each_entry(p, &buffer_used.list, list) {
        buffer_used_total += p->size;
    }
    printf("used buffer size:%d\n\n", buffer_used_total);
#endif
}

int ui_platform_ok()
{
    return (malloc_cnt == 0);
}

static void draw_rect_range_check(struct rect *r, struct fb_map_user *map)
{
    if (r->left < map->xoffset) {
        r->left = map->xoffset;
    }
    if (r->left > (map->xoffset + map->width)) {
        r->left = map->xoffset + map->width;
    }
    if ((r->left + r->width) > (map->xoffset + map->width)) {
        r->width = map->xoffset + map->width - r->left;
    }
    if (r->top < map->yoffset) {
        r->top = map->yoffset;
    }
    if (r->top > (map->yoffset + map->height)) {
        r->top = map->yoffset + map->height;
    }
    if ((r->top + r->height) > (map->yoffset + map->height)) {
        r->height = map->yoffset + map->height - r->top;
    }

    ASSERT(r->left >= map->xoffset);
    ASSERT(r->top  >= map->yoffset);
    ASSERT((r->left + r->width) <= (map->xoffset + map->width));
    ASSERT((r->top + r->height) <= (map->yoffset + map->height));
}


/* 透明色: 16bits 0x55aa      0101 0xxx 1011 01xx 0101 0xxx
 *         24bits 0x50b450    0101 0000 1011 0100 0101 0000 , 80 180 80
 * */
__attribute__((always_inline))
void __font_pix_copy(struct draw_context *dc, int format, struct fb_map_user *map, u8 *pix, struct rect *draw, int x, int y,
                     int height, int width, int color)
{

    int i, j, h;
    u16 osd_color;
    u32 size;

    osd_color = (format == DC_DATA_FORMAT_OSD8) || (format == DC_DATA_FORMAT_OSD8A) ? color & 0xff : color & 0xffff ;
    osd_color = ((osd_color << 8) & 0xff00) | ((osd_color >> 8) & 0xff); //
    for (j = 0; j < (height + 7) / 8; j++) { /* 纵向8像素为1字节 */
        for (i = 0; i < width; i++) {
            if (((i + x) >= draw->left)
                && ((i + x) <= (draw->left + draw->width - 1))) { /* x在绘制区域，要绘制 */
                u8 pixel = pix[j * width + i];
                int hh = height - (j * 8);
                if (hh > 8) {
                    hh = 8;
                }
                for (h = 0; h < hh; h++) {
                    if (((y + j * 8 + h) >= draw->top)
                        && ((y + j * 8 + h) <= (draw->top + draw->height - 1))) { /* y在绘制区域，要绘制 */
                        u16 clr = pixel & BIT(h) ? 1 : 0;
                        if (clr) {
                            if (platform_api->draw_point) {
#if 0
                                platform_api->draw_point(dc, x + i, y + j * 8 + h, osd_color);
#else
                                u16 *pdisp = (u16 *)dc->buf;
                                int offset = (y + j * 8 + h - dc->disp.top) * dc->disp.width + (x + i - dc->disp.left);
                                if ((offset * 2 + 1) < dc->len) {
                                    pdisp[offset] = osd_color;
                                }
#endif
                            }
                        }
                    }
                } /* endof for h */
            }
        }/* endof for i */
    }/* endof for j */
}


static int image_str_size_check(struct draw_context *dc, int page_num, const char *txt, int *width, int *height)
{

    u16 id = ((u8)txt[1] << 8) | (u8)txt[0];
    u16 cnt = 0;
    struct image_file file = {0};
    int w = 0, h = 0;

    while (id != 0x00ff) {
        select_resfile(dc->prj);
        struct mem_var_list *list;
        if ((list = mem_var_search(0, 0, id, page_num, dc->prj)) != NULL) {
            mem_var_get(list, &file, sizeof(struct image_file));
        } else {
            if (open_image_by_id(NULL, &file, id, page_num) != 0) {
                return -EFAULT;
            }
            mem_var_add(0, 0, id, page_num, dc->prj, &file, sizeof(struct image_file));
        }
        w += file.width;
        cnt += 2;
        id = ((u8)txt[cnt + 1] << 8) | (u8)txt[cnt];
    }
    h = file.height;
    *width = w;
    *height = h;
    return 0;
}

void platform_putchar(struct font_info *info, u8 *pixel, u16 width, u16 height, u16 x, u16 y)
{
    __font_pix_copy(info->dc, info->disp.format,
                    (struct fb_map_user *)info->disp.map,
                    pixel,
                    (struct rect *)info->disp.rect,
                    (s16)x,
                    (s16)y,
                    height,
                    width,
                    info->disp.color);
}



struct file_dev {
    const char *logo;
    const char *root_path;
};


struct file_browser {
    int show_mode;
    struct rect r;
    struct vfscan *fscan;
    /* struct server *server; */
    struct ui_file_browser bro;
    struct file_dev dev;//支持三个设备
};


static int check_file_ext(const char *ext_table, const char *ext)
{
    const char *str;

    for (str = ext_table; *str != '\0'; str += 3) {
        if (0 == ASCII_StrCmpNoCase(ext, str, 3)) {
            return true;
        }
    }
    return false;
}

static const u8 MUSIC_SCAN_PARAM[] = "-t"
                                     "MP1MP2MP3"
                                     " -sn -d"
                                     ;




static int platform_file_get_dev_total()
{
    /*return dev_manager_get_total(1);*/
}



static void platform_file_browser_get_dev_info(struct ui_file_browser *_bro, u8 index)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);
    struct file_dev *file_dev;//支持三个设备


    if (!bro) {
        return;
    }
    struct __dev *dev = dev_manager_find_by_index(index, 0);//storage_dev_find_by_index(index);
    if (dev) {
        file_dev = &bro->dev;
        file_dev->logo = dev_manager_get_logo(dev);//获取设备logo
        file_dev->root_path = dev_manager_get_root_path_by_logo(dev);//获取设备路径
    }
}



static struct ui_file_browser *platform_file_browser_open(struct rect *r,
        const char *path, const char *ftype, int show_mode)
{
    int err;
    struct file_browser *bro;
    struct __dev *dev = 0;
    bro = (struct file_browser *)malloc(sizeof(*bro));
    if (!bro) {
        return NULL;
    }
    bro->bro.file_number = 0;
    bro->show_mode = show_mode;

    if (!path) {
        dev = dev_manager_find_active(0);///storage_dev_last();//获取最后一个设备的路径
        if (!dev) {
            free(bro);
            return NULL;
        }
        path = dev_manager_get_root_path_by_logo(dev);//dev->root_path;
    }

    if (!ftype) {
        ftype = MUSIC_SCAN_PARAM;
    }

    bro->fscan = fscan(path, ftype, 9);
    /*bro->bro.dev_num =  dev_manager_get_total(1);//获取在线设备总数*/

    if (bro->fscan) {
        bro->bro.file_number = bro->fscan->file_number;
        if (bro->bro.file_number == 0) {
            return &bro->bro;
        }
    }

    if (r) {
        memcpy(&bro->r, r, sizeof(struct rect));
    }

    return &bro->bro;

__err:
    fscan_release(bro->fscan);
    free(bro);
    return NULL;
}



static void platform_file_browser_close(struct ui_file_browser *_bro)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro) {
        return;
    }
    if (bro->fscan) {
        fscan_release(bro->fscan);
    }
    free(bro);
}

static int platform_get_file_attrs(struct ui_file_browser *_bro,
                                   struct ui_file_attrs *attrs)
{
    int i, j;
    struct vfs_attr attr;
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return -ENOENT;
    }

    FILE *file = fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
    if (!file) {
        return -ENOENT;
    }

    attrs->format = "ascii";

    fget_attrs(file, &attrs->attr);
    /* printf(" attr = %x, fsize =  %x,sclust = %x\n",attrs->attr.attr,attrs->attr.fsize,attrs->attr.sclust); */

    struct sys_time *time;
    time =  &(attrs->attr.crt_time);
    /* printf("y =%d  m =%d d = %d,h = %d ,m = %d ,s =%d\n",time->year,time->month,time->day,time->hour,time->min,time->sec);  */

    time =  &(attrs->attr.wrt_time);
    /* printf("y =%d  m =%d d = %d,h = %d ,m = %d ,s =%d\n",time->year,time->month,time->day,time->hour,time->min,time->sec);  */


    int len = fget_name(file, (u8 *)attrs->fname, 16);//长文件获取有问题
    if (len < 0) {
        fclose(file);
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        if ((u8)attrs->fname[i] >= 0x80) {
            attrs->format = "uft16";
            goto _next;
        }
    }


    /* ASCII_ToUpper(attrs->fname, strlen(attrs->fname)); */

_next:

#if 0//文件系统接口不完善，临时解决
    for (i = 0; i < len; i++) {
        if (attrs->fname[i] == '.') {
            break;
        }
    }

    if (i == len) {
        attrs->ftype = UI_FTYPE_DIR;
    } else {
        char *ext = attrs->fname + i + 1;

        if (check_file_ext("JPG", ext)) {
            attrs->ftype = UI_FTYPE_IMAGE;
        } else if (check_file_ext("MOVAVI", ext)) {
            attrs->ftype = UI_FTYPE_VIDEO;
        } else if (check_file_ext("MP3WMAWAV", ext)) {
            attrs->ftype = UI_FTYPE_AUDIO;
        } else {
            attrs->ftype = UI_FTYPE_UNKNOW;
        }
    }
#else
    /* printf("name = %d %d \n",strlen(attrs->fname),len); */
    /* put_buf(attrs->fname,len); */

    if (attrs->attr.attr & F_ATTR_DIR) {
        attrs->ftype = UI_FTYPE_DIR;
    } else {
        char *ext = attrs->fname + strlen(attrs->fname) - 3;
        if (check_file_ext("JPG", ext)) {
            attrs->ftype = UI_FTYPE_IMAGE;
        } else if (check_file_ext("MOVAVI", ext)) {
            attrs->ftype = UI_FTYPE_VIDEO;
        } else if (check_file_ext("MP3WMAWAV", ext)) {
            attrs->ftype = UI_FTYPE_AUDIO;
        } else {
            attrs->ftype = UI_FTYPE_UNKNOW;
        }

    }

#endif

    fclose(file);

    return 0;
}

static int platform_set_file_attrs(struct ui_file_browser *_bro,
                                   struct ui_file_attrs *attrs)
{
    int attr = 0;
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return -ENOENT;
    }

    FILE *file = fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
    if (!file) {
        return -EINVAL;
    }

    fget_attr(file, &attr);
    if (attrs->attr.attr & F_ATTR_RO) {
        attr |= F_ATTR_RO;
    } else {
        attr &= ~F_ATTR_RO;
    }
    fset_attr(file, attr);

    fclose(file);

    return 0;
}

static void *platform_open_file(struct ui_file_browser *_bro,
                                struct ui_file_attrs *attrs)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return NULL;
    }

    return fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
}

static int platform_delete_file(struct ui_file_browser *_bro,
                                struct ui_file_attrs *attrs)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return -EINVAL;
    }

    FILE *file = fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
    if (!file) {
        return -EFAULT;
    }
    fdelete(file);

    return 0;
}

void test_browser()
{
    struct ui_file_browser *browser = NULL;
    static struct ui_file_attrs attrs = {0};
    if (!browser) {
        browser = platform_file_browser_open(NULL, NULL, MUSIC_SCAN_PARAM, 0);
    }

    if (browser) {
        printf("file num = %d \n", browser->file_number);
        platform_get_file_attrs(browser, &attrs);
        printf("format =%s name =%s type = %x \n", attrs.format, attrs.fname, attrs.ftype);
        platform_delete_file(browser, &attrs);
        attrs.file_num ++;
        if (attrs.file_num >= browser->file_number) {
            attrs.file_num = 0;
        }
    }
}

static void *br23_set_timer(void *priv, void (*callback)(void *), u32 msec)
{
    return (void *)sys_timer_add(priv, callback, msec);
}

static int br23_del_timer(void *fd)
{
    if (fd) {
        sys_timer_del((int)fd);
    }

    return 0;
}

u32 __attribute__((weak)) set_retry_cnt()
{
    return 10;
}

const char *WATCH_VERSION_LIST = {
    "W001"

};

/* static const char *WATCH_STY_CHECK_LIST[] = { */
/* RES_PATH"JL/JL.sty", */
/* RES_PATH"watch/watch.sty", */
/* RES_PATH"watch1/watch1.sty", */
/* RES_PATH"watch2/watch2.sty", */
/* RES_PATH"watch3/watch3.sty", */
/* RES_PATH"watch4/watch4.sty", */
/* RES_PATH"watch5/watch5.sty", */
/* }; */

#define WATCH_ITEMS_LIMIT		40
#define BGP_ITEMS_LIMIT		    (WATCH_ITEMS_LIMIT + 10)//(确保 >= WATCH_ITEMS_LIMIT就可以)
static u8 watch_style;
static u8 watch_items;
static volatile u8 watch_update_over = 0;

static char *watch_res[WATCH_ITEMS_LIMIT] = {0};
static char *watch_bgp_related[WATCH_ITEMS_LIMIT] = {0};
static char *watch_bgp[BGP_ITEMS_LIMIT] = {0};
static u8 watch_bgp_items;

static char *watch_bgpic_path = NULL;

extern u8 smartbox_eflash_flag_get(void);
extern u8 smartbox_eflash_update_flag_get(void);
extern void smartbox_eflash_flag_set(u8 eflash_state_type);
extern int smartbox_eflash_resume_opt(void);
static int watch_mem_bgp_related();

u32 watch_bgp_get_nums()
{
    return watch_bgp_items;
}

char *watch_bgp_get_item(u8 sel_item)
{
    if (sel_item >= watch_bgp_items) {
        printf("\n\n\nwatch_bgp items overflow %d\n\n\n\n", sel_item);
        return NULL;
    }

    return watch_bgp[sel_item];
}

char *watch_bgp_add(char *bgp)
{
    char *root_path = RES_PATH;
    char *new_bgp_item = NULL;
    u32 len;
    u32 i;

    if ((watch_bgp_items + 1) >= BGP_ITEMS_LIMIT) {
        printf("\n\n\nwatch_bgp items overflow %d\n\n\n\n", watch_bgp_items);
        return NULL;
    }

    len = strlen(bgp) + strlen(root_path) + 1;
    new_bgp_item = malloc(len);
    if (new_bgp_item == NULL) {
        printf("\n\n\nwatch_bgp items malloc err %d\n\n\n\n", len);
        return NULL;
    }

    ASCII_ToLower(bgp, strlen(bgp));
    strcpy(new_bgp_item, root_path);
    /* strcpy(&new_bgp_item[strlen(root_path)], &bgp[1]); */
    strcpy(&new_bgp_item[strlen(root_path)], bgp);
    new_bgp_item[len - 1] = '\0';

    //如果已经存在这个背景图片路径，则直接返回对应的地址
    for (i = 0; i < watch_bgp_items; i++) {
        /* printf("num %s\n", watch_bgp[i]); */
        if (strncmp(new_bgp_item, watch_bgp[i], strlen(new_bgp_item)) == 0) {
            free(new_bgp_item);
            new_bgp_item = watch_bgp[i];
            printf("already has %s\n", new_bgp_item);
            return new_bgp_item;
        }
    }

    watch_bgp[watch_bgp_items] = new_bgp_item;
    watch_bgp_items++;

    printf("add new_bgp_item succ %d, %s\n", watch_bgp_items, new_bgp_item);

    return new_bgp_item;
}

int watch_bgp_del(char *bgp)
{
    u32 i;
    char watch_bgp_item[64];
    u32 cur_items = watch_bgp_items;
    char *root_path = RES_PATH;
    char *bgp_item = NULL;

    ASSERT(((strlen(bgp) + strlen(root_path) + 1) < sizeof(watch_bgp_item)), "bgp err name0 %s\n", bgp);

    ASCII_ToLower(bgp, strlen(bgp));
    strcpy(watch_bgp_item, root_path);
    /* strcpy(&watch_bgp_item[strlen(root_path)], &bgp[1]); */
    strcpy(&watch_bgp_item[strlen(root_path)], bgp);
    watch_bgp_item[strlen(bgp) + strlen(root_path)] = '\0';
    printf("watch_bgp_item %s\n", watch_bgp_item);

    for (i = 0; i < cur_items; i++) {
        if (strncmp(watch_bgp_item, watch_bgp[i], strlen(watch_bgp_item)) == 0) {
            bgp_item = watch_bgp[i];
            watch_bgp[i] = NULL;
            free(bgp_item);
            watch_bgp_items--;
            break;
        }
    }

    if (bgp_item == NULL) {
        printf("can not find bgp_item %s\n", watch_bgp_item);
        return -1;
    }

    for (; i < cur_items; i++) {
        if (watch_bgp[i + 1] != NULL) {
            watch_bgp[i] = watch_bgp[i + 1];
        } else {
            watch_bgp[i] = NULL;
            break;
        }
    }

    //del related item
    cur_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    for (i = 0; i < cur_items; i++) {
        if (bgp_item == watch_bgp_related[i]) {
            watch_bgp_related[i] = NULL;
        }
    }

    for (i = 0; i < cur_items; i++) {
        printf("cur related items %d, %s\n", i, watch_bgp_related[i]);
    }
    for (i = 0; i < watch_bgp_items; i++) {
        printf("cur bgp items %d, %s\n", i, watch_bgp[i]);
    }

    return 0;
}


//替换背景图片,(1)如果某个表盘没有背景图片，则增加，(2)如果有就替换
//如果被替换的这个背景图片已经不关联任何表盘，则将它从表watch_bgp删除
int watch_bgp_set_related(char *bgp, u8 cur_watch, u8 del)
{
    u32 i;
    u32 cur_items;
    char *root_path = RES_PATH;
    char *bgp_item = NULL;
    char *new_bgp_item = NULL;
    u32 total_relate_items;
    char old_bgp[16];
    u32 len;

    total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    if (cur_watch >= total_relate_items) {
        printf("\n\n\nwatch_bgp_related items overflow %d\n\n\n\n", cur_watch);
        return -1;
    }

    //提取旧的背景图片名字
    if (watch_bgp_related[cur_watch]) {
        bgp_item = watch_bgp_related[cur_watch];
        len = strlen(bgp_item) - strlen(root_path) + 1;
        if (len >= sizeof(old_bgp)) {
            return -1;
        }
        memcpy(old_bgp, &bgp_item[strlen(root_path)], len);
        printf("old bpg %s\n", old_bgp);
    }

    if (bgp != NULL) {
        new_bgp_item = watch_bgp_add(bgp);
        if (new_bgp_item == NULL) {
            printf("add bgp item err %s\n", bgp);
            return -1;
        }
    }

    //如果没有背景图片
    if (watch_bgp_related[cur_watch] == NULL) {
        watch_bgp_related[cur_watch] = new_bgp_item;
        /* printf("a1\n"); */
    } else {
        bgp_item = watch_bgp_related[cur_watch];
        watch_bgp_related[cur_watch] = new_bgp_item;
        cur_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
        /* printf("a2 %s\n", bgp_item); */
        for (i = 0; i < cur_items; i++) {
            if (bgp_item == watch_bgp_related[i]) {
                /* printf("a3 %d\n", i); */
                break;
            }
        }
        if ((i == cur_items) && del) { //被替换的这个背景图片已经不关联任何表盘
            /* printf("a4\n"); */
            watch_bgp_del(old_bgp);
        }
    }

    watch_mem_bgp_related();

    return 0;
}

//根据某个表盘获取对应的背景图片
char *watch_bgp_get_related(u8 cur_watch)
{
    u32 total_relate_items;

    total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    if (cur_watch >= total_relate_items) {
        printf("\n\n\nwatch_bgp_related items overflow %d\n\n\n\n", cur_watch);
        return NULL;
    }

    return watch_bgp_related[cur_watch];
}

int watch_bgp_related_del_all(char *bgp)
{
    watch_bgp_del(bgp);
    watch_mem_bgp_related();
    return 0;
}

int watch_get_style_by_name(char *name)
{
    u32 i;
    u32 ret;
    printf("watch find %s\n", name);

    for (i = 0; i < watch_items; i++) {
        /* printf("watch finding %s\n", watch_res[i]); */
        if (strncmp(name, watch_res[i], strlen(name)) == 0) {
            printf("find watch style %d, %s\n\n\n", i, watch_res[i]);
            return i;
        }
    }

    printf("watch find faile\n");
    return -1;
}

#define WATCH_MEM_BGP		0x55aa66bb
static u32 wmem_last = 0;
static u32 wmem_area_num = 0;
static void *wmem_file = NULL;

int watch_mem_new(u32 area)
{
    wmem_last = area;
    wmem_area_num++;
    return 0;
}

void *watch_mem_open()
{
    if (wmem_file == NULL) {
        /* wmem_file = fopen(RES_PATH"wmem.bin", "w+"); */
        wmem_file = fopen("storage/sd1/C/downlad/wmem.bin", "w+");
    }
    return wmem_file;
}

void watch_mem_close()
{
    if (wmem_file) {
        fclose(wmem_file);
        wmem_file = NULL;
    }
}

int watch_mem_write(u32 offset, u32 len, u8 *buf, u32 area)
{
    u32 ret;
    u8 tmp_buf[8];
    u32 area_offset = 0;
    u32 area_len = 0;
    u32 find_tag = 0;

    if (wmem_file == NULL) {
        return -1;
    }

    if ((flen(wmem_file) == 0) || (wmem_area_num <= 1)) {
        area_offset = 0;
    } else {
        do {
            fseek(wmem_file, area_offset, SEEK_SET);
            ret = fread(tmp_buf, 8, 1, wmem_file);
            if (ret != 8) {
                printf("wmem find tag err end\n");
                return -1;
            }
            memcpy(&ret, tmp_buf + 4, 4); //flag
            if (ret == area) {
                memcpy(&area_len, tmp_buf, 4);//len
                find_tag = 1;
                break;
            }
            memcpy(&ret, tmp_buf, 4);//len
            area_offset += ret;
        } while (find_tag == 0);
    }

    fseek(wmem_file, area_offset + offset, SEEK_SET);
    if (area == wmem_last) {
        ret = fwrite(buf, len, 1, wmem_file);
        if (ret != len) {
            return -1;
        }
    } else {
        if ((offset + len) <= area_len) {
            ret = fwrite(buf, len, 1, wmem_file);
            if (ret != len) {
                return -1;
            }
        } else {
            //要将这个区域的内容先搬迁到文件最后，使这个区域成为最后的区域
            //再写数据

        }
    }

    return 0;
}

/* static u32 wmem_test_flag = 0; */
int watch_mem_read(u32 offset, u32 len, u8 *buf, u32 area)
{
    u32 area_offset = 0;
    u32 area_len = 0;
    u32 find_tag = 0;
    u32 ret;
    u8 tmp_buf[8];

    if (wmem_file == NULL) {
        return -1;
    }

    if (flen(wmem_file) == 0) {
        /* wmem_test_flag = 1; */
        return 0;
    }


    if (wmem_area_num <= 1) {
        area_offset = 0;
    } else {
        do {
            fseek(wmem_file, area_offset, SEEK_SET);
            ret = fread(tmp_buf, 8, 1, wmem_file);
            if (ret != 8) {
                printf("wmem find tag err end\n");
                return -1;
            }
            memcpy(&ret, tmp_buf + 4, 4); //flag
            if (ret == area) {
                memcpy(&area_len, tmp_buf, 4);//len
                find_tag = 1;
                break;
            }
            memcpy(&ret, tmp_buf, 4);//len
            area_offset += ret;
        } while (find_tag == 0);

        if ((offset + len) > area_len) {
            return -1;
        }
    }

    fseek(wmem_file, area_offset + offset, SEEK_SET);
    ret = fread(buf, len, 1, wmem_file);
    if (ret != len) {
        printf("wmem read err %d\n", ret);
        return -1;
    }

    return 0;
}


static int watch_mem_bgp_related()
{
    int ret = 0;
    u8 *related_buf;
    u32 total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    u32 len;
    char *related_item;
    u32 area = WATCH_MEM_BGP;
    u32 i;

    if (watch_bgp_items == 0) {
        return -1;
    }

    len = 64 * (total_relate_items + 1);
    related_buf = zalloc(len);
    if (related_buf == NULL) {
        return -1;
    }

    if (watch_mem_open() == NULL) {
        free(related_buf);
        return -1;
    }

    memcpy(related_buf, &len, 4);
    memcpy(related_buf + 4, &area, 4);
    for (i = 0; i < total_relate_items; i++) {
        related_item = watch_bgp_related[i];
        if (related_item) {
            printf("related item : %s, %d, %d\n", related_item, strlen(related_item) + 1, i);
            memcpy(&related_buf[64 + 64 * i], related_item, strlen(related_item) + 1);
        }
    }
    ret = watch_mem_write(0, len, related_buf, area);
    if (ret != 0) {
        printf("watch mem werr %x\n", ret);
    } else {
        printf("watch mem succ\n");
    }

    watch_mem_close();

    free(related_buf);

    return ret;
}

static int watch_bgp_related_init()
{
    static u8 flag = 0;
    int ret = 0;
    u8 *related_buf;
    u32 total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    u32 len;
    char *related_item;
    u32 area = WATCH_MEM_BGP;
    u32 i, j;

    if (flag == 0) {
        watch_mem_new(area);
        flag = 1;
    }

    if (watch_bgp_items == 0) {
        return -1;
    }

    len = 64 * (total_relate_items + 1);
    related_buf = zalloc(len);
    if (related_buf == NULL) {
        return -1;
    }

    if (watch_mem_open() == NULL) {
        free(related_buf);
        return -1;
    }

    ret = watch_mem_read(0, len, related_buf, area);
    if (ret != 0) {
        free(related_buf);
        return -1;
    }

    for (i = 0; i < total_relate_items; i++) {
        watch_bgp_related[i] = NULL;
        related_item = &related_buf[64 + 64 * i];
        if (related_item[0]) {
            printf("related item : %s, %d, %d\n", related_item, strlen(related_item), i);
            for (j = 0; j < watch_bgp_items; j++) {
                if (strncmp(related_item, watch_bgp[j], strlen(watch_bgp[j])) == 0) {
                    watch_bgp_related[i] = 	watch_bgp[j];
                    printf("find bgp related %d\n", j);
                    break;
                }
            }
        }
    }

    watch_mem_close();

    free(related_buf);

    return 0;
}



char *watch_get_background()
{
    if (watch_bgpic_path == NULL) {
        return NULL;
    }
    return watch_bgpic_path;
}

int watch_set_background(char *bg_pic)
{
    u32 bg_strlen = strlen(bg_pic);
    u32 root_path_strlen = strlen(RES_PATH);

    /* if ((bg_strlen + root_path_strlen) >= sizeof(watch_bgpic_path)) { */
    /* printf("set background err %d, %d\n", bg_strlen, root_path_strlen); */
    /* return -1; */
    /* } */

    if (bg_pic == NULL) {
        /* memset(watch_bgpic_path, 0, sizeof(watch_bgpic_path)); */
        if (watch_bgpic_path != NULL) {
            free(watch_bgpic_path);
        }
        watch_bgpic_path = NULL;
    } else {

        if (watch_bgpic_path != NULL) {
            free(watch_bgpic_path);
            watch_bgpic_path = NULL;
        }

        watch_bgpic_path = malloc(bg_strlen + root_path_strlen + 1);
        if (watch_bgpic_path == NULL) {
            printf("set background err %d, %d\n", bg_strlen, root_path_strlen);
            return -1;
        }

        strcpy(watch_bgpic_path, RES_PATH);
        strcpy(watch_bgpic_path + root_path_strlen, bg_pic);
        watch_bgpic_path[root_path_strlen + bg_strlen] = '\0';
    }

    return 0;
}

void watch_update_finish()
{
    printf("\n\nwatch update finish\n\n");
    watch_update_over = 1;
}

int watch_get_update_status()
{
    return watch_update_over;
}

char *watch_get_item(int style)
{
    if (style >= watch_items) {
        return NULL;
    }

    return watch_res[style];
}
char *watch_get_root_path()
{
    return RES_PATH;
}

int watch_set_style(int style)
{
#if UI_WATCH_RES_ENABLE
    /* if (style == 0) { */
    /* style = 1; */
    /* } */
    /* if (style > sizeof(watch_res) / sizeof(watch_res[0])) { */
    if (style >= watch_items) {
        return false;
    }
    watch_style = style;
#endif
    return true;
}

static int watch_get_msg_by_path(char *key, char *data, u32 data_len, u32 offset)
{
    int ret = 0;
    FILE *file = res_fopen(data, "r");
    if (NULL == file) {
        return -1;
    }
    // 获取长度
    u32 tmp_strlen = res_flen(file);
    // 用来装路径名字
    u8 *tmp_data = zalloc(strlen(data) + 1);
    if (NULL == tmp_data) {
        return -1;
    }
    memcpy(tmp_data, data, strlen(data) + 1);
    memset(data, 0, data_len);
    // 读出数据
    res_fseek(file, offset, SEEK_SET);
    res_fread(file, data, data_len);

    // 关闭数据
    if (file) {
        res_fclose(file);
    }

    char *tver = NULL;
    for (u32 i = 0; i < data_len; i++) {
        // 换行
        if (0xd == data[i] && 0x0a == data[i + 1]) {
            offset += i + 1 + 1;
            break;
        }
        // 文件结束
        if (0 == data[i] || (offset + i >= tmp_strlen)) {
            offset += i;
            tmp_strlen = 0;
            break;
        }
        if ('"' == data[i]) {
            if (tver) {
                tver = data + i + 1;
                break;
            }
            if (0 == strncmp(key, data + i + 1, strlen(key))) {
                i += strlen(key) + 1 + 1;
                tver = data + i - 1;
                if ('"' != tver[0]) {
                    tver = NULL;
                }
            }
        }
    }

    if (tver) {
        tmp_strlen = index(tver, '"') - tver;
        memcpy(data, tver, tmp_strlen);
        data[tmp_strlen] = '\0';
    } else {
        memcpy(data, tmp_data, strlen(tmp_data) + 1);
        ret = -1;
    }

    if (tmp_data) {
        free(tmp_data);
    }

    if (offset < tmp_strlen && NULL == tver) {
        ret = watch_get_msg_by_path(key, data, data_len, offset);
    }

    return ret;
}

static int watch_get_msg_from_json(char *watch_item, char *value, char *key)
{
    int ret = 0;
    char *sty_suffix = ".sty";
    char *json_suffix = ".json";
    u32 tmp_strlen;
    char tmp_name[64];
    u32 sty_strlen;

    if (watch_item == NULL) {
        return -1;
    }

    sty_strlen = strlen(sty_suffix);
    tmp_strlen = strlen(watch_item);
    strcpy(tmp_name, watch_item);
    strcpy(&tmp_name[tmp_strlen - sty_strlen], json_suffix);
    tmp_name[tmp_strlen - sty_strlen + strlen(json_suffix)] = '\0';
    printf("key %s, path %s\n", key, tmp_name);
    ret = watch_get_msg_by_path(key, tmp_name, sizeof(tmp_name), 0);
    if (0 == ret) {
        memcpy(value, tmp_name, strlen(tmp_name) + 1);
    }
    return ret;
}

int watch_get_uuid(char *watch_item, char *uuid)
{
    return watch_get_msg_from_json(watch_item, uuid, "prj_uuid");
}

int watch_get_version(char *watch_item, char *version)
{
#if 1
    return watch_get_msg_from_json(watch_item, version, "version_id");
#else
    char *sty_suffix = ".sty";
    char *json_suffix = ".json";
    u32 tmp_strlen;
    char tmp_name[64];
    u32 sty_strlen;
    FILE *file;
    /* char *version; */
    char *tver;
    u32 i;

    if (watch_item == NULL) {
        return -1;
    }

    sty_strlen = strlen(sty_suffix);
    tmp_strlen = strlen(watch_item);
    strcpy(tmp_name, watch_item);
    strcpy(&tmp_name[tmp_strlen - sty_strlen], json_suffix);
    tmp_name[tmp_strlen - sty_strlen + strlen(json_suffix)] = '\0';
    printf("version name %s\n", tmp_name);
    file = res_fopen(tmp_name, "r");
    if (!file) {
        printf("open_jsonfile fail %s\n", tmp_name);
        return -1;
    }
    memset(tmp_name, 0, sizeof(tmp_name));
    res_fread(file, tmp_name, sizeof(tmp_name));
    res_fclose(file);
    /* printf("json buf : %s\n", tmp_name); */
    tmp_name[26] = '\0';
    /* version = &tmp_name[22]; */
    strcpy(version, &tmp_name[22]);

    return 0;
#endif
}


int watch_version_juge(char *watch_item)
{
    char *sty_suffix = ".sty";
    char *json_suffix = ".json";
    u32 tmp_strlen;
    char tmp_name[64];
    u32 sty_strlen;
    FILE *file;
    char *version;
    char *tver;
    u32 i;

    if (watch_item == NULL) {
        return -1;
    }

    sty_strlen = strlen(sty_suffix);
    tmp_strlen = strlen(watch_item);
    strcpy(tmp_name, watch_item);
    strcpy(&tmp_name[tmp_strlen - sty_strlen], json_suffix);
    tmp_name[tmp_strlen - sty_strlen + strlen(json_suffix)] = '\0';
    printf("version name %s\n", tmp_name);
    file = res_fopen(tmp_name, "r");
    if (!file) {
        printf("open_jsonfile fail %s\n", tmp_name);
        return -1;
    }
    memset(tmp_name, 0, sizeof(tmp_name));
    res_fread(file, tmp_name, sizeof(tmp_name));
    res_fclose(file);
    /* printf("json buf : %s\n", tmp_name); */
    tmp_name[26] = '\0';
    version = &tmp_name[22];
    /* printf("ve %s\n", version); */

    for (i = 0; i < strlen(WATCH_VERSION_LIST); i += 5) {
        tver = &WATCH_VERSION_LIST[i];
        /* printf("ver %s, %d\n", tver, strlen(version)); */
        if (strncmp(version, tver, strlen(version)) == 0) {
            return 0;
        }
    }

    printf("juge watch version err\n");

    return -1;
}


int watch_set_style_by_name(char *name)
{
    u32 i;
    u32 ret;
    printf("watch find %s\n", name);

    for (i = 0; i < watch_items; i++) {
        /* printf("watch finding %s\n", watch_res[i]); */
        if (strncmp(name, watch_res[i], strlen(name)) == 0) {
            printf("find watch style %d, %s\n\n\n", i, watch_res[i]);

            ret = watch_version_juge(watch_res[i]);
            if (ret != 0) {
                break;
            }
            watch_set_style(i);
            return 0;
        }
    }

    printf("watch find faile\n");
    return -1;
}


int watch_get_style()
{
    return watch_style;
}

int watch_get_items_num()
{
    return watch_items;
}

char *watch_get_cur_path()
{
    static char path[16];
    u8 i;
    char *tmp = &watch_res[watch_style][strlen(RES_PATH)];

    path[0] = '/';
    for (i = 0; i < 15; i++) {
        if (tmp[i] == '/') {
            break;
        }
        path[i + 1] = tmp[i];
        putchar(tmp[i]);
    }
    path[i + 1] = '\0';

    ASCII_ToUpper(path, strlen(path));
    /* printf("cur : %s, %s, %d\n", tmp, path, i); */

    /* return watch_res[watch_style]; */
    return path;
}

int watch_get_cur_path_len()
{
    return strlen(watch_get_cur_path()) + 1;

    /* char *path = watch_res[watch_style]; */
    /* u32 i; */

    /* for (i = strlen(path) - 1; i > 0; i--) { */
    /* if (path[i] == '/') { */
    /* break; */
    /* } */
    /* } */

    /* return i + 1; */
}

extern void virfat_flash_get_dirinfo(void *file_buf, u32 *file_num);
void watch_add_item_deal(void)
{
    u32 i, j, k;
    u32 index = -1;
    u32 file_num;
    u8 *fname_buf = NULL;
    u8 *fname = NULL;
    u8 fname_len;

    virfat_flash_get_dirinfo(NULL, &file_num);

    fname_buf = zalloc(file_num * 12);
    if (NULL == fname_buf) {
        goto __watch_add_item_deal_end;
    }

    virfat_flash_get_dirinfo(fname_buf, &file_num);

    for (i = 0, k = 0; i < file_num; i++) {
        fname = fname_buf + i * 12;
        for (j = 0; j < 12; j++) {
            if (' ' == fname[j]) {
                fname[j] = '\0';
                break;
            }
        }
        fname_len = strlen(fname);
        ASCII_ToLower(fname, fname_len);
        if (0 == strncmp(fname, "watch", strlen("watch"))) {
            fname_len = rindex(watch_res[k], '.') - rindex(watch_res[k], '/') - 1;
            if (0 != strncmp(fname, rindex(watch_res[k], '/') + 1, fname_len)) {
                // 假如名字不一样，则往下找一样的名字，并进行交换
                for (index = k + 1; index < sizeof(watch_res) / sizeof(watch_res[0]); index++) {
                    fname_len = rindex(watch_res[index], '.') - rindex(watch_res[index], '/') - 1;
                    if (0 == strncmp(fname, rindex(watch_res[index], '/') + 1, fname_len)) {
                        fname = watch_res[index];
                        for (; index > k; index --) {
                            watch_res[index] = watch_res[index - 1];
                            watch_bgp_related[index] = watch_bgp_related[index - 1];
                            if (watch_style == (index - 1)) {
                                watch_style = index;
                            }
                        }
                        watch_res[k] = fname;
                        break;
                    }
                }
            }
            k++;
        }
    }

__watch_add_item_deal_end:
    if (fname_buf) {
        free(fname_buf);
    }
    watch_mem_bgp_related();
}

int watch_add_item(char *s)
{
    char *new_item = NULL;
    /* char watch_name[16]; */
    u32 len;
    char *suffix = ".sty";


    char *root_path = RES_PATH;
    char watch[64];

    ASSERT(((strlen(s) + strlen(root_path) + 1) < sizeof(watch)), "err name3 %s\n", s);
    /* if ((strlen(s) + strlen(root_path) + 1) >= sizeof(watch)) { */
    /* printf("err name3 %s\n", s); */
    /* while (1); */
    /* } */

    ASCII_ToLower(s, strlen(s));
    strcpy(watch, root_path);
    strcpy(&watch[strlen(root_path)], &s[1]);
    s = watch;

    printf("watch_path %s\n", s);

    ASSERT(((watch_items + 1) < WATCH_ITEMS_LIMIT), "watch items overflow %d\n\n\n\n", watch_items);
    /* if ((watch_items + 1) >= WATCH_ITEMS_LIMIT) { */
    /* printf("watch items overflow %d\n\n\n\n", watch_items); */
    /* while (1); */
    /* } */
    len = strlen(s) - strlen(RES_PATH);
    /* printf("test len %d, %d, %d, %s, %s\n", strlen(s), strlen(RES_PATH), len, RES_PATH, s, s[len]); */

    /* strcpy(watch_name, &s[strlen(s) - len]); */
    /* printf("end str %s\n", watch_name); */

    new_item = malloc(strlen(s) + 1 + len + strlen(suffix) + 1);
    if (new_item == NULL) {
        printf("watch add item fail\n");
        return -1;
    }
    strcpy(new_item, s);
    new_item[strlen(s)] = '/';
    strcpy(&new_item[strlen(s) + 1], &s[strlen(s) - len]);
    strcpy(&new_item[strlen(s) + 1 + len], suffix);
    new_item[strlen(s) + 1 + len + strlen(suffix)] = '\0';

    for (u8 i = 0; i < watch_items; i++) {
        if (0 == strcmp(new_item, watch_res[i])) {
            printf("repeat : %s, %s\n", new_item, watch_res[i]);
            free(new_item);
            return 0;
        }
    }

    watch_res[watch_items] = new_item;
    watch_items++;


    printf("watch add item succ %d, %s\n", watch_items, new_item);

    return 0;
}

int watch_del_item(char *s)
{
    u32 i;
    char *item;
    u32 cur_items = watch_items;

    char *root_path = RES_PATH;
    char watch[64];
    char *cur_watch = watch_res[watch_style];

    ASSERT(((strlen(s) + strlen(root_path) + 1) < sizeof(watch)), "err name5 %s\n", s);
    /* if ((strlen(s) + strlen(root_path) + 1) >= sizeof(watch)) { */
    /* printf("err name5 %s\n", s); */
    /* while (1); */
    /* } */
    ASCII_ToLower(s, strlen(s));
    strcpy(watch, root_path);
    strcpy(&watch[strlen(root_path)], &s[1]);
    s = watch;

    printf("watch_path %s\n", s);

    if (watch_items <= 2) {
        return -1;
    }

    for (i = 0; i < cur_items; i++) {
        if (strncmp(s, watch_res[i], strlen(s)) == 0) {
            item = watch_res[i];
            watch_res[i] = NULL;
            free(item);
            watch_items--;

            watch_bgp_related[i] = NULL;
            break;
        }
    }

    for (; i < cur_items; i++) {
        if (watch_res[i + 1] != NULL) {
            watch_res[i] = watch_res[i + 1];

            watch_bgp_related[i] = watch_bgp_related[i + 1];
        } else {
            watch_res[i] = NULL;

            watch_bgp_related[i] = NULL;
            break;
        }
    }

    for (i = 0; i < watch_items; i++) {
        if (cur_watch == watch_res[i]) {
            printf("del set style %d, %s\n", i, watch_res[i]);
            watch_set_style(i);
            break;
        }
    }
    if (i == watch_items) {
        printf("end style\n");
        watch_set_style(0);
    }

    watch_mem_bgp_related();

    for (i = 0; i < watch_items; i++) {
        printf("cur watch item %d, %s\n", watch_items, watch_res[i]);
    }

    return 0;
}

void watch_select_mem()
{
#if UI_WATCH_RES_ENABLE
    syscfg_write(VM_WATCH_SELECT, &watch_style, sizeof(watch_style));
#endif
}

int watch_set_init()
{
#if UI_WATCH_RES_ENABLE
    u32 i, j;
    u32 len;
    u32 file_num;
    char *fname_buf;
    char *suffix = ".sty";
    u8 root_path_len = strlen(RES_PATH);
    u8 suffix_len = strlen(suffix);
    u8 fname_len;
    char *fname;


    for (i = 0; i < WATCH_ITEMS_LIMIT; i++) {
        if (watch_res[i] != NULL) {
            free(watch_res[i]);
            watch_res[i] = NULL;
        }
    }
    watch_style = 0;
    watch_items = 0;


    for (i = 0; i < BGP_ITEMS_LIMIT; i++) {
        if (watch_bgp[i]) {
            free(watch_bgp[i]);
            watch_bgp[i] = NULL;
        }
    }
    watch_bgp_items = 0;


    virfat_flash_get_dirinfo(NULL, &file_num);

    fname_buf = zalloc(file_num * 12);
    /* ASSERT((fname_buf != NULL), "fname_buf zalloc faile\n"); */
    if (fname_buf == NULL) {
        printf("fname_buf zalloc faile %d\n", file_num);
        /* while (1); */
        return -1;
    }

    virfat_flash_get_dirinfo(fname_buf, &file_num);

    /* printf("fnum %d\n", file_num); */
    /* ASSERT((file_num < WATCH_ITEMS_LIMIT), "file num overflow %d\n", file_num); */
    /* if (file_num >= WATCH_ITEMS_LIMIT) { */
    if (file_num >= BGP_ITEMS_LIMIT) {
        printf("file num overflow %d\n", file_num);
        free(fname_buf);
        /* while (1); */
        return -1;
    }

    for (i = 0; i < file_num; i++) {
        fname = &fname_buf[i * 12];
        for (j = 0; j < 12; j++) {
            /* printf("j %x, %x\n", j, fname[j]); */
            if (fname[j] == ' ') {
                fname[j] = '\0';
                break;
            }
        }
        /* ASSERT((j != 12), "\n\n\n\nfname overflow\n\n\n\n\n"); */
        if (j == 12) {
            printf("\n\n\n\nfname overflow\n\n\n\n\n");
            free(fname_buf);
            /* while (1); */
            return -1;
        }
        fname_len = strlen(fname);
        ASCII_ToLower(fname, fname_len);
        //printf("fname %d, %d, %s\n", j, fname_len, fname);
        /* if (strncmp(fname, "watch", strlen("watch")) != 0) { */
        /* continue; */
        /* } */

        if (strncmp(fname, "watch", strlen("watch")) == 0) {
            len = root_path_len + fname_len + 1 + fname_len + suffix_len;
            watch_res[watch_items] = malloc(len + 1);
            /* ASSERT((watch_res[watch_items] != NULL), "\n\nmalloc watch list err\n\n"); */
            if (watch_res[watch_items] == NULL) {
                printf("\n\nmalloc watch list err\n\n");
                free(fname_buf);
                /* while (1); */
                return -1;
            }
            strcpy(watch_res[watch_items], RES_PATH);
            strcpy(&watch_res[watch_items][root_path_len], fname);
            watch_res[watch_items][root_path_len + fname_len] = '/';
            strcpy(&watch_res[watch_items][root_path_len + fname_len + 1], fname);
            strcpy(&watch_res[watch_items][root_path_len + fname_len + 1 + fname_len], suffix);
            watch_res[watch_items][len] = '\0';

            printf("watch list : %s, %d, %d\n", watch_res[watch_items], watch_items, len);

            watch_items++;

        } else if (strncmp(fname, "bgp_w", strlen("bgp_w")) == 0) {
            watch_bgp_add(fname);
        }

    }


    free(fname_buf);

    if (watch_bgp_related_init() != 0) {
        printf("bgp_related_init fail\n");
    } else {
        printf("bgp_related_init succ\n");
    }

    if ((sizeof(watch_style) != syscfg_read(VM_WATCH_SELECT, &watch_style, sizeof(watch_style))) || (watch_style >= watch_items)) {
        watch_style = 0;
    }


    /* if (wmem_test_flag == 1) { */
    /* watch_bgp_set_related("bgp_w0", 0); */
    /* watch_bgp_set_related("bgp_w2", 1); */
    /* watch_bgp_set_related("bgp_w1", 2); */
    /* watch_bgp_set_related("bgp_w0", 3); */
    /* watch_bgp_set_related("bgp_w1", 5); */
    /* } */


    /* watch_bgp_add("dial0.bin"); */
    /* watch_bgp_add("dial1.bin"); */
    /* watch_bgp_add("dial2.bin"); */


    /* watch_bgp_del("dial1.bin"); */

    /* watch_bgp_set_related("dial0.bin", 0); */
    /* watch_bgp_set_related("dial2.bin", 1); */
    /* watch_bgp_set_related("dial1.bin", 2); */
    /* watch_bgp_set_related("dial2.bin", 3); */
    /* watch_bgp_set_related("dial1.bin", 4); */
    /* watch_bgp_set_related("dial0.bin", 5); */

#endif

    return 0;
}



AT_UI_RAM
static void *br23_load_widget_info(void *__head, u8 page)
{
    struct ui_file_head head ALIGNED(4);
    static union ui_control_info info ALIGNED(4) = {0};
    static const int rotate[] = {0, 90, 180, 270};
    static int last_page = -1;
    int head_len;
    int retry = 10;
    u32 offset;
    u32 _head = ((u32)__head) & 0xffff;


    if ((((u32)__head >> 29) & 0x7)) {
        if ((((u32)__head >> 29) & 0x7) == 2) {//by yyj
            printf("%s \n", watch_res[watch_style]);
            ui_set_sty_path_by_pj_id(2, "mnt/sdfile/res/ui_res/prj2.sty");
            /* printf(" >>>>>>>>%s %d %x\n",__FUNCTION__,__LINE__,(int)__head >> 29); */
        }
        ui_file = ui_load_sty_by_pj_id((((u32)__head >> 29) & 0x7));

    } else {
        ui_file = ui_file1;
    }

    if (!_head) {
        struct ui_style style = {0};

#if UI_WATCH_RES_ENABLE
        int load_style = 0;
        if (last_page == -1) { //第一次上电未初始化
            if (page == 0) { //表盘界面
                style.file = watch_res[watch_style];
            } else {//其他界面
                style.file = RES_PATH"JL/JL.sty";
            }
            load_style = 1;
        } else if ((page == 0) && (last_page == 0)) { // 表盘 -> 表盘
            style.file = watch_res[watch_style];
            load_style = 1;
        } else if ((page == 0) && (last_page > 0)) { //其他 -> 表盘
            style.file = watch_res[watch_style];
            load_style = 1;
        } else if ((page > 0) && (last_page == 0)) {//表盘 -> 其他
            style.file = RES_PATH"JL/JL.sty";
            load_style = 1;
        }

        last_page = page;
#if UI_UPGRADE_RES_ENABLE//升级模式加载资源
        if (app_get_curr_task() == APP_WATCH_UPDATE_TASK ||
            app_get_curr_task() == APP_SMARTBOX_ACTION_TASK) {
            ui_set_sty_path_by_pj_id(1, NULL);
            style.file = UPGRADE_PATH"upgrade.sty";
            load_style = 1;
            last_page = -1;//下次能重新加载资源
            if (page) {
                printf("NOW IS UPGRADE MODE ,CAN NOT SHOW OTHER PAGE...\n");
                page = 0;
            }
        }
#endif

#else
        int load_style = 1;
        last_page = page;
#endif

        if (load_style && platform_api->load_style) {
            if (platform_api->load_style(&style)) {
                return NULL;
            }
            load_style = 0;
        }
        if (!ui_file) {
            return NULL;
        }

        /* last_page = page; */
        res_fseek(ui_file, 0, SEEK_SET);
        res_fread(ui_file, &head, sizeof(struct ui_file_head));
        ui_rotate = rotate[head.rotate];
        ui_core_set_rotate(ui_rotate);
        switch (head.rotate) {
        case 1: /* 旋转90度 */
            ui_hori_mirror = true;
            ui_vert_mirror = false;
            break;
        case 3:/* 旋转270度 */
            ui_hori_mirror = false;
            ui_vert_mirror = true;
            break;
        default:
            ui_hori_mirror = false;
            ui_vert_mirror = false;
            break;
        }
        if (page != (u8) - 1) {
            res_fseek(ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, SEEK_SET);
            res_fread(ui_file, &__this->window_offset, sizeof(__this->window_offset));
        }
    }

    ASSERT((u16)_head <= ui_file_len, ",_head invalid! _head : 0x%x ui_file_len : 0x%x\n", (u32)_head, ui_file_len);

    if ((u16)_head > ui_file_len) {
        return NULL;
    }

    if ((u32)__head >= 0x10000) {
        page = ((u32)__head >> 22) & 0x7f;

        struct mem_var_list *list;
        if ((list = mem_var_search(1, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head) * page, page, (((u32)__head >> 29) & 0x7))) != NULL) {
            mem_var_get(list, &offset, sizeof(__this->window_offset));
        } else {
            res_fseek(ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, SEEK_SET);
            res_fread(ui_file, &offset, sizeof(__this->window_offset));
            mem_var_add(1, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, page, (((u32)__head >> 29) & 0x7), &offset, sizeof(__this->window_offset));
        }
    } else {
        offset = __this->window_offset;
    }

    res_fseek(ui_file, offset + (u32)_head, SEEK_SET);
    if ((u32)_head == 0) {
        res_fread(ui_file, &info, sizeof(struct window_info));
    } else {
        res_fread(ui_file, &info.head, sizeof(struct ui_ctrl_info_head));
        head_len = info.head.len - sizeof(struct ui_ctrl_info_head);
        if ((head_len > 0) && (head_len <= sizeof(union ui_control_info))) {
            res_fread(ui_file, &((u8 *)&info)[sizeof(struct ui_ctrl_info_head)], head_len);
        }
    }

    return &info;
}

static void *__load_widget_info(void *__head, u8 page)
{
    static union ui_control_info info ALIGNED(4) = {0};
    struct mem_var_list *list;
    if ((list = mem_var_search(2, (u32)ui_file, (u32)__head, page, (((u32)__head >> 29) & 0x7))) != NULL) {
        mem_var_get(list, &info, sizeof(union ui_control_info));
    } else {
        void *_info = br23_load_widget_info(__head, page);
        memcpy(&info, _info, sizeof(union ui_control_info));
        mem_var_add(2, (u32)ui_file, (u32)__head, page, (((u32)__head >> 29) & 0x7), &info, sizeof(union ui_control_info));
    }
    return &info;
}



AT_UI_RAM
static void *br23_load_css(u8 page, void *_css)
{
    static struct element_css1 css ALIGNED(4) = {0};
    u32 offset;

    if ((((u32)_css >> 29) & 0x7)) {
        ui_file = ui_load_sty_by_pj_id((((u32)_css >> 29) & 0x7));
    } else {
        ui_file = ui_file1;
    }

    if ((u32)_css >= 0x10000) {
        page = ((u32)_css >> 22) & 0x7f;
    }
    struct mem_var_list *list;
    if ((list = mem_var_search(3, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head) * page, page, (((u32)_css >> 29) & 0x7))) != NULL) {
        mem_var_get(list, &offset, sizeof(offset));
    } else {
        res_fseek(ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, SEEK_SET);
        res_fread(ui_file, &offset, sizeof(offset));
        mem_var_add(3, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, page, (((u32)_css >> 29) & 0x7), &offset, sizeof(offset));
    }
    ASSERT((u16)_css <= ui_file_len, ", _css invalid! _css : 0x%x , ui_file_len : 0x%x\n", _css, ui_file_len);


    res_fseek(ui_file, offset/* __this->window_offset */ + ((u32)_css & 0xffff), SEEK_SET);
    res_fread(ui_file, &css, sizeof(struct element_css1));

    return &css;
}





AT_UI_RAM
static void *br23_load_image_list(u8 page, void *_list)
{
    /* u16 image[32] ALIGNED(4); */
    static struct ui_image_list_t list ALIGNED(4) = {0};
    int retry = 10;
    u32 offset;

    /* printf(" >>>>>>>>%s %d %x\n",__FUNCTION__,__LINE__,(int)_list >> 29); */
    if ((((u32)_list >> 29) & 0x7)) {
        ui_file = ui_load_sty_by_pj_id((((u32)_list >> 29) & 0x7));
    } else {
        ui_file = ui_file1;
    }
    if ((u32)_list >= 0x10000) {
        page = ((u32)_list >> 22) & 0x7f;
    }

    struct mem_var_list *mem_list;
    if ((mem_list = mem_var_search(4, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head) * page, page, (((u32)_list >> 29) & 0x7))) != NULL) {
        mem_var_get(mem_list, &offset, sizeof(offset));
    } else {
        res_fseek(ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, SEEK_SET);
        res_fread(ui_file, &offset, sizeof(offset));
        mem_var_add(4, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, page, (((u32)_list >> 29) & 0x7), &offset, sizeof(offset));
    }
    if ((u32)_list == 0) {
        return NULL;
    }

    ASSERT((u16)_list <= ui_file_len, ", _list invalid! _list : 0x%x, ui_file_len : 0x%x\n", (u32)_list, ui_file_len);

    do {
        memset(&list, 0x00, sizeof(struct ui_image_list));

        struct mem_var_list *mem_list;
        if ((mem_list = mem_var_search(5, (u32)ui_file, offset + ((u32)_list & 0xffff), page, (((u32)_list >> 29) & 0x7))) != NULL) {
            mem_var_get(mem_list, &list.num, sizeof(list.num));
        } else {
            res_fseek(ui_file, offset + ((u32)_list & 0xffff), SEEK_SET);
            res_fread(ui_file, &list.num, sizeof(list.num));
            mem_var_add(5, (u32)ui_file, offset + ((u32)_list & 0xffff), page, (((u32)_list >> 29) & 0x7), &list.num, sizeof(list.num));
        }

        /* res_fseek(ui_file, offset + ((u32)_list & 0xffff), SEEK_SET); */
        /* res_fread(ui_file, &list.num, sizeof(list.num)); */
        if (list.num == 0) {
            printf("list.num : %d\n", list.num);
            return NULL;
        }
        if (list.num < 32) {
            res_fseek(ui_file, offset + ((u32)_list & 0xffff) + sizeof(list.num), SEEK_SET);
            res_fread(ui_file, list.image, list.num * sizeof(list.image[0]));
            /* memcpy(list.image, image, list.num * sizeof(list.image[0])); */
            break;
        } else {
            printf("list.num = %d\n", list.num);
            printf("load_image_list error,retry %d!\n", retry);
            if (retry == 0) {
                return NULL;
            }
        }
    } while (retry--);

    return &list;
}


static void *__load_image_list(u8 page, void *_list)
{
    static struct ui_image_list_t image_list ALIGNED(4) = {0};
    struct mem_var_list *list;

    if ((list = mem_var_search(6, (u32)ui_file, (u32)_list, page, (((u32)_list >> 29) & 0x7))) != NULL) {
        mem_var_get(list, &image_list, sizeof(struct ui_image_list_t));
    } else {
        void *_info = br23_load_image_list(page, _list);
        if (_info == NULL) {
            return NULL;
        }
        memcpy(&image_list, _info, sizeof(struct ui_image_list_t));
        mem_var_add(6, (u32)ui_file, (u32)_list, page, (((u32)_list >> 29) & 0x7), &image_list, sizeof(struct ui_image_list_t));
    }
    return &image_list;
}


AT_UI_RAM
static void *br23_load_text_list(u8 page, void *__list)
{
    u8 buf[16 * 2] ALIGNED(4);
    static struct ui_text_list_t _list ALIGNED(4) = {0};
    struct ui_text_list_t *list;
    int retry = 10;
    int i;
    u32 offset;



    /* printf(" >>>>>>>>%s %d %x\n",__FUNCTION__,__LINE__,(u32)__list >> 29); */
    if ((((u32)__list >> 29) & 0x7)) {
        ui_file = ui_load_sty_by_pj_id((((u32)__list >> 29) & 0x7));
    } else {
        ui_file = ui_file1;
    }


    if (!ui_file) {
        return NULL;
    }


    if ((u32)__list >= 0x10000) {
        page = ((u32)__list >> 22) & 0x7f;
    }

    struct mem_var_list *mem_list;
    if ((mem_list = mem_var_search(7, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head) * page, page, (((u32)__list >> 29) & 0x7))) != NULL) {
        mem_var_get(mem_list, &offset, sizeof(offset));
    } else {
        res_fseek(ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, SEEK_SET);
        res_fread(ui_file, &offset, sizeof(offset));
        mem_var_add(7, (u32)ui_file, sizeof(struct ui_file_head) + sizeof(struct window_head)*page, page, (((u32)__list >> 29) & 0x7), &offset, sizeof(offset));
    }
    if ((u32)__list == 0) {
        return NULL;
    }

    ASSERT((u16)__list <= ui_file_len, ", __list invalid! _list : 0x%x, ui_file_len : 0x%x\n", (u32)__list, ui_file_len);

    list = &_list;
    do {
        memset(list, 0x00, sizeof(struct ui_text_list_t));

        struct mem_var_list *mem_list;
        if ((mem_list = mem_var_search(8, (u32)ui_file, offset + ((u32)__list & 0xffff), page, (((u32)__list >> 29) & 0x7))) != NULL) {
            mem_var_get(mem_list, &list->num, sizeof(list->num));
        } else {
            res_fseek(ui_file, offset + ((u32)__list & 0xffff), SEEK_SET);
            res_fread(ui_file, &list->num, sizeof(list->num));
            mem_var_add(8, (u32)ui_file, offset + ((u32)__list & 0xffff), page, (((u32)__list >> 29) & 0x7), &list->num, sizeof(list->num));
        }

        /* res_fseek(ui_file, offset + ((u32)__list & 0xffff), SEEK_SET); */
        /* res_fread(ui_file, &list->num, sizeof(list->num)); */
        if (list->num == 0) {
            return NULL;
        }
        if (list->num <= 16) {
            res_fseek(ui_file, offset + ((u32)__list & 0xffff) + sizeof(list->num), SEEK_SET);
            res_fread(ui_file, buf, list->num * 2);
            for (i = 0; i < list->num; i++) {
                ASSERT(buf[2 * i] < 0x100);
                list->str[i] = buf[2 * i] | (buf[2 * i + 1] << 8);
            }
            break;
        } else {
            printf("list->num = %d\n", list->num);
            printf("load_text_list error, retry %d!\n", retry);
            if (retry == 0) {
                return NULL;
            }
        }
    } while (retry--);
    return list;
}



static void *br23_load_window(int id)
{
    u8 *ui;
    int i;
    u32 *ptr;
    u16 *ptr_table;
    struct ui_file_head head ALIGNED(4);
    struct window_head window ALIGNED(4);
    int len = sizeof(struct ui_file_head);
    int retry;
    static const int rotate[] = {0, 90, 180, 270};


    if (!ui_file) {
        printf("ui_file : 0x%x\n", ui_file);
        return NULL;
    }
    ui_platform_ok();

    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(ui_file, 0, SEEK_SET);
        res_fread(ui_file, &head, len);

        if (id >= head.window_num) {
            return NULL;
        }

        res_fseek(ui_file, sizeof(struct window_head)*id, SEEK_CUR);
        res_fread(ui_file, &window, sizeof(struct window_head));

        u16 crc = CRC16(&window, (u32) & (((struct window_head *)0)->crc_data));
        if (crc == window.crc_head) {
            ui_rotate = rotate[head.rotate];
            ui_core_set_rotate(ui_rotate);
            switch (head.rotate) {
            case 1: /* 旋转90度 */
                ui_hori_mirror = true;
                ui_vert_mirror = false;
                break;
            case 3:/* 旋转270度 */
                ui_hori_mirror = false;
                ui_vert_mirror = true;
                break;
            default:
                ui_hori_mirror = false;
                ui_vert_mirror = false;
                break;
            }
            goto __read_data;
        }
    }

    return NULL;

__read_data:
    ui = (u8 *)__this->api->malloc(window.len);
    if (!ui) {
        return NULL;
    }
    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(ui_file, window.offset, SEEK_SET);
        res_fread(ui_file, ui, window.len);

        u16 crc = CRC16(ui, window.len);
        if (crc == window.crc_data) {
            goto __read_table;
        }
    }

    __this->api->free(ui);
    return NULL;

__read_table:
    ptr_table = (u16 *)__this->api->malloc(window.ptr_table_len);
    if (!ptr_table) {
        __this->api->free(ui);
        return NULL;
    }
    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(ui_file, window.ptr_table_offset, SEEK_SET);
        res_fread(ui_file, ptr_table, window.ptr_table_len);

        u16 crc = CRC16(ptr_table, window.ptr_table_len);
        if (crc == window.crc_table) {
            u16 *offset = ptr_table;
            for (i = 0; i < window.ptr_table_len; i += 2) {
                ptr = (u32 *)(ui + *offset++);
                if (*ptr != 0) {
                    *ptr += (u32)ui;
                }
            }
            __this->api->free(ptr_table);
            return ui;
        }
    }

    __this->api->free(ui);
    __this->api->free(ptr_table);

    return NULL;
}

static void br23_unload_window(void *ui)
{
    if (ui) {
        __this->api->free(ui);
    }
}


static int br23_load_style(struct ui_style *style)
{
    int err;
    int i, j;
    int len;
    struct vfscan *fs;
    char name[64];
    char style_name[16];
    static char cur_style = 0xff;


    if (!style->file && cur_style == 0) {
        return 0;
    }

    if (ui_file1) {
        fclose(ui_file1);
        ui_file1 = NULL;
    }

    if (style->file == NULL) {
        cur_style = 0;
        err = open_resource_file();
        if (err) {
            return -EINVAL;
        }
#if 0
        fs = fscan("mnt/spiflash/res", "-t*.sty");
        if (!fs) {
            printf("open mnt/spiflash/res fail!\n");
            return -EFAULT;
        }
        ui_file1 = fselect(fs, FSEL_FIRST_FILE, 0);
        if (!ui_file1) {
            fscan_release(fs);
            return -ENOENT;
        }
        ui_file = ui_file1;
        len = fget_name(ui_file1, (u8 *)name, 16);
        if (len) {
            style_name[len - 4] = 0;
            memcpy(style_name, name, len - 4);
            ui_core_set_style(style_name);
        }

        fscan_release(fs);
#else
        ui_file1 = res_fopen(RES_PATH"JL.sty", "r");
        if (!ui_file1) {
            printf("[UI RESOURCES LOAD FAIL(4/4)] : JL.sty\n");
            return -ENOENT;
        } else {
            printf("[UI RESOURCES LOAD SUCCESS (4/4)] : JL.sty\n");
        }
        ui_file = ui_file1;
        ui_file_len = res_flen(ui_file1);
        len = 6;
        strcpy(style_name, "JL.sty");
        if (len) {
            style_name[len - 4] = 0;
            ui_core_set_style(style_name);
        }
#endif
    } else {
        cur_style = 1;
        printf("open stylefile %s\n", style->file);
        ui_file1 = res_fopen(style->file, "r");
        if (!ui_file1) {
            return -EINVAL;
        }


        /* if (!ui_file2) { */
        /*     ui_file2 = fopen(RES_PATH"sidebar/sidebar.sty", "r"); */
        /* } */

        ui_file = ui_file1;

        ui_file_len = 0xffffffff;//res_flen(ui_file1);
        for (i = 0; style->file[i] != '.'; i++) {
            name[i] = style->file[i];
        }
        printf("open resfile 0 :%s\n", name);
        name[i++] = '.';
        name[i++] = 'r';
        name[i++] = 'e';
        name[i++] = 's';
        name[i] = '\0';
        printf("open resfile %s\n", name);
        open_resfile(name);

        name[--i] = 'r';
        name[--i] = 't';
        name[--i] = 's';
        printf("open strfile %s\n", name);
        open_str_file(name);

        name[i++] = 'a';
        name[i++] = 's';
        name[i++] = 'i';
        printf("open asciifile %s\n", name);
        font_ascii_init(RES_PATH"font/ascii.res");

        for (i = strlen(style->file) - 5; i >= 0; i--) {
            if (style->file[i] == '/') {
                break;
            }
        }

        for (i++, j = 0; style->file[i] != '\0'; i++) {
            if (style->file[i] == '.') {
                name[j] = '\0';
                break;
            }
            name[j++] = style->file[i];
        }
        ASCII_ToUpper(name, j);
        if (!strncmp(name, "WATCH", strlen("WATCH"))) {
            strcpy(name, "DIAL");
        }
        err = ui_core_set_style(name);
        if (err) {
            printf("style_err: %s\n", name);
        }
    }

    return 0;

__err2:
    close_resfile();
__err1:
    fclose(ui_file1);
    ui_file1 = NULL;

    return err;
}




static int br23_open_draw_context(struct draw_context *dc)
{
    dc->buf_num = 1;
    if (__this->lcd->buffer_malloc) {
        u8 *buf;
        u32 len;
        __this->lcd->buffer_malloc(&buf, &len);
        dc->buf0 = buf;
#if UI_USED_DOUBLE_BUFFER
        dc->buf1 = &buf[len / 2];
        dc->len  = len / 2;
#else
        dc->buf1 = NULL;
        dc->len  = len;
#endif
        dc->buf  = dc->buf0;
        /* __this->lcd->buffer_malloc(&dc->buf, &dc->len); */
    }

    if (__this->lcd->get_screen_info) {
        __this->lcd->get_screen_info(&__this->info);
        dc->width = __this->info.width;
        dc->height = __this->info.height;
        dc->col_align = __this->info.col_align;
        dc->row_align = __this->info.row_align;
        dc->lines = dc->len / dc->width / 2;
        if (dc->lines > (dc->height / 10)) {
            dc->lines = dc->height / 10;
        }
        printf("dc->width : %d, dc->lines : %d\n", dc->width, dc->lines);
    }
    switch (__this->info.color_format) {
    case LCD_COLOR_RGB565:

        if (dc->data_format != DC_DATA_FORMAT_OSD16) { //2
            ASSERT(0, "The color format of layer don't match the lcd driver,page %d please select OSD16!", dc->page);
        }
        break;
    case LCD_COLOR_MONO:
        if (dc->data_format != DC_DATA_FORMAT_MONO) {
            ASSERT(0, "The color format of layer don't match the lcd driver,page %d please select OSD1!", dc->page);
        }
        break;
    }
    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        dc->fbuf_len = __this->info.width * (3 + 3 * dc->lines) + 0x80;
        dc->fbuf = (u8 *)__this->api->malloc(dc->fbuf_len);
    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
        dc->fbuf_len = __this->info.width * 2;
        dc->fbuf = (u8 *)__this->api->malloc(dc->fbuf_len);
    }

    return 0;
}

static int br23_get_draw_context(struct draw_context *dc)
{
#if UI_USED_DOUBLE_BUFFER
    if (dc->buf == dc->buf0) {
        dc->buf = dc->buf1;
    } else {
        dc->buf = dc->buf0;
    }
#endif

    dc->disp.left  = dc->need_draw.left;
    dc->disp.width = dc->need_draw.width;
    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        int lines = dc->len / dc->need_draw.width / 2;

        if ((dc->disp.top == 0) && (dc->disp.height == 0)) {
            dc->disp.top   = dc->need_draw.top;
            dc->disp.height = lines > dc->need_draw.height ? dc->need_draw.height : lines;
        } else {
            dc->disp.top   = dc->disp.top + dc->disp.height;
            dc->disp.height = lines > (dc->need_draw.top + dc->need_draw.height - dc->disp.top) ?
                              (dc->need_draw.top + dc->need_draw.height - dc->disp.top) : lines;
        }
        dc->disp.height = dc->disp.height / dc->row_align * dc->row_align;
    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
        dc->disp.top = dc->need_draw.top;
        dc->disp.height = dc->need_draw.height;
    }

    return 0;
}

static int br23_put_draw_context(struct draw_context *dc)
{
    if (__this->lcd->set_draw_area) {
        __this->lcd->set_draw_area(dc->disp.left, dc->disp.left + dc->disp.width - 1,
                                   dc->disp.top, dc->disp.top + dc->disp.height - 1);
    }

    u8 wait = ((dc->need_draw.top + dc->need_draw.height) == (dc->disp.top + dc->disp.height)) ? 1 : 0;
    if (__this->lcd->draw) {
        if (dc->data_format == DC_DATA_FORMAT_OSD16) {
            __this->lcd->draw(dc->buf, dc->disp.height * dc->disp.width * 2, wait);
        } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
            __this->lcd->draw(dc->buf, __this->info.width * __this->info.height / 8, wait);
        }
    }
    return 0;
}


static int br23_set_draw_context(struct draw_context *dc)
{
    return 0;
}

static int br23_close_draw_context(struct draw_context *dc)
{
    if (__this->lcd->buffer_free) {
        __this->lcd->buffer_free(dc->buf);
    }
    if (dc->fbuf) {
        __this->api->free(dc->fbuf);
        dc->fbuf = NULL;
        dc->fbuf_len = 0;
    }

    return 0;
}

static int br23_invert_rect(struct draw_context *dc, u32 acolor)
{
    int i;
    int len;
    int w, h;
    int color = acolor & 0xffff;

    if (dc->data_format == DC_DATA_FORMAT_MONO) {
        color |= BIT(31);
        for (h = 0; h < dc->draw.height; h++) {
            for (w = 0; w < dc->draw.width; w++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, dc->draw.left + w, dc->draw.top + h, color);
                }
            }
        }
    }
    return 0;
}

static int br23_fill_rect(struct draw_context *dc, u32 acolor)
{
    int i;
    int w, h;
    u16 color = acolor & 0xffff;

    if (!dc->buf) {
        return 0;
    }

    if (dc->data_format == DC_DATA_FORMAT_MONO) {
        color = (color == UI_RGB565(BGC_MONO_SET)) ? 0xffff : 0x55aa;

        for (h = 0; h < dc->draw.height; h++) {
            for (w = 0; w < dc->draw.width; w++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, dc->draw.left + w, dc->draw.top + h, color);
                }
            }
        }
    } else {
        u16 color16 = (color >> 8) | ((color & 0xff) << 8);
        u32 color32 = (color16 << 16) | color16;

        h = 0;
        u32 *p32 = (u32 *)&dc->buf[(dc->draw.top + h - dc->disp.top) * dc->disp.width * 2 + (dc->draw.left - dc->disp.left) * 2];
        u32 *_p32 = p32;
        u32 len = dc->draw.width * 2;
        if ((u32)p32 % 4) {
            u16 *p16 = (u16 *)p32;
            *p16++ = color16;
            p32 = (u32 *)p16;
            len -= 2;
            ASSERT((u32)p32 % 4 == 0);
        }

        u32 count = len / 4;
#if USE_PI32V2_MATH == 0
        while (count--) {
            *p32++ = color32;
        }
#else
        pi32v2_fill_q31(color32, p32, count);
#endif
        count = (len % 4) / 2;
        u16 *p16 = (u16 *)p32;
#if USE_PI32V2_MATH == 0
        while (count--) {
            *p16++ = color16;
        }
#else
        pi32v2_fill_q15(color16, p16, count);
#endif

        int offset = (dc->draw.top - dc->disp.top) * dc->disp.width * 2 + (dc->draw.left - dc->disp.left) * 2;
#if USE_PI32V2_MATH
        if ((((u32)_p32 % 4) == 0) && (((u32)dc->disp.width % 2) == 0) && (((u32)&dc->buf[offset] % 4) == 0)) {
            for (h = 1; h < dc->draw.height; h++) {
                u32 *__p32 = (u32 *)&dc->buf[offset + h * dc->disp.width * 2];
                pi32v2_copy_q31(_p32, __p32, dc->draw.width / 2);
            }
        } else
#endif
        {
            for (h = 1; h < dc->draw.height; h++) {
                u32 *__p32 = (u32 *)&dc->buf[offset + h * dc->disp.width * 2];
                memcpy(__p32, _p32, dc->draw.width * 2);
            }
        }
    }

    return 0;
}


__attribute__((always_inline))
static inline void __draw_vertical_line(struct draw_context *dc, int x, int y, int width, int height, int color, int format)
{
    int i, j;
    struct rect r = {0};
    struct rect disp = {0};

    disp.left  = x;
    disp.top   = y;
    disp.width = width;
    disp.height = height;
    if (!get_rect_cover(&dc->draw, &disp, &r)) {
        return;
    }

    switch (format) {
    case DC_DATA_FORMAT_OSD16:
        for (i = 0; i < r.width; i++) {
            for (j = 0; j < r.height; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + i, r.top + j, color);
                }
            }
        }
        break;
    case DC_DATA_FORMAT_MONO:
        for (i = 0; i < r.width; i++) {
            for (j = 0; j < r.height; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + i, r.top + j, color);
                }
            }
        }
        break;

    }
}


__attribute__((always_inline))
static inline void __draw_line(struct draw_context *dc, int x, int y, int width, int height, int color, int format)
{
    int i, j;
    struct rect r = {0};
    struct rect disp = {0};

    disp.left  = x;
    disp.top   = y;
    disp.width = width;
    disp.height = height;
    if (!get_rect_cover(&dc->draw, &disp, &r)) {
        return;
    }

    switch (format) {
    case DC_DATA_FORMAT_OSD16:
        for (i = 0; i < r.height; i++) {
            for (j = 0; j < r.width; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + j, r.top + i, color);
                }
            }
        }
        break;
    case DC_DATA_FORMAT_MONO:
        for (i = 0; i < r.height; i++) {
            for (j = 0; j < r.width; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + j, r.top + i, color);
                }
            }
        }
        break;
    }
}

static int br23_draw_rect(struct draw_context *dc, struct css_border *border)
{
    int err;
    int offset;
    int color = border->color & 0xffff;

    /* draw_rect_range_check(&dc->draw, map); */
    /* draw_rect_range_check(&dc->rect, map); */

    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        color = border->color & 0xffff;
    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
        color = (color != UI_RGB565(RECT_MONO_CLR)) ? (color ? color : 0xffff) : 0x55aa;
    }

    if (border->left) {
        if (dc->rect.left >= dc->draw.left &&
            dc->rect.left <= rect_right(&dc->draw)) {
            __draw_vertical_line(dc, dc->draw.left, dc->draw.top,
                                 border->left, dc->draw.height, color, dc->data_format);
        }
    }
    if (border->right) {
        if (rect_right(&dc->rect) >= dc->draw.left &&
            rect_right(&dc->rect) <= rect_right(&dc->draw)) {
            __draw_vertical_line(dc, dc->draw.left + dc->draw.width - border->right, dc->draw.top,
                                 border->right, dc->draw.height, color, dc->data_format);
        }
    }
    if (border->top) {
        if (dc->rect.top >= dc->draw.top &&
            dc->rect.top <= rect_bottom(&dc->draw)) {
            __draw_line(dc, dc->draw.left, dc->draw.top,
                        dc->draw.width, border->top, color, dc->data_format);
        }
    }
    if (border->bottom) {
        if (rect_bottom(&dc->rect) >= dc->draw.top &&
            rect_bottom(&dc->rect) <= rect_bottom(&dc->draw)) {
            __draw_line(dc, dc->draw.left, dc->draw.top + dc->draw.height - border->bottom,
                        dc->draw.width, border->bottom, color, dc->data_format);
        }
    }

    return 0;
}

__attribute__((always_inline_when_const_args))
AT_UI_RAM
static u16 get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha)
{
    u16 mixed_color;
    u8 r0, g0, b0;
    u8 r1, g1, b1;
    u8 r2, g2, b2;

    if (alpha == 255) {
        return (forecolor >> 8) | (forecolor & 0xff) << 8;
    } else if (alpha == 0) {
        return (backcolor >> 8) | (backcolor & 0xff) << 8;
    }

    r0 = ((backcolor >> 11) & 0x1f) << 3;
    g0 = ((backcolor >> 5) & 0x3f) << 2;
    b0 = ((backcolor >> 0) & 0x1f) << 3;

    r1 = ((forecolor >> 11) & 0x1f) << 3;
    g1 = ((forecolor >> 5) & 0x3f) << 2;
    b1 = ((forecolor >> 0) & 0x1f) << 3;

    r2 = (alpha * r1 + (255 - alpha) * r0) / 255;
    g2 = (alpha * g1 + (255 - alpha) * g0) / 255;
    b2 = (alpha * b1 + (255 - alpha) * b0) / 255;

    mixed_color = ((r2 >> 3) << 11) | ((g2 >> 2) << 5) | (b2 >> 3);

    return (mixed_color >> 8) | (mixed_color & 0xff) << 8;
}

static int br23_read_image_info(struct draw_context *dc, u32 id, u8 page, struct ui_image_attrs *attrs)
{
    struct image_file file = {0};

    if (((u16) - 1 == id) || ((u32) - 1 == id)) {
        return -1;
    }

    select_resfile(dc->prj);
    int err = open_image_by_id(NULL, &file, id, dc->page);
    if (err) {
        return -EFAULT;
    }
    attrs->width = file.width;
    attrs->height = file.height;

    return 0;
}

AT_UI_RAM
int line_update(u8 *mask, u16 y, u16 width)
{
    int i;
    if (!mask) {
        return true;
    }
    for (i = 0; i < (width + 7) / 8; i++) {
        if (mask[y * ((width + 7) / 8) + i]) {
            return true;
        }
    }
    return false;
}

u16 crc_16(u16 crc_val, u8 dat)
{
    static const u16 crc_ta[16] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    };
    u8 da;

    da = crc_val >> 12;
    crc_val <<= 4;
    crc_val ^= crc_ta[da ^ (dat / 16)];

    da = crc_val >> 12;
    crc_val <<= 4;
    crc_val ^= crc_ta[da ^ (dat & 0x0f)];

    return crc_val;
}

static int dump_image_data(struct draw_context *dc, FILE *fp, struct image_file *file)
{
    printf("dump_image_data::\n");

    printf("image info:\n");
    printf("format : %d\n", file->format);
    printf("compress : %d\n", file->compress);
    printf("data_crc : 0x%x\n", file->data_crc);
    printf("width : %d\n", file->width);
    printf("height : %d\n", file->height);
    printf("offset : 0x%x\n", file->offset);
    printf("len : %d\n", file->len);

    u8 *fbuf = dc->fbuf;
    u32 buflen = dc->fbuf_len;
    u32 readlen;
    u32 totallen = file->len;
    u32 offset = 0;
    printf("image data:\n");
    u16 data_crc16 = 0;
    int i;

    void wdt_disable(void);
    wdt_disable();
    while (totallen) {
        readlen = totallen > buflen ? buflen : totallen;
        br23_read_image_data(fp, file, fbuf, readlen, offset);
        put_buf(fbuf, readlen);
        for (i = 0; i < readlen; i++) {
            data_crc16 = crc_16(data_crc16, fbuf[i]);
        }
        offset += readlen;
        totallen -= readlen;
    }
    if (file->data_crc != data_crc16) {
        printf("data crc err!\n");
        printf("orig crc : 0x%x\n", file->data_crc);
        printf("calc crc : 0x%x\n", data_crc16);
    }
    void wdt_enable(void);
    wdt_enable();

    return 0;
}

#define MAX_RETRY_TIMES 3
/* AT_UI_RAM */
void select_resfile(u8 index);
void select_strfile(u8 index);

struct filepos {
    u32 offset;
    u32 len;
};

static int dump_spec_data(struct draw_context *dc, FILE *fp, struct image_file *file, struct filepos *pos, int num)
{
    int i;

    printf("dump_spec_data::\n");
    printf("image info:\n");
    printf("format : %d\n", file->format);
    printf("compress : %d\n", file->compress);
    printf("data_crc : 0x%x\n", file->data_crc);
    printf("width : %d\n", file->width);
    printf("height : %d\n", file->height);
    printf("offset : 0x%x\n", file->offset);
    printf("len : %d\n", file->len);

    void wdt_disable(void);
    wdt_disable();
    for (i = 0; i < num; i++) {
        u8 *fbuf = dc->fbuf;
        u32 buflen = dc->fbuf_len;
        u32 readlen;
        u32 totallen = pos[i].len;
        u32 offset = pos[i].offset;

        printf("read file data: 0x%x ~ 0x%x, %d\n", file->offset + offset, file->offset + offset + totallen - 1, totallen);
        u16 data_crc16 = 0;
        int i;

        while (totallen) {
            readlen = totallen > buflen ? buflen : totallen;
            br23_read_image_data(fp, file, fbuf, readlen, offset);
            put_buf(fbuf, readlen);
            offset += readlen;
            totallen -= readlen;
        }
    }
    void wdt_enable(void);
    wdt_enable();

    return 0;
}

static int br23_draw_image(struct draw_context *dc, u32 src, u8 quadrant, u8 *mask)
{
    u8 *pixelbuf;
    u8 *temp_pixelbuf;
    u8 *alphabuf;
    u8 *temp_alphabuf;
    struct rect draw_r;
    struct rect r = {0};
    struct rect disp = {0};
    struct image_file file = {0};
    int h, hh, w;
    int buf_offset;
    int id;
    int page;
    FILE *fp;
    int retry_cnt = 0;
    struct filepos filepos[4];

    if (dc->preview.file) {
        fp = dc->preview.file;
        id = dc->preview.id;
        page = dc->preview.page;
    } else {
        fp = NULL;
        id = src;
        page = dc->page;
    }

    if (((u16) - 1 == id) || ((u32) - 1 == id)) {
        return -1;
    }

    draw_r.left   = dc->draw.left;//by yyj 解决滑块控件显示异常问题    dc->disp.left > dc->draw.left
    draw_r.top    = dc->draw.top;
    draw_r.width  = dc->draw.width;
    draw_r.height = dc->draw.height;

    /* UI_PRINTF("image draw %d, %d, %d, %d\n", dc->draw.left, dc->draw.top, dc->draw.width, dc->draw.height); */
    /* UI_PRINTF("image rect %d, %d, %d, %d\n", dc->rect.left, dc->rect.top, dc->rect.width, dc->rect.height); */

    select_resfile(dc->prj);

    if (dc->draw_img.en) {
        id = dc->draw_img.id;
        page = dc->draw_img.page;
    }


    struct mem_var_list *list;
    if ((list = mem_var_search(9, (u32)fp, id, page, dc->prj)) != NULL) {
        mem_var_get(list, &file, sizeof(struct image_file));
    } else {
        int err = open_image_by_id(fp, &file, id, page);
        if (err) {
            printf("%s, read file err\n", __func__);
            return -EFAULT;
        }
        mem_var_add(9, (u32)fp, id, page, dc->prj, &file, sizeof(struct image_file));
    }

    int x = dc->rect.left;
    int y = dc->rect.top;

    if (dc->align == UI_ALIGN_CENTER) {
        x += (dc->rect.width / 2 - file.width / 2);
        y += (dc->rect.height / 2 - file.height / 2);
    } else if (dc->align == UI_ALIGN_RIGHT) {
        x += dc->rect.width - file.width;
    }

    int temp_pixelbuf_len;
    int temp_alphabuf_len;
    int align;

    if (dc->draw_img.en) {
        disp.left   = dc->draw_img.x;
        disp.top    = dc->draw_img.y;
    } else {
        disp.left   = x;
        disp.top    = y;
    }
    disp.width  = file.width;
    disp.height = file.height;

    if (dc->data_format == DC_DATA_FORMAT_MONO) {
        pixelbuf = dc->fbuf;
        if (get_rect_cover(&draw_r, &disp, &r)) {
            int _offset = -1;
            for (h = 0; h < r.height; h++) {
                if (file.compress == 0) {
                    int offset = (r.top + h - disp.top) / 8 * file.width + (r.left - disp.left);
                    if (_offset != offset) {
                        if (br23_read_image_data(fp, &file, pixelbuf, r.width, offset) != r.width) {
                            return -EFAULT;
                        }
                        _offset = offset;
                    }
                } else {
                    ASSERT(0, "the compress mode not support!");
                }

                for (w = 0; w < r.width; w++) {
                    u8 color = (pixelbuf[w] & BIT((r.top + h - disp.top) % 8)) ? 1 : 0;
                    if (color) {
                        if (platform_api->draw_point) {
                            platform_api->draw_point(dc, r.left + w, r.top + h, color);
                        }
                    }
                }
            }
        }
    } else if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        if (get_rect_cover(&draw_r, &disp, &r)) {
            u32 alpha_addr = 0;
            br23_read_image_data(fp, &file, &alpha_addr, sizeof(alpha_addr), 0);
            if (alpha_addr) {
                temp_pixelbuf_len = dc->width * 2 * dc->lines + 0x40 - 8;
                temp_alphabuf_len = dc->width * dc->lines + 0x40 - 8;

                buf_offset = 0;
                pixelbuf = &dc->fbuf[buf_offset];//2 bytes * line
                buf_offset += dc->width * 2;
                buf_offset = (buf_offset + 3) / 4 * 4;
                alphabuf = &dc->fbuf[buf_offset];//1 bytes * line
                buf_offset += dc->width;
                buf_offset = (buf_offset + 3) / 4 * 4;
                temp_pixelbuf = &dc->fbuf[buf_offset];
                buf_offset += temp_pixelbuf_len;
                buf_offset = (buf_offset + 3) / 4 * 4;
                temp_alphabuf = &dc->fbuf[buf_offset];

            } else {
                /* temp_pixelbuf_len = dc->width * 3 * dc->lines + 0x80; */
                temp_pixelbuf_len = dc->fbuf_len - dc->width * 2;

                buf_offset = 0;
                pixelbuf = &dc->fbuf[buf_offset];//2 bytes * line
                buf_offset += dc->width * 2;
                buf_offset = (buf_offset + 3) / 4 * 4;
                temp_pixelbuf = &dc->fbuf[buf_offset];
            }

            for (h = 0; h < r.height;) {
                int rh = r.top + h - disp.top;
                int rw = r.left - disp.left;
                int vh = rh;
                int vw = rw;
                if (quadrant == 0) {
                    ;
                } else if (quadrant == 1) {
                    vh = file.height - (r.top - disp.top + r.height - h);
                } else if (quadrant == 2) {
                    vh = file.height - (r.top - disp.top + r.height - h);
                    vw = file.width - rw - r.width;
                } else {
                    vw = file.width - rw - r.width;
                }
                ASSERT(vw >= 0);
                ASSERT(vh >= 0);

                struct rle_line *line;
                struct rle_line *alpha_line;
                u8 *ptemp;
                u8 *alpha_ptemp;
                int lines;

                if (file.compress == 0) {
                    int remain = (r.height - h) > (file.height - vh) ? (file.height - vh) : (r.height - h);
                    int offset = 4 + vh * file.width * 2 + vw * 2;

                    if (!alpha_addr) {
                        lines = dc->fbuf_len / file.width / 2;
                    } else {
                        lines = dc->fbuf_len / file.width / 3;
                    }
                    lines = (lines > remain) ? remain : lines;
                    pixelbuf = dc->fbuf;
                    alphabuf = &dc->fbuf[(lines * file.width * 2 + 3) / 4 * 4];

                    int remain_bytes = 4 + file.width * 2 * file.height - offset;
                    int read_bytes = remain_bytes > (file.width * 2 * lines) ? file.width * 2 * lines : remain_bytes;
                    if (br23_read_image_data(fp, &file, pixelbuf, read_bytes, offset) != read_bytes) {
                        return -EFAULT;
                    }
                    if (alpha_addr) {
                        offset = alpha_addr + vh * file.width + vw;
                        int remain_bytes = alpha_addr + file.width * file.height - offset;
                        int read_bytes = remain_bytes > (file.width * lines) ? file.width * lines : remain_bytes;
                        if (br23_read_image_data(fp, &file, alphabuf, read_bytes, offset) != read_bytes) {
                            return -EFAULT;
                        }
                    }
                } else if (file.compress == 1) {
                    int remain = (r.height - h) > (file.height - vh) ? (file.height - vh) : (r.height - h);
                    int headlen = sizeof(struct rle_header) + (remain * 2 + 3) / 4 * 4;

                    line = (struct rle_line *)temp_pixelbuf;
                    ptemp = &temp_pixelbuf[headlen];
                    memset(line, 0x00, sizeof(struct rle_line));

                    filepos[0].offset = 4 + vh * sizeof(struct rle_header);
                    filepos[0].len = sizeof(struct rle_header) * remain;
                    br23_read_image_data(fp, &file, ptemp, sizeof(struct rle_header)*remain, 4 + vh * sizeof(struct rle_header));

                    int i;
                    struct rle_header *rle = (struct rle_header *)ptemp;
                    int total_len = 0;
                    for (i = 0; i < remain; i++) {
                        if (i == 0) {
                            line->addr = rle[i].addr;
                            line->len[i] = rle[i].len;
                        } else {
                            line->len[i] = rle[i].len;
                        }
                        if ((total_len + rle[i].len) > (temp_pixelbuf_len - headlen)) {
                            break;
                        }
                        total_len += rle[i].len;
                        line->num ++;
                    }
                    filepos[1].offset = 4 + line->addr;
                    filepos[1].len = total_len;
                    br23_read_image_data(fp, &file, ptemp, total_len, 4 + line->addr);

                    if (alpha_addr) {
                        int headlen = sizeof(struct rle_header) + (line->num * 2 + 3) / 4 * 4;
                        alpha_ptemp = &temp_alphabuf[headlen];
                        filepos[2].offset = alpha_addr + vh * sizeof(struct rle_header);
                        filepos[2].len = sizeof(struct rle_header) * line->num;
                        br23_read_image_data(fp, &file, alpha_ptemp, sizeof(struct rle_header)*line->num, alpha_addr + vh * sizeof(struct rle_header));

                        struct rle_header *rle = (struct rle_header *)alpha_ptemp;
                        alpha_line = (struct rle_line *)temp_alphabuf;
                        memset(alpha_line, 0x00, sizeof(struct rle_line));
                        int total_len = 0;
                        for (i = 0; i < line->num; i++) {
                            if (i == 0) {
                                alpha_line->addr = rle[i].addr;
                                alpha_line->len[i] = rle[i].len;
                            } else {
                                alpha_line->len[i] = rle[i].len;
                            }
                            if ((total_len + rle[i].len) > (temp_alphabuf_len - headlen)) {
                                break;
                            }
                            total_len += rle[i].len;
                            alpha_line->num ++;
                        }
                        filepos[3].offset = alpha_addr + alpha_line->addr;
                        filepos[3].len = total_len;
                        br23_read_image_data(fp, &file, alpha_ptemp, total_len, alpha_addr + alpha_line->addr);
                    }
                } else {
                    ASSERT(0, "the compress mode not support!");
                }

                u8 *p0 = ptemp;
                u8 *p1 = alpha_ptemp;
                int line_num;
                if (file.compress == 0) {
                    line_num = lines;
                } else {
                    if (alpha_addr) {
                        line_num = (line->num > alpha_line->num) ? alpha_line->num : line->num;
                    } else {
                        line_num = line->num;
                    }
                }

                int hs;
                int he;
                int hstep;

                he = h + line_num;
                if ((quadrant == 1) || (quadrant == 2)) {
                    hs = r.height - h  - 1;
                    hstep = -1;
                } else {
                    hs = h;
                    hstep = 1;
                }

                for (hh = 0, h = hs; hh < line_num; hh++, h += hstep) {
                    if (file.compress == 1) {
                        if (line_update(mask, r.top + h - dc->disp.top, dc->disp.width)) {
                            int ret = Rle_Decode(p0, line->len[hh], pixelbuf, file.width * 2, vw * 2, r.width * 2, 2);
                            if (ret == -1) {
                                printf("Rle_Decode err! len:%d\n", line->len[hh]);
                                put_buf(p0, line->len[hh]);
                                if (++retry_cnt > MAX_RETRY_TIMES) {
                                    static int dump_image = false;
                                    if (!dump_image) {
                                        dump_image_data(dc, fp, &file);
                                        dump_image = true;
                                    }
                                    dump_spec_data(dc, fp, &file, &filepos, 4);
                                    return -EFAULT;
                                } else {
                                    h = hs;
                                    printf("%d line rle_decode 0 fail, retry %d\n", r.top + h + hh, retry_cnt);
                                    break;
                                }
                            } else {
                                retry_cnt = 0;
                            }

                            p0 += line->len[hh];
                            if (alpha_addr) {
                                int ret = Rle_Decode(p1, alpha_line->len[hh], alphabuf, file.width, vw, r.width, 1);
                                if (ret == -1) {
                                    printf("Rle_Decode err! len:%d\n", alpha_line->len[hh]);
                                    put_buf(p1, alpha_line->len[hh]);
                                    if (++retry_cnt > MAX_RETRY_TIMES) {
                                        static int dump_image = false;
                                        if (!dump_image) {
                                            dump_image_data(dc, fp, &file);
                                            dump_image = true;
                                        }
                                        dump_spec_data(dc, fp, &file, &filepos, 4);
                                        return -EFAULT;
                                    } else {
                                        h = hs;
                                        printf("%d line rle_decode 1 fail, retry %d\n", r.top + h + hh, retry_cnt);
                                        break;
                                    }
                                } else {
                                    retry_cnt = 0;
                                }
                                p1 += alpha_line->len[hh];
                            }
                        } else {
                            p0 += line->len[hh];
                            p1 += alpha_line->len[hh];
                            continue;
                        }
                    }

                    u16 *pdisp = (u16 *)dc->buf;
                    u16 *pixelbuf16 = (u16 *)pixelbuf;

                    if (!alpha_addr) {
                        u16 x0 = r.left;
                        u16 y0 = r.top + h;
                        int offset = (y0 - dc->disp.top) * dc->disp.width + (x0 - dc->disp.left);
                        /* if ((offset * 2 + 1) < dc->len) { */
                        /* pdisp[offset] = pixel; */
                        /* } */
                        memcpy(&pdisp[offset], pixelbuf16, r.width * 2);
                        if (file.compress == 0) {
                            pixelbuf += file.width * 2;
                        }
                        continue;
                    }

                    for (w = 0; w < r.width; w++) {
                        u16 color, pixel;
                        u8  alpha = alpha_addr ? alphabuf[w] : 255;

                        pixel = color = pixelbuf16[w];
                        if (alpha) {
                            if (platform_api->draw_point) {
                                int vww = w;
                                if ((quadrant == 2) || (quadrant == 3)) {
                                    vww = r.width - w - 1;
                                }
                                u16 x0 = r.left + vww;
                                u16 y0 = r.top + h;

                                if (alpha < 255) {
                                    u16 backcolor = platform_api->read_point(dc, x0, y0);
                                    pixel = get_mixed_pixel((backcolor >> 8) | (backcolor & 0xff) << 8, (color >> 8) | (color & 0xff) << 8, alpha);
                                }

                                if (mask) {
                                    int yy = y0 - dc->disp.top;
                                    int xx = x0 - dc->disp.left;
                                    if (yy >= dc->disp.height) {
                                        continue;
                                    }
                                    if (xx >= dc->disp.width) {
                                        continue;
                                    }
                                    if (mask[yy * ((dc->disp.width + 7) / 8) + xx / 8] & BIT(xx % 8)) {
                                        int offset = (y0 - dc->disp.top) * dc->disp.width + (x0 - dc->disp.left);
                                        if ((offset * 2 + 1) < dc->len) {
                                            pdisp[offset] = pixel;
                                        }
                                    }
                                } else {
                                    int offset = (y0 - dc->disp.top) * dc->disp.width + (x0 - dc->disp.left);
                                    if ((offset * 2 + 1) < dc->len) {
                                        pdisp[offset] = pixel;
                                    }
                                }
                            }
                        }
                    }

                    if (file.compress == 0) {
                        pixelbuf += file.width * 2;
                        alphabuf += file.width;
                    }
                }
                if (!retry_cnt) {
                    h = he;
                }
            }
        }
    }

    return 0;
}


int get_multi_string_width(struct draw_context *dc, u8 *str, int *str_width, int *str_height)
{
    struct image_file file = {0};
    u16 *p = (u16 *)str;
    int width = 0;
    while (*p != 0) {
        select_strfile(dc->prj);
        if (open_string_pic(&file, *p)) {
            return -EINVAL;
        }
        width += file.width;
        p++;
    }
    *str_width = width;
    *str_height = file.height;

    return 0;
}

struct font_info *text_font_init(u8 init)
{
    static struct font_info *info = NULL;
    static int language = 0;

    if (init) {
        if (!info || (language != ui_language_get())) {
            language = ui_language_get();
            if (info) {
                font_close(info);
            }
            info = font_open(NULL, language);
            ASSERT(info, "font_open fail!");
        }
    } else {
        if (info) {
            font_close(info);
            info = NULL;
        }
    }

    return info;
}

static int br23_show_text(struct draw_context *dc, struct ui_text_attrs *text)
{
    struct rect draw_r;
    struct rect r = {0};
    struct rect disp = {0};
    struct image_file file = {0};
    int retry_cnt = 0;
    struct filepos filepos[4];


    /* 控件从绝对x,y 转成相对图层的x,y */
    int x = dc->rect.left;
    int y = dc->rect.top;

    /* 绘制区域从绝对x,y 转成相对图层的x,y */
    draw_r.left   = dc->disp.left;
    draw_r.top    = dc->disp.top;
    draw_r.width  = dc->disp.width;
    draw_r.height = dc->disp.height;

    if (text->format && !strcmp(text->format, "text")) {
        struct font_info *info = text_font_init(true);

        if (info && (FT_ERROR_NONE == (info->sta & (~(FT_ERROR_NOTABFILE | FT_ERROR_NOPIXFILE))))) {
            info->disp.map    = 0;
            info->disp.rect   = &draw_r;
            info->disp.format = dc->data_format;
            if ((dc->data_format == DC_DATA_FORMAT_MONO) && (text->color == UI_RGB565(TEXT_MONO_INV))) {
                if (__this->api->fill_rect) {
                    __this->api->fill_rect(dc, UI_RGB565(BGC_MONO_SET));
                }
            }
            if (dc->data_format == DC_DATA_FORMAT_OSD16) {
                info->disp.color  = text->color;
            } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
                if (text->color == UI_RGB565(TEXT_MONO_INV)) {
                    info->disp.color = 0x55aa;//清显示
                } else {
                    info->disp.color = (text->color != UI_RGB565(TEXT_MONO_CLR)) ? (text->color ? text->color : 0xffff) : 0x55aa;
                }
            }
            info->dc = dc;
            info->text_width  = dc->rect.width;
            info->text_height = dc->rect.height;
            info->flags       = text->flags;
            /* info->offset      = text->offset; */
            int roll = 0;//需要滚动
            int multi_line = 0;
            /* FONT_SHOW_MULTI_LINE */
            if (text->encode == FONT_ENCODE_ANSI) {
                int width = font_text_width(info, (u8 *)text->str, text->strlen);
                int height;


                if (info->ascpixel.size) {
                    height = info->ascpixel.size;
                } else if (info->pixel.size) {
                    height = info->pixel.size;
                } else {
                    ASSERT(0, "can't get the height of font.");
                }

                if (width > dc->rect.width) {
                    width = dc->rect.width;
                    roll = 1;
                    multi_line = 1;
                }


                if (text->flags & FONT_SHOW_MULTI_LINE) {
                    height += multi_line * height;
                }

                if (height > dc->rect.height) {
                    height = dc->rect.height;
                }

                y += (dc->rect.height / 2 - height / 2);

                if (dc->align == UI_ALIGN_CENTER) {
                    x += (dc->rect.width / 2 - width / 2);
                } else if (dc->align == UI_ALIGN_RIGHT) {
                    x += (dc->rect.width - width);
                }

                if (dc->draw_img.en) {//指定坐标刷新
                    x = dc->draw_img.x;
                    y = dc->draw_img.y;
                }

                info->x = x;
                info->y = y;
                int len = font_textout(info, (u8 *)(text->str + roll * text->offset * 2), text->strlen - roll * text->offset * 2, x, y + text->y_offset);
                ASSERT(len <= 255);
                text->displen = len;
            } else if (text->encode == FONT_ENCODE_UNICODE) {
                if (FT_ERROR_NONE == (info->sta & ~(FT_ERROR_NOTABFILE | FT_ERROR_NOPIXFILE))) {
                    if (text->endian == FONT_ENDIAN_BIG) {
                        info->bigendian = true;
                    } else {
                        info->bigendian = false;
                    }
                    int width = font_textw_width(info, (u8 *)text->str, text->strlen);
                    int height;

                    if (info->ascpixel.size) {
                        height = info->ascpixel.size;
                    } else if (info->pixel.size) {
                        height = info->pixel.size;
                    } else {
                        ASSERT(0, "can't get the height of font.");
                    }

                    if (width > dc->rect.width) {
                        width = dc->rect.width;
                        roll = 1;
                        multi_line = 1;
                    }

                    if (text->flags & FONT_SHOW_MULTI_LINE) {
                        height += multi_line * height;
                    }


                    if (height > dc->rect.height) {
                        height = dc->rect.height;
                    }

                    y += (dc->rect.height / 2 - height / 2);
                    if (dc->align == UI_ALIGN_CENTER) {
                        x += (dc->rect.width / 2 - width / 2);
                    } else if (dc->align == UI_ALIGN_RIGHT) {
                        x += (dc->rect.width - width);
                    }

                    if (dc->draw_img.en) {//指定坐标刷新
                        x = dc->draw_img.x;
                        y = dc->draw_img.y;
                    }

                    info->x = x;
                    info->y = y;
                    int len = font_textout_unicode(info, (u8 *)(text->str + roll * text->offset * 2), text->strlen - roll * text->offset * 2, x, y + text->y_offset);
                    ASSERT(len <= 255);
                    text->displen = len;
                }
            } else {
                int width = font_textu_width(info, (u8 *)text->str, text->strlen);
                int height;

                if (info->ascpixel.size) {
                    height = info->ascpixel.size;
                } else if (info->pixel.size) {
                    height = info->pixel.size;
                } else {
                    ASSERT(0, "can't get the height of font.");
                }

                if (width > dc->rect.width) {
                    width = dc->rect.width;
                }
                if (height > dc->rect.height) {
                    height = dc->rect.height;
                }

                y += (dc->rect.height / 2 - height / 2);
                if (dc->align == UI_ALIGN_CENTER) {
                    x += (dc->rect.width / 2 - width / 2);
                } else if (dc->align == UI_ALIGN_RIGHT) {
                    x += (dc->rect.width - width);
                }

                if (dc->draw_img.en) {//指定坐标刷新
                    x = dc->draw_img.x;
                    y = dc->draw_img.y;
                }

                info->x = x;
                info->y = y;
                int len = font_textout_utf8(info, (u8 *)text->str, text->strlen, x, y + text->y_offset);
                ASSERT(len <= 255);
                text->displen = len;
            }
        }
    } else if (text->format && !strcmp(text->format, "ascii")) {
        char *str;
        u32 w_sum;
        if (!text->str) {
            return 0;
        }
        if ((u8)text->str[0] == 0xff) {
            return 0;
        }

        if (dc->align == UI_ALIGN_CENTER) {
            w_sum = font_ascii_width_check(text->str);
            x += (dc->rect.width / 2 - w_sum / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            w_sum = font_ascii_width_check(text->str);
            x += (dc->rect.width - w_sum);
        }

        if (dc->draw_img.en) {//指定坐标刷新
            x = dc->draw_img.x;
            y = dc->draw_img.y;
        }

        if ((dc->data_format == DC_DATA_FORMAT_MONO) && (text->color == UI_RGB565(TEXT_MONO_INV))) {
            if (__this->api->fill_rect) {
                __this->api->fill_rect(dc, UI_RGB565(BGC_MONO_SET));
            }
        }
        str = text->str;
        while (*str) {
            u8 *pixbuf = dc->fbuf;
            int width;
            int height;
            int color;
            font_ascii_get_pix(*str, pixbuf, dc->fbuf_len, &height, &width);
            if (dc->data_format == DC_DATA_FORMAT_OSD16) {
                color  = text->color;
            } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
                if (text->color == UI_RGB565(TEXT_MONO_INV)) {
                    color = 0x55aa;//清显示
                } else {
                    color = (text->color != UI_RGB565(TEXT_MONO_CLR)) ? (text->color ? text->color : 0xffff) : 0x55aa;
                }
            }
            __font_pix_copy(dc, dc->data_format, 0, pixbuf, &draw_r, x, y, height, width, color);
            x += width;
            str++;
        }
    } else if (text->format && !strcmp(text->format, "strpic")) {
        u16 id = *((u16 *)text->str);
        u8 *pixbuf;
        int w;
        int h;

        if (id == 0xffff) {
            return -EINVAL;
        }

        select_strfile(dc->prj);
        if (open_string_pic(&file, id)) {
            return -EEXIST;
        }

        y += (dc->rect.height / 2 - file.height / 2);
        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - file.width / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += (dc->rect.width - file.width);
        }

        pixbuf = dc->fbuf;
        if (!pixbuf) {
            return -ENOMEM;
        }

        if (dc->draw_img.en) {
            x = dc->draw_img.x;
            y = dc->draw_img.y;
        }

        disp.left   = x;
        disp.top    = y;
        disp.width  = file.width;
        disp.height = file.height;

        if (get_rect_cover(&draw_r, &disp, &r)) {
            if ((dc->data_format == DC_DATA_FORMAT_MONO) && (text->color == UI_RGB565(TEXT_MONO_INV))) {
                if (__this->api->fill_rect) {
                    __this->api->fill_rect(dc, UI_RGB565(BGC_MONO_SET));
                }
            }
            for (h = 0; h < file.height; h += 8) {
                if (file.compress == 0) {
                    int offset = (h / 8) * file.width;
                    if (br23_read_str_data(&file, pixbuf, file.width, offset) != file.width) {
                        return -EFAULT;
                    }
                } else {
                    ASSERT(0, "the compress mode not support!");
                }
                int color;
                if (dc->data_format == DC_DATA_FORMAT_OSD16) {
                    color  = text->color;
                } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
                    if (text->color == UI_RGB565(TEXT_MONO_INV)) {
                        color = 0x55aa;//清显示
                    } else {
                        color = (text->color != UI_RGB565(TEXT_MONO_CLR)) ? (text->color ? text->color : 0xffff) : 0x55aa;
                    }
                }
                __font_pix_copy(dc, dc->data_format, 0, pixbuf, &r, x, y + h / 8 * 8, 8, file.width, color);
            }
        }
    } else if (text->format && !strcmp(text->format, "mulstr")) {
        u16 id = *((u16 *)text->str);
        u8 *pixbuf;
        int w;
        int h;
        u16 *p = (u16 *)text->str;
        select_strfile(dc->prj);

        if (get_multi_string_width(dc, text->str, &w, &h)) {
            return -EINVAL;
        }

        y += (dc->rect.height / 2 - h / 2);
        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - w / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += (dc->rect.width - w);
        }

        while (*p != 0) {
            id = *p;

            if (id == 0xffff) {
                return 0;
            }

            select_strfile(dc->prj);
            if (open_string_pic(&file, id)) {
                return 0;
            }


            pixbuf = dc->fbuf;
            if (!pixbuf) {
                return -ENOMEM;
            }

            disp.left   = x;
            disp.top    = y;
            disp.width  = file.width;
            disp.height = file.height;

            if (get_rect_cover(&draw_r, &disp, &r)) {
                if ((dc->data_format == DC_DATA_FORMAT_MONO) && (text->color == UI_RGB565(TEXT_MONO_INV))) {
                    if (__this->api->fill_rect) {
                        __this->api->fill_rect(dc, UI_RGB565(BGC_MONO_SET));
                    }
                }
                for (h = 0; h < file.height; h += 8) {
                    if (file.compress == 0) {
                        int offset = (h / 8) * file.width;
                        if (br23_read_str_data(&file, pixbuf, file.width, offset) != file.width) {
                            return -EFAULT;
                        }
                    } else {
                        ASSERT(0, "the compress mode not support!");
                    }
                    int color;
                    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
                        color  = text->color;
                    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
                        if (text->color == UI_RGB565(TEXT_MONO_INV)) {
                            color = 0x55aa;//清显示
                        } else {
                            color = (text->color != UI_RGB565(TEXT_MONO_CLR)) ? (text->color ? text->color : 0xffff) : 0x55aa;
                        }
                    }
                    __font_pix_copy(dc, dc->data_format, 0, pixbuf, &r, x, y + h / 8 * 8, 8, file.width, color);
                }
            }
            x += file.width;
            p++;
        }

    } else if (text->format && !strcmp(text->format, "image")) {
        u8 *pixelbuf;
        u8 *temp_pixelbuf;
        u8 *alphabuf;
        u8 *temp_alphabuf;
        u16 cnt = 0;
        u16 id = ((u8)text->str[1] << 8) | (u8)text->str[0];
        u32 w, h;
        int ww, hh;


        if (image_str_size_check(dc, dc->page, text->str, &ww, &hh) != 0) {
            return -EFAULT;
        }
        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - ww / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += (dc->rect.width - ww);
        }
        y += (dc->rect.height / 2 - hh / 2);
        while ((id != 0x00ff) && (id != 0xffff)) {
            select_resfile(dc->prj);
            struct mem_var_list *list;
            if ((list = mem_var_search(10, 0, id, dc->page, dc->prj)) != NULL) {
                mem_var_get(list, &file, sizeof(struct image_file));
            } else {
                if (open_image_by_id(NULL, &file, id, dc->page) != 0) {
                    return -EFAULT;
                }
                mem_var_add(10, 0, id, dc->page, dc->prj, &file, sizeof(struct image_file));
            }

            disp.left   = x;
            disp.top    = y;
            disp.width  = file.width;
            disp.height = file.height;

            if (dc->data_format == DC_DATA_FORMAT_MONO) {
                pixelbuf = dc->fbuf;
                if (get_rect_cover(&draw_r, &disp, &r)) {
                    int _offset = -1;
                    for (h = 0; h < r.height; h++) {
                        if (file.compress == 0) {
                            int offset = (r.top + h - disp.top) / 8 * file.width + (r.left - disp.left);
                            if (_offset != offset) {
                                if (br23_read_image_data(NULL, &file, pixelbuf, r.width, offset) != r.width) {
                                    return -EFAULT;
                                }
                                _offset = offset;
                            }
                        } else {
                            ASSERT(0, "the compress mode not support!");
                        }
                        for (w = 0; w < r.width; w++) {
                            u8 color = (pixelbuf[w] & BIT((r.top + h - disp.top) % 8)) ? 1 : 0;
                            if (color) {
                                if (platform_api->draw_point) {
                                    u16 text_color;
                                    if (text->color != 0xffffff) {
                                        text_color = (text->color != UI_RGB565(TEXT_MONO_CLR)) ? (text->color ? text->color : 0xffff) : 0x55aa;
                                    } else {
                                        text_color = color;
                                    }
                                    platform_api->draw_point(dc, r.left + w, r.top + h, text_color);
                                }
                            }
                        }
                    }
                }
            } else if (dc->data_format == DC_DATA_FORMAT_OSD16) {
                int temp_pixelbuf_len = dc->width * 2 * dc->lines + 0x40 - 8;
                int temp_alphabuf_len = dc->width * dc->lines + 0x40 - 8;
                int buf_offset;

                buf_offset = 0;
                pixelbuf = &dc->fbuf[buf_offset];//2 bytes * line
                buf_offset += dc->width * 2;
                buf_offset = (buf_offset + 3) / 4 * 4;
                alphabuf = &dc->fbuf[buf_offset];//1 bytes * line
                buf_offset += dc->width;
                buf_offset = (buf_offset + 3) / 4 * 4;
                temp_pixelbuf = &dc->fbuf[buf_offset];
                buf_offset += temp_pixelbuf_len;
                buf_offset = (buf_offset + 3) / 4 * 4;
                temp_alphabuf = &dc->fbuf[buf_offset];

                u32 alpha_addr = 0;
                if (get_rect_cover(&draw_r, &disp, &r)) {
                    br23_read_image_data(NULL, &file, &alpha_addr, sizeof(alpha_addr), 0);
                    for (h = 0; h < r.height;) {
                        int vh = r.top + h - disp.top;
                        int vw = r.left - disp.left;

                        struct rle_line *line;
                        struct rle_line *alpha_line;
                        u8 *ptemp;
                        u8 *alpha_ptemp;

                        if (file.compress == 0) {
                            int offset = 4 + vh * file.width * 2 + vw * 2;
                            if (br23_read_image_data(NULL, &file, pixelbuf, r.width * 2, offset) != r.width * 2) {
                                return -EFAULT;
                            }

                            if (alpha_addr) {
                                offset = alpha_addr + vh * file.width + vw;
                                br23_read_image_data(NULL, &file, alphabuf, r.width, offset);
                            }
                        } else if (file.compress == 1) {
                            int remain = (r.height - h) > (file.height - vh) ? (file.height - vh) : (r.height - h);
                            int headlen = sizeof(struct rle_header) + (remain * 2 + 3) / 4 * 4;

                            line = (struct rle_line *)temp_pixelbuf;
                            ptemp = &temp_pixelbuf[headlen];
                            memset(line, 0x00, sizeof(struct rle_line));

                            int rle_header_len = sizeof(struct rle_header) * remain;
                            filepos[0].len = rle_header_len;
                            filepos[0].offset = 4 + vh * sizeof(struct rle_header);
                            br23_read_image_data(NULL, &file, ptemp, rle_header_len, 4 + vh * sizeof(struct rle_header));

                            int i;
                            struct rle_header *rle = (struct rle_header *)ptemp;
                            int total_len = 0;
                            for (i = 0; i < remain; i++) {
                                if (i == 0) {
                                    line->addr = rle[i].addr;
                                    line->len[i] = rle[i].len;
                                } else {
                                    line->len[i] = rle[i].len;
                                }
                                if ((total_len + rle[i].len) > (temp_pixelbuf_len - headlen)) {
                                    break;
                                }
                                total_len += rle[i].len;
                                line->num ++;
                            }

                            filepos[1].len = total_len;
                            filepos[1].offset = 4 + line->addr;
                            br23_read_image_data(NULL, &file, ptemp, total_len, 4 + line->addr);

                            if (alpha_addr) {
                                int headlen = sizeof(struct rle_header) + (line->num * 2 + 3) / 4 * 4;
                                alpha_ptemp = &temp_alphabuf[headlen];
                                filepos[2].len = sizeof(struct rle_header) * line->num;
                                filepos[2].offset = alpha_addr + vh * sizeof(struct rle_header);
                                br23_read_image_data(NULL, &file, alpha_ptemp, sizeof(struct rle_header)*line->num, alpha_addr + vh * sizeof(struct rle_header));

                                struct rle_header *rle = (struct rle_header *)alpha_ptemp;
                                alpha_line = (struct rle_line *)temp_alphabuf;
                                memset(alpha_line, 0x00, sizeof(struct rle_line));
                                int total_len = 0;
                                for (i = 0; i < line->num; i++) {
                                    if (i == 0) {
                                        alpha_line->addr = rle[i].addr;
                                        alpha_line->len[i] = rle[i].len;
                                    } else {
                                        alpha_line->len[i] = rle[i].len;
                                    }
                                    if ((total_len + rle[i].len) > (temp_alphabuf_len - headlen)) {
                                        break;
                                    }
                                    total_len += rle[i].len;
                                    alpha_line->num ++;
                                }

                                filepos[3].len = total_len;
                                filepos[3].offset = alpha_addr + alpha_line->addr;
                                br23_read_image_data(NULL, &file, alpha_ptemp, total_len, alpha_addr + alpha_line->addr);
                            }
                        } else {
                            ASSERT(0, "the compress mode not support!");
                        }

                        u8 *p0 = ptemp;
                        u8 *p1 = alpha_ptemp;
                        int line_num;
                        if (file.compress == 0) {
                            line_num = 1;
                        } else {
                            line_num = (line->num > alpha_line->num) ? alpha_line->num : line->num;
                        }

                        int hs = h;
                        int he = h + line_num;

                        for (hh = 0; hh < line_num; hh++, h++) {
                            if (file.compress == 1) {
                                int ret = Rle_Decode(p0, line->len[hh], pixelbuf, file.width * 2, vw * 2, r.width * 2, 2);
                                if (ret == -1) {
                                    printf("Rle_Decode err! len:%d\n", line->len[hh]);
                                    put_buf(p0, line->len[hh]);
                                    if (++retry_cnt > MAX_RETRY_TIMES) {
                                        dump_spec_data(dc, NULL, &file, &filepos, 4);
                                        return -EFAULT;
                                    } else {
                                        h = hs;
                                        printf("rle_decode 2 fail, retry %d\n", retry_cnt);
                                        break;
                                    }
                                } else {
                                    retry_cnt = 0;
                                }
                                p0 += line->len[hh];
                                ret = Rle_Decode(p1, alpha_line->len[hh], alphabuf, file.width, vw, r.width, 1);
                                if (ret == -1) {
                                    printf("Rle_Decode err! len:%d\n", alpha_line->len[hh]);
                                    put_buf(p1, alpha_line->len[hh]);
                                    if (++retry_cnt > MAX_RETRY_TIMES) {
                                        dump_spec_data(dc, NULL, &file, &filepos, 4);
                                        return -EFAULT;
                                    } else {
                                        h = hs;
                                        printf("rle_decode 3 fail, retry %d\n", retry_cnt);
                                        break;
                                    }
                                } else {
                                    retry_cnt = 0;
                                }
                                p1 += alpha_line->len[hh];
                            }
                            u16 *pdisp = (u16 *)dc->buf;
                            u16 *pixelbuf16 = (u16 *)pixelbuf;
                            for (w = 0; w < r.width; w++) {
                                u16 color, pixel;
                                u8  alpha = alpha_addr ? alphabuf[w] : 255;

                                pixel = color = pixelbuf16[w];
                                if (alpha) {
                                    if (platform_api->draw_point) {
                                        int vww = w;
                                        u16 x0 = r.left + vww;
                                        u16 y0 = r.top + h;

                                        if (alpha < 255) {
                                            u16 backcolor = platform_api->read_point(dc, x0, y0);
                                            pixel = get_mixed_pixel((backcolor >> 8) | (backcolor & 0xff) << 8, (color >> 8) | (color & 0xff) << 8, alpha);
                                        }

                                        int offset = (y0 - dc->disp.top) * dc->disp.width + (x0 - dc->disp.left);
                                        if ((offset * 2 + 1) < dc->len) {
                                            pdisp[offset] = pixel;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            x += file.width;
            cnt += 2;
            id = ((u8)text->str[cnt + 1] << 8) | (u8)text->str[cnt];
        }
    }

    return 0;
}


#include "ui/ui_circle.h"
void ui_draw_circle(struct draw_context *dc, int center_x, int center_y,
                    int radius_small, int radius_big, int angle_begin,
                    int angle_end, int color, int percent)
{
    struct circle_info info;
    info.center_x = center_x;
    info.center_y = center_y;
    info.radius_big = radius_big;
    info.radius_small = radius_small;
    info.angle_begin = angle_begin;
    info.angle_end = angle_end;
    info.left = dc->draw.left;
    info.top = dc->draw.top;
    info.width = dc->draw.width;
    info.height = dc->draw.height;

    info.x = 0;
    info.y = 0;
    info.disp_x = dc->disp.left;
    info.disp_y = dc->disp.top;

    info.color = color;
    info.bitmap_width = dc->disp.width;
    info.bitmap_height = dc->disp.height;
    info.bitmap_pitch = dc->disp.width * 2;
    info.bitmap = dc->buf;
    info.bitmap_size = dc->len;
    info.bitmap_depth = 16;

    draw_circle_by_percent(&info, percent);
}

AT_UI_RAM
u32 br23_read_point(struct draw_context *dc, u16 x, u16 y)
{
    u32 pixel;
    u16 *pdisp = dc->buf;
    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        int offset = (y - dc->disp.top) * dc->disp.width + (x - dc->disp.left);
        ASSERT((offset * 2 + 1) < dc->len, "dc->len:%d", dc->len);
        if ((offset * 2 + 1) >= dc->len) {
            return -1;
        }

        pixel = pdisp[offset];//(dc->buf[offset * 2] << 8) | dc->buf[offset * 2 + 1];
    } else {
        ASSERT(0);
    }

    return pixel;
}


__attribute__((always_inline_when_const_args))
AT_UI_RAM
int br23_draw_point(struct draw_context *dc, u16 x, u16 y, u32 pixel)
{
    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        int offset = (y - dc->disp.top) * dc->disp.width + (x - dc->disp.left);

        /* ASSERT((offset * 2 + 1) < dc->len, "dc->len:%d", dc->len); */
        if ((offset * 2 + 1) >= dc->len) {
            return -1;
        }

        dc->buf[offset * 2    ] = pixel >> 8;
        dc->buf[offset * 2 + 1] = pixel;
    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
        /* ASSERT(x < __this->info.width); */
        /* ASSERT(y < __this->info.height); */
        if ((x >= __this->info.width) || (y >= __this->info.height)) {
            return -1;
        }

        if (pixel & BIT(31)) {
            dc->buf[y / 8 * __this->info.width + x] ^= BIT(y % 8);
        } else if (pixel == 0x55aa) {
            dc->buf[y / 8 * __this->info.width + x] &= ~BIT(y % 8);
        } else if (pixel) {
            dc->buf[y / 8 * __this->info.width + x] |= BIT(y % 8);
        } else {
            dc->buf[y / 8 * __this->info.width + x] &= ~BIT(y % 8);
        }
    }

    return 0;
}

int br23_open_device(struct draw_context *dc, const char *device)
{
    return 0;
}

int br23_close_device(int fd)
{
    return 0;
}


int ui_fill_rect(struct draw_context *dc, int left, int top, int width, int height, u32 acolor)
{
    int i;
    int w, h;
    u16 color = acolor & 0xffff;

    if (!dc->buf) {
        return 0;
    }

    struct rect rect;
    rect.left = left;
    rect.top = top;
    rect.width = width;
    rect.height = height;

    struct rect cover;
    if (!get_rect_cover(&dc->draw, &rect, &cover)) {
        return 0;
    }

    if (dc->data_format == DC_DATA_FORMAT_MONO) {
        color = (color == UI_RGB565(BGC_MONO_SET)) ? 0xffff : 0x55aa;
        for (h = 0; h < cover.height; h++) {
            for (w = 0; w < cover.width; w++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, cover.left + w, cover.top + h, color);
                }
            }
        }
    } else {
#if 0
        for (h = 0; h < cover.height; h++) {
            for (w = 0; w < cover.width; w++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, cover.left + w, cover.top + h, color);
                }
            }
        }
#else
        u16 color16 = (color >> 8) | ((color & 0xff) << 8);
        u32 color32 = (color16 << 16) | color16;

        h = 0;
        u32 *p32 = (u32 *)&dc->buf[(cover.top + h - dc->disp.top) * dc->disp.width * 2 + (cover.left - dc->disp.left) * 2];
        u32 *_p32 = p32;
        u32 len = cover.width * 2;
        if ((u32)p32 % 4) {
            u16 *p16 = (u16 *)p32;
            *p16++ = color16;
            p32 = (u32 *)p16;
            len -= 2;
            ASSERT((u32)p32 % 4 == 0);
        }

        u32 count = len / 4;
        while (count--) {
            *p32++ = color32;
        }
        count = (len % 4) / 2;
        u16 *p16 = (u16 *)p32;
        while (count--) {
            *p16++ = color16;
        }

        for (h = 1; h < cover.height; h++) {
            u32 *__p32 = (u32 *)&dc->buf[(cover.top + h - dc->disp.top) * dc->disp.width * 2 + (cover.left - dc->disp.left) * 2];
            memcpy(__p32, _p32, cover.width * 2);
        }
#endif
    }

    return 0;
}

int ui_draw_image(struct draw_context *dc, int page, int id, int x, int y)
{
    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;
    dc->draw_img.id = id;
    dc->draw_img.page = page;
    return br23_draw_image(dc, id, 0, NULL);
}


int ui_draw_ascii(struct draw_context *dc, char *str, int strlen, int x, int y, int color)
{
    struct ui_text_attrs text = {0};
    text.str = str;
    text.format = "ascii";
    text.color = color;
    text.strlen = strlen;
    text.flags = FONT_DEFAULT;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;

    return br23_show_text(dc, &text);
}

int ui_draw_text(struct draw_context *dc, int encode, int endian, char *str, int strlen, int x, int y, int color)
{
    struct ui_text_attrs text = {0};
    text.str = str;
    text.format = "text";
    text.color = color;
    text.strlen = strlen;
    text.encode = encode;
    text.endian = endian;
    text.flags = FONT_DEFAULT;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;

    return br23_show_text(dc, &text);
}

int ui_draw_strpic(struct draw_context *dc, int id, int x, int y, int color)
{
    struct ui_text_attrs text = {0};
    u32 strbuf ALIGNED(32);

    strbuf = id;
    text.str = &strbuf;
    text.format = "strpic";
    text.color = color;
    text.strlen = 0;
    text.encode = 0;
    text.endian = 0;
    text.flags = 0;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;

    return br23_show_text(dc, &text);
}

int ui_draw_set_pixel(struct draw_context *dc, int x, int y, int pixel)
{
    return platform_api->draw_point(dc, x, y, pixel);
}

u32 ui_draw_get_pixel(struct draw_context *dc, int x, int y)
{
    return platform_api->read_point(dc, x, y);
}

u16 ui_draw_get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha)
{
    return get_mixed_pixel(backcolor, forecolor, alpha);
}

static const struct ui_platform_api br23_platform_api = {
    .malloc             = br23_malloc,
    .free               = br23_free,

    .load_style         = br23_load_style,
    .load_window        = br23_load_window,
    .unload_window      = br23_unload_window,

    .open_draw_context  = br23_open_draw_context,
    .get_draw_context   = br23_get_draw_context,
    .put_draw_context   = br23_put_draw_context,
    .set_draw_context   = br23_set_draw_context,
    .close_draw_context = br23_close_draw_context,

    .load_widget_info   = br23_load_widget_info, //__load_widget_info,//
    .load_css           = br23_load_css,
    .load_image_list    = br23_load_image_list, //__load_image_list,//
    .load_text_list     = br23_load_text_list,

    .fill_rect          = br23_fill_rect,
    .draw_rect          = br23_draw_rect,
    .draw_image         = br23_draw_image,
    .show_text          = br23_show_text,
    .read_point         = br23_read_point,
    .draw_point         = br23_draw_point,
    .invert_rect        = br23_invert_rect,

    .read_image_info    = br23_read_image_info,

    .open_device        = br23_open_device,
    .close_device       = br23_close_device,

    .set_timer          = br23_set_timer,
    .del_timer          = br23_del_timer,

    .file_browser_open  = NULL,
    .get_file_attrs     = NULL,
    .set_file_attrs     = NULL,
    .show_file_preview  = NULL,
    .move_file_preview  = NULL,
    .clear_file_preview = NULL,
    .flush_file_preview = NULL,
    .open_file          = NULL,
    .delete_file        = NULL,
    .file_browser_close = NULL,
};




static int open_resource_file()
{
    int ret;

    printf("open_resouece_file...\n");

    ret = open_resfile(RES_PATH"menu.res");
    if (ret == 0) {
        printf("[UI RESOURCES LOAD SUCCESS (1/4)] : menu.res");
    } else {
        printf("[UI RESOURCES LOAD FAIL (1/4)] : menu.res");
        return -EINVAL;
    }

    ret = open_str_file(RES_PATH"str.res");
    if (ret == 0) {
        printf("[UI RESOURCES LOAD SUCCESS (2/4)] : str.res");
    } else {
        printf("[UI RESOURCES LOAD FAIL (2/4)] : str.res");
        return -EINVAL;
    }

    ret = font_ascii_init(RES_PATH"ascii.res");
    if (ret == 0) {
        printf("[UI RESOURCES LOAD SUCCESS (3/4)] : ascii.res");
    } else {
        printf("[UI RESOURCES LOAD FAIL (3/4)] : ascii.res");
        return -EINVAL;
    }

    return 0;
}

int __attribute__((weak)) lcd_get_scrennifo(struct fb_var_screeninfo *info)
{
    info->s_xoffset = 0;
    info->s_yoffset = 0;
    info->s_xres = 240;
    info->s_yres = 240;

    return 0;
}

int ui_platform_init(void *lcd)
{
    struct rect rect;
    struct lcd_info info = {0};

#ifdef UI_BUF_CALC
    INIT_LIST_HEAD(&buffer_used.list);
#endif

    __this->api = &br23_platform_api;
    ASSERT(__this->api->open_draw_context);
    ASSERT(__this->api->get_draw_context);
    ASSERT(__this->api->put_draw_context);
    ASSERT(__this->api->set_draw_context);
    ASSERT(__this->api->close_draw_context);


    __this->lcd = lcd_get_hdl();
    ASSERT(__this->lcd);
    ASSERT(__this->lcd->init);
    ASSERT(__this->lcd->get_screen_info);
    ASSERT(__this->lcd->buffer_malloc);
    ASSERT(__this->lcd->buffer_free);
    ASSERT(__this->lcd->draw);
    ASSERT(__this->lcd->set_draw_area);
    ASSERT(__this->lcd->backlight_ctrl);

    if (__this->lcd->init) {
        __this->lcd->init(lcd);
    }

    if (__this->lcd->backlight_ctrl) {
        __this->lcd->backlight_ctrl(true);
    }

    if (__this->lcd->get_screen_info) {
        __this->lcd->get_screen_info(&info);
    }
    rect.left   = 0;
    rect.top    = 0;
    rect.width  = info.width;
    rect.height = info.height;

    printf("ui_platform_init :: [%d,%d,%d,%d]\n", rect.left, rect.top, rect.width, rect.height);

    ui_core_init(__this->api, &rect);

    return 0;
}



int ui_style_file_version_compare(int version)
{
    int v;
    int len;
    struct ui_file_head head;
    static u8 checked = 0;

    if (checked == 0) {
        if (!ui_file) {
            puts("ui version_compare ui_file null!\n");
            ASSERT(0);
            return 0;
        }
        res_fseek(ui_file, 0, SEEK_SET);
        len = sizeof(struct ui_file_head);
        res_fread(ui_file, &head, len);
        printf("style file version is: 0x%x,UI_VERSION is: 0x%x\n", *(u32 *)(head.res), version);
        if (*(u32 *)head.res != version) {
            puts("style file version is not the same as UI_VERSION !!\n");
            ASSERT(0);
        }
        checked = 1;
    }
    return 0;
}




/* static const char *WATCH_RES_CHECK_LIST[] = { */
/* RES_PATH"JL/JL.res", */
/* RES_PATH"watch/watch.res", */
/* RES_PATH"watch1/watch1.res", */
/* RES_PATH"watch2/watch2.res", */
/* RES_PATH"watch3/watch3.res", */
/* RES_PATH"watch4/watch4.res", */
/* RES_PATH"watch5/watch5.res", */
/* }; */

/* static const char *WATCH_STR_CHECK_LIST[] = { */
/* RES_PATH"JL/JL.str", */
/* RES_PATH"watch/watch.str", */
/* RES_PATH"watch1/watch1.str", */
/* RES_PATH"watch2/watch2.str", */
/* RES_PATH"watch3/watch3.str", */
/* RES_PATH"watch4/watch4.str", */
/* RES_PATH"watch5/watch5.str", */
/* }; */

int ui_watch_poweron_update_check()
{
#if(CONFIG_UI_STYLE == STYLE_JL_WTACH)
    //如果上次表盘升级异常，直接进入表盘升级模式等待升级完成
    //加入新标志位的判断，用于升级过程中断电后开机进入升级模式的情况
    if (smartbox_eflash_flag_get() != 0 ||
        smartbox_eflash_update_flag_get()) {
        printf("\n\ngoto watch update mode\n\n");

        //上电后，发现上一次表盘升级异常,先初始化升级需要的蓝牙相关部分,
        //再跳转到升级模式

        watch_update_over = 2;
        /*app_smartbox_task_prepare(0, SMARTBOX_TASK_ACTION_WATCH_TRANSFER, 0);*/
        /* app_task_switch_to(APP_WATCH_UPDATE_TASK); */
        return -1;
        /* while (watch_update_over == 2) { */
        /* os_time_dly(3); */
        /* } */
        /* watch_update_over = 0; */
    }
#endif
    return 0;
}


int ui_upgrade_file_check_valid()
{
#if UI_UPGRADE_RES_ENABLE
    //简单实现
    //假设升级界面必须存在，调用了该接口证明资源不完整
    //需要进行升级
    smartbox_eflash_flag_set(1);//这个标志的清理需要注意
    return 0;
#endif
    return -ENOENT;
}

int ui_file_check_valid()
{

#if UI_WATCH_RES_ENABLE
    int ret;
    printf("open_resouece_file...\n");
    int i = 0;
    FILE *file = NULL;
    char *sty_suffix = ".sty";
    char *res_suffix = ".res";
    char *str_suffix = ".str";
    char tmp_name[100];
    /* u32 list_len = sizeof(WATCH_STY_CHECK_LIST) / sizeof(WATCH_STY_CHECK_LIST[0]); */
    u32 list_len;
    u32 tmp_strlen;
    u32 suffix_len;

    //如果上次表盘升级异常，直接进入表盘升级模式等待升级完成
    /* if (smartbox_eflash_flag_get() != 0 || */
    /*     smartbox_eflash_update_flag_get()) { */
    if (smartbox_eflash_update_flag_get() != 0 ||
        smartbox_eflash_resume_opt()) {
        printf("\n\nneed goto watch update mode\n\n");
        return -EINVAL;
    } else {
        ret = watch_set_init();
        if (ret != 0) {
            return -EINVAL;
        }
    }

    list_len = watch_items;

    for (i = 0; i < list_len; i++) {
        if (file) {
            res_fclose(file);
            file = NULL;
        }
        /* file = res_fopen(WATCH_STY_CHECK_LIST[i], "r"); */
        file = res_fopen(watch_res[i], "r");
        if (!file) {
            return -ENOENT;
        }
    }


    suffix_len = strlen(sty_suffix);
    close_resfile();
    for (i = 0; i < list_len; i++) {

        memset(tmp_name, 0, sizeof(tmp_name));
        /* tmp_strlen = strlen(WATCH_STY_CHECK_LIST[i]); */
        /* strcpy(tmp_name, WATCH_STY_CHECK_LIST[i]); */
        tmp_strlen = strlen(watch_res[i]);
        strcpy(tmp_name, watch_res[i]);
        strcpy(&tmp_name[tmp_strlen - suffix_len], res_suffix);
        tmp_name[tmp_strlen - suffix_len + strlen(res_suffix)] = '\0';

        /* ret = open_resfile(WATCH_RES_CHECK_LIST[i]); */
        ret = open_resfile(tmp_name);
        if (ret) {
            return -EINVAL;
        }
        close_resfile();
    }


    close_str_file();
    for (i = 0; i < list_len; i++) {

        memset(tmp_name, 0, sizeof(tmp_name));
        /* tmp_strlen = strlen(WATCH_STY_CHECK_LIST[i]); */
        /* strcpy(tmp_name, WATCH_STY_CHECK_LIST[i]); */
        tmp_strlen = strlen(watch_res[i]);
        strcpy(tmp_name, watch_res[i]);
        strcpy(&tmp_name[tmp_strlen - suffix_len], str_suffix);
        tmp_name[tmp_strlen - suffix_len + strlen(str_suffix)] = '\0';

        /* ret = open_str_file(WATCH_STR_CHECK_LIST[i]); */
        ret = open_resfile(tmp_name);
        if (ret) {
            return -EINVAL;
        }
        close_str_file();
    }

#endif
    return 0;
}


#endif

