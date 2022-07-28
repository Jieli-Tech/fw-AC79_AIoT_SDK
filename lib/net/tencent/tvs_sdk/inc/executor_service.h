#ifndef __EXECUTOR_SERVICE_H_8JNWAK__
#define __EXECUTOR_SERVICE_H_8JNWAK__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "os_wrapper.h"
#include "executor_list.h"

typedef struct _executor_node_param executor_node_param;

typedef bool(*executor_callback_should_break)(executor_node_param *param);

typedef bool(*executor_callback_should_cancel)(executor_node_param *param);

typedef bool(*executor_runnable_function)(executor_node_param *node_param);

typedef bool(*executor_release_function)(executor_node_param *node_param);

struct _executor_node_param {
    int stop;
    int cancel;
    int cmd;
    executor_runnable_function runnable;
    executor_callback_should_break should_break;
    executor_callback_should_cancel should_cancel;
    executor_release_function release;
    void *runnable_param;
    int runnable_param_size;
};

void executor_service_init();

void executor_service_start_immediately(int cmd, executor_runnable_function runnable,
                                        executor_release_function release, void *runnable_param, int runnable_param_size);

void executor_service_start_tail(int cmd, executor_runnable_function runnable,
                                 executor_release_function release, void *runnable_param, int runnable_param_size);

void executor_service_start_head(int cmd, executor_runnable_function runnable,
                                 executor_release_function release, void *runnable_param, int runnable_param_size);

void executor_service_cancel_cmds(int cmd);

int get_current_node(void);
void executor_service_stop_all_activities(int cms_activities_start);

#ifdef __cplusplus
}
#endif
#endif
