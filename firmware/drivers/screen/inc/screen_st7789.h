#ifndef SCREEN_ST7789_H
#define SCREEN_ST7789_H

#include <rtthread.h>

#define SCREEN_ST7789_WIDTH              240U
#define SCREEN_ST7789_HEIGHT             240U

#define SCREEN_ST7789_COLOR_BLACK        0x0000U
#define SCREEN_ST7789_COLOR_BLUE         0x001FU
#define SCREEN_ST7789_COLOR_GREEN        0x07E0U
#define SCREEN_ST7789_COLOR_RED          0xF800U
#define SCREEN_ST7789_COLOR_WHITE        0xFFFFU
#define SCREEN_ST7789_COLOR_YELLOW       0xFFE0U
#define SCREEN_ST7789_COLOR_CYAN         0x07FFU
#define SCREEN_ST7789_COLOR_MAGENTA      0xF81FU

void screen_st7789_gpio_init(void);
void screen_st7789_bus_swap_set(rt_bool_t swapped);
rt_bool_t screen_st7789_bus_swap_get(void);
void screen_st7789_backlight_set(rt_bool_t on);
void screen_st7789_reset(void);
void screen_st7789_init(void);
void screen_st7789_fill_color(rt_uint16_t color);
void screen_st7789_draw_pixel(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color);
void screen_st7789_fill_rect(rt_uint16_t x, rt_uint16_t y, rt_uint16_t width, rt_uint16_t height, rt_uint16_t color);
void screen_st7789_draw_char(rt_uint16_t x, rt_uint16_t y, char ch, rt_uint16_t fg_color, rt_uint16_t bg_color, rt_uint8_t scale);
void screen_st7789_draw_string(rt_uint16_t x, rt_uint16_t y, const char *text, rt_uint16_t fg_color, rt_uint16_t bg_color, rt_uint8_t scale);
void screen_st7789_draw_demo(void);

#endif
