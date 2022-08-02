#include "server/ai_server.h"
#include "vt_types.h"
#include "timer.h"
#include "os/os_api.h"
#include "vt_bk.h"
#include "vt_types.h"
#include "linux_camera.h"
#include "linux_player.h"
#include "version.h"

#ifdef WT_SDK_VERSION
MODULE_VERSION_EXPORT(wt_sdk, WT_SDK_VERSION);
#endif

static int wt_app_init(void);
static void wt_app_uninit(void);
extern void vt_reconghttpRequest_http_request_exit(void);
extern void vt_reconghttpRequest_http_close(void);

static int wt_app_task_pid = 0;

struct wt_var {
    volatile u8 exit_flag;
    u8 connect_status;
    u8 init_flag;
};

static struct wt_var wt_hdl;

#define __this 	    (&wt_hdl)

const struct ai_sdk_api wt_sdk_api;

enum {
    WT_TURNING_START = 0x10,
    WT_TURNING_STOP,
    WT_TURNING_PAUSE,
    WT_TURNING_RESUME,
    WT_TURNING_QUIT,
    WT_TURNING_MEDIA_END,
    WT_TURNING_PLAY_PAUSE,
    WT_TURNING_MEDIA_STOP,
    WT_TURNING_PLAY_START,
    WT_TURNING_ERR,
    WT_TURNING_RECON_SUCC,
};

static const int events[][2] = {
    { AI_EVENT_RUN_START, WT_TURNING_START			 },
    { AI_EVENT_RUN_STOP,  WT_TURNING_STOP			 },
#if 0
    { AI_EVENT_RUN_PAUASE, WT_TURNING_PAUSE			 },
    { AI_EVENT_RUN_RESUME, WT_TURNING_RESUME		 },
    { AI_EVENT_PLAY_START, WT_TURNING_PLAY_START     },
#endif
    { AI_EVENT_QUIT   	, WT_TURNING_QUIT    	     },
    { AI_EVENT_MEDIA_END, WT_TURNING_MEDIA_END       },
    { AI_EVENT_PLAY_PAUSE, WT_TURNING_PLAY_PAUSE     },
    { AI_EVENT_MEDIA_STOP, WT_TURNING_MEDIA_STOP     },
};

static int event2page_turning(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }

    return -1;
}

static int wt_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[4];
    int page_turning_event;

    page_turning_event = event2page_turning(event);
    if (page_turning_event == -1) {
        return 0;
    }

    printf("wt_sdk_do_event: %x\n", page_turning_event);

    argv[0] = page_turning_event;
    argv[1] = arg;

    do {
        err = os_taskq_post_type("wt_app_task", Q_USER, argc, argv);
        if (err != OS_Q_FULL) {
            break;
        }
        os_time_dly(10);
    } while (1);

    return err;
}

static int wt_sdk_open(void)
{
    return wt_app_init();
}

static int wt_sdk_check(void)
{
    if (__this->connect_status) {
        return AI_STAT_CONNECTED;
    }

    return AI_STAT_DISCONNECTED;
}

static int wt_sdk_disconnect(void)
{
    wt_app_uninit();
    return 0;
}



#if 0
//羽恒账号,不能发出去
#define  LICENSE_TEXT "QTY5Mjg3MEM4N0Y3Mzk3QkE5QTEyMzRGRUZENEIyQ0YxRUI2NEM2RURENkU5Q0YyNzlDRUJGMTQ4MjJBMzZFNzU4M0Y1NkM5MUM5RENGNjIyMDJBNUU0Q0RFMUU2M0UxQ0ZERjkzQzE4M0YwNEE4RjEyNUJEOENCNjNDNzExMDQ0NzRGQkNEQkIzNzA0M0QwRDAxOUYzOTRBMUE5ODhCMjNBRTA5QzFDOURGNTlDQTczNkQ4OTQ3MDNFNDMwRkQxNjBFQUEyMjVFNEIxQkNENDkwRjY0MjM1NTE5RjVBQ0ZDRDIwN0ZBNjI5RTg4NzZCN0ZDNjM4NEY1QTU4MDQ3RDIxMUY0QTc3QUE4MkRDMjFBMDFDMzkyMTkwMEI0MDdFNDMwNzM0Q0ExQ0JENTVEMzY0REUwMUUyOTEzQzg3OTE2OTlCRkZDODFCMjYxNDM2NEE2QTlDMjU4NTcwRDlBRUFFOUUyRDMzRTZDODMwMjFEN0EzRTAyNkEyNzExRDAwRDM2NDc2MTgzNjRFNTFCMjBBMDYwMjJBNDg5MUU4Mzk0RTBFOERDNDAwNjJDMEQxMzNBQUY3RkU0RDFFQUE1NUQ0RUE4MzYzMDUwRTJFNUVFMjEwNjgwQkNCMDcwOTJCOTMyNjI1N0VCRTBCNjQyMjZCQzE3QjU2RTFCNjNBNzYxRTAyNTRCMDE2OTY1NDk2RjIwMzA4QUNCRTE3NEE4MDBGQzdBRTRDMkU4MzI0MjVDQUM2RTJDQjdBNUY2QkI2"

#define MODULE_TYPE "rtos_yh"

#else

#define LICENSE_TEXT "OUM2MTZEOTIyRjE2MDA3RUU2REYwNjQzNDQ1OEZEOTlBNTBBNDJCRkI3NkM2OERBNzEyNTU1RTJDQUIyMjc0QUYzMTZCRTE3RkIxN0I3MTZCQ0YyNDIwNDg1Q0QwNzc3NEZEMTJFRTQzMjM1QkI0NTYyRjg4MDM5NDA5RjMzQUFDODM1MDk2QUYyQUJFRTZBOUYxOEQ5Njg0RUM5M0E1OEE4RThFRDUzRTA2OENFOTU5RTM2NkIxNTk4NTI5NjJGQzAyMUNGNjU2MTQzRkM1OEQ0QjlENkE5NzkzOTE1QTdDRjIwREMwQURBQUI2MTc0MjY0QkMxNzRGNDQyRjFFQUNDODE5MjBDNDdCQjQ0RTQ1NUQzNkRCNDU4ODhCMDU5OTUxNDAyMDI0Q0QxNjZDREU0NTE4NEE1NzAzQzEwRDI5NzNEMzg0Q0U5ODgwODM3Mzc5QUVDQkM3MDEwRkEyNkQyMjFGOEEzN0Q4QzQzOUExOTUwRkEyQzY2N0YxQUZDRDRFODM1RUU1MTNDMjM0RDkwNjMxNjZFOTMxODJFMzU0NkFGRkZEMEYzRDkwMDVFNzgwNEM4MUVBRThBNEU2NTM0OTNCMjFCQjhERkI5QUM5QzhCMjU4RDc0QzU5NkYxQzQ0Q0EzN0QzNzU2MzU3QTE2QTU3NUE3Q0NEQTJFNTg3RjA5QzRGQjJFNzUyMjNDQ0VFMjIyRTAwNEIwOUFBQ0NBNjM3MTQ1REFEODlDQkFDRDFGNzY4QzEzNkU4NzVD"

/* #define LICENSE_TEXT "QTMwMjQzMzJFNEI2M0M2OTRCOEFFQTQ5NzM0MDc0RjQyNjc2RThBRURFMEEzNzBDMkRENzc3OUFGQkY3RjczQUI4RUEwNjY0RDAyRjU4MzNCQkMyMUI1RUE5M0I0NDlGQURFOEQyQzI4NjJGQzIwNTU0QzVEODg5QzA1MTQzN0JERjVBMUUzODAzQUM3NDI5OTgzQUVEQTIzRDUzMkE1QjlDQzQ1OTI3QzQ0RTc0NThBNDQyRDdDOUJBQjY3NkRFQjMzREI4MTJGREE1NzBFMUQ3QkJFOUExQTgxNEZFNjMyNjA1NzlFRkFDQ0UyMEVBOEVCRTc3NzM3RjMzNzk5N0I4QUY3QzAxNDU1REVBOTg0MjBGQUQxOTdFNTRGQkE5NDI3MkE5MUNGRkYwQjg3NjIwQzVBQ0RGMzBGRDE2MzMzRTc1QTJBRkUwQ0Y4OTQ0REI0MEREOURBNkUyODFFRUUwNzgwM0REREM5RTJBMjI5RjY0MkZBN0Q1REVCNDMxMTFBMzc2RDkxQUY4QjlBOEFBNUYwQ0JCQ0NDQzUzMjJFOTUwMjFEQjVDNzRGMUQ5NjFFOTdCMTc2M0NDNDk3ODAzQUVFQ0Y2NDFGM0I2M0I1QzAwMjAxNEE5MjVBQkNBNDNDOUIzRTM3MEM0Mjg1RjhGNzU1RjRGNTg4OUFFNjA1NEIwQTM2QjM5RkFENTZCRkU5RDFBQTc3MzY5QzFENTJBRERGMEZBMDY1Mjk5RTMzMjExQkM4OURBMTcxNjE4" */
#define MODULE_TYPE "rtos_test"

#endif

#define DEVICE_ID "DamB4dtHjfne"

#define MAX_SELECT_RESOURCE 10

extern int vt_httpRequest_cb(str_vt_networkPara *param, str_vt_networkRespond *res);
extern int vt_httpCommonRequest_cb(str_vt_networkPara *param, str_vt_networkRespond *res);
extern void player_state_changed_notify(vt_mediaplayer_state state);
extern const char *get_wt_license_text(void);
extern const char *get_wt_device_id(void);
extern const char *get_wt_module_type(void);

extern const _recog_ops_t _recog_ops_t_ops;
extern const vt_os_ops_t linux_ops_t;  //此为Linux平台的OS功能函数，适配RTOS, rtthread 等需要重新实现一个vt_os_ops_t，这个例子是Linux平台的适配DEMO例子

typedef struct demo_resinfo {
    int current_res_id;
    int  res_id[MAX_SELECT_RESOURCE];
    byte res_languare[MAX_SELECT_RESOURCE];
    byte res_sound_type[MAX_SELECT_RESOURCE];
    byte res_size;
} demo_resinfo_t;

static demo_resinfo_t _resinfo_demo;
static int current_recong_bookid = -1;

static int recongnize_info_cb_(_recongnize_page_info_t *pageinfo, _recongnize_resinfo_t *resinfo, void *recong_userp)
{
    int i = 0;
    printf("recognize bookid: %d pageId: %d, pagetype: %d\n", pageinfo->BookID, pageinfo->PageID, pageinfo->PageType);
    if (current_recong_bookid != pageinfo->BookID) {
        current_recong_bookid = pageinfo->BookID;
        if (resinfo) {
            if (resinfo->res_id && resinfo->res_size > 0) {
                printf("current_res_id: %d size: %d\n", resinfo->current_res_id, resinfo->res_size);
                for (i = 0; i < resinfo->res_size && resinfo->res_size <= MAX_SELECT_RESOURCE; i++) {
                    _resinfo_demo.res_id[i] = resinfo->res_id[i];
                    _resinfo_demo.res_languare[i] = resinfo->res_languare[i];
                    _resinfo_demo.res_sound_type[i] = resinfo->res_sound_type[i];
                }
            }
        }
    }

    return 0;
}

static int recongnize_error_cb(int errorCode, void *userp)
{
    printf("=========errorCode: %d userp: %p\n", errorCode, userp);
    return 0;
}

static void wt_app_task(void *priv)
{
    int ret = 0;
    int msg[32];
    int err;
    u32 cnt = 0;
    u32 wait_cnt = 2;
    const char *device_id;
    const char *module_type;
    const char *license_text;
    struct wt_var *p = (struct wt_var *)priv;
    struct vt_bk_handle *bk_instance = NULL;

    StatedetParamEx stateParam;
    stateParam.width = 640;
    stateParam.height = 480;
    stateParam.channels	= 1;
    stateParam.x = 0;
    stateParam.y = 0;
    stateParam.w = 640;
    stateParam.h = 480;
    stateParam.ds_factor = 8;
    stateParam.in_type = 2;
    stateParam.thresh = 2;
    stateParam.handle_edge = 0;
    stateParam.propose_hand = 0;

    printf("******************wt_app_task******************\r\n");

    device_id = get_wt_device_id();
    module_type = get_wt_module_type();
    license_text = get_wt_license_text();

    if (!device_id || !module_type || !license_text) {
        printf("get wt license error!!!\n");
        return;
    }

    memset((void *)&_resinfo_demo, 0, sizeof(demo_resinfo_t));
    bk_instance = vt_bk_instance();
    if (!bk_instance) {
        return;
    }
    vt_bk_register_os_wrapper(bk_instance, &linux_ops_t);
    vt_bk_env_init(bk_instance, license_text, device_id, module_type);

    while (p->exit_flag == 0) {
        if (!cnt) {
            ret = vt_bk_login(bk_instance, vt_httpRequest_cb);
        }
        if (ret == 0 || p->exit_flag) {
            break;
        }
        if (!cnt) {
            if (wait_cnt < 128) {
                wait_cnt <<= 1;
            }
            cnt = wait_cnt;
        }
        --cnt;
        os_time_dly(50);
    }

    if (p->exit_flag) {
        if (bk_instance) {
            vt_bk_destory(bk_instance);
            bk_instance = NULL;
        }
        return;
    }

    p->connect_status = 1;

    const camera_ops_t *ops_camera = get_linux_camera();
    const media_player_ops_t *ops_player = get_linux_player();
    vt_bk_register_camera_ops(bk_instance, ops_camera);
    vt_bk_register_mediaplayer_ops(bk_instance, ops_player);
    vt_bk_register_common_request(bk_instance, vt_httpCommonRequest_cb);
    vt_bk_register_recognize_ops(bk_instance, &_recog_ops_t_ops, (void *)&stateParam);
    vt_bk_register_reconginfo_cb(bk_instance, recongnize_info_cb_, NULL);
    vt_bk_register_error_cb(bk_instance, recongnize_error_cb, NULL);
    /* vt_bk_start_recognize(bk_instance); */

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case WT_TURNING_START:
            ret = vt_bk_start_recognize(bk_instance);
            if (!ret) {
                printf("vt_bk_start_recognize success\n");
            }
            break;
        case WT_TURNING_STOP:
            ret = vt_bk_stop_recognize(bk_instance);
            if (!ret) {
                printf("vt_bk_start_recognize success\n");
            }
            break;
        case WT_TURNING_PAUSE:
            ret = vt_bk_pause(bk_instance);
            if (!ret) {
                printf("vt_bk_pause success\n");
            }
            break;
        case WT_TURNING_RESUME:
            ret = vt_bk_resume(bk_instance);
            if (!ret) {
                printf("vt_bk_resume success\n");
            }
            break;
        case WT_TURNING_QUIT:
            ret = vt_bk_destory(bk_instance);
            if (!ret) {
                printf("vt_bk_destory success\n");
            }
            bk_instance = NULL;
            p->exit_flag = 0;
            return;
        case WT_TURNING_MEDIA_END:
            printf("=====WT_TURNING_PLAY_END=====\n");
            player_state_changed_notify(E_VTPLAYER_STATE_COMPLETE);
            break;
        case WT_TURNING_PLAY_PAUSE:
            printf("=====WT_TURNING_PLAY_PAUSE=====\n");
            if (msg[2] == 1) { //暂停
                player_state_changed_notify(E_VTPLAYER_STATE_PAUSED);
            } else {
                player_state_changed_notify(E_VTPLAYER_STATE_PLAYING);
            }
            break;
        case WT_TURNING_MEDIA_STOP:
            printf("=====WT_TURNING_PLAY_STOP=====\n");
            player_state_changed_notify(E_VTPLAYER_STATE_STOPED);
            break;
        case WT_TURNING_PLAY_START:
            printf("=====WT_TURNING_PLAY_START=====\n");
            player_state_changed_notify(E_VTPLAYER_STATE_PLAYING);
            break;
#if 0
        case 6:
            if (!bk_instance) {
                bk_instance = vt_bk_instance();
            }
            vt_bk_register_os_wrapper(bk_instance, &linux_ops_t);
            ret = vt_bk_env_init(bk_instance, license_text, device_id, module_type);
            if (!ret) {
                printf("vt_bk_env_init success\n");
            }
            ret = vt_bk_login(bk_instance, vt_httpRequest_cb);
            if (!ret) {
                printf("vt_bk_login success\n");
            }
            ret = vt_bk_register_camera_ops(bk_instance, ops_camera);
            if (!ret) {
                printf("vt_bk_register_camera_ops success\n");
            }
            ret = vt_bk_register_mediaplayer_ops(bk_instance, ops_player);
            if (!ret) {
                printf("vt_bk_register_mediaplayer_ops success\n");
            }
            ret = vt_bk_register_recognize_ops(bk_instance, &_recog_ops_t_ops, &stateParam);
            if (!ret) {
                printf("vt_bk_register_recognize_ops success\n");
            }
            vt_bk_register_reconginfo_cb(bk_instance, recongnize_info_cb_, NULL);
            vt_bk_register_error_cb(bk_instance, recongnize_error_cb, NULL);
            break;
        case 7:
            ret = vt_bk_get_recongstate(bk_instance);
            printf("vt_bk_get_recongstate: %d\n", ret);
            break;
        case 8:
            ret = vt_bk_select_resource(bk_instance, vt_httpCommonPostRequest_cb, _resinfo_demo.res_id[0]);
            break;
#endif
        default:
            break;
        }
    }
}

static int wt_app_init(void)
{
    if (!__this->init_flag) {
        __this->init_flag = 1;
        __this->exit_flag = 0;
        return thread_fork("wt_app_task", 22, 512 * 4, 64, &wt_app_task_pid, wt_app_task, __this);
    }
    return -1;
}

static void wt_app_uninit(void)
{
    if (__this->init_flag) {
        __this->init_flag = 0;
        __this->connect_status = 0;
        __this->exit_flag = 1;
        vt_reconghttpRequest_http_request_exit();

        do {
            if (OS_Q_FULL != os_taskq_post("wt_app_task", 1, WT_TURNING_QUIT)) {
                break;
            }
            log_e("wt_app_task send msg QUIT timeout \n");
            os_time_dly(10);
        } while (1);

        thread_kill(&wt_app_task_pid, KILL_WAIT);
        vt_reconghttpRequest_http_close();
    }
}

REGISTER_AI_SDK(wt_sdk_api) = {
    .name           = "wt",
    .connect        = wt_sdk_open,
    .state_check    = wt_sdk_check,
    .do_event       = wt_sdk_do_event,
    .disconnect     = wt_sdk_disconnect,
};

