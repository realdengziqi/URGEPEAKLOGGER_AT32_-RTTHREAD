#include "app_backend_request.h"

#include <string.h>

#include "app_factory_service.h"
#include "app_modbus_rtu.h"
#include "app_modbus_tcp.h"
#include "app_record_service.h"

#define BACKEND_REQUEST_THREAD_STACK_SIZE    1536U
#define BACKEND_REQUEST_THREAD_PRIORITY      17U
#define BACKEND_REQUEST_THREAD_TIMESLICE     10U
#define BACKEND_REQUEST_QUEUE_DEPTH          8U

typedef enum
{
    BACKEND_REQUEST_SET_PARAM = 0,
    BACKEND_REQUEST_SET_PARAMS,
    BACKEND_REQUEST_SAVE_PARAMS,
    BACKEND_REQUEST_LOAD_PARAMS,
    BACKEND_REQUEST_RESET_PARAMS,
    BACKEND_REQUEST_CLEAR_RECORDS,
    BACKEND_REQUEST_APPLY_MODBUS_RTU,
    BACKEND_REQUEST_APPLY_NETWORK,
    BACKEND_REQUEST_SET_FACTORY_MAC
} backend_request_type_t;

typedef struct
{
    backend_request_type_t type;
    union
    {
        struct
        {
            param_service_id_t id;
            rt_int32_t value;
        } set_param;
        struct
        {
            param_service_id_t first_id;
            rt_size_t count;
            param_service_id_t ids[PARAM_SERVICE_ID_COUNT];
            rt_int32_t values[PARAM_SERVICE_ID_COUNT];
        } set_params;
        struct
        {
            rt_uint8_t mac[6];
        } set_factory_mac;
    } payload;
    rt_sem_t done;
    rt_err_t *result;
    rt_uint32_t sequence;
    rt_uint8_t public_type;
} backend_request_t;

static rt_thread_t backend_request_thread = RT_NULL;
static rt_mq_t backend_request_queue = RT_NULL;
static backend_request_status_t backend_request_status;
static rt_uint32_t backend_request_next_sequence;

static rt_uint8_t backend_request_get_public_type(backend_request_type_t type)
{
    switch (type)
    {
    case BACKEND_REQUEST_SAVE_PARAMS:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_SAVE_PARAMS;
    case BACKEND_REQUEST_LOAD_PARAMS:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_LOAD_PARAMS;
    case BACKEND_REQUEST_RESET_PARAMS:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_RESET_PARAMS;
    case BACKEND_REQUEST_CLEAR_RECORDS:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_CLEAR_RECORDS;
    case BACKEND_REQUEST_APPLY_MODBUS_RTU:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_APPLY_MODBUS_RTU;
    case BACKEND_REQUEST_APPLY_NETWORK:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_APPLY_NETWORK;
    case BACKEND_REQUEST_SET_FACTORY_MAC:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_SET_FACTORY_MAC;
    default:
        return (rt_uint8_t)BACKEND_REQUEST_PUBLIC_NONE;
    }
}

static void backend_request_mark_async_started(backend_request_t *request)
{
    rt_base_t level;

    if (request == RT_NULL || request->public_type == (rt_uint8_t)BACKEND_REQUEST_PUBLIC_NONE)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    request->sequence = ++backend_request_next_sequence;
    backend_request_status.sequence = request->sequence;
    backend_request_status.busy = 1U;
    backend_request_status.active_type = request->public_type;
    backend_request_status.last_result = 0;
    backend_request_status.last_done_ms = 0U;
    rt_hw_interrupt_enable(level);
}

static rt_bool_t backend_request_async_busy(void)
{
    rt_base_t level;
    rt_bool_t busy;

    level = rt_hw_interrupt_disable();
    busy = (backend_request_status.busy != 0U) ? RT_TRUE : RT_FALSE;
    rt_hw_interrupt_enable(level);

    return busy;
}

static void backend_request_mark_async_done(const backend_request_t *request, rt_err_t result)
{
    rt_base_t level;

    if (request == RT_NULL || request->public_type == (rt_uint8_t)BACKEND_REQUEST_PUBLIC_NONE)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    if (backend_request_status.sequence == request->sequence)
    {
        backend_request_status.busy = 0U;
        backend_request_status.active_type = request->public_type;
        backend_request_status.last_result = (rt_int16_t)result;
        backend_request_status.last_done_ms = rt_tick_get_millisecond();
    }
    rt_hw_interrupt_enable(level);
}

static rt_err_t backend_request_validate_param_range(param_service_id_t first_id, rt_size_t count)
{
    if (count == 0U ||
        count > (rt_size_t)PARAM_SERVICE_ID_COUNT)
    {
        return -RT_EINVAL;
    }

    if (first_id == PARAM_SERVICE_ID_COUNT)
    {
        return RT_EOK;
    }

    if ((rt_uint32_t)first_id >= (rt_uint32_t)PARAM_SERVICE_ID_COUNT ||
        ((rt_uint32_t)first_id + count) > (rt_uint32_t)PARAM_SERVICE_ID_COUNT)
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static rt_err_t backend_request_execute_set_params(const backend_request_t *request)
{
    rt_size_t i;
    rt_bool_t use_id_list;

    if (request == RT_NULL ||
        backend_request_validate_param_range(request->payload.set_params.first_id,
                                             request->payload.set_params.count) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    use_id_list = (request->payload.set_params.first_id == PARAM_SERVICE_ID_COUNT) ? RT_TRUE : RT_FALSE;

    for (i = 0U; i < request->payload.set_params.count; i++)
    {
        param_service_id_t id = use_id_list ?
            request->payload.set_params.ids[i] :
            (param_service_id_t)((rt_uint32_t)request->payload.set_params.first_id + i);
        rt_int32_t value = request->payload.set_params.values[i];

        if (param_service_validate_i32(id, value) != RT_EOK)
        {
            return -RT_EINVAL;
        }
    }

    for (i = 0U; i < request->payload.set_params.count; i++)
    {
        param_service_id_t id = use_id_list ?
            request->payload.set_params.ids[i] :
            (param_service_id_t)((rt_uint32_t)request->payload.set_params.first_id + i);
        rt_err_t result = param_service_set_i32(id, request->payload.set_params.values[i]);

        if (result != RT_EOK)
        {
            return result;
        }
    }

    return RT_EOK;
}

static rt_err_t backend_request_execute(const backend_request_t *request)
{
    switch (request->type)
    {
    case BACKEND_REQUEST_SET_PARAM:
        return param_service_set_i32(request->payload.set_param.id,
                                     request->payload.set_param.value);

    case BACKEND_REQUEST_SET_PARAMS:
        return backend_request_execute_set_params(request);

    case BACKEND_REQUEST_SAVE_PARAMS:
        return param_service_save();

    case BACKEND_REQUEST_LOAD_PARAMS:
        return param_service_load();

    case BACKEND_REQUEST_RESET_PARAMS:
        param_service_reset_defaults();
        return RT_EOK;

    case BACKEND_REQUEST_CLEAR_RECORDS:
        record_service_clear();
        return RT_EOK;

    case BACKEND_REQUEST_APPLY_MODBUS_RTU:
        return app_modbus_rtu_request_apply();

    case BACKEND_REQUEST_APPLY_NETWORK:
        return app_modbus_tcp_request_apply();

    case BACKEND_REQUEST_SET_FACTORY_MAC:
        return factory_service_set_mac(request->payload.set_factory_mac.mac);

    default:
        return -RT_EINVAL;
    }
}

static void backend_request_thread_entry(void *parameter)
{
    backend_request_t request;

    RT_UNUSED(parameter);

    while (1)
    {
        if (rt_mq_recv(backend_request_queue,
                       &request,
                       sizeof(request),
                       RT_WAITING_FOREVER) <= 0)
        {
            continue;
        }

        rt_err_t execute_result;

        if (request.result != RT_NULL)
        {
            execute_result = backend_request_execute(&request);
            *request.result = execute_result;
        }
        else
        {
            execute_result = backend_request_execute(&request);
        }

        backend_request_mark_async_done(&request, execute_result);

        if (request.done != RT_NULL)
        {
            rt_sem_release(request.done);
        }
    }
}

static rt_err_t backend_request_submit_sync(backend_request_t *request)
{
    struct rt_semaphore done;
    rt_err_t execute_result = -RT_ERROR;
    rt_err_t send_result;

    if (request == RT_NULL || backend_request_queue == RT_NULL)
    {
        return -RT_EINVAL;
    }

    send_result = rt_sem_init(&done, "be_done", 0U, RT_IPC_FLAG_FIFO);
    if (send_result != RT_EOK)
    {
        return send_result;
    }

    request->done = &done;
    request->result = &execute_result;
    send_result = rt_mq_send(backend_request_queue, request, sizeof(*request));
    if (send_result != RT_EOK)
    {
        rt_sem_detach(&done);
        return send_result;
    }

    (void)rt_sem_take(&done, RT_WAITING_FOREVER);
    rt_sem_detach(&done);

    return execute_result;
}

static rt_err_t backend_request_submit_async(backend_request_t *request)
{
    if (request == RT_NULL || backend_request_queue == RT_NULL)
    {
        return -RT_EINVAL;
    }

    request->done = RT_NULL;
    request->result = RT_NULL;

    return rt_mq_send(backend_request_queue, request, sizeof(*request));
}

static rt_err_t backend_request_submit_async_tracked(backend_request_t *request)
{
    rt_err_t result;

    if (request == RT_NULL || backend_request_queue == RT_NULL)
    {
        return -RT_EINVAL;
    }

    request->done = RT_NULL;
    request->result = RT_NULL;
    request->public_type = backend_request_get_public_type(request->type);
    if (request->public_type != (rt_uint8_t)BACKEND_REQUEST_PUBLIC_NONE &&
        backend_request_async_busy() != RT_FALSE)
    {
        return -RT_EBUSY;
    }
    backend_request_mark_async_started(request);

    result = rt_mq_send(backend_request_queue, request, sizeof(*request));
    if (result != RT_EOK)
    {
        backend_request_mark_async_done(request, result);
    }

    return result;
}

static rt_err_t backend_request_fill_set_param(backend_request_t *request,
                                               param_service_id_t id,
                                               rt_int32_t value)
{
    if (request == RT_NULL || param_service_validate_i32(id, value) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    memset(request, 0, sizeof(*request));
    request->type = BACKEND_REQUEST_SET_PARAM;
    request->payload.set_param.id = id;
    request->payload.set_param.value = value;

    return RT_EOK;
}

static rt_err_t backend_request_fill_set_params(backend_request_t *request,
                                                param_service_id_t first_id,
                                                const rt_int32_t *values,
                                                rt_size_t count)
{
    rt_size_t i;

    if (request == RT_NULL || values == RT_NULL ||
        backend_request_validate_param_range(first_id, count) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < count; i++)
    {
        param_service_id_t id = (param_service_id_t)((rt_uint32_t)first_id + i);

        if (param_service_validate_i32(id, values[i]) != RT_EOK)
        {
            return -RT_EINVAL;
        }
    }

    memset(request, 0, sizeof(*request));
    request->type = BACKEND_REQUEST_SET_PARAMS;
    request->payload.set_params.first_id = first_id;
    request->payload.set_params.count = count;
    memcpy(request->payload.set_params.values, values, count * sizeof(values[0]));

    return RT_EOK;
}

static rt_err_t backend_request_fill_set_param_list(backend_request_t *request,
                                                    const param_service_id_t *ids,
                                                    const rt_int32_t *values,
                                                    rt_size_t count)
{
    rt_size_t i;

    if (request == RT_NULL || ids == RT_NULL || values == RT_NULL ||
        count == 0U || count > (rt_size_t)PARAM_SERVICE_ID_COUNT)
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < count; i++)
    {
        if (param_service_validate_i32(ids[i], values[i]) != RT_EOK)
        {
            return -RT_EINVAL;
        }
    }

    memset(request, 0, sizeof(*request));
    request->type = BACKEND_REQUEST_SET_PARAMS;
    request->payload.set_params.first_id = PARAM_SERVICE_ID_COUNT;
    request->payload.set_params.count = count;
    memcpy(request->payload.set_params.ids, ids, count * sizeof(ids[0]));
    memcpy(request->payload.set_params.values, values, count * sizeof(values[0]));

    return RT_EOK;
}

static rt_err_t backend_request_fill_set_factory_mac(backend_request_t *request,
                                                     const rt_uint8_t mac[6])
{
    if (request == RT_NULL || factory_service_mac_is_valid(mac) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    memset(request, 0, sizeof(*request));
    request->type = BACKEND_REQUEST_SET_FACTORY_MAC;
    memcpy(request->payload.set_factory_mac.mac, mac, sizeof(request->payload.set_factory_mac.mac));

    return RT_EOK;
}

rt_err_t backend_request_init(void)
{
    if (backend_request_thread != RT_NULL)
    {
        return RT_EOK;
    }

    if (backend_request_queue == RT_NULL)
    {
        backend_request_queue = rt_mq_create("backend_req",
                                             sizeof(backend_request_t),
                                             BACKEND_REQUEST_QUEUE_DEPTH,
                                             RT_IPC_FLAG_PRIO);
        if (backend_request_queue == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    backend_request_thread = rt_thread_create("backend_req",
                                             backend_request_thread_entry,
                                             RT_NULL,
                                             BACKEND_REQUEST_THREAD_STACK_SIZE,
                                             BACKEND_REQUEST_THREAD_PRIORITY,
                                             BACKEND_REQUEST_THREAD_TIMESLICE);
    if (backend_request_thread == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    return rt_thread_startup(backend_request_thread);
}

rt_err_t backend_request_set_param(param_service_id_t id, rt_int32_t value)
{
    backend_request_t request;

    if (backend_request_fill_set_param(&request, id, value) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_set_param_async(param_service_id_t id, rt_int32_t value)
{
    backend_request_t request;

    if (backend_request_fill_set_param(&request, id, value) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return backend_request_submit_async(&request);
}

rt_err_t backend_request_set_params_async(param_service_id_t first_id,
                                          const rt_int32_t *values,
                                          rt_size_t count)
{
    backend_request_t request;

    if (backend_request_fill_set_params(&request, first_id, values, count) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return backend_request_submit_async(&request);
}

rt_err_t backend_request_set_param_list_async(const param_service_id_t *ids,
                                              const rt_int32_t *values,
                                              rt_size_t count)
{
    backend_request_t request;

    if (backend_request_fill_set_param_list(&request, ids, values, count) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return backend_request_submit_async(&request);
}

rt_err_t backend_request_save_params(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_SAVE_PARAMS;
    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_load_params(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_LOAD_PARAMS;
    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_reset_params(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_RESET_PARAMS;
    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_clear_records(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_CLEAR_RECORDS;
    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_apply_modbus_rtu(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_APPLY_MODBUS_RTU;
    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_apply_network(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_APPLY_NETWORK;
    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_set_factory_mac(const rt_uint8_t mac[6])
{
    backend_request_t request;

    if (backend_request_fill_set_factory_mac(&request, mac) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return backend_request_submit_sync(&request);
}

rt_err_t backend_request_save_params_async(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_SAVE_PARAMS;
    return backend_request_submit_async_tracked(&request);
}

rt_err_t backend_request_load_params_async(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_LOAD_PARAMS;
    return backend_request_submit_async_tracked(&request);
}

rt_err_t backend_request_reset_params_async(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_RESET_PARAMS;
    return backend_request_submit_async_tracked(&request);
}

rt_err_t backend_request_clear_records_async(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_CLEAR_RECORDS;
    return backend_request_submit_async_tracked(&request);
}

rt_err_t backend_request_apply_modbus_rtu_async(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_APPLY_MODBUS_RTU;
    return backend_request_submit_async_tracked(&request);
}

rt_err_t backend_request_apply_network_async(void)
{
    backend_request_t request;

    memset(&request, 0, sizeof(request));
    request.type = BACKEND_REQUEST_APPLY_NETWORK;
    return backend_request_submit_async_tracked(&request);
}

rt_err_t backend_request_set_factory_mac_async(const rt_uint8_t mac[6])
{
    backend_request_t request;

    if (backend_request_fill_set_factory_mac(&request, mac) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return backend_request_submit_async_tracked(&request);
}

void backend_request_get_status(backend_request_status_t *status)
{
    rt_base_t level;

    if (status == RT_NULL)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    *status = backend_request_status;
    rt_hw_interrupt_enable(level);
}
