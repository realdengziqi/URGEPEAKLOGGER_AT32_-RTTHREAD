#include "app_record_service.h"

#include <string.h>

#include "app_data_service.h"
#include "storage_flashdb.h"

#define RECORD_SERVICE_WRITER_THREAD_STACK_SIZE    1024U
#define RECORD_SERVICE_WRITER_THREAD_PRIORITY      20U
#define RECORD_SERVICE_WRITER_THREAD_TIMESLICE     10U
#define RECORD_SERVICE_PERSIST_QUEUE_DEPTH         16U
#define RECORD_SERVICE_SEQUENCE_MAX                1000000000U

typedef struct
{
    record_service_item_t items[RECORD_SERVICE_CAPACITY];
    rt_uint32_t next_sequence;
    rt_uint32_t total_written;
    rt_uint32_t persist_written;
    rt_uint32_t persist_failed;
    rt_uint32_t persist_dropped;
    rt_uint32_t persist_pending;
    rt_size_t head;
    rt_size_t count;
    rt_mutex_t lock;
    rt_mutex_t persist_op_lock;
    rt_mq_t persist_queue;
    rt_thread_t persist_thread;
    rt_uint32_t persist_generation;
    rt_bool_t ready;
    rt_bool_t persistent_ready;
} record_service_context_t;

typedef struct
{
    rt_uint32_t generation;
    record_service_item_t item;
} record_service_persist_message_t;

typedef struct
{
    record_service_item_t items[RECORD_SERVICE_CAPACITY];
    rt_size_t count;
    fdb_tsdb_t db;
} record_service_restore_context_t;

typedef struct
{
    record_service_item_t *items;
    rt_size_t offset;
    rt_size_t max_count;
    rt_size_t total_count;
    rt_size_t actual_count;
    fdb_tsdb_t db;
} record_service_alarm_page_context_t;

static record_service_context_t record_service_ctx;

static rt_err_t record_service_lock_take(void);
static void record_service_lock_release(void);

static rt_err_t record_service_persist_op_lock_take(void)
{
    if (record_service_ctx.persist_op_lock == RT_NULL)
    {
        return -RT_ERROR;
    }

    return rt_mutex_take(record_service_ctx.persist_op_lock, RT_WAITING_FOREVER);
}

static void record_service_persist_op_lock_release(void)
{
    if (record_service_ctx.persist_op_lock != RT_NULL)
    {
        rt_mutex_release(record_service_ctx.persist_op_lock);
    }
}

static void record_service_update_persist_counters(rt_int32_t pending_delta,
                                                   rt_bool_t write_ok,
                                                   rt_bool_t dropped)
{
    if (record_service_lock_take() != RT_EOK)
    {
        return;
    }

    if (pending_delta > 0)
    {
        record_service_ctx.persist_pending += (rt_uint32_t)pending_delta;
    }
    else if (pending_delta < 0)
    {
        rt_uint32_t decrease = (rt_uint32_t)(-pending_delta);
        if (record_service_ctx.persist_pending > decrease)
        {
            record_service_ctx.persist_pending -= decrease;
        }
        else
        {
            record_service_ctx.persist_pending = 0U;
        }
    }

    if (write_ok == RT_TRUE)
    {
        record_service_ctx.persist_written++;
    }
    if (dropped == RT_TRUE)
    {
        record_service_ctx.persist_dropped++;
    }

    record_service_lock_release();
}

static const char *record_service_type_name(record_service_event_type_t type)
{
    switch (type)
    {
    case RECORD_SERVICE_EVENT_SURGE:
        return "surge";
    case RECORD_SERVICE_EVENT_ALARM:
        return "alarm";
    case RECORD_SERVICE_EVENT_MANUAL_TEST:
        return "manual_test";
    default:
        return "unknown";
    }
}

const char *record_service_get_type_name(record_service_event_type_t type);
const char *record_service_get_type_name(record_service_event_type_t type)
{
    return record_service_type_name(type);
}

const char *record_service_get_alarm_reason_name(record_service_alarm_reason_t reason)
{
    switch (reason)
    {
    case RECORD_SERVICE_ALARM_REASON_NTC_HIGH:
        return "ntc_high";
    case RECORD_SERVICE_ALARM_REASON_NTC_LOW:
        return "ntc_low";
    case RECORD_SERVICE_ALARM_REASON_TI1_OPEN:
        return "ti1_open";
    case RECORD_SERVICE_ALARM_REASON_TI2_OPEN:
        return "ti2_open";
    case RECORD_SERVICE_ALARM_REASON_TI3_OPEN:
        return "ti3_open";
    case RECORD_SERVICE_ALARM_REASON_TI4_OPEN:
        return "ti4_open";
    case RECORD_SERVICE_ALARM_REASON_TI5_OPEN:
        return "ti5_open";
    case RECORD_SERVICE_ALARM_REASON_NONE:
    default:
        return "none";
    }
}

const char *record_service_get_alarm_action_name(record_service_alarm_action_t action)
{
    switch (action)
    {
    case RECORD_SERVICE_ALARM_ACTION_START:
        return "alarm_start";
    case RECORD_SERVICE_ALARM_ACTION_CLEAR:
        return "alarm_clear";
    case RECORD_SERVICE_ALARM_ACTION_INSTANT:
        return "alarm_instant";
    case RECORD_SERVICE_ALARM_ACTION_NONE:
    default:
        return "none";
    }
}

static rt_bool_t record_service_item_is_valid(const record_service_item_t *item)
{
    if (item == RT_NULL)
    {
        return RT_FALSE;
    }

    if (item->sequence == 0U || item->sequence > RECORD_SERVICE_SEQUENCE_MAX)
    {
        return RT_FALSE;
    }

    if (item->format_version != RECORD_SERVICE_FORMAT_VERSION)
    {
        return RT_FALSE;
    }

    if (item->type != RECORD_SERVICE_EVENT_SURGE &&
        item->type != RECORD_SERVICE_EVENT_ALARM &&
        item->type != RECORD_SERVICE_EVENT_MANUAL_TEST)
    {
        return RT_FALSE;
    }

    return RT_TRUE;
}

static rt_err_t record_service_lock_take(void)
{
    if (record_service_ctx.lock == RT_NULL)
    {
        return -RT_ERROR;
    }

    return rt_mutex_take(record_service_ctx.lock, RT_WAITING_FOREVER);
}

static void record_service_lock_release(void)
{
    if (record_service_ctx.lock != RT_NULL)
    {
        rt_mutex_release(record_service_ctx.lock);
    }
}

static rt_size_t record_service_physical_index(rt_size_t logical_index)
{
    rt_size_t oldest;

    if (record_service_ctx.count < RECORD_SERVICE_CAPACITY)
    {
        oldest = 0U;
    }
    else
    {
        oldest = record_service_ctx.head;
    }

    return (oldest + logical_index) % RECORD_SERVICE_CAPACITY;
}

static rt_err_t record_service_append_persistent(const record_service_item_t *item)
{
    struct fdb_blob blob;
    fdb_tsdb_t db;

    if (item == RT_NULL || record_service_ctx.persistent_ready == RT_FALSE)
    {
        return -RT_ERROR;
    }

    db = (item->type == RECORD_SERVICE_EVENT_SURGE) ?
         storage_flashdb_get_surge_tsdb() :
         storage_flashdb_get_alarm_tsdb();
    if (db == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (fdb_tsl_append(db, fdb_blob_make(&blob, item, sizeof(*item))) != FDB_NO_ERR)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void record_service_persist_thread_entry(void *parameter)
{
    record_service_persist_message_t message;
    rt_err_t result;
    rt_bool_t stale;

    RT_UNUSED(parameter);

    while (1)
    {
        result = rt_mq_recv(record_service_ctx.persist_queue,
                            &message,
                            sizeof(message),
                            RT_WAITING_FOREVER);
        if (result <= 0)
        {
            continue;
        }

        record_service_update_persist_counters(-1, RT_FALSE, RT_FALSE);
        if (record_service_persist_op_lock_take() != RT_EOK)
        {
            result = -RT_ERROR;
        }
        else
        {
            stale = RT_TRUE;
            if (record_service_lock_take() == RT_EOK)
            {
                stale = (message.generation == record_service_ctx.persist_generation) ? RT_FALSE : RT_TRUE;
                record_service_lock_release();
            }

            result = (stale == RT_FALSE) ?
                     record_service_append_persistent(&message.item) :
                     -RT_EEMPTY;
            record_service_persist_op_lock_release();
        }

        if (record_service_lock_take() == RT_EOK)
        {
            if (result == RT_EOK)
            {
                record_service_ctx.persist_written++;
            }
            else if (result == -RT_EEMPTY)
            {
                record_service_ctx.persist_dropped++;
            }
            else
            {
                record_service_ctx.persist_failed++;
            }
            record_service_lock_release();
        }
    }
}

static rt_err_t record_service_submit_persistent(const record_service_item_t *item)
{
    record_service_persist_message_t message;
    rt_err_t result;

    if (item == RT_NULL || record_service_ctx.persistent_ready == RT_FALSE)
    {
        return -RT_ERROR;
    }

    if (record_service_ctx.persist_queue == RT_NULL)
    {
        return -RT_ERROR;
    }

    message.item = *item;
    message.generation = 0U;
    if (record_service_lock_take() == RT_EOK)
    {
        message.generation = record_service_ctx.persist_generation;
        record_service_lock_release();
    }

    record_service_update_persist_counters(1, RT_FALSE, RT_FALSE);
    result = rt_mq_send(record_service_ctx.persist_queue, &message, sizeof(message));
    if (result != RT_EOK)
    {
        record_service_update_persist_counters(-1, RT_FALSE, RT_TRUE);
    }

    return result;
}

static rt_err_t record_service_start_persist_thread(void)
{
    if (record_service_ctx.persistent_ready == RT_FALSE)
    {
        return RT_EOK;
    }

    if (record_service_ctx.persist_queue == RT_NULL)
    {
        record_service_ctx.persist_queue = rt_mq_create("record_wrq",
                                                        sizeof(record_service_persist_message_t),
                                                        RECORD_SERVICE_PERSIST_QUEUE_DEPTH,
                                                        RT_IPC_FLAG_PRIO);
        if (record_service_ctx.persist_queue == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    if (record_service_ctx.persist_thread == RT_NULL)
    {
        record_service_ctx.persist_thread = rt_thread_create("record_wr",
                                                            record_service_persist_thread_entry,
                                                            RT_NULL,
                                                            RECORD_SERVICE_WRITER_THREAD_STACK_SIZE,
                                                            RECORD_SERVICE_WRITER_THREAD_PRIORITY,
                                                            RECORD_SERVICE_WRITER_THREAD_TIMESLICE);
        if (record_service_ctx.persist_thread == RT_NULL)
        {
            return -RT_ENOMEM;
        }

        return rt_thread_startup(record_service_ctx.persist_thread);
    }

    return RT_EOK;
}

static void record_service_put_restored_item(const record_service_item_t *item)
{
    if (item == RT_NULL)
    {
        return;
    }

    record_service_ctx.items[record_service_ctx.head] = *item;
    record_service_ctx.head = (record_service_ctx.head + 1U) % RECORD_SERVICE_CAPACITY;
    if (record_service_ctx.count < RECORD_SERVICE_CAPACITY)
    {
        record_service_ctx.count++;
    }
    record_service_ctx.total_written++;
    if (item->sequence >= record_service_ctx.next_sequence)
    {
        record_service_ctx.next_sequence = item->sequence + 1U;
    }
}

static bool record_service_restore_tsl_cb(fdb_tsl_t tsl, void *arg)
{
    record_service_restore_context_t *restore = (record_service_restore_context_t *)arg;
    record_service_item_t item;
    struct fdb_blob blob;

    if (restore == RT_NULL || tsl == RT_NULL || tsl->status != FDB_TSL_WRITE)
    {
        return false;
    }

    if (restore->count >= RECORD_SERVICE_CAPACITY)
    {
        return true;
    }

    memset(&item, 0, sizeof(item));
    fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &item, sizeof(item)));
    if (fdb_blob_read((fdb_db_t)restore->db, &blob) != sizeof(item))
    {
        return false;
    }
    if (record_service_item_is_valid(&item) == RT_FALSE)
    {
        return false;
    }

    restore->items[restore->count++] = item;
    return false;
}

static bool record_service_alarm_page_tsl_cb(fdb_tsl_t tsl, void *arg)
{
    record_service_alarm_page_context_t *page = (record_service_alarm_page_context_t *)arg;
    record_service_item_t item;
    struct fdb_blob blob;

    if (page == RT_NULL || tsl == RT_NULL || tsl->status != FDB_TSL_WRITE)
    {
        return false;
    }

    memset(&item, 0, sizeof(item));
    fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &item, sizeof(item)));
    if (fdb_blob_read((fdb_db_t)page->db, &blob) != sizeof(item))
    {
        return false;
    }
    if (record_service_item_is_valid(&item) == RT_FALSE ||
        item.type != RECORD_SERVICE_EVENT_ALARM)
    {
        return false;
    }

    if (page->total_count >= page->offset &&
        page->actual_count < page->max_count)
    {
        page->items[page->actual_count++] = item;
    }
    page->total_count++;

    return (page->actual_count >= page->max_count) ? true : false;
}

static void record_service_restore_from_tsdb(void)
{
    record_service_restore_context_t restore;
    rt_size_t i;

    if (record_service_ctx.persistent_ready == RT_FALSE)
    {
        return;
    }

    memset(&restore, 0, sizeof(restore));
    restore.db = storage_flashdb_get_alarm_tsdb();
    if (restore.db != RT_NULL)
    {
        fdb_tsl_iter_reverse(restore.db, record_service_restore_tsl_cb, &restore);
    }

    restore.db = storage_flashdb_get_surge_tsdb();
    if (restore.db != RT_NULL && restore.count < RECORD_SERVICE_CAPACITY)
    {
        fdb_tsl_iter_reverse(restore.db, record_service_restore_tsl_cb, &restore);
    }

    for (i = 0U; i < restore.count; i++)
    {
        rt_size_t j;
        for (j = i + 1U; j < restore.count; j++)
        {
            if (restore.items[j].sequence < restore.items[i].sequence)
            {
                record_service_item_t tmp = restore.items[i];
                restore.items[i] = restore.items[j];
                restore.items[j] = tmp;
            }
        }
    }

    for (i = 0U; i < restore.count; i++)
    {
        record_service_put_restored_item(&restore.items[i]);
    }
}

static rt_err_t record_service_append_item(record_service_item_t *item)
{
    rt_err_t result;

    if (item == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = record_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    item->sequence = record_service_ctx.next_sequence++;
    item->format_version = RECORD_SERVICE_FORMAT_VERSION;
    record_service_ctx.items[record_service_ctx.head] = *item;
    record_service_ctx.head = (record_service_ctx.head + 1U) % RECORD_SERVICE_CAPACITY;
    if (record_service_ctx.count < RECORD_SERVICE_CAPACITY)
    {
        record_service_ctx.count++;
    }
    record_service_ctx.total_written++;

    record_service_lock_release();

    (void)record_service_submit_persistent(item);

    return RT_EOK;
}

rt_err_t record_service_init(void)
{
    if (record_service_ctx.ready != RT_FALSE)
    {
        return RT_EOK;
    }

    record_service_ctx.lock = rt_mutex_create("record_svc", RT_IPC_FLAG_PRIO);
    if (record_service_ctx.lock == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    record_service_ctx.persist_op_lock = rt_mutex_create("record_op", RT_IPC_FLAG_PRIO);
    if (record_service_ctx.persist_op_lock == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    record_service_ctx.next_sequence = 1U;
    record_service_ctx.total_written = 0U;
    record_service_ctx.head = 0U;
    record_service_ctx.count = 0U;
    record_service_ctx.persistent_ready =
        (storage_flashdb_init() == RT_EOK) ? RT_TRUE : RT_FALSE;
    record_service_restore_from_tsdb();
    if (record_service_start_persist_thread() != RT_EOK)
    {
        record_service_ctx.persistent_ready = RT_FALSE;
    }
    record_service_ctx.ready = RT_TRUE;

    return RT_EOK;
}

rt_err_t record_service_append_snapshot(record_service_event_type_t type)
{
    device_data_snapshot_t snapshot;
    record_service_item_t item;
    rt_err_t result;

    if (record_service_ctx.ready == RT_FALSE)
    {
        result = record_service_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    data_service_get_snapshot(&snapshot);

    item.sequence = 0U;
    item.format_version = RECORD_SERVICE_FORMAT_VERSION;
    item.type = type;
    item.alarm_id = 0U;
    item.alarm_reason = RECORD_SERVICE_ALARM_REASON_NONE;
    item.event_action = RECORD_SERVICE_ALARM_ACTION_NONE;
    item.source_index = 0U;
    item.timestamp_ms = snapshot.timestamp_ms;
    item.duration_ms = 0U;
    item.time_valid = snapshot.time_valid;
    item.datetime = snapshot.datetime;
    item.peak_milli_kv = snapshot.surge_peak_value_milli_kv;
    item.ntc_temp_milli_c = snapshot.ntc_temp_milli_c;
    item.env_temp_milli_c = snapshot.env_temp_milli_c;
    item.env_humi_milli_percent = snapshot.env_humi_milli_percent;
    item.digital_input_state = snapshot.digital_input_raw_state;
    item.alarm_state = (rt_uint8_t)snapshot.alarm_state;

    return record_service_append_item(&item);
}

rt_err_t record_service_append_alarm_record(const record_service_item_t *alarm_item)
{
    record_service_item_t item;
    rt_err_t result;

    if (alarm_item == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (record_service_ctx.ready == RT_FALSE)
    {
        result = record_service_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    item = *alarm_item;
    item.sequence = 0U;
    item.format_version = RECORD_SERVICE_FORMAT_VERSION;
    item.type = RECORD_SERVICE_EVENT_ALARM;

    return record_service_append_item(&item);
}

rt_err_t record_service_append_manual_test(rt_int32_t peak_milli_kv)
{
    device_data_snapshot_t snapshot;
    record_service_item_t item;

    if (record_service_ctx.ready == RT_FALSE)
    {
        rt_err_t result = record_service_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    data_service_get_snapshot(&snapshot);

    item.sequence = 0U;
    item.format_version = RECORD_SERVICE_FORMAT_VERSION;
    item.type = RECORD_SERVICE_EVENT_MANUAL_TEST;
    item.alarm_id = 0U;
    item.alarm_reason = RECORD_SERVICE_ALARM_REASON_NONE;
    item.event_action = RECORD_SERVICE_ALARM_ACTION_NONE;
    item.source_index = 0U;
    item.timestamp_ms = snapshot.timestamp_ms;
    item.duration_ms = 0U;
    item.time_valid = snapshot.time_valid;
    item.datetime = snapshot.datetime;
    item.peak_milli_kv = peak_milli_kv;
    item.ntc_temp_milli_c = snapshot.ntc_temp_milli_c;
    item.env_temp_milli_c = snapshot.env_temp_milli_c;
    item.env_humi_milli_percent = snapshot.env_humi_milli_percent;
    item.digital_input_state = snapshot.digital_input_raw_state;
    item.alarm_state = (rt_uint8_t)snapshot.alarm_state;

    return record_service_append_item(&item);
}

rt_err_t record_service_get_latest(record_service_item_t *item)
{
    rt_size_t latest_index;
    rt_err_t result;

    if (item == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = record_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    if (record_service_ctx.count == 0U)
    {
        record_service_lock_release();
        return -RT_EEMPTY;
    }

    latest_index = (record_service_ctx.head + RECORD_SERVICE_CAPACITY - 1U) % RECORD_SERVICE_CAPACITY;
    *item = record_service_ctx.items[latest_index];
    record_service_lock_release();

    return RT_EOK;
}

rt_err_t record_service_get_by_index(rt_size_t index, record_service_item_t *item)
{
    rt_size_t physical_index;
    rt_err_t result;

    if (item == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = record_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    if (index >= record_service_ctx.count)
    {
        record_service_lock_release();
        return -RT_EINVAL;
    }

    physical_index = record_service_physical_index(index);
    *item = record_service_ctx.items[physical_index];
    record_service_lock_release();

    return RT_EOK;
}

rt_err_t record_service_query_latest_page(rt_size_t offset_from_latest,
                                          rt_size_t max_count,
                                          record_service_item_t *items,
                                          rt_size_t *actual_count)
{
    rt_size_t available;
    rt_size_t copy_count;
    rt_size_t i;
    rt_err_t result;

    if (items == RT_NULL || actual_count == RT_NULL || max_count == 0U)
    {
        return -RT_EINVAL;
    }

    *actual_count = 0U;
    result = record_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    if (offset_from_latest >= record_service_ctx.count)
    {
        record_service_lock_release();
        return -RT_EEMPTY;
    }

    available = record_service_ctx.count - offset_from_latest;
    copy_count = (available < max_count) ? available : max_count;
    for (i = 0U; i < copy_count; i++)
    {
        rt_size_t logical_index = record_service_ctx.count - 1U - offset_from_latest - i;
        rt_size_t physical_index = record_service_physical_index(logical_index);

        items[i] = record_service_ctx.items[physical_index];
    }
    *actual_count = copy_count;
    record_service_lock_release();

    return RT_EOK;
}

rt_err_t record_service_query_page_by_sequence(rt_uint32_t start_sequence,
                                               rt_size_t max_count,
                                               record_service_item_t *items,
                                               rt_size_t *actual_count)
{
    rt_size_t start_index;
    rt_size_t copy_count;
    rt_size_t i;
    rt_err_t result;

    if (items == RT_NULL || actual_count == RT_NULL || max_count == 0U)
    {
        return -RT_EINVAL;
    }

    *actual_count = 0U;
    result = record_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    if (record_service_ctx.count == 0U)
    {
        record_service_lock_release();
        return -RT_EEMPTY;
    }

    start_index = record_service_ctx.count;
    for (i = 0U; i < record_service_ctx.count; i++)
    {
        rt_size_t physical_index = record_service_physical_index(i);

        if (record_service_ctx.items[physical_index].sequence >= start_sequence)
        {
            start_index = i;
            break;
        }
    }

    if (start_index >= record_service_ctx.count)
    {
        record_service_lock_release();
        return -RT_EEMPTY;
    }

    copy_count = record_service_ctx.count - start_index;
    if (copy_count > max_count)
    {
        copy_count = max_count;
    }

    for (i = 0U; i < copy_count; i++)
    {
        rt_size_t physical_index = record_service_physical_index(start_index + i);

        items[i] = record_service_ctx.items[physical_index];
    }
    *actual_count = copy_count;
    record_service_lock_release();

    return RT_EOK;
}

rt_err_t record_service_query_alarm_latest_page(rt_size_t offset_from_latest,
                                                rt_size_t max_count,
                                                record_service_item_t *items,
                                                rt_size_t *actual_count,
                                                rt_size_t *total_count)
{
    record_service_alarm_page_context_t page;
    rt_err_t result;

    if (items == RT_NULL || actual_count == RT_NULL || total_count == RT_NULL || max_count == 0U)
    {
        return -RT_EINVAL;
    }

    *actual_count = 0U;
    *total_count = 0U;

    if (record_service_ctx.ready == RT_FALSE)
    {
        result = record_service_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    if (record_service_ctx.persistent_ready == RT_FALSE)
    {
        return -RT_ERROR;
    }

    memset(&page, 0, sizeof(page));
    page.items = items;
    page.offset = offset_from_latest;
    page.max_count = max_count;
    page.db = storage_flashdb_get_alarm_tsdb();
    if (page.db == RT_NULL)
    {
        return -RT_ERROR;
    }

    result = record_service_persist_op_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    fdb_tsl_iter_reverse(page.db, record_service_alarm_page_tsl_cb, &page);
    record_service_persist_op_lock_release();

    *actual_count = page.actual_count;
    *total_count = page.total_count;

    return (page.actual_count > 0U) ? RT_EOK : -RT_EEMPTY;
}

rt_err_t record_service_get_status(record_service_status_t *status)
{
    rt_err_t result;

    if (status == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = record_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    status->next_sequence = record_service_ctx.next_sequence;
    status->generation = record_service_ctx.persist_generation;
    status->total_written = record_service_ctx.total_written;
    status->persist_written = record_service_ctx.persist_written;
    status->persist_failed = record_service_ctx.persist_failed;
    status->persist_dropped = record_service_ctx.persist_dropped;
    status->persist_pending = record_service_ctx.persist_pending;
    status->capacity = RECORD_SERVICE_CAPACITY;
    status->count = record_service_ctx.count;
    status->persistent_ready = (record_service_ctx.persistent_ready == RT_TRUE) ? 1U : 0U;
    record_service_lock_release();

    return RT_EOK;
}

void record_service_clear(void)
{
    rt_mq_t persist_queue = record_service_ctx.persist_queue;
    rt_bool_t persistent_ready;

    if (record_service_persist_op_lock_take() != RT_EOK)
    {
        return;
    }

    if (persist_queue != RT_NULL)
    {
        (void)rt_mq_control(persist_queue, RT_IPC_CMD_RESET, RT_NULL);
    }

    if (record_service_lock_take() != RT_EOK)
    {
        record_service_persist_op_lock_release();
        return;
    }

    record_service_ctx.persist_generation++;
    record_service_ctx.next_sequence = 1U;
    record_service_ctx.total_written = 0U;
    record_service_ctx.head = 0U;
    record_service_ctx.count = 0U;
    record_service_ctx.persist_written = 0U;
    record_service_ctx.persist_failed = 0U;
    record_service_ctx.persist_dropped = 0U;
    record_service_ctx.persist_pending = 0U;
    persistent_ready = record_service_ctx.persistent_ready;
    record_service_lock_release();

    if (persistent_ready == RT_TRUE)
    {
        (void)storage_flashdb_clear_records(STORAGE_FLASHDB_RECORD_KIND_ALARM);
        (void)storage_flashdb_clear_records(STORAGE_FLASHDB_RECORD_KIND_SURGE);
    }

    record_service_persist_op_lock_release();
}
