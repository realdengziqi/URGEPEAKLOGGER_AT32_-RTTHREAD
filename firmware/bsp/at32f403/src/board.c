#include <rtthread.h>

#include "board.h"
#include "at32f403a_407_clock.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_crm.h"
#include "at32f403a_407_misc.h"
#include "at32f403a_407_usart.h"

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN                      (&Image$$RW_IRAM1$$ZI$$Limit)
#elif defined(__GNUC__)
extern int _end;
#define HEAP_BEGIN                      (&_end)
#else
#define HEAP_BEGIN                      RT_NULL
#endif

#define DEBUG_USART1_RX_BUFFER_SIZE      128U

static rt_uint8_t debug_usart1_rx_buffer[DEBUG_USART1_RX_BUFFER_SIZE];
static volatile rt_uint16_t debug_usart1_rx_head = 0;
static volatile rt_uint16_t debug_usart1_rx_tail = 0;

static void debug_usart1_rx_push(rt_uint8_t data)
{
    rt_uint16_t next_head = (rt_uint16_t)((debug_usart1_rx_head + 1U) % DEBUG_USART1_RX_BUFFER_SIZE);

    if (next_head != debug_usart1_rx_tail)
    {
        debug_usart1_rx_buffer[debug_usart1_rx_head] = data;
        debug_usart1_rx_head = next_head;
    }
}

void board_debug_console_rx_isr(rt_uint8_t data)
{
    debug_usart1_rx_push(data);
}

static void debug_usart1_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

    gpio_init_struct.gpio_pins = GPIO_PINS_9;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init(GPIOA, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_10;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init(GPIOA, &gpio_init_struct);

    usart_init(USART1, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_parity_selection_config(USART1, USART_PARITY_NONE);
    usart_transmitter_enable(USART1, TRUE);
    usart_receiver_enable(USART1, TRUE);
    usart_interrupt_enable(USART1, USART_RDBF_INT, TRUE);
    usart_enable(USART1, TRUE);

    nvic_irq_enable(USART1_IRQn, 3, 0);
}

static void interaction_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;

    gpio_init_struct.gpio_pins = GPIO_PINS_0 | GPIO_PINS_1;
    gpio_init(GPIOA, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_0;
    gpio_init(GPIOB, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_2 | GPIO_PINS_3;
    gpio_init(GPIOC, &gpio_init_struct);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;

    gpio_init_struct.gpio_pins = GPIO_PINS_2;
    gpio_init(GPIOA, &gpio_init_struct);
    gpio_bits_set(GPIOA, GPIO_PINS_2);

    gpio_init_struct.gpio_pins = GPIO_PINS_4;
    gpio_init(GPIOC, &gpio_init_struct);
    gpio_bits_set(GPIOC, GPIO_PINS_4);
}

void rt_hw_console_output(const char *str)
{
    while (*str)
    {
        if (*str == '\n')
        {
            while (usart_flag_get(USART1, USART_TDBE_FLAG) == RESET)
            {
            }
            usart_data_transmit(USART1, '\r');
        }

        while (usart_flag_get(USART1, USART_TDBE_FLAG) == RESET)
        {
        }
        usart_data_transmit(USART1, (uint16_t)*str++);
    }
}

int board_debug_console_getchar(void)
{
    int ch;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (debug_usart1_rx_tail == debug_usart1_rx_head)
    {
        rt_hw_interrupt_enable(level);
        return -1;
    }

    ch = debug_usart1_rx_buffer[debug_usart1_rx_tail];
    debug_usart1_rx_tail = (rt_uint16_t)((debug_usart1_rx_tail + 1U) % DEBUG_USART1_RX_BUFFER_SIZE);
    rt_hw_interrupt_enable(level);

    return ch;
}

signed char rt_hw_console_getchar(void)
{
    int ch = board_debug_console_getchar();

    if (ch < 0)
    {
        return -1;
    }

    return (signed char)ch;
}

rt_uint32_t board_interaction_key_read(void)
{
    rt_uint32_t key_state = 0;

    if (gpio_input_data_bit_read(GPIOC, GPIO_PINS_2) == RESET)
    {
        key_state |= BOARD_KEY_DOWN_MASK;
    }
    if (gpio_input_data_bit_read(GPIOC, GPIO_PINS_3) == RESET)
    {
        key_state |= BOARD_KEY_LEFT_MASK;
    }
    if (gpio_input_data_bit_read(GPIOA, GPIO_PINS_0) == RESET)
    {
        key_state |= BOARD_KEY_RIGHT_MASK;
    }
    if (gpio_input_data_bit_read(GPIOA, GPIO_PINS_1) == RESET)
    {
        key_state |= BOARD_KEY_UP_MASK;
    }
    if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_0) == RESET)
    {
        key_state |= BOARD_KEY_MID_MASK;
    }

    return key_state;
}

void board_debug_led_set(board_debug_led_t led, rt_bool_t on)
{
    confirm_state pin_level = on ? FALSE : TRUE;

    if (led == BOARD_DEBUG_LED_DATA)
    {
        gpio_bits_write(GPIOA, GPIO_PINS_2, pin_level);
    }
    else if (led == BOARD_DEBUG_LED_TEST)
    {
        gpio_bits_write(GPIOC, GPIO_PINS_4, pin_level);
    }
}

void board_debug_led_toggle(board_debug_led_t led)
{
    if (led == BOARD_DEBUG_LED_DATA)
    {
        board_debug_led_set(led, gpio_output_data_bit_read(GPIOA, GPIO_PINS_2) != RESET);
    }
    else if (led == BOARD_DEBUG_LED_TEST)
    {
        board_debug_led_set(led, gpio_output_data_bit_read(GPIOC, GPIO_PINS_4) != RESET);
    }
}

void rt_hw_board_init(void)
{
    system_clock_config();

    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

    debug_usart1_init();
    interaction_gpio_init();

#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)HEAP_BEGIN, (void *)AT32_SRAM_END);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
