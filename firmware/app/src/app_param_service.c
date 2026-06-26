#include "app_param_service.h"

#include <string.h>

#include "storage_flashdb.h"

#define PARAM_SERVICE_STORAGE_MAGIC      0x5052414DU
#define PARAM_SERVICE_STORAGE_VERSION    6U
#define PARAM_SERVICE_KV_KEY             "param_image"
#define PARAM_SERVICE_STORAGE_MAX_ENTRIES 64U
#define PARAM_SERVICE_I32_MIN            ((rt_int32_t)(-2147483647 - 1))
#define PARAM_SERVICE_I32_MAX            ((rt_int32_t)2147483647)
#define PARAM_SERVICE_IPV4(a, b, c, d)   ((rt_int32_t)((((rt_uint32_t)(a) & 0xffU) << 24) | \
                                                       (((rt_uint32_t)(b) & 0xffU) << 16) | \
                                                       (((rt_uint32_t)(c) & 0xffU) << 8) | \
                                                       ((rt_uint32_t)(d) & 0xffU)))

typedef struct
{
    const char *name;
    rt_int32_t default_value;
    rt_int32_t min_value;
    rt_int32_t max_value;
    const char *unit;
    param_service_value_type_t value_type;
    rt_uint16_t holding_register;
    rt_uint16_t holding_count;
} param_service_descriptor_t;

typedef struct
{
    rt_uint16_t id;
    rt_int32_t value;
} param_service_storage_entry_t;

typedef struct
{
    rt_uint32_t magic;
    rt_uint16_t version;
    rt_uint16_t image_size;
    rt_uint16_t item_count;
    rt_uint16_t reserved;
    param_service_storage_entry_t entries[PARAM_SERVICE_STORAGE_MAX_ENTRIES];
    rt_uint32_t checksum;
} param_service_storage_image_t;

static const param_service_descriptor_t param_service_descriptors[PARAM_SERVICE_ID_COUNT] =
{
    {"modbus_addr", 1, 1, 247, "", PARAM_SERVICE_VALUE_U16, 1U, 1U},
    {"modbus_baudrate", 9600, 1200, 115200, "bps", PARAM_SERVICE_VALUE_U32, 2U, 1U},
    {"modbus_data_format", PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1, 0,
     PARAM_SERVICE_MODBUS_DATA_FORMAT_COUNT - 1, "", PARAM_SERVICE_VALUE_U16, 3U, 1U},
    {"alarm_peak_milli_kv", 50000, 0, 99900, "milli_kV", PARAM_SERVICE_VALUE_I32, 4U, 1U},
    {"display_brightness_percent", 80, 0, 100, "%", PARAM_SERVICE_VALUE_U16, 5U, 1U},
    {"env_temp_offset_mc", 0, -10000, 10000, "milli_C", PARAM_SERVICE_VALUE_I32, 6U, 1U},
    {"env_humi_offset_mpermil", 0, -20000, 20000, "mpermil", PARAM_SERVICE_VALUE_I32, 7U, 1U},
    {"ti_alarm_enable_mask", 0x00, 0, 0x1f, "mask", PARAM_SERVICE_VALUE_MASK, 8U, 1U},
    {"ti_alarm_open_level_mask", 0x00, 0, 0x1f, "mask", PARAM_SERVICE_VALUE_MASK, 9U, 1U},
    {"ntc_sensor_enable", 1, 0, 1, "bool", PARAM_SERVICE_VALUE_BOOL, 10U, 1U},
    {"ntc_high_alarm_enable", 0, 0, 1, "bool", PARAM_SERVICE_VALUE_BOOL, 11U, 1U},
    {"ntc_high_alarm_threshold_mc", 70000, -40000, 125000, "milli_C", PARAM_SERVICE_VALUE_I32, 12U, 1U},
    {"ntc_low_alarm_enable", 0, 0, 1, "bool", PARAM_SERVICE_VALUE_BOOL, 13U, 1U},
    {"ntc_low_alarm_threshold_mc", -20000, -40000, 125000, "milli_C", PARAM_SERVICE_VALUE_I32, 14U, 1U},
    {"ti_alarm_debounce_ms", 50, 0, 60000, "ms", PARAM_SERVICE_VALUE_U32, 15U, 1U},
    {"ntc_alarm_debounce_ms", 1000, 0, 60000, "ms", PARAM_SERVICE_VALUE_U32, 16U, 1U},
    {"surge_alarm_enable", 1, 0, 1, "bool", PARAM_SERVICE_VALUE_BOOL, 17U, 1U},
    {"net_dhcp_enable", 0, 0, 1, "bool", PARAM_SERVICE_VALUE_BOOL, 50U, 1U},
    {"net_ip_addr", PARAM_SERVICE_IPV4(192, 168, 1, 223),
     PARAM_SERVICE_I32_MIN, PARAM_SERVICE_I32_MAX, "ipv4", PARAM_SERVICE_VALUE_IPADDR, 51U, 4U},
    {"net_netmask", PARAM_SERVICE_IPV4(255, 255, 255, 0),
     PARAM_SERVICE_I32_MIN, PARAM_SERVICE_I32_MAX, "ipv4", PARAM_SERVICE_VALUE_IPADDR, 55U, 4U},
    {"net_gateway", PARAM_SERVICE_IPV4(192, 168, 1, 1),
     PARAM_SERVICE_I32_MIN, PARAM_SERVICE_I32_MAX, "ipv4", PARAM_SERVICE_VALUE_IPADDR, 59U, 4U},
    {"modbus_tcp_port", 502, 1, 65535, "port", PARAM_SERVICE_VALUE_U16, 63U, 1U},
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

static rt_bool_t param_service_values_policy_is_valid(const rt_int32_t *values);
rt_err_t param_service_validate_i32(param_service_id_t id, rt_int32_t value);

static rt_uint32_t param_service_checksum(const param_service_storage_image_t *image)
{
    const rt_uint8_t *bytes;
    rt_size_t length;
    rt_size_t i;
    rt_uint32_t checksum = 0xA55A5AA5U;

    bytes = (const rt_uint8_t *)image;
    length = sizeof(*image) - sizeof(image->checksum);

    for (i = 0U; i < length; i++)
    {
        checksum = (checksum << 5) | (checksum >> 27);
        checksum ^= bytes[i];
    }

    return checksum;
}

static rt_bool_t param_service_values_are_valid(const rt_int32_t *values)
{
    rt_uint32_t i;

    if (values == RT_NULL)
    {
        return RT_FALSE;
    }

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        if (values[i] < param_service_descriptors[i].min_value ||
            values[i] > param_service_descriptors[i].max_value)
        {
            return RT_FALSE;
        }
    }

    return param_service_values_policy_is_valid(values);
}

static rt_bool_t param_service_values_policy_is_valid(const rt_int32_t *values)
{
    if (values == RT_NULL)
    {
        return RT_FALSE;
    }

    return RT_TRUE;
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

    if (storage_flashdb_init() == RT_EOK && param_service_load() == RT_EOK)
    {
        param_service_status.persistent_ready = 1U;
    }

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

rt_err_t param_service_validate_i32(param_service_id_t id, rt_int32_t value)
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

    return RT_EOK;
}

rt_err_t param_service_set_i32(param_service_id_t id, rt_int32_t value)
{
    rt_int32_t candidate_values[PARAM_SERVICE_ID_COUNT];
    rt_uint32_t i;

    if (param_service_validate_i32(id, value) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        candidate_values[i] = param_service_values[i];
    }
    candidate_values[id] = value;

    if (param_service_values_policy_is_valid(candidate_values) == RT_FALSE)
    {
        param_service_lock_release();
        return -RT_EINVAL;
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
    item->value_type = descriptor->value_type;
    item->holding_register = descriptor->holding_register;
    item->holding_count = descriptor->holding_count;
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

rt_err_t param_service_find_by_holding(rt_uint16_t holding_register, param_service_id_t *id)
{
    return param_service_find_holding(holding_register, id, RT_NULL);
}

rt_err_t param_service_find_holding(rt_uint16_t holding_register,
                                    param_service_id_t *id,
                                    rt_uint16_t *holding_offset)
{
    rt_uint32_t i;

    if (id == RT_NULL)
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        rt_uint16_t start = param_service_descriptors[i].holding_register;
        rt_uint16_t count = param_service_descriptors[i].holding_count;

        if (holding_register >= start &&
            holding_register < (rt_uint16_t)(start + count))
        {
            *id = (param_service_id_t)i;
            if (holding_offset != RT_NULL)
            {
                *holding_offset = (rt_uint16_t)(holding_register - start);
            }
            return RT_EOK;
        }
    }

    return -RT_ERROR;
}

rt_err_t param_service_get_holding(param_service_id_t id, rt_uint16_t *holding_register)
{
    if (param_service_id_valid(id) == RT_FALSE || holding_register == RT_NULL)
    {
        return -RT_EINVAL;
    }

    *holding_register = param_service_descriptors[id].holding_register;
    return RT_EOK;
}

rt_err_t param_service_load(void)
{
    param_service_storage_image_t image;
    rt_int32_t candidate_values[PARAM_SERVICE_ID_COUNT];
    struct fdb_blob blob;
    fdb_kvdb_t kvdb;
    size_t read_size;
    rt_uint32_t i;

    kvdb = storage_flashdb_get_param_kvdb();
    if (kvdb == RT_NULL)
    {
        return -RT_ERROR;
    }

    memset(&image, 0, sizeof(image));
    read_size = fdb_kv_get_blob(kvdb,
                                PARAM_SERVICE_KV_KEY,
                                fdb_blob_make(&blob, &image, sizeof(image)));
    if (read_size != sizeof(image) ||
        image.magic != PARAM_SERVICE_STORAGE_MAGIC ||
        image.version != PARAM_SERVICE_STORAGE_VERSION ||
        image.image_size != (rt_uint16_t)sizeof(image) ||
        image.item_count > (rt_uint16_t)PARAM_SERVICE_STORAGE_MAX_ENTRIES ||
        image.checksum != param_service_checksum(&image))
    {
        return -RT_ERROR;
    }

    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        candidate_values[i] = param_service_descriptors[i].default_value;
    }

    for (i = 0U; i < image.item_count; i++)
    {
        param_service_id_t id = (param_service_id_t)image.entries[i].id;

        if (param_service_id_valid(id) == RT_FALSE ||
            param_service_validate_i32(id, image.entries[i].value) != RT_EOK)
        {
            return -RT_ERROR;
        }
        candidate_values[id] = image.entries[i].value;
    }

    if (param_service_values_are_valid(candidate_values) == RT_FALSE)
    {
        return -RT_ERROR;
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    memcpy(param_service_values, candidate_values, sizeof(param_service_values));
    param_service_status.dirty = 0U;
    param_service_status.version++;
    param_service_status.last_update_ms = rt_tick_get_millisecond();
    param_service_status.persistent_ready = 1U;
    param_service_lock_release();

    return RT_EOK;
}

rt_err_t param_service_save(void)
{
    param_service_storage_image_t image;
    struct fdb_blob blob;
    fdb_kvdb_t kvdb;
    rt_uint32_t i;

    memset(&image, 0, sizeof(image));

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    image.magic = PARAM_SERVICE_STORAGE_MAGIC;
    image.version = PARAM_SERVICE_STORAGE_VERSION;
    image.image_size = (rt_uint16_t)sizeof(image);
    image.item_count = (rt_uint16_t)PARAM_SERVICE_ID_COUNT;
    for (i = 0U; i < (rt_uint32_t)PARAM_SERVICE_ID_COUNT; i++)
    {
        image.entries[i].id = (rt_uint16_t)i;
        image.entries[i].value = param_service_values[i];
    }
    image.checksum = param_service_checksum(&image);
    param_service_lock_release();

    kvdb = storage_flashdb_get_param_kvdb();
    if (kvdb == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (fdb_kv_set_blob(kvdb,
                        PARAM_SERVICE_KV_KEY,
                        fdb_blob_make(&blob, &image, sizeof(image))) != FDB_NO_ERR)
    {
        return -RT_ERROR;
    }

    if (param_service_lock_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    param_service_status.last_update_ms = rt_tick_get_millisecond();
    param_service_status.dirty = 0U;
    param_service_status.persistent_ready = 1U;
    param_service_lock_release();

    return RT_EOK;
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
