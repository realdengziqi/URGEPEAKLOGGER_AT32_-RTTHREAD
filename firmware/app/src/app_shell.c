#include "app_shell.h"

#include <string.h>

#include "app_bringup.h"
#include "app_data_service.h"
#include "app_param_service.h"
#include "finsh.h"
#include "shell.h"

#define APP_SHELL_DATA_WATCH_DEFAULT_COUNT      5U
#define APP_SHELL_DATA_WATCH_MAX_COUNT          30U
#define APP_SHELL_DATA_WATCH_DEFAULT_PERIOD_MS  1000U
#define APP_SHELL_DATA_WATCH_MIN_PERIOD_MS      200U
#define APP_SHELL_DATA_WATCH_MAX_PERIOD_MS      10000U
#define APP_SHELL_THREAD_LIST_MAX               12

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
    rt_kprintf("[INFO] data sensor: env_temp_mc=%d humi_mpermil=%d sensor_flags=0x%02x\r\n",
               snapshot.env_temp_milli_c,
               snapshot.env_humi_milli_percent,
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

static void app_shell_print_param_item(param_service_id_t id)
{
    param_service_item_t item;

    if (param_service_get_item(id, &item) != RT_EOK)
    {
        rt_kprintf("[WARN] param: failed to read id=%u\r\n", (rt_uint32_t)id);
        return;
    }

    rt_kprintf("%-28s value=%d default=%d range=%d..%d %s\r\n",
               item.name,
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

        if (param_service_set_i32(id, value) != RT_EOK)
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
        param_service_reset_defaults();
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

        rt_kprintf("[INFO] param: runtime_only=1 persistent_ready=%u dirty=%u version=%u last_update_ms=%u count=%u\r\n",
                   status.persistent_ready,
                   status.dirty,
                   status.version,
                   status.last_update_ms,
                   (rt_uint32_t)status.count);
        return 0;
    }

    if ((argc == 2 && strcmp(argv[1], "save") == 0) ||
        (argc == 2 && strcmp(argv[1], "load") == 0))
    {
        rt_kprintf("[WARN] param: persistent storage is not connected yet\r\n");
        return 0;
    }

    rt_kprintf("[WARN] param: usage param [list] | get <name> | set <name> <value> | defaults | status | save | load\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_param, param, inspect or change runtime parameters);

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

static int app_shell_lcd(int argc, char **argv)
{
    return app_shell_forward_bringup(argc, argv);
}
MSH_CMD_EXPORT_ALIAS(app_shell_lcd, lcd, lcd bring-up command);

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
