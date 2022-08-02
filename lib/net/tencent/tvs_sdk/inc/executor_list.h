#ifndef __EXECUTOR_LIST_H__
#define __EXECUTOR_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *next;
    void *prev;
    void *param;
    int param_size;
} executor_list_node;

typedef struct {
    executor_list_node *head;
    executor_list_node *tail;
} executor_list;

executor_list *executor_list_create();

executor_list_node *executor_list_node_create(void *param, int param_size);

void executor_list_add_head(executor_list *list, executor_list_node *node);

void executor_list_add_tail(executor_list *list, executor_list_node *node);

executor_list_node *executor_list_get_head(executor_list *list);

void executor_list_delete(executor_list *list, executor_list_node *node);

void executor_list_print(executor_list *list);

#ifdef __cplusplus
}
#endif

#endif
