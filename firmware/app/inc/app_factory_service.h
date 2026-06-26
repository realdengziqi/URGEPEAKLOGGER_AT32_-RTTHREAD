#ifndef APP_FACTORY_SERVICE_H
#define APP_FACTORY_SERVICE_H

#include <rtthread.h>

typedef struct
{
    rt_uint32_t version;
    rt_uint8_t mac[6];
    rt_uint8_t mac_valid;
    rt_uint8_t reserved[5];
} factory_service_info_t;

rt_err_t factory_service_init(void);
rt_err_t factory_service_get_info(factory_service_info_t *info);
rt_err_t factory_service_get_mac(rt_uint8_t mac[6]);
rt_err_t factory_service_set_mac(const rt_uint8_t mac[6]);
rt_err_t factory_service_clear(void);
rt_bool_t factory_service_mac_is_valid(const rt_uint8_t mac[6]);

#endif
