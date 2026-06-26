#ifndef SURGE_UI_TYPES_H
#define SURGE_UI_TYPES_H

#if defined(SURGE_UI_USE_RTTHREAD)
#include <rtthread.h>
#else
#include <stdint.h>
#include <stddef.h>

typedef int16_t rt_int16_t;
typedef int32_t rt_int32_t;
typedef uint8_t rt_uint8_t;
typedef uint16_t rt_uint16_t;

#ifndef RT_NULL
#define RT_NULL ((void *)0)
#endif
#endif

#endif
