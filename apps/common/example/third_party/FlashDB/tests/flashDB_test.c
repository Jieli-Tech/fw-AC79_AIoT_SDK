#include "device/device.h"
#include "fdb_def.h"
#include "fdb_cfg.h"
#include "app_config.h"
#include "system/includes.h"


#ifdef USE_FLASHDB_TEST_DEMO

#define FDB_LOG_TAG "[main]"

static uint32_t boot_count = 0;
static size_t boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
    {"username", "armink", 0}, /* string KV */
    {"password", "123456", 0}, /* string KV */
    {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
    {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};
/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };
/* TSDB object */
struct fdb_tsdb tsdb = { 0 };
/* counts for simulated timestamp */
static int counts = 0;

extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_string_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);
extern void tsdb_sample(fdb_tsdb_t tsdb);
extern void fdb_tsdb_control(fdb_tsdb_t db, int cmd, void *arg);
extern void fdb_kvdb_control(fdb_kvdb_t db, int cmd, void *arg);

static void lock(fdb_db_t db)
{
    /*__disable_irq();*/
}

static void unlock(fdb_db_t db)
{
    /*__enable_irq();*/
}

static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}

extern fdb_err_t fdb_kvdb_init(fdb_kvdb_t db, const char *name, const char *part_name, struct fdb_default_kv *default_kv,
                               void *user_data);
extern fdb_err_t fdb_tsdb_init(fdb_tsdb_t db, const char *name, const char *part_name, fdb_get_time get_time, size_t max_len, void *user_data);
int test_DB(void)
{

    fdb_err_t result;
#ifdef FDB_USING_KVDB
    {
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        /* set the lock and unlock function if you want */
        /* fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock); */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);



        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
        /* run string KV samples */
        kvdb_type_string_sample(&kvdb);
        /* run blob KV samples */
        kvdb_type_blob_sample(&kvdb);
    }
#endif
#ifdef FDB_USING_TSDB

    printf("========tsdb_sample========");
    { /* TSDB Sample */
        /* set the lock and unlock function if you want */
        /*fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);*/
        /*fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);*/
        /* Time series database initialization
         *
         *       &tsdb: database object
         *       "log": database name
         * "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         *    get_time: The get current timestamp function.
         *         128: maximum length of each log
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);
        /* read last saved time for simulated timestamp */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);

        if (result != FDB_NO_ERR) {
            return -1   ;
        }

        /* run TSDB sample */

        tsdb_sample(&tsdb);
    }
#endif /* FDB_USING_TSDB */

    return 0;
}

static int c_main(void)
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------flashDB_test %s -------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    /* test_DB(); */
    thread_fork("test_DB", 13, 1000, 128, NULL, test_DB, NULL);

    return 0;
}

late_initcall(c_main);
#endif

