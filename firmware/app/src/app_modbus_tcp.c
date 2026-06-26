#include "app_modbus_tcp.h"

#include <rthw.h>
#include <string.h>

#include "app_modbus_map.h"
#include "app_param_service.h"
#include "app_factory_service.h"
#include "finsh.h"
#include "mb.h"
#include "mbproto.h"
#include "net_w5500.h"

#define MODBUS_TCP_THREAD_STACK_SIZE      2048U
#define MODBUS_TCP_THREAD_PRIORITY        16U
#define MODBUS_TCP_THREAD_TIMESLICE       10U
#define MODBUS_TCP_APPLY_TIMEOUT_MS       35000U
#define MODBUS_TCP_DHCP_TIMEOUT_MS        30000U
#define MODBUS_TCP_POLL_DELAY_MS          2U
#define MODBUS_TCP_IDLE_TIMEOUT_MS        5000U
#define MODBUS_TCP_EVENT_APPLY            0x01U

#define MODBUS_TCP_MBAP_LENGTH            7U
#define MODBUS_TCP_ADU_MAX                260U
#define MODBUS_TCP_PDU_MAX                253U

#define MODBUS_TCP_FUNC_READ_HOLDING      0x03U
#define MODBUS_TCP_FUNC_READ_INPUT        0x04U
#define MODBUS_TCP_FUNC_WRITE_SINGLE      0x06U
#define MODBUS_TCP_FUNC_WRITE_MULTIPLE    0x10U

#define MODBUS_TCP_EX_ILLEGAL_FUNCTION    0x01U
#define MODBUS_TCP_EX_ILLEGAL_ADDRESS     0x02U
#define MODBUS_TCP_EX_ILLEGAL_VALUE       0x03U
#define MODBUS_TCP_EX_SERVER_FAILURE      0x04U
#define MODBUS_TCP_EX_SERVER_BUSY         0x06U

#define MODBUS_TCP_SOCKET_ESTABLISHED     0x17U

static rt_thread_t modbus_tcp_thread;
static struct rt_event modbus_tcp_event;
static rt_sem_t modbus_tcp_apply_done;
static volatile rt_uint8_t modbus_tcp_running;
static volatile rt_uint8_t modbus_tcp_apply_pending;
static volatile rt_err_t modbus_tcp_apply_result;
static volatile rt_uint32_t modbus_tcp_rx_count;
static volatile rt_uint32_t modbus_tcp_tx_count;
static volatile rt_uint32_t modbus_tcp_error_count;
static rt_uint32_t modbus_tcp_running_ip;
static rt_uint32_t modbus_tcp_running_netmask;
static rt_uint32_t modbus_tcp_running_gateway;
static rt_uint32_t modbus_tcp_running_dns;
static rt_uint32_t modbus_tcp_running_lease_seconds;
static rt_uint16_t modbus_tcp_running_port;
static rt_uint8_t modbus_tcp_running_mac[6];
static rt_uint8_t modbus_tcp_running_dhcp;
static volatile rt_uint8_t modbus_tcp_socket_status;
static volatile rt_uint8_t modbus_tcp_link_up;
static volatile rt_uint8_t modbus_tcp_speed_100m;
static volatile rt_uint8_t modbus_tcp_full_duplex;

static rt_uint16_t modbus_tcp_get_u16(const rt_uint8_t *data)
{
    return (rt_uint16_t)(((rt_uint16_t)data[0] << 8) | data[1]);
}

static void modbus_tcp_put_u16(rt_uint8_t *data, rt_uint16_t value)
{
    data[0] = (rt_uint8_t)(value >> 8);
    data[1] = (rt_uint8_t)(value & 0xFFU);
}

static void modbus_tcp_format_ip(rt_uint32_t ip, char *buffer, rt_size_t buffer_size)
{
    if (buffer == RT_NULL || buffer_size == 0U)
    {
        return;
    }

    rt_snprintf(buffer,
                buffer_size,
                "%u.%u.%u.%u",
                (rt_uint32_t)((ip >> 24) & 0xFFU),
                (rt_uint32_t)((ip >> 16) & 0xFFU),
                (rt_uint32_t)((ip >> 8) & 0xFFU),
                (rt_uint32_t)(ip & 0xFFU));
}

static rt_uint8_t modbus_tcp_exception_from_map(eMBErrorCode result)
{
    switch (result)
    {
    case MB_ENOREG:
        return MODBUS_TCP_EX_ILLEGAL_ADDRESS;
    case MB_EINVAL:
        return MODBUS_TCP_EX_ILLEGAL_VALUE;
    case MB_ETIMEDOUT:
        return MODBUS_TCP_EX_SERVER_BUSY;
    case MB_EIO:
    case MB_EPORTERR:
    case MB_ENORES:
    default:
        return MODBUS_TCP_EX_SERVER_FAILURE;
    }
}

static rt_size_t modbus_tcp_build_exception(rt_uint8_t *response,
                                            const rt_uint8_t *request,
                                            rt_uint8_t function,
                                            rt_uint8_t exception)
{
    memcpy(response, request, MODBUS_TCP_MBAP_LENGTH);
    modbus_tcp_put_u16(&response[4], 3U);
    response[7] = (rt_uint8_t)(function | 0x80U);
    response[8] = exception;
    return 9U;
}

static rt_size_t modbus_tcp_handle_read_registers(const rt_uint8_t *request,
                                                  rt_size_t request_length,
                                                  rt_uint8_t *response,
                                                  rt_uint8_t function)
{
    rt_uint16_t start;
    rt_uint16_t count;
    rt_uint16_t callback_address;
    eMBErrorCode result;
    rt_size_t data_bytes;

    if (request_length < 12U)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          function,
                                          MODBUS_TCP_EX_ILLEGAL_VALUE);
    }

    start = modbus_tcp_get_u16(&request[8]);
    count = modbus_tcp_get_u16(&request[10]);
    if (count == 0U || count > 125U)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          function,
                                          MODBUS_TCP_EX_ILLEGAL_VALUE);
    }

    callback_address = (rt_uint16_t)(start + 1U);
    memcpy(response, request, MODBUS_TCP_MBAP_LENGTH);
    response[7] = function;
    response[8] = (rt_uint8_t)(count * 2U);

    if (function == MODBUS_TCP_FUNC_READ_HOLDING)
    {
        result = eMBRegHoldingCB(&response[9], callback_address, count, MB_REG_READ);
    }
    else
    {
        result = eMBRegInputCB(&response[9], callback_address, count);
    }

    if (result != MB_ENOERR)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          function,
                                          modbus_tcp_exception_from_map(result));
    }

    data_bytes = (rt_size_t)count * 2U;
    modbus_tcp_put_u16(&response[4], (rt_uint16_t)(data_bytes + 3U));
    return data_bytes + 9U;
}

static rt_size_t modbus_tcp_handle_write_single(const rt_uint8_t *request,
                                                rt_size_t request_length,
                                                rt_uint8_t *response)
{
    rt_uint16_t address;
    eMBErrorCode result;
    rt_uint8_t value[2];

    if (request_length < 12U)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          MODBUS_TCP_FUNC_WRITE_SINGLE,
                                          MODBUS_TCP_EX_ILLEGAL_VALUE);
    }

    address = (rt_uint16_t)(modbus_tcp_get_u16(&request[8]) + 1U);
    value[0] = request[10];
    value[1] = request[11];
    result = eMBRegHoldingCB(value, address, 1U, MB_REG_WRITE);
    if (result != MB_ENOERR)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          MODBUS_TCP_FUNC_WRITE_SINGLE,
                                          modbus_tcp_exception_from_map(result));
    }

    memcpy(response, request, 12U);
    return 12U;
}

static rt_size_t modbus_tcp_handle_write_multiple(const rt_uint8_t *request,
                                                  rt_size_t request_length,
                                                  rt_uint8_t *response)
{
    rt_uint16_t start;
    rt_uint16_t count;
    rt_uint8_t byte_count;
    eMBErrorCode result;

    if (request_length < 13U)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          MODBUS_TCP_FUNC_WRITE_MULTIPLE,
                                          MODBUS_TCP_EX_ILLEGAL_VALUE);
    }

    start = modbus_tcp_get_u16(&request[8]);
    count = modbus_tcp_get_u16(&request[10]);
    byte_count = request[12];
    if (count == 0U || count > 123U ||
        byte_count != (rt_uint8_t)(count * 2U) ||
        request_length < (rt_size_t)(13U + byte_count))
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          MODBUS_TCP_FUNC_WRITE_MULTIPLE,
                                          MODBUS_TCP_EX_ILLEGAL_VALUE);
    }

    result = eMBRegHoldingCB((UCHAR *)&request[13],
                             (rt_uint16_t)(start + 1U),
                             count,
                             MB_REG_WRITE);
    if (result != MB_ENOERR)
    {
        return modbus_tcp_build_exception(response,
                                          request,
                                          MODBUS_TCP_FUNC_WRITE_MULTIPLE,
                                          modbus_tcp_exception_from_map(result));
    }

    memcpy(response, request, MODBUS_TCP_MBAP_LENGTH);
    response[7] = MODBUS_TCP_FUNC_WRITE_MULTIPLE;
    response[8] = (rt_uint8_t)(start >> 8);
    response[9] = (rt_uint8_t)(start & 0xFFU);
    response[10] = (rt_uint8_t)(count >> 8);
    response[11] = (rt_uint8_t)(count & 0xFFU);
    modbus_tcp_put_u16(&response[4], 6U);
    return 12U;
}

static rt_size_t modbus_tcp_handle_frame(const rt_uint8_t *request,
                                         rt_size_t request_length,
                                         rt_uint8_t *response,
                                         rt_size_t response_capacity)
{
    rt_uint16_t protocol;
    rt_uint16_t mbap_length;
    rt_uint8_t function;

    if (request == RT_NULL || response == RT_NULL ||
        response_capacity < MODBUS_TCP_ADU_MAX ||
        request_length < 8U)
    {
        return 0U;
    }

    protocol = modbus_tcp_get_u16(&request[2]);
    mbap_length = modbus_tcp_get_u16(&request[4]);
    function = request[7];
    if (protocol != 0U ||
        mbap_length < 2U ||
        request_length < (rt_size_t)(6U + mbap_length))
    {
        return 0U;
    }

    switch (function)
    {
    case MODBUS_TCP_FUNC_READ_HOLDING:
    case MODBUS_TCP_FUNC_READ_INPUT:
        return modbus_tcp_handle_read_registers(request,
                                                request_length,
                                                response,
                                                function);
    case MODBUS_TCP_FUNC_WRITE_SINGLE:
        return modbus_tcp_handle_write_single(request, request_length, response);
    case MODBUS_TCP_FUNC_WRITE_MULTIPLE:
        return modbus_tcp_handle_write_multiple(request, request_length, response);
    default:
        return modbus_tcp_build_exception(response,
                                          request,
                                          function,
                                          MODBUS_TCP_EX_ILLEGAL_FUNCTION);
    }
}

static rt_err_t modbus_tcp_apply_from_params(void)
{
    net_w5500_config_t config;
    net_w5500_dhcp_result_t dhcp_result;
    rt_uint32_t dhcp_enable = 0U;
    rt_int32_t ip_value = 0;
    rt_int32_t netmask_value = 0;
    rt_int32_t gateway_value = 0;
    rt_uint32_t ip = 0U;
    rt_uint32_t netmask = 0U;
    rt_uint32_t gateway = 0U;
    rt_uint32_t dns = 0U;
    rt_uint32_t lease_seconds = 0U;
    rt_uint32_t port = 502U;
    rt_uint8_t dhcp_active = 0U;
    char ip_text[16];
    char gateway_text[16];
    rt_err_t result;

    (void)param_service_get(PARAM_SERVICE_ID_NET_DHCP_ENABLE, &dhcp_enable);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NET_IP_ADDR, &ip_value);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NET_NETMASK, &netmask_value);
    (void)param_service_get_i32(PARAM_SERVICE_ID_NET_GATEWAY, &gateway_value);
    (void)param_service_get(PARAM_SERVICE_ID_MODBUS_TCP_PORT, &port);
    ip = (rt_uint32_t)ip_value;
    netmask = (rt_uint32_t)netmask_value;
    gateway = (rt_uint32_t)gateway_value;

    memset(&config, 0, sizeof(config));
    config.ip = ip;
    config.netmask = netmask;
    config.gateway = gateway;
    if (factory_service_get_mac(config.mac) != RT_EOK)
    {
        net_w5500_make_local_mac(ip, config.mac);
    }

    if (dhcp_enable != 0U)
    {
        result = net_w5500_acquire_dhcp(config.mac,
                                        MODBUS_TCP_DHCP_TIMEOUT_MS,
                                        &dhcp_result);
        if (result == RT_EOK && dhcp_result.ip != 0U)
        {
            config.ip = dhcp_result.ip;
            config.netmask = dhcp_result.netmask;
            config.gateway = dhcp_result.gateway;
            ip = dhcp_result.ip;
            netmask = dhcp_result.netmask;
            gateway = dhcp_result.gateway;
            dns = dhcp_result.dns;
            lease_seconds = dhcp_result.lease_seconds;
            dhcp_active = 1U;

            modbus_tcp_format_ip(ip, ip_text, sizeof(ip_text));
            modbus_tcp_format_ip(gateway, gateway_text, sizeof(gateway_text));
            rt_kprintf("[INFO] modbus_tcp: dhcp lease ip=%s gw=%s lease=%u\r\n",
                       ip_text,
                       gateway_text,
                       (rt_uint32_t)dhcp_result.lease_seconds);
        }
        else
        {
            modbus_tcp_format_ip(ip, ip_text, sizeof(ip_text));
            rt_kprintf("[WARN] modbus_tcp: dhcp failed %d, fallback static %s\r\n",
                       result,
                       ip_text);
        }
    }

    result = net_w5500_apply_config(&config);
    if (result == RT_EOK)
    {
        result = net_w5500_socket0_listen((rt_uint16_t)port);
    }
    if (result != RT_EOK)
    {
        modbus_tcp_running = 0U;
        return result;
    }

    modbus_tcp_running_ip = ip;
    modbus_tcp_running_netmask = netmask;
    modbus_tcp_running_gateway = gateway;
    modbus_tcp_running_dns = dns;
    modbus_tcp_running_lease_seconds = lease_seconds;
    modbus_tcp_running_port = (rt_uint16_t)port;
    memcpy(modbus_tcp_running_mac, config.mac, sizeof(modbus_tcp_running_mac));
    modbus_tcp_running_dhcp = dhcp_active;
    modbus_tcp_socket_status = net_w5500_socket0_status();
    modbus_tcp_running = 1U;

    modbus_tcp_format_ip(ip, ip_text, sizeof(ip_text));
    rt_kprintf("[INFO] modbus_tcp: listen %s:%u mode=%s mac=%02x:%02x:%02x:%02x:%02x:%02x socket=0\r\n",
               ip_text,
               (rt_uint32_t)modbus_tcp_running_port,
               (modbus_tcp_running_dhcp != 0U) ? "dhcp" : "static",
               modbus_tcp_running_mac[0],
               modbus_tcp_running_mac[1],
               modbus_tcp_running_mac[2],
               modbus_tcp_running_mac[3],
               modbus_tcp_running_mac[4],
               modbus_tcp_running_mac[5]);

    return RT_EOK;
}

static void modbus_tcp_finish_apply(rt_err_t result)
{
    rt_uint8_t was_pending = modbus_tcp_apply_pending;

    modbus_tcp_apply_result = result;
    modbus_tcp_apply_pending = 0U;
    if (was_pending != 0U && modbus_tcp_apply_done != RT_NULL)
    {
        rt_sem_release(modbus_tcp_apply_done);
    }
}

static void modbus_tcp_thread_entry(void *parameter)
{
    rt_uint8_t request[MODBUS_TCP_ADU_MAX];
    rt_uint8_t response[MODBUS_TCP_ADU_MAX];
    rt_uint32_t idle_since_ms = 0U;
    rt_uint8_t idle_tracking = 0U;

    RT_UNUSED(parameter);

    modbus_tcp_finish_apply(modbus_tcp_apply_from_params());

    while (1)
    {
        rt_uint32_t event_set;
        rt_size_t request_length = 0U;
        rt_size_t response_length;
        rt_err_t result;
        net_w5500_status_t phy_status;

        if (rt_event_recv(&modbus_tcp_event,
                          MODBUS_TCP_EVENT_APPLY,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          0,
                          &event_set) == RT_EOK)
        {
            modbus_tcp_finish_apply(modbus_tcp_apply_from_params());
            continue;
        }

        result = net_w5500_socket0_service();
        modbus_tcp_socket_status = net_w5500_socket0_status();
        if (net_w5500_read_status(&phy_status) == RT_EOK)
        {
            modbus_tcp_link_up = phy_status.link_up;
            modbus_tcp_speed_100m = (phy_status.link_up != 0U) ? phy_status.speed_100m : 0U;
            modbus_tcp_full_duplex = (phy_status.link_up != 0U) ? phy_status.full_duplex : 0U;
        }
        if (result != RT_EOK && modbus_tcp_running != 0U)
        {
            (void)net_w5500_socket0_listen(modbus_tcp_running_port);
            modbus_tcp_socket_status = net_w5500_socket0_status();
            idle_tracking = 0U;
            idle_since_ms = 0U;
        }

        if (modbus_tcp_socket_status == MODBUS_TCP_SOCKET_ESTABLISHED)
        {
            rt_uint32_t now_ms = rt_tick_get_millisecond();

            if (idle_tracking == 0U)
            {
                idle_tracking = 1U;
                idle_since_ms = now_ms;
            }
            else if ((now_ms - idle_since_ms) >= MODBUS_TCP_IDLE_TIMEOUT_MS)
            {
                (void)net_w5500_socket0_listen(modbus_tcp_running_port);
                modbus_tcp_socket_status = net_w5500_socket0_status();
                modbus_tcp_error_count++;
                idle_tracking = 0U;
                idle_since_ms = 0U;
                rt_thread_mdelay(MODBUS_TCP_POLL_DELAY_MS);
                continue;
            }
        }
        else
        {
            idle_tracking = 0U;
            idle_since_ms = 0U;
        }

        result = net_w5500_socket0_recv(request, sizeof(request), &request_length);
        if (result == RT_EOK && request_length > 0U)
        {
            idle_since_ms = rt_tick_get_millisecond();
            idle_tracking = 1U;
            modbus_tcp_rx_count++;
            response_length = modbus_tcp_handle_frame(request,
                                                      request_length,
                                                      response,
                                                      sizeof(response));
            if (response_length > 0U)
            {
                if (net_w5500_socket0_send(response, response_length) == RT_EOK)
                {
                    modbus_tcp_tx_count++;
                }
                else
                {
                    modbus_tcp_error_count++;
                }
            }
            else
            {
                modbus_tcp_error_count++;
            }
        }

        rt_thread_mdelay(MODBUS_TCP_POLL_DELAY_MS);
    }
}

rt_err_t app_modbus_tcp_init(void)
{
    if (modbus_tcp_thread != RT_NULL)
    {
        return RT_EOK;
    }

    rt_event_init(&modbus_tcp_event, "mbtcp_ctl", RT_IPC_FLAG_PRIO);
    if (modbus_tcp_apply_done == RT_NULL)
    {
        modbus_tcp_apply_done = rt_sem_create("mbtcp_ap", 0U, RT_IPC_FLAG_PRIO);
        if (modbus_tcp_apply_done == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    modbus_tcp_thread = rt_thread_create("mb_tcp",
                                         modbus_tcp_thread_entry,
                                         RT_NULL,
                                         MODBUS_TCP_THREAD_STACK_SIZE,
                                         MODBUS_TCP_THREAD_PRIORITY,
                                         MODBUS_TCP_THREAD_TIMESLICE);
    if (modbus_tcp_thread == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    return rt_thread_startup(modbus_tcp_thread);
}

rt_err_t app_modbus_tcp_request_apply(void)
{
    rt_err_t result;

    if (modbus_tcp_thread == RT_NULL || modbus_tcp_apply_done == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (modbus_tcp_apply_pending != 0U)
    {
        return -RT_EBUSY;
    }

    modbus_tcp_apply_pending = 1U;
    modbus_tcp_apply_result = -RT_EBUSY;
    if (rt_event_send(&modbus_tcp_event, MODBUS_TCP_EVENT_APPLY) != RT_EOK)
    {
        modbus_tcp_apply_pending = 0U;
        return -RT_ERROR;
    }

    result = rt_sem_take(modbus_tcp_apply_done,
                         rt_tick_from_millisecond(MODBUS_TCP_APPLY_TIMEOUT_MS));
    if (result != RT_EOK)
    {
        modbus_tcp_apply_pending = 0U;
        return -RT_ETIMEOUT;
    }

    return modbus_tcp_apply_result;
}

void app_modbus_tcp_get_status(app_modbus_tcp_status_t *status)
{
    rt_base_t level;

    if (status == RT_NULL)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    status->running = modbus_tcp_running;
    status->dhcp_active = modbus_tcp_running_dhcp;
    status->apply_pending = modbus_tcp_apply_pending;
    status->socket_status = modbus_tcp_socket_status;
    status->link_up = modbus_tcp_link_up;
    status->speed_100m = modbus_tcp_speed_100m;
    status->full_duplex = modbus_tcp_full_duplex;
    status->ip = modbus_tcp_running_ip;
    status->netmask = modbus_tcp_running_netmask;
    status->gateway = modbus_tcp_running_gateway;
    status->dns = modbus_tcp_running_dns;
    status->lease_seconds = modbus_tcp_running_lease_seconds;
    status->port = modbus_tcp_running_port;
    memcpy(status->mac, modbus_tcp_running_mac, sizeof(status->mac));
    status->rx_count = modbus_tcp_rx_count;
    status->tx_count = modbus_tcp_tx_count;
    status->error_count = modbus_tcp_error_count;
    rt_hw_interrupt_enable(level);
}

static int app_shell_modbus_tcp(int argc, char **argv)
{
    char ip_text[16];

    if (argc == 2 && strcmp(argv[1], "apply") == 0)
    {
        rt_err_t result = app_modbus_tcp_request_apply();

        rt_kprintf("[INFO] modbus_tcp: apply result=%d running=%u\r\n",
                   result,
                   modbus_tcp_running);
        return 0;
    }

    if (argc != 1 && !(argc == 2 && strcmp(argv[1], "status") == 0))
    {
        rt_kprintf("[WARN] modbus_tcp: usage modbus_tcp [status] | apply\r\n");
        return 0;
    }

    modbus_tcp_format_ip(modbus_tcp_running_ip, ip_text, sizeof(ip_text));
    rt_kprintf("[INFO] modbus_tcp: running=%u apply_pending=%u socket_sr=0x%02x rx=%u tx=%u err=%u\r\n",
               modbus_tcp_running,
               modbus_tcp_apply_pending,
               net_w5500_socket0_status(),
               modbus_tcp_rx_count,
               modbus_tcp_tx_count,
               modbus_tcp_error_count);
    rt_kprintf("[INFO] modbus_tcp: active %s:%u mode=%s mac=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
               ip_text,
               (rt_uint32_t)modbus_tcp_running_port,
               (modbus_tcp_running_dhcp != 0U) ? "dhcp" : "static",
               modbus_tcp_running_mac[0],
               modbus_tcp_running_mac[1],
               modbus_tcp_running_mac[2],
               modbus_tcp_running_mac[3],
               modbus_tcp_running_mac[4],
               modbus_tcp_running_mac[5]);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_modbus_tcp, modbus_tcp, show Modbus TCP server status);
