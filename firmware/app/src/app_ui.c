#include "app_ui.h"

#include <rtthread.h>

#include "app_data_service.h"
#include "app_key_input.h"
#include "screen_st7789.h"

#define UI_COLOR_BG                 0x0000U
#define UI_COLOR_PANEL              0x1082U
#define UI_COLOR_PANEL_ALT          0x2104U
#define UI_COLOR_HEADER             0x001FU
#define UI_COLOR_TEXT               SCREEN_ST7789_COLOR_WHITE
#define UI_COLOR_TEXT_MUTED         0xBDF7U
#define UI_COLOR_ACCENT             SCREEN_ST7789_COLOR_CYAN
#define UI_COLOR_WARN               SCREEN_ST7789_COLOR_YELLOW
#define UI_COLOR_OK                 SCREEN_ST7789_COLOR_GREEN
#define UI_COLOR_ALARM              SCREEN_ST7789_COLOR_RED

typedef enum
{
    APP_UI_PAGE_HOME = 0,
    APP_UI_PAGE_STATUS,
    APP_UI_PAGE_ABOUT,
    APP_UI_PAGE_COUNT
} app_ui_page_t;

static app_ui_page_t app_ui_current_page = APP_UI_PAGE_HOME;

static void app_ui_draw_label_value(rt_uint16_t x,
                                    rt_uint16_t y,
                                    const char *label,
                                    const char *value,
                                    rt_uint16_t value_color)
{
    screen_st7789_draw_string(x, y, label, UI_COLOR_TEXT_MUTED, UI_COLOR_PANEL, 1);
    screen_st7789_draw_string(x, (rt_uint16_t)(y + 11U), value, value_color, UI_COLOR_PANEL, 2);
}

static void app_ui_draw_footer(const char *text)
{
    screen_st7789_fill_rect(0, 224, 240, 16, UI_COLOR_PANEL);
    screen_st7789_draw_string(8, 229, text, UI_COLOR_TEXT, UI_COLOR_PANEL, 1);
}

static void app_ui_format_fixed_1(char *buffer, rt_size_t size, rt_int32_t milli_value, const char *unit)
{
    rt_int32_t whole;
    rt_int32_t decimal;
    const char *sign;

    sign = "";
    if (milli_value < 0)
    {
        sign = "-";
        milli_value = -milli_value;
    }

    whole = milli_value / 1000;
    decimal = (milli_value % 1000) / 100;
    rt_snprintf(buffer, size, "%s%d.%d%s", sign, whole, decimal, unit);
}

static void app_ui_draw_header(const char *title, const char *right_text)
{
    screen_st7789_fill_rect(0, 0, 240, 30, UI_COLOR_HEADER);
    screen_st7789_draw_string(8, 8, title, UI_COLOR_TEXT, UI_COLOR_HEADER, 2);
    screen_st7789_draw_string(184, 10, right_text, UI_COLOR_TEXT, UI_COLOR_HEADER, 1);
    screen_st7789_fill_rect(0, 31, 240, 1, UI_COLOR_ACCENT);
}

static void app_ui_format_time(char *buffer, rt_size_t size, const device_data_snapshot_t *snapshot)
{
    if (snapshot->time_valid == 0U)
    {
        rt_snprintf(buffer, size, "NO TIME");
        return;
    }

    rt_snprintf(buffer,
                size,
                "%02u:%02u",
                snapshot->datetime.hour,
                snapshot->datetime.minute);
}

static void app_ui_draw_current_page(void);

void app_ui_init(void)
{
    app_key_input_init();
    app_ui_current_page = APP_UI_PAGE_HOME;
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
        if (app_ui_current_page == APP_UI_PAGE_HOME)
        {
            app_ui_current_page = (app_ui_page_t)(APP_UI_PAGE_COUNT - 1);
        }
        else
        {
            app_ui_current_page = (app_ui_page_t)(app_ui_current_page - 1);
        }

        app_ui_draw_current_page();
    }
    else if (event == APP_KEY_EVENT_DOWN || event == APP_KEY_EVENT_RIGHT)
    {
        app_ui_current_page = (app_ui_page_t)((app_ui_current_page + 1) % APP_UI_PAGE_COUNT);
        app_ui_draw_current_page();
    }
    else if (event == APP_KEY_EVENT_MID)
    {
        app_ui_draw_current_page();
    }
}

void app_ui_draw_home(void)
{
    device_data_snapshot_t snapshot;
    char peak_text[16];
    char temp_text[16];
    char humi_text[16];
    char time_text[12];
    const char *alarm_text;
    const char *tcp_text;
    rt_uint16_t alarm_color;
    rt_uint16_t tcp_color;

    data_service_get_snapshot(&snapshot);
    app_ui_format_fixed_1(peak_text, sizeof(peak_text), snapshot.surge_peak_value_milli_kv, "KV");
    app_ui_format_fixed_1(temp_text, sizeof(temp_text), snapshot.env_temp_milli_c, "C");
    app_ui_format_fixed_1(humi_text, sizeof(humi_text), snapshot.env_humi_milli_percent, "%");
    app_ui_format_time(time_text, sizeof(time_text), &snapshot);

    if (snapshot.alarm_state == DATA_SERVICE_ALARM_ACTIVE)
    {
        alarm_text = "ACTIVE";
        alarm_color = UI_COLOR_ALARM;
    }
    else
    {
        alarm_text = "NORMAL";
        alarm_color = UI_COLOR_OK;
    }

    if (snapshot.tcp_connected != 0U)
    {
        tcp_text = "TCP OK";
        tcp_color = UI_COLOR_OK;
    }
    else
    {
        tcp_text = "TCP WAIT";
        tcp_color = UI_COLOR_WARN;
    }

    screen_st7789_init();
    screen_st7789_fill_color(UI_COLOR_BG);
    app_ui_draw_header("SURGE LOGGER", time_text);

    screen_st7789_draw_string(12, 42, "PEAK", UI_COLOR_TEXT_MUTED, UI_COLOR_BG, 2);
    screen_st7789_draw_string(12, 66, peak_text, UI_COLOR_ACCENT, UI_COLOR_BG, 3);
    screen_st7789_draw_string(158, 74, "READY", UI_COLOR_OK, UI_COLOR_BG, 2);

    screen_st7789_fill_rect(10, 118, 220, 44, UI_COLOR_PANEL);
    app_ui_draw_label_value(20, 126, "TEMP", temp_text, UI_COLOR_TEXT);
    app_ui_draw_label_value(126, 126, "HUMI", humi_text, UI_COLOR_TEXT);

    screen_st7789_fill_rect(10, 170, 220, 32, UI_COLOR_PANEL_ALT);
    screen_st7789_draw_string(20, 178, "ALARM", UI_COLOR_TEXT_MUTED, UI_COLOR_PANEL_ALT, 1);
    screen_st7789_draw_string(70, 176, alarm_text, alarm_color, UI_COLOR_PANEL_ALT, 2);

    screen_st7789_draw_string(20, 207, snapshot.rs485_online ? "RS485 OK" : "RS485 WAIT", UI_COLOR_TEXT_MUTED, UI_COLOR_BG, 1);
    screen_st7789_draw_string(126, 207, tcp_text, tcp_color, UI_COLOR_BG, 1);

    app_ui_draw_footer("UP/DN PAGE  MID RFSH");
}

static void app_ui_draw_status(void)
{
    device_data_snapshot_t snapshot;
    char uptime_text[20];

    data_service_get_snapshot(&snapshot);
    rt_snprintf(uptime_text, sizeof(uptime_text), "%uMS", snapshot.timestamp_ms);

    screen_st7789_init();
    screen_st7789_fill_color(UI_COLOR_BG);
    app_ui_draw_header("STATUS", "2/3");

    screen_st7789_fill_rect(10, 46, 220, 42, UI_COLOR_PANEL);
    screen_st7789_draw_string(20, 56, "ALARM", UI_COLOR_TEXT_MUTED, UI_COLOR_PANEL, 1);
    screen_st7789_draw_string(88, 54,
                              (snapshot.alarm_state == DATA_SERVICE_ALARM_ACTIVE) ? "ACTIVE" : "NORMAL",
                              (snapshot.alarm_state == DATA_SERVICE_ALARM_ACTIVE) ? UI_COLOR_ALARM : UI_COLOR_OK,
                              UI_COLOR_PANEL,
                              2);

    screen_st7789_fill_rect(10, 98, 220, 42, UI_COLOR_PANEL_ALT);
    screen_st7789_draw_string(20, 108, "RS485", UI_COLOR_TEXT_MUTED, UI_COLOR_PANEL_ALT, 1);
    screen_st7789_draw_string(88, 106, snapshot.rs485_online ? "ONLINE" : "WAIT", UI_COLOR_TEXT, UI_COLOR_PANEL_ALT, 2);

    screen_st7789_fill_rect(10, 150, 220, 42, UI_COLOR_PANEL);
    screen_st7789_draw_string(20, 160, "TCP", UI_COLOR_TEXT_MUTED, UI_COLOR_PANEL, 1);
    screen_st7789_draw_string(88, 158, snapshot.tcp_connected ? "ONLINE" : "WAIT", snapshot.tcp_connected ? UI_COLOR_OK : UI_COLOR_WARN, UI_COLOR_PANEL, 2);

    screen_st7789_draw_string(14, 205, "UPTIME", UI_COLOR_TEXT_MUTED, UI_COLOR_BG, 1);
    screen_st7789_draw_string(74, 203, uptime_text, UI_COLOR_ACCENT, UI_COLOR_BG, 2);

    app_ui_draw_footer("UP/DN PAGE  MID RFSH");
}

static void app_ui_draw_about(void)
{
    screen_st7789_init();
    screen_st7789_fill_color(UI_COLOR_BG);
    app_ui_draw_header("DEVICE", "3/3");

    screen_st7789_draw_string(18, 50, "SURGE LOGGER", UI_COLOR_ACCENT, UI_COLOR_BG, 2);
    screen_st7789_draw_string(18, 78, "AT32F403ARGT7", UI_COLOR_TEXT, UI_COLOR_BG, 2);
    screen_st7789_draw_string(18, 106, "RT-THREAD BASE", UI_COLOR_TEXT, UI_COLOR_BG, 2);
    screen_st7789_draw_string(18, 134, "LCD ST7789 SPI2", UI_COLOR_TEXT, UI_COLOR_BG, 2);
    screen_st7789_draw_string(18, 162, "KEY UI EVENTS OK", UI_COLOR_OK, UI_COLOR_BG, 2);

    screen_st7789_fill_rect(18, 198, 204, 4, UI_COLOR_ACCENT);
    app_ui_draw_footer("UP/DN PAGE  MID RFSH");
}

static void app_ui_draw_current_page(void)
{
    if (app_ui_current_page == APP_UI_PAGE_STATUS)
    {
        app_ui_draw_status();
    }
    else if (app_ui_current_page == APP_UI_PAGE_ABOUT)
    {
        app_ui_draw_about();
    }
    else
    {
        app_ui_draw_home();
    }
}
