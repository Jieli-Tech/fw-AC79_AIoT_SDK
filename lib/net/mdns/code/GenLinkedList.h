#ifndef __GenLinkedList__
#define __GenLinkedList__

#include <stddef.h>


struct	GenLinkedList {
    void		*Head,
                *Tail;
    size_t		LinkOffset;
};
typedef struct GenLinkedList	GenLinkedList;


void		InitLinkedList(GenLinkedList *pList, size_t linkOffset);

void		AddToHead(GenLinkedList *pList, void *elem);
void		AddToTail(GenLinkedList *pList, void *elem);

int		RemoveFromList(GenLinkedList *pList, void *elem);

int		ReplaceElem(GenLinkedList *pList, void *elemInList, void *newElem);



struct	GenDoubleLinkedList {
    void		*Head,
                *Tail;
    size_t		FwdLinkOffset,
                BackLinkOffset;
};
typedef struct GenDoubleLinkedList	GenDoubleLinkedList;


void		InitDoubleLinkedList(GenDoubleLinkedList *pList, size_t fwdLinkOffset,
                                 size_t backLinkOffset);

void		DLLAddToHead(GenDoubleLinkedList *pList, void *elem);

void		DLLRemoveFromList(GenDoubleLinkedList *pList, void *elem);



/* A GenLinkedOffsetList is like a GenLinkedList that stores the *Next field as a signed */
/* offset from the address of the beginning of the element, rather than as a pointer. */

struct	GenLinkedOffsetList {
    size_t		Head,
                Tail;
    size_t		LinkOffset;
};
typedef struct GenLinkedOffsetList	GenLinkedOffsetList;


void		InitLinkedOffsetList(GenLinkedOffsetList *pList, size_t linkOffset);

void		*GetHeadPtr(GenLinkedOffsetList *pList);
void		*GetTailPtr(GenLinkedOffsetList *pList);
void		*GetOffsetLink(GenLinkedOffsetList *pList, void *elem);

void		OffsetAddToHead(GenLinkedOffsetList *pList, void *elem);
void		OffsetAddToTail(GenLinkedOffsetList *pList, void *elem);

int		OffsetRemoveFromList(GenLinkedOffsetList *pList, void *elem);

int		OffsetReplaceElem(GenLinkedOffsetList *pList, void *elemInList, void *newElem);


#endif //	__GenLinkedList__
