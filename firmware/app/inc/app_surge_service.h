#ifndef APP_SURGE_SERVICE_H
#define APP_SURGE_SERVICE_H

#include <rtthread.h>

typedef enum
{
    SURGE_SERVICE_STATUS_IDLE = 0,
    SURGE_SERVICE_STATUS_ARMED = 1,
    SURGE_SERVICE_STATUS_LATCHED = 2
} surge_service_status_t;

typedef enum
{
    SURGE_SERVICE_SOURCE_ADC = 0,
    SURGE_SERVICE_SOURCE_INJECT = 1
} surge_service_source_t;

typedef struct
{
    surge_service_status_t status;
    surge_service_source_t source;
    rt_uint16_t current_raw;
    rt_uint16_t baseline_raw;
    rt_uint16_t peak_raw;
    rt_uint16_t peak_delta_raw;
    rt_uint16_t threshold_delta_raw;
    rt_uint32_t trigger_count;
    rt_uint32_t last_update_ms;
    rt_uint32_t last_trigger_ms;
} surge_service_snapshot_t;

void surge_service_init(void);
void surge_service_arm(void);
void surge_service_clear(void);
rt_err_t surge_service_set_threshold(rt_uint16_t threshold_delta_raw);
void surge_service_update_adc_raw(rt_uint16_t raw, rt_uint32_t timestamp_ms);
void surge_service_inject_raw(rt_uint16_t raw, rt_uint32_t timestamp_ms);
void surge_service_get_snapshot(surge_service_snapshot_t *snapshot);
const char *surge_service_status_name(surge_service_status_t status);

#endif
