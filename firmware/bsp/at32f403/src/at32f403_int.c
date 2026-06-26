#include <rtthread.h>

#include "at32f403_int.h"
#include "board.h"
#include "at32f403a_407_usart.h"
#include "app_buzzer_service.h"
#include "freemodbus_port.h"

void NMI_Handler(void)
{
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

void USART1_IRQHandler(void)
{
    rt_interrupt_enter();

    if (usart_interrupt_flag_get(USART1, USART_RDBF_FLAG) != RESET)
    {
        board_debug_console_rx_isr((rt_uint8_t)(usart_data_receive(USART1) & 0xFFU));
    }

    rt_interrupt_leave();
}

void USART3_IRQHandler(void)
{
    rt_interrupt_enter();
    freemodbus_port_usart3_isr();
    rt_interrupt_leave();
}

void TMR7_GLOBAL_IRQHandler(void)
{
    rt_interrupt_enter();
    buzzer_service_timer_isr();
    rt_interrupt_leave();
}
