#include "system/includes.h"
#include "action.h"
#include "app_core.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "app_config.h"
#include "storage_device.h"
#include "generic/log.h"
#include "os/os_api.h"

#ifdef CONFIG_AUTO_PRODUCTION_ENABLE

#define JPEG_CHACK_MAX_CNT		3 //连续张数JPEG图片校验相同则为校验正确
#define JPEG_CHACK_MAX_TIMES	3 //检测超时时间秒

//JPEG:CRC16校验码（先看正常摄像头前10秒的连续3个crc一样的值，再把crc填入数组）
//注意：每次修改摄像头的寄存器则需要重新获取crc值，重新把crc填入数组
//添加摄像头型号：先确定摄像头可以输出调试模式(彩条图片输出)才能进行自动量产测试（可联系摄像头原厂调试）
static const u16 jpeg_check_sum[8][4] = {
    {0xbed8, 0x8365, 0xb352, 0xe3cd}, 	/*GC0308*/ 	//支持4个CRC16校验验码
    {0x4e0f, }, 						/*GC0328*/ 	//支持4个CRC16校验验码
    {0x3523, 0xa7c9}, 					/*OV7670*/ 	//支持4个CRC16校验验码
    {0xc550, }, 						/*PV4109*/	//支持4个CRC16校验验码
    {0xb0ad, }, 						/*BF3901*/	//支持4个CRC16校验验码
};
struct jpeg_test {
    u8 prod_test;
    u8 check_jpeg_camera;
    u16 crc_ok_cnt;
    u16 last_crc;
};
static struct jpeg_test jpegtest ALIGNED(4) = {0};

int jpeg_self_image_info_check(u8 *jpeg, u32 len);
void set_video_rt_cb(u32(*cb)(void *, u8 *, u32), void *priv);
int user_net_video_rec_open(char forward);
int user_net_video_rec_close(char forward);
int net_video1_open(void *net_priv);
int net_video1_close(void);
void led_exe(unsigned int i, unsigned int time);
void hhf_led_ctl(char on);

int app_jpeg_set_qval_default(u8 qval)//量产测试固定q值
{
    if (jpegtest.prod_test) {
        return 2;//q值=2
    }
    return 0;
}
void prodct_jpeg_test_set(u8 start)
{
    jpegtest.prod_test = start;
    jpegtest.check_jpeg_camera = 0;
}
int prodct_jpeg_test_get(void)
{
    return jpegtest.prod_test;
}
int prodct_jpeg_test_get_result(void)
{
    if (jpegtest.check_jpeg_camera) {
        printf("---> jpeg check camera ok \n");
        return 0;
    }
    printf("check_jpeg_camera err \n");
    return -EINVAL;
}
void prodct_jpeg_check(void *priv, u8 *buf, u32 len)
{
    u32 *head = (u32 *)buf;
    if (jpegtest.check_jpeg_camera || !jpegtest.prod_test || *head != 0xE0FFD8FF || jpeg_self_image_info_check(buf, len)) {
        return;
    }
    int i = 0, j, find;
    u16 crc = CRC16(buf, len);
    printf("jpeg crc = 0x%x \n", crc);
    if (!jpegtest.check_jpeg_camera) {
        for (i = 0, find = 0; i < ARRAY_SIZE(jpeg_check_sum); i++) {
            for (j = 0; j < ARRAY_SIZE(jpeg_check_sum[0]); j++) {
                if (crc == jpeg_check_sum[i][j] && crc != 0) {
                    find = 1;
                    break;
                }
            }
            if (find) {
                break;
            }
        }
        if ((crc == jpegtest.last_crc || !jpegtest.last_crc || !jpegtest.crc_ok_cnt) && find) {
            jpegtest.crc_ok_cnt++;
        } else {
            jpegtest.crc_ok_cnt = 0;
        }
        jpegtest.last_crc = crc;
        __asm_csync();
        if (jpegtest.crc_ok_cnt >= JPEG_CHACK_MAX_CNT) {
            jpegtest.check_jpeg_camera = 1;
            jpegtest.crc_ok_cnt = 0;
            jpegtest.last_crc = 0;
            printf("---> jpeg check camera ok\n");
        }
    }
}

int prodct_auto_test(void)
{
    int err;
    int wait = JPEG_CHACK_MAX_TIMES;//超时
    int time;
    int ret = 0;

    //=================摄像头视频-JPEG图片检测=====================//
    set_video_rt_cb(prodct_jpeg_check, NULL);//设置JPEG回调数据

    //前视测试
    prodct_jpeg_test_set(1);//启动摄像头JPEG校验
    err = user_net_video_rec_open(1);
    if (!err) {
        time = timer_get_sec() + wait;
        while (time_after(time, timer_get_sec()) && !jpegtest.check_jpeg_camera) {
            os_time_dly(2);
        }
    }
    user_net_video_rec_close(1);
    err = prodct_jpeg_test_get_result();//获取摄像头的JPEG校验结果
    ret |= err;//存留前视摄像头检测结果
    prodct_jpeg_test_set(0);//获取检测结果后，关闭JPEG检测

#if 0
    //后视测试
    prodct_jpeg_test_set(1);//启动摄像头JPEG校验
    err = net_video1_open(NULL);
    if (!err) {
        time = timer_get_sec() + wait;
        while (time_after(time, timer_get_sec()) && !jpegtest.check_jpeg_camera) {
            os_time_dly(2);
        }
    }
    net_video1_close();
    err = prodct_jpeg_test_get_result();//获取摄像头的JPEG校验结果
    ret |= err;
    prodct_jpeg_test_set(0);//获取检测结果后，关闭JPEG检测
#endif
    //=================摄像头视频-JPEG图片检测=====================//

    return ret;
}

int prodct_auto_test_task_create(void)
{
    return thread_fork("prodct_auto_test", 10, 1500, 0, 0, prodct_auto_test, NULL);
}

#endif

