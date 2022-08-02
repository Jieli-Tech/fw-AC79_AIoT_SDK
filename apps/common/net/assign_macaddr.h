#ifndef __ASSIGN_MACADDR_H__
#define __ASSIGN_MACADDR_H__

extern char is_server_assign_macaddr_ok(void);
extern int server_assign_macaddr(void (*callback)(void));
extern void cancel_server_assign_macaddr(void);

#endif // __ASSIGN_MACADDR_H__
