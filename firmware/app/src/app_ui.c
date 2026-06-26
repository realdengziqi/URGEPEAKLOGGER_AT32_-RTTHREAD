#include "app_ui.h"

#include <rtthread.h>

#include "app_key_input.h"
#include "surge_ui_widgets.h"

#define UI_TEXT_LOGGER          "\xE6\xB5\xAA\xE6\xB6\x8C\xE8\xAE\xB0\xE5\xBD\x95\xE5\x99\xA8"
#define UI_TEXT_PEAK_VOLTAGE    "\xE5\xB3\xB0\xE5\x80\xBC\xE7\x94\xB5\xE5\x8E\x8B"
#define UI_TEXT_PEAK_VALUE      "\xE5\xB3\xB0\xE5\x80\xBC"
#define UI_TEXT_STYLE_PREVIEW   "\xE6\xA0\xB7\xE5\xBC\x8F\xE9\xA2\x84\xE8\xA7\x88"
#define UI_TEXT_COMM            "\xE9\x80\x9A\xE4\xBF\xA1"
#define UI_TEXT_ONLINE          "\xE5\x9C\xA8\xE7\xBA\xBF"
#define UI_TEXT_NETWORK         "\xE7\xBD\x91\xE7\xBB\x9C"
#define UI_TEXT_WAIT            "\xE7\xAD\x89\xE5\xBE\x85"
#define UI_TEXT_FOOTER_REFRESH  "\xE4\xB8\x8A\xE4\xB8\x8B\xE7\xBF\xBB\xE9\xA1\xB5  \xE7\xA1\xAE\xE8\xAE\xA4\xE5\x88\xB7\xE6\x96\xB0"
#define UI_TEXT_CONTROL_STATUS  "\xE6\x8E\xA7\xE5\x88\xB6\xE7\x8A\xB6\xE6\x80\x81"
#define UI_TEXT_TEMP            "\xE6\xB8\xA9\xE5\xBA\xA6"
#define UI_TEXT_HUMI            "\xE6\xB9\xBF\xE5\xBA\xA6"
#define UI_TEXT_INPUT_1         "\xE8\xBE\x93\xE5\x85\xA5""1"
#define UI_TEXT_INPUT_2         "\xE8\xBE\x93\xE5\x85\xA5""2"
#define UI_TEXT_NORMAL          "\xE6\xAD\xA3\xE5\xB8\xB8"
#define UI_TEXT_WIDGET_PREVIEW  "\xE6\x8E\xA7\xE4\xBB\xB6\xE6\x95\x88\xE6\x9E\x9C\xE9\xA2\x84\xE8\xA7\x88"
#define UI_TEXT_ALARM_STATUS    "\xE6\x8A\xA5\xE8\xAD\xA6\xE7\x8A\xB6\xE6\x80\x81"
#define UI_TEXT_ALARM           "\xE6\x8A\xA5\xE8\xAD\xA6"
#define UI_TEXT_SURGE_ALARM     "\xE6\xB5\xAA\xE6\xB6\x8C\xE6\x8A\xA5\xE8\xAD\xA6"
#define UI_TEXT_LAST_PEAK       "\xE6\x9C\x80\xE8\xBF\x91\xE5\xB3\xB0\xE5\x80\xBC"
#define UI_TEXT_RECORD          "\xE8\xAE\xB0\xE5\xBD\x95"
#define UI_TEXT_NO_RECORDS      "\xE6\x9A\x82\xE6\x97\xA0\xE8\xAE\xB0\xE5\xBD\x95"
#define UI_TEXT_LOADING         "\xE5\x8A\xA0\xE8\xBD\xBD\xE4\xB8\xAD"
#define UI_TEXT_PLEASE_WAIT     "\xE8\xAF\xB7\xE7\xAD\x89\xE5\xBE\x85"
#define UI_TEXT_INDEX           "\xE7\xBC\x96\xE5\x8F\xB7"
#define UI_TEXT_STATUS          "\xE7\x8A\xB6\xE6\x80\x81"
#define UI_TEXT_SAVED           "\xE4\xBF\x9D\xE5\xAD\x98"
#define UI_TEXT_SAVED_DONE      "\xE5\xB7\xB2\xE4\xBF\x9D\xE5\xAD\x98"
#define UI_TEXT_SAVE_FAILED     "\xE4\xBF\x9D\xE5\xAD\x98\xE5\xA4\xB1\xE8\xB4\xA5"
#define UI_TEXT_CONFIRM_REFRESH "\xE7\xA1\xAE\xE8\xAE\xA4\xE5\x88\xB7\xE6\x96\xB0"
#define UI_TEXT_DESIGN_PAGE     "\xE8\xAE\xBE\xE8\xAE\xA1\xE7\xA1\xAE\xE8\xAE\xA4\xE9\xA1\xB5"
#define UI_TEXT_MENU            "\xE8\x8F\x9C\xE5\x8D\x95"
#define UI_TEXT_FONT_DEMO       "\xE5\xAD\x97\xE5\x8F\xB7\xE5\xB1\x95\xE7\xA4\xBA"
#define UI_TEXT_18_BODY         "18 \xE4\xB8\xAD\xE6\x96\x87\xE6\xAD\xA3\xE6\x96\x87"
#define UI_TEXT_20_TITLE        "20 \xE6\xA0\x87\xE9\xA2\x98\xE6\x96\x87\xE5\xAD\x97"
#define UI_TEXT_24_STATUS       "24 \xE5\xA4\xA7\xE5\xAD\x97\xE7\x8A\xB6\xE6\x80\x81"
#define UI_TEXT_FONT_FOOTER     "18\xE6\xAD\xA3\xE6\x96\x87 20\xE6\xA0\x87\xE9\xA2\x98 24\xE5\xA4\xA7\xE5\xAD\x97"
#define UI_TEXT_CLEAR_RECORD    "\xE6\xB8\x85\xE9\x99\xA4\xE8\xAE\xB0\xE5\xBD\x95"
#define UI_TEXT_CONFIRM_CLEAR   "\xE7\xA1\xAE\xE8\xAE\xA4\xE6\xB8\x85\xE9\x99\xA4\xE8\xAE\xB0\xE5\xBD\x95"
#define UI_TEXT_RETURN          "\xE8\xBF\x94\xE5\x9B\x9E"
#define UI_TEXT_CONFIRM         "\xE7\xA1\xAE\xE8\xAE\xA4"
#define UI_TEXT_SETTING         "\xE8\xAE\xBE\xE7\xBD\xAE"
#define UI_TEXT_MODE            "\xE6\xA8\xA1\xE5\xBC\x8F"
#define UI_TEXT_AUTO            "\xE8\x87\xAA\xE5\x8A\xA8"
#define UI_TEXT_MANUAL          "\xE6\x89\x8B\xE5\x8A\xA8"
#define UI_TEXT_PARAM           "\xE5\x8F\x82\xE6\x95\xB0"
#define UI_TEXT_ALARM_VALUE     "\xE6\x8A\xA5\xE8\xAD\xA6\xE5\x80\xBC"
#define UI_TEXT_TREND           "\xE8\xB6\x8B\xE5\x8A\xBF"
#define UI_TEXT_PEAK_TREND      "\xE5\xB3\xB0\xE5\x80\xBC\xE8\xB6\x8B\xE5\x8A\xBF"
#define UI_TEXT_CONFIG_ITEM     "\xE9\x85\x8D\xE7\xBD\xAE\xE9\xA1\xB9"
#define UI_TEXT_CANCEL          "\xE5\x8F\x96\xE6\xB6\x88"
#define UI_TEXT_PENDING_SAVE    "\xE5\xBE\x85\xE4\xBF\x9D\xE5\xAD\x98"

typedef enum
{
    APP_UI_PAGE_ACTION = 0,
    APP_UI_PAGE_COMPACT_STATUS,
    APP_UI_PAGE_COMPACT,
    APP_UI_PAGE_TREND,
    APP_UI_PAGE_EDITORS,
    APP_UI_PAGE_RECORDS,
    APP_UI_PAGE_WIDGETS,
    APP_UI_PAGE_STATUS,
    APP_UI_PAGE_ALARM,
    APP_UI_PAGE_EMPTY,
    APP_UI_PAGE_LOADING,
    APP_UI_PAGE_DETAIL,
    APP_UI_PAGE_FONTS,
    APP_UI_PAGE_CONFIRM,
    APP_UI_PAGE_TOAST,
    APP_UI_PAGE_ENGLISH,
    APP_UI_PAGE_COUNT
} app_ui_page_t;

static app_ui_page_t app_ui_current_page = APP_UI_PAGE_ACTION;

static void app_ui_draw_current_page(void);

void app_ui_init(void)
{
    app_key_input_init();
    app_ui_current_page = APP_UI_PAGE_ACTION;
    app_ui_draw_current_page();
}

void app_ui_poll(void)
{
    app_key_event_t event;

    event = app_key_input_poll();
    if (event == APP_KEY_EVENT_NONE)
    {
        return;
    }

    if (event == APP_KEY_EVENT_UP || event == APP_KEY_EVENT_LEFT)
    {
        if (app_ui_current_page == APP_UI_PAGE_ACTION)
        {
            app_ui_current_page = (app_ui_page_t)(APP_UI_PAGE_COUNT - 1);
        }
        else
        {
            app_ui_current_page = (app_ui_page_t)(app_ui_current_page - 1);
        }
    }
    else if (event == APP_KEY_EVENT_DOWN || event == APP_KEY_EVENT_RIGHT)
    {
        app_ui_current_page = (app_ui_page_t)((app_ui_current_page + 1) % APP_UI_PAGE_COUNT);
    }
    else if (event != APP_KEY_EVENT_MID)
    {
        return;
    }

    app_ui_draw_current_page();
}

void app_ui_draw_home(void)
{
    app_ui_current_page = APP_UI_PAGE_ACTION;
    app_ui_draw_current_page();
}

static void app_ui_draw_compact_status_page(void)
{
    const rt_int16_t gap = 4;
    const rt_int16_t tile_w = (rt_int16_t)((220 - (gap * 4)) / 5);

    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_STATUS, "2/16", RT_NULL);
    surge_ui_draw_section_title(10, 44, 220, UI_TEXT_CONTROL_STATUS, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile(10, 76, tile_w, 56, "TI1", "OK", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile((rt_int16_t)(10 + tile_w + gap), 76, tile_w, 56, "TI2", "OK", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile((rt_int16_t)(10 + ((tile_w + gap) * 2)), 76, tile_w, 56, "TI3", "OK", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile((rt_int16_t)(10 + ((tile_w + gap) * 3)), 76, tile_w, 56, "TI4", "OK", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile((rt_int16_t)(10 + ((tile_w + gap) * 4)), 76, tile_w, 56, "PE", "OK", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile(10, 144, 68, 56, "RS485", "OK", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_status_tile(86, 144, 68, 56, "ETH", "WAIT", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_compact_status_tile(162, 144, 68, 56, "ALM", "ERR", SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_compact_page(void)
{
    const rt_int16_t gap = 4;
    const rt_int16_t tile_w = (rt_int16_t)((220 - (gap * 3)) / 4);

    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_STATUS, "3/16", RT_NULL);
    surge_ui_draw_compact_value_tile(10, 46, tile_w, 58, UI_TEXT_PEAK_VALUE, "1122", "", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_compact_value_tile((rt_int16_t)(10 + tile_w + gap), 46, tile_w, 58, UI_TEXT_ALARM, "11", "", SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_compact_value_tile((rt_int16_t)(10 + ((tile_w + gap) * 2)), 46, tile_w, 58, UI_TEXT_TEMP, "26.5", "C", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_value_tile((rt_int16_t)(10 + ((tile_w + gap) * 3)), 46, tile_w, 58, UI_TEXT_HUMI, "23", "%", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_compact_value_tile(10, 114, 106, 58, "NTC", "112.3", "C", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_compact_value_tile(124, 114, 106, 58, "ID", "001", "", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_action_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_SETTING, "1/16", RT_NULL);
    surge_ui_draw_section_title(10, 44, 132, UI_TEXT_CONFIG_ITEM, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_save_status_chip(150, 44, 80, UI_TEXT_PENDING_SAVE, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_param_row(10, 76, 220, UI_TEXT_INPUT_1, "1", "", 0, RT_NULL);
    surge_ui_draw_param_edit_row(10, 112, 220, UI_TEXT_ALARM_VALUE, "31", "KV", RT_NULL);
    surge_ui_draw_param_row(10, 148, 220, UI_TEXT_NETWORK, UI_TEXT_WAIT, "", 0, RT_NULL);
    surge_ui_draw_action_bar(10, 194, 220, UI_TEXT_CANCEL, UI_TEXT_SAVED, 1, SURGE_UI_STATUS_NORMAL, RT_NULL);
}

static void app_ui_draw_trend_page(void)
{
    const rt_uint8_t trend_values[] = { 18, 24, 20, 36, 31, 54, 48, 62, 57, 76, 68, 84 };

    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_TREND, "4/16", RT_NULL);
    surge_ui_draw_mini_trend(10, 46, 220, 76, UI_TEXT_PEAK_TREND, trend_values, sizeof(trend_values) / sizeof(trend_values[0]), SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_status_row(10, 138, 220, UI_TEXT_LAST_PEAK, "31.6 KV", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_page_indicator(10, 184, 220, 1, 6, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_FOOTER_REFRESH, RT_NULL);
}

static void app_ui_draw_editors_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_SETTING, "5/16", RT_NULL);
    surge_ui_draw_section_title(10, 44, 220, UI_TEXT_MODE, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_switch(40, 72, 160, UI_TEXT_AUTO, UI_TEXT_MANUAL, 0, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_section_title(10, 120, 220, UI_TEXT_PARAM, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_stepper(10, 150, 220, UI_TEXT_ALARM_VALUE, "31", "KV", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_FOOTER_REFRESH, RT_NULL);
}

static void app_ui_draw_widgets_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_LOGGER, "7/16", RT_NULL);
    surge_ui_draw_value_tile(10, 44, 220, UI_TEXT_PEAK_VOLTAGE, "12.8", "KV", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_alarm_banner(10, 118, 220, UI_TEXT_STYLE_PREVIEW, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_status_row(10, 158, 220, UI_TEXT_COMM, UI_TEXT_ONLINE, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_status_row(10, 188, 220, UI_TEXT_NETWORK, UI_TEXT_WAIT, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_FOOTER_REFRESH, RT_NULL);
}

static void app_ui_draw_status_page(void)
{
    const surge_ui_value_item_t values[] =
    {
        { UI_TEXT_TEMP, "26.4", "C", SURGE_UI_STATUS_NORMAL },
        { UI_TEXT_HUMI, "48", "%", SURGE_UI_STATUS_NORMAL }
    };

    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_CONTROL_STATUS, "8/16", RT_NULL);
    surge_ui_draw_value_pair(10, 44, 220, &values[0], &values[1], RT_NULL);
    surge_ui_draw_status_row(10, 122, 220, UI_TEXT_INPUT_1, UI_TEXT_NORMAL, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_status_row(10, 152, 220, UI_TEXT_INPUT_2, UI_TEXT_WAIT, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_progress_bar(10, 192, 220, 68, SURGE_UI_STATUS_NORMAL);
    surge_ui_draw_footer(UI_TEXT_WIDGET_PREVIEW, RT_NULL);
}

static void app_ui_draw_alarm_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_ALARM_STATUS, "9/16", RT_NULL);
    surge_ui_draw_alarm_banner(10, 46, 220, UI_TEXT_SURGE_ALARM, SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_value_tile(10, 88, 220, UI_TEXT_LAST_PEAK, "31.6", "KV", SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_status_row(10, 166, 220, UI_TEXT_RECORD, UI_TEXT_SAVED, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_progress_bar(10, 202, 220, 92, SURGE_UI_STATUS_WARN);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_records_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_RECORD, "6/16", RT_NULL);
    surge_ui_draw_record_row(10, 46, 220, UI_TEXT_RECORD "1", "2026-06-21 00:12", "31.6", "KV", SURGE_UI_STATUS_ALARM, 1, RT_NULL);
    surge_ui_draw_record_row(10, 88, 220, UI_TEXT_RECORD "2", "2026-06-20 22:48", "18.2", "KV", SURGE_UI_STATUS_WARN, 0, RT_NULL);
    surge_ui_draw_record_row(10, 130, 220, UI_TEXT_RECORD "3", "2026-06-20 19:31", "12.8", "KV", SURGE_UI_STATUS_NORMAL, 0, RT_NULL);
    surge_ui_draw_page_indicator(10, 184, 220, 1, 6, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_empty_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_RECORD, "10/16", RT_NULL);
    surge_ui_draw_empty_state(18, 64, 204, 126, UI_TEXT_NO_RECORDS, UI_TEXT_CONFIRM_REFRESH, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_loading_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_RECORD, "11/16", RT_NULL);
    surge_ui_draw_loading_state(18, 64, 204, 126, UI_TEXT_LOADING, UI_TEXT_PLEASE_WAIT, 2, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_detail_page(void)
{
    surge_ui_begin();
    surge_ui_draw_nav_header(UI_TEXT_RECORD, "12/16", UI_TEXT_RETURN, RT_NULL);
    surge_ui_draw_detail_value(10, 42, 220, UI_TEXT_PEAK_VALUE, "31.6", "KV", SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_section_title(10, 130, 220, UI_TEXT_RECORD, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_detail_field_row(10, 154, 220, UI_TEXT_INDEX, "000128", RT_NULL);
    surge_ui_draw_detail_field_row(10, 184, 220, "TIME", "2026-06-21 00:12", RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_fonts_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_FONT_DEMO, "13/16", RT_NULL);
    surge_ui_draw_text_18(10, 46, UI_TEXT_18_BODY, SURGE_UI_COLOR_TEXT, SURGE_UI_COLOR_BG, &FONT_10X16);
    surge_ui_draw_text_20(10, 78, UI_TEXT_20_TITLE, SURGE_UI_COLOR_ACCENT, SURGE_UI_COLOR_BG, &FONT_12X20);
    surge_ui_draw_text_24(10, 112, UI_TEXT_24_STATUS, SURGE_UI_COLOR_WARN, SURGE_UI_COLOR_BG, &FONT_12X20);
    surge_ui_draw_status_row(10, 166, 220, UI_TEXT_MENU, "18", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_FONT_FOOTER, RT_NULL);
}

static void app_ui_draw_confirm_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_CLEAR_RECORD, "14/16", RT_NULL);
    surge_ui_draw_status_row(10, 46, 220, UI_TEXT_RECORD, "128", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_confirm_dialog(18,
                                 82,
                                 204,
                                 UI_TEXT_CLEAR_RECORD,
                                 UI_TEXT_CONFIRM_CLEAR,
                                 UI_TEXT_RETURN,
                                 UI_TEXT_CONFIRM,
                                 1,
                                 SURGE_UI_STATUS_ALARM,
                                 RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_toast_page(void)
{
    surge_ui_begin();
    surge_ui_draw_header(UI_TEXT_STATUS, "15/16", RT_NULL);
    surge_ui_draw_status_row(10, 62, 220, UI_TEXT_RECORD, "128", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_status_row(10, 94, 220, "FLASH", UI_TEXT_WAIT, SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_toast(35, 142, 170, UI_TEXT_SAVED_DONE, SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_toast(35, 184, 170, UI_TEXT_SAVE_FAILED, SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_footer(UI_TEXT_DESIGN_PAGE, RT_NULL);
}

static void app_ui_draw_english_page(void)
{
    const surge_ui_header_style_t header_style =
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_12X20 },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        8,
        7,
        196,
        10
    };
    const surge_ui_value_pair_style_t pair_style =
    {
        10,
        {
            { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
            { SURGE_UI_ZH_SIZE_18, &FONT_16X26 },
            { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
            64,
            10,
            8,
            10,
            28,
            30,
            42
        }
    };
    const surge_ui_status_row_style_t row_style =
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        26,
        8,
        7,
        86,
        7
    };
    const surge_ui_footer_style_t footer_style =
    {
        { SURGE_UI_ZH_SIZE_18, &FONT_8X12 },
        8,
        226
    };
    const surge_ui_value_item_t values[] =
    {
        { "PEAK", "12.8", "KV", SURGE_UI_STATUS_NORMAL },
        { "EVENT", "128", "", SURGE_UI_STATUS_WARN }
    };

    surge_ui_begin();
    surge_ui_draw_header("EN STYLE", "16/16", &header_style);
    surge_ui_draw_value_pair(10, 44, 220, &values[0], &values[1], &pair_style);
    surge_ui_draw_status_row(10, 122, 220, "RS485 LINK", "ONLINE", SURGE_UI_STATUS_NORMAL, &row_style);
    surge_ui_draw_status_row(10, 152, 220, "ETHERNET", "WAIT", SURGE_UI_STATUS_WARN, &row_style);
    surge_ui_draw_status_dot(12, 184, "RS485", SURGE_UI_STATUS_NORMAL, RT_NULL);
    surge_ui_draw_status_dot(82, 184, "ETH", SURGE_UI_STATUS_WARN, RT_NULL);
    surge_ui_draw_status_badge(152, 181, 78, "ALARM", SURGE_UI_STATUS_ALARM, RT_NULL);
    surge_ui_draw_footer("ENGLISH LAYOUT TEST", &footer_style);
}

static void app_ui_draw_current_page(void)
{
    if (app_ui_current_page == APP_UI_PAGE_ACTION)
    {
        app_ui_draw_action_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_COMPACT_STATUS)
    {
        app_ui_draw_compact_status_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_COMPACT)
    {
        app_ui_draw_compact_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_TREND)
    {
        app_ui_draw_trend_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_EDITORS)
    {
        app_ui_draw_editors_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_STATUS)
    {
        app_ui_draw_status_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_ALARM)
    {
        app_ui_draw_alarm_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_RECORDS)
    {
        app_ui_draw_records_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_EMPTY)
    {
        app_ui_draw_empty_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_LOADING)
    {
        app_ui_draw_loading_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_DETAIL)
    {
        app_ui_draw_detail_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_FONTS)
    {
        app_ui_draw_fonts_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_CONFIRM)
    {
        app_ui_draw_confirm_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_TOAST)
    {
        app_ui_draw_toast_page();
    }
    else if (app_ui_current_page == APP_UI_PAGE_ENGLISH)
    {
        app_ui_draw_english_page();
    }
    else
    {
        app_ui_draw_widgets_page();
    }
}




