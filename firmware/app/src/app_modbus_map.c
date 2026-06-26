#include "app_modbus_map.h"

#include <rthw.h>
#include <string.h>

#include "app_alarm_service.h"
#include "app_backend_request.h"
#include "app_data_service.h"
#include "app_factory_service.h"
#include "app_modbus_tcp.h"
#include "app_param_service.h"
#include "app_record_service.h"
#include "mb.h"

#define MODBUS_PARAM_REG_START    1U
#define MODBUS_PARAM_REG_COUNT    99U
#define MODBUS_FACTORY_MAC_REG_START 64U
#define MODBUS_FACTORY_MAC_REG_COUNT 6U
#define MODBUS_COMMAND_REG_START  100U
#define MODBUS_COMMAND_REG_COUNT  8U
#define MODBUS_COMMAND_UNLOCK_KEY 0xA55AU
#define MODBUS_COMMAND_UNLOCK_TIMEOUT_MS 5000U
#define MODBUS_RECORD_QUERY_REG_START 120U
#define MODBUS_RECORD_QUERY_REG_COUNT 20U
#define MODBUS_RECORD_QUERY_MODE_IDLE 0U
#define MODBUS_RECORD_QUERY_MODE_LATEST 1U
#define MODBUS_RECORD_QUERY_MODE_SEQUENCE 2U
#define MODBUS_RECORD_QUERY_MODE_ALARM_LATEST 3U
#define MODBUS_RECORD_QUERY_THREAD_STACK_SIZE 1536U
#define MODBUS_RECORD_QUERY_THREAD_PRIORITY   19U
#define MODBUS_RECORD_QUERY_THREAD_TIMESLICE  10U
#define MODBUS_ALARM_PAGE_REG_START 140U
#define MODBUS_ALARM_PAGE_REG_COUNT 10U
#define MODBUS_ALARM_PAGE_COMMAND_IDLE 0U
#define MODBUS_ALARM_PAGE_COMMAND_LOAD 1U
#define MODBUS_ALARM_PAGE_COMMAND_NEXT 2U
#define MODBUS_ALARM_PAGE_COMMAND_PREV 3U
#define MODBUS_ALARM_PAGE_COMMAND_FIRST 4U
#define MODBUS_ALARM_PAGE_DEFAULT_SIZE 4U
#define MODBUS_ALARM_PAGE_MAX_SIZE 4U
#define MODBUS_ALARM_PAGE_QUERY_MAX_COUNT (MODBUS_ALARM_PAGE_MAX_SIZE + 1U)
#define MODBUS_ALARM_PAGE_THREAD_STACK_SIZE 2048U
#define MODBUS_ALARM_PAGE_THREAD_PRIORITY   19U
#define MODBUS_ALARM_PAGE_THREAD_TIMESLICE  10U
#define MODBUS_INPUT_REG_START    1U
#define MODBUS_INPUT_REG_COUNT    259U
#define MODBUS_INPUT_ACTIVE_ALARM_BASE    25U
#define MODBUS_INPUT_ACTIVE_ALARM_STRIDE  10U
#define MODBUS_INPUT_ACTIVE_ALARM_COUNT   6U
#define MODBUS_INPUT_LATEST_RECORD_BASE   85U
#define MODBUS_INPUT_RECORD_QUERY_BASE     120U
#define MODBUS_INPUT_RECORD_QUERY_COUNT    40U
#define MODBUS_INPUT_ALARM_PAGE_BASE       160U
#define MODBUS_INPUT_ALARM_PAGE_COUNT      80U
#define MODBUS_INPUT_ALARM_PAGE_HEADER_COUNT 12U
#define MODBUS_INPUT_ALARM_PAGE_STRIDE     17U
#define MODBUS_INPUT_NET_STATUS_BASE       240U
#define MODBUS_INPUT_NET_STATUS_COUNT      20U

static volatile rt_uint32_t modbus_holding_reads;
static volatile rt_uint32_t modbus_holding_writes;
static volatile rt_uint32_t modbus_input_reads;
static volatile rt_uint16_t modbus_command_unlock_key;
static volatile rt_uint32_t modbus_command_unlock_ms;

typedef struct
{
    device_data_snapshot_t snapshot;
    record_service_status_t record_status;
    param_service_status_t param_status;
    alarm_service_snapshot_t alarm_snapshot;
    app_modbus_tcp_status_t tcp_status;
    record_service_item_t latest_record;
    rt_uint8_t latest_record_valid;
} modbus_input_context_t;

typedef struct
{
    rt_uint16_t mode;
    rt_uint16_t latest_offset;
    rt_uint32_t query_sequence;
    rt_int16_t result;
    rt_uint32_t generation;
    rt_uint8_t valid;
    rt_uint8_t busy;
    rt_size_t result_count;
    record_service_item_t item;
} modbus_record_query_context_t;

typedef struct
{
    rt_uint16_t command;
    rt_uint16_t requested_page;
    rt_uint16_t page_size;
    rt_uint16_t busy;
    rt_uint16_t done;
    rt_int16_t result;
    rt_uint16_t current_page;
    rt_uint16_t item_count;
    rt_uint32_t generation;
    rt_uint16_t has_prev;
    rt_uint16_t has_next;
    record_service_item_t items[MODBUS_ALARM_PAGE_MAX_SIZE];
} modbus_alarm_page_context_t;

static modbus_record_query_context_t modbus_record_query_ctx;
static rt_sem_t modbus_record_query_sem;
static rt_thread_t modbus_record_query_thread;
static modbus_alarm_page_context_t modbus_alarm_page_ctx = {
    .page_size = MODBUS_ALARM_PAGE_DEFAULT_SIZE
};
static rt_sem_t modbus_alarm_page_sem;
static rt_thread_t modbus_alarm_page_thread;

static rt_bool_t modbus_record_query_is_busy(void)
{
    rt_uint8_t busy;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    busy = modbus_record_query_ctx.busy;
    rt_hw_interrupt_enable(level);

    return (busy != 0U) ? RT_TRUE : RT_FALSE;
}

static rt_uint16_t modbus_alarm_page_sanitize_size(rt_uint16_t page_size)
{
    if (page_size == 0U)
    {
        return MODBUS_ALARM_PAGE_DEFAULT_SIZE;
    }
    if (page_size > MODBUS_ALARM_PAGE_MAX_SIZE)
    {
        return MODBUS_ALARM_PAGE_MAX_SIZE;
    }

    return page_size;
}

static rt_bool_t modbus_alarm_page_is_busy(void)
{
    rt_uint16_t busy;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    busy = modbus_alarm_page_ctx.busy;
    rt_hw_interrupt_enable(level);

    return (busy != 0U) ? RT_TRUE : RT_FALSE;
}

static void modbus_alarm_page_get_snapshot(modbus_alarm_page_context_t *snapshot)
{
    rt_base_t level;

    if (snapshot == RT_NULL)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    *snapshot = modbus_alarm_page_ctx;
    rt_hw_interrupt_enable(level);
}

static void modbus_record_query_get_snapshot(modbus_record_query_context_t *snapshot)
{
    rt_base_t level;

    if (snapshot == RT_NULL)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    *snapshot = modbus_record_query_ctx;
    rt_hw_interrupt_enable(level);
}

static void modbus_record_query_invalidate(void)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    modbus_record_query_ctx.valid = 0U;
    modbus_record_query_ctx.result_count = 0U;
    modbus_record_query_ctx.generation = 0U;
    memset(&modbus_record_query_ctx.item, 0, sizeof(modbus_record_query_ctx.item));
    rt_hw_interrupt_enable(level);
}

static void modbus_alarm_page_invalidate(void)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    modbus_alarm_page_ctx.done = 0U;
    modbus_alarm_page_ctx.result = 0;
    modbus_alarm_page_ctx.item_count = 0U;
    modbus_alarm_page_ctx.generation = 0U;
    modbus_alarm_page_ctx.has_prev = 0U;
    modbus_alarm_page_ctx.has_next = 0U;
    memset(modbus_alarm_page_ctx.items, 0, sizeof(modbus_alarm_page_ctx.items));
    rt_hw_interrupt_enable(level);
}

static rt_uint8_t modbus_record_query_snapshot_is_valid(const modbus_record_query_context_t *snapshot)
{
    record_service_status_t status;

    if (snapshot == RT_NULL || snapshot->valid == 0U)
    {
        return 0U;
    }

    memset(&status, 0, sizeof(status));
    if (record_service_get_status(&status) != RT_EOK)
    {
        return 0U;
    }

    return (status.generation == snapshot->generation) ? 1U : 0U;
}

static rt_uint8_t modbus_alarm_page_snapshot_is_current(const modbus_alarm_page_context_t *snapshot)
{
    record_service_status_t status;

    if (snapshot == RT_NULL || snapshot->done == 0U || snapshot->result != RT_EOK)
    {
        return 0U;
    }

    memset(&status, 0, sizeof(status));
    if (record_service_get_status(&status) != RT_EOK)
    {
        return 0U;
    }

    return (status.generation == snapshot->generation) ? 1U : 0U;
}

static void modbus_put_u16(UCHAR **buffer, rt_uint16_t value)
{
    *(*buffer)++ = (UCHAR)(value >> 8);
    *(*buffer)++ = (UCHAR)(value & 0xFFU);
}

static rt_uint16_t modbus_get_u16(UCHAR **buffer)
{
    rt_uint16_t value;

    value = (rt_uint16_t)((rt_uint16_t)(*(*buffer)++) << 8);
    value |= (rt_uint16_t)(*(*buffer)++);

    return value;
}

static rt_bool_t modbus_range_in(USHORT address, USHORT nregs, USHORT start, USHORT count)
{
    if (nregs == 0U)
    {
        return RT_FALSE;
    }

    return (address >= start &&
            ((address + nregs - 1U) < (start + count))) ? RT_TRUE : RT_FALSE;
}

static rt_bool_t modbus_command_unlock_active(void)
{
    rt_uint32_t now = rt_tick_get_millisecond();

    if (modbus_command_unlock_key != MODBUS_COMMAND_UNLOCK_KEY)
    {
        return RT_FALSE;
    }

    return ((now - modbus_command_unlock_ms) <= MODBUS_COMMAND_UNLOCK_TIMEOUT_MS) ?
           RT_TRUE : RT_FALSE;
}

static void modbus_command_unlock_clear(void)
{
    modbus_command_unlock_key = 0U;
    modbus_command_unlock_ms = 0U;
}

static rt_err_t modbus_submit_command(rt_uint16_t command)
{
    switch (command)
    {
    case 0U:
        return RT_EOK;

    case BACKEND_REQUEST_PUBLIC_SAVE_PARAMS:
        return backend_request_save_params_async();

    case BACKEND_REQUEST_PUBLIC_LOAD_PARAMS:
        return backend_request_load_params_async();

    case BACKEND_REQUEST_PUBLIC_RESET_PARAMS:
        if (modbus_command_unlock_active() == RT_FALSE)
        {
            return -RT_EINVAL;
        }
        modbus_command_unlock_clear();
        return backend_request_reset_params_async();

    case BACKEND_REQUEST_PUBLIC_CLEAR_RECORDS:
        if (modbus_command_unlock_active() == RT_FALSE)
        {
            return -RT_EINVAL;
        }
        modbus_command_unlock_clear();
        return backend_request_clear_records_async();

    case BACKEND_REQUEST_PUBLIC_APPLY_MODBUS_RTU:
        return backend_request_apply_modbus_rtu_async();

    case BACKEND_REQUEST_PUBLIC_APPLY_NETWORK:
        return backend_request_apply_network_async();

    default:
        return -RT_EINVAL;
    }
}

static rt_uint16_t modbus_param_to_reg(param_service_id_t id, rt_int32_t value, rt_uint16_t holding_offset)
{
    if (id == PARAM_SERVICE_ID_ALARM_PEAK_MILLI_KV)
    {
        return (rt_uint16_t)(value / 100);
    }

    if (id == PARAM_SERVICE_ID_NTC_HIGH_ALARM_THRESHOLD_MC ||
        id == PARAM_SERVICE_ID_NTC_LOW_ALARM_THRESHOLD_MC)
    {
        return (rt_uint16_t)(rt_int16_t)(value / 10);
    }

    if (id == PARAM_SERVICE_ID_NET_IP_ADDR ||
        id == PARAM_SERVICE_ID_NET_NETMASK ||
        id == PARAM_SERVICE_ID_NET_GATEWAY)
    {
        rt_uint32_t uvalue = (rt_uint32_t)value;
        return (rt_uint16_t)((uvalue >> ((3U - holding_offset) * 8U)) & 0xFFU);
    }

    return (rt_uint16_t)value;
}

static rt_int32_t modbus_reg_to_param(param_service_id_t id, rt_uint16_t value)
{
    if (id == PARAM_SERVICE_ID_ALARM_PEAK_MILLI_KV)
    {
        return (rt_int32_t)value * 100;
    }

    if (id == PARAM_SERVICE_ID_NTC_HIGH_ALARM_THRESHOLD_MC ||
        id == PARAM_SERVICE_ID_NTC_LOW_ALARM_THRESHOLD_MC)
    {
        return (rt_int32_t)(rt_int16_t)value * 10;
    }

    if (id == PARAM_SERVICE_ID_ENV_TEMP_OFFSET_MC ||
        id == PARAM_SERVICE_ID_ENV_HUMI_OFFSET_MPERMIL)
    {
        return (rt_int32_t)(rt_int16_t)value;
    }

    if (id == PARAM_SERVICE_ID_NET_IP_ADDR ||
        id == PARAM_SERVICE_ID_NET_NETMASK ||
        id == PARAM_SERVICE_ID_NET_GATEWAY)
    {
        return (rt_int32_t)value;
    }

    return (rt_int32_t)value;
}

static rt_bool_t modbus_address_is_factory_mac(USHORT address)
{
    return (address >= MODBUS_FACTORY_MAC_REG_START &&
            address < (MODBUS_FACTORY_MAC_REG_START + MODBUS_FACTORY_MAC_REG_COUNT)) ?
           RT_TRUE : RT_FALSE;
}

static eMBErrorCode modbus_handle_factory_mac_registers(UCHAR *buffer,
                                                        USHORT address,
                                                        USHORT nregs,
                                                        eMBRegisterMode mode)
{
    rt_uint8_t mac[6] = {0};
    rt_uint16_t i;

    if (buffer == RT_NULL ||
        modbus_range_in(address,
                        nregs,
                        MODBUS_FACTORY_MAC_REG_START,
                        MODBUS_FACTORY_MAC_REG_COUNT) == RT_FALSE)
    {
        return MB_ENOREG;
    }

    if (mode == MB_REG_READ)
    {
        (void)factory_service_get_mac(mac);
        for (i = 0U; i < nregs; i++)
        {
            rt_uint16_t offset = (rt_uint16_t)(address - MODBUS_FACTORY_MAC_REG_START + i);
            modbus_put_u16(&buffer, mac[offset]);
        }
        return MB_ENOERR;
    }

    if (address != MODBUS_FACTORY_MAC_REG_START ||
        nregs != MODBUS_FACTORY_MAC_REG_COUNT)
    {
        return MB_EINVAL;
    }

    for (i = 0U; i < MODBUS_FACTORY_MAC_REG_COUNT; i++)
    {
        rt_uint16_t value = modbus_get_u16(&buffer);

        if (value > 0xFFU)
        {
            return MB_EINVAL;
        }
        mac[i] = (rt_uint8_t)value;
    }

    if (factory_service_mac_is_valid(mac) == RT_FALSE)
    {
        return MB_EINVAL;
    }

    switch (backend_request_set_factory_mac_async(mac))
    {
    case RT_EOK:
        return MB_ENOERR;
    case -RT_EBUSY:
        return MB_ETIMEDOUT;
    case -RT_EFULL:
    case -RT_ENOMEM:
        return MB_ENORES;
    default:
        return MB_EIO;
    }
}

static rt_uint16_t modbus_u32_hi(rt_uint32_t value)
{
    return (rt_uint16_t)(value >> 16);
}

static rt_uint16_t modbus_u32_lo(rt_uint32_t value)
{
    return (rt_uint16_t)(value & 0xFFFFU);
}

static rt_uint16_t modbus_ipv4_octet(rt_uint32_t ip, rt_uint16_t index)
{
    if (index >= 4U)
    {
        return 0U;
    }

    return (rt_uint16_t)((ip >> ((3U - index) * 8U)) & 0xFFU);
}

static rt_uint16_t modbus_datetime_month_day(const app_rtc_datetime_t *datetime)
{
    if (datetime == RT_NULL)
    {
        return 0U;
    }

    return (rt_uint16_t)(((rt_uint16_t)datetime->month << 8) | datetime->day);
}

static rt_uint16_t modbus_datetime_hour_minute(const app_rtc_datetime_t *datetime)
{
    if (datetime == RT_NULL)
    {
        return 0U;
    }

    return (rt_uint16_t)(((rt_uint16_t)datetime->hour << 8) | datetime->minute);
}

static rt_uint16_t modbus_datetime_second_weekday(const app_rtc_datetime_t *datetime)
{
    if (datetime == RT_NULL)
    {
        return 0U;
    }

    return (rt_uint16_t)(((rt_uint16_t)datetime->second << 8) | datetime->weekday);
}

static rt_uint16_t modbus_active_alarm_mask(const alarm_service_snapshot_t *alarm_snapshot)
{
    rt_uint16_t mask = 0U;
    rt_size_t i;

    if (alarm_snapshot == RT_NULL)
    {
        return 0U;
    }

    for (i = 0U; i < alarm_snapshot->active_count; i++)
    {
        const alarm_service_active_item_t *item = &alarm_snapshot->active_items[i];

        if (item->reason == RECORD_SERVICE_ALARM_REASON_NTC_HIGH ||
            item->reason == RECORD_SERVICE_ALARM_REASON_NTC_LOW)
        {
            mask |= 0x0001U;
        }
        else if (item->source_index >= 1U && item->source_index <= 5U)
        {
            mask |= (rt_uint16_t)(1U << item->source_index);
        }
    }

    return mask;
}

static rt_uint16_t modbus_alarm_pending_flags(const alarm_service_snapshot_t *alarm_snapshot)
{
    rt_uint16_t flags = 0U;

    if (alarm_snapshot == RT_NULL)
    {
        return 0U;
    }

    if (alarm_snapshot->ntc_pending != 0U)
    {
        flags |= 0x0001U;
    }

    flags |= (rt_uint16_t)((alarm_snapshot->ti_pending_mask & 0x1FU) << 1);
    flags |= (rt_uint16_t)((alarm_snapshot->ti_pending_target_active_mask & 0x1FU) << 9);

    return flags;
}

static rt_uint32_t modbus_record_latest_sequence(const record_service_status_t *status)
{
    if (status == RT_NULL || status->count == 0U || status->next_sequence == 0U)
    {
        return 0U;
    }

    return status->next_sequence - 1U;
}

static rt_uint32_t modbus_record_oldest_sequence(void)
{
    record_service_item_t item;

    if (record_service_get_by_index(0U, &item) != RT_EOK)
    {
        return 0U;
    }

    return item.sequence;
}

static void modbus_record_query_finish(rt_err_t result,
                                       const record_service_item_t *item,
                                       rt_size_t actual_count,
                                       rt_uint32_t generation,
                                       rt_bool_t valid)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    modbus_record_query_ctx.result = (rt_int16_t)result;
    modbus_record_query_ctx.result_count = actual_count;
    modbus_record_query_ctx.generation = generation;
    modbus_record_query_ctx.valid = (valid != RT_FALSE) ? 1U : 0U;
    if (item != RT_NULL && valid != RT_FALSE)
    {
        modbus_record_query_ctx.item = *item;
    }
    else
    {
        memset(&modbus_record_query_ctx.item, 0, sizeof(modbus_record_query_ctx.item));
    }
    modbus_record_query_ctx.busy = 0U;
    rt_hw_interrupt_enable(level);
}

static void modbus_alarm_page_finish(rt_err_t result,
                                     rt_uint16_t page,
                                     rt_uint16_t page_size,
                                     const record_service_item_t *items,
                                     rt_size_t actual_count,
                                     rt_uint32_t generation,
                                     rt_bool_t valid)
{
    rt_size_t i;
    rt_base_t level;
    rt_size_t stored_count = (valid != RT_FALSE) ? actual_count : 0U;

    if (stored_count > page_size)
    {
        stored_count = page_size;
    }
    if (stored_count > MODBUS_ALARM_PAGE_MAX_SIZE)
    {
        stored_count = MODBUS_ALARM_PAGE_MAX_SIZE;
    }

    level = rt_hw_interrupt_disable();
    modbus_alarm_page_ctx.busy = 0U;
    modbus_alarm_page_ctx.done = 1U;
    modbus_alarm_page_ctx.result = (rt_int16_t)result;
    modbus_alarm_page_ctx.current_page = page;
    modbus_alarm_page_ctx.page_size = page_size;
    modbus_alarm_page_ctx.item_count = (rt_uint16_t)stored_count;
    modbus_alarm_page_ctx.generation = generation;
    modbus_alarm_page_ctx.has_prev = (page > 0U) ? 1U : 0U;
    modbus_alarm_page_ctx.has_next =
        (valid != RT_FALSE && actual_count > page_size) ? 1U : 0U;

    memset(modbus_alarm_page_ctx.items, 0, sizeof(modbus_alarm_page_ctx.items));
    if (items != RT_NULL && valid != RT_FALSE)
    {
        for (i = 0U; i < stored_count && i < MODBUS_ALARM_PAGE_MAX_SIZE; i++)
        {
            modbus_alarm_page_ctx.items[i] = items[i];
        }
    }
    rt_hw_interrupt_enable(level);
}

static void modbus_record_query_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);

    while (1)
    {
        record_service_status_t status_before;
        record_service_status_t status_after;
        record_service_item_t item;
        rt_size_t actual_count = 0U;
        rt_size_t scanned_count = 0U;
        rt_uint16_t mode;
        rt_uint16_t latest_offset;
        rt_uint32_t query_sequence;
        rt_base_t level;
        rt_err_t result;

        if (rt_sem_take(modbus_record_query_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        level = rt_hw_interrupt_disable();
        mode = modbus_record_query_ctx.mode;
        latest_offset = modbus_record_query_ctx.latest_offset;
        query_sequence = modbus_record_query_ctx.query_sequence;
        rt_hw_interrupt_enable(level);

        memset(&item, 0, sizeof(item));
        memset(&status_before, 0, sizeof(status_before));
        memset(&status_after, 0, sizeof(status_after));
        (void)record_service_get_status(&status_before);

        switch (mode)
        {
        case MODBUS_RECORD_QUERY_MODE_LATEST:
            result = record_service_query_latest_page((rt_size_t)latest_offset,
                                                      1U,
                                                      &item,
                                                      &actual_count);
            break;

        case MODBUS_RECORD_QUERY_MODE_SEQUENCE:
            result = record_service_query_page_by_sequence(query_sequence,
                                                           1U,
                                                           &item,
                                                           &actual_count);
            break;

        case MODBUS_RECORD_QUERY_MODE_ALARM_LATEST:
            result = record_service_query_alarm_latest_page((rt_size_t)latest_offset,
                                                            1U,
                                                            &item,
                                                            &actual_count,
                                                            &scanned_count);
            break;

        case MODBUS_RECORD_QUERY_MODE_IDLE:
        default:
            result = -RT_EINVAL;
            break;
        }

        (void)record_service_get_status(&status_after);
        modbus_record_query_finish(result,
                                   &item,
                                   actual_count,
                                   status_after.generation,
                                   (result == RT_EOK &&
                                    actual_count > 0U &&
                                    status_before.generation == status_after.generation) ?
                                   RT_TRUE : RT_FALSE);
    }
}

static void modbus_alarm_page_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);

    while (1)
    {
        record_service_status_t status_before;
        record_service_status_t status_after;
        record_service_item_t items[MODBUS_ALARM_PAGE_QUERY_MAX_COUNT];
        rt_size_t actual_count = 0U;
        rt_size_t scanned_count = 0U;
        rt_uint16_t command;
        rt_uint16_t current_page;
        rt_uint16_t requested_page;
        rt_uint16_t page_size;
        rt_uint16_t target_page;
        rt_base_t level;
        rt_err_t result;
        rt_err_t report_result;

        if (rt_sem_take(modbus_alarm_page_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        level = rt_hw_interrupt_disable();
        command = modbus_alarm_page_ctx.command;
        current_page = modbus_alarm_page_ctx.current_page;
        requested_page = modbus_alarm_page_ctx.requested_page;
        page_size = modbus_alarm_page_sanitize_size(modbus_alarm_page_ctx.page_size);
        rt_hw_interrupt_enable(level);

        switch (command)
        {
        case MODBUS_ALARM_PAGE_COMMAND_LOAD:
            target_page = requested_page;
            break;
        case MODBUS_ALARM_PAGE_COMMAND_NEXT:
            target_page = (current_page < RT_UINT16_MAX) ?
                          (rt_uint16_t)(current_page + 1U) :
                          current_page;
            break;
        case MODBUS_ALARM_PAGE_COMMAND_PREV:
            target_page = (current_page > 0U) ? (rt_uint16_t)(current_page - 1U) : 0U;
            break;
        case MODBUS_ALARM_PAGE_COMMAND_FIRST:
            target_page = 0U;
            break;
        case MODBUS_ALARM_PAGE_COMMAND_IDLE:
        default:
            target_page = current_page;
            result = -RT_EINVAL;
            modbus_alarm_page_finish(result, target_page, page_size, RT_NULL, 0U, 0U, RT_FALSE);
            continue;
        }

        memset(items, 0, sizeof(items));
        memset(&status_before, 0, sizeof(status_before));
        memset(&status_after, 0, sizeof(status_after));
        (void)record_service_get_status(&status_before);
        result = record_service_query_alarm_latest_page((rt_size_t)target_page * page_size,
                                                        (rt_size_t)page_size + 1U,
                                                        items,
                                                        &actual_count,
                                                        &scanned_count);
        (void)record_service_get_status(&status_after);
        report_result = (result == -RT_EEMPTY) ? RT_EOK : result;
        modbus_alarm_page_finish(report_result,
                                 target_page,
                                 page_size,
                                 items,
                                 actual_count,
                                 status_after.generation,
                                 ((result == RT_EOK || result == -RT_EEMPTY) &&
                                  status_before.generation == status_after.generation) ?
                                 RT_TRUE : RT_FALSE);
    }
}

static rt_err_t modbus_record_query_start_worker(void)
{
    if (modbus_record_query_sem == RT_NULL)
    {
        modbus_record_query_sem = rt_sem_create("mb_recq_sem", 0U, RT_IPC_FLAG_FIFO);
        if (modbus_record_query_sem == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    if (modbus_record_query_thread == RT_NULL)
    {
        modbus_record_query_thread = rt_thread_create("mb_recq",
                                                      modbus_record_query_thread_entry,
                                                      RT_NULL,
                                                      MODBUS_RECORD_QUERY_THREAD_STACK_SIZE,
                                                      MODBUS_RECORD_QUERY_THREAD_PRIORITY,
                                                      MODBUS_RECORD_QUERY_THREAD_TIMESLICE);
        if (modbus_record_query_thread == RT_NULL)
        {
            return -RT_ENOMEM;
        }

        return rt_thread_startup(modbus_record_query_thread);
    }

    return RT_EOK;
}

static rt_err_t modbus_record_query_submit(void)
{
    rt_base_t level;
    rt_err_t result;

    result = modbus_record_query_start_worker();
    if (result != RT_EOK)
    {
        return result;
    }

    if (modbus_record_query_is_busy() != RT_FALSE)
    {
        return -RT_EBUSY;
    }

    level = rt_hw_interrupt_disable();
    modbus_record_query_ctx.busy = 1U;
    modbus_record_query_ctx.valid = 0U;
    modbus_record_query_ctx.result = (rt_int16_t)(-RT_EBUSY);
    modbus_record_query_ctx.result_count = 0U;
    modbus_record_query_ctx.generation = 0U;
    memset(&modbus_record_query_ctx.item, 0, sizeof(modbus_record_query_ctx.item));
    rt_hw_interrupt_enable(level);

    result = rt_sem_release(modbus_record_query_sem);
    if (result != RT_EOK)
    {
        modbus_record_query_finish(result, RT_NULL, 0U, 0U, RT_FALSE);
    }

    return result;
}

static rt_err_t modbus_alarm_page_start_worker(void)
{
    if (modbus_alarm_page_sem == RT_NULL)
    {
        modbus_alarm_page_sem = rt_sem_create("mb_apage_sem", 0U, RT_IPC_FLAG_FIFO);
        if (modbus_alarm_page_sem == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    if (modbus_alarm_page_thread == RT_NULL)
    {
        modbus_alarm_page_thread = rt_thread_create("mb_apage",
                                                    modbus_alarm_page_thread_entry,
                                                    RT_NULL,
                                                    MODBUS_ALARM_PAGE_THREAD_STACK_SIZE,
                                                    MODBUS_ALARM_PAGE_THREAD_PRIORITY,
                                                    MODBUS_ALARM_PAGE_THREAD_TIMESLICE);
        if (modbus_alarm_page_thread == RT_NULL)
        {
            return -RT_ENOMEM;
        }

        return rt_thread_startup(modbus_alarm_page_thread);
    }

    return RT_EOK;
}

static rt_err_t modbus_alarm_page_submit(void)
{
    rt_base_t level;
    rt_err_t result;

    result = modbus_alarm_page_start_worker();
    if (result != RT_EOK)
    {
        return result;
    }

    if (modbus_alarm_page_is_busy() != RT_FALSE)
    {
        return -RT_EBUSY;
    }

    level = rt_hw_interrupt_disable();
    modbus_alarm_page_ctx.busy = 1U;
    modbus_alarm_page_ctx.done = 0U;
    modbus_alarm_page_ctx.result = (rt_int16_t)(-RT_EBUSY);
    modbus_alarm_page_ctx.page_size =
        modbus_alarm_page_sanitize_size(modbus_alarm_page_ctx.page_size);
    modbus_alarm_page_ctx.item_count = 0U;
    modbus_alarm_page_ctx.generation = 0U;
    modbus_alarm_page_ctx.has_prev = 0U;
    modbus_alarm_page_ctx.has_next = 0U;
    memset(modbus_alarm_page_ctx.items, 0, sizeof(modbus_alarm_page_ctx.items));
    rt_hw_interrupt_enable(level);

    result = rt_sem_release(modbus_alarm_page_sem);
    if (result != RT_EOK)
    {
        modbus_alarm_page_finish(result,
                                 modbus_alarm_page_ctx.current_page,
                                 modbus_alarm_page_ctx.page_size,
                                 RT_NULL,
                                 0U,
                                 0U,
                                 RT_FALSE);
    }

    return result;
}

static void modbus_prepare_input_context(modbus_input_context_t *context)
{
    if (context == RT_NULL)
    {
        return;
    }

    memset(context, 0, sizeof(*context));
    data_service_get_snapshot(&context->snapshot);
    if (record_service_get_status(&context->record_status) != RT_EOK)
    {
        context->record_status.count = 0U;
        context->record_status.total_written = 0U;
    }
    if (param_service_get_status(&context->param_status) != RT_EOK)
    {
        context->param_status.version = 0U;
        context->param_status.dirty = 0U;
    }
    (void)alarm_service_get_snapshot(&context->alarm_snapshot);
    app_modbus_tcp_get_status(&context->tcp_status);
    context->latest_record_valid =
        (record_service_get_latest(&context->latest_record) == RT_EOK) ? 1U : 0U;
}

static eMBErrorCode modbus_read_record_item_register(UCHAR **buffer,
                                                     USHORT field_index,
                                                     const record_service_item_t *item,
                                                     rt_uint8_t valid)
{
    record_service_item_t empty_item;

    if (buffer == RT_NULL || field_index >= MODBUS_INPUT_RECORD_QUERY_COUNT)
    {
        return MB_ENOREG;
    }

    memset(&empty_item, 0, sizeof(empty_item));
    if (item == RT_NULL || valid == 0U)
    {
        item = &empty_item;
    }

    switch (field_index)
    {
    case 0:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)valid << 15) |
                                     ((rt_uint16_t)item->type & 0x7FFFU)));
        break;
    case 1:
        modbus_put_u16(buffer, modbus_u32_hi(item->sequence));
        break;
    case 2:
        modbus_put_u16(buffer, modbus_u32_lo(item->sequence));
        break;
    case 3:
        modbus_put_u16(buffer, item->format_version);
        break;
    case 4:
        modbus_put_u16(buffer, modbus_u32_hi(item->alarm_id));
        break;
    case 5:
        modbus_put_u16(buffer, modbus_u32_lo(item->alarm_id));
        break;
    case 6:
        modbus_put_u16(buffer, (rt_uint16_t)item->alarm_reason);
        break;
    case 7:
        modbus_put_u16(buffer, modbus_u32_hi(item->timestamp_ms));
        break;
    case 8:
        modbus_put_u16(buffer, modbus_u32_lo(item->timestamp_ms));
        break;
    case 9:
        modbus_put_u16(buffer, (rt_uint16_t)item->event_action);
        break;
    case 10:
        modbus_put_u16(buffer, (rt_uint16_t)item->source_index);
        break;
    case 11:
        modbus_put_u16(buffer, modbus_u32_hi(item->duration_ms));
        break;
    case 12:
        modbus_put_u16(buffer, modbus_u32_lo(item->duration_ms));
        break;
    case 13:
        modbus_put_u16(buffer, item->time_valid ? item->datetime.year : 0U);
        break;
    case 14:
        modbus_put_u16(buffer, item->time_valid ? modbus_datetime_month_day(&item->datetime) : 0U);
        break;
    case 15:
        modbus_put_u16(buffer, item->time_valid ? modbus_datetime_hour_minute(&item->datetime) : 0U);
        break;
    case 16:
        modbus_put_u16(buffer, item->time_valid ? modbus_datetime_second_weekday(&item->datetime) : 0U);
        break;
    case 17:
    case 18:
    case 19:
    case 20:
        modbus_put_u16(buffer, 0U);
        break;
    case 21:
        modbus_put_u16(buffer, (rt_uint16_t)(rt_int16_t)(item->ntc_temp_milli_c / 10));
        break;
    case 22:
        modbus_put_u16(buffer, (rt_uint16_t)(rt_int16_t)(item->env_temp_milli_c / 10));
        break;
    case 23:
        modbus_put_u16(buffer, (rt_uint16_t)(item->env_humi_milli_percent / 10));
        break;
    case 24:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)item->digital_input_state << 8) |
                                     item->alarm_state));
        break;
    default:
        modbus_put_u16(buffer, 0U);
        break;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_active_alarm_register(UCHAR **buffer,
                                                      USHORT address,
                                                      const modbus_input_context_t *context)
{
    rt_size_t item_index;
    rt_size_t field_index;
    alarm_service_active_item_t empty_item;
    const alarm_service_active_item_t *item;

    if (context == RT_NULL ||
        address < MODBUS_INPUT_ACTIVE_ALARM_BASE ||
        address >= (MODBUS_INPUT_ACTIVE_ALARM_BASE +
                    (MODBUS_INPUT_ACTIVE_ALARM_STRIDE * MODBUS_INPUT_ACTIVE_ALARM_COUNT)))
    {
        return MB_ENOREG;
    }

    item_index = (rt_size_t)((address - MODBUS_INPUT_ACTIVE_ALARM_BASE) /
                             MODBUS_INPUT_ACTIVE_ALARM_STRIDE);
    field_index = (rt_size_t)((address - MODBUS_INPUT_ACTIVE_ALARM_BASE) %
                              MODBUS_INPUT_ACTIVE_ALARM_STRIDE);
    memset(&empty_item, 0, sizeof(empty_item));
    item = (item_index < context->alarm_snapshot.active_count) ?
           &context->alarm_snapshot.active_items[item_index] :
           &empty_item;

    switch (field_index)
    {
    case 0:
        modbus_put_u16(buffer, modbus_u32_hi(item->alarm_id));
        break;
    case 1:
        modbus_put_u16(buffer, modbus_u32_lo(item->alarm_id));
        break;
    case 2:
        modbus_put_u16(buffer, (rt_uint16_t)item->reason);
        break;
    case 3:
        modbus_put_u16(buffer, (rt_uint16_t)item->source_index);
        break;
    case 4:
        modbus_put_u16(buffer, modbus_u32_hi(item->duration_ms));
        break;
    case 5:
        modbus_put_u16(buffer, modbus_u32_lo(item->duration_ms));
        break;
    case 6:
        modbus_put_u16(buffer, item->time_valid ? item->start_datetime.year : 0U);
        break;
    case 7:
        modbus_put_u16(buffer,
                       item->time_valid ? modbus_datetime_month_day(&item->start_datetime) : 0U);
        break;
    case 8:
        modbus_put_u16(buffer,
                       item->time_valid ? modbus_datetime_hour_minute(&item->start_datetime) : 0U);
        break;
    case 9:
        modbus_put_u16(buffer,
                       item->time_valid ? modbus_datetime_second_weekday(&item->start_datetime) : 0U);
        break;
    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_latest_record_register(UCHAR **buffer,
                                                       USHORT address,
                                                       const modbus_input_context_t *context)
{
    const record_service_item_t *item;
    record_service_item_t empty_item;

    if (context == RT_NULL ||
        address < MODBUS_INPUT_LATEST_RECORD_BASE ||
        address >= (MODBUS_INPUT_LATEST_RECORD_BASE + 17U))
    {
        return MB_ENOREG;
    }

    memset(&empty_item, 0, sizeof(empty_item));
    item = (context->latest_record_valid != 0U) ? &context->latest_record : &empty_item;

    switch (address - MODBUS_INPUT_LATEST_RECORD_BASE)
    {
    case 0:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)context->latest_record_valid << 15) |
                                     ((rt_uint16_t)item->type & 0x7FFFU)));
        break;
    case 1:
        modbus_put_u16(buffer, modbus_u32_hi(item->sequence));
        break;
    case 2:
        modbus_put_u16(buffer, modbus_u32_lo(item->sequence));
        break;
    case 3:
        modbus_put_u16(buffer, modbus_u32_hi(item->alarm_id));
        break;
    case 4:
        modbus_put_u16(buffer, modbus_u32_lo(item->alarm_id));
        break;
    case 5:
        modbus_put_u16(buffer, (rt_uint16_t)item->alarm_reason);
        break;
    case 6:
        modbus_put_u16(buffer, modbus_u32_hi(item->duration_ms));
        break;
    case 7:
        modbus_put_u16(buffer, modbus_u32_lo(item->duration_ms));
        break;
    case 8:
        modbus_put_u16(buffer, (rt_uint16_t)item->event_action);
        break;
    case 9:
        modbus_put_u16(buffer, (rt_uint16_t)item->source_index);
        break;
    case 10:
        modbus_put_u16(buffer, item->time_valid ? item->datetime.year : 0U);
        break;
    case 11:
        modbus_put_u16(buffer, item->time_valid ? modbus_datetime_month_day(&item->datetime) : 0U);
        break;
    case 12:
        modbus_put_u16(buffer, item->time_valid ? modbus_datetime_hour_minute(&item->datetime) : 0U);
        break;
    case 13:
        modbus_put_u16(buffer, item->time_valid ? modbus_datetime_second_weekday(&item->datetime) : 0U);
        break;
    case 14:
        modbus_put_u16(buffer, 0U);
        break;
    case 15:
        modbus_put_u16(buffer, 0U);
        break;
    case 16:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)item->digital_input_state << 8) |
                                     item->alarm_state));
        break;
    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_record_query_result_register(UCHAR **buffer, USHORT address)
{
    modbus_record_query_context_t query;
    rt_uint8_t valid;

    if (address < MODBUS_INPUT_RECORD_QUERY_BASE ||
        address >= (MODBUS_INPUT_RECORD_QUERY_BASE + MODBUS_INPUT_RECORD_QUERY_COUNT))
    {
        return MB_ENOREG;
    }

    modbus_record_query_get_snapshot(&query);
    valid = modbus_record_query_snapshot_is_valid(&query);
    return modbus_read_record_item_register(buffer,
                                            (USHORT)(address - MODBUS_INPUT_RECORD_QUERY_BASE),
                                            &query.item,
                                            valid);
}

static eMBErrorCode modbus_read_alarm_page_register(UCHAR **buffer, USHORT address)
{
    modbus_alarm_page_context_t page;
    rt_uint16_t offset;
    rt_uint16_t flags;
    rt_uint8_t current;

    if (buffer == RT_NULL ||
        address < MODBUS_INPUT_ALARM_PAGE_BASE ||
        address >= (MODBUS_INPUT_ALARM_PAGE_BASE + MODBUS_INPUT_ALARM_PAGE_COUNT))
    {
        return MB_ENOREG;
    }

    modbus_alarm_page_get_snapshot(&page);
    current = modbus_alarm_page_snapshot_is_current(&page);
    offset = (rt_uint16_t)(address - MODBUS_INPUT_ALARM_PAGE_BASE);

    if (offset < MODBUS_INPUT_ALARM_PAGE_HEADER_COUNT)
    {
        switch (offset)
        {
        case 0:
            modbus_put_u16(buffer, page.busy);
            break;
        case 1:
            modbus_put_u16(buffer, page.done);
            break;
        case 2:
            modbus_put_u16(buffer, (rt_uint16_t)page.result);
            break;
        case 3:
            modbus_put_u16(buffer, page.current_page);
            break;
        case 4:
            modbus_put_u16(buffer, page.page_size);
            break;
        case 5:
            modbus_put_u16(buffer, current ? page.item_count : 0U);
            break;
        case 6:
            flags = 0U;
            if (current != 0U)
            {
                flags |= (page.has_prev != 0U) ? 0x0001U : 0U;
                flags |= (page.has_next != 0U) ? 0x0002U : 0U;
            }
            modbus_put_u16(buffer, flags);
            break;
        case 7:
            modbus_put_u16(buffer, modbus_u32_hi(page.generation));
            break;
        case 8:
            modbus_put_u16(buffer, modbus_u32_lo(page.generation));
            break;
        case 9:
            modbus_put_u16(buffer, page.command);
            break;
        case 10:
            modbus_put_u16(buffer, page.requested_page);
            break;
        case 11:
            modbus_put_u16(buffer, 0U);
            break;
        default:
            return MB_ENOREG;
        }

        return MB_ENOERR;
    }

    offset = (rt_uint16_t)(offset - MODBUS_INPUT_ALARM_PAGE_HEADER_COUNT);
    return modbus_read_record_item_register(buffer,
                                            (USHORT)(offset % MODBUS_INPUT_ALARM_PAGE_STRIDE),
                                            &page.items[offset / MODBUS_INPUT_ALARM_PAGE_STRIDE],
                                            (current != 0U &&
                                             (offset / MODBUS_INPUT_ALARM_PAGE_STRIDE) <
                                            page.item_count) ? 1U : 0U);
}

static eMBErrorCode modbus_read_network_status_register(UCHAR **buffer,
                                                        USHORT address,
                                                        const modbus_input_context_t *context)
{
    const app_modbus_tcp_status_t *status;
    rt_uint16_t flags;
    rt_uint16_t offset;

    if (buffer == RT_NULL || context == RT_NULL ||
        address < MODBUS_INPUT_NET_STATUS_BASE ||
        address >= (MODBUS_INPUT_NET_STATUS_BASE + MODBUS_INPUT_NET_STATUS_COUNT))
    {
        return MB_ENOREG;
    }

    status = &context->tcp_status;
    offset = (rt_uint16_t)(address - MODBUS_INPUT_NET_STATUS_BASE);

    switch (offset)
    {
    case 0:
        flags = 0U;
        flags |= (status->running != 0U) ? 0x0001U : 0U;
        flags |= (status->dhcp_active != 0U) ? 0x0002U : 0U;
        flags |= (status->apply_pending != 0U) ? 0x0004U : 0U;
        flags |= (status->link_up != 0U) ? 0x0008U : 0U;
        flags |= (status->speed_100m != 0U) ? 0x0010U : 0U;
        flags |= (status->full_duplex != 0U) ? 0x0020U : 0U;
        modbus_put_u16(buffer, flags);
        break;
    case 1:
        modbus_put_u16(buffer, status->socket_status);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        modbus_put_u16(buffer, modbus_ipv4_octet(status->ip, (rt_uint16_t)(offset - 2U)));
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        modbus_put_u16(buffer, modbus_ipv4_octet(status->netmask, (rt_uint16_t)(offset - 6U)));
        break;
    case 10:
    case 11:
    case 12:
    case 13:
        modbus_put_u16(buffer, modbus_ipv4_octet(status->gateway, (rt_uint16_t)(offset - 10U)));
        break;
    case 14:
    case 15:
    case 16:
    case 17:
        modbus_put_u16(buffer, modbus_ipv4_octet(status->dns, (rt_uint16_t)(offset - 14U)));
        break;
    case 18:
        modbus_put_u16(buffer, modbus_u32_hi(status->lease_seconds));
        break;
    case 19:
        modbus_put_u16(buffer, modbus_u32_lo(status->lease_seconds));
        break;
    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_input_register(UCHAR **buffer,
                                               USHORT address,
                                               const modbus_input_context_t *context)
{
    const device_data_snapshot_t *snapshot;
    const record_service_status_t *record_status;
    const param_service_status_t *param_status;

    if (context == RT_NULL)
    {
        return MB_ENOREG;
    }

    snapshot = &context->snapshot;
    record_status = &context->record_status;
    param_status = &context->param_status;

    switch (address)
    {
    case 1:
        modbus_put_u16(buffer, (rt_uint16_t)snapshot->update_count);
        break;
    case 2:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)snapshot->time_valid << 8) |
                                     snapshot->sensor_valid_flags));
        break;
    case 3:
        modbus_put_u16(buffer, (rt_uint16_t)snapshot->surge_peak_value_milli_kv);
        break;
    case 4:
        modbus_put_u16(buffer, (rt_uint16_t)(snapshot->ntc_temp_milli_c / 10));
        break;
    case 5:
        modbus_put_u16(buffer, (rt_uint16_t)(snapshot->env_temp_milli_c / 10));
        break;
    case 6:
        modbus_put_u16(buffer, (rt_uint16_t)(snapshot->env_humi_milli_percent / 10));
        break;
    case 7:
        modbus_put_u16(buffer, (rt_uint16_t)snapshot->alarm_state);
        break;
    case 8:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)param_status->dirty << 8) |
                                     (param_status->version & 0xFFU)));
        break;
    case 9:
        modbus_put_u16(buffer, (rt_uint16_t)record_status->count);
        break;
    case 10:
        modbus_put_u16(buffer, (rt_uint16_t)record_status->total_written);
        break;
    case 11:
        modbus_put_u16(buffer, snapshot->surge_adc_raw);
        break;
    case 12:
        modbus_put_u16(buffer, snapshot->surge_peak_raw);
        break;
    case 13:
        modbus_put_u16(buffer, snapshot->surge_peak_delta_raw);
        break;
    case 14:
        modbus_put_u16(buffer, (rt_uint16_t)snapshot->surge_trigger_count);
        break;
    case 15:
        modbus_put_u16(buffer, (rt_uint16_t)snapshot->surge_status);
        break;
    case 16:
        modbus_put_u16(buffer, snapshot->surge_threshold_delta_raw);
        break;
    case 17:
        modbus_put_u16(buffer, (rt_uint16_t)context->alarm_snapshot.active_count);
        break;
    case 18:
        modbus_put_u16(buffer, modbus_active_alarm_mask(&context->alarm_snapshot));
        break;
    case 19:
        modbus_put_u16(buffer, modbus_alarm_pending_flags(&context->alarm_snapshot));
        break;
    case 20:
        modbus_put_u16(buffer,
                       (rt_uint16_t)(((rt_uint16_t)context->alarm_snapshot.persistent_ready << 0) |
                                     ((rt_uint16_t)context->alarm_snapshot.image_restored << 1)));
        break;
    case 21:
        modbus_put_u16(buffer, (rt_uint16_t)context->alarm_snapshot.total_started);
        break;
    case 22:
        modbus_put_u16(buffer, (rt_uint16_t)context->alarm_snapshot.total_ended);
        break;
    case 23:
        modbus_put_u16(buffer, (rt_uint16_t)context->alarm_snapshot.image_write_count);
        break;
    case 24:
        modbus_put_u16(buffer, (rt_uint16_t)context->alarm_snapshot.image_write_failed);
        break;
    default:
        if (address >= MODBUS_INPUT_ACTIVE_ALARM_BASE &&
            address < (MODBUS_INPUT_ACTIVE_ALARM_BASE +
                       (MODBUS_INPUT_ACTIVE_ALARM_STRIDE * MODBUS_INPUT_ACTIVE_ALARM_COUNT)))
        {
            return modbus_read_active_alarm_register(buffer, address, context);
        }

        if (address >= MODBUS_INPUT_LATEST_RECORD_BASE &&
            address < (MODBUS_INPUT_LATEST_RECORD_BASE + 17U))
        {
            return modbus_read_latest_record_register(buffer, address, context);
        }

        if (address >= MODBUS_INPUT_RECORD_QUERY_BASE &&
            address < (MODBUS_INPUT_RECORD_QUERY_BASE + MODBUS_INPUT_RECORD_QUERY_COUNT))
        {
            return modbus_read_record_query_result_register(buffer, address);
        }

        if (address >= MODBUS_INPUT_ALARM_PAGE_BASE &&
            address < (MODBUS_INPUT_ALARM_PAGE_BASE + MODBUS_INPUT_ALARM_PAGE_COUNT))
        {
            return modbus_read_alarm_page_register(buffer, address);
        }

        if (address >= MODBUS_INPUT_NET_STATUS_BASE &&
            address < (MODBUS_INPUT_NET_STATUS_BASE + MODBUS_INPUT_NET_STATUS_COUNT))
        {
            return modbus_read_network_status_register(buffer, address, context);
        }

        modbus_put_u16(buffer, 0U);
        break;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_record_query_register(UCHAR **buffer, USHORT address)
{
    record_service_status_t status;
    modbus_record_query_context_t query;
    rt_uint32_t sequence;

    if (buffer == RT_NULL ||
        address < MODBUS_RECORD_QUERY_REG_START ||
        address >= (MODBUS_RECORD_QUERY_REG_START + MODBUS_RECORD_QUERY_REG_COUNT))
    {
        return MB_ENOREG;
    }

    memset(&status, 0, sizeof(status));
    (void)record_service_get_status(&status);
    modbus_record_query_get_snapshot(&query);

    switch (address - MODBUS_RECORD_QUERY_REG_START)
    {
    case 0:
        modbus_put_u16(buffer, query.mode);
        break;
    case 1:
        modbus_put_u16(buffer, query.latest_offset);
        break;
    case 2:
        modbus_put_u16(buffer, modbus_u32_hi(query.query_sequence));
        break;
    case 3:
        modbus_put_u16(buffer, modbus_u32_lo(query.query_sequence));
        break;
    case 4:
        modbus_put_u16(buffer, 0U);
        break;
    case 5:
        modbus_put_u16(buffer, query.busy);
        break;
    case 6:
        modbus_put_u16(buffer, (rt_uint16_t)query.result);
        break;
    case 7:
        modbus_put_u16(buffer, (rt_uint16_t)modbus_record_query_snapshot_is_valid(&query));
        break;
    case 8:
        modbus_put_u16(buffer, (rt_uint16_t)status.count);
        break;
    case 9:
        sequence = modbus_record_latest_sequence(&status);
        modbus_put_u16(buffer, modbus_u32_hi(sequence));
        break;
    case 10:
        sequence = modbus_record_latest_sequence(&status);
        modbus_put_u16(buffer, modbus_u32_lo(sequence));
        break;
    case 11:
        sequence = modbus_record_oldest_sequence();
        modbus_put_u16(buffer, modbus_u32_hi(sequence));
        break;
    case 12:
        sequence = modbus_record_oldest_sequence();
        modbus_put_u16(buffer, modbus_u32_lo(sequence));
        break;
    case 13:
        modbus_put_u16(buffer, modbus_u32_hi(query.generation));
        break;
    case 14:
        modbus_put_u16(buffer, modbus_u32_lo(query.generation));
        break;
    case 15:
        modbus_put_u16(buffer,
                       modbus_record_query_snapshot_is_valid(&query) ?
                       (rt_uint16_t)query.result_count : 0U);
        break;
    default:
        modbus_put_u16(buffer, 0U);
        break;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_write_record_query_register(USHORT address, rt_uint16_t value)
{
    if (address < MODBUS_RECORD_QUERY_REG_START ||
        address >= (MODBUS_RECORD_QUERY_REG_START + MODBUS_RECORD_QUERY_REG_COUNT))
    {
        return MB_ENOREG;
    }

    switch (address - MODBUS_RECORD_QUERY_REG_START)
    {
    case 0:
        if (value > MODBUS_RECORD_QUERY_MODE_ALARM_LATEST)
        {
            return MB_EINVAL;
        }
        if (modbus_record_query_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        modbus_record_query_ctx.mode = value;
        modbus_record_query_invalidate();
        break;
    case 1:
        if (modbus_record_query_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        modbus_record_query_ctx.latest_offset = value;
        modbus_record_query_invalidate();
        break;
    case 2:
        if (modbus_record_query_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        modbus_record_query_ctx.query_sequence =
            ((rt_uint32_t)value << 16) | (modbus_record_query_ctx.query_sequence & 0x0000FFFFUL);
        modbus_record_query_invalidate();
        break;
    case 3:
        if (modbus_record_query_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        modbus_record_query_ctx.query_sequence =
            (modbus_record_query_ctx.query_sequence & 0xFFFF0000UL) | value;
        modbus_record_query_invalidate();
        break;
    case 4:
        if (value == 0U)
        {
            return MB_ENOERR;
        }
        if (value != 1U)
        {
            return MB_EINVAL;
        }
        switch (modbus_record_query_submit())
        {
        case RT_EOK:
            return MB_ENOERR;
        case -RT_EBUSY:
            return MB_ETIMEDOUT;
        case -RT_ENOMEM:
        case -RT_EFULL:
            return MB_ENORES;
        default:
            return MB_EIO;
        }
    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_handle_record_query_registers(UCHAR *buffer,
                                                         USHORT address,
                                                         USHORT nregs,
                                                         eMBRegisterMode mode)
{
    USHORT i;

    if (buffer == RT_NULL ||
        modbus_range_in(address,
                        nregs,
                        MODBUS_RECORD_QUERY_REG_START,
                        MODBUS_RECORD_QUERY_REG_COUNT) == RT_FALSE)
    {
        return MB_ENOREG;
    }

    for (i = 0U; i < nregs; i++)
    {
        USHORT current_address = (USHORT)(address + i);

        if (mode == MB_REG_READ)
        {
            eMBErrorCode result = modbus_read_record_query_register(&buffer, current_address);
            if (result != MB_ENOERR)
            {
                return result;
            }
        }
        else
        {
            rt_uint16_t value = modbus_get_u16(&buffer);
            eMBErrorCode result = modbus_write_record_query_register(current_address, value);
            if (result != MB_ENOERR)
            {
                return result;
            }
        }
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_alarm_page_control_register(UCHAR **buffer, USHORT address)
{
    modbus_alarm_page_context_t page;
    rt_uint16_t flags;

    if (buffer == RT_NULL ||
        address < MODBUS_ALARM_PAGE_REG_START ||
        address >= (MODBUS_ALARM_PAGE_REG_START + MODBUS_ALARM_PAGE_REG_COUNT))
    {
        return MB_ENOREG;
    }

    modbus_alarm_page_get_snapshot(&page);
    flags = 0U;
    flags |= (page.has_prev != 0U) ? 0x0001U : 0U;
    flags |= (page.has_next != 0U) ? 0x0002U : 0U;

    switch (address - MODBUS_ALARM_PAGE_REG_START)
    {
    case 0:
        modbus_put_u16(buffer, page.command);
        break;
    case 1:
        modbus_put_u16(buffer, page.requested_page);
        break;
    case 2:
        modbus_put_u16(buffer, page.page_size);
        break;
    case 3:
        modbus_put_u16(buffer, 0U);
        break;
    case 4:
        modbus_put_u16(buffer, page.busy);
        break;
    case 5:
        modbus_put_u16(buffer, page.done);
        break;
    case 6:
        modbus_put_u16(buffer, (rt_uint16_t)page.result);
        break;
    case 7:
        modbus_put_u16(buffer, page.current_page);
        break;
    case 8:
        modbus_put_u16(buffer, page.item_count);
        break;
    case 9:
        modbus_put_u16(buffer, flags);
        break;
    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_write_alarm_page_control_register(USHORT address, rt_uint16_t value)
{
    rt_base_t level;

    if (address < MODBUS_ALARM_PAGE_REG_START ||
        address >= (MODBUS_ALARM_PAGE_REG_START + MODBUS_ALARM_PAGE_REG_COUNT))
    {
        return MB_ENOREG;
    }

    switch (address - MODBUS_ALARM_PAGE_REG_START)
    {
    case 0:
        if (value > MODBUS_ALARM_PAGE_COMMAND_FIRST)
        {
            return MB_EINVAL;
        }
        if (modbus_alarm_page_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        level = rt_hw_interrupt_disable();
        modbus_alarm_page_ctx.command = value;
        rt_hw_interrupt_enable(level);
        modbus_alarm_page_invalidate();
        break;

    case 1:
        if (modbus_alarm_page_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        level = rt_hw_interrupt_disable();
        modbus_alarm_page_ctx.requested_page = value;
        rt_hw_interrupt_enable(level);
        modbus_alarm_page_invalidate();
        break;

    case 2:
        if (modbus_alarm_page_is_busy() != RT_FALSE)
        {
            return MB_ETIMEDOUT;
        }
        level = rt_hw_interrupt_disable();
        modbus_alarm_page_ctx.page_size = modbus_alarm_page_sanitize_size(value);
        rt_hw_interrupt_enable(level);
        modbus_alarm_page_invalidate();
        break;

    case 3:
        if (value == 0U)
        {
            return MB_ENOERR;
        }
        if (value != 1U)
        {
            return MB_EINVAL;
        }
        switch (modbus_alarm_page_submit())
        {
        case RT_EOK:
            return MB_ENOERR;
        case -RT_EBUSY:
            return MB_ETIMEDOUT;
        case -RT_ENOMEM:
        case -RT_EFULL:
            return MB_ENORES;
        default:
            return MB_EIO;
        }

    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_handle_alarm_page_control_registers(UCHAR *buffer,
                                                               USHORT address,
                                                               USHORT nregs,
                                                               eMBRegisterMode mode)
{
    USHORT i;

    if (buffer == RT_NULL ||
        modbus_range_in(address,
                        nregs,
                        MODBUS_ALARM_PAGE_REG_START,
                        MODBUS_ALARM_PAGE_REG_COUNT) == RT_FALSE)
    {
        return MB_ENOREG;
    }

    for (i = 0U; i < nregs; i++)
    {
        USHORT current_address = (USHORT)(address + i);

        if (mode == MB_REG_READ)
        {
            eMBErrorCode result =
                modbus_read_alarm_page_control_register(&buffer, current_address);
            if (result != MB_ENOERR)
            {
                return result;
            }
        }
        else
        {
            rt_uint16_t value = modbus_get_u16(&buffer);
            eMBErrorCode result =
                modbus_write_alarm_page_control_register(current_address, value);
            if (result != MB_ENOERR)
            {
                return result;
            }
        }
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_read_command_register(UCHAR **buffer, USHORT address)
{
    backend_request_status_t status;

    if (buffer == RT_NULL ||
        address < MODBUS_COMMAND_REG_START ||
        address >= (MODBUS_COMMAND_REG_START + MODBUS_COMMAND_REG_COUNT))
    {
        return MB_ENOREG;
    }

    backend_request_get_status(&status);

    switch (address - MODBUS_COMMAND_REG_START)
    {
    case 0:
        modbus_put_u16(buffer, (rt_uint16_t)status.active_type);
        break;
    case 1:
        modbus_put_u16(buffer, (rt_uint16_t)status.sequence);
        break;
    case 2:
        modbus_put_u16(buffer, (rt_uint16_t)status.busy);
        break;
    case 3:
        modbus_put_u16(buffer, (rt_uint16_t)status.last_result);
        break;
    case 4:
        modbus_put_u16(buffer, modbus_u32_hi(status.last_done_ms));
        break;
    case 5:
        modbus_put_u16(buffer, modbus_u32_lo(status.last_done_ms));
        break;
    case 6:
        modbus_put_u16(buffer, modbus_command_unlock_active() ? 1U : 0U);
        break;
    case 7:
        modbus_put_u16(buffer, 0U);
        break;
    default:
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

static eMBErrorCode modbus_write_command_register(USHORT address, rt_uint16_t value)
{
    if (address < MODBUS_COMMAND_REG_START ||
        address >= (MODBUS_COMMAND_REG_START + MODBUS_COMMAND_REG_COUNT))
    {
        return MB_ENOREG;
    }

    switch (address - MODBUS_COMMAND_REG_START)
    {
    case 0:
    {
        rt_err_t result = modbus_submit_command(value);

        if (result == RT_EOK)
        {
            return MB_ENOERR;
        }
        if (result == -RT_EBUSY)
        {
            return MB_ETIMEDOUT;
        }
        if (result == -RT_EFULL || result == -RT_ENOMEM)
        {
            return MB_ENORES;
        }
        return MB_EIO;
    }

    case 1:
        if (value == MODBUS_COMMAND_UNLOCK_KEY)
        {
            modbus_command_unlock_key = value;
            modbus_command_unlock_ms = rt_tick_get_millisecond();
            return MB_ENOERR;
        }
        modbus_command_unlock_clear();
        return MB_EINVAL;

    default:
        return MB_ENOREG;
    }
}

static eMBErrorCode modbus_handle_command_registers(UCHAR *buffer,
                                                    USHORT address,
                                                    USHORT nregs,
                                                    eMBRegisterMode mode)
{
    USHORT i;

    if (buffer == RT_NULL ||
        modbus_range_in(address, nregs, MODBUS_COMMAND_REG_START, MODBUS_COMMAND_REG_COUNT) == RT_FALSE)
    {
        return MB_ENOREG;
    }

    for (i = 0U; i < nregs; i++)
    {
        USHORT current_address = (USHORT)(address + i);

        if (mode == MB_REG_READ)
        {
            eMBErrorCode result = modbus_read_command_register(&buffer, current_address);
            if (result != MB_ENOERR)
            {
                return result;
            }
        }
        else
        {
            rt_uint16_t value = modbus_get_u16(&buffer);
            eMBErrorCode result = modbus_write_command_register(current_address, value);
            if (result != MB_ENOERR)
            {
                return result;
            }
        }
    }

    return MB_ENOERR;
}

void modbus_map_get_stats(modbus_map_stats_t *stats)
{
    if (stats == RT_NULL)
    {
        return;
    }

    stats->holding_reads = modbus_holding_reads;
    stats->holding_writes = modbus_holding_writes;
    stats->input_reads = modbus_input_reads;
}

eMBErrorCode eMBRegHoldingCB(UCHAR *buffer, USHORT address, USHORT nregs, eMBRegisterMode mode)
{
    USHORT i;
    rt_size_t write_count = 0U;
    rt_int32_t write_values[PARAM_SERVICE_ID_COUNT];
    param_service_id_t write_ids[PARAM_SERVICE_ID_COUNT];

    if (buffer == RT_NULL)
    {
        return MB_ENOREG;
    }

    if (modbus_range_in(address, nregs, MODBUS_COMMAND_REG_START, MODBUS_COMMAND_REG_COUNT) != RT_FALSE)
    {
        eMBErrorCode result = modbus_handle_command_registers(buffer, address, nregs, mode);
        if (result == MB_ENOERR)
        {
            if (mode == MB_REG_READ)
            {
                modbus_holding_reads++;
            }
            else
            {
                modbus_holding_writes++;
            }
        }
        return result;
    }

    if (modbus_range_in(address, nregs, MODBUS_RECORD_QUERY_REG_START, MODBUS_RECORD_QUERY_REG_COUNT) != RT_FALSE)
    {
        eMBErrorCode result = modbus_handle_record_query_registers(buffer, address, nregs, mode);
        if (result == MB_ENOERR)
        {
            if (mode == MB_REG_READ)
            {
                modbus_holding_reads++;
            }
            else
            {
                modbus_holding_writes++;
            }
        }
        return result;
    }

    if (modbus_range_in(address, nregs, MODBUS_ALARM_PAGE_REG_START, MODBUS_ALARM_PAGE_REG_COUNT) != RT_FALSE)
    {
        eMBErrorCode result = modbus_handle_alarm_page_control_registers(buffer, address, nregs, mode);
        if (result == MB_ENOERR)
        {
            if (mode == MB_REG_READ)
            {
                modbus_holding_reads++;
            }
            else
            {
                modbus_holding_writes++;
            }
        }
        return result;
    }

    if (modbus_range_in(address, nregs, MODBUS_FACTORY_MAC_REG_START, MODBUS_FACTORY_MAC_REG_COUNT) != RT_FALSE)
    {
        eMBErrorCode result = modbus_handle_factory_mac_registers(buffer, address, nregs, mode);
        if (result == MB_ENOERR)
        {
            if (mode == MB_REG_READ)
            {
                modbus_holding_reads++;
            }
            else
            {
                modbus_holding_writes++;
            }
        }
        return result;
    }

    if (modbus_range_in(address, nregs, MODBUS_PARAM_REG_START, MODBUS_PARAM_REG_COUNT) == RT_FALSE)
    {
        return MB_ENOREG;
    }

    if (modbus_address_is_factory_mac(address) != RT_FALSE ||
        modbus_address_is_factory_mac((USHORT)(address + nregs - 1U)) != RT_FALSE)
    {
        return MB_ENOREG;
    }

    if (nregs > (USHORT)PARAM_SERVICE_ID_COUNT)
    {
        return MB_ENOREG;
    }

    for (i = 0U; i < nregs; i++)
    {
        param_service_id_t id;
        rt_uint16_t holding_offset = 0U;

        if (param_service_find_holding((rt_uint16_t)(address + i), &id, &holding_offset) != RT_EOK)
        {
            return MB_ENOREG;
        }

        if (mode == MB_REG_READ)
        {
            rt_int32_t value = 0;

            if (param_service_get_i32(id, &value) != RT_EOK)
            {
                return MB_EIO;
            }
            modbus_put_u16(&buffer, modbus_param_to_reg(id, value, holding_offset));
        }
        else
        {
            rt_uint16_t raw = modbus_get_u16(&buffer);
            rt_int32_t value;

            if (id == PARAM_SERVICE_ID_NET_IP_ADDR ||
                id == PARAM_SERVICE_ID_NET_NETMASK ||
                id == PARAM_SERVICE_ID_NET_GATEWAY)
            {
                rt_uint16_t j;
                rt_uint32_t ip_value = 0U;

                if (holding_offset != 0U || (i + 4U) > nregs || raw > 255U)
                {
                    return MB_EINVAL;
                }

                ip_value = ((rt_uint32_t)raw & 0xFFU) << 24;
                for (j = 1U; j < 4U; j++)
                {
                    param_service_id_t next_id;
                    rt_uint16_t next_offset = 0U;
                    rt_uint16_t octet = modbus_get_u16(&buffer);

                    if (param_service_find_holding((rt_uint16_t)(address + i + j),
                                                   &next_id,
                                                   &next_offset) != RT_EOK ||
                        next_id != id ||
                        next_offset != j ||
                        octet > 255U)
                    {
                        return MB_EINVAL;
                    }
                    ip_value |= ((rt_uint32_t)octet & 0xFFU) << ((3U - j) * 8U);
                }

                value = (rt_int32_t)ip_value;
                write_ids[write_count] = id;
                write_values[write_count] = value;
                write_count++;
                i = (USHORT)(i + 3U);
                continue;
            }

            value = modbus_reg_to_param(id, raw);
            write_ids[write_count] = id;
            write_values[write_count] = value;
            if (param_service_validate_i32(id, value) != RT_EOK)
            {
                return MB_EINVAL;
            }
            write_count++;
        }
    }

    if (mode == MB_REG_READ)
    {
        modbus_holding_reads++;
    }
    else
    {
        if (backend_request_set_param_list_async(write_ids, write_values, write_count) != RT_EOK)
        {
            return MB_ENORES;
        }
        modbus_holding_writes++;
    }

    return MB_ENOERR;
}

eMBErrorCode eMBRegInputCB(UCHAR *buffer, USHORT address, USHORT nregs)
{
    USHORT i;
    modbus_input_context_t context;

    if (buffer == RT_NULL ||
        address < MODBUS_INPUT_REG_START ||
        ((address + nregs - 1U) >= (MODBUS_INPUT_REG_START + MODBUS_INPUT_REG_COUNT)))
    {
        return MB_ENOREG;
    }

    modbus_prepare_input_context(&context);
    for (i = 0U; i < nregs; i++)
    {
        eMBErrorCode result = modbus_read_input_register(&buffer, (USHORT)(address + i), &context);
        if (result != MB_ENOERR)
        {
            return result;
        }
    }

    modbus_input_reads++;
    return MB_ENOERR;
}

eMBErrorCode eMBRegCoilsCB(UCHAR *buffer, USHORT address, USHORT ncoils, eMBRegisterMode mode)
{
    RT_UNUSED(buffer);
    RT_UNUSED(address);
    RT_UNUSED(ncoils);
    RT_UNUSED(mode);
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB(UCHAR *buffer, USHORT address, USHORT ndiscrete)
{
    RT_UNUSED(buffer);
    RT_UNUSED(address);
    RT_UNUSED(ndiscrete);
    return MB_ENOREG;
}
