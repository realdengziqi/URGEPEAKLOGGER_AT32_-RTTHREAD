#include "sensor_digital_input.h"

#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"

typedef struct
{
    gpio_type *port;
    rt_uint16_t pin;
} sensor_digital_input_channel_t;

static const sensor_digital_input_channel_t digital_input_channels[SENSOR_DIGITAL_INPUT_COUNT] =
{
    {GPIOA, GPIO_PINS_7}, /* TI1, raw bit DI0 */
    {GPIOA, GPIO_PINS_6}, /* TI2, raw bit DI1 */
    {GPIOA, GPIO_PINS_5}, /* TI3, raw bit DI2 */
    {GPIOA, GPIO_PINS_4}, /* TI4, raw bit DI3 */
    {GPIOA, GPIO_PINS_3}, /* TI5, raw bit DI4 */
};

static rt_bool_t digital_input_initialized = RT_FALSE;

rt_err_t sensor_digital_input_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_DOWN;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_pins = GPIO_PINS_3 | GPIO_PINS_4 | GPIO_PINS_5 | GPIO_PINS_6 | GPIO_PINS_7;
    gpio_init(GPIOA, &gpio_init_struct);

    digital_input_initialized = RT_TRUE;
    return RT_EOK;
}

rt_err_t sensor_digital_input_read(sensor_digital_input_sample_t *sample)
{
    rt_uint8_t raw_state = 0U;
    rt_uint32_t i;

    if (sample == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (digital_input_initialized == RT_FALSE)
    {
        (void)sensor_digital_input_init();
    }

    for (i = 0U; i < SENSOR_DIGITAL_INPUT_COUNT; i++)
    {
        if (gpio_input_data_bit_read(digital_input_channels[i].port,
                                     digital_input_channels[i].pin) != RESET)
        {
            raw_state |= (rt_uint8_t)(1U << i);
        }
    }

    sample->raw_state = raw_state;
    return RT_EOK;
}
