#ifndef STORAGE_AT24C128_H
#define STORAGE_AT24C128_H

#include <rtthread.h>

#define STORAGE_AT24C128_I2C_ADDR_7BIT      0x50U
#define STORAGE_AT24C128_SIZE_BYTES         16384U
#define STORAGE_AT24C128_PAGE_SIZE_BYTES    64U

rt_err_t storage_at24c128_init(void);
rt_err_t storage_at24c128_read(rt_uint16_t address, rt_uint8_t *data, rt_size_t length);
rt_err_t storage_at24c128_write(rt_uint16_t address, const rt_uint8_t *data, rt_size_t length);
rt_err_t storage_at24c128_ready(void);

#endif
