#ifndef __OS_TEST_H__
#define __OS_TEST_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "os/os_api.h"
#include "system/task.h"
#include "system/init.h"

//互斥量测试
void mutex_test(void);

//消息队列测试
void queue_test(void);

//信号量测试
void sem_test(void);

//静态任务创建测试
void static_task_test(void);

//模拟线程无法杀死测试
void thread_can_not_kill_test(void);

#ifdef __cplusplus
}
#endif

#endif
