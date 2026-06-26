#ifndef NET_W5500_H
#define NET_W5500_H

#include <rtthread.h>

typedef struct
{
    rt_uint8_t version;
    rt_uint8_t phycfgr;
    rt_uint8_t link_up;
    rt_uint8_t speed_100m;
    rt_uint8_t full_duplex;
    rt_uint8_t int_pin_level;
} net_w5500_status_t;

typedef struct
{
    rt_uint32_t ip;
    rt_uint32_t netmask;
    rt_uint32_t gateway;
    rt_uint8_t mac[6];
} net_w5500_config_t;

typedef struct
{
    rt_uint32_t ip;
    rt_uint32_t netmask;
    rt_uint32_t gateway;
    rt_uint32_t dns;
    rt_uint32_t lease_seconds;
} net_w5500_dhcp_result_t;

rt_err_t net_w5500_init(void);
rt_err_t net_w5500_apply_config(const net_w5500_config_t *config);
rt_err_t net_w5500_acquire_dhcp(const rt_uint8_t mac[6],
                                rt_uint32_t timeout_ms,
                                net_w5500_dhcp_result_t *result);
rt_err_t net_w5500_read_status(net_w5500_status_t *status);
rt_err_t net_w5500_read_common(rt_uint16_t address, rt_uint8_t *value);
rt_err_t net_w5500_write_common(rt_uint16_t address, rt_uint8_t value);
rt_err_t net_w5500_socket0_listen(rt_uint16_t port);
rt_err_t net_w5500_socket0_service(void);
rt_err_t net_w5500_socket0_recv(rt_uint8_t *buffer, rt_size_t capacity, rt_size_t *length);
rt_err_t net_w5500_socket0_send(const rt_uint8_t *buffer, rt_size_t length);
rt_uint8_t net_w5500_socket0_status(void);
void net_w5500_make_local_mac(rt_uint32_t ip, rt_uint8_t mac[6]);
const char *net_w5500_link_name(rt_uint8_t link_up);

#endif
