#include "app_buzzer_service.h"

#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_misc.h"
#include "at32f403a_407_tmr.h"
#include "finsh.h"

#define BUZZER_SERVICE_TIMER                    TMR7
#define BUZZER_SERVICE_TIMER_CLOCK              CRM_TMR7_PERIPH_CLOCK
#define BUZZER_SERVICE_TIMER_IRQn               TMR7_GLOBAL_IRQn
#define BUZZER_SERVICE_TIMER_IRQ_PREEMPTION     6U
#define BUZZER_SERVICE_TIMER_IRQ_SUB            0U
#define BUZZER_SERVICE_TIMER_BASE_HZ            10000U
#define BUZZER_SERVICE_TIMER_TICK_HZ            100U
#define BUZZER_SERVICE_TIMER_TICK_MS            10U
#define BUZZER_SERVICE_ALARM_INTERVAL_MS        400U
#define BUZZER_SERVICE_PROMPT_DEFAULT_MS        60U
#define BUZZER_SERVICE_PROMPT_MAX_MS            1000U

#define BUZZER_PORT                             GPIOB
#define BUZZER_PIN                              GPIO_PINS_12

typedef struct
{
    volatile rt_uint8_t ready;
    volatile rt_uint8_t output_on;
    volatile rt_uint8_t alarm_active;
    volatile rt_uint8_t alarm_phase_on;
    volatile rt_uint16_t alarm_phase_ticks;
    volatile rt_uint16_t prompt_ticks_remaining;
    volatile buzzer_service_mode_t mode;
    volatile rt_uint32_t toggle_count;
    volatile rt_uint32_t timer_ticks;
} buzzer_service_context_t;

static buzzer_service_context_t buzzer_service_ctx;

static rt_uint16_t buzzer_service_ms_to_ticks(rt_uint32_t duration_ms)
{
    rt_uint32_t ticks = (duration_ms + BUZZER_SERVICE_TIMER_TICK_MS - 1U) /
                        BUZZER_SERVICE_TIMER_TICK_MS;

    if (ticks == 0U)
    {
        ticks = 1U;
    }
    if (ticks > 0xFFFFU)
    {
        ticks = 0xFFFFU;
    }

    return (rt_uint16_t)ticks;
}

static rt_uint16_t buzzer_service_alarm_interval_ticks(void)
{
    return buzzer_service_ms_to_ticks(BUZZER_SERVICE_ALARM_INTERVAL_MS);
}

static void buzzer_gpio_write(rt_bool_t on)
{
    gpio_bits_write(BUZZER_PORT, BUZZER_PIN, on ? TRUE : FALSE);
}

static void buzzer_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = BUZZER_PIN;
    gpio_init(BUZZER_PORT, &gpio_init_struct);

    buzzer_gpio_write(RT_FALSE);
}

static void buzzer_service_set_output_locked(rt_bool_t on)
{
    rt_uint8_t output = on ? 1U : 0U;

    if (buzzer_service_ctx.output_on == output)
    {
        return;
    }

    buzzer_service_ctx.output_on = output;
    buzzer_service_ctx.toggle_count++;
    buzzer_gpio_write(on);
}

static void buzzer_service_apply_auto_output_locked(void)
{
    if (buzzer_service_ctx.prompt_ticks_remaining != 0U)
    {
        buzzer_service_set_output_locked(RT_TRUE);
    }
    else if (buzzer_service_ctx.alarm_active != 0U)
    {
        buzzer_service_set_output_locked((buzzer_service_ctx.alarm_phase_on != 0U) ? RT_TRUE : RT_FALSE);
    }
    else
    {
        buzzer_service_set_output_locked(RT_FALSE);
    }
}

static void buzzer_service_timer_init(void)
{
    rt_uint32_t prescaler;
    rt_uint32_t period;

    crm_periph_clock_enable(BUZZER_SERVICE_TIMER_CLOCK, TRUE);
    tmr_reset(BUZZER_SERVICE_TIMER);

    prescaler = SystemCoreClock / BUZZER_SERVICE_TIMER_BASE_HZ;
    if (prescaler == 0U)
    {
        prescaler = 1U;
    }
    prescaler -= 1U;

    period = BUZZER_SERVICE_TIMER_BASE_HZ / BUZZER_SERVICE_TIMER_TICK_HZ;
    if (period == 0U)
    {
        period = 1U;
    }
    period -= 1U;

    tmr_base_init(BUZZER_SERVICE_TIMER, period, prescaler);
    tmr_cnt_dir_set(BUZZER_SERVICE_TIMER, TMR_COUNT_UP);
    tmr_period_buffer_enable(BUZZER_SERVICE_TIMER, FALSE);
    tmr_flag_clear(BUZZER_SERVICE_TIMER, TMR_OVF_FLAG);
    tmr_interrupt_enable(BUZZER_SERVICE_TIMER, TMR_OVF_INT, TRUE);
    nvic_irq_enable(BUZZER_SERVICE_TIMER_IRQn,
                    BUZZER_SERVICE_TIMER_IRQ_PREEMPTION,
                    BUZZER_SERVICE_TIMER_IRQ_SUB);
    tmr_counter_enable(BUZZER_SERVICE_TIMER, TRUE);
}

rt_err_t buzzer_service_init(void)
{
    if (buzzer_service_ctx.ready != 0U)
    {
        return RT_EOK;
    }

    rt_memset(&buzzer_service_ctx, 0, sizeof(buzzer_service_ctx));
    buzzer_service_ctx.mode = BUZZER_SERVICE_MODE_AUTO;
    buzzer_gpio_init();
    buzzer_service_timer_init();
    buzzer_service_ctx.ready = 1U;

    return RT_EOK;
}

void buzzer_service_get_status(buzzer_service_status_t *status)
{
    rt_base_t level;

    if (status == RT_NULL)
    {
        return;
    }

    level = rt_hw_interrupt_disable();
    status->ready = buzzer_service_ctx.ready;
    status->output_on = buzzer_service_ctx.output_on;
    status->alarm_active = buzzer_service_ctx.alarm_active;
    status->prompt_active = (buzzer_service_ctx.prompt_ticks_remaining != 0U) ? 1U : 0U;
    status->mode = buzzer_service_ctx.mode;
    status->toggle_count = buzzer_service_ctx.toggle_count;
    status->timer_ticks = buzzer_service_ctx.timer_ticks;
    status->prompt_remaining_ms =
        (rt_uint32_t)buzzer_service_ctx.prompt_ticks_remaining * BUZZER_SERVICE_TIMER_TICK_MS;
    rt_hw_interrupt_enable(level);
}

rt_err_t buzzer_service_set_mode(buzzer_service_mode_t mode)
{
    rt_base_t level;

    if (mode != BUZZER_SERVICE_MODE_AUTO &&
        mode != BUZZER_SERVICE_MODE_FORCE_OFF &&
        mode != BUZZER_SERVICE_MODE_FORCE_ON)
    {
        return -RT_EINVAL;
    }

    level = rt_hw_interrupt_disable();
    buzzer_service_ctx.mode = mode;
    if (mode == BUZZER_SERVICE_MODE_FORCE_OFF)
    {
        buzzer_service_set_output_locked(RT_FALSE);
    }
    else if (mode == BUZZER_SERVICE_MODE_FORCE_ON)
    {
        buzzer_service_set_output_locked(RT_TRUE);
    }
    else
    {
        buzzer_service_apply_auto_output_locked();
    }
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

void buzzer_service_set_alarm_active(rt_bool_t active)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (active != RT_FALSE)
    {
        if (buzzer_service_ctx.alarm_active == 0U)
        {
            buzzer_service_ctx.alarm_phase_on = 1U;
            buzzer_service_ctx.alarm_phase_ticks = buzzer_service_alarm_interval_ticks();
        }
        buzzer_service_ctx.alarm_active = 1U;
    }
    else
    {
        buzzer_service_ctx.alarm_active = 0U;
        buzzer_service_ctx.alarm_phase_on = 0U;
        buzzer_service_ctx.alarm_phase_ticks = 0U;
    }

    if (buzzer_service_ctx.mode == BUZZER_SERVICE_MODE_AUTO)
    {
        buzzer_service_apply_auto_output_locked();
    }
    rt_hw_interrupt_enable(level);
}

rt_err_t buzzer_service_beep(rt_uint32_t duration_ms)
{
    rt_base_t level;

    if (duration_ms == 0U)
    {
        duration_ms = BUZZER_SERVICE_PROMPT_DEFAULT_MS;
    }
    if (duration_ms > BUZZER_SERVICE_PROMPT_MAX_MS)
    {
        duration_ms = BUZZER_SERVICE_PROMPT_MAX_MS;
    }

    level = rt_hw_interrupt_disable();
    buzzer_service_ctx.prompt_ticks_remaining = buzzer_service_ms_to_ticks(duration_ms);
    if (buzzer_service_ctx.mode == BUZZER_SERVICE_MODE_AUTO ||
        buzzer_service_ctx.mode == BUZZER_SERVICE_MODE_FORCE_ON)
    {
        buzzer_service_set_output_locked(RT_TRUE);
    }
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

void buzzer_service_timer_isr(void)
{
    if (tmr_interrupt_flag_get(BUZZER_SERVICE_TIMER, TMR_OVF_FLAG) == RESET)
    {
        return;
    }

    tmr_flag_clear(BUZZER_SERVICE_TIMER, TMR_OVF_FLAG);
    buzzer_service_ctx.timer_ticks++;

    if (buzzer_service_ctx.ready == 0U)
    {
        return;
    }

    if (buzzer_service_ctx.mode == BUZZER_SERVICE_MODE_FORCE_ON)
    {
        buzzer_service_set_output_locked(RT_TRUE);
        return;
    }
    if (buzzer_service_ctx.mode == BUZZER_SERVICE_MODE_FORCE_OFF)
    {
        buzzer_service_set_output_locked(RT_FALSE);
        return;
    }

    if (buzzer_service_ctx.prompt_ticks_remaining != 0U)
    {
        buzzer_service_ctx.prompt_ticks_remaining--;
        buzzer_service_set_output_locked(RT_TRUE);
        return;
    }

    if (buzzer_service_ctx.alarm_active != 0U)
    {
        if (buzzer_service_ctx.alarm_phase_ticks != 0U)
        {
            buzzer_service_ctx.alarm_phase_ticks--;
        }
        if (buzzer_service_ctx.alarm_phase_ticks == 0U)
        {
            buzzer_service_ctx.alarm_phase_on =
                (buzzer_service_ctx.alarm_phase_on == 0U) ? 1U : 0U;
            buzzer_service_ctx.alarm_phase_ticks = buzzer_service_alarm_interval_ticks();
        }
        buzzer_service_set_output_locked((buzzer_service_ctx.alarm_phase_on != 0U) ? RT_TRUE : RT_FALSE);
    }
    else
    {
        buzzer_service_set_output_locked(RT_FALSE);
    }
}

static rt_err_t buzzer_service_parse_u32(const char *text, rt_uint32_t *value)
{
    rt_uint32_t result = 0U;

    if (text == RT_NULL || value == RT_NULL || *text == '\0')
    {
        return -RT_EINVAL;
    }

    while (*text != '\0')
    {
        if (*text < '0' || *text > '9')
        {
            return -RT_EINVAL;
        }
        result = (result * 10U) + (rt_uint32_t)(*text - '0');
        text++;
    }

    *value = result;
    return RT_EOK;
}

static int app_shell_buzzer(int argc, char **argv)
{
    buzzer_service_status_t status;

    if (argc >= 2)
    {
        if (rt_strcmp(argv[1], "auto") == 0)
        {
            (void)buzzer_service_set_mode(BUZZER_SERVICE_MODE_AUTO);
        }
        else if (rt_strcmp(argv[1], "off") == 0)
        {
            (void)buzzer_service_set_mode(BUZZER_SERVICE_MODE_FORCE_OFF);
        }
        else if (rt_strcmp(argv[1], "on") == 0)
        {
            (void)buzzer_service_set_mode(BUZZER_SERVICE_MODE_FORCE_ON);
        }
        else if (rt_strcmp(argv[1], "beep") == 0)
        {
            rt_uint32_t duration_ms = BUZZER_SERVICE_PROMPT_DEFAULT_MS;

            if (argc >= 3 &&
                buzzer_service_parse_u32(argv[2], &duration_ms) != RT_EOK)
            {
                rt_kprintf("[ERROR] buzzer: invalid beep duration\r\n");
                return -1;
            }
            (void)buzzer_service_beep(duration_ms);
        }
        else
        {
            rt_kprintf("[ERROR] buzzer: usage buzzer [auto|off|on|beep [ms]]\r\n");
            return -1;
        }
    }

    buzzer_service_get_status(&status);
    rt_kprintf("[INFO] buzzer: ready=%u mode=%u output=%u alarm=%u prompt=%u prompt_ms=%u toggle=%u tick=%u timer=TMR7 interval_ms=%u\r\n",
               status.ready,
               (rt_uint32_t)status.mode,
               status.output_on,
               status.alarm_active,
               status.prompt_active,
               status.prompt_remaining_ms,
               status.toggle_count,
               status.timer_ticks,
               (rt_uint32_t)BUZZER_SERVICE_ALARM_INTERVAL_MS);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(app_shell_buzzer, buzzer, show or control buzzer service);
