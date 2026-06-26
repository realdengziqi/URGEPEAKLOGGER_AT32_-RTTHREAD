#include <fal.h>
#include <string.h>

#include "at32f403a_407_flash.h"

static rt_mutex_t onchip_flash_lock = RT_NULL;

static rt_err_t onchip_flash_take(void)
{
    if (onchip_flash_lock == RT_NULL)
    {
        onchip_flash_lock = rt_mutex_create("iflash", RT_IPC_FLAG_PRIO);
        if (onchip_flash_lock == RT_NULL)
        {
            return -RT_ENOMEM;
        }
    }

    return rt_mutex_take(onchip_flash_lock, RT_WAITING_FOREVER);
}

static void onchip_flash_release(void)
{
    if (onchip_flash_lock != RT_NULL)
    {
        rt_mutex_release(onchip_flash_lock);
    }
}

static rt_bool_t onchip_flash_range_valid(long offset, size_t size)
{
    if (offset < 0)
    {
        return RT_FALSE;
    }

    if ((rt_uint32_t)offset > STORAGE_FLASHDB_ONCHIP_SIZE_BYTES ||
        size > (STORAGE_FLASHDB_ONCHIP_SIZE_BYTES - (rt_uint32_t)offset))
    {
        return RT_FALSE;
    }

    return RT_TRUE;
}

static int fal_onchip_init(void)
{
    return (onchip_flash_take() == RT_EOK) ? (onchip_flash_release(), 0) : -1;
}

static int fal_onchip_read(long offset, rt_uint8_t *buf, size_t size)
{
    const void *src;

    if (buf == RT_NULL || onchip_flash_range_valid(offset, size) == RT_FALSE)
    {
        return -1;
    }

    src = (const void *)(STORAGE_FLASHDB_ONCHIP_BASE_ADDR + (rt_uint32_t)offset);
    memcpy(buf, src, size);

    return (int)size;
}

static int fal_onchip_write(long offset, const rt_uint8_t *buf, size_t size)
{
    rt_uint32_t address;
    size_t i;
    int result = (int)size;

    if (buf == RT_NULL || onchip_flash_range_valid(offset, size) == RT_FALSE)
    {
        return -1;
    }

    if (onchip_flash_take() != RT_EOK)
    {
        return -1;
    }

    address = STORAGE_FLASHDB_ONCHIP_BASE_ADDR + (rt_uint32_t)offset;
    flash_unlock();
    for (i = 0U; i < size; i++)
    {
        if (flash_byte_program(address + (rt_uint32_t)i, buf[i]) != FLASH_OPERATE_DONE)
        {
            result = -1;
            break;
        }
    }
    flash_lock();

    onchip_flash_release();
    return result;
}

static int fal_onchip_erase(long offset, size_t size)
{
    rt_uint32_t start;
    rt_uint32_t end;
    rt_uint32_t address;
    int result = (int)size;

    if (size == 0U || onchip_flash_range_valid(offset, size) == RT_FALSE)
    {
        return -1;
    }

    if ((((rt_uint32_t)offset) % STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES) != 0U ||
        (size % STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES) != 0U)
    {
        return -1;
    }

    if (onchip_flash_take() != RT_EOK)
    {
        return -1;
    }

    start = STORAGE_FLASHDB_ONCHIP_BASE_ADDR + (rt_uint32_t)offset;
    end = start + (rt_uint32_t)size;

    flash_unlock();
    for (address = start; address < end; address += STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES)
    {
        if (flash_sector_erase(address) != FLASH_OPERATE_DONE)
        {
            result = -1;
            break;
        }
    }
    flash_lock();

    onchip_flash_release();
    return result;
}

const struct fal_flash_dev fal_flash_onchip =
{
    .name = FAL_FLASH_ONCHIP_NAME,
    .addr = STORAGE_FLASHDB_ONCHIP_BASE_ADDR,
    .len = STORAGE_FLASHDB_ONCHIP_SIZE_BYTES,
    .blk_size = STORAGE_FLASHDB_ONCHIP_SECTOR_BYTES,
    .ops = {fal_onchip_init, fal_onchip_read, fal_onchip_write, fal_onchip_erase},
    .write_gran = 8,
};

