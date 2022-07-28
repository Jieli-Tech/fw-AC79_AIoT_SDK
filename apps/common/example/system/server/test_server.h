#ifndef __TEST_SERVER_H__
#define __TEST_SERVER_H__

enum {
    TEST_REQ_DO1 = 1,
    TEST_REQ_DO2,
};

enum {
    TEST_SERVER_EVENT = 1,
    TEST_SERVER_END,
};


struct test_server_req_parm {
    u32 cmd;
};


#endif
