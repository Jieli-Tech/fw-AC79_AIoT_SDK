#ifdef CONFIG_UI_ENABLE

#include "GUI_GIF_Private.h"
#include "lcd_drive.h"
#include "lcd_config.h"
#include "server/video_dec_server.h"//fopen
#include "sys_common.h" //flen()

void play_gif_to_lcd(char *gif_path, char play_speed)
{
    GUI_GIF_IMAGE_INFO info = {0};

    FILE *gif_fd = fopen(gif_path, "r");

    if (!gif_fd) {
        printf("[error]>>>>>>>>gif_file_open_fail\n");
        return;
    }

    u32 gif_len = flen(gif_fd);

    char *buf = (char *)calloc(1, gif_len); //申请GIF缓存BUF
    if (!buf) {
        printf("[error]>>>>>>>>malloc_gif_buf_fail\n");
    }

    fread(buf, gif_len, 1, gif_fd);
    fclose(gif_fd);

    GUI_GIF_GetInfo(buf, gif_len, &info);	//获取图片信息

    u16 *rgb565_buf = (u16 *)calloc(1, info.xSize * info.ySize * 2); //申请RGB565BUF
    if (!rgb565_buf) {
        printf("[error]>>>>>>>>malloc__buf_fail\n");
    }



    if (info.xSize < LCD_W && info.ySize < LCD_H) {
        lcd_set_draw_area(0, info.xSize - 1, 0, info.ySize - 1);
    } else { //比屏大的GIF需要自行缩放 我们提供了工具进行缩放GiF_Cat
        printf("[error]>>>>>>>>>>>>>>>get_gif_h_w != LCD_H_w please use right gif_h_w");
        return ;
    }
    printf("gif--wsize = %d  , hsize = %d-------NumImages = %d  delay = %d\n", info.xSize, info.ySize, info.NumImages, info.Delay);

    for (u16 i = 0; i < info.NumImages; i++) {
        Gif_to_Picture(buf, gif_len, &info, rgb565_buf, 2, i);
        ui_show_frame(rgb565_buf, info.xSize * info.ySize * 2);

        if (play_speed == 0) {
            os_time_dly(info.Delay);
        } else {
            os_time_dly(play_speed);
        }
    }
    os_time_dly(5);//等待屏幕数据发送完再释放内存
    free(buf);
    free(rgb565_buf);
}
#endif
