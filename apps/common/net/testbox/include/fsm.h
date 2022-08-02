#ifndef _FSM_H_
#define _FSM_H_

#include <stdint.h>
#include <stddef.h>


enum {
    FSM_NEXT_STATE,
    FSM_KEEP_STATE,
    FSM_ERR,
};


typedef struct FsmTable_s {
    u8 event;                    /* 触发事件 */
    u8 CurState;                 /* 当前状态 */
    s8(*eventActFun)(void *);    /* 动作函数 */
    u8 NextState;                /* 跳转状态 */
    s32 timeout;
} FsmTable_T;

typedef struct FSM_s {
    FsmTable_T *FsmTable;        /* 状态迁移表 */
    u8 curState;                 /* 状态机当前状态 */
    u8 stuMaxNum;                /* 状态机状态迁移数量 */
    void (*errorFun)(void *);
} FSM_T;


void FSM_StateTransfer(FSM_T *pFsm, u8 state);
void FSM_Init(FSM_T *pFsm, FsmTable_T *pTable, u8 stuMaxNum, u8 curState, void (*errorFun)(void *));
void FSM_EventHandle(FSM_T *pFsm, u8 event, void *parm);

#endif
