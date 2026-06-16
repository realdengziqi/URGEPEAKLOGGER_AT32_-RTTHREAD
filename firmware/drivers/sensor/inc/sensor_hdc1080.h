#ifndef SENSOR_HDC1080_H
#define SENSOR_HDC1080_H

#include <rtthread.h>

#define SENSOR_HDC1080_I2C_ADDR_7BIT          0x40U
#define SENSOR_HDC1080_EXPECTED_MANUFACTURER  0x5449U
#define SENSOR_HDC1080_EXPECTED_DEVICE_ID     0x1050U

typedef struct
{
    rt_uint16_t raw_temperature;
    rt_uint16_t raw_humidity;
    rt_int32_t temperature_milli_c;
    rt_int32_t humidity_milli_percent;
} sensor_hdc1080_sample_t;

rt_err_t sensor_hdc1080_init(void);
rt_err_t sensor_hdc1080_read_ids(rt_uint16_t *manufacturer_id, rt_uint16_t *device_id);
rt_err_t sensor_hdc1080_read_sample(sensor_hdc1080_sample_t *sample);

#endif
