#include "port.h"
#include "mb.h"
#include "mbport.h"

static struct rt_timer slave_timer;
static BOOL slave_timer_ready;

static void timer_timeout(void *parameter)
{
    RT_UNUSED(parameter);
    (void)pxMBPortCBTimerExpired();
}

BOOL xMBPortTimersInit(USHORT timeout_50us)
{
    rt_tick_t ticks;

    ticks = (rt_tick_t)(((rt_uint32_t)timeout_50us * 50U * RT_TICK_PER_SECOND + 999999U) / 1000000U);
    if (ticks == 0U)
    {
        ticks = 1U;
    }

    if (slave_timer_ready != FALSE)
    {
        rt_timer_detach(&slave_timer);
        slave_timer_ready = FALSE;
    }

    rt_timer_init(&slave_timer, "mb_tmr", timer_timeout, RT_NULL, ticks, RT_TIMER_FLAG_ONE_SHOT);
    slave_timer_ready = TRUE;
    return TRUE;
}

void vMBPortTimersEnable(void)
{
    rt_timer_start(&slave_timer);
}

void vMBPortTimersDisable(void)
{
    rt_timer_stop(&slave_timer);
}

void xMBPortTimersClose(void)
{
    if (slave_timer_ready != FALSE)
    {
        rt_timer_detach(&slave_timer);
        slave_timer_ready = FALSE;
    }
}
