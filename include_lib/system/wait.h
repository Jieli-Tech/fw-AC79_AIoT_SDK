#ifndef WAIT_COMPLETION_H
#define WAIT_COMPLETION_H

/**
 * @brief 条件轮询回调,回调函数将通过队列消息由指定线程接收执行
 * @param task_name，	   指定执行回调函数的任务名
 * @param condition:	   条件判断函数指针
 * @param callback: 	   回调函数指针
 * @param priv，    	   传到回调函数参数指针
 * @param condition_priv:  传到判断条件函数参数指针
 * @note  指定的线程一定要接收任务队列消息才会执行回调函数
 * @return id:创建成功的id号 0:失败
 */
u16 wait_completion_add_to_task(const char *task_name, int (*condition)(void *), int (*callback)(void *), void *priv, void *condition_priv);

/**
 * @brief 条件轮询回调,回调函数将通过队列消息由当前线程接收执行
 * @param condition:	   条件判断函数指针
 * @param callback: 	   回调函数指针
 * @param priv: 	   	   传到回调函数参数指针
 * @param condition_priv:  传到判断条件函数参数指针
 * @note  调用此函数的线程一定要接收任务队列消息才会执行回调函数
 * @return id:创建成功的id号 0:失败
 */
u16 wait_completion(int (*condition)(void *), int (*callback)(void *), void *priv, void *condition_priv);

/**
 * @brief 删除条件轮询回调函数
 * @param id: 创建的时候对应返回的ID号
 * @return 0:成功  非0:失败
 */
int wait_completion_del(u16 id);

#endif
