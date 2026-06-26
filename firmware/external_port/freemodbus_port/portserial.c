#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_misc.h"
#include "at32f403a_407_usart.h"
#include "port.h"

#include "mb.h"
#include "mbport.h"

#define MB_USART                         USART3
#define MB_USART_IRQn                    USART3_IRQn
#define MB_USART_BAUD_FALLBACK           9600U
#define MB_RX_BUFFER_SIZE                128U

#define MB_RS485_DIR_PORT                GPIOA
#define MB_RS485_DIR_PIN                 GPIO_PINS_15

static volatile rt_uint8_t rx_buffer[MB_RX_BUFFER_SIZE];
static volatile rt_uint16_t rx_head;
static volatile rt_uint16_t rx_tail;
static volatile BOOL tx_enabled;
static BOOL serial_ready;
static usart_stop_bit_num_type serial_stop_bits = USART_STOP_1_BIT;

void freemodbus_port_set_stop_bits(UCHAR stop_bits)
{
    serial_stop_bits = (stop_bits == 2U) ? USART_STOP_2_BIT : USART_STOP_1_BIT;
}

static void rs485_set_tx(BOOL enable)
{
    if (enable != FALSE)
    {
        gpio_bits_set(MB_RS485_DIR_PORT, MB_RS485_DIR_PIN);
    }
    else
    {
        gpio_bits_reset(MB_RS485_DIR_PORT, MB_RS485_DIR_PIN);
    }
}

static void rx_push(rt_uint8_t byte)
{
    rt_uint16_t next = (rt_uint16_t)((rx_head + 1U) % MB_RX_BUFFER_SIZE);

    if (next != rx_tail)
    {
        rx_buffer[rx_head] = byte;
        rx_head = next;
    }
}

static BOOL rx_pop(CHAR *byte)
{
    rt_base_t level;

    if (byte == RT_NULL)
    {
        return FALSE;
    }

    level = rt_hw_interrupt_disable();
    if (rx_head == rx_tail)
    {
        rt_hw_interrupt_enable(level);
        return FALSE;
    }

    *byte = (CHAR)rx_buffer[rx_tail];
    rx_tail = (rt_uint16_t)((rx_tail + 1U) % MB_RX_BUFFER_SIZE);
    rt_hw_interrupt_enable(level);

    return TRUE;
}

static void serial_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_USART3_PERIPH_CLOCK, TRUE);

    gpio_pin_remap_config(SWJTAG_MUX_010, TRUE);
    gpio_pin_remap_config(USART3_MUX_01, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = GPIO_PINS_10;
    gpio_init(GPIOC, &gpio_init_struct);

    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pins = GPIO_PINS_11;
    gpio_init(GPIOC, &gpio_init_struct);

    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = MB_RS485_DIR_PIN;
    gpio_init(MB_RS485_DIR_PORT, &gpio_init_struct);
    rs485_set_tx(FALSE);
}

BOOL xMBPortSerialInit(UCHAR port, ULONG baud_rate, UCHAR data_bits, eMBParity parity)
{
    usart_data_bit_num_type usart_data_bits;

    RT_UNUSED(port);

    serial_gpio_init();

    if (baud_rate == 0U)
    {
        baud_rate = MB_USART_BAUD_FALLBACK;
    }

    /* AT32 USART data_bit includes parity bit when parity is enabled. */
    usart_data_bits = (data_bits == 9U || parity != MB_PAR_NONE) ?
                      USART_DATA_9BITS :
                      USART_DATA_8BITS;

    usart_reset(MB_USART);
    usart_init(MB_USART, baud_rate, usart_data_bits, serial_stop_bits);

    switch (parity)
    {
    case MB_PAR_ODD:
        usart_parity_selection_config(MB_USART, USART_PARITY_ODD);
        break;
    case MB_PAR_EVEN:
        usart_parity_selection_config(MB_USART, USART_PARITY_EVEN);
        break;
    case MB_PAR_NONE:
    default:
        usart_parity_selection_config(MB_USART, USART_PARITY_NONE);
        break;
    }

    usart_transmitter_enable(MB_USART, TRUE);
    usart_receiver_enable(MB_USART, TRUE);
    usart_enable(MB_USART, TRUE);
    usart_interrupt_enable(MB_USART, USART_RDBF_INT, FALSE);
    usart_interrupt_enable(MB_USART, USART_TDBE_INT, FALSE);

    rx_head = 0U;
    rx_tail = 0U;
    tx_enabled = FALSE;
    serial_ready = TRUE;

    nvic_irq_enable(MB_USART_IRQn, 4, 0);
    return TRUE;
}

void vMBPortSerialEnable(BOOL rx_enable, BOOL tx_enable)
{
    if (serial_ready == FALSE)
    {
        return;
    }

    if (tx_enable != FALSE)
    {
        tx_enabled = TRUE;
        rs485_set_tx(TRUE);
        usart_interrupt_enable(MB_USART, USART_RDBF_INT, FALSE);
        usart_interrupt_enable(MB_USART, USART_TDBE_INT, TRUE);
    }
    else
    {
        usart_interrupt_enable(MB_USART, USART_TDBE_INT, FALSE);
        if (tx_enabled != FALSE)
        {
            while (usart_flag_get(MB_USART, USART_TDC_FLAG) == RESET)
            {
            }
        }
        tx_enabled = FALSE;
        rs485_set_tx(FALSE);
    }

    usart_interrupt_enable(MB_USART, USART_RDBF_INT, (rx_enable != FALSE) ? TRUE : FALSE);
}

void vMBPortClose(void)
{
    usart_interrupt_enable(MB_USART, USART_RDBF_INT, FALSE);
    usart_interrupt_enable(MB_USART, USART_TDBE_INT, FALSE);
    usart_enable(MB_USART, FALSE);
    serial_ready = FALSE;
    rs485_set_tx(FALSE);
}

void xMBPortSerialClose(void)
{
    vMBPortClose();
}

BOOL xMBPortSerialPutByte(CHAR byte)
{
    usart_data_transmit(MB_USART, (uint16_t)byte);
    return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR *byte)
{
    return rx_pop(byte);
}

void freemodbus_port_usart3_isr(void)
{
    if (serial_ready == FALSE)
    {
        return;
    }

    if (usart_interrupt_flag_get(MB_USART, USART_RDBF_FLAG) != RESET)
    {
        rx_push((rt_uint8_t)(usart_data_receive(MB_USART) & 0xFFU));
        (void)pxMBFrameCBByteReceived();
    }

    if (usart_interrupt_flag_get(MB_USART, USART_TDBE_FLAG) != RESET)
    {
        if (tx_enabled != FALSE)
        {
            (void)pxMBFrameCBTransmitterEmpty();
        }
        else
        {
            usart_interrupt_enable(MB_USART, USART_TDBE_INT, FALSE);
        }
    }
}
