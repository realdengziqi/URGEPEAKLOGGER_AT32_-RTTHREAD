#include "surge_ui_widgets.h"

#include "surge_ui_font_zh_18.h"
#include "surge_ui_font_zh_20.h"
#include "surge_ui_font_zh_24.h"
#include "ugui_port.h"

typedef struct
{
    rt_uint8_t width;
    rt_uint8_t height;
    rt_uint8_t bytes_per_row;
    rt_uint8_t bits_per_pixel;
    const surge_ui_zh_glyph_t *glyphs;
    rt_uint16_t glyph_count;
} surge_ui_zh_font_t;

static const surge_ui_zh_font_t surge_ui_zh_font_18 =
{
    SURGE_UI_ZH_18_FONT_W,
    SURGE_UI_ZH_18_FONT_H,
    SURGE_UI_ZH_18_FONT_BYTES_PER_ROW,
    SURGE_UI_ZH_18_FONT_BITS_PER_PIXEL,
    surge_ui_zh_18_glyphs,
    SURGE_UI_ZH_18_GLYPH_COUNT
};

static const surge_ui_zh_font_t surge_ui_zh_font_20 =
{
    SURGE_UI_ZH_20_FONT_W,
    SURGE_UI_ZH_20_FONT_H,
    SURGE_UI_ZH_20_FONT_BYTES_PER_ROW,
    SURGE_UI_ZH_20_FONT_BITS_PER_PIXEL,
    surge_ui_zh_20_glyphs,
    SURGE_UI_ZH_20_GLYPH_COUNT
};

static const surge_ui_zh_font_t surge_ui_zh_font_24 =
{
    SURGE_UI_ZH_24_FONT_W,
    SURGE_UI_ZH_24_FONT_H,
    SURGE_UI_ZH_24_FONT_BYTES_PER_ROW,
    SURGE_UI_ZH_24_FONT_BITS_PER_PIXEL,
    surge_ui_zh_24_glyphs,
    SURGE_UI_ZH_24_GLYPH_COUNT
};

static const surge_ui_text_style_t surge_ui_default_text_8 =
{
    SURGE_UI_ZH_SIZE_18,
    &FONT_8X12
};

#if SURGE_UI_USE_HEADER
static const surge_ui_header_style_t surge_ui_default_header_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_12X20 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    8,
    7,
    182,
    10
};
#endif

#if SURGE_UI_USE_NAV_HEADER
static const surge_ui_nav_header_style_t surge_ui_default_nav_header_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_12X20 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    8,
    10,
    48,
    7,
    182,
    10
};
#endif

#if SURGE_UI_USE_FOOTER
static const surge_ui_footer_style_t surge_ui_default_footer_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    8,
    226
};
#endif

#if SURGE_UI_USE_SECTION_TITLE
static const surge_ui_section_title_style_t surge_ui_default_section_title_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    22,
    4,
    14,
    4,
    12,
    4,
    8,
    13
};
#endif

#if SURGE_UI_USE_EMPTY_STATE
static const surge_ui_empty_state_style_t surge_ui_default_empty_state_style =
{
    { SURGE_UI_ZH_SIZE_20, &FONT_12X20 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    96,
    24,
    5,
    44,
    72
};
#endif

#if SURGE_UI_USE_LOADING_STATE
static const surge_ui_loading_state_style_t surge_ui_default_loading_state_style =
{
    { SURGE_UI_ZH_SIZE_20, &FONT_12X20 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    96,
    25,
    4,
    14,
    44,
    72
};
#endif

#if SURGE_UI_USE_TOAST
static const surge_ui_toast_style_t surge_ui_default_toast_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    34,
    8,
    5
};
#endif

#if SURGE_UI_USE_PAGE_INDICATOR
static const surge_ui_page_indicator_style_t surge_ui_default_page_indicator_style =
{
    18,
    8,
    3,
    8
};
#endif

#if SURGE_UI_USE_ACTION_BAR
static const surge_ui_action_bar_style_t surge_ui_default_action_bar_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    34,
    0,
    10,
    8
};
#endif

#if SURGE_UI_USE_SAVE_STATUS_CHIP
static const surge_ui_save_status_chip_style_t surge_ui_default_save_status_chip_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    22,
    8,
    10,
    11,
    3,
    5
};
#endif

#if SURGE_UI_USE_COMPACT_VALUE_TILE
static const surge_ui_compact_value_tile_style_t surge_ui_default_compact_value_tile_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    56,
    6,
    25,
    42,
    2,
    3
};
#endif

#if SURGE_UI_USE_COMPACT_STATUS_TILE
static const surge_ui_compact_status_tile_style_t surge_ui_default_compact_status_tile_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_12X20 },
    54,
    7,
    28,
    2
};
#endif

#if SURGE_UI_USE_SWITCH
static const surge_ui_switch_style_t surge_ui_default_switch_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    32,
    8
};
#endif

#if SURGE_UI_USE_STEPPER
static const surge_ui_stepper_style_t surge_ui_default_stepper_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    46,
    10,
    7,
    20,
    22,
    26,
    24,
    11,
    62,
    4
};
#endif

#if SURGE_UI_USE_MINI_TREND
static const surge_ui_mini_trend_style_t surge_ui_default_mini_trend_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    70,
    10,
    8,
    10,
    28,
    10,
    8,
    3,
    2
};
#endif

#if SURGE_UI_USE_VALUE_TILE
static const surge_ui_value_tile_style_t surge_ui_default_value_tile_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_16X26 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    64,
    12,
    8,
    12,
    28,
    36,
    42
};
#endif

#if SURGE_UI_USE_VALUE_PAIR
static const surge_ui_value_pair_style_t surge_ui_default_value_pair_style =
{
    10,
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        { SURGE_UI_ZH_SIZE_18, &FONT_16X26 },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        64,
        12,
        8,
        12,
        28,
        36,
        42
    }
};
#endif

#if SURGE_UI_USE_STATUS_ROW
static const surge_ui_status_row_style_t surge_ui_default_status_row_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    26,
    8,
    7,
    76,
    5
};
#endif

#if SURGE_UI_USE_STATUS_DOT
static const surge_ui_status_dot_style_t surge_ui_default_status_dot_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    5,
    8,
    4,
    14,
    2
};
#endif

#if SURGE_UI_USE_STATUS_BADGE
static const surge_ui_status_badge_style_t surge_ui_default_status_badge_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    22,
    8,
    5
};
#endif

#if SURGE_UI_USE_ALARM_BANNER
static const surge_ui_alarm_banner_style_t surge_ui_default_alarm_banner_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    31,
    12,
    8
};
#endif

#if SURGE_UI_USE_MENU_ROW
static const surge_ui_menu_row_style_t surge_ui_default_menu_row_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    28,
    10,
    7,
    74,
    8
};
#endif

#if SURGE_UI_USE_MENU_SELECTED_ROW
static const surge_ui_menu_selected_row_style_t surge_ui_default_menu_selected_row_style =
{
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        32,
        12,
        8,
        82,
        10
    },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    14,
    10
};
#endif

#if SURGE_UI_USE_MENU_LIST
static const surge_ui_menu_list_style_t surge_ui_default_menu_list_style =
{
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        28,
        10,
        7,
        74,
        8
    },
    {
        {
            { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
            { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
            32,
            12,
            8,
            82,
            10
        },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        14,
        10
    },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    32,
    8
};
#endif

#if SURGE_UI_USE_PARAM_ROW
static const surge_ui_param_row_style_t surge_ui_default_param_row_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    32,
    12,
    10,
    30,
    74,
    92,
    8,
    28,
    10
};
#endif

#if SURGE_UI_USE_PARAM_EDIT_ROW
static const surge_ui_param_edit_row_style_t surge_ui_default_param_edit_row_style =
{
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        32,
        12,
        10,
        30,
        82,
        88,
        8,
        28,
        10
    },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    106,
    42,
    10
};
#endif

#if SURGE_UI_USE_RECORD_ROW
static const surge_ui_record_row_style_t surge_ui_default_record_row_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    40,
    10,
    12,
    3,
    20,
    6,
    20,
    24,
    6,
    5,
    32,
    9
};
#endif

#if SURGE_UI_USE_DETAIL_VALUE
static const surge_ui_detail_value_style_t surge_ui_default_detail_value_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_16X26 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    82,
    10,
    36,
    44,
    6
};
#endif

#if SURGE_UI_USE_DETAIL_FIELD_ROW
static const surge_ui_detail_field_row_style_t surge_ui_default_detail_field_row_style =
{
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
    28,
    10,
    8,
    10,
    8
};
#endif

#if SURGE_UI_USE_CONFIRM_DIALOG
static const surge_ui_confirm_dialog_style_t surge_ui_default_confirm_dialog_style =
{
    { SURGE_UI_ZH_SIZE_20, &FONT_12X20 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    { SURGE_UI_ZH_SIZE_18, &FONT_10X16 },
    118,
    14,
    48,
    78,
    12,
    30,
    16
};
#endif

static UG_COLOR surge_ui_status_color(surge_ui_status_t status)
{
    if (status == SURGE_UI_STATUS_ALARM)
    {
        return SURGE_UI_COLOR_ALARM;
    }

    if (status == SURGE_UI_STATUS_WARN)
    {
        return SURGE_UI_COLOR_WARN;
    }

    return SURGE_UI_COLOR_OK;
}

#if SURGE_UI_USE_ACTION_BAR || SURGE_UI_USE_CONFIRM_DIALOG
static UG_COLOR surge_ui_status_text_color(surge_ui_status_t status)
{
    if (status == SURGE_UI_STATUS_ALARM)
    {
        return SURGE_UI_COLOR_TEXT;
    }

    return SURGE_UI_COLOR_BG;
}
#endif

static const rt_uint8_t *surge_ui_find_zh_glyph(const surge_ui_zh_font_t *zh_font, rt_uint16_t codepoint)
{
    rt_uint16_t i;

    for (i = 0U; i < zh_font->glyph_count; i++)
    {
        if (zh_font->glyphs[i].codepoint == codepoint)
        {
            return zh_font->glyphs[i].bitmap;
        }
    }

    return RT_NULL;
}

static const surge_ui_zh_font_t *surge_ui_get_zh_font(surge_ui_zh_size_t size)
{
    if (size == SURGE_UI_ZH_SIZE_24)
    {
        return &surge_ui_zh_font_24;
    }

    if (size == SURGE_UI_ZH_SIZE_20)
    {
        return &surge_ui_zh_font_20;
    }

    return &surge_ui_zh_font_18;
}

#if SURGE_UI_USE_SECTION_TITLE || SURGE_UI_USE_EMPTY_STATE || SURGE_UI_USE_LOADING_STATE || SURGE_UI_USE_TOAST || SURGE_UI_USE_ACTION_BAR || SURGE_UI_USE_SAVE_STATUS_CHIP || SURGE_UI_USE_COMPACT_VALUE_TILE || SURGE_UI_USE_COMPACT_STATUS_TILE || SURGE_UI_USE_SWITCH || SURGE_UI_USE_STEPPER || SURGE_UI_USE_RECORD_ROW || SURGE_UI_USE_DETAIL_VALUE || SURGE_UI_USE_DETAIL_FIELD_ROW || SURGE_UI_USE_CONFIRM_DIALOG
static rt_int16_t surge_ui_text_width(const char *text, const surge_ui_text_style_t *style)
{
    const surge_ui_text_style_t *resolved;
    const surge_ui_zh_font_t *zh_font;
    const UG_FONT *ascii_font;
    const unsigned char *p;
    rt_int16_t width;

    if (text == RT_NULL)
    {
        return 0;
    }

    resolved = (style != RT_NULL) ? style : &surge_ui_default_text_8;
    zh_font = surge_ui_get_zh_font(resolved->zh_size);
    ascii_font = (resolved->ascii_font != RT_NULL) ? resolved->ascii_font : &FONT_8X12;
    p = (const unsigned char *)text;
    width = 0;

    while (*p != '\0')
    {
        if (*p < 0x80U)
        {
            width = (rt_int16_t)(width + ascii_font->char_width);
            p++;
            continue;
        }

        if (((p[0] & 0xF0U) == 0xE0U) && ((p[1] & 0xC0U) == 0x80U) && ((p[2] & 0xC0U) == 0x80U))
        {
            width = (rt_int16_t)(width + zh_font->width);
            p = (const unsigned char *)(p + 3);
        }
        else
        {
            width = (rt_int16_t)(width + zh_font->width);
            p++;
        }
    }

    return width;
}
#endif

#if SURGE_UI_USE_EMPTY_STATE || SURGE_UI_USE_LOADING_STATE || SURGE_UI_USE_TOAST || SURGE_UI_USE_ACTION_BAR || SURGE_UI_USE_SAVE_STATUS_CHIP || SURGE_UI_USE_COMPACT_VALUE_TILE || SURGE_UI_USE_COMPACT_STATUS_TILE || SURGE_UI_USE_SWITCH || SURGE_UI_USE_STEPPER || SURGE_UI_USE_DETAIL_VALUE || SURGE_UI_USE_CONFIRM_DIALOG
static void surge_ui_draw_centered_text(rt_int16_t x,
                                        rt_int16_t y,
                                        rt_int16_t w,
                                        const char *text,
                                        UG_COLOR fg,
                                        UG_COLOR bg,
                                        const surge_ui_text_style_t *style)
{
    rt_int16_t text_w;
    rt_int16_t text_x;

    text_w = surge_ui_text_width(text, style);
    text_x = x;
    if (text_w < w)
    {
        text_x = (rt_int16_t)(x + ((w - text_w) / 2));
    }

    surge_ui_draw_text(text_x, y, text, fg, bg, style);
}
#endif

static UG_COLOR surge_ui_blend_rgb565(UG_COLOR fg, UG_COLOR bg, rt_uint8_t level)
{
    rt_uint16_t fr;
    rt_uint16_t fg6;
    rt_uint16_t fb;
    rt_uint16_t br;
    rt_uint16_t bg6;
    rt_uint16_t bb;
    rt_uint16_t r;
    rt_uint16_t g;
    rt_uint16_t b;

    if (level == 0U)
    {
        return bg;
    }
    if (level >= 15U)
    {
        return fg;
    }

    fr = (rt_uint16_t)((fg >> 11) & 0x1FU);
    fg6 = (rt_uint16_t)((fg >> 5) & 0x3FU);
    fb = (rt_uint16_t)(fg & 0x1FU);
    br = (rt_uint16_t)((bg >> 11) & 0x1FU);
    bg6 = (rt_uint16_t)((bg >> 5) & 0x3FU);
    bb = (rt_uint16_t)(bg & 0x1FU);

    r = (rt_uint16_t)(((fr * level) + (br * (15U - level)) + 7U) / 15U);
    g = (rt_uint16_t)(((fg6 * level) + (bg6 * (15U - level)) + 7U) / 15U);
    b = (rt_uint16_t)(((fb * level) + (bb * (15U - level)) + 7U) / 15U);

    return (UG_COLOR)((r << 11) | (g << 5) | b);
}

static void surge_ui_draw_zh_glyph(rt_int16_t x,
                                   rt_int16_t y,
                                   const surge_ui_zh_font_t *zh_font,
                                   const rt_uint8_t *bitmap,
                                   UG_COLOR fg,
                                   UG_COLOR bg)
{
    rt_uint8_t row;

    for (row = 0U; row < zh_font->height; row++)
    {
        rt_uint8_t col;
        for (col = 0U; col < zh_font->width; col++)
        {
            rt_uint8_t byte_value;
            rt_uint8_t byte_index;
            rt_uint8_t shift;
            rt_uint8_t level;

            byte_index = (rt_uint8_t)((row * zh_font->bytes_per_row) + (col / 2U));
            shift = (rt_uint8_t)(4U - ((col % 2U) * 4U));
            byte_value = bitmap[byte_index];
            level = (rt_uint8_t)((byte_value >> shift) & 0x0FU);
            UG_DrawPixel((UG_S16)(x + col), (UG_S16)(y + row), surge_ui_blend_rgb565(fg, bg, level));
        }
    }
}

static void surge_ui_put_string_base(rt_int16_t x,
                                     rt_int16_t y,
                                     const char *text,
                                     UG_COLOR fg,
                                     UG_COLOR bg,
                                     const UG_FONT *font,
                                     const surge_ui_zh_font_t *zh_font)
{
    const unsigned char *p;
    rt_int16_t cursor_x;
    char ascii[2];

    if ((text == RT_NULL) || (font == RT_NULL) || (zh_font == RT_NULL))
    {
        return;
    }

    p = (const unsigned char *)text;
    cursor_x = x;
    ascii[1] = '\0';

    while (*p != '\0')
    {
        rt_uint16_t codepoint;
        const rt_uint8_t *bitmap;

        if (*p < 0x80U)
        {
            ascii[0] = (char)*p;
            UG_FontSelect(font);
            UG_SetForecolor(fg);
            UG_SetBackcolor(bg);
            UG_PutString(cursor_x, y, ascii);
            cursor_x = (rt_int16_t)(cursor_x + font->char_width);
            p++;
            continue;
        }

        codepoint = 0U;
        if (((p[0] & 0xF0U) == 0xE0U) && ((p[1] & 0xC0U) == 0x80U) && ((p[2] & 0xC0U) == 0x80U))
        {
            codepoint = (rt_uint16_t)(((p[0] & 0x0FU) << 12) | ((p[1] & 0x3FU) << 6) | (p[2] & 0x3FU));
            p = (const unsigned char *)(p + 3);
        }
        else
        {
            p++;
            cursor_x = (rt_int16_t)(cursor_x + zh_font->width);
            continue;
        }

        bitmap = surge_ui_find_zh_glyph(zh_font, codepoint);
        if (bitmap != RT_NULL)
        {
            surge_ui_draw_zh_glyph(cursor_x, y, zh_font, bitmap, fg, bg);
        }
        else
        {
            UG_FillFrame(cursor_x, y, (UG_S16)(cursor_x + zh_font->width - 1), (UG_S16)(y + zh_font->height - 1), bg);
        }

        cursor_x = (rt_int16_t)(cursor_x + zh_font->width);
    }
}

void surge_ui_draw_text_18(rt_int16_t x,
                           rt_int16_t y,
                           const char *text,
                           UG_COLOR fg,
                           UG_COLOR bg,
                           const UG_FONT *ascii_font)
{
    surge_ui_put_string_base(x, y, text, fg, bg, ascii_font, &surge_ui_zh_font_18);
}

void surge_ui_draw_text_20(rt_int16_t x,
                           rt_int16_t y,
                           const char *text,
                           UG_COLOR fg,
                           UG_COLOR bg,
                           const UG_FONT *ascii_font)
{
    surge_ui_put_string_base(x, y, text, fg, bg, ascii_font, &surge_ui_zh_font_20);
}

void surge_ui_draw_text_24(rt_int16_t x,
                           rt_int16_t y,
                           const char *text,
                           UG_COLOR fg,
                           UG_COLOR bg,
                           const UG_FONT *ascii_font)
{
    surge_ui_put_string_base(x, y, text, fg, bg, ascii_font, &surge_ui_zh_font_24);
}

void surge_ui_draw_text(rt_int16_t x,
                        rt_int16_t y,
                        const char *text,
                        UG_COLOR fg,
                        UG_COLOR bg,
                        const surge_ui_text_style_t *style)
{
    const surge_ui_text_style_t *resolved;
    const UG_FONT *ascii_font;

    resolved = (style != RT_NULL) ? style : &surge_ui_default_text_8;
    ascii_font = (resolved->ascii_font != RT_NULL) ? resolved->ascii_font : &FONT_8X12;

    if (resolved->zh_size == SURGE_UI_ZH_SIZE_24)
    {
        surge_ui_draw_text_24(x, y, text, fg, bg, ascii_font);
    }
    else if (resolved->zh_size == SURGE_UI_ZH_SIZE_20)
    {
        surge_ui_draw_text_20(x, y, text, fg, bg, ascii_font);
    }
    else
    {
        surge_ui_draw_text_18(x, y, text, fg, bg, ascii_font);
    }
}

void surge_ui_begin(void)
{
    ugui_port_init();
    ugui_port_clear(SURGE_UI_COLOR_BG);
}

#if SURGE_UI_USE_HEADER
void surge_ui_draw_header(const char *title,
                          const char *right_text,
                          const surge_ui_header_style_t *style)
{
    UG_FillFrame(0, 0, 239, 31, SURGE_UI_COLOR_HEADER);
    UG_FillFrame(0, 31, 239, 32, SURGE_UI_COLOR_ACCENT);
    if (style == RT_NULL)
    {
        style = &surge_ui_default_header_style;
    }
    surge_ui_draw_text(style->title_x, style->title_y, title, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_HEADER, &style->title_text);
    surge_ui_draw_text(style->right_x, style->right_y, right_text, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_HEADER, &style->right_text);
}
#endif

#if SURGE_UI_USE_NAV_HEADER
void surge_ui_draw_nav_header(const char *title,
                              const char *right_text,
                              const char *back_text,
                              const surge_ui_nav_header_style_t *style)
{
    if (style == RT_NULL)
    {
        style = &surge_ui_default_nav_header_style;
    }

    if (back_text == RT_NULL)
    {
        back_text = "<";
    }

    UG_FillFrame(0, 0, 239, 31, SURGE_UI_COLOR_HEADER);
    UG_FillFrame(0, 31, 239, 32, SURGE_UI_COLOR_ACCENT);
    surge_ui_draw_text(style->back_x, style->back_y, back_text, SURGE_UI_COLOR_ACCENT, SURGE_UI_COLOR_HEADER, &style->back_text);
    surge_ui_draw_text(style->title_x, style->title_y, title, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_HEADER, &style->title_text);
    surge_ui_draw_text(style->right_x, style->right_y, right_text, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_HEADER, &style->right_text);
}
#endif

#if SURGE_UI_USE_FOOTER
void surge_ui_draw_footer(const char *text, const surge_ui_footer_style_t *style)
{
    UG_FillFrame(0, 222, 239, 239, SURGE_UI_COLOR_SURFACE);
    if (style == RT_NULL)
    {
        style = &surge_ui_default_footer_style;
    }
    surge_ui_draw_text(style->text_x, style->text_y, text, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->text);
}
#endif

#if SURGE_UI_USE_SECTION_TITLE
void surge_ui_draw_section_title(rt_int16_t x,
                                 rt_int16_t y,
                                 rt_int16_t w,
                                 const char *title,
                                 surge_ui_status_t status,
                                 const surge_ui_section_title_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t marker_w;
    rt_int16_t marker_h;
    rt_int16_t title_w;
    rt_int16_t line_x;
    rt_int16_t line_y;

    if (w < 32)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_section_title_style;
    }

    height = style->height;
    if (height < 16)
    {
        height = surge_ui_default_section_title_style.height;
    }

    marker_w = style->marker_w;
    if (marker_w < 1)
    {
        marker_w = 1;
    }

    marker_h = style->marker_h;
    if (marker_h < 4)
    {
        marker_h = 4;
    }

    accent = surge_ui_status_color(status);
    title_w = surge_ui_text_width(title, &style->text);
    line_x = (rt_int16_t)(x + style->text_x + title_w + style->line_gap);
    line_y = (rt_int16_t)(y + style->line_y);

    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_BG);
    UG_FillFrame(x,
                 (rt_int16_t)(y + style->marker_y),
                 (UG_S16)(x + marker_w - 1),
                 (UG_S16)(y + style->marker_y + marker_h - 1),
                 accent);
    surge_ui_draw_text((rt_int16_t)(x + style->text_x),
                       (rt_int16_t)(y + style->text_y),
                       title,
                       SURGE_UI_COLOR_TEXT,
                       SURGE_UI_COLOR_BG,
                       &style->text);

    if (line_x < (rt_int16_t)(x + w - 2))
    {
        UG_DrawLine(line_x, line_y, (UG_S16)(x + w - 1), line_y, SURGE_UI_COLOR_LINE);
    }
}
#endif

#if SURGE_UI_USE_EMPTY_STATE
void surge_ui_draw_empty_state(rt_int16_t x,
                               rt_int16_t y,
                               rt_int16_t w,
                               rt_int16_t h,
                               const char *title,
                               const char *message,
                               surge_ui_status_t status,
                               const surge_ui_empty_state_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t icon_radius;
    rt_int16_t icon_x;
    rt_int16_t icon_y;

    if (w < 80 || h < 64)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_empty_state_style;
    }

    height = h;
    if (height < style->min_height)
    {
        height = style->min_height;
    }

    icon_radius = style->icon_radius;
    if (icon_radius < 2)
    {
        icon_radius = 2;
    }

    accent = surge_ui_status_color(status);
    icon_x = (rt_int16_t)(x + (w / 2));
    icon_y = (rt_int16_t)(y + style->icon_y);

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_LINE);
    UG_FillCircle(icon_x, icon_y, icon_radius, accent);
    if (icon_radius > 3)
    {
        UG_FillCircle(icon_x, icon_y, (UG_S16)(icon_radius - 3), SURGE_UI_COLOR_SURFACE);
    }

    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->title_y),
                                w,
                                title,
                                SURGE_UI_COLOR_TEXT,
                                SURGE_UI_COLOR_SURFACE,
                                &style->title_text);
    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->message_y),
                                w,
                                message,
                                SURGE_UI_COLOR_MUTED,
                                SURGE_UI_COLOR_SURFACE,
                                &style->message_text);
}
#endif

#if SURGE_UI_USE_LOADING_STATE
void surge_ui_draw_loading_state(rt_int16_t x,
                                 rt_int16_t y,
                                 rt_int16_t w,
                                 rt_int16_t h,
                                 const char *title,
                                 const char *message,
                                 rt_uint8_t step,
                                 surge_ui_status_t status,
                                 const surge_ui_loading_state_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t dot_radius;
    rt_int16_t dot_gap;
    rt_int16_t dot_y;
    rt_int16_t start_x;
    rt_uint8_t i;
    rt_uint8_t active;

    if (w < 80 || h < 64)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_loading_state_style;
    }

    height = h;
    if (height < style->min_height)
    {
        height = style->min_height;
    }

    dot_radius = style->dot_radius;
    if (dot_radius < 2)
    {
        dot_radius = 2;
    }

    dot_gap = style->dot_gap;
    if (dot_gap < (rt_int16_t)(dot_radius * 2 + 2))
    {
        dot_gap = (rt_int16_t)(dot_radius * 2 + 2);
    }

    accent = surge_ui_status_color(status);
    active = (rt_uint8_t)(step % 4U);
    dot_y = (rt_int16_t)(y + style->dots_y);
    start_x = (rt_int16_t)(x + (w / 2) - ((dot_gap * 3) / 2));

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_LINE);

    for (i = 0U; i < 4U; i++)
    {
        UG_COLOR dot_color = (i == active) ? accent : SURGE_UI_COLOR_LINE;
        UG_FillCircle((rt_int16_t)(start_x + (dot_gap * i)), dot_y, dot_radius, dot_color);
    }

    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->title_y),
                                w,
                                title,
                                SURGE_UI_COLOR_TEXT,
                                SURGE_UI_COLOR_SURFACE,
                                &style->title_text);
    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->message_y),
                                w,
                                message,
                                SURGE_UI_COLOR_MUTED,
                                SURGE_UI_COLOR_SURFACE,
                                &style->message_text);
}
#endif

#if SURGE_UI_USE_TOAST
void surge_ui_draw_toast(rt_int16_t x,
                         rt_int16_t y,
                         rt_int16_t w,
                         const char *text,
                         surge_ui_status_t status,
                         const surge_ui_toast_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t accent_w;

    if (w < 56)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_toast_style;
    }

    height = style->height;
    if (height < 22)
    {
        height = surge_ui_default_toast_style.height;
    }

    accent_w = style->accent_w;
    if (accent_w < 2)
    {
        accent_w = 2;
    }

    accent = surge_ui_status_color(status);

    UG_FillRoundFrame((rt_int16_t)(x + 2),
                      (rt_int16_t)(y + 3),
                      (UG_S16)(x + w + 1),
                      (UG_S16)(y + height + 2),
                      5,
                      SURGE_UI_COLOR_HEADER);
    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE_ALT);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, accent);
    UG_FillFrame((rt_int16_t)(x + 1),
                 (rt_int16_t)(y + 5),
                 (UG_S16)(x + accent_w),
                 (UG_S16)(y + height - 6),
                 accent);
    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->text_y),
                                w,
                                text,
                                SURGE_UI_COLOR_TEXT,
                                SURGE_UI_COLOR_SURFACE_ALT,
                                &style->text);
}
#endif

#if SURGE_UI_USE_PAGE_INDICATOR
void surge_ui_draw_page_indicator(rt_int16_t x,
                                  rt_int16_t y,
                                  rt_int16_t w,
                                  rt_uint8_t current_page,
                                  rt_uint8_t page_count,
                                  const surge_ui_page_indicator_style_t *style)
{
    rt_int16_t height;
    rt_int16_t dot_radius;
    rt_int16_t dot_gap;
    rt_int16_t dot_step;
    rt_int16_t total_w;
    rt_int16_t start_x;
    rt_int16_t dot_y;
    rt_uint8_t i;

    if (w < 24 || page_count == 0U)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_page_indicator_style;
    }

    if (current_page < 1U)
    {
        current_page = 1U;
    }
    if (current_page > page_count)
    {
        current_page = page_count;
    }

    height = style->height;
    if (height < 8)
    {
        height = surge_ui_default_page_indicator_style.height;
    }

    dot_radius = style->dot_radius;
    if (dot_radius < 1)
    {
        dot_radius = 1;
    }

    dot_gap = style->dot_gap;
    if (dot_gap < 2)
    {
        dot_gap = 2;
    }

    dot_step = (rt_int16_t)((dot_radius * 2) + dot_gap);
    total_w = (rt_int16_t)((page_count * dot_radius * 2) + ((page_count - 1U) * dot_gap));
    while (total_w > w && dot_gap > 2)
    {
        dot_gap--;
        dot_step = (rt_int16_t)((dot_radius * 2) + dot_gap);
        total_w = (rt_int16_t)((page_count * dot_radius * 2) + ((page_count - 1U) * dot_gap));
    }

    if (total_w > w)
    {
        return;
    }

    start_x = (rt_int16_t)(x + ((w - total_w) / 2) + dot_radius);
    dot_y = (rt_int16_t)(y + style->dot_y);

    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_BG);

    for (i = 0U; i < page_count; i++)
    {
        UG_COLOR color = ((rt_uint8_t)(i + 1U) == current_page) ? SURGE_UI_COLOR_ACCENT : SURGE_UI_COLOR_LINE;
        UG_FillCircle((rt_int16_t)(start_x + (i * dot_step)), dot_y, dot_radius, color);
    }
}
#endif

#if SURGE_UI_USE_LIST_VIEW
static rt_uint8_t surge_ui_list_resolve_visible_count(rt_uint8_t visible_count)
{
    return visible_count == 0U ? 1U : visible_count;
}

static rt_uint8_t surge_ui_list_max_top(rt_uint8_t item_count, rt_uint8_t visible_count)
{
    rt_uint8_t resolved_visible = surge_ui_list_resolve_visible_count(visible_count);
    if (item_count <= resolved_visible)
    {
        return 0U;
    }

    return (rt_uint8_t)(item_count - resolved_visible);
}

void surge_ui_list_ensure_visible(surge_ui_list_state_t *state)
{
    rt_uint8_t max_top;

    if (state == RT_NULL)
    {
        return;
    }

    state->visible_count = surge_ui_list_resolve_visible_count(state->visible_count);
    if (state->item_count == 0U)
    {
        state->selected_index = 0U;
        state->top_index = 0U;
        return;
    }

    if (state->selected_index >= state->item_count)
    {
        state->selected_index = (rt_uint8_t)(state->item_count - 1U);
    }

    max_top = surge_ui_list_max_top(state->item_count, state->visible_count);
    if (state->top_index > max_top)
    {
        state->top_index = max_top;
    }

    if (state->selected_index < state->top_index)
    {
        state->top_index = state->selected_index;
    }
    else if (state->selected_index >= (rt_uint8_t)(state->top_index + state->visible_count))
    {
        state->top_index = (rt_uint8_t)(state->selected_index - state->visible_count + 1U);
    }

    if (state->top_index > max_top)
    {
        state->top_index = max_top;
    }
}

void surge_ui_list_init(surge_ui_list_state_t *state,
                        rt_uint8_t item_count,
                        rt_uint8_t visible_count)
{
    if (state == RT_NULL)
    {
        return;
    }

    state->item_count = item_count;
    state->visible_count = surge_ui_list_resolve_visible_count(visible_count);
    state->selected_index = 0U;
    state->top_index = 0U;
    surge_ui_list_ensure_visible(state);
}

void surge_ui_list_set_item_count(surge_ui_list_state_t *state, rt_uint8_t item_count)
{
    if (state == RT_NULL)
    {
        return;
    }

    state->item_count = item_count;
    surge_ui_list_ensure_visible(state);
}

void surge_ui_list_set_visible_count(surge_ui_list_state_t *state, rt_uint8_t visible_count)
{
    if (state == RT_NULL)
    {
        return;
    }

    state->visible_count = surge_ui_list_resolve_visible_count(visible_count);
    surge_ui_list_ensure_visible(state);
}

void surge_ui_list_set_selected(surge_ui_list_state_t *state, rt_uint8_t selected_index)
{
    if (state == RT_NULL)
    {
        return;
    }

    state->selected_index = selected_index;
    surge_ui_list_ensure_visible(state);
}

void surge_ui_list_move(surge_ui_list_state_t *state, surge_ui_list_move_t move)
{
    rt_uint8_t step;

    if (state == RT_NULL)
    {
        return;
    }

    state->visible_count = surge_ui_list_resolve_visible_count(state->visible_count);
    if (state->item_count == 0U)
    {
        surge_ui_list_ensure_visible(state);
        return;
    }

    step = state->visible_count;
    if (move == SURGE_UI_LIST_MOVE_PREV)
    {
        if (state->selected_index > 0U)
        {
            state->selected_index--;
        }
    }
    else if (move == SURGE_UI_LIST_MOVE_NEXT)
    {
        if (state->selected_index < (rt_uint8_t)(state->item_count - 1U))
        {
            state->selected_index++;
        }
    }
    else if (move == SURGE_UI_LIST_MOVE_PAGE_PREV)
    {
        if (state->selected_index > step)
        {
            state->selected_index = (rt_uint8_t)(state->selected_index - step);
        }
        else
        {
            state->selected_index = 0U;
        }
    }
    else if (move == SURGE_UI_LIST_MOVE_PAGE_NEXT)
    {
        rt_uint8_t max_index = (rt_uint8_t)(state->item_count - 1U);
        if ((rt_uint16_t)(state->selected_index + step) < state->item_count)
        {
            state->selected_index = (rt_uint8_t)(state->selected_index + step);
        }
        else
        {
            state->selected_index = max_index;
        }
    }
    else if (move == SURGE_UI_LIST_MOVE_FIRST)
    {
        state->selected_index = 0U;
    }
    else if (move == SURGE_UI_LIST_MOVE_LAST)
    {
        state->selected_index = (rt_uint8_t)(state->item_count - 1U);
    }

    surge_ui_list_ensure_visible(state);
}

rt_uint8_t surge_ui_list_page_count(const surge_ui_list_state_t *state)
{
    rt_uint8_t visible_count;

    if ((state == RT_NULL) || (state->item_count == 0U))
    {
        return 0U;
    }

    visible_count = surge_ui_list_resolve_visible_count(state->visible_count);
    return (rt_uint8_t)((state->item_count + visible_count - 1U) / visible_count);
}

rt_uint8_t surge_ui_list_page_index(const surge_ui_list_state_t *state)
{
    rt_uint8_t visible_count;

    if ((state == RT_NULL) || (state->item_count == 0U))
    {
        return 0U;
    }

    visible_count = surge_ui_list_resolve_visible_count(state->visible_count);
    return (rt_uint8_t)((state->selected_index / visible_count) + 1U);
}
#endif

#if SURGE_UI_USE_ACTION_BAR
void surge_ui_draw_action_bar(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *left_text,
                              const char *right_text,
                              rt_uint8_t selected_button,
                              surge_ui_status_t status,
                              const surge_ui_action_bar_style_t *style)
{
    UG_COLOR accent;
    UG_COLOR selected_text;
    rt_int16_t height;
    rt_int16_t side_margin;
    rt_int16_t gap;
    rt_int16_t button_w;
    rt_int16_t left_x;
    rt_int16_t right_x;
    rt_uint8_t right_selected;

    if (w < 96)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_action_bar_style;
    }

    height = style->height;
    if (height < 26)
    {
        height = surge_ui_default_action_bar_style.height;
    }

    side_margin = style->side_margin;
    if (side_margin < 0)
    {
        side_margin = 0;
    }

    gap = style->button_gap;
    if (gap < 4)
    {
        gap = 4;
    }

    button_w = (rt_int16_t)((w - (side_margin * 2) - gap) / 2);
    if (button_w < 40)
    {
        return;
    }

    accent = surge_ui_status_color(status);
    selected_text = surge_ui_status_text_color(status);
    right_selected = selected_button != 0U ? 1U : 0U;
    left_x = (rt_int16_t)(x + side_margin);
    right_x = (rt_int16_t)(left_x + button_w + gap);

    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_BG);

    if (right_selected == 0U)
    {
        UG_FillRoundFrame(left_x, y, (UG_S16)(left_x + button_w - 1), (UG_S16)(y + height - 1), 5, accent);
        UG_FillRoundFrame(right_x, y, (UG_S16)(right_x + button_w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
        UG_DrawRoundFrame(right_x, y, (UG_S16)(right_x + button_w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_LINE);
    }
    else
    {
        UG_FillRoundFrame(left_x, y, (UG_S16)(left_x + button_w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
        UG_DrawRoundFrame(left_x, y, (UG_S16)(left_x + button_w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_LINE);
        UG_FillRoundFrame(right_x, y, (UG_S16)(right_x + button_w - 1), (UG_S16)(y + height - 1), 5, accent);
    }

    surge_ui_draw_centered_text(left_x,
                                (rt_int16_t)(y + style->text_y),
                                button_w,
                                left_text,
                                right_selected == 0U ? selected_text : SURGE_UI_COLOR_MUTED,
                                right_selected == 0U ? accent : SURGE_UI_COLOR_SURFACE,
                                &style->button_text);
    surge_ui_draw_centered_text(right_x,
                                (rt_int16_t)(y + style->text_y),
                                button_w,
                                right_text,
                                right_selected != 0U ? selected_text : SURGE_UI_COLOR_MUTED,
                                right_selected != 0U ? accent : SURGE_UI_COLOR_SURFACE,
                                &style->button_text);
}
#endif

#if SURGE_UI_USE_SAVE_STATUS_CHIP
void surge_ui_draw_save_status_chip(rt_int16_t x,
                                    rt_int16_t y,
                                    rt_int16_t w,
                                    const char *text,
                                    surge_ui_status_t status,
                                    const surge_ui_save_status_chip_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t dot_radius;
    rt_int16_t text_x;
    rt_int16_t text_w;
    rt_int16_t text_area_w;

    if (w < 36)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_save_status_chip_style;
    }

    height = style->height;
    if (height < 18)
    {
        height = surge_ui_default_save_status_chip_style.height;
    }

    dot_radius = style->dot_radius;
    if (dot_radius < 1)
    {
        dot_radius = surge_ui_default_save_status_chip_style.dot_radius;
    }

    accent = surge_ui_status_color(status);
    text_x = (rt_int16_t)(x + style->dot_x + dot_radius + 5);
    text_area_w = (rt_int16_t)(w - (text_x - x) - style->padding_x);
    text_w = surge_ui_text_width(text, &style->text);
    if ((text_area_w > text_w) && (text_area_w > 0))
    {
        text_x = (rt_int16_t)(text_x + ((text_area_w - text_w) / 2));
    }

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, accent);
    UG_FillCircle((UG_S16)(x + style->dot_x), (UG_S16)(y + style->dot_y), (UG_S16)dot_radius, accent);
    surge_ui_draw_text(text_x, (rt_int16_t)(y + style->text_y), text, accent, SURGE_UI_COLOR_SURFACE, &style->text);
}
#endif

#if SURGE_UI_USE_COMPACT_VALUE_TILE
void surge_ui_draw_compact_value_tile(rt_int16_t x,
                                      rt_int16_t y,
                                      rt_int16_t w,
                                      rt_int16_t h,
                                      const char *label,
                                      const char *value,
                                      const char *unit,
                                      surge_ui_status_t status,
                                      const surge_ui_compact_value_tile_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t inner_w;
    rt_int16_t value_w;
    rt_int16_t value_x;
    rt_int16_t padding_x;
    rt_int16_t value_y;

    if (w < 36)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_compact_value_tile_style;
    }

    height = h;
    if (height < style->min_height)
    {
        height = style->min_height;
    }
    if (height < 40)
    {
        height = surge_ui_default_compact_value_tile_style.min_height;
    }

    accent = surge_ui_status_color(status);
    padding_x = style->padding_x;
    if (padding_x < 0)
    {
        padding_x = 0;
    }
    inner_w = (rt_int16_t)(w - (padding_x * 2));
    if (inner_w < 20)
    {
        return;
    }

    value_w = surge_ui_text_width(value, &style->value_text);
    value_x = (rt_int16_t)(x + padding_x);
    if (value_w < inner_w)
    {
        value_x = (rt_int16_t)(x + padding_x + ((inner_w - value_w) / 2));
    }
    value_y = style->value_y;
    if ((unit == RT_NULL) || (unit[0] == '\0'))
    {
        value_y = (rt_int16_t)(value_y + 6);
    }

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, accent);
    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + 2), accent);

    surge_ui_draw_centered_text((rt_int16_t)(x + padding_x),
                                (rt_int16_t)(y + style->label_y),
                                inner_w,
                                label,
                                SURGE_UI_COLOR_TEXT,
                                SURGE_UI_COLOR_SURFACE,
                                &style->label_text);
    surge_ui_draw_text(value_x,
                       (rt_int16_t)(y + value_y),
                       value,
                       accent,
                       SURGE_UI_COLOR_SURFACE,
                       &style->value_text);
    if ((unit != RT_NULL) && (unit[0] != '\0'))
    {
        surge_ui_draw_centered_text((rt_int16_t)(x + padding_x),
                                    (rt_int16_t)(y + style->unit_y),
                                    inner_w,
                                    unit,
                                    SURGE_UI_COLOR_MUTED,
                                    SURGE_UI_COLOR_SURFACE,
                                    &style->unit_text);
    }
}
#endif

#if SURGE_UI_USE_COMPACT_STATUS_TILE
void surge_ui_draw_compact_status_tile(rt_int16_t x,
                                       rt_int16_t y,
                                       rt_int16_t w,
                                       rt_int16_t h,
                                       const char *label,
                                       const char *state_text,
                                       surge_ui_status_t status,
                                       const surge_ui_compact_status_tile_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t padding_x;
    rt_int16_t inner_w;

    if (w < 32)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_compact_status_tile_style;
    }

    height = h;
    if (height < style->min_height)
    {
        height = style->min_height;
    }
    if (height < 36)
    {
        height = surge_ui_default_compact_status_tile_style.min_height;
    }

    padding_x = style->padding_x;
    if (padding_x < 0)
    {
        padding_x = 0;
    }
    inner_w = (rt_int16_t)(w - (padding_x * 2));
    if (inner_w < 20)
    {
        return;
    }

    accent = surge_ui_status_color(status);

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, accent);
    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + 2), accent);

    surge_ui_draw_centered_text((rt_int16_t)(x + padding_x),
                                (rt_int16_t)(y + style->label_y),
                                inner_w,
                                label,
                                SURGE_UI_COLOR_TEXT,
                                SURGE_UI_COLOR_SURFACE,
                                &style->label_text);
    surge_ui_draw_centered_text((rt_int16_t)(x + padding_x),
                                (rt_int16_t)(y + style->state_y),
                                inner_w,
                                state_text,
                                accent,
                                SURGE_UI_COLOR_SURFACE,
                                &style->state_text);
}
#endif

#if SURGE_UI_USE_SWITCH
void surge_ui_draw_switch(rt_int16_t x,
                          rt_int16_t y,
                          rt_int16_t w,
                          const char *left_text,
                          const char *right_text,
                          rt_uint8_t active_right,
                          surge_ui_status_t status,
                          const surge_ui_switch_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t half_w;
    rt_int16_t active_x;

    if (w < 72)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_switch_style;
    }

    height = style->height;
    if (height < 22)
    {
        height = surge_ui_default_switch_style.height;
    }

    accent = surge_ui_status_color(status);
    half_w = (rt_int16_t)(w / 2);
    active_x = active_right ? (rt_int16_t)(x + half_w) : x;

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 6, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 6, SURGE_UI_COLOR_LINE);
    UG_FillRoundFrame(active_x, (rt_int16_t)(y + 2), (UG_S16)(active_x + half_w - 3), (UG_S16)(y + height - 3), 5, accent);

    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->text_y),
                                half_w,
                                left_text,
                                active_right ? SURGE_UI_COLOR_MUTED : SURGE_UI_COLOR_BG,
                                active_right ? SURGE_UI_COLOR_SURFACE : accent,
                                &style->text);
    surge_ui_draw_centered_text((rt_int16_t)(x + half_w),
                                (rt_int16_t)(y + style->text_y),
                                (rt_int16_t)(w - half_w),
                                right_text,
                                active_right ? SURGE_UI_COLOR_BG : SURGE_UI_COLOR_MUTED,
                                active_right ? accent : SURGE_UI_COLOR_SURFACE,
                                &style->text);
}
#endif

#if SURGE_UI_USE_STEPPER
void surge_ui_draw_stepper(rt_int16_t x,
                           rt_int16_t y,
                           rt_int16_t w,
                           const char *label,
                           const char *value,
                           const char *unit,
                           surge_ui_status_t status,
                           const surge_ui_stepper_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t button_w;
    rt_int16_t button_h;
    rt_int16_t plus_x;
    rt_int16_t minus_x;
    rt_int16_t value_area_x;
    rt_int16_t value_area_w;
    rt_int16_t value_w;
    rt_int16_t unit_w;
    rt_int16_t group_w;
    rt_int16_t value_x;
    rt_int16_t unit_x;

    if (w < 140)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_stepper_style;
    }

    height = style->height;
    if (height < 34)
    {
        height = surge_ui_default_stepper_style.height;
    }

    button_w = style->button_w;
    if (button_w < 20)
    {
        button_w = surge_ui_default_stepper_style.button_w;
    }

    button_h = style->button_h;
    if (button_h < 18)
    {
        button_h = surge_ui_default_stepper_style.button_h;
    }

    value_area_w = style->value_area_w;
    if (value_area_w < 40)
    {
        value_area_w = surge_ui_default_stepper_style.value_area_w;
    }

    accent = surge_ui_status_color(status);
    plus_x = (rt_int16_t)(x + w - button_w - 8);
    minus_x = (rt_int16_t)(plus_x - button_w - value_area_w);
    value_area_x = (rt_int16_t)(minus_x + button_w);

    value_w = surge_ui_text_width(value, &style->value_text);
    unit_w = surge_ui_text_width(unit, &style->unit_text);
    group_w = (rt_int16_t)(value_w + style->value_unit_gap + unit_w);
    value_x = (rt_int16_t)(value_area_x + ((value_area_w - group_w) / 2));
    unit_x = (rt_int16_t)(value_x + value_w + style->value_unit_gap);

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, accent);
    UG_FillFrame(x, (rt_int16_t)(y + 5), (UG_S16)(x + 3), (UG_S16)(y + height - 6), accent);

    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_SURFACE, &style->label_text);

    UG_FillRoundFrame(minus_x, (rt_int16_t)(y + style->button_y), (UG_S16)(minus_x + button_w - 1), (UG_S16)(y + style->button_y + button_h - 1), 4, SURGE_UI_COLOR_SURFACE_ALT);
    UG_DrawRoundFrame(minus_x, (rt_int16_t)(y + style->button_y), (UG_S16)(minus_x + button_w - 1), (UG_S16)(y + style->button_y + button_h - 1), 4, SURGE_UI_COLOR_LINE);
    UG_FillRoundFrame(plus_x, (rt_int16_t)(y + style->button_y), (UG_S16)(plus_x + button_w - 1), (UG_S16)(y + style->button_y + button_h - 1), 4, accent);

    surge_ui_draw_centered_text(minus_x, (rt_int16_t)(y + style->value_y), button_w, "-", SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE_ALT, &style->button_text);
    surge_ui_draw_text(value_x, (rt_int16_t)(y + style->value_y), value, accent, SURGE_UI_COLOR_SURFACE, &style->value_text);
    surge_ui_draw_text(unit_x, (rt_int16_t)(y + style->unit_y), unit, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->unit_text);
    surge_ui_draw_centered_text(plus_x, (rt_int16_t)(y + style->value_y), button_w, "+", SURGE_UI_COLOR_BG, accent, &style->button_text);
}
#endif

#if SURGE_UI_USE_MINI_TREND
void surge_ui_draw_mini_trend(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              rt_int16_t h,
                              const char *label,
                              const rt_uint8_t *values,
                              rt_uint8_t value_count,
                              surge_ui_status_t status,
                              const surge_ui_mini_trend_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t chart_x;
    rt_int16_t chart_y;
    rt_int16_t chart_w;
    rt_int16_t chart_h;
    rt_int16_t radius;
    rt_uint8_t grid_count;
    rt_uint8_t i;

    if (w < 86)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_mini_trend_style;
    }

    height = h;
    if (height < style->min_height)
    {
        height = style->min_height;
    }
    if (height < 44)
    {
        height = surge_ui_default_mini_trend_style.min_height;
    }

    chart_x = (rt_int16_t)(x + style->chart_x);
    chart_y = (rt_int16_t)(y + style->chart_y);
    chart_w = (rt_int16_t)(w - style->chart_x - style->chart_right_margin);
    chart_h = (rt_int16_t)(height - style->chart_y - style->chart_bottom_margin);
    if ((chart_w < 24) || (chart_h < 16))
    {
        return;
    }

    accent = surge_ui_status_color(status);
    radius = style->point_radius;
    if (radius < 0)
    {
        radius = 0;
    }
    if (radius > 3)
    {
        radius = 3;
    }

    grid_count = style->grid_count;
    if (grid_count < 2U)
    {
        grid_count = 2U;
    }

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_LINE);
    UG_FillFrame(x, (rt_int16_t)(y + 6), (UG_S16)(x + 3), (UG_S16)(y + height - 7), accent);
    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_SURFACE, &style->label_text);

    UG_FillFrame(chart_x, chart_y, (UG_S16)(chart_x + chart_w - 1), (UG_S16)(chart_y + chart_h - 1), SURGE_UI_COLOR_SURFACE_ALT);
    UG_DrawFrame(chart_x, chart_y, (UG_S16)(chart_x + chart_w - 1), (UG_S16)(chart_y + chart_h - 1), SURGE_UI_COLOR_LINE);

    for (i = 0U; i < grid_count; i++)
    {
        rt_int16_t grid_y = (rt_int16_t)(chart_y + ((chart_h - 1) * i) / (grid_count - 1U));
        UG_DrawLine(chart_x, grid_y, (UG_S16)(chart_x + chart_w - 1), grid_y, SURGE_UI_COLOR_LINE);
    }

    if ((values == RT_NULL) || (value_count == 0U))
    {
        return;
    }

    if (value_count == 1U)
    {
        rt_uint8_t value = values[0] > 100U ? 100U : values[0];
        rt_int16_t point_x = (rt_int16_t)(chart_x + (chart_w / 2));
        rt_int16_t point_y = (rt_int16_t)(chart_y + chart_h - 1 - ((chart_h - 1) * value) / 100U);
        UG_FillCircle(point_x, point_y, radius > 0 ? radius : 1, accent);
        return;
    }

    {
        rt_uint8_t first_value = values[0] > 100U ? 100U : values[0];
        rt_int16_t prev_x = chart_x;
        rt_int16_t prev_y = (rt_int16_t)(chart_y + chart_h - 1 - ((chart_h - 1) * first_value) / 100U);

        if (radius > 0)
        {
            UG_FillCircle(prev_x, prev_y, radius, accent);
        }

        for (i = 1U; i < value_count; i++)
        {
            rt_uint8_t value = values[i] > 100U ? 100U : values[i];
            rt_int16_t point_x = (rt_int16_t)(chart_x + ((chart_w - 1) * i) / (value_count - 1U));
            rt_int16_t point_y = (rt_int16_t)(chart_y + chart_h - 1 - ((chart_h - 1) * value) / 100U);
            UG_DrawLine(prev_x, prev_y, point_x, point_y, accent);
            if (radius > 0)
            {
                UG_FillCircle(point_x, point_y, radius, accent);
            }
            prev_x = point_x;
            prev_y = point_y;
        }
    }
}
#endif

#if SURGE_UI_USE_VALUE_TILE
void surge_ui_draw_value_tile(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *label,
                              const char *value,
                              const char *unit,
                              surge_ui_status_t status,
                              const surge_ui_value_tile_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_value_tile_style;
    }

    height = style->height;
    if (height < 1)
    {
        height = surge_ui_default_value_tile_style.height;
    }
    accent = surge_ui_status_color(status);
    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 4, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 4, SURGE_UI_COLOR_LINE);
    UG_FillFrame(x, y, (UG_S16)(x + 4), (UG_S16)(y + height - 1), accent);

    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->label_text);
    surge_ui_draw_text((rt_int16_t)(x + style->value_x), (rt_int16_t)(y + style->value_y), value, accent, SURGE_UI_COLOR_SURFACE, &style->value_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->unit_right_margin), (rt_int16_t)(y + style->unit_y), unit, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->unit_text);
}
#endif

#if SURGE_UI_USE_VALUE_PAIR
void surge_ui_draw_value_pair(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const surge_ui_value_item_t *left,
                              const surge_ui_value_item_t *right,
                              const surge_ui_value_pair_style_t *style)
{
    rt_int16_t gap;
    rt_int16_t tile_w;

    if ((left == RT_NULL) || (right == RT_NULL) || (w < 32))
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_value_pair_style;
    }

    gap = style->gap;
    if (gap < 0)
    {
        gap = 0;
    }
    tile_w = (rt_int16_t)((w - gap) / 2);
    if (tile_w < 1)
    {
        return;
    }

    surge_ui_draw_value_tile(x,
                             y,
                             tile_w,
                             left->label,
                             left->value,
                             left->unit,
                             left->status,
                             &style->tile);
    surge_ui_draw_value_tile((rt_int16_t)(x + tile_w + gap),
                             y,
                             tile_w,
                             right->label,
                             right->value,
                             right->unit,
                             right->status,
                             &style->tile);
}
#endif

#if SURGE_UI_USE_STATUS_ROW
void surge_ui_draw_status_row(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *label,
                              const char *value,
                              surge_ui_status_t status,
                              const surge_ui_status_row_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_status_row_style;
    }

    height = style->height;
    if (height < 1)
    {
        height = surge_ui_default_status_row_style.height;
    }
    accent = surge_ui_status_color(status);
    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_SURFACE_ALT);
    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE_ALT, &style->label_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->value_right_margin), (rt_int16_t)(y + style->value_y), value, accent, SURGE_UI_COLOR_SURFACE_ALT, &style->value_text);
}
#endif

#if SURGE_UI_USE_STATUS_DOT
void surge_ui_draw_status_dot(rt_int16_t x,
                              rt_int16_t y,
                              const char *label,
                              surge_ui_status_t status,
                              const surge_ui_status_dot_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t radius;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_status_dot_style;
    }

    radius = style->dot_radius;
    if (radius < 1)
    {
        radius = surge_ui_default_status_dot_style.dot_radius;
    }

    accent = surge_ui_status_color(status);
    UG_FillCircle((UG_S16)(x + style->dot_x), (UG_S16)(y + style->dot_y), (UG_S16)radius, accent);
    UG_DrawCircle((UG_S16)(x + style->dot_x), (UG_S16)(y + style->dot_y), (UG_S16)radius, SURGE_UI_COLOR_LINE);
    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_BG, &style->label_text);
}
#endif

#if SURGE_UI_USE_STATUS_BADGE
void surge_ui_draw_status_badge(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                const char *text,
                                surge_ui_status_t status,
                                const surge_ui_status_badge_style_t *style)
{
    UG_COLOR accent;
    UG_COLOR text_color;
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_status_badge_style;
    }

    height = style->height;
    if (height < 1)
    {
        height = surge_ui_default_status_badge_style.height;
    }

    accent = surge_ui_status_color(status);
    text_color = (status == SURGE_UI_STATUS_ALARM) ? SURGE_UI_COLOR_TEXT : SURGE_UI_COLOR_BG;
    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 4, accent);
    surge_ui_draw_text((rt_int16_t)(x + style->text_x), (rt_int16_t)(y + style->text_y), text, text_color, accent, &style->text);
}
#endif

#if SURGE_UI_USE_ALARM_BANNER
void surge_ui_draw_alarm_banner(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                const char *text,
                                surge_ui_status_t status,
                                const surge_ui_alarm_banner_style_t *style)
{
    UG_COLOR accent;
    UG_COLOR text_color;
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_alarm_banner_style;
    }

    height = style->height;
    if (height < 1)
    {
        height = surge_ui_default_alarm_banner_style.height;
    }
    accent = surge_ui_status_color(status);
    text_color = (status == SURGE_UI_STATUS_ALARM) ? SURGE_UI_COLOR_TEXT : SURGE_UI_COLOR_BG;

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 4, accent);
    surge_ui_draw_text((rt_int16_t)(x + style->text_x), (rt_int16_t)(y + style->text_y), text, text_color, accent, &style->text);
}
#endif

#if SURGE_UI_USE_PROGRESS_BAR
void surge_ui_draw_progress_bar(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                rt_uint8_t percent,
                                surge_ui_status_t status)
{
    rt_int16_t fill_w;
    UG_COLOR accent;

    if (percent > 100U)
    {
        percent = 100U;
    }

    accent = surge_ui_status_color(status);
    fill_w = (rt_int16_t)(((rt_int32_t)(w - 4) * percent) / 100);

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + 13), 3, SURGE_UI_COLOR_SURFACE_ALT);
    if (fill_w > 0)
    {
        UG_FillRoundFrame((rt_int16_t)(x + 2), (rt_int16_t)(y + 2), (UG_S16)(x + 1 + fill_w), (UG_S16)(y + 11), 2, accent);
    }
}
#endif

#if SURGE_UI_USE_MENU_ROW
void surge_ui_draw_menu_row(rt_int16_t x,
                            rt_int16_t y,
                            rt_int16_t w,
                            const char *label,
                            const char *value,
                            const surge_ui_menu_row_style_t *style)
{
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_menu_row_style;
    }

    height = style->height;
    if (height < 1)
    {
        height = surge_ui_default_menu_row_style.height;
    }
    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_SURFACE);
    UG_DrawLine(x, (UG_S16)(y + height - 1), (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_LINE);

    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_SURFACE, &style->label_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->value_right_margin), (rt_int16_t)(y + style->value_y), value, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->value_text);
}
#endif

#if SURGE_UI_USE_MENU_SELECTED_ROW
void surge_ui_draw_menu_selected_row(rt_int16_t x,
                                     rt_int16_t y,
                                     rt_int16_t w,
                                     const char *label,
                                     const char *value,
                                     const surge_ui_menu_selected_row_style_t *style)
{
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_menu_selected_row_style;
    }

    height = style->row.height;
    if (height < 1)
    {
        height = surge_ui_default_menu_selected_row_style.row.height;
    }

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_SURFACE_ALT);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_ACCENT);
    UG_FillFrame(x, (rt_int16_t)(y + 4), (UG_S16)(x + 3), (UG_S16)(y + height - 5), SURGE_UI_COLOR_ACCENT);

    surge_ui_draw_text((rt_int16_t)(x + style->row.label_x), (rt_int16_t)(y + style->row.label_y), label, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_SURFACE_ALT, &style->row.label_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->row.value_right_margin), (rt_int16_t)(y + style->row.value_y), value, SURGE_UI_COLOR_ACCENT, SURGE_UI_COLOR_SURFACE_ALT, &style->row.value_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->arrow_right_margin), (rt_int16_t)(y + style->arrow_y), ">", SURGE_UI_COLOR_ACCENT, SURGE_UI_COLOR_SURFACE_ALT, &style->arrow_text);
}
#endif

#if SURGE_UI_USE_MENU_LIST
void surge_ui_draw_menu_list(rt_int16_t x,
                             rt_int16_t y,
                             rt_int16_t w,
                             const surge_ui_menu_item_t *items,
                             rt_uint8_t item_count,
                             rt_uint8_t selected_index,
                             rt_uint8_t top_index,
                             rt_uint8_t visible_count,
                             surge_ui_menu_scroll_hint_t hint,
                             const surge_ui_menu_list_style_t *style)
{
    rt_uint8_t row;
    rt_uint8_t max_top;
    rt_uint8_t available_count;
    rt_int16_t row_w;
    rt_int16_t list_h;
    rt_int16_t row_step;
    rt_int16_t scroll_bar_width;

    if ((items == RT_NULL) || (item_count == 0U) || (visible_count == 0U))
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_menu_list_style;
    }
    row_step = style->row_step;
    if (row_step < 1)
    {
        row_step = surge_ui_default_menu_list_style.row_step;
    }
    scroll_bar_width = style->scroll_bar_width;
    if (scroll_bar_width < 0)
    {
        scroll_bar_width = 0;
    }

    if (selected_index >= item_count)
    {
        selected_index = (rt_uint8_t)(item_count - 1U);
    }

    max_top = 0U;
    if (item_count > visible_count)
    {
        max_top = (rt_uint8_t)(item_count - visible_count);
    }

    if (top_index > max_top)
    {
        top_index = max_top;
    }

    available_count = (rt_uint8_t)(item_count - top_index);
    if (available_count > visible_count)
    {
        available_count = visible_count;
    }

    row_w = w;
    if ((hint == SURGE_UI_MENU_SCROLL_HINT_BAR) && (item_count > visible_count) && (w > (scroll_bar_width + 4)))
    {
        row_w = (rt_int16_t)(w - scroll_bar_width);
    }

    for (row = 0U; row < available_count; row++)
    {
        rt_uint8_t item_index;
        rt_int16_t row_y;

        item_index = (rt_uint8_t)(top_index + row);
        row_y = (rt_int16_t)(y + (row * row_step));

        if (item_index == selected_index)
        {
            surge_ui_draw_menu_selected_row(x, row_y, row_w, items[item_index].label, items[item_index].value, &style->selected_row);
        }
        else
        {
            surge_ui_draw_menu_row(x, row_y, row_w, items[item_index].label, items[item_index].value, &style->row);
        }
    }

    list_h = (rt_int16_t)(available_count * row_step);

    if ((hint == SURGE_UI_MENU_SCROLL_HINT_BAR) && (item_count > visible_count))
    {
        rt_int16_t track_x;
        rt_int16_t track_y1;
        rt_int16_t track_y2;
        rt_int16_t thumb_y1;
        rt_int16_t thumb_h;
        rt_int16_t travel_h;

        track_x = (rt_int16_t)(x + w - 4);
        track_y1 = y;
        track_y2 = (rt_int16_t)(y + list_h - 5);
        UG_FillRoundFrame(track_x, track_y1, (UG_S16)(track_x + 3), track_y2, 2, SURGE_UI_COLOR_SURFACE_ALT);

        thumb_h = (rt_int16_t)(((rt_int32_t)list_h * visible_count) / item_count);
        if (thumb_h < 16)
        {
            thumb_h = 16;
        }
        if (thumb_h > list_h)
        {
            thumb_h = list_h;
        }

        travel_h = (rt_int16_t)(list_h - thumb_h);
        thumb_y1 = y;
        if ((max_top > 0U) && (travel_h > 0))
        {
            thumb_y1 = (rt_int16_t)(y + (((rt_int32_t)travel_h * top_index) / max_top));
        }

        UG_FillRoundFrame(track_x, thumb_y1, (UG_S16)(track_x + 3), (UG_S16)(thumb_y1 + thumb_h - 1), 2, SURGE_UI_COLOR_ACCENT);
        return;
    }

    if ((hint == SURGE_UI_MENU_SCROLL_HINT_ARROWS) && (top_index > 0U))
    {
        surge_ui_draw_text((rt_int16_t)(x + w - 14), (rt_int16_t)(y - 11), "^", SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_BG, &style->hint_text);
    }

    if ((hint == SURGE_UI_MENU_SCROLL_HINT_ARROWS) && ((rt_uint8_t)(top_index + available_count) < item_count))
    {
        rt_int16_t hint_y;

        hint_y = (rt_int16_t)(y + list_h - 3);
        surge_ui_draw_text((rt_int16_t)(x + w - 14), hint_y, "v", SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_BG, &style->hint_text);
    }
}
#endif

#if SURGE_UI_USE_PARAM_ROW
void surge_ui_draw_param_row(rt_int16_t x,
                             rt_int16_t y,
                             rt_int16_t w,
                             const char *label,
                             const char *value,
                             const char *unit,
                             rt_uint8_t selected,
                             const surge_ui_param_row_style_t *style)
{
    UG_COLOR bg;
    UG_COLOR value_fg;
    rt_int16_t value_x;
    rt_int16_t unit_x;
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_param_row_style;
    }

    height = style->height;
    if (height < 1)
    {
        height = surge_ui_default_param_row_style.height;
    }
    bg = selected ? SURGE_UI_COLOR_SURFACE_ALT : SURGE_UI_COLOR_SURFACE;
    value_fg = selected ? SURGE_UI_COLOR_ACCENT : SURGE_UI_COLOR_TEXT;
    value_x = (rt_int16_t)(x + w - style->value_x);
    unit_x = (rt_int16_t)(x + w - style->unit_right_margin);

    if (selected)
    {
        UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, bg);
        UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_ACCENT);
        UG_FillFrame(x, (rt_int16_t)(y + 4), (UG_S16)(x + 3), (UG_S16)(y + height - 5), SURGE_UI_COLOR_ACCENT);
    }
    else
    {
        UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), bg);
        UG_DrawLine(x, (UG_S16)(y + height - 1), (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_LINE);
    }

    UG_FillRoundFrame((rt_int16_t)(x + w - style->value_box_width - style->value_box_right_margin),
                      (rt_int16_t)(y + 5),
                      (UG_S16)(x + w - style->value_box_right_margin),
                      (UG_S16)(y + height - 6),
                      3,
                      SURGE_UI_COLOR_BG);

    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_TEXT, bg, &style->label_text);
    surge_ui_draw_text(value_x, (rt_int16_t)(y + style->value_y), value, value_fg, SURGE_UI_COLOR_BG, &style->value_text);
    surge_ui_draw_text(unit_x, (rt_int16_t)(y + style->unit_y), unit, SURGE_UI_COLOR_MUTED, bg, &style->unit_text);
}
#endif

#if SURGE_UI_USE_PARAM_EDIT_ROW
void surge_ui_draw_param_edit_row(rt_int16_t x,
                                  rt_int16_t y,
                                  rt_int16_t w,
                                  const char *label,
                                  const char *value,
                                  const char *unit,
                                  const surge_ui_param_edit_row_style_t *style)
{
    rt_int16_t value_x;
    rt_int16_t unit_x;
    rt_int16_t height;

    if (style == RT_NULL)
    {
        style = &surge_ui_default_param_edit_row_style;
    }

    height = style->row.height;
    if (height < 1)
    {
        height = surge_ui_default_param_edit_row_style.row.height;
    }

    value_x = (rt_int16_t)(x + w - style->row.value_x);
    unit_x = (rt_int16_t)(x + w - style->row.unit_right_margin);

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_SURFACE_ALT);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, SURGE_UI_COLOR_WARN);
    UG_FillFrame(x, (rt_int16_t)(y + 4), (UG_S16)(x + 3), (UG_S16)(y + height - 5), SURGE_UI_COLOR_WARN);
    UG_FillRoundFrame((rt_int16_t)(x + w - style->row.value_box_width - style->row.value_box_right_margin),
                      (rt_int16_t)(y + 4),
                      (UG_S16)(x + w - style->row.value_box_right_margin),
                      (UG_S16)(y + height - 5),
                      3,
                      SURGE_UI_COLOR_BG);

    surge_ui_draw_text((rt_int16_t)(x + style->row.label_x), (rt_int16_t)(y + style->row.label_y), label, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_SURFACE_ALT, &style->row.label_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->left_arrow_right_margin), (rt_int16_t)(y + style->arrow_y), "<", SURGE_UI_COLOR_WARN, SURGE_UI_COLOR_BG, &style->arrow_text);
    surge_ui_draw_text(value_x, (rt_int16_t)(y + style->row.value_y), value, SURGE_UI_COLOR_WARN, SURGE_UI_COLOR_BG, &style->row.value_text);
    surge_ui_draw_text((rt_int16_t)(x + w - style->right_arrow_right_margin), (rt_int16_t)(y + style->arrow_y), ">", SURGE_UI_COLOR_WARN, SURGE_UI_COLOR_BG, &style->arrow_text);
    surge_ui_draw_text(unit_x, (rt_int16_t)(y + style->row.unit_y), unit, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE_ALT, &style->row.unit_text);
}
#endif

#if SURGE_UI_USE_RECORD_ROW
void surge_ui_draw_record_row(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *index_text,
                              const char *time_text,
                              const char *value,
                              const char *unit,
                              surge_ui_status_t status,
                              rt_uint8_t selected,
                              const surge_ui_record_row_style_t *style)
{
    UG_COLOR accent;
    UG_COLOR bg;
    rt_int16_t height;
    rt_int16_t radius;
    rt_int16_t unit_x;
    rt_int16_t value_x;
    rt_int16_t value_w;

    if (w < 80)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_record_row_style;
    }

    height = style->height;
    if (height < 24)
    {
        height = surge_ui_default_record_row_style.height;
    }

    radius = style->dot_radius;
    if (radius < 1)
    {
        radius = surge_ui_default_record_row_style.dot_radius;
    }

    accent = surge_ui_status_color(status);
    bg = selected ? SURGE_UI_COLOR_SURFACE_ALT : SURGE_UI_COLOR_SURFACE;
    unit_x = (rt_int16_t)(x + w - style->unit_right_margin);
    value_w = surge_ui_text_width(value, &style->value_text);
    value_x = (rt_int16_t)(unit_x - style->value_gap - value_w);

    if (selected)
    {
        UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, bg);
        UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 3, accent);
        UG_FillFrame(x, (rt_int16_t)(y + 5), (UG_S16)(x + 3), (UG_S16)(y + height - 6), accent);
    }
    else
    {
        UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), bg);
        UG_DrawLine(x, (UG_S16)(y + height - 1), (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_LINE);
    }

    UG_FillCircle((UG_S16)(x + style->dot_x), (UG_S16)(y + style->dot_y), (UG_S16)radius, accent);
    surge_ui_draw_text((rt_int16_t)(x + style->index_x), (rt_int16_t)(y + style->index_y), index_text, SURGE_UI_COLOR_TEXT, bg, &style->index_text);
    surge_ui_draw_text((rt_int16_t)(x + style->time_x), (rt_int16_t)(y + style->time_y), time_text, SURGE_UI_COLOR_MUTED, bg, &style->time_text);
    surge_ui_draw_text(value_x, (rt_int16_t)(y + style->value_y), value, accent, bg, &style->value_text);
    surge_ui_draw_text(unit_x, (rt_int16_t)(y + style->unit_y), unit, SURGE_UI_COLOR_MUTED, bg, &style->unit_text);
}
#endif

#if SURGE_UI_USE_DETAIL_VALUE
void surge_ui_draw_detail_value(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                const char *label,
                                const char *value,
                                const char *unit,
                                surge_ui_status_t status,
                                const surge_ui_detail_value_style_t *style)
{
    UG_COLOR accent;
    rt_int16_t height;
    rt_int16_t value_w;
    rt_int16_t unit_w;
    rt_int16_t group_w;
    rt_int16_t value_x;
    rt_int16_t unit_x;

    if (w < 80)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_detail_value_style;
    }

    height = style->height;
    if (height < 52)
    {
        height = surge_ui_default_detail_value_style.height;
    }

    accent = surge_ui_status_color(status);
    value_w = surge_ui_text_width(value, &style->value_text);
    unit_w = surge_ui_text_width(unit, &style->unit_text);
    group_w = (rt_int16_t)(value_w + style->value_unit_gap + unit_w);
    value_x = (rt_int16_t)(x + ((w - group_w) / 2));
    if (value_x < (rt_int16_t)(x + 8))
    {
        value_x = (rt_int16_t)(x + 8);
    }
    unit_x = (rt_int16_t)(value_x + value_w + style->value_unit_gap);

    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 5, SURGE_UI_COLOR_LINE);
    UG_FillFrame(x, (rt_int16_t)(y + 5), (UG_S16)(x + 4), (UG_S16)(y + height - 6), accent);

    surge_ui_draw_centered_text(x, (rt_int16_t)(y + style->label_y), w, label, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->label_text);
    surge_ui_draw_text(value_x, (rt_int16_t)(y + style->value_y), value, accent, SURGE_UI_COLOR_SURFACE, &style->value_text);
    surge_ui_draw_text(unit_x, (rt_int16_t)(y + style->unit_y), unit, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->unit_text);
}
#endif

#if SURGE_UI_USE_DETAIL_FIELD_ROW
void surge_ui_draw_detail_field_row(rt_int16_t x,
                                    rt_int16_t y,
                                    rt_int16_t w,
                                    const char *label,
                                    const char *value,
                                    const surge_ui_detail_field_row_style_t *style)
{
    rt_int16_t height;
    rt_int16_t value_w;
    rt_int16_t value_x;

    if (w < 48)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_detail_field_row_style;
    }

    height = style->height;
    if (height < 18)
    {
        height = surge_ui_default_detail_field_row_style.height;
    }

    value_w = surge_ui_text_width(value, &style->value_text);
    value_x = (rt_int16_t)(x + w - style->value_right_margin - value_w);
    if (value_x < (rt_int16_t)(x + style->label_x + 42))
    {
        value_x = (rt_int16_t)(x + style->label_x + 42);
    }

    UG_FillFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_SURFACE);
    UG_DrawLine(x, (UG_S16)(y + height - 1), (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), SURGE_UI_COLOR_LINE);
    surge_ui_draw_text((rt_int16_t)(x + style->label_x), (rt_int16_t)(y + style->label_y), label, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE, &style->label_text);
    surge_ui_draw_text(value_x, (rt_int16_t)(y + style->value_y), value, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_SURFACE, &style->value_text);
}
#endif

#if SURGE_UI_USE_CONFIRM_DIALOG
void surge_ui_draw_confirm_dialog(rt_int16_t x,
                                  rt_int16_t y,
                                  rt_int16_t w,
                                  const char *title,
                                  const char *message,
                                  const char *left_text,
                                  const char *right_text,
                                  rt_uint8_t selected_button,
                                  surge_ui_status_t status,
                                  const surge_ui_confirm_dialog_style_t *style)
{
    UG_COLOR accent;
    UG_COLOR selected_text;
    rt_int16_t height;
    rt_int16_t button_gap;
    rt_int16_t button_height;
    rt_int16_t button_side_margin;
    rt_int16_t button_w;
    rt_int16_t left_x;
    rt_int16_t right_x;
    rt_int16_t button_y;

    if (w < 80)
    {
        return;
    }

    if (style == RT_NULL)
    {
        style = &surge_ui_default_confirm_dialog_style;
    }

    height = style->height;
    if (height < 72)
    {
        height = surge_ui_default_confirm_dialog_style.height;
    }

    button_gap = style->button_gap;
    if (button_gap < 0)
    {
        button_gap = 0;
    }

    button_height = style->button_height;
    if (button_height < 20)
    {
        button_height = surge_ui_default_confirm_dialog_style.button_height;
    }

    button_side_margin = style->button_side_margin;
    if (button_side_margin < 0)
    {
        button_side_margin = 0;
    }

    button_w = (rt_int16_t)((w - (button_side_margin * 2) - button_gap) / 2);
    if (button_w < 24)
    {
        return;
    }

    accent = surge_ui_status_color(status);
    selected_text = surge_ui_status_text_color(status);
    left_x = (rt_int16_t)(x + button_side_margin);
    right_x = (rt_int16_t)(left_x + button_w + button_gap);
    button_y = (rt_int16_t)(y + style->button_y);

    UG_FillRoundFrame((rt_int16_t)(x + 3),
                      (rt_int16_t)(y + 4),
                      (UG_S16)(x + w + 2),
                      (UG_S16)(y + height + 3),
                      6,
                      SURGE_UI_COLOR_HEADER);
    UG_FillRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 6, SURGE_UI_COLOR_SURFACE);
    UG_DrawRoundFrame(x, y, (UG_S16)(x + w - 1), (UG_S16)(y + height - 1), 6, accent);
    UG_FillFrame((rt_int16_t)(x + 6), (rt_int16_t)(y + 1), (UG_S16)(x + w - 7), (UG_S16)(y + 3), accent);

    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->title_y),
                                w,
                                title,
                                SURGE_UI_COLOR_TEXT,
                                SURGE_UI_COLOR_SURFACE,
                                &style->title_text);
    surge_ui_draw_centered_text(x,
                                (rt_int16_t)(y + style->message_y),
                                w,
                                message,
                                SURGE_UI_COLOR_MUTED,
                                SURGE_UI_COLOR_SURFACE,
                                &style->message_text);

    if (selected_button == 0U)
    {
        UG_FillRoundFrame(left_x, button_y, (UG_S16)(left_x + button_w - 1), (UG_S16)(button_y + button_height - 1), 4, accent);
        UG_FillRoundFrame(right_x, button_y, (UG_S16)(right_x + button_w - 1), (UG_S16)(button_y + button_height - 1), 4, SURGE_UI_COLOR_SURFACE_ALT);
        UG_DrawRoundFrame(right_x, button_y, (UG_S16)(right_x + button_w - 1), (UG_S16)(button_y + button_height - 1), 4, SURGE_UI_COLOR_LINE);
        surge_ui_draw_centered_text(left_x, (rt_int16_t)(button_y + 6), button_w, left_text, selected_text, accent, &style->button_text);
        surge_ui_draw_centered_text(right_x, (rt_int16_t)(button_y + 6), button_w, right_text, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE_ALT, &style->button_text);
    }
    else
    {
        UG_FillRoundFrame(left_x, button_y, (UG_S16)(left_x + button_w - 1), (UG_S16)(button_y + button_height - 1), 4, SURGE_UI_COLOR_SURFACE_ALT);
        UG_DrawRoundFrame(left_x, button_y, (UG_S16)(left_x + button_w - 1), (UG_S16)(button_y + button_height - 1), 4, SURGE_UI_COLOR_LINE);
        UG_FillRoundFrame(right_x, button_y, (UG_S16)(right_x + button_w - 1), (UG_S16)(button_y + button_height - 1), 4, accent);
        surge_ui_draw_centered_text(left_x, (rt_int16_t)(button_y + 6), button_w, left_text, SURGE_UI_COLOR_MUTED, SURGE_UI_COLOR_SURFACE_ALT, &style->button_text);
        surge_ui_draw_centered_text(right_x, (rt_int16_t)(button_y + 6), button_w, right_text, selected_text, accent, &style->button_text);
    }
}
#endif
