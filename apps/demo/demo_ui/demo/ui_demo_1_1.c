/********************************测试例程说明************************************
 *  功能说明：
 *    	该文件主要用于测试屏幕刷新能力测试 已经各个模式下的屏刷能力
 *     	播放视频bad_apple.avi,并在显示时显示刷新帧数 //改显示帧数是为两张整帧合成的
 *
 *  使用说明：
 *      显示屏使用LCD ST7789S,规格320*240  ui使用的横屏
 *      屏幕驱动接口使用 硬件PAP来进行并行发数据
 *      UI工程需要使用 V1.11.0工程 将该UI工程中的res文件夹中的资源文件拷贝到
 *      本工程 wl80中的tool中的res文件夹中
 * ******************************************************************************/

#include "ename.h"
#include "ui/ui.h"
#include "ui_api.h"
#include "app_config.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"
#include "font/language_list.h"

#include "yuv_to_rgb.h"
#include "lcd_drive.h"
#include "lcd_te_driver.h"
#include "yuv_soft_scalling.h"

#include "os/os_api.h"
#include "asm/includes.h"
#include "system/timer.h"
#include "sys_common.h"
#include "system/includes.h"
#include "simple_avi_unpkg.h"
#include "server/server_core.h"
#include "server/audio_server.h"
#include "server/video_dec_server.h"

#ifdef UI_DEMO_1_1

#define IDX_00DC   	ntohl(0x30306463)
#define IDX_01WB   	ntohl(0x30317762)
#define IDX_00WB   	ntohl(0x30307762)


static void put_fps_to_lcd(void)
{
    struct unumber timer;
    timer.numbs = 2;
    timer.type = TYPE_NUM;
    timer.number[0] = 00;
    get_te_fps(&timer.number[0]);
    ui_number_update_by_id(BASEFORM_80, &timer);
}
static void Calculation_frame(void)
{
    struct unumber timer;
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;
        if (tdiff >= 1000) {
            printf("\n [data_to_te]fps_count = %d\n", fps_cnt *  1000 / tdiff);
            tstart = 0;
            fps_cnt = 0;
        }
    }
}

/*==========avi视频===============*/
static void bad_apple_avi_play(void)
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

    if (fbuf == NULL) {
        fbuf = malloc(fbuflen);
    }

    fd = fopen(CONFIG_UI_PATH_FLASH"bad.avi", "r");
    if (fd == NULL) {
        printf("[error]>>>>>fd= NULL");
        return;
    }

    ret = avi_net_playback_unpkg_init(fd, 1); //解码初始化,最多10分钟视频

    if (ret) {
        printf("avi_net_playback_unpkg_init err!!!\n");
    }
    printf(">>>>>>>>>video_start");

    static char str1[] = "视频播放测试";
    static char str2[] = "fps";
    ui_show_main(PAGE_7); //开机图片
    ui_text_set_textu_by_id(BASEFORM_78, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    ui_text_set_textu_by_id(BASEFORM_79, str2, strlen(str2), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

    struct unumber timer;
    timer.numbs = 2;
    timer.type = TYPE_NUM;
    timer.number[0] = 00;
    ui_number_update_by_id(BASEFORM_80, &timer);

    sys_timer_add(NULL, put_fps_to_lcd, 1000);

    set_lcd_show_data_mode(ui_camera);

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
                /*req.bits_mode = BITS_MODE_UNCACHE;*/
                req.bits_mode = BITS_MODE_CACHE;

                ret = jpeg_decode_one_image(&req);//JPEG转YUV解码
                if (ret) {
                    printf("jpeg decode err !!\n");
                    break;
                }
#if HORIZONTAL_SCREEN
                YUV420p_Soft_Scaling(yuv, NULL, info.width, info.height, 320, 240);
#else
                YUV420p_Soft_Scaling(yuv, NULL, info.width, info.height, 240, 320);
#endif


                lcd_show_frame(yuv, (320 * 240 * 2));//240*320*2=153600
                Calculation_frame();

                os_time_dly(3);

            }
        } else {
            printf("@@@>>>>>>>>>>video_end");
            while (1) {
                os_time_dly(100);

            }
            break ;
        }
    }
    fd = NULL;
}

/*==========ui初始化=============*/
static void bad_apply_fps_test(void *priv)
{
    extern const struct ui_devices_cfg ui_cfg_data;
    int lcd_ui_init(void *arg);
    lcd_ui_init(&ui_cfg_data);

    /******开机图片************/
    ui_show_main(PAGE_2); //开机图片
    os_time_dly(100);
    ui_hide_curr_main(); //关闭上一个画面
    bad_apple_avi_play();
}

static int bad_apple_task_init(void)   //主要是create wifi 线程的
{
    puts("bad_apple_task_init \n\n");
    return thread_fork("bad_apply_fps_test", 10, 1024, 0, 0, bad_apply_fps_test, NULL);
}

late_initcall(bad_apple_task_init);

#endif
