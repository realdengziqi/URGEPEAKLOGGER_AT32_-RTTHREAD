#ifndef APP_ALARM_SERVICE_H
#define APP_ALARM_SERVICE_H

#include <rtthread.h>

#include "app_record_service.h"
#include "app_rtc.h"

#define ALARM_SERVICE_ACTIVE_CAPACITY    6U

typedef struct
{
    rt_uint32_t timestamp_ms;
    rt_uint8_t time_valid;
    app_rtc_datetime_t datetime;
    rt_uint8_t sensor_valid_flags;
    rt_int32_t ntc_temp_milli_c;
    rt_uint8_t digital_input_state;
} alarm_service_input_t;

typedef struct
{
    rt_uint8_t active;
    rt_uint32_t alarm_id;
    record_service_alarm_reason_t reason;
    rt_uint8_t source_index;
    rt_uint32_t start_timestamp_ms;
    rt_uint32_t duration_ms;
    rt_uint8_t time_valid;
    app_rtc_datetime_t start_datetime;
} alarm_service_active_item_t;

typedef struct
{
    rt_size_t active_count;
    rt_uint8_t ntc_pending;
    record_service_alarm_reason_t ntc_pending_reason;
    rt_uint32_t ntc_pending_duration_ms;
    rt_uint8_t ti_pending_mask;
    rt_uint8_t ti_pending_target_active_mask;
    rt_uint32_t ti_pending_duration_ms[5];
    rt_uint32_t total_started;
    rt_uint32_t total_ended;
    rt_uint8_t persistent_ready;
    rt_uint8_t image_restored;
    rt_uint32_t image_write_count;
    rt_uint32_t image_write_failed;
    alarm_service_active_item_t active_items[ALARM_SERVICE_ACTIVE_CAPACITY];
} alarm_service_snapshot_t;

rt_err_t alarm_service_init(void);
rt_err_t alarm_service_update(const alarm_service_input_t *input);
rt_err_t alarm_service_get_snapshot(alarm_service_snapshot_t *snapshot);

#endif
