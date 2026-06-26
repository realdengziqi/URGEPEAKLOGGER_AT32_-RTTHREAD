#ifndef APP_BUZZER_SERVICE_H
#define APP_BUZZER_SERVICE_H

#include <rtthread.h>

typedef enum
{
    BUZZER_SERVICE_MODE_AUTO = 0,
    BUZZER_SERVICE_MODE_FORCE_OFF,
    BUZZER_SERVICE_MODE_FORCE_ON
} buzzer_service_mode_t;

typedef struct
{
    rt_uint8_t ready;
    rt_uint8_t output_on;
    rt_uint8_t alarm_active;
    rt_uint8_t prompt_active;
    buzzer_service_mode_t mode;
    rt_uint32_t toggle_count;
    rt_uint32_t timer_ticks;
    rt_uint32_t prompt_remaining_ms;
} buzzer_service_status_t;

rt_err_t buzzer_service_init(void);
void buzzer_service_get_status(buzzer_service_status_t *status);
rt_err_t buzzer_service_set_mode(buzzer_service_mode_t mode);
void buzzer_service_set_alarm_active(rt_bool_t active);
rt_err_t buzzer_service_beep(rt_uint32_t duration_ms);
void buzzer_service_timer_isr(void);

#endif
