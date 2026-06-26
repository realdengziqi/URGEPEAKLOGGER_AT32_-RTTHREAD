#include <fal.h>
#include <string.h>

#include "storage_at24c128.h"

#define AT24C128_FAL_BLOCK_SIZE    1024U

static int fal_at24c128_init(void)
{
    return (storage_at24c128_init() == RT_EOK) ? 0 : -1;
}

static int fal_at24c128_read(long offset, rt_uint8_t *buf, size_t size)
{
    if (offset < 0 || buf == RT_NULL)
    {
        return -1;
    }

    if (storage_at24c128_read((rt_uint16_t)offset, buf, (rt_size_t)size) != RT_EOK)
    {
        return -1;
    }

    return (int)size;
}

static int fal_at24c128_write(long offset, const rt_uint8_t *buf, size_t size)
{
    if (offset < 0 || buf == RT_NULL)
    {
        return -1;
    }

    if (storage_at24c128_write((rt_uint16_t)offset, buf, (rt_size_t)size) != RT_EOK)
    {
        return -1;
    }

    return (int)size;
}

static int fal_at24c128_erase(long offset, size_t size)
{
    rt_uint8_t erase_buf[STORAGE_AT24C128_PAGE_SIZE_BYTES];
    size_t written = 0U;

    if (offset < 0)
    {
        return -1;
    }

    memset(erase_buf, 0xFF, sizeof(erase_buf));
    while (written < size)
    {
        size_t chunk = size - written;
        if (chunk > sizeof(erase_buf))
        {
            chunk = sizeof(erase_buf);
        }

        if (storage_at24c128_write((rt_uint16_t)((rt_uint32_t)offset + written),
                                   erase_buf,
                                   (rt_size_t)chunk) != RT_EOK)
        {
            return -1;
        }

        written += chunk;
    }

    return (int)size;
}

const struct fal_flash_dev fal_flash_at24c128 =
{
    .name = FAL_FLASH_AT24C128_NAME,
    .addr = 0,
    .len = STORAGE_AT24C128_SIZE_BYTES,
    .blk_size = AT24C128_FAL_BLOCK_SIZE,
    .ops = {fal_at24c128_init, fal_at24c128_read, fal_at24c128_write, fal_at24c128_erase},
    .write_gran = 8,
};

