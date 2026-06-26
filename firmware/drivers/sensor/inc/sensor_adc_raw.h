#ifndef SENSOR_ADC_RAW_H
#define SENSOR_ADC_RAW_H

#include <rtthread.h>

typedef struct
{
    rt_uint16_t ntc_raw;
    rt_int32_t ntc_temp_milli_c;
    rt_uint8_t ntc_temp_valid;
    rt_uint16_t surge_raw;
} sensor_adc_raw_sample_t;

rt_err_t sensor_adc_raw_init(void);
rt_err_t sensor_adc_raw_read(sensor_adc_raw_sample_t *sample);

#endif
