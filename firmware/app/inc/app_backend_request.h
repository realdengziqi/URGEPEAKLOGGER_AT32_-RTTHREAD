#ifndef APP_BACKEND_REQUEST_H
#define APP_BACKEND_REQUEST_H

#include <rtthread.h>

#include "app_param_service.h"

typedef enum
{
    BACKEND_REQUEST_PUBLIC_NONE = 0,
    BACKEND_REQUEST_PUBLIC_SAVE_PARAMS = 1,
    BACKEND_REQUEST_PUBLIC_LOAD_PARAMS = 2,
    BACKEND_REQUEST_PUBLIC_RESET_PARAMS = 3,
    BACKEND_REQUEST_PUBLIC_CLEAR_RECORDS = 4,
    BACKEND_REQUEST_PUBLIC_APPLY_MODBUS_RTU = 5,
    BACKEND_REQUEST_PUBLIC_APPLY_NETWORK = 6,
    BACKEND_REQUEST_PUBLIC_SET_FACTORY_MAC = 7
} backend_request_public_type_t;

typedef struct
{
    rt_uint32_t sequence;
    rt_uint8_t busy;
    rt_uint8_t active_type;
    rt_int16_t last_result;
    rt_uint32_t last_done_ms;
} backend_request_status_t;

rt_err_t backend_request_init(void);
rt_err_t backend_request_set_param(param_service_id_t id, rt_int32_t value);
rt_err_t backend_request_set_param_async(param_service_id_t id, rt_int32_t value);
rt_err_t backend_request_set_params_async(param_service_id_t first_id,
                                          const rt_int32_t *values,
                                          rt_size_t count);
rt_err_t backend_request_set_param_list_async(const param_service_id_t *ids,
                                              const rt_int32_t *values,
                                              rt_size_t count);
rt_err_t backend_request_save_params(void);
rt_err_t backend_request_load_params(void);
rt_err_t backend_request_reset_params(void);
rt_err_t backend_request_clear_records(void);
rt_err_t backend_request_apply_modbus_rtu(void);
rt_err_t backend_request_apply_network(void);
rt_err_t backend_request_set_factory_mac(const rt_uint8_t mac[6]);
rt_err_t backend_request_save_params_async(void);
rt_err_t backend_request_load_params_async(void);
rt_err_t backend_request_reset_params_async(void);
rt_err_t backend_request_clear_records_async(void);
rt_err_t backend_request_apply_modbus_rtu_async(void);
rt_err_t backend_request_apply_network_async(void);
rt_err_t backend_request_set_factory_mac_async(const rt_uint8_t mac[6]);
void backend_request_get_status(backend_request_status_t *status);

#endif
