#include "ugui_port.h"

#include "screen_st7789.h"

static UG_GUI ugui_port_gui;
static rt_bool_t ugui_port_ready = RT_FALSE;

static void ugui_port_set_pixel(UG_S16 x, UG_S16 y, UG_COLOR color)
{
    if ((x < 0) || (y < 0))
    {
        return;
    }

    if (((rt_uint16_t)x >= SCREEN_ST7789_WIDTH) || ((rt_uint16_t)y >= SCREEN_ST7789_HEIGHT))
    {
        return;
    }

    screen_st7789_draw_pixel((rt_uint16_t)x, (rt_uint16_t)y, (rt_uint16_t)color);
}

void ugui_port_init(void)
{
    screen_st7789_init();
    UG_Init(&ugui_port_gui, ugui_port_set_pixel, SCREEN_ST7789_WIDTH, SCREEN_ST7789_HEIGHT);
#ifdef USE_FONT_12X20
    UG_FontSelect(&FONT_12X20);
#elif defined(USE_FONT_10X16)
    UG_FontSelect(&FONT_10X16);
#endif
    ugui_port_ready = RT_TRUE;
}

void ugui_port_clear(UG_COLOR color)
{
    if (ugui_port_ready == RT_FALSE)
    {
        ugui_port_init();
    }

    screen_st7789_fill_color((rt_uint16_t)color);
}

rt_bool_t ugui_port_is_ready(void)
{
    return ugui_port_ready;
}
