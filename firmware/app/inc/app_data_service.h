#ifndef APP_DATA_SERVICE_H
#define APP_DATA_SERVICE_H

#include <rtthread.h>

#include "app_rtc.h"
#include "app_param_service.h"

typedef enum
{
    DATA_SERVICE_ALARM_NORMAL = 0,
    DATA_SERVICE_ALARM_ACTIVE = 1
} data_service_alarm_state_t;

#define DATA_SERVICE_SENSOR_HDC1080_VALID    (1U << 0)
#define DATA_SERVICE_SENSOR_DI_VALID         (1U << 1)
#define DATA_SERVICE_SENSOR_ADC_VALID        (1U << 2)
#define DATA_SERVICE_SENSOR_NTC_VALID        (1U << 3)

typedef struct
{
    rt_uint32_t timestamp_ms;
    rt_uint32_t update_count;
    rt_uint8_t time_valid;
    app_rtc_datetime_t datetime;

    rt_int32_t surge_peak_value_milli_kv;
    rt_uint16_t surge_peak_raw;
    rt_uint16_t surge_peak_delta_raw;
    rt_uint16_t surge_threshold_delta_raw;
    rt_uint32_t surge_trigger_count;
    rt_uint8_t surge_status;
    rt_int32_t ntc_temp_milli_c;
    rt_uint16_t ntc_adc_raw;
    rt_uint16_t surge_adc_raw;
    rt_int32_t env_temp_raw_milli_c;
    rt_int32_t env_humi_raw_milli_percent;
    rt_int32_t env_temp_calibrated_milli_c;
    rt_int32_t env_humi_calibrated_milli_percent;
    rt_int32_t env_temp_milli_c;
    rt_int32_t env_humi_milli_percent;
    rt_uint8_t digital_input_raw_state;
    rt_uint8_t sensor_valid_flags;

    data_service_alarm_state_t alarm_state;
    rt_uint8_t rs485_online;
    rt_uint8_t tcp_connected;

    param_service_status_t param_status;
} device_data_snapshot_t;

rt_err_t data_service_init(void);
void data_service_get_snapshot(device_data_snapshot_t *snapshot);
rt_err_t data_service_get_datetime(app_rtc_datetime_t *datetime);
rt_err_t data_service_set_datetime(const app_rtc_datetime_t *datetime);

#endif
