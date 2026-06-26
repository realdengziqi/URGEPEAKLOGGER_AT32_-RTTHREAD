#include "storage_flashdb.h"

#include <string.h>

#include "app_rtc.h"
#include "fal.h"

#define STORAGE_FLASHDB_TSDB_MAX_LEN    128U

static struct fdb_kvdb param_kvdb;
static struct fdb_tsdb alarm_tsdb;
static struct fdb_tsdb surge_tsdb;
static rt_mutex_t flashdb_lock = RT_NULL;
static rt_bool_t flashdb_ready = RT_FALSE;
static fdb_time_t flashdb_last_time = 0;

static void storage_flashdb_lock(fdb_db_t db)
{
    RT_UNUSED(db);
    if (flashdb_lock != RT_NULL)
    {
        (void)rt_mutex_take(flashdb_lock, RT_WAITING_FOREVER);
    }
}

static void storage_flashdb_unlock(fdb_db_t db)
{
    RT_UNUSED(db);
    if (flashdb_lock != RT_NULL)
    {
        rt_mutex_release(flashdb_lock);
    }
}

static fdb_time_t storage_flashdb_get_time(void)
{
    fdb_time_t now = (fdb_time_t)(rt_tick_get_millisecond() / 1000U);

    if (now <= flashdb_last_time)
    {
        now = flashdb_last_time + 1;
    }
    flashdb_last_time = now;

    return now;
}

static rt_err_t storage_flashdb_init_kvdb(void)
{
    fdb_err_t result;
    rt_uint32_t sec_size = 1024U;

    fdb_kvdb_control(&param_kvdb, FDB_KVDB_CTRL_SET_SEC_SIZE, &sec_size);
    fdb_kvdb_control(&param_kvdb, FDB_KVDB_CTRL_SET_LOCK, storage_flashdb_lock);
    fdb_kvdb_control(&param_kvdb, FDB_KVDB_CTRL_SET_UNLOCK, storage_flashdb_unlock);

    result = fdb_kvdb_init(&param_kvdb,
                           "param",
                           STORAGE_FLASHDB_PARAM_KVDB_PART_NAME,
                           RT_NULL,
                           RT_NULL);

    return (result == FDB_NO_ERR) ? RT_EOK : -RT_ERROR;
}

static rt_err_t storage_flashdb_init_tsdb(fdb_tsdb_t db, const char *name, const char *part_name)
{
    fdb_err_t result;
    rt_uint32_t sec_size = STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES;
    fdb_time_t db_last_time = 0;

    fdb_tsdb_control(db, FDB_TSDB_CTRL_SET_SEC_SIZE, &sec_size);
    fdb_tsdb_control(db, FDB_TSDB_CTRL_SET_LOCK, storage_flashdb_lock);
    fdb_tsdb_control(db, FDB_TSDB_CTRL_SET_UNLOCK, storage_flashdb_unlock);

    result = fdb_tsdb_init(db,
                           name,
                           part_name,
                           storage_flashdb_get_time,
                           STORAGE_FLASHDB_TSDB_MAX_LEN,
                           RT_NULL);
    if (result == FDB_NO_ERR)
    {
        fdb_tsdb_control(db, FDB_TSDB_CTRL_GET_LAST_TIME, &db_last_time);
        if (db_last_time > flashdb_last_time)
        {
            flashdb_last_time = db_last_time;
        }
    }

    return (result == FDB_NO_ERR) ? RT_EOK : -RT_ERROR;
}

rt_err_t storage_flashdb_init(void)
{
    if (flashdb_ready == RT_TRUE)
    {
        return RT_EOK;
    }

    if (flashdb_lock == RT_NULL)
    {
        flashdb_lock = rt_mutex_create("flashdb", RT_IPC_FLAG_PRIO);
        if (flashdb_lock == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    if (fal_init() <= 0)
    {
        return -RT_ERROR;
    }

    if (storage_flashdb_init_kvdb() != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (storage_flashdb_init_tsdb(&alarm_tsdb,
                                  "alarm",
                                  STORAGE_FLASHDB_ALARM_TSDB_PART_NAME) != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (storage_flashdb_init_tsdb(&surge_tsdb,
                                  "surge",
                                  STORAGE_FLASHDB_SURGE_TSDB_PART_NAME) != RT_EOK)
    {
        return -RT_ERROR;
    }

    flashdb_ready = RT_TRUE;
    return RT_EOK;
}

rt_bool_t storage_flashdb_ready(void)
{
    return flashdb_ready;
}

fdb_kvdb_t storage_flashdb_get_param_kvdb(void)
{
    return (flashdb_ready == RT_TRUE) ? &param_kvdb : RT_NULL;
}

fdb_tsdb_t storage_flashdb_get_alarm_tsdb(void)
{
    return (flashdb_ready == RT_TRUE) ? &alarm_tsdb : RT_NULL;
}

fdb_tsdb_t storage_flashdb_get_surge_tsdb(void)
{
    return (flashdb_ready == RT_TRUE) ? &surge_tsdb : RT_NULL;
}

rt_err_t storage_flashdb_clear_records(storage_flashdb_record_kind_t kind)
{
    fdb_tsdb_t db;

    if (flashdb_ready != RT_TRUE)
    {
        return -RT_ERROR;
    }

    db = (kind == STORAGE_FLASHDB_RECORD_KIND_SURGE) ? &surge_tsdb : &alarm_tsdb;
    fdb_tsl_clean(db);

    return RT_EOK;
}

rt_size_t storage_flashdb_record_count(storage_flashdb_record_kind_t kind)
{
    fdb_tsdb_t db;
    fdb_time_t last_time = 0;

    if (flashdb_ready != RT_TRUE)
    {
        return 0U;
    }

    db = (kind == STORAGE_FLASHDB_RECORD_KIND_SURGE) ? &surge_tsdb : &alarm_tsdb;
    fdb_tsdb_control(db, FDB_TSDB_CTRL_GET_LAST_TIME, &last_time);
    if (last_time <= 0)
    {
        return 0U;
    }

    return (rt_size_t)fdb_tsl_query_count(db, 0, last_time, FDB_TSL_WRITE);
}

rt_uint32_t storage_flashdb_last_time(void)
{
    return (rt_uint32_t)flashdb_last_time;
}
