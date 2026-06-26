#include "app_alarm_service.h"

#include <string.h>

#include "app_data_service.h"
#include "app_param_service.h"
#include "storage_flashdb.h"

#define ALARM_SERVICE_NTC_HIGH_THRESHOLD_MC      70000
#define ALARM_SERVICE_NTC_LOW_THRESHOLD_MC       (-20000)
#define ALARM_SERVICE_DI_MONITOR_MASK            0x1FU
#define ALARM_SERVICE_DI_REASON_BASE_INDEX       1U
#define ALARM_SERVICE_IMAGE_KEY                  "active_alarm_image"
#define ALARM_SERVICE_IMAGE_MAGIC                0x414C4D49U
#define ALARM_SERVICE_IMAGE_VERSION              1U
#define ALARM_SERVICE_IMAGE_THREAD_STACK_SIZE    1536U
#define ALARM_SERVICE_IMAGE_THREAD_PRIORITY      21U
#define ALARM_SERVICE_IMAGE_THREAD_TIMESLICE     10U

typedef struct
{
    rt_uint8_t active;
    rt_uint8_t reason;
    rt_uint8_t source_index;
    rt_uint8_t start_time_valid;
    rt_uint32_t alarm_id;
    rt_uint32_t start_timestamp_ms;
    app_rtc_datetime_t start_datetime;
    rt_int32_t ntc_temp_milli_c;
    rt_uint8_t digital_input_state;
    rt_uint8_t reserved[3];
} alarm_service_slot_image_t;

typedef struct
{
    rt_uint32_t magic;
    rt_uint16_t version;
    rt_uint16_t slot_count;
    rt_uint32_t next_alarm_id;
    rt_uint32_t checksum;
    alarm_service_slot_image_t slots[ALARM_SERVICE_ACTIVE_CAPACITY];
} alarm_service_active_image_t;

typedef struct
{
    rt_uint8_t active;
    rt_uint32_t alarm_id;
    record_service_alarm_reason_t reason;
    rt_uint8_t source_index;
    rt_uint32_t start_timestamp_ms;
    rt_uint8_t start_time_valid;
    app_rtc_datetime_t start_datetime;
    rt_int32_t ntc_temp_milli_c;
    rt_uint8_t digital_input_state;
} alarm_service_slot_t;

typedef struct
{
    alarm_service_slot_t slots[ALARM_SERVICE_ACTIVE_CAPACITY];
    rt_uint8_t ntc_pending_active;
    record_service_alarm_reason_t ntc_pending_reason;
    rt_uint32_t ntc_pending_start_ms;
    rt_uint8_t ti_pending_active[5];
    rt_uint8_t ti_pending_target_active[5];
    rt_uint32_t ti_pending_start_ms[5];
    rt_uint32_t next_alarm_id;
    rt_uint32_t total_started;
    rt_uint32_t total_ended;
    rt_uint32_t image_write_count;
    rt_uint32_t image_write_failed;
    rt_bool_t persistent_ready;
    rt_bool_t image_restored;
    rt_bool_t image_dirty;
    rt_sem_t image_sem;
    rt_thread_t image_thread;
    rt_mutex_t lock;
    rt_bool_t ready;
} alarm_service_context_t;

static alarm_service_context_t alarm_service_ctx;

static rt_err_t alarm_service_lock_take(void);
static void alarm_service_lock_release(void);

static rt_uint32_t alarm_service_checksum(const void *data, rt_size_t size)
{
    const rt_uint8_t *bytes = (const rt_uint8_t *)data;
    rt_uint32_t hash = 2166136261UL;
    rt_size_t i;

    if (data == RT_NULL)
    {
        return 0U;
    }

    for (i = 0U; i < size; i++)
    {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }

    return hash;
}

static rt_bool_t alarm_service_reason_is_valid(record_service_alarm_reason_t reason,
                                               rt_uint8_t source_index,
                                               rt_size_t slot_index)
{
    if (slot_index == 0U)
    {
        return (reason == RECORD_SERVICE_ALARM_REASON_NTC_HIGH ||
                reason == RECORD_SERVICE_ALARM_REASON_NTC_LOW) ? RT_TRUE : RT_FALSE;
    }

    if (slot_index >= ALARM_SERVICE_ACTIVE_CAPACITY ||
        source_index != (rt_uint8_t)slot_index)
    {
        return RT_FALSE;
    }

    if (reason < RECORD_SERVICE_ALARM_REASON_TI1_OPEN ||
        reason > RECORD_SERVICE_ALARM_REASON_TI5_OPEN)
    {
        return RT_FALSE;
    }

    return ((rt_size_t)(reason - RECORD_SERVICE_ALARM_REASON_TI1_OPEN + 1) == slot_index) ?
           RT_TRUE :
           RT_FALSE;
}

static rt_bool_t alarm_service_slot_image_is_valid(const alarm_service_slot_image_t *slot,
                                                   rt_size_t slot_index)
{
    record_service_alarm_reason_t reason;

    if (slot == RT_NULL)
    {
        return RT_FALSE;
    }

    if (slot->active == 0U)
    {
        return RT_TRUE;
    }

    if (slot->active != 1U ||
        slot->alarm_id == 0U ||
        slot->start_time_valid > 1U ||
        (slot->digital_input_state & (rt_uint8_t)~ALARM_SERVICE_DI_MONITOR_MASK) != 0U)
    {
        return RT_FALSE;
    }

    reason = (record_service_alarm_reason_t)slot->reason;
    return alarm_service_reason_is_valid(reason, slot->source_index, slot_index);
}

static rt_uint32_t alarm_service_saturating_seconds_to_ms(rt_uint32_t seconds)
{
    if (seconds > (0xFFFFFFFFUL / 1000UL))
    {
        return 0xFFFFFFFFUL;
    }

    return seconds * 1000UL;
}

static rt_uint32_t alarm_service_elapsed_ms(rt_uint32_t start_ms,
                                            rt_uint32_t end_ms,
                                            rt_uint8_t start_time_valid,
                                            const app_rtc_datetime_t *start_datetime,
                                            rt_uint8_t end_time_valid,
                                            const app_rtc_datetime_t *end_datetime)
{
    rt_uint32_t start_unix = 0U;
    rt_uint32_t end_unix = 0U;

    if (start_time_valid != 0U &&
        end_time_valid != 0U &&
        start_datetime != RT_NULL &&
        end_datetime != RT_NULL &&
        app_rtc_datetime_to_unix(start_datetime, &start_unix) == RT_EOK &&
        app_rtc_datetime_to_unix(end_datetime, &end_unix) == RT_EOK &&
        end_unix >= start_unix)
    {
        return alarm_service_saturating_seconds_to_ms(end_unix - start_unix);
    }

    if (end_ms >= start_ms)
    {
        return end_ms - start_ms;
    }

    return end_ms;
}

static rt_uint32_t alarm_service_active_duration_ms(const alarm_service_slot_t *slot,
                                                    rt_uint32_t now_ms)
{
    app_rtc_datetime_t now_datetime;

    if (slot == RT_NULL)
    {
        return 0U;
    }

    if (slot->start_time_valid != 0U &&
        app_rtc_get_datetime(&now_datetime) == RT_EOK)
    {
        return alarm_service_elapsed_ms(slot->start_timestamp_ms,
                                        now_ms,
                                        slot->start_time_valid,
                                        &slot->start_datetime,
                                        1U,
                                        &now_datetime);
    }

    return alarm_service_elapsed_ms(slot->start_timestamp_ms,
                                    now_ms,
                                    0U,
                                    RT_NULL,
                                    0U,
                                    RT_NULL);
}

static void alarm_service_build_image(alarm_service_active_image_t *image)
{
    rt_size_t i;

    memset(image, 0, sizeof(*image));
    image->magic = ALARM_SERVICE_IMAGE_MAGIC;
    image->version = ALARM_SERVICE_IMAGE_VERSION;
    image->slot_count = ALARM_SERVICE_ACTIVE_CAPACITY;
    image->next_alarm_id = alarm_service_ctx.next_alarm_id;

    for (i = 0U; i < ALARM_SERVICE_ACTIVE_CAPACITY; i++)
    {
        const alarm_service_slot_t *slot = &alarm_service_ctx.slots[i];
        alarm_service_slot_image_t *out = &image->slots[i];

        out->active = slot->active;
        out->reason = (rt_uint8_t)slot->reason;
        out->source_index = slot->source_index;
        out->start_time_valid = slot->start_time_valid;
        out->alarm_id = slot->alarm_id;
        out->start_timestamp_ms = slot->start_timestamp_ms;
        out->start_datetime = slot->start_datetime;
        out->ntc_temp_milli_c = slot->ntc_temp_milli_c;
        out->digital_input_state = slot->digital_input_state;
    }

    image->checksum = 0U;
    image->checksum = alarm_service_checksum(image, sizeof(*image));
}

static rt_bool_t alarm_service_image_is_valid(alarm_service_active_image_t *image)
{
    rt_uint32_t saved_checksum;
    rt_uint32_t calc_checksum;
    rt_size_t i;

    if (image == RT_NULL ||
        image->magic != ALARM_SERVICE_IMAGE_MAGIC ||
        image->version != ALARM_SERVICE_IMAGE_VERSION ||
        image->slot_count != ALARM_SERVICE_ACTIVE_CAPACITY ||
        image->next_alarm_id == 0U)
    {
        return RT_FALSE;
    }

    saved_checksum = image->checksum;
    image->checksum = 0U;
    calc_checksum = alarm_service_checksum(image, sizeof(*image));
    image->checksum = saved_checksum;
    if (calc_checksum != saved_checksum)
    {
        return RT_FALSE;
    }

    for (i = 0U; i < ALARM_SERVICE_ACTIVE_CAPACITY; i++)
    {
        if (alarm_service_slot_image_is_valid(&image->slots[i], i) == RT_FALSE)
        {
            return RT_FALSE;
        }
    }

    return RT_TRUE;
}

static rt_err_t alarm_service_write_active_image(const alarm_service_active_image_t *image)
{
    struct fdb_blob blob;
    fdb_kvdb_t kvdb;

    if (image == RT_NULL || alarm_service_ctx.persistent_ready == RT_FALSE)
    {
        return -RT_ERROR;
    }

    kvdb = storage_flashdb_get_param_kvdb();
    if (kvdb == RT_NULL)
    {
        alarm_service_ctx.image_write_failed++;
        return -RT_ERROR;
    }

    if (fdb_kv_set_blob(kvdb,
                        ALARM_SERVICE_IMAGE_KEY,
                        fdb_blob_make(&blob, image, sizeof(*image))) != FDB_NO_ERR)
    {
        alarm_service_ctx.image_write_failed++;
        return -RT_ERROR;
    }

    alarm_service_ctx.image_write_count++;
    return RT_EOK;
}

static void alarm_service_request_image_save(void)
{
    if (alarm_service_ctx.persistent_ready == RT_FALSE)
    {
        return;
    }

    alarm_service_ctx.image_dirty = RT_TRUE;
    if (alarm_service_ctx.image_sem != RT_NULL)
    {
        (void)rt_sem_release(alarm_service_ctx.image_sem);
    }
}

static void alarm_service_image_thread_entry(void *parameter)
{
    alarm_service_active_image_t image;

    RT_UNUSED(parameter);

    while (1)
    {
        if (rt_sem_take(alarm_service_ctx.image_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        if (alarm_service_lock_take() != RT_EOK)
        {
            continue;
        }

        alarm_service_build_image(&image);
        alarm_service_ctx.image_dirty = RT_FALSE;
        alarm_service_lock_release();

        (void)alarm_service_write_active_image(&image);
    }
}

static rt_err_t alarm_service_start_image_thread(void)
{
    if (alarm_service_ctx.persistent_ready == RT_FALSE)
    {
        return RT_EOK;
    }

    if (alarm_service_ctx.image_sem == RT_NULL)
    {
        alarm_service_ctx.image_sem = rt_sem_create("alarm_img", 0U, RT_IPC_FLAG_FIFO);
        if (alarm_service_ctx.image_sem == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    if (alarm_service_ctx.image_thread == RT_NULL)
    {
        alarm_service_ctx.image_thread = rt_thread_create("alarm_img",
                                                         alarm_service_image_thread_entry,
                                                         RT_NULL,
                                                         ALARM_SERVICE_IMAGE_THREAD_STACK_SIZE,
                                                         ALARM_SERVICE_IMAGE_THREAD_PRIORITY,
                                                         ALARM_SERVICE_IMAGE_THREAD_TIMESLICE);
        if (alarm_service_ctx.image_thread == RT_NULL)
        {
            return -RT_ENOMEM;
        }

        return rt_thread_startup(alarm_service_ctx.image_thread);
    }

    return RT_EOK;
}

static void alarm_service_restore_active_image(void)
{
    alarm_service_active_image_t image;
    struct fdb_blob blob;
    fdb_kvdb_t kvdb;
    rt_size_t read_size;
    rt_size_t i;
    rt_uint32_t max_alarm_id = 0U;

    if (alarm_service_ctx.persistent_ready == RT_FALSE)
    {
        return;
    }

    kvdb = storage_flashdb_get_param_kvdb();
    if (kvdb == RT_NULL)
    {
        return;
    }

    memset(&image, 0, sizeof(image));
    read_size = fdb_kv_get_blob(kvdb,
                                ALARM_SERVICE_IMAGE_KEY,
                                fdb_blob_make(&blob, &image, sizeof(image)));
    if (read_size != sizeof(image) ||
        alarm_service_image_is_valid(&image) == RT_FALSE)
    {
        return;
    }

    memset(alarm_service_ctx.slots, 0, sizeof(alarm_service_ctx.slots));
    for (i = 0U; i < ALARM_SERVICE_ACTIVE_CAPACITY; i++)
    {
        const alarm_service_slot_image_t *src = &image.slots[i];
        alarm_service_slot_t *dst = &alarm_service_ctx.slots[i];

        if (src->active == 0U)
        {
            continue;
        }

        dst->active = 1U;
        dst->alarm_id = src->alarm_id;
        dst->reason = (record_service_alarm_reason_t)src->reason;
        dst->source_index = src->source_index;
        dst->start_timestamp_ms = src->start_timestamp_ms;
        dst->start_time_valid = src->start_time_valid;
        dst->start_datetime = src->start_datetime;
        dst->ntc_temp_milli_c = src->ntc_temp_milli_c;
        dst->digital_input_state = src->digital_input_state;

        if (dst->alarm_id > max_alarm_id)
        {
            max_alarm_id = dst->alarm_id;
        }
        alarm_service_ctx.total_started++;
    }

    alarm_service_ctx.next_alarm_id = image.next_alarm_id;
    if (alarm_service_ctx.next_alarm_id <= max_alarm_id)
    {
        alarm_service_ctx.next_alarm_id = max_alarm_id + 1U;
    }
    if (alarm_service_ctx.next_alarm_id == 0U)
    {
        alarm_service_ctx.next_alarm_id = 1U;
    }
    alarm_service_ctx.image_restored = (max_alarm_id > 0U) ? RT_TRUE : RT_FALSE;
}

static rt_err_t alarm_service_lock_take(void)
{
    if (alarm_service_ctx.lock == RT_NULL)
    {
        return -RT_ERROR;
    }

    return rt_mutex_take(alarm_service_ctx.lock, RT_WAITING_FOREVER);
}

static void alarm_service_lock_release(void)
{
    if (alarm_service_ctx.lock != RT_NULL)
    {
        rt_mutex_release(alarm_service_ctx.lock);
    }
}

static rt_uint32_t alarm_service_next_id(void)
{
    rt_uint32_t id = alarm_service_ctx.next_alarm_id++;

    if (alarm_service_ctx.next_alarm_id == 0U)
    {
        alarm_service_ctx.next_alarm_id = 1U;
    }

    return id;
}

static void alarm_service_ntc_pending_clear(void)
{
    alarm_service_ctx.ntc_pending_active = 0U;
    alarm_service_ctx.ntc_pending_reason = RECORD_SERVICE_ALARM_REASON_NONE;
    alarm_service_ctx.ntc_pending_start_ms = 0U;
}

static void alarm_service_ti_pending_clear(rt_size_t index)
{
    if (index >= 5U)
    {
        return;
    }

    alarm_service_ctx.ti_pending_active[index] = 0U;
    alarm_service_ctx.ti_pending_target_active[index] = 0U;
    alarm_service_ctx.ti_pending_start_ms[index] = 0U;
}

static void alarm_service_fill_common_record(record_service_item_t *record,
                                             rt_uint32_t alarm_id,
                                             record_service_alarm_reason_t reason,
                                             rt_uint8_t source_index,
                                             record_service_alarm_action_t action,
                                             const alarm_service_input_t *input)
{
    if (record == RT_NULL || input == RT_NULL)
    {
        return;
    }

    memset(record, 0, sizeof(*record));
    record->type = RECORD_SERVICE_EVENT_ALARM;
    record->alarm_id = alarm_id;
    record->alarm_reason = reason;
    record->event_action = action;
    record->source_index = source_index;
    record->timestamp_ms = input->timestamp_ms;
    record->time_valid = input->time_valid;
    record->datetime = input->datetime;
    record->ntc_temp_milli_c = input->ntc_temp_milli_c;
    record->digital_input_state = input->digital_input_state;
}

static rt_err_t alarm_service_start_slot(rt_size_t index,
                                         record_service_alarm_reason_t reason,
                                         rt_uint8_t source_index,
                                         const alarm_service_input_t *input)
{
    alarm_service_slot_t *slot;
    record_service_item_t record;
    rt_err_t result;

    if (index >= ALARM_SERVICE_ACTIVE_CAPACITY || input == RT_NULL)
    {
        return -RT_EINVAL;
    }

    slot = &alarm_service_ctx.slots[index];
    if (slot->active != 0U)
    {
        return RT_EOK;
    }

    slot->active = 1U;
    slot->alarm_id = alarm_service_next_id();
    slot->reason = reason;
    slot->source_index = source_index;
    slot->start_timestamp_ms = input->timestamp_ms;
    slot->start_time_valid = input->time_valid;
    slot->start_datetime = input->datetime;
    slot->ntc_temp_milli_c = input->ntc_temp_milli_c;
    slot->digital_input_state = input->digital_input_state;

    alarm_service_fill_common_record(&record,
                                     slot->alarm_id,
                                     slot->reason,
                                     slot->source_index,
                                     RECORD_SERVICE_ALARM_ACTION_START,
                                     input);
    record.alarm_state = DATA_SERVICE_ALARM_ACTIVE;
    result = record_service_append_alarm_record(&record);
    RT_UNUSED(result);

    alarm_service_ctx.total_started++;
    alarm_service_request_image_save();

    return RT_EOK;
}

static rt_err_t alarm_service_finish_slot(rt_size_t index, const alarm_service_input_t *input)
{
    alarm_service_slot_t slot;
    record_service_item_t record;
    rt_err_t result;

    if (index >= ALARM_SERVICE_ACTIVE_CAPACITY || input == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (alarm_service_ctx.slots[index].active == 0U)
    {
        return RT_EOK;
    }

    slot = alarm_service_ctx.slots[index];
    alarm_service_fill_common_record(&record,
                                     slot.alarm_id,
                                     slot.reason,
                                     slot.source_index,
                                     RECORD_SERVICE_ALARM_ACTION_CLEAR,
                                     input);
    record.duration_ms = alarm_service_elapsed_ms(slot.start_timestamp_ms,
                                                  input->timestamp_ms,
                                                  slot.start_time_valid,
                                                  &slot.start_datetime,
                                                  input->time_valid,
                                                  &input->datetime);
    record.alarm_state = DATA_SERVICE_ALARM_NORMAL;

    result = record_service_append_alarm_record(&record);
    RT_UNUSED(result);

    memset(&alarm_service_ctx.slots[index], 0, sizeof(alarm_service_ctx.slots[index]));
    alarm_service_ctx.total_ended++;
    alarm_service_request_image_save();

    return RT_EOK;
}

rt_err_t alarm_service_init(void)
{
    if (alarm_service_ctx.ready != RT_FALSE)
    {
        return RT_EOK;
    }

    memset(&alarm_service_ctx, 0, sizeof(alarm_service_ctx));
    alarm_service_ctx.lock = rt_mutex_create("alarm_svc", RT_IPC_FLAG_PRIO);
    if (alarm_service_ctx.lock == RT_NULL)
    {
        return -RT_ENOMEM;
    }
    alarm_service_ctx.next_alarm_id = 1U;
    alarm_service_ctx.persistent_ready =
        (storage_flashdb_init() == RT_EOK) ? RT_TRUE : RT_FALSE;
    alarm_service_restore_active_image();
    if (alarm_service_start_image_thread() != RT_EOK)
    {
        alarm_service_ctx.persistent_ready = RT_FALSE;
    }
    alarm_service_ctx.ready = RT_TRUE;

    return RT_EOK;
}

rt_err_t alarm_service_update(const alarm_service_input_t *input)
{
    rt_int32_t ti_enable_mask = 0;
    rt_int32_t ti_open_level_mask = 0;
    rt_int32_t ntc_sensor_enable = 1;
    rt_int32_t ntc_high_alarm_enable = 0;
    rt_int32_t ntc_high_threshold_mc = ALARM_SERVICE_NTC_HIGH_THRESHOLD_MC;
    rt_int32_t ntc_low_alarm_enable = 0;
    rt_int32_t ntc_low_threshold_mc = ALARM_SERVICE_NTC_LOW_THRESHOLD_MC;
    rt_int32_t ti_alarm_debounce_ms = 0;
    rt_int32_t ntc_alarm_debounce_ms = 0;
    rt_uint8_t ti_raw_state;
    rt_uint8_t ntc_valid;
    rt_uint8_t di_valid;
    rt_size_t i;
    rt_err_t result;

    if (input == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (alarm_service_ctx.ready == RT_FALSE)
    {
        result = alarm_service_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    ntc_valid = ((input->sensor_valid_flags & DATA_SERVICE_SENSOR_NTC_VALID) != 0U) ? 1U : 0U;
    di_valid = ((input->sensor_valid_flags & DATA_SERVICE_SENSOR_DI_VALID) != 0U) ? 1U : 0U;

    (void)param_service_get_i32(PARAM_SERVICE_ID_TI_ALARM_ENABLE_MASK, &ti_enable_mask);
    (void)param_service_get_i32(PARAM_SERVICE_ID_TI_ALARM_OPEN_LEVEL_MASK, &ti_open_level_mask);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_SENSOR_ENABLE, &ntc_sensor_enable);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_HIGH_ALARM_ENABLE, &ntc_high_alarm_enable);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_HIGH_ALARM_THRESHOLD_MC, &ntc_high_threshold_mc);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_LOW_ALARM_ENABLE, &ntc_low_alarm_enable);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_LOW_ALARM_THRESHOLD_MC, &ntc_low_threshold_mc);
    (void)param_service_get_i32(PARAM_SERVICE_ID_TI_ALARM_DEBOUNCE_MS, &ti_alarm_debounce_ms);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_ALARM_DEBOUNCE_MS, &ntc_alarm_debounce_ms);

    ti_enable_mask &= ALARM_SERVICE_DI_MONITOR_MASK;
    ti_open_level_mask &= ALARM_SERVICE_DI_MONITOR_MASK;

    result = alarm_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    if (ntc_sensor_enable == 0 || ntc_valid == 0U)
    {
        alarm_service_ntc_pending_clear();
        (void)alarm_service_finish_slot(0U, input);
    }
    else
    {
        if (alarm_service_ctx.slots[0].active == 0U)
        {
            record_service_alarm_reason_t pending_reason = RECORD_SERVICE_ALARM_REASON_NONE;

            if (ntc_high_alarm_enable != 0 &&
                input->ntc_temp_milli_c >= ntc_high_threshold_mc)
            {
                pending_reason = RECORD_SERVICE_ALARM_REASON_NTC_HIGH;
            }
            else if (ntc_low_alarm_enable != 0 &&
                     input->ntc_temp_milli_c <= ntc_low_threshold_mc)
            {
                pending_reason = RECORD_SERVICE_ALARM_REASON_NTC_LOW;
            }

            if (pending_reason == RECORD_SERVICE_ALARM_REASON_NONE)
            {
                alarm_service_ntc_pending_clear();
            }
            else if (ntc_alarm_debounce_ms <= 0)
            {
                alarm_service_ntc_pending_clear();
                (void)alarm_service_start_slot(0U,
                                               pending_reason,
                                               0U,
                                               input);
            }
            else
            {
                if (alarm_service_ctx.ntc_pending_active == 0U ||
                    alarm_service_ctx.ntc_pending_reason != pending_reason)
                {
                    alarm_service_ctx.ntc_pending_active = 1U;
                    alarm_service_ctx.ntc_pending_reason = pending_reason;
                    alarm_service_ctx.ntc_pending_start_ms = input->timestamp_ms;
                }
                else if ((input->timestamp_ms - alarm_service_ctx.ntc_pending_start_ms) >=
                         (rt_uint32_t)ntc_alarm_debounce_ms)
                {
                    alarm_service_ntc_pending_clear();
                    (void)alarm_service_start_slot(0U,
                                                   pending_reason,
                                                   0U,
                                                   input);
                }
            }
        }
        else if (alarm_service_ctx.slots[0].reason == RECORD_SERVICE_ALARM_REASON_NTC_HIGH &&
                 (ntc_high_alarm_enable == 0 ||
                  input->ntc_temp_milli_c < ntc_high_threshold_mc))
        {
            if (ntc_alarm_debounce_ms <= 0 ||
                ntc_high_alarm_enable == 0)
            {
                alarm_service_ntc_pending_clear();
                (void)alarm_service_finish_slot(0U, input);
            }
            else if (alarm_service_ctx.ntc_pending_active == 0U ||
                     alarm_service_ctx.ntc_pending_reason != RECORD_SERVICE_ALARM_REASON_NONE)
            {
                alarm_service_ctx.ntc_pending_active = 1U;
                alarm_service_ctx.ntc_pending_reason = RECORD_SERVICE_ALARM_REASON_NONE;
                alarm_service_ctx.ntc_pending_start_ms = input->timestamp_ms;
            }
            else if ((input->timestamp_ms - alarm_service_ctx.ntc_pending_start_ms) >=
                     (rt_uint32_t)ntc_alarm_debounce_ms)
            {
                alarm_service_ntc_pending_clear();
                (void)alarm_service_finish_slot(0U, input);
            }
        }
        else if (alarm_service_ctx.slots[0].reason == RECORD_SERVICE_ALARM_REASON_NTC_LOW &&
                 (ntc_low_alarm_enable == 0 ||
                  input->ntc_temp_milli_c > ntc_low_threshold_mc))
        {
            if (ntc_alarm_debounce_ms <= 0 ||
                ntc_low_alarm_enable == 0)
            {
                alarm_service_ntc_pending_clear();
                (void)alarm_service_finish_slot(0U, input);
            }
            else if (alarm_service_ctx.ntc_pending_active == 0U ||
                     alarm_service_ctx.ntc_pending_reason != RECORD_SERVICE_ALARM_REASON_NONE)
            {
                alarm_service_ctx.ntc_pending_active = 1U;
                alarm_service_ctx.ntc_pending_reason = RECORD_SERVICE_ALARM_REASON_NONE;
                alarm_service_ctx.ntc_pending_start_ms = input->timestamp_ms;
            }
            else if ((input->timestamp_ms - alarm_service_ctx.ntc_pending_start_ms) >=
                     (rt_uint32_t)ntc_alarm_debounce_ms)
            {
                alarm_service_ntc_pending_clear();
                (void)alarm_service_finish_slot(0U, input);
            }
        }
        else
        {
            alarm_service_ntc_pending_clear();
        }
    }

    if (di_valid != 0U)
    {
        ti_raw_state = input->digital_input_state & ALARM_SERVICE_DI_MONITOR_MASK;
        for (i = 0U; i < 5U; i++)
        {
            rt_size_t slot_index = ALARM_SERVICE_DI_REASON_BASE_INDEX + i;
            rt_uint8_t bit = (rt_uint8_t)(1U << i);
            rt_uint8_t ti_enabled = ((rt_uint8_t)ti_enable_mask & bit) != 0U ? 1U : 0U;
            rt_uint8_t ti_raw_high = (ti_raw_state & bit) != 0U ? 1U : 0U;
            rt_uint8_t ti_open_high = ((rt_uint8_t)ti_open_level_mask & bit) != 0U ? 1U : 0U;
            rt_uint8_t ti_open = (ti_raw_high == ti_open_high) ? 1U : 0U;
            rt_uint8_t target_active = (ti_enabled != 0U && ti_open != 0U) ? 1U : 0U;
            rt_uint8_t current_active =
                (alarm_service_ctx.slots[slot_index].active != 0U) ? 1U : 0U;

            if (target_active == current_active)
            {
                alarm_service_ti_pending_clear(i);
            }
            else if (target_active == 0U && ti_enabled == 0U)
            {
                alarm_service_ti_pending_clear(i);
                (void)alarm_service_finish_slot(slot_index, input);
            }
            else if (ti_alarm_debounce_ms <= 0)
            {
                alarm_service_ti_pending_clear(i);
                if (target_active != 0U)
                {
                    record_service_alarm_reason_t reason =
                        (record_service_alarm_reason_t)(RECORD_SERVICE_ALARM_REASON_TI1_OPEN + i);

                    (void)alarm_service_start_slot(slot_index,
                                                   reason,
                                                   (rt_uint8_t)(i + 1U),
                                                   input);
                }
                else
                {
                    (void)alarm_service_finish_slot(slot_index, input);
                }
            }
            else
            {
                if (alarm_service_ctx.ti_pending_active[i] == 0U ||
                    alarm_service_ctx.ti_pending_target_active[i] != target_active)
                {
                    alarm_service_ctx.ti_pending_active[i] = 1U;
                    alarm_service_ctx.ti_pending_target_active[i] = target_active;
                    alarm_service_ctx.ti_pending_start_ms[i] = input->timestamp_ms;
                }
                else if ((input->timestamp_ms - alarm_service_ctx.ti_pending_start_ms[i]) >=
                         (rt_uint32_t)ti_alarm_debounce_ms)
                {
                    alarm_service_ti_pending_clear(i);
                    if (target_active != 0U)
                    {
                        record_service_alarm_reason_t reason =
                            (record_service_alarm_reason_t)(RECORD_SERVICE_ALARM_REASON_TI1_OPEN + i);

                        (void)alarm_service_start_slot(slot_index,
                                                       reason,
                                                       (rt_uint8_t)(i + 1U),
                                                       input);
                    }
                    else
                    {
                        (void)alarm_service_finish_slot(slot_index, input);
                    }
                }
            }
        }
    }
    else
    {
        for (i = 0U; i < 5U; i++)
        {
            alarm_service_ti_pending_clear(i);
        }
    }

    alarm_service_lock_release();

    return RT_EOK;
}

rt_err_t alarm_service_get_snapshot(alarm_service_snapshot_t *snapshot)
{
    rt_size_t i;
    rt_size_t out_index = 0U;
    rt_uint32_t now_ms;
    rt_err_t result;

    if (snapshot == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (alarm_service_ctx.ready == RT_FALSE)
    {
        result = alarm_service_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    memset(snapshot, 0, sizeof(*snapshot));
    now_ms = rt_tick_get_millisecond();

    result = alarm_service_lock_take();
    if (result != RT_EOK)
    {
        return result;
    }

    snapshot->total_started = alarm_service_ctx.total_started;
    snapshot->total_ended = alarm_service_ctx.total_ended;
    snapshot->persistent_ready = (alarm_service_ctx.persistent_ready == RT_TRUE) ? 1U : 0U;
    snapshot->image_restored = (alarm_service_ctx.image_restored == RT_TRUE) ? 1U : 0U;
    snapshot->image_write_count = alarm_service_ctx.image_write_count;
    snapshot->image_write_failed = alarm_service_ctx.image_write_failed;
    snapshot->ntc_pending = alarm_service_ctx.ntc_pending_active;
    snapshot->ntc_pending_reason = alarm_service_ctx.ntc_pending_reason;
    snapshot->ntc_pending_duration_ms =
        (alarm_service_ctx.ntc_pending_active != 0U) ?
        (now_ms - alarm_service_ctx.ntc_pending_start_ms) : 0U;

    for (i = 0U; i < 5U; i++)
    {
        rt_uint8_t bit = (rt_uint8_t)(1U << i);

        if (alarm_service_ctx.ti_pending_active[i] != 0U)
        {
            snapshot->ti_pending_mask |= bit;
            if (alarm_service_ctx.ti_pending_target_active[i] != 0U)
            {
                snapshot->ti_pending_target_active_mask |= bit;
            }
            snapshot->ti_pending_duration_ms[i] =
                now_ms - alarm_service_ctx.ti_pending_start_ms[i];
        }
    }

    for (i = 0U; i < ALARM_SERVICE_ACTIVE_CAPACITY; i++)
    {
        const alarm_service_slot_t *slot = &alarm_service_ctx.slots[i];
        if (slot->active == 0U || out_index >= ALARM_SERVICE_ACTIVE_CAPACITY)
        {
            continue;
        }

        snapshot->active_items[out_index].active = 1U;
        snapshot->active_items[out_index].alarm_id = slot->alarm_id;
        snapshot->active_items[out_index].reason = slot->reason;
        snapshot->active_items[out_index].source_index = slot->source_index;
        snapshot->active_items[out_index].start_timestamp_ms = slot->start_timestamp_ms;
        snapshot->active_items[out_index].duration_ms =
            alarm_service_active_duration_ms(slot, now_ms);
        snapshot->active_items[out_index].time_valid = slot->start_time_valid;
        snapshot->active_items[out_index].start_datetime = slot->start_datetime;
        out_index++;
    }

    snapshot->active_count = out_index;
    alarm_service_lock_release();

    return RT_EOK;
}
