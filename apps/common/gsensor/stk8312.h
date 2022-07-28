


#ifndef _STK8312_H
#define _STK8312_H



extern s8 stk8312_check(void);
extern void stk8312_init(void);
extern int stk8312_gravity_sensity(u8 gsid);
extern int stk8312_pin_int_interrupt();

#endif
