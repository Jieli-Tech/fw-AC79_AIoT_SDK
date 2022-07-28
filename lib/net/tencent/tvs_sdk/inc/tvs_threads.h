#ifndef __TVS_THREADS_TOOLS_FAWE3FFW332SDCS__
#define __TVS_THREADS_TOOLS_FAWE3FFW332SDCS__


bool tvs_do_lock(const char *file, int line, const char *last_file, int last_line, void *mutex);


#define do_lock() do_lock_ex(__FILE__, __LINE__)


#define TVS_LOCKER_DEFINE  \
	static void* g_lock_mutex; \
	static char* g_last_lock_file; \
	static int g_last_lock_line; \
	\
	static bool do_lock_ex(const char* file, int line) { \
		bool ret = tvs_do_lock(file, line, g_last_lock_file, g_last_lock_line, g_lock_mutex); \
		g_last_lock_file = (char*)file; \
		g_last_lock_line = line; \
		return ret; \
	} \
	\
	static void do_unlock() { \
		os_wrapper_unlock_mutex(g_lock_mutex); \
	} \


#define TVS_LOCKER_INIT  \
	if (g_lock_mutex == NULL) { \
		g_lock_mutex = os_wrapper_create_locker_mutex(); \
	}

typedef struct tvs_thread_handle tvs_thread_handle_t;

typedef void(*thread_func)(tvs_thread_handle_t *thread_handle_t);

typedef void(*thread_end_func)(tvs_thread_handle_t *thread_handle_t);

tvs_thread_handle_t *tvs_thread_new(thread_func func, thread_end_func end_func);

void *tvs_thread_get_task_handle(tvs_thread_handle_t *thread);

void tvs_thread_start_prepare(tvs_thread_handle_t *thread, void *param, int param_size);

void tvs_thread_start_now(tvs_thread_handle_t *thread, const char *name, int prior, int stack_depth);

bool tvs_thread_is_stop(tvs_thread_handle_t *thread);

void tvs_thread_try_stop(tvs_thread_handle_t *thread);

bool tvs_thread_can_loop(tvs_thread_handle_t *thread);

void *tvs_thread_get_param(tvs_thread_handle_t *thread);

bool tvs_thread_join(tvs_thread_handle_t *thread, int block_ms);

void tvs_thread_release(tvs_thread_handle_t **p_handle);


#endif
