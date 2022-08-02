#ifndef ADAPTER_LIST_H
#define ADAPTER_LIST_H

#include "generic/typedef.h"
#include "generic/list.h"

/**
 * list_is_init - tests whether a list is already init
 * @head: the list to test.
 */
_INLINE_
static inline int list_need_init(const struct list_head *head)
{
    return head->next == NULL;
}

#endif
