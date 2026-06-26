#ifndef UGUI_PORT_H
#define UGUI_PORT_H

#include <rtthread.h>
#include "ugui.h"

#ifdef __cplusplus
extern "C" {
#endif

void ugui_port_init(void);
void ugui_port_clear(UG_COLOR color);
rt_bool_t ugui_port_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif
