#ifndef APP_RTC_H
#define APP_RTC_H

#include <rtthread.h>

typedef struct
{
    rt_uint16_t year;
    rt_uint8_t month;
    rt_uint8_t day;
    rt_uint8_t weekday;
    rt_uint8_t hour;
    rt_uint8_t minute;
    rt_uint8_t second;
} app_rtc_datetime_t;

rt_err_t app_rtc_init(void);
rt_bool_t app_rtc_time_is_valid(void);
rt_err_t app_rtc_get_datetime(app_rtc_datetime_t *datetime);
rt_err_t app_rtc_get_unix_time(rt_uint32_t *unix_time);
rt_err_t app_rtc_set_datetime(const app_rtc_datetime_t *datetime);
rt_err_t app_rtc_datetime_to_unix(const app_rtc_datetime_t *datetime, rt_uint32_t *unix_time);
void app_rtc_unix_to_datetime(rt_uint32_t unix_time, app_rtc_datetime_t *datetime);

#endif
