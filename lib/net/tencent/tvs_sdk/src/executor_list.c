/**
* @file  executor_list.c
* @brief TVS SDK串行化任务队列
* @date  2019-5-10
* 为了节省内存，TVS sdk的各个功能，包括授权、智能语音对话、媒体播控、数据上报等任务被触发的时候，
* 都将进入一个队列中执行。实时性非常高的任务A，将放在队首，中断当前正在执行的任务B，并立刻执行。
* A执行完毕后，B将视其任务类型，重新执行（比如数据上报被中断后将会重新执行，智能语音对话被中断后，
* 无需重新执行）
*/
#include "executor_list.h"
#define TVS_LOG_DEBUG_MODULE  "EXECLIST"
#define TVS_LOG_DEBUG  0
#include "tvs_log.h"

// 新建一个队列
executor_list *executor_list_create()
{
    executor_list *list = (executor_list *)TVS_MALLOC(sizeof(executor_list));
    if (NULL == list) {
        return NULL;
    }
    memset(list, 0, sizeof(executor_list));

    return list;
}

// 新建一个节点
executor_list_node *executor_list_node_create(void *param, int param_size)
{
    executor_list_node *node = (executor_list_node *)TVS_MALLOC(sizeof(executor_list_node));
    if (NULL == node) {
        return NULL;
    }
    memset(node, 0, sizeof(executor_list_node));

    if (param != NULL && param_size > 0) {
        node->param = TVS_MALLOC(param_size);
        if (NULL == node->param) {
            TVS_FREE(node);
            node = NULL;
        } else {
            node->param_size = param_size;
            memcpy(node->param, param, param_size);
        }
    }

    return node;
}

// 增加节点到队首
void executor_list_add_head(executor_list *list, executor_list_node *node)
{
    if (list == NULL || node == NULL) {
        return;
    }
    TVS_LOG_PRINTF("list add head %p\n", node);
    executor_list_node *cur_head = list->head;
    if (cur_head != NULL) {
        node->next = cur_head;
        cur_head->prev = node;
    }

    list->head = node;
    if (list->tail == NULL) {
        list->tail = list->head;
    }
}

// 增加节点到尾部
void executor_list_add_tail(executor_list *list, executor_list_node *node)
{
    if (list == NULL || node == NULL) {
        return;
    }
    TVS_LOG_PRINTF("list add tail %p\n", node);
    executor_list_node *cur_tail = list->tail;
    if (cur_tail != NULL) {
        cur_tail->next = node;
        node->prev = cur_tail;
    }

    list->tail = node;

    if (list->head == NULL) {
        list->head = list->tail;
    }
}

// 获取队首节点
executor_list_node *executor_list_get_head(executor_list *list)
{
    if (list == NULL) {
        return NULL;
    }

    return list->head;
}

// 删除一个节点
void executor_list_delete(executor_list *list, executor_list_node *node)
{
    if (list == NULL || node == NULL) {
        return;
    }

    TVS_LOG_PRINTF("list delete %p\n", node);
    if (node == list->head) {
        list->head = node->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        } else {
            list->tail = NULL;
        }
    } else if (node == list->tail) {
        list->tail = node->prev;
        if (list->tail != NULL) {
            list->tail->next = NULL;
        } else {
            list->head = NULL;
        }
    } else {
        ((executor_list_node *)node->prev)->next = node->next;
        ((executor_list_node *)node->next)->prev = node->prev;
    }

    if (node->param != NULL) {
        TVS_FREE(node->param);
        node->param = NULL;
    }

    if (node != NULL) {
        TVS_FREE(node);
        node = NULL;
    }
}

void executor_list_print(executor_list *list)
{
    executor_list_node *node = list->head;
    TVS_LOG_PRINTF("print from head:\n");
    while (node != NULL) {
        TVS_LOG_PRINTF("%p\n", node);
        node = node->next;
    }

    TVS_LOG_PRINTF("print from tail:\n");
    node = list->tail;
    while (node != NULL) {
        TVS_LOG_PRINTF("%p\n", node);
        node = node->prev;
    }
}