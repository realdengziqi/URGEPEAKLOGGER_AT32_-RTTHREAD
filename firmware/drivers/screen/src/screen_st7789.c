#include "screen_st7789.h"

#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_spi.h"

#define SCREEN_RES_PORT                  GPIOC
#define SCREEN_RES_PIN                   GPIO_PINS_5
#define SCREEN_BLK_PORT                  GPIOB
#define SCREEN_BLK_PIN                   GPIO_PINS_1
#define SCREEN_DC_PORT                   GPIOB
#define SCREEN_DC_PIN                    GPIO_PINS_2
#define SCREEN_SPI_PORT                  GPIOB
#define SCREEN_SPI_SCK_PIN               GPIO_PINS_13
#define SCREEN_SPI_MOSI_PIN              GPIO_PINS_15

#define SCREEN_PIXEL_COUNT               (SCREEN_ST7789_WIDTH * SCREEN_ST7789_HEIGHT)
#define SCREEN_FONT_WIDTH                5U
#define SCREEN_FONT_HEIGHT               7U
#define SCREEN_FONT_ADVANCE              6U

static rt_bool_t screen_bus_ready = RT_FALSE;

typedef struct
{
    char ch;
    rt_uint8_t column[SCREEN_FONT_WIDTH];
} screen_font5x7_glyph_t;

static const screen_font5x7_glyph_t screen_font5x7[] =
{
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'!', {0x00, 0x00, 0x5F, 0x00, 0x00}},
    {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
    {'/', {0x20, 0x10, 0x08, 0x04, 0x02}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
    {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
    {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
    {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
    {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
    {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
    {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
    {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
    {'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}},
    {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
    {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
    {'?', {0x02, 0x01, 0x51, 0x09, 0x06}},
};

static void screen_pin_write(gpio_type *port, rt_uint16_t pin, rt_bool_t high)
{
    gpio_bits_write(port, pin, high ? TRUE : FALSE);
}

static void screen_write_bus(rt_uint8_t data)
{
    while (spi_i2s_flag_get(SPI2, SPI_I2S_TDBE_FLAG) == RESET)
    {
    }

    spi_i2s_data_transmit(SPI2, data);

    while (spi_i2s_flag_get(SPI2, SPI_I2S_RDBF_FLAG) == RESET)
    {
    }

    (void)spi_i2s_data_receive(SPI2);
}

static void screen_write_command(rt_uint8_t command)
{
    screen_pin_write(SCREEN_DC_PORT, SCREEN_DC_PIN, RT_FALSE);
    screen_write_bus(command);
    screen_pin_write(SCREEN_DC_PORT, SCREEN_DC_PIN, RT_TRUE);
}

static void screen_write_data8(rt_uint8_t data)
{
    screen_write_bus(data);
}

static void screen_write_data16(rt_uint16_t data)
{
    screen_write_bus((rt_uint8_t)(data >> 8));
    screen_write_bus((rt_uint8_t)data);
}

static void screen_write_pixels(rt_uint16_t color, rt_uint32_t count)
{
    rt_uint32_t i;

    for (i = 0; i < count; i++)
    {
        screen_write_data16(color);
    }
}

static void screen_set_address(rt_uint16_t x1, rt_uint16_t y1, rt_uint16_t x2, rt_uint16_t y2)
{
    screen_write_command(0x2A);
    screen_write_data16(x1);
    screen_write_data16(x2);

    screen_write_command(0x2B);
    screen_write_data16(y1);
    screen_write_data16(y2);

    screen_write_command(0x2C);
}

static const rt_uint8_t *screen_find_glyph(char ch)
{
    rt_size_t i;
    char key;

    if (ch >= 'a' && ch <= 'z')
    {
        key = (char)(ch - ('a' - 'A'));
    }
    else
    {
        key = ch;
    }

    for (i = 0; i < (sizeof(screen_font5x7) / sizeof(screen_font5x7[0])); i++)
    {
        if (screen_font5x7[i].ch == key)
        {
            return screen_font5x7[i].column;
        }
    }

    return screen_font5x7[sizeof(screen_font5x7) / sizeof(screen_font5x7[0]) - 1U].column;
}

void screen_st7789_gpio_init(void)
{
    gpio_init_type gpio_init_struct;
    spi_init_type spi_init_struct;

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_SPI2_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;

    gpio_init_struct.gpio_pins = SCREEN_BLK_PIN | SCREEN_DC_PIN;
    gpio_init(GPIOB, &gpio_init_struct);

    gpio_init_struct.gpio_pins = SCREEN_RES_PIN;
    gpio_init(GPIOC, &gpio_init_struct);

    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = SCREEN_SPI_SCK_PIN | SCREEN_SPI_MOSI_PIN;
    gpio_init(SCREEN_SPI_PORT, &gpio_init_struct);

    screen_pin_write(SCREEN_DC_PORT, SCREEN_DC_PIN, RT_TRUE);
    screen_pin_write(SCREEN_RES_PORT, SCREEN_RES_PIN, RT_TRUE);
    screen_st7789_backlight_set(RT_FALSE);

    spi_i2s_reset(SPI2);
    spi_default_para_init(&spi_init_struct);
    spi_init_struct.transmission_mode = SPI_TRANSMIT_FULL_DUPLEX;
    spi_init_struct.master_slave_mode = SPI_MODE_MASTER;
    spi_init_struct.mclk_freq_division = SPI_MCLK_DIV_8;
    spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_MSB;
    spi_init_struct.frame_bit_num = SPI_FRAME_8BIT;
    spi_init_struct.clock_polarity = SPI_CLOCK_POLARITY_HIGH;
    spi_init_struct.clock_phase = SPI_CLOCK_PHASE_2EDGE;
    spi_init_struct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;
    spi_init(SPI2, &spi_init_struct);
    spi_enable(SPI2, TRUE);

    screen_bus_ready = RT_TRUE;
}

void screen_st7789_bus_swap_set(rt_bool_t swapped)
{
    RT_UNUSED(swapped);
}

rt_bool_t screen_st7789_bus_swap_get(void)
{
    return RT_FALSE;
}

void screen_st7789_backlight_set(rt_bool_t on)
{
    screen_pin_write(SCREEN_BLK_PORT, SCREEN_BLK_PIN, on ? RT_TRUE : RT_FALSE);
}

void screen_st7789_reset(void)
{
    if (screen_bus_ready == RT_FALSE)
    {
        screen_st7789_gpio_init();
    }

    screen_pin_write(SCREEN_RES_PORT, SCREEN_RES_PIN, RT_TRUE);
    rt_thread_mdelay(50);
    screen_pin_write(SCREEN_RES_PORT, SCREEN_RES_PIN, RT_FALSE);
    rt_thread_mdelay(50);
    screen_pin_write(SCREEN_RES_PORT, SCREEN_RES_PIN, RT_TRUE);
    rt_thread_mdelay(120);
}

void screen_st7789_init(void)
{
    if (screen_bus_ready == RT_FALSE)
    {
        screen_st7789_gpio_init();
    }

    screen_st7789_reset();

    screen_write_command(0x11);
    rt_thread_mdelay(120);

    screen_write_command(0x36);
    screen_write_data8(0x00);

    screen_write_command(0x3A);
    screen_write_data8(0x05);

    screen_write_command(0xB2);
    screen_write_data8(0x1F);
    screen_write_data8(0x1F);
    screen_write_data8(0x00);
    screen_write_data8(0x33);
    screen_write_data8(0x33);

    screen_write_command(0xB7);
    screen_write_data8(0x00);

    screen_write_command(0xBB);
    screen_write_data8(0x3F);

    screen_write_command(0xC0);
    screen_write_data8(0x2C);

    screen_write_command(0xC2);
    screen_write_data8(0x01);

    screen_write_command(0xC3);
    screen_write_data8(0x0F);

    screen_write_command(0xC4);
    screen_write_data8(0x20);

    screen_write_command(0xC6);
    screen_write_data8(0x13);

    screen_write_command(0xD0);
    screen_write_data8(0xA4);
    screen_write_data8(0xA1);

    screen_write_command(0xD6);
    screen_write_data8(0xA1);

    screen_write_command(0xE0);
    screen_write_data8(0xF0);
    screen_write_data8(0x06);
    screen_write_data8(0x0D);
    screen_write_data8(0x0B);
    screen_write_data8(0x0A);
    screen_write_data8(0x07);
    screen_write_data8(0x2E);
    screen_write_data8(0x43);
    screen_write_data8(0x45);
    screen_write_data8(0x38);
    screen_write_data8(0x14);
    screen_write_data8(0x13);
    screen_write_data8(0x25);
    screen_write_data8(0x29);

    screen_write_command(0xE1);
    screen_write_data8(0xF0);
    screen_write_data8(0x07);
    screen_write_data8(0x0A);
    screen_write_data8(0x08);
    screen_write_data8(0x07);
    screen_write_data8(0x23);
    screen_write_data8(0x2E);
    screen_write_data8(0x33);
    screen_write_data8(0x44);
    screen_write_data8(0x3A);
    screen_write_data8(0x16);
    screen_write_data8(0x17);
    screen_write_data8(0x26);
    screen_write_data8(0x2C);

    screen_write_command(0xE4);
    screen_write_data8(0x1D);
    screen_write_data8(0x00);
    screen_write_data8(0x00);

    screen_write_command(0x21);
    screen_write_command(0x11);
    rt_thread_mdelay(120);
    screen_write_command(0x29);

    screen_st7789_backlight_set(RT_TRUE);
}

void screen_st7789_fill_color(rt_uint16_t color)
{
    if (screen_bus_ready == RT_FALSE)
    {
        screen_st7789_gpio_init();
    }

    screen_set_address(0, 0, SCREEN_ST7789_WIDTH - 1U, SCREEN_ST7789_HEIGHT - 1U);
    screen_write_pixels(color, SCREEN_PIXEL_COUNT);

    while (spi_i2s_flag_get(SPI2, SPI_I2S_BF_FLAG) != RESET)
    {
    }
}

void screen_st7789_draw_pixel(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color)
{
    screen_st7789_fill_rect(x, y, 1U, 1U, color);
}

void screen_st7789_fill_rect(rt_uint16_t x, rt_uint16_t y, rt_uint16_t width, rt_uint16_t height, rt_uint16_t color)
{
    rt_uint16_t clipped_width;
    rt_uint16_t clipped_height;
    rt_uint32_t pixel_count;

    if (screen_bus_ready == RT_FALSE)
    {
        screen_st7789_gpio_init();
    }

    if ((width == 0U) || (height == 0U) || (x >= SCREEN_ST7789_WIDTH) || (y >= SCREEN_ST7789_HEIGHT))
    {
        return;
    }

    clipped_width = width;
    clipped_height = height;

    if ((x + clipped_width) > SCREEN_ST7789_WIDTH)
    {
        clipped_width = SCREEN_ST7789_WIDTH - x;
    }

    if ((y + clipped_height) > SCREEN_ST7789_HEIGHT)
    {
        clipped_height = SCREEN_ST7789_HEIGHT - y;
    }

    pixel_count = (rt_uint32_t)clipped_width * (rt_uint32_t)clipped_height;
    screen_set_address(x, y, x + clipped_width - 1U, y + clipped_height - 1U);
    screen_write_pixels(color, pixel_count);

    while (spi_i2s_flag_get(SPI2, SPI_I2S_BF_FLAG) != RESET)
    {
    }
}

void screen_st7789_draw_char(rt_uint16_t x, rt_uint16_t y, char ch, rt_uint16_t fg_color, rt_uint16_t bg_color, rt_uint8_t scale)
{
    const rt_uint8_t *glyph;
    rt_uint8_t col;
    rt_uint8_t row;
    rt_uint8_t sx;
    rt_uint8_t sy;
    rt_uint8_t actual_scale;
    rt_uint16_t char_width;
    rt_uint16_t char_height;
    rt_uint16_t draw_color;

    actual_scale = (scale == 0U) ? 1U : scale;
    glyph = screen_find_glyph(ch);
    char_width = (rt_uint16_t)(SCREEN_FONT_ADVANCE * actual_scale);
    char_height = (rt_uint16_t)(SCREEN_FONT_HEIGHT * actual_scale);

    if ((x + char_width) <= SCREEN_ST7789_WIDTH && (y + char_height) <= SCREEN_ST7789_HEIGHT)
    {
        screen_set_address(x, y, x + char_width - 1U, y + char_height - 1U);

        for (row = 0U; row < SCREEN_FONT_HEIGHT; row++)
        {
            for (sy = 0U; sy < actual_scale; sy++)
            {
                for (col = 0U; col < SCREEN_FONT_ADVANCE; col++)
                {
                    if ((col < SCREEN_FONT_WIDTH) && ((glyph[col] & (1U << row)) != 0U))
                    {
                        draw_color = fg_color;
                    }
                    else
                    {
                        draw_color = bg_color;
                    }

                    for (sx = 0U; sx < actual_scale; sx++)
                    {
                        screen_write_data16(draw_color);
                    }
                }
            }
        }

        while (spi_i2s_flag_get(SPI2, SPI_I2S_BF_FLAG) != RESET)
        {
        }

        return;
    }

    for (col = 0U; col < SCREEN_FONT_ADVANCE; col++)
    {
        for (row = 0U; row < SCREEN_FONT_HEIGHT; row++)
        {
            if ((col < SCREEN_FONT_WIDTH) && ((glyph[col] & (1U << row)) != 0U))
            {
                draw_color = fg_color;
            }
            else
            {
                draw_color = bg_color;
            }

            screen_st7789_fill_rect((rt_uint16_t)(x + (rt_uint16_t)col * actual_scale),
                                    (rt_uint16_t)(y + (rt_uint16_t)row * actual_scale),
                                    actual_scale,
                                    actual_scale,
                                    draw_color);
        }
    }
}

void screen_st7789_draw_string(rt_uint16_t x, rt_uint16_t y, const char *text, rt_uint16_t fg_color, rt_uint16_t bg_color, rt_uint8_t scale)
{
    rt_uint16_t cursor_x;
    rt_uint16_t cursor_y;
    rt_uint16_t start_x;
    rt_uint8_t actual_scale;

    if (text == RT_NULL)
    {
        return;
    }

    actual_scale = (scale == 0U) ? 1U : scale;
    cursor_x = x;
    cursor_y = y;
    start_x = x;

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            cursor_x = start_x;
            cursor_y = (rt_uint16_t)(cursor_y + (SCREEN_FONT_HEIGHT + 1U) * actual_scale);
        }
        else
        {
            screen_st7789_draw_char(cursor_x, cursor_y, *text, fg_color, bg_color, actual_scale);
            cursor_x = (rt_uint16_t)(cursor_x + SCREEN_FONT_ADVANCE * actual_scale);
        }

        text++;

        if (cursor_y >= SCREEN_ST7789_HEIGHT)
        {
            break;
        }
    }
}

void screen_st7789_draw_demo(void)
{
    screen_st7789_fill_color(SCREEN_ST7789_COLOR_BLACK);

    screen_st7789_fill_rect(0, 0, 40, 40, SCREEN_ST7789_COLOR_RED);
    screen_st7789_fill_rect(40, 0, 40, 40, SCREEN_ST7789_COLOR_GREEN);
    screen_st7789_fill_rect(80, 0, 40, 40, SCREEN_ST7789_COLOR_BLUE);
    screen_st7789_fill_rect(120, 0, 40, 40, SCREEN_ST7789_COLOR_YELLOW);
    screen_st7789_fill_rect(160, 0, 40, 40, SCREEN_ST7789_COLOR_CYAN);
    screen_st7789_fill_rect(200, 0, 40, 40, SCREEN_ST7789_COLOR_MAGENTA);

    screen_st7789_fill_rect(10, 62, 220, 54, 0x18E3U);
    screen_st7789_fill_rect(14, 66, 212, 46, SCREEN_ST7789_COLOR_BLACK);
    screen_st7789_draw_string(24, 76, "SURGE RT", SCREEN_ST7789_COLOR_CYAN, SCREEN_ST7789_COLOR_BLACK, 3);

    screen_st7789_draw_string(18, 136, "SPI2 OK", SCREEN_ST7789_COLOR_GREEN, SCREEN_ST7789_COLOR_BLACK, 2);
    screen_st7789_draw_string(18, 158, "ST7789 240X240", SCREEN_ST7789_COLOR_WHITE, SCREEN_ST7789_COLOR_BLACK, 2);
    screen_st7789_draw_string(18, 180, "DRAW RECT TEXT", SCREEN_ST7789_COLOR_YELLOW, SCREEN_ST7789_COLOR_BLACK, 2);

    screen_st7789_fill_rect(18, 214, 204, 8, SCREEN_ST7789_COLOR_WHITE);
    screen_st7789_fill_rect(22, 216, 196, 4, SCREEN_ST7789_COLOR_RED);
}
