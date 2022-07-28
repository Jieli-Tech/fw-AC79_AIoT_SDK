#ifndef __TVS_STATE_H_111__
#define __TVS_STATE_H_111__

void tvs_state_start_work(bool busy, int type, int error);

void tvs_state_set(int new_state, int type, int error);

void tvs_state_manager_init();


#endif
