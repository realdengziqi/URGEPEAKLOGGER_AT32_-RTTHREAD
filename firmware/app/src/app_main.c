#include <rtthread.h>

#include "app_backend_request.h"
#include "app_bringup.h"
#include "app_buzzer_service.h"
#include "app_data_service.h"
#include "app_factory_service.h"
#include "app_main.h"
#include "app_modbus_rtu.h"
#include "app_modbus_tcp.h"
#include "app_param_service.h"
#include "app_record_service.h"
#include "app_shell.h"

int app_main(void)
{
    rt_err_t data_service_result;
    rt_err_t param_service_result;
    rt_err_t record_service_result;
    rt_err_t backend_request_result;
    rt_err_t buzzer_service_result;
    rt_err_t factory_service_result;

    factory_service_result = factory_service_init();
    if (factory_service_result != RT_EOK)
    {
        rt_kprintf("[WARN] factory: no valid factory image, using runtime defaults/fallbacks\r\n");
    }
    else
    {
        rt_kprintf("[INFO] factory: ready storage=onchip_flash\r\n");
    }

    param_service_result = param_service_init();
    if (param_service_result != RT_EOK)
    {
        rt_kprintf("[ERROR] param_service: init failed %d\r\n", param_service_result);
    }
    else
    {
        rt_kprintf("[INFO] param_service: ready count=%u storage=eeprom_kvdb\r\n",
                   (rt_uint32_t)param_service_count());
    }

    record_service_result = record_service_init();
    if (record_service_result != RT_EOK)
    {
        rt_kprintf("[ERROR] record_service: init failed %d\r\n", record_service_result);
    }
    else
    {
        rt_kprintf("[INFO] record_service: ring buffer ready capacity=%u storage=flash_tsdb writer_prio=20 queue=16\r\n",
                   (rt_uint32_t)RECORD_SERVICE_CAPACITY);
    }

    buzzer_service_result = buzzer_service_init();
    if (buzzer_service_result != RT_EOK)
    {
        rt_kprintf("[ERROR] buzzer_service: init failed %d\r\n", buzzer_service_result);
    }
    else
    {
        rt_kprintf("[INFO] buzzer_service: timer=TMR7 tick=10ms alarm_interval=400ms pin=PB12\r\n");
    }

    data_service_result = data_service_init();
    if (data_service_result != RT_EOK)
    {
        rt_kprintf("[ERROR] data_service: init failed %d\r\n", data_service_result);
    }
    else
    {
        rt_kprintf("[INFO] data_service: thread started prio=18 stack=2048 period=200ms\r\n");
    }

    backend_request_result = backend_request_init();
    if (backend_request_result != RT_EOK)
    {
        rt_kprintf("[ERROR] backend_req: init failed %d\r\n", backend_request_result);
    }
    else
    {
        rt_kprintf("[INFO] backend_req: thread started prio=17 stack=1024 queue=8\r\n");
    }

    if (app_modbus_rtu_init() != RT_EOK)
    {
        rt_kprintf("[ERROR] modbus_rtu: thread create failed\r\n");
    }

    if (app_modbus_tcp_init() != RT_EOK)
    {
        rt_kprintf("[ERROR] modbus_tcp: thread create failed\r\n");
    }

    app_shell_init();

    rt_kprintf("[INFO] boot: SurgePeakLogger RT-Thread base project start.\r\n");
    rt_kprintf("[INFO] boot: backend-first mode, UI auto polling disabled.\r\n");
    rt_kprintf("[INFO] shell: type 'help' on COM12 for backend debug commands.\r\n");

    while (1)
    {
        app_bringup_poll();

        rt_thread_mdelay(20);
    }
}
