#ifndef APP_KEY_INPUT_H
#define APP_KEY_INPUT_H

#include <rtthread.h>

typedef enum
{
    APP_KEY_EVENT_NONE = 0,
    APP_KEY_EVENT_DOWN,
    APP_KEY_EVENT_LEFT,
    APP_KEY_EVENT_RIGHT,
    APP_KEY_EVENT_UP,
    APP_KEY_EVENT_MID
} app_key_event_t;

void app_key_input_init(void);
app_key_event_t app_key_input_poll(void);

#endif
