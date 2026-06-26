#ifndef APP_PARAM_SERVICE_H
#define APP_PARAM_SERVICE_H

#include <rtthread.h>

typedef enum
{
    PARAM_SERVICE_ID_MODBUS_ADDR = 0,
    PARAM_SERVICE_ID_MODBUS_BAUDRATE,
    PARAM_SERVICE_ID_MODBUS_DATA_FORMAT,
    PARAM_SERVICE_ID_ALARM_PEAK_MILLI_KV,
    PARAM_SERVICE_ID_DISPLAY_BRIGHTNESS_PERCENT,
    PARAM_SERVICE_ID_ENV_TEMP_OFFSET_MC,
    PARAM_SERVICE_ID_ENV_HUMI_OFFSET_MPERMIL,
    PARAM_SERVICE_ID_TI_ALARM_ENABLE_MASK,
    PARAM_SERVICE_ID_TI_ALARM_OPEN_LEVEL_MASK,
    PARAM_SERVICE_ID_NTC_SENSOR_ENABLE,
    PARAM_SERVICE_ID_NTC_HIGH_ALARM_ENABLE,
    PARAM_SERVICE_ID_NTC_HIGH_ALARM_THRESHOLD_MC,
    PARAM_SERVICE_ID_NTC_LOW_ALARM_ENABLE,
    PARAM_SERVICE_ID_NTC_LOW_ALARM_THRESHOLD_MC,
    PARAM_SERVICE_ID_TI_ALARM_DEBOUNCE_MS,
    PARAM_SERVICE_ID_NTC_ALARM_DEBOUNCE_MS,
    PARAM_SERVICE_ID_SURGE_ALARM_ENABLE,
    PARAM_SERVICE_ID_NET_DHCP_ENABLE,
    PARAM_SERVICE_ID_NET_IP_ADDR,
    PARAM_SERVICE_ID_NET_NETMASK,
    PARAM_SERVICE_ID_NET_GATEWAY,
    PARAM_SERVICE_ID_MODBUS_TCP_PORT,
    PARAM_SERVICE_ID_COUNT
} param_service_id_t;

typedef enum
{
    PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1 = 0,
    PARAM_SERVICE_MODBUS_DATA_FORMAT_8E1,
    PARAM_SERVICE_MODBUS_DATA_FORMAT_8O1,
    PARAM_SERVICE_MODBUS_DATA_FORMAT_8N2,
    PARAM_SERVICE_MODBUS_DATA_FORMAT_8E2,
    PARAM_SERVICE_MODBUS_DATA_FORMAT_8O2,
    PARAM_SERVICE_MODBUS_DATA_FORMAT_COUNT
} param_service_modbus_data_format_t;

typedef enum
{
    PARAM_SERVICE_VALUE_BOOL = 0,
    PARAM_SERVICE_VALUE_U16,
    PARAM_SERVICE_VALUE_I16,
    PARAM_SERVICE_VALUE_U32,
    PARAM_SERVICE_VALUE_I32,
    PARAM_SERVICE_VALUE_MASK,
    PARAM_SERVICE_VALUE_IPADDR
} param_service_value_type_t;

typedef struct
{
    const char *name;
    rt_int32_t value;
    rt_int32_t default_value;
    rt_int32_t min_value;
    rt_int32_t max_value;
    const char *unit;
    param_service_value_type_t value_type;
    rt_uint16_t holding_register;
    rt_uint16_t holding_count;
} param_service_item_t;

typedef struct
{
    rt_uint32_t version;
    rt_uint32_t last_update_ms;
    rt_uint8_t dirty;
    rt_uint8_t persistent_ready;
    rt_size_t count;
} param_service_status_t;

rt_err_t param_service_init(void);
rt_err_t param_service_get(param_service_id_t id, rt_uint32_t *value);
rt_err_t param_service_get_i32(param_service_id_t id, rt_int32_t *value);
rt_err_t param_service_validate_i32(param_service_id_t id, rt_int32_t value);
rt_err_t param_service_set(param_service_id_t id, rt_uint32_t value);
rt_err_t param_service_set_i32(param_service_id_t id, rt_int32_t value);
rt_err_t param_service_get_item(param_service_id_t id, param_service_item_t *item);
rt_err_t param_service_get_status(param_service_status_t *status);
rt_err_t param_service_find(const char *name, param_service_id_t *id);
rt_err_t param_service_find_by_holding(rt_uint16_t holding_register, param_service_id_t *id);
rt_err_t param_service_find_holding(rt_uint16_t holding_register,
                                    param_service_id_t *id,
                                    rt_uint16_t *holding_offset);
rt_err_t param_service_get_holding(param_service_id_t id, rt_uint16_t *holding_register);
rt_err_t param_service_load(void);
rt_err_t param_service_save(void);
void param_service_reset_defaults(void);
rt_size_t param_service_count(void);

#endif
