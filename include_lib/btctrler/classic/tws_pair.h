#ifndef TWS_PAIR_H
#define TWS_PAIR_H


void tws_pair_init();

int tws_pair_wait_connection(int timeout);

int tws_pair_create_connection(int timeout);

int tws_pair_cancle_create_connection();

int tws_pair_search_sibling_by_code(int code, int timeout_ms);

int tws_pair_wait_pair_by_code(int code, const char *name);

int tws_pair_cancle_wait_pair();

int tws_pair_wait_connect_in_esco();

int tws_pair_cancle_wait_connect_in_esco();

void tws_pair_conntion_suss_irq(int role);



int tws_open_tws_wait_pair(u16 code, const char *name);
int tws_open_tws_wait_conn(int timeout);
int tws_open_tws_conn(int timeout);

int tws_open_phone_wait_pair(u16 code, const char *name);


int tws_close_tws_pair();
int tws_close_phone_wait_pair();


int tws_remove_tws_pairs();
int tws_disconnect();
int tws_fast_conn_with_pair(u8 role_type);




















#endif
