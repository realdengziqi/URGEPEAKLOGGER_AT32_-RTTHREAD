#include "app_shell.h"

#include <string.h>

#include "app_backend_request.h"
#include "app_alarm_service.h"
#include "app_bringup.h"
#include "app_data_service.h"
#include "app_factory_service.h"
#include "app_param_service.h"
#include "app_record_service.h"
#include "app_surge_service.h"
#include "finsh.h"
#include "sensor_adc_raw.h"
#include "sensor_digital_input.h"
#include "shell.h"
#include "storage_flashdb.h"

#define APP_SHELL_DATA_WATCH_DEFAULT_COUNT      5U
#define APP_SHELL_DATA_WATCH_MAX_COUNT          30U
#define APP_SHELL_DATA_WATCH_DEFAULT_PERIOD_MS  1000U
#define APP_SHELL_DATA_WATCH_MIN_PERIOD_MS      200U
#define APP_SHELL_DATA_WATCH_MAX_PERIOD_MS      10000U
#define APP_SHELL_THREAD_LIST_MAX               12
#define APP_SHELL_DI_WATCH_DEFAULT_COUNT        30U
#define APP_SHELL_DI_WATCH_MAX_COUNT            100U
#define APP_SHELL_DI_WATCH_DEFAULT_PERIOD_MS    200U
#define APP_SHELL_DI_WATCH_MIN_PERIOD_MS        50U
#define APP_SHELL_DI_WATCH_MAX_PERIOD_MS        5000U
#define APP_SHELL_ADC_WATCH_DEFAULT_COUNT       20U
#define APP_SHELL_ADC_WATCH_MAX_COUNT           100U
#define APP_SHELL_ADC_WATCH_DEFAULT_PERIOD_MS   200U
#define APP_SHELL_ADC_WATCH_MIN_PERIOD_MS       50U
#define APP_SHELL_ADC_WATCH_MAX_PERIOD_MS       5000U
#define APP_SHELL_RECORD_ALARM_PAGE_DEFAULT     8U
#define APP_SHELL_RECORD_ALARM_PAGE_MAX         16U

static int app_shell_parse_u32_arg(const char *text, rt_uint32_t *value);
static void app_shell_alarm_print_config(void);

static rt_bool_t app_shell_hex_digit(char ch, rt_uint8_t *value)
{
    if (value == RT_NULL)
    {
        return RT_FALSE;
    }

    if (ch >= '0' && ch <= '9')
    {
        *value = (rt_uint8_t)(ch - '0');
        return RT_TRUE;
    }
    if (ch >= 'a' && ch <= 'f')
    {
        *value = (rt_uint8_t)(ch - 'a' + 10);
        return RT_TRUE;
    }
    if (ch >= 'A' && ch <= 'F')
    {
        *value = (rt_uint8_t)(ch - 'A' + 10);
        return RT_TRUE;
    }

    return RT_FALSE;
}

static rt_bool_t app_shell_parse_mac(const char *text, rt_uint8_t mac[6])
{
    rt_size_t i;

    if (text == RT_NULL || mac == RT_NULL || strlen(text) != 17U)
    {
        return RT_FALSE;
    }

    for (i = 0U; i < 6U; i++)
    {
        rt_uint8_t high;
        rt_uint8_t low;
        rt_size_t offset = i * 3U;

        if (app_shell_hex_digit(text[offset], &high) == RT_FALSE ||
            app_shell_hex_digit(text[offset + 1U], &low) == RT_FALSE)
        {
            return RT_FALSE;
        }

        mac[i] = (rt_uint8_t)((high << 4) | low);
        if (i < 5U && text[offset + 2U] != ':')
        {
            return RT_FALSE;
        }
    }

    return RT_TRUE;
}

static void app_shell_print_mac(const char *prefix, const rt_uint8_t mac[6])
{
    rt_kprintf("%s%02x:%02x:%02x:%02x:%02x:%02x\r\n",
               prefix,
               mac[0],
               mac[1],
               mac[2],
               mac[3],
               mac[4],
               mac[5]);
}

static void app_shell_print_datetime(const char *tag, const app_rtc_datetime_t *datetime)
{
    rt_kprintf("[INFO] %s: %04u-%02u-%02u %02u:%02u:%02u weekday=%u\r\n",
               tag,
               datetime->year,
               datetime->month,
               datetime->day,
               datetime->hour,
               datetime->minute,
               datetime->second,
               datetime->weekday);
}

static rt_bool_t app_shell_parse_uint(const char *text, rt_size_t length, rt_uint16_t *value)
{
    rt_size_t i;
    rt_uint16_t result = 0U;

    if (text == RT_NULL || value == RT_NULL || length == 0U)
    {
        return RT_FALSE;
    }

    for (i = 0U; i < length; i++)
    {
        if (text[i] < '0' || text[i] > '9')
        {
            return RT_FALSE;
        }

        result = (rt_uint16_t)(result * 10U + (rt_uint16_t)(text[i] - '0'));
    }

    *value = result;
    return RT_TRUE;
}

static rt_bool_t app_shell_parse_datetime(const char *date_text,
                                          const char *time_text,
                                          app_rtc_datetime_t *datetime)
{
    rt_uint16_t value;

    if (date_text == RT_NULL || time_text == RT_NULL || datetime == RT_NULL ||
        strlen(date_text) != 10U || strlen(time_text) != 8U ||
        date_text[4] != '-' || date_text[7] != '-' ||
        time_text[2] != ':' || time_text[5] != ':')
    {
        return RT_FALSE;
    }

    if (app_shell_parse_uint(date_text, 4U, &datetime->year) == RT_FALSE ||
        app_shell_parse_uint(date_text + 5, 2U, &value) == RT_FALSE)
    {
        return RT_FALSE;
    }
    datetime->month = (rt_uint8_t)value;

    if (app_shell_parse_uint(date_text + 8, 2U, &value) == RT_FALSE)
    {
        return RT_FALSE;
    }
    datetime->day = (rt_uint8_t)value;

    if (app_shell_parse_uint(time_text, 2U, &value) == RT_FALSE)
    {
        return RT_FALSE;
    }
    datetime->hour = (rt_uint8_t)value;

    if (app_shell_parse_uint(time_text + 3, 2U, &value) == RT_FALSE)
    {
        return RT_FALSE;
    }
    datetime->minute = (rt_uint8_t)value;

    if (app_shell_parse_uint(time_text + 6, 2U, &value) == RT_FALSE)
    {
        return RT_FALSE;
    }
    datetime->second = (rt_uint8_t)value;
    datetime->weekday = 0U;

    return RT_TRUE;
}

static const char *app_shell_thread_state_text(rt_uint8_t state)
{
    if ((state & RT_THREAD_SUSPEND_MASK) == RT_THREAD_SUSPEND_MASK)
    {
        return "suspend";
    }

    switch (state & RT_THREAD_STAT_MASK)
    {
    case RT_THREAD_INIT:
        return "init";
    case RT_THREAD_CLOSE:
        return "close";
    case RT_THREAD_READY:
        return "ready";
    case RT_THREAD_RUNNING:
        return "run";
    default:
        return "unknown";
    }
}

static rt_uint32_t app_shell_thread_stack_used(const struct rt_thread *thread)
{
    const rt_uint8_t *stack;
    rt_uint32_t unused = 0U;

    if (thread == RT_NULL || thread->stack_addr == RT_NULL)
    {
        return 0U;
    }

    stack = (const rt_uint8_t *)thread->stack_addr;
    while ((unused < thread->stack_size) && (stack[unused] == (rt_uint8_t)'#'))
    {
        unused++;
    }

    return thread->stack_size - unused;
}

static void app_shell_print_snapshot(void)
{
    device_data_snapshot_t snapshot;

    data_service_get_snapshot(&snapshot);
    rt_kprintf("[INFO] data: update=%u time_ms=%u time_valid=%u peak_mkv=%d ntc_mc=%d\r\n",
               snapshot.update_count,
               snapshot.timestamp_ms,
               snapshot.time_valid,
               snapshot.surge_peak_value_milli_kv,
               snapshot.ntc_temp_milli_c);
    rt_kprintf("[INFO] data adc: ntc_raw=%u surge_raw=%u\r\n",
               snapshot.ntc_adc_raw,
               snapshot.surge_adc_raw);
    rt_kprintf("[INFO] data surge: status=%u peak_raw=%u delta=%u threshold=%u triggers=%u\r\n",
               snapshot.surge_status,
               snapshot.surge_peak_raw,
               snapshot.surge_peak_delta_raw,
               snapshot.surge_threshold_delta_raw,
               snapshot.surge_trigger_count);
    rt_kprintf("[INFO] data sensor: env_temp_mc=%d humi_mpermil=%d di_raw=0x%02x sensor_flags=0x%02x\r\n",
               snapshot.env_temp_milli_c,
               snapshot.env_humi_milli_percent,
               snapshot.digital_input_raw_state,
               snapshot.sensor_valid_flags);
    rt_kprintf("[INFO] data env raw: temp_mc=%d humi_mpermil=%d\r\n",
               snapshot.env_temp_raw_milli_c,
               snapshot.env_humi_raw_milli_percent);
    rt_kprintf("[INFO] data env cal: temp_mc=%d humi_mpermil=%d\r\n",
               snapshot.env_temp_calibrated_milli_c,
               snapshot.env_humi_calibrated_milli_percent);
    rt_kprintf("[INFO] data state: alarm=%u rs485=%u tcp=%u param_ver=%u param_dirty=%u\r\n",
               snapshot.alarm_state,
               snapshot.rs485_online,
               snapshot.tcp_connected,
               snapshot.param_status.version,
               snapshot.param_status.dirty);
    if (snapshot.time_valid != 0U)
    {
        app_shell_print_datetime("data time", &snapshot.datetime);
    }
}

static void app_shell_print_record_item(const record_service_item_t *item)
{
    if (item == RT_NULL)
    {
        return;
    }

    rt_kprintf("[INFO] record: seq=%u fmt=%u type=%s alarm_id=%u action=%s reason=%s source=%u event_ms=%u duration_ms=%u peak_mkv=%d ntc_mc=%d env_temp_mc=%d humi_mpermil=%d di=0x%02x alarm=%u\r\n",
               item->sequence,
               item->format_version,
               record_service_get_type_name(item->type),
               item->alarm_id,
               record_service_get_alarm_action_name(item->event_action),
               record_service_get_alarm_reason_name(item->alarm_reason),
               item->source_index,
               item->timestamp_ms,
               item->duration_ms,
               item->peak_milli_kv,
               item->ntc_temp_milli_c,
               item->env_temp_milli_c,
               item->env_humi_milli_percent,
               item->digital_input_state,
               item->alarm_state);
    if (item->time_valid != 0U)
    {
        app_shell_print_datetime("record event_time", &item->datetime);
    }
}

static void app_shell_alarm_print_config(void)
{
    rt_int32_t ti_enable_mask = 0;
    rt_int32_t ti_open_level_mask = 0;
    rt_int32_t ntc_sensor_enable = 0;
    rt_int32_t ntc_high_alarm_enable = 0;
    rt_int32_t ntc_low_alarm_enable = 0;

    (void)param_service_get_i32(PARAM_SERVICE_ID_TI_ALARM_ENABLE_MASK, &ti_enable_mask);
    (void)param_service_get_i32(PARAM_SERVICE_ID_TI_ALARM_OPEN_LEVEL_MASK, &ti_open_level_mask);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_SENSOR_ENABLE, &ntc_sensor_enable);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_HIGH_ALARM_ENABLE, &ntc_high_alarm_enable);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NTC_LOW_ALARM_ENABLE, &ntc_low_alarm_enable);

    rt_kprintf("[INFO] alarm config: ntc_sensor=%d ntc_high=%d ntc_low=%d ti_enable_mask=0x%02x ti_open_level_mask=0x%02x\r\n",
               ntc_sensor_enable,
               ntc_high_alarm_enable,
               ntc_low_alarm_enable,
               (rt_uint32_t)ti_enable_mask & 0x1FU,
               (rt_uint32_t)ti_open_level_mask & 0x1FU);
    rt_kprintf("[INFO] alarm config: TI1=%u TI2=%u TI3=%u TI4=%u TI5=%u open_level 0=low 1=high\r\n",
               (ti_enable_mask & (1 << 0)) ? 1U : 0U,
               (ti_enable_mask & (1 << 1)) ? 1U : 0U,
               (ti_enable_mask & (1 << 2)) ? 1U : 0U,
               (ti_enable_mask & (1 << 3)) ? 1U : 0U,
               (ti_enable_mask & (1 << 4)) ? 1U : 0U);
}

static int app_shell_alarm(int argc, char **argv)
{
    alarm_service_snapshot_t snapshot;
    rt_size_t i;
    rt_err_t result;

    if (argc == 2 && strcmp(argv[1], "config") == 0)
    {
        app_shell_alarm_print_config();
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "enable") == 0)
    {
        rt_uint32_t enable;

        if (app_shell_parse_u32_arg(argv[3], &enable) != RT_EOK || enable > 1U)
        {
            rt_kprintf("[WARN] alarm: enable value must be 0 or 1\r\n");
            return 0;
        }

        if (strcmp(argv[2], "ntc_sensor") == 0)
        {
            if (backend_request_set_param(PARAM_SERVICE_ID_NTC_SENSOR_ENABLE,
                                          (rt_int32_t)enable) != RT_EOK)
            {
                rt_kprintf("[WARN] alarm: set ntc sensor enable failed\r\n");
            }
            app_shell_alarm_print_config();
            return 0;
        }

        if (strcmp(argv[2], "ntc_high") == 0)
        {
            if (backend_request_set_param(PARAM_SERVICE_ID_NTC_HIGH_ALARM_ENABLE,
                                          (rt_int32_t)enable) != RT_EOK)
            {
                rt_kprintf("[WARN] alarm: set ntc high enable failed\r\n");
            }
            app_shell_alarm_print_config();
            return 0;
        }

        if (strcmp(argv[2], "ntc_low") == 0)
        {
            if (backend_request_set_param(PARAM_SERVICE_ID_NTC_LOW_ALARM_ENABLE,
                                          (rt_int32_t)enable) != RT_EOK)
            {
                rt_kprintf("[WARN] alarm: set ntc low enable failed\r\n");
            }
            app_shell_alarm_print_config();
            return 0;
        }

        if (strcmp(argv[2], "ntc") == 0)
        {
            if (backend_request_set_param(PARAM_SERVICE_ID_NTC_HIGH_ALARM_ENABLE,
                                          (rt_int32_t)enable) != RT_EOK ||
                backend_request_set_param(PARAM_SERVICE_ID_NTC_LOW_ALARM_ENABLE,
                                          (rt_int32_t)enable) != RT_EOK)
            {
                rt_kprintf("[WARN] alarm: set ntc alarm enable failed\r\n");
            }
            app_shell_alarm_print_config();
            return 0;
        }

        if (strlen(argv[2]) == 3U &&
            argv[2][0] == 't' &&
            argv[2][1] == 'i' &&
            argv[2][2] >= '1' &&
            argv[2][2] <= '5')
        {
            rt_int32_t ti_enable_mask = 0;
            rt_uint8_t bit = (rt_uint8_t)(1U << (argv[2][2] - '1'));

            (void)param_service_get_i32(PARAM_SERVICE_ID_TI_ALARM_ENABLE_MASK, &ti_enable_mask);
            if (enable != 0U)
            {
                ti_enable_mask |= bit;
            }
            else
            {
                ti_enable_mask &= (rt_int32_t)(~bit);
            }

            if (backend_request_set_param(PARAM_SERVICE_ID_TI_ALARM_ENABLE_MASK,
                                          ti_enable_mask) != RT_EOK)
            {
                rt_kprintf("[WARN] alarm: set ti enable failed\r\n");
            }
            app_shell_alarm_print_config();
            return 0;
        }

        rt_kprintf("[WARN] alarm: source must be ntc or ti1..ti5\r\n");
        return 0;
    }

    if (argc != 1)
    {
        rt_kprintf("[WARN] alarm: usage alarm | alarm config | alarm enable <ntc|ti1..ti5> <0|1>\r\n");
        return 0;
    }

    result = alarm_service_get_snapshot(&snapshot);
    if (result != RT_EOK)
    {
        rt_kprintf("[WARN] alarm: status unavailable %d\r\n", result);
        return 0;
    }

    rt_kprintf("[INFO] alarm: active=%u total_started=%u total_ended=%u\r\n",
               (rt_uint32_t)snapshot.active_count,
               snapshot.total_started,
               snapshot.total_ended);
    rt_kprintf("[INFO] alarm image: persistent_ready=%u restored=%u writes=%u failed=%u\r\n",
               snapshot.persistent_ready,
               snapshot.image_restored,
               snapshot.image_write_count,
               snapshot.image_write_failed);
    if (snapshot.ntc_pending != 0U)
    {
        rt_kprintf("[INFO] alarm: ntc_pending reason=%s duration_ms=%u\r\n",
                   record_service_get_alarm_reason_name(snapshot.ntc_pending_reason),
                   snapshot.ntc_pending_duration_ms);
    }
    if (snapshot.ti_pending_mask != 0U)
    {
        for (i = 0U; i < 5U; i++)
        {
            rt_uint8_t bit = (rt_uint8_t)(1U << i);

            if ((snapshot.ti_pending_mask & bit) == 0U)
            {
                continue;
            }

            rt_kprintf("[INFO] alarm: ti%u_pending target=%s duration_ms=%u\r\n",
                       (rt_uint32_t)(i + 1U),
                       ((snapshot.ti_pending_target_active_mask & bit) != 0U) ? "active" : "normal",
                       snapshot.ti_pending_duration_ms[i]);
        }
    }

    for (i = 0U; i < snapshot.active_count; i++)
    {
        const alarm_service_active_item_t *item = &snapshot.active_items[i];
        rt_kprintf("[INFO] alarm: id=%u reason=%s source=%u start_ms=%u duration_ms=%u\r\n",
                   item->alarm_id,
                   record_service_get_alarm_reason_name(item->reason),
                   item->source_index,
                   item->start_timestamp_ms,
                   item->duration_ms);
        if (item->time_valid != 0U)
        {
            app_shell_print_datetime("alarm start_time", &item->start_datetime);
        }
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_alarm, alarm, show active alarms);

static int app_shell_parse_u32_arg(const char *text, rt_uint32_t *value)
{
    rt_uint32_t result = 0U;
    rt_size_t i;

    if (text == RT_NULL || value == RT_NULL || text[0] == '\0')
    {
        return -RT_EINVAL;
    }

    for (i = 0U; text[i] != '\0'; i++)
    {
        if (text[i] < '0' || text[i] > '9')
        {
            return -RT_EINVAL;
        }

        result = (result * 10U) + (rt_uint32_t)(text[i] - '0');
    }

    *value = result;
    return RT_EOK;
}

static int app_shell_parse_i32_arg(const char *text, rt_int32_t *value)
{
    rt_int32_t sign = 1;
    rt_int32_t result = 0;
    rt_size_t i = 0U;

    if (text == RT_NULL || value == RT_NULL || text[0] == '\0')
    {
        return -RT_EINVAL;
    }

    if (text[0] == '-')
    {
        sign = -1;
        i = 1U;
        if (text[i] == '\0')
        {
            return -RT_EINVAL;
        }
    }

    for (; text[i] != '\0'; i++)
    {
        if (text[i] < '0' || text[i] > '9')
        {
            return -RT_EINVAL;
        }

        result = (result * 10) + (rt_int32_t)(text[i] - '0');
    }

    *value = result * sign;
    return RT_EOK;
}

static void app_shell_print_di_sample(void)
{
    sensor_digital_input_sample_t sample;

    if (sensor_digital_input_read(&sample) != RT_EOK)
    {
        rt_kprintf("[WARN] di: read failed\r\n");
        return;
    }

    rt_kprintf("[INFO] ti: raw=0x%02x TI1=%u TI2=%u TI3=%u TI4=%u TI5=%u\r\n",
               sample.raw_state,
               (sample.raw_state & (1U << 0)) ? 1U : 0U,
               (sample.raw_state & (1U << 1)) ? 1U : 0U,
               (sample.raw_state & (1U << 2)) ? 1U : 0U,
               (sample.raw_state & (1U << 3)) ? 1U : 0U,
               (sample.raw_state & (1U << 4)) ? 1U : 0U);
}

static int app_shell_di(int argc, char **argv)
{
    rt_uint32_t count = APP_SHELL_DI_WATCH_DEFAULT_COUNT;
    rt_uint32_t period_ms = APP_SHELL_DI_WATCH_DEFAULT_PERIOD_MS;
    rt_uint32_t i;

    if (argc == 1)
    {
        app_shell_print_di_sample();
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "watch") == 0)
    {
        if (argc >= 3 && app_shell_parse_u32_arg(argv[2], &count) != RT_EOK)
        {
            rt_kprintf("[WARN] di: count must be a positive integer\r\n");
            return 0;
        }

        if (argc >= 4 && app_shell_parse_u32_arg(argv[3], &period_ms) != RT_EOK)
        {
            rt_kprintf("[WARN] di: period_ms must be a positive integer\r\n");
            return 0;
        }

        if (argc > 4 || count == 0U ||
            count > APP_SHELL_DI_WATCH_MAX_COUNT ||
            period_ms < APP_SHELL_DI_WATCH_MIN_PERIOD_MS ||
            period_ms > APP_SHELL_DI_WATCH_MAX_PERIOD_MS)
        {
            rt_kprintf("[WARN] di: usage di watch [count<=%u] [period_ms %u..%u]\r\n",
                       APP_SHELL_DI_WATCH_MAX_COUNT,
                       APP_SHELL_DI_WATCH_MIN_PERIOD_MS,
                       APP_SHELL_DI_WATCH_MAX_PERIOD_MS);
            return 0;
        }

        for (i = 0U; i < count; i++)
        {
            app_shell_print_di_sample();
            if ((i + 1U) < count)
            {
                rt_thread_mdelay(period_ms);
            }
        }

        return 0;
    }

    rt_kprintf("[WARN] di: usage di | di watch [count] [period_ms]\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_di, di, show digital input raw state);

static void app_shell_print_adc_sample(void)
{
    sensor_adc_raw_sample_t sample;
    rt_err_t result;

    result = sensor_adc_raw_read(&sample);
    if (result != RT_EOK)
    {
        rt_kprintf("[WARN] adc: read failed %d\r\n", result);
        return;
    }

    rt_kprintf("[INFO] adc: pc0_ntc_adc1_ch10=%u ntc_mc=%d ntc_valid=%u pc1_surge_adc2_ch11=%u\r\n",
               sample.ntc_raw,
               sample.ntc_temp_milli_c,
               sample.ntc_temp_valid,
               sample.surge_raw);
}

static int app_shell_adc(int argc, char **argv)
{
    rt_uint32_t count = APP_SHELL_ADC_WATCH_DEFAULT_COUNT;
    rt_uint32_t period_ms = APP_SHELL_ADC_WATCH_DEFAULT_PERIOD_MS;
    rt_uint32_t i;

    if (argc == 1)
    {
        app_shell_print_adc_sample();
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "watch") == 0)
    {
        if (argc >= 3 && app_shell_parse_u32_arg(argv[2], &count) != RT_EOK)
        {
            rt_kprintf("[WARN] adc: count must be a positive integer\r\n");
            return 0;
        }

        if (argc >= 4 && app_shell_parse_u32_arg(argv[3], &period_ms) != RT_EOK)
        {
            rt_kprintf("[WARN] adc: period_ms must be a positive integer\r\n");
            return 0;
        }

        if (argc > 4 || count == 0U ||
            count > APP_SHELL_ADC_WATCH_MAX_COUNT ||
            period_ms < APP_SHELL_ADC_WATCH_MIN_PERIOD_MS ||
            period_ms > APP_SHELL_ADC_WATCH_MAX_PERIOD_MS)
        {
            rt_kprintf("[WARN] adc: usage adc watch [count<=%u] [period_ms %u..%u]\r\n",
                       APP_SHELL_ADC_WATCH_MAX_COUNT,
                       APP_SHELL_ADC_WATCH_MIN_PERIOD_MS,
                       APP_SHELL_ADC_WATCH_MAX_PERIOD_MS);
            return 0;
        }

        for (i = 0U; i < count; i++)
        {
            app_shell_print_adc_sample();
            if ((i + 1U) < count)
            {
                rt_thread_mdelay(period_ms);
            }
        }

        return 0;
    }

    rt_kprintf("[WARN] adc: usage adc | adc watch [count] [period_ms]\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_adc, adc, show PC0/PC1 ADC raw state);

static int app_shell_ping(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("[INFO] shell: pong tick=%u\r\n", rt_tick_get());
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_ping, ping, development ping command);

static int app_shell_tick(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("[INFO] shell: tick=%u\r\n", rt_tick_get());
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_tick, tick, show RT-Thread tick);

static int app_shell_sys(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "info") == 0))
    {
        rt_kprintf("[INFO] sys: mcu=AT32F403ARGT7 rtos=RT-Thread tick=%u tick_hz=%u prio_max=%u\r\n",
                   rt_tick_get(),
                   RT_TICK_PER_SECOND,
                   RT_THREAD_PRIORITY_MAX);
        rt_kprintf("[INFO] sys: finsh name=%s prio=%u stack=%u cmd=%u arg_max=%u\r\n",
                   FINSH_THREAD_NAME,
                   FINSH_THREAD_PRIORITY,
                   FINSH_THREAD_STACK_SIZE,
                   FINSH_CMD_SIZE,
                   FINSH_ARG_MAX);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "tick") == 0)
    {
        rt_kprintf("[INFO] sys: tick=%u\r\n", rt_tick_get());
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "threads") == 0)
    {
        rt_object_t objects[APP_SHELL_THREAD_LIST_MAX];
        int total;
        int copied;
        int i;

        total = rt_object_get_length(RT_Object_Class_Thread);
        copied = rt_object_get_pointers(RT_Object_Class_Thread,
                                        objects,
                                        APP_SHELL_THREAD_LIST_MAX);

        rt_kprintf("thread           prio stat     stack used/size\r\n");
        for (i = 0; i < copied; i++)
        {
            struct rt_thread *thread = (struct rt_thread *)objects[i];
            rt_uint32_t used = app_shell_thread_stack_used(thread);

            rt_kprintf("%-16s %4u %-8s %5u/%-5u\r\n",
                       thread->parent.name,
                       RT_SCHED_PRIV(thread).current_priority,
                       app_shell_thread_state_text(RT_SCHED_CTX(thread).stat),
                       used,
                       thread->stack_size);
        }

        if (total > copied)
        {
            rt_kprintf("[WARN] sys: thread list truncated copied=%d total=%d\r\n", copied, total);
        }

        return 0;
    }

    rt_kprintf("[WARN] sys: usage sys [info|tick|threads]\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_sys, sys, show system runtime information);

static int app_shell_data(int argc, char **argv)
{
    rt_uint32_t count = APP_SHELL_DATA_WATCH_DEFAULT_COUNT;
    rt_uint32_t period_ms = APP_SHELL_DATA_WATCH_DEFAULT_PERIOD_MS;
    rt_uint32_t i;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "show") == 0))
    {
        app_shell_print_snapshot();
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "watch") == 0)
    {
        if (argc >= 3 && app_shell_parse_u32_arg(argv[2], &count) != RT_EOK)
        {
            rt_kprintf("[WARN] data: count must be a positive integer\r\n");
            return 0;
        }

        if (argc >= 4 && app_shell_parse_u32_arg(argv[3], &period_ms) != RT_EOK)
        {
            rt_kprintf("[WARN] data: period_ms must be a positive integer\r\n");
            return 0;
        }

        if (argc > 4 || count == 0U ||
            count > APP_SHELL_DATA_WATCH_MAX_COUNT ||
            period_ms < APP_SHELL_DATA_WATCH_MIN_PERIOD_MS ||
            period_ms > APP_SHELL_DATA_WATCH_MAX_PERIOD_MS)
        {
            rt_kprintf("[WARN] data: usage data watch [count<=%u] [period_ms %u..%u]\r\n",
                       APP_SHELL_DATA_WATCH_MAX_COUNT,
                       APP_SHELL_DATA_WATCH_MIN_PERIOD_MS,
                       APP_SHELL_DATA_WATCH_MAX_PERIOD_MS);
            return 0;
        }

        for (i = 0U; i < count; i++)
        {
            app_shell_print_snapshot();
            if ((i + 1U) < count)
            {
                rt_thread_mdelay(period_ms);
            }
        }

        return 0;
    }

    rt_kprintf("[WARN] data: usage data [show] | data watch [count] [period_ms]\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_data, data, show data_service snapshot);

static void app_shell_print_surge_snapshot(void)
{
    surge_service_snapshot_t snapshot;

    surge_service_get_snapshot(&snapshot);
    rt_kprintf("[INFO] surge: status=%s current=%u baseline=%u peak=%u delta=%u threshold=%u triggers=%u source=%u\r\n",
               surge_service_status_name(snapshot.status),
               snapshot.current_raw,
               snapshot.baseline_raw,
               snapshot.peak_raw,
               snapshot.peak_delta_raw,
               snapshot.threshold_delta_raw,
               snapshot.trigger_count,
               snapshot.source);
    rt_kprintf("[INFO] surge: last_update_ms=%u last_trigger_ms=%u calibrated=0\r\n",
               snapshot.last_update_ms,
               snapshot.last_trigger_ms);
}

static int app_shell_surge(int argc, char **argv)
{
    rt_uint32_t value;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "status") == 0))
    {
        app_shell_print_surge_snapshot();
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arm") == 0)
    {
        surge_service_arm();
        app_shell_print_surge_snapshot();
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0)
    {
        surge_service_clear();
        app_shell_print_surge_snapshot();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "threshold") == 0)
    {
        if (app_shell_parse_u32_arg(argv[2], &value) != RT_EOK || value > 4095U)
        {
            rt_kprintf("[WARN] surge: threshold must be 1..4095 raw counts\r\n");
            return 0;
        }

        if (surge_service_set_threshold((rt_uint16_t)value) != RT_EOK)
        {
            rt_kprintf("[WARN] surge: threshold set failed\r\n");
            return 0;
        }

        app_shell_print_surge_snapshot();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "inject") == 0)
    {
        if (app_shell_parse_u32_arg(argv[2], &value) != RT_EOK || value > 4095U)
        {
            rt_kprintf("[WARN] surge: inject raw must be 0..4095\r\n");
            return 0;
        }

        surge_service_inject_raw((rt_uint16_t)value, rt_tick_get_millisecond());
        app_shell_print_surge_snapshot();
        return 0;
    }

    rt_kprintf("[WARN] surge: usage surge [status] | arm | clear | threshold <delta_raw> | inject <raw>\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_surge, surge, inspect or inject surge detector state);

static void app_shell_print_param_item(param_service_id_t id)
{
    param_service_item_t item;
    rt_uint32_t uvalue;

    if (param_service_get_item(id, &item) != RT_EOK)
    {
        rt_kprintf("[WARN] param: failed to read id=%u\r\n", (rt_uint32_t)id);
        return;
    }

    if (item.value_type == PARAM_SERVICE_VALUE_IPADDR)
    {
        uvalue = (rt_uint32_t)item.value;
        rt_kprintf("%-28s hreg=%u count=%u value=%u.%u.%u.%u default=",
                   item.name,
                   item.holding_register,
                   item.holding_count,
                   (uvalue >> 24) & 0xFFU,
                   (uvalue >> 16) & 0xFFU,
                   (uvalue >> 8) & 0xFFU,
                   uvalue & 0xFFU);
        uvalue = (rt_uint32_t)item.default_value;
        rt_kprintf("%u.%u.%u.%u %s\r\n",
                   (uvalue >> 24) & 0xFFU,
                   (uvalue >> 16) & 0xFFU,
                   (uvalue >> 8) & 0xFFU,
                   uvalue & 0xFFU,
                   item.unit);
        return;
    }

    rt_kprintf("%-28s hreg=%u count=%u value=%d default=%d range=%d..%d %s\r\n",
               item.name,
               item.holding_register,
               item.holding_count,
               item.value,
               item.default_value,
               item.min_value,
               item.max_value,
               item.unit);
}

static int app_shell_param(int argc, char **argv)
{
    param_service_id_t id;
    rt_int32_t value;
    rt_uint32_t i;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "list") == 0))
    {
        for (i = 0U; i < (rt_uint32_t)param_service_count(); i++)
        {
            app_shell_print_param_item((param_service_id_t)i);
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "get") == 0)
    {
        if (param_service_find(argv[2], &id) != RT_EOK)
        {
            rt_kprintf("[WARN] param: unknown name '%s'\r\n", argv[2]);
            return 0;
        }

        app_shell_print_param_item(id);
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "set") == 0)
    {
        if (param_service_find(argv[2], &id) != RT_EOK)
        {
            rt_kprintf("[WARN] param: unknown name '%s'\r\n", argv[2]);
            return 0;
        }

        if (app_shell_parse_i32_arg(argv[3], &value) != RT_EOK)
        {
            rt_kprintf("[WARN] param: value must be an integer\r\n");
            return 0;
        }

        if (backend_request_set_param(id, value) != RT_EOK)
        {
            rt_kprintf("[WARN] param: set failed, check range\r\n");
            app_shell_print_param_item(id);
            return 0;
        }

        app_shell_print_param_item(id);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "defaults") == 0)
    {
        (void)backend_request_reset_params();
        rt_kprintf("[INFO] param: runtime defaults restored\r\n");
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "status") == 0)
    {
        param_service_status_t status;

        if (param_service_get_status(&status) != RT_EOK)
        {
            rt_kprintf("[WARN] param: status unavailable\r\n");
            return 0;
        }

        rt_kprintf("[INFO] param: storage=eeprom_kvdb persistent_ready=%u dirty=%u version=%u last_update_ms=%u count=%u\r\n",
                   status.persistent_ready,
                   status.dirty,
                   status.version,
                   status.last_update_ms,
                   (rt_uint32_t)status.count);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "save") == 0)
    {
        rt_err_t result = backend_request_save_params();

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] param: saved to EEPROM KVDB\r\n");
        }
        else
        {
            rt_kprintf("[WARN] param: save failed %d\r\n", result);
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "load") == 0)
    {
        rt_err_t result = backend_request_load_params();

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] param: loaded from EEPROM KVDB\r\n");
        }
        else
        {
            rt_kprintf("[WARN] param: load failed %d\r\n", result);
        }
        return 0;
    }

    rt_kprintf("[WARN] param: usage param [list] | get <name> | set <name> <value> | defaults | status | save | load\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_param, param, inspect or change runtime parameters);

static int app_shell_record(int argc, char **argv)
{
    record_service_status_t status;
    record_service_item_t item;
    rt_int32_t peak_milli_kv;
    rt_uint32_t index;
    rt_size_t i;
    rt_err_t result;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "status") == 0))
    {
        result = record_service_get_status(&status);
        if (result != RT_EOK)
        {
            rt_kprintf("[WARN] record: status unavailable %d\r\n", result);
            return 0;
        }

        rt_kprintf("[INFO] record: generation=%u count=%u capacity=%u total_written=%u next_seq=%u persistent_ready=%u\r\n",
                   status.generation,
                   (rt_uint32_t)status.count,
                   (rt_uint32_t)status.capacity,
                   status.total_written,
                   status.next_sequence,
                   status.persistent_ready);
        rt_kprintf("[INFO] record: persist pending=%u written=%u failed=%u dropped=%u\r\n",
                   status.persist_pending,
                   status.persist_written,
                   status.persist_failed,
                   status.persist_dropped);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "add") == 0)
    {
        result = record_service_append_snapshot(RECORD_SERVICE_EVENT_MANUAL_TEST);
        if (result == RT_EOK)
        {
            (void)record_service_get_latest(&item);
            app_shell_print_record_item(&item);
        }
        else
        {
            rt_kprintf("[WARN] record: add failed %d\r\n", result);
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "add") == 0)
    {
        if (app_shell_parse_i32_arg(argv[2], &peak_milli_kv) != RT_EOK)
        {
            rt_kprintf("[WARN] record: peak_milli_kv must be an integer\r\n");
            return 0;
        }

        result = record_service_append_manual_test(peak_milli_kv);
        if (result == RT_EOK)
        {
            (void)record_service_get_latest(&item);
            app_shell_print_record_item(&item);
        }
        else
        {
            rt_kprintf("[WARN] record: add failed %d\r\n", result);
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "latest") == 0)
    {
        result = record_service_get_latest(&item);
        if (result == RT_EOK)
        {
            app_shell_print_record_item(&item);
        }
        else
        {
            rt_kprintf("[WARN] record: no records %d\r\n", result);
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "list") == 0)
    {
        result = record_service_get_status(&status);
        if (result != RT_EOK)
        {
            rt_kprintf("[WARN] record: status unavailable %d\r\n", result);
            return 0;
        }

        for (i = 0U; i < status.count; i++)
        {
            if (record_service_get_by_index(i, &item) == RT_EOK)
            {
                app_shell_print_record_item(&item);
            }
        }
        return 0;
    }

    if (argc >= 2 && argc <= 4 && strcmp(argv[1], "alarms") == 0)
    {
        static record_service_item_t items[APP_SHELL_RECORD_ALARM_PAGE_MAX];
        rt_uint32_t offset = 0U;
        rt_uint32_t count = APP_SHELL_RECORD_ALARM_PAGE_DEFAULT;
        rt_size_t actual_count = 0U;
        rt_size_t total_count = 0U;

        if (argc >= 3 && app_shell_parse_u32_arg(argv[2], &offset) != RT_EOK)
        {
            rt_kprintf("[WARN] record: alarm offset must be an integer\r\n");
            return 0;
        }
        if (argc >= 4 && app_shell_parse_u32_arg(argv[3], &count) != RT_EOK)
        {
            rt_kprintf("[WARN] record: alarm count must be an integer\r\n");
            return 0;
        }

        if (count == 0U)
        {
            rt_kprintf("[WARN] record: alarm count must be > 0\r\n");
            return 0;
        }
        if (count > APP_SHELL_RECORD_ALARM_PAGE_MAX)
        {
            count = APP_SHELL_RECORD_ALARM_PAGE_MAX;
        }

        result = record_service_query_alarm_latest_page((rt_size_t)offset,
                                                        (rt_size_t)count,
                                                        items,
                                                        &actual_count,
                                                        &total_count);
        rt_kprintf("[INFO] record alarms: offset=%u request=%u actual=%u total=%u result=%d\r\n",
                   offset,
                   count,
                   (rt_uint32_t)actual_count,
                   (rt_uint32_t)total_count,
                   result);
        for (i = 0U; i < actual_count; i++)
        {
            app_shell_print_record_item(&items[i]);
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "show") == 0)
    {
        if (app_shell_parse_u32_arg(argv[2], &index) != RT_EOK)
        {
            rt_kprintf("[WARN] record: index must be an integer\r\n");
            return 0;
        }

        result = record_service_get_by_index((rt_size_t)index, &item);
        if (result == RT_EOK)
        {
            app_shell_print_record_item(&item);
        }
        else
        {
            rt_kprintf("[WARN] record: show failed %d\r\n", result);
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0)
    {
        (void)backend_request_clear_records();
        rt_kprintf("[INFO] record: runtime records cleared\r\n");
        return 0;
    }

    rt_kprintf("[WARN] record: usage record [status] | add [peak_milli_kv] | latest | list | alarms [offset] [count] | show <index> | clear\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_record, record, inspect or append backend records);

static int app_shell_storage(int argc, char **argv)
{
    RT_UNUSED(argv);

    if (argc > 1)
    {
        rt_kprintf("[WARN] storage: usage storage\r\n");
        return 0;
    }

    rt_kprintf("[INFO] storage: flashdb_ready=%u last_time=%u\r\n",
               storage_flashdb_ready() ? 1U : 0U,
               storage_flashdb_last_time());
    rt_kprintf("[INFO] storage: eeprom_kvdb=%s alarm_tsdb_count=%u surge_tsdb_count=%u\r\n",
               (storage_flashdb_get_param_kvdb() != RT_NULL) ? "ready" : "not_ready",
               (rt_uint32_t)storage_flashdb_record_count(STORAGE_FLASHDB_RECORD_KIND_ALARM),
               (rt_uint32_t)storage_flashdb_record_count(STORAGE_FLASHDB_RECORD_KIND_SURGE));
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_storage, storage, show FlashDB backend status);

static int app_shell_factory(int argc, char **argv)
{
    factory_service_info_t info;
    rt_uint8_t mac[6];
    rt_err_t result;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "status") == 0))
    {
        result = factory_service_get_info(&info);
        if (result != RT_EOK)
        {
            rt_kprintf("[WARN] factory: status unavailable %d\r\n", result);
            return 0;
        }

        rt_kprintf("[INFO] factory: version=%u mac_valid=%u\r\n",
                   info.version,
                   info.mac_valid);
        if (info.mac_valid != 0U)
        {
            app_shell_print_mac("[INFO] factory: mac=", info.mac);
        }
        else
        {
            rt_kprintf("[INFO] factory: mac=<unset>\r\n");
        }
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "mac") == 0 && strcmp(argv[2], "set") == 0)
    {
        if (app_shell_parse_mac(argv[3], mac) == RT_FALSE)
        {
            rt_kprintf("[WARN] factory: mac format must be xx:xx:xx:xx:xx:xx\r\n");
            return 0;
        }

        result = backend_request_set_factory_mac(mac);
        if (result == RT_EOK)
        {
            app_shell_print_mac("[INFO] factory: mac saved ", mac);
        }
        else
        {
            rt_kprintf("[WARN] factory: mac save failed %d\r\n", result);
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0)
    {
        result = factory_service_clear();
        rt_kprintf("[INFO] factory: clear result=%d\r\n", result);
        return 0;
    }

    rt_kprintf("[WARN] factory: usage factory [status] | mac set xx:xx:xx:xx:xx:xx | clear\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_factory, factory, inspect or write factory area);

static int app_shell_backend(int argc, char **argv)
{
    param_service_status_t param_status;
    record_service_status_t record_status;
    device_data_snapshot_t snapshot;

    RT_UNUSED(argv);

    if (argc > 1)
    {
        rt_kprintf("[WARN] backend: usage backend\r\n");
        return 0;
    }

    data_service_get_snapshot(&snapshot);
    if (param_service_get_status(&param_status) != RT_EOK)
    {
        param_status.version = 0U;
        param_status.dirty = 0U;
        param_status.persistent_ready = 0U;
        param_status.count = 0U;
        param_status.last_update_ms = 0U;
    }
    if (record_service_get_status(&record_status) != RT_EOK)
    {
        record_status.count = 0U;
        record_status.capacity = 0U;
        record_status.next_sequence = 0U;
        record_status.total_written = 0U;
        record_status.persistent_ready = 0U;
    }

    rt_kprintf("[INFO] backend: data_update=%u time_valid=%u sensor_flags=0x%02x\r\n",
               snapshot.update_count,
               snapshot.time_valid,
               snapshot.sensor_valid_flags);
    rt_kprintf("[INFO] backend: param_count=%u param_ver=%u dirty=%u persistent=%u\r\n",
               (rt_uint32_t)param_status.count,
               param_status.version,
               param_status.dirty,
               param_status.persistent_ready);
    rt_kprintf("[INFO] backend: record_count=%u/%u total_written=%u persistent=%u\r\n",
               (rt_uint32_t)record_status.count,
               (rt_uint32_t)record_status.capacity,
               record_status.total_written,
               record_status.persistent_ready);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_backend, backend, show backend service status);

static int app_shell_rtc(int argc, char **argv)
{
    app_rtc_datetime_t datetime;
    rt_err_t result;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "get") == 0))
    {
        result = data_service_get_datetime(&datetime);
        if (result != RT_EOK)
        {
            rt_kprintf("[WARN] rtc: time invalid or DS1302 read failed (%d)\r\n", result);
            return 0;
        }

        app_shell_print_datetime("rtc", &datetime);
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "set") == 0)
    {
        if (app_shell_parse_datetime(argv[2], argv[3], &datetime) == RT_FALSE)
        {
            rt_kprintf("[WARN] rtc: usage rtc set YYYY-MM-DD HH:MM:SS\r\n");
            return 0;
        }

        result = data_service_set_datetime(&datetime);
        if (result != RT_EOK)
        {
            rt_kprintf("[WARN] rtc: set failed (%d)\r\n", result);
            return 0;
        }

        return app_shell_rtc(1, argv);
    }

    rt_kprintf("[WARN] rtc: usage rtc [get] | rtc set YYYY-MM-DD HH:MM:SS\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_rtc, rtc, read or set DS1302 time);

static int app_shell_forward_bringup(int argc, char **argv)
{
    char command[96];
    rt_size_t offset = 0U;
    int i;

    if (argc <= 0)
    {
        return 0;
    }

    command[0] = '\0';
    for (i = 0; i < argc; i++)
    {
        int written;

        written = rt_snprintf(command + offset,
                              sizeof(command) - offset,
                              "%s%s",
                              (i == 0) ? "" : " ",
                              argv[i]);
        if (written < 0)
        {
            return 0;
        }

        if ((rt_size_t)written >= (sizeof(command) - offset))
        {
            rt_kprintf("[WARN] bringup: command too long\r\n");
            return 0;
        }

        offset += (rt_size_t)written;
    }

    if (app_bringup_process_command(command) == RT_FALSE)
    {
        rt_kprintf("[WARN] bringup: unsupported command '%s'\r\n", command);
    }

    return 0;
}

static int app_shell_key(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_key, key, key bring-up command);

static int app_shell_led(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_led, led, led bring-up command);

#if defined(SURGE_ENABLE_UI) && (SURGE_ENABLE_UI != 0)
static int app_shell_lcd(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_lcd, lcd, lcd bring-up command);
#endif

static int app_shell_hdc1080(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_hdc1080, hdc1080, HDC1080 sensor bring-up command);

static int app_shell_eeprom(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_eeprom, eeprom, AT24C128C EEPROM bring-up command);

static int app_shell_w5500(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_w5500, w5500, W5500 Ethernet bring-up command);

static int app_shell_bringup(int argc, char **argv)
{
    if (argc > 1)
    {
        return app_shell_forward_bringup(argc - 1, argv + 1);
    }

    app_bringup_print_help();
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_bringup, bringup, show bring-up commands);

void app_shell_init(void)
{
    int result;

    result = finsh_system_init();
    if (result == 0)
    {
        finsh_set_prompt("msh ");
        rt_kprintf("[INFO] msh: development shell started prio=%u stack=%u\r\n",
                   FINSH_THREAD_PRIORITY,
                   FINSH_THREAD_STACK_SIZE);
    }
    else
    {
        rt_kprintf("[ERROR] msh: finsh_system_init failed %d\r\n", result);
    }
}
