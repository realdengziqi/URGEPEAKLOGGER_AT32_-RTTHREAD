#ifndef APP_RECORD_SERVICE_H
#define APP_RECORD_SERVICE_H

#include <rtthread.h>

#include "app_rtc.h"

#define RECORD_SERVICE_CAPACITY    16U
#define RECORD_SERVICE_FORMAT_VERSION    3U

typedef enum
{
    RECORD_SERVICE_EVENT_SURGE = 0,
    RECORD_SERVICE_EVENT_ALARM,
    RECORD_SERVICE_EVENT_MANUAL_TEST
} record_service_event_type_t;

typedef enum
{
    RECORD_SERVICE_ALARM_REASON_NONE = 0,
    RECORD_SERVICE_ALARM_REASON_NTC_HIGH = 1,
    RECORD_SERVICE_ALARM_REASON_NTC_LOW = 2,
    RECORD_SERVICE_ALARM_REASON_TI1_OPEN = 3,
    RECORD_SERVICE_ALARM_REASON_TI2_OPEN = 4,
    RECORD_SERVICE_ALARM_REASON_TI3_OPEN = 5,
    RECORD_SERVICE_ALARM_REASON_TI4_OPEN = 6,
    RECORD_SERVICE_ALARM_REASON_TI5_OPEN = 7
} record_service_alarm_reason_t;

typedef enum
{
    RECORD_SERVICE_ALARM_ACTION_NONE = 0,
    RECORD_SERVICE_ALARM_ACTION_START = 1,
    RECORD_SERVICE_ALARM_ACTION_CLEAR = 2,
    RECORD_SERVICE_ALARM_ACTION_INSTANT = 3
} record_service_alarm_action_t;

#define RECORD_SERVICE_SOURCE_NTC      0U
#define RECORD_SERVICE_SOURCE_TI1      1U
#define RECORD_SERVICE_SOURCE_TI2      2U
#define RECORD_SERVICE_SOURCE_TI3      3U
#define RECORD_SERVICE_SOURCE_TI4      4U
#define RECORD_SERVICE_SOURCE_TI5      5U
#define RECORD_SERVICE_SOURCE_SURGE    10U

typedef struct
{
    rt_uint32_t sequence;
    rt_uint16_t format_version;
    record_service_event_type_t type;
    rt_uint32_t alarm_id;
    record_service_alarm_reason_t alarm_reason;
    record_service_alarm_action_t event_action;
    rt_uint8_t source_index;
    rt_uint32_t timestamp_ms;
    rt_uint32_t duration_ms;
    rt_uint8_t time_valid;
    app_rtc_datetime_t datetime;
    rt_int32_t peak_milli_kv;
    rt_int32_t ntc_temp_milli_c;
    rt_int32_t env_temp_milli_c;
    rt_int32_t env_humi_milli_percent;
    rt_uint8_t digital_input_state;
    rt_uint8_t alarm_state;
} record_service_item_t;

typedef struct
{
    rt_uint32_t generation;
    rt_uint32_t next_sequence;
    rt_uint32_t total_written;
    rt_uint32_t persist_written;
    rt_uint32_t persist_failed;
    rt_uint32_t persist_dropped;
    rt_uint32_t persist_pending;
    rt_size_t capacity;
    rt_size_t count;
    rt_uint8_t persistent_ready;
} record_service_status_t;

rt_err_t record_service_init(void);
rt_err_t record_service_append_manual_test(rt_int32_t peak_milli_kv);
rt_err_t record_service_append_snapshot(record_service_event_type_t type);
rt_err_t record_service_append_alarm_record(const record_service_item_t *alarm_item);
rt_err_t record_service_get_latest(record_service_item_t *item);
rt_err_t record_service_get_by_index(rt_size_t index, record_service_item_t *item);
rt_err_t record_service_query_latest_page(rt_size_t offset_from_latest,
                                          rt_size_t max_count,
                                          record_service_item_t *items,
                                          rt_size_t *actual_count);
rt_err_t record_service_query_page_by_sequence(rt_uint32_t start_sequence,
                                               rt_size_t max_count,
                                               record_service_item_t *items,
                                               rt_size_t *actual_count);
rt_err_t record_service_query_alarm_latest_page(rt_size_t offset_from_latest,
                                                rt_size_t max_count,
                                                record_service_item_t *items,
                                                rt_size_t *actual_count,
                                                rt_size_t *total_count);
rt_err_t record_service_get_status(record_service_status_t *status);
void record_service_clear(void);
const char *record_service_get_type_name(record_service_event_type_t type);
const char *record_service_get_alarm_reason_name(record_service_alarm_reason_t reason);
const char *record_service_get_alarm_action_name(record_service_alarm_action_t action);

#endif
