/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-04-30     Bernard      first implementation
 * 2006-05-04     Bernard      add list_thread,
 *                                 list_sem,
 *                                 list_timer
 * 2006-05-20     Bernard      add list_mutex,
 *                                 list_mailbox,
 *                                 list_msgqueue,
 *                                 list_event,
 *                                 list_fevent,
 *                                 list_mempool
 * 2006-06-03     Bernard      display stack information in list_thread
 * 2006-08-10     Bernard      change version to invoke rt_show_version
 * 2008-09-10     Bernard      update the list function for finsh syscall
 *                                 list and sysvar list
 * 2009-05-30     Bernard      add list_device
 * 2010-04-21     yi.qiu       add list_module
 * 2012-04-29     goprife      improve the command line auto-complete feature.
 * 2012-06-02     lgnq         add list_memheap
 * 2012-10-22     Bernard      add MS VC++ patch.
 * 2016-06-02     armink       beautify the list_thread command
 * 2018-11-22     Jesven       list_thread add smp support
 * 2018-12-27     Jesven       Fix the problem that disable interrupt too long in list_thread
 *                             Provide protection for the "first layer of objects" when list_*
 * 2020-04-07     chenhui      add clear
 */

//#include <rthw.h>
//#include <rtthread.h>

#ifdef RT_USING_FINSH
#include "event.h"
#include "event/key_event.h"
#include "finsh.h"

#define LIST_FIND_OBJ_NR 8

static long clear(void)
{
    rt_kprintf("\x1b[2J\x1b[H");

    return 0;
}
FINSH_FUNCTION_EXPORT(clear, clear the terminal screen);
MSH_CMD_EXPORT(clear, clear the terminal screen);


long list_version(void)
{
    extern const char *sdk_version(void);

    rt_kprintf("\n \\ | /\n");
    rt_kprintf("- AC79 \n");
    rt_kprintf(" / | \\     %s build %s\n", sdk_version(), __DATE__);
    rt_kprintf(" 2010 - %s Copyright by jieli team\n", __DATE__);

    return 0;
}
FINSH_FUNCTION_EXPORT(list_version, show SDK version information);
MSH_CMD_EXPORT(list_version, show SDK version information);

rt_inline void object_split(int len)
{
    while (len--) {
        rt_kprintf("-");
    }
}

long list_thread(void)
{
    extern void get_task_state(void *parm);
    get_task_state(NULL); //1分钟以内调用一次才准确
    return 0;
}
FINSH_FUNCTION_EXPORT(list_thread, list thread);
MSH_CMD_EXPORT(list_thread, list thread);

long key_up(void)
{
    struct key_event key = {0};
    key.value = KEY_UP;
    key.action = KEY_EVENT_CLICK;
    key.type = KEY_EVENT_USER;
    key_event_notify(KEY_EVENT_FROM_USER, &key);
    return 0;
}
FINSH_FUNCTION_EXPORT(key_up, send key_up event);
MSH_CMD_EXPORT(key_up, send key_up event);

long key_down(void)
{
    struct key_event key = {0};
    key.value = KEY_DOWN;
    key.action = KEY_EVENT_CLICK;
    key.type = KEY_EVENT_USER;
    key_event_notify(KEY_EVENT_FROM_USER, &key);
    return 0;
}
FINSH_FUNCTION_EXPORT(key_down, send key_down event);
MSH_CMD_EXPORT(key_down, send key_down event);

long key_ok(void)
{
    struct key_event key = {0};
    key.value = KEY_OK;
    key.action = KEY_EVENT_CLICK;
    key.type = KEY_EVENT_USER;
    key_event_notify(KEY_EVENT_FROM_USER, &key);
    return 0;
}
FINSH_FUNCTION_EXPORT(key_ok, send key_ok event);
MSH_CMD_EXPORT(key_ok, send key_ok event);


long key_menu(void)
{
    struct key_event key = {0};
    key.value = KEY_MENU;
    key.action = KEY_EVENT_CLICK;
    key.type = KEY_EVENT_USER;
    key_event_notify(KEY_EVENT_FROM_USER, &key);
    return 0;
}
FINSH_FUNCTION_EXPORT(key_menu, send key_menu event);
MSH_CMD_EXPORT(key_menu, key_menu);

long key_right(void)
{
    struct key_event key = {0};
    key.value = KEY_RIGHT;
    key.action = KEY_EVENT_CLICK;
    key.type = KEY_EVENT_USER;
    key_event_notify(KEY_EVENT_FROM_USER, &key);
    return 0;
}
FINSH_FUNCTION_EXPORT(key_right, send key_right event);
MSH_CMD_EXPORT(key_right, send key_right event);

long key_left(void)
{
    struct key_event key = {0};
    key.value = KEY_LEFT;
    key.action = KEY_EVENT_CLICK;
    key.type = KEY_EVENT_USER;
    key_event_notify(KEY_EVENT_FROM_USER, &key);
    return 0;
}
FINSH_FUNCTION_EXPORT(key_left, send key_left event);
MSH_CMD_EXPORT(key_left, send key_left event);


long list_memheap(void)
{
    malloc_stats();
    malloc_dump();

    return 0;
}
FINSH_FUNCTION_EXPORT(list_memheap, list memory heap in system);
MSH_CMD_EXPORT(list_memheap, list memory heap in system);

long list(void)
{
#ifndef FINSH_USING_MSH_ONLY
    struct finsh_syscall_item *syscall_item;
    struct finsh_sysvar_item *sysvar_item;
#endif

    rt_kprintf("--Function List:\n");
    {
        struct finsh_syscall *index;
        for (index = _syscall_table_begin;
             index < _syscall_table_end;
             FINSH_NEXT_SYSCALL(index)) {
            /* skip the internal command */
            if (strncmp((char *)index->name, "__", 2) == 0) {
                continue;
            }

#if defined(FINSH_USING_DESCRIPTION) && defined(FINSH_USING_SYMTAB)
            rt_kprintf("%-16s -- %s\n", index->name, index->desc);
#else
            rt_kprintf("%s\n", index->name);
#endif
        }
    }

#ifndef FINSH_USING_MSH_ONLY
    /* list syscall list */
    syscall_item = global_syscall_list;
    while (syscall_item != NULL) {
        rt_kprintf("[l] %s\n", syscall_item->syscall.name);
        syscall_item = syscall_item->next;
    }

    rt_kprintf("--Variable List:\n");
    {
        struct finsh_sysvar *index;
        for (index = _sysvar_table_begin;
             index < _sysvar_table_end;
             FINSH_NEXT_SYSVAR(index)) {
#ifdef FINSH_USING_DESCRIPTION
            rt_kprintf("%-16s -- %s\n", index->name, index->desc);
#else
            rt_kprintf("%s\n", index->name);
#endif
        }
    }

    sysvar_item = global_sysvar_list;
    while (sysvar_item != NULL) {
        rt_kprintf("[l] %s\n", sysvar_item->sysvar.name);
        sysvar_item = sysvar_item->next;
    }
#endif

    return 0;
}
FINSH_FUNCTION_EXPORT(list, list all symbol in system)

#ifndef FINSH_USING_MSH_ONLY
static int str_is_prefix(const char *prefix, const char *str)
{
    while ((*prefix) && (*prefix == *str)) {
        prefix ++;
        str ++;
    }

    if (*prefix == 0) {
        return 0;
    }

    return -1;
}

static int str_common(const char *str1, const char *str2)
{
    const char *str = str1;

    while ((*str != 0) && (*str2 != 0) && (*str == *str2)) {
        str ++;
        str2 ++;
    }

    return (str - str1);
}

void list_prefix(char *prefix)
{
    struct finsh_syscall_item *syscall_item;
    struct finsh_sysvar_item *sysvar_item;
    rt_uint16_t func_cnt, var_cnt;
    int length, min_length;
    const char *name_ptr;

    func_cnt = 0;
    var_cnt  = 0;
    min_length = 0;
    name_ptr = RT_NULL;

    /* checks in system function call */
    {
        struct finsh_syscall *index;
        for (index = _syscall_table_begin;
             index < _syscall_table_end;
             FINSH_NEXT_SYSCALL(index)) {
            /* skip internal command */
            if (str_is_prefix("__", index->name) == 0) {
                continue;
            }

            if (str_is_prefix(prefix, index->name) == 0) {
                if (func_cnt == 0) {
                    rt_kprintf("--function:\n");

                    if (*prefix != 0) {
                        /* set name_ptr */
                        name_ptr = index->name;

                        /* set initial length */
                        min_length = strlen(name_ptr);
                    }
                }

                func_cnt ++;

                if (*prefix != 0) {
                    length = str_common(name_ptr, index->name);
                    if (length < min_length) {
                        min_length = length;
                    }
                }

#ifdef FINSH_USING_DESCRIPTION
                rt_kprintf("%-16s -- %s\n", index->name, index->desc);
#else
                rt_kprintf("%s\n", index->name);
#endif
            }
        }
    }

    /* checks in dynamic system function call */
    syscall_item = global_syscall_list;
    while (syscall_item != NULL) {
        if (str_is_prefix(prefix, syscall_item->syscall.name) == 0) {
            if (func_cnt == 0) {
                rt_kprintf("--function:\n");
                if (*prefix != 0 && name_ptr == NULL) {
                    /* set name_ptr */
                    name_ptr = syscall_item->syscall.name;

                    /* set initial length */
                    min_length = strlen(name_ptr);
                }
            }

            func_cnt ++;

            if (*prefix != 0) {
                length = str_common(name_ptr, syscall_item->syscall.name);
                if (length < min_length) {
                    min_length = length;
                }
            }

            rt_kprintf("[l] %s\n", syscall_item->syscall.name);
        }
        syscall_item = syscall_item->next;
    }

    /* checks in system variable */
    {
        struct finsh_sysvar *index;
        for (index = _sysvar_table_begin;
             index < _sysvar_table_end;
             FINSH_NEXT_SYSVAR(index)) {
            if (str_is_prefix(prefix, index->name) == 0) {
                if (var_cnt == 0) {
                    rt_kprintf("--variable:\n");

                    if (*prefix != 0 && name_ptr == NULL) {
                        /* set name_ptr */
                        name_ptr = index->name;

                        /* set initial length */
                        min_length = strlen(name_ptr);

                    }
                }

                var_cnt ++;

                if (*prefix != 0) {
                    length = str_common(name_ptr, index->name);
                    if (length < min_length) {
                        min_length = length;
                    }
                }

#ifdef FINSH_USING_DESCRIPTION
                rt_kprintf("%-16s -- %s\n", index->name, index->desc);
#else
                rt_kprintf("%s\n", index->name);
#endif
            }
        }
    }

    /* checks in dynamic system variable */
    sysvar_item = global_sysvar_list;
    while (sysvar_item != NULL) {
        if (str_is_prefix(prefix, sysvar_item->sysvar.name) == 0) {
            if (var_cnt == 0) {
                rt_kprintf("--variable:\n");
                if (*prefix != 0 && name_ptr == NULL) {
                    /* set name_ptr */
                    name_ptr = sysvar_item->sysvar.name;

                    /* set initial length */
                    min_length = strlen(name_ptr);
                }
            }

            var_cnt ++;

            if (*prefix != 0) {
                length = str_common(name_ptr, sysvar_item->sysvar.name);
                if (length < min_length) {
                    min_length = length;
                }
            }

            rt_kprintf("[v] %s\n", sysvar_item->sysvar.name);
        }
        sysvar_item = sysvar_item->next;
    }

    /* only one matched */
    if (name_ptr != NULL) {
        rt_strncpy(prefix, name_ptr, min_length);
    }
}
#endif

#if defined(FINSH_USING_SYMTAB) && !defined(FINSH_USING_MSH_ONLY)
static int dummy = 0;
FINSH_VAR_EXPORT(dummy, finsh_type_int, dummy variable for finsh)
#endif

#endif /* RT_USING_FINSH */

