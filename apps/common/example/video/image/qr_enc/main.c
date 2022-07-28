#include "app_config.h"
#include "ui/ui.h"
#include "ui_api.h"
#include "system/timer.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "asm/gpio.h"
#include "asm/port_waked_up.h"
#include "system/includes.h"
#include "server/audio_server.h"
#include "server/video_dec_server.h"
#include "font/font_textout.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/language_list.h"
#include "ename.h"
#include "asm/rtc.h"
#include "asm/p33.h"
#include "lcd_te_driver.h"
#include "lcd_config.h"
#include "enc_qr_code.h"

/****************使用说明****************
 *使用例子为动态编码二维码 到屏幕指定位置
 *
 ****************************************/
#define QR_LEN 152  //这里我设置的版本号为5 长度计算为 21+(6-1)x4 = 21 + 16 = 37 长度要为 37的倍数
#define QR_URL "{\"apkIsbn\":\"rbwc_jxw\",\"series\":\"DEV_a462849e710f\",\"modelName\":\"wordcard\",\"bleName\":\"JL-AC79XX-710F(BLE)\"}"

#ifdef USE_ENC_QR_CODE_DEMO

static uint8_t row_img[QR_LEN];

void set_compose_mode3(int x, int y, int w, int h);
void qr_data_updata(u8 *buf, u32 data_size);

static void ui_demo_test(void *priv)
{
    char *qr_buf = NULL;
    char *qr = NULL;
    int n;

    int code128_mode = 60;//模式配置勿动
    uint8_t qr_version = 1; //设置当前二维码版本号
    uint8_t qr_max_version = 12;//当解析内容过于复杂的时候会自动升级版本号
    uint8_t qr_ecc_level = 1;//自动纠错等级
    int qr_code_max_input_len_ = 384;//字符串最大长度
    int qr_buf_size = 4096;//二维码内部申请长度

    jl_code_param_t jl_code_param;
    jl_code_init(code128_mode, qr_version, qr_max_version, qr_ecc_level,  qr_code_max_input_len_, qr_buf_size, QR_LEN);

    int out_size;
    int line_size;
    int ret;
    int off = 0;

    qr_buf = calloc(1, QR_LEN * QR_LEN * 3 / 2);
    if (!qr_buf) {
        printf("[error]>>>>>>>fail");
    }

    user_ui_lcd_init();//初始化ui服务和lcd

    ui_show_main(PAGE_0);
    static char str1[] = "URL:https://www.ai-alloy.com/";
    ret = ui_text_set_textu_by_id(BASEFORM, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

    static char str2[] = "微信扫码";
    ret = ui_text_set_textu_by_id(BASEFORM_1, str2, strlen(str2), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

    os_time_dly(200);

    set_lcd_show_data_mode(ui_qr);
    set_compose_mode3(0, 35, QR_LEN, QR_LEN);  //设置图像更新坐标，尺寸
    memset(qr_buf + QR_LEN * QR_LEN, 0x80, QR_LEN * QR_LEN * 1 / 2);//将VU数据全部填充0X80空白数据

    memset(row_img, 0, QR_LEN);

    ret = jl_code_process(3, QR_URL, strlen(QR_URL), &out_size, &line_size); //50为带编码长度 可以理解为字符串长度
    printf(">>>>>>>>>>>>out_size=%d, &line_size=%d", out_size, line_size); //这个out_size为最小单位长度
    if (ret == 1) {
        jl_code_param.l_size = line_size;
        jl_code_set_info(&jl_code_param);
        for (int j = 0; j < out_size; j++) {
            jl_code_get_data(out_size, j, row_img);
            for (int k = 0; k < jl_code_param.l_size; k++) {
                memcpy(qr_buf + off, row_img, QR_LEN);
                off += QR_LEN;
            }
        }
    } else {
        printf(">>>>>>>qr_code enc [error]");
    }
    jl_code_deinit();//释放二维码库中申请的内存
    qr_data_updata(qr_buf, QR_LEN * QR_LEN * 3 / 2); //数据发到数据处理线程去处理
}

static int ui_complete_demo_task_init(void)   //主要是create wifi 线程的
{
    puts("ui_complete_demo_task_init \n\n");
    return thread_fork("ui_demo_test", 10, 1024, 0, 0, ui_demo_test, NULL);
}

late_initcall(ui_complete_demo_task_init);

#endif

