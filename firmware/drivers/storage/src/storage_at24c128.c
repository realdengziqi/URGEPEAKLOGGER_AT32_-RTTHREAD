#include "storage_at24c128.h"

#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_i2c.h"

#define AT24C128_I2C                         I2C2
#define AT24C128_I2C_SPEED_HZ                100000U
#define AT24C128_I2C_ADDRESS_8BIT            (STORAGE_AT24C128_I2C_ADDR_7BIT << 1)
#define AT24C128_I2C_TIMEOUT_MS              50U
#define AT24C128_WRITE_READY_TIMEOUT_MS      20U

static rt_bool_t at24c128_initialized = RT_FALSE;
static rt_mutex_t at24c128_bus_lock = RT_NULL;

static rt_bool_t at24c128_timeout_expired(rt_uint32_t start_ms, rt_uint32_t timeout_ms)
{
    return ((rt_int32_t)(rt_tick_get_millisecond() - start_ms) >= (rt_int32_t)timeout_ms) ? RT_TRUE : RT_FALSE;
}

static void at24c128_i2c_clear_errors(void)
{
    i2c_flag_clear(AT24C128_I2C, I2C_BUSERR_FLAG);
    i2c_flag_clear(AT24C128_I2C, I2C_ARLOST_FLAG);
    i2c_flag_clear(AT24C128_I2C, I2C_ACKFAIL_FLAG);
    i2c_flag_clear(AT24C128_I2C, I2C_OUF_FLAG);
    i2c_flag_clear(AT24C128_I2C, I2C_TMOUT_FLAG);
}

static rt_bool_t at24c128_i2c_has_error(void)
{
    if (i2c_flag_get(AT24C128_I2C, I2C_BUSERR_FLAG) != RESET ||
        i2c_flag_get(AT24C128_I2C, I2C_ARLOST_FLAG) != RESET ||
        i2c_flag_get(AT24C128_I2C, I2C_OUF_FLAG) != RESET ||
        i2c_flag_get(AT24C128_I2C, I2C_TMOUT_FLAG) != RESET)
    {
        at24c128_i2c_clear_errors();
        return RT_TRUE;
    }

    return RT_FALSE;
}

static rt_err_t at24c128_wait_flag(rt_uint32_t flag, flag_status expected, rt_uint32_t timeout_ms)
{
    rt_uint32_t start_ms = rt_tick_get_millisecond();

    while (i2c_flag_get(AT24C128_I2C, flag) != expected)
    {
        if (i2c_flag_get(AT24C128_I2C, I2C_ACKFAIL_FLAG) != RESET)
        {
            i2c_flag_clear(AT24C128_I2C, I2C_ACKFAIL_FLAG);
            return -RT_ERROR;
        }

        if (at24c128_i2c_has_error() == RT_TRUE)
        {
            return -RT_ERROR;
        }

        if (at24c128_timeout_expired(start_ms, timeout_ms) == RT_TRUE)
        {
            return -RT_ETIMEOUT;
        }

        rt_thread_mdelay(1);
    }

    return RT_EOK;
}

static rt_err_t at24c128_wait_bus_idle(void)
{
    return at24c128_wait_flag(I2C_BUSYF_FLAG, RESET, AT24C128_I2C_TIMEOUT_MS);
}

static rt_err_t at24c128_wait_addr(rt_uint32_t timeout_ms, rt_bool_t ack_poll)
{
    rt_uint32_t start_ms = rt_tick_get_millisecond();

    while (i2c_flag_get(AT24C128_I2C, I2C_ADDR7F_FLAG) == RESET)
    {
        if (i2c_flag_get(AT24C128_I2C, I2C_ACKFAIL_FLAG) != RESET)
        {
            i2c_flag_clear(AT24C128_I2C, I2C_ACKFAIL_FLAG);
            return (ack_poll == RT_TRUE) ? -RT_EBUSY : -RT_ERROR;
        }

        if (at24c128_i2c_has_error() == RT_TRUE)
        {
            return -RT_ERROR;
        }

        if (at24c128_timeout_expired(start_ms, timeout_ms) == RT_TRUE)
        {
            return -RT_ETIMEOUT;
        }

        rt_thread_mdelay(1);
    }

    return RT_EOK;
}

static void at24c128_i2c_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_I2C2_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_pins = GPIO_PINS_10 | GPIO_PINS_11;
    gpio_init(GPIOB, &gpio_init_struct);
}

static rt_err_t at24c128_bus_take(void)
{
    if (at24c128_bus_lock == RT_NULL)
    {
        at24c128_bus_lock = rt_mutex_create("at24_i2c", RT_IPC_FLAG_PRIO);
        if (at24c128_bus_lock == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    return rt_mutex_take(at24c128_bus_lock, rt_tick_from_millisecond(AT24C128_I2C_TIMEOUT_MS));
}

static void at24c128_bus_release(void)
{
    if (at24c128_bus_lock != RT_NULL)
    {
        rt_mutex_release(at24c128_bus_lock);
    }
}

static rt_err_t at24c128_address_check(rt_uint16_t address, rt_size_t length)
{
    if (length == 0U)
    {
        return RT_EOK;
    }

    if (address >= STORAGE_AT24C128_SIZE_BYTES ||
        length > (STORAGE_AT24C128_SIZE_BYTES - (rt_size_t)address))
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static rt_err_t at24c128_send_device_address(i2c_direction_type direction, rt_bool_t ack_poll)
{
    rt_err_t result;

    i2c_start_generate(AT24C128_I2C);
    result = at24c128_wait_flag(I2C_STARTF_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(AT24C128_I2C);
        return result;
    }

    i2c_7bit_address_send(AT24C128_I2C, AT24C128_I2C_ADDRESS_8BIT, direction);
    result = at24c128_wait_addr(AT24C128_I2C_TIMEOUT_MS, ack_poll);
    if (result != RT_EOK)
    {
        i2c_stop_generate(AT24C128_I2C);
        return result;
    }

    return RT_EOK;
}

static rt_err_t at24c128_send_memory_address(rt_uint16_t address)
{
    rt_err_t result;

    result = at24c128_wait_flag(I2C_TDBE_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(AT24C128_I2C);
        return result;
    }
    i2c_data_send(AT24C128_I2C, (rt_uint8_t)(address >> 8));

    result = at24c128_wait_flag(I2C_TDBE_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(AT24C128_I2C);
        return result;
    }
    i2c_data_send(AT24C128_I2C, (rt_uint8_t)(address & 0xFFU));

    return RT_EOK;
}

static rt_err_t at24c128_ready_check_unlocked(void)
{
    rt_err_t result;

    result = at24c128_wait_bus_idle();
    if (result != RT_EOK)
    {
        return result;
    }

    at24c128_i2c_clear_errors();
    i2c_master_receive_ack_set(AT24C128_I2C, I2C_MASTER_ACK_CURRENT);
    i2c_ack_enable(AT24C128_I2C, TRUE);

    result = at24c128_send_device_address(I2C_DIRECTION_TRANSMIT, RT_TRUE);
    if (result != RT_EOK)
    {
        (void)at24c128_wait_bus_idle();
        return result;
    }

    i2c_flag_clear(AT24C128_I2C, I2C_ADDR7F_FLAG);
    i2c_stop_generate(AT24C128_I2C);
    return at24c128_wait_bus_idle();
}

static rt_err_t at24c128_ready_wait_unlocked(void)
{
    rt_uint32_t start_ms = rt_tick_get_millisecond();
    rt_err_t result;

    do
    {
        result = at24c128_ready_check_unlocked();
        if (result == RT_EOK)
        {
            return RT_EOK;
        }

        rt_thread_mdelay(1);
    } while (at24c128_timeout_expired(start_ms, AT24C128_WRITE_READY_TIMEOUT_MS) == RT_FALSE);

    return -RT_ETIMEOUT;
}

static rt_err_t at24c128_write_page_unlocked(rt_uint16_t address, const rt_uint8_t *data, rt_size_t length)
{
    rt_err_t result;
    rt_size_t i;

    result = at24c128_wait_bus_idle();
    if (result != RT_EOK)
    {
        return result;
    }

    at24c128_i2c_clear_errors();
    i2c_master_receive_ack_set(AT24C128_I2C, I2C_MASTER_ACK_CURRENT);
    i2c_ack_enable(AT24C128_I2C, TRUE);

    result = at24c128_send_device_address(I2C_DIRECTION_TRANSMIT, RT_FALSE);
    if (result != RT_EOK)
    {
        return result;
    }
    i2c_flag_clear(AT24C128_I2C, I2C_ADDR7F_FLAG);

    result = at24c128_send_memory_address(address);
    if (result != RT_EOK)
    {
        return result;
    }

    for (i = 0U; i < length; i++)
    {
        result = at24c128_wait_flag(I2C_TDBE_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
        if (result != RT_EOK)
        {
            i2c_stop_generate(AT24C128_I2C);
            return result;
        }
        i2c_data_send(AT24C128_I2C, data[i]);
    }

    result = at24c128_wait_flag(I2C_TDC_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
    i2c_stop_generate(AT24C128_I2C);
    if (result != RT_EOK)
    {
        return result;
    }

    return at24c128_ready_wait_unlocked();
}

static rt_err_t at24c128_read_unlocked(rt_uint16_t address, rt_uint8_t *data, rt_size_t length)
{
    rt_err_t result;
    rt_size_t remaining = length;
    rt_uint8_t *p = data;

    result = at24c128_wait_bus_idle();
    if (result != RT_EOK)
    {
        return result;
    }

    at24c128_i2c_clear_errors();
    i2c_master_receive_ack_set(AT24C128_I2C, I2C_MASTER_ACK_CURRENT);
    i2c_ack_enable(AT24C128_I2C, TRUE);

    result = at24c128_send_device_address(I2C_DIRECTION_TRANSMIT, RT_FALSE);
    if (result != RT_EOK)
    {
        return result;
    }
    i2c_flag_clear(AT24C128_I2C, I2C_ADDR7F_FLAG);

    result = at24c128_send_memory_address(address);
    if (result != RT_EOK)
    {
        return result;
    }

    result = at24c128_wait_flag(I2C_TDBE_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(AT24C128_I2C);
        return result;
    }

    result = at24c128_send_device_address(I2C_DIRECTION_RECEIVE, RT_FALSE);
    if (result != RT_EOK)
    {
        return result;
    }

    if (remaining == 1U)
    {
        i2c_ack_enable(AT24C128_I2C, FALSE);
        i2c_flag_clear(AT24C128_I2C, I2C_ADDR7F_FLAG);
        i2c_stop_generate(AT24C128_I2C);
    }
    else if (remaining == 2U)
    {
        i2c_master_receive_ack_set(AT24C128_I2C, I2C_MASTER_ACK_NEXT);
        i2c_flag_clear(AT24C128_I2C, I2C_ADDR7F_FLAG);
        i2c_ack_enable(AT24C128_I2C, FALSE);
    }
    else
    {
        i2c_ack_enable(AT24C128_I2C, TRUE);
        i2c_flag_clear(AT24C128_I2C, I2C_ADDR7F_FLAG);
    }

    while (remaining > 0U)
    {
        if (remaining <= 3U)
        {
            if (remaining == 1U)
            {
                result = at24c128_wait_flag(I2C_RDBF_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
                if (result != RT_EOK)
                {
                    i2c_stop_generate(AT24C128_I2C);
                    break;
                }
                *p++ = i2c_data_receive(AT24C128_I2C);
                remaining--;
            }
            else if (remaining == 2U)
            {
                result = at24c128_wait_flag(I2C_TDC_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
                if (result != RT_EOK)
                {
                    i2c_stop_generate(AT24C128_I2C);
                    break;
                }
                i2c_stop_generate(AT24C128_I2C);
                *p++ = i2c_data_receive(AT24C128_I2C);
                *p++ = i2c_data_receive(AT24C128_I2C);
                remaining -= 2U;
            }
            else
            {
                result = at24c128_wait_flag(I2C_TDC_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
                if (result != RT_EOK)
                {
                    i2c_stop_generate(AT24C128_I2C);
                    break;
                }
                i2c_ack_enable(AT24C128_I2C, FALSE);
                *p++ = i2c_data_receive(AT24C128_I2C);
                remaining--;

                result = at24c128_wait_flag(I2C_TDC_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
                if (result != RT_EOK)
                {
                    i2c_stop_generate(AT24C128_I2C);
                    break;
                }
                i2c_stop_generate(AT24C128_I2C);
                *p++ = i2c_data_receive(AT24C128_I2C);
                *p++ = i2c_data_receive(AT24C128_I2C);
                remaining -= 2U;
            }
        }
        else
        {
            result = at24c128_wait_flag(I2C_RDBF_FLAG, SET, AT24C128_I2C_TIMEOUT_MS);
            if (result != RT_EOK)
            {
                i2c_stop_generate(AT24C128_I2C);
                break;
            }
            *p++ = i2c_data_receive(AT24C128_I2C);
            remaining--;
        }
    }

    i2c_ack_enable(AT24C128_I2C, TRUE);
    i2c_master_receive_ack_set(AT24C128_I2C, I2C_MASTER_ACK_CURRENT);

    return result;
}

rt_err_t storage_at24c128_init(void)
{
    rt_err_t result;

    result = at24c128_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    at24c128_i2c_gpio_init();
    i2c_reset(AT24C128_I2C);
    i2c_init(AT24C128_I2C, I2C_FSMODE_DUTY_2_1, AT24C128_I2C_SPEED_HZ);
    i2c_own_address1_set(AT24C128_I2C, I2C_ADDRESS_MODE_7BIT, 0x00U);
    i2c_ack_enable(AT24C128_I2C, TRUE);
    i2c_enable(AT24C128_I2C, TRUE);

    result = at24c128_ready_wait_unlocked();
    if (result == RT_EOK)
    {
        at24c128_initialized = RT_TRUE;
    }

    at24c128_bus_release();

    return result;
}

rt_err_t storage_at24c128_ready(void)
{
    rt_err_t result;

    if (at24c128_initialized == RT_FALSE)
    {
        return storage_at24c128_init();
    }

    result = at24c128_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    result = at24c128_ready_wait_unlocked();
    at24c128_bus_release();

    return result;
}

rt_err_t storage_at24c128_read(rt_uint16_t address, rt_uint8_t *data, rt_size_t length)
{
    rt_err_t result;

    if (data == RT_NULL && length > 0U)
    {
        return -RT_EINVAL;
    }

    result = at24c128_address_check(address, length);
    if (result != RT_EOK || length == 0U)
    {
        return result;
    }

    if (at24c128_initialized == RT_FALSE)
    {
        result = storage_at24c128_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    result = at24c128_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    result = at24c128_read_unlocked(address, data, length);
    at24c128_bus_release();

    return result;
}

rt_err_t storage_at24c128_write(rt_uint16_t address, const rt_uint8_t *data, rt_size_t length)
{
    rt_err_t result;
    rt_size_t remaining = length;
    rt_uint16_t current_address = address;
    const rt_uint8_t *current_data = data;

    if (data == RT_NULL && length > 0U)
    {
        return -RT_EINVAL;
    }

    result = at24c128_address_check(address, length);
    if (result != RT_EOK || length == 0U)
    {
        return result;
    }

    if (at24c128_initialized == RT_FALSE)
    {
        result = storage_at24c128_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    result = at24c128_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    while (remaining > 0U)
    {
        rt_size_t page_remaining =
            STORAGE_AT24C128_PAGE_SIZE_BYTES - ((rt_size_t)current_address % STORAGE_AT24C128_PAGE_SIZE_BYTES);
        rt_size_t chunk = (remaining < page_remaining) ? remaining : page_remaining;

        result = at24c128_write_page_unlocked(current_address, current_data, chunk);
        if (result != RT_EOK)
        {
            break;
        }

        current_address = (rt_uint16_t)(current_address + chunk);
        current_data += chunk;
        remaining -= chunk;
    }

    at24c128_bus_release();

    return result;
}
