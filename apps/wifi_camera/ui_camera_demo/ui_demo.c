#include "app_config.h"
#ifdef CONFIG_UI_ENABLE //上电执行则打开app_config.h TCFG_DEMO_UI_RUN = 1

#include "ui/ui.h"
#include "ui_api.h"
#include "system/timer.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "asm/gpio.h"
#include "system/includes.h"
#include "server/audio_server.h"
#include "storage_device.h"
#include "font/font_textout.h"
#include "ui/includes.h"
#include "ui_action_video.h"
#include "font/font_all.h"
#include "font/language_list.h"
#include "ename.h"
#include "asm/rtc.h"
#include "lcd_drive.h"
#include "video_rec.h"
#include "yuv_soft_scalling.h"
#include "event/key_event.h"
#include "net_video_rec.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"

#if TCFG_DEMO_UI_RUN

#if (defined CONFIG_VIDEO_ENABLE && (__SDRAM_SIZE__ >= (2 * 1024 * 1024)))

#if 1
#define log_info(x, ...)    printf("\r\n>>>>>>[UI]" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

#define __TIME1 BASEFORM_51 //年
#define __TIME2 BASEFORM_52 //月
#define __TIME3 BASEFORM_53 //日
#define __TIME4 BASEFORM_54 //时
#define __TIME5 BASEFORM_55 //分
#define __TIME6 BASEFORM_56 //秒
#define __SOUND BASEFORM_69 //秒



static FILE	*fs;
static u32 time_time_id = 0;
static u32 red_time_id = 0;
static u32 SD_time_id = 0;
static u8 sound_value = 40;

struct key_note_hdl { //按键记录按键光标位置信息
    u8 page_x; //记录光标位置
    u8 page_y; //记录页信息
    u8 page2_x;//记录二级界面光标信息
    u8 page2_y;//记录二级菜单确认信息
    u8 language_indx;//语言序号
    u8 init_flog;//刷新标志
};
struct key_note_hdl note = {0, 0, 0, 0, 0};



//初始化时间结构体
static struct sys_time test_rtc_time = {
    .sec = 44,
    .min = 23,
    .hour = 14,
    .day = 21,
    .month = 4,
    .year = 2021,
};



#if CONFIG_MP3_DEC_ENABLE
extern void post_msg_play_flash_mp3(char *file_name, u8 dec_volume);
#endif
extern void camera_to_lcd_init(char camera_ID);//摄像头选择 并初始化
extern void camera_to_lcd_uninit(void);
extern void get_jpg_show_to_lcd(u8 *img_bug, u32 img_len);
extern void get_jpg_show_lcd(u8 *img_buf, u32 img_len);
extern void lcd_show_frame_1(u8 *buf, u32 len);
extern int video_rec_start_notify(void);
extern int avi_net_playback_unpkg_init(FILE *fd, u8 state);//state : 0 preview , 1 playback
extern int avi_video_get_frame(FILE *fd, int offset_num, u8 *buf, u32 buf_len, u8 state);




/*==================开机播提示音已经开机动画==================*/
static void open_animation(char speed)//开机图片以及开机音乐播放
{
    set_lcd_show_data_mode(ui);
    key_event_disable();//如果需要提示音播放完要关按键开关

#if CONFIG_MP3_DEC_ENABLE
    post_msg_play_flash_mp3("001.mp3", 30); //开机提示音
#endif
    ui_show_main(PAGE_2);

    for (u8 i = 0; i < 17; i++) {

        log_info("i=%d.\n", i);
        ui_pic_show_image_by_id(BASEFORM_19, i);
        os_time_dly(speed);
    }

    ui_hide_main(PAGE_2);
    key_event_enable();
}
/*****************end**********************/

/*===========声音数字加减操作==============*/
static void sound_mub(int mub, char dir)//0加时间 1减时间
{
    struct unumber timer;

    timer.type = TYPE_NUM;
    timer.numbs = 2;

    if (dir) { //+
        sound_value++;
    } else { //-
        sound_value--;
    }

    timer.number[0] = mub;
    ui_number_update_by_id(__SOUND, &timer);
}
static void sound_mub_show_init(int mub)
{
    struct unumber timer;

    timer.type = TYPE_NUM;
    timer.numbs = 2;
    timer.number[0] =  mub;
    ui_number_update_by_id(__SOUND, &timer);
}
static void form_highlight(u32 id, int status)
{
    struct element *elm = ui_core_get_element_by_id(id);
    /* log_info("elm = %x.\n", elm); */
    int ret1 = ui_core_highlight_element(elm, status);
    /* log_info("ret1 = %d.\n", ret1); */
    int ret2 = ui_core_redraw(elm->parent);
    /* log_info("ret2 = %d.\n", ret2); */
}
/*****************end**********************/

/*===========日期时间=刷新时间设置界面==============*/
static void time_set_page(char day, char dir) //0加时间 1减时间 day时间选择
{
    struct unumber timer;

    timer.type = TYPE_NUM;
    timer.numbs = 2;

    if (dir) { //加时间
        switch (day) {
        case 0://年
            test_rtc_time.year++;
            timer.number[0] = test_rtc_time.year;
            ui_number_update_by_id(__TIME1, &timer);
            break;

        case 1://月
            test_rtc_time.month++;

            if (test_rtc_time.month == 13) {
                test_rtc_time.month = 1;
            }

            timer.number[0] = test_rtc_time.month;
            ui_number_update_by_id(__TIME2, &timer);
            break;

        case 2://日
            test_rtc_time.day++;

            if (test_rtc_time.day == 31) {
                test_rtc_time.day = 1;
            }

            timer.number[0] = test_rtc_time.day;
            ui_number_update_by_id(__TIME3, &timer);
            break;

        case 3://时
            test_rtc_time.hour++;

            if (test_rtc_time.hour == 25) {
                test_rtc_time.hour = 1;
            }

            timer.number[0] = test_rtc_time.hour;
            ui_number_update_by_id(__TIME4, &timer);
            break;

        case 4://分
            test_rtc_time.min++;

            if (test_rtc_time.min == 61) {
                test_rtc_time.min = 1;
            }

            timer.number[0] = test_rtc_time.min;
            ui_number_update_by_id(__TIME5, &timer);
            break;

        case 5://退出

            break;
        }
    } else { //减时间
        switch (day) {
        case 0://年
            test_rtc_time.year--;

            if (test_rtc_time.year == 0) {
                test_rtc_time.year = 2021;
            }

            timer.number[0] = test_rtc_time.year;
            ui_number_update_by_id(__TIME1, &timer);
            break;

        case 1://月
            test_rtc_time.month--;

            if (test_rtc_time.month == 0) {
                test_rtc_time.month = 12;
            }

            timer.number[0] = test_rtc_time.month;
            ui_number_update_by_id(__TIME2, &timer);
            break;

        case 2://日
            test_rtc_time.day--;

            if (test_rtc_time.day == 0) {
                test_rtc_time.day = 31;
            }

            timer.number[0] = test_rtc_time.day;
            ui_number_update_by_id(__TIME3, &timer);
            break;

        case 3://时
            test_rtc_time.hour--;

            if (test_rtc_time.hour == 0) {
                test_rtc_time.hour = 24;
            }

            timer.number[0] = test_rtc_time.hour;
            ui_number_update_by_id(__TIME4, &timer);
            break;

        case 4://分
            test_rtc_time.min--;

            if (test_rtc_time.min == 0) {
                test_rtc_time.min = 60;
            }

            timer.number[0] = test_rtc_time.min;
            ui_number_update_by_id(__TIME5, &timer);
            break;

        case 5://退出

            break;
        }
    }
}
/*****************end**********************/

/*===========刷新系统时间================*/
void undata_sys_time(void)
{
    void *dev = NULL;
    dev = dev_open("rtc", NULL);
    dev_ioctl(dev, IOCTL_SET_SYS_TIME, (u32)&test_rtc_time);
    dev_close(dev);

    printf("@@@@@@@@@@@@@@@@get_sys_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);
}

/*===========日期时间=刷新时间设置界面==============*/
void time_updata(void)
{
    struct unumber timer;

    timer.type = TYPE_NUM;
    timer.numbs = 2;
    timer.number[0] = test_rtc_time.year;
    ui_number_update_by_id(__TIME1, &timer);

    timer.number[0] = test_rtc_time.month;
    ui_number_update_by_id(__TIME2, &timer);

    timer.number[0] = test_rtc_time.day;
    ui_number_update_by_id(__TIME3, &timer);

    timer.number[0] = test_rtc_time.hour;
    ui_number_update_by_id(__TIME4, &timer);

    timer.number[0] = test_rtc_time.min;
    ui_number_update_by_id(__TIME5, &timer);

    /*timer.number[0] = test_rtc_time.sec;*/
    timer.number[0] = 00;
    ui_number_update_by_id(__TIME6, &timer);
}
/*****************end**********************/

/*==================UI_camera时间显示==================*/
static void set_rtc_init_time(void)
{
    void *dev = NULL;
    /* 打开RTC设备 */
    dev = dev_open("rtc", NULL);
    /* 赋值时间信息 */
    test_rtc_time.year = 2021;
    test_rtc_time.month = 5;
    test_rtc_time.day = 19;
    test_rtc_time.hour = 11;
    test_rtc_time.min = 43;
    test_rtc_time.sec = 0;
    /* 设置时间信息  */
    dev_ioctl(dev, IOCTL_SET_SYS_TIME, (u32)&test_rtc_time);
    dev_close(dev);
}
static void get_rtc_time(void)
{
    void *dev = NULL;
    /* 打开RTC设备 */
    dev = dev_open("rtc", NULL);
    /* 获取时间信息 */
    dev_ioctl(dev, IOCTL_GET_SYS_TIME, (u32)&test_rtc_time);
    /* 打印时间信息 */
    /*printf("get_sys_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);*/
    dev_close(dev);
}

static void timer_change_handler(void *priv)
{
    void *dev = NULL;
    static u32 sec = 0;
    struct utime time_r;

    if (!note.init_flog) {
        dev = dev_open("rtc", NULL);
        /* 获取时间信息 */
        dev_ioctl(dev, IOCTL_GET_SYS_TIME, (u32)&test_rtc_time);

        /*printf("get_sys_time: %d-%d-%d %d:%d:%d\n", test_rtc_time.year, test_rtc_time.month, test_rtc_time.day, test_rtc_time.hour, test_rtc_time.min, test_rtc_time.sec);*/
        time_r.year = test_rtc_time.year;
        time_r.month = test_rtc_time.month;
        time_r.day = test_rtc_time.day;
        time_r.hour = test_rtc_time.hour;
        time_r.min = test_rtc_time.min;
        time_r.sec = test_rtc_time.sec;

        ui_time_update_by_id(BASEFORM_2, &time_r);
        putchar('C');
        /* 关闭RTC设备 */
        dev_close(dev);
    }
}

static int timer_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *time = (struct ui_time *)ctr;
    static u8 init_flog = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        if (!init_flog) {
            init_flog = 1;
            printf("!!!!!!!!!!!!!!!!!init");
            /*set_rtc_init_time();*/
            /*time_time_id = sys_timer_add(NULL, timer_change_handler, 1000);*/
        }

        break;

    case ON_CHANGE_HIDE:
        break;

    case ON_CHANGE_SHOW_PROBE:
        break;

    case ON_CHANGE_SHOW_POST:
        break;

    default:
        return false;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_2)
.onchange = timer_onchange,
};
/*****************end**********************/

/*==================UI_camera电池图标显示==================*/
static void battery_change_handler(void *priv)
{
    static u8 percent = 0;
    putchar('B');
    ui_battery_level_change(percent, 0);
    percent += 20;

    if (percent >= 120) {
        percent = 0;
    }
}

static int battery_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_battery *battery = (struct ui_battery *)ctr;
    static u32 timer = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        if (!timer) {
            timer = sys_timer_add(NULL, battery_change_handler, 1000); //开启进入动态电池电量显示
        }

        break;

    case ON_CHANGE_RELEASE:
        if (timer) {
            sys_timer_del(timer);
            timer = 0;
        }

        break;

    default:
        return false;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_4)
.onchange = battery_onchange,
};
/*****************end**********************/


/*=============SD卡图标===============*/
static void SD_car_image_change(char sd_in)
{
    if (sd_in) {
        ui_pic_show_image_by_id(BASEFORM_3, 0);
    } else {
        ui_pic_show_image_by_id(BASEFORM_3, 1);
    }
}
/*****************end**********************/

/*============录像红点闪烁以及录像控制===============*/
static void display_red_dot(void *priv)
{
    static u8 time = 0;
    time++;

    if (time == 2) {
        time = 0;
    }

    log_info(">>>>>>>>>red_dot=%d.\n", time);
    ui_pic_show_image_by_id(BASEFORM_6, time);
}

static void ui_video_rec_display(void)
{
    static u8 run_flog = 0;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>ui_video_rec_display");
    if (!run_flog) {
        run_flog = 1;
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>add_time");
        red_time_id = sys_timer_add(NULL, display_red_dot, 800);
    } else {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>else");
        run_flog = 0;
        sys_timer_del(red_time_id);
        ui_pic_show_image_by_id(BASEFORM_6, 0);
        red_time_id = 0;
    }
}
//UI_camera录像控制
static void ui_video_rec_contrl_doing()
{
    video_rec_control_doing();
}
/*****************end**********************/

/*===========视频回放ui================*/
static const char ascii_str[] = "ABCDEFGHIJK";

static void ui_video_playback_show_text(char *file_name)
{
    ui_text_set_text_by_id(BASEFORM_64, file_name, strlen(file_name), FONT_DEFAULT);
}
/*****************end**********************/

/*===========照片回放ui================*/
static void ui_photo_playback_show_text(char *file_name)
{
    ui_text_set_text_by_id(BASEFORM_68, file_name, strlen(file_name), FONT_DEFAULT);
}
void photo_find_to_show_lcd(char dir, char run_flog)//选择文件的方向dir = 1 nextfile 0you know,last ,run_flog = 1start, run_flog = 0end
{
    static FILE *fd = NULL;
    static struct vfscan *fs = NULL;
    static u8 run_time = 0;
    u32 f_size;
    char path[64];
    char *buf_img = NULL;

    if (run_time == 0) { // run one time start
        run_time = 1;
        strcpy(path, CONFIG_REC_PATH_0);
        fs = fscan(path, "-tJPG -st", 0);//筛选出JPG图片 每次调用这个都会重新指向第一个
        if (fs == NULL) {
            printf("[error]>>>>>fs = NULL");
            return;
        }
    }

    if (run_flog == 0) { //run end
        run_time = 0;
        free(buf_img);
        fscan_release(fs);
        buf_img = NULL;
        fs = NULL;
        fd = NULL;
        run_time = 0;
        return;
    }

    if (fd == NULL) {
        fd = fselect(fs, FSEL_FIRST_FILE, 0);//文件选择第一个
    } else {
        if (dir) { //next
            fd = fselect(fs, FSEL_NEXT_FILE, 0);//文件选择下一个
        } else {
            fd = fselect(fs, FSEL_PREV_FILE, 0);//
            if (fd == NULL) {
                fd = fselect(fs, FSEL_LAST_FILE, 0);//
            }
        }
    }

    if (fd == NULL) {
        printf("[error]>>>>>>>>>>>>>>>>fd == NULL");
        return;
    }

    f_size = flen(fd);
    buf_img = malloc(f_size);
    fread(buf_img, f_size, 1, fd);
    fclose(fd);
    get_jpg_show_lcd(buf_img, f_size);
    free(buf_img);
}


/*****************end**********************/
extern int avi_net_playback_unpkg_init(FILE *fd, u8 state);//state : 0 preview , 1 playbackh
void yuv420p_quto_rgb565(unsigned char *yuvBuffer_in, unsigned char *rgbBuffer_out, int width, int height);
int avi_video_get_frame(FILE *fd, int offset_num, u8 *buf, u32 buf_len, u8 state);
#define IDX_00DC   ntohl(0x30306463)
#define IDX_01WB   ntohl(0x30317762)
#define IDX_00WB   ntohl(0x30307762)
/*int start_play_video(const char *path)*/
/*==========视频回放ui=================*/
static void video_find_to_show_lcd(char dir, run_flog)//选择文件的方向
{
    int ret;
    char *fbuf = NULL;
    char *yuv = NULL;
    char *cy, *cb, *cr;
    int fbuflen = 50 * 1024;
    int num = 0;
    int pix;
    char ytype;
    int yuv_len;

    FILE *fd = NULL;
    char path[64];
    struct vfscan *fs = NULL;
    static char *buf_img = NULL;
    static u8 run_time = 0;

    if (buf_img == NULL) {
        buf_img = malloc(320 * 240 * 2);
        fbuf = malloc(fbuflen);
    }

    if (run_time == 0) {
        run_time = 1;
        strcpy(path, CONFIG_REC_PATH_0);
        fs = fscan(path, "-tAVI -st", 0);//筛选出AVI图片 每次调用这个都会重新指向第一个
        if (fs == NULL) {
            printf("[error]>>>>>fs = NULL");
            return;
        }

    }

    if (run_flog == 0) { //run end
        run_time = 0;
        free(buf_img);
        fscan_release(fs);
        buf_img = NULL;
        fs = NULL;
        fd = NULL;
        run_time = 0;
        return;
    }

    if (fd == NULL) {
        fd = fselect(fs, FSEL_FIRST_FILE, 0);//文件选择第一个
    } else {
        if (dir) { //next
            fd = fselect(fs, FSEL_NEXT_FILE, 0);//文件选择下一个
        } else {
            fd = fselect(fs, FSEL_PREV_FILE, 0);//
            if (fd == NULL) {
                fd = fselect(fs, FSEL_LAST_FILE, 0);//
            }
        }
    }

    if (fd == NULL) {
        printf("[error]>>>>>>>>>>>>>>>>fd == NULL");
        return;
    }

    ret = avi_net_playback_unpkg_init(fd, 1); //解码初始化,最多10分钟视频
    if (ret) {
        printf("avi_net_playback_unpkg_init err!!!\n");
    }
    printf("@@@>>>>>>>>>>video_start");
    while (1) {
        ret = avi_video_get_frame(fd, ++num, fbuf, fbuflen, 1); //全回放功能获取帧
        if (ret > 0) {
            struct jpeg_image_info info = {0};
            struct jpeg_decode_req req = {0};
            u32 *head = (u32 *)fbuf;
            u8 *dec_buf = fbuf;
            u32 fblen = ret;
            if (*head == IDX_00DC || *head == IDX_01WB || *head == IDX_00WB) {
                fblen -= 8;
                dec_buf += 8;
            }
            info.input.data.buf = dec_buf;
            info.input.data.len = fblen;
            if (jpeg_decode_image_info(&info)) {//获取JPEG图片信息
                printf("jpeg_decode_image_info err\n");
                break;
            } else {
                switch (info.sample_fmt) {
                case JPG_SAMP_FMT_YUV444:
                    ytype = 1;
                    break;//444
                case JPG_SAMP_FMT_YUV420:
                    ytype = 4;
                    break;//420
                default:
                    ytype = 2;
                }
                pix = info.width * info.height;
                yuv_len = pix + pix / ytype * 2;
                if (!yuv) {
                    yuv = malloc(yuv_len);
                    if (!yuv) {
                        printf("yuv malloc err len : %d , width : %d , height : %d \n", yuv_len, info.width, info.height);
                        break;
                    }
                }
                /*printf("width : %d , height : %d \n", info.width, info.height);*/

                cy = yuv;
                cb = cy + pix;
                cr = cb + pix / ytype;

                req.input_type = JPEG_INPUT_TYPE_DATA;
                req.input.data.buf = info.input.data.buf;
                req.input.data.len = info.input.data.len;
                req.buf_y = cy;
                req.buf_u = cb;
                req.buf_v = cr;
                req.buf_width = info.width;
                req.buf_height = info.height;
                req.out_width = info.width;
                req.out_height = info.height;
                req.output_type = JPEG_DECODE_TYPE_DEFAULT;
                req.bits_mode = BITS_MODE_UNCACHE;
                /*req.bits_mode = BITS_MODE_CACHE;*/

                ret = jpeg_decode_one_image(&req);//JPEG转YUV解码
                if (ret) {
                    printf("jpeg decode err !!\n");
                    break;
                }
                YUV420p_Soft_Scaling(yuv, NULL, 640, 480, 320, 240);
                yuv420p_quto_rgb565(yuv, buf_img, 320, 240);
                lcd_show_frame_1(buf_img, (320 * 240 * 2));//240*320*2=153600
            }
        } else {
            printf("@@@>>>>>>>>>>video_end");
            break ;
        }
    }
    run_time = 0; //这里没有做切视频播放 请参考照片切换
    free(buf_img);
    fscan_release(fs);
    buf_img = NULL;
    fs = NULL;
    fd = NULL;
    run_time = 0;
}
/*****************end**********************/

/*==========ui界面：主界面=================*/
static int ui_page1_main(void *_grid, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_grid;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        (grid)->hi_index = note.page_x;
        (grid)->page_mode = 1;
        break;

    case ON_CHANGE_RELEASE:
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_32)
.onchange = ui_page1_main,
};
/*****************end**********************/

/*==========显示某个界面===============*/
static void show_page(int id)
{
    ui_hide_curr_main(); //关闭上一个画面
    ui_show_main(id);
}
/*****************end**********************/

static u8 choice_photo_flog = 0;
static u8 choice_video_flog = 0;
static u8 taka_video_flog = 0;
static u8 taka_photo_flog = 0;

static void ui_demo(void *priv)
{
    static char str1[] = "camera_in";
    static char str2[] = "camera_out";
    int msg[32];
#if NO_UI_LCD_TEST
    void camera_to_lcd_test(void);
    camera_to_lcd_test();
    while (1) {
        os_time_dly(100);
    }
#endif

    camera_to_lcd_init(0);

    user_ui_lcd_init();//初始化ui服务和lcd
    open_animation(1);//开机动画 20为延时 200ms每帧

#if TCFG_LCD_USB_SHOW_COLLEAGUE
    set_lcd_show_data_mode(camera);
    while (1) {
        os_taskq_pend("taskq", msg, ARRAY_SIZE(msg)); //接收app_music.c中发来的消息 没有消息在这行等待

        switch (msg[1]) {
        case APP_CAMERA_IN:
            printf(">>>>>>>>>>>>>APP_CAMERA_IN");
            set_lcd_show_data_mode(ui);
            ui_show_main(PAGE_7);
            ui_text_set_textu_by_id(BASEFORM_73, str2, strlen(str2), FONT_DEFAULT);
            os_time_dly(150);
            set_lcd_show_data_mode(camera);
            break;
        case APP_CAMERA_OUT:
            printf(">>>>>>>>>>>>>APP_CAMERA_OUT");
            set_lcd_show_data_mode(ui);
            ui_show_main(PAGE_7);
            ui_text_set_textu_by_id(BASEFORM_73, str1, strlen(str1), FONT_DEFAULT);
            os_time_dly(150);
            set_lcd_show_data_mode(camera);
            break;
        }
    }
#endif

    set_lcd_show_data_mode(ui_camera);
    ui_show_main(PAGE_0);

    static u8 time = 0 ;
    while (1) {
        if (get_lcd_show_deta_mode() == ui_camera) {

            if (storage_device_ready()) {
                SD_car_image_change(1);
            } else {
                SD_car_image_change(0);
            }

            if (taka_video_flog == 1) {
                taka_video_flog = 0;
                ui_video_rec_contrl_doing();
            }

            if (taka_photo_flog == 1) {
                taka_photo_flog = 0;
                video_rec_take_photo();
            }
        }
        if (get_lcd_show_deta_mode() == ui) {

        }
        if (get_lcd_show_deta_mode() == ui_user) {
            if (choice_photo_flog == 1) { //start init
                choice_photo_flog = 0;
                photo_find_to_show_lcd(1, 1);
            }
            if (choice_photo_flog == 2) { //photo next
                choice_photo_flog = 0;
                photo_find_to_show_lcd(1, 1);
            }
            if (choice_photo_flog == 3) { //photo last
                choice_photo_flog = 0;
                photo_find_to_show_lcd(0, 1);
            }
            if (choice_photo_flog == 4) { //out end
                choice_photo_flog = 0;
                photo_find_to_show_lcd(0, 0);
            }
            if (choice_video_flog == 1) {
                choice_video_flog = 0;
                video_find_to_show_lcd(1, 1);
            }
        }
        os_time_dly(50);//500ms检查一次SD卡状态
    }
}
static int ui_demo_task_init(void)
{
    puts("ui_demo_task_init \n\n");
    return thread_fork("ui_demo", 11, 1024, 32, 0, ui_demo, NULL);
}
late_initcall(ui_demo_task_init);

/*****************end**********************/

/*==========ui按键控制============*/
int ui_key_control(u8 value, u8 event)//纯ui按键控制
{
    int msg;
    switch (event) {
    case KEY_EVENT_CLICK:
        switch (value) {
        case KEY_ENC:
            printf(">>>>>>>>>>>>>>>>ui_key1");//向下

            if (note.page_y == 0) {
                note.page_x++;
                if (note.page_x == 6) {
                    note.page_x = 0;
                }
                printf(">>>>>>>>>>page_x=%d", note.page_x);
                ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
            } else { //表示已经进入二级界面
                switch (note.page_x) {
                case 0:	//图片质量
                    note.page2_x++;
                    if (note.page2_x == 4) {
                        note.page2_x = 0;
                    }
                    ui_key_msg_post(KEY_DOWN);//这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
                    break;

                case 1:	//语言
                    note.language_indx ^= 1;
                    ui_text_show_index_by_id(BASEFORM_11, note.language_indx);//切换语言文字
                    break;

                case 2:	//日期时间
                    if (note.page2_y) { //二级菜单确认
                        time_set_page(note.page2_x, 0);//加时间操作
                    } else { //二级菜单退出确认
                        note.page2_x++;
                        if (note.page2_x == 6) {
                            note.page2_x = 0;
                        }
                        ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
                    }
                    break;

                case 3:	//声音
                    sound_mub(sound_value, 0);

                    break;

                case 4:	//录像回放
                    choice_video_flog = 1;

                    break;

                case 5:	//相册
                    choice_photo_flog = 2;
                    break;
                }
            }

            break;

        case KEY_UP:
            printf(">>>>>>>>>>>>>>>ui_key1");//向上

            if (note.page_y == 0) {
                note.page_x--;
                if (note.page_x == 255) {
                    note.page_x = 5;
                }

                printf(">>>>>>>>>>page_x=%d", note.page_x);
                ui_key_msg_post(KEY_UP);
            } else { //表示已经进入二级界面
                switch (note.page_x) {
                case 0:	//图片质量
                    note.page2_x--;
                    if (note.page2_x == 255) {
                        note.page2_x = 3;
                    }
                    ui_key_msg_post(KEY_UP);
                    break;

                case 1:	//语言
                    note.language_indx ^= 1;
                    ui_text_show_index_by_id(BASEFORM_11, note.language_indx);//切换语言文字
                    break;

                case 2:	//日期时间
                    if (note.page2_y) { //二级菜单确认
                        time_set_page(note.page2_x, 1);//减时间操作
                    } else { //二级菜单退出确认
                        note.page2_x--;
                        if (note.page2_x == 255) {
                            note.page2_x = 5;
                        }
                        ui_key_msg_post(KEY_UP);
                    }
                    break;

                case 3:	//声音
                    sound_mub(sound_value, 1);

                    break;

                case 4:	//录像回放

                    break;

                case 5:	//相册
                    choice_photo_flog = 3;
                    break;
                }
            }
            break;

        case KEY_F1:
            printf(">>>>>>>>>>>>>>>>ui_key3");//确定

            switch (note.page_x) { //目前在一级菜单 二级菜单控制 note.page_y = 0; 表示退出二级菜单
            case 0:	//图片质量
                note.page_y++;
                if (note.page_y == 2) {
                    note.page_y = 0;
                }

                break;

            case 1:	//语言 //后面可以进行高亮语言文字
                note.page_y++;
                if (note.page_y == 2) {
                    note.page_y = 0;
                }

                break;

            case 2:	//日期时间
                note.page_y = 1;
                if (note.init_flog == 1) { //初始化完后在点确认//才能++；
                    note.page2_y++;
                }
                if (note.page2_y == 2) { //二级菜单中确认控制
                    note.page2_y = 0;
                }
                if (note.page2_x == 5) { //完成秒设置后退出
                    note.page_y++;
                    if (note.page_y == 2) {
                        note.page_y = 0;
                    }
                }
                break;

            case 3:	//声音
                note.page_y++;
                if (note.page_y == 2) {
                    note.page_y = 0;
                }

                break;

            case 4:	//录像回放
                note.page_y++;
                if (note.page_y == 2) {
                    note.page_y = 0;
                }

                break;

            case 5:	//相册
                note.page_y++;
                if (note.page_y == 2) {
                    note.page_y = 0;
                }
                break;
            }

            if (note.page_y == 1) { //进入二级菜单
                switch (note.page_x) {
                case 0:	//图片质量
                    show_page(PAGE_4);
                    break;

                case 1:	//语言 //后面可以进行高亮语言文字
                    break;

                case 2:	//日期时间
                    if (note.init_flog == 0) {
                        note.init_flog = 1;
                        show_page(PAGE_3);
                        get_rtc_time();
                        time_updata();
                    }

                    break;

                case 3:	//声音
                    if (note.init_flog == 0) {
                        note.init_flog = 1;
                        form_highlight(BASEFORM_69, note.init_flog); //自定义数字不支持高亮

                    }

                    break;

                case 4:	//录像回放
                    set_lcd_show_data_mode(ui_user);
                    show_page(PAGE_5);
                    ui_video_playback_show_text("video");
                    choice_video_flog = 1;
                    break;

                case 5:	//相册
                    set_lcd_show_data_mode(ui_user);
                    show_page(PAGE_6);
                    choice_photo_flog = 1;
                    printf(">>>>>lcd_mode == ui_camera");
                    break;
                }
            } else { //退出二级菜单
                switch (note.page_x) {
                case 1:	//语言
                    if (note.language_indx == 1) {
                        ui_language_set(Chinese_Traditional);
                        log_info(">>>Chinese_Traditional");
                    } else {
                        ui_language_set(Chinese_Simplified);
                        log_info(">>>Chinese_Simplified");
                    }

                    break;

                case 2:	//日期时间
                    note.init_flog = 0;
                    note.page2_x = 0;
                    note.page2_y = 0;
                    undata_sys_time();
                    break;

                case 3:	//声音
                    note.init_flog = 0;
                    form_highlight(BASEFORM_69, note.init_flog); //自定义数字不支持高亮
                    break;

                case 4:	//录像回放
                    set_lcd_show_data_mode(ui);
                    break;

                case 5:	//相册
                    set_lcd_show_data_mode(ui);
                    printf(">>>>>lcd_mode == ui");
                    choice_photo_flog = 4;
                    break;
                }

                show_page(PAGE_1);
                sound_mub_show_init(sound_value);
            }

            break;

        case KEY_MENU:
            printf(">>>>>>>>>>>>>>>key4");//按键切显示模式
            set_lcd_show_data_mode(ui);
            printf(">>>>>lcd_mode == ui");
            show_page(PAGE_1);
            sound_mub_show_init(sound_value);
            break;
        }

        break;

    default:
        break;
    }

    return 0;
}
/*****************end**********************/

/*==========ui_camera按键控制============*/
int ui_camera_key_control(u8 value, u8 event)//图像合成界面按键控制
{
    switch (event) {
    case KEY_EVENT_CLICK:
        switch (value) {
        case KEY_CANCLE:
            printf(">>>>>>>>>>>>>>>>ui_camera_key1"); //拍照
            taka_photo_flog = 1;//需要开录像才能拍照 //如果想不开录像就拍照走YUV获取一帧数据
            break;

        case KEY_ENC:
            taka_video_flog = 1;
            ui_video_rec_display();
            printf(">>>>>>>>>>>>>>>>ui_camera_key2"); //录像
            break;

        case KEY_F1:
            printf(">>>>>>>>>>>>>>>>ui_camera_key3"); //切换摄像头

            break;

        case KEY_DOWN:
            printf(">>>>>>>>>>>>>>>key4");//按键切显示模式
            show_page(PAGE_0);
            sound_mub_show_init(sound_value);
            set_lcd_show_data_mode(ui_camera);
            printf(">>>>>lcd_mode == ui_camera");

            break;
        }

        break;

    default:
        break;
    }

    return 0;
}
/*****************end**********************/
#endif
#endif
#endif
