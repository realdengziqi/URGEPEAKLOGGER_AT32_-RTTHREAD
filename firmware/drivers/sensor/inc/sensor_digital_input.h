#ifndef SENSOR_DIGITAL_INPUT_H
#define SENSOR_DIGITAL_INPUT_H

#include <rtthread.h>

#define SENSOR_DIGITAL_INPUT_COUNT    5U

typedef struct
{
    rt_uint8_t raw_state;
} sensor_digital_input_sample_t;

rt_err_t sensor_digital_input_init(void);
rt_err_t sensor_digital_input_read(sensor_digital_input_sample_t *sample);

#endif
