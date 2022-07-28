#include "vt_bk.h"
#include "os/os_api.h"


typedef struct rcongnzie_obj {
    pfun_vt_task_cb task_cb;
    void *task_ctx;
} rcongnzie_obj;

static struct rcongnzie_obj rcongnzie_obj_t = {0};

extern int vt_reconghttpRequest_cb(str_vt_networkPara *param, str_vt_networkRespond *res);

static void demo_recongnize_task_cb_(void *args)
{
    int ret = 0;
    rcongnzie_obj *obj = (rcongnzie_obj *)args;

    while (1) {
        /*通知SDK，线程已经running起来了，同时调用SDK的TASK任务接口*/
        ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_RUNNING);
        if (ret < 0) {
            printf("demo_recongnize_task_cb_ request exit\n");
            break;
        }
    }
    /*通知SDK，TASK确实退出了，用于状态确认*/
    ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_EXITED);
    printf("demo_recongnize_task_cb_ exit\n");
}

int recongnize_start_task_cb_demo(pfun_vt_task_cb taskcb, void *ctx)
{
    rcongnzie_obj_t.task_cb = taskcb;
    rcongnzie_obj_t.task_ctx = ctx;
    //此处是Linux平台的task创建函数，适配其他系统需要替换为对应平台的task创建函数
    return thread_fork("wt_recong_app", 21, 256 * 7, 0, NULL, demo_recongnize_task_cb_, (void *)&rcongnzie_obj_t);
}

const _recog_ops_t _recog_ops_t_ops = {vt_reconghttpRequest_cb, recongnize_start_task_cb_demo};

