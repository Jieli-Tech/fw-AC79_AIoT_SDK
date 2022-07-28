#ifndef __TURING_ALARM_H
#define __TURING_ALARM_H

#define MAX_ALARM_NUM	16

int turing_alarm_set(const char *p_date, const char *p_time, const char *p_cyc, const char *second, void *priv);
void turing_alarm_init(void);
void turing_alarm_del_all(void);


#endif/*__TURING_ALARM_H*/

