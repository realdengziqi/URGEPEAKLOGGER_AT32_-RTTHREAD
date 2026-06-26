#ifndef AT32F403_INT_H
#define AT32F403_INT_H

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void SysTick_Handler(void);
void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
void TMR7_GLOBAL_IRQHandler(void);

#endif
