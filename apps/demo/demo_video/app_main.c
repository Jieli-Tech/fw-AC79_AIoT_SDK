#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"

/*中断列表 */
const struct irq_info irq_info_table[] = {
#if CPU_CORE_NUM == 1
    { IRQ_SOFT5_IDX,      7,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      7,   1    }, //此中断强制注册到cpu1
    { -2,     			-2,   -2   },//如果加入了该行, 那么只有该行之前的中断注册到对应核, 其他所有中断强制注册到CPU0
#endif

    { -1,     -1,   -1    },
};

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            15,     2048,	  1024 },
    {"sys_event",           29,     512,	   0 },
    {"systimer",            14,     256, 	   0 },
    {"sys_timer",            9,     512,	  128  },
    {"audio_server",        16,     1024,     256  },
    {"audio_encoder",       14,     1024,     64   },
    {"video_server",        26,     800,      1024 },
    {"dynamic_huffman0",    15,     256,      32   },
    {"video0_rec0",         25,     512,      512  },
    {"video0_rec1",         25,     512,      512  },
    {"audio_rec0",          22,     256,      256  },
    {"audio_rec1",          22,     256,      256  },

    {0, 0, 0, 0, 0},
};

/*
 * hello_video task
 */
static void hello_video_task(void *p)
{
    int cnt = 0;
    int user_video_rec0_open(void);
    int user_video_rec0_close(void);
    u32 user_video_frame_callback(void *data, u32 len, u8 type);
    //数据回调在:user_video_frame_callback

    os_time_dly(10);//延时10个os_tick，等待摄像头电源稳定
    user_video_rec0_open();//打开摄像头
    while (1) {
        os_time_dly(100);
        if (++cnt % 5 == 0) {
            user_video_rec0_close();//关闭摄像头
            os_time_dly(100);
            user_video_rec0_open();//打开摄像头
        }
    }
}

/*
 * 应用程序主函数
 */
void app_main()
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------video demo app_main run %s-------------\r\n\r\n\r\n\r\n\r\n", __TIME__);
    os_task_create(hello_video_task, NULL, 12, 1000, 0, "hello_video_task");
}

