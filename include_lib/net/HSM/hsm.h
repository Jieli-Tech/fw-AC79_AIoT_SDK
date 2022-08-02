#ifndef __HSM_H__
#define __HSM_H__


//#include "linux/stdio.h"
#include "printf.h"
#ifdef __cplusplus
extern "C" {
#endif

//----HSM OPTIONAL FEATURES SECTION[BEGIN]----
// Enable for HSM debugging
// #define HSM_DEBUG_ENABLE
// Enable safety checks.  Can be disabled after validating all states
// #define HSM_CHECK_ENABLE
//----HSM OPTIONAL FEATURES SECTION[END]----

// Set the maximum nested levels
#define HSM_MAX_DEPTH 4

//----State definitions----
#define HSME_INIT   (-3)
#define HSME_ENTRY  (-2)
#define HSME_EXIT   (-1)
#define HSME_NULL   0
#define HSME_START  1

//----Debug Macros----
#ifdef HSM_DEBUG_ENABLE
// Use this macro to Enable/Disable HSM debugging for that object / define custom "event to string" function / changing the prefix for that object
#define HSM_SET_DEBUG(hsm, bEnable, preFix, evt2str_func) { (hsm)->hsmDebug = (bEnable);(hsm)->prefix =   (preFix);(hsm)->evt2str =   (evt2str_func);}
// Use this macro to get the current HSM debugging state for that object
#define HSM_GET_DEBUG(hsm) ((hsm)->hsmDebug)

// Using printf for DEBUG
#define HSM_DEBUG(x, ...) { if (This->hsmDebug) printf("%s" x "\r\n", This->prefix, __VA_ARGS__); }
#else
#define HSM_SET_DEBUG(hsm, bEnable, preFix, evt2str_func)
#define HSM_GET_DEBUG(hsm)
#define HSM_DEBUG(...)
#endif // HSM_DEBUG_ENABLE

#define HSM_ERROR(x, ...) { printf("%s" x "\r\n", This->prefix, __VA_ARGS__); }

//----Structure declaration----
typedef struct HSM_STATE_T HSM_STATE;
typedef struct HSM_T HSM;

typedef int (* HSM_FN)(HSM *This, int event, void *param);

struct HSM_STATE_T {
    HSM_STATE *parent;          // parent state
    HSM_FN handler;             // associated event handler for state
    const char *name;           // name of state
    unsigned char level;              // depth level of the state
};

struct HSM_T {
    HSM_STATE *curState;        // Current HSM State
#ifdef HSM_DEBUG_ENABLE
    const char *name;           // Name of HSM Machine
    const char *prefix;         // Prefix for debugging (e.g. grep)
    unsigned char hsmDebug;           // HSM run-time debug flag
    const char *(*evt2str)(int event);                 //"event to string" function
#endif // HSM_DEBUG_ENABLE
#ifdef HSM_CHECK_ENABLE
    unsigned char hsmTran;            // HSM Transition Flag
#endif // HSM_CHECK_ENABLE
};

//----Function Declarations----
// Func: void HSM_STATE_Create(HSM_STATE *This, const char *name, HSM_FN handler, HSM_STATE *parent)
// Desc: Create an HSM State for the HSM.
// This: Pointer to HSM_STATE object
// name: Name of state (for debugging)
// handler: State Event Handler
// parent: Parent state.  If NULL, then internal ROOT handler is used as catch-all
void HSM_STATE_Create(HSM_STATE *This, const char *name, HSM_FN handler, HSM_STATE *parent);

// Func: void HSM_Create(HSM *This, const char *name, HSM_STATE *initState)
// Desc: Create the HSM instance.  Required for each instance
// name: Name of state machine (for debugging)
// initState: Initial state of statemachine
void HSM_Create(HSM *This, const char *name, HSM_STATE *initState);

// Func: HSM_STATE *HSM_GetState(HSM *This)
// Desc: Get the current HSM STATE
// This: Pointer to HSM instance
// return|HSM_STATE *: Pointer to HSM STATE
HSM_STATE *HSM_GetState(HSM *This);

// Func: unsigned char HSM_IsInState(HSM *This, HSM_STATE *state)
// Desc: Tests whether HSM is in state or parent state
// This: Pointer to HSM instance
// state: Pointer to HSM_STATE to test
// return|unsigned char: 1 - HSM instance is in state or parent state, 0 - otherwise
unsigned char HSM_IsInState(HSM *This, HSM_STATE *state);

// Func: void HSM_Run(HSM *This, int event, void *param)
// Desc: Run the HSM with event
// This: Pointer to HSM instance
// event: int processed by HSM
// param: Parameter associated with int
void HSM_Run(HSM *This, int event, void *param);

// Func: void HSM_Tran(HSM *This, HSM_STATE *nextState, int event, void *param, void (*method)(HSM *This, void *param))
// Desc: Transition to another HSM STATE
// This: Pointer to HSM instance
// nextState: Pointer to next HSM STATE
// event : int processed by next HSM STATE
// param: Optional Parameter associated with HSME_ENTRY and HSME_EXIT event
// method: Optional function hook between the HSME_ENTRY and HSME_EXIT event handling
void HSM_Tran(HSM *This, HSM_STATE *nextState, int event, void *param, void (*method)(HSM *This, void *param));




/**
 * ------------------------------------------------ FSM --------------------------------------------------------
 */
typedef unsigned char fsm_ret_t;
typedef struct fsm_s fsm_t;
typedef struct fsm_event_s {
    int sig;
    void *event;
} fsm_event_t;
typedef fsm_ret_t (*fsm_state_handler_t)(fsm_t *fsm, fsm_event_t const *e);
struct fsm_s {
    fsm_state_handler_t state;
    fsm_state_handler_t temp;
};
enum {
    FSM_RET_HANDLED,
    FSM_RET_IGNORE,
    FSM_RET_UNHANDLED,
    FSM_RET_TRAN,
};
#define FSM_RET_CAST(x) 	( (fsm_ret_t)(x) )

#define FSM_HANDLED() 		SM_RET_CAST(SM_RET_HANDLED)
#define FSM_IGNORE() 		SM_RET_CAST(SM_RET_IGNORE)
#define FSM_UNHANDLED() 	SM_RET_CAST(SM_RET_UNHANDLED)

#define FSM_TRAN(me, target) \
			((me)->temp = (target), FSM_RET_CAST(SM_RET_TRAN))
#define FSM_SUPER(me, super) \
			((me)->temp = (super), FSM_RET_CAST(FSM_RET_SUPER))
#define FSM_TRIG(me, state, sig) 	((state)(me, &fsm_reserved_event[5 + (sig)]))
#define FSM_ENTRY(me, state) 		FSM_TRIG(me, state, FSM_ENTRY_SIG)
#define FSM_EXIT(me, state) 		FSM_TRIG(me, state, FSM_EXIT_SIG)

enum fsm_reserved_sig {
    FSM_EMPTY_SIG = -5,
    FSM_ENTRY_SIG = -4,
    FSM_EXIT_SIG  = -3,
    FSM_INIT_SIG  = -2,
    FSM_USER_SIG  = -1,
};

extern void fsm_ctor(fsm_t *me, fsm_state_handler_t init);
extern fsm_ret_t  fsm_init(fsm_t *me, fsm_event_t *e);
extern void fsm_dispatch(fsm_t *me, fsm_event_t *e);

#ifdef __cplusplus
}
#endif

#endif // __HSM_H__

