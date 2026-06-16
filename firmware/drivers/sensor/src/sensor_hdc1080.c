#include "sensor_hdc1080.h"

#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_i2c.h"

#define HDC1080_I2C                         I2C1
#define HDC1080_I2C_SPEED_HZ                100000U
#define HDC1080_I2C_ADDRESS_8BIT            (SENSOR_HDC1080_I2C_ADDR_7BIT << 1)
#define HDC1080_I2C_TIMEOUT_MS              50U
#define HDC1080_MEASUREMENT_DELAY_MS        20U

#define HDC1080_REG_TEMPERATURE             0x00U
#define HDC1080_REG_HUMIDITY                0x01U
#define HDC1080_REG_CONFIGURATION           0x02U
#define HDC1080_REG_MANUFACTURER_ID         0xFEU
#define HDC1080_REG_DEVICE_ID               0xFFU

#define HDC1080_CONFIG_DEFAULT_14BIT         0x0000U

static rt_bool_t hdc1080_initialized = RT_FALSE;
static rt_mutex_t hdc1080_bus_lock = RT_NULL;

static rt_bool_t hdc1080_timeout_expired(rt_uint32_t start_ms, rt_uint32_t timeout_ms)
{
    return ((rt_int32_t)(rt_tick_get_millisecond() - start_ms) >= (rt_int32_t)timeout_ms) ? RT_TRUE : RT_FALSE;
}

static void hdc1080_i2c_clear_errors(void)
{
    i2c_flag_clear(HDC1080_I2C, I2C_BUSERR_FLAG);
    i2c_flag_clear(HDC1080_I2C, I2C_ARLOST_FLAG);
    i2c_flag_clear(HDC1080_I2C, I2C_ACKFAIL_FLAG);
    i2c_flag_clear(HDC1080_I2C, I2C_OUF_FLAG);
    i2c_flag_clear(HDC1080_I2C, I2C_TMOUT_FLAG);
}

static rt_bool_t hdc1080_i2c_has_error(void)
{
    if (i2c_flag_get(HDC1080_I2C, I2C_BUSERR_FLAG) != RESET ||
        i2c_flag_get(HDC1080_I2C, I2C_ARLOST_FLAG) != RESET ||
        i2c_flag_get(HDC1080_I2C, I2C_ACKFAIL_FLAG) != RESET ||
        i2c_flag_get(HDC1080_I2C, I2C_OUF_FLAG) != RESET ||
        i2c_flag_get(HDC1080_I2C, I2C_TMOUT_FLAG) != RESET)
    {
        hdc1080_i2c_clear_errors();
        return RT_TRUE;
    }

    return RT_FALSE;
}

static rt_err_t hdc1080_wait_flag(rt_uint32_t flag, flag_status expected, rt_uint32_t timeout_ms)
{
    rt_uint32_t start_ms = rt_tick_get_millisecond();

    while (i2c_flag_get(HDC1080_I2C, flag) != expected)
    {
        if (hdc1080_i2c_has_error() == RT_TRUE)
        {
            return -RT_ERROR;
        }

        if (hdc1080_timeout_expired(start_ms, timeout_ms) == RT_TRUE)
        {
            return -RT_ETIMEOUT;
        }

        rt_thread_mdelay(1);
    }

    return RT_EOK;
}

static rt_err_t hdc1080_wait_bus_idle(void)
{
    return hdc1080_wait_flag(I2C_BUSYF_FLAG, RESET, HDC1080_I2C_TIMEOUT_MS);
}

static void hdc1080_i2c_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_I2C1_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_pins = GPIO_PINS_6 | GPIO_PINS_7;
    gpio_init(GPIOB, &gpio_init_struct);
}

static rt_err_t hdc1080_bus_take(void)
{
    if (hdc1080_bus_lock == RT_NULL)
    {
        hdc1080_bus_lock = rt_mutex_create("hdc_i2c", RT_IPC_FLAG_PRIO);
        if (hdc1080_bus_lock == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    return rt_mutex_take(hdc1080_bus_lock, rt_tick_from_millisecond(HDC1080_I2C_TIMEOUT_MS));
}

static void hdc1080_bus_release(void)
{
    if (hdc1080_bus_lock != RT_NULL)
    {
        rt_mutex_release(hdc1080_bus_lock);
    }
}

static rt_err_t hdc1080_write_bytes(rt_uint8_t reg, const rt_uint8_t *data, rt_size_t length)
{
    rt_err_t result;
    rt_size_t i;

    result = hdc1080_wait_bus_idle();
    if (result != RT_EOK)
    {
        return result;
    }

    hdc1080_i2c_clear_errors();
    i2c_ack_enable(HDC1080_I2C, TRUE);
    i2c_start_generate(HDC1080_I2C);

    result = hdc1080_wait_flag(I2C_STARTF_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(HDC1080_I2C);
        return result;
    }

    i2c_7bit_address_send(HDC1080_I2C, HDC1080_I2C_ADDRESS_8BIT, I2C_DIRECTION_TRANSMIT);
    result = hdc1080_wait_flag(I2C_ADDR7F_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(HDC1080_I2C);
        return result;
    }
    i2c_flag_clear(HDC1080_I2C, I2C_ADDR7F_FLAG);

    result = hdc1080_wait_flag(I2C_TDBE_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(HDC1080_I2C);
        return result;
    }
    i2c_data_send(HDC1080_I2C, reg);

    for (i = 0U; i < length; i++)
    {
        result = hdc1080_wait_flag(I2C_TDBE_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
        if (result != RT_EOK)
        {
            i2c_stop_generate(HDC1080_I2C);
            return result;
        }
        i2c_data_send(HDC1080_I2C, data[i]);
    }

    result = hdc1080_wait_flag(I2C_TDC_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    i2c_stop_generate(HDC1080_I2C);

    return result;
}

static rt_err_t hdc1080_write_pointer(rt_uint8_t reg)
{
    return hdc1080_write_bytes(reg, RT_NULL, 0U);
}

static rt_err_t hdc1080_read_two_bytes(rt_uint8_t *msb, rt_uint8_t *lsb)
{
    rt_err_t result;

    if (msb == RT_NULL || lsb == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = hdc1080_wait_bus_idle();
    if (result != RT_EOK)
    {
        return result;
    }

    hdc1080_i2c_clear_errors();
    i2c_ack_enable(HDC1080_I2C, TRUE);
    i2c_master_receive_ack_set(HDC1080_I2C, I2C_MASTER_ACK_CURRENT);
    i2c_start_generate(HDC1080_I2C);

    result = hdc1080_wait_flag(I2C_STARTF_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(HDC1080_I2C);
        return result;
    }

    i2c_7bit_address_send(HDC1080_I2C, HDC1080_I2C_ADDRESS_8BIT, I2C_DIRECTION_RECEIVE);
    result = hdc1080_wait_flag(I2C_ADDR7F_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(HDC1080_I2C);
        return result;
    }

    i2c_master_receive_ack_set(HDC1080_I2C, I2C_MASTER_ACK_NEXT);
    i2c_flag_clear(HDC1080_I2C, I2C_ADDR7F_FLAG);
    i2c_ack_enable(HDC1080_I2C, FALSE);

    result = hdc1080_wait_flag(I2C_TDC_FLAG, SET, HDC1080_I2C_TIMEOUT_MS);
    if (result != RT_EOK)
    {
        i2c_stop_generate(HDC1080_I2C);
        i2c_ack_enable(HDC1080_I2C, TRUE);
        i2c_master_receive_ack_set(HDC1080_I2C, I2C_MASTER_ACK_CURRENT);
        return result;
    }

    i2c_stop_generate(HDC1080_I2C);
    *msb = i2c_data_receive(HDC1080_I2C);
    *lsb = i2c_data_receive(HDC1080_I2C);
    i2c_ack_enable(HDC1080_I2C, TRUE);
    i2c_master_receive_ack_set(HDC1080_I2C, I2C_MASTER_ACK_CURRENT);

    return RT_EOK;
}

static rt_err_t hdc1080_read_reg16(rt_uint8_t reg, rt_uint16_t *value, rt_uint32_t delay_ms)
{
    rt_err_t result;
    rt_uint8_t msb;
    rt_uint8_t lsb;

    if (value == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = hdc1080_write_pointer(reg);
    if (result != RT_EOK)
    {
        return result;
    }

    if (delay_ms > 0U)
    {
        rt_thread_mdelay(delay_ms);
    }

    result = hdc1080_read_two_bytes(&msb, &lsb);
    if (result != RT_EOK)
    {
        return result;
    }

    *value = (rt_uint16_t)(((rt_uint16_t)msb << 8) | lsb);
    return RT_EOK;
}

static rt_err_t hdc1080_write_reg16(rt_uint8_t reg, rt_uint16_t value)
{
    rt_uint8_t data[2];

    data[0] = (rt_uint8_t)(value >> 8);
    data[1] = (rt_uint8_t)(value & 0xFFU);

    return hdc1080_write_bytes(reg, data, sizeof(data));
}

rt_err_t sensor_hdc1080_init(void)
{
    rt_err_t result;

    result = hdc1080_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    hdc1080_i2c_gpio_init();
    i2c_reset(HDC1080_I2C);
    i2c_init(HDC1080_I2C, I2C_FSMODE_DUTY_2_1, HDC1080_I2C_SPEED_HZ);
    i2c_own_address1_set(HDC1080_I2C, I2C_ADDRESS_MODE_7BIT, 0x00U);
    i2c_ack_enable(HDC1080_I2C, TRUE);
    i2c_enable(HDC1080_I2C, TRUE);

    result = hdc1080_write_reg16(HDC1080_REG_CONFIGURATION, HDC1080_CONFIG_DEFAULT_14BIT);
    if (result == RT_EOK)
    {
        hdc1080_initialized = RT_TRUE;
    }

    hdc1080_bus_release();

    return result;
}

rt_err_t sensor_hdc1080_read_ids(rt_uint16_t *manufacturer_id, rt_uint16_t *device_id)
{
    rt_err_t result;

    if (manufacturer_id == RT_NULL || device_id == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (hdc1080_initialized == RT_FALSE)
    {
        result = sensor_hdc1080_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    result = hdc1080_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    result = hdc1080_read_reg16(HDC1080_REG_MANUFACTURER_ID, manufacturer_id, 0U);
    if (result == RT_EOK)
    {
        result = hdc1080_read_reg16(HDC1080_REG_DEVICE_ID, device_id, 0U);
    }

    hdc1080_bus_release();

    return result;
}

rt_err_t sensor_hdc1080_read_sample(sensor_hdc1080_sample_t *sample)
{
    rt_err_t result;
    rt_uint16_t raw_temperature;
    rt_uint16_t raw_humidity;

    if (sample == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (hdc1080_initialized == RT_FALSE)
    {
        result = sensor_hdc1080_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    result = hdc1080_bus_take();
    if (result != RT_EOK)
    {
        return result;
    }

    result = hdc1080_read_reg16(HDC1080_REG_TEMPERATURE,
                                &raw_temperature,
                                HDC1080_MEASUREMENT_DELAY_MS);
    if (result == RT_EOK)
    {
        result = hdc1080_read_reg16(HDC1080_REG_HUMIDITY,
                                    &raw_humidity,
                                    HDC1080_MEASUREMENT_DELAY_MS);
    }

    hdc1080_bus_release();

    if (result != RT_EOK)
    {
        return result;
    }

    sample->raw_temperature = raw_temperature;
    sample->raw_humidity = raw_humidity;
    sample->temperature_milli_c = (rt_int32_t)((((rt_int64_t)raw_temperature * 165000LL) / 65536LL) - 40000LL);
    sample->humidity_milli_percent = (rt_int32_t)(((rt_int64_t)raw_humidity * 100000LL) / 65536LL);

    return RT_EOK;
}
