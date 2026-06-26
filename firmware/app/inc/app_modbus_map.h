#ifndef APP_MODBUS_MAP_H
#define APP_MODBUS_MAP_H

#include <rtthread.h>

typedef struct
{
    rt_uint32_t holding_reads;
    rt_uint32_t holding_writes;
    rt_uint32_t input_reads;
} modbus_map_stats_t;

void modbus_map_get_stats(modbus_map_stats_t *stats);

#endif
