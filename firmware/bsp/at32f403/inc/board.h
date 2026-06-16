#ifndef BOARD_H
#define BOARD_H

#include <rtthread.h>
#include "at32f403a_407.h"

/*
 * AT32F403ARGT7 RAM size is set for initial RT-Thread heap bring-up.
 * Confirm against the exact MCU datasheet before enabling complex components.
 */
#ifndef AT32_SRAM_SIZE
#define AT32_SRAM_SIZE                  (96U * 1024U)
#endif

#define AT32_SRAM_BASE                  (0x20000000U)
#define AT32_SRAM_END                   (AT32_SRAM_BASE + AT32_SRAM_SIZE)

#define BOARD_KEY_DOWN_MASK             (1U << 0)
#define BOARD_KEY_LEFT_MASK             (1U << 1)
#define BOARD_KEY_RIGHT_MASK            (1U << 2)
#define BOARD_KEY_UP_MASK               (1U << 3)
#define BOARD_KEY_MID_MASK              (1U << 4)

typedef enum
{
    BOARD_DEBUG_LED_DATA = 0,
    BOARD_DEBUG_LED_TEST = 1,
} board_debug_led_t;

void rt_hw_board_init(void);
int board_debug_console_getchar(void);
void board_debug_console_rx_isr(rt_uint8_t data);
rt_uint32_t board_interaction_key_read(void);
void board_debug_led_set(board_debug_led_t led, rt_bool_t on);
void board_debug_led_toggle(board_debug_led_t led);

#endif
