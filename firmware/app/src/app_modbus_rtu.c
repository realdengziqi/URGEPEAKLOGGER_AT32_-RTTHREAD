#include "app_modbus_rtu.h"

#include <string.h>

#include "app_modbus_map.h"
#include "app_param_service.h"
#include "finsh.h"
#include "mb.h"
#include "mbport.h"

#define MODBUS_RTU_THREAD_STACK_SIZE      1536U
#define MODBUS_RTU_THREAD_PRIORITY        16U
#define MODBUS_RTU_THREAD_TIMESLICE       10U
#define MODBUS_RTU_APPLY_DELAY_MS         100U
#define MODBUS_RTU_APPLY_TIMEOUT_MS       3000U
#define MODBUS_RTU_EVENT_APPLY            0x01U

void freemodbus_port_set_stop_bits(UCHAR stop_bits);

static rt_thread_t modbus_rtu_thread;
static struct rt_event modbus_rtu_event;
static rt_sem_t modbus_apply_done;
static volatile rt_uint32_t modbus_poll_count;
static volatile rt_uint8_t modbus_running;
static volatile rt_uint8_t modbus_apply_pending;
static volatile rt_err_t modbus_apply_result;
static rt_uint32_t modbus_running_addr;
static rt_uint32_t modbus_running_baudrate;
static rt_uint32_t modbus_running_data_format;

static const char *modbus_data_format_name(rt_uint32_t data_format)
{
    switch (data_format)
    {
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8E1:
        return "8E1";
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8O1:
        return "8O1";
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8N2:
        return "8N2";
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8E2:
        return "8E2";
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8O2:
        return "8O2";
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1:
    default:
        return "8N1";
    }
}

static eMBParity modbus_data_format_parity(rt_uint32_t data_format)
{
    switch (data_format)
    {
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8E1:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8E2:
        return MB_PAR_EVEN;
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8O1:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8O2:
        return MB_PAR_ODD;
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8N2:
    default:
        return MB_PAR_NONE;
    }
}

static UCHAR modbus_data_format_stop_bits(rt_uint32_t data_format)
{
    switch (data_format)
    {
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8N2:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8E2:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8O2:
        return 2U;
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8E1:
    case PARAM_SERVICE_MODBUS_DATA_FORMAT_8O1:
    default:
        return 1U;
    }
}

static eMBErrorCode modbus_start_from_params(void)
{
    rt_uint32_t addr = 1U;
    rt_uint32_t baudrate = 9600U;
    rt_uint32_t data_format = PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1;
    eMBParity parity;
    eMBErrorCode result;

    (void)param_service_get(PARAM_SERVICE_ID_MODBUS_ADDR, &addr);
    (void)param_service_get(PARAM_SERVICE_ID_MODBUS_BAUDRATE, &baudrate);
    (void)param_service_get(PARAM_SERVICE_ID_MODBUS_DATA_FORMAT, &data_format);
    if (data_format >= PARAM_SERVICE_MODBUS_DATA_FORMAT_COUNT)
    {
        data_format = PARAM_SERVICE_MODBUS_DATA_FORMAT_8N1;
    }

    parity = modbus_data_format_parity(data_format);
    freemodbus_port_set_stop_bits(modbus_data_format_stop_bits(data_format));
    result = eMBInit(MB_RTU, (UCHAR)addr, 3, baudrate, parity);
    if (result != MB_ENOERR)
    {
        rt_kprintf("[ERROR] modbus_rtu: init failed %d\r\n", result);
        return result;
    }

    result = eMBEnable();
    if (result != MB_ENOERR)
    {
        rt_kprintf("[ERROR] modbus_rtu: enable failed %d\r\n", result);
        return result;
    }

    modbus_running_addr = addr;
    modbus_running_baudrate = baudrate;
    modbus_running_data_format = data_format;
    modbus_running = 1U;
    rt_kprintf("[INFO] modbus_rtu: slave started addr=%u baud=%u format=%s uart=USART3 pc10/pc11 de=pa15\r\n",
               addr,
               baudrate,
               modbus_data_format_name(data_format));

    return MB_ENOERR;
}

static rt_err_t modbus_apply_pending_params(void)
{
    eMBErrorCode result;

    rt_thread_mdelay(MODBUS_RTU_APPLY_DELAY_MS);
    modbus_running = 0U;
    (void)eMBDisable();

    result = modbus_start_from_params();
    if (result != MB_ENOERR)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void modbus_finish_apply(rt_err_t result)
{
    modbus_apply_result = result;
    modbus_apply_pending = 0U;
    if (modbus_apply_done != RT_NULL)
    {
        rt_sem_release(modbus_apply_done);
    }
}

static void modbus_thread_entry(void *parameter)
{
    eMBErrorCode result;

    RT_UNUSED(parameter);

    result = modbus_start_from_params();
    if (result != MB_ENOERR)
    {
        modbus_finish_apply(-RT_ERROR);
        return;
    }

    while (1)
    {
        rt_uint32_t event_set;

        if (rt_event_recv(&modbus_rtu_event,
                          MODBUS_RTU_EVENT_APPLY,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          0,
                          &event_set) == RT_EOK)
        {
            modbus_finish_apply(modbus_apply_pending_params());
            continue;
        }

        (void)eMBPoll();
        modbus_poll_count++;
    }
}

rt_err_t app_modbus_rtu_init(void)
{
    if (modbus_rtu_thread != RT_NULL)
    {
        return RT_EOK;
    }

    rt_event_init(&modbus_rtu_event, "mb_ctl", RT_IPC_FLAG_PRIO);
    if (modbus_apply_done == RT_NULL)
    {
        modbus_apply_done = rt_sem_create("mb_apply", 0U, RT_IPC_FLAG_PRIO);
        if (modbus_apply_done == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    modbus_rtu_thread = rt_thread_create("mb_rtu",
                                         modbus_thread_entry,
                                         RT_NULL,
                                         MODBUS_RTU_THREAD_STACK_SIZE,
                                         MODBUS_RTU_THREAD_PRIORITY,
                                         MODBUS_RTU_THREAD_TIMESLICE);
    if (modbus_rtu_thread == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    return rt_thread_startup(modbus_rtu_thread);
}

rt_err_t app_modbus_rtu_request_apply(void)
{
    rt_err_t result;

    if (modbus_rtu_thread == RT_NULL || modbus_apply_done == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (modbus_apply_pending != 0U)
    {
        return -RT_EBUSY;
    }

    modbus_apply_pending = 1U;
    modbus_apply_result = -RT_EBUSY;
    if (rt_event_send(&modbus_rtu_event, MODBUS_RTU_EVENT_APPLY) != RT_EOK)
    {
        modbus_apply_pending = 0U;
        return -RT_ERROR;
    }
    (void)xMBPortEventPost(EV_READY);

    result = rt_sem_take(modbus_apply_done, rt_tick_from_millisecond(MODBUS_RTU_APPLY_TIMEOUT_MS));
    if (result != RT_EOK)
    {
        modbus_apply_pending = 0U;
        return -RT_ETIMEOUT;
    }

    return modbus_apply_result;
}

static int app_shell_modbus(int argc, char **argv)
{
    modbus_map_stats_t stats;

    if (argc == 2 && strcmp(argv[1], "apply") == 0)
    {
        rt_err_t result = app_modbus_rtu_request_apply();

        rt_kprintf("[INFO] modbus: apply result=%d running=%u addr=%u baud=%u format=%s\r\n",
                   result,
                   modbus_running,
                   modbus_running_addr,
                   modbus_running_baudrate,
                   modbus_data_format_name(modbus_running_data_format));
        return 0;
    }

    if (argc != 1 && !(argc == 2 && strcmp(argv[1], "status") == 0))
    {
        rt_kprintf("[WARN] modbus: usage modbus [status] | apply\r\n");
        return 0;
    }

    modbus_map_get_stats(&stats);
    rt_kprintf("[INFO] modbus: running=%u apply_pending=%u poll=%u holding_rd=%u holding_wr=%u input_rd=%u\r\n",
               modbus_running,
               modbus_apply_pending,
               modbus_poll_count,
               stats.holding_reads,
               stats.holding_writes,
               stats.input_reads);
    rt_kprintf("[INFO] modbus: active addr=%u baud=%u format=%s\r\n",
               modbus_running_addr,
               modbus_running_baudrate,
               modbus_data_format_name(modbus_running_data_format));
    rt_kprintf("[INFO] modbus: holding[1..%u]=runtime params holding[100..107]=async commands\r\n",
               (rt_uint32_t)param_service_count());
    rt_kprintf("[INFO] modbus: holding[120..149]=record query/page input[1..239]=snapshot/records\r\n");
    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_modbus, modbus, show Modbus RTU slave status);
