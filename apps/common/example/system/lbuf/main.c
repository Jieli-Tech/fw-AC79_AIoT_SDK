#include "app_config.h"
#include "system/includes.h"

//lbuf 头文件
#include "lbuf.h"

#ifdef USE_LBUF_TEST_DEMO
//====================================================//
/* lbuf模块接口测试点:
1. 各个接口调用读写基本功能;
2. 碎片回收性能;
3. 多线程读写测试;
*/
//====================================================//

#define LBUF_WR_API_TEST_ENABLE 					0
#define LBUF_MULT_TASK_WR_TEST_ENABLE 				1

#define LBUF_TEST_SIZE 			1024
#define LBUF_MAGIC_DATA 		0x5a

struct lbuf_test_head {
    u32 len;
    u8 data[0];
};

/*----------------------------------------------------------------------------*/
/**@brief   lbuf初始化
   @param   buf_size: lbuf空间长度
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void *lbuf_ptr = NULL;
static struct lbuff_head *lib_system_lbuf_test_init(u32 buf_size)
{
    struct lbuff_head *lbuf_handle = NULL;
    lbuf_ptr = malloc(buf_size);

    if (lbuf_ptr == NULL) {
        printf("lbuf malloc buf err");
        return NULL;
    }

    //lbuf初始化:
    lbuf_handle = lbuf_init(lbuf_ptr, buf_size, 4, sizeof(struct lbuf_test_head));

    return lbuf_handle;
}

/*----------------------------------------------------------------------------*/
/**@brief   lbuf_test释放资源
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void lib_system_cbuf_test_close(void)
{
    if (lbuf_ptr) {
        free(lbuf_ptr);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   查询lbuf状态
   @param   lbuf_handle
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void lib_system_lbuf_state(struct lbuff_head *handle)
{
    struct lbuff_state state;
    if (handle) {
        lbuf_state(handle, &state);
        printf("%s: %x", __func__, (u32)handle);
        printf("avaliable: %d", state.avaliable);
        printf("fragment: %d", state.fragment);
        printf("max_continue_len: %d", state.max_continue_len);
        printf("num: %d", state.num);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief   lbuf基本读写接口测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static void lib_system_lbuf_write_read_test(void)
{
    struct lbuff_head *lbuf_handle = lib_system_lbuf_test_init(LBUF_TEST_SIZE);
    const u32 alloc_size_table[] = {56, 65, 80, 100, 133};
    u32 i = 0;

    u32 buf_num;

    struct lbuf_test_head *wbuf = NULL;
    struct lbuf_test_head *rbuf = NULL;
    u32 wcount = 0;
    u32 rcount = 0;
    u32 wlen = 0;
    u32 rlen = 0;

    if (lbuf_handle == NULL) {
        return;
    }

    if (lbuf_empty(lbuf_handle)) {//查询LBUF内是否有数据帧
        printf("lbuf is empty");
    }

    printf("lbuf init, total size: %d", lbuf_remain_space(lbuf_handle));

    //lbuf写入:
    while (1) {
        wlen = alloc_size_table[i++];
        if (lbuf_free_space(lbuf_handle) < wlen) {//查询LBUF空闲数据块是否有足够长度
            break;
        }
        wbuf = (struct lbuf_test_head *)lbuf_alloc(lbuf_handle, wlen);  //lbuf内申请一块空间
        memset(&(wbuf->data[0]), LBUF_MAGIC_DATA, wlen);
        wbuf->len = wlen;
        lbuf_push(wbuf, BIT(0));//把数据块推送更新到lbuf的通道0
        wcount += wlen;
        i = i < ARRAY_SIZE(alloc_size_table) ? i : 0;
    }
    buf_num = lbuf_traversal(lbuf_handle);//查询LBUF内一共有多少个数据块还未读取
    printf("total write: %d byte, buf_num = %d block, remain: %d byte", wcount, buf_num, lbuf_remain_space(lbuf_handle));//查询LBUF 总共写入多少字节, 多少块数据帧, 剩余多少字节可写

    lib_system_lbuf_state(lbuf_handle);//查询LBUF状态信息

    //lbuf读出和释放:
    while (1) {
        if (lbuf_empty(lbuf_handle)) {//查询LBUF内是否有数据帧
            break;
        }
        rbuf = (struct lbuf_test_head *)lbuf_pop(lbuf_handle, BIT(0));//从lbuf的通道0读取数据块
        rlen = rbuf->len;
        for (i = 0; i < rlen; i++) {
            if ((rbuf->data[i]) != LBUF_MAGIC_DATA) {
                printf("lbuf read err: %d", rbuf->data[i]);
            }
        }

        rcount += rlen;

        if (lbuf_free(rbuf) == 0) { //释放lbuf通道0的数据块
            printf("lbuf free fail!!!");
        }
    }
    printf("total read: %d, remain: %d", rcount, lbuf_remain_space(lbuf_handle));
    if (rcount == wcount) {
        printf("lbuf wr pass");
    } else {
        printf("rw data count err!!!");
    }

__test_end:
    lib_system_cbuf_test_close();
}

/*----------------------------------------------------------------------------*/
/**@brief   lbuf多线程读写测试
   @param   void
   @return  void
   @note
*/
/*----------------------------------------------------------------------------*/
static OS_SEM sem1;
static OS_SEM sem2;
static struct lbuff_head *lbuf_handle_test = NULL;
static void lib_system_lbuf_task1(void *priv)
{
    int ret = 0;
    struct lbuf_test_head *wbuf = NULL;
    u32 wlen = 0;
    const u32 alloc_size_table[] = {56, 65, 80, 100, 133};
    u32 i = 0;

    while (1) {
        putchar('1');
        i = i < ARRAY_SIZE(alloc_size_table) ? i : 0;
        wlen = alloc_size_table[i++];
        wbuf = (struct lbuf_test_head *)lbuf_alloc(lbuf_handle_test, wlen);
        if (wbuf != NULL) {
            memset((u8 *)(&(wbuf->data[0])), LBUF_MAGIC_DATA, wlen);
            wbuf->len = wlen;
            lbuf_push(wbuf, (BIT(0) | BIT(1)));//把数据块推送更新到lbuf的通道0和通道1
            os_sem_post(&sem1);
            os_sem_post(&sem2);
        } else {
            printf("%s no buf", __func__);
        }
        os_time_dly(2);
    }
}

static void lib_system_lbuf_task2(void *priv)
{
    int ret = 0;
    struct lbuf_test_head *rbuf = NULL;

    while (1) {
        os_sem_pend(&sem1, 0);
        while (1) {
            rbuf = (struct lbuf_test_head *)lbuf_pop(lbuf_handle_test, BIT(0));//从lbuf的通道0读取数据块
            if (rbuf == NULL) {
                break;
            } else {
                putchar('2');
                for (u32 i = 0; i < rbuf->len; i++) {
                    if (rbuf->data[i] != LBUF_MAGIC_DATA) {
                        printf("%s read err: %d", __func__, rbuf->data[i]);
                    }
                }
                if (lbuf_free(rbuf)) {
                    putchar('f');
                }
            }
        }
    }
}


static void lib_system_lbuf_task3(void *priv)
{
    int ret = 0;
    struct lbuf_test_head *rbuf = NULL;

    while (1) {
        os_sem_pend(&sem2, 0);
        while (1) {
            rbuf = (struct lbuf_test_head *)lbuf_pop(lbuf_handle_test, BIT(1));//从lbuf的通道1读取数据块
            if (rbuf == NULL) {
                break;
            } else {
                putchar('3');
                for (u32 i = 0; i < rbuf->len; i++) {
                    if (rbuf->data[i] != LBUF_MAGIC_DATA) {
                        printf("%s read err: %d", __func__, rbuf->data[i]);
                    }
                }
                if (lbuf_free(rbuf)) {
                    putchar('f');
                }
            }
        }
    }
}


static void lib_system_lbuf_mult_task_wr_api_test(void)
{
    os_sem_create(&sem1, 0);
    os_sem_create(&sem2, 0);
    lbuf_handle_test = lib_system_lbuf_test_init(LBUF_TEST_SIZE);
    os_task_create(lib_system_lbuf_task1, NULL, 1, 256, 0, "lbuf_1");
    os_task_create(lib_system_lbuf_task2, NULL, 1, 256, 0, "lbuf_2");
    os_task_create(lib_system_lbuf_task3, NULL, 1, 256, 0, "lbuf_3");
}

static int c_main(void)
{
#if LBUF_WR_API_TEST_ENABLE
    lib_system_lbuf_write_read_test();
#endif /* #if LBUF_WR_API_TEST_ENABLE */

#if LBUF_MULT_TASK_WR_TEST_ENABLE
    lib_system_lbuf_mult_task_wr_api_test();
#endif /* #if LBUF_MULT_TASK_WR_TEST_ENABLE */

    return 0;
}
late_initcall(c_main);
#endif
