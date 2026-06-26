#ifndef FAL_CFG_H
#define FAL_CFG_H

#include <rtthread.h>
#include "fal_def.h"

#define FAL_PART_HAS_TABLE_CFG
#define FAL_DEBUG                      0

#define FAL_FLASH_AT24C128_NAME        "at24c128"
#define FAL_FLASH_ONCHIP_NAME          "onchip"

#define STORAGE_FLASHDB_PARAM_KVDB_PART_NAME   "param_kvdb"
#define STORAGE_FLASHDB_ALARM_TSDB_PART_NAME   "alarm_tsdb"
#define STORAGE_FLASHDB_SURGE_TSDB_PART_NAME   "surge_tsdb"
#define STORAGE_FLASHDB_FACTORY_PART_NAME      "factory"

#define STORAGE_FLASHDB_ONCHIP_BASE_ADDR        0x08000000UL
#define STORAGE_FLASHDB_ONCHIP_SIZE_BYTES       (1024UL * 1024UL)
#define STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES     2048UL

/*
 * 片内 Flash 最后 256KB 预留给持久化数据：
 * - alarm_tsdb: 64KB，报警/普通事件
 * - surge_tsdb: 190KB，浪涌事件
 * - factory: 2KB，出厂身份信息，当前用于 MAC 地址
 */
#define STORAGE_FLASHDB_LOG_REGION_OFFSET       (768UL * 1024UL)
#define STORAGE_FLASHDB_ALARM_TSDB_OFFSET       STORAGE_FLASHDB_LOG_REGION_OFFSET
#define STORAGE_FLASHDB_ALARM_TSDB_SIZE         (64UL * 1024UL)
#define STORAGE_FLASHDB_SURGE_TSDB_OFFSET       (STORAGE_FLASHDB_ALARM_TSDB_OFFSET + STORAGE_FLASHDB_ALARM_TSDB_SIZE)
#define STORAGE_FLASHDB_FACTORY_SIZE            STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES
#define STORAGE_FLASHDB_SURGE_TSDB_SIZE         ((192UL * 1024UL) - STORAGE_FLASHDB_FACTORY_SIZE)
#define STORAGE_FLASHDB_FACTORY_OFFSET          (STORAGE_FLASHDB_SURGE_TSDB_OFFSET + STORAGE_FLASHDB_SURGE_TSDB_SIZE)

extern const struct fal_flash_dev fal_flash_at24c128;
extern const struct fal_flash_dev fal_flash_onchip;

#define FAL_FLASH_DEV_TABLE                                      \
{                                                                \
    &fal_flash_at24c128,                                         \
    &fal_flash_onchip,                                           \
}

#define FAL_PART_TABLE                                                                                         \
{                                                                                                              \
    {FAL_PART_MAGIC_WORD, STORAGE_FLASHDB_PARAM_KVDB_PART_NAME, FAL_FLASH_AT24C128_NAME, 0, 16UL * 1024UL, 0}, \
    {FAL_PART_MAGIC_WORD, STORAGE_FLASHDB_ALARM_TSDB_PART_NAME, FAL_FLASH_ONCHIP_NAME,                         \
     STORAGE_FLASHDB_ALARM_TSDB_OFFSET, STORAGE_FLASHDB_ALARM_TSDB_SIZE, 0},                                   \
    {FAL_PART_MAGIC_WORD, STORAGE_FLASHDB_SURGE_TSDB_PART_NAME, FAL_FLASH_ONCHIP_NAME,                         \
     STORAGE_FLASHDB_SURGE_TSDB_OFFSET, STORAGE_FLASHDB_SURGE_TSDB_SIZE, 0},                                   \
    {FAL_PART_MAGIC_WORD, STORAGE_FLASHDB_FACTORY_PART_NAME, FAL_FLASH_ONCHIP_NAME,                            \
     STORAGE_FLASHDB_FACTORY_OFFSET, STORAGE_FLASHDB_FACTORY_SIZE, 0},                                         \
}

#endif
