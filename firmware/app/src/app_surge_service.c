#include "app_surge_service.h"

#define SURGE_SERVICE_DEFAULT_BASELINE_RAW    2048U
#define SURGE_SERVICE_DEFAULT_THRESHOLD_RAW   300U
#define SURGE_SERVICE_MAX_ADC_RAW             4095U

static surge_service_snapshot_t surge_service_ctx;
static rt_bool_t surge_service_ready = RT_FALSE;

static rt_uint16_t surge_abs_delta(rt_uint16_t a, rt_uint16_t b)
{
    return (a >= b) ? (rt_uint16_t)(a - b) : (rt_uint16_t)(b - a);
}

static void surge_service_process_raw(rt_uint16_t raw,
                                      rt_uint32_t timestamp_ms,
                                      surge_service_source_t source)
{
    rt_uint16_t delta;

    if (surge_service_ready == RT_FALSE)
    {
        surge_service_init();
    }

    if (raw > SURGE_SERVICE_MAX_ADC_RAW)
    {
        raw = SURGE_SERVICE_MAX_ADC_RAW;
    }

    surge_service_ctx.current_raw = raw;
    surge_service_ctx.source = source;
    surge_service_ctx.last_update_ms = timestamp_ms;

    if (surge_service_ctx.status == SURGE_SERVICE_STATUS_IDLE)
    {
        return;
    }

    delta = surge_abs_delta(raw, surge_service_ctx.baseline_raw);
    if (delta > surge_service_ctx.peak_delta_raw)
    {
        surge_service_ctx.peak_delta_raw = delta;
        surge_service_ctx.peak_raw = raw;
    }

    if (surge_service_ctx.status == SURGE_SERVICE_STATUS_ARMED &&
        delta >= surge_service_ctx.threshold_delta_raw)
    {
        surge_service_ctx.status = SURGE_SERVICE_STATUS_LATCHED;
        surge_service_ctx.trigger_count++;
        surge_service_ctx.last_trigger_ms = timestamp_ms;
    }
}

void surge_service_init(void)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (surge_service_ready == RT_FALSE)
    {
        surge_service_ctx.status = SURGE_SERVICE_STATUS_IDLE;
        surge_service_ctx.source = SURGE_SERVICE_SOURCE_ADC;
        surge_service_ctx.current_raw = SURGE_SERVICE_DEFAULT_BASELINE_RAW;
        surge_service_ctx.baseline_raw = SURGE_SERVICE_DEFAULT_BASELINE_RAW;
        surge_service_ctx.peak_raw = SURGE_SERVICE_DEFAULT_BASELINE_RAW;
        surge_service_ctx.peak_delta_raw = 0U;
        surge_service_ctx.threshold_delta_raw = SURGE_SERVICE_DEFAULT_THRESHOLD_RAW;
        surge_service_ctx.trigger_count = 0U;
        surge_service_ctx.last_update_ms = 0U;
        surge_service_ctx.last_trigger_ms = 0U;
        surge_service_ready = RT_TRUE;
    }
    rt_hw_interrupt_enable(level);
}

void surge_service_arm(void)
{
    rt_base_t level;

    if (surge_service_ready == RT_FALSE)
    {
        surge_service_init();
    }

    level = rt_hw_interrupt_disable();
    surge_service_ctx.status = SURGE_SERVICE_STATUS_ARMED;
    surge_service_ctx.baseline_raw = surge_service_ctx.current_raw;
    surge_service_ctx.peak_raw = surge_service_ctx.current_raw;
    surge_service_ctx.peak_delta_raw = 0U;
    rt_hw_interrupt_enable(level);
}

void surge_service_clear(void)
{
    rt_base_t level;

    if (surge_service_ready == RT_FALSE)
    {
        surge_service_init();
    }

    level = rt_hw_interrupt_disable();
    surge_service_ctx.status = SURGE_SERVICE_STATUS_IDLE;
    surge_service_ctx.baseline_raw = surge_service_ctx.current_raw;
    surge_service_ctx.peak_raw = surge_service_ctx.current_raw;
    surge_service_ctx.peak_delta_raw = 0U;
    surge_service_ctx.last_trigger_ms = 0U;
    rt_hw_interrupt_enable(level);
}

rt_err_t surge_service_set_threshold(rt_uint16_t threshold_delta_raw)
{
    rt_base_t level;

    if (threshold_delta_raw == 0U || threshold_delta_raw > SURGE_SERVICE_MAX_ADC_RAW)
    {
        return -RT_EINVAL;
    }

    if (surge_service_ready == RT_FALSE)
    {
        surge_service_init();
    }

    level = rt_hw_interrupt_disable();
    surge_service_ctx.threshold_delta_raw = threshold_delta_raw;
    rt_hw_interrupt_enable(level);
    return RT_EOK;
}

void surge_service_update_adc_raw(rt_uint16_t raw, rt_uint32_t timestamp_ms)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    surge_service_process_raw(raw, timestamp_ms, SURGE_SERVICE_SOURCE_ADC);
    rt_hw_interrupt_enable(level);
}

void surge_service_inject_raw(rt_uint16_t raw, rt_uint32_t timestamp_ms)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    surge_service_process_raw(raw, timestamp_ms, SURGE_SERVICE_SOURCE_INJECT);
    rt_hw_interrupt_enable(level);
}

void surge_service_get_snapshot(surge_service_snapshot_t *snapshot)
{
    rt_base_t level;

    if (snapshot == RT_NULL)
    {
        return;
    }

    if (surge_service_ready == RT_FALSE)
    {
        surge_service_init();
    }

    level = rt_hw_interrupt_disable();
    *snapshot = surge_service_ctx;
    rt_hw_interrupt_enable(level);
}

const char *surge_service_status_name(surge_service_status_t status)
{
    switch (status)
    {
    case SURGE_SERVICE_STATUS_IDLE:
        return "idle";
    case SURGE_SERVICE_STATUS_ARMED:
        return "armed";
    case SURGE_SERVICE_STATUS_LATCHED:
        return "latched";
    default:
        return "unknown";
    }
}
