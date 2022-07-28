/**
* @file  executor_service.c
* @brief TVS SDK串行化任务队列
* @date  2019-5-10
* 为了节省内存，TVS sdk的各个功能，包括授权、智能语音对话、媒体播控、数据上报等任务被触发的时候，
* 都将进入一个队列中执行。实时性非常高的任务A，将放在队首，中断当前正在执行的任务B，并立刻执行。
* A执行完毕后，B将视其任务类型，重新执行（比如数据上报被中断后将会重新执行，智能语音对话被中断后，
* 无需重新执行）
*/
#include "executor_service.h"
#define TVS_LOG_DEBUG  0
#define TVS_LOG_DEBUG_MODULE  "EXECSER"
#include "tvs_log.h"
#include "tvs_state.h"
#include "tvs_threads.h"

static executor_list *g_executor_list = 0;

static void *g_waiter_mutex = NULL;

TVS_LOCKER_DEFINE

// wait -- notify
static bool executor_wait(long ms)
{
    return os_wrapper_wait_signal(g_waiter_mutex, ms);
}

// wait -- notify
static void executor_notify()
{
    os_wrapper_post_signal(g_waiter_mutex);
    TVS_LOG_PRINTF("%s\n", __func__);
}

// 判断当前节点的任务是否需要被打断
static bool executor_should_break(executor_node_param *param)
{
    do_lock();
    bool should_break = (param->stop == 1);
    do_unlock();
    return should_break;
}

// 判断当前节点的任务是否需要退出
static bool executor_should_cancel(executor_node_param *param)
{
    do_lock();
    bool cancel = (param->cancel == 1);
    do_unlock();
    return cancel;
}

// 打断当前节点的任务
static void executor_break(executor_list_node *current_node)
{
    if (current_node == NULL || current_node->param == NULL) {
        return;
    }
    TVS_LOG_PRINTF("%s %p\n", __func__, current_node);
    ((executor_node_param *)current_node->param)->stop = 1;
}

// 新增一个节点，打断当前节点的任务，立即开始执行新节点的任务
static void executor_start_immediately(executor_list_node *new_node)
{
    TVS_LOG_PRINTF("%s\n", __func__);
    do_lock();
    executor_list_node *current_node = executor_list_get_head(g_executor_list);
    executor_list_add_head(g_executor_list, new_node);
    // 打断当前正在进行的任务
    executor_break(current_node);
    do_unlock();
    TVS_LOG_PRINTF("%s end\n", __func__);
    executor_notify();
}

// 把一个节点加到队尾，后续会执行其任务
static void executor_start_tail(executor_list_node *new_node)
{
    do_lock();
    executor_list_add_tail(g_executor_list, new_node);
    do_unlock();

    executor_notify();
}

static void executor_start_head(executor_list_node *new_node)
{
    do_lock();
    executor_list_add_head(g_executor_list, new_node);
    do_unlock();

    executor_notify();
}

static void executor_thread(void *param)
{
    while (1) {
        do_lock();
        executor_list_node *node = executor_list_get_head(g_executor_list);

        if (node == 0) {
            // 队列中无任务，等待
            do_unlock();
            TVS_LOG_PRINTF("executor is empty, wait\n");
            executor_wait(3000 * 1000);
            TVS_LOG_PRINTF("executor wait end\n");
            continue;
        }

        // 用于判断被打断的任务是否需要重新执行
        bool shoule_retry = false;
        executor_node_param *param = NULL;
        if (node->param == NULL) {
            shoule_retry = false;
            do_unlock();
        } else {
            param = (executor_node_param *)(node->param);
            if (param->cancel == 1) {
                // 被cancel的任务不需要重试
                shoule_retry = false;
                TVS_LOG_PRINTF("executor cancel cmd %d, node %p\n", param->cmd, node);
                do_unlock();

            } else {
                param->stop = 0;
                do_unlock();

                TVS_LOG_PRINTF("executor run %p\n", node);
                // 执行节点的任务
                shoule_retry = param->runnable(param);
            }
        }

        if (!shoule_retry) {
            // 如果不需要重试，删除节点，释放资源
            do_lock();
            TVS_LOG_PRINTF("executor release %p\n", node);
            if (param != NULL && param->release != NULL) {
                param->release(param);
            }

            if (param != NULL && param->runnable_param != NULL) {
                //TVS_LOG_PRINTF("executor free param\n");
                TVS_FREE(param->runnable_param);
                //TVS_LOG_PRINTF("executor free param - end\n");
            }
            executor_list_delete(g_executor_list, node);
            do_unlock();
        }
    }
}

void executor_service_init()
{
    if (g_executor_list) {
        return;
    }
    g_executor_list = executor_list_create();

    TVS_LOCKER_INIT

    // 创建一个初始值为0的二值信号量用于等待队列中有任务
    g_waiter_mutex = os_wrapper_create_signal_mutex(0);

    os_wrapper_start_thread(executor_thread, NULL, "executor", 3, 2048);
}


// 新建节点
static executor_list_node *create_node(int cmd, executor_runnable_function runnable,
                                       executor_release_function release, void *runnable_param, int runnable_param_size)
{
    executor_node_param param;
    memset(&param, 0, sizeof(executor_node_param));
    param.cmd = cmd;
    param.should_break = executor_should_break;
    param.should_cancel = executor_should_cancel;
    param.runnable = runnable;

    param.runnable_param_size = runnable_param_size;
    if (runnable_param_size > 0) {
        param.runnable_param = TVS_MALLOC(runnable_param_size);
        memcpy(param.runnable_param, runnable_param, runnable_param_size);
    }
    param.release = release;
    executor_list_node *node = executor_list_node_create(&param, sizeof(param));
    return node;
}

// cancel当前队列中指定cmd的所有节点
void executor_service_cancel_cmds(int cmd)
{
    TVS_LOG_PRINTF("cancel cmds start\n");
    do_lock();
    executor_list_node *node = g_executor_list->head;
    executor_node_param *param = NULL;
    while (node != NULL) {
        if (node->param != NULL) {
            param = (executor_node_param *)(node->param);
            if (param->cmd == cmd) {
                TVS_LOG_PRINTF("cancel node %p, cmd %d\n", node, cmd);
                param->cancel = 1;
            }
        }
        node = node->next;
    }

    do_unlock();
    TVS_LOG_PRINTF("cancel cmds end\n");
}

int get_current_node(void)
{
    do_lock();
    executor_list_node *current_node = executor_list_get_head(g_executor_list);
    do_unlock();
    if (current_node) {
        return 1;
    }
    return 0;
}

// 用于cancel当前队列中，所有cmd值大于cms_activities_start的节点
void executor_service_stop_all_activities(int cms_activities_start)
{
    TVS_LOG_PRINTF("%s\n", __func__);
    do_lock();
    executor_list_node *current_node = executor_list_get_head(g_executor_list);
    if (current_node != NULL && current_node->param != NULL) {
        executor_node_param *param = (executor_node_param *)(current_node->param);
        int cmd = param->cmd;
        if (cmd >= cms_activities_start) {
            TVS_LOG_PRINTF("cancel node %p, cmd %d\n", current_node, cmd);
            param->cancel = 1;
        }
    }
    do_unlock();
    TVS_LOG_PRINTF("%s end\n", __func__);
}

void executor_service_start_immediately(int cmd, executor_runnable_function runnable,
                                        executor_release_function release, void *runnable_param, int runnable_param_size)
{
    executor_start_immediately(create_node(cmd, runnable, release, runnable_param, runnable_param_size));
}

void executor_service_start_tail(int cmd, executor_runnable_function runnable,
                                 executor_release_function release, void *runnable_param, int runnable_param_size)
{
    executor_start_tail(create_node(cmd, runnable, release, runnable_param, runnable_param_size));
}

void executor_service_start_head(int cmd, executor_runnable_function runnable,
                                 executor_release_function release, void *runnable_param, int runnable_param_size)
{
    executor_start_head(create_node(cmd, runnable, release, runnable_param, runnable_param_size));
}
