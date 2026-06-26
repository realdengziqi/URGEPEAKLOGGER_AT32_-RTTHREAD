#include "port.h"

static rt_base_t critical_level;

void EnterCriticalSection(void)
{
    critical_level = rt_hw_interrupt_disable();
}

void ExitCriticalSection(void)
{
    rt_hw_interrupt_enable(critical_level);
}
