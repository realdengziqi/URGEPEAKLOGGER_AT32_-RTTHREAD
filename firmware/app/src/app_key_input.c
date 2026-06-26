#include "app_key_input.h"

#include "app_buzzer_service.h"
#include "board.h"

#define APP_KEY_DEBOUNCE_SAMPLES         2U

static rt_uint32_t key_last_sample;
static rt_uint32_t key_stable_state;
static rt_uint32_t key_reported_state;
static rt_uint8_t key_same_count;

static app_key_event_t app_key_pick_pressed_event(rt_uint32_t pressed)
{
    if ((pressed & BOARD_KEY_UP_MASK) != 0U)
    {
        return APP_KEY_EVENT_UP;
    }
    if ((pressed & BOARD_KEY_DOWN_MASK) != 0U)
    {
        return APP_KEY_EVENT_DOWN;
    }
    if ((pressed & BOARD_KEY_LEFT_MASK) != 0U)
    {
        return APP_KEY_EVENT_LEFT;
    }
    if ((pressed & BOARD_KEY_RIGHT_MASK) != 0U)
    {
        return APP_KEY_EVENT_RIGHT;
    }
    if ((pressed & BOARD_KEY_MID_MASK) != 0U)
    {
        return APP_KEY_EVENT_MID;
    }

    return APP_KEY_EVENT_NONE;
}

void app_key_input_init(void)
{
    key_last_sample = board_interaction_key_read();
    key_stable_state = key_last_sample;
    key_reported_state = key_stable_state;
    key_same_count = 0U;
}

app_key_event_t app_key_input_poll(void)
{
    rt_uint32_t sample;
    rt_uint32_t newly_pressed;
    app_key_event_t event;

    sample = board_interaction_key_read();

    if (sample == key_last_sample)
    {
        if (key_same_count < APP_KEY_DEBOUNCE_SAMPLES)
        {
            key_same_count++;
        }
    }
    else
    {
        key_last_sample = sample;
        key_same_count = 0U;
        return APP_KEY_EVENT_NONE;
    }

    if ((key_same_count < APP_KEY_DEBOUNCE_SAMPLES) || (sample == key_stable_state))
    {
        return APP_KEY_EVENT_NONE;
    }

    key_stable_state = sample;
    newly_pressed = key_stable_state & (~key_reported_state);
    key_reported_state = key_stable_state;

    event = app_key_pick_pressed_event(newly_pressed);
    if (event != APP_KEY_EVENT_NONE)
    {
        (void)buzzer_service_beep(0U);
    }

    return event;
}
