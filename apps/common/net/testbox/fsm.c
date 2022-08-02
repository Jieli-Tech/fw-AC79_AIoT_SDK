#include "system/includes.h"
#include "client.h"

#ifdef CONFIG_WIFIBOX_ENABLE

#ifdef FSM_DEBUG_ENABLE
#define     log_info(x, ...)     printf("[WB_FSM][INFO] " x " ", ## __VA_ARGS__)
#define     log_err(x, ...)      printf("[WB_FSM][ERR] " x " ", ## __VA_ARGS__)
#else
#define     log_info(...)
#define     log_err(...)
#endif


void FSM_StateTransfer(FSM_T *pFsm, u8 state)
{
    pFsm->curState = state;
}

void FSM_EventHandle(FSM_T *pFsm, u8 event, void *parm)
{
    FsmTable_T *pAcTable = pFsm->FsmTable;
    s8(*eventActFun)(void *) = NULL;
    u8 NextState;
    u8 CurState = pFsm->curState;
    u8 flag = 0;
    s32 ret = -1;
    static s32 timer = -1, reason = 0;
    s32 timeout = 0;

    for (u8 i = 0; i < pFsm->stuMaxNum; i++) {
        if (event == pAcTable[i].event && CurState == pAcTable[i].CurState) {
            flag = 1;
            timeout = pAcTable[i].timeout;
            eventActFun = pAcTable[i].eventActFun;
            NextState = pAcTable[i].NextState;
            break;
        }
    }

    if (flag) {
        log_info("CurState = %d", CurState);
        if (CurState == pAcTable[0].CurState) {
            reason = -1;
            timer = sys_timeout_add(&reason, pFsm->errorFun, timeout);
            log_info("add : timer_id = %d, timeout = %d.\n", timer, timeout);
        } else {
            if (timer) {
                sys_timer_modify(timer, timeout);
                log_info("modify : timer_id = %d, timeout = %d.\n", timer, timeout);
            }
        }

        if (eventActFun != NULL) {
            ret = eventActFun(parm);
        }

        if (timer && CurState == pAcTable[pFsm->stuMaxNum - 1].CurState && ret != FSM_KEEP_STATE) {
            sys_timeout_del(timer);
            timer = -1;
            log_info("del : timer_id = %d.\n", timer);
        }

        switch (ret) {
        case FSM_NEXT_STATE :
            log_info("FSM_NEXT_STATE");
            FSM_StateTransfer(pFsm, NextState);
            break;

        case FSM_KEEP_STATE :
            log_info("FSM_KEEP_STATE");
            break;

        case FSM_ERR :
            log_info("FSM_ERR");
            reason = -2;
            pFsm->errorFun(&reason);
            if (timer) {
                sys_timeout_del(timer);
                log_info("del : timer_id = %d.\n", timer);
                timer = -1;
            }
            break;

        default :
            break;
        }
    } else {
        // do nothing
        log_err("no match status.");
    }
}


void FSM_Init(FSM_T *pFsm, FsmTable_T *pTable, u8 stuMaxNum, u8 curState, void (*errorFun)(void *))
{
    pFsm->FsmTable = pTable;
    pFsm->curState = curState;
    pFsm->stuMaxNum = stuMaxNum;
    pFsm->errorFun = errorFun;
}

#endif

