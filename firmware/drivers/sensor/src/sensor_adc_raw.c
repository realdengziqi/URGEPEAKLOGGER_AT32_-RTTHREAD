#include "sensor_adc_raw.h"

#include "at32f403a_407_adc.h"
#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"

#define SENSOR_ADC_RAW_TIMEOUT_LOOP    100000U

typedef struct
{
    rt_int32_t temp_milli_c;
    rt_uint16_t raw;
} sensor_ntc_lookup_t;

/*
 * 10k NTC, B=3380, fixed divider resistor=10k.
 * Assumed topology: 3.3V -> fixed 10k -> ADC node -> NTC -> GND.
 */
static const sensor_ntc_lookup_t ntc_10k3380_table[] =
{
    {  -40000, 3928U },
    {  -35000, 3872U },
    {  -30000, 3802U },
    {  -25000, 3716U },
    {  -20000, 3613U },
    {  -15000, 3492U },
    {  -10000, 3353U },
    {   -5000, 3196U },
    {       0, 3024U },
    {    5000, 2839U },
    {   10000, 2644U },
    {   15000, 2445U },
    {   20000, 2245U },
    {   25000, 2048U },
    {   30000, 1857U },
    {   35000, 1675U },
    {   40000, 1505U },
    {   45000, 1347U },
    {   50000, 1203U },
    {   55000, 1072U },
    {   60000,  954U },
    {   65000,  849U },
    {   70000,  755U },
    {   75000,  672U },
    {   80000,  598U },
    {   85000,  533U },
    {   90000,  476U },
    {   95000,  425U },
    {  100000,  380U },
    {  105000,  341U },
    {  110000,  306U },
    {  115000,  276U },
    {  120000,  249U },
    {  125000,  224U },
};

static rt_bool_t adc_raw_initialized = RT_FALSE;
static rt_mutex_t adc_raw_mutex = RT_NULL;

static rt_err_t sensor_ntc_raw_to_milli_c(rt_uint16_t raw, rt_int32_t *temp_milli_c)
{
    rt_size_t i;

    if (temp_milli_c == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (raw > ntc_10k3380_table[0].raw ||
        raw < ntc_10k3380_table[(sizeof(ntc_10k3380_table) / sizeof(ntc_10k3380_table[0])) - 1U].raw)
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < ((sizeof(ntc_10k3380_table) / sizeof(ntc_10k3380_table[0])) - 1U); i++)
    {
        rt_uint16_t raw_high = ntc_10k3380_table[i].raw;
        rt_uint16_t raw_low = ntc_10k3380_table[i + 1U].raw;

        if (raw <= raw_high && raw >= raw_low)
        {
            rt_int32_t temp_high = ntc_10k3380_table[i].temp_milli_c;
            rt_int32_t temp_low = ntc_10k3380_table[i + 1U].temp_milli_c;
            rt_int32_t raw_span = (rt_int32_t)raw_high - (rt_int32_t)raw_low;
            rt_int32_t raw_offset = (rt_int32_t)raw_high - (rt_int32_t)raw;

            if (raw_span <= 0)
            {
                return -RT_ERROR;
            }

            *temp_milli_c = temp_high + (((temp_low - temp_high) * raw_offset) / raw_span);
            return RT_EOK;
        }
    }

    return -RT_EINVAL;
}

static rt_err_t sensor_adc_raw_wait_flag(adc_type *adc_x, rt_uint8_t flag)
{
    rt_uint32_t timeout = SENSOR_ADC_RAW_TIMEOUT_LOOP;

    while (adc_flag_get(adc_x, flag) == RESET)
    {
        if (timeout == 0U)
        {
            return -RT_ETIMEOUT;
        }
        timeout--;
    }

    return RT_EOK;
}

static rt_err_t sensor_adc_raw_calibrate(adc_type *adc_x)
{
    rt_uint32_t timeout;

    adc_enable(adc_x, TRUE);

    adc_calibration_init(adc_x);
    timeout = SENSOR_ADC_RAW_TIMEOUT_LOOP;
    while (adc_calibration_init_status_get(adc_x) != RESET)
    {
        if (timeout == 0U)
        {
            return -RT_ETIMEOUT;
        }
        timeout--;
    }

    adc_calibration_start(adc_x);
    timeout = SENSOR_ADC_RAW_TIMEOUT_LOOP;
    while (adc_calibration_status_get(adc_x) != RESET)
    {
        if (timeout == 0U)
        {
            return -RT_ETIMEOUT;
        }
        timeout--;
    }

    return RT_EOK;
}

static rt_err_t sensor_adc_raw_read_channel(adc_type *adc_x,
                                            adc_channel_select_type channel,
                                            rt_uint16_t *raw)
{
    if (raw == RT_NULL)
    {
        return -RT_EINVAL;
    }

    adc_ordinary_channel_set(adc_x, channel, 1U, ADC_SAMPLETIME_239_5);
    adc_flag_clear(adc_x, ADC_CCE_FLAG);
    adc_ordinary_software_trigger_enable(adc_x, TRUE);

    if (sensor_adc_raw_wait_flag(adc_x, ADC_CCE_FLAG) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    *raw = adc_ordinary_conversion_data_get(adc_x);
    adc_flag_clear(adc_x, ADC_CCE_FLAG);

    return RT_EOK;
}

rt_err_t sensor_adc_raw_init(void)
{
    adc_base_config_type adc_base_struct;
    gpio_init_type gpio_init_struct;

    if (adc_raw_initialized == RT_TRUE)
    {
        return RT_EOK;
    }

    if (adc_raw_mutex == RT_NULL)
    {
        adc_raw_mutex = rt_mutex_create("adc_raw", RT_IPC_FLAG_PRIO);
        if (adc_raw_mutex == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_ADC2_PERIPH_CLOCK, TRUE);
    crm_adc_clock_div_set(CRM_ADC_DIV_16);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_pins = GPIO_PINS_0 | GPIO_PINS_1;
    gpio_init(GPIOC, &gpio_init_struct);

    adc_reset(ADC1);
    adc_reset(ADC2);
    adc_combine_mode_select(ADC_INDEPENDENT_MODE);

    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = FALSE;
    adc_base_struct.repeat_mode = FALSE;
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1U;
    adc_base_config(ADC1, &adc_base_struct);
    adc_base_config(ADC2, &adc_base_struct);

    adc_ordinary_conversion_trigger_set(ADC1, ADC12_ORDINARY_TRIG_SOFTWARE, TRUE);
    adc_ordinary_conversion_trigger_set(ADC2, ADC12_ORDINARY_TRIG_SOFTWARE, TRUE);

    if (sensor_adc_raw_calibrate(ADC1) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    if (sensor_adc_raw_calibrate(ADC2) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    adc_raw_initialized = RT_TRUE;
    return RT_EOK;
}

rt_err_t sensor_adc_raw_read(sensor_adc_raw_sample_t *sample)
{
    rt_err_t result;

    if (sample == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (adc_raw_initialized == RT_FALSE)
    {
        result = sensor_adc_raw_init();
        if (result != RT_EOK)
        {
            return result;
        }
    }

    if (rt_mutex_take(adc_raw_mutex, RT_WAITING_FOREVER) != RT_EOK)
    {
        return -RT_ERROR;
    }

    sample->ntc_temp_milli_c = 0;
    sample->ntc_temp_valid = 0U;

    result = sensor_adc_raw_read_channel(ADC1, ADC_CHANNEL_10, &sample->ntc_raw);
    if (result != RT_EOK)
    {
        rt_mutex_release(adc_raw_mutex);
        return result;
    }
    if (sensor_ntc_raw_to_milli_c(sample->ntc_raw, &sample->ntc_temp_milli_c) == RT_EOK)
    {
        sample->ntc_temp_valid = 1U;
    }

    result = sensor_adc_raw_read_channel(ADC2, ADC_CHANNEL_11, &sample->surge_raw);
    if (result != RT_EOK)
    {
        rt_mutex_release(adc_raw_mutex);
        return result;
    }

    rt_mutex_release(adc_raw_mutex);
    return RT_EOK;
}
