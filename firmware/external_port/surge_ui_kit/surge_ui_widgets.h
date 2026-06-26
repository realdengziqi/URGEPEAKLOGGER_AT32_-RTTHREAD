#ifndef SURGE_UI_WIDGETS_H
#define SURGE_UI_WIDGETS_H

#include "surge_ui_config.h"
#include "surge_ui_types.h"
#include "surge_ui_style.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SURGE_UI_STATUS_NORMAL = 0,
    SURGE_UI_STATUS_WARN,
    SURGE_UI_STATUS_ALARM
} surge_ui_status_t;

typedef enum
{
    SURGE_UI_MENU_SCROLL_HINT_ARROWS = 0,
    SURGE_UI_MENU_SCROLL_HINT_BAR
} surge_ui_menu_scroll_hint_t;

typedef enum
{
    SURGE_UI_LIST_MOVE_PREV = 0,
    SURGE_UI_LIST_MOVE_NEXT,
    SURGE_UI_LIST_MOVE_PAGE_PREV,
    SURGE_UI_LIST_MOVE_PAGE_NEXT,
    SURGE_UI_LIST_MOVE_FIRST,
    SURGE_UI_LIST_MOVE_LAST
} surge_ui_list_move_t;

typedef enum
{
    SURGE_UI_ZH_SIZE_18 = 18,
    SURGE_UI_ZH_SIZE_20 = 20,
    SURGE_UI_ZH_SIZE_24 = 24
} surge_ui_zh_size_t;

typedef struct
{
    surge_ui_zh_size_t zh_size;
    const UG_FONT *ascii_font;
} surge_ui_text_style_t;

typedef struct
{
    const char *label;
    const char *value;
} surge_ui_menu_item_t;

typedef struct
{
    rt_uint8_t item_count;
    rt_uint8_t visible_count;
    rt_uint8_t selected_index;
    rt_uint8_t top_index;
} surge_ui_list_state_t;

typedef struct
{
    const char *label;
    const char *value;
    const char *unit;
    surge_ui_status_t status;
} surge_ui_value_item_t;

typedef struct
{
    surge_ui_text_style_t title_text;
    surge_ui_text_style_t right_text;
    rt_int16_t title_x;
    rt_int16_t title_y;
    rt_int16_t right_x;
    rt_int16_t right_y;
} surge_ui_header_style_t;

typedef struct
{
    surge_ui_text_style_t back_text;
    surge_ui_text_style_t title_text;
    surge_ui_text_style_t right_text;
    rt_int16_t back_x;
    rt_int16_t back_y;
    rt_int16_t title_x;
    rt_int16_t title_y;
    rt_int16_t right_x;
    rt_int16_t right_y;
} surge_ui_nav_header_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t text_x;
    rt_int16_t text_y;
} surge_ui_footer_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t height;
    rt_int16_t marker_w;
    rt_int16_t marker_h;
    rt_int16_t marker_y;
    rt_int16_t text_x;
    rt_int16_t text_y;
    rt_int16_t line_gap;
    rt_int16_t line_y;
} surge_ui_section_title_style_t;

typedef struct
{
    surge_ui_text_style_t title_text;
    surge_ui_text_style_t message_text;
    rt_int16_t min_height;
    rt_int16_t icon_y;
    rt_int16_t icon_radius;
    rt_int16_t title_y;
    rt_int16_t message_y;
} surge_ui_empty_state_style_t;

typedef struct
{
    surge_ui_text_style_t title_text;
    surge_ui_text_style_t message_text;
    rt_int16_t min_height;
    rt_int16_t dots_y;
    rt_int16_t dot_radius;
    rt_int16_t dot_gap;
    rt_int16_t title_y;
    rt_int16_t message_y;
} surge_ui_loading_state_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t height;
    rt_int16_t text_y;
    rt_int16_t accent_w;
} surge_ui_toast_style_t;

typedef struct
{
    rt_int16_t height;
    rt_int16_t dot_y;
    rt_int16_t dot_radius;
    rt_int16_t dot_gap;
} surge_ui_page_indicator_style_t;

typedef struct
{
    surge_ui_text_style_t button_text;
    rt_int16_t height;
    rt_int16_t side_margin;
    rt_int16_t button_gap;
    rt_int16_t text_y;
} surge_ui_action_bar_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t height;
    rt_int16_t padding_x;
    rt_int16_t dot_x;
    rt_int16_t dot_y;
    rt_int16_t dot_radius;
    rt_int16_t text_y;
} surge_ui_save_status_chip_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    surge_ui_text_style_t unit_text;
    rt_int16_t min_height;
    rt_int16_t label_y;
    rt_int16_t value_y;
    rt_int16_t unit_y;
    rt_int16_t value_unit_gap;
    rt_int16_t padding_x;
} surge_ui_compact_value_tile_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t state_text;
    rt_int16_t min_height;
    rt_int16_t label_y;
    rt_int16_t state_y;
    rt_int16_t padding_x;
} surge_ui_compact_status_tile_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t height;
    rt_int16_t text_y;
} surge_ui_switch_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    surge_ui_text_style_t unit_text;
    surge_ui_text_style_t button_text;
    rt_int16_t height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t value_y;
    rt_int16_t unit_y;
    rt_int16_t button_w;
    rt_int16_t button_h;
    rt_int16_t button_y;
    rt_int16_t value_area_w;
    rt_int16_t value_unit_gap;
} surge_ui_stepper_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    rt_int16_t min_height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t chart_x;
    rt_int16_t chart_y;
    rt_int16_t chart_right_margin;
    rt_int16_t chart_bottom_margin;
    rt_uint8_t grid_count;
    rt_int16_t point_radius;
} surge_ui_mini_trend_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    surge_ui_text_style_t unit_text;
    rt_int16_t height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t value_x;
    rt_int16_t value_y;
    rt_int16_t unit_right_margin;
    rt_int16_t unit_y;
} surge_ui_value_tile_style_t;

typedef struct
{
    rt_int16_t gap;
    surge_ui_value_tile_style_t tile;
} surge_ui_value_pair_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    rt_int16_t height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t value_right_margin;
    rt_int16_t value_y;
} surge_ui_status_row_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    rt_int16_t dot_x;
    rt_int16_t dot_y;
    rt_int16_t dot_radius;
    rt_int16_t label_x;
    rt_int16_t label_y;
} surge_ui_status_dot_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t height;
    rt_int16_t text_x;
    rt_int16_t text_y;
} surge_ui_status_badge_style_t;

typedef struct
{
    surge_ui_text_style_t text;
    rt_int16_t height;
    rt_int16_t text_x;
    rt_int16_t text_y;
} surge_ui_alarm_banner_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    rt_int16_t height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t value_right_margin;
    rt_int16_t value_y;
} surge_ui_menu_row_style_t;

typedef struct
{
    surge_ui_menu_row_style_t row;
    surge_ui_text_style_t arrow_text;
    rt_int16_t arrow_right_margin;
    rt_int16_t arrow_y;
} surge_ui_menu_selected_row_style_t;

typedef struct
{
    surge_ui_menu_row_style_t row;
    surge_ui_menu_selected_row_style_t selected_row;
    surge_ui_text_style_t hint_text;
    rt_int16_t row_step;
    rt_int16_t scroll_bar_width;
} surge_ui_menu_list_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    surge_ui_text_style_t unit_text;
    rt_int16_t height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t value_box_right_margin;
    rt_int16_t value_box_width;
    rt_int16_t value_x;
    rt_int16_t value_y;
    rt_int16_t unit_right_margin;
    rt_int16_t unit_y;
} surge_ui_param_row_style_t;

typedef struct
{
    surge_ui_param_row_style_t row;
    surge_ui_text_style_t arrow_text;
    rt_int16_t left_arrow_right_margin;
    rt_int16_t right_arrow_right_margin;
    rt_int16_t arrow_y;
} surge_ui_param_edit_row_style_t;

typedef struct
{
    surge_ui_text_style_t index_text;
    surge_ui_text_style_t time_text;
    surge_ui_text_style_t value_text;
    surge_ui_text_style_t unit_text;
    rt_int16_t height;
    rt_int16_t dot_x;
    rt_int16_t dot_y;
    rt_int16_t dot_radius;
    rt_int16_t index_x;
    rt_int16_t index_y;
    rt_int16_t time_x;
    rt_int16_t time_y;
    rt_int16_t value_y;
    rt_int16_t value_gap;
    rt_int16_t unit_right_margin;
    rt_int16_t unit_y;
} surge_ui_record_row_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    surge_ui_text_style_t unit_text;
    rt_int16_t height;
    rt_int16_t label_y;
    rt_int16_t value_y;
    rt_int16_t unit_y;
    rt_int16_t value_unit_gap;
} surge_ui_detail_value_style_t;

typedef struct
{
    surge_ui_text_style_t label_text;
    surge_ui_text_style_t value_text;
    rt_int16_t height;
    rt_int16_t label_x;
    rt_int16_t label_y;
    rt_int16_t value_right_margin;
    rt_int16_t value_y;
} surge_ui_detail_field_row_style_t;

typedef struct
{
    surge_ui_text_style_t title_text;
    surge_ui_text_style_t message_text;
    surge_ui_text_style_t button_text;
    rt_int16_t height;
    rt_int16_t title_y;
    rt_int16_t message_y;
    rt_int16_t button_y;
    rt_int16_t button_gap;
    rt_int16_t button_height;
    rt_int16_t button_side_margin;
} surge_ui_confirm_dialog_style_t;

void surge_ui_begin(void);
void surge_ui_draw_text(rt_int16_t x,
                        rt_int16_t y,
                        const char *text,
                        UG_COLOR fg,
                        UG_COLOR bg,
                        const surge_ui_text_style_t *style);
void surge_ui_draw_text_18(rt_int16_t x,
                           rt_int16_t y,
                           const char *text,
                           UG_COLOR fg,
                           UG_COLOR bg,
                           const UG_FONT *ascii_font);
void surge_ui_draw_text_20(rt_int16_t x,
                           rt_int16_t y,
                           const char *text,
                           UG_COLOR fg,
                           UG_COLOR bg,
                           const UG_FONT *ascii_font);
void surge_ui_draw_text_24(rt_int16_t x,
                           rt_int16_t y,
                           const char *text,
                           UG_COLOR fg,
                           UG_COLOR bg,
                           const UG_FONT *ascii_font);
#if SURGE_UI_USE_HEADER
void surge_ui_draw_header(const char *title,
                          const char *right_text,
                          const surge_ui_header_style_t *style);
#endif
#if SURGE_UI_USE_NAV_HEADER
void surge_ui_draw_nav_header(const char *title,
                              const char *right_text,
                              const char *back_text,
                              const surge_ui_nav_header_style_t *style);
#endif
#if SURGE_UI_USE_FOOTER
void surge_ui_draw_footer(const char *text, const surge_ui_footer_style_t *style);
#endif
#if SURGE_UI_USE_SECTION_TITLE
void surge_ui_draw_section_title(rt_int16_t x,
                                 rt_int16_t y,
                                 rt_int16_t w,
                                 const char *title,
                                 surge_ui_status_t status,
                                 const surge_ui_section_title_style_t *style);
#endif
#if SURGE_UI_USE_EMPTY_STATE
void surge_ui_draw_empty_state(rt_int16_t x,
                               rt_int16_t y,
                               rt_int16_t w,
                               rt_int16_t h,
                               const char *title,
                               const char *message,
                               surge_ui_status_t status,
                               const surge_ui_empty_state_style_t *style);
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
                                 const surge_ui_loading_state_style_t *style);
#endif
#if SURGE_UI_USE_TOAST
void surge_ui_draw_toast(rt_int16_t x,
                         rt_int16_t y,
                         rt_int16_t w,
                         const char *text,
                         surge_ui_status_t status,
                         const surge_ui_toast_style_t *style);
#endif
#if SURGE_UI_USE_PAGE_INDICATOR
void surge_ui_draw_page_indicator(rt_int16_t x,
                                  rt_int16_t y,
                                  rt_int16_t w,
                                  rt_uint8_t current_page,
                                  rt_uint8_t page_count,
                                  const surge_ui_page_indicator_style_t *style);
#endif
#if SURGE_UI_USE_LIST_VIEW
void surge_ui_list_init(surge_ui_list_state_t *state,
                        rt_uint8_t item_count,
                        rt_uint8_t visible_count);
void surge_ui_list_set_item_count(surge_ui_list_state_t *state, rt_uint8_t item_count);
void surge_ui_list_set_visible_count(surge_ui_list_state_t *state, rt_uint8_t visible_count);
void surge_ui_list_set_selected(surge_ui_list_state_t *state, rt_uint8_t selected_index);
void surge_ui_list_move(surge_ui_list_state_t *state, surge_ui_list_move_t move);
void surge_ui_list_ensure_visible(surge_ui_list_state_t *state);
rt_uint8_t surge_ui_list_page_index(const surge_ui_list_state_t *state);
rt_uint8_t surge_ui_list_page_count(const surge_ui_list_state_t *state);
#endif
#if SURGE_UI_USE_ACTION_BAR
void surge_ui_draw_action_bar(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *left_text,
                              const char *right_text,
                              rt_uint8_t selected_button,
                              surge_ui_status_t status,
                              const surge_ui_action_bar_style_t *style);
#endif
#if SURGE_UI_USE_SAVE_STATUS_CHIP
void surge_ui_draw_save_status_chip(rt_int16_t x,
                                    rt_int16_t y,
                                    rt_int16_t w,
                                    const char *text,
                                    surge_ui_status_t status,
                                    const surge_ui_save_status_chip_style_t *style);
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
                                      const surge_ui_compact_value_tile_style_t *style);
#endif
#if SURGE_UI_USE_COMPACT_STATUS_TILE
void surge_ui_draw_compact_status_tile(rt_int16_t x,
                                       rt_int16_t y,
                                       rt_int16_t w,
                                       rt_int16_t h,
                                       const char *label,
                                       const char *state_text,
                                       surge_ui_status_t status,
                                       const surge_ui_compact_status_tile_style_t *style);
#endif
#if SURGE_UI_USE_SWITCH
void surge_ui_draw_switch(rt_int16_t x,
                          rt_int16_t y,
                          rt_int16_t w,
                          const char *left_text,
                          const char *right_text,
                          rt_uint8_t active_right,
                          surge_ui_status_t status,
                          const surge_ui_switch_style_t *style);
#endif
#if SURGE_UI_USE_STEPPER
void surge_ui_draw_stepper(rt_int16_t x,
                           rt_int16_t y,
                           rt_int16_t w,
                           const char *label,
                           const char *value,
                           const char *unit,
                           surge_ui_status_t status,
                           const surge_ui_stepper_style_t *style);
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
                              const surge_ui_mini_trend_style_t *style);
#endif
#if SURGE_UI_USE_VALUE_TILE
void surge_ui_draw_value_tile(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *label,
                              const char *value,
                              const char *unit,
                              surge_ui_status_t status,
                              const surge_ui_value_tile_style_t *style);
#endif
#if SURGE_UI_USE_VALUE_PAIR
void surge_ui_draw_value_pair(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const surge_ui_value_item_t *left,
                              const surge_ui_value_item_t *right,
                              const surge_ui_value_pair_style_t *style);
#endif
#if SURGE_UI_USE_STATUS_ROW
void surge_ui_draw_status_row(rt_int16_t x,
                              rt_int16_t y,
                              rt_int16_t w,
                              const char *label,
                              const char *value,
                              surge_ui_status_t status,
                              const surge_ui_status_row_style_t *style);
#endif
#if SURGE_UI_USE_STATUS_DOT
void surge_ui_draw_status_dot(rt_int16_t x,
                              rt_int16_t y,
                              const char *label,
                              surge_ui_status_t status,
                              const surge_ui_status_dot_style_t *style);
#endif
#if SURGE_UI_USE_STATUS_BADGE
void surge_ui_draw_status_badge(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                const char *text,
                                surge_ui_status_t status,
                                const surge_ui_status_badge_style_t *style);
#endif
#if SURGE_UI_USE_ALARM_BANNER
void surge_ui_draw_alarm_banner(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                const char *text,
                                surge_ui_status_t status,
                                const surge_ui_alarm_banner_style_t *style);
#endif
#if SURGE_UI_USE_PROGRESS_BAR
void surge_ui_draw_progress_bar(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                rt_uint8_t percent,
                                surge_ui_status_t status);
#endif
#if SURGE_UI_USE_MENU_ROW
void surge_ui_draw_menu_row(rt_int16_t x,
                            rt_int16_t y,
                            rt_int16_t w,
                            const char *label,
                            const char *value,
                            const surge_ui_menu_row_style_t *style);
#endif
#if SURGE_UI_USE_MENU_SELECTED_ROW
void surge_ui_draw_menu_selected_row(rt_int16_t x,
                                     rt_int16_t y,
                                     rt_int16_t w,
                                     const char *label,
                                     const char *value,
                                     const surge_ui_menu_selected_row_style_t *style);
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
                             const surge_ui_menu_list_style_t *style);
#endif
#if SURGE_UI_USE_PARAM_ROW
void surge_ui_draw_param_row(rt_int16_t x,
                             rt_int16_t y,
                             rt_int16_t w,
                             const char *label,
                             const char *value,
                             const char *unit,
                             rt_uint8_t selected,
                             const surge_ui_param_row_style_t *style);
#endif
#if SURGE_UI_USE_PARAM_EDIT_ROW
void surge_ui_draw_param_edit_row(rt_int16_t x,
                                  rt_int16_t y,
                                  rt_int16_t w,
                                  const char *label,
                                  const char *value,
                                  const char *unit,
                                  const surge_ui_param_edit_row_style_t *style);
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
                              const surge_ui_record_row_style_t *style);
#endif
#if SURGE_UI_USE_DETAIL_VALUE
void surge_ui_draw_detail_value(rt_int16_t x,
                                rt_int16_t y,
                                rt_int16_t w,
                                const char *label,
                                const char *value,
                                const char *unit,
                                surge_ui_status_t status,
                                const surge_ui_detail_value_style_t *style);
#endif
#if SURGE_UI_USE_DETAIL_FIELD_ROW
void surge_ui_draw_detail_field_row(rt_int16_t x,
                                    rt_int16_t y,
                                    rt_int16_t w,
                                    const char *label,
                                    const char *value,
                                    const surge_ui_detail_field_row_style_t *style);
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
                                  const surge_ui_confirm_dialog_style_t *style);
#endif

#ifdef __cplusplus
}
#endif

#endif
