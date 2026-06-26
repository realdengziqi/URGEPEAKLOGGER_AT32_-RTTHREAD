#include "app_factory_service.h"

#include <string.h>

#include "fal.h"

#define FACTORY_SERVICE_MAGIC            0x46544359U
#define FACTORY_SERVICE_VERSION          1U
#define FACTORY_SERVICE_IMAGE_SIZE       64U

typedef struct
{
    rt_uint32_t magic;
    rt_uint16_t version;
    rt_uint16_t image_size;
    rt_uint8_t mac[6];
    rt_uint8_t mac_valid;
    rt_uint8_t reserved[45];
    rt_uint32_t checksum;
} factory_service_image_t;

static struct rt_mutex factory_service_lock;
static rt_bool_t factory_service_lock_ready = RT_FALSE;
static rt_bool_t factory_service_ready = RT_FALSE;
static factory_service_info_t factory_service_info;

static rt_uint32_t factory_service_checksum(const factory_service_image_t *image)
{
    const rt_uint8_t *bytes = (const rt_uint8_t *)image;
    rt_size_t length = sizeof(*image) - sizeof(image->checksum);
    rt_size_t i;
    rt_uint32_t checksum = 0x31415926U;

    for (i = 0U; i < length; i++)
    {
        checksum = (checksum << 7) | (checksum >> 25);
        checksum ^= bytes[i];
    }

    return checksum;
}

rt_bool_t factory_service_mac_is_valid(const rt_uint8_t mac[6])
{
    static const rt_uint8_t zero_mac[6] = {0};
    static const rt_uint8_t broadcast_mac[6] = {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU};

    if (mac == RT_NULL)
    {
        return RT_FALSE;
    }

    if (memcmp(mac, zero_mac, sizeof(zero_mac)) == 0 ||
        memcmp(mac, broadcast_mac, sizeof(broadcast_mac)) == 0)
    {
        return RT_FALSE;
    }

    /* bit0=1 means multicast, which is not valid for this device MAC. */
    return ((mac[0] & 0x01U) == 0U) ? RT_TRUE : RT_FALSE;
}

static rt_err_t factory_service_take(void)
{
    return rt_mutex_take(&factory_service_lock, RT_WAITING_FOREVER);
}

static void factory_service_release(void)
{
    rt_mutex_release(&factory_service_lock);
}

static void factory_service_clear_info_unlocked(void)
{
    memset(&factory_service_info, 0, sizeof(factory_service_info));
    factory_service_info.version = FACTORY_SERVICE_VERSION;
}

static const struct fal_partition *factory_service_find_part(void)
{
    if (fal_init() <= 0)
    {
        return RT_NULL;
    }

    return fal_partition_find(STORAGE_FLASHDB_FACTORY_PART_NAME);
}

static rt_err_t factory_service_load_unlocked(void)
{
    const struct fal_partition *part;
    factory_service_image_t image;

    factory_service_clear_info_unlocked();
    part = factory_service_find_part();
    if (part == RT_NULL)
    {
        return -RT_ERROR;
    }

    memset(&image, 0, sizeof(image));
    if (fal_partition_read(part, 0U, (rt_uint8_t *)&image, sizeof(image)) != (int)sizeof(image))
    {
        return -RT_ERROR;
    }

    if (image.magic != FACTORY_SERVICE_MAGIC ||
        image.version != FACTORY_SERVICE_VERSION ||
        image.image_size != (rt_uint16_t)sizeof(image) ||
        image.checksum != factory_service_checksum(&image))
    {
        return -RT_ERROR;
    }

    factory_service_info.version = image.version;
    if (image.mac_valid != 0U && factory_service_mac_is_valid(image.mac) != RT_FALSE)
    {
        memcpy(factory_service_info.mac, image.mac, sizeof(factory_service_info.mac));
        factory_service_info.mac_valid = 1U;
    }

    return RT_EOK;
}

rt_err_t factory_service_init(void)
{
    rt_err_t result;

    if (factory_service_ready != RT_FALSE)
    {
        return RT_EOK;
    }

    if (factory_service_lock_ready == RT_FALSE)
    {
        rt_mutex_init(&factory_service_lock, "factory", RT_IPC_FLAG_PRIO);
        factory_service_lock_ready = RT_TRUE;
    }

    if (factory_service_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    result = factory_service_load_unlocked();
    factory_service_ready = RT_TRUE;
    factory_service_release();

    return (result == RT_EOK) ? RT_EOK : -RT_ERROR;
}

rt_err_t factory_service_get_info(factory_service_info_t *info)
{
    if (info == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (factory_service_ready == RT_FALSE)
    {
        (void)factory_service_init();
    }

    if (factory_service_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    *info = factory_service_info;
    factory_service_release();

    return RT_EOK;
}

rt_err_t factory_service_get_mac(rt_uint8_t mac[6])
{
    if (mac == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (factory_service_ready == RT_FALSE)
    {
        (void)factory_service_init();
    }

    if (factory_service_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (factory_service_info.mac_valid == 0U)
    {
        factory_service_release();
        return -RT_ERROR;
    }

    memcpy(mac, factory_service_info.mac, sizeof(factory_service_info.mac));
    factory_service_release();

    return RT_EOK;
}

rt_err_t factory_service_set_mac(const rt_uint8_t mac[6])
{
    const struct fal_partition *part;
    factory_service_image_t image;

    if (factory_service_mac_is_valid(mac) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    part = factory_service_find_part();
    if (part == RT_NULL)
    {
        return -RT_ERROR;
    }

    memset(&image, 0xff, sizeof(image));
    image.magic = FACTORY_SERVICE_MAGIC;
    image.version = FACTORY_SERVICE_VERSION;
    image.image_size = (rt_uint16_t)sizeof(image);
    memcpy(image.mac, mac, sizeof(image.mac));
    image.mac_valid = 1U;
    image.checksum = factory_service_checksum(&image);

    if (factory_service_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (fal_partition_erase_all(part) < 0 ||
        fal_partition_write(part, 0U, (const rt_uint8_t *)&image, sizeof(image)) != (int)sizeof(image))
    {
        factory_service_release();
        return -RT_ERROR;
    }

    factory_service_info.version = FACTORY_SERVICE_VERSION;
    memcpy(factory_service_info.mac, mac, sizeof(factory_service_info.mac));
    factory_service_info.mac_valid = 1U;
    factory_service_ready = RT_TRUE;
    factory_service_release();

    return RT_EOK;
}

rt_err_t factory_service_clear(void)
{
    const struct fal_partition *part;

    part = factory_service_find_part();
    if (part == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (factory_service_take() != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (fal_partition_erase_all(part) < 0)
    {
        factory_service_release();
        return -RT_ERROR;
    }

    factory_service_clear_info_unlocked();
    factory_service_ready = RT_TRUE;
    factory_service_release();

    return RT_EOK;
}
