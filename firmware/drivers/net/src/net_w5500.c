#include "net_w5500.h"

#include <string.h>

#include "at32f403a_407.h"
#include "dhcp.h"
#include "socket.h"
#include "wizchip_conf.h"

#define W5500_GPIO_SPI_PORT            GPIOB
#define W5500_GPIO_SPI_PINS            (GPIO_PINS_3 | GPIO_PINS_4 | GPIO_PINS_5)
#define W5500_GPIO_CTRL_PORT           GPIOC
#define W5500_GPIO_CS_PIN              GPIO_PINS_13
#define W5500_GPIO_INT_PIN             GPIO_PINS_14
#define W5500_GPIO_RST_PIN             GPIO_PINS_15

#define W5500_REG_PHYCFGR              0x002EU
#define W5500_REG_VERSIONR             0x0039U

#define W5500_SPI_CTRL_COMMON_READ     0x00U
#define W5500_SPI_CTRL_COMMON_WRITE    0x04U

#define W5500_PHYCFGR_LNK              0x01U
#define W5500_PHYCFGR_SPD              0x02U
#define W5500_PHYCFGR_DPX              0x04U

#define W5500_SOCKET0                  0U
#define W5500_DHCP_SOCKET              1U
#define W5500_DHCP_POLL_MS             20U

static rt_bool_t w5500_ready = RT_FALSE;

static void w5500_cs(rt_bool_t selected)
{
    gpio_bits_write(W5500_GPIO_CTRL_PORT,
                    W5500_GPIO_CS_PIN,
                    (selected != RT_FALSE) ? FALSE : TRUE);
}

static void w5500_select(void)
{
    w5500_cs(RT_TRUE);
}

static void w5500_deselect(void)
{
    w5500_cs(RT_FALSE);
}

static rt_uint8_t w5500_spi_transfer(rt_uint8_t tx)
{
    while (spi_i2s_flag_get(SPI1, SPI_I2S_TDBE_FLAG) == RESET)
    {
    }

    spi_i2s_data_transmit(SPI1, tx);

    while (spi_i2s_flag_get(SPI1, SPI_I2S_RDBF_FLAG) == RESET)
    {
    }

    return (rt_uint8_t)spi_i2s_data_receive(SPI1);
}

static void w5500_spi_write_byte(rt_uint8_t value)
{
    (void)w5500_spi_transfer(value);
}

static rt_uint8_t w5500_spi_read_byte(void)
{
    return w5500_spi_transfer(0xFFU);
}

static void w5500_spi_write_burst(rt_uint8_t *buffer, rt_uint16_t length)
{
    rt_uint16_t i;

    if (buffer == RT_NULL)
    {
        return;
    }

    for (i = 0U; i < length; i++)
    {
        (void)w5500_spi_transfer(buffer[i]);
    }
}

static void w5500_spi_read_burst(rt_uint8_t *buffer, rt_uint16_t length)
{
    rt_uint16_t i;

    if (buffer == RT_NULL)
    {
        return;
    }

    for (i = 0U; i < length; i++)
    {
        buffer[i] = w5500_spi_transfer(0xFFU);
    }
}

static void w5500_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);

    /*
     * PB3/PB4 are JTAG pins by default. Keep SWD enabled and release JTAG
     * pins for SPI1 SCK/MISO/MOSI.
     */
    gpio_pin_remap_config(SWJTAG_MUX_010, TRUE);
    gpio_pin_remap_config(SPI1_MUX_01, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = W5500_GPIO_CS_PIN | W5500_GPIO_RST_PIN;
    gpio_init(W5500_GPIO_CTRL_PORT, &gpio_init_struct);

    gpio_bits_set(W5500_GPIO_CTRL_PORT, W5500_GPIO_CS_PIN);
    gpio_bits_set(W5500_GPIO_CTRL_PORT, W5500_GPIO_RST_PIN);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pins = W5500_GPIO_INT_PIN;
    gpio_init(W5500_GPIO_CTRL_PORT, &gpio_init_struct);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = W5500_GPIO_SPI_PINS;
    gpio_init(W5500_GPIO_SPI_PORT, &gpio_init_struct);
}

static void w5500_spi_init(void)
{
    spi_init_type spi_init_struct;

    crm_periph_clock_enable(CRM_SPI1_PERIPH_CLOCK, TRUE);

    spi_i2s_reset(SPI1);
    spi_default_para_init(&spi_init_struct);
    spi_init_struct.transmission_mode = SPI_TRANSMIT_FULL_DUPLEX;
    spi_init_struct.master_slave_mode = SPI_MODE_MASTER;
    spi_init_struct.mclk_freq_division = SPI_MCLK_DIV_64;
    spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_MSB;
    spi_init_struct.frame_bit_num = SPI_FRAME_8BIT;
    spi_init_struct.clock_polarity = SPI_CLOCK_POLARITY_LOW;
    spi_init_struct.clock_phase = SPI_CLOCK_PHASE_1EDGE;
    spi_init_struct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;
    spi_init(SPI1, &spi_init_struct);
    spi_enable(SPI1, TRUE);
}

static void w5500_hardware_reset(void)
{
    gpio_bits_reset(W5500_GPIO_CTRL_PORT, W5500_GPIO_RST_PIN);
    rt_thread_mdelay(10);
    gpio_bits_set(W5500_GPIO_CTRL_PORT, W5500_GPIO_RST_PIN);
    rt_thread_mdelay(150);
}

rt_err_t net_w5500_init(void)
{
    rt_uint8_t version = 0U;
    rt_err_t result;
    rt_uint8_t buffer_size[8] = {2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U};

    w5500_gpio_init();
    w5500_spi_init();
    w5500_hardware_reset();

    reg_wizchip_cs_cbfunc(w5500_select, w5500_deselect);
    reg_wizchip_spi_cbfunc(w5500_spi_read_byte, w5500_spi_write_byte);
    reg_wizchip_spiburst_cbfunc(w5500_spi_read_burst, w5500_spi_write_burst);

    result = net_w5500_read_common(W5500_REG_VERSIONR, &version);
    if (result != RT_EOK)
    {
        w5500_ready = RT_FALSE;
        return result;
    }

    w5500_ready = (version == 0x04U) ? RT_TRUE : RT_FALSE;
    if (w5500_ready == RT_FALSE)
    {
        return -RT_ERROR;
    }

    return (wizchip_init(buffer_size, buffer_size) == 0) ? RT_EOK : -RT_ERROR;
}

rt_err_t net_w5500_read_common(rt_uint16_t address, rt_uint8_t *value)
{
    if (value == RT_NULL)
    {
        return -RT_EINVAL;
    }

    w5500_cs(RT_TRUE);
    (void)w5500_spi_transfer((rt_uint8_t)(address >> 8));
    (void)w5500_spi_transfer((rt_uint8_t)(address & 0xFFU));
    (void)w5500_spi_transfer(W5500_SPI_CTRL_COMMON_READ);
    *value = w5500_spi_transfer(0xFFU);
    w5500_cs(RT_FALSE);

    return RT_EOK;
}

rt_err_t net_w5500_write_common(rt_uint16_t address, rt_uint8_t value)
{
    w5500_cs(RT_TRUE);
    (void)w5500_spi_transfer((rt_uint8_t)(address >> 8));
    (void)w5500_spi_transfer((rt_uint8_t)(address & 0xFFU));
    (void)w5500_spi_transfer(W5500_SPI_CTRL_COMMON_WRITE);
    (void)w5500_spi_transfer(value);
    w5500_cs(RT_FALSE);

    return RT_EOK;
}

static void w5500_put_ipv4(rt_uint8_t *data, rt_uint32_t ip)
{
    data[0] = (rt_uint8_t)(ip >> 24);
    data[1] = (rt_uint8_t)(ip >> 16);
    data[2] = (rt_uint8_t)(ip >> 8);
    data[3] = (rt_uint8_t)ip;
}

static rt_uint32_t w5500_get_ipv4(const rt_uint8_t *data)
{
    if (data == RT_NULL)
    {
        return 0U;
    }

    return ((rt_uint32_t)data[0] << 24) |
           ((rt_uint32_t)data[1] << 16) |
           ((rt_uint32_t)data[2] << 8) |
           (rt_uint32_t)data[3];
}

void net_w5500_make_local_mac(rt_uint32_t ip, rt_uint8_t mac[6])
{
    rt_uint32_t mix;

    if (mac == RT_NULL)
    {
        return;
    }

    /*
     * 0x02 marks a locally administered unicast MAC. This is suitable for
     * development and small internal tests. Production should replace it with
     * a customer-assigned or IEEE-assigned MAC range.
     */
    mix = ip ^ 0x575A5500UL;
    mac[0] = 0x02U;
    mac[1] = 0x57U;
    mac[2] = 0x50U;
    mac[3] = (rt_uint8_t)(mix >> 16);
    mac[4] = (rt_uint8_t)(mix >> 8);
    mac[5] = (rt_uint8_t)mix;
}

rt_err_t net_w5500_apply_config(const net_w5500_config_t *config)
{
    wiz_NetInfo net_info;
    rt_err_t result;

    if (config == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (w5500_ready == RT_FALSE)
    {
        result = net_w5500_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    memset(&net_info, 0, sizeof(net_info));
    memcpy(net_info.mac, config->mac, sizeof(net_info.mac));
    w5500_put_ipv4(net_info.ip, config->ip);
    w5500_put_ipv4(net_info.sn, config->netmask);
    w5500_put_ipv4(net_info.gw, config->gateway);
    net_info.dhcp = NETINFO_STATIC;
    wizchip_setnetinfo(&net_info);

    return RT_EOK;
}

rt_err_t net_w5500_acquire_dhcp(const rt_uint8_t mac[6],
                                rt_uint32_t timeout_ms,
                                net_w5500_dhcp_result_t *result)
{
    static rt_uint8_t dhcp_buffer[548];
    wiz_NetInfo net_info;
    rt_uint32_t start_ms;
    rt_uint32_t last_tick_ms;
    rt_err_t init_result;

    if (mac == RT_NULL || result == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (w5500_ready == RT_FALSE)
    {
        init_result = net_w5500_init();
        if (init_result != RT_EOK)
        {
            return init_result;
        }
    }

    memset(&net_info, 0, sizeof(net_info));
    memcpy(net_info.mac, mac, sizeof(net_info.mac));
    net_info.dhcp = NETINFO_DHCP;
    wizchip_setnetinfo(&net_info);

    memset(result, 0, sizeof(*result));
    DHCP_init(W5500_DHCP_SOCKET, dhcp_buffer);
    start_ms = rt_tick_get_millisecond();
    last_tick_ms = start_ms;

    while ((rt_tick_get_millisecond() - start_ms) < timeout_ms)
    {
        rt_uint32_t now_ms = rt_tick_get_millisecond();
        rt_uint8_t dhcp_state;

        if ((now_ms - last_tick_ms) >= 1000U)
        {
            DHCP_time_handler();
            last_tick_ms += 1000U;
        }

        dhcp_state = DHCP_run();
        if (dhcp_state == DHCP_IP_ASSIGN ||
            dhcp_state == DHCP_IP_CHANGED ||
            dhcp_state == DHCP_IP_LEASED)
        {
            rt_uint8_t ip[4];
            rt_uint8_t netmask[4];
            rt_uint8_t gateway[4];
            rt_uint8_t dns[4];

            getIPfromDHCP(ip);
            getSNfromDHCP(netmask);
            getGWfromDHCP(gateway);
            getDNSfromDHCP(dns);
            result->ip = w5500_get_ipv4(ip);
            result->netmask = w5500_get_ipv4(netmask);
            result->gateway = w5500_get_ipv4(gateway);
            result->dns = w5500_get_ipv4(dns);
            result->lease_seconds = getDHCPLeasetime();
            DHCP_stop();
            return RT_EOK;
        }

        if (dhcp_state == DHCP_FAILED)
        {
            break;
        }

        rt_thread_mdelay(W5500_DHCP_POLL_MS);
    }

    DHCP_stop();
    return -RT_ETIMEOUT;
}

rt_err_t net_w5500_read_status(net_w5500_status_t *status)
{
    rt_uint8_t version;
    rt_uint8_t phycfgr;
    rt_err_t result;

    if (status == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (w5500_ready == RT_FALSE)
    {
        result = net_w5500_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    result = net_w5500_read_common(W5500_REG_VERSIONR, &version);
    if (result == RT_EOK)
    {
        result = net_w5500_read_common(W5500_REG_PHYCFGR, &phycfgr);
    }
    if (result != RT_EOK)
    {
        return result;
    }

    status->version = version;
    status->phycfgr = phycfgr;
    status->link_up = (phycfgr & W5500_PHYCFGR_LNK) ? 1U : 0U;
    status->speed_100m = (phycfgr & W5500_PHYCFGR_SPD) ? 1U : 0U;
    status->full_duplex = (phycfgr & W5500_PHYCFGR_DPX) ? 1U : 0U;
    status->int_pin_level = gpio_input_data_bit_read(W5500_GPIO_CTRL_PORT, W5500_GPIO_INT_PIN) ? 1U : 0U;

    return (version == 0x04U) ? RT_EOK : -RT_ERROR;
}

rt_uint8_t net_w5500_socket0_status(void)
{
    return getSn_SR(W5500_SOCKET0);
}

rt_err_t net_w5500_socket0_listen(rt_uint16_t port)
{
    if (w5500_ready == RT_FALSE)
    {
        rt_err_t result = net_w5500_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    (void)close(W5500_SOCKET0);
    if (socket(W5500_SOCKET0, Sn_MR_TCP, port, 0x00U) != W5500_SOCKET0)
    {
        return -RT_ERROR;
    }

    return (listen(W5500_SOCKET0) == SOCK_OK) ? RT_EOK : -RT_ERROR;
}

rt_err_t net_w5500_socket0_service(void)
{
    rt_uint8_t status = getSn_SR(W5500_SOCKET0);

    if (status == SOCK_CLOSE_WAIT)
    {
        (void)disconnect(W5500_SOCKET0);
        return RT_EOK;
    }

    if (status == SOCK_CLOSED)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t net_w5500_socket0_recv(rt_uint8_t *buffer, rt_size_t capacity, rt_size_t *length)
{
    rt_uint16_t rx_size;
    int32_t recv_len;

    if (buffer == RT_NULL || length == RT_NULL || capacity == 0U)
    {
        return -RT_EINVAL;
    }

    *length = 0U;
    if (getSn_SR(W5500_SOCKET0) != SOCK_ESTABLISHED)
    {
        return -RT_EEMPTY;
    }

    rx_size = getSn_RX_RSR(W5500_SOCKET0);
    if (rx_size == 0U)
    {
        return -RT_EEMPTY;
    }

    if (rx_size > capacity)
    {
        rx_size = (rt_uint16_t)capacity;
    }

    recv_len = recv(W5500_SOCKET0, buffer, rx_size);
    if (recv_len <= 0)
    {
        return -RT_ERROR;
    }

    *length = (rt_size_t)recv_len;
    return RT_EOK;
}

rt_err_t net_w5500_socket0_send(const rt_uint8_t *buffer, rt_size_t length)
{
    int32_t send_len;

    if (buffer == RT_NULL || length == 0U || length > 0xFFFFU)
    {
        return -RT_EINVAL;
    }

    if (getSn_SR(W5500_SOCKET0) != SOCK_ESTABLISHED)
    {
        return -RT_ERROR;
    }

    send_len = send(W5500_SOCKET0, (uint8_t *)buffer, (uint16_t)length);
    return (send_len == (int32_t)length) ? RT_EOK : -RT_ERROR;
}

const char *net_w5500_link_name(rt_uint8_t link_up)
{
    return (link_up != 0U) ? "up" : "down";
}
