#include "app_param_service.h"

#include <string.h>

typedef struct
{
    const char *name;
    rt_int32_t default_value;
    rt_int32_t min_value;
    rt_int32_t max_value;
    const char *unit;
} param_service_descriptor_t;

static const param_service_descriptor_t param_service_descriptors[PARAM_SERVICE_ID_COUNT] =
{
    {"modbus_addr", 1U, 1U, 247U, ""},
    {"modbus_baudrate", 9600U, 1200U, 115200U, "bps"},
    {"modbus_tcp_port", 502U, 1U, 65535U, ""},
    {"alarm_peak_milli_kv", 50000U, 0U, 99900U, "milli_kV"},
    {"display_brightness_percent", 80U, 0U, 100U, "%"},
    {"env_temp_offset_mc", 0, -10000, 10000, "milli_C"},
    {"env_humi_offset_mpermil", 0, -20000, 20000, "mpermil"},
};

static rt_int32_t param_service_values[PARAM_SERVICE_ID_COUNT];
static param_service_status_t param_service_status;
static rt_mutex_t param_service_lock = RT_NULL;
static rt_bool_t param_service_ready = RT_FALSE;

static rt_err_t param_service_lock_take(void)
{
    if (param_service_lock == RT_NULL)
    {
        return -RT_ERROR;
    }

    return rt_mutex_take(param_service_lock, RT_WAITING_FOREVER);
}

static void param_service_lock_release(void)
{
    if (param_service_lock != RT_NULL)
    {
        rt_mutex_release(param_service_lock);
    }
}

static rt_bool_t param_service_id_valid(param_service_id_t id)
{
    return ((rt_uint32_t)id < (rt_uint32_t)PARAM_SERVICE_ID_COUNT) ? RT_TRUE : RT_FALSE;
}

static void param_service_load_defaults_unlocked(void)
{
    rt_uint32_t i;

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        param_service_values[i] = param_service_descriptors[i].default_value;
    }
}

static void param_service_mark_changed_unlocked(void)
{
    param_service_status.version++;
    param_service_status.last_update_ms = rt_tick_get_millisecond();
    param_service_status.dirty = 1U;
}

rt_err_t param_service_init(void)
{
    if (param_service_ready == RT_TRUE)
    {
        return RT_EOK;
    }

    if (param_service_lock == RT_NULL)
    {
        param_service_lock = rt_mutex_create("param_svc", RT_IPC_FLAG_PRIO);
        if (param_service_lock == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    param_service_load_defaults_unlocked();
    param_service_status.version = 1U;
    param_service_status.last_update_ms = rt_tick_get_millisecond();
    param_service_status.dirty = 0U;
    param_service_status.persistent_ready = 0U;
    param_service_status.count = (rt_size_t)PARAM_SERVICE_ID_COUNT;
    param_service_ready = RT_TRUE;
    param_service_lock_release();

    return RT_EOK;
}

rt_err_t param_service_get(param_service_id_t id, rt_uint32_t *value)
{
    rt_int32_t signed_value;
    rt_err_t result;

    if (value == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = param_service_get_i32(id, &signed_value);
    if (result != RT_EOK)
    {
        return result;
    }

    if (signed_value < 0)
    {
        return -RT_EINVAL;
    }

    *value = (rt_uint32_t)signed_value;
    return RT_EOK;
}

rt_err_t param_service_get_i32(param_service_id_t id, rt_int32_t *value)
{
    if (param_service_id_valid(id) == RT_FALSE || value == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    *value = param_service_values[id];
    param_service_lock_release();

    return RT_EOK;
}

rt_err_t param_service_set(param_service_id_t id, rt_uint32_t value)
{
    if (value > 0x7fffffffUL)
    {
        return -RT_EINVAL;
    }

    return param_service_set_i32(id, (rt_int32_t)value);
}

rt_err_t param_service_set_i32(param_service_id_t id, rt_int32_t value)
{
    const param_service_descriptor_t *descriptor;

    if (param_service_id_valid(id) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    descriptor = &param_service_descriptors[id];
    if (value < descriptor->min_value || value > descriptor->max_value)
    {
        return -RT_EINVAL;
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (param_service_values[id] != value)
    {
        param_service_values[id] = value;
        param_service_mark_changed_unlocked();
    }
    param_service_lock_release();

    return RT_EOK;
}

rt_err_t param_service_get_item(param_service_id_t id, param_service_item_t *item)
{
    const param_service_descriptor_t *descriptor;

    if (param_service_id_valid(id) == RT_FALSE || item == RT_NULL)
    {
        return -RT_EINVAL;
    }

    descriptor = &param_service_descriptors[id];

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    item->name = descriptor->name;
    item->value = param_service_values[id];
    item->default_value = descriptor->default_value;
    item->min_value = descriptor->min_value;
    item->max_value = descriptor->max_value;
    item->unit = descriptor->unit;
    param_service_lock_release();

    return RT_EOK;
}

rt_err_t param_service_get_status(param_service_status_t *status)
{
    if (status == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    *status = param_service_status;
    param_service_lock_release();

    return RT_EOK;
}

rt_err_t param_service_find(const char *name, param_service_id_t *id)
{
    rt_uint32_t i;

    if (name == RT_NULL || id == RT_NULL)
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        if (strcmp(name, param_service_descriptors[i].name) == 0)
        {
            *id = (param_service_id_t)i;
            return RT_EOK;
        }
    }

    return -RT_ERROR;
}

void param_service_reset_defaults(void)
{
    rt_uint32_t i;
    rt_bool_t changed = RT_FALSE;

    if (param_service_lock_take() != RT_EOK)
    {
        return;
    }

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        if (param_service_values[i] != param_service_descriptors[i].default_value)
        {
            changed = RT_TRUE;
            break;
        }
    }

    param_service_load_defaults_unlocked();
    if (changed == RT_TRUE)
    {
        param_service_mark_changed_unlocked();
    }
    param_service_lock_release();
}

rt_size_t param_service_count(void)
{
    return (rt_size_t)PARAM_SERVICE_ID_COUNT;
}
