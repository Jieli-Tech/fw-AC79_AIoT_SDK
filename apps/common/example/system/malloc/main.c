//有两种方法查找内存泄漏,可以同时使用
//方法1.定位一个功能模块内部有没有内存泄漏
//方法2.需要使用mem_heap库来定位不断打印定位整个SDK申请内存的情况来分析

#include "app_config.h"
#include "system/includes.h"
#include "system/malloc.h"
#include "system/mem_leak_test.h" //如果需要启用方法1侦测内存泄漏, 必须所有需要侦测的.c和.h文件都要包含此头文件

#ifdef  USE_MALLOC_TEST_DEMO
//malloc_stats的打印:
/*
DLMALLOC_STATS:
total: 493488   //总共内存堆可使用的大小
left: 435416    //当前剩余可使用的大小
*/

//malloc_debug_show的打印:
/*
MALLOC LEAK DBG:malloc_test_task 20 rets:0x1c10660,size:0x1000 发现malloc_test_task函数的第20行,rets地址为0x1c10660,申请大小为0x1000
MALLOC REMAIN_CNT = 1, current_allocated = 24576, max_allocated=28672剩余 1个内存申请的指针还未释放,从启动内存泄漏侦测开始 已经申请了24576字节大小内存, 最大一共申请过28672字节大小
*/

//malloc_dump的打印:
/*
[00:00:03.130]mem_used: 1c1c988 - 1c1c9ec, size=64, call_from=2001f4c
[00:00:03.131]mem_used: 1c1c9ec - 1c1d9fc, size=1010, call_from=20072b4
[00:00:03.132]mem_used: 1c1d9fc - 1c1da60, size=64, call_from=2001f4c
[00:00:03.133]mem_used: 1c1da60 - 1c1ea70, size=1010, call_from=20072b4

如果发现有多个call_from地址相同,证明存在内存泄漏, 使用 定位异常地址.bat 查找是哪个位置泄漏的
*/

//mem_heap_check 侦测到内存被篡改的打印
/*
malloc_test_task, 104                       -----在某个函数的某个行号运行之前发现内存管理区域被篡改
mem_heap was maliciously modified:
mem_heap.ram = 0x1c07ba0
mem_heap.mem_heap_size = 0xa08c
mem = 0x1c0fc38
mem->next = 0x1c0fc90
mem->prev = 0x1c0fbe0
mem->used = 0x5a5aeaae
mem->check_before = 0x53555a66               -----发现这个被改成0x66了
mem->check_after = 0x53555aca
*/

static void malloc_test_task(void *p)
{
    char *p1, *p2;
    while (1) {
        p1 = malloc(4096);  //申请一块内存,默认出来的指针地址4字节对齐
        if (p1 == NULL) {
            puts("malloc_test_task malloc fail !!! \r\n");
        }


        memset(p1, 0x55, 4096);
//        put_buf(p1,32);


        p2 = zalloc(4096);//申请一块内存,并且把它清0
        if (p2 == NULL) {
            puts("malloc_test_task zalloc fail !!! \r\n");
        }

//        put_buf(p2,32);

        free(p1);        //使用完成释放内存

//        free(p2);       //可以把这句话注释掉,用于测试内存泄漏侦测

        os_time_dly(30);


#if 0
        char *ptr = malloc(64);
        memset(ptr, 0xab, 10240);
        puts("读写操作的地址范围超出内存申请指针包含的范围,导致概率性还没跑到这里就死机\r\n");
#endif

#if 0
        char *ptr = malloc(64);
        free(ptr);
        memset(ptr, 0xab, 10240);
        puts("内存指针释放了还继续使用,导致概率性还没跑到这里就死机\r\n");
#endif

#if 0
        char *ptr = malloc(64);
        free(ptr);
        free(ptr);
        puts("多次释放同一个内存申请的指针,导致还没跑到这里就死机\r\n");
#endif

#if 0 //如果无法使用, 需要联系FAE 重新编译库支持
        while (1) {
            char *ptr1 = malloc(64);
            char *ptr2 = malloc(64);

            memset(ptr1, 0xab, 64); //合理范围改写
            mem_heap_check(__FUNCTION__, __LINE__); //监测全部内存堆管理空间有没有被篡改

            putchar('.');

            if ((rand32() % 66) == 0) { //模拟可能是其他线程篡改了内存前后区域
                printf(" 模拟可能是其他线程篡改了内存前后区域\r\n");
                ptr1[64] = 0x66;   //模拟可能是其他线程篡改了内存前后区域
            }

            memset(ptr2, 0xab, 64); //合理范围改写

            mem_heap_check(__FUNCTION__, __LINE__); //监测全部内存堆管理空间有没有被篡改

            os_time_dly(10);

            free(ptr1);
            free(ptr2);
        }
#endif

    }
}
static int c_main(void)
{
    os_task_create(malloc_test_task, NULL, 10, 1000, 0, "malloc_test_task");
    return 0;
}
late_initcall(c_main);
#endif
