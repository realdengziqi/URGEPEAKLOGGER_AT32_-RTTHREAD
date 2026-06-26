#ifndef APP_MODBUS_TCP_H
#define APP_MODBUS_TCP_H

#include <rtthread.h>

typedef struct
{
    rt_uint8_t running;
    rt_uint8_t dhcp_active;
    rt_uint8_t apply_pending;
    rt_uint8_t socket_status;
    rt_uint8_t link_up;
    rt_uint8_t speed_100m;
    rt_uint8_t full_duplex;
    rt_uint32_t ip;
    rt_uint32_t netmask;
    rt_uint32_t gateway;
    rt_uint32_t dns;
    rt_uint32_t lease_seconds;
    rt_uint16_t port;
    rt_uint8_t mac[6];
    rt_uint32_t rx_count;
    rt_uint32_t tx_count;
    rt_uint32_t error_count;
} app_modbus_tcp_status_t;

rt_err_t app_modbus_tcp_init(void);
rt_err_t app_modbus_tcp_request_apply(void);
void app_modbus_tcp_get_status(app_modbus_tcp_status_t *status);

#endif
