#ifndef FDB_CFG_H
#define FDB_CFG_H

#include <rtthread.h>

#define FDB_USING_KVDB
#define FDB_USING_TSDB
#define FDB_USING_FAL_MODE

/* AT24C128 和 AT32F403A 片内 Flash 都按 byte program 方式接入。 */
#define FDB_WRITE_GRAN                 8

#define FDB_KV_NAME_MAX                32
#define FDB_KV_CACHE_TABLE_SIZE        8
#define FDB_SECTOR_CACHE_TABLE_SIZE    4

#define FDB_PRINT(...)                 rt_kprintf(__VA_ARGS__)

#endif

