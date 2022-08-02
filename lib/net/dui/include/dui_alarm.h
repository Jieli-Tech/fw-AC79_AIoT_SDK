#ifndef _DUI_ALARM_H_
#define _DUI_ALARM_H_


typedef enum {
    ALARM_CLOCK_TYPE,
    SCHEDULE_TYPE,
} ALARM_OBJECT_TYPE;

extern int dui_alarm_init(void);
extern int dui_alarm_sync(const char *extra);
extern int dui_alarm_query(void);

#endif
