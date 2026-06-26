#include "port.h"
#include "mb.h"
#include "mbport.h"

static struct rt_event slave_event;
static BOOL slave_event_ready;

BOOL xMBPortEventInit(void)
{
    if (slave_event_ready == FALSE)
    {
        rt_event_init(&slave_event, "mb_evt", RT_IPC_FLAG_PRIO);
        slave_event_ready = TRUE;
    }
    else
    {
        (void)rt_event_control(&slave_event, RT_IPC_CMD_RESET, RT_NULL);
    }

    return TRUE;
}

BOOL xMBPortEventPost(eMBEventType event)
{
    return (rt_event_send(&slave_event, (rt_uint32_t)event) == RT_EOK) ? TRUE : FALSE;
}

BOOL xMBPortEventGet(eMBEventType *event)
{
    rt_uint32_t received;

    if (event == RT_NULL)
    {
        return FALSE;
    }

    if (rt_event_recv(&slave_event,
                      EV_READY | EV_FRAME_RECEIVED | EV_EXECUTE | EV_FRAME_SENT,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER,
                      &received) != RT_EOK)
    {
        return FALSE;
    }

    *event = (eMBEventType)received;
    return TRUE;
}
