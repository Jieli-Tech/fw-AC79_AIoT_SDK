#include "app_config.h"
#include "system/includes.h"

//cbuf 头文件
#include "circular_buf.h"

#ifdef USE_CBUF_TEST_DEMO
//====================================================//
/* cbuf模块接口测试点:
1. 各个接口调用读写基本功能;
2. 多线程读写测试;
*/
//====================================================//

#define CBUF_WR_API_TEST_ENABLE 					1
#define CBUF_WR_ALLOC_API_TEST_ENABLE 				0
#define CBUF_PREWRITE_API_TEST_ENABLE 				0
#define CBUF_MULT_TASK_WR_TEST_ENABLE 				0


#define CBUF_TEST_LEN 		(1024)
#define CBUF_MAGIC_DATA 	0x5a

/*----------------------------------------------------------------------------*/
/**@brief   cbuf初始化
   @param   buf_size: cbuf空间长度
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static cbuffer_t *lib_system_cbuf_test_init(u32 buf_size)
{
    cbuffer_t *cbuf_handle = NULL;
    cbuf_handle = (cbuffer_t *)malloc(sizeof(cbuffer_t) + buf_size);

    if (cbuf_handle == NULL) {
        printf("cbuf malloc buf err");
        return NULL;
    }

    //cbuf初始化:
    cbuf_init(cbuf_handle, (void *)(cbuf_handle + 1), buf_size);

    return cbuf_handle;
}

/*----------------------------------------------------------------------------*/
/**@brief   cbuf_test释放资源
   @param   buf: 待释放buf指针
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void lib_system_cbuf_test_close(void *buf)
{
    if (buf) {
        free(buf);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   cbuf基本读写接口测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
void lib_system_cbuf_write_read_test(void)
{
    u8 rbuf[50] = {0};
    u8 wbuf[50] = {0};
    u32 wlen = 0;
    u32 rlen = 0;
    u32 ret = 0;
    u32 i = 0;

    cbuffer_t *cbuf_handle = lib_system_cbuf_test_init(CBUF_TEST_LEN);
    if (cbuf_handle == NULL) {
        return;
    }
    //cbuf 写测试:
    memset(wbuf, CBUF_MAGIC_DATA, sizeof(wbuf));
    while (1) {
        if (cbuf_is_write_able(cbuf_handle, sizeof(wbuf))) {    //查询cbuf是否有足够空间可写
            ret = cbuf_write(cbuf_handle, wbuf, sizeof(wbuf));  //写入并且更新到cbuf
        } else {
            ret = cbuf_get_data_size(cbuf_handle);//查询cbuf内已经写入多少数据量
            ret = CBUF_TEST_LEN - ret;            //得到cbuf剩余可写数据量
            ret = cbuf_write(cbuf_handle, wbuf, ret); //写入并且更新到cbuf
        }
        if (ret == 0) {
            break;
        }
        wlen += ret;
    }

    if (wlen == CBUF_TEST_LEN) {
        printf("cbuf write test pass");
    } else {
        printf("cbuf write test err!!! wlen = %d, target = %d", wlen, CBUF_TEST_LEN);
        goto __test_end;
    }

    //cbuf 读测试:
    while (1) {
        memset(rbuf, 0x0, sizeof(rbuf));
        ret = cbuf_get_data_size(cbuf_handle);//查询cbuf内有多少可读数据量
        if (ret >= sizeof(rbuf)) {
            ret = cbuf_read(cbuf_handle, rbuf, sizeof(rbuf));//读取并且从cbuf释放
        } else {
            ret = cbuf_read(cbuf_handle, rbuf, ret);//读取并且从cbuf释放
        }
        if (ret == 0) {
            break;
        } else {
            if (memcmp(rbuf, wbuf, ret)) {
                printf("cbuf read data err!!!");
            }
        }
        rlen += ret;
    }

    if (rlen == CBUF_TEST_LEN) {
        printf("cbuf read test pass");
    } else {
        printf("cbuf read test err!!! rlen = %d, target = %d", rlen, CBUF_TEST_LEN);
        goto __test_end;
    }

__test_end:
    if (cbuf_handle) {
        free(cbuf_handle);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   cbuf申请buf类型接口测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
void lib_system_cbuf_write_read_alloc_test(void)
{
    u32 wlen;
    u32 rlen;
    u32 count = 0;
    u8 *wbuf;
    u8 *rbuf;
    u32 i = 0;

    cbuffer_t *cbuf_handle = lib_system_cbuf_test_init(CBUF_TEST_LEN);
    if (cbuf_handle == NULL) {
        return;
    }

    while (1) {
        wbuf = cbuf_write_alloc(cbuf_handle, &wlen);//查询cbuf从写指针到cbuf末尾有多少空间可写
        if (wlen) {
            memset(wbuf, CBUF_MAGIC_DATA, wlen);
            cbuf_write_updata(cbuf_handle, wlen);   //写入数据后更新到cbuf管理
            count += wlen;
        } else {
            break;
        }
    }

    if (count == CBUF_TEST_LEN) {
        printf("cbuf write alloc pass");
    } else {
        printf("cbuf write alloc err!!! wlen = %d, target = %d", count, CBUF_TEST_LEN);
        goto __test_end;
    }

    count = 0;
    while (1) {
        rbuf = cbuf_read_alloc(cbuf_handle, &rlen);//查询cbuf从读指针到cbuf末尾有多少空间可读
        if (rlen) {
            for (i = 0; i < rlen; i++) {
                if (rbuf[i] != CBUF_MAGIC_DATA) {
                    printf("cbuf read alloc data err: 0x%x", rbuf[i]);
                }
            }
            count += rlen;
            cbuf_read_updata(cbuf_handle, rlen);//数据读取使用完后更新到cbuf数据可以释放
        } else {
            break;
        }
    }

    if (count == CBUF_TEST_LEN) {
        printf("cbuf read alloc pass");
    } else {
        printf("cbuf read alloc err!!! wlen = %d, target = %d", count, CBUF_TEST_LEN);
        goto __test_end;
    }


__test_end:
    lib_system_cbuf_test_close(cbuf_handle);
}

/*----------------------------------------------------------------------------*/
/**@brief   cbuf prewrite 接口测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
void lib_system_cbuf_prewrite_test(void)
{
    u8 rbuf[50] = {0};
    u8 wbuf[50] = {0};
    u32 count = 0;
    u32 ret = 0;

    cbuffer_t *cbuf_handle = lib_system_cbuf_test_init(CBUF_TEST_LEN);
    if (cbuf_handle == NULL) {
        return;
    }

    memset(wbuf, CBUF_MAGIC_DATA, sizeof(wbuf));
    cbuf_prewrite(cbuf_handle, wbuf, sizeof(wbuf)); //预写入数据到cbuf,但是只是临时的,cbuf读取不到
    cbuf_discard_prewrite(cbuf_handle);             //放弃预写入的临时数据

    while (1) {
        ret = cbuf_prewrite(cbuf_handle, wbuf, sizeof(wbuf));//预写入数据到cbuf,但是只是临时的,cbuf读取不到
        if (ret) {
            count += ret;
            cbuf_updata_prewrite(cbuf_handle);//把预写入的数据更新到cbuf
        } else {
            ret = cbuf_get_data_size(cbuf_handle);//查询cbuf内有多少可读数据量
            ret = CBUF_TEST_LEN - ret;            //得到cbuf剩余可写数据量
            ret = cbuf_prewrite(cbuf_handle, wbuf, ret);//预写入数据到cbuf,但是只是临时的,cbuf读取不到
            cbuf_updata_prewrite(cbuf_handle);//把预写入的数据更新到cbuf
            count += ret;
            break;
        }
    }

    if (count == CBUF_TEST_LEN) {
        printf("cbuf priwrite test pass");
    } else {
        printf("cbuf prewrite err!!! wlen = %d, target = %d", count, CBUF_TEST_LEN);
        goto __test_end;
    }

    //cbuf数据清除
    cbuf_clear(cbuf_handle);

    ret = cbuf_get_data_len(cbuf_handle);
    if (ret == 0) {
        printf("cbuf clear pass");
    } else {
        printf("cbuf clear fail, ret = %d", ret);
    }

__test_end:
    lib_system_cbuf_test_close(cbuf_handle);
}

/*----------------------------------------------------------------------------*/
/**@brief   cbuf 多线程读写测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static OS_SEM cbuf_sem;
static cbuffer_t *cbuf_handle_test;
static void cbuf_write_task(void *p)
{
    u8 wbuf[77];
    u32 cbuf_data_size, cbuf_remain_size, ret;
    while (1) {
        os_time_dly(50);   //adjust  send period

        memset(wbuf, CBUF_MAGIC_DATA, sizeof(wbuf));
        if (cbuf_is_write_able(cbuf_handle_test, sizeof(wbuf))) { //查询cbuf是否有足够空间可写
            ret = cbuf_write(cbuf_handle_test, wbuf, sizeof(wbuf)); //写入并且更新到cbuf
            printf("cbuf_is_whole_write_able %d ,ret= %d\r\n", sizeof(wbuf), ret);
            os_sem_set(&cbuf_sem, 0);
            os_sem_post(&cbuf_sem);
        } else {
            cbuf_data_size = cbuf_get_data_size(cbuf_handle_test);//查询cbuf内已经写入多少数据量
            if (cbuf_data_size == CBUF_TEST_LEN) {
                printf("cbuf_is_not_write_able, cbuf_data_size= %d\r\n", cbuf_data_size);
                os_time_dly(300);
            } else {
                cbuf_remain_size = CBUF_TEST_LEN - cbuf_data_size; //得到cbuf剩余可写数据量
                ret = cbuf_write(cbuf_handle_test, wbuf, cbuf_remain_size); //写入并且更新到cbuf
                printf("cbuf_is_partial_write_able %d ,ret= %d\r\n", cbuf_remain_size, ret);
                os_sem_set(&cbuf_sem, 0);
                os_sem_post(&cbuf_sem);
            }
        }
    }
}

static void cbuf_read_task(void *p)
{
    u8 rbuf[53];
    u32 cbuf_data_size, ret;
    while (1) {
        memset(rbuf, 0x0, sizeof(rbuf));
        cbuf_data_size = cbuf_get_data_size(cbuf_handle_test);//查询cbuf内有多少可读数据量
        if (cbuf_data_size == 0) {
            puts("cbuf_is_not_readable,waitting...\r\n");
            os_sem_pend(&cbuf_sem, 0);
        } else if (cbuf_data_size >= sizeof(rbuf)) {
            ret = cbuf_read(cbuf_handle_test, rbuf, sizeof(rbuf));//读取并且从cbuf释放
            printf("cbuf_is_whole_read_able %d ,ret= %d\r\n", sizeof(rbuf), ret);
        } else {
            ret = cbuf_read(cbuf_handle_test, rbuf, cbuf_data_size);//读取并且从cbuf释放
            printf("cbuf_is_partial_read_able %d ,ret= %d\r\n", cbuf_data_size, ret);

        }
        if (cbuf_data_size) {
            for (int i = 0; i < ret; i++) {
                if (rbuf[i] != CBUF_MAGIC_DATA) {
                    printf("cbuf read data err = 0x%x \r\n", rbuf[i]);
                }
            }
        }
    }
}



/*----------------------------------------------------------------------------*/
/**@brief   cbuf 多线程读写测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void lib_system_cbuf_mult_task_wr_api_test(void)
{
    os_sem_create(&cbuf_sem, 0);
    cbuf_handle_test = lib_system_cbuf_test_init(CBUF_TEST_LEN);
    os_task_create(cbuf_write_task, NULL, 10, 1000, 0, "cbuf_write_task");
    os_task_create(cbuf_read_task, NULL, 11, 1000, 0, "cbuf_read_task");
}


static int c_main(void)
{
#if CBUF_WR_API_TEST_ENABLE
    lib_system_cbuf_write_read_test();
#endif /* #if CBUF_WR_API_TEST_ENABLE */

#if CBUF_WR_ALLOC_API_TEST_ENABLE
    lib_system_cbuf_write_read_alloc_test();
#endif /* #if CBUF_WR_ALLOC_API_TEST_ENABLE */

#if CBUF_PREWRITE_API_TEST_ENABLE
    lib_system_cbuf_prewrite_test();
#endif /* #if CBUF_PREWRITE_API_TEST_ENABLE */

#if CBUF_MULT_TASK_WR_TEST_ENABLE
    lib_system_cbuf_mult_task_wr_api_test();
#endif /* #if CBUF_MULT_TASK_WR_TEST_ENABLE */

    return 0;
}
late_initcall(c_main);
#endif
