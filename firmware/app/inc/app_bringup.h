#ifndef APP_BRINGUP_H
#define APP_BRINGUP_H

#include <rtthread.h>

void app_bringup_print_help(void);
rt_bool_t app_bringup_process_command(const char *cmd);
void app_bringup_poll(void);

#endif
