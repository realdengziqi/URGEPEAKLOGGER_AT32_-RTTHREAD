#include "app_bringup.h"

#include <string.h>

#include "board.h"
#include "net_w5500.h"
#include "sensor_hdc1080.h"
#include "storage_at24c128.h"

#if defined(SURGE_ENABLE_UI) && (SURGE_ENABLE_UI != 0)
#include "app_ui.h"
#include "screen_st7789.h"
#endif

#define KEY_WATCH_PERIOD_MS              50U
#define KEY_WATCH_IDLE_STATE             0xFFFFFFFFU

static rt_bool_t key_watch_enabled = RT_FALSE;
static rt_uint32_t key_watch_last_state = KEY_WATCH_IDLE_STATE;

static const char *app_bringup_skip_spaces(const char *text)
{
    while (text != RT_NULL && *text == ' ')
    {
        text++;
    }

    return text;
}

static rt_bool_t app_bringup_parse_u32_token(const char **cursor, rt_uint32_t *value)
{
    const char *p;
    rt_uint32_t result = 0U;
    rt_uint32_t base = 10U;
    rt_bool_t has_digit = RT_FALSE;

    if (cursor == RT_NULL || *cursor == RT_NULL || value == RT_NULL)
    {
        return RT_FALSE;
    }

    p = app_bringup_skip_spaces(*cursor);
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        base = 16U;
        p += 2;
    }

    while (*p != '\0' && *p != ' ')
    {
        rt_uint32_t digit;

        if (*p >= '0' && *p <= '9')
        {
            digit = (rt_uint32_t)(*p - '0');
        }
        else if (*p >= 'a' && *p <= 'f')
        {
            digit = (rt_uint32_t)(*p - 'a' + 10);
        }
        else if (*p >= 'A' && *p <= 'F')
        {
            digit = (rt_uint32_t)(*p - 'A' + 10);
        }
        else
        {
            return RT_FALSE;
        }

        if (digit >= base)
        {
            return RT_FALSE;
        }

        result = (result * base) + digit;
        has_digit = RT_TRUE;
        p++;
    }

    if (has_digit == RT_FALSE)
    {
        return RT_FALSE;
    }

    *value = result;
    *cursor = p;
    return RT_TRUE;
}

static rt_bool_t app_bringup_hex_digit(char ch, rt_uint8_t *value)
{
    if (value == RT_NULL)
    {
        return RT_FALSE;
    }

    if (ch >= '0' && ch <= '9')
    {
        *value = (rt_uint8_t)(ch - '0');
        return RT_TRUE;
    }

    if (ch >= 'a' && ch <= 'f')
    {
        *value = (rt_uint8_t)(ch - 'a' + 10);
        return RT_TRUE;
    }

    if (ch >= 'A' && ch <= 'F')
    {
        *value = (rt_uint8_t)(ch - 'A' + 10);
        return RT_TRUE;
    }

    return RT_FALSE;
}

static rt_bool_t app_bringup_parse_hex_bytes(const char *text,
                                             rt_uint8_t *data,
                                             rt_size_t max_length,
                                             rt_size_t *length)
{
    rt_size_t count = 0U;
    const char *p;

    if (text == RT_NULL || data == RT_NULL || length == RT_NULL)
    {
        return RT_FALSE;
    }

    p = app_bringup_skip_spaces(text);
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        p += 2;
    }

    while (*p != '\0' && *p != ' ')
    {
        rt_uint8_t high;
        rt_uint8_t low;

        if (count >= max_length ||
            app_bringup_hex_digit(p[0], &high) == RT_FALSE ||
            app_bringup_hex_digit(p[1], &low) == RT_FALSE)
        {
            return RT_FALSE;
        }

        data[count++] = (rt_uint8_t)((high << 4) | low);
        p += 2;
    }

    if (count == 0U || *app_bringup_skip_spaces(p) != '\0')
    {
        return RT_FALSE;
    }

    *length = count;
    return RT_TRUE;
}

static void app_bringup_print_hex_line(const rt_uint8_t *data, rt_size_t length)
{
    rt_size_t i;

    for (i = 0U; i < length; i++)
    {
        rt_kprintf("%02x%s", data[i], ((i + 1U) == length) ? "\r\n" : " ");
    }
}

static void app_bringup_print_key_state(const char *tag, rt_uint32_t keys)
{
    rt_kprintf("[INFO] %s: raw=0x%02x down=%u left=%u right=%u up=%u mid=%u\r\n",
               tag,
               keys,
               (keys & BOARD_KEY_DOWN_MASK) ? 1U : 0U,
               (keys & BOARD_KEY_LEFT_MASK) ? 1U : 0U,
               (keys & BOARD_KEY_RIGHT_MASK) ? 1U : 0U,
               (keys & BOARD_KEY_UP_MASK) ? 1U : 0U,
               (keys & BOARD_KEY_MID_MASK) ? 1U : 0U);
}

void app_bringup_print_help(void)
{
    rt_kprintf("[INFO] bringup: key, key watch|key watch off\r\n");
    rt_kprintf("[INFO] bringup: led data|test on|off|toggle\r\n");
#if defined(SURGE_ENABLE_UI) && (SURGE_ENABLE_UI != 0)
    rt_kprintf("[INFO] bringup: lcd init|red|green|blue|black|white|demo|ui|backlight on|backlight off\r\n");
    rt_kprintf("[INFO] bringup: lcd bus status\r\n");
#endif
    rt_kprintf("[INFO] bringup: hdc1080 init|id|read\r\n");
    rt_kprintf("[INFO] bringup: eeprom init|info|read <addr> <len>|write <addr> <bytes...>|writehex <addr> <hex>|test [addr]\r\n");
    rt_kprintf("[INFO] bringup: w5500 init|status|reg <addr>|write <addr> <value>\r\n");
}

rt_bool_t app_bringup_process_command(const char *cmd)
{
    if (strcmp(cmd, "key") == 0)
    {
        rt_uint32_t keys = board_interaction_key_read();

        app_bringup_print_key_state("key", keys);
        return RT_TRUE;
    }
    else if (strcmp(cmd, "key watch") == 0 || strcmp(cmd, "key watch on") == 0)
    {
        rt_uint32_t keys = board_interaction_key_read();

        key_watch_enabled = RT_TRUE;
        key_watch_last_state = keys;
        rt_kprintf("[INFO] key watch: on\r\n");
        app_bringup_print_key_state("key watch", keys);
        return RT_TRUE;
    }
    else if (strcmp(cmd, "key watch off") == 0)
    {
        key_watch_enabled = RT_FALSE;
        key_watch_last_state = KEY_WATCH_IDLE_STATE;
        rt_kprintf("[INFO] key watch: off\r\n");
        return RT_TRUE;
    }
    else if (strncmp(cmd, "led ", 4) == 0)
    {
        board_debug_led_t led;
        const char *action;

        if (strncmp(cmd + 4, "data ", 5) == 0)
        {
            led = BOARD_DEBUG_LED_DATA;
            action = cmd + 9;
        }
        else if (strncmp(cmd + 4, "test ", 5) == 0)
        {
            led = BOARD_DEBUG_LED_TEST;
            action = cmd + 9;
        }
        else
        {
            rt_kprintf("[WARN] cli: led target must be data or test\r\n");
            return RT_TRUE;
        }

        if (strcmp(action, "on") == 0)
        {
            board_debug_led_set(led, RT_TRUE);
            rt_kprintf("[INFO] led: %s on\r\n", (led == BOARD_DEBUG_LED_DATA) ? "data" : "test");
        }
        else if (strcmp(action, "off") == 0)
        {
            board_debug_led_set(led, RT_FALSE);
            rt_kprintf("[INFO] led: %s off\r\n", (led == BOARD_DEBUG_LED_DATA) ? "data" : "test");
        }
        else if (strcmp(action, "toggle") == 0)
        {
            board_debug_led_toggle(led);
            rt_kprintf("[INFO] led: %s toggled\r\n", (led == BOARD_DEBUG_LED_DATA) ? "data" : "test");
        }
        else
        {
            rt_kprintf("[WARN] cli: led action must be on, off, or toggle\r\n");
        }

        return RT_TRUE;
    }
    else if (strncmp(cmd, "lcd", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' '))
    {
#if defined(SURGE_ENABLE_UI) && (SURGE_ENABLE_UI != 0)
        if (strcmp(cmd, "lcd init") == 0)
        {
            screen_st7789_init();
            screen_st7789_fill_color(SCREEN_ST7789_COLOR_BLACK);
            rt_kprintf("[INFO] lcd: init done, filled black\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd red") == 0)
        {
            screen_st7789_fill_color(SCREEN_ST7789_COLOR_RED);
            rt_kprintf("[INFO] lcd: filled red\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd green") == 0)
        {
            screen_st7789_fill_color(SCREEN_ST7789_COLOR_GREEN);
            rt_kprintf("[INFO] lcd: filled green\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd blue") == 0)
        {
            screen_st7789_fill_color(SCREEN_ST7789_COLOR_BLUE);
            rt_kprintf("[INFO] lcd: filled blue\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd black") == 0)
        {
            screen_st7789_fill_color(SCREEN_ST7789_COLOR_BLACK);
            rt_kprintf("[INFO] lcd: filled black\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd white") == 0)
        {
            screen_st7789_fill_color(SCREEN_ST7789_COLOR_WHITE);
            rt_kprintf("[INFO] lcd: filled white\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd demo") == 0)
        {
            screen_st7789_draw_demo();
            rt_kprintf("[INFO] lcd: drew demo screen\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd ui") == 0)
        {
            app_ui_draw_home();
            rt_kprintf("[INFO] lcd: drew ui home screen\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd backlight on") == 0)
        {
            screen_st7789_gpio_init();
            screen_st7789_backlight_set(RT_TRUE);
            rt_kprintf("[INFO] lcd: backlight on\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd backlight off") == 0)
        {
            screen_st7789_gpio_init();
            screen_st7789_backlight_set(RT_FALSE);
            rt_kprintf("[INFO] lcd: backlight off\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd bus normal") == 0)
        {
            screen_st7789_bus_swap_set(RT_FALSE);
            rt_kprintf("[WARN] lcd: bus is fixed to hardware SPI2, sck=PB13 mosi=PB15\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd bus swap") == 0)
        {
            screen_st7789_bus_swap_set(RT_TRUE);
            rt_kprintf("[WARN] lcd: bus swap is disabled, using hardware SPI2 sck=PB13 mosi=PB15\r\n");
            return RT_TRUE;
        }
        else if (strcmp(cmd, "lcd bus status") == 0)
        {
            rt_kprintf("[INFO] lcd: bus hardware spi2, sck=PB13 mosi=PB15 mode=3 div=8\r\n");
            return RT_TRUE;
        }
#else
        rt_kprintf("[WARN] lcd: UI/LCD code is not linked in this backend target\r\n");
        return RT_TRUE;
#endif

        rt_kprintf("[WARN] cli: unsupported lcd command\r\n");
        return RT_TRUE;
    }
    else if (strcmp(cmd, "hdc1080") == 0)
    {
        rt_kprintf("[INFO] hdc1080: usage hdc1080 init|id|read\r\n");
        return RT_TRUE;
    }
    else if (strcmp(cmd, "hdc1080 init") == 0)
    {
        rt_err_t result = sensor_hdc1080_init();

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] hdc1080: init ok i2c1 addr=0x%02x scl=PB6 sda=PB7\r\n",
                       SENSOR_HDC1080_I2C_ADDR_7BIT);
        }
        else
        {
            rt_kprintf("[WARN] hdc1080: init failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strcmp(cmd, "hdc1080 id") == 0)
    {
        rt_uint16_t manufacturer_id;
        rt_uint16_t device_id;
        rt_err_t result = sensor_hdc1080_read_ids(&manufacturer_id, &device_id);

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] hdc1080: manufacturer=0x%04x device=0x%04x expected=0x%04x/0x%04x\r\n",
                       manufacturer_id,
                       device_id,
                       SENSOR_HDC1080_EXPECTED_MANUFACTURER,
                       SENSOR_HDC1080_EXPECTED_DEVICE_ID);
        }
        else
        {
            rt_kprintf("[WARN] hdc1080: id read failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strcmp(cmd, "hdc1080 read") == 0)
    {
        sensor_hdc1080_sample_t sample;
        rt_err_t result = sensor_hdc1080_read_sample(&sample);

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] hdc1080: temp_mc=%d humi_mpermil=%d raw_temp=0x%04x raw_humi=0x%04x\r\n",
                       sample.temperature_milli_c,
                       sample.humidity_milli_percent,
                       sample.raw_temperature,
                       sample.raw_humidity);
        }
        else
        {
            rt_kprintf("[WARN] hdc1080: sample read failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strcmp(cmd, "eeprom") == 0 || strcmp(cmd, "eeprom info") == 0)
    {
        rt_kprintf("[INFO] eeprom: AT24C128C size=%u bytes page=%u i2c2 addr=0x%02x scl=PB10 sda=PB11\r\n",
                   STORAGE_AT24C128_SIZE_BYTES,
                   STORAGE_AT24C128_PAGE_SIZE_BYTES,
                   STORAGE_AT24C128_I2C_ADDR_7BIT);
        rt_kprintf("[INFO] eeprom: usage eeprom init|read <addr> <len>|write <addr> <bytes...>|writehex <addr> <hex>|test [addr]\r\n");
        return RT_TRUE;
    }
    else if (strcmp(cmd, "eeprom init") == 0)
    {
        rt_err_t result = storage_at24c128_init();

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] eeprom: init ok i2c2 addr=0x%02x size=%u\r\n",
                       STORAGE_AT24C128_I2C_ADDR_7BIT,
                       STORAGE_AT24C128_SIZE_BYTES);
        }
        else
        {
            rt_kprintf("[WARN] eeprom: init failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strncmp(cmd, "eeprom read ", 12) == 0)
    {
        const char *cursor = cmd + 12;
        rt_uint32_t address;
        rt_uint32_t length;
        rt_uint8_t data[32];
        rt_err_t result;

        if (app_bringup_parse_u32_token(&cursor, &address) == RT_FALSE ||
            app_bringup_parse_u32_token(&cursor, &length) == RT_FALSE ||
            length == 0U || length > sizeof(data) ||
            address >= STORAGE_AT24C128_SIZE_BYTES ||
            length > (STORAGE_AT24C128_SIZE_BYTES - address))
        {
            rt_kprintf("[WARN] eeprom: usage eeprom read <addr> <len<=32>\r\n");
            return RT_TRUE;
        }

        result = storage_at24c128_read((rt_uint16_t)address, data, (rt_size_t)length);
        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] eeprom: read addr=0x%04x len=%u data=",
                       address,
                       length);
            app_bringup_print_hex_line(data, (rt_size_t)length);
        }
        else
        {
            rt_kprintf("[WARN] eeprom: read failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strncmp(cmd, "eeprom write ", 13) == 0)
    {
        const char *cursor = cmd + 13;
        rt_uint32_t address;
        rt_uint32_t value;
        rt_uint8_t data[16];
        rt_size_t length = 0U;
        rt_err_t result;

        if (app_bringup_parse_u32_token(&cursor, &address) == RT_FALSE ||
            address >= STORAGE_AT24C128_SIZE_BYTES)
        {
            rt_kprintf("[WARN] eeprom: usage eeprom write <addr> <byte... up to 16>\r\n");
            return RT_TRUE;
        }

        while (app_bringup_parse_u32_token(&cursor, &value) == RT_TRUE)
        {
            if (length >= sizeof(data) || value > 0xFFU)
            {
                rt_kprintf("[WARN] eeprom: byte count/value out of range\r\n");
                return RT_TRUE;
            }
            data[length++] = (rt_uint8_t)value;
        }

        if (length == 0U || length > (STORAGE_AT24C128_SIZE_BYTES - address))
        {
            rt_kprintf("[WARN] eeprom: usage eeprom write <addr> <byte... up to 16>\r\n");
            return RT_TRUE;
        }

        result = storage_at24c128_write((rt_uint16_t)address, data, length);
        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] eeprom: write ok addr=0x%04x len=%u\r\n", address, (rt_uint32_t)length);
        }
        else
        {
            rt_kprintf("[WARN] eeprom: write failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strncmp(cmd, "eeprom writehex ", 16) == 0)
    {
        const char *cursor = cmd + 16;
        rt_uint32_t address;
        rt_uint8_t data[32];
        rt_size_t length = 0U;
        rt_err_t result;

        if (app_bringup_parse_u32_token(&cursor, &address) == RT_FALSE ||
            app_bringup_parse_hex_bytes(cursor, data, sizeof(data), &length) == RT_FALSE ||
            address >= STORAGE_AT24C128_SIZE_BYTES ||
            length > (STORAGE_AT24C128_SIZE_BYTES - address))
        {
            rt_kprintf("[WARN] eeprom: usage eeprom writehex <addr> <hexstring up to 64 chars>\r\n");
            return RT_TRUE;
        }

        result = storage_at24c128_write((rt_uint16_t)address, data, length);
        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] eeprom: writehex ok addr=0x%04x len=%u\r\n", address, (rt_uint32_t)length);
        }
        else
        {
            rt_kprintf("[WARN] eeprom: writehex failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strncmp(cmd, "eeprom test", 11) == 0)
    {
        const char *cursor = cmd + 11;
        rt_uint32_t address = 0x3FC0U;
        rt_uint8_t tx[32];
        rt_uint8_t rx[32];
        rt_size_t i;
        rt_err_t result;

        if (*app_bringup_skip_spaces(cursor) != '\0' &&
            app_bringup_parse_u32_token(&cursor, &address) == RT_FALSE)
        {
            rt_kprintf("[WARN] eeprom: usage eeprom test [addr]\r\n");
            return RT_TRUE;
        }

        if (address > (STORAGE_AT24C128_SIZE_BYTES - sizeof(tx)))
        {
            rt_kprintf("[WARN] eeprom: test addr out of range, need 32 bytes\r\n");
            return RT_TRUE;
        }

        for (i = 0U; i < sizeof(tx); i++)
        {
            tx[i] = (rt_uint8_t)(0xA0U + i);
            rx[i] = 0U;
        }

        result = storage_at24c128_write((rt_uint16_t)address, tx, sizeof(tx));
        if (result == RT_EOK)
        {
            result = storage_at24c128_read((rt_uint16_t)address, rx, sizeof(rx));
        }

        if (result != RT_EOK)
        {
            rt_kprintf("[WARN] eeprom: test transfer failed %d\r\n", result);
            return RT_TRUE;
        }

        for (i = 0U; i < sizeof(tx); i++)
        {
            if (tx[i] != rx[i])
            {
                rt_kprintf("[WARN] eeprom: verify failed index=%u tx=0x%02x rx=0x%02x\r\n",
                           (rt_uint32_t)i,
                           tx[i],
                           rx[i]);
                return RT_TRUE;
            }
        }

        rt_kprintf("[INFO] eeprom: test ok addr=0x%04x len=%u\r\n",
                   address,
                   (rt_uint32_t)sizeof(tx));
        return RT_TRUE;
    }
    else if (strcmp(cmd, "w5500") == 0 || strcmp(cmd, "w5500 status") == 0)
    {
        net_w5500_status_t status;
        rt_err_t result = net_w5500_read_status(&status);

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] w5500: version=0x%02x phycfgr=0x%02x link=%s speed=%s duplex=%s int=%u\r\n",
                       status.version,
                       status.phycfgr,
                       net_w5500_link_name(status.link_up),
                       status.speed_100m ? "100M" : "10M",
                       status.full_duplex ? "full" : "half",
                       status.int_pin_level);
        }
        else
        {
            rt_kprintf("[WARN] w5500: status failed %d, check SPI1 PB3/PB4/PB5 PC13/PC15 and 3V3\r\n",
                       result);
        }
        return RT_TRUE;
    }
    else if (strcmp(cmd, "w5500 init") == 0)
    {
        rt_err_t result = net_w5500_init();

        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] w5500: init ok spi1=PB3/PB4/PB5 cs=PC13 int=PC14 rst=PC15\r\n");
        }
        else
        {
            rt_kprintf("[WARN] w5500: init failed %d, VERSIONR expected 0x04\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strncmp(cmd, "w5500 reg ", 10) == 0)
    {
        const char *cursor = cmd + 10;
        rt_uint32_t address;
        rt_uint8_t value = 0U;
        rt_err_t result;

        if (app_bringup_parse_u32_token(&cursor, &address) == RT_FALSE ||
            address > 0xFFFFU)
        {
            rt_kprintf("[WARN] w5500: usage w5500 reg <common_addr>\r\n");
            return RT_TRUE;
        }

        result = net_w5500_read_common((rt_uint16_t)address, &value);
        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] w5500: reg[0x%04x]=0x%02x\r\n", address, value);
        }
        else
        {
            rt_kprintf("[WARN] w5500: reg read failed %d\r\n", result);
        }
        return RT_TRUE;
    }
    else if (strncmp(cmd, "w5500 write ", 12) == 0)
    {
        const char *cursor = cmd + 12;
        rt_uint32_t address;
        rt_uint32_t value;
        rt_err_t result;

        if (app_bringup_parse_u32_token(&cursor, &address) == RT_FALSE ||
            app_bringup_parse_u32_token(&cursor, &value) == RT_FALSE ||
            address > 0xFFFFU || value > 0xFFU)
        {
            rt_kprintf("[WARN] w5500: usage w5500 write <common_addr> <byte>\r\n");
            return RT_TRUE;
        }

        result = net_w5500_write_common((rt_uint16_t)address, (rt_uint8_t)value);
        if (result == RT_EOK)
        {
            rt_kprintf("[INFO] w5500: reg[0x%04x] <= 0x%02x\r\n", address, value);
        }
        else
        {
            rt_kprintf("[WARN] w5500: reg write failed %d\r\n", result);
        }
        return RT_TRUE;
    }

    return RT_FALSE;
}

void app_bringup_poll(void)
{
    static rt_tick_t next_watch_tick = 0;
    rt_tick_t now;
    rt_uint32_t keys;

    if (key_watch_enabled == RT_FALSE)
    {
        return;
    }

    now = rt_tick_get();
    if ((rt_base_t)(now - next_watch_tick) < 0)
    {
        return;
    }

    next_watch_tick = now + rt_tick_from_millisecond(KEY_WATCH_PERIOD_MS);
    keys = board_interaction_key_read();
    if (keys != key_watch_last_state)
    {
        key_watch_last_state = keys;
        app_bringup_print_key_state("key watch", keys);
    }
}
