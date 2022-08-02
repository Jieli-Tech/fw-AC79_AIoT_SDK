#include "os/os_api.h"
#include "generic/list.h"

static int net_timer_task_pid;
static struct list_head timer_head;
static OS_MUTEX	timer_mutex;
static u8 net_timer_task_exit_flag;
static u8 net_timer_init_flag;
static u64 s_timestamp = 0;
static u32 last_timestamp = 0;

extern u32 timer_get_ms(void);

struct timer_list {
    struct list_head entry;

    void (*function)(void *priv);
    void *data;

    u64 expires;
    u32 msec;
    u8 periodic;
    u8 del;
};

static inline u64 net_timer_get_timestamp()
{
    u32 read_timestamp = timer_get_ms();

    if (s_timestamp <= read_timestamp) {
        s_timestamp = read_timestamp;
        last_timestamp = read_timestamp;
    } else {
        if (last_timestamp > read_timestamp) {
            s_timestamp += (~read_timestamp + 1);
        }
        s_timestamp += read_timestamp;
        last_timestamp = read_timestamp;
    }

    return s_timestamp;
}

static void net_timer_task(void *arg)
{
    struct timer_list *curr;
    struct timer_list *p;

    while (1) {

        os_mutex_pend(&timer_mutex, 0);
        while (!list_empty(&timer_head)) {

            curr = list_first_entry(&timer_head, struct timer_list, entry);

            if (curr == NULL || !curr->function) {
                printf("\n net_timer_check error happend -> 0x%x, 0x%x \n", (u32)curr, (u32)(curr->function));
                break;
            }

            if (net_timer_get_timestamp() < curr->expires) {
                break;
            }
            list_del(&curr->entry);

            os_mutex_post(&timer_mutex);

            if (!curr->del) {
                curr->function(curr->data);
            }

            os_mutex_pend(&timer_mutex, 0);

            if (curr->periodic && !curr->del) {
                curr->expires = net_timer_get_timestamp() + curr->msec;
                list_for_each_entry(p, &timer_head, entry) {
                    if (p->expires > curr->expires) {
                        break;
                    }
                }
                __list_add(&curr->entry, p->entry.prev, &p->entry);
            } else {
                free(curr);
            }
        }

#if	0
        list_for_each_entry(p, &timer_head, entry) {
            if (p->del) {
                list_del(&p->entry);
                free(p);
            }
        }
#endif

        os_mutex_post(&timer_mutex);

        if (net_timer_task_exit_flag) {
            os_mutex_pend(&timer_mutex, 0);
            while (!list_empty(&timer_head)) {
                curr = list_first_entry(&timer_head, struct timer_list, entry);
                if (!curr->del) {
                    printf("The timer list should be del before uninit !!!\n");
                }
                list_del(&curr->entry);
                free(curr);
            }
            os_mutex_post(&timer_mutex);
            return;
        }

        msleep(80);
    }
}

int net_timer_init(void)
{
    if (!net_timer_init_flag) {
        INIT_LIST_HEAD(&timer_head);
        net_timer_init_flag = 1;
        os_mutex_create(&timer_mutex);
        return thread_fork("net_timer_task", 22, 256, 0, &net_timer_task_pid, net_timer_task, NULL);
    }

    return 0;
}

void net_timer_uninit(void)
{
    if (net_timer_init_flag) {
        net_timer_task_exit_flag = 1;
        thread_kill(&net_timer_task_pid, KILL_WAIT);
        net_timer_task_exit_flag = 0;
        os_mutex_del(&timer_mutex, 1);
        net_timer_init_flag = 0;
    }
}

int net_timer_add(void *priv, void (*func)(void *priv), u32 msec, u8 periodic)
{
    struct timer_list *t;
    struct timer_list *p;

    t = calloc(1, sizeof(struct timer_list));
    if (!t) {
        return -1;
    }

    t->msec = msec;
    t->data = priv;
    t->function = func;
    t->periodic = periodic;
    t->del = 0;

    os_mutex_pend(&timer_mutex, 0);

    t->expires = net_timer_get_timestamp() + msec;
    list_for_each_entry(p, &timer_head, entry) {
        if (p->expires > t->expires) {
            break;
        }
    }
    __list_add(&t->entry, p->entry.prev, &p->entry);
    os_mutex_post(&timer_mutex);

    return (int)t;
}

void net_timer_del(int __t)
{
    struct timer_list *t = (struct timer_list *)__t;
    struct timer_list *p;

    os_mutex_pend(&timer_mutex, 0);
    list_for_each_entry(p, &timer_head, entry) {
        if (p == t) {
            t->del = 1;
            break;
        }
    }
    os_mutex_post(&timer_mutex);
}
