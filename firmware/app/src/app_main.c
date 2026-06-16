#include <rtthread.h>

#include "app_bringup.h"
#include "app_data_service.h"
#include "app_main.h"
#include "app_param_service.h"
#include "app_shell.h"
#include "app_ui.h"

int app_main(void)
{
    rt_err_t data_service_result;
    rt_err_t param_service_result;

    param_service_result = param_service_init();
    if (param_service_result != RT_EOK)
    {
        rt_kprintf("[ERROR] param_service: init failed %d\r\n", param_service_result);
    }
    else
    {
        rt_kprintf("[INFO] param_service: runtime defaults loaded count=%u\r\n",
                   (rt_uint32_t)param_service_count());
    }

    data_service_result = data_service_init();
    if (data_service_result != RT_EOK)
    {
        rt_kprintf("[ERROR] data_service: init failed %d\r\n", data_service_result);
    }
    else
    {
        rt_kprintf("[INFO] data_service: thread started prio=18 stack=768 period=200ms\r\n");
    }

    app_ui_init();
    app_shell_init();

    rt_kprintf("[INFO] boot: SurgePeakLogger RT-Thread base project start.\r\n");
    rt_kprintf("[INFO] shell: type 'help' on COM12 for debug commands.\r\n");

    while (1)
    {
        app_bringup_poll();
        app_ui_poll();

        rt_thread_mdelay(20);
    }
}
