#include "app_data_service.h"

#include "sensor_hdc1080.h"

#define DATA_SERVICE_THREAD_STACK_SIZE   768U
#define DATA_SERVICE_THREAD_PRIORITY     18U
#define DATA_SERVICE_THREAD_TIMESLICE    10U
#define DATA_SERVICE_UPDATE_PERIOD_MS    200U
#define DATA_SERVICE_RTC_READ_PERIOD_MS  1000U
#define DATA_SERVICE_ENV_READ_PERIOD_MS  1000U
#define DATA_SERVICE_ENV_FILTER_SHIFT    3U
#define DATA_SERVICE_HUMI_MIN_MPERMIL    0
#define DATA_SERVICE_HUMI_MAX_MPERMIL    100000

static device_data_snapshot_t data_service_snapshot;
static rt_thread_t data_service_thread = RT_NULL;

static rt_int32_t data_service_clamp_i32(rt_int32_t value, rt_int32_t min_value, rt_int32_t max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static rt_int32_t data_service_filter_i32(rt_int32_t current, rt_int32_t input)
{
    return current + ((input - current) / (rt_int32_t)(1U << DATA_SERVICE_ENV_FILTER_SHIFT));
}

static void data_service_snapshot_write(const device_data_snapshot_t *snapshot)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    data_service_snapshot = *snapshot;
    rt_hw_interrupt_enable(level);
}

static void data_service_thread_entry(void *parameter)
{
    device_data_snapshot_t local_snapshot;
    rt_uint32_t last_rtc_read_ms = 0U;
    rt_uint32_t last_env_read_ms = 0U;
    app_rtc_datetime_t datetime;
    sensor_hdc1080_sample_t env_sample;
    rt_bool_t env_filter_ready = RT_FALSE;

    RT_UNUSED(parameter);

    local_snapshot = data_service_snapshot;
    if (sensor_hdc1080_init() == RT_EOK)
    {
        local_snapshot.sensor_valid_flags |= DATA_SERVICE_SENSOR_HDC1080_VALID;
    }
    else
    {
        local_snapshot.sensor_valid_flags &= (rt_uint8_t)~DATA_SERVICE_SENSOR_HDC1080_VALID;
    }

    while (1)
    {
        local_snapshot.timestamp_ms = rt_tick_get_millisecond();
        local_snapshot.update_count++;

        if ((local_snapshot.time_valid == 0U) ||
            ((local_snapshot.timestamp_ms - last_rtc_read_ms) >= DATA_SERVICE_RTC_READ_PERIOD_MS))
        {
            last_rtc_read_ms = local_snapshot.timestamp_ms;
            if (app_rtc_get_datetime(&datetime) == RT_EOK)
            {
                local_snapshot.datetime = datetime;
                local_snapshot.time_valid = 1U;
            }
            else
            {
                local_snapshot.time_valid = 0U;
            }
        }

        if ((local_snapshot.timestamp_ms - last_env_read_ms) >= DATA_SERVICE_ENV_READ_PERIOD_MS)
        {
            last_env_read_ms = local_snapshot.timestamp_ms;
            if (sensor_hdc1080_read_sample(&env_sample) == RT_EOK)
            {
                rt_int32_t temp_offset_mc = 0;
                rt_int32_t humi_offset_mpermil = 0;

                (void)param_service_get_i32(PARAM_SERVICE_ID_ENV_TEMP_OFFSET_MC, &temp_offset_mc);
                (void)param_service_get_i32(PARAM_SERVICE_ID_ENV_HUMI_OFFSET_MPERMIL, &humi_offset_mpermil);

                local_snapshot.env_temp_raw_milli_c = env_sample.temperature_milli_c;
                local_snapshot.env_humi_raw_milli_percent = env_sample.humidity_milli_percent;
                local_snapshot.env_temp_calibrated_milli_c =
                    env_sample.temperature_milli_c + temp_offset_mc;
                local_snapshot.env_humi_calibrated_milli_percent =
                    data_service_clamp_i32(env_sample.humidity_milli_percent + humi_offset_mpermil,
                                           DATA_SERVICE_HUMI_MIN_MPERMIL,
                                           DATA_SERVICE_HUMI_MAX_MPERMIL);

                if (env_filter_ready == RT_FALSE)
                {
                    local_snapshot.env_temp_milli_c = local_snapshot.env_temp_calibrated_milli_c;
                    local_snapshot.env_humi_milli_percent = local_snapshot.env_humi_calibrated_milli_percent;
                    env_filter_ready = RT_TRUE;
                }
                else
                {
                    local_snapshot.env_temp_milli_c =
                        data_service_filter_i32(local_snapshot.env_temp_milli_c,
                                                local_snapshot.env_temp_calibrated_milli_c);
                    local_snapshot.env_humi_milli_percent =
                        data_service_filter_i32(local_snapshot.env_humi_milli_percent,
                                                local_snapshot.env_humi_calibrated_milli_percent);
                }

                local_snapshot.sensor_valid_flags |= DATA_SERVICE_SENSOR_HDC1080_VALID;
            }
            else
            {
                local_snapshot.sensor_valid_flags &= (rt_uint8_t)~DATA_SERVICE_SENSOR_HDC1080_VALID;
            }
        }

        local_snapshot.surge_peak_value_milli_kv += 100;
        if (local_snapshot.surge_peak_value_milli_kv > 99900)
        {
            local_snapshot.surge_peak_value_milli_kv = 0;
        }

        local_snapshot.ntc_temp_milli_c = 25300 + (rt_int32_t)((local_snapshot.update_count % 20U) * 10U);
        local_snapshot.alarm_state = DATA_SERVICE_ALARM_NORMAL;
        local_snapshot.rs485_online = 1U;
        local_snapshot.tcp_connected = 0U;
        if (param_service_get_status(&local_snapshot.param_status) != RT_EOK)
        {
            local_snapshot.param_status.version = 0U;
            local_snapshot.param_status.last_update_ms = 0U;
            local_snapshot.param_status.dirty = 0U;
            local_snapshot.param_status.persistent_ready = 0U;
            local_snapshot.param_status.count = 0U;
        }

        data_service_snapshot_write(&local_snapshot);
        rt_thread_mdelay(DATA_SERVICE_UPDATE_PERIOD_MS);
    }
}

rt_err_t data_service_init(void)
{
    data_service_snapshot.timestamp_ms = 0U;
    data_service_snapshot.update_count = 0U;
    data_service_snapshot.time_valid = 0U;
    data_service_snapshot.datetime.year = 2000U;
    data_service_snapshot.datetime.month = 1U;
    data_service_snapshot.datetime.day = 1U;
    data_service_snapshot.datetime.weekday = 6U;
    data_service_snapshot.datetime.hour = 0U;
    data_service_snapshot.datetime.minute = 0U;
    data_service_snapshot.datetime.second = 0U;
    data_service_snapshot.surge_peak_value_milli_kv = 0;
    data_service_snapshot.ntc_temp_milli_c = 25300;
    data_service_snapshot.env_temp_raw_milli_c = 25300;
    data_service_snapshot.env_humi_raw_milli_percent = 45000;
    data_service_snapshot.env_temp_calibrated_milli_c = 25300;
    data_service_snapshot.env_humi_calibrated_milli_percent = 45000;
    data_service_snapshot.env_temp_milli_c = 25300;
    data_service_snapshot.env_humi_milli_percent = 45000;
    data_service_snapshot.sensor_valid_flags = 0U;
    data_service_snapshot.alarm_state = DATA_SERVICE_ALARM_NORMAL;
    data_service_snapshot.rs485_online = 1U;
    data_service_snapshot.tcp_connected = 0U;
    data_service_snapshot.param_status.version = 0U;
    data_service_snapshot.param_status.last_update_ms = 0U;
    data_service_snapshot.param_status.dirty = 0U;
    data_service_snapshot.param_status.persistent_ready = 0U;
    data_service_snapshot.param_status.count = 0U;

    if (data_service_thread != RT_NULL)
    {
        return RT_EOK;
    }

    if (app_rtc_init() != RT_EOK)
    {
        data_service_snapshot.time_valid = 0U;
    }

    data_service_thread = rt_thread_create("data_svc",
                                           data_service_thread_entry,
                                           RT_NULL,
                                           DATA_SERVICE_THREAD_STACK_SIZE,
                                           DATA_SERVICE_THREAD_PRIORITY,
                                           DATA_SERVICE_THREAD_TIMESLICE);
    if (data_service_thread == RT_NULL)
    {
        return -RT_ERROR;
    }

    return rt_thread_startup(data_service_thread);
}

void data_service_get_snapshot(device_data_snapshot_t *snapshot)
{
    rt_base_t level;

    if (snapshot == RT_NULL)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    *snapshot = data_service_snapshot;
    rt_hw_interrupt_enable(level);
}

rt_err_t data_service_get_datetime(app_rtc_datetime_t *datetime)
{
    if (datetime == RT_NULL)
    {
        return -RT_EINVAL;
    }

    return app_rtc_get_datetime(datetime);
}

rt_err_t data_service_set_datetime(const app_rtc_datetime_t *datetime)
{
    rt_err_t result;
    device_data_snapshot_t snapshot;

    if (datetime == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = app_rtc_set_datetime(datetime);
    if (result != RT_EOK)
    {
        return result;
    }

    data_service_get_snapshot(&snapshot);
    if (app_rtc_get_datetime(&snapshot.datetime) == RT_EOK)
    {
        snapshot.time_valid = 1U;
    }
    else
    {
        snapshot.datetime = *datetime;
        snapshot.time_valid = 0U;
    }
    data_service_snapshot_write(&snapshot);

    return RT_EOK;
}
