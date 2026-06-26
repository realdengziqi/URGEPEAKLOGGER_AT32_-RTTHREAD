#ifndef STORAGE_FLASHDB_H
#define STORAGE_FLASHDB_H

#include <rtthread.h>
#include <flashdb.h>

typedef enum
{
    STORAGE_FLASHDB_RECORD_KIND_ALARM = 0,
    STORAGE_FLASHDB_RECORD_KIND_SURGE
} storage_flashdb_record_kind_t;

rt_err_t storage_flashdb_init(void);
rt_bool_t storage_flashdb_ready(void);

fdb_kvdb_t storage_flashdb_get_param_kvdb(void);
fdb_tsdb_t storage_flashdb_get_alarm_tsdb(void);
fdb_tsdb_t storage_flashdb_get_surge_tsdb(void);

rt_err_t storage_flashdb_clear_records(storage_flashdb_record_kind_t kind);
rt_size_t storage_flashdb_record_count(storage_flashdb_record_kind_t kind);
rt_uint32_t storage_flashdb_last_time(void);

#endif
